#include <stdio.h>
#include <vector>
#include <string>

#include "cbw_linux.h"
#include "pmd.h"
#include "mcBoard.h"
#include "mcBoard_E-TC.h"
#include "mcBoard_E-TC32.h"
#include "mcBoard_E-1608.h"
#include "mcBoard_E-DIO24.h"
#include "mcBoard_USB-CTR.h"
#include "mcBoard_USB-TEMP-AI.h"
#include "mcBoard_USB-TEMP.h"


std::vector<mcBoard*> boardList;

#define MAX_DEVICES 100

// System functions

// Copy information from an EthernetDeviceInfo to a DaqDevuceDescriptor
static void copyEDIToDDD(EthernetDeviceInfo *pEDI, DaqDeviceDescriptor *pDDD)
{
  pDDD->ProductID = pEDI->ProductID;
  strcpy(pDDD->DevString, pEDI->NetBIOS_Name);
  std::string productName = pDDD->DevString;
  productName = productName.substr(0, productName.rfind("-"));
  strcpy(pDDD->ProductName, productName.c_str());
  pDDD->InterfaceType = ETHERNET_IFC;
  sprintf(pDDD->UniqueID, "%.2X:%.2X:%.2X:%.2X:%.2X:%.2X", 
          pEDI->MAC[0], pEDI->MAC[1], pEDI->MAC[2],
          pEDI->MAC[3], pEDI->MAC[4], pEDI->MAC[5]);
  pDDD->NUID = 0;
  for (int j=0; j<6; j++) {
    pDDD->NUID += (ULONGLONG)(pEDI->MAC[j]) << 8*(5-j);
  }
  int addr=pEDI->Address.sin_addr.s_addr;
  sprintf(pDDD->Reserved, "%d.%d.%d.%d", addr&0xFF, addr>>8&0xFF, addr>>16&0xFF, addr>>24&0xFF);
}

int cbGetDaqDeviceInventory(DaqDeviceInterface InterfaceType, DaqDeviceDescriptor* Inventory, INT* NumberOfDevices)
{
  *NumberOfDevices = 0;

  // First search for USB devices if requested
  if ((InterfaceType == ANY_IFC) || (InterfaceType == USB_IFC)) {
    int vendorId = MCC_VID;
    struct libusb_device_handle *udev = NULL;
    struct libusb_device_descriptor desc;
    struct libusb_device **list;
    struct libusb_device *found = NULL;
    struct libusb_device *device;
    char serial[9];
    ssize_t cnt = 0;
    ssize_t i = 0;
    int err = 0;

    err = libusb_init(NULL);
    if (err < 0) {
      perror("libusb_init returned error");
      goto usb_done;
    }
 
    // discover devices
    cnt = libusb_get_device_list(NULL, &list);
   
    for (i = 0; i < cnt; i++) {
      device = list[i];
      err = libusb_get_device_descriptor(device, &desc);
      if (err < 0) {
        perror("usb_device_find_USB_MCC: Can not get USB device descriptor");
        goto usb_done;
      }
      if (desc.idVendor == vendorId) {
        found = device;
        err = libusb_open(found, &udev);
        if (err < 0) {
	         perror("usb_device_find_USB_MCC: libusb_open failed.");
	         udev = NULL;
	         continue;
        }
        err = libusb_kernel_driver_active(udev, 0);
        if (err == 1) {
          /* 
            device active by another driver. (like HID).  This can be dangerous,
            as we don't know if the kenel has claimed the interface or another
            process.  No easy way to tell at this moment, but HID devices won't
            work otherwise.
          */
          #if defined(LIBUSB_API_VERSION) && (LIBUSB_API_VERSION >= 0x01000103)
            libusb_set_auto_detach_kernel_driver(udev, 1);
          #else
            libusb_detach_kernel_driver(udev, 0);
          #endif
        }
        err = libusb_claim_interface(udev, 0);
        if (err < 0) {
          perror("error claiming interface 0");
          libusb_close(udev);
          udev = NULL;
          continue;
        }
        err = libusb_get_string_descriptor_ascii(udev, desc.iSerialNumber, (unsigned char *) serial, sizeof(serial));
        if (err < 0) {
          perror("usb_device_find_USB_MCC: Error reading serial number for device.");
          libusb_release_interface(udev, 0);
          libusb_close(udev);
          udev = NULL;
          continue;
        }
        // We found a device and got the serial number. Add it to the inventory.
        DaqDeviceDescriptor* pDevice = Inventory + (*NumberOfDevices)++;
        pDevice->ProductID = desc.idProduct;
        pDevice->InterfaceType = USB_IFC;
        err = libusb_get_string_descriptor_ascii(udev, desc.iSerialNumber, 
                                                 (unsigned char*)pDevice->UniqueID, sizeof(pDevice->UniqueID));
        err = libusb_get_string_descriptor_ascii(udev, desc.iProduct, 
                                                 (unsigned char*)pDevice->ProductName, sizeof(pDevice->ProductName));
        strcpy(pDevice->DevString, pDevice->ProductName);
        pDevice->NUID = strtol(pDevice->UniqueID, NULL, 16);      
        libusb_release_interface(udev, 0);
        libusb_close(udev);
        udev = NULL;
      } 
    }
    usb_done:
    libusb_free_device_list(list,1);
    libusb_exit(NULL);
  }

  // Now search for Ethernet devices
  if ((InterfaceType == ANY_IFC) || (InterfaceType == ETHERNET_IFC)) {
    int i;
    EthernetDeviceInfo *deviceInfo[MAX_DEVICES];

    for (i=0; i<MAX_DEVICES; i++) {
      deviceInfo[i] = (EthernetDeviceInfo *)malloc(sizeof(EthernetDeviceInfo));
    }

    int numFound = discoverDevices(deviceInfo, 0, MAX_DEVICES);
    if (numFound < 0) {
       perror("Error calling discoverDevices");
       return -1;
    }
    for (i=0; i<numFound; i++) {
      DaqDeviceDescriptor* pDevice = Inventory + (*NumberOfDevices)++;
      copyEDIToDDD(deviceInfo[i], pDevice);
    }
  }
  return 0;
}

int cbIgnoreInstaCal()
{
  return 0;
}

int cbCreateDaqDevice(int BoardNum, DaqDeviceDescriptor deviceDescriptor)
{
    mcBoard *pBoard;
    if (strcmp(deviceDescriptor.ProductName, "E-TC") ==0) {
        pBoard = (mcBoard *)new mcE_TC(deviceDescriptor.Reserved);
    }
    else if (strcmp(deviceDescriptor.ProductName, "E-TC32") ==0) {
        pBoard = (mcBoard *)new mcE_TC32(deviceDescriptor.Reserved);
    }
    else if (strcmp(deviceDescriptor.ProductName, "E-1608") ==0) {
        pBoard = (mcBoard *)new mcE_1608(deviceDescriptor.Reserved);
    }
    else if (strcmp(deviceDescriptor.ProductName, "E-DIO24") ==0) {
        pBoard = (mcBoard *)new mcE_DIO24(deviceDescriptor.Reserved);
    }
    else if (strcmp(deviceDescriptor.ProductName, "USB-CTR") ==0) {
        pBoard = (mcBoard *)new mcUSB_CTR(deviceDescriptor.UniqueID);
    }
    else if (strcmp(deviceDescriptor.ProductName, "USB-TEMP-AI") ==0) {
        pBoard = (mcBoard *)new mcUSB_TEMP_AI(deviceDescriptor.UniqueID);
    }
    else {
        printf("Unknown board type %s\n", deviceDescriptor.ProductName);
        return BADBOARD;
    }
    boardList[BoardNum] = pBoard;
    return NOERRORS;
}

int cbGetNetDeviceDescriptor(char* Host, int Port, DaqDeviceDescriptor* DeviceDescriptor, int Timeout)
{
    EthernetDeviceInfo deviceInfo;

    int numFound = discoverRemoteDevice(Host, &deviceInfo, 0);
    if (numFound != 1) {
       printf("Could not find device %s", Host);
       return -1;
    }
    copyEDIToDDD(&deviceInfo, DeviceDescriptor);
    return 0;
}

int cbGetConfig(int InfoType, int BoardNum, int DevNum, 
                int ConfigItem, int *ConfigVal)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbGetConfig(InfoType, DevNum, ConfigItem, ConfigVal);
}

int cbSetConfig(int InfoType, int BoardNum, int DevNum, int ConfigItem, int ConfigVal)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbSetConfig(InfoType, DevNum, ConfigItem, ConfigVal);
}

int cbGetBoardName(int BoardNum, char *BoardName)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbGetBoardName(BoardName);
}

int cbGetErrMsg(int ErrCode, char *ErrMsg)
{
    static const char *functionName = "cbGetErrMsg";
    printf("Function %s not supported\n", functionName);
    return NOERRORS;
}

int cbSetAsynUser(int BoardNum, asynUser *pasynUser)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbSetAsynUser(pasynUser);
}

HGLOBAL cbWinBufAlloc(long NumPoints)
{
    return calloc(NumPoints, sizeof(short));
}

HGLOBAL cbScaledWinBufAlloc(long NumPoints)
{
    return calloc(NumPoints, sizeof(double));
}

HGLOBAL cbWinBufAlloc32(long NumPoints)
{
    return calloc(NumPoints, sizeof(int));
}

int cbStopIOBackground(int BoardNum, int FunctionType)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbStopIOBackground(FunctionType);
}

int cbGetIOStatus(int BoardNum, short *Status, long *CurCount, long *CurIndex,int FunctionType)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbGetIOStatus(Status, CurCount, CurIndex, FunctionType);
}

      
int cbAIn(int BoardNum, int Chan, int Gain, USHORT *DataValue)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbAIn(Chan, Gain, DataValue);
}

int cbAIn32(int BoardNum, int Chan, int Gain, ULONG *DataValue, int Options)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbAIn32(Chan, Gain, DataValue, Options);
}

int cbAInScan(int BoardNum, int LowChan, int HighChan, long Count,
              long *Rate, int Gain, HGLOBAL MemHandle, int Options)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbAInScan(LowChan, HighChan, Count, Rate, Gain, MemHandle, Options);
}

int cbALoadQueue(int BoardNum, short *ChanArray, short *GainArray, int NumChans)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbALoadQueue(ChanArray, GainArray, NumChans);
}

int cbAOut(int BoardNum, int Chan, int Gain, USHORT DataValue)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbAOut(Chan, Gain, DataValue);
}

int cbAOutScan(int BoardNum, int LowChan, int HighChan, 
               long Count, long *Rate, int Gain, 
               HGLOBAL MemHandle, int Options)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbAOutScan(LowChan, HighChan, Count, Rate, Gain, MemHandle, Options);
}


// Counter functions
int cbC9513Config(int BoardNum, int CounterNum, int GateControl,
                  int CounterEdge, int CountSource, 
                  int SpecialGate, int Reload, int RecycleMode, 
                  int BCDMode, int CountDirection, 
                  int OutputControl)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbC9513Config(CounterNum, GateControl, CounterEdge, CountSource, SpecialGate, 
                                 Reload, RecycleMode, BCDMode, CountDirection, OutputControl);
}
                                
int cbC9513Init(int BoardNum, int ChipNum, int FOutDivider, 
                int FOutSource, int Compare1, int Compare2, 
                int TimeOfDay)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbC9513Init(ChipNum, FOutDivider, FOutSource, Compare1, Compare2, TimeOfDay);
}

int cbCIn32(int BoardNum, int CounterNum, ULONG *Count)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbCIn32(CounterNum, Count);
}

int cbCLoad32(int BoardNum, int RegNum, ULONG LoadValue)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbCLoad32(RegNum, LoadValue);
}

int cbCInScan(int BoardNum, int FirstCtr,int LastCtr, LONG Count,
              LONG *Rate, HGLOBAL MemHandle, ULONG Options)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbCInScan(FirstCtr, LastCtr, Count, Rate, MemHandle, Options);
}

int cbCConfigScan(int BoardNum, int CounterNum, int Mode,int DebounceTime,
                  int DebounceMode, int EdgeDetection,
                  int TickSize, int MappedChannel)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbCConfigScan(CounterNum, Mode, DebounceTime, DebounceMode, 
                                 EdgeDetection, TickSize, MappedChannel);
}


// Digital I/O functions
int cbDBitOut(int BoardNum, int PortType, int BitNum, USHORT BitValue)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbDBitOut(PortType, BitNum, BitValue);
}

int cbDConfigPort(int BoardNum, int PortType, int Direction)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbDConfigPort(PortType, Direction);
}

int cbDConfigBit(int BoardNum, int PortType, int BitNum, int Direction)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbDConfigBit(PortType, BitNum, Direction);
}

int cbDIn(int BoardNum, int PortType, USHORT *DataValue)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbDIn(PortType, DataValue);
}

int cbDIn32(int BoardNum, int PortType, UINT *DataValue)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbDIn32(PortType, DataValue);
}

int cbDOut(int BoardNum, int PortType, USHORT DataValue)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbDOut(PortType, DataValue);
}

int cbDOut32(int BoardNum, int PortType, UINT DataValue)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbDOut32(PortType, DataValue);
}

// Pulse functions
int cbPulseOutStart(int BoardNum, int TimerNum, double *Frequency, 
                    double *DutyCycle, unsigned int PulseCount, 
                    double *InitialDelay, int IdleState, int Options)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbPulseOutStart(TimerNum, Frequency, DutyCycle, PulseCount, 
                                   InitialDelay, IdleState, Options);
}
                    
int cbPulseOutStop(int BoardNum, int TimerNum)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbPulseOutStop(TimerNum);
}


// Temperature functions
int cbTIn(int BoardNum, int Chan, int Scale, float *TempValue, int Options)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbTIn(Chan, Scale, TempValue, Options);
}

// Voltage functions
int cbVIn(int BoardNum, int Chan, int Range, float *DataValue, int Options)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbVIn(Chan, Range, DataValue, Options);
}

// Trigger functions
int cbSetTrigger(int BoardNum, int TrigType, USHORT LowThreshold, 
                 USHORT HighThreshold)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbSetTrigger(TrigType, LowThreshold, HighThreshold);
}

// Daq functions
int cbDaqInScan(int BoardNum, short *ChanArray, short *ChanTypeArray, short *GainArray, int ChanCount, long *Rate,
                long *PretrigCount, long *TotalCount, HGLOBAL MemHandle, int Options)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbDaqInScan(ChanArray, ChanTypeArray, GainArray, ChanCount, Rate,
							                 PretrigCount, TotalCount, MemHandle, Options);
}

int cbDaqSetTrigger(int BoardNum, int TrigSource, int TrigSense, int TrigChan, int ChanType, 
                    int Gain, float Level, float Variance, int TrigEvent)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbDaqSetTrigger(TrigSource, TrigSense, TrigChan, ChanType, 
							                     Gain, Level, Variance, TrigEvent);
}