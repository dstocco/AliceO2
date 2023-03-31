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

/// \file   MID/Workflow/src/ClusterReaderSpec.cxx
/// \brief  MID cluster reader device
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   15 February 2023

#include "MIDWorkflow/ClusterReaderSpec.h"

#include <memory>
#include <sstream>
#include <string>
#include "DPLUtils/RootTreeReader.h"
#include "Framework/ConfigParamRegistry.h"
#include "Framework/ControlService.h"
#include "Framework/DataSpecUtils.h"
#include "Framework/Logger.h"
#include "Framework/Output.h"
#include "Framework/Task.h"
#include "Framework/WorkflowSpec.h"
#include "SimulationDataFormat/MCTruthContainer.h"
#include "DataFormatsMID/ColumnData.h"
#include "DataFormatsMID/ROFRecord.h"
#include "DataFormatsMID/MCLabel.h"
#include "CommonUtils/NameConf.h"
#include "CommonUtils/StringUtils.h"

using namespace o2::framework;

namespace o2
{
namespace mid
{

template <typename T>
void printBranch(char* data, const char* what)
{
  auto tdata = reinterpret_cast<std::vector<T>*>(data);
  LOGP(info, "MID {:d} {:s}", tdata->size(), what);
}

RootTreeReader::SpecialPublishHook logging{
  [](std::string_view name, ProcessingContext&, Output const&, char* data) -> bool {
    if (name == "MIDCluster") {
      printBranch<ROFRecord>(data, "CLUSTERS");
    }
    if (name == "MIDClusterROF") {
      printBranch<ROFRecord>(data, "CLUSTERSROF");
    }
    if (name == "MIDClusterLabel") {
      auto tdata = reinterpret_cast<o2::dataformats::MCTruthContainer<MCClusterLabel>*>(data);
      LOGP(info, "MID {:d} {:s}", tdata->getNElements(), "CLUSTERSLABELS");
    }
    return false;
  }};

class ClusterReaderDeviceDPL
{
 public:
  ClusterReaderDeviceDPL(bool useMC) : mUseMC(useMC) {}

  void init(InitContext& ic)
  {
    auto treename = "midcluster";
    auto filename = utils::Str::concat_string(utils::Str::rectifyDirectory(ic.options().get<std::string>("input-dir")),
                                              ic.options().get<std::string>("mid-digit-infile"));
    auto nofEntries{-1};
    auto clusterOut =
      if (mUseMC)
    {
      mReader = std::make_unique<RootTreeReader>(
        treename, filename.c_str(), nofEntries,
        RootTreeReader::PublishingMode::Single,
        RootTreeReader::BranchDefinition<std::vector<Cluster>>{Output{header::gDataOriginMID, "CLUSTERS", 0}, "MIDCluster"},
        RootTreeReader::BranchDefinition<std::vector<ROFRecord>>{Output{header::gDataOriginMID, "CLUSTERSROF", 0}, "MIDClusterROF"},
        RootTreeReader::BranchDefinition<dataformats::MCTruthContainer<MCLabel>>{Output{header::gDataOriginMID, "CLUSTERSLABELS", 0, "MIDClusterMCLabels"}},
        &logging);
    }
    else
    {
      mReader = std::make_unique<RootTreeReader>(
        treename, filename.c_str(), nofEntries,
        RootTreeReader::PublishingMode::Single,
        RootTreeReader::BranchDefinition<std::vector<Cluster>>{Output{header::gDataOriginMID, "CLUSTERS", 0}, "MIDCluster"},
        RootTreeReader::BranchDefinition<std::vector<ROFRecord>>{Output{header::gDataOriginMID, "CLUSTERSROF", 0}, "MIDClusterROF"},
        &logging);
    }
  }

  void run(ProcessingContext& pc)
  {
    if ((++(*mReader))(pc) == false) {
      pc.services().get<ControlService>().endOfStream();
    }
  }

 private:
  std::unique_ptr<RootTreeReader> mReader{};
  bool mUseMC = false;
};

DataProcessorSpec getClusterReaderSpec(bool useMC)
{
  std::vector<OutputSpec> outputs{
    OutputSpec{header::gDataOriginMID, "CLUSTERS"},
    OutputSpec{header::gDataOriginMID, "CLUSTERSROF"}};
  if (useMC) {
    outputs.emplace_back(OutputSpec{header::gDataOriginMID, "CLUSTERSLABELS"});
  }

  return DataProcessorSpec{
    "MIDClusterReader",
    Inputs{},
    outputs,
    AlgorithmSpec{adaptFromTask<ClusterReaderDeviceDPL>(useMC)},
    Options{{"mid-cluster-infile", VariantType::String, "mid-clusters.root", {"Name of the input file"}}}};
}
} // namespace mid
} // namespace o2
