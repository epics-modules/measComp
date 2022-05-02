#ifndef measCompDiscoverInclude
#define measCompDiscoverInclude

#include <string>
#include <shareLib.h>
#ifdef _WIN32
  #include "cbw.h"
#else
  #include "uldaq.h"
#endif

epicsShareFunc int measCompDiscoverDevices();
epicsShareFunc void measCompShowDevices();
epicsShareFunc int measCompCreateDevice(std::string uniqueId, DaqDeviceDescriptor& deviceDescriptor, long long *handle);

#endif /* measCompDiscoverInclude */
