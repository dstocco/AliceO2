#ifndef AliceO2_TPC_PadSecPos_H
#define AliceO2_TPC_PadSecPos_H

#include "TPCBase/Sector.h"
#include "TPCBase/PadPos.h"

namespace AliceO2 {
namespace TPC {
  class PadSecPos {
    public:
      PadSecPos() {};
      PadSecPos(const Sector& sec, const PadPos& padPosition) : mSector(sec), mPadPos(padPosition) {}

      Sector  getSector() const { return mSector; }
      Sector& sector()          { return mSector; }

      PadPos  getPadPos() const { return mPadPos; }
      PadPos& padPos()          { return mPadPos; }
    private:
      Sector mSector{};
      PadPos mPadPos{};
  };
}
}
#endif
