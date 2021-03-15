< envPaths

## Register all support components
dbLoadDatabase "../../dbd/measCompApp.dbd"
measCompApp_registerRecordDeviceDriver pdbbase

epicsEnvSet(INPUT_POINTS, "4096")
epicsEnvSet(OUTPUT_POINTS, "4096")

## Configure port driver
# MultiFunctionConfig((portName,        # The name to give to this asyn port driver
#                      uniqueID,        # For USB the serial number.  For Ethernet the MAC address or IP address.
#                      maxInputPoints,  # Maximum number of input points for waveform digitizer
#                      maxOutputPoints) # Maximum number of output points for waveform generator
MultiFunctionConfig("ETC_1", "10.54.160.218", $(INPUT_POINTS), $(OUTPUT_POINTS))

#asynSetTraceMask ETC_1 -1 255

dbLoadTemplate("$(MEASCOMP)/db/ETC.substitutions", P=ETC:)

< ../save_restore.cmd

iocInit

create_monitor_set("auto_settings.req",30)

