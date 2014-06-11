include android_env_defs.mk

SDCARD_SIZE?=128M
android_build_tools: \
				$(ANDROID_SDK_ROOT)/timestamp \
				$(ANDROID_NDK_ROOT)/timestamp \
				$(ANDROID_ANT_ROOT)/timestamp \
				$(ANDROID_AVD_ROOT)/timestamp

define DOWNLOAD_ANDROID_TOOL
	mkdir -p $(ANDROID_TOOLS_DOWNLOAD)
	wget --progress=dot:mega --timestamping --directory-prefix=$(ANDROID_TOOLS_DOWNLOAD) $(ANDROID_PREBUILT_DOWNLOAD_URL)/$1
endef

$(ANDROID_SDK_ROOT)/timestamp:
	$(call DOWNLOAD_ANDROID_TOOL,$(ANDROID_SDK))
	mkdir -p $(ANDROID_SDK_ROOT)
	rm -fr $(ANDROID_SDK_ROOT)/*
	tar x -C $(ANDROID_TOOLS_DOWNLOAD) -f $(ANDROID_TOOLS_DOWNLOAD)/$(ANDROID_SDK) && \
	mv $(ANDROID_TOOLS_DOWNLOAD)/android-sdk*/* $(ANDROID_SDK_ROOT)
	# If you need to download the latest stuff for the SDK from the web, comment out the line above
	# and uncomment the following two lines. Filename to use is defined in the android_env_defs.mk file.
	#mv $(ANDROID_TOOLS_DOWNLOAD)/android-sdk*/* $(ANDROID_SDK_ROOT) && \
	#cd $(ANDROID_SDK_ROOT) && ./tools/android update sdk --no-ui
	date > $@

$(ANDROID_NDK_ROOT)/timestamp:
	$(call DOWNLOAD_ANDROID_TOOL,$(ANDROID_NDK))
	mkdir -p $(ANDROID_NDK_ROOT)
	rm -fr $(ANDROID_NDK_ROOT)/*
	tar x -C $(ANDROID_TOOLS_DOWNLOAD) -f $(ANDROID_TOOLS_DOWNLOAD)/$(ANDROID_NDK) && \
	mv $(ANDROID_TOOLS_DOWNLOAD)/android-ndk*/* $(ANDROID_NDK_ROOT) && \
	date > $@

$(ANDROID_ANT_ROOT)/timestamp:
	$(call DOWNLOAD_ANDROID_TOOL,$(ANDROID_ANT_PACKAGE))
	mkdir -p $(ANDROID_ANT_ROOT)
	rm -fr $(ANDROID_ANT_ROOT)/*
	tar x -C $(ANDROID_TOOLS_DOWNLOAD) -f $(ANDROID_TOOLS_DOWNLOAD)/$(ANDROID_ANT_PACKAGE) && \
	mv $(ANDROID_TOOLS_DOWNLOAD)/ant*/* $(ANDROID_ANT_ROOT) && \
	date > $@

# The Android Virtual Device INI requires the full path to be specified but does not support environment variables,
# search paths, etc. The only solution is to generate the INI file during setup.
$(ANDROID_AVD_ROOT)/timestamp:
	$(call DOWNLOAD_ANDROID_TOOL,$(ANDROID_AVD_PACKAGE))
	mkdir -p $(ANDROID_AVD_ROOT)
	rm -fr $(ANDROID_AVD_ROOT)/*
	tar x -C $(ANDROID_TOOLS_DOWNLOAD) -f $(ANDROID_TOOLS_DOWNLOAD)/$(ANDROID_AVD_PACKAGE)
	mv $(ANDROID_TOOLS_DOWNLOAD)/avd*/* $(ANDROID_AVD_ROOT)
	/bin/echo -e "target=$(ANDROID_TARGET_8)\npath=$(ANDROID_AVD_ROOT)/$(ANDROID_AVD_8).avd" > $(ANDROID_AVD_ROOT)/$(ANDROID_AVD_8).ini
	/bin/echo -e "target=$(ANDROID_TARGET_10)\npath=$(ANDROID_AVD_ROOT)/$(ANDROID_AVD_10).avd" > $(ANDROID_AVD_ROOT)/$(ANDROID_AVD_10).ini
	/bin/echo -e "avd.ini.encoding=ISO-8859-1\ntarget=$(ANDROID_TARGET_17)\npath=$(ANDROID_AVD_ROOT)/$(ANDROID_AVD_17).avd\npath.rel=avd/$(ANDROID_AVD_17).avd" > $(ANDROID_AVD_ROOT)/$(ANDROID_AVD_17).ini
	$(ANDROID_SDK_ROOT)/tools/mksdcard $(SDCARD_SIZE) $(ANDROID_AVD_ROOT)/$(ANDROID_AVD_17).avd/sdcard.img
	date > $@
