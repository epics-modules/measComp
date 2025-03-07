< envPaths

## Register all support components
dbLoadDatabase "$(MEASCOMP)/dbd/measCompApp.dbd"
measCompApp_registerRecordDeviceDriver pdbbase

epicsEnvSet("PREFIX",        "USBERB24:")
epicsEnvSet("PORT",          "USBERB24_1")
epicsEnvSet("UNIQUE_ID",     "020D2606")

## Configure port driver
# MultiFunctionConfig((portName,        # The name to give to this asyn port driver
#                      uniqueID,        # For USB the serial number.  For Ethernet the MAC address or IP address.
#                      maxInputPoints,  # Maximum number of input points for waveform digitizer
#                      maxOutputPoints) # Maximum number of output points for waveform generator
MultiFunctionConfig("$(PORT)", "$(UNIQUE_ID)", 1, 1)

#asynSetTraceMask($(PORT), -1, ERROR|FLOW|DRIVER)

# Use this file for Windows
dbLoadTemplate("$(MEASCOMP)/db/USBERB24.substitutions", "P=$(PREFIX),PORT=$(PORT)")
# Use this file for Linux
#dbLoadTemplate("$(MEASCOMP)/db/USBERB24_Linux.substitutions", "P=$(PREFIX),PORT=$(PORT)")

< ../save_restore.cmd

iocInit

create_monitor_set("auto_settings.req",30,"P=$(PREFIX)")
