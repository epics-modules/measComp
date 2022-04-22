/*
    CB call demonstrated:        	  cbTIn()

    Purpose:                          Reads temperature input channel 1

    Demonstration:                    Displays the temperature data

    Steps:
    1. Call ulGetDaqDeviceInventory() to get the list of available DAQ devices
    2. Call ulCreateDaqDevice() to to get a handle for the first DAQ device
    3. Call ulConnectDaqDevice() to establish a UL connection to the DAQ device
    4. Call ulTIn() for each channel specified to read a value from the A/D input channel
    5. Display the data
    6. Call ulDisconnectDaqDevice() and ulReleaseDaqDevice() before exiting the process.
*/

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "cbw.h"

#define MAX_DEV_COUNT  100
#define MAX_STR_LENGTH 64
#define ERR_MSG_LEN 256
#define ERR_NO_ERROR 0

int main(void)
{
    DaqDeviceDescriptor devDescriptors[MAX_DEV_COUNT];
	DaqDeviceInterface interfaceType = USB_IFC;  //ANY_IFC;
	unsigned int numDevs = MAX_DEV_COUNT;

	// set some variables used to acquire data
    int boardNum = 0;
    int boardFound = 0;
	int channel = 0;

	char otcDetectstr[MAX_STR_LENGTH];

	int i = 0;
	float data = 0;
	int err = ERR_NO_ERROR;

	// Get descriptors for all of the available DAQ devices
	err = cbGetDaqDeviceInventory(interfaceType, devDescriptors, &numDevs);

	if (err != ERR_NO_ERROR)
		goto end;

	// verify at least one DAQ device is detected
	if (numDevs == 0)
	{
		printf("No DAQ device is detected\n");
		goto end;
	}

	printf("Found %d DAQ device(s)\n", numDevs);
	for (i = 0; i < (int) numDevs; i++)
	{
		if (strstr(devDescriptors[i].ProductName, "USB-2408"))
		{
            printf("  [%d] %s: (%s)\n", i, devDescriptors[i].ProductName, devDescriptors[i].UniqueID);
            err = cbCreateDaqDevice(i, devDescriptors[i]);
            boardNum = i;
            boardFound +=1;
            break;
        }
    }

    if (boardFound < 1)
    {
        printf("No USB-2408 series device attached\n");
        return 0;
    }


	printf("\nConnecting to device %s - please wait ...\n", devDescriptors[boardNum].DevString);

    //1. set channelType = AI_TC
    err = cbSetConfig(BOARDINFO, boardNum, channel, BIADCHANTYPE, AI_CHAN_TYPE_TC);

    //2. Set TC type
    err = cbSetConfig(BOARDINFO, boardNum, channel, BICHANTCTYPE, TC_TYPE_K);

    //3.  Set Open TC detect
    err = cbSetConfig(BOARDINFO, boardNum, 0, BIDETECTOPENTC, 1);
	  strcpy(otcDetectstr, "OTD_ENABLED");

	//4.  Set the Data_rate
  err = cbSetConfig(BOARDINFO, boardNum, 0, BIADDATARATE, 20);


	printf("\n%s ready\n", devDescriptors[boardNum].DevString);
	printf("       Function demonstrated: ulTIn()\n");
	printf("          Channel: %d\n", channel);
	printf("   Open TC detect: %s\n", otcDetectstr);
	printf("\nHit ENTER to continue\n");

  printf("Hit '^C' to terminate the process\n\n");
	printf("Active DAQ device: %s (%s)\n\n", devDescriptors[boardNum].ProductName, devDescriptors[boardNum].UniqueID);
	while(err == ERR_NO_ERROR)
	{
		// reset the cursor to the top of the display and
		// show the termination message
		//resetCursor();

		// display the data...
    err = cbTIn(boardNum, channel, FAHRENHEIT, &data, NOFILTER);
		if(err == ERR_NO_ERROR)
            printf("Channel %d: %7.3f %20s\n", channel, data, "");
		else if(err == OPENCONNECTION)
        {
			printf("Channel %d: %7.3f (open connection)\n", channel, data);
			err = ERR_NO_ERROR;
		}

		Sleep(250);
	}


end:

	if(err != ERR_NO_ERROR)
	{
		char errMsg[ERR_MSG_LEN];
		cbGetErrMsg(err, errMsg);
		printf("Error Code: %d \n", err);
		printf("Error Message: %s \n", errMsg);
	}

	return 0;
}




