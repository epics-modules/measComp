TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

LIBRARY_IOC_Linux += measCompLinuxDrivers

INC += ethernet.h
INC += pmd.h
INC += E-1608.h
INC += E-DIO24.h
INC += E-TC.h
INC += E-TC32.h
INC += usb-ctr.h

LIB_SRCS += ethernet.c 
LIB_SRCS += pmd.c
LIB_SRCS += E-1608.c 
LIB_SRCS += E-DIO24.c 
LIB_SRCS += E-TC32.c 
LIB_SRCS += E-TC.c
LIB_SRCS += usb-ctr.c

PROD_IOC_Linux += test-E-1608 
PROD_IOC_Linux += test-E-DIO24 
PROD_IOC_Linux += test-E-TC32 
PROD_IOC_Linux += test-E-TC
PROD_IOC_Linux += test-usb-ctr

PROD_LIBS += measCompLinuxDrivers
PROD_SYS_LIBS += hidapi-libusb usb-1.0
PROD_LIBS += $(EPICS_BASE_IOC_LIBS)


#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
