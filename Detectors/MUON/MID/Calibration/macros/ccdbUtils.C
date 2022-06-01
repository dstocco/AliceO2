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

/// \file   MID/Calibration/macros/ccdbUtils.cxx
/// \brief  Retrieve or upload MID calibration objects
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   16 May 2022

#include <string>
#include <string_view>
#include <map>
#include <vector>
#include "TObjString.h"
#include "CCDB/CcdbApi.h"
#include "DataFormatsMID/ColumnData.h"

const std::string BadChannelCCDBPath = "MID/Calib/BadChannels";

void queryBadChannels(const std::string ccdbUrl, long timestamp, bool verbose)
{
  o2::ccdb::CcdbApi api;
  api.init(ccdbUrl);
  std::map<std::string, std::string> metadata;
  auto* badChannels = api.retrieveFromTFileAny<std::vector<o2::mid::ColumnData>>(BadChannelCCDBPath.c_str(), metadata, timestamp);
  if (!badChannels) {
    std::cout << "Error: cannot find " << BadChannelCCDBPath << " in " << ccdbUrl << std::endl;
    return;
  }
  std::cout << "number of bad channels = " << badChannels->size() << std::endl;
  if (verbose) {
    for (const auto& badChannel : *badChannels) {
      std::cout << badChannel << "\n";
    }
  }
}

std::string queryDCSMasks(const std::string ccdbUrl, long timestamp, bool verbose)
{
  std::string maskCCDBPath = "MID/Calib/ElectronicsMasks";
  o2::ccdb::CcdbApi api;
  api.init(ccdbUrl);
  std::map<std::string, std::string> metadata;
  auto* masks = api.retrieveFromTFileAny<TObjString>(maskCCDBPath.c_str(), metadata, timestamp);
  if (!masks) {
    std::cout << "Error: cannot find " << maskCCDBPath << " in " << ccdbUrl << std::endl;
    return "";
  }
  if (verbose) {
    std::cout << masks->GetName() << "\n";
  }
  return masks->GetName();
}

void writeDCSMasks(const std::string ccdbUrl, long timestamp, std::string_view outFilename = "masks.txt")
{
  auto masks = queryDCSMasks(ccdbUrl, timestamp, false);
  std::ofstream outFile(outFilename);
  if (!outFile.is_open()) {
    std::cout << "Error: cannot write to file " << outFilename << std::endl;
    return;
  }
  outFile << masks << std::endl;
  outFile.close();
}

void uploadBadChannels(const std::string ccdbUrl, long timestamp)
{
  std::vector<o2::mid::ColumnData> badChannels;

  o2::ccdb::CcdbApi api;
  api.init(ccdbUrl);
  std::map<std::string, std::string> md;
  std::cout << "storing default MID bad channels (valid from " << timestamp << ") to " << BadChannelCCDBPath << "\n";

  api.storeAsTFileAny(&badChannels, BadChannelCCDBPath, md, timestamp, o2::ccdb::CcdbObjectInfo::INFINITE_TIMESTAMP);
}

void ccdbUtils(const std::string_view what, long timestamp = 0, bool verbose = false, const std::string ccdbUrl = "http://ccdb-test.cern.ch:8080")
{
  if (timestamp == 0) {
    timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
  }

  std::vector<std::string> whats = {"querybad", "uploadbad", "querymasks", "writemasks"};

  if (what == whats[0]) {
    queryBadChannels(ccdbUrl, timestamp, verbose);
  } else if (what == whats[1]) {
    uploadBadChannels(ccdbUrl, timestamp);
  } else if (what == whats[2]) {
    queryDCSMasks(ccdbUrl, timestamp, verbose);
  } else if (what == whats[3]) {
    writeDCSMasks(ccdbUrl, timestamp);
  } else {
    std::cout << "Unimplemented option chosen " << what << std::endl;
    std::cout << "Available:\n";
    for (auto& str : whats) {
      std::cout << str << " ";
    }
    std::cout << std::endl;
  }
}
