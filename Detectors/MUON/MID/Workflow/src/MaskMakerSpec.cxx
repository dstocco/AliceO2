// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   MID/Workflow/src/MaskMakerSpec.cxx
/// \brief  Processor to compute the masks
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   12 may 2021

#include "MIDWorkflow/MaskMakerSpec.h"

#include <array>
#include <vector>
#include <chrono>
#include <gsl/gsl>
#include "Framework/ConfigParamRegistry.h"
#include "Framework/ControlService.h"
#include "Framework/InputSpec.h"
#include "Framework/Logger.h"
#include "Framework/Task.h"
#include "DataFormatsMID/ColumnData.h"
#include "DataFormatsMID/ROFRecord.h"
#include "MIDFiltering/ChannelMasks.h"
#include "MIDFiltering/ChannelScalers.h"
#include "MIDFiltering/FetToDead.h"
#include "MIDFiltering/MaskMaker.h"

namespace of = o2::framework;

namespace o2
{
namespace mid
{

class MaskMakerDeviceDPL
{
 public:
  void init(o2::framework::InitContext& ic)
  {
    mThreshold = ic.options().get<double>("mid-mask-threshold");
    mNReset = ic.options().get<int>("mid-mask-reset");

    auto stop = [this]() {
      double scaleFactor = (mCounter == 0) ? 0 : 1.e6 / mCounter;
      LOG(INFO) << "Processing time / " << mCounter << " events: full: " << mTimer.count() * scaleFactor << " us  mask maker: " << mTimerMaskMaker.count() * scaleFactor << " us";
      processScalers();
      printSummary();
    };
    ic.services().get<of::CallbackService>().set(of::CallbackService::Id::Stop, stop);
  }

  void printSummary()
  {
    std::string name = "calib";
    for (auto& masks : mMasks) {
      auto maskVec = masks.getMasks();
      if (maskVec.empty()) {
        LOG(INFO) << "No problematic digit found in " << name << " events";
      } else {
        LOG(INFO) << "Problematic digits found in " << name << " events. Corresponding masks:";
        for (auto& mask : maskVec) {
          LOG(INFO) << mask;
        }
      }
      name = "FET";
    }
  }

  bool processScalers()
  {
    bool isOk = true;
    for (size_t itype = 0; itype < 2; ++itype) {
      auto masks = o2::mid::makeMasks(mScalers[itype], mCounterSinceReset, mThreshold);
      if (!(masks == mMasks[itype])) {
        isOk = false;
        mMasks[itype] = masks;
      }
    }
    return isOk;
  }

  // std::cout << "\nCorresponding boards masks:" << std::endl;
  // o2::mid::ColumnDataToLocalBoard colToBoard;
  // colToBoard.setDebugMode(true);
  // colToBoard.process(maskVec);
  // for (auto& mapIt : colToBoard.getData()) {
  //   for (auto& board : mapIt.second) {
  //     std::cout << board << std::endl;
  //   }
  // }

  void run(o2::framework::ProcessingContext& pc)
  {
    auto tStart = std::chrono::high_resolution_clock::now();

    auto calibData = pc.inputs().get<gsl::span<ColumnData>>("mid_calib");
    auto calibDataRof = pc.inputs().get<gsl::span<ROFRecord>>("mid_calib_rof");

    auto fetData = pc.inputs().get<gsl::span<ColumnData>>("mid_fet");

    unsigned long nEvents = calibDataRof.size();
    if (nEvents == 0) {
      return;
    }

    auto tAlgoStart = std::chrono::high_resolution_clock::now();

    for (auto& col : calibData) {
      mScalers[0].count(col);
    }

    mFetToDead.process(fetData);
    for (auto& col : mFetToDead.getData()) {
      mScalers[1].count(col);
    }

    mCounter += nEvents;

    if (mCounterSinceReset >= mNReset) {
      if (!processScalers()) {
        printSummary();
      }
      mCounterSinceReset = 0;
      for (auto& scaler : mScalers) {
        scaler.reset();
      }
    }

    mTimerMaskMaker += std::chrono::high_resolution_clock::now() - tAlgoStart;

    mTimer += std::chrono::high_resolution_clock::now() - tStart;
  }

 private:
  FetToDead mFetToDead{};                           ///< FET to dead channels converter
  std::array<ChannelScalers, 2> mScalers{};         ///< Array fo channel scalers
  std::array<ChannelMasks, 2> mMasks{};             ///< Output masks
  std::chrono::duration<double> mTimer{0};          ///< full timer
  std::chrono::duration<double> mTimerMaskMaker{0}; ///< mask maker timer
  unsigned long mCounter{0};                        ///< Total number of processed events
  unsigned long mCounterSinceReset{0};              ///< Total number of processed events since last reset
  double mThreshold{0.9};                           ///< Occupancy threshold for producing a mask
  int mNReset{1};                                   ///< Number of calibration events to be tested before checking the scalers
};

framework::DataProcessorSpec getMaskMakerSpec()
{
  std::vector<of::InputSpec> inputSpecs{
    of::InputSpec{"mid_calib", header::gDataOriginMID, "DATA", 1},
    of::InputSpec{"mid_calib_rof", header::gDataOriginMID, "DATAROF", 1},
    of::InputSpec{"mid_fet", header::gDataOriginMID, "DATA", 2}};

  return of::DataProcessorSpec{
    "MIDMaskMaker",
    {inputSpecs},
    {},
    of::AlgorithmSpec{of::adaptFromTask<o2::mid::MaskMakerDeviceDPL>()},
    of::Options{{"mid-mask-threshold", of::VariantType::Double, 0.9, {"Tolerated occupancy before producing a map"}}, {"mid-mask-reset", of::VariantType::Int, 100, {"Number of calibration events to be checked before resetting the scalers"}}}};
}
} // namespace mid
} // namespace o2