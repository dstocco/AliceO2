// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   MID/Workflow/src/RawCheckerSpec.cxx
/// \brief  Data processor spec for MID raw checker device
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   26 February 2020

#include "MIDWorkflow/RawCheckerSpec.h"

#include <fstream>
#include <chrono>
#include "Framework/ConfigParamRegistry.h"
#include "Framework/ControlService.h"
#include "Framework/Logger.h"
#include "Framework/Task.h"
#include "MIDRaw/CrateMasks.h"
#include "MIDQC/CRUBareDataChecker.h"

namespace of = o2::framework;

namespace o2
{
namespace mid
{

class RawCheckerDeviceDPL
{
 public:
  void init(of::InitContext& ic)
  {

    auto filename = ic.options().get<std::string>("output-file");
    mOutFile.open(filename.c_str());

    auto crateMasks = o2::mid::createDefaultCrateMasks();
    auto crateMasksFilename = ic.options().get<std::string>("crate-masks-file");
    if (!crateMasksFilename.empty()) {
      crateMasks.load(crateMasksFilename.c_str());
    }
    mChecker.setCrateMasks(crateMasks);

    auto stop = [this]() {
      bool hasProcessed = (mChecker.getNBCsProcessed() > 0);
      double scaleFactor = 1.e6 / (hasProcessed ? mChecker.getNBCsProcessed() : 0);
      LOG(INFO) << "Processing time / " << mChecker.getNBCsProcessed() << " BCs: full: " << mTimer.count() * scaleFactor << " us  checker: " << mTimerAlgo.count() * scaleFactor << " us";
      std::stringstream summary;
      summary << "Fraction of faulty events: " << mChecker.getNBCsFaulty() << " / " << mChecker.getNBCsProcessed() << " = " << static_cast<double>(mChecker.getNBCsFaulty()) / (hasProcessed ? static_cast<double>(mChecker.getNBCsProcessed()) : 1.) << "\n";
      mOutFile << summary.str();
      LOG(INFO) << summary.str();
    };
    ic.services().get<of::CallbackService>().set(of::CallbackService::Id::Stop, stop);
  }

  void run(of::ProcessingContext& pc)
  {
    auto tStart = std::chrono::high_resolution_clock::now();

    auto msg = pc.inputs().get("mid_decoded");
    gsl::span<const LocalBoardRO> data = of::DataRefUtils::as<const LocalBoardRO>(msg);

    auto msgROF = pc.inputs().get("mid_decoded_rof");
    gsl::span<const ROFRecord> inROFRecords = of::DataRefUtils::as<const ROFRecord>(msgROF);

    std::vector<ROFRecord> dummy;
    auto tAlgoStart = std::chrono::high_resolution_clock::now();
    if (!mChecker.process(data, inROFRecords, dummy)) {
      mOutFile << mChecker.getDebugMessage() << "\n";
    }
    mTimerAlgo += std::chrono::high_resolution_clock::now() - tAlgoStart;

    mTimer += std::chrono::high_resolution_clock::now() - tStart;
  }

 private:
  CRUBareDataChecker mChecker{};               ///< Raw data checker
  std::ofstream mOutFile{};                    ///< Output file
  std::chrono::duration<double> mTimer{0};     ///< full timer
  std::chrono::duration<double> mTimerAlgo{0}; ///< algorithm timer
};

framework::DataProcessorSpec getRawCheckerSpec()
{
  std::vector<of::InputSpec> inputSpecs{of::InputSpec{"mid_decoded", header::gDataOriginMID, "DECODED", 0, of::Lifetime::Timeframe}, of::InputSpec{"mid_decoded_rof", header::gDataOriginMID, "DECODEDROF", 0, of::Lifetime::Timeframe}};

  return of::DataProcessorSpec{
    "MIDRawChecker",
    {inputSpecs},
    {of::Outputs{}},
    of::AlgorithmSpec{of::adaptFromTask<o2::mid::RawCheckerDeviceDPL>()},
    of::Options{
      {"crate-masks-file", of::VariantType::String, "", {"Filename with crate masks"}},
      {"output-file", of::VariantType::String, "bare_data_checker_out.txt", {"Output text filename"}}}};
}
} // namespace mid
} // namespace o2
