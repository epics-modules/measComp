#ifndef mcBoard_USB_CTRInclude
#define mcBoard_USB_CTRInclude

#include <epicsThread.h>
#include <epicsEvent.h>

#include "mcBoard.h"
#include "pmd.h"
#include "usb-ctr.h"

class mcUSB_CTR : mcBoard {
public:
    mcUSB_CTR(const char *address);
/*
    // System functions
    int cbSetConfig(int InfoType, int DevNum, int ConfigItem, int ConfigVal);
    int cbGetIOStatus(short *Status, long *CurCount, long *CurIndex, int FunctionType);
    int cbStopIOBackground(int FunctionType);

*/
    // Digital I/O functions
    int cbDBitOut(int PortType, int BitNum, USHORT BitValue);
    int cbDConfigBit(int PortType, int BitNum, int Direction);

    int cbDIn(int PortType, USHORT *DataValue);
/*
    // Trigger functions
    virtual int cbSetTrigger(int TrigType, USHORT LowThreshold, USHORT HighThreshold);
*/

    // Pulse functions
    int cbPulseOutStart(int TimerNum, double *Frequency, 
                        double *DutyCycle, unsigned int PulseCount, 
                        double *InitialDelay, int IdleState, int Options);
    int cbPulseOutStop(int TimerNum);
/*

    // Counter functions
    int cbCIn32(int CounterNum, ULONG *Count);
    int cbCLoad32(int RegNum, ULONG LoadValue);
    int cbCInScan(int FirstCtr,int LastCtr, LONG Count,
                          LONG *Rate, HGLOBAL MemHandle, ULONG Options);
    int cbCConfigScan(int CounterNum, int Mode,int DebounceTime,
                      int DebounceMode, int EdgeDetection,
                      int TickSize, int MappedChannel);

*/
    void readThread();

private:
    libusb_device_handle *deviceHandle_;
    epicsThreadId readThreadId_;
    epicsMutex readMutex_;
    epicsEventId acquireStartEvent_;

    bool ctrScanAcquiring_;
    bool ctrScanIsScaled_;
    double *ctrScanScaledBuffer_;
    uint16_t *aiScanUnscaledBuffer_;
    int ctrScanTotalElements_;
    int ctrScanCurrentPoint_;
    int ctrScanCurrentIndex_;
    int ctrScanNumChans_;
    int ctrScanTrigType_;

};

#endif /* mcBoard_USB_CTRInclude */

