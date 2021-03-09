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
# This is an E-1608 using its IP address. This will work even if the device is on a different subnet from the IOC.
#MultiFunctionConfig("E1608_1", "10.54.160.216", $(INPUT_POINTS), $(OUTPUT_POINTS))
# This is an E-1608 using its MAC address.  This will only work if the device is on the same subnet as the IOC.
MultiFunctionConfig("E1608_1", "00:80:2F:24:53:DE", $(INPUT_POINTS), $(OUTPUT_POINTS))

#asynSetTraceMask E1608_1 -1 255

dbLoadTemplate("E1608.substitutions", P=E1608:)

< ../save_restore.cmd

iocInit

create_monitor_set("auto_settings.req",30)

# Need to force the time arrays to process because the records are scan=I/O Intr
# but asynPortDriver does not do array callbacks before iocInit.

dbpf E1608:WaveDigDwell.PROC 1
