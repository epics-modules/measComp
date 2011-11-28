TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#=============================
# Build the IOC support library

DBD += measCompApp.dbd
DBD += measCompSupport.dbd

LIBRARY_IOC_WIN32 += measComp
LIBRARY_IOC_cygwin32 += measComp

# Compile and add the code to the support library
# Link locally-provided code into the support library,
# rather than directly into the IOC application.

LIB_SRCS += drvC9513.cpp
LIB_SRCS += drv1608G.cpp

measComp_LIBS += $(EPICS_BASE_IOC_LIBS)

# Note, the following files were manually copied from the Measurement Computing/DAQ/C directory.
ifeq (win32-x86, $(findstring win32-x86, $(T_A)))
LIB_INSTALLS += ../cbw32.lib
else ifeq (cygwin-x86, $(findstring cygwin-x86, $(T_A)))
LIB_INSTALLS += ../cbw32.lib
else ifeq (windows-x64, $(findstring windows-x64, $(T_A)))
LIB_INSTALLS += ../cbw64.lib
endif

#=============================
# Build the IOC application

PROD_IOC_WIN32 += measCompApp
PROD_IOC_cygwin32 += measCompApp

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
else ifeq (cygwin-x86, $(findstring cygwin-x86, $(T_A)))
measCompApp_LIBS += cbw32
else ifeq (windows-x64, $(findstring windows-x64, $(T_A)))
measCompApp_LIBS += cbw64
endif
# Finally link to the EPICS Base libraries
measCompApp_LIBS += $(EPICS_BASE_IOC_LIBS)

PROD_IOC_WIN32 += test_1608G_1
PROD_IOC_cygwin32 += test_1608G_1
test_1608G_1_SRCS += test_1608G_1.c
ifeq (win32-x86, $(findstring win32-x86, $(T_A)))
test_1608G_1_LIBS += cbw32
else ifeq (cygwin-x86, $(findstring cygwin-x86, $(T_A)))
test_1608G_1_LIBS += cbw32
else ifeq (windows-x64, $(findstring windows-x64, $(T_A)))
test_1608G_1_LIBS += cbw64
endif


#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

