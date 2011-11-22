#include <stdio.h>
#include "cbw.h"

#define BOARD_NUM 1
#define MAX_POINTS 2048

int main(int argc, char *argv[])
{
  int numLoops=10;
  long pointsPerSecond = 1000;
  int loop, chan;
  int status;
  unsigned short sData;
  float fData;
  double *pdData;
  HGLOBAL inputMemHandle;

  /* Set output channel 0 to 0V, channel 1 to +5V */
  status  = cbFromEngUnits(BOARD_NUM, BIP10VOLTS, 0.0, &sData);
  status |= cbAOut(BOARD_NUM, 0, BIP10VOLTS, sData);
  status |= cbFromEngUnits(BOARD_NUM, BIP10VOLTS, 5.0, &sData);
  status |= cbAOut(BOARD_NUM, 1, BIP10VOLTS, sData);
  
  /* First read each channel separately */
  printf("\nReading channel 0\n");
  for (loop=0; loop<numLoops; loop++) {
    chan = 0;
    status |= cbAIn(BOARD_NUM, chan, BIP10VOLTS, &sData);
    status |= cbToEngUnits(BOARD_NUM, BIP10VOLTS, sData, &fData);
    printf("Channel %d raw=%d, volts=%f\n", chan, sData, fData);
  }
  printf("\nReading channel 1\n");
  for (loop=0; loop<numLoops; loop++) {
    chan = 1;
    status |= cbAIn(BOARD_NUM, chan, BIP10VOLTS, &sData);
    status |= cbToEngUnits(BOARD_NUM, BIP10VOLTS, sData, &fData);
    printf("Channel %d raw=%d, volts=%f\n", chan, sData, fData);
  }
  /* Now switch between them */
  printf("\nAlternating between channel 0 and channel 1\n");
  for (loop=0; loop<numLoops; loop++) {
    for (chan=0; chan<=1; chan++) {
      status |= cbAIn(BOARD_NUM, chan, BIP10VOLTS, &sData);
      status |= cbToEngUnits(BOARD_NUM, BIP10VOLTS, sData, &fData);
      printf("Channel %d raw=%d, volts=%f\n", chan, sData, fData);
      status |= cbAIn(BOARD_NUM, chan, BIP10VOLTS, &sData);
      status |= cbToEngUnits(BOARD_NUM, BIP10VOLTS, sData, &fData);
      printf("Channel %d raw=%d, volts=%f\n", chan, sData, fData);
    }
  }

  inputMemHandle  = cbScaledWinBufAlloc(2*MAX_POINTS);
  pdData = (double *)inputMemHandle;
  status |= cbAInScan(BOARD_NUM, 0, 1, 2*MAX_POINTS, &pointsPerSecond, BIP10VOLTS,
                      inputMemHandle, SCALEDATA|BURSTMODE);
  printf("\nScan data\n");
  printf("Channel 0 = %8f %8f %8f ...\n", pdData[0], pdData[2], pdData[4]);
  printf("Channel 1 = %8f %8f %8f ...\n", pdData[1], pdData[3], pdData[5]);

  if (status != 0) printf("Error: status=%d\n", status);
  return status;
}
      

