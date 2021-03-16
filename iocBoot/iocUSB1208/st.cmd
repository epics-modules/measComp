< envPaths

## Register all support components
dbLoadDatabase "../../dbd/measCompApp.dbd"
measCompApp_registerRecordDeviceDriver pdbbase

epicsEnvSet("PREFIX",        "USB1208:")
epicsEnvSet("PORT",          "USB1208_1")
epicsEnvSet("UNIQUE_ID",     "123456")

# Configure port driver
# MultiFunctionConfig((portName,        # The name to give to this asyn port driver
#                      uniqueID,        # For USB the serial number.  For Ethernet the MAC address or IP address.
#                      maxInputPoints,  # Maximum number of input points for waveform digitizer
#                      maxOutputPoints) # Maximum number of output points for waveform generator
MultiFunctionConfig("$(PORT)", "$(UNIQUE_ID)", 1, 1)

dbLoadTemplate("$(MEASCOMP)/db/USB1208.substitutions", "P=$(PREFIX),PORT=$(PORT)")

#asynSetTraceMask USB1208_1 -1 255

< ../save_restore.cmd

iocInit

create_monitor_set("auto_settings.req",30,"P=$(PREFIX)")

# Seem to need to process binary direction records to get them to work
dbpf $(PREFIX)Bd1.PROC 1
dbpf $(PREFIX)Bd2.PROC 1
