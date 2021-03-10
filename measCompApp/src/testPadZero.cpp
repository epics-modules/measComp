#include <stdio.h>
#include <inttypes.h>
#ifdef linux
  #include "cbw_linux.h"
#else
  #include "cbw.h"
#endif

#include <measCompDiscover.h>

// Test program for USB-CTR

#define CHAN_COUNT 4

int main(int argc, char *argv[])
{
  int boardNum = 0;
  int i;
  int status=0;
  int options = 0;
  int mode;
  long count;
  int numPoints = 100;
  long rate = 100;
  long pretrigCount = 0;
  int chanCount = CHAN_COUNT;
  int timerNum=0;
  double frequency;
  double delay=0.;
  double dutyCycle=.50;
  int idleState = IDLE_LOW;
  HGLOBAL inputMemHandle;
  short chanArray[CHAN_COUNT];
  short chanTypeArray[CHAN_COUNT];
  short gainArray[CHAN_COUNT];
  short ctrStatus;
  long ctrCount, ctrIndex;

  // Use the serial number for the USB-CTR here.
  boardNum = measCompCreateDevice("123456");

#ifdef linux
  asynUser *pasynUser = pasynManager->createAsynUser(0, 0);
  pasynTrace->setTraceMask(0, 255);
  cbSetAsynUser(boardNum, pasynUser);
#endif

  inputMemHandle  = cbWinBufAlloc32(1000);
  for (i=0; i<CHAN_COUNT; i++) gainArray[i] = 0;

  // Start pulse generator 0 at 1 MHz
  frequency = 1.e6;
  timerNum = 0;
  count = 0;
  status = cbPulseOutStart(boardNum, count, &frequency, &dutyCycle, 0, &delay, idleState, 0);
  if (status) {
    printf("Error calling cbPulseOutStart timerNum=%d frequency=%f, dutyCycle=%f,"
      " count=%ld, delay=%f, idleState=%d, status=%d\n",
      timerNum, frequency, dutyCycle, count, delay, idleState, status);
    return status;
  }

  // Start pulse generator 2 at 10 Hz
  frequency = 10.;
  timerNum = 1;
  status = cbPulseOutStart(boardNum, timerNum, &frequency, &dutyCycle, count, &delay, idleState, 0);
  if (status) {
    printf("Error calling cbPulseOutStart timerNum=%d frequency=%f, dutyCycle=%f,"
      " count=%ld, delay=%f, idleState=%d, status=%d\n",
      timerNum, frequency, dutyCycle, count, delay, idleState, status);
    return status;
  }

  // Configure counter 0
  count = 0;
  mode = OUTPUT_ON | COUNT_DOWN_OFF | CLEAR_ON_READ;
  status = cbCConfigScan(boardNum, count, mode, CTR_DEBOUNCE_NONE, CTR_TRIGGER_BEFORE_STABLE,
                           CTR_RISING_EDGE, CTR_TICK20PT83ns, 0);
  if (status) {
    printf("Error calling cbCConfigScan, counter=%d, mode=0x%x, status=%d\n",
           i, mode, status);
  }

  options |= BACKGROUND;
  chanArray[0] = 0;
  chanTypeArray[0] = CTRBANK0;
  chanArray[1] = 0;
  chanTypeArray[1] = CTRBANK1;
  chanArray[2] = AUXPORT;
  chanTypeArray[2] = DIGITAL8;
  chanArray[3] = 0;
  chanTypeArray[3] = PADZERO;
  chanCount = 4;
  count = chanCount * numPoints;

  status = cbDaqInScan(boardNum, chanArray, chanTypeArray, gainArray, chanCount, &rate,
                       &pretrigCount, &count, inputMemHandle, options);
  printf("Called cbDaqInScan, chanCount=%d, count=%ld, rate=%ld, inputMemHandle_=%p, options=0x%x, status=%d\n",
         chanCount, count, rate, inputMemHandle, options, status);
  if (status) {
    printf("Error calling cbDaqInScan, status=%d\n", status);
  }

  // Wait for scan to complete
  while(ctrCount != count) {
    status = cbGetStatus(boardNum, &ctrStatus, &ctrCount, &ctrIndex, DAQIFUNCTION);
  }
  printf("cbGetStatus returned status=%d, ctrStatus=%d, ctrCount=%ld, ctrIndex=%ld\n",
         status, ctrStatus, ctrCount, ctrIndex);
  uint32_t *pData = (uint32_t *)inputMemHandle;
  int numReadings = ctrCount/4;
  for (i=0; i<numReadings; i++){
    printf("Point %d\n", i);
    printf("     Counter 0: 0x%x\n", pData[2*i]);
    printf("   Digital I/O: 0x%x\n", pData[2*i+1]);
  }
}
