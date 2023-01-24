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

/// \file   MIDFiltering/FiltererBC.h
/// \brief  BC filterer for MID
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   23 January 2023

#ifndef O2_MID_FILTERERBC_H
#define O2_MID_FILTERERBC_H

#include <vector>
#include <gsl/gsl>
#include "CommonDataFormat/BunchFilling.h"
#include "DataFormatsMID/ColumnData.h"
#include "DataFormatsMID/ROFRecord.h"
#include "MIDBase/ColumnDataHandler.h"

namespace o2
{
namespace mid
{
/// Filtering algorithm for MID
class FiltererBC
{
 public:
  /// @brief Filters the data BC
  /// @param data Digits
  /// @param rofRecords ROF records
  /// @param bcFilling Bunch filling
  void process(gsl::span<const ColumnData> data, gsl::span<const ROFRecord> rofRecords, const BunchFilling& bcFilling);

  /// @brief Returns the vector of filtered data
  /// @return Vector of filtered data
  const std::vector<ColumnData>& getData() { return mFilteredData; }

  /// @brief Returns the vector of ROF records
  /// @return Vector of ROF records
  const std::vector<ROFRecord>& getROFRecords() { return mFilteredROFs; }

  /// @brief Set the maximum BC diff in the lower side
  /// @param bcDiffLow maximum BC diff in the lower side (negative value)
  void setBCDiffLow(int bcDiffLow) { mBCDiffLow = bcDiffLow; }

  /// @brief Set the maximum BC diff in the upper side
  /// @param bcDiffHigh maximum BC diff in the upper side (positive value)
  void setBCDiffHigh(int bcDiffHigh) { mBCDiffHigh = bcDiffHigh; }

 private:
  /// @brief Returns the matched collision BC
  /// @param bc BC to be checked
  /// @param bcFilling Filling scheme
  /// @return The BC of the matched collision or -1 if there is no match
  int matchedCollision(int bc, const BunchFilling& bcFilling);

  int mBCDiffLow = 0;                      /// Maximum BC diff on the lower side
  int mBCDiffHigh = 0;                     /// Maximum BC diff on the higher side
  ColumnDataHandler mHandler;              /// Data handler
  std::vector<ColumnData> mFilteredData{}; /// Masked data
  std::vector<ROFRecord> mFilteredROFs{};  /// Masked data ROFs
};
} // namespace mid
} // namespace o2

#endif /* O2_MID_FILTERERBC_H */
