< envPaths

## Register all support components
dbLoadDatabase "../../dbd/measCompApp.dbd"
measCompApp_registerRecordDeviceDriver pdbbase

# Configure port driver
# MultiFunctionConfig(portName,        # The name to give to this asyn port driver
#                     boardNum,        # The number of this board assigned by the Measurement Computing Instacal program 
#                     maxInputPoints,  # Maximum number of input points for waveform digitizer
#                     maxOutputPoints) # Maximum number of output points for waveform generator
#MultiFunctionConfig("TC32_1", 0, 1048576, 1048576)
MultiFunctionConfig("TC32_1", 0, 0, 0)

dbLoadTemplate("TC32.substitutions")

asynSetTraceIOMask TC32_1 -1 2
#asynSetTraceMask TC32_1 -1 255

< save_restore.cmd

iocInit

create_monitor_set("auto_settings.req",30,"P=TC32:")

# Need to force the time arrays to process because the records are scan=I/O Intr
# but asynPortDriver does not do array callbacks before iocInit.

#dbpf USB2408:WaveDigDwell.PROC 1
