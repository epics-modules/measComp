/*
    UL call demonstrated:        	  ulTIn()

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
#include <unistd.h>
#include <memory.h>
#include "uldaq.h"

#define MAX_DEV_COUNT  100
#define MAX_STR_LENGTH 64

void resetCursor() {printf("\033[1;1H");}
void clearEOL() {printf("\033[2K");}
void cursorUp() {printf("\033[A");}
void flush_stdin(void)
{
    int c;
    do {
        c = getchar();
    } while (c != '\n' && c != EOF);
}

int enter_press()
{
	int stdin_value = 0;
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds); //STDIN_FILENO is 0
    select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
    stdin_value = FD_ISSET(STDIN_FILENO, &fds);
    if (stdin_value != 0)
    	flush_stdin();
    return stdin_value;
}

int main(void)
{
    DaqDeviceDescriptor devDescriptors[MAX_DEV_COUNT];
	DaqDeviceInterface interfaceType = USB_IFC;  //ANY_IFC;
	DaqDeviceHandle daqDeviceHandle = 0;
	unsigned int numDevs = MAX_DEV_COUNT;

	// set some variables used to acquire data
    int boardNum = 0;
    int boardFound = 0;
	int channel = 0;
	TempScale scale = TS_FAHRENHEIT;  //TS_CELSIUS;
	TInFlag flags = TIN_FF_DEFAULT;

	char otcDetectstr[MAX_STR_LENGTH];

	int i = 0;
	double data = 0;
	UlError err = ERR_NO_ERROR;

	int __attribute__((unused)) ret;
	char c;

	// Get descriptors for all of the available DAQ devices
	err = ulGetDaqDeviceInventory(interfaceType, devDescriptors, &numDevs);

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
		if (strstr(devDescriptors[i].productName, "USB-2408"))
		{
            printf("  [%d] %s: (%s)\n", i, devDescriptors[i].productName, devDescriptors[i].uniqueId);
            // get a handle to the DAQ device associated with the first descriptor
            daqDeviceHandle = ulCreateDaqDevice(devDescriptors[i]);
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


	if (daqDeviceHandle == 0)
	{
		printf ("\nUnable to create a handle to the specified DAQ device\n");
		goto end;
	}


	printf("\nConnecting to device %s - please wait ...\n", devDescriptors[boardNum].devString);

	// establish a connection to the DAQ device
	err = ulConnectDaqDevice(daqDeviceHandle);

	if (err != ERR_NO_ERROR)
		goto end;

    //1. set channelType = AI_TC
    err = ulAISetConfig(daqDeviceHandle, AI_CFG_CHAN_TYPE, channel, AI_TC);


    //2. Set TC type
    err = ulAISetConfig(daqDeviceHandle, AI_CFG_CHAN_TC_TYPE , channel, TC_K);

    //3.  Set Open TC detect
	err = ulAISetConfig(daqDeviceHandle, AI_CFG_CHAN_OTD_MODE, channel, OTD_ENABLED);
	strcpy(otcDetectstr, "OTD_ENABLED");

	//4.  Set the Data_rate
	err = ulAISetConfigDbl(daqDeviceHandle, AI_CFG_CHAN_DATA_RATE, channel, 20);


	printf("\n%s ready\n", devDescriptors[boardNum].devString);
	printf("       Function demonstrated: ulTIn()\n");
	printf("          Channel: %d\n", channel);
	printf("   Open TC detect: %s\n", otcDetectstr);
	printf("\nHit ENTER to continue\n");

	ret = scanf("%c", &c);

	ret = system("clear");

	while(err == ERR_NO_ERROR && !enter_press())
	{
		// reset the cursor to the top of the display and
		// show the termination message

		/* resetCursor();
		//printf("Hit 'Enter' to terminate the process\n\n");
		//printf("Active DAQ device: %s (%s)\n\n", devDescriptors[boardNum].productName, devDescriptors[boardNum].uniqueId);
         */

		// display the data...
		err = ulTIn(daqDeviceHandle, channel, scale, flags, &data);
		if(err == ERR_NO_ERROR)
            printf("Channel %d: %7.3f %20s\n", channel, data, "");
		else if(err == ERR_OPEN_CONNECTION)
        {
			printf("Channel %d: %7.3f (open connection)\n", channel, data);
			err = ERR_NO_ERROR;
		}

		usleep(10000);
		//usleep(100000);
	}

	// disconnect from the DAQ device
	ulDisconnectDaqDevice(daqDeviceHandle);

end:

	// release the handle to the DAQ device
	if(daqDeviceHandle)
		ulReleaseDaqDevice(daqDeviceHandle);

	if(err != ERR_NO_ERROR)
	{
		char errMsg[ERR_MSG_LEN];
		ulGetErrMsg(err, errMsg);
		printf("Error Code: %d \n", err);
		printf("Error Message: %s \n", errMsg);
	}

	return 0;
}




