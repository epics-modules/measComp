< envPaths

## Register all support components
dbLoadDatabase "../../dbd/measCompApp.dbd"
measCompApp_registerRecordDeviceDriver pdbbase

## Configure port driver
# USB4303Config(portName,        # The name to give to this asyn port driver
#               boardNum,        # The number of this board assigned by the Measurement Computing Instacal program 
#               numChips)        # Number of CTS9513 chips on this module
C9513Config("C9513_1", 0, 2)
dbLoadTemplate("C9513.substitutions")

< save_restore.cmd

iocInit

create_monitor_set("auto_settings.req",30)

