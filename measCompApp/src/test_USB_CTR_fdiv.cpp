/*
    Test problems with pulse frequency division
    Runs on both Windows and Linux.
    Programs Timer 3 to output 1 kHz, 50% duty cycle
    It programs Counter 7 to divide input pulse frequency by 10.
    The following physical connections are required
    - Timer 3 connected to Counter 7 (TMR1 to C7IN).
    - Counter 7 Output connected to scope.

    Program requires 1 argument:
      1) The UniqueID of the device (serial number for USB, MAC address for Ethernet)
      
   The program works as expected on Windows, the scope shows 100 Hz output on Timer 7 Out (9 ms high, 1 ms low).
   On Linux it does not work, The 1 kHz output on timer 3 is fine, but there are no pulse outputs on Counter 7 Out. 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifdef _WIN32
  #include "cbw.h"
#else
  #include <unistd.h>
  #include "uldaq.h"
#endif

#define MAX_DEV_COUNT  100
#define MAX_LIBRARY_MESSAGE_LEN 256
#define PRESCALE_COUNTER 7
#define PRESCALE_FACTOR 10

static void reportError(int err, const char *message)
{
  char libraryMessage[MAX_LIBRARY_MESSAGE_LEN];
  if (err == 0) return;
  if (err == -1) {
    printf("Error: %s\n", message);
  } else {
    #ifdef _WIN32
      cbGetErrMsg(err, libraryMessage);
    #else
      ulGetErrMsg((UlError)err, libraryMessage);
    #endif
    printf("Error: %s, err=%d %s\n", message, err, libraryMessage);
  }
  exit(-1);
}

int main(int argc, char *argv[])
{
  DaqDeviceDescriptor devDescriptors[MAX_DEV_COUNT];
  int devIndex;
  int numDevs=MAX_DEV_COUNT;
  char *productName=0;
  int mode;
  int err;

  if (argc != 2) {
    reportError(-1, "Syntax: test_USB_CTR_fdiv uniqueID");
  }
  char *uniqueID = argv[1];

  // Get descriptors for all of the available DAQ devices
  #ifdef _WIN32
    int boardNum;
    cbIgnoreInstaCal();
    err = cbGetDaqDeviceInventory(ANY_IFC, devDescriptors, &numDevs);
  #else
    DaqDeviceHandle devHandle=-1;
    err = ulGetDaqDeviceInventory(ANY_IFC, devDescriptors, (unsigned int*)&numDevs);
  #endif
  reportError(err, "GetDaqDeviceInventory");

  // verify at least one DAQ device is detected
  if (numDevs == 0) {
    reportError(-1, "No DAQ device is detected\n");
  }

  printf("Found %d DAQ device(s)\n", numDevs);
  for (devIndex=0; devIndex < numDevs; devIndex++) {
    #ifdef _WIN32
      if (strcmp(uniqueID, devDescriptors[devIndex].UniqueID) == 0) {
        printf("Running on Windows\n");
        printf("Connecting to device type=%s serial number=%s\n", devDescriptors[devIndex].ProductName, devDescriptors[devIndex].UniqueID);
        err = cbCreateDaqDevice(devIndex, devDescriptors[devIndex]);
        reportError(err, "cbCreateDaqDevice");
        productName = devDescriptors[devIndex].ProductName;
        boardNum = devIndex;
        break;
      }
    #else
      if (strcmp(uniqueID, devDescriptors[devIndex].uniqueId) == 0) {
        printf("Running on Linux\n");
        printf("Connecting to device type=%s serial number=%s\n", devDescriptors[devIndex].productName, devDescriptors[devIndex].uniqueId);
        devHandle = ulCreateDaqDevice(devDescriptors[devIndex]);
        if (!devHandle) {
          reportError(-1, "ulCreateDaqDevice");
        }
        err = ulConnectDaqDevice(devHandle);
        reportError(err, "ulConnectDaqDevice");
        productName = devDescriptors[devIndex].productName;
        break;
      }
    #endif
  }
  if (devIndex == numDevs) {
    reportError(-1, "Cannot find device");
  }

  // Verify that the device is a USB-CTR08 and set the number of counters
  if (strcmp(productName, "USB-CTR08") != 0) {
    reportError(-1, "Device is not USB-CTR08");
    return -1;
  }

  // Program pulse generator 3 for 1 kHz square wave
  double frequency = 1.e3, dutyCycle = 0.5, delay = 0.;
  #ifdef _WIN32
    err = cbPulseOutStart(boardNum, 3, &frequency, &dutyCycle, 0, &delay, 0, 0);
  #else
    err = ulTmrPulseOutStart(devHandle, 3, &frequency, &dutyCycle, 0, &delay, TMRIS_LOW, PO_DEFAULT);
  #endif
  reportError(err, "PulseOutStart");

  // Program PRESCALE_COUNTER (7) to divide inputs pulses by PRESCALE_FACTOR (10).
  #ifdef _WIN32
    err  = cbCLoad32(boardNum, OUTPUTVAL0REG0+PRESCALE_COUNTER, 0);
    err |= cbCLoad32(boardNum, OUTPUTVAL1REG0+PRESCALE_COUNTER, PRESCALE_FACTOR-1);
    err |= cbCLoad32(boardNum, MAXLIMITREG0+PRESCALE_COUNTER, PRESCALE_FACTOR-1);
    mode = OUTPUT_ON | RANGE_LIMIT_ON;
    err |= cbCConfigScan(boardNum, PRESCALE_COUNTER, mode, CTR_DEBOUNCE_NONE, CTR_TRIGGER_BEFORE_STABLE,
                        CTR_RISING_EDGE, CTR_TICK20PT83ns, 0);
  #else
    err  = ulCLoad(devHandle, PRESCALE_COUNTER, CRT_OUTPUT_VAL0, 0);
    err |= ulCLoad(devHandle, PRESCALE_COUNTER, CRT_OUTPUT_VAL1, PRESCALE_FACTOR-1);
    err |= ulCLoad(devHandle, PRESCALE_COUNTER, CRT_MAX_LIMIT, PRESCALE_FACTOR-1);
    mode = CMM_OUTPUT_ON | CMM_RANGE_LIMIT_ON;
    err |= ulCConfigScan(devHandle, PRESCALE_COUNTER, CMT_COUNT,  (CounterMeasurementMode) mode,
				                CED_RISING_EDGE, CTS_TICK_20PT83ns, CDM_NONE, CDT_DEBOUNCE_0ns, CF_DEFAULT);
  #endif
  reportError(err, "CConfigScan");
  return 0;
}
