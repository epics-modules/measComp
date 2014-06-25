TOP = ..
include $(TOP)/configure/CONFIG
DIRS := $(DIRS) src
DIRS := $(DIRS) demoSrc
# Must build src first because it installs Measurement Computing .h and .lib files
demoSrc_DEPEND_DIRS += src
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *db*))
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *Db*))
include $(TOP)/configure/RULES_DIRS

