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
  mGBTProcessorActive.fill(0);
  for (auto& decoder : mGBTDecoders) {
    decoder.reset();
  }
}

void CRUBareDecoder::init(const CrateFeeIdMapper& feeIdMapper, const CrateMasks& masks, bool debugMode)
{
  /// Initializes the task
  mCrateFeeIdMapper = feeIdMapper;
  for (uint16_t igbt = 0; igbt < crateparams::sNGBTs; ++igbt) {
    mGBTDecoders[igbt].init(igbt, masks.getMask(igbt), debugMode);
  }
}

void CRUBareDecoder::process(gsl::span<const type> bytes)
{
  /// Decodes the buffer
  reset();

  mHandler.setBuffer(bytes);
  uint32_t dummyId = 0xFFFFFFFF;
  uint32_t previusId = dummyId;
  uint32_t uniqueId = dummyId;
  size_t lastIndex = 0;
  while (mHandler.nextNonEmptyHBF()) {
    uniqueId = mCrateFeeIdMapper.getUniqueId(mHandler.getRDH()->linkID, mHandler.getRDH()->endPointID, mHandler.getRDH()->cruID);
    if (uniqueId != previusId && lastIndex > 0) {
      uint16_t feeId = mCrateFeeIdMapper.getFeeId(uniqueId);
      // Add data if needed
      addData(feeId);
      // Launch processor in a separate thread
      mGBTProcessors[feeId] = std::async(std::launch::async, &CRUBareDecoder::processGBT, this, feeId, bytes.subspan(lastIndex, mHandler.getRDHIndex() - lastIndex));
      mGBTProcessorActive[feeId] = true;
      lastIndex = mHandler.getRDHIndex();
    }
    previusId = uniqueId;
  }

  // Process last one
  if (uniqueId != dummyId) {
    // If unique ID is dummy it means that the buffer was empty
    uint16_t feeId = mCrateFeeIdMapper.getFeeId(uniqueId);
    // Add data if needed
    addData(feeId);
    // Launch processor in a separate thread
    mGBTProcessors[feeId] = std::async(std::launch::async, &CRUBareDecoder::processGBT, this, feeId, bytes.subspan(lastIndex));
    mGBTProcessorActive[feeId] = true;
  }

  for (uint16_t igbt = 0; igbt < crateparams::sNGBTs; ++igbt) {
    // Add remaining data
    addData(igbt);
  }
}

bool CRUBareDecoder::addData(uint16_t feeId)
{
  if (mGBTProcessorActive[feeId]) {
    // Terminate the previous processing
    mGBTProcessors[feeId].get();
  } else {
    return false;
  }
  size_t firstEntry = mData.size();
  mData.insert(mData.end(), mGBTDecoders[feeId].getData().begin(), mGBTDecoders[feeId].getData().end());
  size_t lastRof = mROFRecords.size();
  mROFRecords.insert(mROFRecords.end(), mGBTDecoders[feeId].getROFRecords().begin(), mGBTDecoders[feeId].getROFRecords().end());
  for (auto rofIt = mROFRecords.begin() + lastRof; rofIt != mROFRecords.end(); ++rofIt) {
    rofIt->firstEntry += firstEntry;
  }
  return true;
}

void CRUBareDecoder::processGBT(uint16_t feeId, gsl::span<const type> bytes)
{
  /// Decodes the GBT buffer
  RawDataHandler<type> handler;
  handler.setBuffer(bytes);
  while (handler.nextNonEmptyHBF()) {
    mGBTDecoders[feeId].process(handler.getPayload(), *handler.getRDH());
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
