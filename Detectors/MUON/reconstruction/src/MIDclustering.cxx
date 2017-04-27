/**************************************************************************
* This file is property of and copyright by the ALICE O2 Project         *
* All rights reserved.                                                   *
*                                                                        *
* Primary Authors:                                                       *
*   Diego Stocco <Diego.Stocco@cern.ch>                                  *
*                                                                        *
* Permission to use, copy, modify and distribute this software and its   *
* documentation strictly for non-commercial purposes is hereby granted   *
* without fee, provdeIdd that the above copyright notice appears in all   *
* copies and that both the copyright notice and this permission notice   *
* appear in the supporting documentation. The authors make no claims     *
* about the suitability of this software for any purpose. It is          *
* provdeIdd "as is" without express or implied warranty.                  *
**************************************************************************/

// $Id$

///
///  @file   MIDclustering.cxx
///  @author Diego Stocco <Diego.Stocco at cern.ch>
///  @date   24 October 2016
///  @brief  Implementation of cluster reconstruction processing device for the MID
///

#include "MUONReconstruction/MIDclustering.h"
#include "FairMQLogger.h"
#include "options/FairMQProgOptions.h"

using namespace AliceO2::MUON;
using namespace std;

//_________________________________________________________________________________________________
MIDclustering::MIDclustering():
FairMQDevice(),
fMpDEs(),
fPreClusters(),
fActiveDEs(),
fClusters(),
fNClusters(0),
fkSqrt12(3.4641)
{
  // register a handler for data arriving on "data-in" channel
  OnData("data-in", &MIDclustering::HandleData);
}

//_________________________________________________________________________________________________
MIDclustering::~MIDclustering()
{
  /// Default destructor
}

//_________________________________________________________________________________________________
void MIDclustering::AddPad(digiPads &de, UShort_t iPad, preCluster &cl)
{
  /// add the given mpPad and its fired neighbours (recursive method)

  // add the given pad
  Mapping::mpPad* pads = de.mapping->pads;

  Mapping::mpPad &pad(pads[iPad]);
  // if (de.nOrderedPads < static_cast<UShort_t>(de.orderedPads.size())) {
  //   de.orderedPads[de.nOrderedPads] = iPad;
  // }
  // else {
  //   de.orderedPads.push_back(iPad);
  // }
  UShort_t icol = pad.iDigit;
  cl.column |= (1<<icol);
  // cl.lastPad = de.nOrderedPads;
  // ++de.nOrderedPads;

  for ( int ip=0; ip<2; ++ip ) {
    if ( pad.area[ip][0] < cl.area[icol][ip][0]) cl.area[icol][ip][0] = pad.area[ip][0];
    if ( pad.area[ip][1] > cl.area[icol][ip][1]) cl.area[icol][ip][1] = pad.area[ip][1];
  }

  printf("    adding pad %i  col %i  (%g,%g,%g,%g)\n",iPad,pad.iDigit,pad.area[0][0],pad.area[0][1],pad.area[1][0],pad.area[1][1]); // REMEMBER TO CUT

  pad.useMe = kFALSE;

  // loop over its neighbours
  for (UShort_t iNeighbour = 0; iNeighbour < pad.nNeighbours; ++iNeighbour) {

    if (pads[pad.neighbours[iNeighbour]].useMe) {
      // add the pad to the precluster
      AddPad(de, pad.neighbours[iNeighbour], cl);
    }

  }

}

//_________________________________________________________________________________________________
bool MIDclustering::HandleData(FairMQMessagePtr& msg, int /*index*/)
{

  // reset fired pad and precluster information
  ResetPadsAndClusters();

  LOG(INFO) << "Received data of size: " << msg->GetSize();

  // The HLT adds an header. After 100 bytes, we have the number of digits

  if (msg->GetSize() < 100) {
    LOG(INFO) << "Empty message: skip";
    return true;
  }

  // Load the digits to get the fired pads
  if ( LoadDigits(msg) ) {

    // Make clusters
    for ( auto& pair : fActiveDEs ) {
      // loop on active DEs
      for ( int iPlane=0; iPlane<2; iPlane++ ) {
        fNPreClusters[iPlane] = 0;
        // fPreClusters[iPlane].clear();
      }

      Int_t deId = pair.first;

      PreClusterizeRecursive(fMpDEs[deId]);
      MakeClusters(deId);
    }

    // return true; // REMEMBER TO CUT

    // Store clusters
    if ( fNClusters > 0 ) {

      // Create message of the exactly needed buffer size
      int size = sizeof(uint32_t) + fNClusters*sizeof(cluster);
      FairMQMessagePtr msgOut(NewMessage(size));

      Int_t status = StoreClusters(msgOut);
      if (status < 0) {
        LOG(ERROR) << "problem storing MID clusters";
        return false;
      }

      // Send out the output message
      if (Send(msgOut, "data-out") < 0) {
        LOG(ERROR) << "problem sending message";
        return false;
      }
    }
  }

  return true;
}

//_________________________________________________________________________________________________
void MIDclustering::InitTask()
{
  // Initialize mapping

  std::string binmapfile = fConfig->GetValue<string>("binmapfile");

  // Load the mapping from the binary file
  if ( ! ReadMapping(binmapfile.c_str()) ) {
    LOG(ERROR) << "Could not read binary mapping file " << binmapfile.c_str();
    return;
  }

  // // prepare storage of clusters and preclusters
  for (UChar_t iPlane = 0; iPlane < 2; ++iPlane) {
    fPreClusters[iPlane].reserve(100);
    fClusters.reserve(100);
  }
}

//_________________________________________________________________________________________________
bool MIDclustering::LoadDigits ( FairMQMessagePtr& msg )
{
  /// fill the AliMUONHLTMapping::mpDE structure with fired pads

  uint8_t* data = reinterpret_cast<uint8_t*>(msg->GetData());
  uint32_t* digitsData = reinterpret_cast<uint32_t*>(data + 100);
  uint32_t nDigits = digitsData[0];
  uint32_t offset(1);

  bool hasDigits = false;

  // Loop on digits
  for (uint32_t idig=0; idig<nDigits; idig++) {
    // This can in principle change when we will have the final format
    // const DigitStruct *digit(reinterpret_cast<const DigitStruct*>(&digitsData[offset]));
    // offset += 2;
    // uint32_t uniqueID = digit->fId;
    uint32_t uniqueID = digitsData[offset++];
    uint32_t detElemId = uniqueID & 0xFFF;
    uint32_t boardId = (uniqueID & 0xFFF000) >> 12;
    uint32_t channel = (uniqueID & 0x3F000000) >> 24;
    uint32_t cathode = (uniqueID & 0x40000000) >> 30;
    ++offset; // skip idx and adc
    // uint32_t idxAdc = digitsData[offset++];
    // uint32_t idx = digit->fIndex;
    // uint32_t idx = idxAdc & 0xFFFF;
    // uint32_t adc = (idxAdc >> 16) & 0xFFFF;
    // uint32_t adc = digit->fADC;

    // Let us assume that from here we have a decoded digit

    // This check can be removed if we only pass the MID digits to the device
    if ( detElemId < 1100 ) continue;

    fActiveDEs[detElemId]=1;

    hasDigits = true;

    digiPads* de = NULL;

    try {
      de = &fMpDEs.at(detElemId);
    }
    catch ( int err ) {
      continue;
    }

    UChar_t iPlane = (cathode == de->mapping->iCath[0]) ? 0 : 1;
    UShort_t iPad = de->mapping->padIndices[iPlane][uniqueID];

    if (iPad == 0) {
      LOG(WARN) << "pad ID " << uniqueID << " does not exist in the mapping";
      continue;
    }
    --iPad;

    de->mapping->pads[iPad].useMe = kTRUE;

    // set this pad as fired
    if (de->nFiredPads[iPlane] < static_cast<UShort_t>(de->firedPads[iPlane].size()))
    {
      de->firedPads[iPlane][de->nFiredPads[iPlane]] = iPad;
    }
    else {
      de->firedPads[iPlane].push_back(iPad);
    }
    ++de->nFiredPads[iPlane];

    printf("DetElemId %i Board %i channel %i cathode %i (ID %i)  iPad %i\n", detElemId, boardId, channel, cathode, uniqueID, iPad);

  }

  return hasDigits;
}

//_________________________________________________________________________________________________
void MIDclustering::MakeCluster(preCluster& clBend, preCluster& clNonBend, Int_t &deId)
{
  /// Make cluster from pre-clusters

  if (fNClusters >= static_cast<uint32_t>(fClusters.size())) {
    cluster baseCl;
    fClusters.push_back(baseCl);
  }
  cluster &cl(fClusters[fNClusters]);
  ++fNClusters;

  cl.id = deId;

  Float_t x2[2], x3[2], sumArea;

  preCluster* pc[2] = {&clNonBend,&clBend};
  Float_t* coor[2] = {&(cl.xCoor),&(cl.yCoor)};
  Float_t* sigma[2] = {&(cl.sigmaX),&(cl.sigmaY)};

  for ( int iplane=0; iplane<2; ++iplane ) {
    bool fullCalc = true;
    x2[0] = x2[1] = 0.;
    x3[0] = x3[1] = 0.;
    sumArea = 0.;
    for ( int ic=0; ic<7; ++ic ) {
      UShort_t colMask = (1<<ic);
      if ( ( pc[iplane]->column & colMask ) == 0 ) continue;
      printf("Plane %i  column %i  masks 0x%x  area (%g,%g,%g,%g)\n",iplane,ic,pc[iplane]->column,pc[iplane]->area[ic][0][0],pc[iplane]->area[ic][0][1],pc[iplane]->area[ic][1][0],pc[iplane]->area[ic][1][1]); // REMEMBER TO CUT
      if ( pc[iplane]->column == colMask ) {
        // This is easy: we only have 1 block of strips
        // and since we check before that there is correspondence
        // between bending and non-bending plane
        // we can safely fill the cluster for this plane
        // This is actually the most common case
        // so, even if we could always use the general calculation below
        // it is better to separate the two cases to save time
        *(coor[iplane]) = 0.5 * ( pc[iplane]->area[ic][iplane][1] + pc[iplane]->area[ic][iplane][0] );
        *(sigma[iplane]) = ( pc[iplane]->area[ic][iplane][1] - pc[iplane]->area[ic][iplane][0] ) / fkSqrt12;
        fullCalc = false;
        break;
      }
      else if ( ( pc[1-iplane]->column & colMask ) == 0 ) continue;
      // we only build a cluster if the same column on both
      // the bending and non-bending plane is touched


      // This is the general case:
      // perform the full calculation assuming a uniform charge distribution
      Float_t dy = pc[iplane]->area[ic][1-iplane][1] - pc[iplane]->area[ic][1-iplane][0];
      Float_t dx = pc[iplane]->area[ic][iplane][1] - pc[iplane]->area[ic][iplane][0];
      sumArea += dx*dy;
      for ( int ip=0; ip<2; ++ip ) {
        Float_t val = pc[iplane]->area[ic][iplane][ip];
        x2[ip] += val*val*dy;
        x3[ip] += val*val*val*dy;
      }
    } // loop on column

    if ( fullCalc ) {
      Float_t currCoor = (x2[1]-x2[0])/sumArea/2.;
      *(coor[iplane]) = currCoor;
      Float_t sigma2 = (x3[1]-x3[0])/sumArea/3.- currCoor*currCoor;
      *(sigma[iplane]) = sqrt(sigma2);
    }
  }

  printf("pos: (%g, %g) err: (%g, %g)\n",cl.xCoor,cl.yCoor,cl.sigmaX,cl.sigmaY); // REMEMBER TO CUT
}

//_________________________________________________________________________________________________
void MIDclustering::MakeClusters ( Int_t deId )
{
  /// Make clusters and store it

  printf("\n Clusterizing %i\n",deId); // REMEMBER TO CUT

  Bool_t hasClusters = kFALSE;

  // loop over preclusters of the bending plane
  for ( int ipc1=0; ipc1<fNPreClusters[0]; ++ipc1 ) {
    hasClusters = kTRUE;
    // look for overlapping preclusters in the non-bending plane
    for ( int ipc2=0; ipc2<fNPreClusters[1]; ++ipc2 ) {
      if ( fPreClusters[0][ipc1].column & fPreClusters[1][ipc2].column ) {
        // if they have 1 column in common they surely overlap
        // printf("clusters %i and %i overlap!\n",cl1.firstPad,cl2.firstPad); // REMEMBER TO CUT
        MakeCluster(fPreClusters[0][ipc1],fPreClusters[1][ipc2],deId);
        fPreClusters[0][ipc1].useMe = kFALSE;
        fPreClusters[1][ipc2].useMe = kFALSE;
      }
    }
    // This is a monocathodic cluster in the bending plane
    if ( fPreClusters[0][ipc1].useMe ){
      // printf("cluster bend %i is monocathodic!\n",cl1.firstPad); // REMEMBER TO CUT
      MakeCluster(fPreClusters[0][ipc1],fPreClusters[0][ipc1],deId);
    }
  }

  // Search for monocathodic clusters in the non-bending plane
  for ( int ipc2=0; ipc2<fNPreClusters[1]; ++ipc2 ) {
    if ( fPreClusters[1][ipc2].useMe ) {
      // printf("cluster non-bend %i is monocathodic!\n",cl2.firstPad); // REMEMBER TO CUT
      MakeCluster(fPreClusters[1][ipc2],fPreClusters[1][ipc2],deId);
    }
  }
}

//_________________________________________________________________________________________________
void MIDclustering::PreClusterizeRecursive(digiPads &de)
{
  /// preclusterize both planes of every DE using recursive algorithm

  UShort_t iPad(0);

  // loop over planes
  for (UChar_t iPlane = 0; iPlane < 2; ++iPlane) {

    printf("\nPre-clusterizing %i  plane %i\n",de.mapping->id,iPlane); // REMEMBER TO CUT

    // loop over fired pads
    for (UShort_t iFiredPad = 0; iFiredPad < de.nFiredPads[iPlane]; ++iFiredPad) {

      iPad = de.firedPads[iPlane][iFiredPad];

      if (de.mapping->pads[iPad].useMe) {

        // create the precluster if needed
        if (fNPreClusters[iPlane] >= static_cast<UShort_t>(fPreClusters[iPlane].size())) {
          preCluster pcBase;
          fPreClusters[iPlane].push_back(pcBase);
        }

        printf("  new precluster:\n");

        // get the precluster
        preCluster &pc(fPreClusters[iPlane][fNPreClusters[iPlane]]);
        ++fNPreClusters[iPlane];

        // reset its content
        pc.column = 0;
        for ( int ic=0; ic<7; ++ic ) {
          for ( int ip=0; ip<2; ++ip ) {
            pc.area[ic][ip][0] = 1.e6;
            pc.area[ic][ip][1] = -1.e6;
          }
        }

        pc.useMe = kTRUE;

        // add the pad and its fired neighbours recusively
        // cl.firstPad = de.nOrderedPads;
        AddPad(de, iPad, pc);

      }

    }

  }

  // REMEMBER TO CUT
  // loop over planes
  // for (UChar_t iPlane = 0; iPlane < 2; ++iPlane) {
  //   printf("\nDE: %i  plane %i\n",de.mapping->id,iPlane);
  //   for ( auto& pc : fPreClusters[iPlane] ) {
  //     // Print some info on the clusters
  //     printf("\n"); // REMEMBER TO CUT
  //     for ( UShort_t idx=pc.firstPad; idx<=pc.lastPad; idx++ ) {
  //       printf("Adding pad %i\n",fMpDEs[de.mapping->id].orderedPads[idx]);
  //     }
  //   }
  // }
}

//_________________________________________________________________________________________________
bool MIDclustering::ReadMapping ( const char* filename )
{

  /// Read mapping

  auto tStart = std::chrono::high_resolution_clock::now();

  // std::vector<Mapping::mpDE> mpdeList = Mapping::ReadMapping(filename);
  int numberOfDetectionElements = 0;
  Mapping::mpDE* mpdeList = Mapping::ReadMapping(filename,numberOfDetectionElements);

  // if (mpdeList.size() == 0 ) {
  if ( numberOfDetectionElements == 0 ) {
    return false;
  }


  // for ( Mapping::mpDE mpde : mpdeList ) {
  for ( int impde=0; impde<numberOfDetectionElements; impde++ ) {

    // printf("mpDE id %i\n",mpde.id);
    Mapping::mpDE &mpde = mpdeList[impde];

    digiPads& de = fMpDEs[mpde.id];

    // de.mapping = mpde;
    de.mapping = &mpde;

    // Int_t initialSize = (mpde.nPads[0]/10+mpde.nPads[1]/10); // 10 % occupancy
    //
    // de.nOrderedPads = 0;
    // de.orderedPads.reserve(initialSize);

    for (UChar_t iCath = 0; iCath < 2; ++iCath) {
      de.nFiredPads[iCath] = 0;
      de.firedPads[iCath].reserve(mpde.nPads[iCath]/10); // 10% occupancy
    }
  }

  auto tEnd = std::chrono::high_resolution_clock::now();
  LOG(INFO) << "Read mapping in: " << std::chrono::duration<double, std::milli>(tEnd - tStart).count() << " ms\n";

  return true;
}

//_________________________________________________________________________________________________
void MIDclustering::ResetPadsAndClusters()
{
  /// reset fired pad and precluster information
  Mapping::mpPad *pad(NULL);

  // loop over DEs
  for ( auto &pair : fActiveDEs ) {
    Int_t deId = pair.first;

    digiPads& de(fMpDEs[deId]);

    printf("Reset %i\n",deId); // REMEMBER TO CUT

    // loop over planes
    for (UChar_t iPlane = 0; iPlane < 2; ++iPlane) {

      // loop over fired pads
      for (UShort_t iFiredPad = 0; iFiredPad < de.nFiredPads[iPlane]; ++iFiredPad) {

        pad = &de.mapping->pads[de.firedPads[iPlane][iFiredPad]];
        pad->useMe = kFALSE;

      }

      // clear number of fired pads
      de.nFiredPads[iPlane] = 0;

    }
    // clear ordered number of fired pads
    // de.nOrderedPads = 0;

  }

  fActiveDEs.clear();
  fNClusters = 0;

}

//_________________________________________________________________________________________________
Int_t MIDclustering::StoreClusters ( FairMQMessagePtr &msgOut )
{
  ///store clusters


  uint32_t* nCl = reinterpret_cast<uint32_t*>(msgOut->GetData());
  cluster* clusterData = reinterpret_cast<cluster*>(&nCl[1]);
  int icl = 0;
  for ( auto& cl : fClusters ) {
    cluster* currData = &clusterData[icl];
    currData->id = cl.id;
    currData->xCoor = cl.xCoor;
    currData->yCoor = cl.yCoor;
    currData->sigmaX = cl.sigmaX;
    currData->sigmaY = cl.sigmaY;
  }

  return 0;
}
