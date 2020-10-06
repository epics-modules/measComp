TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#=============================
# Build the IOC support library

INC += cbw_linux.h

ifeq ($(HAVE_HIDAPI), YES)
  LIBRARY_IOC_Linux += cbw_linux
endif

LIB_SRCS += cbw_linux.cpp
LIB_SRCS += mcBoard.cpp
LIB_SRCS += mcBoard_E-TC.cpp
LIB_SRCS += mcBoard_E-TC32.cpp
LIB_SRCS += mcBoard_E-1608.cpp
LIB_SRCS += mcBoard_E-DIO24.cpp
LIB_SRCS += mcBoard_USB-CTR.cpp

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
