#include <stdio.h>

#include "cbw_linux.h"
#include "mcBoard.h"


mcBoard::mcBoard(const char *address) {
    strncpy(address_, address, sizeof(address_)-1);
}

// System functions
int mcBoard::cbGetConfig(int InfoType, int DevNum, int ConfigItem, int *ConfigVal) {
    switch (InfoType) {
    case BOARDINFO:
        switch (ConfigItem) {
        case BIBOARDTYPE:
            *ConfigVal = biBoardType_;
            break;        
        case BINUMADCHANS:
            *ConfigVal = biNumADCChans_;
            break;
        case BIADRES:
            *ConfigVal = biADCRes_;
            break;
        case BINUMDACHANS:
            *ConfigVal = biNumDACChans_;
            break;
        case BIDACRES:
            *ConfigVal = biDACRes_;
            break;
        case BINUMTEMPCHANS:
            *ConfigVal = biNumTempChans_;
            break;
        case BIDINUMDEVS:
            *ConfigVal = biDInNumDevs_;
            break;
    }
    break;

    case DIGITALINFO:
        switch(ConfigItem) {
        case DIDEVTYPE:
            *ConfigVal = diDevType_;
            break;
        case DIINMASK:
            *ConfigVal = diInMask_;
            break;
        case DIOUTMASK:
            *ConfigVal = diOutMask_;
            break;
        case DINUMBITS:
            *ConfigVal = diNumBits_;
            break;
        }
    break;
    }
    return NOERRORS;
}

int mcBoard::cbSetConfig(int InfoType, int DevNum, int ConfigItem, int ConfigVal)
{
    printf("Function cbSetConfig not supported\n");
    return NOERRORS;
}

int mcBoard::cbGetBoardName(char *BoardName) 
{
    strcpy(BoardName, boardName_);
    return NOERRORS;
}

int mcBoard::cbGetIOStatus(short *Status, long *CurCount, long *CurIndex,int FunctionType)
{
    // Most unimplemented functions print an error message.  This one does not because it may be called
    // for devices even if they don't support it so the base class sets everything to 0.
    *Status = 0;
    *CurCount = 0;
    *CurIndex = 0;
    return NOERRORS;
}

int mcBoard::cbStopIOBackground(int FunctionType)
{
    printf("cbStopIOBackground not supported\n");
    return NOERRORS;
}

// Analog I/O functions
int mcBoard::cbAIn(int Chan, int Gain, USHORT *DataValue)
{
    printf("Function cbAIn not supported\n");
    return NOERRORS;
}

int mcBoard::cbAIn32(int Chan, int Gain, ULONG *DataValue, int Options)
{
    printf("Function cbAIn32 not supported\n");
    return NOERRORS;
}

int mcBoard::cbAInScan(int LowChan, int HighChan, long Count, long *Rate, 
                       int Gain, HGLOBAL MemHandle, int Options)
{
    printf("Function cbAInScan not supported\n");
    return NOERRORS;
}

int mcBoard::cbALoadQueue(short *ChanArray, short *GainArray, int NumChans)
{
    printf("Function cbALoadQueue not supported\n");
    return NOERRORS;
}

int mcBoard::cbAOut(int Chan, int Gain, USHORT DataValue)
{
    printf("Function cbAOut not supported\n");
    return NOERRORS;
}

int mcBoard::cbAOutScan(int LowChan, int HighChan, 
                        long Count, long *Rate, int Gain, 
                        HGLOBAL MemHandle, int Options)
{
    printf("Function cbAOutScan not supported\n");
    return NOERRORS;
}

// Counter functions
int mcBoard::cbC9513Config(int CounterNum, int GateControl,
                           int CounterEdge, int CountSource, 
                           int SpecialGate, int Reload, int RecycleMode, 
                           int BCDMode, int CountDirection, 
                           int OutputControl)
{
    printf("Function cbC9513Config not supported\n");
    return NOERRORS;
}
                                
int mcBoard::cbC9513Init(int ChipNum, int FOutDivider, 
                         int FOutSource, int Compare1, int Compare2, 
                         int TimeOfDay)
{
    printf("Function cbC9513Init not supported\n");
    return NOERRORS;
}

int mcBoard::cbCIn32(int CounterNum, ULONG *Count)
{
    printf("cbCIn32 is not supported\n");
    return NOERRORS;
}

int mcBoard::cbCLoad32(int RegNum, ULONG LoadValue)
{
    printf("Function cbCLoad32 not supported\n");
    return NOERRORS;
}

int mcBoard::cbCInScan(int FirstCtr,int LastCtr, LONG Count,
                       LONG *Rate, HGLOBAL MemHandle, ULONG Options)
{
    printf("Function cbCInScan not supported\n");
    return NOERRORS;
}

int mcBoard::cbCConfigScan(int CounterNum, int Mode,int DebounceTime,
                           int DebounceMode, int EdgeDetection,
                           int TickSize, int MappedChannel)
{
    printf("Function cbCConfigScan not supported\n");
    return NOERRORS;
}

// Digital I/O functions
int mcBoard::cbDBitOut(int PortType, int BitNum, USHORT BitValue)
{
    printf("Function cbDBitOut not supported\n");
    return NOERRORS;
}

int mcBoard::cbDConfigPort(int PortType, int Direction)
{
    printf("Function cbDConfigPort not supported\n");
    return NOERRORS;
}

int mcBoard::cbDConfigBit(int PortType, int BitNum, int Direction)
{
    printf("Function cbDConfigBit not supported\n");
    return NOERRORS;
}

int mcBoard::cbDIn(int PortType, USHORT *DataValue) 
{
    printf("cbDIn is not supported\n");
    return NOERRORS;
}

int mcBoard::cbDIn32(int PortType, UINT *DataValue)
{
    printf("cbDIn32 is not supported\n");
    return NOERRORS;
}

int mcBoard::cbDOut(int PortType, USHORT DataValue)
{
    printf("cbDOut is not supported\n");
    return NOERRORS;
}

int mcBoard::cbDOut32(int PortType, UINT DataValue)
{
    printf("cbDOut32 is not supported\n");
    return NOERRORS;
}

// Pulse functions
int mcBoard::cbPulseOutStart(int TimerNum, double *Frequency, 
                             double *DutyCycle, unsigned int PulseCount, 
                             double *InitialDelay, int IdleState, int Options)
{
    printf("cbPulseOutStart is not supported\n");
    return NOERRORS;
}
                    
int mcBoard::cbPulseOutStop(int TimerNum)
{
    printf("cbPulseOutStop is not supported\n");
    return NOERRORS;
}

// Temperature functions
int mcBoard::cbTIn(int Chan, int Scale, float *TempValue, int Options)
{
    printf("cbTIn is not supported\n");
    return NOERRORS;
}

// Trigger functions
int mcBoard::cbSetTrigger(int TrigType, USHORT LowThreshold, USHORT HighThreshold)
{
    printf("cbSetTrigger is not supported\n");
    return NOERRORS;
}
