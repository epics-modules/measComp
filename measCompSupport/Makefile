TOP=..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

# Note, the following files were manually copied from the Measurement Computing/DAQ/C directory.
INC += cbw.h

ifeq (win32-x86, $(findstring win32-x86, $(T_A)))
LIB_INSTALLS += ../os/win32-x86/cbw32.lib

else ifeq (windows-x64, $(findstring windows-x64, $(T_A)))
LIB_INSTALLS += ../os/windows-x64/cbw64.lib

endif

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

