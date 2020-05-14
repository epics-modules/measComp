/*implemented DIO24 functionalities, counter function is currently 
 * not supported
 * Created by Danny Wong
 */
#ifndef mcBoard_E_DIO24Include
#define mcBoard_E_DIO24Include

#include <epicsThread.h>
#include <epicsEvent.h>

#include "mcBoard.h"
#include "E-DIO24.h"
#include "mccEthernet.h"

class mcE_DIO24 : mcBoard {
public:
    mcE_DIO24(const char *address);

    // Counter functions
    int cbCIn32(int CounterNum, ULONG *Count);
    int cbCLoad32(int RegNum, ULONG LoadValue);

    // Digital I/O functions
    int cbDBitOut(int PortType, int BitNum, USHORT BitValue);
    int cbDConfigBit(int PortType, int BitNum, int Direction);
    int cbDIn32(int PortType, UINT *DataValue);


 private:
	EthernetDeviceInfo device;
};

#endif /* mcBoard_E_DIO24Include */

