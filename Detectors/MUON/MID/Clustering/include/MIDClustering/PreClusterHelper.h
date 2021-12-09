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

/// \file   MIDClustering/PreClusterHelper.h
/// \brief  Pre-clusters helper for MID
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   16 April 2019

#ifndef O2_MID_PRECLUSTERHELPER_H
#define O2_MID_PRECLUSTERHELPER_H

#include <vector>
#include "MIDBase/MpArea.h"
#include "MIDBase/Mapping.h"
#include "MIDClustering/PreCluster.h"

namespace o2
{
namespace mid
{
class PreClusterHelper
{
 public:
  /// Gets the area of the pre-cluster in the Bending Plane
  /// \param pc Pre-cluster
  MpArea getArea(const PreCluster& pc) const;

  /// Gets the area of the pre-cluster in the Non-Bending Plane in the specified column
  /// \param column column ID
  /// \param pc Pre-cluster
  MpArea getArea(int column, const PreCluster& pc) const;

  /// Gets the list of the local boards
  /// \param pc Pre-cluster
  std::vector<int> getBoardIds(const PreCluster& pc) const;

 private:
  Mapping mMapping; ///< Mapping
};
} // namespace mid
} // namespace o2

#endif /* O2_MID_PRECLUSTERHELPER_H */
