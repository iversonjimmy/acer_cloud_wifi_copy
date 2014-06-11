include $(ROOT)/usr/include/make/GVMdefs


BUILD_DATE ?= $(shell date +%Y%m%d-%H%M)
DATA_DIR ?= /home/tmp/vpl-data.$(BUILD_DATE)

SAMPLE_COUNT ?= 1000

PROG_SUBDIRS=\
	atomic \
	socket \
	thread \
	mutex \
	sem \
	condvar \
	threadprio \
	\

#end of list.

SUBDIRS = lib $(PROG_SUBDIRS) 


SCRIPTS = runTests.sh

#end of of list

TESTDIR ?= $(ROOT)/gvm/usr/local/tests/vpl/microbench

install:	$(SCRIPTS)
	-mkdir -p $(TESTDIR)
	$(INSTALL) -m 755 $(SCRIPTS) $(TESTDIR)


get-data:
	mkdir -p $(DATA_DIR) && $(foreach TEST, $(PROG_SUBDIRS), $(TEST)/vpl_$(TEST)_timing > $(DATA_DIR)/$(TEST)-ops.dat &&) true

include $(GVMRULES)
