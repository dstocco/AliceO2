// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   MID/Filtering/exe/computeMasks.cxx
/// \brief  Utility to build masks from data
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   12 March 2020

#include <iostream>
#include <vector>
#include "boost/program_options.hpp"
#include "CommonDataFormat/InteractionRecord.h"
#include "DataFormatsMID/ROFRecord.h"
#include "MIDRaw/CrateMasks.h"
#include "MIDRaw/DecodedDataAggregator.h"
#include "MIDRaw/Decoder.h"
#include "MIDRaw/ElectronicsDelay.h"
#include "MIDRaw/FEEIdConfig.h"
#include "DataFormatsMID/ROBoard.h"
#include "MIDRaw/RawFileReader.h"
#include "MIDRaw/ColumnDataToLocalBoard.h"
#include "MIDFiltering/ChannelMasks.h"
#include "MIDFiltering/ChannelScalers.h"
#include "MIDFiltering/MaskMaker.h"

namespace po = boost::program_options;

int main(int argc, char* argv[])
{
  po::variables_map vm;
  po::options_description generic("Generic options");

  // clang-format off
  generic.add_options()
          ("help", "produce help message")
          ("feeId-config-file", po::value<std::string>(),"Filename with crate FEE ID correspondence")
          ("crate-masks-file", po::value<std::string>(),"Filename with crate masks")
          ("electronics-delay-file", po::value<std::string>(),"Filename with electronics delay");


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

  o2::mid::FEEIdConfig feeIdConfig;
  if (vm.count("feeId-config-file")) {
    feeIdConfig = o2::mid::FEEIdConfig(vm["feeId-config-file"].as<std::string>().c_str());
  }

  o2::mid::ElectronicsDelay electronicsDelay;
  if (vm.count("electronics-delay-file")) {
    electronicsDelay = o2::mid::readElectronicsDelay(vm["electronics-delay-file"].as<std::string>().c_str());
  }

  o2::mid::CrateMasks crateMasks;
  if (vm.count("crate-masks-file")) {
    crateMasks = o2::mid::CrateMasks(vm["crate-masks-file"].as<std::string>().c_str());
  }

  std::vector<o2::mid::ROBoard> data;
  std::vector<o2::mid::ROFRecord> rofRecords;

  for (auto& filename : inputfiles) {
    o2::mid::RawFileReader rawFileReader;
    if (!rawFileReader.init(filename.c_str())) {
      return 2;
    }

    while (rawFileReader.readHB() > 0) {
      if (!decoder) {
        auto const* rdhPtr = reinterpret_cast<const o2::header::RDHAny*>(rawFileReader.getData().data());
        decoder = o2::mid::createDecoder(*rdhPtr, false, electronicsDelay, crateMasks, feeIdConfig);
      }
      decoder->process(rawFileReader.getData());
      rawFileReader.clear();
      size_t offset = data.size();
      data.insert(data.end(), decoder->getData().begin(), decoder->getData().end());
      for (auto& rof : decoder->getROFRecords()) {
        rofRecords.emplace_back(rof.interactionRecord, rof.eventType, rof.firstEntry + offset, rof.nEntries);
      }
    }
  }

  o2::mid::DecodedDataAggregator aggregator;
  aggregator.process(data, rofRecords, o2::mid::EventType::Noise);
  o2::mid::ChannelScalers scalers;
  for (auto& noisy : aggregator.getData()) {
    scalers.count(noisy);
  }

  auto masks = o2::mid::makeMasks(scalers);
  auto maskVec = masks.getMasks();
  if (maskVec.empty()) {
    std::cout << "No problematic digit found" << std::endl;
    return 0;
  }

  std::cout << "Problematic digits found. Corresponding masks:" << std::endl;
  for (auto& mask : maskVec) {
    std::cout << mask << std::endl;
  }

  std::cout << "\nCorresponding boards masks:" << std::endl;
  o2::mid::ColumnDataToLocalBoard colToBoard;
  colToBoard.process(maskVec);
  for (auto& mapIt : colToBoard.getData()) {
    for (auto& board : mapIt.second) {
      std::cout << board << std::endl;
    }
  }

  return 1;
}
