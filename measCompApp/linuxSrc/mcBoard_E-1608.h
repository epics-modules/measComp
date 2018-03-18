#ifndef mcBoard_E_1608Include
#define mcBoard_E_1608Include

#include "mcBoard.h"
#include "E-1608.h"

class mcE_1608 : mcBoard {
public:
    mcE_1608(const char *address);
    int cbSetConfig(int InfoType, int DevNum, int ConfigItem, int ConfigVal);
    int cbGetIOStatus(short *Status, long *CurCount, long *CurIndex,int FunctionType);
    int cbAIn(int Chan, int Gain, USHORT *DataValue);
    int cbAInScan(int LowChan, int HighChan, long Count, long *Rate, 
                int Gain, HGLOBAL MemHandle, int Options);
    int cbALoadQueue(short *ChanArray, short *GainArray, int NumChans);
    int cbAOut(int Chan, int Gain, USHORT DataValue);
    int cbCIn32(int CounterNum, ULONG *Count);
    int cbCLoad32(int RegNum, ULONG LoadValue);
    int cbDBitOut(int PortType, int BitNum, USHORT BitValue);
    int cbDConfigBit(int PortType, int BitNum, int Direction);
    int cbDIn(int PortType, USHORT *DataValue);

private:
    DeviceInfo_E1608 deviceInfo_;
};

#endif /* mcBoard_E_1608Include */