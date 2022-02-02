// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   MID/Filtering/src/Filterer.cxx
/// \brief  Mask data
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   14 May 2021

#include "MIDFiltering/Filterer.h"

namespace o2
{
namespace mid
{

void Filterer::process(gsl::span<const ColumnData> data, gsl::span<const ROFRecord> rofRecords)
{
  /// Masks data
  mMaskedROFs.clear();
  mMaskedData.clear();
  size_t removed = 0;
  for (auto& rof : rofRecords) {
    size_t rofRemoved = 0;
    for (auto dataIt = data.begin() + rof.firstEntry, end = data.begin() + rof.getEndIndex(); dataIt != end; ++dataIt) {
      auto col = *dataIt;
      if (mMasks.applyMask(col)) {
        // Data are not fully masked
        mMaskedData.emplace_back(col);
      } else {
        ++rofRemoved;
      }
    }
    if (rof.nEntries > rofRemoved) {
      // If this condition is not satisfied it means that all data of this ROF was removed
      auto maskedRof = rof;
      maskedRof.nEntries -= rofRemoved;
      maskedRof.firstEntry -= removed;
      mMaskedROFs.emplace_back(maskedRof);
    }
    removed += rofRemoved;
  }
}

} // namespace mid
} // namespace o2
