#include <stdio.h>
#include <vector>

#include "cbw_linux.h"
#include "mcBoard.h"
#include "mcBoard_E-TC.h"
#include "mcBoard_E-TC32.h"
#include "mcBoard_E-1608.h"


std::vector<mcBoard*> boardList;

// System functions

int cbAddBoard(const char *boardName, const char *address)
{
    mcBoard *pBoard;
    if (strcmp(boardName, "E-TC") ==0) {
        pBoard = (mcBoard *)new mcE_TC(address);
    }
    else if (strcmp(boardName, "E-TC32") ==0) {
        pBoard = (mcBoard *)new mcE_TC32(address);
    }
    else if (strcmp(boardName, "E-1608") ==0) {
        pBoard = (mcBoard *)new mcE_1608(address);
    }
    else {
        printf("Unknown board type %s\n", boardName);
        return BADBOARD;
    }
    boardList.push_back(pBoard);
    return NOERRORS;
}

int cbGetConfig(int InfoType, int BoardNum, int DevNum, 
                int ConfigItem, int *ConfigVal)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbGetConfig(InfoType, DevNum, ConfigItem, ConfigVal);
}

int cbSetConfig(int InfoType, int BoardNum, int DevNum, int ConfigItem, int ConfigVal)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbSetConfig(InfoType, DevNum, ConfigItem, ConfigVal);
}

int cbGetBoardName(int BoardNum, char *BoardName)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbGetBoardName(BoardName);
}

int cbGetErrMsg(int ErrCode, char *ErrMsg)
{
    static const char *functionName = "cbGetErrMsg";
    printf("Function %s not supported\n", functionName);
    return NOERRORS;
}

HGLOBAL cbWinBufAlloc(long NumPoints)
{
    return calloc(NumPoints, sizeof(short));
}

HGLOBAL cbScaledWinBufAlloc(long NumPoints)
{
    return calloc(NumPoints, sizeof(double));
}

HGLOBAL cbWinBufAlloc32(long NumPoints)
{
    return calloc(NumPoints, sizeof(int));
}

int cbStopIOBackground(int BoardNum, int FunctionType)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbStopIOBackground(FunctionType);
}

int cbGetIOStatus(int BoardNum, short *Status, long *CurCount, long *CurIndex,int FunctionType)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbGetIOStatus(Status, CurCount, CurIndex, FunctionType);
}

      
int cbAIn(int BoardNum, int Chan, int Gain, USHORT *DataValue)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbAIn(Chan, Gain, DataValue);
}

int cbAIn32(int BoardNum, int Chan, int Gain, ULONG *DataValue, int Options)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbAIn32(Chan, Gain, DataValue, Options);
}

int cbAInScan(int BoardNum, int LowChan, int HighChan, long Count,
              long *Rate, int Gain, HGLOBAL MemHandle, int Options)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbAInScan(LowChan, HighChan, Count, Rate, Gain, MemHandle, Options);
}

int cbALoadQueue(int BoardNum, short *ChanArray, short *GainArray, int NumChans)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbALoadQueue(ChanArray, GainArray, NumChans);
}

int cbAOut(int BoardNum, int Chan, int Gain, USHORT DataValue)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbAOut(Chan, Gain, DataValue);
}

int cbAOutScan(int BoardNum, int LowChan, int HighChan, 
               long Count, long *Rate, int Gain, 
               HGLOBAL MemHandle, int Options)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbAOutScan(LowChan, HighChan, Count, Rate, Gain, MemHandle, Options);
}


// Counter functions
int cbC9513Config(int BoardNum, int CounterNum, int GateControl,
                  int CounterEdge, int CountSource, 
                  int SpecialGate, int Reload, int RecycleMode, 
                  int BCDMode, int CountDirection, 
                  int OutputControl)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbC9513Config(CounterNum, GateControl, CounterEdge, CountSource, SpecialGate, 
                                 Reload, RecycleMode, BCDMode, CountDirection, OutputControl);
}
                                
int cbC9513Init(int BoardNum, int ChipNum, int FOutDivider, 
                int FOutSource, int Compare1, int Compare2, 
                int TimeOfDay)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbC9513Init(ChipNum, FOutDivider, FOutSource, Compare1, Compare2, TimeOfDay);
}

int cbCIn32(int BoardNum, int CounterNum, ULONG *Count)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbCIn32(CounterNum, Count);
}

int cbCLoad32(int BoardNum, int RegNum, ULONG LoadValue)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbCLoad32(RegNum, LoadValue);
}

int cbCInScan(int BoardNum, int FirstCtr,int LastCtr, LONG Count,
              LONG *Rate, HGLOBAL MemHandle, ULONG Options)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbCInScan(FirstCtr, LastCtr, Count, Rate, MemHandle, Options);
}

int cbCConfigScan(int BoardNum, int CounterNum, int Mode,int DebounceTime,
                  int DebounceMode, int EdgeDetection,
                  int TickSize, int MappedChannel)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbCConfigScan(CounterNum, Mode, DebounceTime, DebounceMode, 
                                 EdgeDetection, TickSize, MappedChannel);
}


// Digital I/O functions
int cbDBitOut(int BoardNum, int PortType, int BitNum, USHORT BitValue)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbDBitOut(PortType, BitNum, BitValue);
}

int cbDConfigPort(int BoardNum, int PortType, int Direction)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbDConfigPort(PortType, Direction);
}

int cbDConfigBit(int BoardNum, int PortType, int BitNum, int Direction)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbDConfigBit(PortType, BitNum, Direction);
}

int cbDIn(int BoardNum, int PortType, USHORT *DataValue)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbDIn(PortType, DataValue);
}

int cbDIn32(int BoardNum, int PortType, UINT *DataValue)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbDIn32(PortType, DataValue);
}

int cbDOut(int BoardNum, int PortType, USHORT DataValue)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbDOut(PortType, DataValue);
}

int cbDOut32(int BoardNum, int PortType, UINT DataValue)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbDOut32(PortType, DataValue);
}

// Pulse functions
int cbPulseOutStart(int BoardNum, int TimerNum, double *Frequency, 
                    double *DutyCycle, unsigned int PulseCount, 
                    double *InitialDelay, int IdleState, int Options)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbPulseOutStart(TimerNum, Frequency, DutyCycle, PulseCount, 
                                   InitialDelay, IdleState, Options);
}
                    
int cbPulseOutStop(int BoardNum, int TimerNum)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbPulseOutStop(TimerNum);
}


// Temperature functions
int cbTIn(int BoardNum, int Chan, int Scale, float *TempValue, int Options)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbTIn(Chan, Scale, TempValue, Options);
}

// Trigger functions
int cbSetTrigger(int BoardNum, int TrigType, USHORT LowThreshold, 
                 USHORT HighThreshold)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->cbSetTrigger(TrigType, LowThreshold, HighThreshold);
}

