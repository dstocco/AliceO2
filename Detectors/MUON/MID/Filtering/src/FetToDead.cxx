// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   MID/Filtering/src/FetToDead.cxx
/// \brief  Class to convert the FEE test event into dead channels
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   10 May 2021

#include "MIDFiltering/FetToDead.h"

#include "MIDFiltering/MaskMaker.h"

namespace o2
{
namespace mid
{
FetToDead::FetToDead()
{
  /// Default ctr.
  mMasks = makeDefaultMasks();
}

void FetToDead::process(gsl::span<const ColumnData> fetData)
{
  /// Invert the patterns
  mInvertedData.clear();
  for (auto& col : fetData) {
    invertPattern(col);
  }
}

bool FetToDead::isEmpty(const ColumnData& col) const
{
  /// Returns true if all patterns are 0
  if (col.getNonBendPattern()) {
    return false;
  }
  for (int iline = 0; iline < 4; ++iline) {
    if (col.getBendPattern(iline)) {
      return false;
    }
  }
  return true;
}

bool FetToDead::invertPattern(const ColumnData& col)
{
  /// Inverts the pattern and add it to the output data
  ColumnData invertedCol;
  invertedCol.setNonBendPattern(~col.getNonBendPattern());
  for (int iline = 0; iline < 4; ++iline) {
    invertedCol.setBendPattern(~col.getBendPattern(iline), iline);
  }
  mMasks.applyMask(invertedCol);
  if (isEmpty(invertedCol)) {
    return true;
  }
  invertedCol.deId = col.deId;
  invertedCol.columnId = col.columnId;
  mInvertedData.emplace_back(invertedCol);
  return true;
}

} // namespace mid
} // namespace o2
