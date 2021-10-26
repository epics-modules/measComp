#ifndef mcBoard_USB_3100Include
#define mcBoard_USB_3100Include

#include <epicsThread.h>
#include <epicsEvent.h>

#include "mcBoard.h"
#include "usb-3100.h"

class mcUSB_3100 : mcBoard {
public:
    mcUSB_3100(const char *address, unsigned int productId, const char *productName);
    // System functions
    int cbSetConfig(int InfoType, int DevNum, int ConfigItem, int ConfigVal);

    // Analog I/O functions
    int cbAOut(int Chan, int Gain, USHORT DataValue);

    // Counter functions
    int cbCIn32(int CounterNum, ULONG *Count);
    int cbCLoad32(int RegNum, ULONG LoadValue);

    // Digital I/O functions
    int cbDBitOut(int PortType, int BitNum, USHORT BitValue);
    int cbDConfigBit(int PortType, int BitNum, int Direction);
    int cbDIn(int PortType, USHORT *DataValue);

private:
    hid_device *hidDevice_;

};

#endif /* mcBoard_USB_3100Include */

