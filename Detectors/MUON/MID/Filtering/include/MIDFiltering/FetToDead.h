// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   MIDFiltering/FetToDead.h
/// \brief  Class to convert the FEE test event into dead channels
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   10 May 2021
#ifndef O2_MID_FETTODEAD_H
#define O2_MID_FETTODEAD_H

#include <cstdint>
#include <gsl/span>
#include "DataFormatsMID/ColumnData.h"
#include "MIDFiltering/ChannelMasks.h"

namespace o2
{
namespace mid
{

class FetToDead
{
 public:
  FetToDead();
  ~FetToDead() = default;

  FetToDead(const FetToDead&) = default;
  FetToDead(FetToDead&&) = default;

  void process(gsl::span<const ColumnData> fetData);

  /// Retuns the inverted patterns
  const std::vector<ColumnData>& getData() { return mInvertedData; }

  void setMask(const ChannelMasks& mask) { mMasks = mask; }

 private:
  bool isEmpty(const ColumnData& col) const;
  bool invertPattern(const ColumnData& col);

  ChannelMasks mMasks{};                   // Default mask
  std::vector<ColumnData> mInvertedData{}; // Inverted data
};

} // namespace mid
} // namespace o2

#endif /* O2_MID_FETTODEAD_H */
