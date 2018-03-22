#ifndef cbwLinuxInclude
#define cbwLinuxInclude

#ifdef linux
  int cbAddBoard(const char *boardName, const char *address);
  #include <stdlib.h>
  #include <string.h>
  typedef char CHAR;
  typedef unsigned char BYTE;
  typedef unsigned short USHORT;
  typedef unsigned int UINT;
  typedef unsigned long long ULONGLONG;
  typedef int INT;
  typedef int LONG;
  typedef unsigned int ULONG;
  typedef void * HGLOBAL;
  #define __stdcall
#endif

#include <cbw.h>

#endif /* cbwLinuxInclude */
