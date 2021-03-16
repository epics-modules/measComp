< envPaths

## Register all support components
dbLoadDatabase "../../dbd/measCompApp.dbd"
measCompApp_registerRecordDeviceDriver pdbbase

epicsEnvSet("PREFIX",        "TC32:")
epicsEnvSet("PORT",          "TC32_1")
epicsEnvSet("UNIQUE_ID",     "10.54.160.20")

# Configure port driver
## Configure port driver
# MultiFunctionConfig((portName,        # The name to give to this asyn port driver
#                      uniqueID,        # For USB the serial number.  For Ethernet the MAC address or IP address.
#                      maxInputPoints,  # Maximum number of input points for waveform digitizer
#                      maxOutputPoints) # Maximum number of output points for waveform generator
MultiFunctionConfig("$(PORT)", "$(UNIQUE_ID)", 1, 1)

dbLoadTemplate("$(MEASCOMP)/db/TC32.substitutions", "P=$(PREFIX),PORT=$(PORT)")

asynSetTraceIOMask TC32_1 -1 2
#asynSetTraceMask TC32_1 -1 255

< ../save_restore.cmd

iocInit

create_monitor_set("auto_settings.req",30,"P=$(PREFIX)")
