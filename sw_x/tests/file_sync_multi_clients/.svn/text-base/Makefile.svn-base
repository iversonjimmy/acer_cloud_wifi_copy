SHELL=/bin/bash

override MY_TEST_NAME=file_sync_multi_clients

# -r is a GNU extension t
XARGS_CMDLINE = xargs -r

# auto detect
override WORKAREA=$(shell cd ../../..; pwd)

# override by env variable
export BUILDROOT ?= $(WORKAREA)/buildroot
export BUILDTYPE ?= release
ifeq ($(shell uname -o),Cygwin)
export PRODUCT ?= win32_ul
override USE_VALGRIND = 0
else
export PRODUCT ?= linux
export USE_VALGRIND ?= 0
endif

# makefile included by all test makefiles
include ../make/test_env.mk
sinclude ../make/$(PRODUCT).makedefs

export TARGET_MACHINE1 ?= frank-win1
export TARGET_MACHINE2 ?= frank-win2
export TARGET_USER ?= 'build'
export TESTCASE ?= 'simul_sync'
export CCD_TEST_ACCOUNT ?= defaultTester@acercloud.net
export PERSONAL_CLOUD_PASSWORD ?= 'password'
export IDT_TOOLS = $(SRC_SWI)/tools/idt

# Make sure sw_i can be located.
ERROR_CHECK := $(shell test -d "$(SRC_SWI)" && echo OK)
ifneq ($(ERROR_CHECK),OK)
$(error The sw_i source tree could not be found at $(SRC_SWI).\
 This test requires sw_i to be checked-out.\
 You must override SRC_SWI if your sw_i and sw_x trees are not siblings)
endif

ifeq ($(PRODUCT), win32_ul) 
override SETUP=setup_win32
override RUNTESTS=runtests_win32
override CLEANUP=cleanup_win32
override TARGET_TESTROOT=/cygdrive/c/Users/$(TARGET_USER)/igware/testroot
override TARGET_OS = win32
else
override SETUP=setup_linux
override RUNTESTS=runtests_linux
override CLEANUP=cleanup_linux
override TARGET_TESTROOT=/temp/igware/testroot
override TARGET_OS = linux
endif

# derived
override TESTROOT=$(BUILDROOT)/$(BUILDTYPE)/$(PRODUCT)
REMOTE_RUN1=/usr/bin/ssh $(TARGET_USER)@$(TARGET_MACHINE1)
REMOTE_RUN2=/usr/bin/ssh $(TARGET_USER)@$(TARGET_MACHINE2)

ifdef TEST_WORKDIR
# Use the suite-specific directory specified by the test harness.
LOGDIR=$(TEST_WORKDIR)
else
LOGDIR=$(BUILDROOT)/test_log/file_sync_multi_client
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
ifeq ($(strip $(wildcard $(WORKAREA)/sw_x/tests/file_sync_multi_clients/Makefile)),)
	$(error "can't locate sw_x/file_sync_multi_clients/Makefile - current directory is $(shell pwd)")
endif
	$(MAKE) setup
	-$(MAKE) runtests
	-$(MAKE) cleanup

setup:
	@if [ -d $(IDT_TOOLS) ]; then \
		echo 'IDT tool detected'; \
	else \
		echo 'IDT tool does not exist, check out the $(IDT_TOOLS)'; \
		false; \
	fi
	-mkdir -p $(LOGDIR)
	-cd $(IDT_TOOLS) && \
		./deleteUser -i ids.$(INTERNAL_LAB_DOMAIN_NAME) -u $(CCD_TEST_ACCOUNT)
	cd $(IDT_TOOLS) && \
		./createUser -i ids.$(INTERNAL_LAB_DOMAIN_NAME) -u $(CCD_TEST_ACCOUNT) -p password
	$(MAKE) $(SETUP)

runtests:
	$(MAKE) simul_sync

simul_sync:
	$(MAKE) testcase TESTCASE='simul_sync'

cam_sync:
	$(MAKE) testcase TESTCASE='cam_sync'

cleanup:
	$(MAKE) $(CLEANUP)

setup_win32:
	-rm $(LOGDIR)/*.out
	export SRCROOT=$(SRC_SWX) ; $(SRC_SWX)/tests/tools/actool.sh $(LAB_DOMAIN_NAME) `pwd` &> $(LOGDIR)/actool.log
	# Computer #1
	-$(REMOTE_RUN1) 'taskkill /F /IM ccd.exe'
	-$(REMOTE_RUN1) 'rm -Rf /cygdrive/c/Users/$(TARGET_USER)/igware/*'
	-$(REMOTE_RUN1) 'mkdir -p $(TARGET_TESTROOT)' 
	$(REMOTE_RUN1) 'wget --progress=dot:mega $(STORE_HOST):$(HTTP_PORT)/$(STORE_PATH)/$(TEST_BRANCH)/win32_ul/win32_ul_output/win32_ul_output.tar.gz -P $(TARGET_TESTROOT)'
	$(REMOTE_RUN1) 'tar -xzvf $(TARGET_TESTROOT)/win32_ul_output.tar.gz -C $(TARGET_TESTROOT)'
	$(REMOTE_RUN1) 'wget --progress=dot:mega $(STORE_HOST):$(HTTP_PORT)/$(STORE_PATH)/$(TEST_BRANCH)/win_desk/ccd/ccd.zip -P $(TARGET_TESTROOT)'
	$(REMOTE_RUN1) 'cd $(TARGET_TESTROOT); unzip ccd.zip'
	$(REMOTE_RUN1) 'cp $(TARGET_TESTROOT)/usr/lib/*.dll $(TARGET_TESTROOT)'
	$(REMOTE_RUN1) 'cp $(TARGET_TESTROOT)/usr/bin/*.dll $(TARGET_TESTROOT)'
	$(REMOTE_RUN1) 'cp $(TARGET_TESTROOT)/tests/test_utils/*.exe $(TARGET_TESTROOT)'
	rsync -p $(WORKAREA)/sw_x/tests/test_utils/*.py $(TARGET_USER)@$(TARGET_MACHINE1):$(TARGET_TESTROOT)
	-$(REMOTE_RUN1) 'mkdir -p /cygdrive/c/Users/$(TARGET_USER)/igware/conf'
	rsync -p conf/ccd.conf  $(TARGET_USER)@$(TARGET_MACHINE1):/cygdrive/c/Users/$(TARGET_USER)/igware/conf/ccd.conf
	# Computer #2
	-$(REMOTE_RUN2) 'taskkill /F /IM ccd.exe'
	-$(REMOTE_RUN2) 'rm -Rf /cygdrive/c/Users/$(TARGET_USER)/igware/*'
	-$(REMOTE_RUN2) 'mkdir -p $(TARGET_TESTROOT)' 
	$(REMOTE_RUN2) 'wget --progress=dot:mega $(STORE_HOST):$(HTTP_PORT)/$(STORE_PATH)/$(TEST_BRANCH)/win32_ul/win32_ul_output/win32_ul_output.tar.gz -P $(TARGET_TESTROOT)'
	$(REMOTE_RUN2) 'tar -xzvf $(TARGET_TESTROOT)/win32_ul_output.tar.gz -C $(TARGET_TESTROOT)'
	$(REMOTE_RUN2) 'wget --progress=dot:mega $(STORE_HOST):$(HTTP_PORT)/$(STORE_PATH)/$(TEST_BRANCH)/win_desk/ccd/ccd.zip -P $(TARGET_TESTROOT)'
	$(REMOTE_RUN2) 'cd $(TARGET_TESTROOT); unzip ccd.zip'
	$(REMOTE_RUN2) 'cp $(TARGET_TESTROOT)/usr/lib/*.dll $(TARGET_TESTROOT)'
	$(REMOTE_RUN2) 'cp $(TARGET_TESTROOT)/usr/bin/*.dll $(TARGET_TESTROOT)'
	$(REMOTE_RUN2) 'cp $(TARGET_TESTROOT)/tests/test_utils/*.exe $(TARGET_TESTROOT)'
	rsync -p $(WORKAREA)/sw_x/tests/test_utils/*.py $(TARGET_USER)@$(TARGET_MACHINE2):$(TARGET_TESTROOT)
	-$(REMOTE_RUN2) 'mkdir -p /cygdrive/c/Users/$(TARGET_USER)/igware/conf'
	rsync -p conf/ccd.conf  $(TARGET_USER)@$(TARGET_MACHINE2):/cygdrive/c/Users/$(TARGET_USER)/igware/conf/ccd.conf
	# Remove temp files
	-rm -rf conf

testcase: 
	python test_master.py --target1 $(TARGET_MACHINE1) --target2 $(TARGET_MACHINE2) --targetlogin $(TARGET_USER) --syncroot /cygdrive/c/MyCloud --os win32 --username $(CCD_TEST_ACCOUNT) --password $(PERSONAL_CLOUD_PASSWORD) --logdir $(LOGDIR) --testcase $(TESTCASE)
	-mkdir -p $(LOGDIR)/target1
	-mkdir -p $(LOGDIR)/target2
	rsync $(TARGET_USER)@$(TARGET_MACHINE1):$(TARGET_TESTROOT)/*.log $(LOGDIR)/target1
	rsync $(TARGET_USER)@$(TARGET_MACHINE2):$(TARGET_TESTROOT)/*.log $(LOGDIR)/target2

cleanup_win32: 
	-$(REMOTE_RUN1) 'taskkill /F /IM ccd.exe'
	#-$(REMOTE_RUN1) 'rm -Rf $(TARGET_TESTROOT)/*'
	-$(REMOTE_RUN2) 'taskkill /F /IM ccd.exe'
	#-$(REMOTE_RUN2) 'rm -Rf $(TARGET_TESTROOT)/*'

setup_linux:
	-rm $(LOGDIR)/*.out $(LOGDIR)/*.log
	-$(REMOTE_RUN1) 'killall ccd'
	-$(REMOTE_RUN1) 'mkdir -p $(TARGET_TESTROOT)' 
	rsync -p $(TESTROOT)/gvm_core/daemons/ccd/ccd $(TARGET_USER)@$(TARGET_MACHINE1):$(TARGET_TESTROOT)
	rsync -p $(TESTROOT)/tests/test_utils/* $(TARGET_USER)@$(TARGET_MACHINE1):$(TARGET_TESTROOT)
	rsync -p $(WORKAREA)/sw_x/tests/test_utils/*.py $(TARGET_USER)@$(TARGET_MACHINE1):$(TARGET_TESTROOT)
	-$(REMOTE_RUN2) 'killall ccd'
	-$(REMOTE_RUN2) 'mkdir -p $(TARGET_TESTROOT)' 
	rsync -p $(TESTROOT)/gvm_core/daemons/ccd/ccd $(TARGET_USER)@$(TARGET_MACHINE2):$(TARGET_TESTROOT)
	rsync -p $(TESTROOT)/tests/test_utils/* $(TARGET_USER)@$(TARGET_MACHINE2):$(TARGET_TESTROOT)
	rsync -p $(WORKAREA)/sw_x/tests/test_utils/*.py $(TARGET_USER)@$(TARGET_MACHINE2):$(TARGET_TESTROOT)

cleanup_linux:
	-$(REMOTE_RUN1) 'killall ccd'
	#-$(REMOTE_RUN1) 'rm -Rf $(TARGET_TESTROOT)/*'
	-$(REMOTE_RUN2) 'killall ccd'
	#-$(REMOTE_RUN2) 'rm -Rf $(TARGET_TESTROOT)/*'

sinclude ../make/$(PRODUCT).makerules
