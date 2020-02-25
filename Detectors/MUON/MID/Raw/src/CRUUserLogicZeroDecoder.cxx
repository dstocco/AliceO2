// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   MID/Raw/src/CRUUserLogicZeroDecoder.cxx
/// \brief  MID CRU decoder for User Logic with Zero Suppression only
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   24 February 2020

#include "MIDRaw/CRUUserLogicZeroDecoder.h"

#include "RawInfo.h"

namespace o2
{
namespace mid
{

void CRUUserLogicZeroDecoder::reset()
{
  /// Rewind bytes
  mData.clear();
  mROFRecords.clear();
}

void CRUUserLogicZeroDecoder::init(bool debugMode)
{
  /// Initializes the task
  if (debugMode == true) {
    mProcessReg = std::bind(&CRUUserLogicZeroDecoder::processRegDebug, this, std::placeholders::_1);
    mProcessLoc = std::bind(&CRUUserLogicZeroDecoder::processLocDebug, this, std::placeholders::_1);
  }
}

void CRUUserLogicZeroDecoder::process(gsl::span<const uint8_t> bytes)
{
  /// Decodes the buffer
  reset();

  mBuffer.setBuffer(bytes);
  mBuffer.nextHeader();

  if (mIRs[0].isDummy()) {
    for (auto& ir : mIRs) {
      ir.orbit = mBuffer.getRDH()->triggerOrbit;
      ir.bc = mBuffer.getRDH()->triggerBC;
    }
    for (auto& clock : mLastClock) {
      clock = constants::lhc::LHCMaxBunches;
    }
  }

  if (mBuffer.getRDH()->pageCnt == 0) {
    // FIXME: in the tests, the BC counter increases at each RDH
    // However, the inner clock of the RO is not reset,
    // so if we want to have the absolute BC,
    // we need to store only the BC counter of the first page.
    // Not sure how it will work on data...
    mIRFirstPage.orbit = mBuffer.getRDH()->triggerOrbit;
    mIRFirstPage.bc = mBuffer.getRDH()->triggerBC;
  }

  while (mBuffer.nextPayload()) {
    // Each CRU word consists of 256 bits, i.e 32 bytes
    size_t nWords = (mBuffer.getRDH()->memorySize - mBuffer.getRDH()->headerSize) / 32;
    for (size_t iword = 0; iword < nWords; ++iword) {
      while (processCard())
        ;
    }
  }
}

bool CRUUserLogicZeroDecoder::processCard()
{
  /// Processes the GBT
  for (size_t ib = 0; ib < 5; ++ib) {
    mELinkDecoder.add(mBuffer.next());
  }
  if (mELinkDecoder.getStatusWord() == 0) {
    return false;
  }
  while (!mELinkDecoder.isComplete()) {
    mELinkDecoder.add(mBuffer.next());
  }

  if (raw::isLoc(mELinkDecoder.getStatusWord())) {
    mProcessLoc(mELinkDecoder.getId());
  } else {
    mProcessReg(8 + mELinkDecoder.getId());
  }
  return true;
}

void CRUUserLogicZeroDecoder::addBoard(size_t ilink)
{
  /// Adds the local or regional board to the output data vector
  uint16_t localClock = mELinkDecoder.getCounter();
  EventType eventType = EventType::Standard;
  if (mELinkDecoder.getTriggerWord() & raw::sCALIBRATE) {
    mCalibClocks[ilink] = localClock;
    eventType = EventType::Noise;
  } else if (localClock == mCalibClocks[ilink] + sDelayCalibToFET) {
    eventType = EventType::Dead;
  }
  auto firstEntry = mData.size();
  mData.push_back({mELinkDecoder.getStatusWord(), mELinkDecoder.getTriggerWord(), crateparams::makeUniqueLocID(mCrateId, mELinkDecoder.getId()), mELinkDecoder.getInputs()});
  InteractionRecord intRec(mIRs[ilink].bc + localClock - sDelayBCToLocal, mIRs[ilink].orbit);
  mROFRecords.emplace_back(intRec, eventType, firstEntry, 1);
}

void CRUUserLogicZeroDecoder::addLoc(size_t ilink)
{
  /// Adds the local board to the output data vector
  addBoard(ilink);
  for (int ich = 0; ich < 4; ++ich) {
    if ((mData.back().firedChambers & (1 << ich))) {
      mData.back().patternsBP[ich] = mELinkDecoder.getPattern(0, ich);
      mData.back().patternsNBP[ich] = mELinkDecoder.getPattern(1, ich);
    }
  }
  if (mROFRecords.back().eventType == EventType::Dead) {
    if (invertPattern(mData.back())) {
      mData.pop_back();
      mROFRecords.pop_back();
    }
  }
}

bool CRUUserLogicZeroDecoder::updateIR(size_t ilink)
{
  /// Updates the interaction record for the link
  if (mELinkDecoder.getTriggerWord() & raw::sORB) {
    // This is the answer to an orbit trigger
    // The local clock is reset: we are now in synch with the new HB
    mIRs[ilink] = mIRFirstPage;
    if (!(mELinkDecoder.getTriggerWord() & (raw::sSOX | raw::sEOX))) {
      mLastClock[ilink] = mELinkDecoder.getCounter();
    }
    return true;
  }
  return false;
}

void CRUUserLogicZeroDecoder::processLoc(size_t ilink)
{
  /// Processes the local board information
  if (updateIR(ilink)) {
    return;
  }
  addLoc(ilink);
}

void CRUUserLogicZeroDecoder::processLocDebug(size_t ilink)
{
  /// Processes the local board information in debug mode
  /// This always adds the local board to the output, without performing tests
  updateIR(ilink);
  addLoc(ilink);
}

void CRUUserLogicZeroDecoder::processRegDebug(size_t ilink)
{
  /// Processes the regional board information in debug mode
  updateIR(ilink);
  addBoard(ilink);
  // The board creation is optimized for the local boards, not the regional
  // (which are transmitted only in debug mode).
  // So, at this point, for the regional board, the local Id is actually the crate ID.
  // If we want to distinguish the two regional e-links, we can use the link ID instead
  mData.back().boardId = crateparams::makeUniqueLocID(crateparams::getLocId(mData.back().boardId), ilink / 5);
  if (mData.back().triggerWord == 0) {
    if (mROFRecords.back().interactionRecord.bc < sDelayRegToLocal) {
      // In the tests, the HB does not really correspond to a change of orbit
      // So we need to keep track of the last clock at which the HB was received
      // and come back to that value
      // FIXME: Remove this part as well as mLastClock when tests are no more needed
      mROFRecords.back().interactionRecord -= (constants::lhc::LHCMaxBunches - mLastClock[ilink] - 1);
    }
    // This is a self-triggered event.
    // In this case the regional card needs to wait to receive the tracklet decision of each local
    // which result in a delay that needs to be subtracted if we want to be able to synchronize
    // local and regional cards for the checks
    mROFRecords.back().interactionRecord -= sDelayRegToLocal;
  }
}

bool CRUUserLogicZeroDecoder::invertPattern(LocalBoardRO& loc)
{
  /// Gets the proper pattern
  for (int ich = 0; ich < 4; ++ich) {
    loc.patternsBP[ich] = ~loc.patternsBP[ich];
    loc.patternsNBP[ich] = ~loc.patternsNBP[ich];
    if (loc.patternsBP[ich] == 0 && loc.patternsNBP[ich] == 0) {
      loc.firedChambers &= ~(1 << ich);
    }
  }
  return (loc.firedChambers == 0);
}

} // namespace mid
} // namespace o2
