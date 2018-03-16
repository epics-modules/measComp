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
    buildGainTableAIn_E1608(&deviceInfo_);
    buildGainTableAOut_E1608(&deviceInfo_);
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

static int gainToRange(int Gain)
{
    switch (Gain) {
      case BIP1VOLTS:
        return BP_1V;
      case BIP2VOLTS:
        return BP_2V;
      case BIP5VOLTS:
        return BP_5V;
      case BIP10VOLTS:
        return BP_10V;
      default:
        printf("Unsupported Gain=%d\n", Gain);
        return BADRANGE;
    }
}
int mcE_1608::aIn(int Chan, int Gain, USHORT *DataValue)
{
    uint16_t value;
    int range = gainToRange(Gain);
    if (range == BADRANGE) {
        printf("Unsupported Gain=%d\n", Gain);
        return BADRANGE;
    }
    if (!AIn_E1608(&deviceInfo_, Chan, (uint8_t)range, &value)) {
       return BADBOARD;
    }
    *DataValue = value;
    return NOERRORS;
}

int mcE_1608::aInScan(int LowChan, int HighChan, long Count, long *Rate, 
                      int Gain, HGLOBAL MemHandle, int Options)
{
    uint8_t options = 0x0;
    // We assume that aLoadQueue has previously been called and deviceInfo_.queue[0] contains number of channels
    // being scanned.
    double frequency = *Rate * deviceInfo_.queue[0];
printf("mcE_1608::aInScan Count=%ld, frequency=%f, Options=0x%x\n", Count, frequency, Options);
    if (!AInScanStart_E1608(&deviceInfo_, (uint32_t)Count, frequency, options)) {
        printf("Error calling AInStartScan_E1608\n");
        return BADBOARD;
    }
    return NOERRORS;
}

int mcE_1608::aLoadQueue(short *ChanArray, short *GainArray, int NumChans)
{
    deviceInfo_.queue[0] = NumChans;
    int range;
    for (int i=0; i<NumChans; i++) {
        deviceInfo_.queue[2*i+1] = ChanArray[i];
        range = gainToRange(GainArray[i]);
        if (range == BADRANGE) {
            printf("Unsupported Gain=%d\n", GainArray[i]);
            return BADRANGE;
        }
        deviceInfo_.queue[2*i+2] = (uint8_t)range;    
    }
    if (!AInQueueW_E1608(&deviceInfo_)) {
        printf("Error calling AInQueueW_E1608\n");
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
