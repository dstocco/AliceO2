// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   MID/Workflow/src/RawDecoderWorkflow.cxx
/// \brief  Definition of the raw data decoder workflow for MID
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   17 March 2020

#include "MIDWorkflow/RawDecoderWorkflow.h"

#include "MIDRaw/CRUBareDecoder.h"
#include "MIDRaw/CRUUserLogicDecoder.h"
#include "MIDWorkflow/RawAggregatorSpec.h"
#include "MIDWorkflow/RawDecoderSpec.h"

namespace of = o2::framework;

namespace o2
{
namespace mid
{

of::WorkflowSpec getRawDecoderWorkflow(bool bare, bool aggregate)
{

  of::WorkflowSpec specs;
  if (bare) {
    specs.emplace_back(getRawDecoderSpec<CRUBareDecoder>());
  } else {
    specs.emplace_back(getRawDecoderSpec<CRUUserLogicDecoder>());
  }
  if (aggregate) {
    specs.emplace_back(getRawAggregatorSpec());
  }

  return specs;
}
} // namespace mid
} // namespace o2
