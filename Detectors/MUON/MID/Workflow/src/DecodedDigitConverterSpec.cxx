// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   MID/Workflow/src/DecodedDigitConverterSpec.cxx
/// \brief  Data processor specs for MID digit converter device
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   25 September 2020

#include "MIDWorkflow/DecodedDigitConverterSpec.h"

#include <vector>
#include <gsl/gsl>
#include "Framework/ControlService.h"
#include "Framework/DataRefUtils.h"
#include "Framework/Logger.h"
#include "Framework/Output.h"
#include "Framework/Task.h"
#include "DataFormatsMID/ColumnData.h"
#include "DataFormatsMID/ROFRecord.h"
#include "MIDSimulation/ColumnDataMC.h"

namespace of = o2::framework;

namespace o2
{
namespace mid
{

class DigitConverterDeviceDPL
{
 public:
  void init(o2::framework::InitContext& ic)
  {
  }

  void run(o2::framework::ProcessingContext& pc)
  {
    auto msg = pc.inputs().get("mid_data");
    gsl::span<const ColumnData> patterns = of::DataRefUtils::as<const ColumnData>(msg);

    auto msgROF = pc.inputs().get("mid_data_rof");
    gsl::span<const ROFRecord> inROFRecords = of::DataRefUtils::as<const ROFRecord>(msgROF);

    std::vector<ColumnDataMC> mcPatterns;
    for (auto& pat : patterns) {
      mcPatterns.push_back({pat.deId, pat.columnId, pat.patterns});
    }

    pc.outputs().snapshot(of::Output{o2::header::gDataOriginMID, "DECODEDDIGITS", 0, of::Lifetime::Timeframe}, mcPatterns);
    pc.outputs().snapshot(of::Output{o2::header::gDataOriginMID, "DECODEDDIGITSROF", 0, of::Lifetime::Timeframe}, inROFRecords);
  }
};

framework::DataProcessorSpec getDecodedDigitConverterSpec()
{
  std::vector<of::InputSpec> inputSpecs{};
  std::vector<of::OutputSpec> outputSpecs{};

  return of::DataProcessorSpec{
    "MIDDigitConverter",
    {of::InputSpec{"mid_data", o2::header::gDataOriginMID, "DATA"}, of::InputSpec{"mid_data_rof", o2::header::gDataOriginMID, "DATAROF"}},
    {of::OutputSpec{o2::header::gDataOriginMID, "DECODEDDIGITS"}, of::OutputSpec{o2::header::gDataOriginMID, "DECODEDDIGITSROF"}},
    of::AlgorithmSpec{of::adaptFromTask<o2::mid::DigitConverterDeviceDPL>()}};
}
} // namespace mid
} // namespace o2