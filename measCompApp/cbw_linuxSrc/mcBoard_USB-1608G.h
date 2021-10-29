#ifndef mcBoard_USB_1608Include
#define mcBoard_USB_1608Include

#include <epicsThread.h>
#include <epicsEvent.h>

#include "mcBoard.h"
#include "usb-1608G.h"

class mcUSB1608G : mcBoard {
public:
    mcUSB1608G(const char *address);
    // System functions
    int cbSetConfig(int InfoType, int DevNum, int ConfigItem, int ConfigVal);
    int cbGetIOStatus(short *Status, long *CurCount, long *CurIndex, int FunctionType);
    int cbStopIOBackground(int FunctionType);

    // Analog I/O functions
    int cbAIn(int Chan, int Gain, USHORT *DataValue);
    int cbAInScan(int LowChan, int HighChan, long Count, long *Rate,
                int Gain, HGLOBAL MemHandle, int Options);
    int cbAInputMode(int InputMode);
    int cbALoadQueue(short *ChanArray, short *GainArray, int NumChans);
    int cbAOut(int Chan, int Gain, USHORT DataValue);

    // Counter functions
    int cbCIn32(int CounterNum, ULONG *Count);
    int cbCLoad32(int RegNum, ULONG LoadValue);

    // Digital I/O functions
    int cbDBitOut(int PortType, int BitNum, USHORT BitValue);
    int cbDConfigBit(int PortType, int BitNum, int Direction);
    int cbDIn(int PortType, USHORT *DataValue);

    // Trigger functions
    virtual int cbSetTrigger(int TrigType, USHORT LowThreshold, USHORT HighThreshold);

    void readThread();

private:
	libusb_device_handle *udev_ = NULL;
    usbDevice1608G deviceInfo_;
    epicsThreadId readThreadId_;
    epicsMutex readMutex_;
    epicsEventId acquireStartEvent_;

    bool aiScanAcquiring_;
    bool aiScanIsScaled_;
    double *aiScanScaledBuffer_;
    uint16_t *aiScanUnscaledBuffer_;
    int aiScanTotalElements_;
    int aiScanCurrentPoint_;
    int aiScanCurrentIndex_;
    int aiScanNumChans_;
    int aiScanTrigType_;
    int aInputMode_;
    int aiScanTrigCount_;  // Note currently used.  Implement for ReTrigger capability.
};

#endif /* mcBoard_USB_1608Include */

