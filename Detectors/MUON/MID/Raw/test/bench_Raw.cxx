// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   MID/Raw/test/bench_Raw.cxx
/// \brief  Benchmark MID raw data decoder
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   17 March 2018

#include "benchmark/benchmark.h"
#include <algorithm>
#include <random>
#include <vector>
#include "Framework/Logger.h"
#include "CommonDataFormat/InteractionRecord.h"
#include "DetectorsRaw/RawFileReader.h"
#include "DPLUtils/RawParser.h"
#include "DataFormatsMID/ColumnData.h"
#include "DataFormatsMID/ROFRecord.h"
#include "MIDBase/DetectorParameters.h"
#include "MIDRaw/Decoder.h"
#include "MIDRaw/Encoder.h"
#include "MIDRaw/LinkDecoder.h"

o2::mid::ColumnData getColData(uint8_t deId, uint8_t columnId, uint16_t nbp = 0, uint16_t bp1 = 0, uint16_t bp2 = 0, uint16_t bp3 = 0, uint16_t bp4 = 0)
{
  o2::mid::ColumnData col;
  col.deId = deId;
  col.columnId = columnId;
  col.setNonBendPattern(nbp);
  col.setBendPattern(bp1, 0);
  col.setBendPattern(bp2, 1);
  col.setBendPattern(bp3, 2);
  col.setBendPattern(bp4, 3);
  return col;
}

std::vector<uint8_t> generateTestData(size_t nTF, size_t nDataInTF, size_t nColDataInEvent, size_t nLinks = o2::mid::crateparams::sNGBTs)
{
  std::vector<o2::mid::ColumnData> colData;
  colData.reserve(nColDataInEvent);
  int maxNcols = 7;
  int nDEs = nColDataInEvent / maxNcols;
  int nColLast = nColDataInEvent % maxNcols;
  if (nColLast > 0) {
    ++nDEs;
  }

  // Generate data
  for (int ide = 0; ide < nDEs; ++ide) {
    int nCol = (ide == nDEs - 1) ? nColLast : maxNcols;
    auto rpcLine = o2::mid::detparams::getRPCLine(ide);
    int firstCol = (rpcLine < 3 || rpcLine > 5) ? 0 : 1;
    for (int icol = firstCol; icol < nCol; ++icol) {
      colData.emplace_back(getColData(ide, icol, 0xFF00, 0xFF0));
    }
  }

  auto severity = fair::Logger::GetConsoleSeverity();
  fair::Logger::SetConsoleSeverity(fair::Severity::warning);
  o2::mid::Encoder encoder;
  encoder.init();
  std::string tmpConfigFilename = "tmp_MIDConfig.cfg";
  encoder.getWriter().writeConfFile("MID", "RAWDATA", tmpConfigFilename.c_str(), false);
  // Fill TF
  for (size_t itf = 0; itf < nTF; ++itf) {
    for (int ilocal = 0; ilocal < nDataInTF; ++ilocal) {
      o2::InteractionRecord ir(ilocal, itf);
      encoder.process(colData, ir, o2::mid::EventType::Standard);
    }
  }
  encoder.finalize();

  o2::raw::RawFileReader rawReader(tmpConfigFilename.c_str());
  rawReader.init();
  size_t nActiveLinks = rawReader.getNLinks() < nLinks ? rawReader.getNLinks() : nLinks;
  std::vector<char> buffer;
  for (size_t itf = 0; itf < rawReader.getNTimeFrames(); ++itf) {
    rawReader.setNextTFToRead(itf);
    for (size_t ilink = 0; ilink < nActiveLinks; ++ilink) {
      auto& link = rawReader.getLink(ilink);
      auto tfsz = link.getNextTFSize();
      if (!tfsz) {
        continue;
      }
      std::vector<char> linkBuffer(tfsz);
      link.readNextTF(linkBuffer.data());
      buffer.insert(buffer.end(), linkBuffer.begin(), linkBuffer.end());
    }
  }
  fair::Logger::SetConsoleSeverity(severity);

  std::remove("MID.raw");
  std::remove(tmpConfigFilename.c_str());

  std::vector<uint8_t> data(buffer.size());
  memcpy(data.data(), buffer.data(), buffer.size());

  return data;
}

std::vector<o2::mid::ROFRecord> generateRofs(size_t nRofs = 2500)
{
  std::uniform_int_distribution<uint16_t> distBC(0, o2::constants::lhc::LHCMaxBunches);
  std::uniform_int_distribution<uint32_t> distOrbit(0, 0xFFFFFFFF);
  std::poisson_distribution<> distNcol(2);

  std::random_device rd;
  std::mt19937 mt(rd());
  std::vector<o2::mid::ROFRecord> rofs;
  for (size_t irof = 0; irof < nRofs; ++irof) {
    std::vector<o2::InteractionRecord> ir(distBC(mt), distOrbit(mt));
    auto nCol = distNcol(mt);
    for (size_t icol = 0; icol < nCol; ++icol) {
      rofs.push_back({{distBC(mt), distOrbit(mt)}, o2::mid::EventType::Standard, irof, 1});
    }
  }
  // std::shuffle(rofs.begin(), rofs.end(), mt);

  return rofs;
}

static void BM_Decoder(benchmark::State& state)
{
  o2::mid::Decoder decoder;

  int nTF = state.range(0);
  int nEventPerTF = state.range(1);
  int nFiredPerEvent = state.range(2);
  double num{0};

  auto inputData = generateTestData(nTF, nEventPerTF, nFiredPerEvent);

  for (auto _ : state) {
    decoder.process(inputData);
    ++num;
  }

  state.counters["num"] = benchmark::Counter(num, benchmark::Counter::kIsRate);
}

static void BM_LinkDecoder(benchmark::State& state)
{
  auto decoder = o2::mid::createLinkDecoder(0);

  int nTF = state.range(0);
  int nEventPerTF = state.range(1);
  int nFiredPerEvent = state.range(2);
  double num{0};

  auto inputData = generateTestData(nTF, nEventPerTF, nFiredPerEvent, 1);
  std::vector<o2::mid::ROBoard> data;
  std::vector<o2::mid::ROFRecord> rofs;

  for (auto _ : state) {
    data.clear();
    rofs.clear();
    o2::framework::RawParser parser(inputData.data(), inputData.size());
    for (auto it = parser.begin(), end = parser.end(); it != end; ++it) {
      if (it.size() == 0) {
        continue;
      }
      auto* rdhPtr = it.template get_if<o2::header::RAWDataHeader>();
      gsl::span<const uint8_t> payload(it.data(), it.size());
      decoder->process(payload, *rdhPtr, data, rofs);
    }
    ++num;
  }

  state.counters["num"] = benchmark::Counter(num, benchmark::Counter::kIsRate);
}

static void BM_OrderMapIdx(benchmark::State& state)
{
  auto rofs = generateRofs();
  std::map<uint64_t, std::vector<size_t>> eventIndexes;
  std::vector<o2::mid::ROFRecord> orderedRofs;
  size_t num{0};
  for (auto _ : state) {
    for (auto rofIt = rofs.begin(); rofIt != rofs.end(); ++rofIt) {
      eventIndexes[rofIt->interactionRecord.toLong()].emplace_back(rofIt - rofs.begin());
    }
    for (auto& item : eventIndexes) {
      orderedRofs.emplace_back(rofs[item.second.front()]);
    }
    eventIndexes.clear();
    orderedRofs.clear();
    ++num;
  }
  state.counters["num"] = benchmark::Counter(num, benchmark::Counter::kIsRate);
}

static void BM_OrderMapIR(benchmark::State& state)
{
  auto rofs = generateRofs();
  std::map<o2::InteractionRecord, std::vector<size_t>> eventIndexes;
  std::vector<o2::mid::ROFRecord> orderedRofs;
  size_t num{0};
  for (auto _ : state) {
    for (auto rofIt = rofs.begin(); rofIt != rofs.end(); ++rofIt) {
      eventIndexes[rofIt->interactionRecord].emplace_back(rofIt - rofs.begin());
    }
    for (auto& item : eventIndexes) {
      orderedRofs.emplace_back(rofs[item.second.front()]);
    }
    eventIndexes.clear();
    orderedRofs.clear();
    ++num;
  }
  state.counters["num"] = benchmark::Counter(num, benchmark::Counter::kIsRate);
}

static void BM_OrderVector(benchmark::State& state)
{
  auto rofs = generateRofs();
  std::unordered_map<uint64_t, std::vector<size_t>> eventIndexes;
  std::vector<o2::mid::ROFRecord> orderedRofs;
  size_t num{0};
  for (auto _ : state) {
    for (auto rofIt = rofs.begin(); rofIt != rofs.end(); ++rofIt) {
      eventIndexes[rofIt->interactionRecord.toLong()].emplace_back(rofIt - rofs.begin());
    }
    for (auto& item : eventIndexes) {
      orderedRofs.emplace_back(rofs[item.second.front()]);
    }
    std::sort(orderedRofs.begin(), orderedRofs.end(), [](const o2::mid::ROFRecord& first, const o2::mid::ROFRecord& second) { return first.interactionRecord < second.interactionRecord; });
    eventIndexes.clear();
    orderedRofs.clear();
    ++num;
  }
  state.counters["num"] = benchmark::Counter(num, benchmark::Counter::kIsRate);
}

static void CustomArguments(benchmark::internal::Benchmark* bench)
{
  // One per event
  bench->Args({1, 1, 1});
  bench->Args({10, 1, 1});
  // One large data
  bench->Args({1, 1, 70 * 4});
  // Many small data
  bench->Args({1, 100, 4});
}

BENCHMARK(BM_LinkDecoder)->Apply(CustomArguments)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_Decoder)->Apply(CustomArguments)->Unit(benchmark::kNanosecond);

BENCHMARK(BM_OrderMapIdx);
BENCHMARK(BM_OrderMapIR);
BENCHMARK(BM_OrderVector);

BENCHMARK_MAIN();
