< envPaths

## Register all support components
dbLoadDatabase "../../dbd/measCompApp.dbd"
measCompApp_registerRecordDeviceDriver pdbbase

# Configure port driver
# MultiFunctionConfig(portName,        # The name to give to this asyn port driver
#                     boardNum,        # The number of this board assigned by the Measurement Computing Instacal program 
#                     maxInputPoints,  # Maximum number of input points for waveform digitizer
#                     maxOutputPoints) # Maximum number of output points for waveform generator
MultiFunctionConfig("USB231_1", 0, 1048576, 1048576)

dbLoadTemplate("USB231.substitutions")

#asynSetTraceMask USB231_1 -1 255

< save_restore.cmd

iocInit

create_monitor_set("auto_settings.req",30)

# Seem to need to process binary direction records to get them to work
dbpf USB231:Bd1.PROC 1
