#ifndef MIDclustering_h
#define MIDclustering_h


///
///  @file   MIDclustering
///  @author Diego Stocco <Diego.Stocco at cern.ch>
///  @date   24 October 2016
///  @brief  Cluster reconstruction processing device for MID
///

#include "FairMQDevice.h"
//#include "aliceHLTwrapper/MessageFormat.h"
#include "MUONBase/Mapping.h"
#include <vector>
#include <unordered_map>

namespace AliceO2 {

  namespace MUON {
    class MIDclustering : public FairMQDevice
    {
    public:
      MIDclustering();
      virtual ~MIDclustering();

    protected:
      bool HandleData(FairMQMessagePtr&, int);
      virtual void InitTask();

    private:

      struct digiPads {
        Mapping::mpDE* mapping;
        UShort_t nFiredPads[2]; // number of fired pads on each plane
        std::vector<UShort_t> firedPads[2]; // indices of fired pads on each plane
        // UShort_t nOrderedPads; // current number of fired pads in the following arrays
        // std::vector<UShort_t> orderedPads; // indices of fired pads ordered after preclustering and merging
      };

      // precluster structure
      struct preCluster {
        UShort_t column; // mask of fired columns
        // UShort_t firstPad; // index of first associated pad in the orderedPads array
        // UShort_t lastPad; // index of last associated pad in the orderedPads array
        Float_t area[7][2][2]; // 2D area containing the precluster per column
        Bool_t useMe; // kFALSE if precluster already merged to another one
      };

      // cluster structure
      struct cluster {
        Int_t id; // Index of the detection element
        Float_t xCoor; // local x coordinate
        Float_t yCoor; // local y coordinate
        Float_t sigmaX; // dispersion along x
        Float_t sigmaY; // dispersion along y
      };

      // Do not allow copying of this class.
      /// Not implemented.
      MIDclustering(const MIDclustering& /*obj*/);
      /// Not implemented.
      MIDclustering& operator = (const MIDclustering& /*obj*/);

      bool ReadMapping ( const char* );

      bool LoadDigits ( FairMQMessagePtr& );
      void ResetPadsAndClusters();

      void PreClusterizeRecursive(digiPads &de);
      void AddPad(digiPads &de, UShort_t iPad, preCluster &cl);

      void MakeClusters ( Int_t idDE );
      void MakeCluster ( preCluster &clx, preCluster &cly, Int_t &idDE );

      int StoreClusters ( FairMQMessagePtr &msgOut );

      std::unordered_map<Int_t,digiPads> fMpDEs; ///< internal mapping

      UShort_t fNPreClusters[2]; ///< number of preclusters in each cathods of each DE
      std::vector<preCluster> fPreClusters[2]; ///< list of preclusters in each cathode of each DE

      std::unordered_map<Int_t,Bool_t> fActiveDEs; ///< List of active detection elements for event
      std::vector<cluster> fClusters; ///< list of clusters
      uint32_t fNClusters; ///< Number of clusters

      const Float_t fkSqrt12; ///< Useful constant

    };
  }
}

#endif /* MIDclustering_h */
