SHELL=/bin/bash

# -r is a GNU extension t
XARGS_CMDLINE = xargs -r

# auto detect
override WORKAREA=$(shell cd ../../..; pwd)

# override by env variable
export BUILDROOT ?= $(WORKAREA)/buildroot
export PRODUCT ?= linux
export BUILDTYPE ?= release
ifeq ($(strip $(USE_VALGRIND)),)
override USE_VALGRIND := 0
endif
export USE_VALGRIND ?= 0

# makefile included by all test makefiles
include ../make/test_env.mk
sinclude ../make/$(PRODUCT).makedefs

# derived
override TESTROOT=$(BUILDROOT)/$(BUILDTYPE)/$(PRODUCT)
override CCD=$(TESTROOT)/gvm_core/daemons/ccd/ccd
override RUNSYNC=$(TESTROOT)/tests/syncWb/runSync/runSync
override PING=/bin/ping

ifdef TEST_WORKDIR
override SYNCWB_ROOT=$(TEST_WORKDIR)/syncFilenameTest
else
override SYNCWB_ROOT=$(TESTROOT)/syncFilenameTest
endif
override C1_ROOT=$(SYNCWB_ROOT)/c1Root
override C2_ROOT=$(SYNCWB_ROOT)/c2Root

ifdef TEST_WORKDIR
# Use the suite-specific directory specified by the test harness.
override LOGDIR=$(TEST_WORKDIR)
else
override LOGDIR=$(BUILDROOT)/test_log/syncFilenameTest
endif

ifeq ($(USE_VALGRIND), 0)
override VALGRIND=
else ifeq ($(USE_VALGRIND), 1)
override VALGRIND=valgrind --log-file=$(LOGDIR)/valgrind.log --suppressions=valgrind.supp --gen-suppressions=all --leak-check=full
else
override VALGRIND=valgrind --tool=$(USE_VALGRIND) --log-file=$(LOGDIR)/valgrind.log --suppressions=valgrind.supp --gen-suppressions=all
endif

.PHONY: default setup cleanup runtests  

default:  
ifeq ($(strip $(wildcard $(WORKAREA)/sw_x/tests/syncFilenameTest/Makefile)),)
	$(error "can't locate sw_x/syncFilenameTest/Makefile - current directory is $(shell pwd)")
endif
	$(MAKE) setup
	-$(MAKE) runtests
	$(MAKE) cleanup

setup:
	-sudo umount $(C1_ROOT)/dev $(C1_ROOT)/proc $(C1_ROOT)/etc $(C1_ROOT)/var 
	-sudo umount $(C2_ROOT)/dev $(C2_ROOT)/proc $(C2_ROOT)/etc $(C2_ROOT)/var
	-rm -fr $(SYNCWB_ROOT)
	mkdir -p $(C1_ROOT)/dev $(C1_ROOT)/proc $(C1_ROOT)/etc $(C1_ROOT)/var
	mkdir -p $(C2_ROOT)/dev $(C2_ROOT)/proc $(C2_ROOT)/etc $(C2_ROOT)/var
	mkdir -p $(C1_ROOT)/lib $(C1_ROOT)/cache $(C1_ROOT)/bin $(C1_ROOT)/tmp/igware
	mkdir -p $(C2_ROOT)/lib $(C2_ROOT)/cache $(C2_ROOT)/bin $(C2_ROOT)/tmp/igware
	mkdir -p $(LOGDIR)
	cp $(CCD) $(C1_ROOT)/bin
	cp $(CCD) $(C2_ROOT)/bin
	cp $(RUNSYNC) $(C1_ROOT)/bin
	cp $(RUNSYNC) $(C2_ROOT)/bin
	cp /lib/libnss* $(C1_ROOT)/lib		# For some reason, gethostbyname requires this to resolve host names but ldd does not show it
	cp /lib/libnss* $(C2_ROOT)/lib		# For some reason, gethostbyname requires this to resolve host names but ldd does not show it
	./prepChroot.sh $(C1_ROOT) /usr/bin/host
	./prepChroot.sh $(C2_ROOT) /usr/bin/host
	./prepChroot.sh $(C1_ROOT) $(CCD)
	./prepChroot.sh $(C1_ROOT) $(RUNSYNC)
	./prepChroot.sh $(C2_ROOT) $(CCD)
	./prepChroot.sh $(C2_ROOT) $(RUNSYNC)
	sudo mount --bind /dev $(C1_ROOT)/dev
	sudo mount --bind /proc $(C1_ROOT)/proc
	sudo mount --bind /dev $(C2_ROOT)/dev
	sudo mount --bind /proc $(C2_ROOT)/proc
	sudo mount -o bind /etc $(C1_ROOT)/etc
	sudo mount -o bind /etc $(C2_ROOT)/etc
	sudo mount --bind /var $(C1_ROOT)/var
	sudo mount --bind /var $(C2_ROOT)/var
	sudo chmod 777 $(C1_ROOT)/cache
	sudo chmod 777 $(C2_ROOT)/cache

runtests: 
	-./syncFilenameTest.py --testRoot $(SYNCWB_ROOT) --c1Root $(C1_ROOT) --c2Root $(C2_ROOT) --testLog $(LOGDIR) 

cleanup: 
	-killall $(CCD) &> /dev/null
	-killall $(RUNSYNC) &> /dev/null
	-sudo umount $(C1_ROOT)/dev $(C1_ROOT)/proc $(C1_ROOT)/etc $(C1_ROOT)/var 
	-sudo umount $(C2_ROOT)/dev $(C2_ROOT)/proc $(C2_ROOT)/etc $(C2_ROOT)/var
	-rm -fr $(C1_ROOT)/* $(C2_ROOT)/*

sinclude ../make/$(PRODUCT).makerules
