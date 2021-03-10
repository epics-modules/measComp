#ifndef measCompDiscoverInclude
#define measCompDiscoverInclude

#include <string>
#include <shareLib.h>

epicsShareFunc int measCompDiscoverDevices();
epicsShareFunc void measCompShowDevices();
epicsShareFunc int measCompCreateDevice(std::string uniqueId);

#endif /* measCompDiscoverInclude */
