TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#=============================
# Build the IOC support library

USR_CXXFLAGS_Linux += -Wno-write-strings

ifdef ULDAQ_INCLUDE
  USR_INCLUDES_Linux += -I$(ULDAQ_INCLUDE)
endif

ifdef ULDAQ_DIR
  uldaq_DIR = $(ULDAQ_DIR)
endif

DBD += measCompApp.dbd
DBD += measCompSupport.dbd

LIBRARY_IOC_WIN32 += measComp
ifeq ($(HAVE_ULDAQ), YES)
  LIBRARY_IOC_Linux+= measComp
endif

# Compile and add the code to the support library
# Link locally-provided code into the support library,
# rather than directly into the IOC application.

measComp_SRCS += drvMultiFunction.cpp
measComp_SRCS += drvUSBCTR.cpp
measComp_SRCS += measCompDiscover.cpp
measComp_SRCS += USBCTR_SNL.st

measComp_LIBS += asyn
measComp_LIBS += seq pv
measComp_LIBS += $(EPICS_BASE_IOC_LIBS)

#=============================
# Build the IOC application

PROD_IOC_WIN32 += measCompApp
ifeq ($(HAVE_ULDAQ), YES)
  PROD_IOC_Linux += measCompApp
endif
measCompApp_SRCS += measCompApp_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
measCompApp_SRCS += measCompAppMain.cpp

# Link in the code from the support library
measCompApp_LIBS += measComp
measCompApp_LIBS += scaler
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
ifdef ULDAQ_DIR
  measCompApp_LIBS_Linux += uldaq
else
  measCompApp_SYS_LIBS_Linux += uldaq
endif
measCompApp_SYS_LIBS_Linux += usb-1.0

# Finally link to the EPICS Base libraries
measCompApp_LIBS += $(EPICS_BASE_IOC_LIBS)

# This tests needing to read each analog input twice on USB-1608G.
#PROD_IOC_WIN32 += test_1608G_1
#test_1608G_1_SRCS += test_1608G_1.c
#ifeq (win32-x86, $(findstring win32-x86, $(T_A)))
#  test_1608G_1_LIBS += cbw32
#else ifeq (windows-x64, $(findstring windows-x64, $(T_A)))
#  test_1608G_1_LIBS += cbw64
#endif

# This tests some issues reading thermocouples on TC-32.
#PROD_IOC_WIN32 += testTC32
#testTC32_SRCS += testTC32.c
#ifeq (win32-x86, $(findstring win32-x86, $(T_A)))
#  testTC32_LIBS += cbw32
#else ifeq (windows-x64, $(findstring windows-x64, $(T_A)))
#  testTC32_LIBS += cbw64
#endif

# This program lists all Measurement Computing devices on USB or local Ethernet subnet
PROD_IOC_WIN32 += test_measCompDiscover
ifeq ($(HAVE_ULDAQ), YES)
  PROD_IOC_Linux += test_measCompDiscover
endif
test_measCompDiscover_SRCS += test_measCompDiscover.cpp
test_measCompDiscover_LIBS += measComp
ifeq (win32-x86, $(findstring win32-x86, $(T_A)))
  test_measCompDiscover_LIBS += cbw32
  test_measCompDiscover_LIBS += cbw32
else ifeq (windows-x64, $(findstring windows-x64, $(T_A)))
  test_measCompDiscover_LIBS += cbw64
endif
ifdef ULDAQ_DIR
  test_measCompDiscover_LIBS_Linux += uldaq
else
  test_measCompDiscover_SYS_LIBS_Linux += uldaq
endif
test_measCompDiscover_SYS_LIBS_Linux += usb-1.0
test_measCompDiscover_LIBS += $(EPICS_BASE_IOC_LIBS)

# These are test programs which were used when finding Ubuntu18 had problems with scans > 1000 points/s
#PROD_IOC_Linux += test_AInScan_Linux
#ifdef ULDAQ_DIR
#  test_AInScan_Linux_LIBS += uldaq
#else
#  test_AInScan_Linux_SYS_LIBS += uldaq
#endif
#test_AInScan_Linux_SYS_LIBS += usb-1.0

#PROD_IOC_Linux += test_USB_CTR
#ifdef ULDAQ_DIR
#  test_USB_CTR_LIBS_Linux += uldaq
#else
#  test_USB_CTR_SYS_LIBS_Linux += uldaq
#endif
#test_USB_CTR_SYS_LIBS_Linux += usb-1.0
#PROD_IOC_WIN32 += test_USB_CTR
#test_USB_CTR_LIBS_WIN32 += cbw64

#PROD_IOC_Linux += DaqInScan
#DaqInScan_SRCS += DaqInScan.c
#ifdef ULDAQ_DIR
#  DaqInScan_LIBS_Linux += uldaq
#else
#  DaqInScan_SYS_LIBS_Linux += uldaq
#endif
#DaqInScan_SYS_LIBS_Linux += usb-1.0

# This test program shows temperature glitches on Linux with USB-2408 and data rate <= 20 Hz.
#PROD_IOC_Linux += 2408_TIn
#2408_TIn_SRCS += 2408_TIn.c
#ifdef ULDAQ_DIR
#  2408_TIn_LIBS_Linux += uldaq
#else
#  2408_TIn_SYS_LIBS_Linux += uldaq
#endif
#2408_TIn_SYS_LIBS_Linux += usb-1.0

#PROD_IOC_Linux += test_USB_CTR_fdiv
#ifdef ULDAQ_DIR
#  test_USB_CTR_fdiv_LIBS_Linux += uldaq
#else
#  test_USB_CTR_fdiv_SYS_LIBS_Linux += uldaq
#endif
#test_USB_CTR_fdiv_SYS_LIBS_Linux += usb-1.0
#PROD_IOC_WIN32 += test_USB_CTR_fdiv
#test_USB_CTR_fdiv_LIBS_WIN32 += cbw64

PROD_IOC_Linux += testDualEthDevice
ifdef ULDAQ_DIR
  testDualEthDevice_LIBS_Linux += uldaq
else
  testDualEthDevice_SYS_LIBS_Linux += uldaq
endif
testDualEthDevice_SYS_LIBS_Linux += usb-1.0 pthread
#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

