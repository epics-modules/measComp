#include <stdio.h>
#include <string>
#ifdef linux
  #include "cbw_linux.h"
#else
  #include "cbw.h"
#endif

#include <epicsExport.h>
#include <measCompDiscover.h>

#define MAX_DEVICES 100
// All current Measurement Computing devices (E-1608, E-TC, TC32) use this port for
// discovery.  But it is configurable so we allow the user to specify it.
#define DEFAULT_DISCOVERY_PORT 54211

static DaqDeviceDescriptor measCompInventory[MAX_DEVICES];
static int measCompNumDevices = 0;
bool measCompInventoryInitialized = false;

int measCompDiscoverDevices()
{
  int numDevices = MAX_DEVICES;
  int status;

  if (measCompInventoryInitialized) return 0;
  cbIgnoreInstaCal();
  status = cbGetDaqDeviceInventory(ANY_IFC, measCompInventory, &numDevices);
  if (status) {
    printf("Error calling cbCreateDaqDevice=%d\n", status);
    return status;
  }
  measCompNumDevices = numDevices;
  measCompInventoryInitialized = true;
  return 0;
}

void measCompShowDevices()
{
  measCompDiscoverDevices();
  printf("measCompShowDevices, numDevices=%d\n", measCompNumDevices);
  for (int i=0; i<measCompNumDevices; i++) {
    printf("Device %d\n", i);
    printf("    ProductName: %s\n",   measCompInventory[i].ProductName);
    printf("      ProductID: %d\n",   measCompInventory[i].ProductID);
    printf("  InterfaceType: %d\n",   measCompInventory[i].InterfaceType);
    printf("      DevString: %s\n",   measCompInventory[i].DevString);
    printf("       UniqueID: %s\n",   measCompInventory[i].UniqueID);
    printf("           NUID: %llu\n", measCompInventory[i].NUID);
    printf("       Reserved: %s\n",   measCompInventory[i].Reserved);
  }
}

int measCompCreateDevice(std::string uniqueId)
{
  size_t colon;
  std::string host = uniqueId;
  std::string port;
  int portNum = DEFAULT_DISCOVERY_PORT;
  int status;
  int devIndex=-1;
  int timeoutMs=1000;

  measCompDiscoverDevices();
  // If the uniqueId contains a period then we assume it is an IP address
  if (uniqueId.find(".") != std::string::npos) {
    // The string can be of the format ipAddress:port, where port is the discovery port.
    colon = uniqueId.find(":");
    if (colon != std::string::npos) {
      host = uniqueId.substr(0, colon-1);
      port = uniqueId.substr(colon+1, std::string::npos);
      portNum = atoi(port.c_str());
    }
    printf("ipAddress=%s, port=%s, portNum=%d\n", uniqueId.c_str(), port.c_str(), portNum);
    status = cbGetNetDeviceDescriptor((char*)host.c_str(), portNum,
                                       &measCompInventory[measCompNumDevices], timeoutMs);
    if (status) {
        printf("Error calling cbGetNetDeviceDescriptor=%d\n", status);
        return -1;
    }
    devIndex = measCompNumDevices++;
  }
  else {
    // uniqueId was not an IP address so it must be a serial number (USB) or MAC address (Ethernet)
    // Search the inventory for the matching UniqueID
    for (int i=0; i<measCompNumDevices; i++) {
      if (uniqueId.compare(measCompInventory[i].UniqueID) == 0) {
        devIndex = i;
        break;
      }
    }
  }
  if (devIndex != -1) {
    status = cbCreateDaqDevice(devIndex, measCompInventory[devIndex]);
    if (status) {
      printf("Error calling cbCreateDaqDevice=%d\n", status);
      return -1;
    }
  }
  return devIndex;
}
