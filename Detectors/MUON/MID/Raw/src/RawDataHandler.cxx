// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   MID/Raw/src/RawDataHandler.cxx
/// \brief  Handler of the RAW data buffer
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   13 March 2020

#include "MIDRaw/RawDataHandler.h"

#include "MIDRaw/RawUnit.h"
#include "RawInfo.h"

namespace o2
{
namespace mid
{

template <typename T>
bool RawDataHandler<T>::nextHBF()
{
  /// Goes to next HBF
  if (mNextRDHIndex >= mBytes.size()) {
    return false;
  }
  mRDHIndex = mNextRDHIndex;
  mRDH = reinterpret_cast<const header::RAWDataHeader*>(&mBytes[mRDHIndex]);
  mPayloadIndex = mRDHIndex + mRDH->headerSize / mElementSizeInBytes;
  mNextRDHIndex = mRDHIndex + mRDH->offsetToNext / mElementSizeInBytes;
  return true;
}

template <typename T>
bool RawDataHandler<T>::nextNonEmptyHBF()
{
  /// Goes to next non-empty HBF
  bool isOk = nextHBF();
  while (isOk && isEmptyNoCheck()) {
    isOk = nextHBF();
  }
  return isOk;
}

template <typename T>
void RawDataHandler<T>::reset()
{
  /// Rewind bytes
  mRDHIndex = 0;
  mNextRDHIndex = 0;
  mPayloadIndex = 0;
  mRDH = nullptr;
}

template <typename T>
void RawDataHandler<T>::setBuffer(gsl::span<const T> bytes)
{
  /// Sets the buffer and reset the internal indexes
  reset();
  mBytes = bytes;
}

template class RawDataHandler<raw::RawUnit>;
template class RawDataHandler<uint8_t>;

} // namespace mid
} // namespace o2
