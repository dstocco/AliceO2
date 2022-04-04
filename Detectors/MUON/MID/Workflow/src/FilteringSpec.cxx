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

/// \file   MID/Workflow/src/FilteringSpec.cxx
/// \brief  MID filtering spec
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   16 March 2022

#include "MIDWorkflow/FilteringSpec.h"

#include <vector>
#include <gsl/gsl>
#include "Framework/CCDBParamSpec.h"
#include "Framework/Output.h"
#include "Framework/Task.h"
#include "DataFormatsMID/ColumnData.h"
#include "DataFormatsMID/ROFRecord.h"
#include "SimulationDataFormat/MCTruthContainer.h"
#include "MIDFiltering/Filterer.h"
#include "MIDSimulation/MCLabel.h"

namespace of = o2::framework;

namespace o2
{
namespace mid
{

class FilteringDeviceDPL
{
 public:
  FilteringDeviceDPL(bool useMC) : mUseMC(useMC)
  {
    if (useMC) {
      mFillLabels = [](size_t inIdx, size_t outIdx, const o2::dataformats::MCTruthContainer<MCLabel>* inMCContainer, o2::dataformats::MCTruthContainer<MCLabel>& outMCContainer) { outMCContainer.addElements(outIdx, inMCContainer->getLabels(inIdx)); };
    }
  }

  void init(o2::framework::InitContext& ic)
  {
  }

  void run(o2::framework::ProcessingContext& pc)
  {
    const auto badChannels = pc.inputs().get<std::vector<ColumnData>*>("mid_bad_channels");

    const auto data = pc.inputs().get<gsl::span<ColumnData>>("mid_data_mc");
    const auto inROFRecords = pc.inputs().get<gsl::span<ROFRecord>>("mid_data_mc_rof");

    const auto inMCContainer = mUseMC ? pc.inputs().get<const o2::dataformats::MCTruthContainer<MCLabel>*>("mid_data_mc_labels") : nullptr;

    o2::dataformats::MCTruthContainer<MCLabel> outMCContainer;

    std::vector<ROFRecord> maskedRofs;
    maskedRofs.reserve(inROFRecords.size());
    std::vector<ColumnData> maskedData;
    maskedData.reserve(data.size());
    for (auto& rof : inROFRecords) {
      auto firstEntry = maskedData.size();
      for (auto dataIt = data.begin() + rof.firstEntry, end = data.begin() + rof.getEndIndex(); dataIt != end; ++dataIt) {
        auto col = *dataIt;
        if (mMasksHandler.applyMask(col)) {
          // Data are not fully masked
          maskedData.emplace_back(col);
          auto inIdx = std::distance(data.begin(), dataIt);
          mFillLabels(inIdx, maskedData.size() - 1, inMCContainer.get(), outMCContainer);
          maskedRofs.emplace_back(rof);
          maskedRofs.back().firstEntry = firstEntry;
          maskedRofs.back().nEntries = maskedData.size() - firstEntry;
        }
      }
    }

    pc.outputs().snapshot(of::Output{header::gDataOriginMID, "FDATA", 0, of::Lifetime::Timeframe}, maskedData);
    pc.outputs().snapshot(of::Output{header::gDataOriginMID, "FDATAROF", 0, of::Lifetime::Timeframe}, maskedRofs);
    if (mUseMC) {
      pc.outputs().snapshot(of::Output{header::gDataOriginMID, "FDATALABELS", 0, of::Lifetime::Timeframe}, outMCContainer);
    }
  }

 private:
  ChannelMasksHandler mMasksHandler{};
  std::function<void(size_t, size_t, const o2::dataformats::MCTruthContainer<MCLabel>*, o2::dataformats::MCTruthContainer<MCLabel>&)> mFillLabels{};
  bool mUseMC{true};
};

of::DataProcessorSpec getFilteringSpec(bool useMC, bool beforeZS)
{
  std::vector<of::InputSpec> inputSpecs;
  std::vector<of::OutputSpec> outputSpecs;
  inputSpecs.emplace_back("mid_bad_channels", header::gDataOriginMID, "BAD_CHANNELS", 0, of::Lifetime::Condition, of::ccdbParamSpec("MID/BadChannels", true));

  if (beforeZS) {
    inputSpecs.emplace_back("mid_in_data", header::gDataOriginMID, "DATAMC");
    inputSpecs.emplace_back("mid_in_data_rof", header::gDataOriginMID, "DATAMCROF");
    outputSpecs.emplace_back(header::gDataOriginMID, "FDATAMC");
    outputSpecs.emplace_back(header::gDataOriginMID, "FDATAMCROF");
  } else {
    inputSpecs.emplace_back("mid_in_data", of::ConcreteDataTypeMatcher(header::gDataOriginMID, "DATA"), of::Lifetime::Timeframe);
    inputSpecs.emplace_back("mid_in_data_rof", of::ConcreteDataTypeMatcher(header::gDataOriginMID, "DATAROF"), of::Lifetime::Timeframe);
    outputSpecs.emplace_back(header::gDataOriginMID, "FDATA");
    outputSpecs.emplace_back(header::gDataOriginMID, "FDATAROF");
  }

  if (useMC) {
    inputSpecs.emplace_back(of::InputSpec{"mid_data_mc_labels", header::gDataOriginMID, "DATAMCLABELS"});
    outputSpecs.emplace_back(of::OutputSpec{header::gDataOriginMID, "FDATAMCLABELS"});
  }

  return of::DataProcessorSpec{
    "MIDFiltering",
    {inputSpecs},
    {outputSpecs},
    of::AlgorithmSpec{of::adaptFromTask<o2::mid::FilteringDeviceDPL>(useMC)}};
}
} // namespace mid
} // namespace o2