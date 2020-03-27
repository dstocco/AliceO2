// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   MID/Workflow/src/RawGBTDecoderSpec.cxx
/// \brief  Data processor spec for MID GBT raw decoder device
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   06 April 2020

#include "MIDWorkflow/RawGBTDecoderSpec.h"

#include <chrono>
#include <vector>
#include "DPLUtils/DPLRawParser.h"
#include "DetectorsRaw/RDHUtils.h"
#include "Framework/ConfigParamRegistry.h"
#include "Framework/ControlService.h"
#include "Framework/DataSpecUtils.h"
#include "Framework/Logger.h"
#include "Framework/Output.h"
#include "Framework/ParallelContext.h"
#include "Framework/Task.h"
#include "Headers/RDHAny.h"
#include "DetectorsRaw/RDHUtils.h"
#include "DataFormatsMID/ROFRecord.h"
#include "MIDRaw/CrateMasks.h"
#include "MIDRaw/FEEIdConfig.h"
#include "MIDRaw/GBTBareDecoder.h"
#include "MIDRaw/GBTUserLogicDecoder.h"
#include "MIDRaw/LocalBoardRO.h"

namespace o2
{
namespace mid
{

template <typename GBTDECODER>
class RawGBTDecoderDeviceDPL
{
 public:
  RawGBTDecoderDeviceDPL<GBTDECODER>(bool isDebugMode, std::string feeIdConfigFilename, std::string crateMasksFilename) : mIsDebugMode(isDebugMode), mFeeIdConfigFilename(feeIdConfigFilename), mCrateMasksFilename(crateMasksFilename) {}

  void init(o2::framework::InitContext& ic)
  {
    auto stop = [this]() {
      double scaleFactor = 1.e6 / mNROFs;
      LOG(INFO) << "Processing time / " << mNROFs << " ROFs: full: " << mTimer.count() * scaleFactor << " us  decoding: " << mTimerAlgo.count() * scaleFactor << " us";
    };
    ic.services().get<o2::framework::CallbackService>().set(o2::framework::CallbackService::Id::Stop, stop);

    FEEIdConfig feeIdConfig;
    if (!mFeeIdConfigFilename.empty()) {
      feeIdConfig = FEEIdConfig(mFeeIdConfigFilename.c_str());
    }
    CrateMasks crateMasks;
    if (!mCrateMasksFilename.empty()) {
      crateMasks = CrateMasks(mCrateMasksFilename.c_str());
    }

    std::vector<uint32_t> gbtIds = feeIdConfig.getConfiguredGBTIds();
    auto idx = ic.services().get<o2::framework::ParallelContext>().index1D();
    mFeeId = feeIdConfig.getFeeId(gbtIds[idx]);
    if constexpr (std::is_same_v<GBTDECODER, GBTBareDecoder>) {
      mDecoder.init(mFeeId, crateMasks.getMask(mFeeId), mIsDebugMode);
    } else {
      mDecoder.init(mFeeId, mIsDebugMode);
    }
  }

  void run(o2::framework::ProcessingContext& pc)
  {
    auto tStart = std::chrono::high_resolution_clock::now();

    o2::framework::DPLRawParser parser(pc.inputs());

    auto tAlgoStart = std::chrono::high_resolution_clock::now();

    for (auto it = parser.begin(), end = parser.end(); it != end; ++it) {
      auto const* rdhPtr = reinterpret_cast<const o2::header::RDHAny*>(it.raw());
      gsl::span<const uint8_t> payload(it.data(), it.size());
      mDecoder.process(payload, o2::raw::RDHUtils::getHeartBeatBC(rdhPtr), o2::raw::RDHUtils::getHeartBeatOrbit(rdhPtr), o2::raw::RDHUtils::getPageCounter(rdhPtr));
    }

    mTimerAlgo += std::chrono::high_resolution_clock::now() - tAlgoStart;

    pc.outputs().snapshot(o2::framework::Output{header::gDataOriginMID, "DECODED", mFeeId, o2::framework::Lifetime::Timeframe}, mDecoder.getData());
    pc.outputs().snapshot(o2::framework::Output{header::gDataOriginMID, "DECODEDROF", mFeeId, o2::framework::Lifetime::Timeframe}, mDecoder.getROFRecords());

    mTimer += std::chrono::high_resolution_clock::now() - tStart;
    mNROFs += mDecoder.getROFRecords().size();
    mDecoder.clear();
  }

 private:
  GBTDECODER mDecoder{};
  bool mIsDebugMode{false};
  std::string mFeeIdConfigFilename{};
  std::string mCrateMasksFilename{};
  uint16_t mFeeId{0};
  std::chrono::duration<double> mTimer{0};     ///< full timer
  std::chrono::duration<double> mTimerAlgo{0}; ///< algorithm timer
  unsigned int mNROFs{0};                      /// Total number of processed ROFs
};

framework::DataProcessorSpec getRawGBTDecoderSpec(bool isBare, bool isDebugMode, const char* feeIdConfigFile, const char* crateMasksFile)
{
  std::vector<o2::framework::InputSpec> inputSpecs{o2::framework::InputSpec{"mid_raw", header::gDataOriginMID, header::gDataDescriptionRawData, 0, o2::framework::Lifetime::Timeframe}};
  std::vector<o2::framework::OutputSpec> outputSpecs{o2::framework::OutputSpec{header::gDataOriginMID, "DECODED", 0, o2::framework::Lifetime::Timeframe}, o2::framework::OutputSpec{header::gDataOriginMID, "DECODEDROF", 0, o2::framework::Lifetime::Timeframe}};

  std::string feeIdConfigFilename(feeIdConfigFile);
  std::string crateMasksFilename(crateMasksFile);

  return o2::framework::DataProcessorSpec{
    "MIDRawGBTDecoder",
    {inputSpecs},
    {outputSpecs},
    isBare ? o2::framework::adaptFromTask<RawGBTDecoderDeviceDPL<GBTBareDecoder>>(isDebugMode, feeIdConfigFilename, crateMasksFilename) : o2::framework::adaptFromTask<RawGBTDecoderDeviceDPL<GBTUserLogicDecoder>>(isDebugMode, feeIdConfigFilename, crateMasksFilename)};
}

o2::framework::WorkflowSpec getRawGBTDecoderSpecs(bool isBare, bool isDebugMode, const char* feeIdConfigFile, const char* crateMasksFile)
{
  FEEIdConfig feeIdConfig;
  std::string feeIdConfigFilename(feeIdConfigFile);
  if (!feeIdConfigFilename.empty()) {
    feeIdConfig = FEEIdConfig(feeIdConfigFilename.c_str());
  }
  std::vector<uint32_t> gbtIds = feeIdConfig.getConfiguredGBTIds();
  o2::framework::WorkflowSpec specs = parallel(getRawGBTDecoderSpec(isBare, isDebugMode, feeIdConfigFile, crateMasksFile), gbtIds.size(), [feeIdConfig, gbtIds](o2::framework::DataProcessorSpec& spec, size_t index) {
    auto feeId = feeIdConfig.getFeeId(gbtIds[index]);
    o2::framework::DataSpecUtils::updateMatchingSubspec(spec.inputs[0], gbtIds[index]);
    o2::framework::DataSpecUtils::updateMatchingSubspec(spec.outputs[0], feeId);
    o2::framework::DataSpecUtils::updateMatchingSubspec(spec.outputs[1], feeId);
  });
  return specs;
}

} // namespace mid
} // namespace o2
