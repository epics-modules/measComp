< envPaths

## Register all support components
dbLoadDatabase "../../dbd/measCompApp.dbd"
measCompApp_registerRecordDeviceDriver pdbbase

# Configure port driver
# MultiFunctionConfig((portName,        # The name to give to this asyn port driver
#                      uniqueID,        # For USB the serial number.  For Ethernet the MAC address or IP address.
#                      maxInputPoints,  # Maximum number of input points for waveform digitizer
#                      maxOutputPoints) # Maximum number of output points for waveform generator
MultiFunctionConfig("USB1208_1", "123456", 1048576, 1048576)

dbLoadTemplate("$(MEASCOMP)/db/USB1208.substitutions", "P=USB1208:")

#asynSetTraceMask USB1208_1 -1 255

< ../save_restore.cmd

iocInit

create_monitor_set("auto_settings.req",30)

# Seem to need to process binary direction records to get them to work
dbpf USB1208:Bd1.PROC 1
dbpf USB1208:Bd2.PROC 1
