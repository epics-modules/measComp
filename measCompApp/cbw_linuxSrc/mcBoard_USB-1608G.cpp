
#include <stdio.h>
#include <math.h>
#include <unistd.h>

#include <epicsThread.h>
#include <epicsMutex.h>

#include <pmd.h>
#include "mcBoard_USB-1608G.h"

static const char *driverName = "mcUSB-1608G";

static void readThreadC(void *pPvt)
{
    mcUSB1608G *pmcUSB1608G = (mcUSB1608G*) pPvt;
    pmcUSB1608G->readThread();
}

mcUSB1608G::mcUSB1608G(const char *address) : mcBoard(address)
{
    static const char *functionName = "mcUSB1608G";
    strcpy(boardName_, "USB-1608G");
    biBoardType_    = USB1608G_PID;
    biNumADCChans_  = 16;
    biADCRes_       = 16;
    biNumDACChans_  = 2;
    biDACRes_       = 16;
    biNumTempChans_ = 0;
    biDInNumDevs_   = 1;
    diDevType_      = AUXPORT;
    diInMask_       = 0;
    diOutMask_      = 0;
    diNumBits_      = 8;

    aiScanAcquiring_ = false;
    aiScanCurrentPoint_ = 0;
    aiScanCurrentIndex_ = 0;

    // Open USB1608G_PID interface
    int ret = libusb_init(NULL);
    if(ret < 0) {
        printf("%s::%s Failure, libusb_init failed with return code %d\n", driverName, functionName,ret);
        return;
    }
    
    printf("Starting Board Search\n");
    if((udev_ = usb_device_find_USB_MCC(USB1608G_V2_PID, NULL))) {
        printf("Success, found a USB-1608G!\n");
        usbInit_1608G(udev_,2);
        biBoardType_ = USB1608G_V2_PID;
        strcpy(boardName_, "USB-1608G");
    } else if((udev_ = usb_device_find_USB_MCC(USB1608GX_V2_PID, NULL))) {
        printf("Success, found a USB-1608GX!\n");
        usbInit_1608G(udev_,2);
        biBoardType_ = USB1608GX_V2_PID;
        strcpy(boardName_, "USB-1608GX");
    } else if((udev_ = usb_device_find_USB_MCC(USB1608GX_2AO_V2_PID, NULL))) {
        printf("Success, found a USB-1608GX_2AO!\n");
        usbInit_1608G(udev_,2);
        biBoardType_ = USB1608GX_2AO_V2_PID;
        strcpy(boardName_, "USB-1608GX_2AO");
    } else if((udev_ = usb_device_find_USB_MCC(USB1608G_PID, NULL))) {
        printf("Success, found a USB-1608G!\n");
        usbInit_1608G(udev_,1);
        biBoardType_ = USB1608G_PID;
        strcpy(boardName_, "USB-1608G");
    } else if((udev_ = usb_device_find_USB_MCC(USB1608GX_PID, NULL))) {

        printf("Success, found a USB-1608GX!\n");
        usbInit_1608G(udev_,1);
        biBoardType_ = USB1608GX_V2_PID;
        strcpy(boardName_, "USB-1608GX");
    } else if((udev_ = usb_device_find_USB_MCC(USB1608GX_2AO_PID, NULL))) {
        printf("Success, found a USB-1608GX_2AO!\n");
        usbInit_1608G(udev_,1);
        biBoardType_ = USB1608GX_2AO_PID;
        strcpy(boardName_, "USB-1608GX_2AO");
    } else {
        printf("%s::%s Failure, did not find a USB-1608G series device!\n",driverName, functionName);
        return;
    }
    
    usbCounterInit_USB1608G(udev_,COUNTER0);

    /* Read the gain table for the analog inputs out of the module's memory */
    usbBuildGainTable_USB1608G(udev_, deviceInfo_.table_AIn);
    
    /* Read the gain table for the analog outputs out of the module's memory */
    usbBuildGainTable_USB1608GX_2AO(udev_, deviceInfo_.table_AOut);

    /* Initially, I want to place all channels of the board in SINGLE_ENDED mode with a +/- 0-10V range */
    for (int count=0; count<NCHAN_1608G; count++) {
        deviceInfo_.list[count].channel = count;
        deviceInfo_.list[count].range = USB1608G_BP_10V;
        deviceInfo_.list[count].mode = USB1608G_SINGLE_ENDED;
    }
    // Mark where the list ends.
    deviceInfo_.list[NCHAN_1608G-1].mode |= USB1608G_LAST_CHANNEL;

    usbAInConfig_USB1608G(udev_,&deviceInfo_);
    acquireStartEvent_ = epicsEventCreate(epicsEventEmpty);

    readThreadId_ = epicsThreadCreate("measCompReadThread", epicsThreadPriorityMedium, epicsThreadGetStackSize(epicsThreadStackMedium), (EPICSTHREADFUNC)readThreadC, this);
}

static const char* rangeToString(int range)
{
    switch(range)
    {
    case USB1608G_BP_1V:
        return("1V Range");
        break;
        
    case USB1608G_BP_2V:
        return("2V Range");
        break;
        
    case USB1608G_BP_5V:
        return("5V Range");
        break;
        
    case USB1608G_BP_10V:
        return("10V Range");
        break;
        
    default:
        return("Unsupported Range");
        break;
    }
}
static const char* modeToString(int mode)
{
    static char modestring[40];
    
    switch(mode & ~USB1608G_LAST_CHANNEL)
    {
    case USB1608G_SINGLE_ENDED: //  (Single-Ended)
        return("Single Ended");
        break;
        
    case USB1608G_DIFFERENTIAL: //  (Differential)
        return("Differential");
        break;
        
    case USB1608G_CALIBRATION:      //   (Calibration mode)
        return("Calibration");
        break;
    default:
        sprintf(modestring,"Unsupported Mode=%d",mode & ~USB1608G_LAST_CHANNEL);
        return(modestring);
        break;
    }
}

static const char* listModeToString(int mode)
{
    static char modestring[40];
    
    switch(mode & ~USB1608G_LAST_CHANNEL)
    {
    case 1: //  (Single-Ended Lower)
    case 2: //  (Single-Ended Upper)
        return("Single Ended");
        break;
        
    case 0: //  (Differential)
        return("Differential");
        break;
        
    case 3:     //   (Calibration mode)
        return("Calibration");
        break;
    default:
        sprintf(modestring,"Unsupported Mode=%d",mode & ~USB1608G_LAST_CHANNEL);
        return(modestring);
        break;
    }
}

static void printScanListTable(usbDevice1608G *deviceInfo_)
{
    // Decode the table and print it's informaton
    for(int i=0;i<16;i++) {
        printf("Entry %2d\tChannel Mode %s\tChannel %x\tRange %s\t%s\n",
               i,
               listModeToString((deviceInfo_->scan_list[i] & 0x60) >> 5), 
               deviceInfo_->scan_list[i] & 7,
               rangeToString((deviceInfo_->scan_list[i] & 0x18) >> 3),
               (deviceInfo_->scan_list[i] & USB1608G_LAST_CHANNEL) ? "Last Chan" : "");
    }
}




void mcUSB1608G::readThread()
{
//  static const char *functionName = "readThread";
    int bytesReceived = 0;
    int totalBytesToRead;
    int totalBytesReceived;
    int currentChannel;
    int channel;
    int range;
    uint16_t *rawBuffer=0;
    uint16_t rawData;
    int correctedData;
    uint8_t mode;
    int ret;

    readMutex_.lock();
    while (1)
    {
        readMutex_.unlock();
        
        /* Wait for the IOC thread to start an acquire */
        epicsEventWait(acquireStartEvent_);
        
        readMutex_.lock();
        
        /* Skip if not actually acquiring */
        if (!aiScanAcquiring_) continue;
        
        totalBytesToRead = aiScanTotalElements_*sizeof(uint16_t);
        totalBytesReceived = 0;
        aiScanCurrentPoint_ = 0;
        aiScanCurrentIndex_ = 0;
        currentChannel = 0;
        
        /* Allocate a new buffer for this acquisition */
        if (rawBuffer)
            free (rawBuffer);
        rawBuffer = (uint16_t *)calloc(aiScanTotalElements_+1, sizeof(uint16_t));
        if(rawBuffer == NULL) {
            printf("Failed to allocate buffer for read\n");
            aiScanAcquiring_ = false;
            continue;
        }

        do {
            // Unlock stuffusbDevice1608G *usb1608G
            readMutex_.unlock();
            
            /* Read what I can */
            bytesReceived = usbAInScanRead_USB1608G(udev_, &deviceInfo_, rawBuffer);
            readMutex_.lock();
            // Check to see if acquisition was aborted
            if (!aiScanAcquiring_) break;
            
            /* Get the number of samples I've received to this point */
            totalBytesReceived += bytesReceived;
            int newPoints = bytesReceived / 2;

            for (int i=0; i<newPoints; i++) {
                rawData = rawBuffer[i];             // Read the raw data...
                channel = deviceInfo_.list[currentChannel].channel;
                mode = deviceInfo_.list[currentChannel].mode;
                range = deviceInfo_.list[currentChannel].range;

                //Convert to actual float data now.
                if ((mode & ~USB1608G_LAST_CHANNEL) == USB1608G_SINGLE_ENDED)
                {  // single ended
                    correctedData = rint(rawData*deviceInfo_.table_AIn[range][0] + deviceInfo_.table_AIn[range][1]);
                }
                else if ((mode & ~USB1608G_LAST_CHANNEL) == USB1608G_DIFFERENTIAL)
                {  // differential
                    correctedData = rint(rawData*deviceInfo_.table_AIn[range][0] + deviceInfo_.table_AIn[range][1]);
                }
                
                if (correctedData > 65536)
                    correctedData = 65535;  // max value
                else if (correctedData < 0)
                    correctedData = 0;
                
                if (aiScanIsScaled_)
                    aiScanScaledBuffer_[aiScanCurrentIndex_] = volts_USB1608G(range, correctedData);
                else
                    aiScanUnscaledBuffer_[aiScanCurrentIndex_] = correctedData;
                currentChannel ++;
                if ( (deviceInfo_.list[currentChannel-1].mode & USB1608G_LAST_CHANNEL) > 0)
                    currentChannel = 0;
                aiScanCurrentIndex_++;
             }
        } while (totalBytesReceived < totalBytesToRead);
        free(rawBuffer);
        rawBuffer=0;
        aiScanAcquiring_ = false;
    }
}

/**
 * Changes board configuration options at runtime
 * 
 * Parameters:
 * InfoType -   The configuration information for each board is grouped into different categories.  InfoType specifies which category you want.  Possibilities are:
 *                  BOARDINFO - general information about a board.
 *                  DIGITALINFO - information about a digital device.
 *                  COUNTERINFO - information about a counter device.
 *                  EXPANSIONINFO - Information about an expansion device.
 *                  MISCINFO - One of the miscellaneous options for the board
 * BoardNum -   The number associated with the board when it was installed or created with cbCreateDaqDevice.  BoardNum may be from 0-99.
 * DeviceNum -  Depends on the ConfigItem being passed.  It can serve as a channel number, an index into the ConfigItem, or can be ignored.
 * ConfigItem - Configuration item you wish to set.
 * ConfigVal -  Value for the configuration item.
 * 
 */
int mcUSB1608G::cbSetConfig(int InfoType, int DevNum, int ConfigItem, int ConfigVal)
{
    switch (InfoType)
    {
    case BOARDINFO:
        switch (ConfigItem)
        {
        case BIADTRIGCOUNT:             // Only Trig Count is recognized right now.
            aiScanTrigCount_ = ConfigVal;
            break;

        case BIDACTRIGCOUNT:
            break;
            
        case BIRANGE:
            printf("Set range for channel %d->0x%x\n",ConfigItem,ConfigVal);
            break;
            
        default:
            printf("mcUSB1608G::cbSetConfig error unknown ConfigItem %d\n", ConfigItem);
            return BADCONFIGITEM;
            break;
        }
        
        break;

    default:
        printf("mcUSB1608G::cbSetConfig error unknown InfoType %d\n", InfoType);
        return BADCONFIGTYPE;
        break;
    }
    return NOERRORS;
}

/**
 * 
 * Gets variousstatus information
 * 
 * Currently, only AIFUNCTION status is returned.
 */
int mcUSB1608G::cbGetIOStatus(short *Status, long *CurCount, long *CurIndex, int FunctionType)
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


/**
 * Analog Input Functions
 * 
 */

/**
 * Stop the stream acquisition if one was in process
 * 
 */
 int mcUSB1608G::cbStopIOBackground(int FunctionType)
{
    if (FunctionType == AIFUNCTION) {
        usbAInScanStop_USB1608G(udev_);
        readMutex_.lock();
        aiScanAcquiring_ = false;
        readMutex_.unlock();
    }
    return NOERRORS;
}

/**
 * Convert the GAIN to a input range
 * 
 */
static int gainToRange(int Gain)
{
    switch(Gain)
    {
    case BIP1VOLTS:
        return USB1608G_BP_1V;
        
    case BIP2VOLTS:
        return USB1608G_BP_2V;
        
    case BIP5VOLTS:
        return USB1608G_BP_5V;
        
    case BIP10VOLTS:
        return USB1608G_BP_10V;
        
    default:
        printf("Unsupported Gain=%d\n", Gain);
        return BADRANGE;
    }
}

/**
 * Read an A/D Input channel from the specified board, and returns a 16-bit unsigned integer value.  
 * If the specified A/D board has programmable gain then it sets the gain to the specified range.
 *  The raw A/D value is converted to an A/D value and returned to DataValue.
 *
 *Parameters:
 * BoardNumber -    The number associated with specific board when installed.
 * Channel -        A/D channel number.
 * Gain -           A/D Gain code.
 * DataValue -      Pointer to where to put the data value.
 * 
 */
int mcUSB1608G::cbAIn(int Chan, int Gain, USHORT *DataValue)
{
    uint16_t value;
    int range = gainToRange(Gain);

    if (range == BADRANGE)
    {
        printf("Unsupported Gain=%d\n", Gain);
        return BADRANGE;
    }
    if (aInputMode_ == DIFFERENTIAL)
        Chan += 8;
    value = usbAIn_USB1608G(udev_, Chan);
    value = rint( (value*deviceInfo_.table_AIn[range][0]) + deviceInfo_.table_AIn[range][1]);
    *DataValue = value;

    return NOERRORS;
}

/**
  * Initiates an Analog Scan function
  * Setup the appropriate structures and lists to provide the
  * SCAN routine with the information it needs.
  * 
  * Parameters:
  * LowChan - Lowest Channel to SCAN (0-15?).  Ignored if cbALoadQueue() is used.
  * HighChan - Highest Channel to SCAN ([LowChan] - 15?). Ignored if cbALoadQueue() is used.
  * Rate - Speed of acquisition (0 = external trigger)
  * Gain - Given gain for all channels (?)
  * MemHandle - Destination buffer pointer
  * Options - Basic options for acquisition
  */
int mcUSB1608G::cbAInScan(int LowChan, int HighChan, long Count, long *Rate,int Gain, HGLOBAL MemHandle, int Options)
{
    double frequency;       // Computed rate of inputs
    uint8_t options=0;

    // We assume that aLoadQueue has previously been called and aiScanNumChans_ is set.
    /* Is this acqisition externally clocked? */
    if (Options & EXTCLOCK)
        frequency = 0;
    else
        frequency = *Rate * aiScanNumChans_;

    /* If this is supposed to be an externally triggered event, move the trigger type into it's bit field */
    if (Options & EXTTRIGGER)
      options = aiScanTrigType_ << 2;
    
    aiScanTotalElements_ = Count;
    
    // Build the data into the deviceInfo_ Structure
    deviceInfo_.count = Count/aiScanNumChans_;
    deviceInfo_.retrig_count = 0;
    deviceInfo_.frequency = frequency;
    deviceInfo_.packet_size = 0;
    deviceInfo_.options = options;
    
    /* If I'm scaling the output data, setup the pointer to the Scaled buffer and set the flag for it */
    if (Options & SCALEDATA) {
        aiScanScaledBuffer_ = (double *)MemHandle;
        aiScanIsScaled_ = true;
    }
    else {
        aiScanUnscaledBuffer_ = (uint16_t *)MemHandle;
        aiScanIsScaled_ = false;
    }
    
    // Ready to start acquire, do it.

    usbAInScanStart_USB1608G(udev_, &deviceInfo_);
    aiScanAcquiring_ = true;
    epicsEventSignal(acquireStartEvent_);
    return NOERRORS;
}

int mcUSB1608G::cbAInputMode(int InputMode)
{
   aInputMode_ = InputMode;
    /* Place all the  channels of the board in the proper mode */
    for (int count=0; count<NCHAN_1608G; count++) {
        deviceInfo_.list[count].channel = count;
        deviceInfo_.list[count].mode = (aInputMode_ == DIFFERENTIAL) ? USB1608G_DIFFERENTIAL : USB1608G_SINGLE_ENDED;
    }
    // Mark where the list ends.
    if(aInputMode_ == DIFFERENTIAL)
        deviceInfo_.list[(NCHAN_1608G/2)-1].mode |= USB1608G_LAST_CHANNEL;
    else
        deviceInfo_.list[NCHAN_1608G-1].mode |= USB1608G_LAST_CHANNEL;

    usbAInConfig_USB1608G(udev_,&deviceInfo_);

    return NOERRORS;
}


/**
 * Build up the list structure that tells the module what channels to acquire, and how to acquire them
 * 
 * Parameters:
 * ChanArray Array containing channel values.  TRhis array should contain all of the channels that will be loaded into the Channel Gain Queue.
 * GainArray -  Array containing A/D rangevalues.  This array should contain each of the A/D  ranges that will be loaded into the Channel Gain Queue
 * NumChans -   Number of elements in the queue. 0 disables the queue.
 *
 */
int mcUSB1608G::cbALoadQueue(short *ChanArray, short *GainArray, int NumChans)
{
    int range;
    int count;
    
    for (count=0; count<NumChans; count++) {
        deviceInfo_.list[count].channel = ChanArray[count];
        range = gainToRange(GainArray[count]);
        if (range == BADRANGE) {
            printf("Unsupported Gain=%d\n", GainArray[count]);
            return BADRANGE;
        }
        deviceInfo_.list[count].range = (uint8_t)range;
        
        // Put in the mode here
        // stupidly, all channels have to be in the same mode.
        deviceInfo_.list[count].mode = (aInputMode_ == DIFFERENTIAL) ? USB1608G_DIFFERENTIAL : USB1608G_SINGLE_ENDED;
        
        // if this is the last channel, mark where the list ends.
        if(count == NumChans-1) {
            deviceInfo_.list[count].mode |= USB1608G_LAST_CHANNEL;
            // set how many channels are being scanned.
            aiScanNumChans_ = count+1;
        }
    }
 
    usbAInConfig_USB1608G(udev_,&deviceInfo_);
    return NOERRORS;
}

int mcUSB1608G::cbAOut(int Chan, int Gain, USHORT DataValue)
{
      usbAOut_USB1608GX_2AO(udev_, Chan, DataValue, deviceInfo_.table_AOut);
      return NOERRORS;
}

int mcUSB1608G::cbCIn32(int CounterNum, ULONG *Count)
{
    if(CounterNum == COUNTER0)
        *Count = usbCounter_USB1608G(udev_, COUNTER0);
    else
        *Count = usbCounter_USB1608G(udev_, COUNTER1);
    return NOERRORS;
}

int mcUSB1608G::cbCLoad32(int RegNum, ULONG LoadValue)
{
      usbCounterInit_USB1608G(udev_, COUNTER0);
      return NOERRORS;
}

int mcUSB1608G::cbDBitOut(int PortType, int BitNum, USHORT BitValue)
{
    uint8_t value;
    uint8_t mask = 0x1 << BitNum;

    // Read the current output register
    value = usbDLatchR_USB1608G(udev_);

    // Set or clear the bit
    if (BitValue)
        value |= mask;
    else
        value &= ~mask;

    usbDLatchW_USB1608G(udev_, value);

    return NOERRORS;
}

int mcUSB1608G::cbDConfigBit(int PortType, int BitNum, int Direction)
{
    uint8_t value;
    uint8_t mask = 0x1 << BitNum;

    // Read the current output register
    value = usbDTristateR_USB1608G(udev_);

    // Set or clear the bit
    if (Direction == DIGITALIN)
        value |= mask;
    else
        value &= ~mask;
    
    usbDTristateW_USB1608G(udev_, value);
    return NOERRORS;
}

int mcUSB1608G::cbDIn(int PortType, USHORT *DataValue)
{
    *DataValue = usbDPort_USB1608G(udev_);
    return NOERRORS;
}

/**
 * Set the type of trigger to use.
 * TRIG_? is defined in the cbw.h file, and is converted to a bit field in this routine
 * 
 * Parameters:
 * TrigType - Type of trigger to use, as determined by the windows driver
 * 
 */
int mcUSB1608G::cbSetTrigger(int TrigType, USHORT LowThreshold, USHORT HighThreshold)
{
    switch (TrigType)
    {
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
        printf("mcUSB1608G::cbSetTrigger unknown TrigType %d\n", TrigType);
        return BADTRIGTYPE;
    }
    return NOERRORS;
}
