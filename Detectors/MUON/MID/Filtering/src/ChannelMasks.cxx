// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   MID/Filtering/src/ChannelMasks.cxx
/// \brief  MID channels masks
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   09 January 2020

#include "MIDFiltering/ChannelMasks.h"

namespace o2
{
namespace mid
{

ColumnData& ChannelMasks::getMask(uint8_t deId, uint8_t columnId)
{
  /// Gets the mask
  auto uniqueId = getUniqueId(deId, columnId);
  auto maskIt = mMasks.find(uniqueId);
  if (maskIt == mMasks.end()) {
    auto& newMask = mMasks[uniqueId];
    newMask.deId = deId;
    newMask.columnId = columnId;
    newMask.patterns.fill(0xFFFF);
    return newMask;
  }
  return maskIt->second;
}

void ChannelMasks::switchOffChannel(uint8_t deId, uint8_t columnId, int lineId, int strip, int cathode)
{
  /// Switches off one channel
  auto& mask = getMask(deId, columnId);
  uint16_t pattern = (1 << strip);
  if (cathode == 0) {
    mask.setBendPattern(mask.getBendPattern(lineId) & ~pattern, lineId);
  } else {
    mask.setNonBendPattern(mask.getNonBendPattern() & ~pattern);
  }
}

void ChannelMasks::switchOffChannels(const ColumnData& dead)
{
  /// Switches off the dead channels
  auto& mask = getMask(dead.deId, dead.columnId);
  mask.setNonBendPattern(mask.getNonBendPattern() & (~dead.getNonBendPattern()));
  for (int iline = 0; iline < 4; ++iline) {
    mask.setBendPattern(mask.getBendPattern(iline) & (~dead.getBendPattern(iline)), iline);
  }
}

bool ChannelMasks::applyMask(ColumnData& data)
{
  /// Applies the mask to the data
  /// Returns false if the data is completely masked
  auto uniqueId = getUniqueId(data.deId, data.columnId);
  auto maskIt = mMasks.find(uniqueId);
  if (maskIt == mMasks.end()) {
    return true;
  }
  uint16_t allPatterns = 0;
  data.setNonBendPattern(data.getNonBendPattern() & maskIt->second.getNonBendPattern());
  allPatterns |= data.getNonBendPattern();
  for (int iline = 0; iline < 4; ++iline) {
    data.setBendPattern(data.getBendPattern(iline) & maskIt->second.getBendPattern(iline), iline);
    allPatterns |= data.getBendPattern(iline);
  }
  return (allPatterns != 0);
}

std::vector<ColumnData> ChannelMasks::getMasks() const
{
  /// Gets the masks
  std::vector<ColumnData> masks;
  for (auto& maskIt : mMasks) {
    masks.emplace_back(maskIt.second);
  }
  return masks;
}

// ChannelMasks buildDefaultChannelMasks()
// {
//   /// Builds the default channel mask
//   Mapping mapping;
//   ChannelMasks ChannelMasks;
//   for (int ide = 0; ide < detparams::NDetectionElements; ++ide) {
//     for (int icol = mapping.getFirstColumn(ide); icol < 7; ++icol) {
//       ColumnData mask;
//       mask.deId = ide;
//       mask.columnId = icol;
//       for (int iline = mapping.getFirstBoardBP(icol, ide); iline <= mapping.getLastBoardBP(icol, ide); ++iline) {
//         mask.setBendPattern(0xFFFF, iline);
//       }
//       for (int istrip = 0; istrip < mapping.getNStripsNBP(icol, ide); ++istrip) {
//         mask.addStrip(istrip, 1, 0);
//       }
//       ChannelMasks.setFromChannelMasks(mask);
//     }
//   }
//   return ChannelMasks;
// }
} // namespace mid
} // namespace o2
