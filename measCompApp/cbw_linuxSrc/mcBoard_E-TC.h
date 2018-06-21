#ifndef mcBoard_E_TCInclude
#define mcBoard_E_TCInclude

#include "mcBoard.h"
#include "E-TC.h"

class mcE_TC : mcBoard {
public:
    mcE_TC(const char *address);
    int cbSetConfig(int InfoType, int DevNum, int ConfigItem, int ConfigVal);
    int cbCIn32(int CounterNum, ULONG *Count);
    int cbCLoad32(int RegNum, ULONG LoadValue);
    int cbDBitOut(int PortType, int BitNum, USHORT BitValue);
    int cbDConfigBit(int PortType, int BitNum, int Direction);
    int cbDIn(int PortType, USHORT *DataValue);
    int cbTIn(int Chan, int Scale, float *TempValue, int Options);

private:
    DeviceInfo_TC deviceInfo_;
};

#endif /* mcBoard_E_TCInclude */

