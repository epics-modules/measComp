#include <stdio.h>
#include <math.h>
#include <unistd.h>

#include <epicsThread.h>
#include <epicsMutex.h>


#include "mcBoard_USB-CTR.h"

#define CLOCK_FREQ 96000000

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
    ctrScanBuffer_ = 0;
    ctrScanRawBuffer_ = 0;
    memset(&scanData_, 0, sizeof(scanData_));

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

    usbInit_CTR(deviceHandle_);
    acquireStartEvent_ = epicsEventCreate(epicsEventEmpty);

    ringBuffer_ = epicsRingBytesCreate(1024);

    readThreadId_ = epicsThreadCreate("measCompReadThread",
                                      epicsThreadPriorityMedium,
                                      epicsThreadGetStackSize(epicsThreadStackMedium),
                                      (EPICSTHREADFUNC)readThreadC,
                                      this);
}


void mcUSB_CTR::readThread()
{
    static const char *functionName = "readThread";

    readMutex_.lock();
    while (1) {
        readMutex_.unlock();
        epicsEventWait(acquireStartEvent_);
        if (!ctrScanAcquiring_) continue;
        readMutex_.lock();
        ctrScanCurrentPoint_ = 0;
        ctrScanCurrentIndex_ = 0;
        int pointsRemaining = ctrScanNumPoints_;
        epicsRingBytesFlush(ringBuffer_);

        while (ctrScanAcquiring_) {
            ctrScanComplete_ = false;
            readMutex_.unlock();
            asynPrint(pasynUser_, ASYN_TRACEIO_DRIVER,
                "%s::%s calling usbScanRead_USB_CTR \n",
                driverName, functionName);
            int bytesRead = usbScanRead_USB_CTR(deviceHandle_, scanData_, ctrScanRawBuffer_);
            asynPrint(pasynUser_, ASYN_TRACEIO_DRIVER,
                "%s::%s usbScanRead_USB_CTR read %d bytes\n",
                driverName, functionName, bytesRead);
            if (bytesRead <= 0) {
                // In most cases it is an error if bytesRead <= 0
                // However in external channel advance or external trigger mode this may be normal since we can't predict when the first pulse will come
                // Ignore the error if this is the first point in the scan.
                if (ctrScanCurrentPoint_ == 0){
                    continue;
                }
                asynPrint(pasynUser_, ASYN_TRACE_ERROR,
                    "%s::%s Error calling usbScanRead_USB_CTR = %d\n", driverName, functionName, bytesRead);
                ctrScanAcquiring_ = false;
                break;
            }
            readMutex_.lock();
            // Copy the data to the ring buffer
            int numCopied = epicsRingBytesPut(ringBuffer_, (char *)ctrScanRawBuffer_, bytesRead);
            if (numCopied != bytesRead) {
                asynPrint(pasynUser_, ASYN_TRACE_ERROR,
                    "%s::%s Error ring buffer overflow copying %d bytes\n", driverName, functionName, bytesRead);
            }
            int bytesAvailable = epicsRingBytesUsedBytes(ringBuffer_);
            asynPrint(pasynUser_, ASYN_TRACEIO_DRIVER,
                "%s::%s usbScanRead_USB_CTR %d bytes available\n",
                driverName, functionName, bytesAvailable);
            // Check to see if acquisition was aborted
            if (!ctrScanAcquiring_) break;
            int pointsAvailable = 0;
            switch (ctrScanDataType_) {
              case CTR32BIT: {
                int bytesPerReading = ctrScanNumElements_ * sizeof(uint32_t);
                pointsAvailable = bytesAvailable / bytesPerReading;
                if (pointsAvailable == 0) continue;
                epicsRingBytesGet(ringBuffer_, counterBuffer_, pointsAvailable * bytesPerReading);
                uint32_t *pIn = (uint32_t *)counterBuffer_;
                uint32_t *pOut = (uint32_t *)ctrScanBuffer_ + ctrScanCurrentPoint_;
                for (int point=0; point<pointsAvailable; point++) {
                    for (int counter=0; counter<ctrScanNumCounters_; counter++) {
                        *pOut++ = *pIn++;
                        ctrScanCurrentPoint_++;
                        ctrScanCurrentIndex_++;
                    }
                }
                break;
              }
              case CTR16BIT: {
                int bytesPerReading = ctrScanNumElements_ * sizeof(uint16_t);
                pointsAvailable = bytesAvailable / bytesPerReading;
                if (pointsAvailable == 0) continue;
                epicsRingBytesGet(ringBuffer_, counterBuffer_, pointsAvailable * bytesPerReading);
                uint16_t *pIn = (uint16_t *)counterBuffer_;
                uint16_t *pOut = (uint16_t *)ctrScanBuffer_ + ctrScanCurrentPoint_;
                for (int point=0; point<pointsAvailable; point++) {
                    for (int counter=0; counter<ctrScanNumCounters_; counter++) {
                        *pOut++ = *pIn++;
                        ctrScanCurrentPoint_++;
                        ctrScanCurrentIndex_++;
                    }
                }
                break;
              }
            }
            pointsRemaining -= pointsAvailable;
            asynPrint(pasynUser_, ASYN_TRACEIO_DRIVER,
                "%s::%s pointRemaining=%d\n",
                driverName, functionName, pointsRemaining);
            //  If the desired number of points have been collected in MCS mode stop
            if (!ctrScanContinuous_ && (pointsRemaining == 0)) {
                ctrScanAcquiring_ = false;
                break;
            }
        }
        ctrScanComplete_ = true;
    }
}


int mcUSB_CTR::cbGetIOStatus(short *Status, long *CurCount, long *CurIndex, int FunctionType)
{
    //static const char *functionName = "cbGetIOStatus";

    if ((FunctionType == CTRFUNCTION) || (FunctionType == DAQIFUNCTION)){
        readMutex_.lock();
        *Status = ctrScanAcquiring_ ? 1 : 0;
        *CurCount = ctrScanCurrentPoint_;
        *CurIndex = ctrScanCurrentIndex_ - 1;
        readMutex_.unlock();
    }
    //asynPrint(pasynUser_, ASYN_TRACEIO_DRIVER,
    //    "%s::%s returning Status=%d CurCount=%d, CurIndex=%d, FunctionType=%d\n",
    //    driverName, functionName, *Status, (int)*CurCount, (int)*CurIndex, FunctionType);
    return NOERRORS;
}


int mcUSB_CTR::cbStopIOBackground(int FunctionType)
{
    static const char *functionName = "cbStopIOBackground";

    if ((FunctionType == CTRFUNCTION) || (FunctionType == DAQIFUNCTION)) {
        readMutex_.lock();
        ctrScanAcquiring_ = false;
        // Wait for the poller to exit
        while (!ctrScanComplete_) {
            readMutex_.unlock();
            epicsThreadSleep(0.001);
            readMutex_.lock();
        }
        readMutex_.unlock();
        asynPrint(pasynUser_, ASYN_TRACEIO_DRIVER,
            "%s::%s Calling usbScanStop_USB_CTR\n", driverName, functionName);
        usbScanStop_USB_CTR(deviceHandle_);
    }
    return NOERRORS;
}

int mcUSB_CTR::cbCIn32(int CounterNum, ULONG *Count)
{
    //static const char *functionName = "cbCIn32";

    *Count = usbCounter_USB_CTR(deviceHandle_, CounterNum);
    return NOERRORS;
}

int mcUSB_CTR::cbCLoad32(int RegNum, ULONG LoadValue)
{
    static const char *functionName = "cbCLoad32";

    uint8_t index=0;
    int reg = (RegNum/100)*100;
    int counterNum = RegNum - reg;
    switch (reg) {
      case OUTPUTVAL0REG0:
      case OUTPUTVAL1REG0:
          if (reg == OUTPUTVAL1REG0) index=1;
          asynPrint(pasynUser_, ASYN_TRACEIO_DRIVER,
              "%s::%s calling usbCounterOutValuesW_USB_CTR counterNum=%d, index=%d, value=%d\n",
              driverName, functionName, counterNum, index, LoadValue);
          usbCounterOutValuesW_USB_CTR(deviceHandle_, counterNum, index, LoadValue);
          break;
      case MINLIMITREG0:
      case MAXLIMITREG0:
          if (reg == MAXLIMITREG0) index=1;
          asynPrint(pasynUser_, ASYN_TRACEIO_DRIVER,
              "%s::%s calling usbCounterLimitValuesW_USB_CTR counterNum=%d, index=%d, value=%d\n",
              driverName, functionName, counterNum, index, LoadValue);
          usbCounterLimitValuesW_USB_CTR(deviceHandle_, counterNum, index, LoadValue);
          break;
      case LOADREG0:
      case LOADREG1:
          if (reg == LOADREG1) index=1;
          //asynPrint(pasynUser_, ASYN_TRACE_ERROR,
          //    "%s::%s got LOADREG0 or LOADREG1 command, don't know what to do counterNum=%d, index=%d, value=%ul\n",
          //    driverName, functionName, counterNum, index, LoadValue);
          // I don't know what function should be called for this
          //usbCounterOutValuesW_USB_CTR(deviceHandle_, counterNum, index, LoadValue);
          break;
    }
    return NOERRORS;
}


int mcUSB_CTR::cbCInScan(int FirstCtr,int LastCtr, LONG Count,
                         LONG *Rate, HGLOBAL MemHandle, ULONG Options)
{
    int numCounters = LastCtr - FirstCtr + 1;
    int scanCount = Count/numCounters;
    static const char *functionName = "cbCInScan";

    int numElements=0;
    for (int i=0; i<numCounters; i++) {
        // Each counter has 4 banks of 16-bit registers that can be scanned in any order.
        // Bank 0 contains bit 0-15, Bank 1 contains bit 16-31, Bank 2 contains bit 32-47
        // and Bank 4 contains bit 48-63.  If bit(5) is set to 1, this element will
        // be a read of the DIO, otherwise it will use the specified counter and bank.
        // ScanList[33] channel configuration:
        // bit(0-2): Counter Nuber (0-7)
        // bit(3-4): Counter Bank (0-3)
        // bit(5): 1 = DIO,  0 = Counter
        // bit(6): 1 = Fill with 16-bits of 0's,  0 = normal (This allows for the creation
        // of a 32 or 64-bit element of the DIO when it is mixed with 32 or 64-bit elements of counters)

        scanData_.scanList[numElements]    = 0x7 & (FirstCtr + i); // Counter number
        scanData_.scanList[numElements++] |= (0x3 & 0) << 3;       // Bank 0 (bits 0-15))
        if (Options & CTR32BIT) {
            scanData_.scanList[numElements]    = 0x7 & (FirstCtr + i); // Counter number
            scanData_.scanList[numElements++] |= (0x3 & 1) << 3;       // Bank 1 (bits 16-31))
        }
    }
    ctrScanAcquiring_ = true;
    ctrScanBuffer_ = (uint16_t *)MemHandle;
    scanData_.lastElement = numElements - 1;
    ctrScanNumPoints_ = scanCount;
    ctrScanNumCounters_ = numCounters;
    ctrScanNumElements_ = numElements;
    ctrScanDataType_ = (Options & CTR32BIT) ? CTR32BIT : CTR16BIT;
    scanData_.mode = 0;
    if (Options & SINGLEIO) {
        scanData_.mode |= USB_CTR_SINGLEIO;
    }
    scanData_.options = 0;
    if (Options & NOCLEAR)    scanData_.options |= 0x1;
    if (Options & EXTTRIGGER) scanData_.options |= 0x1 << 3;
    ctrScanContinuous_ = false;
    scanData_.count = scanCount;
    if (Options & CONTINUOUS) {
        scanData_.count = 0;
        ctrScanContinuous_ = true;
    }
    scanData_.mode |= USB_CTR_CONTINUOUS_READOUT;
    double scanRate = *Rate;
    if (Options & HIGHRESRATE) scanRate = scanRate/1000.;
    uint32_t pacer_period = rint((96.E6/scanRate) - 1);
    scanRate = 96.e6 / (pacer_period + 1);
    *Rate = (Options & HIGHRESRATE) ? scanRate * 1000 : scanRate;

    if (Options & EXTCLOCK) scanRate = 0;
    scanData_.frequency = scanRate;

    // Make sure the FIFO is cleared
    usbScanStop_USB_CTR(deviceHandle_);
    usbScanClearFIFO_USB_CTR(deviceHandle_);
    usbScanBulkFlush_USB_CTR(deviceHandle_, 5);

    asynPrint(pasynUser_, ASYN_TRACEIO_DRIVER,
        "%s::%s calling usbScanConfigW_USB_CTR lastElement=%d, list[0-3]=0x%x, 0x%x, 0x%x, 0x%x\n",
        driverName, functionName, scanData_.lastElement, scanData_.scanList[0], scanData_.scanList[1], scanData_.scanList[2], scanData_.scanList[3]);
    usbScanConfigW_USB_CTR(deviceHandle_, scanData_);

    asynPrint(pasynUser_, ASYN_TRACEIO_DRIVER,
        "%s::%s calling usbScanStart_USB_CTR count=%d, retrig_count=%d, mode=0x%x, frequency=%f, packet_size=%d options=0x%x\n",
        driverName, functionName, scanData_.count, scanData_.retrig_count, scanData_.mode, scanData_.frequency, scanData_.packet_size, scanData_.options);
    usbScanStart_USB_CTR(deviceHandle_, &scanData_);
    asynPrint(pasynUser_, ASYN_TRACEIO_DRIVER,
        "%s::%s after calling usbScanStart_USB_CTR count=%d, retrig_count=%d, mode=0x%x, frequency=%f, packet_size=%d options=0x%x\n",
        driverName, functionName, scanData_.count, scanData_.retrig_count, scanData_.mode, scanData_.frequency, scanData_.packet_size, scanData_.options);

    if (ctrScanRawBuffer_) free(ctrScanRawBuffer_);
    ctrScanRawBuffer_ = (uint16_t *)calloc(ctrScanNumPoints_*ctrScanNumElements_, sizeof(uint32_t)); // Can hold 16-bit or 32-bit data
    epicsEventSignal(acquireStartEvent_);

    return NOERRORS;
}


int mcUSB_CTR::cbCConfigScan(int CounterNum, int Mode,int DebounceTime,
                             int DebounceMode, int EdgeDetection,
                             int TickSize, int MappedChannel)
{
    CounterParams params;
    static const char *functionName = "cbCConfigScan";

    params.counter = CounterNum;

    params.counterOptions = 0;
    if (Mode & CLEAR_ON_READ)  params.counterOptions |= USB_CTR_CLEAR_ON_READ;
    if (Mode & NO_RECYCLE_ON)  params.counterOptions |= USB_CTR_NO_RECYCLE;
    if (Mode & COUNT_DOWN_ON)  params.counterOptions |= USB_CTR_COUNT_DOWN;
    if (Mode & RANGE_LIMIT_ON) params.counterOptions |= USB_CTR_RANGE_LIMIT;
    if (EdgeDetection &
        CTR_FALLING_EDGE)      params.counterOptions |= USB_CTR_FALLING_EDGE;

    params.modeOptions = 0;
    params.modeOptions |= (TickSize << 4);

    params.gateOptions = 0;
    if (Mode & GATING_ON)   params.gateOptions |= 1;
    if (Mode & INVERT_GATE) params.gateOptions |= 2; // Need to check the logic

    params.outputOptions = 0;
    if (Mode & OUTPUT_ON)                 params.outputOptions |= 1;
    if (Mode & OUTPUT_INITIAL_STATE_HIGH) params.outputOptions |= 2;

    params.debounce = 0;
    if (DebounceMode == CTR_TRIGGER_BEFORE_STABLE) params.debounce |= 0x10;
    if (DebounceTime != CTR_DEBOUNCE_NONE)         params.debounce |= (0x0f | (DebounceTime >> 1));

    asynPrint(pasynUser_, ASYN_TRACEIO_DRIVER,
        "%s::%s calling usbCounterParamsW_USB_CTR CounterNum=%d, "
        "params.counterOptions=0x%x, params.gateOptions=0x%x, params.outputOptions=0x%x, params.debounce=0x%x\n",
        driverName, functionName, CounterNum, params.counterOptions, params.gateOptions, params.outputOptions, params.debounce);
    usbCounterParamsW_USB_CTR(deviceHandle_, CounterNum, params);

    return NOERRORS;
}


int mcUSB_CTR::cbPulseOutStart(int TimerNum, double *Frequency,
                    double *DutyCycle, unsigned int PulseCount,
                    double *InitialDelay, int IdleState, int Options)
{
    TimerParams params;
    //static const char *functionName = "cbPulseOutStart";

    params.timer = TimerNum;
    params.period = round(CLOCK_FREQ / *Frequency) - 1;
    params.pulseWidth = round(*DutyCycle * CLOCK_FREQ / *Frequency) - 1;
    params.count = PulseCount;
    params.delay = *InitialDelay * CLOCK_FREQ;
    *DutyCycle = (params.pulseWidth + 1) * *Frequency / CLOCK_FREQ;
    *Frequency = CLOCK_FREQ / (params.period + 1);
    usbTimerParamsW_USB_CTR(deviceHandle_, TimerNum, params);
    uint8_t control = 1;
    if (IdleState == 1) control |= 0x1 << 2;
      usbTimerControlW_USB_CTR(deviceHandle_, TimerNum, control);
      return NOERRORS;
}

int mcUSB_CTR::cbPulseOutStop(int TimerNum)
{
    //static const char *functionName = "cbPulseOutStop";

      usbTimerControlW_USB_CTR(deviceHandle_, TimerNum, 0);
    return NOERRORS;
}

int mcUSB_CTR::cbDBitOut(int PortType, int BitNum, USHORT BitValue)
{
    uint16_t value;
    uint16_t mask = 0x1 << BitNum;
    //static const char *functionName = "cbDBitOut";

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
    //static const char *functionName = "cbDConfigBit";

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
    //static const char *functionName = "cbDIn";

    uint16_t value = usbDPort_USB_CTR(deviceHandle_);
    *DataValue = value;
    return NOERRORS;
}


int mcUSB_CTR::cbSetTrigger(int TrigType, USHORT LowThreshold, USHORT HighThreshold)
{
    uint8_t trigger;
    static const char *functionName = "cbSetTrigger";

    switch (TrigType) {
      case TRIG_POS_EDGE:
          trigger = 1;
          break;
      case TRIG_NEG_EDGE:
          trigger = 2;
          break;
      case TRIG_HIGH:
          trigger = 3;
          break;
      case TRIG_LOW:
          trigger = 4;
          break;
      default:
          asynPrint(pasynUser_, ASYN_TRACE_ERROR, "%s::%s unknown TrigType %d\n", driverName, functionName, TrigType);
          return BADTRIGTYPE;
    }
    usbTriggerConfig_USB_CTR(deviceHandle_, trigger);
    return NOERRORS;
}

// Daq functions
int mcUSB_CTR::cbDaqInScan(short *ChanArray, short *ChanTypeArray, short *GainArray, int ChanCount, long *Rate,
                           long *PretrigCount, long *TotalCount, HGLOBAL MemHandle, int Options)
{
    static const char *functionName = "cbDaqInScan";

    for (int i=0; i<ChanCount; i++) {
        // Each counter has 4 banks of 16-bit registers that can be scanned in any order.
        // Bank 0 contains bit 0-15, Bank 1 contains bit 16-31, Bank 2 contains bit 32-47
        // and Bank 4 contains bit 48-63.  If bit(5) is set to 1, this element will
        // be a read of the DIO, otherwise it will use the specified counter and bank.
        // ScanList[33] channel configuration:
        // bit(0-2): Counter Nuber (0-7)
        // bit(3-4): Counter Bank (0-3)
        // bit(5): 1 = DIO,  0 = Counter
        // bit(6): 1 = Fill with 16-bits of 0's,  0 = normal (This allows for the creation
        // of a 32 or 64-bit element of the DIO when it is mixed with 32 or 64-bit elements of counters)
        scanData_.scanList[i] = 0x7 & ChanArray[i]; // Counter number
        switch (ChanTypeArray[i]) {
          case CTRBANK0:
            scanData_.scanList[i] |= (0x3 & 0) << 3;     // Bank 0 (bits 0-15))
            break;
          case CTRBANK1:
            scanData_.scanList[i] |= (0x3 & 1) << 3;     // Bank 1 (bits 16-31))
            break;
          case DIGITAL8:
            scanData_.scanList[i] = (1 << 5);           // Bit 5 is DIO
            break;
          case PADZERO:
            scanData_.scanList[i] = (1 << 6);           // Bit 6 is padding
            break;
        }
    }
    ctrScanDataType_ = CTR16BIT;
    ctrScanAcquiring_ = true;
    ctrScanBuffer_ = (uint16_t *)MemHandle;
    scanData_.lastElement = ChanCount - 1;
    ctrScanNumPoints_ =   *TotalCount/ChanCount;
    ctrScanNumCounters_ = ChanCount;
    ctrScanNumElements_ = ChanCount;
    scanData_.mode = 0;
    if (Options & SINGLEIO) {
        scanData_.mode |= USB_CTR_SINGLEIO;
    } else {
        scanData_.mode |= USB_CTR_CONTINUOUS_READOUT;
        scanData_.mode |= USB_CTR_FORCE_PACKET_SIZE;
        scanData_.packet_size = 256;
    }
    scanData_.options = 0;
    if (Options & NOCLEAR)    scanData_.options |= 0x1;
    if (Options & EXTTRIGGER) scanData_.options |= 0x1 << 3;
    ctrScanContinuous_ = false;
    scanData_.count = ctrScanNumPoints_;
    if (Options & CONTINUOUS) {
        scanData_.count = 0;
        ctrScanContinuous_ = true;
    }
    double scanRate = *Rate;
    if (Options & HIGHRESRATE) scanRate = scanRate/1000.;
    uint32_t pacer_period = rint((96.E6/scanRate) - 1);
    scanRate = 96.e6 / (pacer_period + 1);
    *Rate = (Options & HIGHRESRATE) ? scanRate * 1000 : scanRate;

    if (Options & EXTCLOCK) scanRate = 0;
    scanData_.frequency = scanRate;

    // Make sure the FIFO is cleared
    usbScanStop_USB_CTR(deviceHandle_);
    usbScanClearFIFO_USB_CTR(deviceHandle_);
    usbScanBulkFlush_USB_CTR(deviceHandle_, 5);

    asynPrint(pasynUser_, ASYN_TRACEIO_DRIVER,
        "%s::%s calling usbScanConfigW_USB_CTR lastElement=%d, list[0-3]=0x%x, 0x%x, 0x%x, 0x%x\n",
        driverName, functionName, scanData_.lastElement, scanData_.scanList[0], scanData_.scanList[1], scanData_.scanList[2], scanData_.scanList[3]);
    usbScanConfigW_USB_CTR(deviceHandle_, scanData_);

    asynPrint(pasynUser_, ASYN_TRACEIO_DRIVER,
        "%s::%s calling usbScanStart_USB_CTR count=%d, retrig_count=%d, mode=0x%x, frequency=%f, packet_size=%d options=0x%x\n",
        driverName, functionName, scanData_.count, scanData_.retrig_count, scanData_.mode, scanData_.frequency, scanData_.packet_size, scanData_.options);
    usbScanStart_USB_CTR(deviceHandle_, &scanData_);
    asynPrint(pasynUser_, ASYN_TRACEIO_DRIVER,
        "%s::%s after calling usbScanStart_USB_CTR count=%d, retrig_count=%d, mode=0x%x, frequency=%f, packet_size=%d options=0x%x\n",
        driverName, functionName, scanData_.count, scanData_.retrig_count, scanData_.mode, scanData_.frequency, scanData_.packet_size, scanData_.options);

    if (ctrScanRawBuffer_) free(ctrScanRawBuffer_);
    ctrScanRawBuffer_ = (uint16_t *)calloc(ctrScanNumPoints_*ctrScanNumElements_, sizeof(uint32_t)); // Can hold 16-bit or 32-bit data
    epicsEventSignal(acquireStartEvent_);
    return NOERRORS;
}

int mcUSB_CTR::cbDaqSetTrigger(int TrigSource, int TrigSense, int TrigChan, int ChanType,
                               int Gain, float Level, float Variance, int TrigEvent)
{
    uint8_t trigger;
    static const char *functionName = "cbDaqSetTrigger";

    switch (TrigSense) {
      case RISING_EDGE:
          trigger = 3;
          break;
      case FALLING_EDGE:
          trigger = 1;
          break;
      case HIGH_LEVEL:
          trigger = 2;
          break;
      case LOW_LEVEL:
          trigger = 0;
          break;
      default:
          asynPrint(pasynUser_, ASYN_TRACE_ERROR, "%s::%s unknown TrigSense %d\n", driverName, functionName, TrigSense);
          return BADTRIGTYPE;
    }
    usbTriggerConfig_USB_CTR(deviceHandle_, trigger);
    return NOERRORS;
}

