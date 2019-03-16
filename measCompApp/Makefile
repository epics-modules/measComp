TOP = ..
include $(TOP)/configure/CONFIG
DIRS := $(DIRS) Linux_Drivers
DIRS := $(DIRS) cbw_linuxSrc
cbw_linuxSrc_DEPEND_DIRS += Linux_Drivers
DIRS := $(DIRS) src
src_DEPEND_DIRS += cbw_linuxSrc
DIRS := $(DIRS) demoSrc
# Must build src first because it installs Measurement Computing .h and .lib files
demoSrc_DEPEND_DIRS += src
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *db*))
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *Db*))
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *op*))
include $(TOP)/configure/RULES_DIRS

