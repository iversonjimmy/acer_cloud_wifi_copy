export UNZIP=unzip
export EXE=.exe
export KILL=taskkill /F /IM
export WGET_OPTIONS=--progress=dot:mega

export DXSHELL_PACKAGE=dxshell.zip
export DXSHELL_PACKAGE_PATH=$(STORE_PATH)/$(TEST_BRANCH)/win_desk/dxshell
export DXSHELL_BINARY_NAME=dxshell
export DXSHELL_BINARY_PATH=$(BUILDROOT)/build_msvc/PersonalCloudAll/Release/dxshell/Win32
export CCD_BINARY_NAME=ccd
export CCD_BINARY_PATH=$(BUILDROOT)/build_msvc/PersonalCloudAll/Release/CCD/Win32
export DXREMOTEAGENT_BINARY_NAME=dx_remote_agent
export DXREMOTEAGENT_BINARY_PATH=$(BUILDROOT)/build_msvc/PersonalCloudAll/Release/dx_remote_agent/Win32

export TARGET_MACHINE ?= localhost
export TEST_MASTER_MACHINE ?= localhost
export TARGET_USER ?= build
export CCD_TEST_ACCOUNT ?= defaultTester@igware.com
export IDT_TOOLS = $(SRC_SWI)/tools/idt

export TARGET_TESTROOT=/cygdrive/c/temp/igware/testroot
export TARGET_DXREMOTE=/cygdrive/c/cygwin/home/$(TARGET_USER)/dx_remote_agent
export WIN_DESK_CORE_DUMP_PATH=/cygdrive/c/Users/$(TARGET_USER)/AppData/Local/CrashDumps
export WIN_DESK_CORE_DUMP_PATH_FOR_SYSTEM_USER=/cygdrive/c/Windows/SysWOW64/config/systemprofile/AppData/Local/CrashDumps
export WIN_DESK_CORE_DUMP_LOC=WinDesk_CrashDump
export WIN_DESK_CCDMSRV_PATH=/cygdrive/c/ProgramData/Acer/CCDMSrv
export WIN_DESK_CCDMSRV_LOG=CCDMSrvLog
export DEFAULT_CCD_APPDATA_PATH=/cygdrive/c/Users/$(TARGET_USER)/AppData/Local/iGware/SyncAgent
export TARGET_LOG=$(TARGET_USER)@$(TARGET_MACHINE)
export REMOTE_RUN=/usr/bin/ssh $(TARGET_USER)@$(TARGET_MACHINE)

export ARCHIVED_TESTDATADIR="pc/archived_test_data/dxdata"
# Do not edit the archived data files; create new ones with updated datestamps and change their names here
export GOLDEN_TEST_DATA_DIR=/cygdrive/c/Users/$(TARGET_USER)/AppData/Local/dxshell/GoldenTest
export CURRENT_DOC_DATA_FILE=TestDocData.20140508.tar.gz
export GOLDEN_TEST_DATA_DOC_DIR=$(GOLDEN_TEST_DATA_DIR)/TestDocData
export GOLDEN_TEST_DATA_DOC_DEPLOYED=$(GOLDEN_TEST_DATA_DIR)/$(CURRENT_DOC_DATA_FILE).already_deployed
export CURRENT_PHOTO_DATA_FILE=TestPhotoData.20131220.tar.gz
export GOLDEN_TEST_DATA_PHOTO_DIR=$(GOLDEN_TEST_DATA_DIR)/TestPhotoData
export GOLDEN_TEST_DATA_PHOTO_DEPLOYED=$(GOLDEN_TEST_DATA_DIR)/$(CURRENT_PHOTO_DATA_FILE).already_deployed
export CURRENT_MUSIC_DATA_FILE=TestMusicData.20120319.tar.gz
export GOLDEN_TEST_DATA_MUSIC_DIR=$(GOLDEN_TEST_DATA_DIR)/TestMusicData
export GOLDEN_TEST_DATA_MUSIC_DEPLOYED=$(GOLDEN_TEST_DATA_DIR)/$(CURRENT_MUSIC_DATA_FILE).already_deployed

ifndef NO_CLOUDPC_DEVICE
export CLOUDPC_ALIAS ?= CloudPC
export CLOUDPC_USER ?= $(TARGET_USER)
export CLOUDPC_IP ?= 127.0.0.1
export CLOUDPC_NAME ?= $(TARGET_MACHINE)
export CLOUDPC_OS ?= Windows
export CLOUDPC_REMOTE_RUN=/usr/bin/ssh $(CLOUDPC_USER)@$(CLOUDPC_IP)
export TARGET_CLOUDPC_LOG=$(CLOUDPC_USER)@$(CLOUDPC_IP)
export CCD_APPDATA_PATH_CLOUDPC=/cygdrive/c/Users/$(TARGET_USER)/AppData/Local/iGware/SyncAgent_$(CLOUDPC_ALIAS)
endif

ifndef NO_MD_DEVICE
export MD_ALIAS ?= MD
export MD_USER ?= $(TARGET_USER)
export MD_IP ?= 127.0.0.1
export MD_NAME ?= $(TARGET_MACHINE)
export MD_OS ?= Windows
export MD_REMOTE_RUN=/usr/bin/ssh $(MD_USER)@$(MD_IP)
export TARGET_MD_LOG=$(MD_USER)@$(MD_IP)
export CCD_APPDATA_PATH_MD=/cygdrive/c/Users/$(TARGET_USER)/AppData/Local/iGware/SyncAgent_$(MD_ALIAS)
endif

ifndef NO_CLIENT_DEVICE
export CLIENT_ALIAS ?= Client
export CLIENT_USER ?= $(TARGET_USER)
export CLIENT_IP ?= 127.0.0.1
export CLIENT_NAME ?= $(TARGET_MACHINE)
export CLIENT_OS ?= Windows 
export CLIENT_REMOTE_RUN=/usr/bin/ssh $(CLIENT_USER)@$(CLIENT_IP)
export TARGET_CLIENT_LOG=$(CLIENT_USER)@$(CLIENT_IP)
export CCD_APPDATA_PATH_CLIENT=/cygdrive/c/Users/$(TARGET_USER)/AppData/Local/iGware/SyncAgent_$(CLIENT_ALIAS)
endif

export MAX_TOTAL_LOG_SIZE ?= 1073741824

# derived
PRODUCT_KNOWN=0

ifeq ($(PRODUCT), win32_ul)
PRODUCT_KNOWN=1
override TESTPLAT=win32
endif

ifeq ($(PRODUCT), msvc)
PRODUCT_KNOWN=1
override TESTPLAT=win32
endif

ifeq ($(PRODUCT), cloudnode)
PRODUCT_KNOWN=1
override TESTPLAT=cloudnode
override TEST_TARGET_IS_CLOUDNODE=1 
override TARGET_HOME=/home/$(TARGET_USER)
override TARGET_TESTROOT=$(TARGET_HOME)/temp/igware/testroot
override TARGET_DXREMOTE=$(TARGET_HOME)/dx_remote_agent
override GOLDEN_TEST_DATA_DIR=$(TARGET_HOME)/dxshell_root/GoldenTest
override DEFAULT_CCD_APPDATA_PATH=$(TARGET_HOME)/temp/SyncAgent
override CCD_APPDATA_PATH_CLOUDPC=$(TARGET_HOME)/temp/SyncAgent_CloudPC
override CCD_APPDATA_PATH_MD=$(TARGET_HOME)/temp/SyncAgent_MD
override CCD_APPDATA_PATH_CLIENT=$(TARGET_HOME)/temp/SyncAgent_Client
override KILL=killall
override DXSHELL_HOST_PACKAGE=dxshell.tar.gz
override DXSHELL_HOST_PACKAGE_PATH=$(STORE_PATH)/$(TEST_BRANCH)/linux/dxshell
override DXSHELL_PACKAGE=dxshell_cloudnode.tar.gz
override DXSHELL_PACKAGE_PATH=$(STORE_PATH)/$(TEST_BRANCH)/$(PRODUCT)/dxshell_cloudnode
override UNZIP=tar xzvf
override CCD_BINARY_PATH=$(BUILDROOT)/$(BUILDTYPE)/linux/gvm_core/daemons/ccd
override DXSHELL_BINARY_PATH=$(BUILDROOT)/$(BUILDTYPE)/linux/tests/dxshell
override DXREMOTEAGENT_BINARY_PATH=$(BUILDROOT)/$(BUILDTYPE)/linux/gvm_apps/dx_remote_agent/platform_linux
override EXE=
#  To use ccd, dxshell, and ccd.conf.tmpl from latest archive tar file; define:
#     USE_ARCHIVED_BUILD=1
#  To use debug version of cloudnode ccd from archive tar file define both:
#     USE_ARCHIVED_BUILD=1
#     CCD_ARCHIVE_BINARY_NAME_EXTENSION=.debug_build
#  To use locally built release version of cloudnode ccd and dxshell from BUILDROOT
#     don't define USE_ARCHIVED_BUILD
#  To use locally built debug version of cloudnode ccd from BUILDROOT
#     don't define USE_ARCHIVED_BUILD and define:
#        CCD_BUILDROOT_BINARY_BUILDTYPE=debug
#  To use locally built debug version of cloudnode dxshell from BUILDROOT
#     don't define USE_ARCHIVED_BUILD and define:
#        DXSHELL_BUILDROOT_BINARY_BUILDTYPE=debug
CCD_BUILDROOT_BINARY_BUILDTYPE ?= $(BUILDTYPE)
DXSHELL_BUILDROOT_BINARY_BUILDTYPE ?= $(BUILDTYPE)
CCD_BUILDROOT_BINARY_BUILDTYPE ?= $(BUILDTYPE)
DXSHELL_BUILDROOT_BINARY_BUILDTYPE ?= $(BUILDTYPE)
override CCD_BUILDROOT_BINARY=$(BUILDROOT)/$(CCD_BUILDROOT_BINARY_BUILDTYPE)/$(PRODUCT)/gvm_core/daemons/ccd/ccd_cloudnode
override CCD_ARCHIVE_BINARY_NAME=ccd_cloudnode$(CCD_ARCHIVE_BINARY_NAME_EXTENSION)
override DXSHELL_BUILDROOT_BINARY=$(BUILDROOT)/$(DXSHELL_BUILDROOT_BINARY_BUILDTYPE)/$(PRODUCT)/tests/dxshell/dxshell_cloudnode
override DXSHELL_ARCHIVE_BINARY_NAME=dxshell_cloudnode
override DXREMOTEAGENT_BUILDROOT_BINARY=$(BUILDROOT)/$(DXSHELL_BUILDROOT_BINARY_BUILDTYPE)/$(PRODUCT)/gvm_apps/dx_remote_agent/platform_linux/dx_remote_agent_cloudnode
override DXREMOTEAGENT_ARCHIVE_BINARY_NAME=dx_remote_agent_cloudnode
override WGET_OPTIONS=
override REMOTE_RUN_PRIVILEGED=/usr/bin/ssh root@$(TARGET_MACHINE)
override AS_TARGET_USER=su $(TARGET_USER) -c
override CLOUDPC_OS=Orbe
override MD_OS=Linux
override CLIENT_OS=Linux
endif

ifeq ($(PRODUCT), winrt)
PRODUCT_KNOWN=1
override TESTPLAT=winrt
override MD_USER=$(TARGET_WINRT_USER)
override MD_IP=$(TARGET_WIN32_MD)
override MD_OS=WindowsRT
override MD_NAME=$(TARGET_WINRT_MACHINE)
override CLOUDPC_IP=$(TARGET_WIN32_CLOUDPC)
override CLOUDPC_NAME=$(TEST_MASTER_MACHINE)
override CLIENT_IP=$(TARGET_WIN32_CLIENT)
override CLIENT_NAME=$(TEST_MASTER_MACHINE)
override TARGET_LOG=$(TARGET_USER)@$(TEST_MASTER_MACHINE)
override TARGET_MD_LOG=$(TARGET_WINRT_USER)@$(TARGET_WIN32_MD)
override REMOTE_RUN=/usr/bin/ssh $(TARGET_USER)@$(TEST_MASTER_MACHINE)
override MD_REMOTE_RUN=/usr/bin/ssh $(TARGET_WINRT_USER)@$(TARGET_WIN32_MD)
override TARGET_TESTROOT=/cygdrive/c/temp/igware/testroot
override CCD_APPDATA_PATH_MD=/cygdrive/c/Users/$(MD_USER)/AppData/Local/Packages/43d863b7-f317-4bf4-a669-9ba42a052c53_bzrwd8f9mzhby/LocalState/
override WIN_METRO_CORE_DUMP_PATH=/cygdrive/c/Users/$(MD_USER)/AppData/Local/CrashDumps
override USE_MOBILE_DEVICE=1
endif

ifeq ($(PRODUCT), ios)
PRODUCT_KNOWN=1
override TESTPLAT=ios
override MD_USER=$(TARGET_MAC_USER)
override MD_IP=$(TARGET_WIN32_MD)
override MD_OS=iOS
override MD_NAME=$(TARGET_MAC_MINI)
override TARGET_MOBILE_DEVICE=$(TARGET_IOS_DEVICE)
override CLOUDPC_IP=$(TARGET_WIN32_CLOUDPC)
override CLOUDPC_NAME=$(TEST_MASTER_MACHINE)
override CLIENT_IP=$(TARGET_WIN32_CLIENT)
override CLIENT_NAME=$(TEST_MASTER_MACHINE)
override REMOTE_RUN=/usr/bin/ssh $(TARGET_USER)@$(TEST_MASTER_MACHINE)
override MD_REMOTE_RUN=/usr/bin/ssh $(TARGET_MAC_USER)@$(TARGET_WIN32_MD)
override TARGET_LOG=$(TARGET_USER)@$(TEST_MASTER_MACHINE)
override TARGET_MD_LOG=$(TARGET_MAC_USER)@$(TARGET_WIN32_MD)
override TARGET_IOS_TESTROOT=/Users/$(TARGET_MAC_USER)/buildslaves/$(TARGET_MAC_MINI)-01/builder_ios_test/sw_x/projects/xcode/PersonalCloud
override TARGET_SCRIPTROOT=/Users/$(TARGET_MAC_USER)/buildslaves/$(TARGET_MAC_MINI)-01/builder_ios_test/sw_x/projects/xcode/PersonalCloud/applescripts
override APP_DATA_DIR=/Users/$(TARGET_MAC_USER)
override TARGET_TESTROOT=/cygdrive/c/temp/igware/testroot
override USE_MOBILE_DEVICE=1
endif

ifeq ($(PRODUCT), android)
PRODUCT_KNOWN=1
override TESTPLAT=android
override MD_USER=$(TARGET_USER)
override MD_IP=$(TARGET_WIN32_MD)
override MD_OS=Android
override MD_NAME=$(TEST_MASTER_MACHINE)
override MD_CCD_Log =MD_CCDLog.tar.gz
override TARGET_MOBILE_DEVICE=$(TARGET_ANDROID)
override CLOUDPC_IP=$(TARGET_WIN32_CLOUDPC)
override CLOUDPC_NAME=$(TEST_MASTER_MACHINE)
override CLIENT_IP=$(TARGET_WIN32_CLIENT)
override CLIENT_NAME=$(TEST_MASTER_MACHINE)
override TARGET_LOG=$(TARGET_USER)@$(TEST_MASTER_MACHINE)
override REMOTE_RUN=/usr/bin/ssh $(TARGET_USER)@$(TEST_MASTER_MACHINE)
override TARGET_TESTROOT=/cygdrive/c/cygwin/home/$(TARGET_USER)/dxshell
override USE_MOBILE_DEVICE=1
endif

ifeq ($(PRODUCT), linux)
PRODUCT_KNOWN=1
override TESTPLAT=linux
override TARGET_TESTROOT=/temp/igware/testroot
override TARGET_DXREMOTE=/home/$(TARGET_USER)/dx_remote_agent
override GOLDEN_TEST_DATA_DIR=/home/$(TARGET_USER)/dxshell_root/GoldenTest
override DEFAULT_CCD_APPDATA_PATH=$(HOME)/temp/SyncAgent
override CCD_APPDATA_PATH_CLOUDPC=$(HOME)/temp/SyncAgent_CloudPC
override CCD_APPDATA_PATH_MD=$(HOME)/temp/SyncAgent_MD
override CCD_APPDATA_PATH_CLIENT=$(HOME)/temp/SyncAgent_Client
override KILL=killall
override DXSHELL_PACKAGE=dxshell.tar.gz
override UNZIP=tar xzvf
override EXE=
override CLOUDPC_OS=Linux
override MD_OS=Linux
override CLIENT_OS=Linux
override DXSHELL_PACKAGE_PATH=$(STORE_PATH)/$(TEST_BRANCH)/$(PRODUCT)/dxshell
override CCD_BINARY_PATH=$(BUILDROOT)/$(BUILDTYPE)/$(PRODUCT)/gvm_core/daemons/ccd
override DXSHELL_BINARY_PATH=$(BUILDROOT)/$(BUILDTYPE)/$(PRODUCT)/tests/dxshell
override DXREMOTEAGENT_BINARY_PATH=$(BUILDROOT)/$(BUILDTYPE)/$(PRODUCT)/gvm_apps/dx_remote_agent/platform_linux
override CCD_BUILDROOT_BINARY=$(BUILDROOT)/$(BUILDTYPE)/$(PRODUCT)/gvm_core/daemons/ccd/ccd_cloudnode
override DXSHELL_BUILDROOT_BINARY=$(BUILDROOT)/$(BUILDTYPE)/$(PRODUCT)/tests/dxshell/dxshell_cloudnode
override DXREMOTEAGENT_BUILDROOT_BINARY_PATH=$(BUILDROOT)/$(BUILDTYPE)/$(PRODUCT)/gvm_apps/dx_remote_agent/platform_linux/dx_remote_agent_cloudnode
ifdef TEST_TARGET_IS_CLOUDNODE
override REMOTE_RUN_PRIVILEGED=$(REMOTE_RUN)
override AS_TARGET_USER=sh -c
override CCD_ARCHIVE_BINARY_NAME=ccd_cloudnode
override DXSHELL_ARCHIVE_BINARY_NAME=dxshell_cloudnode
ifndef CCD_CLOUDNODE_NOT_COPIED_TO_CCD_BY_BUILD
  override CCD_BINARY_NAME=ccd.orig
  override DXSHELL_BINARY_NAME=dxshell.orig
endif #CCD_CLOUDNODE_NOT_COPIED_TO_CCD_BY_BUILD
endif #TEST_TARGET_IS_CLOUDNODE
endif

ifeq ($(PRODUCT_KNOWN), 0)
$(error PRODUCT="$(PRODUCT)" is unknown. Please use a supported PRODUCT.)
endif
