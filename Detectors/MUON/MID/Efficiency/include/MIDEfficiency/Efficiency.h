#ifndef O2_MID_EFFICIENCY_H_
#define O2_MID_EFFICIENCY_H_

#include <array>
#include <unordered_map>
#include <gsl/span>
#include "DataFormatsMID/Track.h"

namespace o2
{
namespace mid
{

/// Class Eff
class Efficiency
{
 public:
  Efficiency() = default;
  ~Efficiency() = default;

  void init();
  void process(gsl::span<const mid::Track> midTracks);

  const std::unordered_map<int, uint64_t>& getNFiredBP() const { return mNFired[0]; }
  const std::unordered_map<int, uint64_t>& getNFiredNBP() const { return mNFired[1]; }
  const std::unordered_map<int, uint64_t>& getNTot() const { return mNTot; }

 private:
  std::array<std::unordered_map<int, uint64_t>, 2> mNFired{};
  std::unordered_map<int, uint64_t> mNTot{};
};

} // namespace mid
} // namespace o2

#endif // O2_MID_EFFICIENCY_H_
