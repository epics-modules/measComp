#!../../bin/linux-x86_64/MEXDIO24IOC01
# 
# $File: //ASP/tec/daq/measComp/trunk/iocBoot/iocEDIO24/st.cmd $
# $Revision: #1 $
# $DateTime: 2020/05/12 13:32:37 $
# Last checked in by: $Author: wongd $
#

## You may have to change MEXDIO24IOC01 to something else
## everywhere it appears in this file

< envPaths

# Usually set by epics.service script
epicsEnvSet ("IOCNAME", "MEXDIO24IOC01")

epicsEnvSet ("IOCSH_PS1","MEXDIO24IOC01> ")

epicsEnvSet("DEVNAME","MEXDAQEDIO24")

cd ${TOP}

## Register all support components
dbLoadDatabase "dbd/MEXDIO24IOC01.dbd"
MEXDIO24IOC01_registerRecordDeviceDriver pdbbase

epicsEnvSet(INPUT_POINTS, "4096")
epicsEnvSet(OUTPUT_POINTS, "4096")

cbAddBoard("E-DIO24", "10.23.92.187")

## Configure port driver
# MultiFunctionConfig((portName,        # The name to give to this asyn port driver
#                      boardNum,        # The number of this board assigned by the Measurement Computing Instacal program 
#                      maxInputPoints,  # Maximum number of input points for waveform digitizer
#                      maxOutputPoints) # Maximum number of output points for waveform generator
# MultiFunctionConfig("E1608_1", 0, $(INPUT_POINTS), $(OUTPUT_POINTS))
MultiFunctionConfig("EDIO24_1", 0, 0, 0)

#asynSetTraceMask E1608_1 -1 255

## Load record instances
#
# Allow epics service script to initiate clean shutdown by performing
#   caput MEXDIO24IOC01:exit.PROC 1
#
dbLoadRecords ("${EPICS_BASE}/db/softIocExit.db", "IOC=${IOCNAME}")

# Load standard IOC (and host) monitoring records.
#
dbLoadRecords ("${IOCSTATUS}/db/IocStatus.template", "IOC=${IOCNAME}")

#dbLoadTemplate("E1608.substitutions")
#dbLoadTemplate("MEXDIO24IOC01.substitutions")
dbLoadRecords ("db/MEXDIO24IOC01.db")

# TODO: Use our standard autosave setup
< save_restore.cmd

iocInit

# Catch SIGINT and SIGTERM - do an orderly shutdown
#
catch_sigint
catch_sigterm

# Update the firewall to allow use of arbitary port number
#
system firewall_update

# Dump all record names
#
dbl > /asp/logs/ioc/${IOCNAME}/${IOC}.dbl

## Autosave monitor set-up.
#
create_monitor_set("auto_settings.req", 30)

# Need to force the time arrays to process because the records are scan=I/O Intr
# but asynPortDriver does not do array callbacks before iocInit.

# dbpf MEXDAQEDIO24:WaveDigDwell.PROC 1

# end
