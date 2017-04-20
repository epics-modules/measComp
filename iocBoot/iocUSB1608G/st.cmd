< envPaths

## Register all support components
dbLoadDatabase "../../dbd/measCompApp.dbd"
measCompApp_registerRecordDeviceDriver pdbbase

epicsEnvSet(INPUT_POINTS, "1048576")
epicsEnvSet(OUTPUT_POINTS, "0")

## Configure port driver
# USB1608GConfig(portName,        # The name to give to this asyn port driver
#                boardNum,        # The number of this board assigned by the Measurement Computing Instacal program 
#                maxInputPoints,  # Maximum number of input points for waveform digitizer
#                maxOutputPoints) # Maximum number of output points for waveform generator
MultiFunctionConfig("1608G_1", 0, $(INPUT_POINTS), $(OUTPUT_POINTS))

#asynSetTraceMask 1608G_1 -1 255

dbLoadTemplate("1608G.substitutions.small")
#dbLoadTemplate("1608G.substitutions.big")

< save_restore.cmd

iocInit

create_monitor_set("auto_settings.req",30)

# Need to force the time arrays to process because the records are scan=I/O Intr
# but asynPortDriver does not do array callbacks before iocInit.

dbpf 1608G:WaveDigDwell.PROC 1
dbpf 1608G:WaveGenUserDwell.PROC 1
