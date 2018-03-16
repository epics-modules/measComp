#ifndef mcBoard_E_1608Include
#define mcBoard_E_1608Include

#include "mcBoard.h"
#include "E-1608.h"

class mcE_1608 : mcBoard {
public:
    mcE_1608(const char *address);
    int setConfig(int InfoType, int DevNum, int ConfigItem, int ConfigVal);
    int getIOStatus(short *Status, long *CurCount, long *CurIndex,int FunctionType);
    int aIn(int Chan, int Gain, USHORT *DataValue);
    int aInScan(int LowChan, int HighChan, long Count, long *Rate, 
                int Gain, HGLOBAL MemHandle, int Options);
    int aLoadQueue(short *ChanArray, short *GainArray, int NumChans);
    int aOut(int Chan, int Gain, USHORT DataValue);
    int cIn32(int CounterNum, ULONG *Count);
    int cLoad32(int RegNum, ULONG LoadValue);
    int dBitOut(int PortType, int BitNum, USHORT BitValue);
    int dConfigBit(int PortType, int BitNum, int Direction);
    int dIn(int PortType, USHORT *DataValue);

private:
    DeviceInfo_E1608 deviceInfo_;
};

#endif /* mcBoard_E_1608Include */