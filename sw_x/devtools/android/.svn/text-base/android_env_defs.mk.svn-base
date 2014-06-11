# Definitions shared by all PRODUCTs targeted for Android.

ifndef SRCROOT
$(error Need to define SRCROOT (or run this from a higher level Makefile))
endif

include $(SRCROOT)/make/external_locations.mk

##############################################
# TODO: copy & pasted
##############################################
define ENDL


endef

# TODO: instead of saying "correct it", it would be better if we could say: run "source devtools/setenv.sh" from sw_x

define OVERRIDE_PREBUILT_TOOL
ifdef $(1)
ifneq ($($(1)),$(2))
$$(error $(1) is different than what the source tree expected.$$(ENDL)\
    Expected: "$(2)"$$(ENDL)\
    Was:      "$($(1))"$$(ENDL)\
  You must either undefine $(1) or correct it)
endif
endif
override $(1) := $(2)
export $(1)
endef

PREBUILT_TOOLS ?= $(HOME)/prebuilt_tools/$(PRODUCT)
##############################################

ANDROID_TOOLS := $(PREBUILT_TOOLS)/android
ANDROID_TOOLS_DOWNLOAD := $(ANDROID_TOOLS)/download

ANDROID_PREBUILT_DOWNLOAD_URL := http://$(TOOL_HOST):$(HTTP_PORT)/$(TOOL_PATH)/android_prebuilt

override ANDROID_ANT_PACKAGE := ant1.8.1_20110314.tar.bz2
$(eval $(call OVERRIDE_PREBUILT_TOOL,ANDROID_ANT_ROOT,$(ANDROID_TOOLS)/$(ANDROID_ANT_PACKAGE:.tar.bz2=)))

override ANDROID_SDK := android-sdk_r21.1-linux_x86_full_20130319.tgz
$(eval $(call OVERRIDE_PREBUILT_TOOL,ANDROID_SDK_ROOT,$(ANDROID_TOOLS)/$(ANDROID_SDK:.tgz=)))

override ANDROID_NDK := android-ndk-r5b-linux-x86_20110303.tar.bz2
$(eval $(call OVERRIDE_PREBUILT_TOOL,ANDROID_NDK_ROOT,$(ANDROID_TOOLS)/$(ANDROID_NDK:.tar.bz2=)))

override ANDROID_AVD_PACKAGE := avd_20130402.tar.bz2
override ANDROID_AVD_ROOT := $(HOME)/.android/avd

# Use "$(ANDROID_SDK_ROOT)/tools/android list targets" to get a list of available platforms.
override ANDROID_TARGET_8 := android-8
override ANDROID_TARGET_10 := android-10
override ANDROID_TARGET_17 := android-17

# Android Virtual Devices (AVD) created with following command:
# $(ANDROID_SDK_ROOT)/android create avd -t $(ANDROID_TARGET_XX) -n $(ANDROID_AVD_XX) -c 128M
# That creates an AVD with a 128MB sdcard with a default hardware profile.
override ANDROID_AVD_8 := $(ANDROID_TARGET_8)-avd
override ANDROID_AVD_10 := $(ANDROID_TARGET_10)-avd
override ANDROID_AVD_17 := $(ANDROID_TARGET_17)-avd

# Logcat filter example: "*:s Email:d"
override ANDROID_LOGCAT_STR := "*"
override ANDROID_EMULATOR_BINARY := emulator
override ANDROID_EMULATOR := $(ANDROID_SDK_ROOT)/tools/$(ANDROID_EMULATOR_BINARY)
override ANDROID_ADB := $(ANDROID_SDK_ROOT)/platform-tools/adb
override ANDROID_EMULATOR_WAIT := $(WORKAREA)/sw_x/devtools/android/emulator-wait-boot
override ANDROID_WAIT := $(WORKAREA)/sw_x/devtools/android/emulator-wait
