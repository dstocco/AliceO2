#ifndef ALI_META_DATA_H
#define ALI_META_DATA_H

#include <TMap.h>     // for TMap
#include <TObject.h>  // for TObject
#include "Rtypes.h"   // for UInt_t, ConditionMetaData::Class, Bool_t, etc
#include "TString.h"  // for TString

namespace AliceO2 {
namespace CDB {
//  Set of data describing the object  				   //
//  but not used to identify the object 			   //

class ConditionMetaData : public TObject
{

  public:
    ConditionMetaData();

    ConditionMetaData(const char *responsible, UInt_t beamPeriod = 0, const char *alirootVersion = "",
                      const char *comment = "");

    virtual ~ConditionMetaData();

    void setObjectClassName(const char *name)
    {
      mObjectClassName = name;
    };

    const char *getObjectClassName() const
    {
      return mObjectClassName.Data();
    };

    void setResponsible(const char *yourName)
    {
      mResponsible = yourName;
    };

    const char *getResponsible() const
    {
      return mResponsible.Data();
    };

    void setBeamPeriod(UInt_t period)
    {
      mBeamPeriod = period;
    };

    UInt_t getBeamPeriod() const
    {
      return mBeamPeriod;
    };

    void setAliRootVersion(const char *version)
    {
      mAliRootVersion = version;
    };

    const char *getAliRootVersion() const
    {
      return mAliRootVersion.Data();
    };

    void setComment(const char *comment)
    {
      mComment = comment;
    };

    const char *getComment() const
    {
      return mComment.Data();
    };

    void addDateToComment();

    void setProperty(const char *property, TObject *object);

    TObject *getProperty(const char *property) const;

    Bool_t removeProperty(const char *property);

    void printConditionMetaData();

  private:
    TString mObjectClassName; // object's class name
    TString mResponsible;     // object's responsible person
    UInt_t mBeamPeriod;       // beam period
    TString mAliRootVersion;  // AliRoot version
    TString mComment;         // extra comments
    // TList mCalibRuns;

    TMap mProperties; // list of object specific properties

  ClassDef(ConditionMetaData, 1)
};
}
}
#endif
