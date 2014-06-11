override MY_TEST_NAME=ccd

ifeq ($(shell uname -o),Cygwin)
export PRODUCT ?= win32_ul
override USE_VALGRIND = 0
else
export PRODUCT ?= linux
export USE_VALGRIND ?= 1
endif

# makefile included by all test makefiles
include ../make/test_env.mk
sinclude ../make/$(PRODUCT).makedefs

export TARGET_MACHINE ?= localhost
export TARGET_USER ?= $(USER)
export IDT_TOOLS = $(SRC_SWI)/tools/idt

# Make sure sw_i can be located.
ERROR_CHECK := $(shell test -d "$(SRC_SWI)" && echo OK)
ifneq ($(ERROR_CHECK),OK)
$(error The sw_i source tree could not be found at $(SRC_SWI).\
 This test requires sw_i to be checked-out.\
 You must override SRC_SWI if your sw_i and sw_x trees are not siblings)
endif

# derived
override CCD=$(TESTROOT)/gvm_core/daemons/ccd/ccd
override WAIT_FOR_CCD=$(TESTROOT)/gvm_core/daemons/ccd/tests/ccd_ready_test
override SHUTDOWN_CCD=$(TESTROOT)/gvm_core/daemons/ccd/tests/ccd_shutdown_test
override CCD_APPDATA = $(TEST_TEMPDIR)/ccdAppData

ifeq ($(PRODUCT), win32_ul)
override TESTPLAT=win32
override TARGET_TESTROOT=/cygdrive/c/tmp/igware/testroot
else
ifeq ($(PRODUCT), msvc)
override TESTPLAT=win32
override CCDPATH=$(BUILDROOT)/build_msvc/PersonalCloudAll/Release/CCD/Win32
override TARGET_TESTROOT=/cygdrive/c/tmp/igware/testroot
else
ifeq ($(PRODUCT), winrt)
override TESTPLAT=winrt
override TARGET_TESTROOT=/cygdrive/c/tmp/igware/testroot
override TARGET_APPXURL_ROOT=file://c\|/tmp/igware/testroot
else
ifeq ($(PRODUCT), ios)
override TESTPLAT=ios
override TEST_BINARY_1=ios_ccdTest
override TEST_BINARY_2=ios_ccdSSOTest_normal
override TEST_BINARY_3=ios_ccdSSOTest_session
override TEST_BINARY_4=ios_ccdSSOTest_normal_clean_app
override TARGET_TESTROOT=$(WORKAREA)/sw_x/projects/xcode/PersonalCloud
override TARGET_SCRIPTROOT=$(WORKAREA)/sw_x/projects/xcode/PersonalCloud/applescripts
override IOS_LOG_DIR=/tmp/log
CURRENT_IP=$(shell ifconfig | grep "inet " | grep -v 127.0.0.1 | cut -d\  -f2)
override IOS_LOG_RECEIVER_URL=http://$(CURRENT_IP)/LogReceiver/LogReceive.php
else
override TESTPLAT=linux
override TARGET_TESTROOT=/tmp/igware/testroot
endif
endif
endif
endif

REMOTE_RUN=/usr/bin/ssh $(TARGET_USER)@$(TARGET_MACHINE)
UNZIP=unzip
EXE=.exe

export CCD_TEST_ACCOUNT ?= defaultCCDTester@igware.com

.PHONY: default setup cleanup runtests
default: verify_dir setup runtests cleanup

verify_dir:
ifeq ($(strip $(wildcard $(SRC_SWX)/tests/ccd/Makefile)),)
	$(error "can't locate sw_x/ccd/Makefile - current directory is $(shell pwd)")
endif

setup: cleanup verify_dir
	@if [ -d $(IDT_TOOLS) ]; then \
		echo 'IDT tool detected'; \
	else \
		echo 'IDT tool does not exist, check out the $(IDT_TOOLS)'; \
		false; \
	fi 
	-mkdir -p $(LOGDIR)
	-mkdir -p $(CCD_APPDATA)
	cd $(IDT_TOOLS) && \
		./createUser -i ids.$(INTERNAL_LAB_DOMAIN_NAME) -u $(CCD_TEST_ACCOUNT) -p password
	$(MAKE) setup_$(TESTPLAT)

runtests: setup
	$(MAKE) runtests_$(TESTPLAT)

cleanup:
	$(MAKE) cleanup_$(TESTPLAT)
	-cd $(IDT_TOOLS) && \
		./deleteUser -i ids.$(INTERNAL_LAB_DOMAIN_NAME) -u $(CCD_TEST_ACCOUNT)

post_suite_timeout:
	$(MAKE) post_suite_timeout_$(TESTPLAT)

# completely remove ccd state
clean:
	rm -fr $(TEST_TEMPDIR)/*
	rm -fr $(HOME)/"My Cloud"/*

setup_win32:
	# Pull down the daemons and depended shared libraries from archived tarball. This sequence supports 
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
	rm -rf $(LOGDIR)/*
	-$(REMOTE_RUN) 'taskkill /F /IM ccd.exe'
	-$(REMOTE_RUN) 'rm -Rf $(TARGET_TESTROOT)/*'
	-$(REMOTE_RUN) 'rm -Rf /cygdrive/c/Users/$(TARGET_USER)/igware/*'
	-$(REMOTE_RUN) 'rm -Rf /cygdrive/c/Users/$(TARGET_USER)/"My Cloud"/*'
	-$(REMOTE_RUN) 'mkdir -p $(TARGET_TESTROOT)'
	$(REMOTE_RUN) 'wget --progress=dot:mega $(STORE_HOST):$(HTTP_PORT)/$(STORE_PATH)/$(TEST_BRANCH)/win_desk/unitTests/unitTests.zip -P $(TARGET_TESTROOT)'
	$(REMOTE_RUN) 'wget --progress=dot:mega $(STORE_HOST):$(HTTP_PORT)/$(STORE_PATH)/$(TEST_BRANCH)/win_desk/ccd/ccd.zip -P $(TARGET_TESTROOT)'
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); $(UNZIP) unitTests.zip'
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); $(UNZIP) ccd.zip'
ifndef USE_ARCHIVED_BUILD
ifneq ($(PRODUCT), win32_ul)    # PRODUCT=win32_ul does not support building of dxshell binary 
	echo "sync test files to $(TARGET_TESTROOT)"
	rsync -p $(BUILDROOT)/build_msvc/PersonalCloudAll/Release/CCD/Win32/CCD$(EXE) $(TARGET_USER)@$(TARGET_MACHINE):$(TARGET_TESTROOT)
	rsync -p $(BUILDROOT)/build_msvc/PersonalCloudAll/Release/ccd_ready_test/Win32/ccd_ready_test$(EXE) $(TARGET_USER)@$(TARGET_MACHINE):$(TARGET_TESTROOT)
	rsync -p $(BUILDROOT)/build_msvc/PersonalCloudAll/Release/ccd_shutdown_test/Win32/ccd_shutdown_test$(EXE) $(TARGET_USER)@$(TARGET_MACHINE):$(TARGET_TESTROOT)
	rsync -p $(BUILDROOT)/build_msvc/PersonalCloudAll/Release/test_ccd_start/Win32/test_ccd_start$(EXE) $(TARGET_USER)@$(TARGET_MACHINE):$(TARGET_TESTROOT)
else
	echo ">>>> WARNING : PRODUCT=win32_ul does not build test binaries. Force to use archived build"
endif
endif
	export SRCROOT=$(SRC_SWX) ; $(SRC_SWX)/tests/tools/actool.sh $(LAB_DOMAIN_NAME) `pwd` &> $(LOGDIR)/actool.log
	-$(REMOTE_RUN) 'mkdir -p /cygdrive/c/Users/$(TARGET_USER)/igware/conf'
	rsync -p conf/ccd.conf  $(TARGET_USER)@$(TARGET_MACHINE):/cygdrive/c/Users/$(TARGET_USER)/igware/conf/ccd.conf
	-rm -rf conf
	-$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); ./test_ccd_start$(EXE) C:/Users/$(TARGET_USER)/igware; sleep 3' &> $(LOGDIR)/test_ccd_start.log

setup_winrt:
	rm -f $(LOGDIR)/*
	mkdir -p $(LOGDIR)/test_ccd_single_sign_on/login_with_password
	mkdir -p $(LOGDIR)/test_ccd_single_sign_on/login_with_credential
	mkdir -p $(LOGDIR)/test_ccd
	-$(REMOTE_RUN) 'rm -Rf $(TARGET_TESTROOT)/*'
	-$(REMOTE_RUN) 'rm -Rf /cygdrive/c/Users/$(TARGET_USER)/Music/AcerCloud'

	# Pull down ccd test app and required utility 
	echo "sync ccd test metro app to $(TARGET_TESTROOT)/test_ccd"
	-$(REMOTE_RUN) 'mkdir -p $(TARGET_TESTROOT)/test_ccd'
	$(REMOTE_RUN) 'wget --progress=dot:mega $(STORE_HOST):$(HTTP_PORT)/$(STORE_PATH)/$(TEST_BRANCH)/winrt/test_ccd/test_ccd.zip -P $(TARGET_TESTROOT)/test_ccd'
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT)/test_ccd; unzip test_ccd.zip'

	# Pull down ccd SSO test app and required utility 
	echo "sync ccd test metro app to $(TARGET_TESTROOT)/test_ccd_single_sign_on"
	-$(REMOTE_RUN) 'mkdir -p $(TARGET_TESTROOT)/test_ccd_single_sign_on'
	$(REMOTE_RUN) 'wget --progress=dot:mega $(STORE_HOST):$(HTTP_PORT)/$(STORE_PATH)/$(TEST_BRANCH)/winrt/test_ccd_single_sign_on/test_ccd_single_sign_on.zip -P $(TARGET_TESTROOT)/test_ccd_single_sign_on'
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT)/test_ccd_single_sign_on; unzip test_ccd_single_sign_on.zip'

	# Remove elder metro app, and install the latest one
	echo "Remove test_ccd app"
	-$(REMOTE_RUN) 'cd $(TARGET_TESTROOT)/test_ccd; ./metro_app_utilities.exe remove f79747ca-8f1e-4a5f-92cc-a0aa94926cf1_1.0.0.0_x86__cb1hhkscw5m06'
	-$(REMOTE_RUN) 'cd $(TARGET_TESTROOT)/test_ccd/test_ccd_1.0.0.0_Win32_Test; /cygdrive/c/Windows/system32/certutil.exe -addstore root test_ccd_1.0.0.0_Win32.cer'
	-$(REMOTE_RUN) 'cd $(TARGET_TESTROOT)/test_ccd; ./metro_app_utilities.exe add $(TARGET_APPXURL_ROOT)/test_ccd/test_ccd_1.0.0.0_Win32_Test/Dependencies/x86/Microsoft.VCLibs.x86.11.00.appx'
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT)/test_ccd; ./metro_app_utilities.exe add $(TARGET_APPXURL_ROOT)/test_ccd/test_ccd_1.0.0.0_Win32_Test/test_ccd_1.0.0.0_Win32.appx'

	echo "Remove test_ccd_single_sign_on app"
	-$(REMOTE_RUN) 'cd $(TARGET_TESTROOT)/test_ccd_single_sign_on; ./metro_app_utilities.exe remove b2d9ebbc-d8f5-4617-9134-6bb731d9d5e1_1.0.0.0_x86__cb1hhkscw5m06'
	-$(REMOTE_RUN) 'cd $(TARGET_TESTROOT)/test_ccd_single_sign_on/test_ccd_single_sign_on_1.0.0.0_Win32_Test; /cygdrive/c/Windows/system32/certutil.exe -addstore root test_ccd_single_sign_on_1.0.0.0_Win32.cer'
	-$(REMOTE_RUN) 'cd $(TARGET_TESTROOT)/test_ccd_single_sign_on; ./metro_app_utilities.exe add $(TARGET_APPXURL_ROOT)/test_ccd_single_sign_on/test_ccd_single_sign_on_1.0.0.0_Win32_Test/Dependencies/x86/Microsoft.VCLibs.x86.11.00.appx'
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT)/test_ccd_single_sign_on; ./metro_app_utilities.exe add $(TARGET_APPXURL_ROOT)/test_ccd_single_sign_on/test_ccd_single_sign_on_1.0.0.0_Win32_Test/test_ccd_single_sign_on_1.0.0.0_Win32.appx'

	export SRCROOT=$(SRC_SWX) ; $(SRC_SWX)/tests/tools/actool.sh $(LAB_DOMAIN_NAME) `pwd` &> $(LOGDIR)/actool.log
	rsync $(SRC_SWX)/tests/test_utils/fileWatcher.py $(TARGET_USER)@$(TARGET_MACHINE):$(TARGET_TESTROOT)/test_ccd_single_sign_on
	rsync $(SRC_SWX)/tests/test_utils/fileWatcher.py $(TARGET_USER)@$(TARGET_MACHINE):$(TARGET_TESTROOT)/test_ccd
	-$(REMOTE_RUN) 'mkdir -p /cygdrive/c/Users/$(TARGET_USER)/Music/AcerCloud/conf'
	rsync -p conf/ccd.conf $(TARGET_USER)@$(TARGET_MACHINE):/cygdrive/c/Users/$(TARGET_USER)/Music/AcerCloud/conf/ccd.conf
	-$(REMOTE_RUN) 'echo -n -e "$(CCD_TEST_ACCOUNT)\npassword" > /cygdrive/c/Users/$(TARGET_USER)/Music/AcerCloud/LoginInfo'
	-rm -rf conf

setup_linux: clean
ifeq ($(CONFIG_ES),km)
	$(MAKE) install_escmod
endif
	-rm -fr $(HOME)/"My Cloud"/*
	mkdir -p $(LOGDIR)
	rm -f $(LOGDIR)/*

setup_ios:
	mkdir -p $(IOS_LOG_DIR)
	chmod 777 $(IOS_LOG_DIR)
	echo -n "$(IOS_LOG_RECEIVER_URL)" > $(TARGET_TESTROOT)/ccd/SanityTest/LogReceiveURL

runtests_win32: 
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); ./ccd_ready_test.exe' &> $(LOGDIR)/1b_wait_for_ccd.log
	-rsync -r $(TARGET_USER)@$(TARGET_MACHINE):/cygdrive/C/Users/igware/igware/logs $(LOGDIR)

runtests_winrt:	
	echo "Launch ccd SSO test: login with password"
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT)/test_ccd_single_sign_on; ./metro_app_utilities.exe launch b2d9ebbc-d8f5-4617-9134-6bb731d9d5e1_cb1hhkscw5m06!App'
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT)/test_ccd_single_sign_on; /usr/bin/python ./fileWatcher.py --filename=test_done --dstdir=/cygdrive/c/Users/$(TARGET_USER)/Music/AcerCloud --int=10 --to=120'
	-rsync $(TARGET_USER)@$(TARGET_MACHINE):/cygdrive/c/Users/$(TARGET_USER)/Music/AcerCloud/test_done $(LOGDIR)/test_ccd_single_sign_on/login_with_password
	echo "Collecting results"
	rsync $(TARGET_USER)@$(TARGET_MACHINE):/cygdrive/c/Users/$(TARGET_USER)/AppData/Local/Packages/b2d9ebbc-d8f5-4617-9134-6bb731d9d5e1_cb1hhkscw5m06/LocalState/test_ccd_single_sign_on.log $(LOGDIR)/test_ccd_single_sign_on/login_with_password
	-rsync $(TARGET_USER)@$(TARGET_MACHINE):/cygdrive/c/Users/$(TARGET_USER)/AppData/Local/Packages/b2d9ebbc-d8f5-4617-9134-6bb731d9d5e1_cb1hhkscw5m06/LocalState/logs/testCCDSingleSignOn/* $(LOGDIR)/test_ccd_single_sign_on/login_with_password
	-rsync $(TARGET_USER)@$(TARGET_MACHINE):/cygdrive/c/Users/$(TARGET_USER)/AppData/Local/Packages/b2d9ebbc-d8f5-4617-9134-6bb731d9d5e1_cb1hhkscw5m06/LocalState/logs/testCCDSingleSignOn/special_logs/* $(LOGDIR)/test_ccd_single_sign_on/login_with_password
	cat $(LOGDIR)/test_ccd_single_sign_on/login_with_password/test_ccd_single_sign_on.log
	@if [ ! -e $(LOGDIR)/test_ccd_single_sign_on/login_with_password/test_done ]; then \
	  echo "TC_RESULT = FAIL ;;; TC_NAME = runtests_test_ccd_single_sign_on_login_with_password"; \
	elif [ `grep 'Summary: FAIL' $(LOGDIR)/test_ccd_single_sign_on/login_with_password/test_ccd_single_sign_on.log | wc -l` -gt 0 ]; then \
	  echo "TC_RESULT = FAIL ;;; TC_NAME = runtests_test_ccd_single_sign_on_login_with_password"; \
	else \
	  echo "TC_RESULT = PASS ;;; TC_NAME = runtests_test_ccd_single_sign_on_login_with_password"; \
	fi
	
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT)/test_ccd_single_sign_on; ./metro_app_utilities.exe remove b2d9ebbc-d8f5-4617-9134-6bb731d9d5e1_1.0.0.0_x86__cb1hhkscw5m06'
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT)/test_ccd_single_sign_on; ./metro_app_utilities.exe add $(TARGET_APPXURL_ROOT)/test_ccd_single_sign_on/test_ccd_single_sign_on_1.0.0.0_Win32_Test/test_ccd_single_sign_on_1.0.0.0_Win32.appx'
	$(REMOTE_RUN) 'rm -f /cygdrive/c/Users/$(TARGET_USER)/Music/AcerCloud/test_done'
	
	echo "Launch ccd SSO test: login with credential"
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT)/test_ccd_single_sign_on; ./metro_app_utilities.exe launch b2d9ebbc-d8f5-4617-9134-6bb731d9d5e1_cb1hhkscw5m06!App'
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT)/test_ccd_single_sign_on; /usr/bin/python ./fileWatcher.py --filename=test_done --dstdir=/cygdrive/c/Users/$(TARGET_USER)/Music/AcerCloud --int=10 --to=120'
	-rsync $(TARGET_USER)@$(TARGET_MACHINE):/cygdrive/c/Users/$(TARGET_USER)/Music/AcerCloud/test_done $(LOGDIR)/test_ccd_single_sign_on/login_with_credential
	$(REMOTE_RUN) 'rm -f /cygdrive/c/Users/$(TARGET_USER)/Music/AcerCloud/test_done'
	$(REMOTE_RUN) 'rm -rf /cygdrive/c/Users/$(TARGET_USER)/Music/AcerCloud/cred'
	echo "Collecting results"
	rsync $(TARGET_USER)@$(TARGET_MACHINE):/cygdrive/c/Users/$(TARGET_USER)/AppData/Local/Packages/b2d9ebbc-d8f5-4617-9134-6bb731d9d5e1_cb1hhkscw5m06/LocalState/test_ccd_single_sign_on.log $(LOGDIR)/test_ccd_single_sign_on/login_with_credential
	-rsync $(TARGET_USER)@$(TARGET_MACHINE):/cygdrive/c/Users/$(TARGET_USER)/AppData/Local/Packages/b2d9ebbc-d8f5-4617-9134-6bb731d9d5e1_cb1hhkscw5m06/LocalState/logs/testCCDSingleSignOn/* $(LOGDIR)/test_ccd_single_sign_on/login_with_credential
	-rsync $(TARGET_USER)@$(TARGET_MACHINE):/cygdrive/c/Users/$(TARGET_USER)/AppData/Local/Packages/b2d9ebbc-d8f5-4617-9134-6bb731d9d5e1_cb1hhkscw5m06/LocalState/logs/testCCDSingleSignOn/special_logs/* $(LOGDIR)/test_ccd_single_sign_on/login_with_credential
	cat $(LOGDIR)/test_ccd_single_sign_on/login_with_credential/test_ccd_single_sign_on.log
	@if [ ! -e $(LOGDIR)/test_ccd_single_sign_on/login_with_credential/test_done ]; then \
	  echo "TC_RESULT = FAIL ;;; TC_NAME = runtests_test_ccd_single_sign_on_login_with_credential"; \
	elif [ `grep 'Summary: FAIL' $(LOGDIR)/test_ccd_single_sign_on/login_with_credential/test_ccd_single_sign_on.log | wc -l` -gt 0 ]; then \
	  echo "TC_RESULT = FAIL ;;; TC_NAME = runtests_test_ccd_single_sign_on_login_with_credential"; \
	else \
	  echo "TC_RESULT = PASS ;;; TC_NAME = runtests_test_ccd_single_sign_on_login_with_credential"; \
	fi

.PHONY: runtests_linux_helper
runtests_linux_helper:
	export SRCROOT=$(SRC_SWX) ; $(SRC_SWX)/tests/tools/actool.sh $(LAB_DOMAIN_NAME) $(CCD_APPDATA) &> $(LOGDIR)/1_actool.log
	sleep 1
	$(CAPTURE_CORE) && $(VALGRIND) $(CCD) $(CCD_APPDATA) &> $(LOGDIR)/2_bg_ccd.log &
	$(CAPTURE_CORE) && $(WAIT_FOR_CCD) &> $(LOGDIR)/3_wait_for_ccd.log # Allow ccd to start up
	$(CAPTURE_CORE) && $(SHUTDOWN_CCD) &> $(LOGDIR)/5_shutdown_ccd.log
	touch $(LOGDIR)/6_done.timestamp
	sleep 10 # Give CCD a chance to exit on its own before we forcefully kill it (with CCD_FULL_SHUTDOWN enabled, it will take *at least* 5 seconds).

runtests_linux: setup_linux
	killall $(CCD) 2>&1 || true
	-$(MAKE) runtests_linux_helper
	@echo '------------'
	@if [ -e $(LOGDIR)/6_done.timestamp ]; then \
	  echo "TC_RESULT = PASS ;;; TC_NAME = runtests_clean_exit"; \
	else \
	  echo "TC_RESULT = FAIL ;;; TC_NAME = runtests_clean_exit"; \
	fi
ifneq ($(USE_VALGRIND), 0)
	-grep "ERROR SUMMARY" $(VALGRIND_LOGFILE)
	@if [[ -n `grep "ERROR SUMMARY: 0 errors from 0 contexts" $(VALGRIND_LOGFILE)` ]]; then \
	  echo "TC_RESULT = PASS ;;; TC_NAME = runtests_valgrind_compliant"; \
	else \
	  echo "TC_RESULT = EXPECTED_TO_FAIL ;;; TC_NAME = runtests_valgrind_compliant"; \
	fi
endif

runtests_ios:
	export SRCROOT=$(SRC_SWX) ; $(SRC_SWX)/tests/tools/actool.sh $(LAB_DOMAIN_NAME) $(SRC_SWX)/projects/xcode/PersonalCloud/ccd/SanityTest
	echo -n "$(CCD_TEST_ACCOUNT)" > $(SRC_SWX)/projects/xcode/PersonalCloud/ccd/SanityTest/CCDTestAccount
	echo -n "password" > $(SRC_SWX)/projects/xcode/PersonalCloud/ccd/SanityTest/CCDTestAccountPassword
	security unlock-keychain -p notsosecret $(HOME)/Library/Keychains/act-mve.keychain
	osascript $(TARGET_SCRIPTROOT)/RunProject.scpt $(TARGET_TESTROOT)/ccd/ccd.xcodeproj SSOLoginTest $(IOS_LOG_DIR)/$(TEST_BINARY_2).log
	osascript $(TARGET_SCRIPTROOT)/CloseProject.scpt $(TARGET_TESTROOT) ccd ccd
	cat $(IOS_LOG_DIR)/$(TEST_BINARY_2).log
	@if [ `grep '^CLEAN EXIT' $(IOS_LOG_DIR)/$(TEST_BINARY_2).log | wc -l` -gt 0 ]; then \
	  echo "$(TEST_BINARY_2) exited cleanly."; \
	  echo "TC_RESULT = PASS ;;; TC_NAME = $(TEST_BINARY_2)_clean_exit"; \
	else \
	  echo "*** $(TEST_BINARY_2) may have crashed! (Check $(TESTDIR) for a core file.)" | grep '.*'; \
	  echo "TC_RESULT = FAIL ;;; TC_NAME = $(TEST_BINARY_2)_clean_exit"; \
	fi
	security unlock-keychain -p notsosecret $(HOME)/Library/Keychains/act-mve.keychain
	osascript $(TARGET_SCRIPTROOT)/RunProject.scpt $(TARGET_TESTROOT)/ccd/ccd.xcodeproj SSOLoginWithSessionTest $(IOS_LOG_DIR)/$(TEST_BINARY_3).log
	osascript $(TARGET_SCRIPTROOT)/CloseProject.scpt $(TARGET_TESTROOT) ccd ccd
	cat $(IOS_LOG_DIR)/$(TEST_BINARY_3).log
	@if [ `grep '^CLEAN EXIT' $(IOS_LOG_DIR)/$(TEST_BINARY_3).log | wc -l` -gt 0 ]; then \
	  echo "$(TEST_BINARY_3) exited cleanly."; \
	  echo "TC_RESULT = PASS ;;; TC_NAME = $(TEST_BINARY_3)_clean_exit"; \
	else \
	  echo "*** $(TEST_BINARY_3) may have crashed! (Check $(TESTDIR) for a core file.)" | grep '.*'; \
	  echo "TC_RESULT = FAIL ;;; TC_NAME = $(TEST_BINARY_3)_clean_exit"; \
	fi
	security unlock-keychain -p notsosecret $(HOME)/Library/Keychains/act-mve.keychain
	osascript $(TARGET_SCRIPTROOT)/RunProject.scpt $(TARGET_TESTROOT)/ccd/ccd.xcodeproj SSOLoginWithCleanAppTest $(IOS_LOG_DIR)/$(TEST_BINARY_4).log
	osascript $(TARGET_SCRIPTROOT)/CloseProject.scpt $(TARGET_TESTROOT) ccd ccd
	cat $(IOS_LOG_DIR)/$(TEST_BINARY_4).log
	@if [ `grep '^CLEAN EXIT' $(IOS_LOG_DIR)/$(TEST_BINARY_4).log | wc -l` -gt 0 ]; then \
	  echo "$(TEST_BINARY_4) exited cleanly."; \
	  echo "TC_RESULT = PASS ;;; TC_NAME = $(TEST_BINARY_4)_clean_exit"; \
	else \
	  echo "*** $(TEST_BINARY_4) may have crashed! (Check $(TESTDIR) for a core file.)" | grep '.*'; \
	  echo "TC_RESULT = FAIL ;;; TC_NAME = $(TEST_BINARY_4)_clean_exit"; \
	fi

cleanup_win32: 
	-$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); ccd_shutdown_test.exe &> 3_shutdown_ccd.log'
	-rsync $(TARGET_USER)@$(TARGET_MACHINE):$(TARGET_TESTROOT)/3_shutdown_ccd.log $(LOGDIR)
	-$(REMOTE_RUN) 'taskkill /F /IM ccd.exe'
	#-$(REMOTE_RUN) 'rm -Rf $(TARGET_TESTROOT)/*'

cleanup_winrt:
	-$(REMOTE_RUN) 'cd $(TARGET_TESTROOT)/test_ccd_single_sign_on; ./metro_app_utilities.exe remove b2d9ebbc-d8f5-4617-9134-6bb731d9d5e1_1.0.0.0_x86__cb1hhkscw5m06'
	-$(REMOTE_RUN) 'cd $(TARGET_TESTROOT)/test_ccd; ./metro_app_utilities.exe remove f79747ca-8f1e-4a5f-92cc-a0aa94926cf1_1.0.0.0_x86__cb1hhkscw5m06'
	-$(REMOTE_RUN) 'rm -Rf $(TARGET_TESTROOT)/*'
	-$(REMOTE_RUN) 'rm -Rf /cygdrive/c/Users/$(TARGET_USER)/Music/AcerCloud'

cleanup_linux:
# Cleanup any other instances of CCD, whether they used valgrind or not.
	killall ccd 2>&1 || true
	ps -o pid,cmd | grep valgrind | grep -v grep| cut -c1-6 | $(XARGS_CMDLINE) kill

cleanup_ios:

post_suite_timeout_ios:
	echo "============= $(TEST_BINARY_2).log ================"
	-cat $(IOS_LOG_DIR)/$(TEST_BINARY_2).log
	echo "============= $(TEST_BINARY_3).log ================"
	-cat $(IOS_LOG_DIR)/$(TEST_BINARY_3).log
	echo "============= $(TEST_BINARY_4).log ================"
	-cat $(IOS_LOG_DIR)/$(TEST_BINARY_4).log

post_suite_timeout_winrt:
	-rsync $(TARGET_USER)@$(TARGET_MACHINE):/cygdrive/c/Users/$(TARGET_USER)/Music/AcerCloud/test_ccd_single_sign_on.log $(LOGDIR)
	-rsync $(TARGET_USER)@$(TARGET_MACHINE):/cygdrive/c/Users/$(TARGET_USER)/Music/AcerCloud/testCCDSingleSignOn.*.log $(LOGDIR)
	-rsync $(TARGET_USER)@$(TARGET_MACHINE):/cygdrive/c/Users/$(TARGET_USER)/Music/AcerCloud/test_ccd.log $(LOGDIR)
	-rsync $(TARGET_USER)@$(TARGET_MACHINE):/cygdrive/c/Users/$(TARGET_USER)/Music/AcerCloud/ccd_*.log $(LOGDIR)

post_suite_timeout_win32:
	-rsync -r $(TARGET_USER)@$(TARGET_MACHINE):/cygdrive/C/Users/igware/igware/logs $(LOGDIR)

post_suite_timeout_linux:

sinclude ../make/$(PRODUCT).makerules


ifdef DEVELOP_BUILD_DEPS
setup runtests: builddeps

.PHONY: builddeps
builddeps:
	cd $(SRC_SWX) &&\
	  $(MAKE) ccd \
	    gvm_core/daemons/ccd/tests/ccd_ready_test \
	    gvm_core/daemons/ccd/tests/ccd_shutdown_test
endif
