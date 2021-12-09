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

/// \file   MID/Clustering/src/PreClusterHelper.cxx
/// \brief  Implementation of the pre-clusters for MID
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   16 April 2019
#include "MIDClustering/PreClusterHelper.h"

namespace o2
{
namespace mid
{

MpArea PreClusterHelper::getArea(const PreCluster& pc) const
{
  /// The method can also return the full area in the NBP
  /// However, in this case the area is always correct only in x.
  /// On the other hand, for the cut RPC the area is not well defined
  /// if the pre-cluster touches the column with the missing board.
  /// In this case we return the maximum rectangle (that includes the missing board)
  MpArea first = mMapping.stripByLocation(pc.firstStrip, pc.cathode, pc.firstLine, pc.firstColumn, pc.deId, false);
  float firstX = first.getXmin();
  float firstY = first.getYmin();
  float lastX = 0., lastY = 0.;
  if (pc.cathode == 0) {
    int nStripsInBetween = pc.lastStrip - pc.firstStrip + 16 * (pc.lastLine - pc.firstLine);
    lastX = first.getXmax();
    lastY = first.getYmax() + nStripsInBetween * mMapping.getStripSize(0, 0, pc.firstColumn, pc.deId);
  } else {
    MpArea last = mMapping.stripByLocation(pc.lastStrip, pc.cathode, 0, pc.lastColumn, pc.deId, false);
    lastX = last.getXmax();
    lastY = (last.getYmax() > first.getYmax()) ? last.getYmax() : first.getYmax();
    if (firstY > last.getYmin()) {
      firstY = last.getYmin();
    }
  }
  return MpArea{firstX, firstY, lastX, lastY};
}

MpArea PreClusterHelper::getArea(int column, const PreCluster& pc) const
{

  if (column > pc.lastColumn || column < pc.firstColumn) {
    throw std::runtime_error("Required column is not in pre-cluster");
  }
  int firstStrip = (column > pc.firstColumn) ? 0 : pc.firstStrip;
  int lastStrip = (column < pc.lastColumn) ? mMapping.getNStripsNBP(column, pc.deId) - 1 : pc.lastStrip;

  MpArea first = mMapping.stripByLocation(firstStrip, 1, 0, column, pc.deId, false);
  MpArea last = mMapping.stripByLocation(lastStrip, 1, 0, column, pc.deId, false);
  return MpArea{first.getXmin(), first.getYmin(), last.getXmax(), last.getYmax()};
}

std::vector<int> PreClusterHelper::getBoardIds(const PreCluster& pc) const
{
  std::vector<int> locIds;
  for (int icol = pc.firstColumn; icol <= pc.lastColumn; ++icol) {
    for (int iline = pc.firstLine; iline <= pc.lastLine; ++iline) {
      locIds.emplace_back(mMapping.getBoardId(iline, icol, pc.deId));
    }
  }
  return locIds;
}

} // namespace mid
} // namespace o2
