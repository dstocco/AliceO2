// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   MID/Raw/src/CRUBareDecoder.cxx
/// \brief  MID CRU core decoder
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   30 September 2019

#include "MIDRaw/CRUBareDecoder.h"

#include "RawInfo.h"

namespace o2
{
namespace mid
{

void CRUBareDecoder::reset()
{
  /// Rewind bytes
  mData.clear();
  mROFRecords.clear();
}

void CRUBareDecoder::init(const CrateFeeIdMapper& feeIdMapper, const CrateMasks& masks, bool debugMode)
{
  /// Initializes the task
  mCrateFeeIdMapper = feeIdMapper;
  for (uint16_t igbt = 0; igbt < crateparams::sNGBTs; ++igbt) {
    mGBTDecoders[igbt].init(igbt, masks.getMask(igbt), debugMode);
  }
}

void CRUBareDecoder::process(gsl::span<const uint8_t> bytes)
{
  /// Decodes the buffer
  reset();

  mHandler.setBuffer(bytes);
  while (mHandler.nextNonEmptyHBF()) {
    uint16_t feeId = mCrateFeeIdMapper.getFeeId(mHandler.getRDH()->linkID, mHandler.getRDH()->endPointID, mHandler.getRDH()->cruID);
    mGBTDecoders[feeId].process(mHandler.getPayload(), *mHandler.getRDH());
    size_t firstEntry = mData.size();
    mData.insert(mData.end(), mGBTDecoders[feeId].getData().begin(), mGBTDecoders[feeId].getData().end());
    size_t lastRof = mROFRecords.size();
    mROFRecords.insert(mROFRecords.end(), mGBTDecoders[feeId].getROFRecords().begin(), mGBTDecoders[feeId].getROFRecords().end());
    for (auto rofIt = mROFRecords.begin() + lastRof; rofIt != mROFRecords.end(); ++rofIt) {
      rofIt->firstEntry += firstEntry;
    }
  }
}

bool CRUBareDecoder::isComplete() const
{
  /// Checks that all links have finished reading
  for (auto& decoder : mGBTDecoders) {
    if (!decoder.isComplete()) {
      return false;
    }
  }
  return true;
}

} // namespace mid
} // namespace o2
