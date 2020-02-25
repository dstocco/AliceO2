// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   MIDRaw/CRUUserLogicZeroDecoder.h
/// \brief  MID CRU decoder for User Logic with Zero Suppression only
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   24 February 2020
#ifndef O2_MID_CRUUSERLOGICZERODECODER_H
#define O2_MID_CRUUSERLOGICZERODECODER_H

#include <cstdint>
#include <vector>
#include <gsl/gsl>
#include "DataFormatsMID/ROFRecord.h"
#include "MIDRaw/CrateParameters.h"
#include "MIDRaw/ELinkDecoder.h"
#include "MIDRaw/LocalBoardRO.h"
#include "MIDRaw/RawBuffer.h"

namespace o2
{
namespace mid
{
class CRUUserLogicZeroDecoder
{
 public:
  using type = uint8_t;
  void init(bool debugMode = false);
  void process(gsl::span<const type> bytes);
  /// Gets the vector of data
  const std::vector<LocalBoardRO>& getData() const { return mData; }

  /// Gets the vector of data RO frame records
  const std::vector<ROFRecord>& getROFRecords() const { return mROFRecords; }

 private:
  RawBuffer<type> mBuffer{};                                         /// Raw buffer handler
  std::vector<LocalBoardRO> mData{};                                 /// Vector of output data
  std::vector<ROFRecord> mROFRecords{};                              /// List of ROF records
  uint8_t mCrateId{0};                                               /// Crate ID
  InteractionRecord mIRFirstPage{};                                  /// Interaction record of the first page
  ELinkDecoder mELinkDecoder{};                                      /// E-link decoder
  std::array<uint16_t, crateparams::sNELinksPerGBT> mCalibClocks{};  /// Calibration clock
  std::array<InteractionRecord, crateparams::sNELinksPerGBT> mIRs{}; /// Interaction records per link
  std::array<uint16_t, crateparams::sNELinksPerGBT> mLastClock{};    /// Last clock per link

  std::function<void(size_t)> mProcessLoc{std::bind(&CRUUserLogicZeroDecoder::processLoc, this, std::placeholders::_1)}; ///! Processes the local board
  std::function<void(size_t)> mProcessReg{[](size_t) {}};                                                                ///! Processes the regional board

  bool processCard();
  void reset();
  void addBoard(size_t ilink);
  void addLoc(size_t ilink);
  bool checkLoc(size_t ilink);
  void processLoc(size_t ilink);
  void processLocDebug(size_t ilink);
  void processRegDebug(size_t ilink);
  bool updateIR(size_t ilink);
  bool invertPattern(LocalBoardRO& loc);
};
} // namespace mid
} // namespace o2

#endif /* O2_MID_CRUUSERLOGICZERODECODER_H */
