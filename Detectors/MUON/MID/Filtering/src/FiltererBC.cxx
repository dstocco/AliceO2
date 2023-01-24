// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   MID/Filtering/src/FiltererBC.cxx
/// \brief  BC filterer for MID
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   23 January 2023

#include "MIDFiltering/FiltererBC.h"

namespace o2
{
namespace mid
{

int FiltererBC::matchedCollision(int bc, const BunchFilling& bcFilling)
{
  for (int ibc = bc + mBCDiffLow, end = bc + mBCDiffHigh; ibc <= end; ++ibc) {
    if (bcFilling.testInteractingBC(ibc)) {
      return ibc;
    }
  }
  return -1;
}

void FiltererBC::process(gsl::span<const ColumnData> data, gsl::span<const ROFRecord> rofRecords, const BunchFilling& bcFilling)
{
  // Clear inner container
  mFilteredROFs.clear();
  mFilteredData.clear();
  auto rofIt = rofRecords.begin();
  auto end = rofRecords.end();
  // Loop on ROFs
  for (; rofIt != end; ++rofIt) {
    // Check if BC matches a collision BC
    auto matchedColl = matchedCollision(rofIt->interactionRecord.bc, bcFilling);
    if (matchedColl >= 0) {
      bool needsMerge = false;
      auto rofData = data.subspan(rofIt->firstEntry, rofIt->nEntries);
      InteractionRecord ir = rofIt->interactionRecord;
      // Use the BC of the matching collision
      ir.bc = matchedColl;
      // Search for neighbor BCs
      for (auto auxIt = rofIt + 1; auxIt != end; ++auxIt) {
        // We assume that data are time-ordered
        if (auxIt->interactionRecord.orbit != rofIt->interactionRecord.orbit) {
          // Orbit does not match
          break;
        }
        int bcDiff = auxIt->interactionRecord.bc - matchedColl;
        if (bcDiff < mBCDiffLow || bcDiff > mBCDiffHigh) {
          // BC is not in the allowed window
          break;
        }

        // The BC is in the good window: merge it
        needsMerge = true;
        mHandler.merge(data.subspan(auxIt->firstEntry, auxIt->nEntries));
        // CAVEAT: we are updating rofIt here. Do not use it in the following
        rofIt = auxIt;
      }
      // This saves a little bit of time.
      // If the current ROF had no neighbors within the defined BC window,
      // we do not need to perform some complex merging.
      auto firstEntry = mFilteredData.size();
      size_t nEntries = rofData.size();
      if (needsMerge) {
        // We need to merge the current ROF with the neighbor ones
        mHandler.merge(rofData);
        auto merged = mHandler.getMerged();
        nEntries = merged.size();
        mFilteredData.insert(mFilteredData.end(), merged.begin(), merged.end());
        // Clear the handler for the next collision BC
        mHandler.clear();
      } else {
        // In this case we simply add the current data
        mFilteredData.insert(mFilteredData.end(), rofData.begin(), rofData.end());
      }
      // Here we build the filtered ROF
      mFilteredROFs.emplace_back(ir, rofIt->eventType, firstEntry, nEntries);
    }
  }
}

} // namespace mid
} // namespace o2
