ANDROID_PLATFORM = $(ANDROID_NDK_ROOT)/platforms/android-5/arch-arm
BREAKPAD_CONFIG_OPTS = \
        --host=arm-android-eabi --disable-processor --disable-tools\
        CC=$(ANDROID_NDK_ROOT)/toolchains/arm-linux-androideabi-4.4.3/prebuilt/linux-x86/bin/arm-linux-androideabi-gcc \
        CPP=$(ANDROID_NDK_ROOT)/toolchains/arm-linux-androideabi-4.4.3/prebuilt/linux-x86/bin/arm-linux-androideabi-cpp \
        CXX=$(ANDROID_NDK_ROOT)/toolchains/arm-linux-androideabi-4.4.3/prebuilt/linux-x86/bin/arm-linux-androideabi-g++ \
        LD=$(ANDROID_NDK_ROOT)/toolchains/arm-linux-androideabi-4.4.3/prebuilt/linux-x86/bin/arm-linux-androideabi-ld \
        AR=$(ANDROID_NDK_ROOT)/toolchains/arm-linux-androideabi-4.4.3/prebuilt/linux-x86/bin/arm-linux-androideabi-ar \
        RANLIB=$(ANDROID_NDK_ROOT)/toolchains/arm-linux-androideabi-4.4.3/prebuilt/linux-x86/bin/arm-linux-androideabi-ranlib \
        STRIP=$(ANDROID_NDK_ROOT)/toolchains/arm-linux-androideabi-4.4.3/prebuilt/linux-x86/bin/arm-linux-androideabi-strip \
        CPPFLAGS="-I$(ANDROID_PLATFORM)/usr/include -D__ANDROID__" $(CPPFLAGS)\
	CFLAGS="-mandroid -I$(ANDROID_PLATFORM)/usr/include -msoft-float -fno-short-enums -fno-exceptions -march=armv5te -mthumb-interwork -D__ANDROID__" $(CFLAGS) \
        CXXFLAGS="-mandroid -I$(ANDROID_PLATFORM)/usr/include -I$(ANDROID_NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/include -I$(ANDROID_NDK_ROOT)/sources/cxx-stl/gnu-libstdc++/libs/armeabi/include -msoft-float -fno-short-enums -fno-exceptions -march=armv5te -mthumb-interwork -D__ANDROID__" $(CXXFLAGS) \
        LDFLAGS="-mandroid -L$(ANDROID_PLATFORM)/usr/lib -Wl,-rpath=/system/lib -Wl,-rpath-link=$(ANDROID_PLATFORM)/usr/lib --sysroot=$(ANDROID_PLATFORM)"  $(LDFLAGS)\
        LIBS="$(ANDROID_PLATFORM)/usr/lib/libc.so"
# end of list
