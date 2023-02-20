< envPaths

## Register all support components
dbLoadDatabase "$(MEASCOMP)/dbd/measCompApp.dbd"
measCompApp_registerRecordDeviceDriver pdbbase

epicsEnvSet("PREFIX",        "E1608:")
epicsEnvSet("PORT",          "E1608_1")
epicsEnvSet("WDIG_POINTS",   "2048")
#epicsEnvSet("UNIQUE_ID",     "00:80:2F:24:53:E5")
#epicsEnvSet("UNIQUE_ID",     "gse-e1608-6")
#epicsEnvSet("UNIQUE_ID",     "gse-e1608-6:54211")
#epicsEnvSet("UNIQUE_ID",     "10.54.160.63")
epicsEnvSet("UNIQUE_ID",     "gse-e1608-4:54211")
#epicsEnvSet("UNIQUE_ID",     "01234567")

## Configure port driver
# MultiFunctionConfig((portName,        # The name to give to this asyn port driver
#                      uniqueID,        # For USB the serial number.  For Ethernet the MAC address or IP address.
#                      maxInputPoints,  # Maximum number of input points for waveform digitizer
#                      maxOutputPoints) # Maximum number of output points for waveform generator
MultiFunctionConfig("$(PORT)", "$(UNIQUE_ID)", $(WDIG_POINTS), 1)

#asynSetTraceMask($(PORT), -1, ERROR|FLOW|DRIVER)

dbLoadTemplate("$(MEASCOMP)/db/E1608.substitutions", "P=$(PREFIX),PORT=$(PORT),WDIG_POINTS=$(WDIG_POINTS)")

< ../save_restore.cmd

iocInit

create_monitor_set("auto_settings.req",30,"P=$(PREFIX)")

# Need to force the time arrays to process because the records are scan=I/O Intr
# but asynPortDriver does not do array callbacks before iocInit.

dbpf $(PREFIX)WaveDigDwell.PROC 1
