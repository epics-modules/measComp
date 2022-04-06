/*
    Does a CInScan on a USB-CTR04/08 to test how fast it can run.
    Program requires 2 arguments, the UnigueID of the device (serial number for USB, MAC address for Ethernet), and the scan rate in Hz.
    Runs on both Windows and Linux.
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
#define MAX_SCAN_POINTS 2048
#define SCAN_BUFFER_SIZE (MAX_SCAN_POINTS * (MAX_COUNTERS + 1))
#define MAX_COUNTERS 8
#define SLEEP_TIME 0.1

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

  if (argc != 3) {
    reportError(-1, "Must specify uniqueID as first argument");
  }
  char *uniqueID = argv[1];
  double rate = atof(argv[2]);

  // Get descriptors for all of the available DAQ devices
  #ifdef _WIN32
    int boardNum;
    cbIgnoreInstaCal();
    int *pCountsI32 = (int *) calloc(SCAN_BUFFER_SIZE, sizeof(int));
    err = cbGetDaqDeviceInventory(ANY_IFC, devDescriptors, &numDevs);
  #else
    DaqDeviceHandle devHandle=-1;
    double *pCountsF64 = (double *) calloc(SCAN_BUFFER_SIZE, sizeof(double));
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
  reportError(err, "ConfigScan");

  #ifdef _WIN32
    long count;
    int chanCount;
    #define MAX_DAQ_LEN 2*(MAX_COUNTERS+1)
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
    DaqInChanDescriptor *pDICD = new DaqInChanDescriptor[MAX_COUNTERS];
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
  
  short ctrStatus=0;
  long ctrCount, ctrIndex;
  while (ctrStatus) {
     #ifdef _WIN32
      err = cbGetIOStatus(boardNum, &ctrStatus, &ctrCount, &ctrIndex, DAQIFUNCTION);
    #else
      ScanStatus scanStatus;
      TransferStatus xferStatus;
      err = ulDaqInScanStatus(devHandle, &scanStatus, &xferStatus);
      ctrStatus = scanStatus;
      ctrCount = xferStatus.currentTotalCount;
      ctrIndex = xferStatus.currentIndex;
    #endif
    reportError(err, "ScanStatus");
    printf("ctrStatus=%d, ctrCount=%ld, ctrIndex=%ld\n", ctrStatus, ctrCount, ctrIndex);
  }
  
  // Print out the values

  return 0;
}
