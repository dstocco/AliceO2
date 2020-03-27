// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   MID/QC/src/CRUBareDataChecker.cxx
/// \brief  Class to check the bare data from the CRU
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   9 December 2019

#include "MIDQC/CRUBareDataChecker.h"

#include <array>
#include <sstream>
#include "MIDRaw/CrateParameters.h"

namespace o2
{
namespace mid
{

bool CRUBareDataChecker::checkLocalBoardSize(const LocalBoardRO& board, std::string& debugMsg) const
{
  /// Checks that the board has the expected non-null patterns

  // This test only make sense when we have a self-trigger,
  // since in this case we expect to have a variable number of non-zero pattern
  // as indicated by the corresponding word.
  if (board.triggerWord != 0) {
    // This is a triggered event
    return true;
  }
  for (int ich = 0; ich < 4; ++ich) {
    bool isExpectedNull = (((board.firedChambers >> ich) & 0x1) == 0);
    bool isNull = (board.patternsBP[ich] == 0 && board.patternsNBP[ich] == 0);
    if (isExpectedNull != isNull) {
      std::stringstream ss;
      ss << "wrong size for local board:\n";
      ss << board << "\n";
      debugMsg += ss.str();
      return false;
    }
  }
  return true;
}

bool CRUBareDataChecker::checkLocalBoardSize(const std::vector<LocalBoardRO>& boards, std::string& debugMsg) const
{
  /// Checks that the boards have the expected non-null patterns
  for (auto& board : boards) {
    if (!checkLocalBoardSize(board, debugMsg)) {
      return false;
    }
  }
  return true;
}

bool CRUBareDataChecker::checkConsistency(const LocalBoardRO& board, std::string& debugMsg) const
{
  /// Checks that the event information is consistent

  bool isSoxOrReset = board.triggerWord & (raw::sSOX | raw::sEOX | raw::sRESET);
  bool isCalib = raw::isCalibration(board.triggerWord);
  bool isPhys = board.triggerWord & raw::sPHY;

  if (isPhys) {
    if (isCalib) {
      debugMsg += "inconsistent trigger: calibration and physics trigger cannot be fired together\n";
      return false;
    }
    if (raw::isLoc(board.statusWord)) {
      if (board.firedChambers) {
        debugMsg += "inconsistent trigger: fired chambers should be 0\n";
        return false;
      }
    }
  }
  if (isSoxOrReset && (isCalib || isPhys)) {
    debugMsg += "inconsistent trigger: cannot be SOX and calibration\n";
    return false;
  }

  return true;
}

bool CRUBareDataChecker::checkConsistency(const std::vector<LocalBoardRO>& boards, std::string& debugMsg) const
{
  /// Checks that the event information is consistent
  for (auto& board : boards) {
    if (!checkConsistency(board, debugMsg)) {
      std::stringstream ss;
      ss << board << "\n";
      debugMsg += ss.str();
      return false;
    }
  }
  return true;
}

bool CRUBareDataChecker::checkMasks(const std::vector<LocalBoardRO>& locs, std::string& debugMsg) const
{
  /// Checks the masks
  for (auto loc : locs) {
    // The board patterns coincide with the masks ("overwritten" mode)
    if (loc.statusWord & raw::sOVERWRITTEN) {
      auto maskItem = mMasks.find(loc.boardId);
      for (int ich = 0; ich < 4; ++ich) {
        uint16_t maskBP = 0;
        uint16_t maskNBP = 0;
        if (maskItem != mMasks.end()) {
          maskBP = maskItem->second.patternsBP[ich];
          maskNBP = maskItem->second.patternsNBP[ich];
        }
        if (maskBP != loc.patternsBP[ich] || maskNBP != loc.patternsNBP[ich]) {
          std::stringstream ss;
          ss << "Pattern is not compatible with mask for:\n";
          ss << loc << "\n";
          debugMsg += ss.str();
          return false;
        }
      }
    }
  }
  return true;
}

bool CRUBareDataChecker::checkRegLocConsistency(uint8_t crateId, const std::vector<LocalBoardRO>& regs, const std::vector<LocalBoardRO>& locs, std::string& debugMsg) const
{
  /// Checks consistency between local and regional info
  int expectedFired = 0;
  for (auto& reg : regs) {
    if (reg.triggerWord != 0) {
      // We received a trigger, so we expect all active boards to respond
      auto boardInfo = crateparams::getLocId(reg.boardId);
      auto crateId = crateparams::getCrateId(reg.boardId);
      auto regInCrate = boardInfo % 2;
      auto linkInCrate = boardInfo / 8;
      auto roId = crateparams::makeROId(crateId, linkInCrate);
      for (int iboard = 0; iboard < 4; ++iboard) {
        if (mCrateMasks.isActive(iboard + 4 * regInCrate, roId)) {
          ++expectedFired;
        }
      }
    } else {
      // This is the case of self-triggered events.
      // In this case the regional card tells us how many local cards to expect
      for (int iboard = 0; iboard < 4; ++iboard) {
        if (reg.firedChambers >> iboard & 0x1) {
          ++expectedFired;
        }
      }
    }
  }

  if (locs.size() != expectedFired) {
    bool busyRaised = false;
    for (int itype = 0; itype < 2; ++itype) {
      uint8_t nBoards = (itype == 0) ? 2 : crateparams::sMaxNBoardsInCrate;
      auto& busyFlag = (itype == 0) ? mBusyFlagReg : mBusyFlag;
      for (uint8_t iboard = 0; iboard < nBoards; ++iboard) {
        auto uniqueLocId = crateparams::makeUniqueLocID(crateId, iboard);
        auto busyItem = busyFlag.find(uniqueLocId);
        if (busyItem != busyFlag.end()) {
          if (busyItem->second) {
            busyRaised = true;
            break;
          }
        }
      }
    }
    if (!busyRaised) {
      // if (!busyRaised && locs.size() != 0) { // TODO: CHANGE
      // The busy flag is currently not correctly set.
      // This typically results in events with just the regional info
      // and no local info, without any warning.
      // Do not raise an error for the time being
      std::stringstream ss;
      ss << "loc-reg inconsistency: fired locals (" << locs.size() << ") != expected from reg (" << expectedFired << ");\n";
      ss << printBoards(regs);
      ss << printBoards(locs);
      debugMsg += ss.str();
      return false;
    }
  }
  return true;
}

std::string CRUBareDataChecker::printBoards(const std::vector<LocalBoardRO>& boards) const
{
  /// Prints the boards
  std::stringstream ss;
  for (auto& board : boards) {
    ss << board << "\n";
  }
  return ss.str();
}

bool CRUBareDataChecker::checkEvent(uint8_t crateId, const std::vector<LocalBoardRO>& regs, const std::vector<LocalBoardRO>& locs, bool isAffectedByEOX, std::string& debugMsg) const
{
  /// Checks the cards belonging to the same BC
  bool isOk = true;

  std::stringstream ss;

  if (!isAffectedByEOX && !checkRegLocConsistency(crateId, regs, locs, debugMsg)) {
    return false;
  }

  if (!checkLocalBoardSize(locs, debugMsg)) {
    return false;
  }

  if (!checkConsistency(regs, debugMsg) || !checkConsistency(locs, debugMsg)) {
    return false;
  }

  if (!checkMasks(locs, debugMsg)) {
    return false;
  }

  return true;
}

bool CRUBareDataChecker::process(gsl::span<const LocalBoardRO> localBoards, gsl::span<const ROFRecord> rofRecords, gsl::span<const ROFRecord> pageRecords)
{
  /// Checks the raw data

  bool isOk = true;
  mDebugMsg.clear();
  std::map<uint64_t, std::vector<size_t>> orderIndexes;
  InteractionRecord eoxReg;

  // First order data according to their BC
  for (auto rofIt = rofRecords.begin(); rofIt != rofRecords.end(); ++rofIt) {
    // Fill the map with ordered events
    orderIndexes[rofIt->interactionRecord.toLong()].emplace_back(rofIt - rofRecords.begin());

    // And compute the masks
    for (size_t iloc = rofIt->firstEntry; iloc < rofIt->firstEntry + rofIt->nEntries; ++iloc) {
      // Tests if this is a Start/End of trigger/continuous
      if ((localBoards[iloc].triggerWord & (raw::sSOX | raw::sEOX))) {
        if (localBoards[iloc].triggerWord & raw::sEOX) {
          eoxReg = rofIt->interactionRecord - 3;
        } else {
          eoxReg = InteractionRecord();
        }
        if (raw::isLoc(localBoards[iloc].statusWord)) {
          auto maskItem = mMasks.find(localBoards[iloc].boardId);
          // Check if we have already a mask for this
          if (maskItem == mMasks.end()) {
            // If not, read the map
            auto& mask = mMasks[localBoards[iloc].boardId];
            for (int ich = 0; ich < 4; ++ich) {
              mask.patternsBP[ich] = localBoards[iloc].patternsBP[ich];
              mask.patternsNBP[ich] = localBoards[iloc].patternsNBP[ich];
            }
          }
        }
      }
    }
  }

  // Then loop on the ordered BC
  std::map<uint16_t, GBT> gbtEvent;
  for (auto& item : orderIndexes) {
    gbtEvent.clear();
    std::vector<uint32_t> pageVec;
    bool isAffectedByEOX = (item.first >= eoxReg.toLong());
    if (rofRecords[item.second.front()].interactionRecord.bc > constants::lhc::LHCMaxBunches) {
      // In electronics simulations it can happen that the orbit frequency is small
      // and the local clock exceeds the maximum number of bunches per orbit
      // This leads to a wrong interaction record.
      // When this happens, it makes no sense to test if the current IR comes after the EOX.
      isAffectedByEOX = false;
    }
    for (auto& idx : item.second) {
      // In principle all of these ROF records have the same timestamp
      for (size_t iloc = rofRecords[idx].firstEntry; iloc < rofRecords[idx].firstEntry + rofRecords[idx].nEntries; ++iloc) {
        auto crateId = crateparams::getCrateId(localBoards[iloc].boardId);
        // We now order all of the cards in the event according to a unique ID made of:
        // - the crateID
        // - the trigger word
        // We are obliged to account for the trigger word because, when a trigger occurs,
        // all of the cards are expected to answer.
        // This can happen on top of the self-triggered event, leading to two separate events for one BC
        // In this way, each element in the map contains all of the boards per GBT link per event
        uint16_t id = (localBoards[iloc].triggerWord << 8) | crateId;
        bool isBusy = ((localBoards[iloc].statusWord & raw::sLOCALBUSY) != 0);
        if (raw::isLoc(localBoards[iloc].statusWord)) {
          gbtEvent[id].locs.push_back(localBoards[iloc]);
          mBusyFlag[localBoards[iloc].boardId] = isBusy;
        } else {
          gbtEvent[id].regs.push_back(localBoards[iloc]);
          mBusyFlagReg[localBoards[iloc].boardId] = isBusy;
        }
        // We then find out to what page this corresponds to.
        // This is useful for debug.
        for (auto& rofPage : pageRecords) {
          if (iloc >= rofPage.firstEntry && iloc < rofPage.firstEntry + rofPage.nEntries) {
            auto found = std::find(pageVec.begin(), pageVec.end(), rofPage.interactionRecord.orbit);
            if (found == pageVec.end()) {
              pageVec.emplace_back(rofPage.interactionRecord.orbit);
            }
            break;
          }
        }
      }
    } // loop on ROF indexes for this BC
    std::stringstream ss;
    ss << std::hex << std::showbase << rofRecords[item.second.front()].interactionRecord << "   [in";
    for (auto& page : pageVec) {
      ss << std::dec << "  page: " << page << "  (line: " << 512 * page + 1 << ")  ";
    }
    ss << "]\n";
    for (auto& gbtEvtItem : gbtEvent) {
      ++mStatistics[0];
      std::string debugStr;
      if (!checkEvent(gbtEvtItem.first & 0xff, gbtEvtItem.second.regs, gbtEvtItem.second.locs, isAffectedByEOX, debugStr)) {
        isOk = false;
        ss << debugStr << "\n";
        mDebugMsg += ss.str();
        ++mStatistics[1];
      }
    }
  }

  return isOk;
}

void CRUBareDataChecker::reset()
{
  /// Resets the masks and flags
  mMasks.clear();
  mBusyFlag.clear();
  mBusyFlagReg.clear();
  mStatistics.fill(0);
}

} // namespace mid
} // namespace o2
