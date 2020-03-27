// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   MID/Workflow/src/RawGBTCheckerSpec.cxx
/// \brief  Data processor spec for MID raw bare checker device
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   06 April 2020

#include "MIDWorkflow/RawCheckerSpec.h"

#include <fstream>
#include <chrono>
#include "Framework/ConfigParamRegistry.h"
#include "Framework/ControlService.h"
#include "Framework/Logger.h"
#include "Framework/ParallelContext.h"
#include "Framework/Task.h"
#include "Framework/WorkflowSpec.h"
#include "MIDRaw/CrateMasks.h"
#include "MIDRaw/FEEIdConfig.h"
#include "MIDQC/RawDataChecker.h"

namespace o2
{
namespace mid
{

template <typename RAWCHECKER>
std::string getSummary(const RAWCHECKER& checker, size_t maxErrors)
{
  std::stringstream ss;
  if (checker.getNEventsFaulty() >= maxErrors) {
    ss << "Too many errors found (" << checker.getNEventsFaulty() << "): abort check!\n";
  }
  ss << "Number of busy raised: " << checker.getNBusyRaised() << "\n";
  ss << "Fraction of faulty events: " << checker.getNEventsFaulty() << " / " << checker.getNEventsProcessed() << " = " << static_cast<double>(checker.getNEventsFaulty()) / ((checker.getNEventsProcessed() > 0) ? static_cast<double>(checker.getNEventsProcessed()) : 1.);
  return ss.str();
}

template <typename RAWCHECKER>
class RawCheckerDeviceDPL
{
 public:
  RawCheckerDeviceDPL<RAWCHECKER>(std::string outFilename, std::string feeIdConfigFilename, std::string crateMasksFilename, size_t maxErrors) : mOutFilename(outFilename), mFeeIdConfigFilename(feeIdConfigFilename), mCrateMasksFilename(crateMasksFilename), mMaxErrors(maxErrors) {}

  void init(o2::framework::InitContext& ic)
  {

    o2::mid::CrateMasks crateMasks;
    if (!mCrateMasksFilename.empty()) {
      crateMasks = o2::mid::CrateMasks(mCrateMasksFilename.c_str());
    }
    if constexpr (std::is_same_v<RAWCHECKER, RawDataChecker>) {
      mChecker.init(crateMasks);
      if (mOutFilename.empty()) {
        mOutFilename = "raw_checker_out.txt";
      }
    } else {
      o2::mid::FEEIdConfig feeIdConfig;
      if (!mFeeIdConfigFilename.empty()) {
        feeIdConfig = o2::mid::FEEIdConfig(mFeeIdConfigFilename.c_str());
      }
      std::vector<uint32_t> gbtIds = feeIdConfig.getConfiguredGBTIds();
      auto idx = ic.services().get<o2::framework::ParallelContext>().index1D();
      auto feeId = feeIdConfig.getFeeId(gbtIds[idx]);
      mChecker.init(feeId, crateMasks.getMask(feeId));
      if (mOutFilename.empty()) {
        std::stringstream ss;
        ss << "raw_checker_out_GBT_" << feeId << ".txt";
        mOutFilename = ss.str();
      }
    }

    mOutFile.open(mOutFilename.c_str());

    auto stop = [this]() {
      bool hasProcessed = (mChecker.getNEventsProcessed() > 0);
      double scaleFactor = (mChecker.getNEventsProcessed() > 0) ? 1.e6 / static_cast<double>(mChecker.getNEventsProcessed()) : 0.;
      LOG(INFO) << "Processing time / " << mChecker.getNEventsProcessed() << " BCs: full: " << mTimer.count() * scaleFactor << " us  checker: " << mTimerAlgo.count() * scaleFactor << " us";
      std::string summary = getSummary(mChecker, mMaxErrors);
      mOutFile << summary << "\n";
      LOG(INFO) << summary;
    };
    ic.services().get<o2::framework::CallbackService>().set(o2::framework::CallbackService::Id::Stop, stop);
  }

  void run(o2::framework::ProcessingContext& pc)
  {
    if (mChecker.getNEventsFaulty() >= mMaxErrors) {
      // Abort checking: too many errors found
      return;
    }

    auto tStart = std::chrono::high_resolution_clock::now();

    auto msg = pc.inputs().get("mid_decoded");
    gsl::span<const LocalBoardRO> data = o2::framework::DataRefUtils::as<const LocalBoardRO>(msg);

    auto msgROF = pc.inputs().get("mid_decoded_rof");
    gsl::span<const ROFRecord> inROFRecords = o2::framework::DataRefUtils::as<const ROFRecord>(msgROF);

    std::vector<ROFRecord> dummy;
    auto tAlgoStart = std::chrono::high_resolution_clock::now();
    if (!mChecker.process(data, inROFRecords, dummy)) {
      mOutFile << mChecker.getDebugMessage() << "\n";
    }
    mTimerAlgo += std::chrono::high_resolution_clock::now() - tAlgoStart;
    mTimer += std::chrono::high_resolution_clock::now() - tStart;
  }

 private:
  RAWCHECKER mChecker{};                       ///< Raw data checker
  std::string mOutFilename{};                  ///< Output filename
  std::string mFeeIdConfigFilename{};          ///< FEE ID configuration file
  std::string mCrateMasksFilename{};           ///< Crate masks file
  size_t mMaxErrors{0};                        ///< Maximum number of errors
  std::ofstream mOutFile{};                    ///< Output file
  std::chrono::duration<double> mTimer{0};     ///< full timer
  std::chrono::duration<double> mTimerAlgo{0}; ///< algorithm timer
};

framework::DataProcessorSpec getRawCheckerSpec(const char* outFile, const char* feeIdConfigFile, const char* crateMasksFile, size_t maxErrors)
{
  std::vector<o2::framework::InputSpec> inputSpecs{o2::framework::InputSpec{"mid_decoded", header::gDataOriginMID, "DECODED", 0, o2::framework::Lifetime::Timeframe}, o2::framework::InputSpec{"mid_decoded_rof", header::gDataOriginMID, "DECODEDROF", 0, o2::framework::Lifetime::Timeframe}};

  std::string outFilename(outFile);
  std::string feeIdConfigFilename(feeIdConfigFile);
  std::string crateMasksFilename(crateMasksFile);

  return o2::framework::DataProcessorSpec{
    "MIDRawDataChecker",
    {inputSpecs},
    {o2::framework::Outputs{}},
    feeIdConfigFilename.empty()
      ? o2::framework::AlgorithmSpec{
          o2::framework::adaptFromTask<RawCheckerDeviceDPL<RawDataChecker>>(outFilename, feeIdConfigFilename, crateMasksFilename, maxErrors)}
      : o2::framework::adaptFromTask<RawCheckerDeviceDPL<GBTRawDataChecker>>(outFilename, feeIdConfigFilename, crateMasksFilename, maxErrors)};
}

o2::framework::WorkflowSpec getRawCheckerSpecs(const char* feeIdConfigFile, size_t maxErrors)
{
  FEEIdConfig feeIdConfig;
  std::string feeIdConfigFilename(feeIdConfigFile);
  if (!feeIdConfigFilename.empty()) {
    feeIdConfig = FEEIdConfig(feeIdConfigFilename.c_str());
  }
  std::vector<uint32_t> gbtIds = feeIdConfig.getConfiguredGBTIds();
  o2::framework::WorkflowSpec specs = parallel(getRawCheckerSpec("", feeIdConfigFile, "", maxErrors), gbtIds.size(), [feeIdConfig, gbtIds](o2::framework::DataProcessorSpec& spec, size_t index) {
    auto feeId = feeIdConfig.getFeeId(gbtIds[index]);
    o2::framework::DataSpecUtils::updateMatchingSubspec(spec.inputs[0], feeId);
    o2::framework::DataSpecUtils::updateMatchingSubspec(spec.inputs[1], feeId);
  });
  return specs;
}

} // namespace mid
} // namespace o2
