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

/// \file   MIDFiltering/Filterer.h
/// \brief  Mask data
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   14 May 2021

#ifndef O2_MID_FILTERER_H
#define O2_MID_FILTERER_H

#include <vector>
#include <gsl/gsl>
#include "DataFormatsMID/ColumnData.h"
#include "DataFormatsMID/ROFRecord.h"
#include "MIDFiltering/ChannelMasksHandler.h"

namespace o2
{
namespace mid
{
/// Filtering algorithm for MID
class Filterer
{
 public:
  void process(gsl::span<const ColumnData> data, gsl::span<const ROFRecord> rofRecords);

  /// Gets the vector of masked data
  const std::vector<ColumnData>& getData() { return mMaskedData; }

  /// Gets the vector of masked data RO frame records
  const std::vector<ROFRecord>& getROFRecords() { return mMaskedROFs; }

  /// Sets the masks
  void setMasks(const ChannelMasksHandler& masks) { mMasks = masks; }

 private:
  ChannelMasksHandler mMasks{};          /// Channel masks
  std::vector<ColumnData> mMaskedData{}; /// Masked data
  std::vector<ROFRecord> mMaskedROFs{};  /// Masked data ROFs
};
} // namespace mid
} // namespace o2

#endif /* O2_MID_FILTERER_H */
