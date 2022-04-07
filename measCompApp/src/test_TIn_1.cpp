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
  int productId=0;
  int err;

  if (argc != 5) {
    reportError(-1, "Usage: test_TIn_1 UniqueID OTDMode SampleRate NumPoints ");
  }
  char *uniqueID  = argv[1];
  int otdMode     = atoi(argv[2]);
  int sampleRate  = atoi(argv[3]);
  int numPoints   = atoi(argv[4]);
  printf("uniqueID=%s, ODTMode=%s, sampleRate=%d, numPoints=%d\n", uniqueID, (otdMode==1) ? "Enabled" : "Disabled", sampleRate, numPoints); 

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
        productId = devDescriptors[devIndex].ProductID;
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
        productId = devDescriptors[devIndex].productId;
        break;
      }
    #endif
  }
  if (devIndex == numDevs) {
    reportError(-1, "Cannot find device");
  }

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

  if (productId == USB_2416_ID || productId == USB_2416_4AO_ID || 
      productId == USB_2408_ID || productId == USB_2408_2AO_ID) {
    #ifdef _WIN32
        err = cbSetConfig(BOARDINFO, boardNum, 0, BIADCHANTYPE, AI_CHAN_TYPE_TC);
    #else
        err = ulAISetConfig(devHandle, AI_CFG_CHAN_TYPE, 0, AI_TC);
    #endif
    reportError(err, "Setting analog input type to thermocouple");
  }

  // Set thermocouple open detect mode
  #ifdef _WIN32 
      err = cbSetConfig(BOARDINFO, boardNum, 0, BIDETECTOPENTC, otdMode);
  #else
      OtdMode mode = otdMode ? OTD_ENABLED : OTD_DISABLED;
      err = ulAISetConfig(devHandle, AI_CFG_CHAN_OTD_MODE, 0, mode);
  #endif
  reportError(err, "Setting ODT mode");

  // Set sampling rate to 60 Hz
  #ifdef _WIN32 
      err = cbSetConfig(BOARDINFO, boardNum, 0, BIADDATARATE, sampleRate);
  #else
      err = ulAISetConfigDbl(devHandle, AI_CFG_CHAN_DATA_RATE, 0, sampleRate);
  #endif
  reportError(err, "Setting data rate to 60 Hz");

  // Set thermocouple type to K
  #ifdef _WIN32 
      err = cbSetConfig(BOARDINFO, boardNum, 0, BICHANTCTYPE, TC_TYPE_K);
  #else
      err = ulAISetConfig(devHandle, AI_CFG_CHAN_TC_TYPE, 0, TC_K);
  #endif
  reportError(err, "Setting thermocouple type to K");

  double data;
  double sum=0;
  bool openTC=false;
  double *pReadings = new double[numPoints];
  for (int i=0; i<numPoints; i++) {
    #ifdef _WIN32
      float fVal;
      err = cbTIn(boardNum, 0, CELSIUS, &fVal, NOFILTER);
      data = fVal;
      Sleep((int)(SLEEP_TIME * 1000));
      openTC = (err == OPENCONNECTION);
    #else
      err = ulTIn(devHandle, 0, TS_CELSIUS, TIN_FF_DEFAULT, &data);
      openTC = (err == ERR_OPEN_CONNECTION);
      usleep((int)(SLEEP_TIME * 1e6));
    #endif
    if (openTC) 
      printf("Open connection: ");
    else
      reportError(err, "Reading temperature");
    printf("Reading: %d, T: %f\n", i, data);
    pReadings[i] = data;
    sum += data;
  }
  
  // Compute mean and standard deviation
  double mean = sum/numPoints;
  double sumsq=0;
  for (int i=0; i<numPoints; i++) {
    sumsq += (pReadings[i] - mean) * (pReadings[i] - mean);
  }
  double stddev = sqrt(sumsq/numPoints);
  printf("Mean=%f\n", mean);
  printf("Standard deviation=%f\n", stddev);
  delete[] pReadings;

  return 0;
}
