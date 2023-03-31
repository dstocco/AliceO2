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

/// \file   MID/Workflow/src/clusters-writer-workflow.cxx
/// \brief  MID decoded digits writer workflow
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   29 October 2020

#include <string>
#include <vector>
#include "Framework/Variant.h"
#include "Framework/ConfigParamSpec.h"
#include "Framework/CompletionPolicyHelpers.h"

using namespace o2::framework;

// we need to add workflow options before including Framework/runDataProcessing
void customize(std::vector<ConfigParamSpec>& workflowOptions)
{
  std::vector<ConfigParamSpec>
    options{
      {"output-filename", VariantType::String, "mid-clusters.root", {"Clusters output file"}}};
  workflowOptions.insert(workflowOptions.end(), options.begin(), options.end());
}

void customize(std::vector<o2::framework::CompletionPolicy>& policies)
{
  // ordered policies for the writers
  policies.push_back(CompletionPolicyHelpers::consumeWhenAllOrdered(".*(?:MID|mid).*[W,w]riter.*"));
}

#include "Framework/runDataProcessing.h"
#include "DPLUtils/MakeRootTreeWriterSpec.h"
#include "DataFormatsMID/Cluster.h"
#include "DataFormatsMID/ROFRecord.h"

WorkflowSpec defineDataProcessing(ConfigContext const& cfgc)
{
  auto outputFilename = cfgc.options().get<std::string>("output-filename");
  auto treeFilename = "midclusters";
  WorkflowSpec specs;
  specs.emplace_back(MakeRootTreeWriterSpec("MIDDigitWriter",
                                            outputFilename.c_str(),
                                            treeFilename.c_str(),
                                            -1,
                                            MakeRootTreeWriterSpec::BranchDefinition<std::vector<o2::mid::Cluster>>{InputSpec{"mid_clusters", o2::header::gDataOriginMID, "CLUSTERS", 0}, "MIDCluster"},
                                            MakeRootTreeWriterSpec::BranchDefinition<std::vector<o2::mid::ROFRecord>>{InputSpec{"mid_clusters_rof", o2::header::gDataOriginMID, "CLUSTERSROF", 0}, "MIDClusterROF"},
                                            MakeRootTreeWriterSpec::BranchDefinition<std::vector<o2::mid::ROFRecord>>{InputSpec{"mid_clusters_labels", o2::header::gDataOriginMID, "CLUSTERSLABELS", 0}, "MIDClusterLabel"})());

  return specs;
}
