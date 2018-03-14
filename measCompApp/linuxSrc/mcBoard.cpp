#include <stdio.h>
#include <vector>

#include "cbw_linux.h"
#include "mcBoard_E-TC.h"


mcBoard::mcBoard(const char *address) {
    strncpy(address_, address, sizeof(address_)-1);
}

std::vector<mcBoard*> boardList;

// System functions

int cbAddBoard(const char *boardName, const char *address)
{
    mcBoard *pBoard;
    if (strcmp(boardName, "E-TC") ==0) {
        pBoard = (mcBoard *)new mcE_TC(address);
    }
    else {
        printf("Unknown board type %s\n", boardName);
        return BADBOARD;
    }
    boardList.push_back(pBoard);
    return NOERRORS;
}

int mcBoard::getConfig(int InfoType, int DevNum, int ConfigItem, int *ConfigVal) {
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

int cbGetConfig(int InfoType, int BoardNum, int DevNum, 
                int ConfigItem, int *ConfigVal)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->getConfig(InfoType, DevNum, ConfigItem, ConfigVal);
}

int mcBoard::setConfig(int InfoType, int DevNum, int ConfigItem, int ConfigVal)
{
    printf("Function cbSetConfig not supported\n");
    return NOERRORS;
}
int cbSetConfig(int InfoType, int BoardNum, int DevNum, int ConfigItem, int ConfigVal)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->setConfig(InfoType, DevNum, ConfigItem, ConfigVal);
}

int mcBoard::getBoardName(char *BoardName) {
    strcpy(BoardName, boardName_);
    return NOERRORS;
}
int cbGetBoardName(int BoardNum, char *BoardName)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->getBoardName(BoardName);
}

int cbGetErrMsg(int ErrCode, char *ErrMsg)
{
    static const char *functionName = "cbGetErrMsg";
    printf("Function %s not supported\n", functionName);
    return NOERRORS;
}

HGLOBAL cbWinBufAlloc(long NumPoints)
{
    static const char *functionName = "cbWinBufAlloc";
    printf("Function %s not supported\n", functionName);
    return NOERRORS;
}

HGLOBAL cbScaledWinBufAlloc(long NumPoints)
{
    static const char *functionName = "cbScaledWinBufAlloc";
    printf("Function %s not supported\n", functionName);
    return NOERRORS;
}

HGLOBAL cbWinBufAlloc32(long NumPoints)
{
    static const char *functionName = "cbWinBufAlloc32";
    printf("Function %s not supported\n", functionName);
    return NOERRORS;
}

int cbStopIOBackground(int BoardNum, int FunctionType)
{
    static const char *functionName = "cbStopIOBackground";
    printf("Function %s not supported\n", functionName);
    return NOERRORS;
}

int mcBoard::getIOStatus(short *Status, long *CurCount, long *CurIndex,int FunctionType)
{
    printf("cbGetIOStatus not supported\n");
    return NOERRORS;
}
int cbGetIOStatus(int BoardNum, short *Status, long *CurCount, long *CurIndex,int FunctionType)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->getIOStatus(Status, CurCount, CurIndex, FunctionType);
}

      
// Analog I/O functions
int cbAIn(int BoardNum, int Chan, int Gain, USHORT *DataValue)
{
    static const char *functionName = "cbAIn";
    printf("Function %s not supported\n", functionName);
    return NOERRORS;
}

int cbAIn32(int BoardNum, int Chan, int Gain, ULONG *DataValue, int Options)
{
    static const char *functionName = "cbAIn32";
    printf("Function %s not supported\n", functionName);
    return NOERRORS;
}

int cbAInScan(int BoardNum, int LowChan, int HighChan, long Count,
              long *Rate, int Gain, HGLOBAL MemHandle, 
              int Options)
{
    static const char *functionName = "cbAInScan";
    printf("Function %s not supported\n", functionName);
    return NOERRORS;
}

int cbALoadQueue(int BoardNum, short *ChanArray, short *GainArray, 
                 int NumChans)
{
    static const char *functionName = "cbALoadQueue";
    printf("Function %s not supported\n", functionName);
    return NOERRORS;
}

int cbAOut(int BoardNum, int Chan, int Gain, USHORT DataValue)
{
    static const char *functionName = "cbAOut";
    printf("Function %s not supported\n", functionName);
    return NOERRORS;
}

int cbAOutScan(int BoardNum, int LowChan, int HighChan, 
               long Count, long *Rate, int Gain, 
               HGLOBAL MemHandle, int Options)
{
    static const char *functionName = "cbAOutScan";
    printf("Function %s not supported\n", functionName);
    return NOERRORS;
}


// Counter functions
int cbC9513Config (int BoardNum, int CounterNum, int GateControl,
                   int CounterEdge, int CountSource, 
                   int SpecialGate, int Reload, int RecycleMode, 
                   int BCDMode, int CountDirection, 
                   int OutputControl)
{
    static const char *functionName = "cbAOutScan";
    printf("Function %s not supported\n", functionName);
    return NOERRORS;
}
                                
int cbC9513Init(int BoardNum, int ChipNum, int FOutDivider, 
                int FOutSource, int Compare1, int Compare2, 
                int TimeOfDay)
{
    static const char *functionName = "cbC9513Init";
    printf("Function %s not supported\n", functionName);
    return NOERRORS;
}

int mcBoard::cIn32(int CounterNum, ULONG *Count)
{
    printf("cbCIn32 is not supported\n");
    return NOERRORS;
}
int cbCIn32(int BoardNum, int CounterNum, ULONG *Count)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cIn32(CounterNum, Count);
}

int mcBoard::cLoad32(int RegNum, ULONG LoadValue)
{
    printf("Function cbCLoad32 not supported\n");
    return NOERRORS;
}
int cbCLoad32(int BoardNum, int RegNum, ULONG LoadValue)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cLoad32(RegNum, LoadValue);
}

int cbCInScan(int BoardNum, int FirstCtr,int LastCtr, LONG Count,
              LONG *Rate, HGLOBAL MemHandle, ULONG Options)
{
    static const char *functionName = "cbCInScan";
    printf("Function %s not supported\n", functionName);
    return NOERRORS;
}

int cbCConfigScan(int BoardNum, int CounterNum, int Mode,int DebounceTime,
                  int DebounceMode, int EdgeDetection,
                  int TickSize, int MappedChannel)
{
    static const char *functionName = "cbCConfigScan";
    printf("Function %s not supported\n", functionName);
    return NOERRORS;
}


// Digital I/O functions
int mcBoard::dBitOut(int PortType, int BitNum, USHORT BitValue)
{
    printf("Function cbDBitOut not supported\n");
    return NOERRORS;
}
int cbDBitOut(int BoardNum, int PortType, int BitNum, USHORT BitValue)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->dBitOut(PortType, BitNum, BitValue);
}

int cbDConfigPort(int BoardNum, int PortType, int Direction)
{
    static const char *functionName = "cbDConfigPort";
    printf("Function %s not supported\n", functionName);
    return NOERRORS;
}

int mcBoard::dConfigBit(int PortType, int BitNum, int Direction)
{
    printf("Function cbDConfigBit not supported\n");
    return NOERRORS;
}
int cbDConfigBit(int BoardNum, int PortType, int BitNum, int Direction)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->dConfigBit(PortType, BitNum, Direction);
}

int mcBoard::dIn(int PortType, USHORT *DataValue) {
    printf("cbDIn is not supported\n");
    return NOERRORS;
}
int cbDIn(int BoardNum, int PortType, USHORT *DataValue)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->dIn(PortType, DataValue);
}

int cbDIn32(int BoardNum, int PortType, UINT *DataValue)
{
    static const char *functionName = "cbDIn32";
    printf("Function %s not supported\n", functionName);
    return NOERRORS;
}

int cbDOut(int BoardNum, int PortType, USHORT DataValue)
{
    static const char *functionName = "cbDOut";
    printf("Function %s not supported\n", functionName);
    return NOERRORS;
}

int cbDOut32(int BoardNum, int PortType, UINT DataValue)
{
    static const char *functionName = "cbDOut32";
    printf("Function %s not supported\n", functionName);
    return NOERRORS;
}

// Pulse functions
int cbPulseOutStart(int BoardNum, int TimerNum, double *Frequency, 
                    double *DutyCycle, unsigned int PulseCount, 
                    double *InitialDelay, int IdleState, int Options)
{
    static const char *functionName = "cbPulseOutStart";
    printf("Function %s not supported\n", functionName);
    return NOERRORS;
}
                    
int cbPulseOutStop(int BoardNum, int TimerNum)
{
    static const char *functionName = "cbPulseOutStop";
    printf("Function %s not supported\n", functionName);
    return NOERRORS;
}


// Temperature functions
int mcBoard::tIn(int Chan, int Scale, float *TempValue, int Options)
{
    printf("cbTIn is not supported\n");
    return NOERRORS;
}
int cbTIn(int BoardNum, int Chan, int Scale, float *TempValue, int Options)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->tIn(Chan, Scale, TempValue, Options);
}

// Trigger functions
int cbSetTrigger(int BoardNum, int TrigType, USHORT LowThreshold, 
                 USHORT HighThreshold)
{
    static const char *functionName = "cbSetTrigger";
    printf("Function %s not supported\n", functionName);
    return NOERRORS;
}

