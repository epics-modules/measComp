#include <stdio.h>
#include <string>
#ifdef linux
  #include "cbw_linux.h"
#else
  #include "cbw.h"
#endif

int measCompFindDevice(std::string uniqueId);
void measCompShowDevices();

int main(int argc, char *argv[])
{
  std::string uniqueId="Dummy";

  if (argc > 1) uniqueId = argv[1];
  int devNum=measCompFindDevice(uniqueId);
  measCompShowDevices();
  printf("test_measCompFindDevice devNum=%d\n", devNum);
}
