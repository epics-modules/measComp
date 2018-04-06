#include <stdio.h>

#include "mcBoard_E-TC.h"


mcE_TC::mcE_TC(const char *address)
  : mcBoard(address) 
{
    strcpy(boardName_, "E-TC");
    biBoardType_    = ETC_PID;
    biNumADCChans_  = 0;
    biADCRes_       = 0;
    biNumDACChans_  = 0;
    biDACRes_       = 0;
    biNumTempChans_ = 8;
    biDInNumDevs_   = 1;
    diDevType_      = AUXPORT;
    diInMask_       = 0;
    diOutMask_      = 0;
    diNumBits_      = 8;
    
    // Open Ethernet socket
    deviceInfo_.device.connectCode = 0x0;   // default connect code
    deviceInfo_.device.frameID = 0;         // zero out the frameID
    deviceInfo_.device.Address.sin_family = AF_INET;
    deviceInfo_.device.Address.sin_port = htons(COMMAND_PORT);
    deviceInfo_.device.Address.sin_addr.s_addr = INADDR_ANY;
    if (inet_aton(address_, &deviceInfo_.device.Address.sin_addr) == 0) {
        printf("Improper destination address.\n");
        return;
    }

    if ((deviceInfo_.device.sock = openDevice(inet_addr(inet_ntoa(deviceInfo_.device.Address.sin_addr)),
                                              deviceInfo_.device.connectCode)) < 0) {
        printf("Error opening socket\n");
        return;
    }

    deviceInfo_.units = CELSIUS;
    deviceInfo_.wait = 0x0;
    for (int i = 0; i< 8; i++) {
        deviceInfo_.config_values[i] = 0x0;   // disable all channels
    }
}

int mcE_TC::cbSetConfig(int InfoType, int DevNum, int ConfigItem, int ConfigVal) {
    switch (InfoType) {
    case BOARDINFO:
        switch (ConfigItem) {
        case BICHANTCTYPE:
            deviceInfo_.config_values[DevNum] = ConfigVal;
            TinConfigW_E_TC(&deviceInfo_);
            break;        

        default:
            printf("mcBoardE_T-TC::setConfig error unknown ConfigItem %d\n", ConfigItem);
            return BADCONFIGITEM;
            break;
    }
    break;
    
    default:
        printf("mcBoardE_T-TC::setConfig error unknown InfoType %d\n", InfoType);
        return BADCONFIGTYPE;
    break;
    }
    return NOERRORS;
}

int mcE_TC::cbCIn32(int CounterNum, ULONG *Count)
{
      if (!CounterR_E_TC(&deviceInfo_)) {
         return BADBOARD;
      }
      *Count = deviceInfo_.counter;
      return NOERRORS;
}

int mcE_TC::cbCLoad32(int RegNum, ULONG LoadValue)
{
      if (!ResetCounter_E_TC(&deviceInfo_)) {
         return BADBOARD;
      }
      return NOERRORS;
}

int mcE_TC::cbDBitOut(int PortType, int BitNum, USHORT BitValue)
{
    uint8_t value;
    uint8_t mask = 0x1 << BitNum;
    // Read the current output register
    if (!DOutR_E_TC(&deviceInfo_, &value)) {
        return BADBOARD;
    }
    // Set or clear the bit
    if (BitValue) {
        value |= mask;
    } else {
        value &= ~mask;
    }
    if (!DOutW_E_TC(&deviceInfo_, value)) {
        return BADBOARD;
    }
    return NOERRORS;
}

int mcE_TC::cbDConfigBit(int PortType, int BitNum, int Direction)
{
    uint8_t value;
    uint8_t mask = 0x1 << BitNum;
    // Read the current output register
    if (!DConfigR_E_TC(&deviceInfo_, &value)) {
        return BADBOARD;
    }
    // Set or clear the bit
    if (Direction == DIGITALIN) {
        value |= mask;
    } else {
        value &= ~mask;
    }
    if (!DConfigW_E_TC(&deviceInfo_, value)) {
        return BADBOARD;
    }
    return NOERRORS;
}

int mcE_TC::cbDIn(int PortType, USHORT *DataValue)
{
    uint8_t value;
    if (!DIn_E_TC(&deviceInfo_, &value)) {
        return BADBOARD;
    }
    *DataValue = value;
    return NOERRORS;
}

int mcE_TC::cbTIn(int Chan, int Scale, float *TempValue, int Options)
{
    uint8_t channelMask = 0x1 << Chan;
    uint8_t units;
    switch (Scale) {
    case VOLTS:
        units = VOLTAGE;
        break;
    case NOSCALE:
        units = ADC_CODE;
        break;
    default:
        units = CELSIUS;
        break;
    }
    if (!Tin_E_TC(&deviceInfo_, channelMask, units, 0, TempValue)) {
        return BADBOARD;
    }
    switch (Scale) {
    case KELVIN:
        *TempValue += 273.15;
        break;
    case FAHRENHEIT:
        *TempValue = (*TempValue * 1.8) + 32;
        break;
    default:
        break;
    }
    return NOERRORS;
}
