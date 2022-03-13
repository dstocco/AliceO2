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

/// \file   MID/TestingSimTools/src/ClustersBuilder.cxx
/// \brief  Build clusters from MID track
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   13 January 2022

#include "MIDTestingSimTools/ClustersBuilder.h"

namespace o2
{
namespace mid
{
ClustersBuilder::ClustersBuilder(const HitFinder& hitFinder)
  : mHitFinder(hitFinder)
{
}

bool buildClusters(Track& track, std::vector<Cluster>& clusters) const
{
  Cluster cl;
  bool isFired = false;
  double sqrt3 = std::sqrt(3.);
  for (int ich = 0; ich < 4; ++ich) {
    auto hits = helper.hitFinder.getLocalPositions(track, ich);
    track.setClusterMatchedUnchecked(ich, -1);
    for (auto& hit : hits) {
      // Check the bending plane
      auto stripIndex = mMapping.stripByPosition(hit.xCoor, hit.yCoor, 0, hit.deId, false);
      if (!stripIndex.isValid()) {
        continue;
      }
      cl.deId = hit.deId;
      auto area = helper.mapping.stripByLocation(stripIndex.strip, 0, stripIndex.line, stripIndex.column, deId);
      cl.yCoor = area.getCenterY();
      cl.yErr = area.getHalfSizeY() / sqrt3;
      // Check the non-bending plane
      stripIndex = helper.mapping.stripByPosition(hit.xCoor, hit.yCoor, 1, hit.deId, false);
      area = helper.mapping.stripByLocation(stripIndex.strip, 1, stripIndex.line, stripIndex.column, deId);
      cl.xCoor = area.getCenterX();
      cl.xErr = area.getHalfSizeX() / sqrt3;
      clusters.emplace_back(cl);
      track.setClusterMatchedUnchecked(ich, clusters.size() - 1);
      isFired = true;
    } // loop on fired pos
  }
  return isFired;
}

} // namespace mid
} // namespace o2
