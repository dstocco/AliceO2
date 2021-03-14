// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   MIDFiltering/Filterer.h
/// \brief  Mask data
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   05 January 2020

#ifndef O2_MID_FILTERER_H
#define O2_MID_FILTERER_H

#include <vector>
#include <gsl/gsl>
#include "DataFormatsMID/ColumnData.h"
#include "DataFormatsMID/ROFRecord.h"

namespace o2
{
namespace mid
{
/// Filtering algorithm for MID
class Filterer
{
 public:
  void process(gsl::span<const ColumnData> data, gsl::span<const ROFRecord> rofRecords, std::vector<ColumnData>& maskedData, std::vector<ROFRecord>& maskedROFs);

 private:
  ChannelMasks mMasks;
};
} // namespace mid
} // namespace o2

#endif /* O2_MID_FILTERER_H */
