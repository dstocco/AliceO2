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
  void setFeeId(uint16_t feeId, int8_t linkId, uint8_t endPointId, uint16_t cruId) { mGBTIdToFeeId[getId(linkId, endPointId, cruId)] = feeId; }

  uint16_t getFeeId(uint8_t linkId, uint8_t endPointId, uint16_t cruId);

  bool load(const char* filename);
  void write(const char* filename) const;

 private:
  inline uint32_t getId(uint8_t linkId, uint8_t endPointId, uint16_t cruId) const { return linkId | (endPointId << 8) | (cruId << 16); }

  std::unordered_map<uint32_t, uint16_t> mGBTIdToFeeId; /// Correspondence between GBT Id and FeeId
};

CrateFeeIdMapper createDefaultCrateFeeIdMapper();

} // namespace mid
} // namespace o2

#endif /* O2_MID_CRATEFEEIDMAPPER_H */
