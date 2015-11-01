< envPaths

epicsEnvSet("PREFIX",                   "USBCTR:MCS:")
epicsEnvSet("RNAME",                    "mca")
epicsEnvSet("MAX_COUNTERS",             "8")
epicsEnvSet("MAX_POINTS",               "2048")
epicsEnvSet("EPICS_CA_MAX_ARRAY_BYTES", "80000")
epicsEnvSet("PORT",                     "USBCTR")
# For MCA records FIELD=READ, for waveform records FIELD=PROC
epicsEnvSet("FIELD",                    "READ")
epicsEnvSet("MODEL",                    "USBCTR-08")

## Register all support components
dbLoadDatabase "../../dbd/measCompApp.dbd"
measCompApp_registerRecordDeviceDriver pdbbase

## Set the minimum sleep time to 1 ms
asynSetMinTimerPeriod(0.001)

## Configure port driver
# USBCTRConfig(portName,       # The name to give to this asyn port driver
#              boardNum,       # The number of this board assigned by the Measurement Computing Instacal program 
#              maxTimePoints)  # Maximum number of time points for MCS
USBCTRConfig("$(PORT)", 0, 2048, .001)

#asynSetTraceMask $(PORT) 0 255

dbLoadTemplate("USBCTR.substitutions")

# This loads the scaler record and supporting records
dbLoadRecords("$(STD)/stdApp/Db/scaler.db", "P=USBCTR:, S=scaler1, DTYP=Asyn Scaler, OUT=@asyn(USBCTR), FREQ=10000000")

# This database provides the support for the MCS functions
dbLoadRecords("$(MEASCOMP)/measCompApp/Db/measCompMCS.template", "P=$(PREFIX), PORT=$(PORT)")

# Load either MCA or waveform records below
# The number of records loaded must be the same as MAX_COUNTERS defined above

# Load the MCA records
dbLoadRecords("$(MCA)/mcaApp/Db/simple_mca.db", "P=$(PREFIX), M=$(RNAME)1,  DTYP=asynMCA, INP=@asyn($(PORT) 0),  PREC=3, CHANS=$(MAX_POINTS)")
dbLoadRecords("$(MCA)/mcaApp/Db/simple_mca.db", "P=$(PREFIX), M=$(RNAME)2,  DTYP=asynMCA, INP=@asyn($(PORT) 1),  PREC=3, CHANS=$(MAX_POINTS)")
dbLoadRecords("$(MCA)/mcaApp/Db/simple_mca.db", "P=$(PREFIX), M=$(RNAME)3,  DTYP=asynMCA, INP=@asyn($(PORT) 2),  PREC=3, CHANS=$(MAX_POINTS)")
dbLoadRecords("$(MCA)/mcaApp/Db/simple_mca.db", "P=$(PREFIX), M=$(RNAME)4,  DTYP=asynMCA, INP=@asyn($(PORT) 3),  PREC=3, CHANS=$(MAX_POINTS)")
dbLoadRecords("$(MCA)/mcaApp/Db/simple_mca.db", "P=$(PREFIX), M=$(RNAME)5,  DTYP=asynMCA, INP=@asyn($(PORT) 4),  PREC=3, CHANS=$(MAX_POINTS)")
dbLoadRecords("$(MCA)/mcaApp/Db/simple_mca.db", "P=$(PREFIX), M=$(RNAME)6,  DTYP=asynMCA, INP=@asyn($(PORT) 5),  PREC=3, CHANS=$(MAX_POINTS)")
dbLoadRecords("$(MCA)/mcaApp/Db/simple_mca.db", "P=$(PREFIX), M=$(RNAME)7,  DTYP=asynMCA, INP=@asyn($(PORT) 6),  PREC=3, CHANS=$(MAX_POINTS)")
dbLoadRecords("$(MCA)/mcaApp/Db/simple_mca.db", "P=$(PREFIX), M=$(RNAME)8,  DTYP=asynMCA, INP=@asyn($(PORT) 7),  PREC=3, CHANS=$(MAX_POINTS)")

# This loads the waveform records
#dbLoadRecords("$(MCA)/mcaApp/Db/SIS38XX_waveform.template", "P=$(PREFIX), R=$(RNAME)1,  INP=@asyn($(PORT) 0),  CHANS=$(MAX_POINTS)")
#dbLoadRecords("$(MCA)/mcaApp/Db/SIS38XX_waveform.template", "P=$(PREFIX), R=$(RNAME)2,  INP=@asyn($(PORT) 1),  CHANS=$(MAX_POINTS)")
#dbLoadRecords("$(MCA)/mcaApp/Db/SIS38XX_waveform.template", "P=$(PREFIX), R=$(RNAME)3,  INP=@asyn($(PORT) 2),  CHANS=$(MAX_POINTS)")
#dbLoadRecords("$(MCA)/mcaApp/Db/SIS38XX_waveform.template", "P=$(PREFIX), R=$(RNAME)4,  INP=@asyn($(PORT) 3),  CHANS=$(MAX_POINTS)")
#dbLoadRecords("$(MCA)/mcaApp/Db/SIS38XX_waveform.template", "P=$(PREFIX), R=$(RNAME)5,  INP=@asyn($(PORT) 4),  CHANS=$(MAX_POINTS)")
#dbLoadRecords("$(MCA)/mcaApp/Db/SIS38XX_waveform.template", "P=$(PREFIX), R=$(RNAME)6,  INP=@asyn($(PORT) 5),  CHANS=$(MAX_POINTS)")
#dbLoadRecords("$(MCA)/mcaApp/Db/SIS38XX_waveform.template", "P=$(PREFIX), R=$(RNAME)7,  INP=@asyn($(PORT) 6),  CHANS=$(MAX_POINTS)")
#dbLoadRecords("$(MCA)/mcaApp/Db/SIS38XX_waveform.template", "P=$(PREFIX), R=$(RNAME)8,  INP=@asyn($(PORT) 7),  CHANS=$(MAX_POINTS)")

asynSetTraceIOMask($(PORT),0,2)
#asynSetTraceFile("$(PORT)",0,"$(MODEL).out")
#asynSetTraceMask("$(PORT)",0,0xff)

< save_restore.cmd
save_restoreSet_status_prefix($(PREFIX))
dbLoadRecords("$(AUTOSAVE)/asApp/Db/save_restoreStatus.db", "P=$(PREFIX)")

iocInit

seq(USBCTR_SNL, "P=$(PREFIX), R=$(RNAME), NUM_SIGNALS=$(MAX_COUNTERS), FIELD=$(FIELD)")

create_monitor_set("auto_settings.req",30)
