/*
    Does a DaqInScan on a USB-CTR04/08 to test how fast it can run.
    Runs on both Windows and Linux.
    It reads all of the counters and the DIO.
    Program requires 3 arguments:
      1) The UnigueID of the device (serial number for USB, MAC address for Ethernet)
      2) The scan rate in Hz
      3) The sleep time in ms between reading the scan data
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
  int numCounters;
  int i;
  int err;

  if (argc != 4) {
    reportError(-1, "Syntax: test_USB_CTR uniqueID rate sleepTime");
  }
  char *uniqueID = argv[1];
  double rate = atof(argv[2]);
  int sleepTimeMs = atoi(argv[3]);

  double *pCountsF64 = (double *) calloc(SCAN_BUFFER_SIZE, sizeof(double));

  // Get descriptors for all of the available DAQ devices
  #ifdef _WIN32
    int boardNum;
    cbIgnoreInstaCal();
    err = cbGetDaqDeviceInventory(ANY_IFC, devDescriptors, &numDevs);
    int *pCountsI32 = (int *) pCountsF64;
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

  // Verify that the device is a USB-CTR04 or 08 and set the number of counters
  if (strcmp(productName, "USB-CTR04") == 0) {
    numCounters=4;
  } else if (strcmp(productName, "USB-CTR08") == 0) {
    numCounters=8;
  } else {
    reportError(-1, "Device is not a USB-CTR04 or USB-CTR08");
  }

  for (i=0; i<numCounters; i++) {
    #ifdef _WIN32
      int mode = OUTPUT_ON | CLEAR_ON_READ;
      err = cbCConfigScan(boardNum, i, mode, CTR_DEBOUNCE_NONE, CTR_TRIGGER_BEFORE_STABLE,
                          CTR_RISING_EDGE, CTR_TICK20PT83ns, 0);
    #else
      int mode = CMM_OUTPUT_ON | CMM_CLEAR_ON_READ;
      err = ulCConfigScan(devHandle, i, CMT_COUNT,  (CounterMeasurementMode) mode,
					                CED_RISING_EDGE, CTS_TICK_20PT83ns, CDM_NONE, CDT_DEBOUNCE_0ns, CF_DEFAULT);
    #endif
  }
  reportError(err, "CConfigScan");

  #ifdef _WIN32
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
  #else
    ScanOption options = SO_DEFAULTIO;
    DaqInScanFlag flags = DAQINSCAN_FF_DEFAULT;
    DaqInChanDescriptor *pDICD = new DaqInChanDescriptor[MAX_DAQ_INPUTS];
    int outChan=0;
    for (i=0; i<numCounters; i++) {
      pDICD[outChan].channel = i;
      pDICD[outChan].type     = DAQI_CTR32;
      pDICD[outChan].range    = BIP10VOLTS;
      outChan++;
    }
    pDICD[outChan].channel  = AUXPORT;
    pDICD[outChan].type     = DAQI_DIGITAL;
    pDICD[outChan].range    = BIP10VOLTS;
    outChan++;
    int numChans = outChan;
    err = ulDaqInScan(devHandle, pDICD, numChans, MAX_SCAN_POINTS, &rate, options, flags, pCountsF64);
    delete[] pDICD;
  #endif  
  reportError(err, "DaqInScan");
  
  short ctrStatus=1;
  long ctrCount, ctrIndex;
  while (ctrStatus) {
     #ifdef _WIN32
      Sleep((int)(sleepTimeMs));
      err = cbGetIOStatus(boardNum, &ctrStatus, &ctrCount, &ctrIndex, DAQIFUNCTION);
      ctrCount /= 2;
      ctrIndex /= 2;
    #else
      ScanStatus scanStatus;
      TransferStatus xferStatus;
      usleep((int)(sleepTimeMs * 1000));
      err = ulDaqInScanStatus(devHandle, &scanStatus, &xferStatus);
      ctrStatus = scanStatus;
      ctrCount = xferStatus.currentTotalCount;
      ctrIndex = xferStatus.currentIndex;
    #endif
    reportError(err, "ScanStatus");
    printf("ctrStatus=%d, ctrCount=%ld, ctrIndex=%ld, counts=", ctrStatus, ctrCount, ctrIndex);
    #ifdef _WIN32
      for (i=0; i<numCounters; i++) printf("%d ", pCountsI32[i]);
      printf("0x%x\n", pCountsI32[i]);
    #else
      for (i=0; i<numCounters; i++) printf("%d ", (int)pCountsF64[ctrIndex+i]);
      printf("0x%x\n", (unsigned int)pCountsF64[ctrIndex+i]);
    #endif
  }
  #ifdef _WIN32
    err = cbStopBackground(boardNum, DAQIFUNCTION);
  #else
    err= ulDaqInScanStop(devHandle);
  #endif
  reportError(err, "Stop scan");
  free(pCountsF64);
  return 0;
}
