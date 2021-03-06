/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

// -------------------------------------------------------------------------
// -----                    PassiveContFact  file                    -----
// -----                Created 26/03/14  by M. Al-Turany              -----
// -------------------------------------------------------------------------


//*-- AUTHOR : Denis Bertini
//*-- Created : 21/06/2005

/////////////////////////////////////////////////////////////
//
//  PassiveContFact
//
//  Factory for the parameter containers in libPassive
//
/////////////////////////////////////////////////////////////
#include "DetectorsPassive/PassiveContFact.h"
#include "FairRuntimeDb.h"              // for FairRuntimeDb
#include "TList.h"                      // for TList
#include "TString.h"                    // for TString
#include <string.h>                     // for strcmp, NULL


using namespace std;
using namespace AliceO2::Passive;

ClassImp(AliceO2::Passive::PassiveContFact)

static PassiveContFact gPassiveContFact;

PassiveContFact::PassiveContFact()
  : FairContFact()
{
  // Constructor (called when the library is loaded)
  fName="PassiveContFact";
  fTitle="Factory for parameter containers in libPassive";
  setAllContainers();
  FairRuntimeDb::instance()->addContFactory(this);
}

void PassiveContFact::setAllContainers()
{
  /** Creates the Container objects with all accepted contexts and adds them to
   *  the list of containers for the STS library.*/

  FairContainer* p= new FairContainer("FairGeoPassivePar",
                                      "Passive Geometry Parameters",
                                      "TestDefaultContext");
  p->addContext("TestNonDefaultContext");

  containers->Add(p);
}

FairParSet* PassiveContFact::createContainer(FairContainer* c)
{
  /** Calls the constructor of the corresponding parameter container.
   * For an actual context, which is not an empty string and not the default context
   * of this container, the name is concatinated with the context. */
 /* const char* name=c->GetName();
  FairParSet* p=NULL;
  if (strcmp(name,"FairGeoPassivePar")==0) {
    p=new FairGeoPassivePar(c->getConcatName().Data(),c->GetTitle(),c->getContext());
  }
  return p;
*/
  return 0;
}
