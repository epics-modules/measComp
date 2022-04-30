/*
    Does a DaqInScan on a USB-CTR04/08 to test problems with rates >1000 Hz on Linux.
    It reads all of the counters and the DIO.
    For best diagnostic results make the following physical connections:
    - Timer 0 connected to Counter 0 (TMR0 to C0IN).  The program configures this for 1 MHz square wave.
    - Timer 1 connected to Counter 1 (TMR1 to C1IN). The program configures this for 100 kHz square wave.
    - Timer 2 connected to Digital input 0 (TMR2 to DIO0). The program configures this for 10 Hz square wave.

   This is the Windows version of the program.  It works fine with RATE up to 220000.
   The equivalent program on Linux works as expected when RATE <= 1000 Hz.  
   Above 1 kHz (e.g. 2 kHz) it works the first time it is run at that rate.
   However, on subsequent runs the data is not in the right order, and the scan never stops.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "cbw.h"

// Need to edit this for your device
#define UNIQUE_ID "1E538A2"
// On Linux program fails with RATE=2000.  Works with RATE=1000
#define RATE 200000
// This is the time to sleep between readings
#define SLEEP_TIME 0.1

#define MAX_DEV_COUNT  100
#define MAX_LIBRARY_MESSAGE_LEN 256
#define MAX_SCAN_POINTS 1000
#define MAX_COUNTERS 8
#define MAX_DAQ_INPUTS (MAX_COUNTERS+1)
#define SCAN_BUFFER_SIZE (MAX_SCAN_POINTS * MAX_DAQ_INPUTS)

static void reportError(int err, const char *message)
{
  char libraryMessage[MAX_LIBRARY_MESSAGE_LEN];
  if (err == 0) return;
  if (err == -1) {
    printf("Error: %s\n", message);
  } else {
    cbGetErrMsg(err, libraryMessage);
    printf("Error: %s, err=%d %s\n", message, err, libraryMessage);
  }
  exit(-1);
}

int main(int argc, char *argv[])
{
  DaqDeviceDescriptor devDescriptors[MAX_DEV_COUNT];
  int devIndex;
  int numDevs=MAX_DEV_COUNT;
  double rate=RATE;
  char *productName=0;
  int numCounters;
  int i;
  int err;

  int *pCountsI32 = (int *) calloc(SCAN_BUFFER_SIZE, sizeof(int));

  // Get descriptors for all of the available DAQ devices
  int boardNum;
  cbIgnoreInstaCal();
  err = cbGetDaqDeviceInventory(ANY_IFC, devDescriptors, &numDevs);
  reportError(err, "GetDaqDeviceInventory");

  // verify at least one DAQ device is detected
  if (numDevs == 0) {
    reportError(-1, "No DAQ device is detected\n");
  }

  printf("Found %d DAQ device(s)\n", numDevs);
  for (devIndex=0; devIndex < numDevs; devIndex++) {
    if (strcmp(UNIQUE_ID, devDescriptors[devIndex].UniqueID) == 0) {
      printf("Running on Windows, RATE=%f\n", rate);
      printf("Connecting to device type=%s serial number=%s\n", devDescriptors[devIndex].ProductName, devDescriptors[devIndex].UniqueID);
      err = cbCreateDaqDevice(devIndex, devDescriptors[devIndex]);
      reportError(err, "cbCreateDaqDevice");
      productName = devDescriptors[devIndex].ProductName;
      boardNum = devIndex;
      break;
    }
  }
  if (devIndex == numDevs) {
    reportError(-1, "Cannot find device");
  }

  // Verify that the device is a USB-CTR04 or 08 and set the number of counters
  if (strcmp(productName, "USB-CTR04") == 0) {
    numCounters=4;
  } else if (strcmp(productName, "USB-CTR08") == 0) {
    numCounters=8;
  } else {
    reportError(-1, "Device is not a USB-CTR04 or USB-CTR08");
  }

  // Program pulse generators.  0=1 MHz, 1=100 kHz, 2=10 Hz, all 50% duty cycle
  double frequency[] = {1e6,1e5,10}, dutyCycle = 0.5, delay = 0.;
  for (i=0; i<3; i++) {
    err = cbPulseOutStart(boardNum, i, &frequency[i], &dutyCycle, 0, &delay, 0, 0);
    reportError(err, "PulseOutStart");
  }

  for (i=0; i<numCounters; i++) {
    int mode = OUTPUT_ON | CLEAR_ON_READ;
    err = cbCConfigScan(boardNum, i, mode, CTR_DEBOUNCE_NONE, CTR_TRIGGER_BEFORE_STABLE,
                        CTR_RISING_EDGE, CTR_TICK20PT83ns, 0);
  }
  reportError(err, "CConfigScan");

  long count;
  int chanCount;
  #define MAX_DAQ_LEN (2*MAX_DAQ_INPUTS)
  short chanArray[MAX_DAQ_LEN];
  short chanTypeArray[MAX_DAQ_LEN];
  short gainArray[MAX_DAQ_LEN];
  long pretrigCount = 0;
  long longRate = (long)(rate + 0.5);
  int options = BACKGROUND;
  for (i=0; i<MAX_DAQ_LEN; i++) {
    gainArray[i] = BIP10VOLTS;
  }
  for (i=0, chanCount=0; i<numCounters; i++) {
    chanArray[chanCount] = i;
    chanTypeArray[chanCount++] = CTRBANK0;
    chanArray[chanCount] = i;
    chanTypeArray[chanCount++] = CTRBANK1;
  }
  chanArray[chanCount]        = AUXPORT;
  chanTypeArray[chanCount++]  = DIGITAL8;
  chanArray[chanCount]        = 0;
  chanTypeArray[chanCount++]  = PADZERO;
  count = chanCount * MAX_SCAN_POINTS;
  err = cbDaqInScan(boardNum, chanArray, chanTypeArray, gainArray, chanCount, &longRate,
                    &pretrigCount, &count, pCountsI32, options);
  // The actual dwell time can be different from requested due to clock granularity
  rate = (double)longRate;
  reportError(err, "DaqInScan");
  
  short ctrStatus=1;
  long ctrCount, ctrIndex;
  while (ctrStatus) {
    Sleep((int)(SLEEP_TIME * 1000));
    err = cbGetIOStatus(boardNum, &ctrStatus, &ctrCount, &ctrIndex, DAQIFUNCTION);
    ctrCount /= 2;
    ctrIndex /= 2;
    reportError(err, "ScanStatus");
    printf("ctrStatus=%d, ctrCount=%ld, ctrIndex=%ld, counts=", ctrStatus, ctrCount, ctrIndex);
    for (i=0; i<numCounters; i++) printf("%d ", pCountsI32[ctrIndex+i]);
    printf("0x%x\n", pCountsI32[ctrIndex+i]);
  }
  err = cbStopBackground(boardNum, DAQIFUNCTION);
  reportError(err, "Stop scan");
  free(pCountsI32);
  return 0;
}
