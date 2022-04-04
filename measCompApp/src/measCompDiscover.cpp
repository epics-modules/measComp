#include <stdio.h>
#include <string>
#include <stdlib.h>
#ifdef _WIN32
  #include "cbw.h"
#else
  #include "uldaq.h"
#endif

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
  #ifdef WIN32
    cbIgnoreInstaCal();
    status = cbGetDaqDeviceInventory(ANY_IFC, measCompInventory, &numDevices);
    // Windows eliminates leading zeros on USB device serial numbers, Linux does not.
    // Add the leading zero if the string length is 7
    for (int i=0; i<numDevices; i++) {
        if (measCompInventory[i].InterfaceType == USB_IFC) {
            if (strlen(measCompInventory[i].UniqueID) == 7) {
                sprintf(measCompInventory[i].UniqueID, "0%s", measCompInventory[i].UniqueID);
            }
        }
    } 
  #else
    status = ulGetDaqDeviceInventory(ANY_IFC, measCompInventory, (unsigned int *)&numDevices);
    // Copy the IP address to the reserved field for Ethernet devices.
    for (int i=0; i<numDevices; i++) {
        if (measCompInventory[i].devInterface == ETHERNET_IFC) {
            DaqDeviceHandle devHandle = ulCreateDaqDevice(measCompInventory[i]);
            unsigned int maxLen = sizeof(measCompInventory[i].reserved);
            ulDevGetConfigStr(devHandle, DEV_CFG_IP_ADDR_STR, 0, measCompInventory[i].reserved, &maxLen);
            ulReleaseDaqDevice(devHandle);
        }
    }
  #endif
  if (status) {
    printf("Error calling cbGetDaqDeviceInventory=%d\n", status);
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
  #ifdef WIN32
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
  #else
    for (int i=0; i<measCompNumDevices; i++) {
      printf("Device %d\n", i);
      printf("    ProductName: %s\n",   measCompInventory[i].productName);
      printf("      ProductID: %d\n",   measCompInventory[i].productId);
      printf("  InterfaceType: %d\n",   measCompInventory[i].devInterface);
      printf("      DevString: %s\n",   measCompInventory[i].devString);
      printf("       UniqueID: %s\n",   measCompInventory[i].uniqueId);
      printf("       Reserved: %s\n",   measCompInventory[i].reserved);
    }
  #endif
}

int measCompCreateDevice(std::string uniqueId, DaqDeviceDescriptor& deviceDescriptor, long long *handle)
{
  size_t colon;
  std::string host = uniqueId;
  std::string port;
  int portNum = DEFAULT_DISCOVERY_PORT;
  int status;
  int devIndex=-1;
  double timeout = 1.0;

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
    // See if this host is already known, i.e. it was found on the local subnet
    for (int i=0; i<measCompNumDevices; i++) {
      #ifdef WIN32
        if (host.compare(measCompInventory[i].Reserved) == 0) {
      #else
        if (host.compare(measCompInventory[i].reserved) == 0) {
      #endif
        devIndex = i;
      }
    }
    // We did not find this device on the local subnet, create a new one
    if (devIndex == -1) {
      devIndex = measCompNumDevices++;
    }
    #ifdef WIN32
      status = cbGetNetDeviceDescriptor((char*)host.c_str(), portNum,
                                        &measCompInventory[devIndex], (int) (timeout * 1000));
    #else
      status = ulGetNetDaqDeviceDescriptor((char*)host.c_str(), portNum, NULL,
                                           &measCompInventory[devIndex], timeout);
    #endif
    if (status) {
        printf("Error calling cbGetNetDeviceDescriptor=%d\n", status);
        return -1;
    }
  }
  else {
    // uniqueId was not an IP address so it must be a serial number (USB) or MAC address (Ethernet)
    // Search the inventory for the matching UniqueID
    for (int i=0; i<measCompNumDevices; i++) {
      #ifdef WIN32
        if (uniqueId.compare(measCompInventory[i].UniqueID) == 0) {
      #else
        if (uniqueId.compare(measCompInventory[i].uniqueId) == 0) {
      #endif
        devIndex = i;
        break;
      }
    }
  }
  if (devIndex != -1) {
    #ifdef WIN32
      status = cbCreateDaqDevice(devIndex, measCompInventory[devIndex]);
      if (status) {
        printf("Error calling cbCreateDaqDevice=%d\n", status);
        return -1;
      }
      *handle = devIndex;
    #else
      DaqDeviceHandle devHandle = ulCreateDaqDevice(measCompInventory[devIndex]);
      if (!devHandle) {
        printf("Error calling ulCreateDaqDevice devIndex=%d\n", devIndex);
        return -1;
      }
      *handle = devHandle;
      UlError error = ulConnectDaqDevice(devHandle);
      if (error) {
        printf("Error calling ulConnectDaqDevice error=%d\n", error);
        return -1;
      }
    #endif
  }
  deviceDescriptor = measCompInventory[devIndex];
  return 0;
}

