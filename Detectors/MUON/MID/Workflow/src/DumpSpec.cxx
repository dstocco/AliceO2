// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   DumpSpec.cxx
/// \brief  Data processor spec for data dump device
/// \author Gabriele G. Fronze <gfronze at cern.ch>
/// \date   11 July 2018

#include "MIDWorkflow/DumpSpec.h"

#include "Framework/DataProcessorSpec.h"
#include "Framework/InputSpec.h"
#include "Framework/AlgorithmSpec.h"
#include "DataFormatsMID/Track.h"
#include <string>

namespace of = o2::framework;

namespace o2
{
namespace mid
{
framework::DataProcessorSpec getDumpSpec()
{
  std::string inputBinding = "mid_tracks";
  return of::DataProcessorSpec{
    "Dump",
    of::Inputs{ of::InputSpec{ inputBinding.c_str(), "MID", "TRACKS" } },
    of::Outputs{},
    of::AlgorithmSpec{
      [inputBinding](of::ProcessingContext& pc) {
        auto msg = pc.inputs().get(inputBinding.c_str());
        gsl::span<Track> spanData = of::DataRefUtils::as<Track>(msg);
        for (auto& track : spanData) {
          LOG(INFO) << track;
        }
      } }
  };
}
} // namespace mid
} // namespace o2