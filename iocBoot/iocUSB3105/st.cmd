< envPaths

## Register all support components
dbLoadDatabase "$(MEASCOMP)/dbd/measCompApp.dbd"
measCompApp_registerRecordDeviceDriver pdbbase

epicsEnvSet("PREFIX",        "USB3105:")
epicsEnvSet("PORT",          "USB3105_1")
epicsEnvSet("UNIQUE_ID",     "1FE4A60")

## Configure port driver
# MultiFunctionConfig((portName,        # The name to give to this asyn port driver
#                      uniqueID,        # For USB the serial number.  For Ethernet the MAC address or IP address.
#                      maxInputPoints,  # Maximum number of input points for waveform digitizer
#                      maxOutputPoints) # Maximum number of output points for waveform generator
MultiFunctionConfig("$(PORT)", "$(UNIQUE_ID)", 0, 0)

#asynSetTraceMask($(PORT), -1, ERROR|FLOW|DRIVER)

dbLoadTemplate("$(MEASCOMP)/db/USB3105.substitutions", "P=$(PREFIX),PORT=$(PORT)")

< ../save_restore.cmd

iocInit

create_monitor_set("auto_settings.req",30,"P=$(PREFIX)")
