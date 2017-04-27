#ifndef MAPPING_H
#define MAPPING_H

#include "Rtypes.h"
#include <vector>
#include <unordered_map>

namespace AliceO2 {

  namespace MUON {

    class Mapping {

    public:

      // pad structure in the internal mapping
      struct mpPad {
        UShort_t iDigit; // index of the corresponding digit
        UChar_t nNeighbours; // number of neighbours
        UShort_t neighbours[10]; // indices of neighbours in array stored in mpDE
        Float_t area[2][2]; // 2D area
        Bool_t useMe; // kFALSE if no digit attached or already visited
      };

      // DE structure in the internal mapping
      struct mpDE {
        Int_t id; // unique ID
        UChar_t iCath[2]; // cathod index corresponding to each plane
        UShort_t nPads[2]; // number of pads on each plane
        mpPad *pads; // array of pads on both planes
        std::unordered_map<Long64_t,Long64_t> padIndices[2]; // indices+1 of pads from their ID
      };

      // static int NumberOfDetectionElements() { return fNDE; }

      // static std::vector<mpDE> ReadMapping(const char* mapfile);
      static mpDE* ReadMapping(const char* mapfile,int& numberOfDetectionElements);

      static Bool_t AreOverlapping(Float_t area1[2][2], Float_t area2[2][2], Float_t precision);

      // static int fNDE;

    };

  }

}

#endif
