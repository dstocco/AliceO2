/// A simple program to dump digits
/// \author Diego Stocco (Diego.Stocco at cern.ch)

#include "runFairMQDevice.h"
#include "MUONReconstruction/MIDclustering.h"

namespace bpo = boost::program_options;
using namespace std;

void addCustomOptions(bpo::options_description& options)
{
  options.add_options()
  // ("help,h", "produces this usage message")
  ("source,s", bpo::value<std::string>(), "address to get the messages from")
  ("binmapfile", bpo::value<std::string>(), "file with segmentation")
  ;
}

FairMQDevicePtr getDevice(const FairMQProgOptions& /*config*/)
{
  return new AliceO2::MUON::MIDclustering();
}
