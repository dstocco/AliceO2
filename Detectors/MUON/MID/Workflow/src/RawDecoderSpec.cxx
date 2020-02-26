// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   MID/Workflow/src/RawDecoderSpec.cxx
/// \brief  Data processor spec for MID raw decoder device
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   26 February 2020

#include "MIDWorkflow/RawDecoderSpec.h"

#include <fstream>
#include <chrono>
#include "Framework/ConfigParamRegistry.h"
#include "Framework/ControlService.h"
#include "Framework/Logger.h"
#include "Framework/Output.h"
#include "Framework/Task.h"
#include "DataFormatsMID/ColumnData.h"
#include "MIDRaw/CrateParameters.h"
#include "MIDRaw/CrateFeeIdMapper.h"
#include "MIDRaw/CrateMasks.h"
#include "MIDRaw/CRUBareDecoder.h"
#include "MIDRaw/CRUUserLogicDecoder.h"
#include "MIDRaw/CRUUserLogicZeroDecoder.h"
#include "MIDRaw/RawFileReader.h"

namespace of = o2::framework;

namespace o2
{
namespace mid
{

template <typename T>
class RawDecoderDeviceDPL
{
 public:
  using TUnit = typename T::type;

  void init(of::InitContext& ic)
  {
    auto stop = [this]() {
      LOG(INFO) << "Capacities: ROFRecords: " << mDecoder.getROFRecords().capacity() << "  LocalBoards: " << mDecoder.getData().capacity();
      double scaleFactor = 1.e6 / mNROFs;
      LOG(INFO) << "Processing time / " << mNROFs << " ROFs: full: " << mTimer.count() * scaleFactor << " us  decoding: " << mTimerAlgo.count() * scaleFactor << " us";
    };
    ic.services().get<of::CallbackService>().set(of::CallbackService::Id::Stop, stop);
    if constexpr (std::is_same_v<T, CRUBareDecoder>) {
      o2::mid::CrateFeeIdMapper feeIdMapper = o2::mid::createDefaultCrateFeeIdMapper();
      auto feeIdMapperFilename = ic.options().get<std::string>("feeId-config-file");
      if (!feeIdMapperFilename.empty()) {
        feeIdMapper.load(feeIdMapperFilename.c_str());
      }
      auto crateMasks = o2::mid::createDefaultCrateMasks();
      auto crateMasksFilename = ic.options().get<std::string>("crate-masks-file");
      if (!crateMasksFilename.empty()) {
        crateMasks.load(crateMasksFilename.c_str());
      }
      mDecoder.init(feeIdMapper, crateMasks, true);
    }
  }

  void run(of::ProcessingContext& pc)
  {
    auto tStart = std::chrono::high_resolution_clock::now();

    auto msg = pc.inputs().get("mid_raw");
    auto buffer = of::DataRefUtils::as<const TUnit>(msg);

    auto tAlgoStart = std::chrono::high_resolution_clock::now();
    mDecoder.process(buffer);
    mTimerAlgo += std::chrono::high_resolution_clock::now() - tAlgoStart;

    pc.outputs().snapshot(of::Output{header::gDataOriginMID, "DECODED", 0, of::Lifetime::Timeframe}, mDecoder.getData());
    pc.outputs().snapshot(of::Output{header::gDataOriginMID, "DECODEDROF", 0, of::Lifetime::Timeframe}, mDecoder.getROFRecords());

    mTimer += std::chrono::high_resolution_clock::now() - tStart;
    mNROFs += mDecoder.getROFRecords().size();
  }

 private:
  T mDecoder{};
  std::chrono::duration<double> mTimer{0};     ///< full timer
  std::chrono::duration<double> mTimerAlgo{0}; ///< algorithm timer
  unsigned int mNROFs{0};                      /// Total number of processed ROFs
};

template <typename T>
framework::DataProcessorSpec getRawDecoderSpec()
{
  std::vector<of::InputSpec> inputSpecs{of::InputSpec{"mid_raw", of::ConcreteDataTypeMatcher{header::gDataOriginMID, header::gDataDescriptionRawData}, of::Lifetime::Timeframe}};
  std::vector<of::OutputSpec> outputSpecs{of::OutputSpec{header::gDataOriginMID, "DECODED", 0, of::Lifetime::Timeframe}, of::OutputSpec{header::gDataOriginMID, "DECODEDROF", 0, of::Lifetime::Timeframe}};

  return of::DataProcessorSpec{
    "MIDRawDecoder",
    {inputSpecs},
    {outputSpecs},
    of::AlgorithmSpec{of::adaptFromTask<o2::mid::RawDecoderDeviceDPL<T>>()},
    of::Options{
      {"feeId-config-file", of::VariantType::String, "", {"Filename with crate FEE ID correspondence"}},
      {"crate-masks-file", of::VariantType::String, "", {"Filename with crate masks"}}}};
}

template framework::DataProcessorSpec getRawDecoderSpec<CRUUserLogicDecoder>();
template framework::DataProcessorSpec getRawDecoderSpec<CRUUserLogicZeroDecoder>();
template framework::DataProcessorSpec getRawDecoderSpec<CRUBareDecoder>();
} // namespace mid
} // namespace o2
