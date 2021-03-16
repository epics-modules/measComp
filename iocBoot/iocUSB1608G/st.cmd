< envPaths

## Register all support components
dbLoadDatabase "../../dbd/measCompApp.dbd"
measCompApp_registerRecordDeviceDriver pdbbase

epicsEnvSet("PREFIX",        "USB1608G:")
epicsEnvSet("PORT",          "USB1608G_1")
epicsEnvSet("WDIG_POINTS",   "1048576")
epicsEnvSet("UNIQUE_ID",     "123456")

# MultiFunctionConfig((portName,        # The name to give to this asyn port driver
#                      uniqueID,        # For USB the serial number.  For Ethernet the MAC address or IP address.
#                      maxInputPoints,  # Maximum number of input points for waveform digitizer
#                      maxOutputPoints) # Maximum number of output points for waveform generator
MultiFunctionConfig("$(PORT)", "$(UNIQUE_ID)", $(WDIG_POINTS), 1)

#asynSetTraceMask 1608G_1 -1 255

dbLoadTemplate("$(MEASCOMP)/db/USB1608G.substitutions", "P=$(PREFIX),PORT=$(PORT),WDIG_POINTS=$(WDIG_POINTS)")

< ../save_restore.cmd

iocInit

create_monitor_set("auto_settings.req",30,"P=$(PREFIX)")

# Need to force the time arrays to process because the records are scan=I/O Intr
# but asynPortDriver does not do array callbacks before iocInit.

dbpf $(PREFIX)WaveDigDwell.PROC 1
dbpf $(PREFIX)WaveGenUserDwell.PROC 1
