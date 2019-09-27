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


    // Scan functions
    int cbGetIOStatus(short *Status, long *CurCount, long *CurIndex, int FunctionType);
    int cbStopIOBackground(int FunctionType);

    // Digital I/O functions
    int cbDBitOut(int PortType, int BitNum, USHORT BitValue);
    int cbDConfigBit(int PortType, int BitNum, int Direction);
    int cbDIn(int PortType, USHORT *DataValue);

    // Trigger functions
    int cbSetTrigger(int TrigType, USHORT LowThreshold, USHORT HighThreshold);

    // Pulse functions
    int cbPulseOutStart(int TimerNum, double *Frequency, 
                        double *DutyCycle, unsigned int PulseCount, 
                        double *InitialDelay, int IdleState, int Options);
    int cbPulseOutStop(int TimerNum);


    // Counter functions
    int cbCIn32(int CounterNum, ULONG *Count);
    int cbCLoad32(int RegNum, ULONG LoadValue);
    int cbCInScan(int FirstCtr,int LastCtr, LONG Count,
                  LONG *Rate, HGLOBAL MemHandle, ULONG Options);
    int cbCConfigScan(int CounterNum, int Mode,int DebounceTime,
                      int DebounceMode, int EdgeDetection,
                      int TickSize, int MappedChannel);

    // Daq functions
    int cbDaqInScan(short *ChanArray, short *ChanTypeArray, short *GainArray, int ChanCount, long *Rate,
                    long *PretrigCount, long *TotalCount, HGLOBAL MemHandle, int Options);
    int cbDaqSetTrigger(int TrigSource, int TrigSense, int TrigChan, int ChanType, 
                        int Gain, float Level, float Variance, int TrigEvent);

    void readThread();

private:
    libusb_device_handle *deviceHandle_;
    epicsThreadId readThreadId_;
    epicsMutex readMutex_;
    epicsEventId acquireStartEvent_;

    bool ctrScanAcquiring_;
    bool ctrScanComplete_;
    uint16_t *ctrScanBuffer_;
    uint16_t *ctrScanRawBuffer_;
    int ctrScanNumPoints_;
    int ctrScanNumCounters_;
    int ctrScanNumElements_;
    int ctrScanDataType_;
    bool ctrScanContinuous_;
    int ctrScanCurrentPoint_;
    int ctrScanCurrentIndex_;
    ScanData scanData_;

};

#endif // mcBoard_USB_CTRInclude

