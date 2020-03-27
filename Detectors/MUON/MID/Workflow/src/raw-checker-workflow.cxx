// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   MID/Workflow/src/raw-checker-workflow.cxx
/// \brief  MID raw checker workflow
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   06 April 2020

#include <string>
#include <vector>
#include "Framework/Variant.h"
#include "Framework/ConfigParamSpec.h"
#include "MIDRaw/FEEIdConfig.h"
#include "MIDRaw/CrateMasks.h"
#include "MIDWorkflow/RawCheckerSpec.h"
#include "MIDWorkflow/RawDecoderSpec.h"
#include "MIDWorkflow/RawGBTDecoderSpec.h"

using namespace o2::framework;

// add workflow options, note that customization needs to be declared before
// including Framework/runDataProcessing
void customize(std::vector<o2::framework::ConfigParamSpec>& workflowOptions)
{
  std::vector<o2::framework::ConfigParamSpec>
    options{
      {"feeId-config-file", o2::framework::VariantType::String, "", {"Filename with crate FEE ID correspondence"}},
      {"crate-masks-file", o2::framework::VariantType::String, "", {"Filename with crate masks"}},
      {"bare", o2::framework::VariantType::Bool, false, {"Is bare decoder"}},
      {"per-gbt", o2::framework::VariantType::Bool, false, {"One process per GBT link"}},
      {"max-errors", o2::framework::VariantType::Int, 10000, {"Maximum number of errors"}}};
  std::swap(workflowOptions, options);
}

// ------------------------------------------------------------------

#include "Framework/runDataProcessing.h"

WorkflowSpec defineDataProcessing(ConfigContext const& cfgc)
{
  auto feeIdConfigFilename = cfgc.options().get<std::string>("feeId-config-file");
  auto crateMasksFilename = cfgc.options().get<std::string>("crate-masks-file");

  bool isBare = cfgc.options().get<bool>("bare");

  bool perGBT = cfgc.options().get<bool>("per-gbt");

  auto nMaxErrors = cfgc.options().get<int>("max-errors");

  o2::framework::WorkflowSpec specs;
  if (perGBT) {
    auto decs = o2::mid::getRawGBTDecoderSpecs(isBare, true, feeIdConfigFilename.c_str(), crateMasksFilename.c_str());
    specs.insert(specs.end(), decs.begin(), decs.end());

    WorkflowSpec checks = o2::mid::getRawCheckerSpecs(feeIdConfigFilename.c_str(), nMaxErrors);
    specs.insert(specs.end(), checks.begin(), checks.end());
  } else {
    specs.emplace_back(o2::mid::getRawDecoderSpec(isBare, true, feeIdConfigFilename.c_str(), crateMasksFilename.c_str()));
    specs.emplace_back(o2::mid::getRawCheckerSpec("", feeIdConfigFilename.c_str(), "", nMaxErrors));
  }
  return specs;
}
