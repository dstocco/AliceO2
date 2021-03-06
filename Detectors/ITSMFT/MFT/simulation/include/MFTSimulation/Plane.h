/// \file Plane.h
/// \brief Class for the description of the structure for the planes of the ALICE Muon Forward Tracker
/// \author Antonio Uras <antonio.uras@cern.ch>

#ifndef ALICEO2_MFT_PLANE_H_
#define ALICEO2_MFT_PLANE_H_

#include "TNamed.h"
#include "THnSparse.h"
#include "TAxis.h"

#include "FairLogger.h"

class TClonesArray;

namespace AliceO2 {
namespace MFT {

class Plane : public TNamed {

public:

  Plane();
  Plane(const Char_t *name, const Char_t *title);
  Plane(const Plane& pt);
  Plane& operator=(const Plane &source);
  
  virtual ~Plane();  // destructor
  virtual void Clear(const Option_t* /*opt*/);
  
  Bool_t Init(Int_t    planeNumber,
	      Double_t zCenter, 
	      Double_t rMin, 
	      Double_t rMax, 
	      Double_t pixelSizeX, 
	      Double_t pixelSizeY, 
	      Double_t thicknessActive, 
	      Double_t thicknessSupport, 
	      Double_t thicknessReadout,
	      Bool_t   hasPixelRectangularPatternAlongY);
  
  Bool_t CreateStructure();

  Int_t GetNActiveElements()  const { return fActiveElements->GetEntries();  }
  Int_t GetNReadoutElements() const { return fReadoutElements->GetEntries(); }
  Int_t GetNSupportElements() const { return fSupportElements->GetEntries(); }

  TClonesArray* GetActiveElements()  { return fActiveElements;  }
  TClonesArray* GetReadoutElements() { return fReadoutElements; }
  TClonesArray* GetSupportElements() { return fSupportElements; }

  THnSparseC* GetActiveElement(Int_t id);
  THnSparseC* GetReadoutElement(Int_t id);
  THnSparseC* GetSupportElement(Int_t id);

  Bool_t IsFront(THnSparseC *element) const { return (element->GetAxis(2)->GetXmin() < fZCenter); }

  void DrawPlane(Option_t *opt="");

  Double_t GetRMinSupport() const { return fRMinSupport; }
  Double_t GetRMaxSupport() const { return fRMaxSupport; }
  Double_t GetThicknessSupport() { return GetSupportElement(0)->GetAxis(2)->GetXmax() - GetSupportElement(0)->GetAxis(2)->GetXmin(); }
  
  Double_t GetZCenter()            const { return fZCenter; }
  Double_t GetZCenterActiveFront() const { return fZCenterActiveFront; }
  Double_t GetZCenterActiveBack()  const { return fZCenterActiveBack; }

  void SetEquivalentSilicon(Double_t equivalentSilicon)                       { fEquivalentSilicon            = equivalentSilicon; }
  void SetEquivalentSiliconBeforeFront(Double_t equivalentSiliconBeforeFront) { fEquivalentSiliconBeforeFront = equivalentSiliconBeforeFront; }
  void SetEquivalentSiliconBeforeBack(Double_t equivalentSiliconBeforeBack)   { fEquivalentSiliconBeforeBack  = equivalentSiliconBeforeBack; }
  Double_t GetEquivalentSilicon()            const { return fEquivalentSilicon; }
  Double_t GetEquivalentSiliconBeforeFront() const { return fEquivalentSiliconBeforeFront; }
  Double_t GetEquivalentSiliconBeforeBack()  const { return fEquivalentSiliconBeforeBack; }

  Int_t GetNumberOfChips(Option_t *opt);
  Bool_t HasPixelRectangularPatternAlongY() { return fHasPixelRectangularPatternAlongY; }
  
private:

  Int_t fPlaneNumber;

  Double_t fZCenter, fRMinSupport, fRMax, fRMaxSupport, fPixelSizeX, fPixelSizeY, fThicknessActive, fThicknessSupport, fThicknessReadout;
  Double_t fZCenterActiveFront, fZCenterActiveBack, fEquivalentSilicon, fEquivalentSiliconBeforeFront, fEquivalentSiliconBeforeBack;

  TClonesArray *fActiveElements, *fReadoutElements, *fSupportElements;

  Bool_t fHasPixelRectangularPatternAlongY, fPlaneIsOdd;

  ClassDef(Plane, 1)

};

}
}

#endif

