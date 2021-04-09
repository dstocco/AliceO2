// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   MID/Raw/src/DecodedDataAggregator.cxx
/// \brief  MID decoded raw data aggregator
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   26 February 2020

#include "MIDRaw/DecodedDataAggregator.h"

#include "MIDBase/DetectorParameters.h"
#include "MIDRaw/CrateParameters.h"

namespace o2
{
namespace mid
{

ColumnData& DecodedDataAggregator::FindColumnData(uint8_t deId, uint8_t columnId, size_t firstEntry)
{
  /// Gets the matching column data
  /// Adds one if not found
  for (auto colIt = mData.begin() + firstEntry; colIt != mData.end(); ++colIt) {
    if (colIt->deId == deId && colIt->columnId == columnId) {
      return *colIt;
    }
  }
  mData.push_back({deId, columnId});
  return mData.back();
}

void DecodedDataAggregator::addData(const ROBoard& loc, size_t firstEntry)
{
  /// Converts the local board data to ColumnData
  uint8_t uniqueLocId = loc.boardId;
  uint8_t crateId = raw::getCrateId(uniqueLocId);
  bool isRightSide = crateparams::isRightSide(crateId);
  uint16_t deBoardId = mCrateMapper.roLocalBoardToDE(crateId, raw::getLocId(loc.boardId));
  auto rpcLineId = mCrateMapper.getRPCLine(deBoardId);
  auto columnId = mCrateMapper.getColumnId(deBoardId);
  auto lineId = mCrateMapper.getLineId(deBoardId);
  for (int ich = 0; ich < 4; ++ich) {
    if (((loc.firedChambers >> ich) & 0x1) == 0) {
      continue;
    }
    uint8_t deId = detparams::getDEId(isRightSide, ich, rpcLineId);
    auto& col = FindColumnData(deId, columnId, firstEntry);
    col.setBendPattern(loc.patternsBP[ich], lineId);
    col.setNonBendPattern(col.getNonBendPattern() | loc.patternsNBP[ich]);
  }
}

void DecodedDataAggregator::process(gsl::span<const ROBoard> localBoards, gsl::span<const ROFRecord> rofRecords)
{
  /// Aggregates the decoded raw data

  // First clear the output
  mData.clear();
  mROFRecords.clear();

  // Fill the map with ordered events
  for (auto rofIt = rofRecords.begin(); rofIt != rofRecords.end(); ++rofIt) {
    mEventIndexes[static_cast<int>(rofIt->eventType)][rofIt->interactionRecord.toLong()].emplace_back(rofIt - rofRecords.begin());
  }

  const ROFRecord* rof = nullptr;
  for (size_t ievtType = 0; ievtType < mEventIndexes.size(); ++ievtType) {
    mEventTypeStart[ievtType] = mROFRecords.size();
    for (auto& item : mEventIndexes[ievtType]) {
      size_t firstEntry = mData.size();
      for (auto& idx : item.second) {
        // In principle all of these ROF records have the same timestamp
        rof = &rofRecords[idx];
        for (size_t iloc = rof->firstEntry; iloc < rof->firstEntry + rof->nEntries; ++iloc) {
          addData(localBoards[iloc], firstEntry);
        }
      }
      mROFRecords.emplace_back(rof->interactionRecord, rof->eventType, firstEntry, mData.size() - firstEntry);
    }
    // Clear the inner objects when the computation is done
    mEventIndexes[ievtType].clear();
  } // loop on event types
}

std::vector<ColumnData> DecodedDataAggregator::getData(EventType eventType)
{
  /// Returns the data for eventType
  int idx = static_cast<int>(eventType);
  auto rofs = getROFRecords(eventType);
  if (rofs.empty()) {
    return std::vector<ColumnData>();
  }
  auto first = mData.begin() + rofs.front().firstEntry;
  auto last = mData.begin() + rofs.back().firstEntry + rofs.back().nEntries;
  return std::vector<ColumnData>(first, last);
}

std::vector<ROFRecord> DecodedDataAggregator::getROFRecords(EventType eventType)
{
  /// Returns the ROFRecords for eventType
  int idx = static_cast<int>(eventType);
  auto first = mROFRecords.begin() + mEventTypeStart[idx];
  auto last = (idx == mEventTypeStart.size() - 1) ? mROFRecords.end() : mROFRecords.begin() + mEventTypeStart[idx + 1];
  return std::vector<ROFRecord>(first, last);
}

} // namespace mid
} // namespace o2
