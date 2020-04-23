// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   MID/Workflow/src/raw-decoder-workflow.cxx
/// \brief  MID raw decoder workflow
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   17 March 2020

#include <string>
#include <vector>
#include "Framework/Variant.h"
#include "Framework/ConfigParamSpec.h"
#include "MIDWorkflow/RawDecoderWorkflow.h"

using namespace o2::framework;

// add workflow options, note that customization needs to be declared before
// including Framework/runDataProcessing
void customize(std::vector<o2::framework::ConfigParamSpec>& workflowOptions)
{
  std::vector<o2::framework::ConfigParamSpec>
    options{
      {"bare", o2::framework::VariantType::Bool, false, {"Use bare decoder"}},
      {"aggregate", o2::framework::VariantType::Bool, false, {"Aggregate output"}}};
  std::swap(workflowOptions, options);
}

// ------------------------------------------------------------------

#include "Framework/runDataProcessing.h"

WorkflowSpec defineDataProcessing(ConfigContext const& cfgc)
{
  return o2::mid::getRawDecoderWorkflow(cfgc.options().get<bool>("bare"), cfgc.options().get<bool>("aggregate"));
}
