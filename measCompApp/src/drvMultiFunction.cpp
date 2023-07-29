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
#include <string.h>
#include <stdlib.h>

#include <iocsh.h>
#include <epicsThread.h>
#include <epicsString.h>

#include <asynPortDriver.h>

#define DRIVER_VERSION "4.2"

#ifdef _WIN32
  #include "cbw.h"
#else
  #include "uldaq.h"
#endif

// This needs to be global because we need to protect the UL library from simultaneous access from any driver
epicsMutex ULMutex;

// This function maps the Gain values from UL on Windows to the Range values in UL for Linux.
// We can't use the macros from Windows cbw.h because they conflict with UL for Linux.
// These definitions are taken from cbw.h, but added CBW_ prefix.
#define CBW_BIP60VOLTS       20             /* -60 to 60 Volts */
#define CBW_BIP30VOLTS       23
#define CBW_BIP20VOLTS       15             /* -20 to +20 Volts */
#define CBW_BIP15VOLTS       21             /* -15 to +15 Volts */
#define CBW_BIP10VOLTS       1              /* -10 to +10 Volts */
#define CBW_BIP5VOLTS        0              /* -5 to +5 Volts */
#define CBW_BIP4VOLTS        16             /* -4 to + 4 Volts */
#define CBW_BIP2PT5VOLTS     2              /* -2.5 to +2.5 Volts */
#define CBW_BIP2VOLTS        14             /* -2.0 to +2.0 Volts */
#define CBW_BIP1PT25VOLTS    3              /* -1.25 to +1.25 Volts */
#define CBW_BIP1VOLTS        4              /* -1 to +1 Volts */
#define CBW_BIPPT625VOLTS    5              /* -.625 to +.625 Volts */
#define CBW_BIPPT5VOLTS      6              /* -.5 to +.5 Volts */
#define CBW_BIPPT25VOLTS     12             /* -0.25 to +0.25 Volts */
#define CBW_BIPPT2VOLTS      13             /* -0.2 to +0.2 Volts */
#define CBW_BIPPT1VOLTS      7              /* -.1 to +.1 Volts */
#define CBW_BIPPT05VOLTS     8              /* -.05 to +.05 Volts */
#define CBW_BIPPT01VOLTS     9              /* -.01 to +.01 Volts */
#define CBW_BIPPT005VOLTS    10             /* -.005 to +.005 Volts */
#define CBW_BIP1PT67VOLTS    11             /* -1.67 to +1.67 Volts */
#define CBW_BIPPT312VOLTS    17             /* -0.312 to +0.312 Volts */
#define CBW_BIPPT156VOLTS    18             /* -0.156 to +0.156 Volts */
#define CBW_BIPPT125VOLTS    22             /* -0.125 to +0.125 Volts */
#define CBW_BIPPT078VOLTS    19             /* -0.078 to +0.078 Volts */


#define CBW_UNI10VOLTS       100            /* 0 to 10 Volts*/
#define CBW_UNI5VOLTS        101            /* 0 to 5 Volts */
#define CBW_UNI4VOLTS        114            /* 0 to 4 Volts */
#define CBW_UNI2PT5VOLTS     102            /* 0 to 2.5 Volts */
#define CBW_UNI2VOLTS        103            /* 0 to 2 Volts */
#define CBW_UNI1PT67VOLTS    109            /* 0 to 1.67 Volts */
#define CBW_UNI1PT25VOLTS    104            /* 0 to 1.25 Volts */
#define CBW_UNI1VOLTS        105            /* 0 to 1 Volt */
#define CBW_UNIPT5VOLTS      110            /* 0 to .5 Volt */
#define CBW_UNIPT25VOLTS     111            /* 0 to 0.25 Volt */
#define CBW_UNIPT2VOLTS      112            /* 0 to .2 Volt */
#define CBW_UNIPT1VOLTS      106            /* 0 to .1 Volt */
#define CBW_UNIPT05VOLTS     113            /* 0 to .05 Volt */
#define CBW_UNIPT02VOLTS     108            /* 0 to .02 Volt*/
#define CBW_UNIPT01VOLTS     107            /* 0 to .01 Volt*/

#define CBW_MA4TO20          200            /* 4 to 20 ma */
#define CBW_MA2TO10          201            /* 2 to 10 ma */
#define CBW_MA1TO5           202            /* 1 to 5 ma */
#define CBW_MAPT5TO2PT5      203            /* .5 to 2.5 ma */
#define CBW_MA0TO20          204            /* 0 to 20 ma */
#define CBW_BIPPT025AMPS     205            /* -0.025 to 0.025 ma */

#include <epicsExport.h>
#include <measCompDiscover.h>

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

// Analog input parameters
#define analogInValueString       "ANALOG_IN_VALUE"
#define analogInRangeString       "ANALOG_IN_RANGE"
#define analogInTypeString        "ANALOG_IN_TYPE"
#define analogInModeString        "ANALOG_IN_MODE"
#define analogInRateString        "ANALOG_IN_RATE"

// Voltage input parameters
#define voltageInValueString      "VOLTAGE_IN_VALUE"
#define voltageInRangeString      "VOLTAGE_IN_RANGE"

// Temperature parameters
#define temperatureInValueString  "TEMPERATURE_IN_VALUE"
#define thermocoupleTypeString    "THERMOCOUPLE_TYPE"
#define thermocoupleOpenDetectString "THERMOCOUPLE_OPEN_DETECT"
#define temperatureScaleString    "TEMPERATURE_SCALE"
#define temperatureFilterString   "TEMPERATURE_FILTER"
#define temperatureSensorString   "TEMPERATURE_SENSOR"
#define temperatureWiringString   "TEMPERATURE_WIRING"

// Waveform digitizer parameters - global
#define waveDigDwellString        "WAVEDIG_DWELL"
#define waveDigDwellActualString  "WAVEDIG_DWELL_ACTUAL"
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
#define waveDigAbsTimeWFString    "WAVEDIG_ABS_TIME_WF"
#define waveDigReadWFString       "WAVEDIG_READ_WF"
// Waveform digitizer parameters - per input
#define waveDigVoltWFString       "WAVEDIG_VOLT_WF"

// Analog output parameters
#define analogOutValueString      "ANALOG_OUT_VALUE"
#define analogOutRangeString      "ANALOG_OUT_RANGE"

// Waveform generator parameters - global
#define waveGenFreqString         "WAVEGEN_FREQ"
#define waveGenDwellString        "WAVEGEN_DWELL"
#define waveGenDwellActualString  "WAVEGEN_DWELL_ACTUAL"
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
#define MAX_ANALOG_IN      16
#define MAX_TEMPERATURE_IN 64
#define MAX_ANALOG_OUT     16
#define MAX_IO_PORTS        8
#define MAX_PULSE_GEN       4
#define MAX_SIGNALS        MAX_TEMPERATURE_IN

// For simplicity define a few constants on Linux to be the same as Windows cbw.h
// These need to be copied from cbw.h because uldaq.h and cbw.h cannot both be included due to some conflicting definitions
#ifdef linux
  #define AI_CHAN_TYPE_VOLTAGE  AI_VOLTAGE
  #define AI_CHAN_TYPE_TC       AI_TC
  #define TC_TYPE_J             TC_J
  #define DIFFERENTIAL          0
  #define SINGLE_ENDED          1
  /* Temperature scales */
  #define CELSIUS          0
  #define FAHRENHEIT       1
  #define KELVIN           2
  #define VOLTS            4     /* special scale for DAS-TC boards */
  #define NOSCALE          5
  /* Types of digital input ports */
  #define DIGITALOUT       1
  #define DIGITALIN        2
#endif

typedef enum {
  USB_231            = 297,
  USB_1208LS         = 122,
  USB_1208FS         = 130,
  USB_1208FS_PLUS    = 232,
  USB_1608G          = 308,
  USB_1608GX         = 309,
  USB_1608GX_2AO_OLD = 274,
  USB_1608GX_2AO     = 310,
  USB_1608HS_2AO     = 153,
  USB_1808           = 317, // Fix this when we know the correct value
  USB_1808X          = 318,
  USB_2408_2AO       = 254,
  USB_3101           = 154,
  USB_3102           = 155,
  USB_3103           = 156,
  USB_3104           = 157,
  USB_3105           = 158,
  USB_3106           = 159,
  USB_3110           = 162,
  USB_3112           = 163,
  USB_3114           = 164,
  USB_SSR08          = 134,
  USB_TEMP           = 141,
  USB_TEMP_AI        = 188,
  USB_TC32           = 305,
  ETH_TC32           = 306,
  E_1608             = 303,
  E_DIO24            = 311,
  E_TC               = 312,
  MAX_BOARD_TYPES
} boardType_t;

typedef struct {
  char *enumString;
  int  enumValue;
} enumStruct_t;

static const enumStruct_t inputRangeUSB_231[] = {
  {"+= 20V",   CBW_BIP20VOLTS},
  {"+= 10V",   CBW_BIP10VOLTS},
  {"+= 5V",    CBW_BIP5VOLTS},
  {"+= 4V",    CBW_BIP4VOLTS},
  {"+= 2.5V",  CBW_BIP2PT5VOLTS},
  {"+= 2V",    CBW_BIP2VOLTS},
  {"+= 1.25V", CBW_BIP1PT25VOLTS},
  {"+= 1V",    CBW_BIP1VOLTS}
};

static const enumStruct_t outputRangeUSB_231[] = {
  {"+= 10V", CBW_UNI5VOLTS}
};

static const enumStruct_t inputTypeUSB_231[] = {
  {"Volts", AI_CHAN_TYPE_VOLTAGE}
};

static const enumStruct_t inputRangeUSB_1208[] = {
  {"+= 20V",   CBW_BIP20VOLTS},
  {"+= 10V",   CBW_BIP10VOLTS},
  {"+= 5V",    CBW_BIP5VOLTS},
  {"+= 4V",    CBW_BIP4VOLTS},
  {"+= 2.5V",  CBW_BIP2PT5VOLTS},
  {"+= 2V",    CBW_BIP2VOLTS},
  {"+= 1.25V", CBW_BIP1PT25VOLTS},
  {"+= 1V",    CBW_BIP1VOLTS}
};

static const enumStruct_t outputRangeUSB_1208[] = {
  {"+= 5V", CBW_UNI5VOLTS}
};

static const enumStruct_t outputRangeUSB_1208FS[] = {
  {"+= 4V", CBW_UNI4VOLTS}
};

static const enumStruct_t inputTypeUSB_1208[] = {
    {"Volts", AI_CHAN_TYPE_VOLTAGE}
};

static const enumStruct_t inputRangeUSB_1608G[] = {
  {"+= 10V", CBW_BIP10VOLTS},
  {"+= 5V",  CBW_BIP5VOLTS},
  {"+= 2V",  CBW_BIP2VOLTS},
  {"+= 1V",  CBW_BIP1VOLTS}
};

static const enumStruct_t outputRangeUSB_1608G[] = {
  {"+= 10V", CBW_BIP10VOLTS}
};

static const enumStruct_t inputTypeUSB_1608G[] = {
  {"Volts", AI_CHAN_TYPE_VOLTAGE}
};

static const enumStruct_t inputRangeUSB_1808[] = {
  {"+= 10V",    CBW_BIP10VOLTS},
  {"+= 5V",     CBW_BIP5VOLTS},
  {"0-10V",     CBW_UNI10VOLTS},
  {"0-5V",      CBW_UNI5VOLTS}
};

static const enumStruct_t outputRangeUSB_1808[] = {
  {"+= 10V", CBW_BIP10VOLTS}
};

static const enumStruct_t inputTypeUSB_1808[] = {
  {"Volts", AI_CHAN_TYPE_VOLTAGE}
};

static const enumStruct_t inputRangeUSB_2408[] = {
  {"+= 10V",    CBW_BIP10VOLTS},
  {"+= 5V",     CBW_BIP5VOLTS},
  {"+= 2.5V",   CBW_BIP2PT5VOLTS},
  {"+= 1.25V",  CBW_BIP1PT25VOLTS},
  {"+= 0.625V", CBW_BIPPT625VOLTS},
  {"+= 0.312V", CBW_BIPPT312VOLTS},
  {"+= 0.156V", CBW_BIPPT156VOLTS},
  {"+= 0.078V", CBW_BIPPT078VOLTS}
};

static const enumStruct_t outputRangeUSB_2408[] = {
  {"+= 10V", CBW_BIP10VOLTS}
};

static const enumStruct_t inputTypeUSB_2408[] = {
  {"Volts",   AI_CHAN_TYPE_VOLTAGE},
  {"TC deg.", AI_CHAN_TYPE_TC}
};

static const enumStruct_t inputRangeUSB_3101[] = {
  {"N.A.", 0}
};

static const enumStruct_t outputRangeUSB_3101[] = {
  {"0-10V",  CBW_UNI10VOLTS},
  {"+= 10V", CBW_BIP10VOLTS}
};

static const enumStruct_t inputTypeUSB_3101[] = {
  {"N.A.", 0}
};

static const enumStruct_t inputRangeUSB_SSR08[] = {
  {"N.A.",  0}
};

static const enumStruct_t outputRangeUSB_SSR08[] = {
  {"N.A.", 0}
};

static const enumStruct_t inputTypeUSB_SSR08[] = {
  {"N.A.", 0}
};

static const enumStruct_t inputRangeUSB_TEMP[] = {
  {"N.A.", 0},
};

static const enumStruct_t outputRangeUSB_TEMP[] = {
  {"N.A.", 0}
};

static const enumStruct_t inputTypeUSB_TEMP[] = {
  {"N.A.", 0}
};

static const enumStruct_t inputRangeUSB_TEMP_AI[] = {
  {"+= 10V",   CBW_BIP10VOLTS},
  {"+= 5V",    CBW_BIP5VOLTS},
  {"+= 2.5V",  CBW_BIP2PT5VOLTS},
  {"+= 1.25V", CBW_BIP1PT25VOLTS}
};

static const enumStruct_t outputRangeUSB_TEMP_AI[] = {
  {"N.A.", 0}
};

static const enumStruct_t inputTypeUSB_TEMP_AI[] = {
  {"N.A.", 0}
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

static const enumStruct_t inputRangeE_1608[] = {
  {"+= 10V", CBW_BIP10VOLTS},
  {"+= 5V",  CBW_BIP5VOLTS},
  {"+= 2V",  CBW_BIP2VOLTS},
  {"+= 1V",  CBW_BIP1VOLTS}
};

static const enumStruct_t outputRangeE_1608[] = {
  {"+= 10V", CBW_BIP10VOLTS}
};

static const enumStruct_t inputTypeE_1608[] = {
  {"Volts", AI_CHAN_TYPE_VOLTAGE}
};

static const enumStruct_t inputRangeE_DIO24[] = {
  {"N.A.", 0}
};

static const enumStruct_t outputRangeE_DIO24[] = {
  {"N.A.", 0}
};

static const enumStruct_t inputTypeE_DIO24[] = {
  {"N.A.", 0}
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

#ifdef _WIN32
  // The sensor type cannot be configured in UL for Windows so widthwe use these default enum values
  static const enumStruct_t temperatureSensorUSB_TEMP[] = {
    {"RTD",           0},
    {"Thermistor",    1},
    {"Thermocouple",  2},
    {"Semiconductor", 3},
    {"Disabled",      4}
  };
  // The wiring cannot be configured in UL for Windows so we use these default enum values
  static const enumStruct_t temperatureWiringUSB_TEMP[] = {
    {"2 wire 1 sensor",   0},
    {"2 wire 2 sensors",  1},
    {"3 wire",            2},
    {"4 wire",            3}
  };
#else
  // These enum values are for UL for Linux.  
  static const enumStruct_t temperatureSensorUSB_TEMP[] = {
    {"RTD",           AI_RTD},
    {"Thermistor",    AI_THERMISTOR},
    {"Thermocouple",  AI_TC},
    {"Semiconductor", AI_SEMICONDUCTOR},
    {"Disabled",      AI_DISABLED}
  };
  static const enumStruct_t temperatureWiringUSB_TEMP[] = {
    {"2 wire 1 sensor",   SCT_2_WIRE_1},
    {"2 wire 2 sensors",  SCT_2_WIRE_2},
    {"3 wire",            SCT_3_WIRE},
    {"4 wire",            SCT_4_WIRE}
  };
#endif

typedef struct {
  boardType_t boardFamily;
  const enumStruct_t *pInputRange;
  int numInputRange;
  const enumStruct_t *pOutputRange;
  int numOutputRange;
  const enumStruct_t *pInputType;
  int numInputType;
} boardEnums_t;

static const boardEnums_t allBoardEnums[] = {
  {USB_231,        inputRangeUSB_231,     sizeof(inputRangeUSB_231)/sizeof(enumStruct_t),
                   outputRangeUSB_231,    sizeof(outputRangeUSB_231)/sizeof(enumStruct_t),
                   inputTypeUSB_231,      sizeof(inputTypeUSB_231)/sizeof(enumStruct_t)},

  {USB_1208LS,     inputRangeUSB_1208,    sizeof(inputRangeUSB_1208)/sizeof(enumStruct_t),
                   outputRangeUSB_1208,   sizeof(outputRangeUSB_1208)/sizeof(enumStruct_t),
                   inputTypeUSB_1208,     sizeof(inputTypeUSB_1208)/sizeof(enumStruct_t)},

  {USB_1208FS,     inputRangeUSB_1208,    sizeof(inputRangeUSB_1208)/sizeof(enumStruct_t),
                   outputRangeUSB_1208FS, sizeof(outputRangeUSB_1208FS)/sizeof(enumStruct_t),
                   inputTypeUSB_1208,     sizeof(inputTypeUSB_1208)/sizeof(enumStruct_t)},

  {USB_1608G,      inputRangeUSB_1608G,   sizeof(inputRangeUSB_1608G)/sizeof(enumStruct_t),
                   outputRangeUSB_1608G,  sizeof(outputRangeUSB_1608G)/sizeof(enumStruct_t),
                   inputTypeUSB_1608G,    sizeof(inputTypeUSB_1608G)/sizeof(enumStruct_t)},

  {USB_1808,       inputRangeUSB_1808,    sizeof(inputRangeUSB_1808)/sizeof(enumStruct_t),
                   outputRangeUSB_1808,   sizeof(outputRangeUSB_1808)/sizeof(enumStruct_t),
                   inputTypeUSB_1808,     sizeof(inputTypeUSB_1808)/sizeof(enumStruct_t)},

  {USB_3101,       inputRangeUSB_3101,    sizeof(inputRangeUSB_3101)/sizeof(enumStruct_t),
                   outputRangeUSB_3101,   sizeof(outputRangeUSB_3101)/sizeof(enumStruct_t),
                   inputTypeUSB_3101,     sizeof(inputTypeUSB_3101)/sizeof(enumStruct_t)},

  {USB_2408_2AO,   inputRangeUSB_2408,    sizeof(inputRangeUSB_2408)/sizeof(enumStruct_t),
                   outputRangeUSB_2408,   sizeof(outputRangeUSB_2408)/sizeof(enumStruct_t),
                   inputTypeUSB_2408,     sizeof(inputTypeUSB_2408)/sizeof(enumStruct_t)},

  {USB_SSR08,      inputRangeUSB_SSR08,   sizeof(inputRangeUSB_SSR08)/sizeof(enumStruct_t),
                   outputRangeUSB_SSR08,  sizeof(outputRangeUSB_SSR08)/sizeof(enumStruct_t),
                   inputTypeUSB_SSR08,    sizeof(inputTypeUSB_SSR08)/sizeof(enumStruct_t)},

  {USB_TEMP,       inputRangeUSB_TEMP,    sizeof(inputRangeUSB_TEMP)/sizeof(enumStruct_t),
                   outputRangeUSB_TEMP,   sizeof(outputRangeUSB_TEMP)/sizeof(enumStruct_t),
                   inputTypeUSB_TEMP,     sizeof(inputTypeUSB_TEMP)/sizeof(enumStruct_t)},

  {USB_TEMP_AI,    inputRangeUSB_TEMP_AI, sizeof(inputRangeUSB_TEMP_AI)/sizeof(enumStruct_t),
                   outputRangeUSB_TEMP_AI,sizeof(outputRangeUSB_TEMP_AI)/sizeof(enumStruct_t),
                   inputTypeUSB_TEMP_AI,  sizeof(inputTypeUSB_TEMP_AI)/sizeof(enumStruct_t)},

  {USB_TC32,       inputRangeTC32,        sizeof(inputRangeTC32)/sizeof(enumStruct_t),
                   outputRangeTC32,       sizeof(outputRangeTC32)/sizeof(enumStruct_t),
                   inputTypeTC32,         sizeof(inputTypeTC32)/sizeof(enumStruct_t)},

  {E_1608,         inputRangeE_1608,      sizeof(inputRangeE_1608)/sizeof(enumStruct_t),
                   outputRangeE_1608,     sizeof(outputRangeE_1608)/sizeof(enumStruct_t),
                   inputTypeE_1608,       sizeof(inputTypeE_1608)/sizeof(enumStruct_t)},

  {E_DIO24,        inputRangeE_DIO24,     sizeof(inputRangeE_DIO24)/sizeof(enumStruct_t),
                   outputRangeE_DIO24,    sizeof(outputRangeE_DIO24)/sizeof(enumStruct_t),
                   inputTypeE_DIO24,      sizeof(inputTypeE_DIO24)/sizeof(enumStruct_t)},

  {E_TC,           inputRangeE_TC,        sizeof(inputRangeE_TC)/sizeof(enumStruct_t),
                   outputRangeE_TC,       sizeof(outputRangeE_TC)/sizeof(enumStruct_t),
                   inputTypeE_TC,         sizeof(inputTypeE_TC)/sizeof(enumStruct_t)},
};

static int maxBoardFamilies = (int) (sizeof(allBoardEnums) / sizeof(boardEnums_t));
#define ROUND(x) ((x) >= 0. ? (int)x+0.5 : (int)(x-0.5))
#define MAX_BOARDNAME_LEN 256
#define MAX_LIBRARY_MESSAGE_LEN 256
#define PI 3.14159265

/** This is the class definition for the MultiFunction class
  */
class MultiFunction : public asynPortDriver {
public:
  MultiFunction(const char *portName, const char *uniqueID, int maxInputPoints, int maxOutputPoints);

  /* These are the methods that we override from asynPortDriver */
  virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
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

  // Analog input parameters
  int analogInValue_;
  int analogInRange_;
  int analogInType_;
  int analogInMode_;
  int analogInRate_;

  // Voltage input parameters
  int voltageInValue_;
  int voltageInRange_;

  // Temperature parameters
  int temperatureInValue_;
  int thermocoupleType_;
  int thermocoupleOpenDetect_;
  int temperatureScale_;
  int temperatureFilter_;
  int temperatureSensor_;
  int temperatureWiring_;

  // Waveform digitizer parameters - global
  int waveDigDwell_;
  int waveDigDwellActual_;
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
  int waveDigAbsTimeWF_;
  int waveDigReadWF_;
  // Waveform digitizer parameters - per input
  int waveDigVoltWF_;

  // Analog output parameters
  int analogOutValue_;
  int analogOutRange_;

  // Waveform generator parameters - global
  int waveGenFreq_;
  int waveGenDwell_;
  int waveGenDwellActual_;
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
  #ifdef _WIN32
    int boardNum_;
  #else
    DaqDeviceHandle daqDeviceHandle_;
  #endif
  DaqDeviceDescriptor daqDeviceDescriptor_;
  char boardName_[MAX_BOARDNAME_LEN];
  int boardType_;
  int boardFamily_;
  const boardEnums_t *pBoardEnums_;
  int numAnalogIn_;
  int analogInTypeConfigurable_;
  int analogInDataRateConfigurable_;
  int analogOutRangeConfigurable_;
  int numAnalogOut_;
  #ifdef linux
    AiInputMode aiInputMode_;
    TriggerType triggerType_;
    int aiScanTrigCount_;  // Not currently used
    int aoScanTrigCount_;  // Not currently used
  #endif
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
  int digitalIOPortWriteOnly_[MAX_IO_PORTS];
  int digitalIOPortReadOnly_[MAX_IO_PORTS];
  int numIOBits_[MAX_IO_PORTS];
  epicsUInt32 digitalIOMask_[MAX_IO_PORTS];
  double minPulseGenFrequency_;
  double maxPulseGenFrequency_;
  double minPulseGenDelay_;
  double maxPulseGenDelay_;
  double pollTime_;
  int forceCallback_[MAX_IO_PORTS];
  size_t maxInputPoints_;
  size_t maxOutputPoints_;
  epicsFloat64 *waveDigBuffer_[MAX_ANALOG_IN];
  epicsFloat32 *waveDigTimeBuffer_;
  epicsFloat64 *waveDigAbsTimeBuffer_;
  epicsFloat32 *waveGenIntBuffer_[MAX_ANALOG_OUT];
  epicsFloat32 *waveGenUserBuffer_[MAX_ANALOG_OUT];
  epicsFloat32 *waveGenUserTimeBuffer_;
  epicsFloat32 *waveGenIntTimeBuffer_;
  epicsFloat64 *pInBuffer_;
  #ifdef _WIN32
    epicsUInt16  *waveGenOutBuffer_;
  #else
    epicsFloat64 *waveGenOutBuffer_;
  #endif
  int numWaveGenChans_;
  int numWaveDigChans_;
  int pulseGenRunning_[MAX_PULSE_GEN];
  int waveGenRunning_;
  int waveDigRunning_;
  int startPulseGenerator(int timerNum);
  int stopPulseGenerator(int timerNum);
  int startWaveGen();
  int stopWaveGen();
  int computeWaveGenTimes();
  int startWaveDig();
  int stopWaveDig();
  int readWaveDig();
  int computeWaveDigTimes();
  int defineWaveform(int channel);
  int setOpenThermocoupleDetect(int addr, int value);
  int reportError(int err, const char *functionName, const char *message);
  #ifdef linux
  int mapRange(int Gain, Range *range);
  int mapTriggerType(int cbwTriggerType, TriggerType *triggerType);
  #endif
};

static void pollerThreadC(void * pPvt)
{
    MultiFunction *pMultiFunction = (MultiFunction *)pPvt;
    pMultiFunction->pollerThread();
}

MultiFunction::MultiFunction(const char *portName, const char *uniqueID, int maxInputPoints, int maxOutputPoints)
  : asynPortDriver(portName, MAX_SIGNALS,
      asynUInt32DigitalMask | asynInt32Mask   | asynInt32ArrayMask   | asynFloat32ArrayMask | 
                              asynFloat64Mask | asynFloat64ArrayMask | asynOctetMask        | asynEnumMask | asynDrvUserMask,
      asynUInt32DigitalMask | asynInt32Mask   | asynInt32ArrayMask   | asynFloat32ArrayMask | 
                              asynFloat64Mask | asynFloat64ArrayMask | asynOctetMask        | asynEnumMask,
      ASYN_MULTIDEVICE | ASYN_CANBLOCK, 1, /* ASYN_CANBLOCK=1, ASYN_MULTIDEVICE=1, autoConnect=1 */
      0, 0),  /* Default priority and stack size */
    maxInputPoints_(maxInputPoints),
    maxOutputPoints_(maxOutputPoints),
    numWaveGenChans_(1),
    numWaveDigChans_(1),
    waveGenRunning_(0),
    waveDigRunning_(0)
{
  int i, j;
  int status;
  long long handle;
  static const char *functionName = "MultiFunction";

  for (i=0; i<MAX_PULSE_GEN; i++) pulseGenRunning_[i]=0;
  for (i=0; i<MAX_IO_PORTS; i++) forceCallback_[i] = 1;

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

  // Analog input parameters
  createParam(analogInValueString,             asynParamInt32, &analogInValue_);
  createParam(analogInRangeString,             asynParamInt32, &analogInRange_);
  createParam(analogInTypeString,              asynParamInt32, &analogInType_);
  createParam(analogInModeString,              asynParamInt32, &analogInMode_);
  createParam(analogInRateString,              asynParamInt32, &analogInRate_);

  // Voltage input parameters
  createParam(voltageInValueString,          asynParamFloat64, &voltageInValue_);
  createParam(voltageInRangeString,            asynParamInt32, &voltageInRange_);

  // Temperature parameters
  createParam(temperatureInValueString,      asynParamFloat64, &temperatureInValue_);
  createParam(thermocoupleTypeString,          asynParamInt32, &thermocoupleType_);
  createParam(thermocoupleOpenDetectString,    asynParamInt32, &thermocoupleOpenDetect_);
  createParam(temperatureScaleString,          asynParamInt32, &temperatureScale_);
  createParam(temperatureFilterString,         asynParamInt32, &temperatureFilter_);
  createParam(temperatureSensorString,         asynParamInt32, &temperatureSensor_);
  createParam(temperatureWiringString,         asynParamInt32, &temperatureWiring_);

  // Waveform digitizer parameters - global
  createParam(waveDigDwellString,            asynParamFloat64, &waveDigDwell_);
  createParam(waveDigDwellActualString,      asynParamFloat64, &waveDigDwellActual_);
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
  createParam(waveDigAbsTimeWFString,   asynParamFloat64Array, &waveDigAbsTimeWF_);
  createParam(waveDigReadWFString,             asynParamInt32, &waveDigReadWF_);
  // Waveform digitizer parameters - per input
  createParam(waveDigVoltWFString,      asynParamFloat32Array, &waveDigVoltWF_);

  // Analog output parameters
  createParam(analogOutValueString,            asynParamInt32, &analogOutValue_);
  createParam(analogOutRangeString,            asynParamInt32, &analogOutRange_);

  // Waveform generator parameters - global
  createParam(waveGenFreqString,             asynParamFloat64, &waveGenFreq_);
  createParam(waveGenDwellString,            asynParamFloat64, &waveGenDwell_);
  createParam(waveGenDwellActualString,      asynParamFloat64, &waveGenDwellActual_);
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

  // Map very similar boards for simplicity
  boardFamily_ = boardType_;
  switch (boardType_) {
    case USB_1208LS:
    case USB_1208FS_PLUS:
      boardFamily_ = USB_1208LS;
      break;
    case USB_1608G:
    case USB_1608GX:
    case USB_1608GX_2AO:
    case USB_1608GX_2AO_OLD:
    case USB_1608HS_2AO:
      boardFamily_ = USB_1608G;
      break;
    case USB_1808:
    case USB_1808X:
      boardFamily_ = USB_1808;
      break;
    case USB_3101:
    case USB_3102:
    case USB_3103:
    case USB_3104:
    case USB_3105:
    case USB_3106:
    case USB_3110:
    case USB_3112:
    case USB_3114:
      boardFamily_ = USB_3101;
      break;
    case USB_TC32:
    case ETH_TC32:
      boardFamily_ = USB_TC32;
      break;
    default:
      break;
  }

  char uniqueIDStr[256];
  char firmwareVersion[256];
  char ULVersion[256];
  ULMutex.lock();
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
  
  #ifdef _WIN32
    int inMask, outMask;
    cbGetConfig(BOARDINFO, boardNum_, 0, BINUMADCHANS,    &numAnalogIn_);
    cbGetConfig(BOARDINFO, boardNum_, 0, BINUMDACHANS,    &numAnalogOut_);
    cbGetConfig(BOARDINFO, boardNum_, 0, BIADRES,         &ADCResolution_);
    cbGetConfig(BOARDINFO, boardNum_, 0, BIDACRES,        &DACResolution_);
    cbGetConfig(BOARDINFO, boardNum_, 0, BIDINUMDEVS,     &numIOPorts_);
    cbGetConfig(BOARDINFO, boardNum_, 0, BINUMTEMPCHANS,  &numTempChans_);
  #else
    long long infoValue;
    status = ulAIGetInfo(daqDeviceHandle_, AI_INFO_NUM_CHANS_BY_TYPE, AI_VOLTAGE, &infoValue);
    if (status)
      numAnalogIn_ = 0;
    else
      numAnalogIn_ = infoValue;
    status = ulAOGetInfo(daqDeviceHandle_, AO_INFO_NUM_CHANS, 0, &infoValue);
    if (status)
      numAnalogOut_ = 0;
    else
      numAnalogOut_ = infoValue;
    status = ulAIGetInfo(daqDeviceHandle_, AI_INFO_RESOLUTION, 0, &infoValue);
    ADCResolution_ = infoValue;
    status = ulAOGetInfo(daqDeviceHandle_, AO_INFO_RESOLUTION, 0, &infoValue);
    DACResolution_ = infoValue;
    status = ulDIOGetInfo(daqDeviceHandle_, DIO_INFO_NUM_PORTS, 0, &infoValue);
    numIOPorts_ = infoValue;
    status = ulAIGetInfo(daqDeviceHandle_, AI_INFO_NUM_CHANS_BY_TYPE, AI_TC, &infoValue);
    numTempChans_ = infoValue;
  #endif
  if (numIOPorts_ > MAX_IO_PORTS) numIOPorts_ = MAX_IO_PORTS;
  for (i=0; i<numIOPorts_; i++) {
    digitalIOPortConfigurable_[i] = 0;
    #ifdef _WIN32
      cbGetConfig(DIGITALINFO, boardNum_, i, DIDEVTYPE, &digitalIOPort_[i]);
      cbGetConfig(DIGITALINFO, boardNum_, i, DIINMASK,  &inMask);
      cbGetConfig(DIGITALINFO, boardNum_, i, DIOUTMASK, &outMask);
      digitalIOPortReadOnly_[i]    = ((inMask != 0) && (outMask == 0));
      digitalIOPortWriteOnly_[i]   = ((inMask == 0) && (outMask != 0));
      digitalIOBitConfigurable_[i] = ((inMask & outMask) == 0);
      cbGetConfig(DIGITALINFO, boardNum_, i, DINUMBITS, &numIOBits_[i]);
    #else
      status = ulDIOGetInfo(daqDeviceHandle_, DIO_INFO_PORT_TYPE, i, &infoValue);
      digitalIOPort_[i] = infoValue;
      status = ulDIOGetInfo(daqDeviceHandle_, DIO_INFO_PORT_IO_TYPE, i, &infoValue);
      digitalIOPortReadOnly_[i]    = (infoValue == DPIOT_IN);
      digitalIOPortWriteOnly_[i]   = (infoValue == DPIOT_OUT);
      digitalIOBitConfigurable_[i] = (infoValue == DPIOT_BITIO);
      status = ulDIOGetInfo(daqDeviceHandle_, DIO_INFO_NUM_BITS, i, &infoValue);
      numIOBits_[i] = infoValue;
    #endif
    digitalIOMask_[i] = 0;
    for (j=0; j<numIOBits_[i]; j++) {
      digitalIOMask_[i] |= (1 << j);
    }
  }
  ULMutex.unlock();
  // Assume only voltage input is supported
  analogInTypeConfigurable_ = 0;
  // Assume analog in data rate not configurable
  analogInDataRateConfigurable_ = 0;
  // Assume analog output range is not configurable
  analogOutRangeConfigurable_ = 0;
  switch (boardFamily_) {
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
    case USB_1208LS:
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
    case USB_1608G:
      numTimers_    = 1;
      numCounters_  = 2;
      firstCounter_ = 0;
      minPulseGenFrequency_ = 0.0149;
      maxPulseGenFrequency_ = 32e6;
      minPulseGenDelay_ = 0.;
      maxPulseGenDelay_ = 67.11;
      break;
    case USB_1608HS_2AO:
      numTimers_    = 1;
      numCounters_  = 1;
      firstCounter_ = 0;
      minPulseGenFrequency_ = 0.0149;
      maxPulseGenFrequency_ = 32e6;
      minPulseGenDelay_ = 0.;
      maxPulseGenDelay_ = 67.11;
      break;
    case USB_1808:
      numTimers_    = 2;
      numCounters_  = 4;
      firstCounter_ = 0;
      minPulseGenFrequency_ = 0.0149;
      maxPulseGenFrequency_ = 50e6;
      minPulseGenDelay_ = 0.;
      maxPulseGenDelay_ = 42.94;
      break;
    case USB_2408_2AO:
      numTimers_    = 0;
      numCounters_  = 2;
      firstCounter_ = 0;
      analogInTypeConfigurable_  = 1; // Supports voltage and thermocouple
      analogInDataRateConfigurable_ = 1; // Can configure analog input data rate
      break;
    case USB_3101:
      numTimers_    = 0;
      numCounters_  = 1;
      firstCounter_ = 0;
      analogOutRangeConfigurable_ = 1; // Can select 0-10V or +-10V
      break;
    case USB_SSR08:
      numTimers_    = 0;
      numCounters_  = 0;
      for (i=0; i<2; i++) {
        // Digital I/O port 0 is outputs 1 - 4      
        // Digital I/O port 1 is outputs 5 - 8      
        setUIntDigitalParam(i, digitalDirection_, 0xFFFFFFFF, digitalIOMask_[i]);
      }
      break;
    case USB_TEMP:
      numTimers_    = 0;
      numCounters_  = 1;
      firstCounter_ = 0;
      for (i=0; i<8; i++) {
        setIntegerParam(i, analogInType_, AI_CHAN_TYPE_TC);
      }
      break;
    case USB_TEMP_AI:
      numTimers_    = 0;
      numCounters_  = 1;
      firstCounter_ = 0;
      for (i=0; i<4; i++) {
        setIntegerParam(i, analogInType_, AI_CHAN_TYPE_TC);
      }
      for (i=4; i<8; i++) {
        setIntegerParam(i, analogInType_, AI_CHAN_TYPE_VOLTAGE);
      }
      break;
    case USB_TC32:
      numTimers_    = 0;
      numCounters_  = 0;
      for (i=0; i<MAX_TEMPERATURE_IN; i++) {
        setIntegerParam(i, analogInType_, AI_CHAN_TYPE_TC);
      }
      // On Linux the TC-32 reports that numIOPorts_ = 4, but it is really only 2
      // This should be fixed in uldaq.
      #ifdef linux
        numIOPorts_ = 2;
      #endif
      // Digital I/O port 0 is input only
      setUIntDigitalParam(0, digitalDirection_, 0, 0xFFFFFFFF);
      // Digital I/O port 1 is output and temperature alarms
      setUIntDigitalParam(1, digitalDirection_, 0xFFFFFFFF, 0xFFFFFFFF);
      break;
    case E_1608:
      numTimers_    = 0;
      numCounters_  = 1;
      firstCounter_ = 0;
      break;
    case E_DIO24:
      numTimers_    = 0;
      numCounters_  = 1;
      firstCounter_ = 0;
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
        "%s::%s error, unknown board type=%d, board family=%d\n",
        driverName, functionName, boardType_, boardFamily_);
      break;
  }

  for (i=0, pBoardEnums_=0; i<maxBoardFamilies; i++) {
    if (allBoardEnums[i].boardFamily == boardFamily_) {
       pBoardEnums_ = &allBoardEnums[i];
       break;
    }
  }
  if (pBoardEnums_ == 0) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
      "%s::%s error, unknown board family=%d\n",
      driverName, functionName, boardFamily_);
  }

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
  waveDigAbsTimeBuffer_  = (epicsFloat64 *) calloc(maxInputPoints_,  sizeof(epicsFloat64));
  pInBuffer_ = (epicsFloat64 *) calloc(maxInputPoints  * numAnalogIn_, sizeof(epicsFloat64));
  #ifdef _WIN32
    waveGenOutBuffer_ = (epicsUInt16 *) calloc(maxOutputPoints * numAnalogOut_, sizeof(epicsUInt16));
  #else
    waveGenOutBuffer_ = (epicsFloat64 *) calloc(maxOutputPoints * numAnalogOut_, sizeof(epicsFloat64));
  #endif

  // Set values of some parameters that need to be set because init record order is not predictable
  // or because the corresponding records are PINI=NO.
  setIntegerParam(waveGenUserNumPoints_, 1);
  setIntegerParam(waveGenIntNumPoints_, 1);
  setIntegerParam(waveDigNumPoints_, 1);
  setIntegerParam(pulseGenRun_, 0);
  setIntegerParam(waveDigRun_, 0);
  setIntegerParam(waveGenRun_, 0);
  for (i=0; i<numTempChans_; i++) {
    setIntegerParam(i, thermocoupleType_, TC_TYPE_J);
  }
  // Set the analog output range to the first supported value for this model
  for (i=0; i<MAX_ANALOG_OUT; i++) {
    setIntegerParam(i, analogOutRange_, pBoardEnums_->pOutputRange[0].enumValue);
  }

  /* Start the thread to poll counters and digital inputs and do callbacks to
   * device support */
  epicsThreadCreate("MultiFunctionPoller",
                    epicsThreadPriorityLow,
                    epicsThreadGetStackSize(epicsThreadStackMedium),
                    (EPICSTHREADFUNC)pollerThreadC,
                    this);
}

int  MultiFunction::reportError(int err, const char *functionName, const char *message)
{
  char libraryMessage[MAX_LIBRARY_MESSAGE_LEN];
  ULMutex.lock();
  switch (err) {
    case 0: 
      asynPrint(pasynUserSelf, ASYN_TRACEIO_DRIVER,
        "%s::%s Info: %s\n", driverName, functionName, message);
      break;
    case -1: 
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
        "%s::%s Error: %s\n", driverName, functionName, message);
      break;
    default:
      #ifdef _WIN32
        cbGetErrMsg(err, libraryMessage);
      #else
        ulGetErrMsg((UlError)err, libraryMessage);
      #endif
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
        "%s::%s Error: %s, err=%d %s\n", driverName, functionName, message, err, libraryMessage);
  }
  ULMutex.unlock();
  return err;
}

#ifdef linux
int MultiFunction::mapRange(int Gain, Range *range)
{
    static const char *functionName = "mapRange";
    // Converts cbw Gain to uldaq Range
    switch (Gain) {
      case CBW_BIP60VOLTS:    *range = BIP60VOLTS; break;
      case CBW_BIP30VOLTS:    *range = BIP30VOLTS; break;
      case CBW_BIP20VOLTS:    *range = BIP20VOLTS; break;
      case CBW_BIP15VOLTS:    *range = BIP15VOLTS; break;
      case CBW_BIP10VOLTS:    *range = BIP10VOLTS; break;
      case CBW_BIP5VOLTS:     *range = BIP5VOLTS; break;
      case CBW_BIP4VOLTS:     *range = BIP4VOLTS; break;
      case CBW_BIP2PT5VOLTS:  *range = BIP2PT5VOLTS; break;
      case CBW_BIP2VOLTS:     *range = BIP2VOLTS; break;
      case CBW_BIP1PT25VOLTS: *range = BIP1PT25VOLTS; break;
      case CBW_BIP1VOLTS:     *range = BIP1VOLTS; break;
      case CBW_BIPPT625VOLTS: *range = BIPPT625VOLTS; break;
      case CBW_BIPPT5VOLTS:   *range = BIPPT5VOLTS; break;
      case CBW_BIPPT25VOLTS:  *range = BIPPT25VOLTS; break;
      case CBW_BIPPT2VOLTS:   *range = BIPPT2VOLTS; break;
      case CBW_BIPPT1VOLTS:   *range = BIPPT1VOLTS; break;
      case CBW_BIPPT05VOLTS:  *range = BIPPT05VOLTS; break;
      case CBW_BIPPT01VOLTS:  *range = BIPPT01VOLTS; break;
      case CBW_BIPPT005VOLTS: *range = BIPPT005VOLTS; break;
//      case CBW_BIP1PT67VOLTS: *range = BIP1PT67VOLTS; break;
      case CBW_BIPPT312VOLTS: *range = BIPPT312VOLTS; break;
      case CBW_BIPPT156VOLTS: *range = BIPPT156VOLTS; break;
      case CBW_BIPPT125VOLTS: *range = BIPPT125VOLTS; break;
      case CBW_BIPPT078VOLTS: *range = BIPPT078VOLTS; break;

      case CBW_UNI10VOLTS:    *range = UNI10VOLTS; break;
      case CBW_UNI5VOLTS:     *range = UNI5VOLTS; break;
      case CBW_UNI4VOLTS:     *range = UNI4VOLTS; break;
      case CBW_UNI2PT5VOLTS:  *range = UNI2PT5VOLTS; break;
      case CBW_UNI2VOLTS:     *range = UNI2VOLTS; break;
//      case CBW_UNI1PT67VOLTS: *range = UNI1PT67VOLTS; break;
      case CBW_UNI1PT25VOLTS: *range = UNI1PT25VOLTS; break;
      case CBW_UNI1VOLTS:     *range = UNI1VOLTS; break;
      case CBW_UNIPT5VOLTS:   *range = UNIPT5VOLTS; break;
      case CBW_UNIPT25VOLTS:  *range = UNIPT25VOLTS; break;
      case CBW_UNIPT2VOLTS:   *range = UNIPT2VOLTS; break;
      case CBW_UNIPT1VOLTS:   *range = UNIPT1VOLTS; break;
      case CBW_UNIPT05VOLTS:  *range = UNIPT05VOLTS; break;
//      case CBW_UNIPT02VOLTS:  *range = UNIPT02VOLTS; break;
      case CBW_UNIPT01VOLTS:  *range = UNIPT01VOLTS; break;

      case CBW_MA0TO20:       *range = MA0TO20; break;
      default:
          asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
            "%s::%s unsupported gain=%d\n", driverName, functionName, Gain);
          return -1;
    }
    return 0;
}

// This function maps the trigger types from UL on Windows to the values in UL for Linux.
// We can't use the macros from Windows cbw.h because they conflict with UL for Linux.
// These definitions are taken from cbw.h, but added CBW_ prefix.

#define CBW_TRIGABOVE           0
#define CBW_TRIGBELOW           1
#define CBW_GATE_NEG_HYS        2
#define CBW_GATE_POS_HYS        3
#define CBW_GATE_ABOVE          4
#define CBW_GATE_BELOW          5
#define CBW_GATE_IN_WINDOW      6
#define CBW_GATE_OUT_WINDOW     7
#define CBW_GATE_HIGH           8
#define CBW_GATE_LOW            9
#define CBW_TRIG_HIGH           10
#define CBW_TRIG_LOW            11
#define CBW_TRIG_POS_EDGE       12
#define CBW_TRIG_NEG_EDGE       13
#define CBW_TRIG_RISING         14
#define CBW_TRIG_FALLING        15
#define CBW_TRIG_PATTERN_EQ     16
#define CBW_TRIG_PATTERN_NE     17
#define CBW_TRIG_PATTERN_ABOVE  18
#define CBW_TRIG_PATTERN_BELOW  19

int MultiFunction::mapTriggerType(int cbwTriggerType, TriggerType *triggerType)
{
    static const char *functionName = "mapTriggerType";
    // Converts cbw trigger type to uldaq trigger type;
    switch (cbwTriggerType) {
      case CBW_TRIG_POS_EDGE:       *triggerType = TRIG_POS_EDGE; break;
      case CBW_TRIG_NEG_EDGE:       *triggerType = TRIG_NEG_EDGE; break;
      case CBW_TRIG_HIGH:           *triggerType = TRIG_HIGH; break;
      case CBW_TRIG_LOW:            *triggerType = TRIG_LOW; break;
      case CBW_GATE_HIGH:           *triggerType = GATE_HIGH; break;
      case CBW_GATE_LOW:            *triggerType = GATE_LOW; break;
      case CBW_TRIG_RISING:         *triggerType = TRIG_RISING; break;
      case CBW_TRIG_FALLING:        *triggerType = TRIG_FALLING; break;
      case CBW_TRIGABOVE:           *triggerType = TRIG_ABOVE; break;
      case CBW_TRIGBELOW:           *triggerType = TRIG_BELOW; break;
      case CBW_GATE_ABOVE:          *triggerType = GATE_ABOVE; break;
      case CBW_GATE_BELOW:          *triggerType = GATE_BELOW; break;
      case CBW_GATE_IN_WINDOW:      *triggerType = GATE_IN_WINDOW; break;
      case CBW_GATE_OUT_WINDOW:     *triggerType = GATE_OUT_WINDOW; break;
      case CBW_TRIG_PATTERN_EQ:     *triggerType = TRIG_PATTERN_EQ; break;
      case CBW_TRIG_PATTERN_NE:     *triggerType = TRIG_PATTERN_NE; break;
      case CBW_TRIG_PATTERN_ABOVE:  *triggerType = TRIG_PATTERN_ABOVE; break;
      case CBW_TRIG_PATTERN_BELOW:  *triggerType = TRIG_PATTERN_BELOW; break;
      default:
          asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
          "%s::%s unsupported cbwTriggerType=%d\n", driverName, functionName, cbwTriggerType);
          *triggerType = TRIG_NONE;
          return -1;
    }
    return 0;
}

#endif

int MultiFunction::startPulseGenerator(int timerNum)
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
  if (frequency < minPulseGenFrequency_) frequency = minPulseGenFrequency_;
  if (frequency > maxPulseGenFrequency_) frequency = maxPulseGenFrequency_;
  period = 1. / frequency;
  if (dutyCycle <= 0.) dutyCycle = .0001;
  if (dutyCycle >= 1.) dutyCycle = .9999;
  if (delay < minPulseGenDelay_) delay = minPulseGenDelay_;
  if (delay > maxPulseGenDelay_) delay = maxPulseGenDelay_;

  ULMutex.lock();
  #ifdef _WIN32
    status = cbPulseOutStart(boardNum_, timerNum, &frequency, &dutyCycle, count, &delay, idleState, 0);
  #else
    TmrIdleState idle = (idleState == 0) ? TMRIS_LOW : TMRIS_HIGH;
    status = ulTmrPulseOutStart(daqDeviceHandle_, timerNum, &frequency, &dutyCycle, count, &delay, idle, PO_DEFAULT);
  #endif
  ULMutex.unlock();
  reportError(status, functionName, "Calling PulseOutStart");
  if (status) return status;
  // We may not have gotten the frequency, dutyCycle, and delay we asked for, set the actual values
  // in the parameter library
  pulseGenRunning_[timerNum] = 1;
  period = 1. / frequency;
  asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
    "%s:%s: started pulse generator %d actual frequency=%f, actual period=%f, actual duty cycle=%f, actual delay=%f\n",
    driverName, functionName, timerNum, frequency, period, dutyCycle, delay);
  setDoubleParam(timerNum, pulseGenPeriod_, period);
  setDoubleParam(timerNum, pulseGenDutyCycle_, dutyCycle);
  setDoubleParam(timerNum, pulseGenDelay_, delay);
  return 0;
}

int MultiFunction::stopPulseGenerator(int timerNum)
{
  pulseGenRunning_[timerNum] = 0;
  int err;
  ULMutex.lock();
  #ifdef _WIN32
    err = cbPulseOutStop(boardNum_, timerNum);
  #else
    err = ulTmrPulseOutStop(daqDeviceHandle_, timerNum);
  #endif
  ULMutex.unlock();
  return err;
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
  int waveType;
  int extTrigger, extClock, continuous, retrigger;
  int options;
  int i, j, k;
  double offset, scale;
  double userOffset, userAmplitude;
  double dwell;
  epicsFloat32* inPtr[MAX_ANALOG_OUT];
  #ifdef _WIN32
    epicsUInt16 *outPtr;
  #else
    epicsFloat64 *outPtr;
  #endif
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
 
  // Copy data from float32 array to outputMemHandel, converting from volts to D/A units
  // Pre-defined waveforms have been fully defined at this point
  // User-defined waveforms need to have the offset and scale applied
  for (i=0; i<numWaveGenChans_; i++) {
    k = firstChan + i;
    outPtr = &(waveGenOutBuffer_[i]);
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
  ULMutex.lock();
  #ifdef _WIN32
    long pointsPerSecond = (long)((1. / dwell) + 0.5);
    options                  = BACKGROUND;
    if (extTrigger) options |= EXTTRIGGER;
    if (extClock)   options |= EXTCLOCK;
    if (continuous) options |= CONTINUOUS;
    if (retrigger)  options |= RETRIGMODE;
    status = cbAOutScan(boardNum_, firstChan, lastChan, numWaveGenChans_*numPoints, &pointsPerSecond, BIP10VOLTS,
                        waveGenOutBuffer_, options);
    // Convert back from pointsPerSecond to dwell, since value might have changed
    dwell = (1. / pointsPerSecond);
  #else
    options                  = SO_DEFAULTIO;
    if (extTrigger) options |= SO_EXTTRIGGER;
    if (extClock)   options |= SO_EXTCLOCK;
    if (continuous) options |= SO_CONTINUOUS;
    if (retrigger)  options |= SO_RETRIGGER;
    double rate = 1./dwell;
    status = ulAOutScan(daqDeviceHandle_, firstChan, lastChan, BIP10VOLTS, numPoints, &rate, (ScanOption) options, AOUTSCAN_FF_NOSCALEDATA, waveGenOutBuffer_);
    // Convert back from rate to dwell, since value might have changed
    dwell = 1./rate;
  #endif
  ULMutex.unlock();
  reportError(status, functionName, "Calling AOutScan");

  if (status) return status;

  waveGenRunning_ = 1;
  setIntegerParam(waveGenRun_, 1);
  asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
    "%s:%s: called cbAOutScan, firstChan=%d, lastChan=%d, numPoints*numWaveGenChans_=%d, dwell=%f, options=0x%x\n",
    driverName, functionName, firstChan, lastChan,  numWaveGenChans_*numPoints, dwell, options);

  setDoubleParam(waveGenDwellActual_, dwell);
  setDoubleParam(waveGenTotalTime_, dwell*numPoints);
  return status;
}

int MultiFunction::stopWaveGen()
{
  int err;
  waveGenRunning_ = 0;
  setIntegerParam(waveGenRun_, 0);
  ULMutex.lock();
  #ifdef _WIN32
    err = cbStopBackground(boardNum_, AOFUNCTION);
  #else
    err = ulAOutScanStop(daqDeviceHandle_);
  #endif
  ULMutex.unlock();
  return err;
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
  double dwell;
  bool invalidScanRate=false;
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

  lastChan = firstChan + numChans - 1;
  setIntegerParam(waveDigCurrentPoint_, 0);

  // Construct the gain array
  for (i=0; i<numChans; i++) {
    chan = firstChan + i;
    chanArray[i] = chan;
    getIntegerParam(chan, analogInRange_, &range);
    gainArray[i] = range;
  }
  ULMutex.lock();
  #ifdef _WIN32
    status = cbALoadQueue(boardNum_, chanArray, gainArray, numChans);
  #else
    AiQueueElement *queue = new AiQueueElement[numChans];
    for (int i=0; i<numChans; i++) {
        queue[i].channel = chanArray[i];
        queue[i].inputMode = aiInputMode_ == DIFFERENTIAL ? AI_DIFFERENTIAL : AI_SINGLE_ENDED;
        mapRange(gainArray[i], &queue[i].range);
    }
    status = ulAInLoadQueue(daqDeviceHandle_, queue, numChans);
    delete[] queue;
  #endif
  ULMutex.unlock();
  reportError(status, functionName, "Calling ALoadQueue");
  if (status) return status;

  ULMutex.lock();
  #ifdef _WIN32
    long pointsPerSecond = (long)((1. / dwell) + 0.5);
    options                  = BACKGROUND;
    options                 |= SCALEDATA;
    if (extTrigger) options |= EXTTRIGGER;
    if (extClock)   options |= EXTCLOCK;
    if (continuous) options |= CONTINUOUS;
    if (retrigger)  options |= RETRIGMODE;
    if (burstMode)  options |= BURSTMODE;
    status = cbAInScan(boardNum_, firstChan, lastChan, numWaveDigChans_*numPoints, &pointsPerSecond, BIP10VOLTS,
                       pInBuffer_, options);
    if (status == BADRATE) invalidScanRate = true;
    // Convert back from pointsPerSecond to dwell, since value might have changed
    dwell = (1. / pointsPerSecond);
  #else
    double rate = 1./dwell;
    options                  = SO_DEFAULTIO;
    if (extTrigger) options |= SO_EXTTRIGGER;
    if (extClock)   options |= SO_EXTCLOCK;
    if (continuous) options |= SO_CONTINUOUS;
    if (retrigger)  options |= SO_RETRIGGER;
    if (burstMode)  options |= SO_BURSTMODE;
    // This is equivalent to OPTIONS |= SCALEDATA on Windows
    AInScanFlag flags = AINSCAN_FF_DEFAULT;
    status = ulAInScan(daqDeviceHandle_, firstChan, lastChan, aiInputMode_, BIP10VOLTS, numPoints, &rate, (ScanOption) options, flags, pInBuffer_);
    if (status == ERR_BAD_RATE) invalidScanRate = true;
     // Convert back from rate to dwell, since value might have changed
    dwell = (1. / rate);
  #endif
  ULMutex.unlock();

  if (invalidScanRate) {
    setDoubleParam(waveDigDwellActual_, -9999);
  } else {
    setDoubleParam(waveDigDwellActual_, dwell);
  }

  reportError(status, functionName, "Calling AInScan");
  if (status) return status;

  waveDigRunning_ = 1;
  setIntegerParam(waveDigRun_, 1);
  asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
    "%s:%s: called cbAInScan, firstChan=%d, lastChan=%d, numPoints=%d, dwell=%f, options=0x%x\n",
    driverName, functionName, firstChan, lastChan, numPoints, dwell, options);

  setDoubleParam(waveDigTotalTime_, dwell*numPoints);
  return 0;
}

int MultiFunction::stopWaveDig()
{
  int autoRestart;
  int status;
  static const char *functionName = "stopWaveDig";

  waveDigRunning_ = 0;
  setIntegerParam(waveDigRun_, 0);
  readWaveDig();
  getIntegerParam(waveDigAutoRestart_, &autoRestart);
  ULMutex.lock();
  #ifdef _WIN32
    status = cbStopBackground(boardNum_, AIFUNCTION);
  #else
    status = ulAInScanStop(daqDeviceHandle_);
  #endif
  ULMutex.unlock();
  reportError(status, functionName, "Stopping AIn scan");
  if (autoRestart)
    status |= startWaveDig();
  return status;
}

int MultiFunction::readWaveDig()
{
  int firstChan, lastChan;
  int currentPoint;
  int i;
  static const char *functionName = "readWaveDig";

  getIntegerParam(waveDigFirstChan_,    &firstChan);
  lastChan = firstChan + numWaveDigChans_ - 1;
  getIntegerParam(waveDigCurrentPoint_, &currentPoint);

  for (i=firstChan; i<=lastChan; i++) {
    asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
      "%s:%s:, doing callbacks on input %d, first value=%f\n",
      driverName, functionName, i, waveDigBuffer_[i][0]);
    doCallbacksFloat64Array(waveDigBuffer_[i], currentPoint, waveDigVoltWF_, i);
  }
  doCallbacksFloat64Array(waveDigAbsTimeBuffer_, currentPoint, waveDigAbsTimeWF_, 0);
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
  int range;
  int status=0;
  static const char *functionName = "writeInt32";

  this->getAddress(pasynUser, &addr);
  setIntegerParam(addr, function, value);

  bool isThermocouple = true;
  if (analogInTypeConfigurable_) {
    int ival;
    getIntegerParam(addr, analogInType_, &ival);
    if (ival != AI_CHAN_TYPE_TC) isThermocouple = false;
  }

  ULMutex.lock();
  // Analog input functions
  if (function == analogInType_ && analogInTypeConfigurable_) {
    #ifdef _WIN32
      status = cbSetConfig(BOARDINFO, boardNum_, addr, BIADCHANTYPE, value);
      // It seems to be necessary to reprogram the thermocouple type when switching from volts to TC
    #else
      long long configValue = (value == AI_CHAN_TYPE_VOLTAGE) ? AI_VOLTAGE : AI_TC;
      status = ulAISetConfig(daqDeviceHandle_, AI_CFG_CHAN_TYPE, addr, configValue);
    #endif
    reportError(status, functionName, "Setting analog input type");
    // It seems to be necessary to reprogram the thermocouple type when switching from volts to TC
    if (value == AI_CHAN_TYPE_TC) {
      int ival;
      // Set the TC type.  Note that the enums for thermocouple types are the same on Windows and Linux
      getIntegerParam(addr, thermocoupleType_, &ival);
      #ifdef _WIN32
        status = cbSetConfig(BOARDINFO, boardNum_, addr, BICHANTCTYPE, ival);
      #else
        status = ulAISetConfig(daqDeviceHandle_, AI_CFG_CHAN_TC_TYPE, addr, ival);
      #endif
      reportError(status, functionName, "Set thermocouple type");
      // Set open thermocouple detection
      getIntegerParam(addr, thermocoupleOpenDetect_, &ival);
      setOpenThermocoupleDetect(addr, ival);
    }
  }

  // Analog output functions
  if ((function == analogOutRange_) && analogOutRangeConfigurable_) {
    #ifdef _WIN32
      status = cbSetConfig(BOARDINFO, boardNum_, addr, BIDACRANGE, value);
    #else
      // No function to immediately set it on Linux, this value is read from parameter library when calling ulAOut
    #endif
    reportError(status, functionName, "Setting analog out range");
  }

  else if (function == analogInMode_) {
    #ifdef _WIN32
      status = cbAInputMode(boardNum_, value);
    #else
      aiInputMode_ = (value == DIFFERENTIAL) ? AI_DIFFERENTIAL : AI_SINGLE_ENDED;
    #endif
    reportError(status, functionName, "Setting analog input mode");
  }

  else if ((function == analogInRate_) && analogInDataRateConfigurable_) {
    #ifdef _WIN32 
        status = cbSetConfig(BOARDINFO, boardNum_, addr, BIADDATARATE, value);
    #else
        status = ulAISetConfigDbl(daqDeviceHandle_, AI_CFG_CHAN_DATA_RATE, addr, value);
    #endif
    reportError(status, functionName, "Setting data rate");
  }

  else if ((function == thermocoupleType_) && isThermocouple) {
    // NOTE:
    // This sleep is a hack to get it working on the TC-32.  Without it the call to cbSetConfig()
    // will often hang if more than 6 channels are being configured.
    // This makes no sense.  The problem cannot be reproduced in the testTC32.c test application
    if (boardFamily_ == USB_TC32) epicsThreadSleep(0.01);
    #ifdef _WIN32
      status = cbSetConfig(BOARDINFO, boardNum_, addr, BICHANTCTYPE, value);
    #else
      // The enums for thermocouple types are the same on Windows and Linux
      status = ulAISetConfig(daqDeviceHandle_, AI_CFG_CHAN_TC_TYPE, addr, value);
    #endif
    reportError(status, functionName, "Setting thermocouple type");
  }

  else if ((function == thermocoupleOpenDetect_) && isThermocouple) {
    setOpenThermocoupleDetect(addr, value);
  }

  else if (function == temperatureSensor_) {
    #ifdef _WIN32 
      // Sensor cannot be configured on UL for Windows
      status = NOERRORS;
    #else
      status = ulAISetConfig(daqDeviceHandle_, AI_CFG_CHAN_TYPE, addr, value);
    #endif
    reportError(status, functionName, "Setting temperature sensor");
  }

  else if (function == temperatureWiring_) {
    #ifdef _WIN32
      // Wiring cannot be configured on UL for Windows
      status = NOERRORS;
    #else
      status = ulAISetConfig(daqDeviceHandle_, AI_CFG_CHAN_SENSOR_CONNECTION_TYPE, addr, value);
    #endif
    reportError(status, functionName, "Setting temperature wiring");
  }

  // Pulse generator functions
  else if (function == pulseGenRun_) {
    // Allow starting a run even if it thinks its running,
    // since there is no way to know when it got done if Count!=0
    if (value) {
      status = startPulseGenerator(addr);
    }
    else if (!value && pulseGenRunning_[addr]) {
      status = stopPulseGenerator(addr);
    }
  }
  else if ((function == pulseGenCount_) ||
           (function == pulseGenIdleState_)) {
    if (pulseGenRunning_[addr]) {
      status = stopPulseGenerator(addr);
      status |= startPulseGenerator(addr);
    }
  }

  // Counter functions
  else if (function == counterReset_) {
    #ifdef _WIN32
      // LOADREG0=0, LOADREG1=1, so we use addr
      status = cbCLoad32(boardNum_, addr, 0);
    #else
      status = ulCLoad(daqDeviceHandle_, addr, CRT_LOAD, 0);
    #endif
    reportError(status, functionName, "Resetting counter");
  }

  // Trigger functions
  else if (function == triggerMode_) {
    #ifdef _WIN32
      status = cbSetTrigger(boardNum_, value, 0, 0);
    #else
      // In uldaq there are separate calls for ulDaqInSetTrigger, ulDaqOutSetTrigger, etc.
      // We just cache the information here and then call those functions when starting the appropriate scan
      status = mapTriggerType(value, &triggerType_);
    #endif
    reportError(status, functionName, "Setting trigger mode");
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
    if (value > (int)maxInputPoints_) {
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "%s::%s error WaveDigNumPoints=%d must be less than MaxInputPoints=%d\n",
                driverName, functionName, value, (int)maxInputPoints_);
      setIntegerParam(waveDigNumPoints_, maxInputPoints_);
    }
    computeWaveDigTimes();
  }

  else if (function == waveDigTriggerCount_) {
    #ifdef _WIN32
      status = cbSetConfig(BOARDINFO, boardNum_, 0, BIADTRIGCOUNT, value);
    #else
      aiScanTrigCount_ = value;
    #endif
    reportError(status, functionName, "Setting waveDig trigger count");
  }

  // Analog output functions
  else if (function == analogOutValue_) {
    if (waveGenRunning_) {
      reportError(-1, functionName, "cannot write analog outputs while waveform generator is running.");
      ULMutex.unlock();
      return asynError;
    }
    status = getIntegerParam(addr, analogOutRange_, &range);

    #ifdef _WIN32
      status = cbAOut(boardNum_, addr, range, value);
    #else
      Range ulRange;
      mapRange(range, &ulRange);
      status = ulAOut(daqDeviceHandle_, addr, ulRange, AOUT_FF_NOSCALEDATA, (double) value);
    #endif
    reportError(status, functionName, "calling AOut");
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
    #ifdef _WIN32
      status = cbSetConfig(BOARDINFO, boardNum_, 0, BIDACTRIGCOUNT, value);
    #else
      aoScanTrigCount_ = value;
    #endif
    reportError(status, functionName, "Setting waveGen trigger count");
  }
  ULMutex.unlock();

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

int MultiFunction::setOpenThermocoupleDetect(int addr, int value)
{
  int status=0;
  static const char *functionName = "setOpenThermocoupleDetect";

  if ((boardFamily_ != USB_TEMP) && (boardFamily_ != USB_TEMP_AI)) {
    ULMutex.lock();
    #ifdef _WIN32 
      status = cbSetConfig(BOARDINFO, boardNum_, addr, BIDETECTOPENTC, value);
    #else
      OtdMode mode = value ? OTD_ENABLED : OTD_DISABLED;
      // TC-32 and E-TC can only change open thermocouple detect for the entire device, not per-channel
      if (boardFamily_ == USB_TC32 || boardFamily_ == E_TC ) {
        int dev = 0;
        if (addr >= 32) dev = 1;
        status = ulAISetConfig(daqDeviceHandle_, AI_CFG_OTD_MODE, dev, mode);
      } else { 
        status = ulAISetConfig(daqDeviceHandle_, AI_CFG_CHAN_OTD_MODE, addr, mode);
      }
    #endif
    ULMutex.unlock();
    reportError(status, functionName, "Setting thermocouple open detect mode");
  }
  return status;
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
      (function == pulseGenDutyCycle_) ||
      (function == pulseGenDelay_)) {
    if (pulseGenRunning_[addr]) {
      status = stopPulseGenerator(addr);
      status |= startPulseGenerator(addr);
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
  int filter=0;
  int range;
  static const char *functionName = "readFloat64";

  this->getAddress(pasynUser, &addr);

  // Temperature input function
  if (function == temperatureInValue_) {
    if (waveDigRunning_) {
      int currentPoint;
      getIntegerParam(waveDigCurrentPoint_, &currentPoint);
      if (currentPoint > 0) {
        *value = waveDigBuffer_[addr][currentPoint-1];
      }
    }
    else {
      getIntegerParam(addr, analogInType_, &type);
      getIntegerParam(addr, temperatureScale_, &scale);
      getIntegerParam(addr, temperatureFilter_, &filter);
      if (type != AI_CHAN_TYPE_TC) return asynSuccess;
      ULMutex.lock();
      #ifdef _WIN32
        float fVal;
        status = cbTIn(boardNum_, addr, scale, &fVal, filter);
        if (status == OPENCONNECTION) {
          // This is an "expected" error if the thermocouple is broken or disconnected
          // Don't print error message, just set temp to -9999.
          fVal = -9999.;
          status = 0;
          *value = (double) fVal;
        }
        *value = (double) fVal;
      #else
        TempScale tempScale;
        // cbTin has a filter option but ulTin does not?
        TInFlag flags = TIN_FF_DEFAULT;
        switch (scale) {
            case CELSIUS:     tempScale = TS_CELSIUS; break;
            case FAHRENHEIT:  tempScale = TS_FAHRENHEIT; break;
            case KELVIN:      tempScale = TS_KELVIN; break;
            case VOLTS:       tempScale = TS_VOLTS; break;
            case NOSCALE:     tempScale = TS_NOSCALE; break;
            default:
                asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                  "%s::%s unsupported Scale=%d\n", driverName, functionName, scale);
                tempScale = TS_CELSIUS;
        }
        status = ulTIn(daqDeviceHandle_, addr, tempScale, flags, value);
        if (status == ERR_OPEN_CONNECTION) {
          // This is an "expected" error if the thermocouple is broken or disconnected
          // Don't print error message, just set temp to -9999.
          status = 0;
          *value = -9999.;
        }
      #endif
      ULMutex.unlock();
    }
    setDoubleParam(addr, temperatureInValue_, *value);
    reportError(status, functionName, "Calling TIn");
  }
  else if (function == voltageInValue_) {
    getIntegerParam(addr, voltageInRange_, &range);
    ULMutex.lock();
    #ifdef _WIN32
      float fVal;
      status = cbVIn(boardNum_, addr, range, &fVal, 0);
      *value = fVal;
    #else
      double data;
      Range ulRange;
      mapRange(range, &ulRange);
      int chan = addr;
      if (boardFamily_ == USB_TEMP_AI) {
        // On Linux the address needs to be 4 larger
        chan = addr + 4;
      }
      status = ulAIn(daqDeviceHandle_, chan, aiInputMode_, ulRange, AIN_FF_DEFAULT, &data);
      *value = (float) data;
    #endif
    ULMutex.unlock();
    reportError(status, functionName, "Calling AIn");
    setDoubleParam(addr, voltageInValue_, *value);
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
  epicsUInt32 direction=0;
  static const char *functionName = "writeUInt32Digital";

  this->getAddress(pasynUser, &addr);
  setUIntDigitalParam(addr, function, value, mask);
  ULMutex.lock();
  if (function == digitalDirection_) {
    if (digitalIOPortConfigurable_[addr]) {
      #ifdef _WIN32
        int dir = (value == 0) ? DIGITALIN : DIGITALOUT;
        status = cbDConfigPort(boardNum_, digitalIOPort_[addr], dir);
      #else
        DigitalDirection dir = (value == 0) ? DD_INPUT : DD_OUTPUT;
        status = ulDConfigPort(daqDeviceHandle_, (DigitalPortType)digitalIOPort_[addr], dir);
      #endif
      reportError(status, functionName, "Calling ConfigPort");
      direction = value ? 0xFFFF : 0;
      setUIntDigitalParam(0, digitalDirection_, direction, 0xFFFFFFFF);
    }
    else {
      for (i=0; i<numIOBits_[addr]; i++) {
        if ((mask & (1<<i)) != 0) {
          if (digitalIOBitConfigurable_[addr]) {
            #ifdef _WIN32
              int dir = (value == 0) ? DIGITALIN : DIGITALOUT;
              status = cbDConfigBit(boardNum_, digitalIOPort_[addr], i, dir);
            #else
             DigitalDirection dir = (value == 0) ? DD_INPUT : DD_OUTPUT;
             status = ulDConfigBit(daqDeviceHandle_, (DigitalPortType)digitalIOPort_[addr], i, dir);
            #endif
            reportError(status, functionName, "Calling ConfigBit");
          }
          else {
            // Cannot program direction.  Set open collector output to 0.
            #ifdef _WIN32
              status = cbDBitOut(boardNum_, digitalIOPort_[addr], i, 0);
            #else
              status = ulDBitOut(daqDeviceHandle_, (DigitalPortType)digitalIOPort_[addr], i, 0);
            #endif
            reportError(status, functionName, "Calling BitOut");
          }
        }
      }
    }
  }

  else if (function == digitalOutput_) {
    getUIntDigitalParam(addr, digitalDirection_, &direction, 0xFFFFFFFF);
    if ((mask & direction) == digitalIOMask_[addr]) {
      // Use word I/O if all bits are outputs and we are writing all bits
      #ifdef _WIN32
        if (numIOBits_[addr] > 16) {
          status = cbDOut32(boardNum_, digitalIOPort_[addr], value & mask);
        } else {
          status = cbDOut(boardNum_, digitalIOPort_[addr], value & mask);
        }
      #else
        status = ulDOut(daqDeviceHandle_, (DigitalPortType)digitalIOPort_[addr], value & mask);
      #endif
      reportError(status, functionName, "Calling DOut");
    }
    else {
      // Use bit I/O if we are not writing all bits
      epicsUInt32 outMask, outValue;
      for (i=0, outMask=1; i<numIOBits_[addr]; i++, outMask = (outMask<<1)) {
        // Only write the value if the mask has this bit set and the direction for that bit is output (1)
        outValue = ((value & outMask) == 0) ? 0 : 1;
        if ((mask & outMask & direction) != 0) {
          #ifdef _WIN32
            status = cbDBitOut(boardNum_, digitalIOPort_[addr], i, outValue);
          #else
            status = ulDBitOut(daqDeviceHandle_, (DigitalPortType)digitalIOPort_[addr], i, outValue);
          #endif
          reportError(status, functionName, "Calling DBitOut");
        }
      }
    }
  }
  ULMutex.unlock();

  callParamCallbacks();
  if (status == 0) {
    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
             "%s:%s, port %s, function=%d, wrote value=0x%x, mask=0x%x, direction=0x%x\n",
             driverName, functionName, this->portName, function, value, mask, direction);
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
  epicsFloat64 *inPtr;
  static const char *functionName = "readFloat64Array";

  this->getAddress(pasynUser, &addr);

  if (function == waveDigVoltWF_) {
    if (addr >= numAnalogIn_) {
      asynPrint(pasynUser, ASYN_TRACE_ERROR,
        "%s:%s: ERROR: addr=%d max=%d\n",
        driverName, functionName, addr, numAnalogIn_-1);
      return asynError;
    }
    inPtr = waveDigBuffer_[addr];
  }
  else if (function == waveDigAbsTimeWF_) {
    inPtr = waveDigAbsTimeBuffer_;
  }
  else {
    asynPrint(pasynUser, ASYN_TRACE_ERROR,
      "%s:%s: ERROR: unknown function=%d\n",
      driverName, functionName, function);
    return asynError;
  }
  *nIn = nElements;
  getIntegerParam(waveDigNumPoints_, &numPoints);
  if (*nIn > (size_t)numPoints) *nIn = numPoints;
  memcpy(value, inPtr, *nIn*sizeof(epicsFloat64));
  return asynSuccess;
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
  else if (function == voltageInRange_) {
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
  else if (function == temperatureSensor_) {
    pEnum    = temperatureSensorUSB_TEMP;
    numEnums = sizeof(temperatureSensorUSB_TEMP)/sizeof(enumStruct_t);
  }
  else if (function == temperatureWiring_) {
    pEnum    = temperatureWiringUSB_TEMP;
    numEnums = sizeof(temperatureWiringUSB_TEMP)/sizeof(enumStruct_t);
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
  epicsUInt32 newValue, changedBits, prevInput[MAX_IO_PORTS]={0};
  int i;
  int currentPoint;
  epicsUInt32 countVal;
  long aoCount, aoIndex, aiCount, aiIndex;
  short aoStatus, aiStatus;
  epicsTime startTime=epicsTime::getCurrent(), endTime, currentTime;
  int lastPoint;
  int status=0, prevStatus=0;

  while(1) {
    lock();
    ULMutex.lock();
    endTime = epicsTime::getCurrent();
    setDoubleParam(pollTimeMS_, (endTime-startTime)*1000.);
    startTime = epicsTime::getCurrent();

    // Read the digital inputs
    for (i=0; i<numIOPorts_; i++) {
      if (digitalIOPortWriteOnly_[i]) continue;
      #ifdef _WIN32
        epicsUInt16 biVal16;
        if (numIOBits_[i] > 16) {
          status = cbDIn32(boardNum_, digitalIOPort_[i], &newValue);
        } else {
          status = cbDIn(boardNum_, digitalIOPort_[i], &biVal16);
          newValue = biVal16;
        }
      #else
        unsigned long long data;
        status = ulDIn(daqDeviceHandle_, (DigitalPortType)digitalIOPort_[i], &data);
        newValue = (epicsUInt32) data;
      #endif
      if (status) {
        if (!prevStatus) {
          reportError(status, functionName, "Calling DIn");
          asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "portNumber=%d\n", i);
        }
        goto error;
      }
      changedBits = newValue ^ prevInput[i];
      if (forceCallback_[i] || (changedBits != 0)) {
        prevInput[i] = newValue;
        forceCallback_[i] = 0;
        setUIntDigitalParam(i, digitalInput_, newValue, 0xFFFFFFFF);
      }
    }

    // Read the counter inputs
    for (i=0; i<numCounters_; i++) {
      #ifdef _WIN32
        ULONG data;
        status = cbCIn32(boardNum_, firstCounter_ + i, &data);
        countVal = (epicsUInt32)data;
      #else
        unsigned long long data;
        status = ulCIn(daqDeviceHandle_, firstCounter_ + i, &data);
        countVal = (epicsUInt32)data;
      #endif
      if (status) {
        if (!prevStatus) {
          reportError(status, functionName, "Calling CIn");
        }
        goto error;
      }
      setIntegerParam(i, counterCounts_, countVal);
    }

    if (waveGenRunning_) {
      // Poll the status of the waveform generator output
      #ifdef _WIN32
        status = cbGetIOStatus(boardNum_, &aoStatus, &aoCount, &aoIndex, AOFUNCTION);
      #else
        ScanStatus scanStatus;
        TransferStatus xferStatus;
        status = ulAOutScanStatus(daqDeviceHandle_, &scanStatus, &xferStatus);
        aoStatus = scanStatus;
        aoCount = xferStatus.currentTotalCount;
        aoIndex = xferStatus.currentIndex;
      #endif
      if (status) {
        if (!prevStatus) {
          reportError(status, functionName, "Calling AOutScanStatus");
        }
        goto error;
      } else {
        asynPrint(pasynUserSelf, ASYN_TRACEIO_DRIVER,
          "%s::%s waveform generator status, aoStatus=%d, aoCount=%ld, aoIndex=%ld\n",
          driverName, functionName, aoStatus, aoCount, aoIndex);
      }
      currentPoint = (aoIndex / numWaveGenChans_) + 1;
      setIntegerParam(waveGenCurrentPoint_, currentPoint);
      if (aoStatus == 0) {
        stopWaveGen();
      }
    }

    if (waveDigRunning_) {
      // Poll the status of the waveform digitizer input
      #ifdef _WIN32
        status = cbGetIOStatus(boardNum_, &aiStatus, &aiCount, &aiIndex, AIFUNCTION);
      #else
        ScanStatus scanStatus;
        TransferStatus xferStatus;
        status = ulAInScanStatus(daqDeviceHandle_, &scanStatus, &xferStatus);
        aiStatus = scanStatus;
        aiCount = xferStatus.currentTotalCount;
        aiIndex = xferStatus.currentIndex;
      #endif
      if (status) {
        if (!prevStatus) {
          reportError(status, functionName, "Calling AInScanStatus");
        }
        #ifdef _WIN32
          // On Windows after a network glitch cbGetIOStatus will return continually return DEADDEV
          // Need to stop and start the waveform digitizer if it was running
          if (status == DEADDEV) {
            stopWaveDig();
            startWaveDig();
          }
          goto error;
        #endif
      } else {
        asynPrint(pasynUserSelf, ASYN_TRACEIO_DRIVER,
          "%s::%s waveform digitizer status, aiStatus=%d, aiCount=%ld, aiIndex=%ld\n",
          driverName, functionName, aiStatus, aiCount, aiIndex);
      }
      getIntegerParam(waveDigCurrentPoint_, &currentPoint);
      lastPoint = aiIndex / numWaveDigChans_ + 1;
      if (lastPoint > currentPoint) {
        currentTime = epicsTime::getCurrent();
        epicsTimeStamp now = (epicsTimeStamp)currentTime;
        int firstChan;
        getIntegerParam(waveDigFirstChan_, &firstChan);
        int lastChan = firstChan + numWaveDigChans_ - 1;
        epicsFloat64 *pAnalogIn = pInBuffer_ + currentPoint*numWaveDigChans_;
        for(; currentPoint < lastPoint; currentPoint++) {
          for (int j=firstChan; j<=lastChan; j++) {
            waveDigBuffer_[j][currentPoint] = *pAnalogIn++;
          }
          waveDigAbsTimeBuffer_[currentPoint] = now.secPastEpoch + now.nsec/1.e9;
        }
        setIntegerParam(waveDigCurrentPoint_, currentPoint);
      }
      if (aiStatus == 0) {
        stopWaveDig();
      }
    } else {
      // If the waveform digitizer is not running then read the analog inputs
      int range, type, mode;
      epicsInt32 value;
      getIntegerParam(0, analogInMode_, &mode);
      for (i=0; i<numAnalogIn_; i++) {
        getIntegerParam(i, analogInRange_, &range);
        getIntegerParam(i, analogInType_, &type);
        if (type != AI_CHAN_TYPE_VOLTAGE) continue;
        if ((boardType_ == E_1608) && (mode == DIFFERENTIAL) && (i>3)) break;
        #ifdef _WIN32
          if (ADCResolution_ <= 16) {
            epicsUInt16 shortVal;
            status = cbAIn(boardNum_, i, range, &shortVal);
            value = shortVal;
          } else {
            ULONG ulongVal;
            status = cbAIn32(boardNum_, i, range, &ulongVal, 0);
            value = (epicsInt32)ulongVal;
          }
        #else
          double data;
          Range ulRange;
          mapRange(range, &ulRange);
          status = ulAIn(daqDeviceHandle_, i, aiInputMode_, ulRange, AIN_FF_NOSCALEDATA, &data);
          value = (epicsInt32) data;
        #endif
        setIntegerParam(i, analogInValue_, value);
      }
    }

    for (i=0; i<MAX_SIGNALS; i++) {
      callParamCallbacks(i);
    }
error:
    if (prevStatus && !status) {
      reportError(-1, functionName, "Device returned to normal status");
    }
    prevStatus = status;
    double pollTime;
    getDoubleParam(pollSleepMS_, &pollTime);
    ULMutex.unlock();
    unlock();
    epicsThreadSleep(pollTime/1000.);
  }
}

/* Report  parameters */
void MultiFunction::report(FILE *fp, int details)
{
  int i;
  int counts;

  asynPortDriver::report(fp, details);
  fprintf(fp, "  Port: %s, board ID=%d, board type=%s\n",
          this->portName, boardType_, boardName_);
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
      fprintf(fp, "    I/O port read only    = %d\n", digitalIOPortReadOnly_[i]);
      fprintf(fp, "    I/O port write only   = %d\n", digitalIOPortWriteOnly_[i]);
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
extern "C" int MultiFunctionConfig(const char *portName, const char *uniqueID,
                              int maxInputPoints, int maxOutputPoints)
{
  new MultiFunction(portName, uniqueID, maxInputPoints, maxOutputPoints);
  return asynSuccess;
}


static const iocshArg configArg0 = { "Port name",      iocshArgString};
static const iocshArg configArg1 = { "UniqueID",       iocshArgString};
static const iocshArg configArg2 = { "Max. input points", iocshArgInt};
static const iocshArg configArg3 = { "Max. output points",iocshArgInt};
static const iocshArg * const configArgs[] = {&configArg0,
                                              &configArg1,
                                              &configArg2,
                                              &configArg3};
static const iocshFuncDef configFuncDef = {"MultiFunctionConfig",4,configArgs};
static void configCallFunc(const iocshArgBuf *args)
{
  MultiFunctionConfig(args[0].sval, args[1].sval, args[2].ival, args[3].ival);
}


static const iocshFuncDef showDevicesFuncDef = {"measCompShowDevices",0,0};
static void showDevicesCallFunc(const iocshArgBuf *args)
{
  measCompShowDevices();
}

void drvMultiFunctionRegister(void)
{
  iocshRegister(&configFuncDef,configCallFunc);
  iocshRegister(&showDevicesFuncDef,showDevicesCallFunc);
}


extern "C" {
epicsExportRegistrar(drvMultiFunctionRegister);
}

