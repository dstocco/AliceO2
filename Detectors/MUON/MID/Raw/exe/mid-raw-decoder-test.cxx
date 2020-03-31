// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   mid-raw-checker.cxx
/// \brief  CRU bare data checker for MID
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   09 December 2019

#include <iostream>
#include <fstream>
#include <chrono>
#include "boost/program_options.hpp"
#include "MIDRaw/CrateFeeIdMapper.h"
#include "MIDRaw/CrateMasks.h"
#include "MIDRaw/CRUBareDecoder.h"
#include "MIDRaw/RawFileReader.h"
#include "CRUBareDataChecker.h"

namespace po = boost::program_options;

o2::header::RAWDataHeader buildCustomRDH()
{
  o2::header::RAWDataHeader rdh;
  rdh.word1 |= 0x2000;
  rdh.word1 |= ((0x2000 - 0x100)) << 16;
  // rdh.word1 |= 0x2000 << 16;
  return rdh;
}

int main(int argc, char* argv[])
{
  po::variables_map vm;
  po::options_description generic("Generic options");
  unsigned long nHBs = 0xFFFFFFFFFFFFFFFF, nLoops = 0xFFFFFFFFFFFFFFFF;
  bool ignoreRDH = false;
  std::string feeIdMapperFilename, crateMasksFilename;

  // clang-format off
  generic.add_options()
          ("help", "produce help message")
          ("ignore-RDH", po::value<bool>(&ignoreRDH)->implicit_value(true),"Ignore read RDH. Use custom one instead")
          ("nHBs", po::value<unsigned long int>(&nHBs),"Number of HBs per file")
          ("nLoops", po::value<unsigned long int>(&nLoops),"Number of loops per file")
          ("feeId-config-file", po::value<std::string>(&feeIdMapperFilename),"Filename with crate FEE ID correspondence")
          ("crate-masks-file", po::value<std::string>(&crateMasksFilename),"Filename with crate masks");


  po::options_description hidden("hidden options");
  hidden.add_options()
          ("input", po::value<std::vector<std::string>>(),"Input filename");
  // clang-format on

  po::options_description cmdline;
  cmdline.add(generic).add(hidden);

  po::positional_options_description pos;
  pos.add("input", -1);

  po::store(po::command_line_parser(argc, argv).options(cmdline).positional(pos).run(), vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::cout << "Usage: " << argv[0] << " <input_raw_filename> [input_raw_filename_1 ...]\n";
    std::cout << generic << std::endl;
    return 2;
  }
  if (vm.count("input") == 0) {
    std::cout << "no input file specified" << std::endl;
    return 1;
  }

  std::vector<std::string> inputfiles{vm["input"].as<std::vector<std::string>>()};

  o2::mid::CRUBareDecoder decoder;
  o2::mid::CrateFeeIdMapper feeIdMapper = o2::mid::createDefaultCrateFeeIdMapper();
  if (!feeIdMapperFilename.empty()) {
    feeIdMapper.load(feeIdMapperFilename.c_str());
  }
  auto crateMasks = o2::mid::createDefaultCrateMasks();
  if (!crateMasksFilename.empty()) {
    crateMasks.load(crateMasksFilename.c_str());
  }
  decoder.init(feeIdMapper, crateMasks, true);

  o2::mid::CRUBareDataChecker checker;
  checker.setCrateMasks(crateMasks);

  std::chrono::duration<double> timer{0};
  unsigned long int processedHBs = 0;

  for (auto& filename : inputfiles) {
    o2::mid::RawFileReader<uint8_t> rawFileReader;
    if (!rawFileReader.init(filename.c_str())) {
      return 2;
    }
    if (ignoreRDH) {
      rawFileReader.setCustomRDH(buildCustomRDH());
    }

    unsigned long iHB = 0;
    for (; iHB < nHBs; ++iHB) {
      if (!rawFileReader.readHB()) {
        break;
      }
    }

    unsigned long int iloop = 0;
    auto tAlgoStart = std::chrono::high_resolution_clock::now();
    for (; iloop < nLoops; ++iloop) {
      decoder.process(rawFileReader.getData());
    }
    timer += std::chrono::high_resolution_clock::now() - tAlgoStart;
    processedHBs += iHB * iloop;
  }
  std::cout << "Decoding time / " << processedHBs << " = " << ((processedHBs == 0) ? 0. : timer.count() * 1.e6 / static_cast<double>(processedHBs)) << " us" << std::endl;

  return 0;
}
