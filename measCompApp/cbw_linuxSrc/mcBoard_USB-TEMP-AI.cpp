#include <stdio.h>

#include <pmd.h>
#include "mcBoard_USB-TEMP-AI.h"

static const char *driverName = "mcUSB_TEMP-AI";

mcUSB_TEMP_AI::mcUSB_TEMP_AI(const char *address)
  : mcBoard(address) 
{
    static const char *functionName = "mcUSB_TEMP_AI";
    strcpy(boardName_, "USB-TEMP-AI");
    biBoardType_    = 0;
    biNumADCChans_  = 4;
    biADCRes_       = 0;
    biNumDACChans_  = 0;
    biDACRes_       = 0;
    biNumTempChans_ = 4;
    biDInNumDevs_   = 1;
    diDevType_      = AUXPORT;
    diInMask_       = 0;
    diOutMask_      = 0;
    diNumBits_      = 8;

    int ret = hid_init();
    if (ret < 0) {
        printf("%s::%s Failure, hid_init failed with return code %d\n", driverName, functionName, ret);
        return;
    }

   if ((hidDevice_ = hid_open(MCC_VID, USB_TEMP_AI_PID, NULL)) > 0) {
        printf("Success, found a USB-TEMP-AI!\n");
        biBoardType_ = USB_TEMP_AI_PID;
        strcpy(boardName_, "USB-TEMP-AI");
    } else {
        printf("%s::%s Failure, did not find a USB-TEMP-AI series device!\n", driverName, functionName);
        return;
    }
    
    usbInitCounter_USBTC_AI(hidDevice_);
}

int mcUSB_TEMP_AI::cbSetConfig(int InfoType, int DevNum, int ConfigItem, int ConfigVal) {
    static const char *functionName="cbSetConfig";

    switch (InfoType) {
    case BOARDINFO:
        switch (ConfigItem) {
        case BIADCHANTYPE:
            usbSetItem_USBTEMP_AI(hidDevice_, DevNum/2, USB_TEMP_AI_SENSOR_TYPE, ConfigVal);
            switch (ConfigVal) {
            case USB_TEMP_AI_RTD:            
	              usbSetItem_USBTEMP_AI(hidDevice_, DevNum/2, USB_TEMP_AI_EXCITATION, USB_TEMP_AI_MU_A_210);
                break;
            case USB_TEMP_AI_THERMISTOR:            
	              usbSetItem_USBTEMP_AI(hidDevice_, DevNum/2, USB_TEMP_AI_EXCITATION, USB_TEMP_AI_MU_A_10);
                break;
            case USB_TEMP_AI_THERMOCOUPLE:
                usbSetItem_USBTEMP_AI(hidDevice_, DevNum/2, USB_TEMP_AI_EXCITATION, USB_TEMP_AI_EXCITATION_OFF);
                break;
            case USB_TEMP_AI_SEMICONDUCTOR:
                usbSetItem_USBTEMP_AI(hidDevice_, DevNum/2, USB_TEMP_AI_EXCITATION, USB_TEMP_AI_EXCITATION_OFF);
                break;
            case USB_TEMP_AI_DISABLED:
                 break;
            case USB_TEMP_AI_VOLTAGE:
                break;
            default:
                printf("%s::%s error unknown ConfigVal %d\n", driverName, functionName, ConfigVal);
                return BADCONFIGITEM;
                break;
            }        

        case BICHANRTDTYPE:
            usbSetItem_USBTEMP_AI(hidDevice_, DevNum/2, USB_TEMP_AI_CONNECTION_TYPE, ConfigVal);
            break;        

        case BICHANTCTYPE:
            usbSetItem_USBTEMP_AI(hidDevice_, DevNum/2, DevNum%2+USB_TEMP_AI_CH_0_TC, ConfigVal-1);
            //usbCalibrate_USBTC_AI(hidDevice_, 0);
            //usbCalibrate_USBTC_AI(hidDevice_, 1);
            break;        

        default:
            printf("%s::%s error unknown ConfigItem %d\n", driverName, functionName, ConfigItem);
            return BADCONFIGITEM;
            break;
    }
    break;
    
    default:
        printf("%s::%s error unknown InfoType %d\n", driverName, functionName, InfoType);
        return BADCONFIGTYPE;
    break;
    }
    return NOERRORS;
}

int mcUSB_TEMP_AI::cbCIn32(int CounterNum, ULONG *Count)
{
    *Count = usbReadCounter_USBTC_AI(hidDevice_);
    return NOERRORS;
}

int mcUSB_TEMP_AI::cbCLoad32(int RegNum, ULONG LoadValue)
{
    usbInitCounter_USBTC_AI(hidDevice_);
    return NOERRORS;
}

int mcUSB_TEMP_AI::cbDBitOut(int PortType, int BitNum, USHORT BitValue)
{
    uint8_t value;
    uint8_t mask = 0x1 << BitNum;

    // Read the current output register
    usbDIn_USBTC_AI(hidDevice_, &value);
    // Set or clear the bit
    if (BitValue) {
        value |= mask;
    } else {
        value &= ~mask;
    }
    usbDOut_USBTC_AI(hidDevice_, value);
    return NOERRORS;
}

int mcUSB_TEMP_AI::cbDConfigBit(int PortType, int BitNum, int Direction)
{
    uint8_t direction;

    if (Direction == DIGITALIN) {
        direction = USB_TEMP_AI_DIO_DIR_IN;
    } else {
        direction = USB_TEMP_AI_DIO_DIR_OUT;
    }
    usbDConfigBit_USBTC_AI(hidDevice_, BitNum,  direction);
    return NOERRORS;
}

int mcUSB_TEMP_AI::cbDIn(int PortType, USHORT *DataValue)
{
    uint8_t value;

    usbDIn_USBTC_AI(hidDevice_, &value);
    *DataValue = value;
    return NOERRORS;
}

int mcUSB_TEMP_AI::cbTIn(int Chan, int Scale, float *TempValue, int Options)
{
    uint8_t units=0;

    usbAin_USBTC_AI(hidDevice_, Chan, units, TempValue);
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

static int RangeToGain(int Range)
{
    switch (Range) {
      case BIP1PT25VOLTS:
        return USB_TEMP_AI_BP_1_25V;
      case BIP2PT5VOLTS:
        return USB_TEMP_AI_BP_2_5V;
      case BIP5VOLTS:
        return USB_TEMP_AI_BP_5V;
      case BIP10VOLTS:
        return USB_TEMP_AI_BP_10V;
      default:
        printf("Unsupported Range=%d\n", Range);
        return BADRANGE;
    }
}

int mcUSB_TEMP_AI::cbVIn(int Chan, int Range, float *DataValue, int Options)
{
    int gain = RangeToGain(Range);
    int units = 0;

    if (gain == BADRANGE) {
        printf("Unsupported Range=%d\n", Range);
        return BADRANGE;
    }

    Chan = Chan + 4; // Convert from 0-3 to 4-7 because voltages are last 7 channels.
	  usbSetItem_USBTEMP_AI(hidDevice_, Chan/2, Chan%2+USB_TEMP_AI_CH_0_GAIN, gain);
    usbAin_USBTC_AI(hidDevice_, Chan, units, DataValue);
    return NOERRORS;
}
