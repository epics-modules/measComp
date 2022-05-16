/*
    Does a AInScan to test problems with rates >1000 Hz on Linux.
    It reads all of the analog inputs

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
#define UNIQUE_ID "016669C2"
// Program fails with RATE=1001 or higher the second time it is run.  
// Works with RATE=1000 or less
#define RATE 10000
// This is the time to sleep between readings
#define SLEEP_TIME 0.1

#define MAX_DEV_COUNT  100
#define MAX_LIBRARY_MESSAGE_LEN 256
#define MAX_SCAN_POINTS 2048
#define MAX_INPUTS 8
#define SCAN_BUFFER_SIZE (MAX_SCAN_POINTS * MAX_INPUTS)

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
  int i;
  int err;

  double *pData = (double *) calloc(SCAN_BUFFER_SIZE, sizeof(double));

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

  // Verify that the device is a USB-1608 variant
  if (strstr(productName, "USB-1608") == 0) {
    reportError(-1, "Device is not in the USB-1608 family");
  }

  ScanOption options = SO_DEFAULTIO;
  AInScanFlag flags = AINSCAN_FF_DEFAULT;
  err = ulAInScan(devHandle, 0, 7, AI_SINGLE_ENDED, BIP10VOLTS, MAX_SCAN_POINTS, &rate, options, flags, pData);
  reportError(err, "AInScan");
  
  short acqStatus=1;
  long acqCount, acqIndex;
  ScanStatus scanStatus;
  TransferStatus xferStatus;
  while (acqStatus) {
    usleep((int)(SLEEP_TIME * 1000000));
    err = ulAInScanStatus(devHandle, &scanStatus, &xferStatus);
    acqStatus = scanStatus;
    acqCount = xferStatus.currentTotalCount;
    acqIndex = xferStatus.currentIndex;
    reportError(err, "ScanStatus");
    printf("acqStatus=%d, acqCount=%ld, acqIndex=%ld, counts=", acqStatus, acqCount, acqIndex);
    for (i=0; i<MAX_INPUTS; i++) printf("%f ", pData[acqIndex+i]);
    printf("\n");
  }
  err= ulAInScanStop(devHandle);
  reportError(err, "Stop scan");
  free(pData);
  return 0;
}
