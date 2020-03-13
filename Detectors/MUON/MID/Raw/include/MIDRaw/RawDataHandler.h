// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   MIDRaw/RawDataHandler.h
/// \brief  Handler of the RAW data buffer
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   13 March 2020
#ifndef O2_MID_RAWDATAHANDLER_H
#define O2_MID_RAWDATAHANDLER_H

#include <cstdint>
#include <vector>
#include <gsl/gsl>
#include "Headers/RAWDataHeader.h"

namespace o2
{
namespace mid
{
template <typename T>
class RawDataHandler
{
 public:
  void setBuffer(gsl::span<const T> bytes);

  bool nextHBF();

  bool nextNonEmptyHBF();

  /// Gets the current RDH
  const header::RAWDataHeader* getRDH() { return mRDH; }

  /// Gets the payload
  gsl::span<const T> getPayload() const { return mBytes.subspan(mPayloadIndex, (mRDH->memorySize - mRDH->headerSize) / mElementSizeInBytes); }

  /// Tests if the HBF has no payload
  bool isEmpty() const { return mRDH ? isEmptyNoCheck() : true; }

  /// Tests if the HB is closed
  bool isHBClosed() { return mRDH ? mRDH->stop : false; }

 private:
  inline bool isEmptyNoCheck() const { return mRDH->memorySize == mRDH->headerSize; }

  gsl::span<const T> mBytes{};                       /// gsl span with encoded information
  size_t mRDHIndex{0};                               /// Index of the current RDH
  size_t mNextRDHIndex{0};                           /// Index of the next RDH
  size_t mPayloadIndex{0};                           /// Index of the payload
  const unsigned int mElementSizeInBytes{sizeof(T)}; /// Element size in bytes
  const header::RAWDataHeader* mRDH{nullptr};        /// Current header (not owner)

  void reset();
};
} // namespace mid
} // namespace o2

#endif /* O2_MID_RAWDATAHANDLER_H */
