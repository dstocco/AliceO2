// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   MID/Workflow/src/ul-checker-workflow.cxx
/// \brief  MID user logic checker workflow
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   06 April 2020

#include <string>
#include <vector>
#include "Framework/Variant.h"
#include "Framework/ConfigParamSpec.h"
#include "MIDRaw/CrateMasks.h"
#include "MIDRaw/ElectronicsDelay.h"
#include "MIDRaw/FEEIdConfig.h"
#include "MIDWorkflow/RawDecoderSpec.h"
#include "MIDWorkflow/RawGBTDecoderSpec.h"
#include "MIDWorkflow/UserLogicCheckerSpec.h"

using namespace o2::framework;

// add workflow options, note that customization needs to be declared before
// including Framework/runDataProcessing
void customize(std::vector<o2::framework::ConfigParamSpec>& workflowOptions)
{
  std::vector<ConfigParamSpec>
    options{
      {"feeId-config-file", VariantType::String, "", {"Filename with crate FEE ID correspondence"}},
      {"crate-masks-file", VariantType::String, "", {"Filename with crate masks"}},
      {"electronics-delay-file", VariantType::String, "", {"Filename with electronics delay"}}};
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

  std::vector<uint32_t> gbtIds = feeIdConfig.getConfiguredLinkUniqueIDs();
  std::vector<uint16_t> feeIds;
  for (auto& gbtId : gbtIds) {
    feeIds.emplace_back(feeIdConfig.getGBTUniqueId(gbtId));
  }

  WorkflowSpec specs;
  WorkflowSpec templateSpecs;
  templateSpecs.emplace_back(o2::mid::getRawGBTDecoderSpec(true, feeIds, crateMasks, electronicsDelay));

  auto parallelSpecs = parallelPipeline(
    templateSpecs, gbtIds.size(),
    [&gbtIds]() { return gbtIds.size(); },
    [&gbtIds](size_t index) { return gbtIds[index]; });
  specs.insert(specs.end(), parallelSpecs.begin(), parallelSpecs.end());

  std::vector<int> bareSubSpecs, ulSubSpecs;
  for (auto& gbtId : gbtIds) {
    bareSubSpecs.emplace_back(static_cast<int>(gbtId));
    int ulSubSpec = feeIdConfig.getLinkUniqueId(15, feeIdConfig.getEndPointId(gbtId), feeIdConfig.getCRUId(gbtId));
    if (std::find(ulSubSpecs.begin(), ulSubSpecs.end(), ulSubSpec) == ulSubSpecs.end()) {
      ulSubSpecs.emplace_back(ulSubSpec);
    }
  }

  for (auto& subSpec : ulSubSpecs) {
    specs.emplace_back(o2::mid::getRawDecoderSpec(true, feeIdConfig, crateMasks, electronicsDelay, subSpec));
  }
  specs.emplace_back(o2::mid::getUserLogicCheckerSpec(bareSubSpecs, ulSubSpecs));
  return specs;
}
