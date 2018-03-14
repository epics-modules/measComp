#ifndef mcBoard_E_TCInclude
#define mcBoard_E_TCInclude

#include "mcBoard.h"
#include "E-TC.h"

class mcE_TC : mcBoard {
public:
    mcE_TC(const char *address);
    int setConfig(int InfoType, int DevNum, int ConfigItem, int ConfigVal);
    int getIOStatus(short *Status, long *CurCount, long *CurIndex,int FunctionType);
    int cIn32(int CounterNum, ULONG *Count);
    int cLoad32(int RegNum, ULONG LoadValue);
    int dBitOut(int PortType, int BitNum, USHORT BitValue);
    int dConfigBit(int PortType, int BitNum, int Direction);
    int dIn(int PortType, USHORT *DataValue);
    int tIn(int Chan, int Scale, float *TempValue, int Options);

private:
    DeviceInfo_TC deviceInfo_;
};

#endif /* mcBoard_E_TCInclude */