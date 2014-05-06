TOP = ..
include $(TOP)/configure/CONFIG
DIRS := $(DIRS) src
DIRS := $(DIRS) demoSrc
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *db*))
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *Db*))
include $(TOP)/configure/RULES_DIRS

