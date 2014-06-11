override MY_TEST_NAME=sync_config_test

export BUILDTYPE ?= release

# makefile included by all test makefiles
include ../../make/test_env.mk

ifeq ($(PRODUCT),cloudnode)
ifndef CLOUDPC_NAME
    $(error CLOUDPC_NAME must be either the hostname or ip address of an orbe)
endif
export TARGET_USER ?= build
export TARGET_MACHINE = $(CLOUDPC_NAME)
else
export TARGET_MACHINE ?= localhost
export TARGET_USER ?= $(USER)
endif

.PHONY: default setup runtests cleanup

ifeq ($(PRODUCT), win32_ul)
override TESTPLAT=win32
override TARGET_TESTROOT=/cygdrive/c/testroot
else
ifeq ($(PRODUCT), msvc)
override TESTPLAT=win32
override TARGET_TESTROOT=/cygdrive/c/testroot
else
ifeq ($(PRODUCT), winrt)
override TESTPLAT=winrt
override TARGET_TESTROOT=/cygdrive/c/tmp/igware/testroot
override TARGET_APPXURL_ROOT=file://c\|/tmp/igware/testroot
else
ifeq ($(PRODUCT), ios)
override TESTPLAT=ios
override TEST_BINARY_1=ios_SyncConfig
override TARGET_TESTROOT=$(WORKAREA)/sw_x/projects/xcode/PersonalCloud
override TARGET_SCRIPTROOT=$(WORKAREA)/sw_x/projects/xcode/PersonalCloud/applescripts
override IOS_LOG_DIR=/tmp/log
override APP_DATA_DIR=/Users/$(TARGET_USER)
CURRENT_IP=$(shell ifconfig | grep "inet " | grep -v 127.0.0.1 | cut -d\  -f2)
override IOS_LOG_RECEIVER_URL=http://$(CURRENT_IP)/LogReceiver/LogReceive.php
else
ifeq ($(PRODUCT), cloudnode)
override TESTPLAT=cloudnode
override TARGET_HOME=/home/$(TARGET_USER)
override TARGET_TESTROOT=$(TARGET_HOME)/temp/igware/testroot
override KILL=killall
override TEST_PACKAGE=tests_cloudnode.tar.gz
override TEST_PACKAGE_STORE_PATH=$(STORE_PATH)/$(TEST_BRANCH)/$(PRODUCT)/tests_cloudnode
override TEST_SYNC_CONFIG=$(BUILDTYPE)/$(PRODUCT)/tests/sync_config/syncConfig/syncConfigTest
override BUILDROOT_TEST_FILES=$(TEST_SYNC_CONFIG)
override REMOTE_RUN_PRIVILEGED=/usr/bin/ssh root@$(TARGET_MACHINE)
override AS_TARGET_USER=su $(TARGET_USER) -c
else
override TESTPLAT=linux
override TARGET_TESTROOT=/tmp/testroot
endif #($(PRODUCT), cloudnode)
endif #($(PRODUCT), ios)
endif #($(PRODUCT), winrt)
endif #($(PRODUCT), msvc)
endif #($(PRODUCT), win32_ul)

ifeq ($(PRODUCT), android)
override TESTPLAT=android
override TEST_BINARY=syncConfigTest-release.apk
TEST_INTENT ?= com.igware.action.START
TEST_BINARY_CLASS ?= com.igware.sync_config.test/.SyncConfigTestActivity
ERR_AVD_INUSE ="ERROR: the user data image is used by another emulator. aborting"
STR_BOOT_COMPLETE = "Boot is finished"
STR_TEST_COMPLETE = "SyncConfig TEST COMPLETED"
EMULATOR_LOG=$(LOGDIR)/emulator.log
EMULATOR_PID=$(LOGDIR)/emulator.pid
endif

export CCD_TEST_ACCOUNT ?= sc_acct_test_SET_YOUR_OWN_ACCT@igware.com

REMOTE_RUN=/usr/bin/ssh $(TARGET_USER)@$(TARGET_MACHINE)
UNZIP=unzip

export IDT_TOOLS = $(SRC_SWI)/tools/idt
# Make sure sw_i can be located.
ERROR_CHECK := $(shell test -d "$(SRC_SWI)" && echo OK)
ifneq ($(ERROR_CHECK),OK)
$(error The sw_i source tree could not be found at $(SRC_SWI).\
 This test requires sw_i to be checked-out.\
 You must override SRC_SWI if your sw_i and sw_x trees are not siblings)
endif

default:  
ifeq ($(strip $(wildcard $(SRC_SWX)/tests/sync_config/syncConfig/Makefile)),)
	$(error "can't locate sw_x/tests/sync_config/syncConfig/Makefile - current directory is $(shell pwd)")
endif
	make setup
	make runtests
	make cleanup

setup:
	@if [ -d $(IDT_TOOLS) ]; then \
		echo 'IDT tool detected'; \
	else \
		echo 'IDT tool does not exist, check out the $(IDT_TOOLS)'; \
		false; \
	fi
	-mkdir -p $(LOGDIR)
	$(MAKE) setup_$(TESTPLAT)
	-cd $(IDT_TOOLS) && \
		./deleteUser -i ids.$(INTERNAL_LAB_DOMAIN_NAME) -u $(CCD_TEST_ACCOUNT)
	cd $(IDT_TOOLS) && \
		./createUser -i ids.$(INTERNAL_LAB_DOMAIN_NAME) -u $(CCD_TEST_ACCOUNT) -p password

runtests:
	$(MAKE) runtests_$(TESTPLAT)

cleanup:
	$(MAKE) cleanup_$(TESTPLAT)

setup_win32:
	# Pull down the daemons and depended shared libraries from the saved tarball. This sequence supports 
	# test builder and test target to be on different machines
	# TESTROOT is the test root path of the test builder
	# TARGET_TESTROOT is the test root path of the test target
	# By default, the TARGET_MACHINE is localhost (i.e. on the same machine as test builder)
	-$(REMOTE_RUN) 'taskkill /F /IM test_sync_config.exe'
	-$(REMOTE_RUN) 'mv $(TARGET_TESTROOT)/*.DMP /cygdrive/c/syncConfigCore'
	-$(REMOTE_RUN) 'rm -Rf $(TARGET_TESTROOT)/*'
	-$(REMOTE_RUN) 'rm -Rf /cygdrive/c/tmp/testSyncConfig/*'
	-$(REMOTE_RUN) 'mkdir -p $(TARGET_TESTROOT)'
	$(REMOTE_RUN) 'wget --progress=dot:mega $(STORE_HOST):$(HTTP_PORT)/$(STORE_PATH)/$(TEST_BRANCH)/win_desk/unitTests/unitTests.zip -P $(TARGET_TESTROOT)'
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); $(UNZIP) unitTests.zip'

setup_linux:
	# Assume test drive and test target for linux is on the same machine for now
	-mv $(TARGET_TESTROOT)/core* /tmp/syncConfigCore
	-rm -Rf $(TARGET_TESTROOT)/*
	-mkdir -p $(TARGET_TESTROOT)
	cp $(TESTROOT)/tests/sync_config/syncConfig/syncConfigTest $(TARGET_TESTROOT)

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

setup_ios:
	mkdir -p $(IOS_LOG_DIR)
	chmod 777 $(IOS_LOG_DIR)
	echo -n "$(IOS_LOG_RECEIVER_URL)" > $(TARGET_TESTROOT)/ccd/SanityTest/LogReceiveURL

setup_winrt:
	rm -f $(LOGDIR)/*
	mkdir -p $(LOGDIR)/test_ccd
	-$(REMOTE_RUN) 'rm -Rf $(TARGET_TESTROOT)/*'
	-$(REMOTE_RUN) 'rm -Rf /cygdrive/c/Users/$(TARGET_USER)/Music/AcerCloud'
	$(REMOTE_RUN) 'mkdir -p /cygdrive/c/Users/$(TARGET_USER)/Music/AcerCloud'

	# Pull down ccd test app and required utility 
	echo "Download test metro app to $(TARGET_TESTROOT)/test_ccd"
	-$(REMOTE_RUN) 'mkdir -p $(TARGET_TESTROOT)/test_ccd'
	$(REMOTE_RUN) 'wget --progress=dot:mega $(STORE_HOST):$(HTTP_PORT)/$(STORE_PATH)/$(TEST_BRANCH)/winrt/test_ccd/test_ccd.zip -P $(TARGET_TESTROOT)/test_ccd'
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT)/test_ccd; unzip test_ccd.zip'

	# Remove elder metro app, and install the latest one
	echo "Remove test_ccd app"
	-$(REMOTE_RUN) 'cd $(TARGET_TESTROOT)/test_ccd; ./metro_app_utilities.exe remove f79747ca-8f1e-4a5f-92cc-a0aa94926cf1_1.0.0.0_x86__cb1hhkscw5m06'
	-$(REMOTE_RUN) 'cd $(TARGET_TESTROOT)/test_ccd/test_ccd_1.0.0.0_Win32_Test; /cygdrive/c/Windows/system32/certutil.exe -addstore root test_ccd_1.0.0.0_Win32.cer'
	-$(REMOTE_RUN) 'cd $(TARGET_TESTROOT)/test_ccd; ./metro_app_utilities.exe add $(TARGET_APPXURL_ROOT)/test_ccd/test_ccd_1.0.0.0_Win32_Test/Dependencies/x86/Microsoft.VCLibs.x86.11.00.appx'
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT)/test_ccd; ./metro_app_utilities.exe add $(TARGET_APPXURL_ROOT)/test_ccd/test_ccd_1.0.0.0_Win32_Test/test_ccd_1.0.0.0_Win32.appx'

	rsync $(SRC_SWX)/tests/test_utils/fileWatcher.py $(TARGET_USER)@$(TARGET_MACHINE):$(TARGET_TESTROOT)/test_ccd
	-$(REMOTE_RUN) 'echo -n -e "$(CCD_TEST_ACCOUNT)\npassword\n$(LAB_DOMAIN_NAME)\n443" > /cygdrive/c/Users/$(TARGET_USER)/Music/AcerCloud/syncConfig'

setup_android:
	mkdir -p $(LOGDIR)
	rm -f $(LOGDIR)/*
	# kill other emulator process
	$(eval pid = `pgrep $(ANDROID_EMULATOR_BINARY)`)
	@if [ "$(pid)" != "" ]; then \
	    echo "... killing running emulator process before auto-test... $(pid) ...";  \
	    pkill $(ANDROID_EMULATOR_BINARY); \
	fi

	$(ANDROID_EMULATOR) -no-window -no-boot-anim -noaudio -noskin -wipe-data -avd $(ANDROID_AVD_17) -partition-size 128 -netspeed full & echo $$! > $(EMULATOR_PID)
	$(ANDROID_ADB) start-server
	$(ANDROID_ADB) wait-for-device
	$(ANDROID_ADB) logcat -v time $(ANDROID_LOGCAT_STR) > $(EMULATOR_LOG) &
	$(ANDROID_WAIT) $(STR_BOOT_COMPLETE) $(EMULATOR_LOG)

	# Install test apk
	$(ANDROID_ADB) install $(BUILDROOT)/release/$(PRODUCT)/gvm_apps/syncConfigTest/platform_android/syncConfigTest/bin/$(TEST_BINARY)

runtests_win32: 
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT) && ./test_sync_config.exe $(CCD_TEST_ACCOUNT) password $(LAB_DOMAIN_NAME) 443'

runtests_linux:
	cd $(TARGET_TESTROOT) && $(VALGRIND) ./syncConfigTest $(CCD_TEST_ACCOUNT) password $(LAB_DOMAIN_NAME) 443

runtests_cloudnode:
	-$(REMOTE_RUN) '$(KILL) syncConfigTest'
	@if [ -z "$(LOGDIR)" ]; then echo "Bad variable"; exit 2; fi
	rm -fr $(LOGDIR)/*
	$(REMOTE_RUN_PRIVILEGED) ' \
	  { \
	    ulimit -c unlimited && cd $(TARGET_TESTROOT) && $(AS_TARGET_USER) " \
	      $(TEST_SYNC_CONFIG)  $(CCD_TEST_ACCOUNT) password $(LAB_DOMAIN_NAME) 443 && \
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

runtests_ios:
	export SRCROOT=$(SRC_SWX) ; $(SRC_SWX)/tests/tools/actool.sh $(LAB_DOMAIN_NAME) $(SRC_SWX)/projects/xcode/PersonalCloud/ccd/SanityTest
	echo -n -e "$(CCD_TEST_ACCOUNT)\npassword\n$(LAB_DOMAIN_NAME)\n443" > $(TARGET_TESTROOT)/ccd/SanityTest/TestArguments
	security unlock-keychain -p notsosecret $(HOME)/Library/Keychains/act-mve.keychain
	osascript $(TARGET_SCRIPTROOT)/RunProject.scpt $(TARGET_TESTROOT)/ccd/ccd.xcodeproj OnDeviceTest $(IOS_LOG_DIR)/$(TEST_BINARY_1).log Y
	osascript $(TARGET_SCRIPTROOT)/CloseProject.scpt $(TARGET_TESTROOT) ccd ccd
	cat $(IOS_LOG_DIR)/$(TEST_BINARY_1).log
	@if [ `grep '^CLEAN EXIT' $(IOS_LOG_DIR)/$(TEST_BINARY_1).log | wc -l` -gt 0 ]; then \
	  echo "$(TEST_BINARY_1) exited cleanly."; \
	  echo "TC_RESULT = PASS ;;; TC_NAME = $(TEST_BINARY_1)_clean_exit"; \
	else \
	  echo "*** $(TEST_BINARY_1) may have crashed! (Check $(TESTDIR) for a core file.)" | grep '.*'; \
	  echo "TC_RESULT = FAIL ;;; TC_NAME = $(TEST_BINARY_1)_clean_exit"; \
	fi

runtests_winrt:
	echo "Launch sync config test"
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT)/test_ccd; ./metro_app_utilities.exe launch f79747ca-8f1e-4a5f-92cc-a0aa94926cf1_cb1hhkscw5m06!App'
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT)/test_ccd; /usr/bin/python ./fileWatcher.py --filename=test_done --dstdir=/cygdrive/c/Users/$(TARGET_USER)/Music/AcerCloud --int=10 --to=600'
	-rsync $(TARGET_USER)@$(TARGET_MACHINE):/cygdrive/c/Users/$(TARGET_USER)/Music/AcerCloud/test_done $(LOGDIR)/test_ccd
	-rsync $(TARGET_USER)@$(TARGET_MACHINE):/cygdrive/c/Users/$(TARGET_USER)/AppData/Local/Packages/f79747ca-8f1e-4a5f-92cc-a0aa94926cf1_cb1hhkscw5m06/LocalState/logs/syncConfig/* $(LOGDIR)/test_ccd
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT)/test_ccd; ./metro_app_utilities.exe remove f79747ca-8f1e-4a5f-92cc-a0aa94926cf1_1.0.0.0_x86__cb1hhkscw5m06'
	$(REMOTE_RUN) 'rm -f /cygdrive/c/Users/$(TARGET_USER)/Music/AcerCloud/test_done'

	cat $(LOGDIR)/test_ccd/*.log
	@if [ ! -e $(LOGDIR)/test_ccd/test_done ]; then \
	  echo "TC_RESULT=FAIL ;;; TC_NAME = sync_config_test_complete"; \
	else \
	  echo "TC_RESULT=PASS ;;; TC_NAME = sync_config_test_complete"; \
	fi

runtests_android:
	# TODO: We need to launch an activity so that we can make JNI calls from C to Java.
	$(ANDROID_ADB) shell am start -a $(TEST_INTENT) -n $(TEST_BINARY_CLASS) \
	 --es account $(CCD_TEST_ACCOUNT) --es password password --es domain "$(LAB_DOMAIN_NAME)" --es port 443 \
	| tee $(LOGDIR)/$(TEST_BINARY).log
	source $(ANDROID_WAIT) $(STR_TEST_COMPLETE) $(EMULATOR_LOG)
	@grep "TC_RESULT=" $(EMULATOR_LOG) || true 
	@grep "  T" $(EMULATOR_LOG) || true
	@grep "TOTAL" $(EMULATOR_LOG) || true


log_collection_win32:

log_collection_linux:

log_collection_cloudnode:

log_collection_ios:
	export SRCROOT=$(SRC_SWX)
	-rm -rf $(APP_DATA_DIR)/*.xcappdata
	security unlock-keychain -p notsosecret $(HOME)/Library/Keychains/act-mve.keychain
	osascript $(TARGET_SCRIPTROOT)/CloseProject.scpt $(TARGET_TESTROOT) ccd ccd
	osascript $(TARGET_SCRIPTROOT)/BackupAndDeleteApp.scpt Backup OnDeviceTest 1
	chmod 777 $(TARGET_SCRIPTROOT)/WaitForAppData.sh
	$(TARGET_SCRIPTROOT)/WaitForAppData.sh $(APP_DATA_DIR)/www.acer.com.OnDeviceTest*.xcappdata
	-rsync -av $(APP_DATA_DIR)/www.acer.com.OnDeviceTest*.xcappdata $(LOGDIR)

log_collection_winrt:

log_collection_android:


cleanup_win32: 
	-$(REMOTE_RUN) 'mv $(TARGET_TESTROOT)/*.DMP /cygdrive/c/syncTesterCore'
	$(REMOTE_RUN) 'rm -Rf $(TARGET_TESTROOT)/*'

cleanup_linux:

cleanup_cloudnode: 
	-$(REMOTE_RUN) '$(KILL) syncConfigTest'

cleanup_ios:

cleanup_winrt:

cleanup_android:
	$(ANDROID_ADB) kill-server
	kill `cat $(EMULATOR_PID)`
	rm -f $(EMULATOR_PID)

post_test_run:
	-$(MAKE)  log_collection_$(TESTPLAT)

post_suite_timeout:


ifdef DEVELOP_BUILD_DEPS
setup runtests: builddeps

.PHONY: builddeps
builddeps:
	cd $(SRC_SWX) &&\
	  $(MAKE) tests/sync_config/syncConfig/syncConfigTest
endif
