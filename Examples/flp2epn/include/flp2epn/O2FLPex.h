/**
 * O2FLPex.h
 *
 * @since 2014-02-24
 * @author A. Rybalchenko
 */

#ifndef O2FLPEX_H_
#define O2FLPEX_H_

#include "FairMQDevice.h"

class O2FLPex : public FairMQDevice
{
  public:
    O2FLPex();

    virtual ~O2FLPex();

  protected:
    int fNumContent;

    virtual void InitTask();
    virtual bool ConditionalRun();
};

#endif
