// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   MIDWorkflow/RawDecoderWorkflow.h
/// \brief  Definition of the raw data decoder workflow for MID
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   17 March 2020

#ifndef O2_MID_RAWDECODERWORKFLOW_H
#define O2_MID_RAWDECODERWORKFLOW_H

#include "Framework/WorkflowSpec.h"

namespace o2
{
namespace mid
{
framework::WorkflowSpec getRawDecoderWorkflow(bool bare, bool aggregate);
}
} // namespace o2

#endif //O2_MID_RAWDECODERWORKFLOW_H
