#include <stdio.h>
#include <string>

#ifdef _WIN32
  #include "cbw.h"
#else
  #include "uldaq.h"
#endif

#include <measCompDiscover.h>

int main(int argc, char *argv[])
{
  std::string uniqueId="Dummy";

  if (argc > 1) uniqueId = argv[1];
  DaqDeviceDescriptor devDescriptor;
  long long handle;
  int status = measCompCreateDevice(uniqueId, devDescriptor, &handle);
  measCompShowDevices();
  printf("test_measCompFindDevice status=%d\n", status);
}
