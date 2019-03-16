/* drvMultiFunction.cpp
 *
 * Driver for Measurement Computing multi-function DAQ board using asynPortDriver base class
 *
 * This driver supports simple analog in/out, digital in/out bit and word, timer (digital pulse generator), counter,
 *   waveform out (aribtrary waveform generator), and waveform in (digital oscilloscope)
 *
 * This driver was previously name drv1608G.cpp but was renamed because it now supports several models.
 *
 * Mark Rivers
 * November 1, 2015
*/

#include <math.h>

#include <iocsh.h>
#include <epicsThread.h>
#include <epicsString.h>

#include <asynPortDriver.h>

#ifdef linux
  #include "cbw_linux.h"
#else
  #include "cbw.h"
#endif

#include <epicsExport.h>

static const char *driverName = "MultiFunction";

typedef enum {
  waveTypeUser,
  waveTypeSin,
  waveTypeSquare,
  waveTypeSawTooth,
  waveTypePulse,
  waveTypeRandom
} waveType_t;

// Board parameters
#define modelNameString           "MODEL_NAME"
#define modelNumberString         "MODEL_NUMBER"

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

// Analog input parameters
#define analogInValueString       "ANALOG_IN_VALUE"
#define analogInRangeString       "ANALOG_IN_RANGE"
#define analogInTypeString        "ANALOG_IN_TYPE"

// Temperature parameters
#define temperatureInValueString  "TEMPERATURE_IN_VALUE"
#define thermocoupleTypeString    "THERMOCOUPLE_TYPE"
#define temperatureScaleString    "TEMPERATURE_SCALE"
#define temperatureFilterString   "TEMPERATURE_FILTER"

// Waveform digitizer parameters - global
#define waveDigDwellString        "WAVEDIG_DWELL"
#define waveDigTotalTimeString    "WAVEDIG_TOTAL_TIME"
#define waveDigFirstChanString    "WAVEDIG_FIRST_CHAN"
#define waveDigNumChansString     "WAVEDIG_NUM_CHANS"
#define waveDigNumPointsString    "WAVEDIG_NUM_POINTS"
#define waveDigCurrentPointString "WAVEDIG_CURRENT_POINT"
#define waveDigExtTriggerString   "WAVEDIG_EXT_TRIGGER"
#define waveDigExtClockString     "WAVEDIG_EXT_CLOCK"
#define waveDigContinuousString   "WAVEDIG_CONTINUOUS"
#define waveDigAutoRestartString  "WAVEDIG_AUTO_RESTART"
#define waveDigRetriggerString    "WAVEDIG_RETRIGGER"
#define waveDigTriggerCountString "WAVEDIG_TRIGGER_COUNT"
#define waveDigBurstModeString    "WAVEDIG_BURST_MODE"
#define waveDigRunString          "WAVEDIG_RUN"
#define waveDigTimeWFString       "WAVEDIG_TIME_WF"
#define waveDigReadWFString       "WAVEDIG_READ_WF"
// Waveform digitizer parameters - per input
#define waveDigVoltWFString       "WAVEDIG_VOLT_WF"

// Analog output parameters
#define analogOutValueString      "ANALOG_OUT_VALUE"
#define analogOutRangeString      "ANALOG_OUT_RANGE"

// Waveform generator parameters - global
#define waveGenFreqString         "WAVEGEN_FREQ"
#define waveGenDwellString        "WAVEGEN_DWELL"
#define waveGenTotalTimeString    "WAVEGEN_TOTAL_TIME"
#define waveGenNumPointsString    "WAVEGEN_NUM_POINTS"
#define waveGenCurrentPointString "WAVEGEN_CURRENT_POINT"
#define waveGenIntDwellString     "WAVEGEN_INT_DWELL"
#define waveGenUserDwellString    "WAVEGEN_USER_DWELL"
#define waveGenIntNumPointsString  "WAVEGEN_INT_NUM_POINTS"
#define waveGenUserNumPointsString "WAVEGEN_USER_NUM_POINTS"
#define waveGenExtTriggerString   "WAVEGEN_EXT_TRIGGER"
#define waveGenExtClockString     "WAVEGEN_EXT_CLOCK"
#define waveGenContinuousString   "WAVEGEN_CONTINUOUS"
#define waveGenRetriggerString    "WAVEGEN_RETRIGGER"
#define waveGenTriggerCountString "WAVEGEN_TRIGGER_COUNT"
#define waveGenRunString          "WAVEGEN_RUN"
#define waveGenUserTimeWFString   "WAVEGEN_USER_TIME_WF"
#define waveGenIntTimeWFString    "WAVEGEN_INT_TIME_WF"
// Waveform generator parameters - per output
#define waveGenWaveTypeString     "WAVEGEN_WAVE_TYPE"
#define waveGenEnableString       "WAVEGEN_ENABLE"
#define waveGenAmplitudeString    "WAVEGEN_AMPLITUDE"
#define waveGenOffsetString       "WAVEGEN_OFFSET"
#define waveGenPulseWidthString   "WAVEGEN_PULSE_WIDTH"
#define waveGenIntWFString        "WAVEGEN_INT_WF"
#define waveGenUserWFString       "WAVEGEN_USER_WF"

// Trigger parameters
#define triggerModeString         "TRIGGER_MODE"

// Digital I/O parameters
#define digitalDirectionString    "DIGITAL_DIRECTION"
#define digitalInputString        "DIGITAL_INPUT"
#define digitalOutputString       "DIGITAL_OUTPUT"

// MAX_ANALOG_IN and MAX_ANALOG_OUT may need to be changed if additional models are added with larger numbers
// These are used as a convenience for allocating small arrays of pointers, not large amounts of data
#define MAX_ANALOG_IN   16
#define MAX_TEMPERATURE_IN 32
#define MAX_ANALOG_OUT  2
#define MAX_IO_PORTS    2
#define MAX_SIGNALS     MAX_TEMPERATURE_IN

typedef enum {
  USB_1208LS         = 122,
  USB_1208FS         = 130,
  USB_231            = 297,
  USB_1608G          = 308,
  USB_1608GX_2A0     = 274,
  USB_1608GX_2A0_NEW = 310,
  USB_2408_2A0       = 254,
  USB_TC32           = 305,
  ETH_TC32           = 306,
  E_1608             = 303,
  E_TC               = 312,
  MAX_BOARD_TYPES
} boardType_t;

typedef struct {
  char *enumString;
  int  enumValue;
} enumStruct_t;

static const enumStruct_t inputRangeUSB_1208LS[] = {
  {"+= 20V",   BIP20VOLTS},
  {"+= 10V",   BIP10VOLTS},
  {"+= 5V",    BIP5VOLTS},
  {"+= 4V",    BIP4VOLTS},
  {"+= 2.5V",  BIP2PT5VOLTS},
  {"+= 2V",    BIP2VOLTS},
  {"+= 1.25V", BIP1PT25VOLTS},
  {"+= 1V",    BIP1VOLTS}
};

static const enumStruct_t outputRangeUSB_1208LS[] = {
  {"+= 10V", UNI5VOLTS}
};

static const enumStruct_t inputTypeUSB_1208LS[] = {
  {"Volts", AI_CHAN_TYPE_VOLTAGE}
};

static const enumStruct_t inputRangeUSB_1208FS[] = {
  {"+= 20V",   BIP20VOLTS},
  {"+= 10V",   BIP10VOLTS},
  {"+= 5V",    BIP5VOLTS},
  {"+= 4V",    BIP4VOLTS},
  {"+= 2.5V",  BIP2PT5VOLTS},
  {"+= 2V",    BIP2VOLTS},
  {"+= 1.25V", BIP1PT25VOLTS},
  {"+= 1V",    BIP1VOLTS}
};

static const enumStruct_t outputRangeUSB_1208FS[] = {
  {"+= 10V", UNI4VOLTS}
};

static const enumStruct_t inputTypeUSB_1208FS[] = {
  {"Volts", AI_CHAN_TYPE_VOLTAGE}
};

static const enumStruct_t inputRangeUSB_231[] = {
  {"+= 20V",   BIP20VOLTS},
  {"+= 10V",   BIP10VOLTS},
  {"+= 5V",    BIP5VOLTS},
  {"+= 4V",    BIP4VOLTS},
  {"+= 2.5V",  BIP2PT5VOLTS},
  {"+= 2V",    BIP2VOLTS},
  {"+= 1.25V", BIP1PT25VOLTS},
  {"+= 1V",    BIP1VOLTS}
};

static const enumStruct_t outputRangeUSB_231[] = {
  {"+= 10V", UNI5VOLTS}
};

static const enumStruct_t inputTypeUSB_231[] = {
  {"Volts", AI_CHAN_TYPE_VOLTAGE}
};

static const enumStruct_t inputRangeUSB_1608G[] = {
  {"+= 10V", BIP10VOLTS},
  {"+= 5V",  BIP5VOLTS},
  {"+= 2V",  BIP2VOLTS},
  {"+= 1V",  BIP1VOLTS}
};

static const enumStruct_t outputRangeUSB_1608G[] = {
  {"+= 10V", BIP10VOLTS}
};

static const enumStruct_t inputTypeUSB_1608G[] = {
  {"Volts", AI_CHAN_TYPE_VOLTAGE}
};

static const enumStruct_t inputRangeUSB_2408[] = {
  {"+= 10V",    BIP10VOLTS},
  {"+= 5V",     BIP5VOLTS},
  {"+= 2.5V",   BIP2PT5VOLTS},
  {"+= 1.25V",  BIP1PT25VOLTS},
  {"+= 0.625V", BIPPT625VOLTS},
  {"+= 0.312V", BIPPT312VOLTS},
  {"+= 0.156V", BIPPT156VOLTS},
  {"+= 0.078V", BIPPT078VOLTS}
};

static const enumStruct_t outputRangeUSB_2408[] = {
  {"+= 10V", BIP10VOLTS}
};

static const enumStruct_t inputTypeUSB_2408[] = {
  {"Volts",   AI_CHAN_TYPE_VOLTAGE},
  {"TC deg.", AI_CHAN_TYPE_TC}
};

static const enumStruct_t inputRangeE_1608[] = {
  {"+= 10V", BIP10VOLTS},
  {"+= 5V",  BIP5VOLTS},
  {"+= 2V",  BIP2VOLTS},
  {"+= 1V",  BIP1VOLTS}
};

static const enumStruct_t outputRangeE_1608[] = {
  {"+= 10V", BIP10VOLTS}
};

static const enumStruct_t inputTypeE_1608[] = {
  {"Volts", AI_CHAN_TYPE_VOLTAGE}
};

static const enumStruct_t inputRangeTC32[] = {
  {"N.A.", 0}
};

static const enumStruct_t outputRangeTC32[] = {
  {"N.A.", 0}
};

static const enumStruct_t inputTypeTC32[] = {
  {"TC deg.", AI_CHAN_TYPE_TC}
};

static const enumStruct_t inputRangeE_TC[] = {
  {"N.A.", 0}
};

static const enumStruct_t outputRangeE_TC[] = {
  {"N.A.", 0}
};

static const enumStruct_t inputTypeE_TC[] = {
  {"TC deg.", AI_CHAN_TYPE_TC}
};

typedef struct {
  boardType_t boardType;
  const enumStruct_t *pInputRange;
  int numInputRange;
  const enumStruct_t *pOutputRange;
  int numOutputRange;
  const enumStruct_t *pInputType;
  int numInputType;
} boardEnums_t;

static const boardEnums_t allBoardEnums[MAX_BOARD_TYPES] = {
  {USB_1208LS,     inputRangeUSB_1208LS,  sizeof(inputRangeUSB_1208LS)/sizeof(enumStruct_t),
                   outputRangeUSB_1208LS, sizeof(outputRangeUSB_1208LS)/sizeof(enumStruct_t),
                   inputTypeUSB_1208LS,   sizeof(inputTypeUSB_1208LS)/sizeof(enumStruct_t)},

  {USB_1208FS,     inputRangeUSB_1208FS,  sizeof(inputRangeUSB_1208FS)/sizeof(enumStruct_t),
                   outputRangeUSB_1208FS, sizeof(outputRangeUSB_1208FS)/sizeof(enumStruct_t),
                   inputTypeUSB_1208FS,   sizeof(inputTypeUSB_1208FS)/sizeof(enumStruct_t)},

  {USB_231,        inputRangeUSB_231,     sizeof(inputRangeUSB_231)/sizeof(enumStruct_t),
                   outputRangeUSB_231,    sizeof(outputRangeUSB_231)/sizeof(enumStruct_t),
                   inputTypeUSB_231,      sizeof(inputTypeUSB_231)/sizeof(enumStruct_t)},

  {USB_1608G,      inputRangeUSB_1608G,   sizeof(inputRangeUSB_1608G)/sizeof(enumStruct_t),
                   outputRangeUSB_1608G,  sizeof(outputRangeUSB_1608G)/sizeof(enumStruct_t),
                   inputTypeUSB_1608G,    sizeof(inputTypeUSB_1608G)/sizeof(enumStruct_t)},

  {USB_1608GX_2A0, inputRangeUSB_1608G,   sizeof(inputRangeUSB_1608G)/sizeof(enumStruct_t),
                   outputRangeUSB_1608G,  sizeof(outputRangeUSB_1608G)/sizeof(enumStruct_t),
                   inputTypeUSB_1608G,    sizeof(inputTypeUSB_1608G)/sizeof(enumStruct_t)},

  {USB_1608GX_2A0_NEW, inputRangeUSB_1608G,   sizeof(inputRangeUSB_1608G)/sizeof(enumStruct_t),
                   outputRangeUSB_1608G,  sizeof(outputRangeUSB_1608G)/sizeof(enumStruct_t),
                   inputTypeUSB_1608G,    sizeof(inputTypeUSB_1608G)/sizeof(enumStruct_t)},

  {E_1608,         inputRangeE_1608,      sizeof(inputRangeE_1608)/sizeof(enumStruct_t),
                   outputRangeE_1608,     sizeof(outputRangeE_1608)/sizeof(enumStruct_t),
                   inputTypeE_1608,       sizeof(inputTypeE_1608)/sizeof(enumStruct_t)},

  {USB_2408_2A0,   inputRangeUSB_2408,    sizeof(inputRangeUSB_2408)/sizeof(enumStruct_t),
                   outputRangeUSB_2408,   sizeof(outputRangeUSB_2408)/sizeof(enumStruct_t),
                   inputTypeUSB_2408,     sizeof(inputTypeUSB_2408)/sizeof(enumStruct_t)},

  {USB_TC32,       inputRangeTC32,        sizeof(inputRangeTC32)/sizeof(enumStruct_t),
                   outputRangeTC32,       sizeof(outputRangeTC32)/sizeof(enumStruct_t),
                   inputTypeTC32,         sizeof(inputTypeTC32)/sizeof(enumStruct_t)},

  {ETH_TC32,       inputRangeTC32,        sizeof(inputRangeTC32)/sizeof(enumStruct_t),
                   outputRangeTC32,       sizeof(outputRangeTC32)/sizeof(enumStruct_t),
                   inputTypeTC32,         sizeof(inputTypeTC32)/sizeof(enumStruct_t)},

  {E_TC,           inputRangeE_TC,        sizeof(inputRangeE_TC)/sizeof(enumStruct_t),
                   outputRangeE_TC,       sizeof(outputRangeE_TC)/sizeof(enumStruct_t),
                   inputTypeE_TC,         sizeof(inputTypeE_TC)/sizeof(enumStruct_t)},
};

#define DEFAULT_POLL_TIME 0.01
#define ROUND(x) ((x) >= 0. ? (int)x+0.5 : (int)(x-0.5))

#define PI 3.14159265

/** This is the class definition for the MultiFunction class
  */
class MultiFunction : public asynPortDriver {
public:
  MultiFunction(const char *portName, int boardNum, int maxInputPoints, int maxOutputPoints);

  /* These are the methods that we override from asynPortDriver */
  virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
  virtual asynStatus readInt32(asynUser *pasynUser, epicsInt32 *value);
  virtual asynStatus getBounds(asynUser *pasynUser, epicsInt32 *low, epicsInt32 *high);
  virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
  virtual asynStatus readFloat64(asynUser *pasynUser, epicsFloat64 *value);
  virtual asynStatus writeUInt32Digital(asynUser *pasynUser, epicsUInt32 value, epicsUInt32 mask);
  virtual asynStatus readFloat32Array(asynUser *pasynUser, epicsFloat32 *value, size_t nElements, size_t *nIn);
  virtual asynStatus writeFloat32Array(asynUser *pasynUser, epicsFloat32 *value, size_t nElements);
  virtual asynStatus readFloat64Array(asynUser *pasynUser, epicsFloat64 *value, size_t nElements, size_t *nIn);
  virtual asynStatus readEnum(asynUser *pasynUser, char *strings[], int values[], int severities[], size_t nElements, size_t *nIn);
  virtual void report(FILE *fp, int details);
  // These should be private but are called from C
  virtual void pollerThread(void);

protected:
  // Model parameters
  int modelName_;
  int modelNumber_;

  // Pulse generator parameters
  int pulseGenRun_;
  int pulseGenPeriod_;
  int pulseGenWidth_;
  int pulseGenDelay_;
  int pulseGenCount_;
  int pulseGenIdleState_;
  
  // Counter parameters
  int counterCounts_;
  int counterReset_;
  
  // Analog input parameters
  int analogInValue_;
  int analogInRange_;
  int analogInType_;
  
  // Temperature parameters
  int temperatureInValue_;
  int thermocoupleType_;
  int temperatureScale_;
  int temperatureFilter_;
  
  // Waveform digitizer parameters - global
  int waveDigDwell_;
  int waveDigTotalTime_;
  int waveDigFirstChan_;
  int waveDigNumChans_;
  int waveDigNumPoints_;
  int waveDigCurrentPoint_;
  int waveDigExtTrigger_;
  int waveDigExtClock_;
  int waveDigContinuous_;
  int waveDigAutoRestart_;
  int waveDigRetrigger_;
  int waveDigTriggerCount_;
  int waveDigBurstMode_;
  int waveDigRun_;
  int waveDigTimeWF_;
  int waveDigReadWF_;
  // Waveform digitizer parameters - per input
  int waveDigVoltWF_;

  // Analog output parameters
  int analogOutValue_;
  int analogOutRange_;
  
  // Waveform generator parameters - global
  int waveGenFreq_;
  int waveGenDwell_;
  int waveGenTotalTime_;
  int waveGenNumPoints_;
  int waveGenCurrentPoint_;
  int waveGenIntDwell_;
  int waveGenUserDwell_;
  int waveGenIntNumPoints_;
  int waveGenUserNumPoints_;
  int waveGenExtTrigger_;
  int waveGenExtClock_;
  int waveGenContinuous_;
  int waveGenRetrigger_;
  int waveGenTriggerCount_;
  int waveGenRun_;
  int waveGenUserTimeWF_;
  int waveGenIntTimeWF_;
  // Waveform generator parameters - per output
  int waveGenWaveType_;
  int waveGenEnable_;
  int waveGenAmplitude_;
  int waveGenOffset_;
  int waveGenPulseWidth_;
  int waveGenIntWF_;
  int waveGenUserWF_;

  // Trigger parameters
  int triggerMode_;

  // Digital I/O parameters
  int digitalDirection_;
  int digitalInput_;
  int digitalOutput_;

private:
  int boardNum_;
  int boardType_;
  char boardName_[BOARDNAMELEN];
  const boardEnums_t *pBoardEnums_;
  int numAnalogIn_;
  int analogInTypeConfigurable_;
  int numAnalogOut_;
  int ADCResolution_;
  int DACResolution_;
  int numCounters_;
  int firstCounter_;
  int numTimers_;
  int numIOPorts_;
  int numTempChans_;
  int digitalIOPort_[MAX_IO_PORTS];
  int digitalIOBitConfigurable_[MAX_IO_PORTS];
  int digitalIOPortConfigurable_[MAX_IO_PORTS];
  int numIOBits_[MAX_IO_PORTS];
  epicsUInt32 digitalIOMask_[MAX_IO_PORTS];
  double minPulseGenFrequency_;
  double maxPulseGenFrequency_;
  double minPulseGenDelay_;
  double maxPulseGenDelay_;
  double pollTime_;
  int forceCallback_;
  size_t maxInputPoints_;
  size_t maxOutputPoints_;
  epicsFloat64 *waveDigBuffer_[MAX_ANALOG_IN];
  epicsFloat32 *waveDigTimeBuffer_;
  epicsFloat32 *waveGenIntBuffer_[MAX_ANALOG_OUT];
  epicsFloat32 *waveGenUserBuffer_[MAX_ANALOG_OUT];
  epicsFloat32 *waveGenUserTimeBuffer_;
  epicsFloat32 *waveGenIntTimeBuffer_;
  HGLOBAL inputMemHandle_;
  HGLOBAL outputMemHandle_;
  
  int numWaveGenChans_;
  int numWaveDigChans_;
  int pulseGenRunning_;
  int waveGenRunning_;
  int waveDigRunning_;
  int startPulseGenerator();
  int stopPulseGenerator();
  int startWaveGen();
  int stopWaveGen();
  int computeWaveGenTimes();
  int startWaveDig();
  int stopWaveDig();
  int readWaveDig();
  int computeWaveDigTimes();
  int defineWaveform(int channel);
};

#define NUM_PARAMS ((int)(&LAST_MultiFunction_PARAM - &FIRST_MultiFunction_PARAM + 1))

static void pollerThreadC(void * pPvt)
{
    MultiFunction *pMultiFunction = (MultiFunction *)pPvt;
    pMultiFunction->pollerThread();
}

MultiFunction::MultiFunction(const char *portName, int boardNum, int maxInputPoints, int maxOutputPoints)
  : asynPortDriver(portName, MAX_SIGNALS,
      asynInt32Mask | asynUInt32DigitalMask | asynInt32ArrayMask | asynFloat32ArrayMask | asynFloat64ArrayMask | 
                      asynFloat64Mask       | asynEnumMask       | asynDrvUserMask,
      asynInt32Mask | asynUInt32DigitalMask | asynInt32ArrayMask | asynFloat32ArrayMask | asynFloat64ArrayMask | 
                      asynFloat64Mask       | asynEnumMask, 
      ASYN_MULTIDEVICE | ASYN_CANBLOCK, 1, /* ASYN_CANBLOCK=1, ASYN_MULTIDEVICE=1, autoConnect=1 */
      0, 0),  /* Default priority and stack size */
    boardNum_(boardNum),
    pollTime_(DEFAULT_POLL_TIME),
    forceCallback_(1),
    maxInputPoints_(maxInputPoints),
    maxOutputPoints_(maxOutputPoints),
    numWaveGenChans_(1),
    numWaveDigChans_(1),
    pulseGenRunning_(0),
    waveGenRunning_(0),
    waveDigRunning_(0)
{
  int i, j;
  int inMask, outMask;
  static const char *functionName = "MultiFunction";
   
  // Model parameters
  createParam(modelNameString,                 asynParamOctet, &modelName_);
  createParam(modelNumberString,               asynParamInt32, &modelNumber_);

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

  // Analog input parameters
  createParam(analogInValueString,             asynParamInt32, &analogInValue_);
  createParam(analogInRangeString,             asynParamInt32, &analogInRange_);
  createParam(analogInTypeString,              asynParamInt32, &analogInType_);
  
  // Temperature parameters
  createParam(temperatureInValueString,      asynParamFloat64, &temperatureInValue_);
  createParam(thermocoupleTypeString,          asynParamInt32, &thermocoupleType_);
  createParam(temperatureScaleString,          asynParamInt32, &temperatureScale_);
  createParam(temperatureFilterString,         asynParamInt32, &temperatureFilter_);
 
  // Waveform digitizer parameters - global
  createParam(waveDigDwellString,            asynParamFloat64, &waveDigDwell_);
  createParam(waveDigTotalTimeString,        asynParamFloat64, &waveDigTotalTime_);
  createParam(waveDigFirstChanString,          asynParamInt32, &waveDigFirstChan_);
  createParam(waveDigNumChansString,           asynParamInt32, &waveDigNumChans_);
  createParam(waveDigNumPointsString,          asynParamInt32, &waveDigNumPoints_);
  createParam(waveDigCurrentPointString,       asynParamInt32, &waveDigCurrentPoint_);
  createParam(waveDigExtTriggerString,         asynParamInt32, &waveDigExtTrigger_);
  createParam(waveDigExtClockString,           asynParamInt32, &waveDigExtClock_);
  createParam(waveDigContinuousString,         asynParamInt32, &waveDigContinuous_);
  createParam(waveDigAutoRestartString,        asynParamInt32, &waveDigAutoRestart_);
  createParam(waveDigRetriggerString,          asynParamInt32, &waveDigRetrigger_);
  createParam(waveDigTriggerCountString,       asynParamInt32, &waveDigTriggerCount_);
  createParam(waveDigBurstModeString,          asynParamInt32, &waveDigBurstMode_);
  createParam(waveDigRunString,                asynParamInt32, &waveDigRun_);
  createParam(waveDigTimeWFString,      asynParamFloat32Array, &waveDigTimeWF_);
  createParam(waveDigReadWFString,             asynParamInt32, &waveDigReadWF_);
  // Waveform digitizer parameters - per input
  createParam(waveDigVoltWFString,      asynParamFloat32Array, &waveDigVoltWF_);

  // Analog output parameters
  createParam(analogOutValueString,            asynParamInt32, &analogOutValue_);
  createParam(analogOutRangeString,            asynParamInt32, &analogOutRange_);
  
  // Waveform generator parameters - global
  createParam(waveGenFreqString,             asynParamFloat64, &waveGenFreq_);
  createParam(waveGenDwellString,            asynParamFloat64, &waveGenDwell_);
  createParam(waveGenTotalTimeString,        asynParamFloat64, &waveGenTotalTime_);
  createParam(waveGenNumPointsString,          asynParamInt32, &waveGenNumPoints_);
  createParam(waveGenCurrentPointString,       asynParamInt32, &waveGenCurrentPoint_);
  createParam(waveGenIntDwellString,         asynParamFloat64, &waveGenIntDwell_);
  createParam(waveGenUserDwellString,        asynParamFloat64, &waveGenUserDwell_);
  createParam(waveGenIntNumPointsString,       asynParamInt32, &waveGenIntNumPoints_);
  createParam(waveGenUserNumPointsString,      asynParamInt32, &waveGenUserNumPoints_);
  createParam(waveGenExtTriggerString,         asynParamInt32, &waveGenExtTrigger_);
  createParam(waveGenExtClockString,           asynParamInt32, &waveGenExtClock_);
  createParam(waveGenContinuousString,         asynParamInt32, &waveGenContinuous_);
  createParam(waveGenRetriggerString,          asynParamInt32, &waveGenRetrigger_);
  createParam(waveGenTriggerCountString,       asynParamInt32, &waveGenTriggerCount_);
  createParam(waveGenRunString,                asynParamInt32, &waveGenRun_);
  createParam(waveGenUserTimeWFString,  asynParamFloat32Array, &waveGenUserTimeWF_);
  createParam(waveGenIntTimeWFString,   asynParamFloat32Array, &waveGenIntTimeWF_);
  // Waveform generator parameters - per output
  createParam(waveGenWaveTypeString,           asynParamInt32, &waveGenWaveType_);
  createParam(waveGenEnableString,             asynParamInt32, &waveGenEnable_);
  createParam(waveGenAmplitudeString,        asynParamFloat64, &waveGenAmplitude_);
  createParam(waveGenOffsetString,           asynParamFloat64, &waveGenOffset_);
  createParam(waveGenPulseWidthString,       asynParamFloat64, &waveGenPulseWidth_);
  createParam(waveGenIntWFString,       asynParamFloat32Array, &waveGenIntWF_);
  createParam(waveGenUserWFString,      asynParamFloat32Array, &waveGenUserWF_);

  // Trigger parameters
  createParam(triggerModeString,               asynParamInt32, &triggerMode_);

  // Digital I/O parameters
  createParam(digitalDirectionString,  asynParamUInt32Digital, &digitalDirection_);
  createParam(digitalInputString,      asynParamUInt32Digital, &digitalInput_);
  createParam(digitalOutputString,     asynParamUInt32Digital, &digitalOutput_);

  // Get the board number and board name
  cbGetConfig(BOARDINFO, boardNum_, 0, BIBOARDTYPE,  &boardType_);

  setIntegerParam(modelNumber_, boardType_);
  cbGetBoardName(boardNum_, boardName_);
  setStringParam(modelName_, boardName_);
  cbGetConfig(BOARDINFO, boardNum_, 0, BINUMADCHANS,    &numAnalogIn_);
  cbGetConfig(BOARDINFO, boardNum_, 0, BINUMDACHANS,    &numAnalogOut_);
  cbGetConfig(BOARDINFO, boardNum_, 0, BIADRES,         &ADCResolution_);
  cbGetConfig(BOARDINFO, boardNum_, 0, BIDACRES,        &DACResolution_);
  cbGetConfig(BOARDINFO, boardNum_, 0, BIDINUMDEVS,     &numIOPorts_);
  cbGetConfig(BOARDINFO, boardNum_, 0, BINUMTEMPCHANS,  &numTempChans_);
  if (numIOPorts_ > MAX_IO_PORTS) numIOPorts_ = MAX_IO_PORTS;
  for (i=0; i<numIOPorts_; i++) {
    cbGetConfig(DIGITALINFO, boardNum_, i, DIDEVTYPE, &digitalIOPort_[i]);
    cbGetConfig(DIGITALINFO, boardNum_, i, DIINMASK,  &inMask);
    cbGetConfig(DIGITALINFO, boardNum_, i, DIOUTMASK, &outMask);
    cbGetConfig(DIGITALINFO, boardNum_, i, DINUMBITS, &numIOBits_[i]);
    digitalIOPortConfigurable_[i] = 0;
    digitalIOBitConfigurable_[i] = ((inMask & outMask) == 0);
    digitalIOMask_[i] = 0;
    for (j=0; j<numIOBits_[i]; j++) {
      digitalIOMask_[i] |= (1 << j);
    }
  }
  // Assume only voltage input is supported
  analogInTypeConfigurable_ = 0;
  switch (boardType_) {
    case USB_1208LS:
      numTimers_    = 0;
      numCounters_  = 1;
      firstCounter_ = 1;
      // For output need to address all bits using first port
      numIOBits_[0] = 16;
      // The rules for bit configurable above don't work for this model                
      digitalIOPortConfigurable_[0] = 1;
      digitalIOPortConfigurable_[1] = 1;
      digitalIOBitConfigurable_[0] = 0;
      digitalIOPortConfigurable_[1] = 1;
      break;
    case USB_1208FS:
      numTimers_    = 0;
      numCounters_  = 1;
      firstCounter_ = 1;
      // For output need to address all bits using first port
      numIOBits_[0] = 16;
      // The rules for bit configurable above don't work for this model                
      digitalIOPortConfigurable_[0] = 1;
      digitalIOPortConfigurable_[1] = 1;
      digitalIOBitConfigurable_[0] = 0;
      digitalIOPortConfigurable_[1] = 1;
      break;
    case USB_231:
      numTimers_    = 0;
      numCounters_  = 1;
      firstCounter_ = 0;
      // For output need to address all bits using first port
      numIOBits_[0] = 8;
      // The rules for bit configurable above don't work for this model                
      digitalIOPortConfigurable_[0] = 1;
      digitalIOPortConfigurable_[1] = 1;
      digitalIOBitConfigurable_[0] = 0;
      digitalIOPortConfigurable_[1] = 1;
      break;
    case USB_2408_2A0:
      numTimers_    = 0;
      numCounters_  = 2;
      firstCounter_ = 0;
      analogInTypeConfigurable_  = 1; // Supports voltage and thermocouple
      break;
    case USB_1608G:
    case USB_1608GX_2A0:
    case USB_1608GX_2A0_NEW:
      numTimers_    = 1;
      numCounters_  = 2;
      firstCounter_ = 0;
      minPulseGenFrequency_ = 0.0149;
      maxPulseGenFrequency_ = 32e6;
      minPulseGenDelay_ = 0.;
      maxPulseGenDelay_ = 67.11;
      break;
    case E_1608:
      numTimers_    = 0;
      numCounters_  = 1;
      firstCounter_ = 0;
      break;
    case USB_TC32:
    case ETH_TC32:
      numTimers_    = 0;
      numCounters_  = 0;
      for (i=0; i<MAX_TEMPERATURE_IN; i++) {
        setIntegerParam(i, analogInType_, AI_CHAN_TYPE_TC);
      }
      // Digital I/O port 0 is input only
      setUIntDigitalParam(0, digitalDirection_, 0, 0xFFFFFFFF);
      // Digital I/O port 1 is output and temperature alarms
      setUIntDigitalParam(1, digitalDirection_, 0xFFFFFFFF, 0xFFFFFFFF);
      break;
    case E_TC:
      numTimers_    = 0;
      numCounters_  = 1;
      firstCounter_ = 0;
      for (i=0; i<MAX_TEMPERATURE_IN; i++) {
        setIntegerParam(i, analogInType_, AI_CHAN_TYPE_TC);
      }
      break;
    default:
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
        "%s::%s error, unknown board type=%d\n", 
        driverName, functionName, boardType_);
      break;
  }

  for (pBoardEnums_=allBoardEnums; pBoardEnums_->boardType != boardType_; pBoardEnums_++);

  // Allocate memory for the input and output buffers
  for (i=0; i<numAnalogIn_; i++) {
    waveDigBuffer_[i]  = (epicsFloat64 *) calloc(maxInputPoints_,  sizeof(epicsFloat64));
  }
  for (i=0; i<numAnalogOut_; i++) {
    waveGenIntBuffer_[i]  = (epicsFloat32 *) calloc(maxOutputPoints_, sizeof(epicsFloat32));
    waveGenUserBuffer_[i] = (epicsFloat32 *) calloc(maxOutputPoints_, sizeof(epicsFloat32));
  }
  waveGenUserTimeBuffer_ = (epicsFloat32 *) calloc(maxOutputPoints_, sizeof(epicsFloat32));
  waveGenIntTimeBuffer_  = (epicsFloat32 *) calloc(maxOutputPoints_, sizeof(epicsFloat32));
  waveDigTimeBuffer_     = (epicsFloat32 *) calloc(maxInputPoints_,  sizeof(epicsFloat32));
  inputMemHandle_  = cbScaledWinBufAlloc(maxInputPoints  * numAnalogIn_);
  outputMemHandle_ = cbWinBufAlloc(maxOutputPoints * numAnalogOut_);
  
  // Set values of some parameters that need to be set because init record order is not predictable
  // or because the corresponding records are PINI=NO.
  setIntegerParam(waveGenUserNumPoints_, 1);
  setIntegerParam(waveGenIntNumPoints_, 1);
  setIntegerParam(waveDigNumPoints_, 1);
  setIntegerParam(pulseGenRun_, 0);
  setIntegerParam(waveDigRun_, 0);
  setIntegerParam(waveGenRun_, 0);

  /* Start the thread to poll counters and digital inputs and do callbacks to 
   * device support */
  epicsThreadCreate("MultiFunctionPoller",
                    epicsThreadPriorityLow,
                    epicsThreadGetStackSize(epicsThreadStackMedium),
                    (EPICSTHREADFUNC)pollerThreadC,
                    this);
}

int MultiFunction::startPulseGenerator()
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
  if (frequency < minPulseGenFrequency_) frequency = minPulseGenFrequency_;
  if (frequency > maxPulseGenFrequency_) frequency = maxPulseGenFrequency_;
  dutyCycle = width * frequency;
  period = 1. / frequency;
  if (dutyCycle <= 0.) dutyCycle = .0001;
  if (dutyCycle >= 1.) dutyCycle = .9999;
  if (delay < minPulseGenDelay_) delay = minPulseGenDelay_;
  if (delay > maxPulseGenDelay_) delay = maxPulseGenDelay_;

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

int MultiFunction::stopPulseGenerator()
{
  pulseGenRunning_ = 0;
  return cbPulseOutStop(boardNum_, 0);
}

int MultiFunction::defineWaveform(int channel)
{
  int waveType;
  int numPoints;
  int nPulse;
  int i;
  epicsFloat32 *outPtr = waveGenIntBuffer_[channel];
  double dwell, offset, base, amplitude, pulseWidth, scale;
  static const char *functionName = "defineWaveform";

  getIntegerParam(channel, waveGenWaveType_,  &waveType);
  if (waveType == waveTypeUser) {
    getIntegerParam(waveGenUserNumPoints_, &numPoints);
    if ((size_t)numPoints > maxOutputPoints_) {
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
        "%s:%s: ERROR numPoints=%d must be less than maxOutputPoints=%d\n",
        driverName, functionName, numPoints, (int)maxOutputPoints_);
      return -1;
    }
    getDoubleParam(waveGenUserDwell_, &dwell);
    setIntegerParam(waveGenNumPoints_, numPoints);
    setDoubleParam(waveGenDwell_, dwell);
    setDoubleParam(waveGenFreq_, 1./dwell/numPoints);
    return 0;
  }

  getIntegerParam(waveGenIntNumPoints_,  &numPoints);
  if ((size_t)numPoints > maxOutputPoints_) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
      "%s:%s: ERROR numPoints=%d must be less than maxOutputPoints=%d\n",
      driverName, functionName, numPoints, (int)maxOutputPoints_);
    return -1;
  }

  getDoubleParam(waveGenIntDwell_,             &dwell);
  getDoubleParam(channel, waveGenOffset_,      &offset);
  getDoubleParam(channel, waveGenAmplitude_,   &amplitude);
  getDoubleParam(channel, waveGenPulseWidth_,  &pulseWidth);
  setIntegerParam(waveGenNumPoints_, numPoints);
  setDoubleParam(waveGenDwell_, dwell);
  setDoubleParam(waveGenFreq_, 1./dwell/numPoints);
  base = offset - amplitude/2.;
  switch (waveType) {
    case waveTypeSin:
      scale = 2.*PI/(numPoints-1);
      for (i=0; i<numPoints; i++)           *outPtr++ = (epicsFloat32) (offset + amplitude/2. * sin(i*scale));
      break;
    case waveTypeSquare:
      for (i=0; i<numPoints/2; i++)         *outPtr++ = (epicsFloat32) (base + amplitude);
      for (i=numPoints/2; i<numPoints; i++) *outPtr++ = (epicsFloat32) (base);
      break;
    case waveTypeSawTooth:
      scale = 1./(numPoints-1);
      for (i=0; i<numPoints; i++)           *outPtr++ = (epicsFloat32) (base + amplitude*i*scale);
      break;
    case waveTypePulse:
      nPulse = (int) ((pulseWidth / dwell) + 0.5);
      if (nPulse < 1) nPulse = 1;
      if (nPulse >= numPoints-1) nPulse = numPoints-1;
      for (i=0; i<nPulse; i++)              *outPtr++ = (epicsFloat32) (base + amplitude);
      for (i=nPulse; i<numPoints; i++)      *outPtr++ = (epicsFloat32) (base);
      break;
    case waveTypeRandom:
      scale = amplitude / RAND_MAX;
      srand(1);
      for (i=0; i<numPoints; i++)           *outPtr++ = (epicsFloat32) (base + rand() * scale);
      break;
  }
  doCallbacksFloat32Array(waveGenIntBuffer_[channel], numPoints, waveGenIntWF_, channel);
  return 0;
}
 
int MultiFunction::startWaveGen()
{
  int status=0;
  int numPoints;
  int enable;
  int firstChan=-1, lastChan=-1, firstType=-1;
  long pointsPerSecond;
  int waveType;
  int extTrigger, extClock, continuous, retrigger;
  int options;
  int i, j, k;
  double offset, scale;
  double userOffset, userAmplitude;
  double dwell;
  epicsFloat32* inPtr[MAX_ANALOG_OUT];
  epicsUInt16 *outPtr;
  static const char *functionName = "startWaveGen";
  
  getIntegerParam(waveGenExtTrigger_, &extTrigger);
  getIntegerParam(waveGenExtClock_,   &extClock);
  getIntegerParam(waveGenContinuous_, &continuous);
  getIntegerParam(waveGenRetrigger_,  &retrigger);
 
  for (i=0; i<numAnalogOut_; i++) {
    getIntegerParam(i, waveGenEnable_, &enable);
    if (!enable) continue;
    getIntegerParam(i, waveGenWaveType_,  &waveType);
    if (waveType == waveTypeUser)
      inPtr[i] = waveGenUserBuffer_[i];
    else
      inPtr[i] = waveGenIntBuffer_[i];
    if (firstChan < 0) {
      firstChan = i;
      firstType = waveType;
    }
    // Cannot mix user-defined and internal waveform types, because internal modifies dwell time
    // based on frequency
    if (((firstType == waveTypeUser) && (waveType != waveTypeUser)) ||
        ((firstType != waveTypeUser) && (waveType == waveTypeUser))) {
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
        "%s:%s: ERROR if any enabled waveform type is user-defined then all must be.\n",
        driverName, functionName);
      return -1;
    }
    lastChan = i;
    status = defineWaveform(i);
    if (status) return -1;
  }
  
  if (firstChan < 0) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
      "%s:%s: ERROR no enabled channels\n",
      driverName, functionName);
     return -1;
  }
  
  numWaveGenChans_ = lastChan - firstChan + 1;

  // dwell and numPoints were computed by defineWaveform above
  getIntegerParam(waveGenNumPoints_, &numPoints);
  getDoubleParam(waveGenDwell_, &dwell);
  pointsPerSecond = (long)((1. / dwell) + 0.5);
  
  // Copy data from float32 array to outputMemHandel, converting from volts to D/A units
  // Pre-defined waveforms have been fully defined at this point
  // User-defined waveforms need to have the offset and scale applied
  for (i=0; i<numWaveGenChans_; i++) {
    k = firstChan + i;
    outPtr = &((epicsUInt16 *) outputMemHandle_)[i];
    offset = 10.;        // Mid-scale range of DAC
    scale = 65535./20.;  // D/A units per volt; 16-bit DAC, +-10V range
    if (waveType == waveTypeUser) {
      getDoubleParam(i, waveGenOffset_, &userOffset);
      getDoubleParam(i, waveGenAmplitude_,  &userAmplitude);
    } else {
      userOffset = 0.;
      userAmplitude = 1.0;
    }
    for (j=0; j<numPoints; j++) {
     *outPtr = (epicsUInt16)((inPtr[k][j]*userAmplitude + userOffset + offset)*scale + 0.5);
      outPtr += numWaveGenChans_;
    }
  }
  options                  = BACKGROUND;
  if (extTrigger) options |= EXTTRIGGER;
  if (extClock)   options |= EXTCLOCK;
  if (continuous) options |= CONTINUOUS;
  if (retrigger)  options |= RETRIGMODE;
  
  status = cbAOutScan(boardNum_, firstChan, lastChan, numWaveGenChans_*numPoints, &pointsPerSecond, BIP10VOLTS,
                      outputMemHandle_, options);

  if (status) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
      "%s:%s: ERROR calling cbAOutScan, firstChan=%d, lastChan=%d, numPoints=%d, pointsPerSecond=%ld, options=0x%x, status=%d\n",
      driverName, functionName, firstChan, lastChan, numPoints, pointsPerSecond, options, status);
    return status;
  }

  waveGenRunning_ = 1;
  setIntegerParam(waveGenRun_, 1);
  asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
    "%s:%s: called cbAOutScan, firstChan=%d, lastChan=%d, numPoints*numWaveGenChans_=%d, pointsPerSecond=%ld, options=0x%x\n",
    driverName, functionName, firstChan, lastChan,  numWaveGenChans_*numPoints, pointsPerSecond, options);

  // Convert back from pointsPerSecond to dwell, since value might have changed
  dwell = (1. / pointsPerSecond);
  setDoubleParam(waveGenDwell_, dwell);
  setDoubleParam(waveGenTotalTime_, dwell*numPoints);
  return status;
}
  
int MultiFunction::stopWaveGen()
{
  waveGenRunning_ = 0;
  setIntegerParam(waveGenRun_, 0);
  return cbStopBackground(boardNum_, AOFUNCTION);
}

int MultiFunction::computeWaveGenTimes()
{
  int numPoints, i;
  double dwell;
  
  getIntegerParam(waveGenUserNumPoints_, &numPoints);
  getDoubleParam(waveGenUserDwell_, &dwell);
  for (i=0; i<numPoints; i++) {
    waveGenUserTimeBuffer_[i] = (epicsFloat32) (i * dwell);
  }
  doCallbacksFloat32Array(waveGenUserTimeBuffer_, numPoints, waveGenUserTimeWF_, 0);

  getIntegerParam(waveGenIntNumPoints_, &numPoints);
  getDoubleParam(waveGenIntDwell_, &dwell);
  for (i=0; i<numPoints; i++) {
    waveGenIntTimeBuffer_[i] = (epicsFloat32) (i * dwell);
  }
  doCallbacksFloat32Array(waveGenIntTimeBuffer_, numPoints, waveGenIntTimeWF_, 0);
  return 0;
}

int MultiFunction::startWaveDig()
{
  int firstChan, lastChan, numChans, numPoints;
  int chan, range;
  short gainArray[MAX_ANALOG_IN], chanArray[MAX_ANALOG_IN];
  int i;
  int extTrigger, extClock, continuous, retrigger, burstMode;
  int status;
  int options;
  long pointsPerSecond;
  double dwell;
  static const char *functionName = "startWaveDig";
  
  getIntegerParam(waveDigNumPoints_,  &numPoints);
  getIntegerParam(waveDigFirstChan_,  &firstChan);
  getIntegerParam(waveDigNumChans_,   &numChans);
  numWaveDigChans_ = numChans;
  getIntegerParam(waveDigExtTrigger_, &extTrigger);
  getIntegerParam(waveDigExtClock_,   &extClock);
  getIntegerParam(waveDigContinuous_, &continuous);
  getIntegerParam(waveDigRetrigger_,  &retrigger);
  getIntegerParam(waveDigBurstMode_,  &burstMode);
  getDoubleParam(waveDigDwell_, &dwell);
  pointsPerSecond = (long)((1. / dwell) + 0.5);
  
  options                  = BACKGROUND;
  options                 |= SCALEDATA;
  if (extTrigger) options |= EXTTRIGGER;
  if (extClock)   options |= EXTCLOCK;
  if (continuous) options |= CONTINUOUS;
  if (retrigger)  options |= RETRIGMODE;
  if (burstMode)  options |= BURSTMODE;
  lastChan = firstChan + numChans - 1;
  
  // Construct the gain array
  for (i=0; i<numChans; i++) {
    chan = firstChan + i;
    chanArray[i] = chan;
    getIntegerParam(chan, analogInRange_, &range);
    gainArray[i] = range;
  }
  status = cbALoadQueue(boardNum_, chanArray, gainArray, numChans);
  if (status) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
      "%s:%s: ERROR calling cbALoadQueue, chanArray[0]=%d, gainArray[0]=%d, numChans=%d, status=%d\n",
      driverName, functionName, chanArray[0], gainArray[0], numChans, status);
    return status;
  }

  status = cbAInScan(boardNum_, firstChan, lastChan, numWaveDigChans_*numPoints, &pointsPerSecond, BIP10VOLTS,
                     inputMemHandle_, options);
                     
  if (status) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
      "%s:%s: ERROR calling cbAInScan, firstChan=%d, lastChan=%d, numPoints=%d, pointsPerSecond=%ld, options=0x%x, status=%d\n",
      driverName, functionName, firstChan, lastChan, numPoints, pointsPerSecond, options, status);
    return status;
  }

  waveDigRunning_ = 1;
  setIntegerParam(waveDigRun_, 1);
  asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
    "%s:%s: called cbAInScan, firstChan=%d, lastChan=%d, numPoints=%d, pointsPerSecond=%ld, options=0x%x\n",
    driverName, functionName, firstChan, lastChan, numPoints, pointsPerSecond, options);

  // Convert back from pointsPerSecond to dwell, since value might have changed
  dwell = (1. / pointsPerSecond);
  setDoubleParam(waveDigDwell_, dwell);
  setDoubleParam(waveDigTotalTime_, dwell*numPoints);
  return 0;
}

int MultiFunction::stopWaveDig()
{
  int autoRestart;
  int status;
  
  waveDigRunning_ = 0;
  setIntegerParam(waveDigRun_, 0);
  readWaveDig();
  getIntegerParam(waveDigAutoRestart_, &autoRestart);
  status = cbStopBackground(boardNum_, AIFUNCTION);
  if (autoRestart)
    status |= startWaveDig();
  return status;
}

int MultiFunction::readWaveDig()
{
  int firstChan, lastChan;
  int currentPoint;
  int i, j;
  epicsFloat64 *pAnalogIn = (epicsFloat64 *)inputMemHandle_;
  static const char *functionName = "readWaveDig";
  
  getIntegerParam(waveDigFirstChan_,    &firstChan);
  lastChan = firstChan + numWaveDigChans_ - 1;
  getIntegerParam(waveDigCurrentPoint_, &currentPoint);
  for (i=0; i<currentPoint; i++) {
    for (j=firstChan; j<=lastChan; j++) {
      waveDigBuffer_[j][i] = *pAnalogIn++;
    }
  }
  for (i=firstChan; i<=lastChan; i++) {
    asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
      "%s:%s:, doing callbacks on input %d, first value=%f\n", 
      driverName, functionName, i, waveDigBuffer_[i][0]);
    doCallbacksFloat64Array(waveDigBuffer_[i], currentPoint, waveDigVoltWF_, i);
  }
  return 0;
}    

int MultiFunction::computeWaveDigTimes()
{
  int numPoints, i;
  double dwell;
  
  getIntegerParam(waveDigNumPoints_, &numPoints);
  getDoubleParam(waveDigDwell_, &dwell);
  for (i=0; i<numPoints; i++) {
    waveDigTimeBuffer_[i] = (epicsFloat32) (i * dwell);
  }
  doCallbacksFloat32Array(waveDigTimeBuffer_, numPoints, waveDigTimeWF_, 0);
  return 0;
}


asynStatus MultiFunction::getBounds(asynUser *pasynUser, epicsInt32 *low, epicsInt32 *high)
{
  int function = pasynUser->reason;

  
  if (function == analogOutValue_) {
    *low = 0;
    *high = (1 << DACResolution_) - 1;
  } 
  else if (function == analogInValue_) {
    *low = 0;
    *high = (1 << ADCResolution_) - 1;
  }
  else {
    return(asynError);
  }
  return(asynSuccess);
}

asynStatus MultiFunction::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
  int addr;
  int function = pasynUser->reason;
  int status=0;
  static const char *functionName = "writeInt32";

  this->getAddress(pasynUser, &addr);
  setIntegerParam(addr, function, value);

  // Analog input functions
  if (function == analogInType_ && analogInTypeConfigurable_) {
    status = cbSetConfig(BOARDINFO, boardNum_, addr, BIADCHANTYPE, value);
    // It seems to be necessary to reprogram the thermocouple type when switching from volts to TC
    if (value == AI_CHAN_TYPE_TC) {
      int tcType;
      getIntegerParam(addr, thermocoupleType_, &tcType);
      status |= cbSetConfig(BOARDINFO, boardNum_, addr, BICHANTCTYPE, tcType);
    }
  }

  else if (function == thermocoupleType_) {
    // NOTE:
    // This sleep is a hack to get it working on the TC-32.  Without it the call to cbSetConfig()
    // will often hang if more than 6 channels are being configured.
    // This makes no sense.  The problem cannot be reproduced in the testTC32.c test application
    if ((boardType_ == USB_TC32) || (boardType_ == ETH_TC32)) epicsThreadSleep(0.01);
    status = cbSetConfig(BOARDINFO, boardNum_, addr, BICHANTCTYPE, value);
  }

  // Pulse generator functions
  else if (function == pulseGenRun_) {
    // Allow starting a run even if it thinks its running,
    // since there is no way to know when it got done if Count!=0
    if (value) {
      status = startPulseGenerator();
    } 
    else if (!value && pulseGenRunning_) {
      status = stopPulseGenerator();
    }
  }
  else if ((function == pulseGenCount_) ||
           (function == pulseGenIdleState_)) {
    if (pulseGenRunning_) {
      status = stopPulseGenerator();
      status |= startPulseGenerator();
    }
  }

  // Counter functions
  else if (function == counterReset_) {
    // LOADREG0=0, LOADREG1=1, so we use addr
    status = cbCLoad32(boardNum_, addr, 0);
  }
  
  // Trigger functions
  else if (function == triggerMode_) {
    status = cbSetTrigger(boardNum_, value, 0, 0);
  }

  // Waveform digitizer functions
  else if (function == waveDigRun_) {
    if (value && !waveDigRunning_)
      status = startWaveDig();
    else if (!value && waveDigRunning_) 
      status = stopWaveDig();
  }
  
  else if (function == waveDigReadWF_) {
    readWaveDig();
  }

  else if (function == waveDigNumPoints_) {
    computeWaveDigTimes();
  }
  
  else if (function == waveDigTriggerCount_) {
    status = cbSetConfig(BOARDINFO, boardNum_, 0, BIADTRIGCOUNT, value);
  }

  // Analog output functions
  else if (function == analogOutValue_) {
    if (waveGenRunning_) {
      asynPrint(pasynUser, ASYN_TRACE_ERROR,
        "%s:%s: ERROR cannot write analog outputs while waveform generator is running.\n",
        driverName, functionName);
      return asynError;
    }
    status = cbAOut(boardNum_, addr, BIP10VOLTS, value);
  }

  // Waveform generator functions
  else if (function == waveGenRun_) {
    if (value && !waveGenRunning_)
      status = startWaveGen();
    else if (!value && waveGenRunning_) 
      status = stopWaveGen();
  }

  else if ((function == waveGenWaveType_) ||
      (function == waveGenUserNumPoints_) ||
      (function == waveGenIntNumPoints_)  ||
      (function == waveGenEnable_)        ||
      (function == waveGenExtTrigger_)    ||
      (function == waveGenExtClock_)      ||
      (function == waveGenContinuous_)) {
    defineWaveform(addr);
    if (waveGenRunning_) {
      status = stopWaveGen();
      status |= startWaveGen();
    }
  }

  else if (function == waveGenTriggerCount_) {
    status = cbSetConfig(BOARDINFO, boardNum_, 0, BIDACTRIGCOUNT, value);
  }

  // This is a separate if statement because these cases are also treated above
  if ((function == waveGenUserNumPoints_) ||
      (function == waveGenIntNumPoints_)) {
    computeWaveGenTimes();
  }
  
  callParamCallbacks(addr);
  if (status == 0) {
    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
             "%s:%s, port %s, wrote %d to address %d\n",
             driverName, functionName, this->portName, value, addr);
  } else {
    asynPrint(pasynUser, ASYN_TRACE_ERROR, 
             "%s:%s, port %s, function=%d, ERROR writing %d to address %d, status=%d\n",
             driverName, functionName, this->portName, function, value, addr, status);
  }
  return (status==0) ? asynSuccess : asynError;
}


asynStatus MultiFunction::readInt32(asynUser *pasynUser, epicsInt32 *value)
{
  int addr;
  int function = pasynUser->reason;
  int status=0;
  int type;
  epicsUInt16 shortVal;
  ULONG ulongVal;
  int range;
  static const char *functionName = "readInt32";

  this->getAddress(pasynUser, &addr);

  // Analog input function
  if (function == analogInValue_) {
    if (waveDigRunning_) return asynSuccess;
    getIntegerParam(addr, analogInRange_, &range);
    getIntegerParam(addr, analogInType_, &type);
    if (type != AI_CHAN_TYPE_VOLTAGE) return asynSuccess;
    // NOTE: There is something wrong with their driver.  
    // If cbAIn is just called once there is a large error due apparently
    // to not allowing settling time before reading.  For now we read twice
    // which removes the error.
    if (ADCResolution_ <= 16) {
      status = cbAIn(boardNum_, addr, range, &shortVal);
      status = cbAIn(boardNum_, addr, range, &shortVal);
      *value = shortVal;
    } else {
      status = cbAIn32(boardNum_, addr, range, &ulongVal, 0);
      status = cbAIn32(boardNum_, addr, range, &ulongVal, 0);
      *value = (epicsInt32)ulongVal;
    }
    setIntegerParam(addr, analogInValue_, *value);
    if (status) {
      asynPrint(pasynUser, ASYN_TRACE_ERROR, 
        "%s::%s, port %s, function=%d, addr=%d, range=%d, ERROR reading status=%d\n",
          driverName, functionName, this->portName, function, addr, range, status);
    }
  }

  // Other functions we call the base class method
  else {
     status = asynPortDriver::readInt32(pasynUser, value);
  }

  callParamCallbacks(addr);
  return (status==0) ? asynSuccess : asynError;
}

asynStatus MultiFunction::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
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
  
  // Waveform generator functions
  else if ((function == waveGenUserDwell_)  ||
           (function == waveGenIntDwell_)   ||
           (function == waveGenPulseWidth_) ||
           (function == waveGenAmplitude_)  ||
           (function == waveGenOffset_)) {
    defineWaveform(addr);
    if (waveGenRunning_) {
      status = stopWaveGen();
      status |= startWaveGen();
    }
  }
  
  // Waveform digitizer functions
  else if (function == waveDigDwell_) {
    computeWaveDigTimes();
  }

  // This is a separate if statement because these cases are also treated above
  if ((function == waveGenUserDwell_)  ||
      (function == waveGenIntDwell_)) {
    computeWaveGenTimes();
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

asynStatus MultiFunction::readFloat64(asynUser *pasynUser, epicsFloat64 *value)
{
  int addr;
  int function = pasynUser->reason;
  int status=0;
  int type;
  int scale;
  float fVal;
  int filter=0;
  static const char *functionName = "readFloat64";

  this->getAddress(pasynUser, &addr);

  // Temperature input function
  if (function == temperatureInValue_) {
    if (waveDigRunning_) return asynSuccess;
    getIntegerParam(addr, analogInType_, &type);
    getIntegerParam(addr, temperatureScale_, &scale);
    getIntegerParam(addr, temperatureFilter_, &filter);
    if (type != AI_CHAN_TYPE_TC) return asynSuccess;
    status = cbTIn(boardNum_, addr, scale, &fVal, filter);
    if (status == OPENCONNECTION) {
      // This is an "expected" error if the thermocouple is broken or disconnected
      // Don't print error message, just set temp to -9999.
      fVal = -9999.;
      status = 0;
    }
    *value = fVal;
    setDoubleParam(addr, temperatureInValue_, *value);
    if (status) {
      asynPrint(pasynUser, ASYN_TRACE_ERROR, 
        "%s::%s, port %s, function=%d, addr=%d, scale=%d, filter=%d, ERROR reading status=%d\n",
          driverName, functionName, this->portName, function, addr, scale, filter, status);
    }
  }

  // Other functions we call the base class method
  else {
     status = asynPortDriver::readFloat64(pasynUser, value);
  }

  callParamCallbacks(addr);
  return (status==0) ? asynSuccess : asynError;
}

asynStatus MultiFunction::writeUInt32Digital(asynUser *pasynUser, epicsUInt32 value, epicsUInt32 mask)
{
  int function = pasynUser->reason;
  int status=0;
  int i;
  int addr;
  epicsUInt32 outValue=0, outMask, direction=0;
  static const char *functionName = "writeUInt32Digital";

  this->getAddress(pasynUser, &addr);
  setUIntDigitalParam(addr, function, value, mask);
  if (function == digitalDirection_) {
    outValue = (value == 0) ? DIGITALIN : DIGITALOUT;
    if (digitalIOPortConfigurable_[addr]) {
        status = cbDConfigPort(boardNum_, digitalIOPort_[addr], outValue);
        direction = value ? 0xFFFF : 0;
        setUIntDigitalParam(0, digitalDirection_, direction, 0xFFFFFFFF);
    }
    else {
      for (i=0; i<numIOBits_[addr]; i++) {
        if ((mask & (1<<i)) != 0) {
          if (digitalIOBitConfigurable_[addr]) {
            status = cbDConfigBit(boardNum_, digitalIOPort_[addr], i, outValue);
          }
          else {
            // Cannot program direction.  Set open collector output to 0.
            status = cbDBitOut(boardNum_, digitalIOPort_[addr], i, 0);
          }
        }
      }
    }
  }

  else if (function == digitalOutput_) {
    getUIntDigitalParam(addr, digitalDirection_, &direction, 0xFFFFFFFF);
    if ((mask & direction) == digitalIOMask_[addr]) {
      // Use word I/O if all bits are outputs and we are writing all bits
      if (numIOBits_[addr] > 16) {
        status = cbDOut32(boardNum_, digitalIOPort_[addr], value & mask);
      } else {
        status = cbDOut(boardNum_, digitalIOPort_[addr], value & mask);
      }
    }
    else {
      // Use bit I/O if we are not writing all bits
      for (i=0, outMask=1; i<numIOBits_[addr]; i++, outMask = (outMask<<1)) {
        // Only write the value if the mask has this bit set and the direction for that bit is output (1)
        outValue = ((value & outMask) == 0) ? 0 : 1; 
        if ((mask & outMask & direction) != 0) {
          status = cbDBitOut(boardNum_, digitalIOPort_[addr], i, outValue);
        }
      }
    }
  }
  
  callParamCallbacks();
  if (status == 0) {
    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
             "%s:%s, port %s, function=%d, wrote outValue=0x%x, value=0x%x, mask=0x%x, direction=0x%x\n",
             driverName, functionName, this->portName, function, outValue, value, mask, direction);
  } else {
    asynPrint(pasynUser, ASYN_TRACE_ERROR, 
             "%s:%s, port %s, function=%d, ERROR writing outValue=0x%x, value=0x%x, mask=0x%x, direction=0x%x, status=%d\n",
             driverName, functionName, this->portName, function, outValue, value, mask, direction, status);
  }
  return (status==0) ? asynSuccess : asynError;
}

asynStatus MultiFunction::readFloat32Array(asynUser *pasynUser, epicsFloat32 *value, size_t nElements, size_t *nIn)
{
  int function = pasynUser->reason;
  int addr;
  int numPoints;
  epicsFloat32 *inPtr;
  static const char *functionName = "readFloat32Array";
  
  this->getAddress(pasynUser, &addr);
  
  if (addr >= numAnalogOut_) {
    asynPrint(pasynUser, ASYN_TRACE_ERROR,
      "%s:%s: ERROR: addr=%d max=%d\n",
      driverName, functionName, addr, numAnalogOut_-1);
    return asynError;
  }
  // Assume WaveGen function, WaveDig numPoints handled below
  getIntegerParam(waveGenNumPoints_, &numPoints);
  if (function == waveGenUserWF_)
    inPtr = waveGenUserBuffer_[addr];
  else if (function == waveGenIntWF_)
    inPtr = waveGenIntBuffer_[addr];
  else if (function == waveGenUserTimeWF_)
    inPtr = waveGenUserTimeBuffer_;
  else if (function == waveGenIntTimeWF_)
    inPtr = waveGenIntTimeBuffer_;
  else if (function == waveDigTimeWF_) {
    inPtr = waveDigTimeBuffer_;
    getIntegerParam(waveDigNumPoints_, &numPoints);
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

asynStatus MultiFunction::readFloat64Array(asynUser *pasynUser, epicsFloat64 *value, size_t nElements, size_t *nIn)
{
  int function = pasynUser->reason;
  int addr;
  int numPoints;
  static const char *functionName = "readFloat64Array";
  
  this->getAddress(pasynUser, &addr);
  
  if (function == waveDigVoltWF_) {
    if (addr >= numAnalogIn_) {
      asynPrint(pasynUser, ASYN_TRACE_ERROR,
        "%s:%s: ERROR: addr=%d max=%d\n",
        driverName, functionName, addr, numAnalogIn_-1);
      return asynError;
    } 
    *nIn = nElements;
    getIntegerParam(waveDigNumPoints_, &numPoints);
    if (*nIn > (size_t)numPoints) *nIn = numPoints;
    memcpy(value, waveDigBuffer_[addr], *nIn*sizeof(epicsFloat64));
    return asynSuccess; 
  }
  else {
    asynPrint(pasynUser, ASYN_TRACE_ERROR,
      "%s:%s: ERROR: unknown function=%d\n",
      driverName, functionName, function);
    return asynError;
  }
}

asynStatus MultiFunction::writeFloat32Array(asynUser *pasynUser, epicsFloat32 *value, size_t nElements)
{
  int function = pasynUser->reason;
  int addr;
  static const char *functionName = "writeFloat32Array";
  
  this->getAddress(pasynUser, &addr);
  
  if (function == waveGenUserWF_) {
    if ((addr >= numAnalogOut_) || (nElements > maxOutputPoints_)) {
      asynPrint(pasynUser, ASYN_TRACE_ERROR,
        "%s:%s: ERROR: addr=%d max=%d, nElements=%d max=%d\n",
        driverName, functionName, addr, numAnalogOut_-1, (int)nElements, (int)maxOutputPoints_);
      return asynError;
    } 
    memcpy(waveGenUserBuffer_[addr], value, nElements*sizeof(epicsFloat32)); 
  }
  else {
    asynPrint(pasynUser, ASYN_TRACE_ERROR,
      "%s:%s: ERROR: unknown function=%d\n",
      driverName, functionName, function);
    return asynError;
  }

  return asynSuccess;
}

asynStatus MultiFunction::readEnum(asynUser *pasynUser, char *strings[], int values[], int severities[], size_t nElements, size_t *nIn)
{
  int function = pasynUser->reason;
  int i;
  const enumStruct_t *pEnum;
  int numEnums;
  //static const char *functionName = "readEnum";

  if (function == analogInRange_) {
    pEnum    = pBoardEnums_->pInputRange;
    numEnums = pBoardEnums_->numInputRange;
  }
  else if (function == analogOutRange_) {
    pEnum    = pBoardEnums_->pOutputRange;
    numEnums = pBoardEnums_->numOutputRange;
  }
  else if (function == analogInType_) {
    pEnum    = pBoardEnums_->pInputType;
    numEnums = pBoardEnums_->numInputType;
  }
  else {
      *nIn = 0;
      return asynError;
  }
  for (i=0; ((i<numEnums) && (i<(int)nElements)); i++) {
    if (strings[i]) free(strings[i]);
    strings[i] = epicsStrDup(pEnum[i].enumString);
    values[i] = pEnum[i].enumValue;
    severities[i] = 0;
  }
  *nIn = i;
  return asynSuccess;   
}


void MultiFunction::pollerThread()
{
  /* This function runs in a separate thread.  It waits for the poll
   * time */
  static const char *functionName = "pollerThread";
  epicsUInt32 newValue, changedBits, prevInput[MAX_IO_PORTS];
  epicsUInt16 biVal16;
  epicsUInt32 biVal32;
  int i;
  int currentPoint;
  ULONG countVal;
  long aoCount, aoIndex, aiCount, aiIndex;
  short aoStatus, aiStatus;
  int status;

  while(1) { 
    lock();
    
    // Read the digital inputs
    for (i=0; i<numIOPorts_; i++) {
      if (numIOBits_[i] > 16) {
        status = cbDIn32(boardNum_, digitalIOPort_[i], &biVal32);
      } else {
        status = cbDIn(boardNum_, digitalIOPort_[i], &biVal16);
        biVal32 = biVal16;
      }
      if (status) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, 
                  "%s:%s: ERROR calling cbDIn, status=%d\n", 
                  driverName, functionName, status);
        goto error;
      }
      newValue = biVal32;
      changedBits = newValue ^ prevInput[i];
      if (forceCallback_ || (changedBits != 0)) {
        prevInput[i] = newValue;
        forceCallback_ = 0;
        setUIntDigitalParam(i, digitalInput_, newValue, 0xFFFFFFFF);
      }
    }
    
    // Read the counter inputs
    for (i=0; i<numCounters_; i++) {
      status = cbCIn32(boardNum_, firstCounter_ + i, &countVal);
      if (status) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, 
                  "%s:%s: ERROR calling cbCIn32, counter=%d, status=%d\n", 
                  driverName, functionName, i, status);
        goto error;
      }
      setIntegerParam(i, counterCounts_, countVal);
    }
    
    // Poll the status of the waveform generator output
    status = cbGetStatus(boardNum_, &aoStatus, &aoCount, &aoIndex, AOFUNCTION);
    if (status) {
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, 
                "%s:%s: ERROR calling cbGetStatus, status=%d\n", 
                driverName, functionName, status);
      goto error;
    }
    currentPoint = (aoIndex / numWaveGenChans_) + 1;
    setIntegerParam(waveGenCurrentPoint_, currentPoint);
    if (waveGenRunning_ && (aoStatus == 0)) {
      stopWaveGen();
    }
    
    // Poll the status of the waveform digitzer input
    status = cbGetStatus(boardNum_, &aiStatus, &aiCount, &aiIndex, AIFUNCTION);
    if (status) {
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, 
                "%s:%s: ERROR calling cbGetStatus, status=%d\n", 
                driverName, functionName, status);
      // On Windows after a network glitch cbGetStatus will return continually return DEADDEV
      // Need to stop and start the waveform digitizer if it was running
      if ((status == DEADDEV) && waveDigRunning_) {
        stopWaveDig();
        startWaveDig();
      }
      goto error;
    }
    currentPoint = (aiIndex / numWaveDigChans_) + 1;
    setIntegerParam(waveDigCurrentPoint_, currentPoint);
    if (waveDigRunning_ && (aiStatus == 0)) { 
      stopWaveDig();
    }
    
    for (i=0; i<MAX_SIGNALS; i++) {
      callParamCallbacks(i);
    }
error:
    unlock();
    epicsThreadSleep(pollTime_);
  }
}

/* Report  parameters */
void MultiFunction::report(FILE *fp, int details)
{
  int i;
  int counts;

  asynPortDriver::report(fp, details);
  fprintf(fp, "  Port: %s, board number=%d, board ID=%d, board type=%s\n", 
          this->portName, boardNum_, boardType_, boardName_);
  if (details >= 1) {
    fprintf(fp, "  analog inputs      = %d\n", numAnalogIn_);
    fprintf(fp, "  analog input bits  = %d\n", ADCResolution_);
    fprintf(fp, "  analog outputs     = %d\n", numAnalogOut_);
    fprintf(fp, "  analog output bits = %d\n", DACResolution_);
    fprintf(fp, "  temperature inputs = %d\n", numTempChans_);
    fprintf(fp, "  digital I/O ports  = %d\n", numIOPorts_);
    for (i=0; i<numIOPorts_; i++) {
      fprintf(fp, "  digital I/O port     %d\n", i);
      fprintf(fp, "    I/O port              = %d\n", digitalIOPort_[i]);
      fprintf(fp, "    I/O bits              = %d\n", numIOBits_[i]);
      fprintf(fp, "    I/O bit configurable  = %d\n", digitalIOBitConfigurable_[i]);
      fprintf(fp, "    I/O port configurable = %d\n", digitalIOPortConfigurable_[i]);
      fprintf(fp, "    I/O port mask         = 0x%x\n", digitalIOMask_[i]);
    }
    fprintf(fp, "  timers             = %d\n", numTimers_);
    if (numTimers_ > 0) {
      fprintf(fp, "  pulse generator\n");
      fprintf(fp, "    frequency range  = %f : %f\n", minPulseGenFrequency_, maxPulseGenFrequency_);
      fprintf(fp, "    delay range      = %f : %f\n", minPulseGenDelay_, maxPulseGenDelay_);
    }
    fprintf(fp, "  # counters         = %d", numCounters_);
    fprintf(fp, "  first counter      = %d", firstCounter_);
    fprintf(fp, "  counterCounts = ");
    for (i=0; i<numCounters_; i++) {
      getIntegerParam(i, counterCounts_, &counts); 
      fprintf(fp, " %d", counts);
    }
    fprintf(fp, "\n");
  }
}

/** Configuration command, called directly or from iocsh */
extern "C" int MultiFunctionConfig(const char *portName, int boardNum, 
                              int maxInputPoints, int maxOutputPoints)
{
  new MultiFunction(portName, boardNum, maxInputPoints, maxOutputPoints);
  return asynSuccess;
}


static const iocshArg configArg0 = { "Port name",      iocshArgString};
static const iocshArg configArg1 = { "Board number",      iocshArgInt};
static const iocshArg configArg2 = { "Max. input points", iocshArgInt};
static const iocshArg configArg3 = { "Max. output points",iocshArgInt};
static const iocshArg * const configArgs[] = {&configArg0,
                                              &configArg1,
                                              &configArg2,
                                              &configArg3};
static const iocshFuncDef configFuncDef = {"MultiFunctionConfig",4,configArgs};
static void configCallFunc(const iocshArgBuf *args)
{
  MultiFunctionConfig(args[0].sval, args[1].ival, args[2].ival, args[3].ival);
}

#ifdef _WIN32
int cbAddBoard(const char *boardName, const char *address)
{
    printf("cbAddBoard not implemented on Windows\n");
    return 0;
}
#endif

static const iocshArg addBoardArg0 = { "Model name",      iocshArgString};
static const iocshArg addBoardArg1 = { "Address",         iocshArgString};
static const iocshArg * const addBoardArgs[] = {&addBoardArg0,
                                                &addBoardArg1};
static const iocshFuncDef addBoardFuncDef = {"cbAddBoard",2,addBoardArgs};
static void addBoardCallFunc(const iocshArgBuf *args)
{
  cbAddBoard(args[0].sval, args[1].sval);
}

void drvMultiFunctionRegister(void)
{
  iocshRegister(&configFuncDef,configCallFunc);
  iocshRegister(&addBoardFuncDef,addBoardCallFunc);
}


extern "C" {
epicsExportRegistrar(drvMultiFunctionRegister);
}
