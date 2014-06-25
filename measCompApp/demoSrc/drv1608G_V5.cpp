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

// Pulse output parameters
#define pulseGenRunString         "PULSE_RUN"
#define pulseGenPeriodString      "PULSE_PERIOD"
#define pulseGenWidthString       "PULSE_WIDTH"
#define pulseGenDelayString       "PULSE_DELAY"
#define pulseGenCountString       "PULSE_COUNT"
#define pulseGenIdleStateString   "PULSE_IDLE_STATE"

// Counter parameters
#define counterCountsString       "COUNTER_VALUE"
#define counterResetString        "COUNTER_RESET"

#define MIN_FREQUENCY   0.0149
#define MAX_FREQUENCY   32e6
#define MIN_DELAY       0.
#define MAX_DELAY       67.11
#define NUM_ANALOG_IN   16  // Number analog inputs on 1608G
#define NUM_ANALOG_OUT  2   // Number of analog outputs on 1608G
#define NUM_COUNTERS    2   // Number of counters on 1608G
#define NUM_TIMERS      1   // Number of timers on 1608G
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
  virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
  virtual asynStatus getBounds(asynUser *pasynUser, epicsInt32 *low, epicsInt32 *high);
  virtual asynStatus writeUInt32Digital(asynUser *pasynUser, epicsUInt32 value, epicsUInt32 mask);
  virtual void report(FILE *fp, int details);
  // These should be private but are called from C
  virtual void pollerThread(void);

protected:
  // Pulse generator parameters
  int pulseGenRun_;
  #define FIRST_USB1608G_PARAM pulseGenRun_
  int pulseGenPeriod_;
  int pulseGenWidth_;
  int pulseGenDelay_;
  int pulseGenCount_;
  int pulseGenIdleState_;
  
  // Counter parameters
  int counterCounts_;
  int counterReset_;
  
  // Analog output parameters
  int analogOutValue_;

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
  int startPulseGenerator();
  int stopPulseGenerator();
  int pulseGenRunning_;
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
      asynInt32Mask | asynFloat64Mask | asynUInt32DigitalMask | asynDrvUserMask,  // Interfaces that we implement
      asynInt32Mask | asynFloat64Mask | asynUInt32DigitalMask,                    // Interfaces that do callbacks
      ASYN_MULTIDEVICE | ASYN_CANBLOCK, 1, /* ASYN_CANBLOCK=1, ASYN_MULTIDEVICE=1, autoConnect=1 */
      0, 0),  /* Default priority and stack size */
    boardNum_(boardNum),
    pollTime_(DEFAULT_POLL_TIME),
    forceCallback_(1)
{
  // Pulse generator parameters
  createParam(pulseGenRunString,               asynParamInt32, &pulseGenRun_);
  createParam(pulseGenPeriodString,          asynParamFloat64, &pulseGenPeriod_);
  createParam(pulseGenWidthString,           asynParamFloat64, &pulseGenWidth_);
  createParam(pulseGenDelayString,           asynParamFloat64, &pulseGenDelay_);
  createParam(pulseGenCountString,             asynParamInt32, &pulseGenCount_);
  createParam(pulseGenIdleStateString,         asynParamInt32, &pulseGenIdleState_);

  // Counter parameters
  createParam(counterCountsString,             asynParamInt32, &counterCounts_);
  createParam(counterResetString,              asynParamInt32, &counterReset_);

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


int USB1608G::startPulseGenerator()
{
  int status=0;
  double frequency, period, width, delay;
  int timerNum=0;
  double dutyCycle;
  int count, idleState;
  static const char *functionName = "startPulseGenerator";
  
  getDoubleParam (timerNum, pulseGenPeriod_,    &period);
  getDoubleParam (timerNum, pulseGenWidth_,     &width);
  getDoubleParam (timerNum, pulseGenDelay_,     &delay);
  getIntegerParam(timerNum, pulseGenCount_,     &count);
  getIntegerParam(timerNum, pulseGenIdleState_, &idleState);
  
  frequency = 1./period;
  if (frequency < MIN_FREQUENCY) frequency = MIN_FREQUENCY;
  if (frequency > MAX_FREQUENCY) frequency = MAX_FREQUENCY;
  dutyCycle = width * frequency;
  period = 1. / frequency;
  if (dutyCycle <= 0.) dutyCycle = .0001;
  if (dutyCycle >= 1.) dutyCycle = .9999;
  if (delay < MIN_DELAY) delay = MIN_DELAY;
  if (delay > MAX_DELAY) delay = MAX_DELAY;

  status = cbPulseOutStart(boardNum_, timerNum, &frequency, &dutyCycle, count, &delay, idleState, 0);
  if (status != 0) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
      "%s:%s: started pulse generator %d period=%f, width=%f, count=%d, delay=%f, idleState=%d, status=%d\n",
      driverName, functionName, timerNum, period, width, count, delay, idleState, status);
    return status;
  }
  // We may not have gotten the frequency, dutyCycle, and delay we asked for, set the actual values
  // in the parameter library
  pulseGenRunning_ = 1;
  period = 1. / frequency;
  width = period * dutyCycle;
  asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
    "%s:%s: started pulse generator %d actual frequency=%f, actual period=%f, actual width=%f, actual delay=%f\n",
    driverName, functionName, timerNum, frequency, period, width, delay);
  setDoubleParam(timerNum, pulseGenPeriod_, period);
  setDoubleParam(timerNum, pulseGenWidth_, width);
  setDoubleParam(timerNum, pulseGenDelay_, delay);
  return 0;
}

int USB1608G::stopPulseGenerator()
{
  pulseGenRunning_ = 0;
  return cbPulseOutStop(boardNum_, 0);
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

  // Pulse generator functions
  if (function == pulseGenRun_) {
    // Allow starting a run even if it thinks its running,
    // since there is no way to know when it got done if Count!=0
    if (value) {
      status = startPulseGenerator();
    } 
    else if (!value && pulseGenRunning_) {
      status = stopPulseGenerator();
    }
  }
  if ((function == pulseGenCount_) ||
      (function == pulseGenIdleState_)) {
    if (pulseGenRunning_) {
      status = stopPulseGenerator();
      status |= startPulseGenerator();
    }
  }

  // Counter functions
  if (function == counterReset_) {
    // LOADREG0=0, LOADREG1=1, so we use addr
    status = cbCLoad32(boardNum_, addr, 0);
  }
  
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


asynStatus USB1608G::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
  int addr;
  int function = pasynUser->reason;
  int status=0;
  static const char *functionName = "writeFloat64";

  this->getAddress(pasynUser, &addr);
  setDoubleParam(addr, function, value);

  // Pulse generator functions
  if ((function == pulseGenPeriod_)    ||
      (function == pulseGenWidth_)     ||
      (function == pulseGenDelay_)) {
    if (pulseGenRunning_) {
      status = stopPulseGenerator();
      status |= startPulseGenerator();
    }
  }
  
  callParamCallbacks(addr);
  if (status == 0) {
    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
             "%s:%s, port %s, wrote %d to address %d\n",
             driverName, functionName, this->portName, value, addr);
  } else {
    asynPrint(pasynUser, ASYN_TRACE_ERROR, 
             "%s:%s, port %s, ERROR writing %f to address %d, status=%d\n",
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
  unsigned long countVal;
  int i;
  int status;

  while(1) { 
    lock();
    
    // Read the counter inputs
    for (i=0; i<NUM_COUNTERS; i++) {
      status = cbCIn32(boardNum_, i, &countVal);
      if (status)
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, 
                  "%s:%s: ERROR calling cbCIn32, status=%d\n", 
                  driverName, functionName, status);
      setIntegerParam(i, counterCounts_, countVal);
    }
    
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
