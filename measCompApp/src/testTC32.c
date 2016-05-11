#include <stdio.h>
#include <windows.h>
#include "cbw.h"

main(int argc, char* argv[]) 
{
  int i, j;
  int status;
  float temperature;
  int boardNum;
  int numInputs;
  int sleepTimeMs;
  int tcType = TC_TYPE_J;
  if (argc != 4) {
    printf("Usage: testTC32 boardNum numInputs sleepTimeMs\n");
    return;
  }
  boardNum    = atoi(argv[1]);
  numInputs   = atoi(argv[2]);
  sleepTimeMs = atoi(argv[3]);
  
  printf("boardNum=%d, numInputs=%d, sleepTimeMs=%d\n", boardNum, numInputs, sleepTimeMs);

  /* Set the thermocouple type */
  for (i=0; i<numInputs; i++) {
    printf("Calling cbSetConfig(BOARDINFO=%d, boardNum=%d, i=%d, BICHANTCTYPE=%d, tcType=%d\n",
      BOARDINFO, boardNum, i, BICHANTCTYPE, tcType);
    status = cbSetConfig(BOARDINFO, boardNum, i, BICHANTCTYPE, tcType);
    printf("Return from cbSetConfig, status=%d\n", status);
    if (sleepTimeMs > 0) Sleep(sleepTimeMs);
  }

  /* Read each thermocouple 5 times in a loop */
  for (j=0; j<5; j++) {
    for (i=0; i<numInputs; i++) {
      printf("Calling cbTIn(boardNum=%d, i=%d, scale=%d, &temperature=%p, filter=%d\n",
        boardNum, i, CELSIUS, &temperature, FILTER);
      status = cbTIn(boardNum, i, CELSIUS, &temperature, FILTER);
      printf("Return from cbTIn, status=%d, temperature=%f\n", status, temperature);
    }
  }
  
  /* Set bits 0, 1, 16, and 17 to 1 on the 32-bit output port (11) */
  status  = cbDBitOut(boardNum, 11,  0, 1);
  status |= cbDBitOut(boardNum, 11,  1, 1);
  status |= cbDBitOut(boardNum, 11, 16, 1);
  status |= cbDBitOut(boardNum, 11, 17, 1);
  printf("called cbDBitOut, status=%d\n", status);
  
}
  
 
