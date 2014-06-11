LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

###
# In order to do native code debugging, gdb.setup and gdbserver need to be generated.
# 
# They are generated and placed under <project>/libs/armeabi when "ndk-build" is executed 
# from within the project directory and either:
#   1) android:debuggable="true" is in the AndroidManifest.xml, or
#   2) NDK_DEBUG=1 is on the commandline.
#
# Note that ndk-build is not run as part of the iGware builds; you must run it manually and the
# resulting gdb.setup will be specific to your development environment (it uses absolute paths).
##

LOCAL_MODULE    := dummy
LOCAL_SRC_FILES := 
# TODO: is there any way to get it to add this (sw_x) to the list of paths in gdb.setup?
#LOCAL_C_INCLUDES := ../../../../..


include $(BUILD_STATIC_LIBRARY)
