/*
    Reads the first temperature input on a device 20 times.
    Prints the data for each point, and the mean and standard deviation for all 20 readings.
    Program requires 1 argument, the UnigueID of the device (serial number for USB, MAC address for Ethernet).
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
#define MAX_READINGS 20
#define SLEEP_TIME 0.1

void reportError(int err, const char *message)
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
  int err;

  if (argc != 2) {
    reportError(-1, "Must specify uniqueID as first argument");
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
        break;
      }
    #endif
  }
  if (devIndex == numDevs) {
    reportError(-1, "Cannot find device");
  }

  DaqDeviceDescriptor devDescriptor = devDescriptors[devIndex];

  // Verify the specified DAQ device has temperature channels
  int numTempChans;
  #ifdef _WIN32
    err = cbGetConfig(BOARDINFO, boardNum, 0, BINUMTEMPCHANS,  &numTempChans);
  #else
    long long infoValue;
    err = ulAIGetInfo(devHandle, AI_INFO_NUM_CHANS_BY_TYPE, AI_TC, &infoValue);
    numTempChans = infoValue;
  #endif
  reportError(err, "Reading number of temperature channels");
  if (numTempChans < 1) {
    reportError(-1, "The specified DAQ device does not have a temperature channel");
  }

  // Configure the analog input channels if required
  const int USB_2416_ID = 0xd0;
  const int USB_2416_4AO_ID = 0xd1;
  const int USB_2408_ID = 0xfd;
  const int USB_2408_2AO_ID = 0xfe;

  #ifdef _WIN32
  if (devDescriptor.ProductID == USB_2416_ID || devDescriptor.ProductID == USB_2416_4AO_ID || 
      devDescriptor.ProductID == USB_2408_ID || devDescriptor.ProductID == USB_2408_2AO_ID) {
      err = cbSetConfig(BOARDINFO, boardNum, 0, BIADCHANTYPE, AI_CHAN_TYPE_TC);
    }
  #else
    if (devDescriptor.productId == USB_2416_ID || devDescriptor.productId == USB_2416_4AO_ID || 
        devDescriptor.productId == USB_2408_ID || devDescriptor.productId == USB_2408_2AO_ID) {  
      err = ulAISetConfig(devHandle, AI_CFG_CHAN_TYPE, 0, AI_TC);
    }
  #endif
  reportError(err, "Setting analog input type to thermocouple");

  // Set to Type K thermocouple
  #ifdef _WIN32 
      err = cbSetConfig(BOARDINFO, boardNum, 0, BICHANTCTYPE, TC_TYPE_K);
  #else
      err = ulAISetConfig(devHandle, AI_CFG_CHAN_TC_TYPE, 0, TC_K);
  #endif
  reportError(err, "Setting thermocouple type to K");
  double data;
  double sum=0;
  double temperatureReadings[MAX_READINGS];
  for (int i=0; i<MAX_READINGS; i++) {
    #ifdef _WIN32
      float fVal;
      err = cbTIn(boardNum, 0, CELSIUS, &fVal, NOFILTER);
      data = fVal;
      Sleep((int)(SLEEP_TIME * 1000));
    #else
      err = ulTIn(devHandle, 0, TS_CELSIUS, TIN_FF_DEFAULT, &data);
      usleep((int)(SLEEP_TIME * 1e6));
    #endif
    reportError(err, "Reading temperature");
    printf("Reading: %d, T: %f\n", i, data);
    temperatureReadings[i] = data;
    sum += data;
  }
  
  // Compute mean and standard deviation
  double mean = sum/MAX_READINGS;
  double sumsq=0;
  for (int i=0; i<MAX_READINGS; i++) {
    sumsq += (temperatureReadings[i] - mean) * (temperatureReadings[i] - mean);
  }
  double stddev = sqrt(sumsq/MAX_READINGS);
  printf("Mean=%f\n", mean);
  printf("Standard deviation=%f\n", stddev);

  return 0;
}
