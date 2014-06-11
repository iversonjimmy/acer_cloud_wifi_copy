override MY_TEST_NAME=vs_psn

# VS_PSN TESTS NOTE
#
# This test can run on remote clodenode or local Linux
#
# To run on localhost, must be able to ssh to TARGET_USER@LOCAL_HOST without password
# i.e.;   cd ~/.ssh; cat id_rsa.pub >>authorized_keys
#
# Defaults to run on build@localhost, so be sure to specify TARGET_USER
#
# To avoid conflict with other users use:   CCD_TEST_ACCOUNT=<YOUR_CCD_TEST_ACCOUNT>
#
# - To run it locally:
#   make TARGET_USER=<local Linux username> CCD_TEST_ACCOUNT=<YOUR_CCD_TEST_ACCOUNT> PRODUCT=linux
# - To run it remotely:
#   make TARGET_USER=<remote cloudnode username> \
#        TARGET_MACHINE=<remote machine name or ip> \
#        CCD_TEST_ACCOUNT=<YOUR_CCD_TEST_ACCOUNT> \
#        PRODUCT=cloudnode
#

# makefile included by all test makefiles
include ../make/test_env.mk
sinclude ../make/$(PRODUCT).makedefs

export CLOUDPC_NAME ?= localhost
export TARGET_MACHINE = $(CLOUDPC_NAME)
export TARGET_USER ?= build

export CCD_TEST_ACCOUNT ?= defaultVsPsnTester@igware.com
export IDT_TOOLS = $(SRC_SWI)/tools/idt

ifdef FULL_TEST_RUN
    MORE_TEST_ARGS = -A -M
    MORE_DB_ERR_HAND_ARGS = -L
else
    NO_DB_ERR_HAND ?= 1
endif 

# derived

PRODUCT_SUPPORTED=false

ifeq ($(PRODUCT), cloudnode)
PRODUCT_SUPPORTED=true
override TESTPLAT=linux
override TARGET_HOME=/home/$(TARGET_USER)
override SET_HOME=export HOME=$(TARGET_HOME)
override TARGET_TESTROOT=$(TARGET_HOME)/temp/igware/testroot
override CCD_APPDATA_PATH=$(TARGET_HOME)/temp/SyncAgent
override KILL=killall
override DXSHELL_PACKAGE=dxshell_cloudnode.tar.gz
override DXSHELL_PACKAGE_STORE_PATH=$(STORE_PATH)/$(TEST_BRANCH)/cloudnode/dxshell_cloudnode
override DB_PACKAGE_STORE_PATH=pc/test_data/orbe_test_data
override DB_PACKAGE_1=test_data.20131106.tar
override DB_PACKAGE_2=test_data.20131108.tar
override DB_PACKAGE_3=test_data.20131118.tar
override TEST_PACKAGE=tests_cloudnode.tar.gz
override TEST_PACKAGE_STORE_PATH=$(STORE_PATH)/$(TEST_BRANCH)/cloudnode/tests_cloudnode
override UNZIP=tar xzvf
#  To use locally built release version of cloudnode ccd from BUILDROOT
#     don't define USE_ARCHIVED_BUILD
#  To use locally built debug version of cloudnode ccd from BUILDROOT
#     don't define USE_ARCHIVED_BUILD and define:
#        CCD_BUILDROOT_BINARY_BUILDTYPE=debug
#  To use ccd, dxshell, and ccd.conf.tmpl from latest archive tar file; define:
#     USE_ARCHIVED_BUILD=1
#  To use debug version of cloudnode ccd from archive tar file define both:
#     USE_ARCHIVED_BUILD=1
#     CCD_ARCHIVE_BINARY_NAME_EXTENSION=.debug_build
CCD_BUILDROOT_BINARY_BUILDTYPE ?= $(BUILDTYPE)
override CCD_BUILDROOT_BINARY=$(BUILDROOT)/$(CCD_BUILDROOT_BINARY_BUILDTYPE)/cloudnode/gvm_core/daemons/ccd/ccd_cloudnode
override CCD_ARCHIVE_BINARY_NAME=ccd_cloudnode$(CCD_ARCHIVE_BINARY_NAME_EXTENSION)
override DXSHELL_BUILDROOT_BINARY=$(BUILDROOT)/$(BUILDTYPE)/cloudnode/tests/dxshell/dxshell_cloudnode
override DXSHELL_ARCHIVE_BINARY_NAME=dxshell_cloudnode
override TEST_VS_PSN=$(BUILDTYPE)/$(PRODUCT)/gvm_core/internal/vssts/tests/vs_psn/vs_psn
override TEST_VS_PSN_DB_ERR_HAND=$(BUILDTYPE)/$(PRODUCT)/gvm_core/internal/vssts/tests/vs_psn/vsTest_dbeh.sh
override BUILDROOT_TEST_FILES=$(TEST_VS_PSN) $(TEST_VS_PSN_DB_ERR_HAND)
override WGET_OPTIONS=
override REMOTE_RUN_PRIVILEGED=/usr/bin/ssh root@$(TARGET_MACHINE)
override AS_TARGET_USER=su $(TARGET_USER) -c
endif

ifeq ($(PRODUCT), linux)
PRODUCT_SUPPORTED=true
override TESTPLAT=linux
override TARGET_HOME=$(HOME)
override SET_HOME=:
override TARGET_TESTROOT=$(TARGET_HOME)/temp/igware/testroot
override CCD_APPDATA_PATH=$(TARGET_HOME)/temp/SyncAgent
override KILL=killall
override DXSHELL_PACKAGE=dxshell.tar.gz
override DXSHELL_PACKAGE_STORE_PATH=$(STORE_PATH)/$(TEST_BRANCH)/linux/dxshell
override TEST_PACKAGE=
override UNZIP=tar xzvf
override CCD_BUILDROOT_BINARY=$(BUILDROOT)/$(BUILDTYPE)/linux/gvm_core/daemons/ccd/ccd_cloudnode
override CCD_ARCHIVE_BINARY_NAME=ccd_cloudnode
override DXSHELL_BUILDROOT_BINARY=$(BUILDROOT)/$(BUILDTYPE)/linux/tests/dxshell/dxshell_cloudnode
override DXSHELL_ARCHIVE_BINARY_NAME=dxshell_cloudnode
override TEST_VS_PSN=$(BUILDTYPE)/$(PRODUCT)/gvm_core/internal/vssts/tests/vs_psn/vs_psn
override TEST_VS_PSN_DB_ERR_HAND=$(BUILDTYPE)/$(PRODUCT)/gvm_core/internal/vssts/tests/vs_psn/vsTest_dbeh.sh
override BUILDROOT_TEST_FILES=$(TEST_VS_PSN) $(TEST_VS_PSN_DB_ERR_HAND)
override WGET_OPTIONS=--progress=dot:mega
override REMOTE_RUN_PRIVILEGED=$(REMOTE_RUN)
override AS_TARGET_USER=sh -c
ifdef USE_ARCHIVED_BUILD
    $(error vs_psn with product linux does not support USE_ARCHIVED_BUILD)
endif
endif

ifeq ($(PRODUCT_SUPPORTED),false)
$(error Only cloudnode, and linux are currently supported for vs_psn tests, current PRODUCT is $(PRODUCT))
endif

REMOTE_RUN=/usr/bin/ssh $(TARGET_USER)@$(TARGET_MACHINE)



.PHONY: default setup cleanup runtests stopall verify_dir
default: runtests cleanup

stopall:
	-$(REMOTE_RUN) '$(KILL) vs_psn'
	-$(REMOTE_RUN) '$(KILL) ccd'
	-$(REMOTE_RUN) '$(KILL) dxshell'
	-$(REMOTE_RUN) '$(KILL) dx_remote_agent'

# completely remove ccd state
clean: stopall
	@if [ -z "$(TARGET_TESTROOT)" ]; then echo "Bad variable"; exit 2; fi
	-$(REMOTE_RUN) 'rm -fr $(TARGET_TESTROOT)/*'
	@if [ -z "$(CCD_APPDATA_PATH)" ]; then echo "Bad variable"; exit 2; fi
	-$(REMOTE_RUN) 'rm -fr $(CCD_APPDATA_PATH)/*'


verify_dir:
ifeq ($(strip $(wildcard $(SRC_SWX)/tests/vs_psn/Makefile)),)
	$(error "can't locate sw_x/tests/vs_psn/Makefile - current directory is $(shell pwd)")
endif


setup: verify_dir cleanup
	@if [ -d $(IDT_TOOLS) ]; then \
		echo 'IDT tool detected'; \
	else \
		echo 'IDT tool does not exist, check out the $(IDT_TOOLS)'; \
		false; \
	fi 
ifdef DEVELOP_BUILD_DEPS
	$(MAKE) -C $(WORKAREA)/sw_x \
		gvm_core/internal/vssts/tests/vs_psn/vs_psn \
	# end of list
endif
	-mkdir -p $(LOGDIR)
	cd $(IDT_TOOLS) && \
		./createUser -i ids.$(INTERNAL_LAB_DOMAIN_NAME) -u $(CCD_TEST_ACCOUNT) -p password
	# TARGET_TESTROOT is the test root path of the test target
	# By default, the TARGET_MACHINE is localhost (i.e. on the same machine as test builder)
	@if [ "$(TARGET_MACHINE)" == "" ]; then \
	  echo '*** Must define "TARGET_MACHINE"'; \
	  false; \
	fi
	@if [ "$(TARGET_USER)" == "" ]; then \
	  echo '*** Must define "TARGET_USER"'; \
	  false; \
	fi
	-$(REMOTE_RUN) 'mkdir -p $(TARGET_TESTROOT)'
ifdef USE_ARCHIVED_BUILD
	$(REMOTE_RUN) 'wget $(WGET_OPTIONS) http://$(STORE_HOST):$(HTTP_PORT)/$(DXSHELL_PACKAGE_STORE_PATH)/$(DXSHELL_PACKAGE) -P $(TARGET_TESTROOT)'
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); $(UNZIP) $(DXSHELL_PACKAGE)'
else
	echo "sync local ccd and dxshell files to $(TARGET_TESTROOT)"
	scp -p $(CCD_BUILDROOT_BINARY) $(TARGET_USER)@$(TARGET_MACHINE):$(TARGET_TESTROOT)/$(CCD_ARCHIVE_BINARY_NAME)
	scp -p $(DXSHELL_BUILDROOT_BINARY) $(TARGET_USER)@$(TARGET_MACHINE):$(TARGET_TESTROOT)/$(DXSHELL_ARCHIVE_BINARY_NAME)
	rsync -p $(SRC_SWX)/gvm_core/conf/ccd.conf.tmpl $(TARGET_USER)@$(TARGET_MACHINE):$(TARGET_TESTROOT)
endif
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); mv $(CCD_ARCHIVE_BINARY_NAME) ccd'
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); mv $(DXSHELL_ARCHIVE_BINARY_NAME) dxshell'
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); echo -e "\n// Max total size of all log files\nmaxTotalLogSize = 1073741824\n" >> ccd.conf.tmpl'
	if [ "$(TEST_PACKAGE)" == "" ] || [ "$(USE_ARCHIVED_BUILD)" == "" ] || [ "$(USE_BUILDROOT_TESTS)" != "" ]; then \
	  cd $(BUILDROOT); rsync -pR  $(BUILDROOT_TEST_FILES) $(TARGET_USER)@$(TARGET_MACHINE):$(TARGET_TESTROOT); \
	else \
	  $(REMOTE_RUN) 'wget $(WGET_OPTIONS) http://$(STORE_HOST):$(HTTP_PORT)/$(TEST_PACKAGE_STORE_PATH)/$(TEST_PACKAGE) -P $(TARGET_TESTROOT)'; \
	  $(REMOTE_RUN) 'cd $(TARGET_TESTROOT); $(UNZIP) $(TEST_PACKAGE)'; \
	fi
ifeq ($(PRODUCT), cloudnode)
	$(REMOTE_RUN) 'wget $(WGET_OPTIONS) http://$(STORE_HOST):$(HTTP_PORT)/$(DB_PACKAGE_STORE_PATH)/$(DB_PACKAGE_1) -P $(TARGET_TESTROOT)'
	$(REMOTE_RUN) 'wget $(WGET_OPTIONS) http://$(STORE_HOST):$(HTTP_PORT)/$(DB_PACKAGE_STORE_PATH)/$(DB_PACKAGE_2) -P $(TARGET_TESTROOT)'
	$(REMOTE_RUN) 'wget $(WGET_OPTIONS) http://$(STORE_HOST):$(HTTP_PORT)/$(DB_PACKAGE_STORE_PATH)/$(DB_PACKAGE_3) -P $(TARGET_TESTROOT)'
endif

runtests: setup
	@if [ -z "$(LOGDIR)" ]; then echo "Bad variable"; exit 2; fi
	rm -fr $(LOGDIR)/*
	mkdir -p $(LOGDIR)/ccd/special_logs
	$(REMOTE_RUN_PRIVILEGED) ' \
	  { \
	    ulimit -c unlimited && cd $(TARGET_TESTROOT) && $(AS_TARGET_USER) " \
	      ./dxshell SetDomain $(LAB_DOMAIN_NAME) && \
	      ./dxshell StartCCD && ./dxshell StartCloudPC $(CCD_TEST_ACCOUNT) password && sleep 10 && \
	      if [ -z \"$(ONLY_DB_ERR_HAND)\" ]; then \
	        $(VALGRIND) $(TEST_VS_PSN) -u $(CCD_TEST_ACCOUNT) -p password \
	          --vsds-name www-c100.$(LAB_DOMAIN_NAME) --ias-name www-c100.$(LAB_DOMAIN_NAME) $(MORE_TEST_ARGS); fi && \
	      if [ -z \"$(NO_DB_ERR_HAND)\" ]; then \
	        $(TEST_VS_PSN_DB_ERR_HAND) $(MORE_DB_ERR_HAND_ARGS) $(CCD_APPDATA_PATH) $(TEST_VS_PSN) -u $(CCD_TEST_ACCOUNT) -p password \
	          --vsds-name www-c100.$(LAB_DOMAIN_NAME) --ias-name www-c100.$(LAB_DOMAIN_NAME) $(MORE_TEST_ARGS); fi && \
	      ./dxshell StopCloudPC && ./dxshell StopCCD && echo \"ENTIRE TEST FINISHED\""; \
	  }' \
	2>&1 | tee $(LOGDIR)/vs_psn.log | grep $(GREP_TEST_OUTPUT_REGEXP); \
	echo "PIPESTATUS[0] is $${PIPESTATUS[0]} PIPESTATUS[1] is $${PIPESTATUS[1]}";
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); ./imltool $(CCD_APPDATA_PATH)'
	rsync $(TARGET_USER)@$(TARGET_MACHINE):$(CCD_APPDATA_PATH)/logs/imltool/*.log $(LOGDIR)/ccd
	rsync $(TARGET_USER)@$(TARGET_MACHINE):$(CCD_APPDATA_PATH)/logs/ccd/*.log $(LOGDIR)/ccd
	rsync $(TARGET_USER)@$(TARGET_MACHINE):$(CCD_APPDATA_PATH)/logs/ccd/special_logs/*.log $(LOGDIR)/ccd/special_logs
	@if tail -n 1 $(LOGDIR)/vs_psn.log | grep -q '^ENTIRE TEST FINISHED'; then \
	  echo "TC_RESULT = PASS ;;; TC_NAME = $(MY_TEST_NAME)_complete"; \
	else \
	  echo "*** $(MY_TEST_NAME) didn't complete!"; \
	  echo "TC_RESULT = FAIL ;;; TC_NAME = $(MY_TEST_NAME)_complete"; \
	  $(REMOTE_RUN_PRIVILEGED) ' \
	  { \
	    ulimit -c unlimited && cd $(TARGET_TESTROOT) && $(AS_TARGET_USER) " \
	      ./dxshell StopCCD"; \
	  }'; \
	fi


cleanup: 
	$(MAKE) clean
	-cd $(IDT_TOOLS) && \
		./deleteUser -i ids.$(INTERNAL_LAB_DOMAIN_NAME) -u $(CCD_TEST_ACCOUNT)

sinclude ../make/$(PRODUCT).makerules

