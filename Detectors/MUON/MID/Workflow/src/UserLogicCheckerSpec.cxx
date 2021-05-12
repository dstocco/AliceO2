// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   MID/Workflow/src/UserLogicCheckerSpec.cxx
/// \brief  Data processor spec for MID user logic checker device
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   02 November 2020

#include "MIDWorkflow/UserLogicCheckerSpec.h"

#include <chrono>
#include <fstream>
#include <vector>
#include "Framework/ConfigParamRegistry.h"
#include "Framework/ControlService.h"
#include "Framework/Logger.h"
#include "Framework/ParallelContext.h"
#include "Framework/Task.h"
#include "Framework/WorkflowSpec.h"
#include "MIDRaw/CrateMasks.h"
#include "MIDQC/UserLogicChecker.h"

namespace o2
{
namespace mid
{

class UserLogicCheckerDeviceDPL
{
 public:
  UserLogicCheckerDeviceDPL(const std::vector<int>& bareSubSpecs, const std::vector<int>& ulSubSpecs) : mBareSubSpecs(bareSubSpecs), mULSubSpecs(ulSubSpecs) {}

  void init(o2::framework::InitContext& ic)
  {

    auto outFilename = ic.options().get<std::string>("mid-checker-outfile");

    mOutFile.open(outFilename.c_str());

    mMaxOutFileSize = ic.options().get<long int>("mid-checker-max-file-size");

    auto stop = [this]() {
      std::stringstream ss;
      if (mOutSize >= mMaxOutFileSize) {
        ss << "Too many errors found (output file size " << mOutSize << " bytes): abort check!\n";
      }
      LOG(INFO) << "Summary:";
      auto summary = mChecker.getSummary();
      mOutFile << summary << "\n";
      LOG(INFO) << summary;
    };
    ic.services().get<o2::framework::CallbackService>().set(o2::framework::CallbackService::Id::Stop, stop);
  }

  void run(o2::framework::ProcessingContext& pc)
  {
    if (mOutSize >= mMaxOutFileSize) {
      // Abort checking: too many errors found
      return;
    }

    auto tStart = std::chrono::high_resolution_clock::now();

    std::vector<ROBoard> bareDecoded, ulDecoded;
    std::vector<ROFRecord> bareRofs, ulRofs;
    for (auto& subSpec : mBareSubSpecs) {
      auto offset = bareDecoded.size();
      const auto tmpDecoded = pc.inputs().get<gsl::span<ROBoard>>(fmt::format("mid_decoded_bare_{}", subSpec));
      bareDecoded.insert(bareDecoded.end(), tmpDecoded.begin(), tmpDecoded.end());
      const auto tmpRofs = pc.inputs().get<gsl::span<ROFRecord>>(fmt::format("mid_decoded_bare_rof_{}", subSpec));
      for (auto& rof : tmpRofs) {
        bareRofs.emplace_back(rof.interactionRecord, rof.eventType, rof.firstEntry + offset, rof.nEntries);
      }
    }
    for (auto& subSpec : mULSubSpecs) {
      auto offset = ulDecoded.size();
      const auto tmpDecoded = pc.inputs().get<gsl::span<ROBoard>>(fmt::format("mid_decoded_ul_{}", subSpec));
      ulDecoded.insert(ulDecoded.end(), tmpDecoded.begin(), tmpDecoded.end());
      const auto tmpRofs = pc.inputs().get<gsl::span<ROFRecord>>(fmt::format("mid_decoded_ul_rof_{}", subSpec));
      for (auto& rof : tmpRofs) {
        ulRofs.emplace_back(rof.interactionRecord, rof.eventType, rof.firstEntry + offset, rof.nEntries);
      }
    }

    auto tAlgoStart = std::chrono::high_resolution_clock::now();

    if (!mChecker.process(bareDecoded, bareRofs, ulDecoded, ulRofs)) {
      mOutFile << mChecker.getDebugMessage() << "\n";
      mOutSize += mChecker.getDebugMessage().size();
    }
    mTimerAlgo += std::chrono::high_resolution_clock::now() - tAlgoStart;
    mTimer += std::chrono::high_resolution_clock::now() - tStart;
  }

 private:
  UserLogicChecker mChecker{};                 ///< User logic checker
  std::vector<int> mBareSubSpecs{};            ///< Bare sub specs
  std::vector<int> mULSubSpecs{};              ///< UL sub specs
  long int mMaxOutFileSize{10000000};          ///< Maximum output file size
  long int mOutSize{0};                        ///< Output file size
  std::ofstream mOutFile{};                    ///< Output file
  std::chrono::duration<double> mTimer{0};     ///< full timer
  std::chrono::duration<double> mTimerAlgo{0}; ///< algorithm timer
};

framework::DataProcessorSpec getUserLogicCheckerSpec(const std::vector<int>& bareSubSpecs, const std::vector<int>& ulSubSpecs)
{

  std::vector<o2::framework::InputSpec> inputSpecs;
  for (auto& subSpec : bareSubSpecs) {
    inputSpecs.emplace_back(fmt::format("mid_decoded_bare_{}", subSpec), header::gDataOriginMID, "DECODED", subSpec);
    inputSpecs.emplace_back(fmt::format("mid_decoded_bare_rof_{}", subSpec), header::gDataOriginMID, "DECODEDROF", subSpec);
  }

  for (auto& subSpec : ulSubSpecs) {
    inputSpecs.emplace_back(fmt::format("mid_decoded_ul_{}", subSpec), header::gDataOriginMID, "DECODED", subSpec);
    inputSpecs.emplace_back(fmt::format("mid_decoded_ul_rof_{}", subSpec), header::gDataOriginMID, "DECODEDROF", subSpec);
  }

  return o2::framework::DataProcessorSpec{
    "MIDUserLogicChecker",
    {inputSpecs},
    {o2::framework::Outputs{}},
    o2::framework::AlgorithmSpec{o2::framework::adaptFromTask<UserLogicCheckerDeviceDPL>(bareSubSpecs, ulSubSpecs)},
    o2::framework::Options{{"mid-checker-max-file-size", o2::framework::VariantType::Int64, static_cast<long int>(10000000), {"Maximum output file size"}}, {"mid-checker-outfile", o2::framework::VariantType::String, "ul_checker_out.txt", {"Checker output file"}}}};
}

} // namespace mid
} // namespace o2
