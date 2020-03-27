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
#include "MIDRaw/CrateMasks.h"
#include "MIDRaw/ElectronicsDelay.h"
#include "MIDRaw/FEEIdConfig.h"
#include "MIDWorkflow/RawCheckerSpec.h"
#include "MIDWorkflow/RawDecoderSpec.h"
#include "MIDWorkflow/RawGBTDecoderSpec.h"

using namespace o2::framework;

// add workflow options, note that customization needs to be declared before
// including Framework/runDataProcessing
void customize(std::vector<o2::framework::ConfigParamSpec>& workflowOptions)
{
  std::vector<ConfigParamSpec>
    options{
      {"feeId-config-file", VariantType::String, "", {"Filename with crate FEE ID correspondence"}},
      {"crate-masks-file", VariantType::String, "", {"Filename with crate masks"}},
      {"electronics-delay-file", VariantType::String, "", {"Filename with electronics delay"}},
      {"bare", VariantType::Bool, false, {"Is bare decoder"}},
      {"per-gbt", VariantType::Bool, false, {"One process per GBT link"}},
      {"max-errors", VariantType::Int, 10000, {"Maximum number of errors"}}};
  workflowOptions.insert(workflowOptions.end(), options.begin(), options.end());
}

// ------------------------------------------------------------------

#include "Framework/runDataProcessing.h"

WorkflowSpec defineDataProcessing(ConfigContext const& cfgc)
{
  auto feeIdConfigFilename = cfgc.options().get<std::string>("feeId-config-file");
  o2::mid::FEEIdConfig feeIdConfig;
  if (!feeIdConfigFilename.empty()) {
    feeIdConfig = o2::mid::FEEIdConfig(feeIdConfigFilename.c_str());
  }
  auto crateMasksFilename = cfgc.options().get<std::string>("crate-masks-file");
  o2::mid::CrateMasks crateMasks;
  if (!crateMasksFilename.empty()) {
    crateMasks = o2::mid::CrateMasks(crateMasksFilename.c_str());
  }
  auto electronicsDelayFilename = cfgc.options().get<std::string>("electronics-delay-file");
  o2::mid::ElectronicsDelay electronicsDelay;
  if (!electronicsDelayFilename.empty()) {
    electronicsDelay = o2::mid::readElectronicsDelay(electronicsDelayFilename.c_str());
  }

  bool isBare = cfgc.options().get<bool>("bare");

  bool perGBT = cfgc.options().get<bool>("per-gbt");

  auto nMaxErrors = cfgc.options().get<int>("max-errors");

  o2::framework::WorkflowSpec specs;
  if (perGBT) {
    auto decs = o2::mid::getRawGBTDecoderSpecs(isBare, true, feeIdConfig, crateMasks, electronicsDelay);
    specs.insert(specs.end(), decs.begin(), decs.end());

    WorkflowSpec checks = o2::mid::getRawCheckerSpecs(feeIdConfig, electronicsDelay, nMaxErrors);
    specs.insert(specs.end(), checks.begin(), checks.end());
  } else {
    specs.emplace_back(o2::mid::getRawDecoderSpec(isBare, true, feeIdConfig, crateMasks, electronicsDelay));
    specs.emplace_back(o2::mid::getRawCheckerSpec(feeIdConfig, crateMasks, electronicsDelay, "", nMaxErrors));
  }
  return specs;
}
