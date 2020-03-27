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
#include "boost/program_options.hpp"
#include "MIDRaw/CrateFeeIdMapper.h"
#include "MIDRaw/CrateMasks.h"
#include "MIDRaw/CRUBareDecoder.h"
#include "MIDRaw/RawFileReader.h"
#include "MIDQC/CRUBareDataChecker.h"

namespace po = boost::program_options;

o2::header::RAWDataHeader buildCustomRDH()
{
  o2::header::RAWDataHeader rdh;
  rdh.word1 |= 0x2000;
  rdh.word1 |= ((0x2000 - 0x100)) << 16;
  // rdh.word1 |= 0x2000 << 16;
  return rdh;
}

std::string getOutFilename(const char* inFilename, const char* outDir)
{
  std::string basename(inFilename);
  std::string fdir = "./";
  auto pos = basename.find_last_of("/");
  if (pos != std::string::npos) {
    basename.erase(0, pos + 1);
    fdir = inFilename;
    fdir.erase(pos);
  }
  basename.insert(0, "check_");
  basename += ".txt";
  std::string outputDir(outDir);
  if (outputDir.empty()) {
    outputDir = fdir;
  }
  if (outputDir.back() != '/') {
    outputDir += "/";
  }
  std::string outFilename = outputDir + basename;
  return outFilename;
}

int main(int argc, char* argv[])
{
  po::variables_map vm;
  po::options_description generic("Generic options");
  unsigned long int nHBs = 0;
  bool onlyClosedHBs = false;
  bool ignoreRDH = false;
  unsigned long int nMaxErrors = 10000;
  std::string outputDir, feeIdMapperFilename, crateMasksFilename;

  // clang-format off
  generic.add_options()
          ("help", "produce help message")
          ("nHBs", po::value<unsigned long int>(&nHBs),"Number of HBs read")
          ("only-closed-HBs", po::value<bool>(&onlyClosedHBs)->implicit_value(true),"Return only closed HBs")
          ("ignore-RDH", po::value<bool>(&ignoreRDH)->implicit_value(true),"Ignore read RDH. Use custom one instead")
          ("max-errors", po::value<unsigned long int>(&nMaxErrors),"Maximum number of errors before aborting")
          ("feeId-config-file", po::value<std::string>(&feeIdMapperFilename),"Filename with crate FEE ID correspondence")
          ("crate-masks-file", po::value<std::string>(&crateMasksFilename),"Filename with crate masks")
          ("output-dir", po::value<std::string>(&outputDir),"Output directory");


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

  for (auto& filename : inputfiles) {
    o2::mid::RawFileReader<uint8_t> rawFileReader;
    if (!rawFileReader.init(filename.c_str())) {
      return 2;
    }
    if (ignoreRDH) {
      rawFileReader.setCustomRDH(buildCustomRDH());
    }
    std::string outFilename = getOutFilename(filename.c_str(), outputDir.c_str());
    std::ofstream outFile(outFilename.c_str());
    if (!outFile.is_open()) {
      std::cout << "Error: cannot create " << outFilename << std::endl;
      return 2;
    }
    std::cout << "Writing output to: " << outFilename << " ..." << std::endl;

    std::vector<o2::mid::LocalBoardRO> data;
    std::vector<o2::mid::ROFRecord> rofRecords;
    std::vector<o2::mid::ROFRecord> hbRecords;

    checker.reset();
    unsigned long int iHB = 0;
    unsigned long int lastCheckedHB = 0;
    std::stringstream summary;
    while (rawFileReader.readHB(onlyClosedHBs)) {
      decoder.process(rawFileReader.getData());
      rawFileReader.clear();
      size_t offset = data.size();
      data.insert(data.end(), decoder.getData().begin(), decoder.getData().end());
      for (auto& rof : decoder.getROFRecords()) {
        rofRecords.emplace_back(rof.interactionRecord, rof.eventType, rof.firstEntry + offset, rof.nEntries);
      }
      o2::InteractionRecord hb(0, iHB);
      hbRecords.emplace_back(hb, o2::mid::EventType::Noise, offset, decoder.getData().size());
      ++iHB;
      bool isLast = (rawFileReader.getState() != 0 || (nHBs > 0 && iHB >= nHBs));
      if (decoder.isComplete() || isLast) {
        // The check assumes that we have all data corresponding to one event.
        // However this might not be true since we read one HB at the time.
        // So we must test that the event was fully read before running the check.
        if (!checker.process(data, rofRecords, hbRecords)) {
          outFile << checker.getDebugMessage() << "\n";
          lastCheckedHB = iHB;
        }
        data.clear();
        rofRecords.clear();
        hbRecords.clear();
      }

      if (checker.getNBCsFaulty() >= nMaxErrors) {
        summary << "Too many errors found: abort check!\n";
        break;
      }

      if (isLast) {
        break;
      }
    }
    summary << "Fraction of faulty events: " << checker.getNBCsFaulty() << " / " << checker.getNBCsProcessed() << " = " << static_cast<double>(checker.getNBCsFaulty()) / static_cast<double>(checker.getNBCsProcessed()) << "\n";
    outFile << summary.str();
    std::cout << summary.str();

    outFile.close();
  }

  return 0;
}
