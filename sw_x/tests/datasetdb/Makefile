override MY_TEST_NAME=datasetdb

SHELL=/bin/bash

# override by env variable
export PRODUCT ?= linux
export BUILDTYPE ?= release
export TEST_WORKDIR ?= $(BUILDROOT)

# makefile included by all test makefiles
include ../make/test_env.mk
sinclude ../make/$(PRODUCT).makedefs

# -r is a GNU extension t
XARGS_CMDLINE = xargs -r

export TARGET_MACHINE ?= localhost
export TARGET_USER ?= $(USER)


# derived
override TESTROOT=$(BUILDROOT)/$(BUILDTYPE)/$(PRODUCT)
override LOGDIR=$(TEST_WORKDIR)/datasetdb
override TESTPLAT=linux

ifeq ($(PRODUCT), cloudnode)
export CLOUDPC_NAME ?= localhost
override TARGET_MACHINE=$(CLOUDPC_NAME)
override TESTPLAT=cloudnode
override TARGET_HOME=/home/$(TARGET_USER)
override TARGET_TESTROOT=$(TARGET_HOME)/temp/igware/testroot
override KILL=killall
override TEST_PACKAGE=tests_cloudnode.tar.gz
override TEST_PACKAGE_STORE_PATH=$(STORE_PATH)/$(TEST_BRANCH)/$(PRODUCT)/tests_cloudnode
override UNZIP=tar xzvf
override TEST_BINARY=TestDatasetDB
override TARGET_TEST_BINARY=$(BUILDTYPE)/$(PRODUCT)/storageNode/datasetdb2/tests/$(TEST_BINARY)
override BUILDROOT_TEST_FILES=$(TARGET_TEST_BINARY)
override WGET_OPTIONS=
override REMOTE_RUN_PRIVILEGED=/usr/bin/ssh root@$(TARGET_MACHINE)
override AS_TARGET_USER=su $(TARGET_USER) -c
endif

REMOTE_RUN=/usr/bin/ssh $(TARGET_USER)@$(TARGET_MACHINE)


.PHONY: default setup cleanup runtests 

default:  
	$(MAKE) setup
	$(MAKE) runtests
	$(MAKE) cleanup

setup:
	$(MAKE) setup_$(TESTPLAT)

runtests:
	$(MAKE) runtests_$(TESTPLAT)

cleanup: 


.PHONY: setup_linux runtests_linux

setup_linux:

runtests_linux: 
	$(TESTROOT)/storageNode/datasetdb2/tests/TestDatasetDB all


.PHONY: setup_cloudnode runtests_cloudnode

setup_cloudnode:
	# TARGET_TESTROOT is the test root path on the test target
	# TARGET_MACHINE is the IP address, hostname, or localhost (i.e. on the same machine as test builder)
	@if [ "$(TARGET_MACHINE)" == "" ]; then \
	  echo '*** Must define "TARGET_MACHINE"'; \
	  false; \
	fi
	@if [ "$(TARGET_USER)" == "" ]; then \
	  echo '*** Must define "TARGET_USER"'; \
	  false; \
	fi
	@if [ -z "$(TARGET_TESTROOT)" ]; then echo "*** Bad variable TARGET_TESTROOT"; exit 2; fi
	-$(REMOTE_RUN) 'mkdir -p $(TARGET_TESTROOT)'
	-$(REMOTE_RUN) 'rm -fr $(TARGET_TESTROOT)/*'
	if [ "$(TEST_PACKAGE)" == "" ] || [ "$(USE_ARCHIVED_BUILD)" == "" || [ "$(USE_BUILDROOT_TESTS)" != "" ]; then \
	  cd $(BUILDROOT); rsync -pR  $(BUILDROOT_TEST_FILES) $(TARGET_USER)@$(TARGET_MACHINE):$(TARGET_TESTROOT); \
	else \
	  $(REMOTE_RUN) 'wget $(WGET_OPTIONS) http://$(STORE_HOST):$(HTTP_PORT)/$(TEST_PACKAGE_STORE_PATH)/$(TEST_PACKAGE) -P $(TARGET_TESTROOT)'; \
	  $(REMOTE_RUN) 'cd $(TARGET_TESTROOT); $(UNZIP) $(TEST_PACKAGE)'; \
	fi


runtests_cloudnode:
	$(REMOTE_RUN_PRIVILEGED) ' \
	  { \
	    ulimit -c unlimited && cd $(TARGET_TESTROOT) && $(AS_TARGET_USER) " \
	        ./$(TARGET_TEST_BINARY) all"; \
	  }'



sinclude ../make/$(PRODUCT).makerules

