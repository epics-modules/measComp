#ifndef mcBoardInclude
#define mcBoardInclude

#include "cbw_linux.h"

#define MAX_ADDRESS_LEN 100

class mcBoard {
public:
    mcBoard(const char *address);
    // System functions
    virtual int cbGetBoardName(char *BoardName);
    virtual int cbGetConfig(int InfoType, int DevNum, int ConfigItem, int *ConfigVal);
    virtual int cbSetConfig(int InfoType, int DevNum, int ConfigItem, int ConfigVal);
    virtual int cbGetIOStatus(short *Status, long *CurCount, long *CurIndex,int FunctionType);
    virtual int cbStopIOBackground(int FunctionType);

    // Analog I/O functions
    virtual int cbAIn(int Chan, int Gain, USHORT *DataValue);
    virtual int cbAIn32(int Chan, int Gain, ULONG *DataValue, int Options);
    virtual int cbAInScan(int LowChan, int HighChan, long Count, long *Rate, 
                          int Gain, HGLOBAL MemHandle, int Options);
    virtual int cbALoadQueue(short *ChanArray, short *GainArray, int NumChans);
    virtual int cbAOut(int Chan, int Gain, USHORT DataValue);
    virtual int cbAOutScan(int LowChan, int HighChan, long Count, 
                           long *Rate, int Gain, HGLOBAL MemHandle, int Options);

    // Counter functions
    virtual int cbC9513Config(int CounterNum, int GateControl, int CounterEdge, int CountSource, 
                              int SpecialGate, int Reload, int RecycleMode, 
                              int BCDMode, int CountDirection, int OutputControl);
    virtual int cbC9513Init(int ChipNum, int FOutDivider, int FOutSource, int Compare1, 
                            int Compare2,int TimeOfDay);
    virtual int cbCIn32(int CounterNum, ULONG *Count);
    virtual int cbCLoad32(int RegNum, ULONG LoadValue);
    virtual int cbCInScan(int FirstCtr,int LastCtr, LONG Count,
                          LONG *Rate, HGLOBAL MemHandle, ULONG Options);
    virtual int cbCConfigScan(int CounterNum, int Mode,int DebounceTime,
                              int DebounceMode, int EdgeDetection,
                              int TickSize, int MappedChannel);

    // Digital I/O functions
    virtual int cbDBitOut(int PortType, int BitNum, USHORT BitValue);
    virtual int cbDConfigPort(int PortType, int Direction);
    virtual int cbDConfigBit(int PortType, int BitNum, int Direction);
    virtual int cbDIn(int PortType, USHORT *DataValue);
    virtual int cbDIn32(int PortType, UINT *DataValue);
    virtual int cbDOut(int PortType, USHORT DataValue);
    virtual int cbDOut32(int PortType, UINT DataValue);

    // Pulse functions
    virtual int cbPulseOutStart(int TimerNum, double *Frequency, 
                                double *DutyCycle, unsigned int PulseCount, 
                                double *InitialDelay, int IdleState, int Options);
    virtual int cbPulseOutStop(int TimerNum);

    // Temperature functions
    virtual int cbTIn(int Chan, int Scale, float *TempValue, int Options);

    // Trigger functions
    virtual int cbSetTrigger(int TrigType, USHORT LowThreshold, USHORT HighThreshold);

protected:
    char address_[MAX_ADDRESS_LEN];
    char boardName_[BOARDNAMELEN];
    int biBoardType_;
    int biNumADCChans_;
    int biADCRes_;
    int biNumDACChans_;
    int biDACRes_;
    int biNumTempChans_;
    int biDInNumDevs_;
    int diDevType_;
    int diInMask_;
    int diOutMask_;
    int diNumBits_;
};

#endif /* mcBoardInclude */
