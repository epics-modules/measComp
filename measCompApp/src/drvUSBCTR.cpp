/* drvUSBCTR.cpp
 *
 * Driver for Measurement Computing USB-CTR04/08 counter/timer module using asynPortDriver base class
 *
 * This driver supports simple digital in/out bit and word, timer (digital pulse generator), counter,
 *   EPICS scaler record, multi-channel scaler mode
 *
 * Mark Rivers
 * May 29, 2014
*/

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <iocsh.h>
#include <epicsThread.h>
#include <epicsTime.h>

#include <asynPortDriver.h>

#include "drvMca.h"
#include "devScalerAsyn.h"

#ifdef _WIN32
  #include "cbw.h"
#else
  #include "uldaq.h"
#endif

#include <epicsExport.h>
#include <measCompDiscover.h>

#define DRIVER_VERSION "4.2"

typedef enum {
  MCSPoint0Clear,
  MCSPoint0NoClear,
  MCSPoint0Skip
} MCSPoint0Action_t;

static const char *driverName = "USBCTR";

// Board parameters
#define modelNameString           "MODEL_NAME"
#define modelNumberString         "MODEL_NUMBER"
#define firmwareVersionString     "FIRMWARE_VERSION"
#define uniqueIDString            "UNIQUE_ID"
#define ULVersionString           "UL_VERSION"
#define driverVersionString       "DRIVER_VERSION"
#define pollSleepMSString         "POLL_SLEEP_MS"
#define pollTimeMSString          "POLL_TIME_MS"
#define lastErrorMessageString    "LAST_ERROR_MESSAGE"

// Pulse output parameters
#define pulseGenRunString         "PULSE_RUN"
#define pulseGenPeriodString      "PULSE_PERIOD"
#define pulseGenDutyCycleString   "PULSE_DUTY_CYCLE"
#define pulseGenDelayString       "PULSE_DELAY"
#define pulseGenCountString       "PULSE_COUNT"
#define pulseGenIdleStateString   "PULSE_IDLE_STATE"

// Counter parameters
#define counterCountsString       "COUNTER_VALUE"
#define counterResetString        "COUNTER_RESET"

// Trigger parameters
#define triggerModeString         "TRIGGER_MODE"

// Digital I/O parameters
#define digitalDirectionString    "DIGITAL_DIRECTION"
#define digitalInputString        "DIGITAL_INPUT"
#define digitalOutputString       "DIGITAL_OUTPUT"

// MCS parameters other than those in drvMca.h
#define MCSCurrentPointString     "MCS_CURRENT_POINT"
#define MCSMaxPointsString        "MCS_MAX_POINTS"
#define MCSTimeWFString           "MCS_TIME_WF"
#define MCSAbsTimeWFString        "MCS_ABS_TIME_WF"
#define MCSCounterEnableString    "MCS_COUNTER_ENABLE"
#define MCSPrescaleCounterString  "MCS_PRESCALE_COUNTER"
#define MCSPoint0ActionString     "MCS_POINT0_ACTION"

// Model ID
#define modelString               "MODEL"

#define MIN_FREQUENCY   0.023
#define MAX_FREQUENCY   48e6
#define MIN_DELAY       0.
#define MAX_DELAY       67.11
#define MAX_COUNTERS    8   // Maximum mumber of counters on USB-CTR04/08
#define MAX_MCS_COUNTERS (MAX_COUNTERS + 1) // +1 for collecting the digital I/O in MCS mode
#define DIGITAL_IO_COUNTER (MAX_MCS_COUNTERS-1) // Index of the digital I/O enable in mcsCounterEnable_ array;
#define MAX_DAQ_LEN     2*MAX_MCS_COUNTERS // Each counter can take 2 words
#define NUM_TIMERS      4   // Number of timers on USB-CTR08
#define NUM_IO_BITS     8   // Number of digital I/O bits on USB-CTR08
#define MAX_SIGNALS     MAX_MCS_COUNTERS
#define MAX_ERROR_STRING_LEN 256
#define MAX_BOARDNAME_LEN    256

#define DEFAULT_POLL_TIME 0.01
#define SINGLEIO_THRESHOLD_TIME 0.01  // Above this time uses SINGLEIO, below uses block I/O.

/** This is the class definition for the USBCTR class
  */
class USBCTR : public asynPortDriver {
public:
  USBCTR(const char *portName, const char *uniqueID, int maxTimePoints, double pollTime);

  /* These are the methods that we override from asynPortDriver */
  virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
  virtual asynStatus readInt32(asynUser *pasynUser, epicsInt32 *value);
  virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
  virtual asynStatus writeUInt32Digital(asynUser *pasynUser, epicsUInt32 value, epicsUInt32 mask);
  virtual asynStatus readInt32Array(asynUser *pasynUser, epicsInt32 *value, size_t nElements, size_t *nIn);
  virtual asynStatus readFloat32Array(asynUser *pasynUser, epicsFloat32 *value, size_t nElements, size_t *nIn);
  virtual asynStatus readFloat64Array(asynUser *pasynUser, epicsFloat64 *value, size_t nElements, size_t *nIn);
  virtual void report(FILE *fp, int details);
  // These should be private but are called from C
  virtual void pollerThread(void);

protected:
  // Model parameters
  int modelName_;
  int modelNumber_;
  int firmwareVersion_;
  int uniqueID_;
  int ULVersion_;
  int driverVersion_;
  int pollSleepMS_;
  int pollTimeMS_;
  int lastErrorMessage_;

  // Pulse generator parameters
  int pulseGenRun_;
  int pulseGenPeriod_;
  int pulseGenDutyCycle_;
  int pulseGenDelay_;
  int pulseGenCount_;
  int pulseGenIdleState_;

  // Counter parameters
  int counterCounts_;
  int counterReset_;

  // Trigger parameters
  int triggerMode_;

  // Digital I/O parameters
  int digitalDirection_;
  int digitalInput_;
  int digitalOutput_;

  // MCS parameters other than those in drvMca.h
  int MCSCurrentPoint_;
  int MCSMaxPoints_;
  int MCSTimeWF_;
  int MCSAbsTimeWF_;
  int MCSCounterEnable_;
  int MCSPrescaleCounter_;
  int MCSPoint0Action_;

  // Command for EPICS MCA record
  int mcaStartAcquire_;
  int mcaStopAcquire_;
  int mcaErase_;
  int mcaData_;
  int mcaReadStatus_;
  int mcaChannelAdvanceSource_;
  int mcaNumChannels_;
  int mcaDwellTime_;
  int mcaPresetLiveTime_;
  int mcaPresetRealTime_;
  int mcaPresetCounts_;
  int mcaPresetLowChannel_;
  int mcaPresetHighChannel_;
  int mcaPresetSweeps_;
  int mcaAcquireMode_;
  int mcaSequence_;
  int mcaPrescale_;
  int mcaAcquiring_;
  int mcaElapsedLiveTime_;
  int mcaElapsedRealTime_;
  int mcaElapsedCounts_;

  // Commands for EPICS scaler record
  int scalerReset_;
  int scalerChannels_;
  int scalerRead_;
  int scalerPresets_;
  int scalerArm_;
  int scalerDone_;

// Model ID
  int model_;

private:
  int boardType_;
  #ifdef _WIN32
    int boardNum_;
  #else
    DaqDeviceHandle daqDeviceHandle_;
  #endif
  DaqDeviceDescriptor daqDeviceDescriptor_;
  char boardName_[MAX_BOARDNAME_LEN];
  double pollTime_;
  int forceCallback_;
  int numCounters_;
  int numMCSCounters_;
  int maxTimePoints_;
  epicsInt32 scalerCounts_[MAX_COUNTERS];
  epicsInt32 scalerPresetCounts_[MAX_COUNTERS];
  epicsInt32 *MCSBuffer_[MAX_MCS_COUNTERS];
  bool mcsCounterEnable_[MAX_MCS_COUNTERS];
  short chanArray_[MAX_DAQ_LEN];
  short chanTypeArray_[MAX_DAQ_LEN];
  short gainArray_[MAX_DAQ_LEN];

  epicsFloat32 *MCSTimeBuffer_;
  epicsFloat64 *MCSAbsTimeBuffer_;
  epicsFloat64 *pCountsF64_;
  epicsUInt64 *pCountsUI64_;
  epicsInt32 *pCountsI32_;
  epicsInt16 *pCountsI16_;
  int counterBits_;

  bool pulseGenRunning_[NUM_TIMERS];
  bool scalerRunning_;
  bool MCSRunning_;
  bool MCSErased_;
  epicsTimeStamp startTime_;
  double elapsedPrevious_;
  char errorMessage_[MAX_ERROR_STRING_LEN];

  char *getErrorMessage(int error);
  int startPulseGenerator(int timerNum);
  int stopPulseGenerator(int timerNum);
  int resetScaler();
  int startScaler();
  int readScaler();
  int stopScaler();
  int clearScalerPresets();
  int setScalerPresets();
  int startMCS();
  int stopMCS();
  int readMCS();
  int eraseMCS();
  int computeMCSTimes();
};

static void pollerThreadC(void * pPvt)
{
    USBCTR *pUSBCTR = (USBCTR *)pPvt;
    pUSBCTR->pollerThread();
}

USBCTR::USBCTR(const char *portName, const char *uniqueID, int maxTimePoints, double pollTime)
  : asynPortDriver(portName, MAX_SIGNALS,
      asynInt32Mask | asynUInt32DigitalMask | asynInt32ArrayMask | asynFloat32ArrayMask | asynFloat64Mask | asynFloat64ArrayMask | asynOctetMask |asynDrvUserMask,
      asynInt32Mask | asynUInt32DigitalMask | asynInt32ArrayMask | asynFloat32ArrayMask | asynFloat64Mask | asynFloat64ArrayMask | asynOctetMask,
      // Note: ASYN_CANBLOCK must not be set because the scaler record does not work with asynchronous device support
      ASYN_MULTIDEVICE, 1, /* ASYN_CANBLOCK=0, ASYN_MULTIDEVICE=1, autoConnect=1 */
      0, 0),  /* Default priority and stack size */
    pollTime_((pollTime > 0.) ? pollTime : DEFAULT_POLL_TIME),
    forceCallback_(1),
    maxTimePoints_(maxTimePoints),
    scalerRunning_(false),
    MCSRunning_(false)
{
  int i;
  int status;
  long long handle;
  //static const char *functionName = "USBCTR";

  for (i=0; i<NUM_TIMERS; i++) pulseGenRunning_[i]=0;

  status = measCompCreateDevice(uniqueID, daqDeviceDescriptor_, &handle);
  if (status) {
    printf("Error creating device with measCompCreateDevice\n");
    return;
  }

  #ifdef _WIN32
    boardNum_ = (int) handle;
    strcpy(boardName_, daqDeviceDescriptor_.ProductName);
    boardType_ = daqDeviceDescriptor_.ProductID;
  #else
    daqDeviceHandle_ = handle;
    strcpy(boardName_, daqDeviceDescriptor_.productName);
    boardType_ = daqDeviceDescriptor_.productId;
  #endif

  // Model parameters
  createParam(modelNameString,                 asynParamOctet,  &modelName_);
  createParam(modelNumberString,               asynParamInt32,  &modelNumber_);
  createParam(firmwareVersionString,            asynParamOctet, &firmwareVersion_);
  createParam(uniqueIDString,                   asynParamOctet, &uniqueID_);
  createParam(ULVersionString,                  asynParamOctet, &ULVersion_);
  createParam(driverVersionString,              asynParamOctet, &driverVersion_);
  createParam(pollSleepMSString,              asynParamFloat64, &pollSleepMS_);
  createParam(pollTimeMSString,               asynParamFloat64, &pollTimeMS_);
  createParam(lastErrorMessageString,           asynParamOctet, &lastErrorMessage_);

  // Pulse generator parameters
  createParam(pulseGenRunString,               asynParamInt32, &pulseGenRun_);
  createParam(pulseGenPeriodString,          asynParamFloat64, &pulseGenPeriod_);
  createParam(pulseGenDutyCycleString,       asynParamFloat64, &pulseGenDutyCycle_);
  createParam(pulseGenDelayString,           asynParamFloat64, &pulseGenDelay_);
  createParam(pulseGenCountString,             asynParamInt32, &pulseGenCount_);
  createParam(pulseGenIdleStateString,         asynParamInt32, &pulseGenIdleState_);

  // Counter parameters
  createParam(counterCountsString,             asynParamInt32, &counterCounts_);
  createParam(counterResetString,              asynParamInt32, &counterReset_);

  // Trigger parameters
  createParam(triggerModeString,               asynParamInt32, &triggerMode_);

  // Digital I/O parameters
  createParam(digitalDirectionString,  asynParamUInt32Digital, &digitalDirection_);
  createParam(digitalInputString,      asynParamUInt32Digital, &digitalInput_);
  createParam(digitalOutputString,     asynParamUInt32Digital, &digitalOutput_);

  // MCS parameters other than those in drvMca.h
  createParam(MCSCurrentPointString,           asynParamInt32, &MCSCurrentPoint_);
  createParam(MCSMaxPointsString,              asynParamInt32, &MCSMaxPoints_);
  createParam(MCSTimeWFString,          asynParamFloat32Array, &MCSTimeWF_);
  createParam(MCSAbsTimeWFString,       asynParamFloat64Array, &MCSAbsTimeWF_);
  createParam(MCSCounterEnableString,  asynParamUInt32Digital, &MCSCounterEnable_);
  createParam(MCSPrescaleCounterString,        asynParamInt32, &MCSPrescaleCounter_);
  createParam(MCSPoint0ActionString,           asynParamInt32, &MCSPoint0Action_);

  // MCA record parameters
  createParam(mcaStartAcquireString,                asynParamInt32, &mcaStartAcquire_);
  createParam(mcaStopAcquireString,                 asynParamInt32, &mcaStopAcquire_);            /* int32, write */
  createParam(mcaEraseString,                       asynParamInt32, &mcaErase_);                  /* int32, write */
  createParam(mcaDataString,                   asynParamInt32Array, &mcaData_);                   /* int32Array, read/write */
  createParam(mcaReadStatusString,                  asynParamInt32, &mcaReadStatus_);             /* int32, write */
  createParam(mcaChannelAdvanceSourceString,        asynParamInt32, &mcaChannelAdvanceSource_);   /* int32, write */
  createParam(mcaNumChannelsString,                 asynParamInt32, &mcaNumChannels_);            /* int32, write */
  createParam(mcaDwellTimeString,                 asynParamFloat64, &mcaDwellTime_);              /* float64, write */
  createParam(mcaPresetLiveTimeString,            asynParamFloat64, &mcaPresetLiveTime_);         /* float64, write */
  createParam(mcaPresetRealTimeString,            asynParamFloat64, &mcaPresetRealTime_);         /* float64, write */
  createParam(mcaPresetCountsString,              asynParamFloat64, &mcaPresetCounts_);           /* float64, write */
  createParam(mcaPresetLowChannelString,            asynParamInt32, &mcaPresetLowChannel_);       /* int32, write */
  createParam(mcaPresetHighChannelString,           asynParamInt32, &mcaPresetHighChannel_);      /* int32, write */
  createParam(mcaPresetSweepsString,                asynParamInt32, &mcaPresetSweeps_);           /* int32, write */
  createParam(mcaAcquireModeString,                 asynParamInt32, &mcaAcquireMode_);            /* int32, write */
  createParam(mcaSequenceString,                    asynParamInt32, &mcaSequence_);               /* int32, write */
  createParam(mcaPrescaleString,                    asynParamInt32, &mcaPrescale_);               /* int32, write */
  createParam(mcaAcquiringString,                   asynParamInt32, &mcaAcquiring_);              /* int32, read */
  createParam(mcaElapsedLiveTimeString,           asynParamFloat64, &mcaElapsedLiveTime_);        /* float64, read */
  createParam(mcaElapsedRealTimeString,           asynParamFloat64, &mcaElapsedRealTime_);        /* float64, read */
  createParam(mcaElapsedCountsString,             asynParamFloat64, &mcaElapsedCounts_);          /* float64, read */

  // Scaler record parameters
  createParam(SCALER_RESET_COMMAND_STRING,          asynParamInt32, &scalerReset_);               /* int32, write */
  createParam(SCALER_CHANNELS_COMMAND_STRING,       asynParamInt32, &scalerChannels_);            /* int32, read */
  createParam(SCALER_READ_COMMAND_STRING,      asynParamInt32Array, &scalerRead_);                /* int32Array, read */
  createParam(SCALER_PRESET_COMMAND_STRING,         asynParamInt32, &scalerPresets_);             /* int32, write */
  createParam(SCALER_ARM_COMMAND_STRING,            asynParamInt32, &scalerArm_);                 /* int32, write */
  createParam(SCALER_DONE_COMMAND_STRING,           asynParamInt32, &scalerDone_);                /* int32, read */

  // Model ID
  createParam(modelString,                          asynParamInt32, &model_);                     /* int32, read */

  if (strcmp(boardName_, "USB-CTR08") == 0) {
    setIntegerParam(model_, 0);
    numCounters_ = 8;
  } else if (strcmp(boardName_, "USB-CTR04") == 0) {
    setIntegerParam(model_, 1);
    numCounters_ = 4;
  } else {
    printf("Unknown model\n");
  }

  char uniqueIDStr[256];
  char firmwareVersion[256];
  char ULVersion[256];
  #ifdef _WIN32
    int size = sizeof(uniqueIDStr);
    cbGetConfigString(BOARDINFO, boardNum_, 0, BIDEVUNIQUEID, uniqueIDStr, &size);
    size = sizeof(firmwareVersion);
    cbGetConfigString(BOARDINFO, boardNum_, VER_FW_MAIN, BIDEVVERSION, firmwareVersion, &size);
    float DLLRevNum, VXDRevNum;
    cbGetRevision(&DLLRevNum, &VXDRevNum);
    sprintf(ULVersion, "%f %f", DLLRevNum, VXDRevNum);
  #else
    strcpy(uniqueIDStr, uniqueID);
    unsigned int size = sizeof(firmwareVersion);
    ulDevGetConfigStr(daqDeviceHandle_, ::DEV_CFG_VER_STR, DEV_VER_FW_MAIN, firmwareVersion, &size);
    size = sizeof(ULVersion);
    ulGetInfoStr(UL_INFO_VER_STR, 0, ULVersion, &size);
  #endif
  setIntegerParam(modelNumber_, boardType_);
  setStringParam(modelName_, boardName_);
  setStringParam(uniqueID_, uniqueIDStr);
  setStringParam(firmwareVersion_, firmwareVersion);
  setStringParam(ULVersion_, ULVersion);
  setStringParam(driverVersion_, DRIVER_VERSION);
  
  // Allocate memory for the input buffers
  for (i=0; i<MAX_MCS_COUNTERS; i++) {
    MCSBuffer_[i]  = (epicsInt32 *) calloc(maxTimePoints_,  sizeof(epicsInt32));
  }
  MCSTimeBuffer_    = (epicsFloat32 *) calloc(maxTimePoints_,  sizeof(epicsFloat32));
  MCSAbsTimeBuffer_ = (epicsFloat64 *) calloc(maxTimePoints_,  sizeof(epicsFloat64));
  for (i=0; i<MAX_DAQ_LEN; i++) {
    gainArray_[i] = BIP10VOLTS;
  }
  pCountsF64_  = (epicsFloat64 *) calloc((maxTimePoints+1)  * MAX_MCS_COUNTERS, sizeof(epicsFloat64));
  pCountsUI64_ = (epicsUInt64 *)pCountsF64_;
  pCountsI32_ = (epicsInt32 *)pCountsF64_;
  pCountsI16_ = (epicsInt16 *)pCountsF64_;

  // Set values of some parameters that need to be set because init record order is not predictable
  // or because the corresponding records are PINI=NO.
  setIntegerParam(pulseGenRun_, 0);
  setIntegerParam(scalerDone_, 1);
  setIntegerParam(scalerChannels_, numCounters_);
  setIntegerParam(MCSMaxPoints_, maxTimePoints_);
  setIntegerParam(mcaNumChannels_, maxTimePoints_);
  resetScaler();
  clearScalerPresets();
  MCSErased_ = false;

  eraseMCS();

  // Put pulse generators in known state
  for (i=0; i<NUM_TIMERS; i++) {
    stopPulseGenerator(i);
  }

  /* Start the thread to poll counters and digital inputs and do callbacks to
   * device support */
  epicsThreadCreate("USBCTRPoller",
                    epicsThreadPriorityLow,
                    epicsThreadGetStackSize(epicsThreadStackMedium),
                    (EPICSTHREADFUNC)pollerThreadC,
                    this);
}

char *USBCTR::getErrorMessage(int error)
{
  #ifdef _WIN32
      cbGetErrMsg(error, errorMessage_);
  #else
      ulGetErrMsg((UlError)error, errorMessage_);
  #endif
  return errorMessage_;
}

int USBCTR::startPulseGenerator(int timerNum)
{
  int status=0;
  double frequency, period, delay;
  double dutyCycle;
  int count, idleState;
  static const char *functionName = "startPulseGenerator";

  getDoubleParam (timerNum, pulseGenPeriod_,    &period);
  getDoubleParam (timerNum, pulseGenDutyCycle_, &dutyCycle);
  getDoubleParam (timerNum, pulseGenDelay_,     &delay);
  getIntegerParam(timerNum, pulseGenCount_,     &count);
  getIntegerParam(timerNum, pulseGenIdleState_, &idleState);

  frequency = 1./period;
  if (frequency < MIN_FREQUENCY) frequency = MIN_FREQUENCY;
  if (frequency > MAX_FREQUENCY) frequency = MAX_FREQUENCY;
  period = 1. / frequency;
  if (dutyCycle <= 0.) dutyCycle = .0001;
  if (dutyCycle >= 1.) dutyCycle = .9999;
  if (delay < MIN_DELAY) delay = MIN_DELAY;
  if (delay > MAX_DELAY) delay = MAX_DELAY;

  #ifdef _WIN32
    status = cbPulseOutStart(boardNum_, timerNum, &frequency, &dutyCycle, count, &delay, idleState, 0);
  #else
    TmrIdleState idle = (idleState == 0) ? TMRIS_LOW : TMRIS_HIGH;
    status = ulTmrPulseOutStart(daqDeviceHandle_, timerNum, &frequency, &dutyCycle, count, &delay, idle, PO_DEFAULT);
  #endif
  if (status != 0) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
      "%s::%s error calling cbPulseOutStart timerNum=%d frequency=%f, dutyCycle=%f,"
      " count=%d, delay=%f, idleState=%d, status=%d, error=%s\n",
      driverName, functionName, timerNum, frequency, dutyCycle,
      count, delay, idleState, status, getErrorMessage(status));
    return status;
  }
  // We may not have gotten the frequency, dutyCycle, and delay we asked for, set the actual values
  // in the parameter library
  pulseGenRunning_[timerNum] = true;
  period = 1. / frequency;
  asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
    "%s:%s: started pulse generator %d actual frequency=%f, actual period=%f, actual duty cycle=%f, actual delay=%f\n",
    driverName, functionName, timerNum, frequency, period, dutyCycle, delay);
  setDoubleParam(timerNum, pulseGenPeriod_, period);
  setDoubleParam(timerNum, pulseGenDutyCycle_, dutyCycle);
  setDoubleParam(timerNum, pulseGenDelay_, delay);
  return 0;
}

int USBCTR::stopPulseGenerator(int timerNum)
{
  int status;
  static const char *functionName = "stopPulseGenerator";

  pulseGenRunning_[timerNum] = false;
  #ifdef _WIN32
    status = cbPulseOutStop(boardNum_, timerNum);
  #else
    status = ulTmrPulseOutStop(daqDeviceHandle_, timerNum);
  #endif
  if (status != 0) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
      "%s::%s error calling cbPulseOutStop timerNum=%d, status=%d, error=%s\n",
      driverName, functionName, timerNum, status, getErrorMessage(status));
  }
  return status;
}

int USBCTR::startMCS()
{
  int numPoints;
  int i;
  int options;
  int status;
  epicsUInt32 counterEnable;
  int prescale;
  int prescaleCounter;
  int mode;
  int point0Action;
  double dwell;
  int channelAdvance;
  static const char *functionName = "startMCS";

  getIntegerParam(MCSPrescaleCounter_, &prescaleCounter);
  getIntegerParam(mcaPrescale_, &prescale);
  getIntegerParam(mcaChannelAdvanceSource_, &channelAdvance);
  getUIntDigitalParam(MCSCounterEnable_,  &counterEnable, 0xFFFFFFFF);

  for (i=0; i<MAX_MCS_COUNTERS; i++) {
    mcsCounterEnable_[i] = (counterEnable & (1<<i)) ? true : false;
  }
  numMCSCounters_ = 0;
  for (i=0; i<numCounters_; i++) {
    if (!mcsCounterEnable_[i]) continue;
    numMCSCounters_++;
    #ifdef _WIN32
      mode = OUTPUT_ON | CLEAR_ON_READ;
      status = cbCConfigScan(boardNum_, i, mode, CTR_DEBOUNCE_NONE, CTR_TRIGGER_BEFORE_STABLE,
                             CTR_RISING_EDGE, CTR_TICK20PT83ns, 0);
    #else
      mode = CMM_OUTPUT_ON | CMM_CLEAR_ON_READ;
      status = ulCConfigScan(daqDeviceHandle_, i, CMT_COUNT,  (CounterMeasurementMode) mode,
					                   CED_RISING_EDGE, CTS_TICK_20PT83ns, CDM_NONE, CDT_DEBOUNCE_0ns, CF_DEFAULT);
    #endif
    if (status) {
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
        "%s::%s error calling cbCConfigScan, counter=%d, mode=0x%x, status=%d, error=%s\n",
        driverName, functionName, i, mode, status, getErrorMessage(status));
    }
  }

  if ((channelAdvance == mcaChannelAdvance_External) && (prescale > 1) ) {
    #ifdef _WIN32
      status = cbCLoad32(boardNum_, OUTPUTVAL0REG0+prescaleCounter, 0);
      status = cbCLoad32(boardNum_, OUTPUTVAL1REG0+prescaleCounter, prescale-1);
      status = cbCLoad32(boardNum_, MAXLIMITREG0+prescaleCounter, prescale-1);
      mode = OUTPUT_ON | RANGE_LIMIT_ON;
      status = cbCConfigScan(boardNum_, prescaleCounter, mode, CTR_DEBOUNCE_NONE, CTR_TRIGGER_BEFORE_STABLE,
                             CTR_RISING_EDGE, CTR_TICK20PT83ns, 0);
    #else
      status = ulCLoad(daqDeviceHandle_, prescaleCounter, CRT_OUTPUT_VAL0, 0);
      status = ulCLoad(daqDeviceHandle_, prescaleCounter, CRT_OUTPUT_VAL1, prescale-1);
      status = ulCLoad(daqDeviceHandle_, prescaleCounter, CRT_MAX_LIMIT, prescale-1);
      mode = CMM_OUTPUT_ON | CMM_RANGE_LIMIT_ON;
      status = ulCConfigScan(daqDeviceHandle_, prescaleCounter, CMT_COUNT,  (CounterMeasurementMode) mode,
					                   CED_RISING_EDGE, CTS_TICK_20PT83ns, CDM_NONE, CDT_DEBOUNCE_0ns, CF_DEFAULT);
    #endif
    if (status) {
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
        "%s::%s error calling cbCConfigScan, counter=%d, mode=0x%x, status=%d, error=%s\n",
        driverName, functionName, prescaleCounter, mode, status, getErrorMessage(status));
    }
  }
  getIntegerParam(mcaNumChannels_, &numPoints);
  getDoubleParam(mcaDwellTime_, &dwell);
  getIntegerParam(MCSPoint0Action_, &point0Action);
  if (point0Action == MCSPoint0Skip) numPoints++;
  #ifdef _WIN32
    long count;
    int chanCount;
    long pretrigCount = 0;
    double rateFactor=1.0;
    if (dwell > 1e-6) rateFactor = 1000.;
    long rate = (LONG)((rateFactor / dwell) + 0.5);
    options = 0;
    if (dwell > 1e-4) {
      counterBits_ = 32;
    } else {
      counterBits_ = 16;
    }
    options |= BACKGROUND;
    if (rateFactor > 1.0)
      options |= HIGHRESRATE;
    if (channelAdvance == mcaChannelAdvance_External)
      options |= EXTCLOCK;
    // At long dwell times read use a single buffer for reading the data
    if (dwell >= SINGLEIO_THRESHOLD_TIME)
      options |= SINGLEIO;
    // Always use EXTTRIGGER
    options |= EXTTRIGGER;
    if (point0Action == MCSPoint0NoClear)
      options |= NOCLEAR;
  
    for (i=0, chanCount=0; i<numCounters_; i++) {
      if (!mcsCounterEnable_[i]) continue;
      chanArray_[chanCount] = i;
      chanTypeArray_[chanCount] = CTRBANK0;
      chanCount++;
      if (counterBits_ == 16) continue;
      chanArray_[chanCount] = i;
      chanTypeArray_[chanCount] = CTRBANK1;
      chanCount++;
    }
    if (mcsCounterEnable_[DIGITAL_IO_COUNTER]) {
      numMCSCounters_++;
      chanArray_[chanCount] = AUXPORT;
      chanTypeArray_[chanCount] = DIGITAL8;
      chanCount++;
      if (counterBits_ == 32) { // Add padding for binary data
        chanArray_[chanCount] = 0;
        chanTypeArray_[chanCount] = PADZERO;
        chanCount++;
      }
    }
    count = chanCount * numPoints;
    status = cbDaqInScan(boardNum_, chanArray_, chanTypeArray_, gainArray_, chanCount, &rate,
                         &pretrigCount, &count, pCountsI16_, options);
    asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
      "%s::%s called cbDaqInScan, chanCount=%d, count=%ld, rate=%ld,"
      " inputMemHandle_=%p, options=0x%x, status=%d\n",
      driverName, functionName, chanCount, count, rate,
      pCountsI16_, options, status);
    if (status) {
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
        "%s::%s error calling cbDaqInScan, chanCount=%d, count=%ld, rate=%ld,"
        " inputMemHandle_=%p, options=0x%x, status=%d, error=%s\n",
        driverName, functionName, chanCount, count, rate,
        pCountsI16_, options, status, getErrorMessage(status));
    }
    // The actual dwell time can be different from requested due to clock granularity
    setDoubleParam(mcaDwellTime_, rateFactor/rate);
  #else
    double rate = 1. / dwell;  
    options = SO_DEFAULTIO;
    if (channelAdvance == mcaChannelAdvance_External)
      options |= SO_EXTCLOCK;
    // At long dwell times read use a single buffer for reading the data
    if (dwell >= SINGLEIO_THRESHOLD_TIME)
      options |= SO_SINGLEIO;
    // Always use EXTTRIGGER
    options |= SO_EXTTRIGGER;
    int flags = DAQINSCAN_FF_DEFAULT;
    if (point0Action == MCSPoint0NoClear)
      flags |= DAQINSCAN_FF_NOCLEAR;
    DaqInChanDescriptor *pDICD = new DaqInChanDescriptor[MAX_MCS_COUNTERS];
    int outChan=0;
    for (i=0; i<numCounters_; i++) {
      if (!mcsCounterEnable_[i]) continue;
      pDICD[outChan].channel = i;
      pDICD[outChan].type = DAQI_CTR32;
      pDICD[outChan].range = BIP10VOLTS;
      outChan++;
    }
    if (mcsCounterEnable_[DIGITAL_IO_COUNTER]) {
      numMCSCounters_++;
      pDICD[outChan].channel = AUXPORT;
      pDICD[outChan].type = DAQI_DIGITAL;
      pDICD[outChan].range = BIP10VOLTS;
      outChan++;
    }
    int numChans = outChan;
    status = ulDaqInScan(daqDeviceHandle_, pDICD, numChans, numPoints, &rate, (ScanOption) options, (DaqInScanFlag) flags, pCountsF64_);
    delete[] pDICD;
    if (status) {
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
        "%s::%s error calling ulDaqInScan, numChans=%d, numPoints=%d, rate=%f, options=0x%x, flags=0x%x, status=%d, error=%s\n",
        driverName, functionName, numChans, numPoints, rate,
        options, status, flags, getErrorMessage(status));
    }
    // The actual dwell time can be different from requested due to clock granularity
    setDoubleParam(mcaDwellTime_, 1./rate);
  #endif
  setIntegerParam(MCSCurrentPoint_, 0);
  MCSRunning_ = true;

  return 0;
}

int USBCTR::readMCS()
{
  int lastPoint=0;
  int currentPoint;
  int status;
  int i, j;
  short ctrStatus;
  long ctrCount, ctrIndex;
  epicsTimeStamp now;
  int numTimePoints;
  int point0Action;
  double presetReal, elapsedTime;
  static const char *functionName = "readMCS";

  // We need to treat Windows and Linux differently here because with UL for Linux the buffer is always float64, while on
  // Windows it is either int32 or int16.
  getIntegerParam(MCSCurrentPoint_, &currentPoint);
  getIntegerParam(mcaNumChannels_,  &numTimePoints);
  getIntegerParam(MCSPoint0Action_, &point0Action);
  #ifdef _WIN32
    status = cbGetIOStatus(boardNum_, &ctrStatus, &ctrCount, &ctrIndex, DAQIFUNCTION);
  #else
    ScanStatus scanStatus;
    TransferStatus xferStatus;
    status = ulDaqInScanStatus(daqDeviceHandle_, &scanStatus, &xferStatus);
    ctrStatus = scanStatus;
    ctrCount = xferStatus.currentTotalCount;
    ctrIndex = xferStatus.currentIndex;
  #endif
  asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
    "%s::%s getStatus returned status=%d, ctrStatus=%d, ctrCount=%ld, ctrIndex=%ld\n",
    driverName, functionName, status, ctrStatus, ctrCount, ctrIndex);
  if (ctrStatus == 0) {
    MCSRunning_ = false;
  }
  if (ctrIndex >= 0) {
#ifdef _WIN32
    if (counterBits_ == 32) ctrIndex /= 2;
#endif
    lastPoint = ctrIndex / numMCSCounters_ + 1;

    int inPtr = currentPoint;
    if (point0Action == MCSPoint0Skip) {
      inPtr++;
    }
    epicsTimeStamp now;
    epicsTimeGetCurrent(&now);
    for(; inPtr < lastPoint; inPtr++) {
      for (i=0, j=0; i<MAX_MCS_COUNTERS; i++) {
        if (!mcsCounterEnable_[i]) continue;
#ifdef _WIN32
        if (counterBits_ == 32) {
          MCSBuffer_[i][currentPoint] = pCountsI32_[inPtr*numMCSCounters_ + j];
          // There seems to be a bug in PADZERO and it is actually giving counter0 value not 0
          if (i == DIGITAL_IO_COUNTER) MCSBuffer_[i][currentPoint] &= 0xff;
        } else {
          MCSBuffer_[i][currentPoint] = pCountsI16_[inPtr*numMCSCounters_ + j];
        }
#else
          MCSBuffer_[i][currentPoint] = (int) pCountsF64_[inPtr*numMCSCounters_ + j];
#endif
        j++;
      }
      MCSAbsTimeBuffer_[currentPoint] = now.secPastEpoch + now.nsec/1.e9;
      currentPoint++;
    }
  }
  setIntegerParam(MCSCurrentPoint_, currentPoint);

  getDoubleParam(mcaPresetRealTime_,  &presetReal);
  getDoubleParam(mcaElapsedRealTime_, &elapsedTime);
  epicsTimeGetCurrent(&now);
  elapsedTime = epicsTimeDiffInSeconds(&now, &startTime_);
  if (MCSRunning_ && (presetReal > 0) && (elapsedTime >= presetReal)) {
    MCSRunning_ = false;
  }

  // Set elapsed times
  for (i=0; i<MAX_MCS_COUNTERS; i++) {
    setDoubleParam(i, mcaElapsedRealTime_, elapsedTime);
    setDoubleParam(i, mcaElapsedLiveTime_, elapsedTime);
  }

  if (!MCSRunning_) {
    stopMCS();
    for (i=0; i<numCounters_; i++) {
      setIntegerParam(i, mcaAcquiring_, 0);
    }
  }

  // Do callbacks on all channels
  for (i=0; i<numCounters_; i++) {
    callParamCallbacks(i);
  }

  return 0;
}

int USBCTR::eraseMCS()
{
  int i;
  int numTimePoints;
  //static const char *functionName="eraseMCS";

  MCSErased_ = true;

  getIntegerParam(mcaNumChannels_, &numTimePoints);

  /* Reset pointers to start of buffer */
  setIntegerParam(MCSCurrentPoint_, 0);

  /* Reset the elapsed time and counts */
  elapsedPrevious_ = 0.;
  for (i=0; i<MAX_MCS_COUNTERS; i++) {
    memset(MCSBuffer_[i], 0, numTimePoints * sizeof(epicsUInt32));
    setDoubleParam(i, mcaElapsedLiveTime_, 0.0);
    setDoubleParam(i, mcaElapsedRealTime_, 0.0);
    setDoubleParam(i, mcaElapsedCounts_, 0.0);
    callParamCallbacks(i);
  }

  /* Reset the start time.  This is necessary here because we may be
   * acquiring, and AcqOn will not be called. Normally this is set in AcqOn.
   */
  epicsTimeGetCurrent(&startTime_);

  return 0;
}

int USBCTR::stopMCS()
{
  int status;
  static const char *functionName = "stopMCS";

  if (MCSRunning_) {
    // Forced stop
    MCSRunning_ = false;
    readMCS();
    // readMCS will call this function when it finds MCSRunning=false so we can return now
    return 0;
  }
  #ifdef _WIN32
    status = cbStopBackground(boardNum_, DAQIFUNCTION);
  #else
    status = ulDaqInScanStop(daqDeviceHandle_);
  #endif
  if (status) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
      "%s::%s ERROR calling cbStopBackground(CTRFUNCTION), status=%d, error=%s\n",
      driverName, functionName, status, getErrorMessage(status));
  }
  return status;
}

int USBCTR::computeMCSTimes()
{
  int numPoints, i;
  double dwell;

  getIntegerParam(mcaNumChannels_, &numPoints);
  getDoubleParam(mcaDwellTime_, &dwell);
  for (i=0; i<numPoints; i++) {
    MCSTimeBuffer_[i] = (epicsFloat32) (i * dwell);
  }
  doCallbacksFloat32Array(MCSTimeBuffer_, numPoints, MCSTimeWF_, 0);
  return 0;
}

int USBCTR::startScaler()
{
  int status;
  int i;
  int mode;
  int samplesPerCounter = 20;
  long rate = 100;
  int firstCounter = 0;
  int lastCounter = numCounters_ - 1;
  int options;
  static const char *functionName = "startScaler";

  #ifdef _WIN32
    for (i=0; i<numCounters_; i++) {
      mode = OUTPUT_ON | COUNT_DOWN_OFF | GATING_ON;
      if (i == 0) mode = mode | RANGE_LIMIT_ON | NO_RECYCLE_ON | INVERT_GATE;
      status = cbCConfigScan(boardNum_, i, mode, CTR_DEBOUNCE_NONE, CTR_TRIGGER_BEFORE_STABLE,
                             CTR_RISING_EDGE, CTR_TICK20PT83ns, 0);
      if (status) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
          "%s::%s error calling cbCConfigScan, counter=%d, mode=0x%x, status=%d, error=%s\n",
          driverName, functionName, i, mode, status, getErrorMessage(status));
      }
    }
    int count = samplesPerCounter * numCounters_;
    options = BACKGROUND | CONTINUOUS | CTR64BIT | SINGLEIO;
    status = cbCInScan(boardNum_, firstCounter, lastCounter, count, &rate,
                       pCountsUI64_, options);
    if (status) {
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
        "%s::%s error calling cbCInScan, firstCounter=%d, lastCounter=%d, count=%d, rate=%d, options=0x%x, status=%d, error=%s\n",
        driverName, functionName, firstCounter, lastCounter, count, rate, options, status, getErrorMessage(status));
    }
  #else
    for (i=0; i<numCounters_; i++) {
      mode = CMM_OUTPUT_ON | CMM_GATING_ON;
      if (i == 0) mode = mode | CMM_RANGE_LIMIT_ON | CMM_NO_RECYCLE | CMM_INVERT_GATE;
      status = ulCConfigScan(daqDeviceHandle_, i, CMT_COUNT,  (CounterMeasurementMode) mode,
					                   CED_RISING_EDGE, CTS_TICK_20PT83ns, CDM_NONE, CDT_DEBOUNCE_0ns, CF_DEFAULT);
      if (status) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
          "%s::%s error calling ulCConfigScan, counter=%d, mode=0x%x, status=%d, error=%s\n",
          driverName, functionName, i, mode, status, getErrorMessage(status));
      }
    }
    options = SO_CONTINUOUS | SO_SINGLEIO;
    double dblRate = (double) rate;
    CInScanFlag flags = CINSCAN_FF_CTR64_BIT;
    status = ulCInScan(daqDeviceHandle_, firstCounter, lastCounter, samplesPerCounter, &dblRate, (ScanOption) options, flags, pCountsUI64_);
    if (status) {
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
        "%s::%s error calling ulCInScan, firstCounter=%d, lastCounter=%d, samplesPerCounter=%d, rate=%d, options=0x%x, flags=0x%x, status=%d, error=%s\n",
        driverName, functionName, firstCounter, lastCounter, samplesPerCounter, (int) rate, options, flags, status, getErrorMessage(status));
    }
  #endif
  scalerRunning_ = true;
  return 0;
}

int USBCTR::readScaler()
{
  int numValues;
  int i, j;
  int status;
  short ctrStatus;
  long ctrCount, ctrIndex;
  int lastIndex;
  bool scalerDone = false;
  static const char *functionName = "readScaler";

  // Poll the status of the counter scan
  #ifdef _WIN32
    status = cbGetIOStatus(boardNum_, &ctrStatus, &ctrCount, &ctrIndex, CTRFUNCTION);
  #else
    ScanStatus scanStatus;
    TransferStatus xferStatus;
    status = ulDaqInScanStatus(daqDeviceHandle_, &scanStatus, &xferStatus);
    ctrStatus = scanStatus;
    ctrCount = xferStatus.currentTotalCount;
    ctrIndex = xferStatus.currentIndex;
  #endif
  asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
    "%s::%s getStatus returned status=%d, ctrStatus=%d, ctrCount=%ld, ctrIndex=%ld\n",
    driverName, functionName, status, ctrStatus, ctrCount, ctrIndex);

  numValues = ctrIndex + 1;
  // Get the index of the start of the last complete set of counts in the buffer
  if (numValues < numCounters_) return 0;
  lastIndex = (numValues/numCounters_ - 1) * numCounters_;
  for (i=0; i<=lastIndex; i+= numCounters_) {
    for (j=0; j<numCounters_; j++) {
      scalerCounts_[j] = (epicsInt32) pCountsUI64_[i+j];
      if ((scalerPresetCounts_[j] > 0) && (scalerCounts_[j] >= scalerPresetCounts_[j])) {
        scalerDone = true;
      }
    }
    if (scalerDone) {
      stopScaler();
      break;
    }
  }
  asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
    "%s::%s lastIndex=%d, scalerCounts_[0]=%d, scalerPresetCounts_[0]=%d\n",
    driverName, functionName, lastIndex, scalerCounts_[0], scalerPresetCounts_[0]);
  return 0;
}

int USBCTR::stopScaler()
{
  int status;
  static const char *functionName = "stopScaler";

  #ifdef _WIN32
    status = cbStopBackground(boardNum_, CTRFUNCTION);
  #else
    status = ulCInScanStop(daqDeviceHandle_);
  #endif
  if (status) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
      "%s::%s ERROR calling cbStopBackground(CTRFUNCTION), status=%d, error=%s\n",
      driverName, functionName, status, getErrorMessage(status));
  }
  scalerRunning_ = false;
  setIntegerParam(scalerDone_, 1);
  return status;
}

int USBCTR::resetScaler()
{
  int i;
  int status=0;

  /* Reset scaler */
  if (scalerRunning_) {
    status = stopScaler();
  }
  for (i=0; i<numCounters_; i++) {
    scalerCounts_[i] = 0;
  }
  return status;
}


int USBCTR::clearScalerPresets()
{
  int i;

  for (i=0; i<numCounters_; i++) {
    scalerPresetCounts_[i] = 0;
  }
  return 0;
}

int USBCTR::setScalerPresets()
{
  int i;
  int status;
  static const char *functionName = "setScalerPresets";

  for (i=0; i<numCounters_; i++) {
    getIntegerParam(i, scalerPresets_, &scalerPresetCounts_[i]);
    if (scalerPresetCounts_[i] > 0) {
      #ifdef _WIN32
        status = cbCLoad32(boardNum_, MAXLIMITREG0+i, scalerPresetCounts_[i]);
      #else
        status = ulCLoad(daqDeviceHandle_, i, CRT_MAX_LIMIT, scalerPresetCounts_[i]);
      #endif
      if (status) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
          "%s::%s error calling cbCLoad32, counter=%d, presetCounts=%d, status=%d, error=%s\n",
          driverName, functionName, i, scalerPresetCounts_[i], status, getErrorMessage(status));
      }
    }
  }
  // For counter0 output register 0 and 1 control when the counter output goes low and high
  #ifdef _WIN32
    status = cbCLoad32(boardNum_, OUTPUTVAL0REG0, 0);
  #else 
    status = ulCLoad(daqDeviceHandle_, 0, CRT_OUTPUT_VAL0, 0);
  #endif
  if (status) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
      "%s::%s error calling cbCLoad32, reg=OUTPUTVAL0REG0, value=%d, status=%d, error=%s\n",
      driverName, functionName, 0, status, getErrorMessage(status));
  }
  #ifdef _WIN32
    status = cbCLoad32(boardNum_, OUTPUTVAL1REG0, scalerPresetCounts_[0]);
  #else 
    status = ulCLoad(daqDeviceHandle_, 0, CRT_OUTPUT_VAL1, scalerPresetCounts_[0]);
  #endif
  if (status) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
      "%s::%s error calling cbCLoad32, reg=OUTPUTVAL1REG0, value=%d, status=%d, error=%s\n",
      driverName, functionName, scalerPresetCounts_[0], status, getErrorMessage(status));
  }

  return 0;
}

asynStatus USBCTR::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
  int addr;
  int function = pasynUser->reason;
  int status=0;
  int numTimePoints;
  int currentPoint;
  int i;
  static const char *functionName = "writeInt32";

  this->getAddress(pasynUser, &addr);
  setIntegerParam(addr, function, value);

  // Pulse generator functions
  if (function == pulseGenRun_) {
    // Allow starting a run even if it thinks its running,
    // since there is no way to know when it got done if Count!=0
    if (value) {
      status = startPulseGenerator(addr);
    }
    else if (!value && pulseGenRunning_[addr]) {
      status = stopPulseGenerator(addr);
    }
  }
  if ((function == pulseGenCount_) ||
      (function == pulseGenIdleState_)) {
    if (pulseGenRunning_[addr]) {
      status = stopPulseGenerator(addr);
      status |= startPulseGenerator(addr);
    }
  }

  // Counter functions
  if (function == counterReset_) {
    #ifdef _WIN32
      // LOADREG0=0, LOADREG1=1, so we use addr
      status = cbCLoad32(boardNum_, addr, 0);
    #else
      status = ulCLoad(daqDeviceHandle_, addr, CRT_LOAD, 0);
    #endif
  }

  // Trigger functions
  if (function == triggerMode_) {
    #ifdef _WIN32
      status = cbDaqSetTrigger(boardNum_, TRIG_EXTTTL, value, 0, CTRBANK0, 0, 0, 0, START_EVENT);
    #else
      TriggerType triggerType = TRIG_LOW;
      // We map the UL Windows trigger types to Ul Linux
      // We can't use macros from cbw.h because that file conflicts with uldaq.h
      switch (value) {
        case 0: triggerType = TRIG_RISING; break;
        case 1: triggerType = TRIG_FALLING; break;
        case 6: triggerType = TRIG_HIGH; break;
        case 7: triggerType = TRIG_LOW; break;
        default:
          asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
            "%s::%s error unsupported trigger value=%d\n",
            driverName, functionName, value);
          break;
      }
      DaqInChanDescriptor trigChanDescriptor;
      status = ulDaqInSetTrigger(daqDeviceHandle_, triggerType, trigChanDescriptor, 0, 0, 0);
    #endif
    if (status) {
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
        "%s::%s error calling cbDaqSetTrigger status=%d, error=%s\n",
        driverName, functionName, status, getErrorMessage(status));
    }
  }

  // Scaler functions
  else if (function == scalerReset_) {
    /* Reset scaler */
    if (MCSRunning_) goto done;
    resetScaler();
    scalerRunning_ = false;
    /* Clear all of the presets and counts*/
    for (i=0; i<numCounters_; i++) {
      scalerCounts_[i] = 0;
      setIntegerParam(i, scalerPresets_, 0);
    }
  }

  else if (function == scalerArm_) {
    if (MCSRunning_) goto done;
    /* Arm or disarm scaler */
    if (value != 0) {
      setScalerPresets();
      startScaler();
    } else {
      stopScaler();
    }
    setIntegerParam(scalerDone_, 0);
  }

  // MCA commands
  getIntegerParam(mcaNumChannels_, &numTimePoints);
  if (function == mcaStartAcquire_) {
    if (scalerRunning_) {
      status = -1;
      goto done;
    }
    if (MCSRunning_) goto done;
    // If we have already completed acquisition due to nextChan_, don't start, signal error
    getIntegerParam(MCSCurrentPoint_, &currentPoint);
    if (currentPoint >= numTimePoints) {
        // Must toggle mcaAcquiring to 1 and back to 0 to signal SNL program to clear Acquiring
      setIntegerParam(mcaAcquiring_, 1);
      callParamCallbacks();
      setIntegerParam(mcaAcquiring_, 0);
      goto done;
    }
    setIntegerParam(mcaAcquiring_, 1);
    MCSErased_ = false;
    // Set the acquisition start time
    epicsTimeGetCurrent(&startTime_);
    // Start the hardware
    startMCS();
  }

  else if (function == mcaStopAcquire_) {
    if (scalerRunning_) goto done;
    /* Stop data acquisition */
    if (!MCSRunning_) {
      // We are not acquiring.
      status = asynSuccess;
      goto done;
    }
    // Stop the hardware
    stopMCS();
  }

  else if (function == mcaErase_) {
    asynPrint(pasynUser, ASYN_TRACE_FLOW,
              "%s:%s: [%s addr=%d]: erased\n",
              driverName, functionName, portName, addr);

    /* Erase the buffer in the private data structure */
    eraseMCS();

  }

  else if (function == mcaNumChannels_) {
    /* Terminology warning:
     * This is the number of channels that are to be acquired. Channels
     * correspond to time bins or external channel advance triggers, as
     * opposed to the 8 input counters that the USB-CTR08 supports.
     */
    if (value > maxTimePoints_) {
      setIntegerParam(mcaNumChannels_, maxTimePoints_);
      asynPrint(pasynUser, ASYN_TRACE_ERROR,
                "%s:%s:  # channels=%d too large, max=%d\n",
                driverName, functionName, value, maxTimePoints_);
    }
  }

  done:
  callParamCallbacks(addr);
  if (status == 0) {
    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
             "%s:%s, port %s, function=%d, wrote %d to address %d\n",
             driverName, functionName, this->portName, function, value, addr);
  } else {
    asynPrint(pasynUser, ASYN_TRACE_ERROR,
             "%s:%s, port %s, function=%d, ERROR writing %d to address %d, status=%d\n",
             driverName, functionName, this->portName, function, value, addr, status);
  }
  return (status==0) ? asynSuccess : asynError;
}


asynStatus USBCTR::readInt32(asynUser *pasynUser, epicsInt32 *value)
{
  int addr;
  int function = pasynUser->reason;
  int status=0;
  //static const char *functionName = "readInt32";

  this->getAddress(pasynUser, &addr);

  if (function == scalerRead_) {
    /* Read a single scaler channel */
    *value = scalerCounts_[addr];
  }

  // Other functions we call the base class method
  else {
     status = asynPortDriver::readInt32(pasynUser, value);
  }

  callParamCallbacks(addr);
  return (status==0) ? asynSuccess : asynError;
}

asynStatus USBCTR::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
  int addr;
  int function = pasynUser->reason;
  int status=0;
  static const char *functionName = "writeFloat64";

  this->getAddress(pasynUser, &addr);
  setDoubleParam(addr, function, value);

  // Pulse generator functions
  if ((function == pulseGenPeriod_)    ||
      (function == pulseGenDutyCycle_) ||
      (function == pulseGenDelay_)) {
    if (pulseGenRunning_[addr]) {
      status = stopPulseGenerator(addr);
      status |= startPulseGenerator(addr);
    }
  }

  // MCS functions
  else if (function == mcaDwellTime_) {
    computeMCSTimes();
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

asynStatus USBCTR::writeUInt32Digital(asynUser *pasynUser, epicsUInt32 value, epicsUInt32 mask)
{
  int function = pasynUser->reason;
  int status=0;
  int i;
  epicsUInt32 outValue=0, outMask, direction;
  static const char *functionName = "writeUInt32Digital";


  setUIntDigitalParam(function, value, mask);
  if (function == digitalDirection_) {
    for (i=0; i<NUM_IO_BITS; i++) {
      if ((mask & (1<<i)) != 0) {
        #ifdef _WIN32
          int dir = (value == 0) ? DIGITALIN : DIGITALOUT;
          status = cbDConfigBit(boardNum_, AUXPORT, i, dir);
        #else
          DigitalDirection dir = (value == 0) ? DD_INPUT : DD_OUTPUT;
          status = ulDConfigBit(daqDeviceHandle_, AUXPORT, i, dir);
        #endif
      }
    }
  }

  else if (function == digitalOutput_) {
    getUIntDigitalParam(digitalDirection_, &direction, 0xFFFFFFFF);
    for (i=0, outMask=1; i<NUM_IO_BITS; i++, outMask = (outMask<<1)) {
      // Only write the value if the mask has this bit set and the direction for that bit is output (1)
      outValue = ((value &outMask) == 0) ? 0 : 1;
      if ((mask & outMask & direction) != 0) {
        #ifdef _WIN32
          status = cbDBitOut(boardNum_, AUXPORT, i, outValue);
        #else
          status = ulDBitOut(daqDeviceHandle_, AUXPORT, i, outValue);
        #endif
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


asynStatus USBCTR::readInt32Array(asynUser *pasynUser, epicsInt32 *data,
                                  size_t numRead, size_t *numActual)
{
  int signal;
  int command;
  const char *paramName;
  asynStatus status = asynSuccess;
  int currentPoint;
  size_t i;
  static const char* functionName="readInt32Array";

  parseAsynUser(pasynUser, &command, &signal, &paramName);
  pasynManager->getAddr(pasynUser, &signal);
  asynPrint(pasynUser, ASYN_TRACE_FLOW,
            "%s:%s: entry, command=%d, signal=%d, numRead=%d, &data=%p\n",
            driverName, functionName, command, signal, (int)numRead, data);

  if (command == mcaData_) {
    /* Transfer the data from the private driver structure to the supplied data
     * buffer. The private data structure will have the information for all the
     * signals, so we need to just extract the signal being requested.
     */
    int nChans;
    int numCopy;
    getIntegerParam(mcaNumChannels_, &nChans);
    getIntegerParam(MCSCurrentPoint_, &currentPoint);
    numCopy = (int)numRead;
    if (numCopy > nChans) numCopy = nChans;
    // We copy all the channels but we only report nchans
    // This ensures the entire array is correct even if it was not set to zero at the start
    memcpy(data, MCSBuffer_[signal], numCopy*sizeof(epicsInt32));
    *numActual = numRead;
    if ((int)*numActual > currentPoint) *numActual = currentPoint;
    // Make it set NORD non-zero?
    if (*numActual == 0) *numActual = 1;
    asynPrint(pasynUser, ASYN_TRACE_FLOW,
              "%s:%s: [signal=%d]: read %d chans (numRead=%d, numCopy=%d, currentPoint=%d, nChans=%d)\n",
              driverName, functionName, signal, (int)*numActual, (int)numRead, numCopy, currentPoint, nChans);
    }
  else if (command == scalerRead_) {
    for (i=0; (i<numRead && i<(size_t)numCounters_); i++) {
      data[i] = scalerCounts_[i];
    }
    for (i=numCounters_; i<numRead; i++) {
      data[i] = 0;
    }
    *numActual = numRead;
    asynPrint(pasynUser, ASYN_TRACE_FLOW,
              "%s:%s: scalerReadCommand: read %d chans, data=%d %d %d %d %d %d %d %d\n",
              driverName, functionName, (int)numRead, data[0], data[1], data[2], data[3],
                                                      data[4], data[5], data[6], data[7]);
  }
  else {
    asynPrint(pasynUser, ASYN_TRACE_ERROR,
              "%s:%s: got illegal command %d\n",
              driverName, functionName, command);
    status = asynError;
  }
  return status;
}


asynStatus USBCTR::readFloat32Array(asynUser *pasynUser, epicsFloat32 *value, size_t nElements, size_t *nIn)
{
  int function = pasynUser->reason;
  int addr;
  int numPoints;
  epicsFloat32 *inPtr;
  static const char *functionName = "readFloat32Array";

  this->getAddress(pasynUser, &addr);

  if (function == MCSTimeWF_) {
    inPtr = MCSTimeBuffer_;
    getIntegerParam(mcaNumChannels_, &numPoints);
  }
  else {
    asynPrint(pasynUser, ASYN_TRACE_ERROR,
      "%s:%s: ERROR: unknown function=%d\n",
      driverName, functionName, function);
    return asynError;
  }
  *nIn = nElements;
  if (*nIn > (size_t) numPoints) *nIn = (size_t) numPoints;
  memcpy(value, inPtr, *nIn*sizeof(epicsFloat32));

  return asynSuccess;
}

asynStatus USBCTR::readFloat64Array(asynUser *pasynUser, epicsFloat64 *value, size_t nElements, size_t *nIn)
{
  int function = pasynUser->reason;
  int addr;
  int numPoints;
  epicsFloat64 *inPtr;
  static const char *functionName = "readFloat6Array";

  this->getAddress(pasynUser, &addr);

  if (function == MCSAbsTimeWF_) {
    inPtr = MCSAbsTimeBuffer_;
    getIntegerParam(mcaNumChannels_, &numPoints);
  }
  else {
    asynPrint(pasynUser, ASYN_TRACE_ERROR,
      "%s:%s: ERROR: unknown function=%d\n",
      driverName, functionName, function);
    return asynError;
  }
  *nIn = nElements;
  if (*nIn > (size_t) numPoints) *nIn = (size_t) numPoints;
  memcpy(value, inPtr, *nIn*sizeof(epicsFloat64));

  return asynSuccess;
}

void USBCTR::pollerThread()
{
  /* This function runs in a separate thread.  It waits for the poll
   * time */
  static const char *functionName = "pollerThread";
  epicsUInt32 newValue, changedBits, prevInput=0;
  epicsTime startTime=epicsTime::getCurrent(), endTime, currentTime;
  unsigned short biVal;;
  int i;
  int status;

  while(1) {
    lock();
    endTime = epicsTime::getCurrent();
    setDoubleParam(pollTimeMS_, (endTime-startTime)*1000.);
    startTime = epicsTime::getCurrent();

    // Read the digital inputs
    #ifdef _WIN32
      status = cbDIn(boardNum_, AUXPORT, &biVal);
    #else
      unsigned long long data;
      status = ulDIn(daqDeviceHandle_, AUXPORT, &data);
      biVal = (unsigned short) data;
    #endif
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

    if (scalerRunning_) {
      readScaler();
    }

    if (MCSRunning_) {
      readMCS();
    }

    for (i=0; i<MAX_SIGNALS; i++) {
      callParamCallbacks(i);
    }
    double pollTime;
    getDoubleParam(pollSleepMS_, &pollTime);
    unlock();
    epicsThreadSleep(pollTime/1000.);
  }
}


/* Report  parameters */
void USBCTR::report(FILE *fp, int details)
{
  int i;
  int currentPoint;

  fprintf(fp, "  Port: %s, pollTime=%f\n",
          this->portName, pollTime_);
  if (details >= 1) {
    fprintf(fp, "  Pulse generators:\n");
    for (i=0; i<NUM_TIMERS; i++) {
      fprintf(fp, "    %d: Running:%d\n", i, pulseGenRunning_[i]);
    }
    fprintf(fp, "  numCounters: %d\n", numCounters_);
    fprintf(fp, "  Scaler:\n");
    fprintf(fp, "    Running: %d\n", scalerRunning_);
    for (i=0; i<numCounters_; i++) {
      fprintf(fp, "    %d: preset=%d, count=%d\n", i, scalerPresetCounts_[i], scalerCounts_[i]);
    }
    fprintf(fp, "  MCS:\n");
    fprintf(fp, "    Running: %d\n", MCSRunning_);
    fprintf(fp, "    maxTimePoints: %d\n", maxTimePoints_);
    fprintf(fp, "    MCSErased: %d\n", MCSErased_);
    getIntegerParam(MCSCurrentPoint_, &currentPoint);
    fprintf(fp, "    currentPoint: %d\n", currentPoint);
  }
  asynPortDriver::report(fp, details);
}

/** Configuration command, called directly or from iocsh */
extern "C" int USBCTRConfig(const char *portName, const char *uniqueID,
                            int maxTimePoints, double pollTime)
{
  new USBCTR(portName, uniqueID, maxTimePoints, pollTime);
  return(asynSuccess);
}


static const iocshArg configArg0 = { "Port name",             iocshArgString};
static const iocshArg configArg1 = { "uniqueID",              iocshArgString};
static const iocshArg configArg2 = { "Max. # of time points", iocshArgInt};
static const iocshArg configArg3 = { "Poll time",             iocshArgDouble};
static const iocshArg * const configArgs[] = {&configArg0,
                                              &configArg1,
                                              &configArg2,
                                              &configArg3};
static const iocshFuncDef configFuncDef = {"USBCTRConfig",4,configArgs};
static void configCallFunc(const iocshArgBuf *args)
{
  USBCTRConfig(args[0].sval, args[1].sval, args[2].ival, args[3].dval);
}

void drvUSBCTRRegister(void)
{
  iocshRegister(&configFuncDef,configCallFunc);
}

extern "C" {
epicsExportRegistrar(drvUSBCTRRegister);
}
