// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   mid-rawdump.cxx
/// \brief  Raw dumper for MID
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   05 December 2019

#include <iostream>
#include "boost/program_options.hpp"
#include "MIDRaw/CrateFeeIdMapper.h"
#include "MIDRaw/CrateMasks.h"
#include "MIDRaw/CRUBareDecoder.h"
#include "MIDRaw/RawDataHandler.h"
#include "MIDRaw/RawFileReader.h"
#include "fmt/format.h"

namespace po = boost::program_options;

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
  basename.insert(0, "dump_");
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
  std::string outFilename = "";
  unsigned long int nHBs = 0;
  unsigned long int firstHB = 0;
  bool onlyClosedHBs = false;
  bool rdhOnly = false;
  bool decode = false;

  // clang-format off
  generic.add_options()
          ("help", "produce help message")
          ("output", po::value<std::string>(&outFilename),"Output text file")
          ("first", po::value<unsigned long int>(&firstHB),"First HB to read")
          ("nHBs", po::value<unsigned long int>(&nHBs),"Number of HBs read")
          ("only-closed-HBs", po::value<bool>(&onlyClosedHBs)->implicit_value(true),"Return only closed HBs")
          ("rdh-only", po::value<bool>(&rdhOnly)->implicit_value(true),"Only show RDHs")
          ("decode", po::value<bool>(&decode)->implicit_value(true),"Decode output");


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
  decoder.init(o2::mid::createDefaultCrateFeeIdMapper(), o2::mid::createDefaultCrateMasks(), true);
  o2::mid::RawDataHandler<uint8_t> dataHandler;

  std::ofstream outFile;
  std::ostream& out = (outFilename.empty()) ? std::cout : (outFile.open(outFilename), outFile);

  unsigned long int iHB = 0;

  for (auto& filename : inputfiles) {
    o2::mid::RawFileReader<uint8_t> rawFileReader;
    if (!rawFileReader.init(filename.c_str())) {
      return 2;
    }
    while (rawFileReader.readHB(onlyClosedHBs)) {
      if (iHB >= firstHB) {
        if (decode) {
          decoder.process(rawFileReader.getData());
          for (auto& rof : decoder.getROFRecords()) {
            out << "Orbit: " << rof.interactionRecord.orbit << " bc: " << rof.interactionRecord.bc << std::endl;
            for (auto colIt = decoder.getData().begin() + rof.firstEntry; colIt != decoder.getData().begin() + rof.firstEntry + rof.nEntries; ++colIt) {
              out << *colIt << std::endl;
            }
          }
        } else {
          dataHandler.setBuffer(rawFileReader.getData());
          while (dataHandler.nextHBF()) {
            out << fmt::format("Version: {:d}  header size: {:d}  feeId: {:4d}  cruId: {:d}  linkId: {:2d} EP: {:d}\n", dataHandler.getRDH()->version, dataHandler.getRDH()->headerSize, dataHandler.getRDH()->feeId, dataHandler.getRDH()->cruID, dataHandler.getRDH()->linkID, dataHandler.getRDH()->endPointID);
            out << fmt::format("Offset to next: {:4d}  memory size: {:4d}\n", dataHandler.getRDH()->offsetToNext, dataHandler.getRDH()->memorySize);
            out << fmt::format("Orbit: {:8d}  bc: {:8d}  trig: 0x{:08x}\n", dataHandler.getRDH()->triggerOrbit, dataHandler.getRDH()->triggerBC, dataHandler.getRDH()->triggerType);
            out << fmt::format("Page: {:8d}  stop: {:d}\n", dataHandler.getRDH()->pageCnt, dataHandler.getRDH()->stop);
            if (rdhOnly) {
              continue;
            }
            auto payload = dataHandler.getPayload();
            for (size_t iword = 0; iword < payload.size(); iword += 16) {
              auto word = payload.subspan(iword, 16);
              for (auto it = word.rbegin(); it != word.rend(); ++it) {
                auto ibInWord = word.rend() - it;
                if (ibInWord == 4 || ibInWord == 9) {
                  out << " ";
                }
                if (ibInWord == 5 || ibInWord == 10) {
                  out << "  ";
                }
                out << fmt::format("{:02x}", static_cast<int>(*it));
              }
              out << "\n";
            }
          }
        }
      }
      rawFileReader.clear();
      ++iHB;
      if (nHBs > 0 && iHB >= nHBs + firstHB) {
        break;
      }
    }
  }
  outFile.close();

  return 0;
}
