#include "MIDEfficiency/Efficiency.h"

namespace o2
{
namespace mid
{

void Efficiency::process(gsl::span<const mid::Track> midTracks)
{

  for (auto& track : midTracks) {
    if (track.getEfficiencyFlag() >= 2) {

      auto deIdMT11 = track.getFiredDeId();

      for (int ich = 0; ich < 4; ++ich) {

        auto deId = deIdMT11 + 9 * ich;
        ++mNTot[deId];

        for (int icath = 0; icath < 2; ++icath) {

          if (track.isFiredChamber(ich, icath)) {
            ++mNFired[icath][deId];
          }
        }
      }
    }
  }
}

} // namespace mid
} // namespace o2