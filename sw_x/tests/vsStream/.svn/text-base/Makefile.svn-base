override MY_TEST_NAME=vsStream

# makefile included by all test makefiles
include ../make/test_env.mk
sinclude ../make/$(PRODUCT).makedefs


export TARGET_MACHINE ?= localhost
export TARGET_USER ?= $(USER)

# derived
override CCD=$(TESTROOT)/gvm_core/daemons/ccd/ccd
override WAIT_FOR_CCD=$(TESTROOT)/gvm_core/daemons/ccd/tests/ccd_ready_test
override SHUTDOWN_CCD=$(TESTROOT)/gvm_core/daemons/ccd/tests/ccd_shutdown_test
override TESTPLAT=linux

export IDT_TOOLS = $(SRC_SWI)/tools/idt

# Make sure sw_i can be located.
ERROR_CHECK := $(shell test -d "$(SRC_SWI)" && echo OK)
ifneq ($(ERROR_CHECK),OK)
$(error The sw_i source tree could not be found at $(SRC_SWI).\
 This test requires sw_i to be checked-out.\
 You must override SRC_SWI if your sw_i and sw_x trees are not siblings)
endif

# PSN related
PSN_DATA_DIR = $(LOGDIR)/psndata

CCD_TEST_ACCOUNT ?= defaultVsStreamTester@igware.com

ifeq ($(PRODUCT), cloudnode)
export CLOUDPC_NAME ?= localhost
override TARGET_MACHINE=$(CLOUDPC_NAME)
override TESTPLAT=cloudnode
override TARGET_HOME=/home/$(TARGET_USER)
override TARGET_TESTROOT=$(TARGET_HOME)/temp/igware/testroot
override CCD_APPDATA_PATH=$(TARGET_HOME)/temp/SyncAgent
override KILL=killall
override DXSHELL_PACKAGE=dxshell_cloudnode.tar.gz
override DXSHELL_PACKAGE_STORE_PATH=$(STORE_PATH)/$(TEST_BRANCH)/$(PRODUCT)/dxshell_cloudnode
override TEST_PACKAGE=tests_cloudnode.tar.gz
override TEST_PACKAGE_STORE_PATH=$(STORE_PATH)/$(TEST_BRANCH)/$(PRODUCT)/tests_cloudnode
override UNZIP=tar xzvf
override CCD_BINARY_PATH=$(BUILDROOT)/$(BUILDTYPE)/$(PRODUCT)/gvm_core/daemons/ccd
override DXSHELL_BINARY_PATH=$(BUILDROOT)/$(BUILDTYPE)/$(PRODUCT)/tests/dxshell
override TEST_BINARY=vsStreamTest
override TARGET_TEST_BINARY=$(BUILDTYPE)/$(PRODUCT)/gvm_core/internal/vssi/tests/stream/$(TEST_BINARY)
override BUILDROOT_TEST_FILES=$(TARGET_TEST_BINARY)
override WGET_OPTIONS=
override REMOTE_RUN_PRIVILEGED=/usr/bin/ssh root@$(TARGET_MACHINE)
override AS_TARGET_USER=su $(TARGET_USER) -c
endif

REMOTE_RUN=/usr/bin/ssh $(TARGET_USER)@$(TARGET_MACHINE)


.PHONY: default setup cleanup runtests 

default:  
ifeq ($(strip $(wildcard $(WORKAREA)/sw_x/tests/vsStream/Makefile)),)
	$(error "can't locate sw_x/tests/vsStream/Makefile - current directory is $(shell pwd)")
endif
	$(MAKE) setup
	$(MAKE) runtests
	$(MAKE) cleanup

setup: cleanup
	$(MAKE) setup_$(TESTPLAT)

runtests:
	$(MAKE) runtests_$(TESTPLAT)

cleanup: 
	$(MAKE) cleanup_$(TESTPLAT)


.PHONY: setup_linux runtests_linux cleanup_linux

setup_linux:
	-rm -rf $(PSN_DATA_DIR)
ifdef DEVELOP_BUILD_DEPS
	$(MAKE) -C $(WORKAREA)/sw_x \
		gvm_core/internal/vssi/tests/stream/vsStreamTest \
		tests/ccd/test_login \
		tests/test_utils/registerStorageNode \
	# end of list
endif
	-mkdir -p $(LOGDIR)
	cd $(IDT_TOOLS) && \
		./createUser -i ids.$(INTERNAL_LAB_DOMAIN_NAME) -u $(CCD_TEST_ACCOUNT) -p password
	#Create test data for streaming
	-mkdir -p $(PSN_DATA_DIR)/cc/sn
	dd if=/dev/urandom of=$(PSN_DATA_DIR)/cc/sn/test.mov bs=4096 count=1024

runtests_linux: 
	$(MAKE) vsStreamTest

vsStreamTest:
ifneq ($(CONFIG_ES),km) # Run PSN only if not kernel mode
	export SRCROOT=$(WORKAREA)/sw_x ; $(WORKAREA)/sw_x/tests/tools/actool.sh $(LAB_DOMAIN_NAME) $(PSN_DATA_DIR) &> $(LOGDIR)/1_actool.log
	sleep 1
	$(CAPTURE_CORE) && $(CCD) $(PSN_DATA_DIR) &> $(LOGDIR)/2_bg_ccd.log &
	-$(CAPTURE_CORE) && $(WAIT_FOR_CCD) &> $(LOGDIR)/3_wait_for_ccd.log # Allow ccd/psn to start up
	$(CAPTURE_CORE) && $(TESTROOT)/tests/test_utils/registerStorageNode $(CCD_TEST_ACCOUNT) password &> $(LOGDIR)/4_registerStorageNode.log
	sleep 1 # Why?
	$(CAPTURE_CORE) && $(TESTROOT)/tests/ccd/test_login/test_ccd_login -u $(CCD_TEST_ACCOUNT) -p password -n -e &> $(LOGDIR)/5_test_ccd_login.log
	sleep 1 # Why?
endif
	-$(CAPTURE_CORE) && $(VALGRIND) $(TESTROOT)/gvm_core/internal/vssi/tests/stream/vsStreamTest -u $(CCD_TEST_ACCOUNT) -p password \
		--vsds-name www-c100.$(LAB_DOMAIN_NAME) --ias-name www-c100.$(LAB_DOMAIN_NAME) &> $(LOGDIR)/6_vs.log
	-$(CAPTURE_CORE) && $(SHUTDOWN_CCD) &> $(LOGDIR)/7_shutdown_ccd.log
	-grep $(GREP_TEST_OUTPUT_REGEXP) $(LOGDIR)/6_vs.log

cleanup_linux: 
ifneq ($(CONFIG_ES),km) # Run PSN only if not kernel mode
	-killall ccd test_ccd_login vsStreamTest registerStorageNode ccd_ready_test ccd_shutdown_test
endif
	-cd $(IDT_TOOLS) && \
		./deleteUser -i ids.$(INTERNAL_LAB_DOMAIN_NAME) -u $(CCD_TEST_ACCOUNT)



.PHONY: setup_cloudnode runtests_cloudnode cleanup_cloudnode

setup_cloudnode:
	@if [ -d $(IDT_TOOLS) ]; then \
		echo 'IDT tool detected'; \
	else \
		echo 'IDT tool does not exist, check out the $(IDT_TOOLS)'; \
		false; \
	fi 
ifdef DEVELOP_BUILD_DEPS
	$(MAKE) -C $(WORKAREA)/sw_x \
		gvm_core/internal/vssi/tests/vsStream/vsStreamTest \
		tests/ccd/test_login \
		tests/test_utils/registerStorageNode \
	# end of list
endif
	-mkdir -p $(LOGDIR)
	cd $(IDT_TOOLS) && \
		./createUser -i ids.$(INTERNAL_LAB_DOMAIN_NAME) -u $(CCD_TEST_ACCOUNT) -p password
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
	@if [ -z "$(CCD_APPDATA_PATH)" ]; then echo "Bad variable"; exit 2; fi
	-$(REMOTE_RUN) 'rm -fr $(CCD_APPDATA_PATH)/*'
ifdef USE_ARCHIVED_BUILD
	$(REMOTE_RUN) 'wget $(WGET_OPTIONS) http://$(STORE_HOST):$(HTTP_PORT)/$(DXSHELL_PACKAGE_STORE_PATH)/$(DXSHELL_PACKAGE) -P $(TARGET_TESTROOT)'
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); $(UNZIP) $(DXSHELL_PACKAGE)'
else
	echo "sync local ccd and dxshell files to $(TARGET_TESTROOT)"
	rsync -p $(CCD_BINARY_PATH)/ccd_cloudnode $(TARGET_USER)@$(TARGET_MACHINE):$(TARGET_TESTROOT)
	rsync -p $(DXSHELL_BINARY_PATH)/dxshell_cloudnode $(TARGET_USER)@$(TARGET_MACHINE):$(TARGET_TESTROOT)
	rsync -p $(SRC_SWX)/gvm_core/conf/ccd.conf.tmpl $(TARGET_USER)@$(TARGET_MACHINE):$(TARGET_TESTROOT)
endif
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); mv ccd_cloudnode ccd'
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); mv dxshell_cloudnode dxshell'
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); echo -e "\n// Max total size of all log files\nmaxTotalLogSize = 1073741824\n" >> ccd.conf.tmpl'
	if [ "$(TEST_PACKAGE)" == "" ] || [ "$(USE_ARCHIVED_BUILD)" == "" ] || [ "$(USE_BUILDROOT_TESTS)" != "" ]; then \
	  cd $(BUILDROOT); rsync -pR  $(BUILDROOT_TEST_FILES) $(TARGET_USER)@$(TARGET_MACHINE):$(TARGET_TESTROOT); \
	else \
	  $(REMOTE_RUN) 'wget $(WGET_OPTIONS) http://$(STORE_HOST):$(HTTP_PORT)/$(TEST_PACKAGE_STORE_PATH)/$(TEST_PACKAGE) -P $(TARGET_TESTROOT)'; \
	  $(REMOTE_RUN) 'cd $(TARGET_TESTROOT); $(UNZIP) $(TEST_PACKAGE)'; \
	fi
	-$(REMOTE_RUN) rm -fr $(CCD_APPDATA_PATH)/cc




runtests_cloudnode:
	-$(REMOTE_RUN) '$(KILL) ccd test_ccd_login vsStreamTest registerStorageNode ccd_ready_test ccd_shutdown_test'
	@if [ -z "$(LOGDIR)" ]; then echo "Bad variable"; exit 2; fi
	rm -fr $(LOGDIR)/*
	mkdir -p $(LOGDIR)/ccd/special_logs
	$(REMOTE_RUN_PRIVILEGED) ' \
	  { \
	    ulimit -c unlimited && cd $(TARGET_TESTROOT) && $(AS_TARGET_USER) " \
	      ./dxshell SetDomain $(LAB_DOMAIN_NAME) && \
	      mkdir -p $(CCD_APPDATA_PATH)/cc/sn && \
	      dd if=/dev/urandom of=$(CCD_APPDATA_PATH)/cc/sn/test.mov bs=4096 count=1024 && \
	      ./dxshell StartCCD && ./dxshell StartCloudPC $(CCD_TEST_ACCOUNT) password && sleep 10 && \
	      $(VALGRIND) $(TARGET_TEST_BINARY) -u $(CCD_TEST_ACCOUNT) -p password \
	          --vsds-name www-c100.$(LAB_DOMAIN_NAME) --ias-name www-c100.$(LAB_DOMAIN_NAME) && \
	      ./dxshell StopCloudPC && ./dxshell StopCCD && echo \"ENTIRE TEST FINISHED\""; \
	  }' \
	2>&1 | tee $(LOGDIR)/$(MY_TEST_NAME).log | grep $(GREP_TEST_OUTPUT_REGEXP); \
	echo "PIPESTATUS[0] is $${PIPESTATUS[0]} PIPESTATUS[1] is $${PIPESTATUS[1]}";
	rsync $(TARGET_USER)@$(TARGET_MACHINE):$(CCD_APPDATA_PATH)/logs/ccd/*.log $(LOGDIR)/ccd
	rsync $(TARGET_USER)@$(TARGET_MACHINE):$(CCD_APPDATA_PATH)/logs/ccd/special_logs/*.log $(LOGDIR)/ccd/special_logs
	@if tail -n 1 $(LOGDIR)/$(MY_TEST_NAME).log | grep -q '^ENTIRE TEST FINISHED'; then \
	  echo "TC_RESULT = PASS ;;; TC_NAME = $(MY_TEST_NAME)_complete"; \
	else \
	  echo "*** $(MY_TEST_NAME) didn't complete!"; \
	  echo "TC_RESULT = FAIL ;;; TC_NAME = $(MY_TEST_NAME)_complete"; \
	fi


cleanup_cloudnode: 
	-$(REMOTE_RUN) '$(KILL) ccd test_ccd_login vsStreamTest registerStorageNode ccd_ready_test ccd_shutdown_test'
	-cd $(IDT_TOOLS) && \
		./deleteUser -i ids.$(INTERNAL_LAB_DOMAIN_NAME) -u $(CCD_TEST_ACCOUNT)

sinclude ../make/$(PRODUCT).makerules

