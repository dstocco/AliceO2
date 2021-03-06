/// \file HalfSegmentation.h
/// \brief Segmentation class for each half of the ALICE Muon Forward Tracker
/// \author Raphael Tieulent <raphael.tieulent@cern.ch>
/// \date 09/06/2015

#ifndef ALICEO2_MFT_HALFSEGMENTATION_H_
#define ALICEO2_MFT_HALFSEGMENTATION_H_

#include "TNamed.h"
#include "TXMLEngine.h"

#include "MFTSimulation/Segmentation.h"
#include "MFTSimulation/VSegmentation.h"

namespace AliceO2 { namespace MFT { class HalfDiskSegmentation; } }

namespace AliceO2 {
namespace MFT {

class HalfSegmentation : public VSegmentation {

public:
  
  HalfSegmentation();
  HalfSegmentation(const Char_t *initFile, const Short_t id);
  HalfSegmentation(const HalfSegmentation &source);

  virtual ~HalfSegmentation();
  virtual void Clear(const Option_t* /*opt*/);
  
  Bool_t GetID() const {return (GetUniqueID()>>12);};
  
  Int_t GetNHalfDisks() const { return fHalfDisks->GetEntries(); }

  HalfDiskSegmentation* GetHalfDisk(Int_t iDisk) const { if (iDisk>=0 && iDisk<fHalfDisks->GetEntries()) return (HalfDiskSegmentation*) fHalfDisks->At(iDisk); else return NULL; }
 
private:
  
  void FindHalf(TXMLEngine* xml, XMLNodePointer_t node, XMLNodePointer_t &retnode);
  void CreateHalfDisks(TXMLEngine* xml, XMLNodePointer_t node);

  TClonesArray *fHalfDisks; ///< \brief Array of pointer to HalfDiskSegmentation

  /// \cond CLASSIMP
  ClassDef(HalfSegmentation, 1);
  /// \endcond
  
};

}
}

#endif
