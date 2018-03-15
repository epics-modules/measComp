#include <stdio.h>

#include "mcBoard_E-1608.h"


mcE_1608::mcE_1608(const char *address)
  : mcBoard(address) 
{
    strcpy(boardName_, "E-1608");
    biBoardType_    = E1608_PID;
    biNumADCChans_  = 8;
    biADCRes_       = 16;
    biNumDACChans_  = 2;
    biDACRes_       = 16;
    biNumTempChans_ = 0;
    biDInNumDevs_   = 1;
    diDevType_      = AUXPORT;
    diInMask_       = 0;
    diOutMask_      = 0;
    diNumBits_      = 8;
    
    // Open Ethernet socket
    deviceInfo_.device.connectCode = 0x0;   // default connect code
    deviceInfo_.device.frameID = 0;         // zero out the frameID
    deviceInfo_.queue[0] = 0;               // set count in gain queue to zero
    deviceInfo_.timeout = 1000;             // set default timeout to 1000 ms.
    deviceInfo_.device.Address.sin_family = AF_INET;
    deviceInfo_.device.Address.sin_port = htons(COMMAND_PORT);
    deviceInfo_.device.Address.sin_addr.s_addr = INADDR_ANY;
    if (inet_pton(AF_INET, address_, &deviceInfo_.device.Address.sin_addr) == 0) {
        printf("Improper destination address.\n");
        return;
    }

    if ((deviceInfo_.device.sock = openDevice(inet_addr(inet_ntoa(deviceInfo_.device.Address.sin_addr)),
			    deviceInfo_.device.connectCode)) < 0) {
        printf("Error opening socket\n");
        return;
    }

}

int mcE_1608::setConfig(int InfoType, int DevNum, int ConfigItem, int ConfigVal) {
    switch (InfoType) {
    case BOARDINFO:
        switch (ConfigItem) {

        default:
            printf("mcBoardE_T-1608::setConfig error unknown ConfigItem %d\n", ConfigItem);
            return BADCONFIGITEM;
            break;
    }
    break;
    
    default:
        printf("mcBoardE_T-1608::setConfig error unknown InfoType %d\n", InfoType);
        return BADCONFIGTYPE;
    break;
    }
    return NOERRORS;
}

int mcE_1608::getIOStatus(short *Status, long *CurCount, long *CurIndex,int FunctionType)
{
    // Needs to be implemented
	  return NOERRORS;
}

int mcE_1608::aIn(int Chan, int Gain, USHORT *DataValue)
{
	  if (!AIn_E1608(&deviceInfo_, Chan, Gain, DataValue)) {
	     return BADBOARD;
	  }
	  return NOERRORS;
}

int mcE_1608::aOut(int Chan, int Gain, USHORT DataValue)
{
	  if (!AOut_E1608(&deviceInfo_, Chan, DataValue)) {
	     return BADBOARD;
	  }
	  return NOERRORS;
}

int mcE_1608::cIn32(int CounterNum, ULONG *Count)
{
    uint32_t counts;
	  if (!CounterR_E1608(&deviceInfo_, &counts)) {
	     return BADBOARD;
	  }
	  *Count = counts;
	  return NOERRORS;
}

int mcE_1608::cLoad32(int RegNum, ULONG LoadValue)
{
	  if (!ResetCounter_E1608(&deviceInfo_)) {
	     return BADBOARD;
	  }
	  return NOERRORS;
}

int mcE_1608::dBitOut(int PortType, int BitNum, USHORT BitValue)
{
    uint8_t value;
    uint8_t mask = 0x1 << BitNum;
    // Read the current output register
    if (!DOutR_E1608(&deviceInfo_, &value)) {
        return BADBOARD;
    }
    // Set or clear the bit
    if (BitValue) {
        value |= mask;
    } else {
        value &= ~mask;
    }
    if (!DOut_E1608(&deviceInfo_, value)) {
        return BADBOARD;
    }
    return NOERRORS;
}

int mcE_1608::dConfigBit(int PortType, int BitNum, int Direction)
{
    uint8_t value;
    uint8_t mask = 0x1 << BitNum;
    // Read the current output register
    if (!DConfigR_E1608(&deviceInfo_, &value)) {
        return BADBOARD;
    }
    // Set or clear the bit
    if (Direction == DIGITALIN) {
        value |= mask;
    } else {
        value &= ~mask;
    }
    if (!DConfigW_E1608(&deviceInfo_, value)) {
        return BADBOARD;
    }
    return NOERRORS;
}

int mcE_1608::dIn(int PortType, USHORT *DataValue)
{
    uint8_t value;
    if (!DIn_E1608(&deviceInfo_, &value)) {
        return BADBOARD;
    }
    *DataValue = value;
    return NOERRORS;
}
