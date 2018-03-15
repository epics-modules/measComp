TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#=============================
# Build the IOC support library

INC += cbw_linux.h

LIBRARY_IOC += measCompLinux

USR_CXXFLAGS_Linux += -Wno-write-strings

LIB_SRCS += ethernet.c 
LIB_SRCS += E-1608.c 
LIB_SRCS += E-DIO24.c 
LIB_SRCS += E-TC32.c 
LIB_SRCS += E-TC.c
LIB_SRCS_Linux += mcBoard.cpp
LIB_SRCS_Linux += mcBoard_E-TC.cpp
LIB_SRCS_Linux += mcBoard_E-1608.cpp

PROD_IOC_Linux += test-E-1608 
PROD_IOC_Linux += test-E-DIO24 
PROD_IOC_Liunx += test-E-TC32 
PROD_IOC_Linux += test-E-TC

PROD_LIBS += measCompLinux
PROD_LIBS += $(EPICS_BASE_IOC_LIBS)


#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
