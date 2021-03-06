< envPaths

## Register all support components
dbLoadDatabase "$(MEASCOMP)/dbd/measCompApp.dbd"
measCompApp_registerRecordDeviceDriver pdbbase

epicsEnvSet("PREFIX",        "USB231:")
epicsEnvSet("PORT",          "USB231_1")
epicsEnvSet("UNIQUE_ID",     "123456")

## Configure port driver
# MultiFunctionConfig((portName,        # The name to give to this asyn port driver
#                      uniqueID,        # For USB the serial number.  For Ethernet the MAC address or IP address.
#                      maxInputPoints,  # Maximum number of input points for waveform digitizer
#                      maxOutputPoints) # Maximum number of output points for waveform generator
MultiFunctionConfig("$(PORT)", "$(UNIQUE_ID)", 1, 1)

#asynSetTraceMask($(PORT), -1, ERROR|FLOW|DRIVER)

dbLoadTemplate("$(MEASCOMP)/db/USB231.substitutions", "P=$(PREFIX),PORT=$(PORT)")

< ../save_restore.cmd

iocInit

create_monitor_set("auto_settings.req",30,"P=$(PREFIX)")

# Seem to need to process binary direction records to get them to work
dbpf $(PREFIX)Bd1.PROC 1
