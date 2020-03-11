// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   MID/Raw/src/CrateMasks.cxx
/// \brief  MID crate masks
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   11 March 2020

#include "MIDRaw/CrateMasks.h"

#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>

namespace o2
{
namespace mid
{

bool CrateMasks::load(const char* filename)
{
  /// Loads the masks from a configuration file
  /// The file is in the form:
  /// feeId mask
  /// with one line per link
  /// The mask is at most 8 bits, since each GBT link reads at most 8 local boards
  mActiveBoards.fill(0);
  std::ifstream inFile(filename);
  if (!inFile.is_open()) {
    return false;
  }
  std::string line, token;
  while (std::getline(inFile, line)) {
    if (std::count(line.begin(), line.end(), ' ') < 1) {
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
    uint8_t mask = std::atoi(token.c_str());
    mActiveBoards[feeId] = mask;
  }
  inFile.close();
  return true;
}

void CrateMasks::write(const char* filename) const
{
  /// Writes the masks to a configuration file
  std::ofstream outFile(filename);
  for (uint16_t igbt = 0; igbt < crateparams::sNGBTs; ++igbt) {
    outFile << igbt << " " << mActiveBoards[igbt] << std::endl;
  }
  outFile.close();
}

CrateMasks createDefaultCrateMasks()
{
  /// Creates the default crate masks
  CrateMasks config;
  for (uint16_t ioffset = 0; ioffset < crateparams::sNGBTs; ioffset += crateparams::sNGBTsPerSide) {
    // Crate 1
    config.setActiveBoards(0 + ioffset, 0xFF);
    config.setActiveBoards(1 + ioffset, 0xFF);

    // Crate 2
    config.setActiveBoards(2 + ioffset, 0xFF);
    config.setActiveBoards(3 + ioffset, 0x7F);

    // Crate 2-3
    config.setActiveBoards(4 + ioffset, 0x7F);
    config.setActiveBoards(5 + ioffset, 0x7F);

    // Crate 3
    config.setActiveBoards(6 + ioffset, 0xFF);
    config.setActiveBoards(7 + ioffset, 0x7F);

    // Crate 4
    config.setActiveBoards(8 + ioffset, 0xFF);
    config.setActiveBoards(9 + ioffset, 0xFF);

    // Crate 5
    config.setActiveBoards(10 + ioffset, 0xFF);
    config.setActiveBoards(11 + ioffset, 0xFF);

    // Crate 6
    config.setActiveBoards(12 + ioffset, 0xFF);
    config.setActiveBoards(13 + ioffset, 0xFF);

    // Crate 7
    config.setActiveBoards(14 + ioffset, 0xFF);
    config.setActiveBoards(15 + ioffset, 0x1);
  }
  return config;
}

} // namespace mid
} // namespace o2
