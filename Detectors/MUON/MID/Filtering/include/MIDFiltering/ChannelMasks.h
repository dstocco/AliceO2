// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   MIDFiltering/ChannelMasks.h
/// \brief  MID channels masks
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   09 January 2020
#ifndef O2_MID_CHANNELMASKS_H
#define O2_MID_CHANNELMASKS_H

#include <cstdint>
#include <vector>
#include <unordered_map>
#include "DataFormatsMID/ColumnData.h"

namespace o2
{
namespace mid
{

class ChannelMasks
{
 public:
  void switchOffChannel(uint8_t deId, uint8_t columnId, int lineId, int strip, int cathode);
  void switchOffChannels(const ColumnData& dead);
  /// Sets the mask from a channel mask
  void setFromChannelMask(const ColumnData& mask) { mMasks[getUniqueId(mask.deId, mask.columnId)] = mask; }
  bool applyMask(ColumnData& data);

  std::vector<ColumnData> getMasks() const;

 private:
  ColumnData& getMask(uint8_t deId, uint8_t columnId);
  inline uint16_t getUniqueId(uint8_t deId, uint8_t columnId)
  {
    /// Gets an unique ID
    return (static_cast<uint16_t>(deId) << 4) | columnId;
  }
  std::unordered_map<uint16_t, ColumnData> mMasks{}; // Channel masks
};

} // namespace mid
} // namespace o2

#endif /* O2_MID_CHANNELMASKS_H */
