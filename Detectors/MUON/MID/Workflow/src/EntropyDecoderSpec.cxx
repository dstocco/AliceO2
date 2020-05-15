// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// @file   EntropyDecoderSpec.cxx

#include "MIDWorkflow/EntropyDecoderSpec.h"

#include <vector>
#include "Framework/ControlService.h"
#include "Framework/ConfigParamRegistry.h"
#include "DetectorsBase/CTFCoderBase.h"
#include "DetectorsCommonDataFormats/NameConf.h"

using namespace o2::framework;

namespace o2
{
namespace mid
{

EntropyDecoderSpec::EntropyDecoderSpec()
{
  mTimer.Stop();
  mTimer.Reset();
}

void EntropyDecoderSpec::init(o2::framework::InitContext& ic)
{
  std::string dictPath = ic.options().get<std::string>("ctf-dict");
  if (!dictPath.empty() && dictPath != "none") {
    mCTFCoder.createCoders(dictPath, o2::ctf::CTFCoderBase::OpType::Decoder);
  }
}

void EntropyDecoderSpec::run(ProcessingContext& pc)
{
  auto cput = mTimer.CpuTime();
  mTimer.Start(false);

  auto buff = pc.inputs().get<gsl::span<o2::ctf::BufferType>>("ctf");

  std::vector<o2::mid::ROFRecord> rofs;
  std::vector<o2::mid::ColumnData> cols;

  // since the buff is const, we cannot use EncodedBlocks::relocate directly, instead we wrap its data to another flat object
  const auto ctfImage = o2::mid::CTF::getImage(buff.data());
  mCTFCoder.decode(ctfImage, rofs, cols);

  auto first = rofs.begin(), last = rofs.begin(), rofIt = rofs.begin();
  for (auto end = rofs.end(); rofIt != end; ++rofIt) {
    if (rofIt->firstEntry == 0 && first != rofIt) {
      std::vector<o2::mid::ROFRecord> subRofs(first, rofIt);
      std::vector<o2::mid::ColumnData> subCols(cols.begin() + first->firstEntry, cols.begin() + last->firstEntry + last->nEntries);
      o2::header::DataHeader::SubSpecificationType subSpec = static_cast<o2::header::DataHeader::SubSpecificationType>(first->eventType);
      pc.outputs().snapshot(Output{o2::header::gDataOriginMID, "DATA", subSpec, Lifetime::Timeframe}, subCols);
      pc.outputs().snapshot(Output{o2::header::gDataOriginMID, "DATAROF", subSpec, Lifetime::Timeframe}, subRofs);
      first = rofIt;
    }
    last = rofIt;
  }
  if (!rofs.empty()) {
    std::vector<o2::mid::ROFRecord> subRofs(first, rofIt);
    std::vector<o2::mid::ColumnData> subCols(cols.begin() + first->firstEntry, cols.begin() + last->firstEntry + last->nEntries);
    o2::header::DataHeader::SubSpecificationType subSpec = static_cast<o2::header::DataHeader::SubSpecificationType>(first->eventType);
    pc.outputs().snapshot(Output{o2::header::gDataOriginMID, "DATA", subSpec, Lifetime::Timeframe}, subCols);
    pc.outputs().snapshot(Output{o2::header::gDataOriginMID, "DATAROF", subSpec, Lifetime::Timeframe}, subRofs);
  }

  mTimer.Stop();
  LOG(INFO) << "Decoded " << cols.size() << " MID columns in " << rofs.size() << " ROFRecords in " << mTimer.CpuTime() - cput << " s";
}

void EntropyDecoderSpec::endOfStream(EndOfStreamContext& ec)
{
  LOGF(INFO, "MID Entropy Decoding total timing: Cpu: %.3e Real: %.3e s in %d slots",
       mTimer.CpuTime(), mTimer.RealTime(), mTimer.Counter() - 1);
}

DataProcessorSpec getEntropyDecoderSpec()
{
  std::vector<OutputSpec> outputs;
  for (o2::header::DataHeader::SubSpecificationType subSpec = 0; subSpec < 3; ++subSpec) {
    outputs.emplace_back(OutputSpec{header::gDataOriginMID, "DATA", subSpec});
    outputs.emplace_back(OutputSpec{header::gDataOriginMID, "DATAROF", subSpec});
  }

  return DataProcessorSpec{
    "mid-entropy-decoder",
    Inputs{InputSpec{"ctf", "MID", "CTFDATA", 0, Lifetime::Timeframe}},
    outputs,
    AlgorithmSpec{adaptFromTask<EntropyDecoderSpec>()},
    Options{{"ctf-dict", VariantType::String, o2::base::NameConf::getCTFDictFileName(), {"File of CTF decoding dictionary"}}}};
}

} // namespace mid
} // namespace o2
