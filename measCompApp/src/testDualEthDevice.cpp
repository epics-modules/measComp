/*
    Tests for errors when running 2 Ethernet devices in the same process.
    All devices appear to have the problem.  This test is written for the E-DIO24.
    It simply reads all 3 DIO ports in a loop with a 1 ms sleep.
    Program requires 2 arguments, the IP addresses of the two devices.
    
    This program does not fail when the lines to lock and unlock the mutex are enabled.
    If those lines are commented out then I get the same errors as in the EPICS driver, 
    "Invalid frame ID" and error 102, "Invalid network frame".
*/

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "uldaq.h"

#define MAX_DEV_COUNT 100
#define MAX_LIBRARY_MESSAGE_LEN 256
#define MAX_DIO_PORTS 3

static DigitalPortType dioPorts[MAX_DIO_PORTS] = {AUXPORT0, AUXPORT1, AUXPORT2};
static pthread_mutex_t mutexID;

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

struct devStruct {
  int devNum;
  DaqDeviceHandle devHandle;
};

void *poller(void *ptr)
{
  struct devStruct *pds = (devStruct*)ptr;
  int err;
  unsigned long long data;
  
  for (int i=0; ; i++) {
    err = pthread_mutex_lock(&mutexID);
    if (err) {
      reportError(-1, "pthread_mutex_lock failed");
    }
    if ((i % 1000) == 0) {
      printf("loop=%d device %d\n", i, pds->devNum);
    }
    for (int port=0; port<MAX_DIO_PORTS; port++) {
      err = ulDIn(pds->devHandle, dioPorts[port], &data);
      reportError(err, "ulDIn");
    }
    err = pthread_mutex_unlock(&mutexID);
    if (err) {
      reportError(-1, "pthread_mutex_unlock failed");
    }
    usleep(1000);
  }
  return 0;
}

int main(int argc, char *argv[])
{
  DaqDeviceDescriptor devDescriptors[MAX_DEV_COUNT];
  int numDevs=MAX_DEV_COUNT;
  int devIndex;
  char *uniqueID[2];
  struct devStruct ds[2];
  pthread_t threadID[4];
  int i;
  int err;

  if (argc != 3) {
    reportError(-1, "Syntax: testDualEthDevice IPAddress1 IPAddress2");
  }
  uniqueID[0] = argv[1];
  uniqueID[1] = argv[2];
  
  err = pthread_mutex_init(&mutexID, NULL);
  if (err) {
    reportError(-1, "pthread_mutex_init failed");
  }

  err = ulGetDaqDeviceInventory(ANY_IFC, devDescriptors, (unsigned int*)&numDevs);
  reportError(err, "GetDaqDeviceInventory");

  // Verify at least two DAQ devices are detected
  if (numDevs < 2) {
    reportError(-1, "Too few DAQ devices detected\n");
  }

  printf("Found %d DAQ device(s)\n", numDevs);
  
  /* Find devices */
  for (i=0; i<2; i++) {
    ds[i].devNum = i;
    for (devIndex=0; devIndex < numDevs; devIndex++) {
      if (strcmp(uniqueID[i], devDescriptors[devIndex].uniqueId) == 0) {
        printf("Connecting to device type=%s uniqueID=%s\n", devDescriptors[devIndex].productName, devDescriptors[devIndex].uniqueId);
        ds[i].devHandle = ulCreateDaqDevice(devDescriptors[devIndex]);
        if (!ds[i].devHandle) {
          reportError(-1, "ulCreateDaqDevice");
        }
        err = ulConnectDaqDevice(ds[i].devHandle);
        reportError(err, "ulConnectDaqDevice");
        if (strcmp(devDescriptors[devIndex].productName, "E-DIO24") != 0) {
           reportError(-1, "Device is not an E-DIO24");
        }
        break;
      }
    }
    if (devIndex == numDevs) {
      reportError(-1, "Cannot find device");
    }    
  }
  /* Create 4 threads, 2 talking to first device and 2 to second device */
  pthread_create(&threadID[0], NULL, poller, (void *)&ds[0]);
  pthread_create(&threadID[1], NULL, poller, (void *)&ds[0]);
  pthread_create(&threadID[2], NULL, poller, (void *)&ds[1]);
  pthread_create(&threadID[3], NULL, poller, (void *)&ds[1]);

  pthread_join(threadID[0], NULL);
  pthread_join(threadID[1], NULL);
  pthread_join(threadID[2], NULL);
  pthread_join(threadID[3], NULL);
  return 0;
}
