TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#=============================
# Build the IOC support library

INC += cbw_linux.h

USR_CXXFLAGS_Linux += -fmax-errors=20

LIBRARY_IOC += cbw_linux

LIB_SRCS_Linux += cbw_linux.cpp
LIB_SRCS_Linux += mcBoard.cpp
LIB_SRCS_Linux += mcBoard_E-TC.cpp
LIB_SRCS_Linux += mcBoard_E-TC32.cpp
LIB_SRCS_Linux += mcBoard_E-1608.cpp
LIB_SRCS_Linux += mcBoard_E-DIO24.cpp
LIB_SRCS_Linux += mcBoard_USB-CTR.cpp

PROD_LIBS += $(EPICS_BASE_IOC_LIBS)


#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
