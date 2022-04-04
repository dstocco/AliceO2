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

/// \file   MIDRaw/DecodedDataAggregator.h
/// \brief  MID decoded raw data aggregator
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   26 February 2020
#ifndef O2_MID_DECODEDDATAAGGREGATOR_H
#define O2_MID_DECODEDDATAAGGREGATOR_H

#include <vector>
#include <map>
#include <gsl/gsl>
#include "DataFormatsMID/ColumnData.h"
#include "DataFormatsMID/ROBoard.h"
#include "DataFormatsMID/ROFRecord.h"
#include "MIDRaw/CrateMapper.h"

namespace o2
{
namespace mid
{
class DecodedDataAggregator
{
 public:
  /// Aggregates the decoded raw data
  /// \param localBoards Vector of local boards
  /// \param rofRecords Vector of ROF records
  void process(gsl::span<const ROBoard> localBoards, gsl::span<const ROFRecord> rofRecords);

  /// Gets the vector of data per event
  /// \param eventType Event type
  /// \return Vector of column data
  const std::vector<ColumnData>& getData(EventType eventType) const { return mData[static_cast<int>(eventType)]; }

  /// Gets the vector of data with all event types
  std::vector<ColumnData> getData() const;

  /// Gets the vector of data RO frame records
  /// \param eventType Event type
  /// \return Vector of ROF records
  const std::vector<ROFRecord>& getROFRecords(EventType eventType) const { return mROFRecords[static_cast<int>(eventType)]; }

  /// Gets the vector of ROF records with all event types
  std::vector<ROFRecord> getROFRecords() const;

 private:
  /// Converts the local board data to ColumnData
  /// \param col Local board
  /// \param firstEntry Index of the first ColumnData in the event
  /// \param evtTypeIdx Event type as size_t
  void addData(const ROBoard& col, size_t firstEntry, size_t evtTypeIdx);
  /// Gets the matching column data. Adds one if not found
  /// \param deId Detection element ID
  /// \param deId Column ID
  /// \param firstEntry Index of the first ColumnData in the event
  /// \param evtTypeIdx Event type as size_t
  /// \return Matching column data or new one if not found
  ColumnData& FindColumnData(uint8_t deId, uint8_t columnId, size_t firstEntry, size_t evtTypeIdx);

  std::array<std::map<uint64_t, std::vector<size_t>>, 3> mEventIndexes{}; /// Event indexes

  std::array<std::vector<ColumnData>, 3> mData{};      /// Vector of output column data
  std::array<std::vector<ROFRecord>, 3> mROFRecords{}; /// Vector of ROF records
  CrateMapper mCrateMapper;                            /// Mapper to convert the RO info to ColumnData
};
} // namespace mid
} // namespace o2

#endif /* O2_MID_DECODEDDATAAGGREGATOR_H */
