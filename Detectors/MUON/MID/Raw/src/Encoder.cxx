// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file   MID/Raw/src/Encoder.cxx
/// \brief  MID raw data encoder
/// \author Diego Stocco <Diego.Stocco at cern.ch>
/// \date   30 September 2019

#include "MIDRaw/Encoder.h"

#include "DetectorsRaw/HBFUtils.h"
#include "DetectorsRaw/RDHUtils.h"
#include "MIDRaw/CrateMasks.h"
#include <fmt/format.h>

namespace o2
{
namespace mid
{

void Encoder::init(const char* filename, bool perLink, int verbosity, bool debugMode)
{
  /// Initializes links

  CrateMasks masks;
  auto gbtIds = mFEEIdConfig.getConfiguredGBTIds();

  mRawWriter.setVerbosity(verbosity);
  int lcnt = 0;
  for (auto& gbtId : gbtIds) {
    auto feeId = mFEEIdConfig.getFeeId(gbtId);
    mRawWriter.registerLink(feeId, mFEEIdConfig.getCRUId(gbtId), mFEEIdConfig.getLinkId(gbtId), mFEEIdConfig.getEndPointId(gbtId),
                            perLink ? fmt::format("{:s}_L{:d}.raw", filename, lcnt) : fmt::format("{:s}.raw", filename));
    mGBTEncoders[feeId].setFeeId(feeId);
    mGBTEncoders[feeId].setMask(masks.getMask(feeId));
    mGBTIds[feeId] = gbtId;
    lcnt++;

    // Initializes the trigger response to be added to the empty HBs
    auto ir = getOrbitIR(0);
    mGBTEncoders[feeId].processTrigger(ir, raw::sORB);
    mOrbitResponse[feeId] = flushPayload(feeId, ir);
  }

  mRawWriter.setEmptyPageCallBack(this);

  mConverter.setDebugMode(debugMode);
}

void Encoder::emptyHBFMethod(const o2::header::RDHAny* rdh, std::vector<char>& toAdd) const
{
  /// Response to orbit triggers in empty HBFs
  auto feeId = o2::raw::RDHUtils::getFEEID(rdh);
  toAdd = mOrbitResponse[feeId];
}

void Encoder::onOrbitChange(uint32_t orbit)
{
  /// Performs action when orbit changes
  auto ir = getOrbitIR(orbit > 0 ? orbit - 1 : orbit);
  for (uint16_t feeId = 0; feeId < crateparams::sNGBTs; ++feeId) {
    // Write the data corresponding to the previous orbit
    if (!mGBTEncoders[feeId].isEmpty()) {
      writePayload(feeId, mLastIR);
    }
    mGBTEncoders[feeId].processTrigger(ir, raw::sORB);
  }
}

std::vector<char> Encoder::flushPayload(uint16_t feeId, const InteractionRecord& ir)
{
  /// Flushes data to buffer
  std::vector<char> buf;
  mGBTEncoders[feeId].flush(buf, ir);
  size_t dataSize = buf.size();
  size_t cruWord = 2 * o2::raw::RDHUtils::GBTWord;
  size_t modulo = dataSize % cruWord;
  if (modulo) {
    dataSize += cruWord - modulo;
  }
  char fill{static_cast<char>(0)};
  buf.resize(dataSize, fill);
  return buf;
}

void Encoder::writePayload(uint16_t feeId, const InteractionRecord& ir)
{
  /// Writes data
  auto buf = flushPayload(feeId, ir);
  mRawWriter.addData(feeId, mFEEIdConfig.getCRUId(mGBTIds[feeId]), mFEEIdConfig.getLinkId(mGBTIds[feeId]), mFEEIdConfig.getEndPointId(mGBTIds[feeId]), ir, buf);
}

void Encoder::finalize(bool closeFile)
{
  /// Writes remaining data and closes the file
  for (uint16_t feeId = 0; feeId < crateparams::sNGBTs; ++feeId) {
    // Write the last payload
    writePayload(feeId, mLastIR);
    if (!mGBTEncoders[feeId].isEmpty()) {
      // Since the regional response comes after few clocks,
      // we might have the corresponding regional cards in the next orbit.
      // If this is the case, we add a response to an orbit trigger
      auto ir = getOrbitIR(mLastIR.orbit);
      mGBTEncoders[feeId].processTrigger(ir, raw::sORB);
      // And then we add the regionals.
      // Notice that we want to flush all data of the next orbit
      ++ir.orbit;
      writePayload(feeId, ir);
    }
  }
  if (closeFile) {
    mRawWriter.close();
  }
}

void Encoder::process(gsl::span<const ColumnData> data, const InteractionRecord& ir, EventType eventType)
{
  /// Encodes data
  if (ir.orbit != mLastIR.orbit) {
    onOrbitChange(ir.orbit);
  }

  mConverter.process(data);

  for (auto& item : mConverter.getData()) {
    mGBTEncoders[item.first].process(item.second, ir);
  }
  mLastIR = ir;
}
} // namespace mid
} // namespace o2
