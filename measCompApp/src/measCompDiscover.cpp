#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <osiSock.h>
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
                std::string tempString = "0" + std::string(measCompInventory[i].UniqueID);
                strcpy(measCompInventory[i].UniqueID, tempString.c_str());
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

/** Create a measComp device.
  * This method finds a device and connects to it.  It is called from the driver constructor.
  * \param[in] uniqueID 
  *                     This is an 8 digital hex serial number for USB devices without a leading 0x.
  *                     For Ethernet devices it can be:
  *                         An IP DNS name with optional discovery port, e.g. gse-e1608-6:54211
  *                         An IP address with optional discovery port, e.g. 10.54.160.63:54211
  *                         A MAC address, e.g. 00:80:2F:24:53:E5
  * \param[out] deviceDescriptor Pointer to a DaqDeviceDescription structure for this device
  * \param[out] handle A handle for this device.  
  *                         On Windows this is the index in the device inventory list.
  *                         On Linux it is a DaqDeviceHandle.
  */
int measCompCreateDevice(std::string uniqueId, DaqDeviceDescriptor& deviceDescriptor, long long *handle)
{
  size_t colon;
  std::string host = uniqueId;
  std::string port;
  struct sockaddr_in ipAddr;
  bool isEthernet = false;
  char *endptr;
  int portNum = DEFAULT_DISCOVERY_PORT;
  int status;
  int devIndex=-1;
  char ipAddrAsString[25];
  double timeout = 1.0;

  measCompDiscoverDevices();
  // If the uniqueId is a hex number it is USB, else Ethernet
  strtol(uniqueId.c_str(), &endptr, 16);
  if (*endptr != '\0') isEthernet = true;

  // Use hostToIPAddr for 2 reasons:
  // - It will translate IP names to IP addresses
  // - It will fail if the uniqueId is a MAC address
  status = aToIPAddr(uniqueId.c_str(), portNum, &ipAddr);
  if (isEthernet && (status == 0)) {
    ipAddrToDottedIP(&ipAddr, ipAddrAsString, sizeof(ipAddrAsString));
    // The string from ipAddrToDottedIP will always have a :port at the end
    // Parse the port and remove from the host string.
    host = ipAddrAsString;
    colon = host.find(":");
    if (colon != std::string::npos) {
      port = host.substr(colon+1, std::string::npos);
      portNum = atoi(port.c_str());
      host = host.substr(0, colon);
    }
    // See if this host is already known, i.e. it was found on the local subnet
    for (int i=0; i<measCompNumDevices; i++) {
      #ifdef WIN32
        /*  This logic does not work on Windows because there is no way to retrieve the IP address and copy it to .Reserved
         *  This is OK, it is just slightly less efficient because it will always call cbGetNetDeviceDescriptor unless UniqueID is a MAC address.
        if (host.compare(measCompInventory[i].Reserved) == 0) {
          devIndex = i;
        } */
      #else
        if (host.compare(measCompInventory[i].reserved) == 0) {
          devIndex = i;
        }
      #endif
    }
    // We did not find this device on the local subnet, create a new one
    if (devIndex == -1) {
      devIndex = measCompNumDevices++;
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
  }
  else {
    // uniqueId was not an IP address or IP DNS name, so it must be a serial number (USB) or MAC address (Ethernet)
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
    //
    deviceDescriptor = measCompInventory[devIndex];
    return 0;
  }
  return -1;
}

