SHELL=/bin/bash

# xargs -r a GNU extension t
XARGS_CMDLINE = xargs -r

# override by env variable
export PRODUCT ?= linux_km
export TEST_WORKDIR ?= $(BUILDROOT)
export USE_VALGRIND ?= 0

# makefile included by all test makefiles
include ../make/test_env.mk
sinclude ../make/$(PRODUCT).makedefs

# derived
override TESTROOT=$(BUILDROOT)/$(BUILDTYPE)/$(PRODUCT)
override LOGDIR=$(TEST_WORKDIR)/bvd
override TESTDIR=$(TESTROOT)/tests/bvd

# Defined for this test
TID=1
CID=1
UID=1

.PHONY: default setup cleanup runtests download_test_data

default:  
ifeq ($(strip $(wildcard $(WORKAREA)/sw_x/tests/bvd/Makefile)),)
	$(error "can't locate sw_x/tests/bvd/Makefile - current directory is $(shell pwd)")
endif
ifneq ($(CONFIG_ES),km)
	$(error "can't run bvdTest unless using a KM build.")
endif
	$(MAKE) setup
	$(MAKE) runtests
	$(MAKE) cleanup

setup:  download_test_data $(TESTDIR)/bvd_test_data/timestamp
	mkdir -p $(LOGDIR)
	mkdir -p $(TESTDIR) 
	$(MAKE) install_escmod
	$(MAKE) install_bvdmod

download_test_data:
	wget --progress=dot:mega --timestamping --directory-prefix=$(TESTDIR) http://$(TESTDATA_HOST):$(HTTP_PORT)/$(TESTDATA_PATH)/android/bvd_test_data.tar

$(TESTDIR)/bvd_test_data/timestamp:  $(TESTDIR)/bvd_test_data.tar 
	cd $(TESTDIR) && \
	rm -fr bvd_test_data && \
	tar xf bvd_test_data.tar && \
	touch bvd_test_data/timestamp

cleanup: 
	$(MAKE) remove_bvdmod
	$(MAKE) remove_escmod

runtests: 
	-$(TESTROOT)/gvm_core/os/bvd/tests/bvdtest/bvdTest \
	--tid $(TID) --cid $(CID) --uid $(UID) \
	--titleticket $(TESTDIR)/bvd_test_data/$(TID).tik \
	--tmd $(TESTDIR)/bvd_test_data/$(TID).tmd \
	--saveencticket $(TESTDIR)/bvd_test_data/saveenc.tik \
	--savesignticket  $(TESTDIR)/bvd_test_data/savesign.tik \
	--cacert $(TESTDIR)/bvd_test_data/ca_dpki.cert \
	--xscert $(TESTDIR)/bvd_test_data/xs_dpki.cert \
	--bvdbase /dev/bvd \
	--vscache $(TESTDIR)/bvd_test_data/ \
	&> $(LOGDIR)/bvd.log
	-@if [[ -n `tail -1 $(LOGDIR)/bvd.log | grep "PASS"` ]]; then \
	  echo "TC_RESULT = PASS ;;; TC_NAME = bvdTest"; \
	elif [[ -n `tail -1 $(LOGDIR)/bvd.log | grep "FAIL"` ]]; then \
	  echo "TC_RESULT = FAIL ;;; TC_NAME = bvdTest"; \
	fi

sinclude ../make/$(PRODUCT).makerules
