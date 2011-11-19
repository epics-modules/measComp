TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#=============================
# Build the IOC support library

DBD += measCompApp.dbd
DBD += measCompSupport.dbd

LIBRARY_IOC_WIN32 += measComp

# Compile and add the code to the support library
# Link locally-provided code into the support library,
# rather than directly into the IOC application.

LIB_SRCS += drvC9513.cpp
LIB_SRCS += drv1608G.cpp

measComp_LIBS += $(EPICS_BASE_IOC_LIBS)

# Note, the following files were manually copied from the Measurement Computing/DAQ/C directory.
LIB_INSTALLS_WIN32 += ../cbw32.lib ../cbw64.lib

#=============================
# Build the IOC application

PROD_IOC_WIN32 = measCompApp

measCompApp_SRCS += measCompApp_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
measCompApp_SRCS += measCompAppMain.cpp

# Link in the code from the support library
measCompApp_LIBS += measComp
measCompApp_LIBS += std
measCompApp_LIBS += busy
measCompApp_LIBS += calc
measCompApp_LIBS += sscan
measCompApp_LIBS += autosave
measCompApp_LIBS += asyn
measCompApp_LIBS += seq pv
measCompApp_LIBS += cbw32
ifeq (win32-x86, $(findstring win32-x86, $(T_A)))
measCompApp_LIBS += cbw32
else ifeq (windows-x64, $(findstring windows-x64, $(T_A)))
measCompApp_LIBS += cbw32
endif

# Finally link to the EPICS Base libraries
measCompApp_LIBS += $(EPICS_BASE_IOC_LIBS)

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

