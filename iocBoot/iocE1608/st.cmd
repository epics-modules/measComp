< envPaths

## Register all support components
dbLoadDatabase "../../dbd/measCompApp.dbd"
measCompApp_registerRecordDeviceDriver pdbbase

epicsEnvSet("PREFIX",        "USB2408:")
epicsEnvSet("PORT",          "USB2401_1")
epicsEnvSet("WDIG_POINTS",   "2048")
epicsEnvSet("UNIQUE_ID",     "00:80:2F:24:53:DE")

## Configure port driver
# MultiFunctionConfig((portName,        # The name to give to this asyn port driver
#                      uniqueID,        # For USB the serial number.  For Ethernet the MAC address or IP address.
#                      maxInputPoints,  # Maximum number of input points for waveform digitizer
#                      maxOutputPoints) # Maximum number of output points for waveform generator
# This is an E-1608 using its IP address. This will work even if the device is on a different subnet from the IOC.
#MultiFunctionConfig("E1608_1", "10.54.160.216", $(INPUT_POINTS), $(OUTPUT_POINTS))
# This is an E-1608 using its MAC address.  This will only work if the device is on the same subnet as the IOC.
MultiFunctionConfig("$(PORT)", "$(UNIQUE_ID)", $(WDIG_POINTS), 1)

#asynSetTraceMask E1608_1 -1 255

dbLoadTemplate("$(MEASCOMP)/db/E1608.substitutions", "P=$(PREFIX),PORT=$(PORT),WDIG_POINTS=$(WDIG_POINTS)")

< ../save_restore.cmd

iocInit

create_monitor_set("auto_settings.req",30,"P=$(PREFIX)")

# Need to force the time arrays to process because the records are scan=I/O Intr
# but asynPortDriver does not do array callbacks before iocInit.

dbpf $(PREFIX)WaveDigDwell.PROC 1
