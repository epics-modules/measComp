#include <stdio.h>
#include <math.h>
#include <unistd.h>

#include <pmd.h>
#include "mcBoard_USB-3100.h"

static const char *driverName = "mcUSB_3100";

mcUSB_3100::mcUSB_3100(const char *address, unsigned int productId, const char *productName)
  : mcBoard(address)
{
    static const char *functionName = "mcUSB_3100";
    strcpy(boardName_, productName);
    biBoardType_ = productId;
    switch (biBoardType_) {
      case USB3101_PID: biNumDACChans_  = 4; break;
      case USB3102_PID: biNumDACChans_  = 4; break;
      case USB3103_PID: biNumDACChans_  = 8; break;
      case USB3104_PID: biNumDACChans_  = 8; break;
      case USB3105_PID: biNumDACChans_  = 16; break;
      case USB3106_PID: biNumDACChans_  = 16; break;
      case USB3110_PID: biNumDACChans_  = 4; break;
      case USB3112_PID: biNumDACChans_  = 8; break;
      case USB3114_PID: biNumDACChans_  = 16; break;
      default: {
        printf("%s::%s Failure, unknown board type %d\n", driverName, functionName, biBoardType_);
        return;
      }
    }

    biDACRes_       = 16;
    biDInNumDevs_   = 1;
    diDevType_      = AUXPORT;
    diNumBits_      = 8;

    int ret = hid_init();
    if (ret < 0) {
        printf("%s::%s Failure, hid_init failed with return code %d\n", driverName, functionName, ret);
        return;
    }

    if ((hidDevice_ = hid_open(MCC_VID, biBoardType_, NULL)) > 0) {
        printf("Success, found a %s!\n", boardName_);
    } else {
        printf("%s::%s Failure, did not find a USB-3100 series device!\n", driverName, functionName);
        return;
    }
}

int mcUSB_3100::cbSetConfig(int InfoType, int DevNum, int ConfigItem, int ConfigVal) {
    switch (InfoType) {
      case BOARDINFO:
        switch (ConfigItem) {
          uint8_t range;
          case BIDACRANGE:
            switch (ConfigVal) {
              case UNI10VOLTS: range = 0; break;
              case BIP10VOLTS: range = 1; break;
              case MA0TO20:    range = 2; break;
              default: {
                printf("mcUSB_3100::cbSetConfig error unsupported range %d\n", ConfigVal);
                return BADRANGE;
              }
            }
            usbAOutConfig_USB31XX(hidDevice_, DevNum, range);
            break;

          default:
            printf("mcUSB_3100::cbSetConfig error unknown ConfigItem %d\n", ConfigItem);
            return BADCONFIGITEM;
            break;
        }
        break;

      default:
        printf("mcUSB_3100::cbSetConfig error unknown InfoType %d\n", InfoType);
        return BADCONFIGTYPE;
        break;
    }
    return NOERRORS;
}

int mcUSB_3100::cbAOut(int Chan, int Gain, USHORT DataValue)
{
    usbAOut_USB31XX(hidDevice_, Chan, DataValue, 0);
    return NOERRORS;
}

int mcUSB_3100::cbCIn32(int CounterNum, ULONG *Count)
{
    uint32_t counts = usbReadCounter_USB31XX(hidDevice_);
    *Count = counts;
    return NOERRORS;
}

int mcUSB_3100::cbCLoad32(int RegNum, ULONG LoadValue)
{
    usbInitCounter_USB31XX(hidDevice_);
    return NOERRORS;
}

int mcUSB_3100::cbDBitOut(int PortType, int BitNum, USHORT BitValue)
{
    usbDBitOut_USB31XX(hidDevice_, BitNum, BitValue);
    return NOERRORS;
}

int mcUSB_3100::cbDConfigBit(int PortType, int BitNum, int Direction)
{
    uint8_t direction;

    if (Direction == DIGITALIN) {
        direction = USB3100_DIO_DIR_IN;
    } else {
        direction = USB3100_DIO_DIR_OUT;
    }
    usbDConfigBit_USB31XX(hidDevice_, BitNum, direction);
    return NOERRORS;
}

int mcUSB_3100::cbDIn(int PortType, USHORT *DataValue)
{
    uint8_t value;
    usbDIn_USB31XX(hidDevice_, &value);
    *DataValue = value;
    return NOERRORS;
}
