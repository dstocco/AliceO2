// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   MIDRAW/include/CrateFeeIdMapper.h
/// \brief  Hardware Id to FeeId mapper
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   11 March 2020
#ifndef O2_MID_CRATEFEEIDMAPPER_H
#define O2_MID_CRATEFEEIDMAPPER_H

#include <cstdint>
#include <unordered_map>

namespace o2
{
namespace mid
{
class CrateFeeIdMapper
{
 public:
  // Sets the feeId
  void setFeeId(uint16_t feeId, int8_t linkId, uint8_t endPointId, uint16_t cruId) { mGBTIdToFeeId[getUniqueId(linkId, endPointId, cruId)] = feeId; }

  uint16_t getFeeId(uint32_t uniqueId) const;

  /// Gets the FEE ID from the physical ID of the link
  uint16_t getFeeId(uint8_t linkId, uint8_t endPointId, uint16_t cruId) const { return getFeeId(getUniqueId(linkId, endPointId, cruId)); }

  /// Gets a uniqueID from the combination of linkId, endPointId and cruId;
  inline uint32_t getUniqueId(uint8_t linkId, uint8_t endPointId, uint16_t cruId) const { return linkId | (endPointId << 8) | (cruId << 16); }

  bool load(const char* filename);
  void write(const char* filename) const;

 private:
  std::unordered_map<uint32_t, uint16_t> mGBTIdToFeeId; /// Correspondence between GBT Id and FeeId
};

CrateFeeIdMapper createDefaultCrateFeeIdMapper();

} // namespace mid
} // namespace o2

#endif /* O2_MID_CRATEFEEIDMAPPER_H */
