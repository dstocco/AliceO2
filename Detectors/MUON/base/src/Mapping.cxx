#include "MUONBase/Mapping.h"
#include <fstream>
#include <cassert>
#include <iostream>

using namespace AliceO2::MUON;

using namespace std;

// int Mapping::fNDE = 0;

// std::vector<Mapping::mpDE> Mapping::ReadMapping(const char* mapfile)
Mapping::mpDE* Mapping::ReadMapping(const char* mapfile,int& numberOfDetectionElements)
{
  ifstream in(mapfile,ios::binary);

  if (!in.is_open()) {
    // return std::vector<Mapping::mpDE>(0);
    numberOfDetectionElements = 0;
    return 0x0;
  }

  int totalNumberOfPads(0);

  // Int_t numberOfDetectionElements(0);

  in.read((char*)&numberOfDetectionElements,sizeof(int));

  // fNDE = numberOfDetectionElements;

  // std::cout << "numberOfDetectionElements=" << numberOfDetectionElements << std::endl;
  // std::vector<Mapping::mpDE> detectionElements(numberOfDetectionElements);

 Mapping::mpDE* detectionElements = new Mapping::mpDE[numberOfDetectionElements];

  for ( int i = 0; i < numberOfDetectionElements; ++i ) {

    Mapping::mpDE& de(detectionElements[i]);

    in.read((char*)&de.id,sizeof(Int_t));

    in.read((char*)&de.iCath[0],sizeof(UChar_t)*2);
    in.read((char*)&de.nPads[0],sizeof(UShort_t)*2);

    int n = de.nPads[0] + de.nPads[1];

    de.pads = new mpPad[n];

    for ( int ip = 0; ip < n; ++ip ) {
      in.read((char*)&(de.pads[ip]),sizeof(struct mpPad));
      ++totalNumberOfPads;
    }

    int mapsize = 2*n;
    Long64_t* themap = new Long64_t[mapsize];
    int ix(0);

    in.read((char*)&themap[0],sizeof(Long64_t)*mapsize);

    for ( int cathode = 0; cathode < 2; ++cathode) {
      for ( int ii = 0; ii < de.nPads[cathode]; ++ii ) {
        de.padIndices[cathode].insert({themap[ix],themap[ix+1]});
        ix += 2;
      }
    }

    assert(ix==2*n);
    assert(de.padIndices[0].size() + de.padIndices[1].size() == n);

    delete[] themap;
  }

//  if ( totalNumberOfPads != 1064008 ) {
//    return std::vector<Mapping::mpDE>(0);
//  }

  return detectionElements;
}

//_________________________________________________________________________________________________
Bool_t Mapping::AreOverlapping(Float_t area1[2][2], Float_t area2[2][2], Float_t precision)
{
  /// check if the two areas overlap
  /// precision in cm: positive = increase pad size / negative = decrease pad size

  if (area1[0][0] - area2[0][1] > precision) return kFALSE;
  if (area2[0][0] - area1[0][1] > precision) return kFALSE;
  if (area1[1][0] - area2[1][1] > precision) return kFALSE;
  if (area2[1][0] - area1[1][1] > precision) return kFALSE;

  return kTRUE;

}
