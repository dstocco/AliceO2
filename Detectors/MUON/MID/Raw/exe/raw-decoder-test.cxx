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

/// \file   MID/Raw/exe/raw-decoder-test.cxx
/// \brief  Tester for MID raw data
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   09 December 2019

#include <iostream>
#include <fstream>
#include <chrono>
#include <unordered_map>
#include "boost/program_options.hpp"
#include "MIDRaw/Decoder.h"
#include "MIDRaw/RawFileReader.h"

namespace po = boost::program_options;

int main(int argc, char* argv[])
{
  po::variables_map vm;
  po::options_description generic("Generic options");
  unsigned long nHBs = 0xFFFFFFFFFFFFFFFF, nLoops = 0xFFFFFFFFFFFFFFFF;
  bool ignoreRDH = false;

  // clang-format off
  generic.add_options()
          ("help", "produce help message")
          ("custom-memory-size", po::value<uint16_t>()->implicit_value(0x2000 - 0x100),"Ignore read RDH. Use custom memory size")
          ("nHBs", po::value<unsigned long int>(&nHBs),"Number of HBs per file")
          ("nLoops", po::value<unsigned long int>(&nLoops),"Number of loops per file")
          ("feeId-config-file", po::value<std::string>()->default_value(""),"Filename with crate FEE ID correspondence")
          ("crate-masks-file", po::value<std::string>()->default_value(""),"Filename with crate masks");


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

  std::unique_ptr<o2::mid::Decoder> decoder{nullptr};

  std::chrono::duration<double> timer{0};
  unsigned long int processedHBs = 0;
  unsigned long int processedBCs = 0;

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
      if (!rawFileReader.readHB(true)) {
        break;
      }
    }
    if (!decoder) {
      auto const* rdhPtr = reinterpret_cast<const o2::header::RDHAny*>(rawFileReader.getData().data());
      decoder = o2::mid::createDecoder(*rdhPtr, true, "", vm["crate-masks-file"].as<std::string>().c_str(), vm["feeId-config-file"].as<std::string>().c_str());
    }

    std::cout << "Buffer size: " << rawFileReader.getData().size() * 1e-6 << " MB" << std::endl;

    unsigned long int iloop = 0;
    auto tAlgoStart = std::chrono::high_resolution_clock::now();
    for (; iloop < nLoops; ++iloop) {
      decoder->process(rawFileReader.getData());
    }
    timer += std::chrono::high_resolution_clock::now() - tAlgoStart;
    processedHBs += iHB * iloop;
    std::unordered_map<int64_t, bool> bcs;
    for (auto& rof : decoder->getROFRecords()) {
      bcs[rof.interactionRecord.toLong()] = true;
    }
    processedBCs += bcs.size();
  }

  std::cout << "Average number of events / orbit = " << ((processedHBs == 0) ? 0. : static_cast<double>(processedBCs) / static_cast<double>(processedHBs)) << std::endl;
  std::cout << "Decoding time / " << processedHBs << " orbits = " << ((processedHBs == 0) ? 0. : timer.count() * 1.e6 / static_cast<double>(processedHBs)) << " us" << std::endl;

  return 0;
}
