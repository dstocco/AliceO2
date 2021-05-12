// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// @file   EntropyEncoderSpec.cxx
/// @brief  Convert MID DATA to CTF (EncodedBlocks)
/// @author ruben.shahoyan@cern.ch

#include "MIDWorkflow/EntropyEncoderSpec.h"

#include <vector>
#include <unordered_map>
#include "Framework/ControlService.h"
#include "Framework/ConfigParamRegistry.h"
#include "Framework/DataRef.h"
#include "Framework/InputRecordWalker.h"
#include "Headers/DataHeader.h"
#include "DetectorsBase/CTFCoderBase.h"
#include "DetectorsCommonDataFormats/NameConf.h"

using namespace o2::framework;

namespace o2
{
namespace mid
{

EntropyEncoderSpec::EntropyEncoderSpec()
{
  mTimer.Stop();
  mTimer.Reset();
}

void EntropyEncoderSpec::init(o2::framework::InitContext& ic)
{
  std::string dictPath = ic.options().get<std::string>("ctf-dict");
  if (!dictPath.empty() && dictPath != "none") {
    mCTFCoder.createCoders(dictPath, o2::ctf::CTFCoderBase::OpType::Encoder);
  }
}

void EntropyEncoderSpec::run(ProcessingContext& pc)
{
  auto cput = mTimer.CpuTime();
  mTimer.Start(false);

  struct RofData {
    DataRef rofsRef;
    DataRef colsRef;
  };

  std::unordered_map<o2::header::DataHeader::SubSpecificationType, RofData> rofDataMap;

  std::vector<InputSpec>
    filter = {
      {"check", ConcreteDataTypeMatcher{header::gDataOriginMID, "DATA"}, Lifetime::Timeframe},
      {"check", ConcreteDataTypeMatcher{header::gDataOriginMID, "DATAROF"}, Lifetime::Timeframe},
    };
  for (auto const& inputRef : InputRecordWalker(pc.inputs(), filter)) {
    auto const* dh = framework::DataRefUtils::getHeader<o2::header::DataHeader*>(inputRef);
    if (DataRefUtils::match(inputRef, "cols")) {
      rofDataMap[dh->subSpecification].colsRef = inputRef;
    }
    if (DataRefUtils::match(inputRef, "rofs")) {
      rofDataMap[dh->subSpecification].rofsRef = inputRef;
    }
  }

  std::vector<o2::mid::ROFRecord> rofs;
  std::vector<o2::mid::ColumnData> cols;

  for (auto& item : rofDataMap) {
    size_t offset = cols.size();
    auto colsSpec = pc.inputs().get<gsl::span<o2::mid::ColumnData>>(item.second.colsRef);
    cols.insert(cols.end(), colsSpec.begin(), colsSpec.end());
    auto rofsSpec = pc.inputs().get<gsl::span<o2::mid::ROFRecord>>(item.second.rofsRef);
    std::cout << "subspec: " << item.first << " ncols: " << colsSpec.size() << " nRofs: " << rofsSpec.size() << "  offset: " << offset << std::endl; // TODO: REMOVE
    for (auto& rof : rofsSpec) {
      rofs.emplace_back(rof);
      std::cout << "First: " << rofs.back().firstEntry;
      rofs.back().firstEntry += offset;
      std::cout << " => " << rofs.back().firstEntry << std::endl;
    }
  }

  auto& buffer = pc.outputs().make<std::vector<o2::ctf::BufferType>>(Output{header::gDataOriginMID, "CTFDATA", 0, Lifetime::Timeframe});
  printf("Encoding\n"); // TODO: REMOVE
  mCTFCoder.encode(buffer, rofs, cols);
  printf("Getting\n");                // TODO: REMOVE
  auto eeb = CTF::get(buffer.data()); // cast to container pointer
  printf("Compactifying\n");          // TODO: REMOVE
  eeb->compactify();                  // eliminate unnecessary padding
  printf("Resizing\n");               // TODO: REMOVE
  buffer.resize(eeb->size());         // shrink buffer to strictly necessary size
  printf("Stopping\n");               // TODO: REMOVE
  //  eeb->print();
  mTimer.Stop();
  LOG(INFO) << "Created encoded data of size " << eeb->size() << " for MID in " << mTimer.CpuTime() - cput << " s";
}

void EntropyEncoderSpec::endOfStream(EndOfStreamContext& ec)
{
  LOGF(INFO, "MID Entropy Encoding total timing: Cpu: %.3e Real: %.3e s in %d slots",
       mTimer.CpuTime(), mTimer.RealTime(), mTimer.Counter() - 1);
}

DataProcessorSpec getEntropyEncoderSpec()
{
  std::vector<InputSpec> inputs;
  inputs.emplace_back("rofs", ConcreteDataTypeMatcher(header::gDataOriginMID, "DATAROF"), Lifetime::Timeframe);
  inputs.emplace_back("cols", ConcreteDataTypeMatcher(header::gDataOriginMID, "DATA"), Lifetime::Timeframe);

  return DataProcessorSpec{
    "mid-entropy-encoder",
    inputs,
    Outputs{{header::gDataOriginMID, "CTFDATA", 0, Lifetime::Timeframe}},
    AlgorithmSpec{adaptFromTask<EntropyEncoderSpec>()},
    Options{{"ctf-dict", VariantType::String, o2::base::NameConf::getCTFDictFileName(), {"File of CTF encoding dictionary"}}}};
}

} // namespace mid
} // namespace o2
