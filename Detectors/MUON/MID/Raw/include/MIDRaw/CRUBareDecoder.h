// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   MIDRaw/CRUBareDecoder.h
/// \brief  MID CRU core decoder
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   18 November 2019
#ifndef O2_MID_CRUBAREDECODER_H
#define O2_MID_CRUBAREDECODER_H

#include <cstdint>
#include <vector>
#include <gsl/gsl>
#include "DataFormatsMID/ROFRecord.h"
#include "MIDRaw/CrateFeeIdMapper.h"
#include "MIDRaw/CrateMasks.h"
#include "MIDRaw/CrateParameters.h"
#include "MIDRaw/GBTBareDecoder.h"
#include "MIDRaw/LocalBoardRO.h"
#include "MIDRaw/RawDataHandler.h"

namespace o2
{
namespace mid
{
class CRUBareDecoder
{
 public:
  using type = uint8_t;
  void init(const CrateFeeIdMapper& feeIdMapper, const CrateMasks& masks, bool debugMode = false);
  void process(gsl::span<const type> bytes);
  /// Gets the vector of data
  const std::vector<LocalBoardRO>& getData() const { return mData; }

  /// Gets the vector of data RO frame records
  const std::vector<ROFRecord>& getROFRecords() const { return mROFRecords; }

  bool isComplete() const;

 private:
  RawDataHandler<type> mHandler{};                              /// Raw data handler
  std::vector<LocalBoardRO> mData{};                            /// Vector of output data
  std::vector<ROFRecord> mROFRecords{};                         /// List of ROF records
  std::array<GBTBareDecoder, crateparams::sNGBTs> mGBTDecoders; /// GBT bare decoders
  CrateFeeIdMapper mCrateFeeIdMapper{};                         /// Crate FEEID mapper

  void reset();
};
} // namespace mid
} // namespace o2

#endif /* O2_MID_CRUBAREDECODER_H */
