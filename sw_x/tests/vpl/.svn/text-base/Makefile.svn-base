override MY_TEST_NAME=vpl

# makefile included by all test makefiles
include ../make/test_env.mk

override TESTPLAT=linux
override TESTDIR = $(TESTROOT)/gvm_core/vpl/tests
override TEST_BINARY=vplTest
override TARGET_TESTROOT=/tmp/igware/testroot

ifeq ($(PRODUCT), msvc)
override TESTPLAT=win32
override TEST_BINARY=vplTest.exe
override TARGET_TESTROOT=/cygdrive/c/tmp/igware/testroot
override PBROOT=$(BUILDROOT)/$(BUILDTYPE)/$(PRODUCT)
endif

ifeq ($(PRODUCT), winrt)
override TESTPLAT=winrt
override TARGET_TESTROOT=/cygdrive/c/tmp/igware/testroot
override TARGET_APPXURL_ROOT=file://c\|/tmp/igware/testroot
endif

ifeq ($(PRODUCT), android)
override TESTPLAT=android
endif

ifeq ($(PRODUCT), ios)
override TESTPLAT=ios
override TEST_BINARY=ios_vplTest
override TARGET_TESTROOT=$(WORKAREA)/sw_x/projects/xcode/PersonalCloud
override TARGET_SCRIPTROOT=$(WORKAREA)/sw_x/projects/xcode/PersonalCloud/applescripts
override IOS_LOG_DIR=/tmp/log
override APP_DATA_DIR=/Users/$(TARGET_USER)
CURRENT_IP=$(shell ifconfig | grep "inet " | grep -v 127.0.0.1 | cut -d\  -f2)
override IOS_LOG_RECEIVER_URL=http://$(CURRENT_IP)/LogReceiver/LogReceive.php
endif

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
override TEST_BINARY=vplTest
override TARGET_TEST_BINARY=$(BUILDTYPE)/$(PRODUCT)/gvm_core/vpl/tests/$(TEST_BINARY)
override BUILDROOT_TEST_FILES=$(TARGET_TEST_BINARY)
override WGET_OPTIONS=
override REMOTE_RUN_PRIVILEGED=/usr/bin/ssh root@$(TARGET_MACHINE)
override AS_TARGET_USER=su $(TARGET_USER) -c
endif


EMULATOR_PID=$(TESTDIR)/emulator.pid
EMULATOR_LOG=$(TESTDIR)/emulator.log
REMOTE_RUN=/usr/bin/ssh $(TARGET_USER)@$(TARGET_MACHINE)
UNZIP=unzip

.PHONY: default verify_dir setup runtests cleanup

default: verify_dir setup runtests cleanup

verify_dir:  
ifeq ($(strip $(wildcard $(WORKAREA)/sw_x/tests/vpl/Makefile)),)
	$(error "can't locate sw_x/vpl/Makefile - current directory is $(shell pwd)")
endif

setup: verify_dir
	-mkdir -p $(LOGDIR)
	$(MAKE) setup_$(TESTPLAT)

runtests: setup
	$(MAKE) runtests_$(TESTPLAT)

cleanup:
	$(MAKE) cleanup_$(TESTPLAT)

setup_android:
	mkdir -p $(LOGDIR)
	rm -f $(LOGDIR)/*
	$(ANDROID_EMULATOR) -no-window -no-boot-anim -noaudio -noskin -wipe-data -avd $(ANDROID_AVD_10) -logcat $(ANDROID_LOGCAT_STR) > $(EMULATOR_LOG) & echo $$! > $(EMULATOR_PID)
	$(ANDROID_ADB) start-server
	$(ANDROID_ADB) wait-for-device
	$(ANDROID_EMULATOR_WAIT) $(EMULATOR_LOG) 
	$(ANDROID_ADB) push $(TESTDIR)/$(TEST_BINARY) /data/app

setup_win32:
	# Pull down the daemons and depended shared libraries from the saved tarball. This sequence supports
	# test builder and test target to be on different machines
	# TESTROOT is the test root path of the test builder
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
	mkdir -p $(LOGDIR)
	rm -f $(LOGDIR)/*
	-$(REMOTE_RUN) 'rm -Rf $(TARGET_TESTROOT)/*'
	-$(REMOTE_RUN) 'rm -Rf /cygdrive/c/temp/igware/*'
	-$(REMOTE_RUN) 'mkdir -p $(TARGET_TESTROOT)'
	$(REMOTE_RUN) 'wget --progress=dot:mega $(STORE_HOST):$(HTTP_PORT)/$(STORE_PATH)/$(TEST_BRANCH)/win_desk/unitTests/unitTests.zip -P $(TARGET_TESTROOT)'
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); $(UNZIP) unitTests.zip'
ifndef USE_ARCHIVED_BUILD
ifneq ($(PRODUCT), win32_ul)    # PRODUCT=win32_ul does not support building of dxshell binary 
	echo "sync test files to $(TARGET_TESTROOT)"
	rsync -p $(BUILDROOT)/build_msvc/PersonalCloudAll/Release/vplTest/Win32/vplTest$(EXE) $(TARGET_USER)@$(TARGET_MACHINE):$(TARGET_TESTROOT)
else
	echo ">>>> WARNING : PRODUCT=win32_ul does not build test binaries. Force to use archived build"
endif
endif

setup_winrt:
	rm -f $(LOGDIR)/*
	-$(REMOTE_RUN) 'rm -Rf $(TARGET_TESTROOT)/*'
	-$(REMOTE_RUN) 'rm -Rf /cygdrive/c/Users/$(TARGET_USER)/Music/AcerCloud'

	# Pull down vpl test app and required utility 
	echo "sync vpl test metro app to $(TARGET_TESTROOT)/test_vpl_metro"
	-$(REMOTE_RUN) 'mkdir -p $(TARGET_TESTROOT)/test_vpl_metro'
	$(REMOTE_RUN) 'wget --progress=dot:mega $(STORE_HOST):$(HTTP_PORT)/$(STORE_PATH)/$(TEST_BRANCH)/winrt/test_vpl_metro/test_vpl_metro.zip -P $(TARGET_TESTROOT)/test_vpl_metro'
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT)/test_vpl_metro; unzip test_vpl_metro.zip'

	# Remove elder metro app, and install the latest one
	-$(REMOTE_RUN) 'cd $(TARGET_TESTROOT)/test_vpl_metro; ./metro_app_utilities.exe remove a610a011-590b-46e1-9f9f-3cf2c7789ac4_1.0.0.0_x86__cb1hhkscw5m06'
	-$(REMOTE_RUN) 'cd $(TARGET_TESTROOT)/test_vpl_metro/test_vpl_metro_1.0.0.0_Win32_Test; /cygdrive/c/Windows/system32/certutil.exe -addstore root test_vpl_metro_1.0.0.0_Win32.cer'
	-$(REMOTE_RUN) 'cd $(TARGET_TESTROOT)/test_vpl_metro; ./metro_app_utilities.exe add $(TARGET_APPXURL_ROOT)/test_vpl_metro/test_vpl_metro_1.0.0.0_Win32_Test/Dependencies/x86/Microsoft.VCLibs.x86.11.00.appx'
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT)/test_vpl_metro; ./metro_app_utilities.exe add $(TARGET_APPXURL_ROOT)/test_vpl_metro/test_vpl_metro_1.0.0.0_Win32_Test/test_vpl_metro_1.0.0.0_Win32.appx'
	rsync $(SRC_SWX)/tests/test_utils/fileWatcher.py $(TARGET_USER)@$(TARGET_MACHINE):$(TARGET_TESTROOT)/test_vpl_metro
	-$(REMOTE_RUN) 'mkdir -p /cygdrive/c/Users/$(TARGET_USER)/Music/AcerCloud'
	-$(REMOTE_RUN) 'echo -n -e "$(LAB_DOMAIN_NAME)" > /cygdrive/c/Users/$(TARGET_USER)/AppData/Local/Packages/a610a011-590b-46e1-9f9f-3cf2c7789ac4_cb1hhkscw5m06/LocalState/dns'

setup_linux:
	mkdir -p $(LOGDIR)
	rm -f $(LOGDIR)/*
	if [ -e '$(WORKAREA)/sw_x/gvm_core/vpl/tests/platform_$(PLATFORM)/libExampleSharedLibrary.so' ]; then \
	  mkdir -p $(TESTDIR)/lib; \
	  cp $(WORKAREA)/sw_x/gvm_core/vpl/tests/platform_$(PLATFORM)/libExampleSharedLibrary.so $(TESTDIR)/lib/; \
	fi

setup_ios:
	mkdir -p $(IOS_LOG_DIR)
	chmod 777 $(IOS_LOG_DIR)
	echo -n "$(IOS_LOG_RECEIVER_URL)" > $(TARGET_TESTROOT)/libvpl/OnDeviceTest/LogReceiveURL

runtests_android:
	$(ANDROID_ADB) shell "/data/app/$(TEST_BINARY) $(TESTARGS)" 2>&1 \
	  | tee $(LOGDIR)/$(TEST_BINARY).log \
	  | grep $(GREP_TEST_OUTPUT_REGEXP)
	@if [ `grep '^CLEAN EXIT' $(LOGDIR)/$(TEST_BINARY).log | wc -l` -gt 0 ]; then \
	  echo "$(TEST_BINARY) exited cleanly."; \
	  echo "TC_RESULT = PASS ;;; TC_NAME = $(TEST_BINARY)_clean_exit"; \
	else \
	  echo "*** $(TEST_BINARY) may have crashed! (Check Android logcat for possible details.)" | grep '.*'; \
	  echo "TC_RESULT = FAIL ;;; TC_NAME = $(TEST_BINARY)_clean_exit"; \
	fi

runtests_win32:
	-$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); ./$(TEST_BINARY) $(TESTARGS)' 2>&1 \
	  | tee $(LOGDIR)/$(TEST_BINARY).log \
	  | grep $(GREP_TEST_OUTPUT_REGEXP)
	@if [ `grep '^CLEAN EXIT' $(LOGDIR)/$(TEST_BINARY).log | wc -l` -gt 0 ]; then \
	  echo "$(TEST_BINARY) exited cleanly."; \
	  echo "TC_RESULT = PASS ;;; TC_NAME = $(TEST_BINARY)_clean_exit"; \
	else \
	  echo "*** $(TEST_BINARY) may have crashed! (Check the Windows Error Reporting folder for a dump file.)" | grep '.*'; \
	  echo "TC_RESULT = FAIL ;;; TC_NAME = $(TEST_BINARY)_clean_exit"; \
	fi

runtests_winrt:
	echo "Launch vpl test"
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT)/test_vpl_metro; ./metro_app_utilities.exe launch a610a011-590b-46e1-9f9f-3cf2c7789ac4_cb1hhkscw5m06!App'
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT)/test_vpl_metro; /usr/bin/python ./fileWatcher.py --filename=test_done --dstdir=/cygdrive/c/Users/$(TARGET_USER)/Music/AcerCloud --int=10 --to=600'
	-rsync $(TARGET_USER)@$(TARGET_MACHINE):/cygdrive/c/Users/$(TARGET_USER)/Music/AcerCloud/test_done $(LOGDIR)
	rsync $(TARGET_USER)@$(TARGET_MACHINE):/cygdrive/c/Users/$(TARGET_USER)/AppData/Local/Packages/a610a011-590b-46e1-9f9f-3cf2c7789ac4_cb1hhkscw5m06/LocalState/test_vpl.log $(LOGDIR)
	cat $(LOGDIR)/test_vpl.log
	@if [ -e $(LOGDIR)/test_done ]; then \
	  echo "$(TEST_BINARY) exited cleanly."; \
	  echo "TC_RESULT = PASS ;;; TC_NAME = $(TEST_BINARY)_clean_exit"; \
	else \
	  echo "TC_RESULT = FAIL ;;; TC_NAME = $(TEST_BINARY)_clean_exit"; \
	fi

runtests_linux:
	-cd $(TESTDIR) &&\
	  $(VALGRIND) ./$(TEST_BINARY) $(TESTARGS) 2>&1 \
	    | tee $(LOGDIR)/$(TEST_BINARY).log \
	    | grep $(GREP_TEST_OUTPUT_REGEXP)
	@if [ `grep '^CLEAN EXIT' $(LOGDIR)/$(TEST_BINARY).log | wc -l` -gt 0 ]; then \
	  echo "$(TEST_BINARY) exited cleanly."; \
	  echo "TC_RESULT = PASS ;;; TC_NAME = $(TEST_BINARY)_clean_exit"; \
	else \
	  echo "*** $(TEST_BINARY) may have crashed! (Check $(TESTDIR) for a core file.)" | grep '.*'; \
	  echo "TC_RESULT = FAIL ;;; TC_NAME = $(TEST_BINARY)_clean_exit"; \
	fi

runtests_ios:
	security unlock-keychain -p notsosecret $(HOME)/Library/Keychains/act-mve.keychain
	osascript $(TARGET_SCRIPTROOT)/RunProject.scpt $(TARGET_TESTROOT)/libvpl/vpl.xcodeproj OnDeviceVPLTest $(IOS_LOG_DIR)/$(TEST_BINARY).log
	osascript $(TARGET_SCRIPTROOT)/CloseProject.scpt $(TARGET_TESTROOT) libvpl vpl
	cat $(IOS_LOG_DIR)/$(TEST_BINARY).log
	@if [ `grep '^CLEAN EXIT' $(IOS_LOG_DIR)/$(TEST_BINARY).log | wc -l` -gt 0 ]; then \
	  echo "$(TEST_BINARY) exited cleanly."; \
	  echo "TC_RESULT = PASS ;;; TC_NAME = $(TEST_BINARY)_clean_exit"; \
	else \
	  echo "*** $(TEST_BINARY) may have crashed!" | grep '.*'; \
	  echo "TC_RESULT = FAIL ;;; TC_NAME = $(TEST_BINARY)_clean_exit"; \
	fi

cleanup_android:
	$(ANDROID_ADB) kill-server
	kill `cat $(EMULATOR_PID)` 
	rm -f $(EMULATOR_PID)
	rm -f $(EMULATOR_LOG)

cleanup_ios:

cleanup_win32:
	-$(REMOTE_RUN) 'rm -Rf $(TARGET_TESTROOT)/*'

cleanup_winrt:
	-$(REMOTE_RUN) 'cd $(TARGET_TESTROOT)/test_vpl_metro; ./metro_app_utilities.exe remove a610a011-590b-46e1-9f9f-3cf2c7789ac4_1.0.0.0_x86__cb1hhkscw5m06'
	-$(REMOTE_RUN) 'rm -Rf $(TARGET_TESTROOT)/*'
	-$(REMOTE_RUN) 'rm -Rf /cygdrive/c/Users/$(TARGET_USER)/Music/AcerCloud'

cleanup_linux:

ifdef DEVELOP_BUILD_DEPS
setup runtests: builddeps

.PHONY: builddeps
builddeps:
	cd $(WORKAREA)/sw_x &&\
	  $(MAKE) gvm_core/vpl/tests/vplTest
endif


.PHONY: setup_cloudnode cleanup_cloudnode runtests_cloudnode

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
	@if [ -z "$(LOGDIR)" ]; then echo "*** Bad variable LOGDIR"; exit 2; fi
	-mkdir -p $(LOGDIR)
	rm -fr $(LOGDIR)/*
	@if [ -z "$(TARGET_TESTROOT)" ]; then echo "*** Bad variable TARGET_TESTROOT"; exit 2; fi
	-$(REMOTE_RUN) 'mkdir -p $(TARGET_TESTROOT)'
	-$(REMOTE_RUN) 'rm -fr $(TARGET_TESTROOT)/*'
	if [ "$(TEST_PACKAGE)" == "" ] || [ "$(USE_ARCHIVED_BUILD)" == "" ] || [ "$(USE_BUILDROOT_TESTS)" != "" ]; then \
	  cd $(BUILDROOT); rsync -pR  $(BUILDROOT_TEST_FILES) $(TARGET_USER)@$(TARGET_MACHINE):$(TARGET_TESTROOT); \
	else \
	  $(REMOTE_RUN) 'wget $(WGET_OPTIONS) http://$(STORE_HOST):$(HTTP_PORT)/$(TEST_PACKAGE_STORE_PATH)/$(TEST_PACKAGE) -P $(TARGET_TESTROOT)'; \
	  $(REMOTE_RUN) 'cd $(TARGET_TESTROOT); $(UNZIP) $(TEST_PACKAGE)'; \
	fi


runtests_cloudnode:
	-$(REMOTE_RUN) '$(KILL) $(TEST_BINARY)'
	$(REMOTE_RUN_PRIVILEGED) ' \
	  { \
	    ulimit -c unlimited && cd $(TARGET_TESTROOT) && $(AS_TARGET_USER) " \
	        ./$(TARGET_TEST_BINARY) $(TESTARGS)"; \
	  }' \
	2>&1 | tee $(LOGDIR)/$(TEST_BINARY).log | grep $(GREP_TEST_OUTPUT_REGEXP); \
	echo "PIPESTATUS[0] is $${PIPESTATUS[0]} PIPESTATUS[1] is $${PIPESTATUS[1]}";
	@if [ `grep '^CLEAN EXIT' $(LOGDIR)/$(TEST_BINARY).log | wc -l` -gt 0 ]; then \
	  echo "$(TEST_BINARY) exited cleanly."; \
	  echo "TC_RESULT = PASS ;;; TC_NAME = $(TEST_BINARY)_clean_exit"; \
	else \
	  echo "*** $(TEST_BINARY) may have crashed! (Check /home/dev/coredump for a core file.)" | grep '.*'; \
	  echo "TC_RESULT = FAIL ;;; TC_NAME = $(TEST_BINARY)_clean_exit"; \
	fi


cleanup_cloudnode: 


log_collection_win32:

log_collection_linux:
	# Attempt to create core dumps for potentially deadlocked processes by sending SIGQUIT (signal 3).
	-pkill -3 $(TEST_BINARY)
	-rsync $(TESTDIR)/core* $(LOGDIR)/

log_collection_cloudnode:

log_collection_ios:
	export SRCROOT=$(SRC_SWX)
	-rm -rf $(APP_DATA_DIR)/*.xcappdata
	security unlock-keychain -p notsosecret $(HOME)/Library/Keychains/act-mve.keychain
	osascript $(TARGET_SCRIPTROOT)/CloseProject.scpt $(TARGET_TESTROOT) libvpl vpl
	osascript $(TARGET_SCRIPTROOT)/BackupAndDeleteApp.scpt Backup OnDeviceVPLTest 1
	chmod 777 $(TARGET_SCRIPTROOT)/WaitForAppData.sh
	$(TARGET_SCRIPTROOT)/WaitForAppData.sh $(APP_DATA_DIR)/Acer.OnDeviceVPLTest*.xcappdata
	-rsync -av $(APP_DATA_DIR)/Acer.OnDeviceVPLTest*.xcappdata $(LOGDIR)

log_collection_winrt:

log_collection_android:


post_test_run:
	-$(MAKE)  log_collection_$(TESTPLAT)

post_suite_timeout:

