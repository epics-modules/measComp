/*
    Does a DaqInScan on a USB-CTR04/08 to test problems with rates >1000 Hz on Linux.
    It reads all of the counters and the DIO.
    For best diagnostic results make the following physical connections:
    - Timer 0 connected to Counter 0 (TMR0 to C0IN).  The program configures this for 1 MHz square wave.
    - Timer 1 connected to Counter 1 (TMR1 to C1IN). The program configures this for 100 kHz square wave.
    - Timer 2 connected to Digital input 0 (TMR2 to DIO0). The program configures this for 10 Hz square wave.

   The equivalent program works as expected on Windows.
   On Linux this program works as expected when RATE <= 1000 Hz.  
   Above 1 kHz (e.g. 2 kHz) it works the first time it is run at that rate.
   However, on subsequent runs the data is not in the right order, and the scan never stops.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include "uldaq.h"

// Need to edit this for your device
#define UNIQUE_ID "01E538A2"
// Program fails with RATE=1001 or higher the second time it is run.  
// Works with RATE=1000 or less
#define RATE 1001
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
    ulGetErrMsg((UlError)err, libraryMessage);
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

  double *pCountsF64 = (double *) calloc(SCAN_BUFFER_SIZE, sizeof(double));

  // Get descriptors for all of the available DAQ devices
  DaqDeviceHandle devHandle=-1;
  err = ulGetDaqDeviceInventory(ANY_IFC, devDescriptors, (unsigned int*)&numDevs);
  reportError(err, "GetDaqDeviceInventory");

  // verify at least one DAQ device is detected
  if (numDevs == 0) {
    reportError(-1, "No DAQ device is detected\n");
  }

  printf("Found %d DAQ device(s)\n", numDevs);
  for (devIndex=0; devIndex < numDevs; devIndex++) {
    if (strcmp(UNIQUE_ID, devDescriptors[devIndex].uniqueId) == 0) {
      printf("Running on Linux, RATE=%f\n", rate);
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

  int numDaqInputs = numCounters+1;
  char firmwareVersion[256];
  unsigned int configLen = 256;
  ulDevGetConfigStr(devHandle, ::DEV_CFG_VER_STR, DEV_VER_FW_MAIN, firmwareVersion, &configLen);
  printf("Firmware version: %s\n", firmwareVersion);
  ulDevGetConfigStr(devHandle, ::DEV_CFG_VER_STR, DEV_VER_FPGA, firmwareVersion, &configLen);
  printf("FPGA version: %s\n", firmwareVersion);
 
  // Program pulse generators.  0=1 MHz, 1=100 kHz, 2=10 Hz, all 50% duty cycle
  double frequency[] = {1e6,1e5,10}, dutyCycle = 0.5, delay = 0.;
  for (i=0; i<3; i++) {
    err = ulTmrPulseOutStart(devHandle, i, &frequency[i], &dutyCycle, 0, &delay, TMRIS_LOW, PO_DEFAULT);
    reportError(err, "PulseOutStart");
  }

  for (i=0; i<numCounters; i++) {
    //int mode = CMM_OUTPUT_ON | CMM_CLEAR_ON_READ;
    int mode = CMM_OUTPUT_ON;
    err = ulCConfigScan(devHandle, i, CMT_COUNT,  (CounterMeasurementMode) mode,
			                CED_RISING_EDGE, CTS_TICK_20PT83ns, CDM_NONE, CDT_DEBOUNCE_0ns, CF_DEFAULT);
  }
  reportError(err, "CConfigScan");

  ScanOption options = SO_DEFAULTIO;
  DaqInScanFlag flags = DAQINSCAN_FF_DEFAULT;
  DaqInChanDescriptor *pDICD = new DaqInChanDescriptor[numDaqInputs];
  int outChan=0;
  for (i=0; i<numCounters; i++) {
    pDICD[outChan].channel = i;
    pDICD[outChan].type     = DAQI_CTR32;
    outChan++;
  }
  pDICD[outChan].channel  = AUXPORT;
  pDICD[outChan].type     = DAQI_DIGITAL;
  outChan++;
  int numChans = outChan;
  err = ulDaqInScan(devHandle, pDICD, numChans, MAX_SCAN_POINTS, &rate, options, flags, pCountsF64);
  delete[] pDICD;
  reportError(err, "DaqInScan");
  
  short ctrStatus=1;
  long ctrCount, ctrIndex;
  ScanStatus scanStatus;
  TransferStatus xferStatus;
  while (ctrStatus) {
    usleep((int)(SLEEP_TIME * 1000000));
    err = ulDaqInScanStatus(devHandle, &scanStatus, &xferStatus);
    ctrStatus = scanStatus;
    ctrCount = xferStatus.currentTotalCount;
    ctrIndex = xferStatus.currentIndex;
    reportError(err, "ScanStatus");
    printf("ctrStatus=%d, ctrCount=%ld, ctrIndex=%ld, counts=", ctrStatus, ctrCount, ctrIndex);
    for (i=0; i<numCounters; i++) printf("%d ", (int)pCountsF64[ctrIndex+i]);
    printf("0x%x\n", (unsigned int)pCountsF64[ctrIndex+i]);
  }
  err= ulDaqInScanStop(devHandle);
  reportError(err, "Stop scan");
  free(pCountsF64);
  return 0;
}
