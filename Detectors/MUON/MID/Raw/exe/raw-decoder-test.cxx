// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   MID/Raw/exe/raw-decoder-test.cxx
/// \brief  Tester for MID raw data
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   09 December 2019

#include <iostream>
#include <fstream>
#include <chrono>
#include "boost/program_options.hpp"
#include "MIDRaw/FEEIdConfig.h"
#include "MIDRaw/CrateMasks.h"
#include "MIDRaw/Decoder.h"
#include "MIDRaw/RawFileReader.h"

namespace po = boost::program_options;

int main(int argc, char* argv[])
{
  po::variables_map vm;
  po::options_description generic("Generic options");
  unsigned long nHBs = 0xFFFFFFFFFFFFFFFF, nLoops = 0xFFFFFFFFFFFFFFFF;
  bool ignoreRDH = false;
  std::string feeIdConfigFilename, crateMasksFilename;

  // clang-format off
  generic.add_options()
          ("help", "produce help message")
          ("custom-memory-size", po::value<uint16_t>()->implicit_value(0x2000 - 0x100),"Ignore read RDH. Use custom memory size")
          ("nHBs", po::value<unsigned long int>(&nHBs),"Number of HBs per file")
          ("nLoops", po::value<unsigned long int>(&nLoops),"Number of loops per file")
          ("feeId-config-file", po::value<std::string>(&feeIdConfigFilename),"Filename with crate FEE ID correspondence")
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

  o2::mid::Decoder<o2::mid::GBTBareDecoder> decoder;
  if (!feeIdConfigFilename.empty()) {
    o2::mid::FEEIdConfig feeIdConfig(feeIdConfigFilename.c_str());
    decoder.setFeeIdConfig(feeIdConfig);
  }
  if (!crateMasksFilename.empty()) {
    o2::mid::CrateMasks crateMasks(crateMasksFilename.c_str());
    decoder.setCrateMasks(crateMasks);
  }
  decoder.init(true);

  std::chrono::duration<double> timer{0};
  unsigned long int processedHBs = 0;

  for (auto& filename : inputfiles) {
    o2::mid::RawFileReader rawFileReader;
    if (!rawFileReader.init(filename.c_str())) {
      return 2;
    }
    if (vm.count("custom-memory-size")) {
      rawFileReader.setCustomPayloadSize(vm["custom-memory-size"].as<uint16_t>());
    }

    unsigned long iHB = 0;
    for (; iHB < nHBs; ++iHB) {
      if (!rawFileReader.readHB()) {
        break;
      }
    }

    std::cout << "Buffer size: " << rawFileReader.getData().size() * 1e-6 << " MB" << std::endl;

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
