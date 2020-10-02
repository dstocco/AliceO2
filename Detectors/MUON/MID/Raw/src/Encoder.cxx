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

void Encoder::init(const char* filename, bool perLink, int verbosity)
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
    mGBTEncoders[feeId].processTrigger(o2::constants::lhc::LHCMaxBunches, raw::sORB);
    mOrbitResponse[feeId] = getBuffer(feeId);
  }

  mRawWriter.setEmptyPageCallBack(this);
}

void Encoder::emptyHBFMethod(const o2::header::RDHAny* rdh, std::vector<char>& toAdd) const
{
  /// Response to orbit triggers in empty HBFs
  auto feeId = o2::raw::RDHUtils::getFEEID(rdh);
  toAdd = mOrbitResponse[feeId];
}

void Encoder::hbTrigger(const InteractionRecord& ir)
{
  /// Processes HB trigger
  if (ir.orbit != mLastIR.orbit) {
    // There was an orbit change
    for (uint16_t feeId = 0; feeId < crateparams::sNGBTs; ++feeId) {
      // Flush the data corresponding to the previous orbit
      flush(feeId, mLastIR);
      // Add the trigger response
      mGBTEncoders[feeId].processTrigger(o2::constants::lhc::LHCMaxBunches, raw::sORB);
    }
  }
}

std::vector<char> Encoder::getBuffer(uint16_t feeId)
{
  /// Flushes data
  size_t dataSize = mGBTEncoders[feeId].getBufferSize();
  size_t cruWord = 2 * o2::raw::RDHUtils::GBTWord;
  size_t modulo = dataSize % cruWord;
  if (modulo) {
    dataSize += cruWord - modulo;
  }
  std::vector<char> buf(dataSize);
  memcpy(buf.data(), mGBTEncoders[feeId].getBuffer().data(), mGBTEncoders[feeId].getBufferSize());
  mGBTEncoders[feeId].clear();
  return buf;
}

void Encoder::flush(uint16_t feeId, const InteractionRecord& ir)
{
  /// Flushes data
  if (mGBTEncoders[feeId].getBufferSize() == 0) {
    return;
  }
  auto buf = getBuffer(feeId);
  mRawWriter.addData(feeId, mFEEIdConfig.getCRUId(mGBTIds[feeId]), mFEEIdConfig.getLinkId(mGBTIds[feeId]), mFEEIdConfig.getEndPointId(mGBTIds[feeId]), ir, buf);
}

void Encoder::finalize(bool closeFile)
{
  /// Finish the flushing and closes the
  for (uint16_t feeId = 0; feeId < crateparams::sNGBTs; ++feeId) {
    flush(feeId, mLastIR);
  }
  if (closeFile) {
    mRawWriter.close();
  }
}

void Encoder::process(gsl::span<const ColumnData> data, const InteractionRecord& ir, EventType eventType)
{
  /// Encodes data
  hbTrigger(ir);

  mConverter.process(data);

  for (auto& item : mConverter.getData()) {
    mGBTEncoders[item.first].process(item.second, ir.bc);
  }
  mLastIR = ir;
}
} // namespace mid
} // namespace o2
