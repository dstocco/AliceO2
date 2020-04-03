// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   MID/Raw/src/GBTBareDecoder.cxx
/// \brief  MID GBT decoder without user logic
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   12 March 2020

#include "MIDRaw/GBTBareDecoder.h"

#include "RawInfo.h"

namespace o2
{
namespace mid
{

void GBTBareDecoder::reset()
{
  /// Rewind bytes
  mData.clear();
  mROFRecords.clear();
}

void GBTBareDecoder::init(uint16_t feeId, uint8_t mask, bool debugMode)
{
  /// Initializes the task
  mFeeId = feeId;
  mMask = mask;
  if (debugMode == true) {
    mOnDoneLoc = &GBTBareDecoder::onDoneLocDebug;
    mProcessReg = &GBTBareDecoder::processRegDebug;
  }
}

void GBTBareDecoder::process(gsl::span<const uint8_t> bytes, const header::RAWDataHeader& rdh)
{
  /// Decodes the buffer
  // reset();

  if (mIRs[0].isDummy()) {
    for (auto& ir : mIRs) {
      ir.orbit = rdh.triggerOrbit;
      ir.bc = rdh.triggerBC;
    }
    for (auto& clock : mLastClock) {
      clock = constants::lhc::LHCMaxBunches;
    }
    for (auto& clock : mCalibClocks) {
      clock = constants::lhc::LHCMaxBunches;
    }
  }

  if (rdh.pageCnt == 0) {
    // FIXME: in the tests, the BC counter increases at each RDH
    // However, the inner clock of the RO is not reset,
    // so if we want to have the absolute BC,
    // we need to store only the BC counter of the first page.
    // Not sure how it will work on data...
    mIRFirstPage.orbit = rdh.triggerOrbit;
    mIRFirstPage.bc = rdh.triggerBC;
  }

  uint8_t byte = 0;
  size_t ilink = 0, linkMask = 0, byteOffset = 0;

  for (size_t idx = 0; idx < bytes.size(); idx += 16) {
    for (int ireg = 0; ireg < 2; ++ireg) {
      byteOffset = idx + 5 * ireg;
      for (int ib = 0; ib < 4; ++ib) {
        byte = bytes[byteOffset + ib];
        ilink = ib + 4 * ireg;
        linkMask = (1 << ilink);
        if ((mMask & linkMask) && ((mIsFeeding & linkMask) || byte)) {
          processLoc(ilink, byte);
        }
      } // loop on locs
      byte = bytes[byteOffset + 4];
      ilink = 8 + ireg;
      linkMask = (1 << ilink);
      if ((mIsFeeding & linkMask) || byte) {
        std::invoke(mProcessReg, this, ilink, byte);
      }
    } // loop on half regional
  }   // loop on buffer index
}

void GBTBareDecoder::processHalfReg(size_t idx, int halfReg, const gsl::span<const uint8_t>& bytes)
{
  /// Processes the information of one regional and 4 local boards

  size_t linkOffset = 4 * halfReg;
  size_t byteOffset = idx + 5 * halfReg;

  // local links
  for (int ib = 0; ib < 4; ++ib) {
    processLoc(linkOffset + ib, bytes[byteOffset + ib]);
  }

  // Regional link
  std::invoke(mProcessReg, this, 8 + halfReg, bytes[byteOffset + 4]);
}

bool GBTBareDecoder::checkLoc(size_t ilink)
{
  /// Performs checks on the local board
  return (ilink == mELinkDecoders[ilink].getId() % 8);
}

void GBTBareDecoder::addBoard(size_t ilink)
{
  /// Adds the local or regional board to the output data vector
  uint16_t localClock = mELinkDecoders[ilink].getCounter();
  EventType eventType = EventType::Standard;
  if (mELinkDecoders[ilink].getTriggerWord() & raw::sCALIBRATE) {
    mCalibClocks[ilink] = localClock;
    eventType = EventType::Noise;
  } else if (localClock == mCalibClocks[ilink] + sDelayCalibToFET) {
    eventType = EventType::Dead;
  }
  auto firstEntry = mData.size();
  mData.push_back({mELinkDecoders[ilink].getStatusWord(), mELinkDecoders[ilink].getTriggerWord(), crateparams::makeUniqueLocID(crateparams::getCrateIdFromROId(mFeeId), mELinkDecoders[ilink].getId()), mELinkDecoders[ilink].getInputs()});
  InteractionRecord intRec(mIRs[ilink].bc + localClock - sDelayBCToLocal, mIRs[ilink].orbit);
  mROFRecords.emplace_back(intRec, eventType, firstEntry, 1);
}

void GBTBareDecoder::addLoc(size_t ilink)
{
  /// Adds the local board to the output data vector
  addBoard(ilink);
  for (int ich = 0; ich < 4; ++ich) {
    if ((mData.back().firedChambers & (1 << ich))) {
      mData.back().patternsBP[ich] = mELinkDecoders[ilink].getPattern(0, ich);
      mData.back().patternsNBP[ich] = mELinkDecoders[ilink].getPattern(1, ich);
    }
  }
  if (mROFRecords.back().eventType == EventType::Dead) {
    if (invertPattern(mData.back())) {
      mData.pop_back();
      mROFRecords.pop_back();
    }
  }
}

bool GBTBareDecoder::updateIR(size_t ilink)
{
  /// Updates the interaction record for the link
  if (mELinkDecoders[ilink].getTriggerWord() & raw::sORB) {
    // This is the answer to an orbit trigger
    // The local clock is reset: we are now in synch with the new HB
    mIRs[ilink] = mIRFirstPage;
    if (!(mELinkDecoders[ilink].getTriggerWord() & (raw::sSOX | raw::sEOX))) {
      mLastClock[ilink] = mELinkDecoders[ilink].getCounter();
    }
    return true;
  }
  return false;
}

void GBTBareDecoder::onDoneLoc(size_t ilink)
{
  /// Performs action on decoded local board
  if (updateIR(ilink)) {
    return;
  }
  if (checkLoc(ilink)) {
    addLoc(ilink);
  }
}

void GBTBareDecoder::onDoneLocDebug(size_t ilink)
{
  /// Performs action on decoded local board in debug mode.
  /// This always adds the local board to the output, without performing tests
  updateIR(ilink);
  addLoc(ilink);
}

void GBTBareDecoder::processLoc(size_t ilink, uint8_t byte)
{
  /// Processes the local board information
  // if ((mMask & (1 << ilink)) == 0) {
  //   return;
  // }
  if (mELinkDecoders[ilink].getNBytes() > 0) {
    mELinkDecoders[ilink].add(byte);
    if (mELinkDecoders[ilink].isComplete()) {
      std::invoke(mOnDoneLoc, this, ilink);
      mELinkDecoders[ilink].reset();
      mIsFeeding &= (~(1 << ilink));
    }
  } else if ((byte & (raw::sSTARTBIT | raw::sCARDTYPE)) == (raw::sSTARTBIT | raw::sCARDTYPE)) {
    mELinkDecoders[ilink].add(byte);
    mIsFeeding |= (1 << ilink);
  }
}

void GBTBareDecoder::processRegDebug(size_t ilink, uint8_t byte)
{
  /// Processes the regional board information in debug mode
  if (mELinkDecoders[ilink].getNBytes() > 0) {
    mELinkDecoders[ilink].add(byte);
    if (mELinkDecoders[ilink].isComplete()) {
      updateIR(ilink);
      addBoard(ilink);
      // The board creation is optimized for the local boards, not the regional
      // (which are transmitted only in debug mode).
      // So, at this point, for the regional board, the local Id is actually the crate ID.
      // If we want to distinguish the two regional e-links, we can use the link ID instead
      mData.back().boardId = crateparams::makeUniqueLocID(crateparams::getLocId(mData.back().boardId), ilink + 8 * (crateparams::getGBTIdInCrate(mFeeId) - 1));
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
      mELinkDecoders[ilink].reset();
      mIsFeeding &= (~(1 << ilink));
    }
  } else if (byte & raw::sSTARTBIT) {
    mELinkDecoders[ilink].add(byte);
    mIsFeeding |= (1 << ilink);
  }
}

bool GBTBareDecoder::invertPattern(LocalBoardRO& loc)
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

bool GBTBareDecoder::isComplete() const
{
  /// Checks that all links have finished reading
  for (auto& elink : mELinkDecoders) {
    if (elink.getNBytes() > 0) {
      return false;
    }
  }
  return true;
}

} // namespace mid
} // namespace o2
