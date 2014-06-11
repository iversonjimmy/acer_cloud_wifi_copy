override MY_TEST_NAME=simple_http

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

# derived
override TESTROOT=$(BUILDROOT)/$(BUILDTYPE)/$(PRODUCT)
override LOGDIR=$(TEST_WORKDIR)/datasetdb

ifeq ($(PRODUCT), cloudnode)
ifndef CLOUDPC_NAME
    $(error CLOUDPC_NAME must be either the hostname or ip address of an orbe)
endif
override TARGET_MACHINE=$(CLOUDPC_NAME)
export TARGET_USER ?= build
override TESTPLAT=cloudnode
override TARGET_HOME=/home/$(TARGET_USER)
override TARGET_TESTROOT=$(TARGET_HOME)/temp/igware/testroot
override KILL=killall
override TEST_PACKAGE=tests_cloudnode.tar.gz
override TEST_PACKAGE_STORE_PATH=$(STORE_PATH)/$(TEST_BRANCH)/$(PRODUCT)/tests_cloudnode
override TEST_PARSE_QUERY_STRING=$(BUILDTYPE)/$(PRODUCT)/gvm_core/internal/simple_http/tests/test_parse_query_string
override BUILDROOT_TEST_FILES=$(TEST_PARSE_QUERY_STRING)
override REMOTE_RUN_PRIVILEGED=/usr/bin/ssh root@$(TARGET_MACHINE)
override AS_TARGET_USER=su $(TARGET_USER) -c
else
override TESTPLAT=linux
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
	$(MAKE) cleanup_$(TESTPLAT)


.PHONY: setup_linux runtests_linux cleanup_linux

setup_linux:

runtests_linux: 
	$(TESTROOT)/gvm_core/internal/simple_http/tests/test_parse_query_string

cleanup_linux: 


.PHONY: setup_cloudnode runtests_cloudnode cleanup_cloudnode

setup_cloudnode:
	-mkdir -p $(LOGDIR)
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
	if [ "$(TEST_PACKAGE)" == "" ] || [ "$(USE_ARCHIVED_BUILD)" == "" ] || [ "$(USE_BUILDROOT_TESTS)" != "" ]; then \
	  cd $(BUILDROOT); rsync -pR  $(BUILDROOT_TEST_FILES) $(TARGET_USER)@$(TARGET_MACHINE):$(TARGET_TESTROOT); \
	else \
	  $(REMOTE_RUN) 'wget http://$(STORE_HOST):$(HTTP_PORT)/$(TEST_PACKAGE_STORE_PATH)/$(TEST_PACKAGE) -P $(TARGET_TESTROOT)'; \
	  $(REMOTE_RUN) 'cd $(TARGET_TESTROOT); tar -xzvf $(TEST_PACKAGE)'; \
	fi


runtests_cloudnode:
	-$(REMOTE_RUN) '$(KILL) test_parse_query_string'
	@if [ -z "$(LOGDIR)" ]; then echo "Bad variable"; exit 2; fi
	rm -fr $(LOGDIR)/*
	$(REMOTE_RUN_PRIVILEGED) ' \
	  { \
	    ulimit -c unlimited && cd $(TARGET_TESTROOT) && $(AS_TARGET_USER) " \
	      $(TEST_PARSE_QUERY_STRING) && \
	      echo \"ENTIRE TEST FINISHED\""; \
	  }' \
	2>&1 | tee $(LOGDIR)/$(MY_TEST_NAME).log | grep $(GREP_TEST_OUTPUT_REGEXP); \
	echo "PIPESTATUS[0] is $${PIPESTATUS[0]} PIPESTATUS[1] is $${PIPESTATUS[1]}";
	@if tail -n 1 $(LOGDIR)/$(MY_TEST_NAME).log | grep -q '^ENTIRE TEST FINISHED'; then \
	  echo "TC_RESULT = PASS ;;; TC_NAME = $(MY_TEST_NAME)_complete"; \
	else \
	  echo "*** $(MY_TEST_NAME) didn't complete!"; \
	  echo "TC_RESULT = FAIL ;;; TC_NAME = $(MY_TEST_NAME)_complete"; \
	fi


cleanup_cloudnode: 
	-$(REMOTE_RUN) '$(KILL) test_parse_query_string'


sinclude ../make/$(PRODUCT).makerules

