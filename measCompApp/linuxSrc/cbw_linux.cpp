#include <stdio.h>
#include <vector>

#include "cbw_linux.h"
#include "E-TC.h"

#define MAX_ADDRESS_LEN 100

class mcBoard {
public:
    mcBoard(const char *address) {
        strncpy(address_, address, sizeof(address_)-1);
    }
    int getBoardName(char *BoardName) {
        strcpy(BoardName, boardName_);
        return NOERRORS;
    }
    virtual int getConfig(int InfoType, int DevNum, int ConfigItem, int *ConfigVal) {
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
printf("address=%s, boardName=%s, boardType=%d, numTempChans=%d, DInNumDevs=%d\n",
address_, boardName_, biBoardType_, biNumTempChans_, biDInNumDevs_);
printf("getConfig InfoType=%d, ConfigItem=%d, *ConfigVal=%d\n",
InfoType, ConfigItem, *ConfigVal);
        return NOERRORS;
    }

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

class mcE_TC : mcBoard {
public:
    mcE_TC(const char *address)
      : mcBoard(address) {
        strcpy(boardName_, "E-TC");
        biBoardType_    = ETC_PID;
        biNumADCChans_  = 0;
        biADCRes_       = 0;
        biNumDACChans_  = 0;
        biDACRes_       = 0;
        biNumTempChans_ = 8;
        biDInNumDevs_   = 1;
        diDevType_      = AUXPORT;
        diInMask_       = 0;
        diOutMask_      = 0;
        diNumBits_      = 8;
    }

private:
    DeviceInfo_TC deviceInfo;
};

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

int cbGetConfig(int InfoType, int BoardNum, int DevNum, 
                int ConfigItem, int *ConfigVal)
{
    if (BoardNum >= (int)boardList.size()) return BADBOARD;
    mcBoard *pBoard = boardList[BoardNum];
    return pBoard->getConfig(InfoType, DevNum, ConfigItem, ConfigVal);
}

int cbSetConfig(int InfoType, int BoardNum, int DevNum, 
                int ConfigItem, int ConfigVal)
{
    static const char *functionName = "cbSetConfig";
    printf("Function %s not supported\n", functionName);
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

int cbGetIOStatus(int BoardNum, short *Status, long *CurCount,
                  long *CurIndex,int FunctionType)
{
    static const char *functionName = "cbGetIOStatus";
    printf("Function %s not supported\n", functionName);
    return NOERRORS;
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

int cbCIn32(int BoardNum, int CounterNum, ULONG *Count)
{
    static const char *functionName = "cbCIn32";
    printf("Function %s not supported\n", functionName);
    return NOERRORS;
}

int cbCLoad32(int BoardNum, int RegNum, ULONG LoadValue)
{
    static const char *functionName = "cbCLoad32";
    printf("Function %s not supported\n", functionName);
    return NOERRORS;
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
int cbDBitOut(int BoardNum, int PortType, int BitNum, USHORT BitValue)
{
    static const char *functionName = "cbDBitOut";
    printf("Function %s not supported\n", functionName);
    return NOERRORS;
}

int cbDConfigPort(int BoardNum, int PortType, int Direction)
{
    static const char *functionName = "cbDConfigPort";
    printf("Function %s not supported\n", functionName);
    return NOERRORS;
}

int cbDConfigBit(int BoardNum, int PortType, int BitNum, int Direction)
{
    static const char *functionName = "cbDConfigBit";
    printf("Function %s not supported\n", functionName);
    return NOERRORS;
}

int cbDIn(int BoardNum, int PortType, USHORT *DataValue)
{
    static const char *functionName = "cbDIn";
    printf("Function %s not supported\n", functionName);
    return NOERRORS;
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
int cbTIn(int BoardNum, int Chan, int Scale, float *TempValue,
          int Options)
{
    static const char *functionName = "cbTIn";
    printf("Function %s not supported\n", functionName);
    return NOERRORS;
}

// Trigger functions
int cbSetTrigger(int BoardNum, int TrigType, USHORT LowThreshold, 
                 USHORT HighThreshold)
{
    static const char *functionName = "cbSetTrigger";
    printf("Function %s not supported\n", functionName);
    return NOERRORS;
}

