#ifndef mcBoard_USB_TEMPInclude
#define mcBoard_USB_TEMPInclude

#include "mcBoard.h"
#include "usb-temp.h"

class mcUSB_TEMP : mcBoard {
public:
    mcUSB_TEMP(const char *address);

    // System functions
    int cbSetConfig(int InfoType, int DevNum, int ConfigItem, int ConfigVal);

    // Counter functions
    int cbCIn32(int CounterNum, ULONG *Count);
    int cbCLoad32(int RegNum, ULONG LoadValue);

    // Digital I/O functions
    int cbDBitOut(int PortType, int BitNum, USHORT BitValue);
    int cbDConfigBit(int PortType, int BitNum, int Direction);
    int cbDIn(int PortType, USHORT *DataValue);

    // Temperature input functions
    int cbTIn(int Chan, int Scale, float *TempValue, int Options);

private:
    hid_device *hidDevice_;
};

#endif /* mcBoard_USB_TEMPInclude */

