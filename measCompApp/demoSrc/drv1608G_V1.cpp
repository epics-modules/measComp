/* drvUSB1608G_V1.cpp
 *
 * Driver for Measurement Computing USB-1608G multi-function DAQ board using asynPortDriver base class
 *
 * This version implements only simple analog outputs
 *
 * Mark Rivers
 * April 14, 2012
*/

#include <iocsh.h>
#include <asynPortDriver.h>

#include "cbw.h"

#include <epicsExport.h>

static const char *driverName = "USB1608G";

// Analog output parameters
#define analogOutValueString      "ANALOG_OUT_VALUE"

#define NUM_ANALOG_OUT  2   // Number of analog outputs on 1608G
#define MAX_SIGNALS     NUM_ANALOG_OUT

/** Class definition for the USB1608G class
  */
class USB1608G : public asynPortDriver {
public:
  USB1608G(const char *portName, int boardNum);

  /* These are the methods that we override from asynPortDriver */
  virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
  virtual asynStatus getBounds(asynUser *pasynUser, epicsInt32 *low, epicsInt32 *high);

protected:
  // Analog output parameters
  int analogOutValue_;
  #define FIRST_USB1608G_PARAM  analogOutValue_
  #define LAST_USB1608G_PARAM   analogOutValue_

private:
  int boardNum_;
};

#define NUM_PARAMS ((int)(&LAST_USB1608G_PARAM - &FIRST_USB1608G_PARAM + 1))

/** Constructor for the USB1608G class
  */
USB1608G::USB1608G(const char *portName, int boardNum)
  : asynPortDriver(portName, MAX_SIGNALS, NUM_PARAMS, 
      asynInt32Mask | asynDrvUserMask,  // Interfaces that we implement
      0,                                // Interfaces that do callbacks
      ASYN_MULTIDEVICE | ASYN_CANBLOCK, 1, /* ASYN_CANBLOCK=1, ASYN_MULTIDEVICE=1, autoConnect=1 */
      0, 0),  /* Default priority and stack size */
    boardNum_(boardNum)
{
  // Analog output parameters
  createParam(analogOutValueString,            asynParamInt32, &analogOutValue_);
}

asynStatus USB1608G::getBounds(asynUser *pasynUser, epicsInt32 *low, epicsInt32 *high)
{
  int function = pasynUser->reason;

  // Analog outputs are 16-bit devices
  if (function == analogOutValue_) {
    *low = 0;
    *high = 65535;
    return(asynSuccess);
  } else {
    return(asynError);
  }
}


asynStatus USB1608G::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
  int addr;
  int function = pasynUser->reason;
  int status=0;
  static const char *functionName = "writeInt32";

  this->getAddress(pasynUser, &addr);
  setIntegerParam(addr, function, value);

  // Analog output functions
  if (function == analogOutValue_) {
    status = cbAOut(boardNum_, addr, BIP10VOLTS, value);
  }

  callParamCallbacks(addr);
  if (status == 0) {
    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
             "%s:%s, port %s, wrote %d to address %d\n",
             driverName, functionName, this->portName, value, addr);
  } else {
    asynPrint(pasynUser, ASYN_TRACE_ERROR, 
             "%s:%s, port %s, ERROR writing %d to address %d, status=%d\n",
             driverName, functionName, this->portName, value, addr, status);
  }
  return (status==0) ? asynSuccess : asynError;
}


/** Configuration command, called directly or from iocsh */
extern "C" int USB1608GConfig(const char *portName, int boardNum)
{
  USB1608G *pUSB1608G = new USB1608G(portName, boardNum);
  pUSB1608G = NULL;  /* This is just to avoid compiler warnings */
  return(asynSuccess);
}


static const iocshArg configArg0 = { "Port name",      iocshArgString};
static const iocshArg configArg1 = { "Board number",      iocshArgInt};
static const iocshArg * const configArgs[] = {&configArg0,
                                              &configArg1};
static const iocshFuncDef configFuncDef = {"USB1608GConfig", 2, configArgs};
static void configCallFunc(const iocshArgBuf *args)
{
  USB1608GConfig(args[0].sval, args[1].ival);
}

void drvUSB1608GRegister(void)
{
  iocshRegister(&configFuncDef,configCallFunc);
}

extern "C" {
epicsExportRegistrar(drvUSB1608GRegister);
}
