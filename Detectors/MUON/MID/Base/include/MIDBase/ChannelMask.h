// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   MIDBase/include/ChannelMask.h
/// \brief  MID channels masks
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   09 January 2020
#ifndef O2_MID_CHANNELMASK_H
#define O2_MID_CHANNELMASK_H

#include <array>
#include "DataFormatsMID/ColumnData.h"
#include "MIDBase/DetectorParameters.h"

namespace o2
{
namespace mid
{

class ChannelMask
{
 public:
  void switchOffChannels(const ColumnData& dead);
  /// Sets the mask from a channel mask
  void setFromChannelMask(const ColumnData& mask) { mMasks[getIndex(mask)] = mask; }
  const ColumnData& getMask(const ColumnData& data) const { return mMasks[getIndex(data)]; }

 private:
  inline int getIndex(const ColumnData& data) const { return 7 * data.deId + data.columnId; }
  std::array<ColumnData, detparams::NDetectionElements * 7> mMasks; // Channel masks
};

ChannelMask buildDefaultChannelMasks();
} // namespace mid
} // namespace o2

#endif /* O2_MID_CHANNELMASK_H */
