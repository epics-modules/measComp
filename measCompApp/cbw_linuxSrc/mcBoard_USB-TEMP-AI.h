#ifndef mcBoard_USB_TEMP_AIInclude
#define mcBoard_USB_TEMP_AIInclude

#include "mcBoard.h"
#include "usb-tc-ai.h"

class mcUSB_TEMP_AI : mcBoard {
public:
    mcUSB_TEMP_AI(const char *address);

    // System functions
    int cbSetConfig(int InfoType, int DevNum, int ConfigItem, int ConfigVal);

    // Counter functions
    int cbCIn32(int CounterNum, ULONG *Count);
    int cbCLoad32(int RegNum, ULONG LoadValue);

    // Digital I/O functions
    int cbDBitOut(int PortType, int BitNum, USHORT BitValue);
    int cbDConfigBit(int PortType, int BitNum, int Direction);
    int cbDIn(int PortType, USHORT *DataValue);

    // Voltage input functions
    int cbTIn(int Chan, int Scale, float *TempValue, int Options);

    // Voltage input functions
    int cbVIn(int Chan, int Range, float *DataValue, int Options);

private:
    //DeviceInfo_TC deviceInfo_;
};

#endif /* mcBoard_USB_TEMP_AIInclude */

