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

/// \file   MIDTestingSimTools/ClustersBuilder.h
/// \brief  Build clusters from MID track
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   13 January 2022

#ifndef O2_MID_CLUSTERSBUILDER_H
#define O2_MID_CLUSTERSBUILDER_H

#include "DataFormatsMID/Cluster.h"
#include "DataFormatsMID/Track.h"
#include "MIDBase/GeometryTransformer.h"

namespace o2
{
namespace mid
{
/// Class to find the impact point of a track on the chamber
class ClustersBuilder
{
 public:
  /// default constructor
  ClustersBuilder(const HitFinder& hitFinder);
  /// default destructor
  ~ClustersBuilder() = default;

  /// Builds the clusters for the track
  /// \param track Input track. Notice that it will be modified to add the corresponding cluster index
  /// \param clusters Vector of clusters to which the track clusters will be appended
  /// \param return true if new clusters where added
  bool buildClusters(Track& track, std::vector<Cluster>& clusters) const;

 private:
  HitFinder mHitFinder; ///< Hit finder
} // namespace mid
} // namespace o2

#endif /* O2_MID_CLUSTERSBUILDER_H */
