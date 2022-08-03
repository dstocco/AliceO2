#include "MIDWorkflow/EfficiencySpec.h"

#include <string>
#include <unordered_map>
#include <gsl/span>

#include "TFile.h"
#include "TH1.h"

#include "Framework/CallbackService.h"
#include "Framework/ConfigParamRegistry.h"
#include "Framework/Task.h"
#include "Framework/Logger.h"
#include "CommonUtils/ConfigurableParam.h"
#include "DataFormatsMID/Track.h"
#include "MIDEfficiency/Efficiency.h"

namespace o2
{
namespace mid
{

using namespace o2::framework;

class EfficiencyTask
{
 public:
  TH1F buildHisto(const std::unordered_map<int, uint64_t>& counts, const char* name, int nBins = 72)
  {
    TH1F histo(name, name, nBins, 0., nBins);
    if (nBins == 72) {
      histo.GetXaxis()->SetTitle("DE ID");
    }
    for (auto& item : counts) {
      histo.SetBinContent(item.first + 1, item.second);
    }
    return histo;
  }

  // std::vector<uint64_t> buildVector(const std::unordered_map<int, uint64_t>& counts, int nElements = 72)
  // {
  //   std::vector<uint64_t> out;
  //   for (int iel = 0; iel < nElements; ++iel) {
  //     auto found = counts.find(iel);
  //     uint64_t val = (found == counts.end()) ? 0 : found->second;
  //     out.emplace_back(val);
  //   }
  //   return out;
  // }

  /// prepare the efficiency
  void init(InitContext& ic)
  {
    LOG(info) << "initializing efficiency";

    // auto config = ic.options().get<std::string>("mid-eff");
    // if (!config.empty()) {
    //   conf::ConfigurableParam::updateFromFile(config, "MIDEff", true);
    // }

    auto stop = [this]() {
      TFile fout("mid-efficiency.root", "RECREATE");
      // auto firedBP = buildVector(mEfficiency.getNFiredBP());
      // auto firedNBP = buildVector(mEfficiency.getNFiredNBP());
      // auto tot = buildVector(mEfficiency.getNTot());
      // fout.WriteObject(&firedBP, "firedBP");
      // fout.WriteObject(&firedNBP, "firedNBP");
      // fout.WriteObject(&tot, "tot");
      auto histoBP = buildHisto(mEfficiency.getNFiredBP(), "nFiredBP");
      auto histoNBP = buildHisto(mEfficiency.getNFiredNBP(), "nFiredNBP");
      auto histoTot = buildHisto(mEfficiency.getNTot(), "nTot");
      histoBP.Write();
      histoNBP.Write();
      histoTot.Write();
      // histoBP->Write();
      // histoNBP->Write();
      // histoTot->Write();
      fout.Close();
    };
    ic.services().get<o2::framework::CallbackService>().set(o2::framework::CallbackService::Id::Stop, stop);
  }

  /// run the efficiency
  void run(ProcessingContext& pc)
  {
    auto midTracks = pc.inputs().get<gsl::span<mid::Track>>("midtracks");
    mEfficiency.process(midTracks);
  }

 private:
  Efficiency mEfficiency{};
};

DataProcessorSpec getEfficiencySpec()
{

  return DataProcessorSpec{
    "MIDEfficiency",
    Inputs{InputSpec{"midtracks", "MID", "TRACKS", 0, Lifetime::Timeframe}},
    Outputs{},
    AlgorithmSpec{adaptFromTask<EfficiencyTask>()},
    Options{{"mid-eff", VariantType::String, "mid-efficiency.root", {"Root MID RPCs Efficiency"}}}};
}

} // namespace mid
} // namespace o2