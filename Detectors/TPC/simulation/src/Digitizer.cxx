/// \file Digitizer.cxx
/// \brief Digitizer for the TPC
#include "TPCSimulation/Digitizer.h"
#include "TPCSimulation/Point.h"
#include "TPCSimulation/PadResponse.h"

#include "TPCBase/Mapper.h"

#include "TRandom.h"
#include "TF1.h"
#include "TClonesArray.h"
#include "TCollection.h"
#include "TMath.h"

#include "FairLogger.h"

#include <cmath>
#include <iostream>

ClassImp(AliceO2::TPC::Digitizer)

using namespace AliceO2::TPC;

Digitizer::Digitizer():
TObject(),
mPolya(nullptr),
mDigitContainer(nullptr)
{}

Digitizer::~Digitizer(){
  delete mDigitContainer;
  delete mPolya;
}

void Digitizer::init(){
  mDigitContainer = new DigitContainer();
  Float_t SigmaOverMu = 0.78;
  Float_t kappa = 1/(SigmaOverMu*SigmaOverMu);
  Float_t s = 1/kappa;
  
  char strPolya[1000];
  snprintf(strPolya,1000,"1/(TMath::Gamma(%e)*%e) *pow(x/%e, (%e)) *exp(-x/%e)", kappa, s, s, kappa-1, s);
//   mPolya = new TF1("polya", "1/x", 0, 1000);
}

DigitContainer *Digitizer::Process(TClonesArray *points){
  // TODO should be parametrized
  Float_t wIon = 37.3e-6;
  Float_t attCoef = 250.;
  Float_t OxyCont = 5.e-6;
  Float_t driftV = 2.58;
  Float_t zBinWidth = 0.19379844961;
  
  mDigitContainer->reset();

  Float_t posEle[4] = {0., 0., 0., 0.};

  const Mapper& mapper = Mapper::instance();
  
  Int_t A = 0;
  Int_t C = 0;
  
  for (TIter pointiter = TIter(points).Begin(); pointiter != TIter::End(); ++pointiter){
    Point *inputpoint = static_cast<Point *>(*pointiter);
    
    posEle[0] = inputpoint->GetX();
    posEle[1] = inputpoint->GetY();
    posEle[2] = inputpoint->GetZ();
    posEle[3] = static_cast<int>(inputpoint->GetEnergyLoss()/wIon);
    
    //Loop over electrons
    for(Int_t iEle=0; iEle < posEle[3]; ++iEle){
      
      // Attachment
      const Float_t attProb = attCoef * OxyCont * getTime(posEle[2]);
      if((gRandom->Rndm(0)) < attProb) continue;

      // Drift and Diffusion
      ElectronDrift(posEle);

      // remove electrons that end up outside the active volume
      if(TMath::Abs(posEle[2]) > 250.) continue;

      const GlobalPosition3D posElePad (posEle[0], posEle[1], posEle[2]);
      const DigitPos digiPadPos = mapper.findDigitPosFromGlobalPosition(posElePad);
       
      if(!digiPadPos.isValid()) continue;
       
      // GEM amplification
      // Gain values taken from TDR addendum - to be put someplace else
      Int_t nEleGEM1 = SingleGEMAmplification(1, 9.1);
      Int_t nEleGEM2 = SingleGEMAmplification(nEleGEM1, 0.88);
      Int_t nEleGEM3 = SingleGEMAmplification(nEleGEM2, 1.66);
      Int_t nEleGEM4 = SingleGEMAmplification(nEleGEM3, 144);
      
      // Loop over all individual pads with signal due to pad response function
      getPadResponse(posEle[0], posEle[1], mPadResponse);
      for(auto &padresp : mPadResponse ) {
        const Int_t pad = digiPadPos.getPadPos().getPad() + padresp.getPad();
        const Int_t row = digiPadPos.getPadPos().getRow() + padresp.getRow();
        const Float_t weight = padresp.getWeight();
        
        const Float_t startTime = getTime(posEle[2]);
        
        // Loop over all time bins with signal due to time response
        for(Float_t bin = 0; bin<5; ++bin){
          Double_t signal = 55*Gamma4(startTime+bin*zBinWidth, startTime, nEleGEM4*weight);
          mDigitContainer->addDigit(digiPadPos.getCRU().number(), getTimeBinFromTime(startTime+bin*zBinWidth), row, pad, signal);
        }
        // end of loop over time bins
      }
      // end of loop over pads
    }
    //end of loop over electrons
  }
  // end of loop over points

  return mDigitContainer;
}


void Digitizer::ElectronDrift(Float_t *posEle) const {
  // TODO parameters to be stored someplace else
  Float_t DiffT = 0.0209;
  Float_t DiffL = 0.0221;
  
  Float_t driftl=posEle[2];
  if(driftl<0.01) driftl=0.01;
  driftl=TMath::Sqrt(driftl);
  Float_t sigT = driftl*DiffT;
  Float_t sigL = driftl*DiffL;
  posEle[0]=gRandom->Gaus(posEle[0],sigT);
  posEle[1]=gRandom->Gaus(posEle[1],sigT);
  posEle[2]=gRandom->Gaus(posEle[2],sigL);
}