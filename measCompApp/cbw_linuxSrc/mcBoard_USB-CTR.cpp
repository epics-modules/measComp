#include <stdio.h>
#include <math.h>
#include <unistd.h>

#include <epicsThread.h>
#include <epicsMutex.h>


#include "mcBoard_USB-CTR.h"

#define CLOCK_FREQ 48000000

static const char *driverName = "mcUSB_CTR";

static void readThreadC(void *pPvt)
{
    mcUSB_CTR *pmcUSB_CTR = (mcUSB_CTR*) pPvt;
    pmcUSB_CTR->readThread();
}

mcUSB_CTR::mcUSB_CTR(const char *address)
  : mcBoard(address)
{
    static const char *functionName = "mcUSB_CTR";
    strcpy(boardName_, "USB-CTR");
    biBoardType_    = USB_CTR08_PID;
    biNumADCChans_  = 0;
    biADCRes_       = 16;
    biNumDACChans_  = 0;
    biDACRes_       = 16;
    biNumTempChans_ = 0;
    biDInNumDevs_   = 1;
    diDevType_      = AUXPORT;
    diInMask_       = 0;
    diOutMask_      = 0;
    diNumBits_      = 8;
    
    ctrScanAcquiring_ = false;
    ctrScanCurrentPoint_ = 0;
    ctrScanCurrentIndex_ = 0;
    
    if (libusb_init(NULL) < 0) {
        printf("%s::%s Failed to initialize libusb\n", driverName, functionName);
        return;
    }

    if ((deviceHandle_ = usb_device_find_USB_MCC(USB_CTR08_PID, NULL))) {
        printf("Success, found a USB-CTR08!\n");
        biBoardType_ = USB_CTR08_PID;
        strcpy(boardName_, "USB-CTR08");
    } else if ((deviceHandle_ = usb_device_find_USB_MCC(USB_CTR04_PID, NULL))) {
        printf("Success, found a USB-CTR04!\n");
        biBoardType_ = USB_CTR04_PID;
        strcpy(boardName_, "USB-CTR04");
    } else {
        printf("%s::%s Failure, did not find a USB-CTR04/08 series device!\n", driverName, functionName);
        return;
    }
    
    acquireStartEvent_ = epicsEventCreate(epicsEventEmpty);


    readThreadId_ = epicsThreadCreate("measCompReadThread",
                                      epicsThreadPriorityMedium,
                                      epicsThreadGetStackSize(epicsThreadStackMedium),
                                      (EPICSTHREADFUNC)readThreadC,
                                      this);
}


void mcUSB_CTR::readThread()
{
}
/*
    int bytesReceived = 0;
    int sock;
    int totalBytesToRead;
    int totalBytesReceived;
    int currentChannel;
    int channel;
    int range;
    uint16_t *rawBuffer=0;
    uint16_t rawData;
    int correctedData;
    int timeout = deviceInfo_.timeout;

    while (1) {
        readMutex_.unlock();
        epicsEventWait(acquireStartEvent_);
        if (!aiScanAcquiring_) continue;
        readMutex_.lock();
        sock = deviceInfo_.device.scan_sock;
        if (sock < 0) {
            printf("Error no scan sock\n");
            continue;
        }
        totalBytesToRead = aiScanTotalElements_*2;
        totalBytesReceived = 0;
        aiScanCurrentPoint_ = 0;
        aiScanCurrentIndex_ = 0;
        currentChannel = 0;
        if (rawBuffer) free (rawBuffer);
        rawBuffer = (uint16_t *)calloc(aiScanTotalElements_, sizeof(uint16_t));

        do {
            readMutex_.unlock();
            bytesReceived = receiveMessage(sock, &rawBuffer[totalBytesReceived/2], totalBytesToRead - totalBytesReceived, timeout);
            readMutex_.lock();
            // Check to see if acquisition was aborted
            if (!aiScanAcquiring_) break;
            if (bytesReceived <= 0) {  // error
                printf("Error in mcUSB_CTR::readThread: totalBytesReceived = %d totalBytesToRead = %d \n", totalBytesReceived, totalBytesToRead);
                continue;
            }
            totalBytesReceived += bytesReceived;
            int newPoints = bytesReceived / 2;
            for (int i=0; i<newPoints; i++) {
                rawData = rawBuffer[aiScanCurrentIndex_];
                channel = deviceInfo_.queue[2*currentChannel+1];
                range = deviceInfo_.queue[2*currentChannel+2];
                if (channel < DF) {  // single ended
                    correctedData = rint(rawData*deviceInfo_.table_AInSE[range].slope + deviceInfo_.table_AInSE[range].intercept);
                } else {  // differential
                    correctedData = rint(rawData*deviceInfo_.table_AInDF[range].slope + deviceInfo_.table_AInDF[range].intercept);
                }
                if (correctedData > 65536) {
                    correctedData = 65535;  // max value
                } else if (correctedData < 0) {
                    correctedData = 0;
                }
                if (aiScanIsScaled_) {
                    aiScanScaledBuffer_[aiScanCurrentIndex_] = volts_E1608(correctedData, range);
                } else {
                    aiScanUnscaledBuffer_[aiScanCurrentIndex_] = correctedData;
                }
                aiScanCurrentIndex_++;
                currentChannel ++;
                if (currentChannel >= aiScanNumChans_) {
                    currentChannel = 0;
                    aiScanCurrentPoint_++;
                }
             };
        } while (totalBytesReceived < totalBytesToRead);
        aiScanAcquiring_ = false;
        close(sock);
    }
}

int mcUSB_CTR::cbSetConfig(int InfoType, int DevNum, int ConfigItem, int ConfigVal) {
    switch (InfoType) {
      case BOARDINFO:
        switch (ConfigItem) {
          case BIADTRIGCOUNT:
            aiScanTrigCount_ = ConfigVal;
            break;

          default:
            printf("mcUSB_CTR::cbSetConfig error unknown ConfigItem %d\n", ConfigItem);
            return BADCONFIGITEM;
            break;
        }
        break;
    
      default:
        printf("mcUSB_CTR::cbSetConfig error unknown InfoType %d\n", InfoType);
        return BADCONFIGTYPE;
        break;
    }
    return NOERRORS;
}

int mcUSB_CTR::cbGetIOStatus(short *Status, long *CurCount, long *CurIndex, int FunctionType)
{
    if (FunctionType == AIFUNCTION) {
        readMutex_.lock();
        *Status = aiScanAcquiring_ ? 1 : 0;
        *CurCount = aiScanCurrentPoint_;
        *CurIndex = aiScanCurrentIndex_ - 1;
        readMutex_.unlock();
    }
    return NOERRORS;

}

int mcUSB_CTR::cbStopIOBackground(int FunctionType)
{
    if (FunctionType == AIFUNCTION) {
        AInScanStop_E1608(&deviceInfo_, 0);
        readMutex_.lock();
        aiScanAcquiring_ = false;
        readMutex_.unlock();
    }
    return NOERRORS;
}

int mcUSB_CTR::cbCIn32(int CounterNum, ULONG *Count)
{
    uint32_t counts;
      if (!CounterR_E1608(&deviceInfo_, &counts)) {
         return BADBOARD;
      }
      *Count = counts;
      return NOERRORS;
}

int mcUSB_CTR::cbCLoad32(int RegNum, ULONG LoadValue)
{
      if (!ResetCounter_E1608(&deviceInfo_)) {
         return BADBOARD;
      }
      return NOERRORS;
}

*/

int mcUSB_CTR::cbPulseOutStart(int TimerNum, double *Frequency, 
                    double *DutyCycle, unsigned int PulseCount, 
                    double *InitialDelay, int IdleState, int Options) {
    TimerParams timerParams;
    timerParams.timer = TimerNum;
    timerParams.period = round(CLOCK_FREQ / *Frequency);
    timerParams.pulseWidth = round(*DutyCycle * CLOCK_FREQ / *Frequency);
    timerParams.count = PulseCount;
    timerParams.delay = *InitialDelay * CLOCK_FREQ;
    usbTimerParamsW_USB_CTR(deviceHandle_, TimerNum, timerParams);
	  usbTimerControlW_USB_CTR(deviceHandle_, TimerNum, 1);
	  return NOERRORS;
}

int mcUSB_CTR::cbPulseOutStop(int TimerNum) {
	  usbTimerControlW_USB_CTR(deviceHandle_, TimerNum, 0);
    return NOERRORS;
}

int mcUSB_CTR::cbDBitOut(int PortType, int BitNum, USHORT BitValue)
{
    uint16_t value;
    uint16_t mask = 0x1 << BitNum;
    // Read the current output register
	  value = usbDLatchR_USB_CTR(deviceHandle_);
    // Set or clear the bit
    if (BitValue) {
        value |= mask;
    } else {
        value &= ~mask;
    }
    usbDLatchW_USB_CTR(deviceHandle_, value);
    return NOERRORS;
}

int mcUSB_CTR::cbDConfigBit(int PortType, int BitNum, int Direction)
{
    uint8_t value;
    uint8_t mask = 0x1 << BitNum;
    // Read the current tristate register
    value = usbDTristateR_USB_CTR(deviceHandle_);
    // Set or clear the bit
    if (Direction == DIGITALIN) {
        value |= mask;
    } else {
        value &= ~mask;
    }
    usbDTristateW_USB_CTR(deviceHandle_, value);
    return NOERRORS;
}

int mcUSB_CTR::cbDIn(int PortType, USHORT *DataValue)
{
    uint16_t value = usbDPort_USB_CTR(deviceHandle_);
    *DataValue = value;
    return NOERRORS;
}
/*
int mcUSB_CTR::cbSetTrigger(int TrigType, USHORT LowThreshold, USHORT HighThreshold)
{
    switch (TrigType) {
      case TRIG_POS_EDGE:
          aiScanTrigType_ = 1;
          break;
      case TRIG_NEG_EDGE:
          aiScanTrigType_ = 2;
          break;
      case TRIG_HIGH:
          aiScanTrigType_ = 3;
          break;
      case TRIG_LOW:
          aiScanTrigType_ = 4;
          break;
      default:
          printf("mcUSB_CTR::cbSetTrigger unknown TrigType %d\n", TrigType);
          return BADTRIGTYPE;
    }
    return NOERRORS;
}
*/