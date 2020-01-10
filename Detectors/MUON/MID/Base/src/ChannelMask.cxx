// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   MID/Base/src/ChannelMask.cxx
/// \brief  MID channels masks
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   09 January 2020

#include "MIDBase/ChannelMask.h"

#include "MIDBase/DetectorParameters.h"
#include "MIDBase/Mapping.h"

namespace o2
{
namespace mid
{
void ChannelMask::switchOffChannels(const ColumnData& dead)
{
  /// Switches off the dead channels
  int idx = getIndex(dead);
  mMasks[idx].setNonBendPattern(mMasks[idx].getNonBendPattern() & (~dead.getNonBendPattern()));
  for (int iline = 0; iline < 4; ++iline) {
    mMasks[idx].setBendPattern(mMasks[idx].getBendPattern(iline) & (~dead.getBendPattern(iline)), iline);
  }
}

ChannelMask buildDefaultChannelMasks()
{
  /// Builds the default channel mask
  Mapping mapping;
  ChannelMask channelMasks;
  for (int ide = 0; ide < detparams::NDetectionElements; ++ide) {
    for (int icol = mapping.getFirstColumn(ide); icol < 7; ++icol) {
      ColumnData mask;
      mask.deId = ide;
      mask.columnId = icol;
      for (int iline = mapping.getFirstBoardBP(icol, ide); iline <= mapping.getLastBoardBP(icol, ide); ++iline) {
        mask.setBendPattern(0xFFFF, iline);
      }
      for (int istrip = 0; istrip < mapping.getNStripsNBP(icol, ide); ++istrip) {
        mask.addStrip(istrip, 1, 0);
      }
      channelMasks.setFromChannelMask(mask);
    }
  }
  return channelMasks;
}
} // namespace mid
} // namespace o2
