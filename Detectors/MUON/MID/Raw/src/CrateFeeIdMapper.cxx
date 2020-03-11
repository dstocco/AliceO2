// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   MID/Raw/src/CrateFeeIdMapper.cxx
/// \brief  Hardware Id to FeeId mapper
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   11 March 2020

#include "MIDRaw/CrateFeeIdMapper.h"

#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include "MIDRaw/CrateParameters.h"

namespace o2
{
namespace mid
{
uint16_t CrateFeeIdMapper::getFeeId(uint8_t linkId, uint8_t endPointId, uint16_t cruId)
{
  /// Gets the feeId from the physical ID of the link
  auto id = getId(linkId, endPointId, cruId);
  auto feeId = mGBTIdToFeeId.find(id);
  if (feeId == mGBTIdToFeeId.end()) {
    return 0xFFFF;
  }
  return feeId->second;
}

bool CrateFeeIdMapper::load(const char* filename)
{
  /// Loads the FEE Ids from a configuration file
  /// The file is in the form:
  /// feeId linkId endPointId cruId
  /// with one line per link

  mGBTIdToFeeId.clear();
  std::ifstream inFile(filename);
  if (!inFile.is_open()) {
    return false;
  }
  std::string line, token;
  while (std::getline(inFile, line)) {
    if (std::count(line.begin(), line.end(), ' ') < 3) {
      continue;
    }
    if (line.find('#') < line.find(' ')) {
      continue;
    }
    std::stringstream ss;
    ss << line;
    std::getline(ss, token, ' ');
    uint16_t feeId = std::atoi(token.c_str());
    std::getline(ss, token, ' ');
    uint8_t linkId = std::atoi(token.c_str());
    std::getline(ss, token, ' ');
    uint8_t endPointId = std::atoi(token.c_str());
    std::getline(ss, token, ' ');
    uint16_t cruId = std::atoi(token.c_str());
    mGBTIdToFeeId[getId(linkId, endPointId, cruId)] = feeId;
  }
  inFile.close();
  return true;
}

void CrateFeeIdMapper::write(const char* filename) const
{
  /// Writes the FEE Ids to a configuration file
  std::ofstream outFile(filename);
  for (auto& id : mGBTIdToFeeId) {
    outFile << id.first << " " << (id.second & 0xFF) << " " << ((id.second >> 8) & 0xFF) << " " << ((id.second >> 16) & 0xFF) << std::endl;
  }
  outFile.close();
}

CrateFeeIdMapper createDefaultCrateFeeIdMapper()
{
  /// Creates the default FeeId mapper
  CrateFeeIdMapper config;
  for (uint16_t iside = 0; iside < 2; ++iside) {
    for (uint8_t igbt = 0; igbt < crateparams::sNGBTsPerSide; ++igbt) {
      config.setFeeId(igbt + crateparams::sNGBTsPerSide * iside, igbt % 12, igbt / 12, iside);
    }
  }
  return config;
}

} // namespace mid
} // namespace o2
