/* drvC9513.cpp
 *
 * Driver for Measurement Computing 9513 counters (e.g. USB-4303, etc.) using asynPortDriver base class
 *
 * Mark Rivers
 * July 26, 2011
*/

/* To do:
  - Digital pulse generator support
  - Read status register to know when it is done counting?
*/

#include <iocsh.h>
#include <epicsThread.h>

#include <asynPortDriver.h>

#include <devScalerAsyn.h>

#ifdef linux
  #include "cbw_linux.h"
#else
  #include "cbw.h"
#endif

#include <epicsExport.h>

static const char *driverName = "C9513";

// Per counter parameters
#define C9513GateControlString     "GATE_CONTROL"
#define C9513CounterEdgeString     "COUNTER_EDGE"
#define C9513CountSourceString     "COUNT_SOURCE"
#define C9513SpecialGateString     "SPECIAL_GATE"
#define C9513ReloadSourceString    "RELOAD_SOURCE"
#define C9513RecycleModeString     "RECYCLE_MODE"
#define C9513BCDModeString         "BCD_MODE"
#define C9513CountDirectionString  "COUNT_DIRECTION"
#define C9513OutputControlString   "OUTPUT_CONTROL"
#define C9513CounterValueString    "COUNTER_VALUE"
#define C9513LoadRegString         "LOAD_REG"
#define C9513HoldRegString         "HOLD_REG"
#define C9513PollCounterString     "POLL_COUNTER"
#define C9513PulseRunString        "PULSE_RUN"
#define C9513PulsePeriodString     "PULSE_PERIOD"
#define C9513PulseWidthString      "PULSE_WIDTH"
#define C9513PulseDelayString      "PULSE_DELAY"
#define C9513PulseCountString      "PULSE_COUNT"
#define C9513PulseIdleStateString  "PULSE_IDLE_STATE"
// Per chip parameters
#define C9513FOutDividerString     "FOUT_DIVIDER"
#define C9513FOutSourceString      "FOUT_SOURCE"
#define C9513Compare1String        "COMPARE1"
#define C9513Compare2String        "COMPARE2"
#define C9513TimeOfDayString       "TIME_OF_DAY"
#define C9513AlarmReg1String       "ALARM_REG1"
#define C9513AlarmReg2String       "ALARM_REG2"
// Per module parameters
#define DigitalInputString         "DIGITAL_INPUT"
#define DigitalOutputString        "DIGITAL_OUTPUT"
#define ScalerFrequencyString      "SCALER_FREQUENCY"    

#define MAX_SIGNALS 10
#define DEFAULT_POLL_TIME 0.05
#define COUNTERS_PER_CHIP 5

static const double frequencyChoices[] = {5000000., 500000., 50000., 5000., 500.};
#define MAX_FREQUENCIES 5

/** This is the class definition for the C9513 class
  */
class C9513 : public asynPortDriver {
public:
  C9513(const char *portName, int boardNum, int numChips);

  /* These are the methods that we override from asynPortDriver */
  virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
  virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
  virtual asynStatus writeUInt32Digital(asynUser *pasynUser, epicsUInt32 value, epicsUInt32 mask);
  virtual asynStatus readInt32Array(asynUser *pasynUser, epicsInt32 *data, size_t numRead, size_t *numActual);
  virtual void report(FILE *fp, int details);
  virtual void pollerThread(void);

protected:
  // Per counter parameters
  int C9513GateControl_; 
  int C9513CounterEdge_;
  int C9513CountSource_;
  int C9513SpecialGate_;
  int C9513ReloadSource_;
  int C9513RecycleMode_;
  int C9513BCDMode_;
  int C9513CountDirection_;
  int C9513OutputControl_;
  int C9513CounterValue_;
  int C9513LoadReg_;
  int C9513HoldReg_;
  int C9513PollCounter_;
  int C9513PulseRun_;
  int C9513PulsePeriod_;
  int C9513PulseWidth_;
  int C9513PulseDelay_;
  int C9513PulseCount_;
  int C9513PulseIdleState_;
  // Per chip parameters
  int C9513FOutDivider_;
  int C9513FOutSource_;
  int C9513Compare1_;
  int C9513Compare2_;
  int C9513TimeOfDay_;
  int C9513AlarmReg1_;
  int C9513AlarmReg2_;
  // Per module parameters
  int digitalInput_;
  int digitalOutput_;
  int scalerFrequency_;
  // Scaler record parameters
  int scalerReset_;
  int scalerChannels_;
  int scalerRead_;
  int scalerPresets_;
  int scalerArm_;
  int scalerDone_;  

private:
  int boardNum_;
  int numChips_;
  int numCounters_;
  int numScalers_;
  double pollTime_;
  int forceCallback_;
  bool counting_;
  int *scalerCounts_;
  int setupChip(int chipNum);
  int setupCounter(int counterNum);
  int loadCounterRegisters(int counterNum);
  int loadChipRegisters(int chipNum);
  void startScaler();
  void stopScaler();
  void readScalers();
  int setupPulseGenerator(int pulserNum);
};

static void pollerThreadC(void * pPvt)
{
    C9513 *pC9513 = (C9513 *)pPvt;
    pC9513->pollerThread();
}

C9513::C9513(const char *portName, int boardNum, int numChips)
  : asynPortDriver(portName, MAX_SIGNALS, 
      asynInt32Mask | asynUInt32DigitalMask | asynInt32ArrayMask | asynFloat64Mask | asynDrvUserMask,
      asynInt32Mask | asynUInt32DigitalMask | asynFloat64Mask, 
      ASYN_MULTIDEVICE, 1, /* ASYN_CANBLOCK=0, ASYN_MULTIDEVICE=1, autoConnect=1 */
      0, 0),  /* Default priority and stack size */
    boardNum_(boardNum),
    numChips_(numChips),
    pollTime_(DEFAULT_POLL_TIME),
    forceCallback_(1),
    counting_(false)
{
  //static const char *functionName = "C9513";
  int i;
  
  numCounters_ = 5 * numChips_;
  numScalers_ = 3 * numChips_;
  
  scalerCounts_ = (int *)malloc(numScalers_ * sizeof(int));
  
  // Per-counter parameters
  createParam(C9513GateControlString,               asynParamInt32, &C9513GateControl_);
  createParam(C9513CounterEdgeString,               asynParamInt32, &C9513CounterEdge_);
  createParam(C9513CountSourceString,               asynParamInt32, &C9513CountSource_);
  createParam(C9513SpecialGateString,               asynParamInt32, &C9513SpecialGate_);
  createParam(C9513ReloadSourceString,              asynParamInt32, &C9513ReloadSource_);
  createParam(C9513RecycleModeString,               asynParamInt32, &C9513RecycleMode_);
  createParam(C9513BCDModeString,                   asynParamInt32, &C9513BCDMode_);
  createParam(C9513CountDirectionString,            asynParamInt32, &C9513CountDirection_);
  createParam(C9513OutputControlString,             asynParamInt32, &C9513OutputControl_);
  createParam(C9513CounterValueString,              asynParamInt32, &C9513CounterValue_);
  createParam(C9513LoadRegString,                   asynParamInt32, &C9513LoadReg_);
  createParam(C9513HoldRegString,                   asynParamInt32, &C9513HoldReg_);
  createParam(C9513PollCounterString,               asynParamInt32, &C9513PollCounter_);
  createParam(C9513PulseRunString,                  asynParamInt32, &C9513PulseRun_);
  createParam(C9513PulsePeriodString,             asynParamFloat64, &C9513PulsePeriod_);
  createParam(C9513PulseWidthString,              asynParamFloat64, &C9513PulseWidth_);
  createParam(C9513PulseDelayString,              asynParamFloat64, &C9513PulseDelay_);
  createParam(C9513PulseCountString,                asynParamInt32, &C9513PulseCount_);
  createParam(C9513PulseIdleStateString,            asynParamInt32, &C9513PulseIdleState_);

  // Per-chip parameters
  createParam(C9513FOutDividerString,               asynParamInt32, &C9513FOutDivider_);
  createParam(C9513FOutSourceString,                asynParamInt32, &C9513FOutSource_);
  createParam(C9513Compare1String,                  asynParamInt32, &C9513Compare1_);
  createParam(C9513Compare2String,                  asynParamInt32, &C9513Compare2_);
  createParam(C9513TimeOfDayString,                 asynParamInt32, &C9513TimeOfDay_);
  createParam(C9513AlarmReg1String,                 asynParamInt32, &C9513AlarmReg1_);
  createParam(C9513AlarmReg2String,                 asynParamInt32, &C9513AlarmReg2_);

  // Per-module parameters
  createParam(DigitalInputString,           asynParamUInt32Digital, &digitalInput_);
  createParam(DigitalOutputString,          asynParamUInt32Digital, &digitalOutput_);
  createParam(ScalerFrequencyString,                asynParamInt32, &scalerFrequency_);
  
  // Scaler record parameters
  createParam(SCALER_RESET_COMMAND_STRING,          asynParamInt32, &scalerReset_);
  createParam(SCALER_CHANNELS_COMMAND_STRING,       asynParamInt32, &scalerChannels_);
  createParam(SCALER_READ_COMMAND_STRING,      asynParamInt32Array, &scalerRead_);
  createParam(SCALER_PRESET_COMMAND_STRING,         asynParamInt32, &scalerPresets_);
  createParam(SCALER_ARM_COMMAND_STRING,            asynParamInt32, &scalerArm_);
  createParam(SCALER_DONE_COMMAND_STRING,           asynParamInt32, &scalerDone_);
  
  // Set default values for all parameters
  setIntegerParam(scalerChannels_, numScalers_);
  for (i=0; i<numChips_; i++) {
    setIntegerParam(i, C9513FOutDivider_,  0);
    setIntegerParam(i, C9513FOutSource_,   FREQ1);
    setIntegerParam(i, C9513Compare1_,     DISABLED);
    setIntegerParam(i, C9513Compare2_,     DISABLED);
    setIntegerParam(i, C9513TimeOfDay_,    DISABLED);
    setupChip(i);
  }
  for (i=0; i<numCounters_; i++) {
    setIntegerParam(i, C9513GateControl_,  NOGATE);
    setIntegerParam(i, C9513CounterEdge_,  POSITIVEEDGE);
    setIntegerParam(i, C9513CountSource_,  FREQ1);
    setIntegerParam(i, C9513SpecialGate_,  DISABLED);
    setIntegerParam(i, C9513ReloadSource_, LOADREG);
    setIntegerParam(i, C9513RecycleMode_,  RECYCLE);
    setIntegerParam(i, C9513BCDMode_,      DISABLED);
    setIntegerParam(i, C9513CountDirection_, COUNTUP);
    setIntegerParam(i, C9513OutputControl_,  ALWAYSLOW);
    setupCounter(i);
  } 

  /* Start the thread to poll counters and digital inputs and do callbacks to 
   * device support */
  epicsThreadCreate("C9513Poller",
                    epicsThreadPriorityLow,
                    epicsThreadGetStackSize(epicsThreadStackMedium),
                    (EPICSTHREADFUNC)pollerThreadC,
                    this);
}

int C9513::setupChip(int chipNum)
{
  int fOutDivider, fOutSource, compare1, compare2, timeOfDay;
  int status;
  static const char *functionName = "setupChip";
  
  if (chipNum >= numChips_) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
              "%s:%s: invalid chip number=%d, must be 0-%d\n",
              driverName, functionName, chipNum, numChips_-1);
    return asynError;
  }
  getIntegerParam(chipNum, C9513FOutDivider_,  &fOutDivider);
  getIntegerParam(chipNum, C9513FOutSource_,   &fOutSource);
  getIntegerParam(chipNum, C9513Compare1_,     &compare1);
  getIntegerParam(chipNum, C9513Compare2_,     &compare2);
  getIntegerParam(chipNum, C9513TimeOfDay_,    &timeOfDay);
  status = cbC9513Init(boardNum_, chipNum+1, fOutDivider, fOutSource, compare1,
                       compare2, timeOfDay);
  if (status) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
              "%s:%s: error calling cbC9513Init for chip %d, error=%d\n",
              driverName, functionName, chipNum+1, status);
  }
  return status;
}

int C9513::setupCounter(int counterNum)
{
  int gateControl, counterEdge, countSource, specialGate, reloadSource;
  int recycleMode, BCDMode, countDirection, outputControl;
  int status;
  static const char *functionName = "setupCounter";
  
  if (counterNum >= numCounters_) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
              "%s:%s: invalid counter number=%d, must be 0-%d\n",
              driverName, functionName, counterNum, numCounters_-1);
    return asynError;
  }
  getIntegerParam(counterNum, C9513GateControl_,  &gateControl);
  getIntegerParam(counterNum, C9513CounterEdge_,  &counterEdge);
  getIntegerParam(counterNum, C9513CountSource_,  &countSource);
  getIntegerParam(counterNum, C9513SpecialGate_,  &specialGate);
  getIntegerParam(counterNum, C9513ReloadSource_, &reloadSource);
  getIntegerParam(counterNum, C9513RecycleMode_,  &recycleMode);
  getIntegerParam(counterNum, C9513BCDMode_,      &BCDMode);
  getIntegerParam(counterNum, C9513CountDirection_, &countDirection);
  getIntegerParam(counterNum, C9513OutputControl_,  &outputControl);
  status = cbC9513Config(boardNum_, counterNum+1, gateControl, counterEdge,
                         countSource, specialGate, reloadSource, recycleMode,
                         BCDMode, countDirection, outputControl);
  if (status) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
              "%s:%s: error calling cbC9513Config for counter %d, error=%d\n",
              driverName, functionName, counterNum+1, status);
  }
  return status;
}

int C9513::loadCounterRegisters(int counterNum)
{
  int status;
  int regNum;
  int value;
  static const char *functionName = "loadCounterRegister";
  
  if (counterNum >= numCounters_) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
              "%s:%s: invalid counter number=%d, must be 0-%d\n",
              driverName, functionName, counterNum, numCounters_-1);
    return asynError;
  }
  getIntegerParam(counterNum, C9513LoadReg_, &value);
  regNum = counterNum + 1;
  status = cbCLoad32(boardNum_, regNum, value);
  getIntegerParam(counterNum, C9513HoldReg_, &value);
  regNum = counterNum + 101;
  status |= cbCLoad32(boardNum_, regNum, value);
  if (status) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
              "%s:%s: error calling cbCLoad32 for counter %d, error=%d\n",
              driverName, functionName, counterNum+1, status);
  }
  return status;
}

int C9513::loadChipRegisters(int chipNum)
{
  int status;
  int regNum;
  int value;
  static const char *functionName = "loadChipRegister";
  
  if (chipNum >= numChips_) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
              "%s:%s: invalid chip number=%d, must be 0-%d\n",
              driverName, functionName, chipNum, numChips_-1);
    return asynError;
  }
  getIntegerParam(chipNum, C9513AlarmReg1_, &value);
  regNum = 201 + chipNum*100;
  status = cbCLoad32(boardNum_, regNum, value);
  getIntegerParam(chipNum, C9513AlarmReg2_, &value);
  regNum = 202 + chipNum*100;
  status |= cbCLoad32(boardNum_, regNum, value);
  if (status) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
              "%s:%s: error calling cbCLoad32 for chip %d, error=%d\n",
              driverName, functionName, chipNum+1, status);
  }
  return status;
}

void C9513::startScaler()
{
  int preset;
  int counter;
  int frequencyIndex;
  int i;
  //static const char *functionName = "startScaler";

  // We configure the counters as follows
  // Counter1 on each chip is 16-bit, counts down, one-shot
  // This is used as the time base on the first chip, ignorred on other chips
  // Counters 2&3 are configured as a 32-bit counter, as are 4&5.
  
  // Must reset the first chip so that the Toggle TC output has a defined state
  setupChip(0);
  for (i=0; i<numCounters_; i++) {
    counter = (i % COUNTERS_PER_CHIP);
    if (i == 0) {
      // The frequency source is selected in the database so the counter won't
      // overflow during the preset count time.  It is an index: 0=FREQ1, 1=FREQ2, etc.
      getIntegerParam(i, scalerFrequency_,     &frequencyIndex);
      setIntegerParam(i, C9513CountSource_,    FREQ1 + frequencyIndex);
      setIntegerParam(i, C9513RecycleMode_,    ONETIME);
      // Set the output mode to Low Pulse on TC, which will make the output
      // be high, which will inhibit all counters
      setIntegerParam(i, C9513OutputControl_,  LOWPULSEONTC);
      // Gate mode active low level
      setIntegerParam(i, C9513GateControl_,    ALLGATE);
      setIntegerParam(i, C9513CountDirection_, COUNTDOWN);
    }
    else if ((counter == 2) || (counter == 4)) {
      setIntegerParam(i, C9513CountSource_,    TCPREVCTR);
      setIntegerParam(i, C9513RecycleMode_,    ONETIME);
      setIntegerParam(i, C9513GateControl_,    NOGATE);
      setIntegerParam(i, C9513CountDirection_, COUNTUP);
    }
    else {
      setIntegerParam(i, C9513CountSource_,    counter+1);
      setIntegerParam(i, C9513RecycleMode_,    RECYCLE);
      setIntegerParam(i, C9513GateControl_,    ALLGATE);
      setIntegerParam(i, C9513CountDirection_, COUNTUP);
    }
    setIntegerParam(i, C9513BCDMode_,          DISABLED);
    setIntegerParam(i, C9513ReloadSource_,     LOADREG);
    setIntegerParam(i, C9513PollCounter_,      1);
    setupCounter(i);
  }

  // Arm all of the scalers by writing into their Load Register
  getIntegerParam(0, scalerPresets_, &preset);
  setIntegerParam(0, C9513LoadReg_, preset);
  loadCounterRegisters(0);
  for (i=1; i<numCounters_; i++) {
    setIntegerParam(i, C9513LoadReg_, 0);
    loadCounterRegisters(i);
  }
  // Start counting by changing output mode 1 to Toggle on TC
  setIntegerParam(0, C9513OutputControl_, TOGGLEONTC);
  setupCounter(0);
}

void C9513::stopScaler()
{
  //static const char *functionName = "stopScaler";

  setIntegerParam(0, C9513OutputControl_, LOWPULSEONTC);
  setupCounter(0);
}

void C9513::readScalers()
{
  int highCounts;
  int counter;
  int i;
  int preset;
  int scalerToCounter[] = {0, 1, 3, 5, 6, 8, 10, 11, 13, 15, 16, 18};
  //static const char *functionName = "readScalers";

  for (i=0; i<numScalers_; i++) {
    // We have 3 scalers per chip - counter 1 is 16-bit, 2/3 and 4/5 are 32-bit
    counter = scalerToCounter[i];
    getIntegerParam(counter, C9513CounterValue_, &scalerCounts_[i]);
    if ((counter % 5) != 0) {
      getIntegerParam(counter+1, C9513CounterValue_, &highCounts);
      scalerCounts_[i] += highCounts*65536;
    }
  }
  // Correct scaler 0 counts, because it starts at preset
  getIntegerParam(0, scalerPresets_, &preset);
  scalerCounts_[0] = preset - scalerCounts_[0];
  // When the scaler is done scalerCounts[0] = 1.  There should be a more robust way to tell if counting is done?
  if (scalerCounts_[0] == 1) {
    setIntegerParam(scalerDone_, 1);
    scalerCounts_[0] = preset;
    callParamCallbacks();
  }
}

int C9513::setupPulseGenerator(int counterNum)
{
  int status=0;
  int chip = counterNum / COUNTERS_PER_CHIP;
  double period, width, delay;
  double clockPeriod;
  int periodCount, widthCount;
  int count;
  int idleState;
  int pulseRun;
  int i;
  static const char *functionName = "setupPulseGenerator";
  
  if (counterNum >= numCounters_) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
              "%s:%s: invalid counter number=%d, must be 0-%d\n",
              driverName, functionName, counterNum, numCounters_-1);
    return asynError;
  }
  getDoubleParam (counterNum, C9513PulsePeriod_,    &period);
  getDoubleParam (counterNum, C9513PulseWidth_,     &width);
  getDoubleParam (counterNum, C9513PulseDelay_,     &delay);
  getIntegerParam(counterNum, C9513PulseCount_,     &count);
  getIntegerParam(counterNum, C9513PulseIdleState_, &idleState);
  getIntegerParam(counterNum, C9513PulseRun_,       &pulseRun);

  setIntegerParam(counterNum, C9513GateControl_,    NOGATE);
  setIntegerParam(counterNum, C9513SpecialGate_,    DISABLED);
  setIntegerParam(counterNum, C9513ReloadSource_,   LOADANDHOLDREG);
  setIntegerParam(counterNum, C9513PollCounter_,    0);
  if (pulseRun)
    setIntegerParam(counterNum, C9513RecycleMode_,  RECYCLE);
  else
    setIntegerParam(counterNum, C9513RecycleMode_,  ONETIME);
  setIntegerParam(counterNum, C9513CountDirection_, COUNTDOWN);
  setIntegerParam(counterNum, C9513OutputControl_,  TOGGLEONTC);
  // Find the highest frequency that will not overflow 65535 for the desired period
  for (i=0; i<MAX_FREQUENCIES; i++) {
     clockPeriod = 1. / frequencyChoices[i];
     periodCount = (int)((period / clockPeriod) + 0.5);
     if (periodCount <= 65535) break;
  }
  if (i == MAX_FREQUENCIES) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
              "%s:%s: pulse period=%f is outside allowed range for counter %d\n",
              driverName, functionName, period, counterNum+1);
  }
  setIntegerParam(counterNum, C9513CountSource_, FREQ1+i);
  widthCount = (int)(width / clockPeriod + 0.5);
  if (widthCount < 2) widthCount = 2; 
  if ((periodCount - widthCount) < 2) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
              "%s:%s: incompatible period=%f and width=%f for counter %d\n",
              driverName, functionName, period, width, counterNum+1);
  }
  if (idleState == 0) {
    setIntegerParam(counterNum, C9513HoldReg_, widthCount);
    setIntegerParam(counterNum, C9513LoadReg_, periodCount - widthCount);
  } else {
    setIntegerParam(counterNum, C9513LoadReg_, widthCount);
    setIntegerParam(counterNum, C9513HoldReg_, periodCount - widthCount);
  }
  status = setupChip(chip);
  status = setupCounter(counterNum);
  if (pulseRun) status = loadCounterRegisters(counterNum);
  return status;
}

asynStatus C9513::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
  int addr;
  int function = pasynUser->reason;
  int i;
  int status=0;
  static const char *functionName = "writeInt32";

  this->getAddress(pasynUser, &addr);
  setIntegerParam(addr, function, value);
  if ((function >= C9513FOutDivider_) &&
      (function <= C9513TimeOfDay_)) {
    status = setupChip(addr);
  } 
  else if ((function >= C9513GateControl_) &&
           (function <= C9513OutputControl_)) {
    status = setupCounter(addr);
  }
  else if ((function == C9513LoadReg_) ||
           (function == C9513HoldReg_)) {
    status = loadCounterRegisters(addr);
  }
  else if ((function == C9513AlarmReg1_) ||
           (function == C9513AlarmReg2_)) {
    status = loadChipRegisters(addr);
  }
  // Scaler commands
  else if (function == scalerReset_) {
    /* Clear all of the presets and counts*/
    for (i=0; i<numScalers_; i++) {
      setIntegerParam(i, scalerPresets_, 0);
    }
  }
  else if (function == scalerArm_) {
    /* Arm or disarm scaler */
    if (value != 0) {
      startScaler();
      counting_ = true;
    } else {
      stopScaler();
    }
    setIntegerParam(scalerDone_, 0);
  }
  // Pulse generator commands
  else if ((function == C9513PulseRun_) ||
           (function == C9513PulseCount_) ||
           (function == C9513PulseIdleState_)) {
     status = setupPulseGenerator(addr);
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

asynStatus C9513::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
  int addr;
  int function = pasynUser->reason;
  int status=0;
  static const char *functionName = "writeFloat64";

  this->getAddress(pasynUser, &addr);
  setDoubleParam(addr, function, value);
  if ((function == C9513PulsePeriod_) ||
      (function == C9513PulseWidth_)  ||
      (function == C9513PulseDelay_)) {
    status = setupPulseGenerator(addr);
  }
  callParamCallbacks(addr);
  if (status == 0) {
    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
             "%s:%s, port %s, wrote %f to address %d\n",
             driverName, functionName, this->portName, value, addr);
  } else {
    asynPrint(pasynUser, ASYN_TRACE_ERROR, 
             "%s:%s, port %s, ERROR writing %f to address %d, status=%d\n",
             driverName, functionName, this->portName, value, addr, status);
  }
  return (status==0) ? asynSuccess : asynError;
}

asynStatus C9513::writeUInt32Digital(asynUser *pasynUser, epicsUInt32 value, epicsUInt32 mask)
{
  int function = pasynUser->reason;
  int status;
  epicsUInt32 newValue, prevOutput;
  static const char *functionName = "writeUInt32Digital";

  status = getUIntDigitalParam(function, &prevOutput, 0xFFFFFFFF);
  if (status) {
    prevOutput = 0;
    status = 0;
  }
  /* Set any bits that are set in the value and the mask */
  newValue = prevOutput | (value & mask);
  /* Clear bits that are clear in the value and set in the mask */
  newValue &= (value | ~mask);
  status = cbDOut(boardNum_, AUXPORT, newValue);
  setUIntDigitalParam(digitalOutput_, newValue, mask);
  
  callParamCallbacks();
  if (status == 0) {
    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
             "%s:%s, port %s, wrote 0x%x mask=0x%x, newValue=0x%x\n",
             driverName, functionName, this->portName, value, mask, newValue);
  } else {
    asynPrint(pasynUser, ASYN_TRACE_ERROR, 
             "%s:%s, port %s, ERROR writing 0x%x mask=0x%x, newValue=0x%x, status=%d\n",
             driverName, functionName, this->portName, value, mask, newValue, status);
  }
  return (status==0) ? asynSuccess : asynError;
}

asynStatus C9513::readInt32Array(asynUser *pasynUser, epicsInt32 *data, 
                                      size_t numRead, size_t *numActual)
{
  int signal;
  int command = pasynUser->reason;
  asynStatus status = asynSuccess;
  int i;
  static const char* functionName="readInt32Array";

  pasynManager->getAddr(pasynUser, &signal);
  asynPrint(pasynUser, ASYN_TRACE_FLOW, 
            "%s:%s: entry, command=%d, signal=%d, numRead=%d, &data=%p\n", 
            driverName, functionName, command, signal, (int)numRead, data);

  if ((command == scalerRead_) && (numRead > 0)) {
    readScalers();
    for (i=0; (i<(int)numRead && i<numScalers_); i++) {
      data[i] = scalerCounts_[i];
    }
    for (i=numScalers_; i<(int)numRead; i++) {
      data[i] = 0;
    }
    *numActual = numRead;
    asynPrint(pasynUser, ASYN_TRACE_FLOW, 
              "%s:%s: scalerReadCommand: read %d chans, channel[0]=%d\n", 
              driverName, functionName, (int)numRead, data[0]);
  }
  else {
    asynPrint(pasynUser, ASYN_TRACE_ERROR,
              "%s:%s: got illegal command %d\n",
              driverName, functionName, command);
    status = asynError;
  }
  return status;
}

void C9513::pollerThread()
{
  /* This function runs in a separate thread.  It waits for the poll
   * time */
  //static const char *functionName = "pollerThread";
  epicsUInt32 newValue, changedBits, prevInput=0;
  unsigned short temp;
  int i;
  int pollCounter;
  ULONG count;
  int status;

  while(1) { 
    lock();
    status = cbDIn(boardNum_, AUXPORT, &temp);
    newValue = temp;
    changedBits = newValue ^ prevInput;
    if (forceCallback_ || (changedBits != 0)) {
      prevInput = newValue;
      forceCallback_ = 0;
      setUIntDigitalParam(digitalInput_, newValue, 0xFFFFFFFF);
    }
    for (i=0; i<numCounters_; i++) {
      getIntegerParam(i, C9513PollCounter_, &pollCounter);
      if (pollCounter) {
        status = cbCIn32(boardNum_, i+1, &count);
        setIntegerParam(i, C9513CounterValue_, count);
        callParamCallbacks(i);
      }
      callParamCallbacks(i);
    }
    unlock();
    epicsThreadSleep(pollTime_);
  }
}

/* Report  parameters */
void C9513::report(FILE *fp, int details)
{
  int i;
  
  asynPortDriver::report(fp, details);
  fprintf(fp, "  Port: %s, board number=%d, #chips=%d\n", 
          this->portName, boardNum_, numChips_);
  if (details >= 1) {
    fprintf(fp, "  scalerCounts = ");
    for (i=0; i<numScalers_; i++) 
      fprintf(fp, " %d", scalerCounts_[i]);
    fprintf(fp, "\n");
  }
}

/** Configuration command, called directly or from iocsh */
extern "C" int C9513Config(const char *portName, int boardNum, int numChips)
{
  new C9513(portName, boardNum, numChips);
  return asynSuccess;
}


static const iocshArg configArg0 = { "Port name",iocshArgString};
static const iocshArg configArg1 = { "Board number",iocshArgInt};
static const iocshArg configArg2 = { "# chips",iocshArgInt};
static const iocshArg * const configArgs[] = {&configArg0,
                                              &configArg1,
                                              &configArg2};
static const iocshFuncDef configFuncDef = {"C9513Config",3,configArgs};
static void configCallFunc(const iocshArgBuf *args)
{
  C9513Config(args[0].sval, args[1].ival, args[2].ival);
}

void drvC9513Register(void)
{
  iocshRegister(&configFuncDef,configCallFunc);
}

extern "C" {
epicsExportRegistrar(drvC9513Register);
}
