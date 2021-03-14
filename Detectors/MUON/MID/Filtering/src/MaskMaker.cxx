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
#include "MIDBase/Mapping.h"
#include "MIDBase/DetectorParameters.h"

namespace o2
{
namespace mid
{
ChannelMasks makeMasks(const ChannelScalers& scalers, unsigned long nEvents, double threshold)
{
  /// Makes the mask
  uint32_t nThresholdEvents = static_cast<uint32_t>(threshold * nEvents);
  ChannelMasks mask;
  for (const auto scaler : scalers.getScalers()) {
    if (scaler.second >= nThresholdEvents) {
      mask.switchOffChannel(scalers.getDeId(scaler.first), scalers.getColumnId(scaler.first), scalers.getLineId(scaler.first), scalers.getStrip(scaler.first), scalers.getCathode(scaler.first));
    }
  }
  return mask;
}

ChannelMasks makeDefaultMasks()
{
  /// Makes the mask
  Mapping mapping;
  ChannelMasks chMasks;
  uint16_t fullPattern = 0xFFFF;
  for (int ide = 0; ide < detparams::NDetectionElements; ++ide) {
    for (int icol = mapping.getFirstColumn(ide); icol < 7; ++icol) {
      ColumnData mask;
      mask.deId = static_cast<uint8_t>(ide);
      mask.columnId = static_cast<uint8_t>(icol);
      int nFullPatterns = 0;
      for (int iline = mapping.getFirstBoardBP(icol, ide), lastLine = mapping.getLastBoardBP(icol, ide); iline <= lastLine; ++iline) {
        mask.setBendPattern(fullPattern, iline);
        ++nFullPatterns;
      }
      for (int istrip = 0; istrip < mapping.getNStripsNBP(icol, ide); ++istrip) {
        mask.addStrip(istrip, 1, 0);
      }
      if (mask.getNonBendPattern() == fullPattern && nFullPatterns == 4) {
        continue;
      }
      chMasks.setFromChannelMask(mask);
    }
  }

  return chMasks;
}

} // namespace mid
} // namespace o2
