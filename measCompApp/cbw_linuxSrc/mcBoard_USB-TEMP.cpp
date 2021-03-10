#include <stdio.h>

#include <pmd.h>
#include "mcBoard_USB-TEMP.h"

static const char *driverName = "mcUSB_TEMP";

mcUSB_TEMP::mcUSB_TEMP(const char *address)
  : mcBoard(address)
{
    static const char *functionName = "mcUSB_TEMP";
    strcpy(boardName_, "USB-TEMP-AI");
    biBoardType_    = 0;
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

    int ret = hid_init();
    if (ret < 0) {
        printf("%s::%s Failure, hid_init failed with return code %d\n", driverName, functionName, ret);
        return;
    }

   if ((hidDevice_ = hid_open(MCC_VID, USB_TEMP_PID, NULL)) > 0) {
        printf("Success, found a USB-TEMP!\n");
        biBoardType_ = USB_TEMP_PID;
        strcpy(boardName_, "USB-TEMP");
    } else {
        printf("%s::%s Failure, did not find a USB-TEMP series device!\n", driverName, functionName);
        return;
    }

    usbInitCounter_USBTEMP(hidDevice_);
}

int mcUSB_TEMP::cbSetConfig(int InfoType, int DevNum, int ConfigItem, int ConfigVal) {
    static const char *functionName="cbSetConfig";

    switch (InfoType) {
    case BOARDINFO:
        switch (ConfigItem) {
        case BIADCHANTYPE:
            printf("Setting USB_TEMP_SENSOR_TYPE, chan=%d, value=%d\n", DevNum/2, ConfigVal);
            usbSetItem_USBTEMP(hidDevice_, DevNum/2, USB_TEMP_SENSOR_TYPE, ConfigVal);
            if (ConfigVal == USB_TEMP_THERMOCOUPLE) {
                usbSetItem_USBTEMP(hidDevice_, DevNum/2, USB_TEMP_EXCITATION, USB_TEMP_EXCITATION_OFF);
            }
            break;

        case BICHANRTDTYPE:
            printf("Setting USB_TEMP_CONNECTION_TYPE, chan=%d, value=%d\n", DevNum/2, ConfigVal);
            usbSetItem_USBTEMP(hidDevice_, DevNum/2, USB_TEMP_CONNECTION_TYPE, ConfigVal);
            break;

        case BICHANTCTYPE:
            printf("Setting USB_TEMP thermocouple type, chan=%d, value=%d\n", DevNum/2, ConfigVal-1);
            usbSetItem_USBTEMP(hidDevice_, DevNum/2, DevNum%2+USB_TEMP_CH_0_TC, ConfigVal-1);
            //usbCalibrate_USBTEMP(hidDevice_, 0);
            //usbCalibrate_USBTEMP(hidDevice_, 1);
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

int mcUSB_TEMP::cbCIn32(int CounterNum, ULONG *Count)
{
    *Count = usbReadCounter_USBTEMP(hidDevice_);
    return NOERRORS;
}

int mcUSB_TEMP::cbCLoad32(int RegNum, ULONG LoadValue)
{
    usbInitCounter_USBTEMP(hidDevice_);
    return NOERRORS;
}

int mcUSB_TEMP::cbDBitOut(int PortType, int BitNum, USHORT BitValue)
{
    uint8_t value;
    uint8_t mask = 0x1 << BitNum;

    // Read the current output register
    usbDIn_USBTEMP(hidDevice_, &value);
    // Set or clear the bit
    if (BitValue) {
        value |= mask;
    } else {
        value &= ~mask;
    }
    usbDOut_USBTEMP(hidDevice_, value);
    return NOERRORS;
}

int mcUSB_TEMP::cbDConfigBit(int PortType, int BitNum, int Direction)
{
    uint8_t direction;

    if (Direction == DIGITALIN) {
        direction = USB_TEMP_DIO_DIR_IN;
    } else {
        direction = USB_TEMP_DIO_DIR_OUT;
    }
    usbDConfigBit_USBTEMP(hidDevice_, BitNum,  direction);
    return NOERRORS;
}

int mcUSB_TEMP::cbDIn(int PortType, USHORT *DataValue)
{
    uint8_t value;

    usbDIn_USBTEMP(hidDevice_, &value);
    *DataValue = value;
    return NOERRORS;
}

int mcUSB_TEMP::cbTIn(int Chan, int Scale, float *TempValue, int Options)
{
    uint8_t units=0;

    usbTin_USBTEMP(hidDevice_, Chan, units, TempValue);
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
