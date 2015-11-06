< envPaths

## Register all support components
dbLoadDatabase "../../dbd/measCompApp.dbd"
measCompApp_registerRecordDeviceDriver pdbbase

# Configure port driver
# MultiFunctionConfig(portName,        # The name to give to this asyn port driver
#                     boardNum,        # The number of this board assigned by the Measurement Computing Instacal program 
#                     maxInputPoints,  # Maximum number of input points for waveform digitizer
#                     maxOutputPoints) # Maximum number of output points for waveform generator
MultiFunctionConfig("USB2408_1", 0, 1048576, 1048576)

dbLoadTemplate("USB2408.substitutions")

#asynSetTraceMask USB2408_1 -1 255

< save_restore.cmd

iocInit

create_monitor_set("auto_settings.req",30)

# Need to force the time arrays to process because the records are scan=I/O Intr
# but asynPortDriver does not do array callbacks before iocInit.

dbpf USB2408:WaveDigDwell.PROC 1
dbpf USB2408:WaveGenUserDwell.PROC 1
