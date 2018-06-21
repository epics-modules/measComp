#ifndef mcBoard_E_TC32Include
#define mcBoard_E_TC32Include

#include "mcBoard.h"
#include "E-TC32.h"

class mcE_TC32 : mcBoard {
public:
    mcE_TC32(const char *address);
    int cbSetConfig(int InfoType, int DevNum, int ConfigItem, int ConfigVal);
    int cbDBitOut(int PortType, int BitNum, USHORT BitValue);
    int cbDIn(int PortType, USHORT *DataValue);
    int cbTIn(int Chan, int Scale, float *TempValue, int Options);

private:
    DeviceInfo_TC32 deviceInfo_;
};

#endif /* mcBoard_E_TC32Include */

