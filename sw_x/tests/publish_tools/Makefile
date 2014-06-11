override MY_TEST_NAME=publish_tools

# Publishing tool closed loop test running on Cygwin
# To run this from a linux machine 
# set TARGET_MACHINE=<windows machine> PRODUCT=win32_ul and run make

ifeq ($(shell uname -o),Cygwin)
export PRODUCT ?= win32_ul
override USE_VALGRIND = 0
else
export PRODUCT ?= linux
export USE_VALGRIND ?= 1
endif

ifdef TEST_WORKDIR
export TITLEID ?= 00000006FF00000A
export GUID ?= ts-build-test-02
export LARGE_TITLEID ?= 00000006FF000008
export LARGE_GUID ?= test-dxwin-swu-160gb
else
export TITLEID ?= 00000006FF00000B
export GUID ?= ts-dev-test-01
export LARGE_TITLEID ?= 00000006FF00000C
export LARGE_GUID ?= ts-dev-test-02
endif

# makefile included by all test makefiles
include ../make/test_env.mk
sinclude ../make/$(PRODUCT).makedefs

export TARGET_MACHINE ?= localhost
export TARGET_USER ?= $(USER)
export CCD_TEST_ACCOUNT ?= swt-$(shell uname -n)@igware.com
export IDT_TOOLS = $(SRC_SWI)/tools/idt

USE_DXTOOL ?= 0


ifdef TH_RUN_FULL
FULL_FLAGS?=-x
else
FULL_FLAGS?=
endif

#
# Override this to test other versions of the publishing tools.
PUBTOOLS_URL ?= $(STORE_HOST):$(HTTP_PORT)/$(STORE_PATH)/$(TEST_BRANCH)/infra/publish_tools/publish_tools.zip 


# derived
override CCD=$(TESTROOT)/gvm_core/daemons/ccd/ccd

ifeq ($(PRODUCT), win32_ul)
override TESTPLAT=win32
override CCDPATH=$(BUILDROOT)/build_msvc/PersonalCloudAll/Release/CCD/Win32
override DXSHELL_PATH=$(BUILDROOT)/build_msvc/PersonalCloudAll/Release/dxshell/Win32
override DXTOOL_PATH=$(BUILDROOT)/build_msvc/PersonalCloudAll/Release/dx_win/Win32
override TARGET_TESTROOT=/cygdrive/c/tmp/igware/testroot
override TARGET_TESTROOT_WIN_CCD=C:/tmp/igware/testroot/ccd.exe
else
ifeq ($(PRODUCT), msvc)
override TESTPLAT=win32
override CCDPATH=$(BUILDROOT)/build_msvc/PersonalCloudAll/Release/CCD/Win32
override DXSHELL_PATH=$(BUILDROOT)/build_msvc/PersonalCloudAll/Release/dxshell/Win32
override DXTOOL_PATH=$(BUILDROOT)/build_msvc/PersonalCloudAll/Release/dx_win/Win32
override TARGET_TESTROOT=/cygdrive/c/tmp/igware/testroot
override TARGET_TESTROOT_WIN_CCD=C:/tmp/igware/testroot/ccd.exe
else
override TESTPLAT=linux
override TARGET_TESTROOT=/tmp/igware/testroot
endif
endif

REMOTE_RUN=/usr/bin/ssh $(TARGET_USER)@$(TARGET_MACHINE)

CLIENT_CCD_NUM=0
CLIENT_CCD=/cygdrive/c/tmp/igware/testclient


.PHONY: default setup cleanup runtests
default: verify_dir setup runtests cleanup

verify_dir:
ifeq ($(strip $(wildcard $(SRC_SWX)/tests/publish_tools/Makefile)),)
	$(error "can't locate sw_x/tests/publish_tools/Makefile - current directory is $(shell pwd)")
endif

setup: verify_dir
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
	-mkdir -p $(LOGDIR)
	$(MAKE) setup_$(TESTPLAT)

runtests: setup
	$(MAKE) runtests_$(TESTPLAT)

cleanup:
	$(MAKE) cleanup_$(TESTPLAT)
	-cd $(IDT_TOOLS) && \
		./deleteUser -i ids.$(INTERNAL_LAB_DOMAIN_NAME) -u $(CCD_TEST_ACCOUNT)

# completely remove ccd state
clean:
	rm -fr $(CLIENT_CCD)/*
	rm -fr $(HOME)/"My Cloud"/*

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
	rm -rf $(LOGDIR)/*
	-$(REMOTE_RUN) 'taskkill /F /IM ccd.exe'
	-$(REMOTE_RUN) 'rm -Rf $(TARGET_TESTROOT)/*'
	-$(REMOTE_RUN) 'mkdir -p $(TARGET_TESTROOT)'
	-$(REMOTE_RUN) 'rm -Rf $(CLIENT_CCD)/*'
	-$(REMOTE_RUN) 'mkdir -p $(CLIENT_CCD)/client_cloud_root'
	$(REMOTE_RUN) 'wget --progress=dot:mega $(PUBTOOLS_URL) -P $(TARGET_TESTROOT)'
	$(REMOTE_RUN) 'unzip -q -d $(TARGET_TESTROOT) $(TARGET_TESTROOT)/publish_tools.zip'
	$(REMOTE_RUN) 'md5sum $(TARGET_TESTROOT)/publish_tools.zip' 
	rsync -p . --files-from=pubtools_test_file_list.txt $(TARGET_USER)@$(TARGET_MACHINE):$(TARGET_TESTROOT)
ifdef USE_ARCHIVED_BUILD
	$(REMOTE_RUN) 'wget --progress=dot:mega $(STORE_HOST):$(HTTP_PORT)/$(STORE_PATH)/$(TEST_BRANCH)/win_desk/ccd/ccd.zip -P $(TARGET_TESTROOT)'
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); unzip ccd.zip; md5sum ccd.zip'
ifeq ($(USE_DXTOOL),1)
	$(REMOTE_RUN) 'wget --progress=dot:mega $(STORE_HOST):$(HTTP_PORT)/$(STORE_PATH)/$(TEST_BRANCH)/win_desk/DXTools/DXTools.zip -P $(TARGET_TESTROOT)'
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); unzip DXTools.zip; md5sum DXtools.zip'
else
	$(REMOTE_RUN) 'wget --progress=dot:mega $(STORE_HOST):$(HTTP_PORT)/$(STORE_PATH)/$(TEST_BRANCH)/win_desk/dxshell/dxshell.zip -P $(TARGET_TESTROOT)'
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); unzip -o dxshell.zip; md5sum dxshell.zip'
endif
else
	echo "sync test files to $(TARGET_TESTROOT)"
	rsync -p $(CCDPATH) --files-from=../dxtool/ccd_test_file_list_msvc.txt $(TARGET_USER)@$(TARGET_MACHINE):$(TARGET_TESTROOT)
ifeq ($(USE_DXTOOL),1)
	rsync -p $(DXTOOL_PATH) --files-from=../dxtool/dxtool_test_file_list_msvc.txt $(TARGET_USER)@$(TARGET_MACHINE):$(TARGET_TESTROOT)
	export SRCROOT=$(SRC_SWX) ; $(SRC_SWX)/tests/tools/actool.sh $(LAB_DOMAIN_NAME) `pwd` &> $(LOGDIR)/actool.log
	cp conf/ccd.conf conf/ccd_client.conf
	echo "testInstanceNum = $(CLIENT_CCD_NUM)" >> conf/ccd_client.conf
	-$(REMOTE_RUN) 'mkdir -p $(CLIENT_CCD)/conf'
	rsync -p conf/ccd_client.conf  $(TARGET_USER)@$(TARGET_MACHINE):$(CLIENT_CCD)/conf/ccd.conf
	-rm -rf conf
else
	echo "BARBAR"
	rsync ../../gvm_core/conf/ccd.conf.tmpl $(TARGET_USER)@$(TARGET_MACHINE):$(TARGET_TESTROOT)
	rsync -p $(DXSHELL_PATH)/dxshell.exe $(TARGET_USER)@$(TARGET_MACHINE):$(TARGET_TESTROOT)
endif
endif

setup_linux:
	$(error "Linux not supported for dxtool")

runtests_win32:
	-$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); ./pubtests.sh -u $(CCD_TEST_ACCOUNT) -d $(TARGET_TESTROOT_WIN_CCD) -f $(CLIENT_CCD) -l $(LAB_DOMAIN_NAME) -t $(TITLEID) -g $(GUID) -T $(LARGE_TITLEID) -G $(LARGE_GUID) $(FULL_FLAGS)' >> $(LOGDIR)/pubtests.log 2>&1
	-scp $(TARGET_USER)@$(TARGET_MACHINE):$(TARGET_TESTROOT)/ccd/ccd.*.log $(LOGDIR)
	-grep "TC_"RESULT=  $(LOGDIR)/pubtests.log || ( echo "TC_""RESULT=FAIL ;;;TC_NAME=PubToolSetup" && false )

.PHONY: runtests_linux_helper
runtests_linux_helper:
	$(error "Linux not supported for dxtool")

runtests_linux: 
	$(error "Linux not supported for dxtool")

cleanup_win32: 
	-$(REMOTE_RUN) 'taskkill /F /IM dxshell.exe'
	-$(REMOTE_RUN) 'taskkill /F /IM ccd.exe'
	-$(REMOTE_RUN) 'rm -Rf $(TARGET_TESTROOT)/*'

cleanup_linux: 
	$(error "Linux not supported for publisH_tools (yet)")

sinclude ../make/$(PRODUCT).makerules

