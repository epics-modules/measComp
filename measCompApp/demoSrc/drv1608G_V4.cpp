/* drvUSB1608G_V2.cpp
 *
 * Driver for Measurement Computing USB-1608G multi-function DAQ board using asynPortDriver base class
 *
 * This version implements digital inputs and outputs, simple analog inputs and simple analog outputs, will a poller thread
 *
 * Mark Rivers
 * April 14, 2012
*/

#include <iocsh.h>
#include <epicsThread.h>
#include <asynPortDriver.h>

#include "cbw.h"

#include <epicsExport.h>

static const char *driverName = "USB1608G";

// Analog output parameters
#define analogOutValueString      "ANALOG_OUT_VALUE"

// Analog input parameters
#define analogInValueString       "ANALOG_IN_VALUE"
#define analogInRangeString       "ANALOG_IN_RANGE"

// Digital I/O parameters
#define digitalDirectionString    "DIGITAL_DIRECTION"
#define digitalInputString        "DIGITAL_INPUT"
#define digitalOutputString       "DIGITAL_OUTPUT"

#define NUM_ANALOG_IN   16  // Number analog inputs on 1608G
#define NUM_ANALOG_OUT  2   // Number of analog outputs on 1608G
#define NUM_IO_BITS     8   // Number of digital I/O bits on 1608G
#define MAX_SIGNALS     NUM_ANALOG_IN
#define DEFAULT_POLL_TIME 0.01

/** Class definition for the USB1608G class
  */
class USB1608G : public asynPortDriver {
public:
  USB1608G(const char *portName, int boardNum);

  /* These are the methods that we override from asynPortDriver */
  virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
  virtual asynStatus readInt32(asynUser *pasynUser, epicsInt32 *value);
  virtual asynStatus getBounds(asynUser *pasynUser, epicsInt32 *low, epicsInt32 *high);
  virtual asynStatus writeUInt32Digital(asynUser *pasynUser, epicsUInt32 value, epicsUInt32 mask);
  virtual void report(FILE *fp, int details);
  // These should be private but are called from C
  virtual void pollerThread(void);

protected:
  // Analog output parameters
  int analogOutValue_;
  #define FIRST_USB1608G_PARAM  analogOutValue_

  // Analog input parameters
  int analogInValue_;
  int analogInRange_;

  // Digital I/O parameters
  int digitalDirection_;
  int digitalInput_;
  int digitalOutput_;
  #define LAST_USB1608G_PARAM digitalOutput_

private:
  int boardNum_;
  double pollTime_;
  int forceCallback_;
};

#define NUM_PARAMS ((int)(&LAST_USB1608G_PARAM - &FIRST_USB1608G_PARAM + 1))

static void pollerThreadC(void * pPvt)
{
    USB1608G *pUSB1608G = (USB1608G *)pPvt;
    pUSB1608G->pollerThread();
}


/** Constructor for the USB1608G class
  */
USB1608G::USB1608G(const char *portName, int boardNum)
  : asynPortDriver(portName, MAX_SIGNALS, NUM_PARAMS, 
      asynInt32Mask | asynUInt32DigitalMask | asynDrvUserMask,  // Interfaces that we implement
      asynUInt32DigitalMask,                                    // Interfaces that do callbacks
      ASYN_MULTIDEVICE | ASYN_CANBLOCK, 1, /* ASYN_CANBLOCK=1, ASYN_MULTIDEVICE=1, autoConnect=1 */
      0, 0),  /* Default priority and stack size */
    boardNum_(boardNum),
    pollTime_(DEFAULT_POLL_TIME),
    forceCallback_(1)
{
  // Analog output parameters
  createParam(analogOutValueString,            asynParamInt32, &analogOutValue_);

  // Analog input parameters
  createParam(analogInValueString,             asynParamInt32, &analogInValue_);
  createParam(analogInRangeString,             asynParamInt32, &analogInRange_);
  
  // Digital I/O parameters
  createParam(digitalDirectionString,  asynParamUInt32Digital, &digitalDirection_);
  createParam(digitalInputString,      asynParamUInt32Digital, &digitalInput_);
  createParam(digitalOutputString,     asynParamUInt32Digital, &digitalOutput_);

  /* Start the thread to poll digital inputs and do callbacks to 
   * device support */
  epicsThreadCreate("USB1608GPoller",
                    epicsThreadPriorityLow,
                    epicsThreadGetStackSize(epicsThreadStackMedium),
                    (EPICSTHREADFUNC)pollerThreadC,
                    this);
}

asynStatus USB1608G::getBounds(asynUser *pasynUser, epicsInt32 *low, epicsInt32 *high)
{
  int function = pasynUser->reason;

  // Both the analog outputs and analog inputs are 16-bit devices
  if ((function == analogOutValue_) ||
      (function == analogInValue_)) {
    *low = 0;
    *high = 65535;
    return(asynSuccess);
  } else {
    return(asynError);
  }
}


asynStatus USB1608G::readInt32(asynUser *pasynUser, epicsInt32 *value)
{
  int addr;
  int function = pasynUser->reason;
  int status=0;
  unsigned short shortVal;
  int range;
  //static const char *functionName = "readInt32";

  this->getAddress(pasynUser, &addr);

  // Analog input function
  if (function == analogInValue_) {
    getIntegerParam(addr, analogInRange_, &range);
    status = cbAIn(boardNum_, addr, range, &shortVal);
    *value = shortVal;
    setIntegerParam(addr, analogInValue_, *value);
  }

  // Other functions we call the base class method
  else {
     status = asynPortDriver::readInt32(pasynUser, value);
  }
  
  callParamCallbacks(addr);
  return (status==0) ? asynSuccess : asynError;
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


asynStatus USB1608G::writeUInt32Digital(asynUser *pasynUser, epicsUInt32 value, epicsUInt32 mask)
{
  int function = pasynUser->reason;
  int status=0;
  int i;
  epicsUInt32 outValue=0, outMask, direction=0;
  static const char *functionName = "writeUInt32Digital";


  setUIntDigitalParam(function, value, mask);
  if (function == digitalDirection_) {
    outValue = (value == 0) ? DIGITALIN : DIGITALOUT; 
    for (i=0; i<NUM_IO_BITS; i++) {
      if ((mask & (1<<i)) != 0) {
        status = cbDConfigBit(boardNum_, AUXPORT, i, outValue);
      }
    }
  }

  else if (function == digitalOutput_) {
    getUIntDigitalParam(digitalDirection_, &direction, 0xFFFFFFFF);
    for (i=0, outMask=1; i<NUM_IO_BITS; i++, outMask = (outMask<<1)) {
      // Only write the value if the mask has this bit set and the direction for that bit is output (1)
      outValue = ((value &outMask) == 0) ? 0 : 1; 
      if ((mask & outMask & direction) != 0) {
        status = cbDBitOut(boardNum_, AUXPORT, i, outValue);
      }
    }
  }
  
  callParamCallbacks();
  if (status == 0) {
    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
             "%s:%s, port %s, wrote outValue=0x%x, value=0x%x, mask=0x%x, direction=0x%x\n",
             driverName, functionName, this->portName, outValue, value, mask, direction);
  } else {
    asynPrint(pasynUser, ASYN_TRACE_ERROR, 
             "%s:%s, port %s, ERROR writing outValue=0x%x, value=0x%x, mask=0x%x, direction=0x%x, status=%d\n",
             driverName, functionName, this->portName, outValue, value, mask, direction, status);
  }
  return (status==0) ? asynSuccess : asynError;
}


void USB1608G::pollerThread()
{
  /* This function runs in a separate thread.  It waits for the poll time */
  static const char *functionName = "pollerThread";
  epicsUInt32 newValue, changedBits, prevInput=0;
  unsigned short biVal;;
  int i;
  int status;

  while(1) { 
    lock();
    
    // Read the digital inputs
    status = cbDIn(boardNum_, AUXPORT, &biVal);
    if (status) 
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, 
                "%s:%s: ERROR calling cbDIn, status=%d\n", 
                driverName, functionName, status);
    newValue = biVal;
    changedBits = newValue ^ prevInput;
    if (forceCallback_ || (changedBits != 0)) {
      prevInput = newValue;
      forceCallback_ = 0;
      setUIntDigitalParam(digitalInput_, newValue, 0xFFFFFFFF);
    }
    
    for (i=0; i<MAX_SIGNALS; i++) {
      callParamCallbacks(i);
    }
    unlock();
    epicsThreadSleep(pollTime_);
  }
}


/* Report  parameters */
void USB1608G::report(FILE *fp, int details)
{
  int i;
  int range;
  
  fprintf(fp, "  Port: %s, board number=%d\n", 
          this->portName, boardNum_);
  if (details >= 1) {
    for (i=0; i<NUM_ANALOG_IN; i++) {
      getIntegerParam(i, analogInRange_, &range); 
      fprintf(fp, "channel %d range=%d", i, range);
    }
    fprintf(fp, "\n");
  }
  asynPortDriver::report(fp, details);
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
