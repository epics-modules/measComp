TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#=============================
# Build the IOC support library

USR_CXXFLAGS_Linux += -Wno-write-strings

DBD += measCompApp.dbd
DBD += measCompSupport.dbd

LIBRARY_IOC_WIN32 += measComp
LIBRARY_IOC_Linux+= measComp
#LIBRARY_IOC_cygwin32 += measComp

# Compile and add the code to the support library
# Link locally-provided code into the support library,
# rather than directly into the IOC application.

LIB_SRCS += drvC9513.cpp
LIB_SRCS += drvMultiFunction.cpp
LIB_SRCS += drvUSBCTR.cpp
LIB_SRCS += USBCTR_SNL.st

measComp_LIBS += asyn
measComp_LIBS += seq pv
measComp_LIBS += $(EPICS_BASE_IOC_LIBS)

#=============================
# Build the IOC application

PROD_IOC_WIN32 += measCompApp
PROD_IOC_Linux += measCompApp

measCompApp_SRCS += measCompApp_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
measCompApp_SRCS += measCompAppMain.cpp

# Link in the code from the support library
measCompApp_LIBS += measComp
measCompApp_LIBS_Linux += cbw_linux
measCompApp_LIBS_Linux += measCompLinuxDrivers
measCompApp_LIBS += std
measCompApp_LIBS += busy
measCompApp_LIBS += calc
measCompApp_LIBS += mca
measCompApp_LIBS += sscan
measCompApp_LIBS += autosave
measCompApp_LIBS += asyn
measCompApp_LIBS += seq pv
ifeq (win32-x86, $(findstring win32-x86, $(T_A)))
  measComp_LIBS += cbw32
  measCompApp_LIBS += cbw32
else ifeq (windows-x64, $(findstring windows-x64, $(T_A)))
  measComp_LIBS += cbw64
  measCompApp_LIBS += cbw64
endif
# Finally link to the EPICS Base libraries
measCompApp_LIBS += $(EPICS_BASE_IOC_LIBS)

PROD_IOC_WIN32 += test_1608G_1
test_1608G_1_SRCS += test_1608G_1.c
ifeq (win32-x86, $(findstring win32-x86, $(T_A)))
test_1608G_1_LIBS += cbw32
else ifeq (windows-x64, $(findstring windows-x64, $(T_A)))
test_1608G_1_LIBS += cbw64
endif

PROD_IOC_WIN32 += testTC32
testTC32_SRCS += testTC32.c
ifeq (win32-x86, $(findstring win32-x86, $(T_A)))
testTC32_LIBS += cbw32
else ifeq (windows-x64, $(findstring windows-x64, $(T_A)))
testTC32_LIBS += cbw64
endif


#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

