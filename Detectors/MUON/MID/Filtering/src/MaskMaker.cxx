// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   MID/Filtering/src/MaskMaker.cxx
/// \brief  Function to produce the MID masks
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   05 March 2021

#include "MIDFiltering/MaskMaker.h"

namespace o2
{
namespace mid
{
ChannelMasks makeMasks(const ChannelScalers& scalers, uint32_t threshold)
{
  /// Makes the mask
  ChannelMasks mask;
  for (const auto scaler : scalers.getScalers()) {
    if (scaler.second >= threshold) {
      mask.switchOffChannel(scalers.getDeId(scaler.first), scalers.getColumnId(scaler.first), scalers.getLineId(scaler.first), scalers.getStrip(scaler.first), scalers.getCathode(scaler.first));
    }
  }
  return mask;
}

} // namespace mid
} // namespace o2
