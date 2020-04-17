// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   MID/QC/exe/raw-ul-decoder-checker.cxx
/// \brief  Compares the user logic decoder output with the raw input
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   09 December 2019

#include <cstdint>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include "boost/program_options.hpp"
#include "MIDRaw/FEEIdConfig.h"
#include "MIDRaw/CrateMasks.h"
#include "MIDRaw/Decoder.h"
#include "MIDRaw/GBTBareDecoder.h"
#include "MIDRaw/GBTUserLogicDecoder.h"
#include "MIDRaw/RawFileReader.h"

namespace po = boost::program_options;

template <class DECODER>
bool readFile(std::string filename, DECODER& decoder, std::vector<o2::mid::LocalBoardRO>& data, std::vector<o2::mid::ROFRecord>& rofRecords)
{
  std::cout << "Reading " << filename << std::endl;
  data.clear();
  rofRecords.clear();
  o2::mid::RawFileReader rawFileReader;
  if (!rawFileReader.init(filename.c_str())) {
    std::cout << "Cannot initialize file reader with " << filename << std::endl;
    return false;
  }
  while (rawFileReader.readHB(false)) {
    decoder.process(rawFileReader.getData());
    rawFileReader.clear();
    size_t offset = data.size();
    data.insert(data.end(), decoder.getData().begin(), decoder.getData().end());
    for (auto& rof : decoder.getROFRecords()) {
      rofRecords.emplace_back(rof.interactionRecord, rof.eventType, rof.firstEntry + offset, rof.nEntries);
    }
  }
  if (data.empty()) {
    std::cout << "No data found in " << filename << std::endl;
    return false;
  }
  return true;
}

std::unordered_map<uint64_t, std::vector<size_t>> getOrderedIndexes(const std::vector<o2::mid::ROFRecord>& rofRecords)
{
  // Order data according to their IR
  std::unordered_map<uint64_t, std::vector<size_t>> orderIndexes;
  for (auto rofIt = rofRecords.begin(); rofIt != rofRecords.end(); ++rofIt) {
    // Fill the map with ordered events
    orderIndexes[rofIt->interactionRecord.toLong()].emplace_back(rofIt->firstEntry);
  }
  return orderIndexes;
}

std::unordered_map<uint16_t, std::vector<size_t>> getIndexesPerBoard(const std::vector<o2::mid::LocalBoardRO>& data, const std::vector<o2::mid::ROFRecord>& rofRecords, bool isLoc)
{
  std::unordered_map<uint16_t, std::vector<size_t>> indexes;
  for (auto rofIt = rofRecords.begin(); rofIt != rofRecords.end(); ++rofIt) {
    auto& loc = data[rofIt->firstEntry];
    if (isLoc == o2::mid::raw::isLoc(loc.statusWord)) {
      indexes[loc.boardId].emplace_back(rofIt->firstEntry);
    }
  }
  return indexes;
}

o2::InteractionRecord findIR(uint64_t irLong, const std::vector<o2::mid::ROFRecord>& rofRecords)
{
  for (auto& rof : rofRecords) {
    if (rof.interactionRecord.toLong() == irLong) {
      return rof.interactionRecord;
    }
  }
  return o2::InteractionRecord();
}

bool isSame(const o2::mid::LocalBoardRO& loc1, const o2::mid::LocalBoardRO& loc2)
{
  if (loc1.statusWord == loc2.statusWord && loc1.triggerWord == loc2.triggerWord && loc1.firedChambers == loc2.firedChambers && loc1.boardId == loc2.boardId) {
    for (int ich = 0; ich < 4; ++ich) {
      if (loc1.patternsBP[ich] != loc2.patternsBP[ich] || loc1.patternsNBP[ich] != loc2.patternsNBP[ich]) {
        return false;
      }
    }
    return true;
  }
  return false;
}

std::string printIRHex(const o2::InteractionRecord& ir)
{
  std::stringstream ss;
  ss << std::hex << std::showbase << ir;
  return ss.str();
}

bool checkBoards(const std::vector<o2::mid::LocalBoardRO>& bareData, const std::vector<o2::mid::ROFRecord>& bareRofs, const std::vector<o2::mid::LocalBoardRO>& ulData, const std::vector<o2::mid::ROFRecord>& ulRofs, bool isLoc, std::ofstream& out)
{
  auto bareIndexes = getIndexesPerBoard(bareData, bareRofs, isLoc);
  auto ulIndexes = getIndexesPerBoard(ulData, ulRofs, isLoc);
  bool isOk = true;
  for (auto& bareItem : bareIndexes) {
    auto ulItem = ulIndexes.find(bareItem.first);
    if (ulItem == ulIndexes.end()) {
      out << "\nCannot find " << printIRHex(bareRofs[bareItem.second.front()].interactionRecord) << " in ul";
      isOk = false;
      continue;
    }
    auto ulIt = ulItem->second.begin();
    auto lastOk = ulItem->second.end();
    bool isCurrentOk = true;
    for (auto bareIt = bareItem.second.begin(); bareIt != bareItem.second.end(); ++bareIt) {
      if (ulIt == ulItem->second.end()) {
        out << "\nNo more ul from: " << bareData[*bareIt];
        isCurrentOk = false;
      } else if (!isSame(ulData[*ulIt], bareData[*bareIt]) || ulRofs[*ulIt].interactionRecord != bareRofs[*bareIt].interactionRecord) {
        out << "\nFirst divergence at element " << bareIt - bareItem.second.begin() + 1 << " / " << bareItem.second.size() << ":" << std::endl;
        out << "bare: " << printIRHex(bareRofs[*bareIt].interactionRecord) << std::endl;
        out << bareData[*bareIt] << std::endl;
        out << "ul: " << printIRHex(ulRofs[*ulIt].interactionRecord) << std::endl;
        out << ulData[*ulIt] << std::endl;
        isCurrentOk = false;
      }
      if (!isCurrentOk) {
        if (lastOk != ulItem->second.end()) {
          out << "lastOk: " << printIRHex(ulRofs[*lastOk].interactionRecord) << std::endl;
          out << ulData[*lastOk] << std::endl;
        } else {
          out << "lastOk: none. This is the first event!" << std::endl;
        }
        isOk = false;
        break;
      }
      lastOk = ulIt;
      ++ulIt;
    }
  }
  return isOk;
}

bool checkAll(const std::vector<o2::mid::LocalBoardRO>& bareData, const std::vector<o2::mid::ROFRecord>& bareRofs, const std::vector<o2::mid::LocalBoardRO>& ulData, const std::vector<o2::mid::ROFRecord>& ulRofs, std::ofstream& out)
{
  auto bareIndexes = getOrderedIndexes(bareRofs);
  auto ulIndexes = getOrderedIndexes(ulRofs);

  bool isOk = true;
  for (auto& bareItem : bareIndexes) {
    auto ir = findIR(bareItem.first, bareRofs);
    auto ulItem = ulIndexes.find(bareItem.first);
    if (ulItem == ulIndexes.end()) {
      isOk = false;
      out << "\nCannot find: " << ir << " in ul\n";
      continue;
    }
    std::vector<size_t> auxVec = ulItem->second;
    for (auto& idx1 : bareItem.second) {
      bool found = false;
      for (auto auxIt = auxVec.begin(); auxIt != auxVec.end(); ++auxIt) {
        if (isSame(bareData[idx1], ulData[*auxIt])) {
          auxVec.erase(auxIt);
          found = true;
          break;
        }
      }
      if (!found) {
        isOk = false;
        out << "\nOnly in bare: " << ir << "\n";
        out << "  " << bareData[idx1] << "\n";
      }
    }
    for (auto& idx2 : auxVec) {
      isOk = false;
      out << "\nOnly in ul: " << ir << "\n";
      out << "  " << ulData[idx2] << "\n";
    }
  }

  for (auto& ulItem : ulIndexes) {
    auto bareItem = bareIndexes.find(ulItem.first);
    if (bareItem == bareIndexes.end()) {
      isOk = false;
      out << "\nCannot find: " << findIR(ulItem.first, ulRofs) << " in bare\n";
    }
  }
  return isOk;
}

int main(int argc, char* argv[])
{
  po::variables_map vm;
  po::options_description generic("Generic options");
  std::string ulFilename, bareFilename, feeIdConfigFilename, crateMasksFilename;
  std::string outFilename = "check_ul.txt";

  // clang-format off
  generic.add_options()
          ("help", "produce help message")
          ("ul-filename", po::value<std::string>(&ulFilename),"Input raw filename with CRU User Logic")
          ("bare-filename", po::value<std::string>(&bareFilename),"Input raw filename with bare CRU")
          ("feeId-config-file", po::value<std::string>(&feeIdConfigFilename),"Filename with crate FEE ID correspondence")
          ("crate-masks-file", po::value<std::string>(&crateMasksFilename),"Filename with crate masks")
          ("outFilename", po::value<std::string>(&outFilename),"Output filename")
          ("full", po::value<bool>()->implicit_value(true),"Full check");


  po::options_description hidden("hidden options");
  hidden.add_options()
          ("input", po::value<std::vector<std::string>>(),"Input filename");
  // clang-format on

  po::options_description cmdline;
  cmdline.add(generic).add(hidden);

  po::store(po::command_line_parser(argc, argv).options(cmdline).run(), vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::cout << "Usage: " << argv[0] << "\n";
    std::cout << generic << std::endl;
    return 2;
  }
  if (ulFilename.empty() || bareFilename.empty()) {
    std::cout << "Please specify ul-filename and bare-filename" << std::endl;
    return 1;
  }

  o2::mid::Decoder<o2::mid::GBTBareDecoder> bareDecoder;
  o2::mid::Decoder<o2::mid::GBTUserLogicDecoder> ulDecoder;

  if (!feeIdConfigFilename.empty()) {
    o2::mid::FEEIdConfig feeIdConfig(feeIdConfigFilename.c_str());
    bareDecoder.setFeeIdConfig(feeIdConfig);
    ulDecoder.setFeeIdConfig(feeIdConfig);
  }

  if (!crateMasksFilename.empty()) {
    o2::mid::CrateMasks crateMasks(crateMasksFilename.c_str());
    bareDecoder.setCrateMasks(crateMasks);
    ulDecoder.setCrateMasks(crateMasks);
  }

  bareDecoder.init(true);
  ulDecoder.init(true);

  std::vector<o2::mid::LocalBoardRO> bareData, ulData;
  std::vector<o2::mid::ROFRecord> bareRofs, ulRofs;

  if (!readFile(bareFilename, bareDecoder, bareData, bareRofs) || !readFile(ulFilename, ulDecoder, ulData, ulRofs)) {
    return 3;
  }

  if (true) {
    // The orbit information in the UL is not correctly treated
    // This means that its orbit will always be 0, unlike the bare data orbit
    // Let us set the orbit of the raw data to 0 so that the results can be synchronized
    for (auto& rof : bareRofs) {
      rof.interactionRecord.orbit = 0;
    }

    for (auto& rof : ulRofs) {
      rof.interactionRecord.orbit = 0;
    }
  }

  std::ofstream outFile(outFilename.c_str());
  if (!outFile.is_open()) {
    std::cout << "Cannot open output file " << outFilename << std::endl;
    return 3;
  }

  bool isOk = true;
  if (vm.count("full")) {
    isOk &= checkAll(bareData, bareRofs, ulData, ulRofs, outFile);
  } else {
    for (int iloc = 0; iloc < 2; ++iloc) {
      isOk &= checkBoards(bareData, bareRofs, ulData, ulRofs, iloc == 1, outFile);
    }
  }

  if (isOk) {
    std::cout << "Everything ok!" << std::endl;
  } else {
    std::cout << "Problems found. See " << outFilename << " for details" << std::endl;
  }
  outFile.close();

  return 0;
}
