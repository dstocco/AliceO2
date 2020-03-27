// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   MIDQC/CRUBareDataChecker.h
/// \brief  Class to check the bare data from the CRU
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   9 December 2019
#ifndef O2_MID_CRUBAREDATACHECKER_H
#define O2_MID_CRUBAREDATACHECKER_H

#include <cstdint>
#include <vector>
#include <string>
#include <map>
#include <gsl/gsl>
#include "DataFormatsMID/ROFRecord.h"
#include "MIDRaw/CrateMasks.h"
#include "MIDRaw/LocalBoardRO.h"

namespace o2
{
namespace mid
{
class CRUBareDataChecker
{
 public:
  bool process(gsl::span<const LocalBoardRO> localBoards, gsl::span<const ROFRecord> rofRecords, gsl::span<const ROFRecord> pageRecords);
  /// Gets the number of processed events
  unsigned int getNBCsProcessed() const { return mStatistics[0]; }
  /// Gets the number of faulty events
  unsigned int getNBCsFaulty() const { return mStatistics[1]; }
  /// Gets the
  std::string getDebugMessage() const { return mDebugMsg; }
  void reset();

  /// Sets the crate masks
  void setCrateMasks(const CrateMasks& masks) { mCrateMasks = masks; }

 private:
  struct Mask {
    std::array<uint16_t, 4> patternsBP{};  /// Bending plane mask
    std::array<uint16_t, 4> patternsNBP{}; /// Non-bending plane mask
  };

  struct GBT {
    std::vector<LocalBoardRO> regs{}; /// Regional boards
    std::vector<LocalBoardRO> locs{}; /// Local boards
  };

  bool checkEvent(uint8_t crateId, const std::vector<LocalBoardRO>& regs, const std::vector<LocalBoardRO>& locs, bool isAffectedByEOX, std::string& debugMsg) const;
  bool checkConsistency(const LocalBoardRO& board, std::string& debugMsg) const;
  bool checkConsistency(const std::vector<LocalBoardRO>& boards, std::string& debugMsg) const;
  bool checkMasks(const std::vector<LocalBoardRO>& locs, std::string& debugMsg) const;
  bool checkLocalBoardSize(const LocalBoardRO& board, std::string& debugMsg) const;
  bool checkLocalBoardSize(const std::vector<LocalBoardRO>& boards, std::string& debugMsg) const;
  bool checkRegLocConsistency(uint8_t crateId, const std::vector<LocalBoardRO>& regs, const std::vector<LocalBoardRO>& locs, std::string& debugMsg) const;
  std::string printBoards(const std::vector<LocalBoardRO>& boards) const;

  std::string mDebugMsg{};                        /// Debug message
  std::array<unsigned long int, 2> mStatistics{}; /// Processed events statistics
  std::map<uint8_t, Mask> mMasks;                 /// Masks
  std::map<uint8_t, bool> mBusyFlag;              /// Busy flag
  std::map<uint8_t, bool> mBusyFlagReg;           /// Busy flag for regional cards
  CrateMasks mCrateMasks{};                       /// Crate masks
};
} // namespace mid
} // namespace o2

#endif /* O2_MID_CRUBAREDATACHECKER_H */
