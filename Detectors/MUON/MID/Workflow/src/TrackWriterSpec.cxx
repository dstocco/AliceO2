// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   MID/Workflow/src/TrackWriterSpec.cxx
/// \brief  Writer spec for MID tracks
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   13 June 2019

#include "MIDWorkflow/TrackWriterSpec.h"

#include <fstream>

#include "Framework/DataProcessorSpec.h"
#include "Framework/ControlService.h"
#include "Framework/Output.h"
#include "Framework/Task.h"

#include "DataFormatsMID/Cluster3D.h"
#include "DataFormatsMID/Track.h"
namespace of = o2::framework;

namespace o2
{
namespace mid
{

class TrackWriterDeviceDPL
{
 public:
  TrackWriterDeviceDPL(const char* inputTracksBinding, const char* inputClustersBinding) : mInputTracksBinding(inputTracksBinding), mInputClustersBinding(inputClustersBinding), mTracksFile(nullptr), mClustersFile(nullptr), mState(0){};
  ~TrackWriterDeviceDPL() = default;

  void init(o2::framework::InitContext& ic)
  {
    auto filename = ic.options().get<std::string>("mid-track-outfile");
    mTracksFile = std::make_unique<std::ofstream>(filename.c_str(), std::ios::binary);
    if (!mTracksFile->is_open()) {
      LOG(ERROR) << "Cannot open " << filename;
      mState = 1;
      return;
    }

    filename = ic.options().get<std::string>("mid-clusters-outfile");
    mClustersFile = std::make_unique<std::ofstream>(filename.c_str(), std::ios::binary);
    if (!mTracksFile->is_open()) {
      LOG(ERROR) << "Cannot open " << filename;
      mState = 1;
      return;
    }

    mState = 0;

    auto stop = [this]() {
      /// close the output file
      LOG(INFO) << "Stop track sink";
      this->mTracksFile.close();
    };
    ic.services().get<CallbackService>().set(CallbackService::Id::Stop, stop);
  }

  void run(o2::framework::ProcessingContext& pc)
  {
    if (mState != 0) {
      return;
    }

    auto msgTracks = pc.inputs().get(mInputTracksBinding.c_str());
    gsl::span<const Track> tracks = of::DataRefUtils::as<const Track>(msgTracks);
    mTracksFile->write((char*)&tracks[0], tracks.size() * sizeof(Track));
    LOG(INFO) << "Wrote " << tracks.size() << " tracks";

    auto msgClusters = pc.inputs().get(mInputClustersBinding.c_str());
    gsl::span<const Cluster3D> clusters = of::DataRefUtils::as<const Cluster3D>(msgClusters);
    mClustersFile->write((char*)&clusters[0], clusters.size() * sizeof(Cluster3D));
    LOG(INFO) << "Wrote " << tracks.size() << " clusters";
  }

 private:
  std::string mInputTracksBinding;
  std::string mInputClustersBinding;
  std::unique_ptr<std::ofstream> mTracksFile = nullptr;
  std::unique_ptr<std::ofstream> mClustersFile = nullptr;
  int mState = 0;
}; // namespace mid

framework::DataProcessorSpec getTrackWriterSpec()
{

  std::string inputTracksBinding = "mid_tracks";
  std::string inputClustersBinding = "mid_trackclusters";

  std::vector<of::InputSpec> inputSpecs{
    of::InputSpec{inputTracksBinding, "MID", "TRACKS_DATA"},
    of::InputSpec{inputClustersBinding, "MID", "TRCLUS_DATA"}};

  return of::DataProcessorSpec{
    "MIDTrackWriter",
    {inputSpecs},
    {},
    of::adaptFromTask<o2::mid::TracksWriterDeviceDPL>(inputTracksBinding.c_str(), inputClustersBinding.c_str())};
}
} // namespace mid
} // namespace o2