TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

ifeq ($(HAVE_HIDAPI), YES)
  LIBRARY_IOC_Linux += measCompLinuxDrivers
endif

INC += mccEthernet.h
INC += pmd.h
INC += E-1608.h
INC += E-DIO24.h
INC += E-TC.h
INC += E-TC32.h
INC += usb-ctr.h
INC += usb-tc-ai.h
INC += usb-temp.h

LIB_SRCS += mccEthernet.c 
LIB_SRCS += pmd.c
LIB_SRCS += E-1608.c 
LIB_SRCS += E-DIO24.c 
LIB_SRCS += E-TC32.c 
LIB_SRCS += E-TC.c
LIB_SRCS += usb-ctr.c
LIB_SRCS += usb-tc-ai.c
LIB_SRCS += usb-temp.c

ifeq ($(HAVE_HIDAPI), YES)
  PROD_IOC_Linux += test-E-1608 
  PROD_IOC_Linux += test-E-DIO24 
  PROD_IOC_Linux += test-E-TC32 
  PROD_IOC_Linux += test-E-TC
  PROD_IOC_Linux += test-usb-ctr
  PROD_IOC_Linux += test-usb-temp-ai
  PROD_IOC_Linux += test-usb-temp
  #PROD_IOC_Linux += test-usb-ctr-scaler
  #PROD_IOC_Linux += test-usb-ctr-mcs
endif

PROD_LIBS += measCompLinuxDrivers
PROD_SYS_LIBS += hidapi-libusb usb-1.0
PROD_LIBS += $(EPICS_BASE_IOC_LIBS)


#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
