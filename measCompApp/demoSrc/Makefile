TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#=============================
# Build the IOC support library

DBD += measCompDemoApp.dbd
DBD += measCompDemoSupport.dbd

#=============================
# Build the IOC application

PROD_IOC_WIN32 += measCompDemoAppV1
measCompDemoAppV1_SRCS += drv1608G_V1
PROD_IOC_WIN32 += measCompDemoAppV2
measCompDemoAppV2_SRCS += drv1608G_V2
PROD_IOC_WIN32 += measCompDemoAppV3
measCompDemoAppV3_SRCS += drv1608G_V3
PROD_IOC_WIN32 += measCompDemoAppV4
measCompDemoAppV4_SRCS += drv1608G_V4
PROD_IOC_WIN32 += measCompDemoAppV5
measCompDemoAppV5_SRCS += drv1608G_V5
PROD_IOC_WIN32 += measCompDemoApp
measCompDemoApp_SRCS += drv1608G

PROD_SRCS += measCompDemoApp_registerRecordDeviceDriver.cpp
PROD_SRCS += measCompDemoAppMain.cpp
PROD_LIBS += asyn
ifeq (win32-x86, $(findstring win32-x86, $(T_A)))
PROD_LIBS += cbw32
else ifeq (cygwin-x86, $(findstring cygwin-x86, $(T_A)))
PROD_LIBS += cbw32
else ifeq (windows-x64, $(findstring windows-x64, $(T_A)))
PROD_LIBS += cbw64
endif
PROD_LIBS += $(EPICS_BASE_IOC_LIBS)


#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

