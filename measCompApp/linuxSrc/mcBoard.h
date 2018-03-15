#ifndef mcBoardInclude
#define mcBoardInclude

#include "cbw_linux.h"

#define MAX_ADDRESS_LEN 100

class mcBoard {
public:
    mcBoard(const char *address);
    int getBoardName(char *BoardName);
    virtual int getConfig(int InfoType, int DevNum, int ConfigItem, int *ConfigVal);
    virtual int setConfig(int InfoType, int DevNum, int ConfigItem, int ConfigVal);
    virtual int getIOStatus(short *Status, long *CurCount, long *CurIndex,int FunctionType);
    virtual int aIn(int Chan, int Gain, USHORT *DataValue);
    virtual int aOut(int Chan, int Gain, USHORT DataValue);
    virtual int cIn32(int CounterNum, ULONG *Count);
    virtual int cLoad32(int RegNum, ULONG LoadValue);
    virtual int dBitOut(int PortType, int BitNum, USHORT BitValue);
    virtual int dConfigBit(int PortType, int BitNum, int Direction);
    virtual int dIn(int PortType, USHORT *DataValue);
    virtual int tIn(int Chan, int Scale, float *TempValue, int Options);

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
