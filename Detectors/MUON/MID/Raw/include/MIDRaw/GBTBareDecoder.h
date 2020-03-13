// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   MIDRaw/GBTBareDecoder.h
/// \brief  MID GBT decoder without user logic
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   12 March 2020
#ifndef O2_MID_GBTBAREDECODER_H
#define O2_MID_GBTBAREDECODER_H

#include <cstdint>
#include <vector>
#include <gsl/gsl>
#include "Headers/RAWDataHeader.h"
#include "DataFormatsMID/ROFRecord.h"
#include "MIDRaw/CrateParameters.h"
#include "MIDRaw/ELinkDecoder.h"
#include "MIDRaw/LocalBoardRO.h"

namespace o2
{
namespace mid
{
class GBTBareDecoder
{
 public:
  void init(uint16_t feeId, uint8_t mask, bool debugMode = false);
  void process(gsl::span<const uint8_t> bytes, const header::RAWDataHeader& rdh);
  /// Gets the vector of data
  const std::vector<LocalBoardRO>& getData() const { return mData; }

  /// Gets the vector of data RO frame records
  const std::vector<ROFRecord>& getROFRecords() const { return mROFRecords; }

  bool isComplete() const;

 private:
  std::vector<LocalBoardRO> mData{};                                      /// Vector of output data
  std::vector<ROFRecord> mROFRecords{};                                   /// List of ROF records
  uint16_t mFeeId{0};                                                     /// FEE ID
  uint8_t mMask{0xFF};                                                    /// GBT mask
  InteractionRecord mIRFirstPage{};                                       /// Interaction record of the first page
  std::array<uint16_t, crateparams::sNELinksPerGBT> mCalibClocks{};       /// Calibration clock
  std::array<ELinkDecoder, crateparams::sNELinksPerGBT> mELinkDecoders{}; /// E-link decoders
  std::array<InteractionRecord, crateparams::sNELinksPerGBT> mIRs{};      /// Interaction records per link
  std::array<uint16_t, crateparams::sNELinksPerGBT> mLastClock{};         /// Last clock per link

  std::function<void(size_t, size_t)> mProcessLoc{std::bind(&GBTBareDecoder::processLoc, this, std::placeholders::_1, std::placeholders::_2)}; ///! Processes the local board
  std::function<void(size_t, size_t)> mProcessReg{[](size_t, size_t) {}};                                                                      ///! Processes the regional board

  void processHalfReg(size_t idx, int halfReg, const gsl::span<const uint8_t>& bytes);
  void reset();
  void addBoard(size_t ilink);
  void addLoc(size_t ilink);
  bool checkLoc(size_t ilink);
  bool feedLoc(size_t ilink, uint8_t byte);
  bool feedReg(size_t ilink, uint8_t byte);
  void processLoc(size_t ilink, uint8_t byte);
  void processLocDebug(size_t ilink, uint8_t byte);
  void processRegDebug(size_t ilink, uint8_t byte);
  bool updateIR(size_t ilink);
  bool invertPattern(LocalBoardRO& loc);
};
} // namespace mid
} // namespace o2

#endif /* O2_MID_GBTBAREDECODER_H */
