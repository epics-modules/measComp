/*implemented DIO24 functionalities, counter function is currently
 * not supported
 * Created by Danny Wong
 */
#include <stdio.h>
#include <math.h>
#include <unistd.h>

#include <epicsThread.h>
#include <epicsMutex.h>

#include "mcBoard_E-DIO24.h"

mcE_DIO24::mcE_DIO24(const char *address)
  : mcBoard(address)
{
    strcpy(boardName_, "E-DIO24");
    biBoardType_    = EDIO24_PID;
    biNumADCChans_  = 0;
    biADCRes_       = 0;
    biNumDACChans_  = 0;
    biDACRes_       = 0;
    biNumTempChans_ = 0;
    biDInNumDevs_   = 1;
    diDevType_      = AUXPORT;
    diInMask_       = 0;
    diOutMask_      = 0;
    diNumBits_      = 24;

    // Open Ethernet socket
    device.connectCode = 0x0;   // default connect code
    device.frameID = 0;         // zero out the frameID
    device.Address.sin_family = AF_INET;
    device.Address.sin_port = htons(COMMAND_PORT);
    device.Address.sin_addr.s_addr = INADDR_ANY;
    if (inet_pton(AF_INET, address_, &device.Address.sin_addr) == 0) {
        printf("Improper destination address.\n");
        return;
    }

    if ((device.sock = openDevice(inet_addr(inet_ntoa(device.Address.sin_addr)),
                                              device.connectCode)) < 0) {
        printf("Error opening socket\n");
        return;
    }

}


int mcE_DIO24::cbCIn32(int CounterNum, ULONG *Count)
{
    uint32_t counts;
      if (!CounterR_DIO24(&device, &counts)) {
         return BADBOARD;
      }
      *Count = counts;
      return NOERRORS;
}

int mcE_DIO24::cbCLoad32(int RegNum, ULONG LoadValue)
{
      if (!CounterW_DIO24(&device)) {
         return BADBOARD;
      }
      return NOERRORS;
}

int mcE_DIO24::cbDBitOut(int PortType, int BitNum, USHORT BitValue)
{
    uint32_t value;
//     uint32_t mask = 0x1 << BitNum;
    uint32_t mask = 0x1 << BitNum;
    // Read the current output register
    if (!DOutR_DIO24(&device, &value)) {
        return BADBOARD;
    }
    // Set or clear the bit
    if (BitValue) {
        value |= mask;
    } else {
        value &= ~mask;
    }
    if (!DOut_DIO24(&device, mask, value)) {
        return BADBOARD;
    }
    return NOERRORS;
}

int mcE_DIO24::cbDConfigBit(int PortType, int BitNum, int Direction)
{
    uint32_t value;
    //uint32_t mask = 0x1 << BitNum;
    uint32_t mask = 0x1 << BitNum;
    // Read the current output register
    if (!DConfigR_DIO24(&device, &value)) {
        return BADBOARD;
    }
    // Set or clear the bit
    if (Direction == DIGITALIN) {
        value |= mask;
    } else {
        value &= ~mask;
    }
    if (!DConfigW_DIO24(&device, mask, value)) {
        return BADBOARD;
    }
    return NOERRORS;
}

int mcE_DIO24::cbDIn32(int PortType, UINT *DataValue)
{
    uint32_t value;
    if (!DIn_DIO24(&device, &value)) {
        return BADBOARD;
    }
    *DataValue = value;
    return NOERRORS;
}

