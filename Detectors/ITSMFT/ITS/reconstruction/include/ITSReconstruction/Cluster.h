/// \file Cluster.h
/// \brief Definition of the ITS cluster
#ifndef ALICEO2_ITS_CLUSTER_H
#define ALICEO2_ITS_CLUSTER_H

#include "ITSMFTReconstruction/Cluster.h"
#include "ITSBase/GeometryTGeo.h"

class TGeoHMatrix;

namespace AliceO2
{
namespace ITS
{
/// \class Cluster
/// \brief Cluster class for the ITS
///

class Cluster : public AliceO2::ITSMFT::Cluster
{
 public:
  Cluster();
  Cluster(const Cluster& cluster);
  virtual ~Cluster();

  Cluster& operator=(const Cluster& cluster) = delete;

  Int_t getLayer() const { return sGeom->getLayer(mVolumeId); }

  void goToFrameGlo();
  void goToFrameLoc();
  void goToFrameTrk();
  void getLocalXYZ(Float_t xyz[3]) const;
  void getTrackingXYZ(Float_t xyz[3]) const;
  //
  virtual void print(Option_t* option = "") const;
  virtual const TGeoHMatrix* getTracking2LocalMatrix() const;
  virtual TGeoHMatrix* getMatrix(Bool_t original = kFALSE) const;
  virtual Bool_t getGlobalXYZ(Float_t xyz[3]) const;
  virtual Bool_t getGlobalCov(Float_t cov[6]) const;
  virtual Bool_t getXRefPlane(Float_t& xref) const;
  virtual Bool_t getXAlphaRefPlane(Float_t& x, Float_t& alpha) const;
  //
  static void setGeom(GeometryTGeo* gm) { sGeom = gm; }
  static void setSortMode(SortMode_t md)
  {
    sMode &= ~kSortBits;
    sMode |= md;
  }
  static UInt_t getSortMode() { return sMode & kSortBits; }
  static UInt_t getMode() { return sMode; }
  static SortMode_t sortModeIdTrkYZ() { return kSortIdTrkYZ; }
  static SortMode_t sortModeIdLocXZ() { return kSortIdLocXZ; }
  //
  virtual Bool_t isSortable() const { return kTRUE; }
  virtual Bool_t isEqual(const TObject* obj) const;
  virtual Int_t Compare(const TObject* obj) const;
  //
 protected:
  //
  static UInt_t sMode;        //!< general mode (sorting mode etc)
  static GeometryTGeo* sGeom; //!< pointer on the geometry data

  ClassDef(Cluster, 1)
};
}
}

#endif /* ALICEO2_ITS_CLUSTER_H */
