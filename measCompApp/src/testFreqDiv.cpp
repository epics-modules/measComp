#include <asynDriver.h>
#include <epicsThread.h>

#ifdef linux
  #include "cbw_linux.h"
#else
  #include "cbw.h"
#endif

int main(int argc, char *argv[])
{
  int boardNum_ = 0;
  asynUser *pasynUserSelf;
  int i;
  int status=0;
  int firstMCSCounter_ = 0;
  int lastMCSCounter_ = 7;
  int numMCSCounters_ = (lastMCSCounter_ - firstMCSCounter_ + 1);
  int prescaleCounter = 7;
  int prescale = 4;
  int options;
  int mode;
  int numPoints = 2048;
  double dwell = 0.01;
  double rateFactor;
  int timerNum=3;
  double frequency=1000.;
  double delay=0.;
  double dutyCycle=.10;
  int idleState = IDLE_LOW;
  int count=0;
  LONG rate; 
  HGLOBAL inputMemHandle_;
  static const char *driverName = "testFreqDiv";
  static const char *functionName = "";
  
  pasynUserSelf = pasynManager->createAsynUser(0, 0);
  pasynTrace->setTraceMask(0, ASYN_TRACE_ERROR | ASYN_TRACEIO_DRIVER);

#ifdef linux
  cbAddBoard("USB-CTR", "");
  cbSetAsynUser(boardNum_, pasynUserSelf);
#endif

  inputMemHandle_  = cbWinBufAlloc32(1000000);

  // Start pulse generator 3 at 1 kHz
  status = cbPulseOutStart(boardNum_, timerNum, &frequency, &dutyCycle, count, &delay, idleState, 0);
  if (status) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
      "%s::%s error calling cbPulseOutStart timerNum=%d frequency=%f, dutyCycle=%f,"
      " count=%d, delay=%f, idleState=%d, status=%d\n",
      driverName, functionName, timerNum, frequency, dutyCycle, 
      count, delay, idleState, status);
    return status;
  }
  

  for (i=firstMCSCounter_; i<=lastMCSCounter_; i++) {
    mode = OUTPUT_ON | COUNT_DOWN_OFF | CLEAR_ON_READ;
    status = cbCConfigScan(boardNum_, i, mode, CTR_DEBOUNCE_NONE, CTR_TRIGGER_BEFORE_STABLE, 
                           CTR_RISING_EDGE, CTR_TICK20PT83ns, 0);
    if (status) {
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
        "%s::%s error calling cbCConfigScan, counter=%d, mode=0x%x, status=%d\n",
        driverName, functionName, i, mode, status);
    }
  }
  
  mode = OUTPUT_ON | COUNT_DOWN_OFF | RANGE_LIMIT_ON;
  status = cbCLoad32(boardNum_, OUTPUTVAL0REG0+prescaleCounter, 0); 
  status = cbCLoad32(boardNum_, OUTPUTVAL1REG0+prescaleCounter, prescale-1); 
  status = cbCLoad32(boardNum_, MAXLIMITREG0+prescaleCounter, prescale-1); 
  status = cbCConfigScan(boardNum_, prescaleCounter, mode, CTR_DEBOUNCE_NONE, CTR_TRIGGER_BEFORE_STABLE, 
                         CTR_RISING_EDGE, CTR_TICK20PT83ns, 0);
  if (status) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
      "%s::%s error calling cbCConfigScan, counter=%d, mode=0x%x, status=%d\n",
      driverName, functionName, prescaleCounter, mode, status);
  }

  if (dwell > 1e-6) rateFactor = 1000.;
  rate = (long)((rateFactor / dwell) + 0.5);
  count =  numMCSCounters_ * numPoints;
  
  if (dwell > 1e-4) {
    options = CTR32BIT;
  } else {
    options = CTR16BIT;
  }
  options   |= BACKGROUND;
  if (rateFactor > 1.0) 
    options |= HIGHRESRATE;
  options |= EXTTRIGGER;
  options |= EXTCLOCK;

  status = cbSetTrigger(boardNum_, TRIG_LOW, 0, 0);
  
  status = cbCInScan(boardNum_, firstMCSCounter_, lastMCSCounter_, count, &rate,
                     inputMemHandle_, options);
  if (status) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
      "%s::%s error calling cbCInScan, firstMCSCounter=%d, lastMCSCounter=%d, count=%d, rate=%d,"
      " inputMemHandle_=%p, options=0x%x, status=%d\n",
      driverName, functionName, firstMCSCounter_, lastMCSCounter_, count, rate,
      inputMemHandle_, options, status);
  }
  // Wait for 10 seconds and then turn off the pulse generator
  epicsThreadSleep(10.);
  status = cbPulseOutStop(boardNum_, timerNum);
}