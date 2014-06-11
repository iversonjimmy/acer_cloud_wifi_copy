# Note: This configuration is not currently part of the normal build; it is only used to manually
#       create the config.h file for now.

C_CXX_FLAGS='-D__ARM_ARCH_5__ -D__ARM_ARCH_5T__ -D__ARM_ARCH_5E__ -D__ARM_ARCH_5TE__ -march=armv5te -mtune=xscale -msoft-float -mthumb -Os -DANDROID -Wa,--noexecstack -mthumb-interwork -ffunction-sections -funwind-tables -fstack-protector -fno-short-enums -fomit-frame-pointer -fno-strict-aliasing -finline-limit=64'

PROTO_CONF_FLAGS = \
	--host=arm-linux-androideabi \
	--enable-static \
	--disable-shared \
	--with-protoc=$(PROTOC) \
	CXXFLAGS="'-fno-rtti -DGOOGLE_PROTOBUF_NO_RTTI -fno-exceptions \
		-I$(ANDROID_NDK_ROOT)/platforms/android-9/arch-arm/usr/include/ \
		-I$(ANDROID_NDK_ROOT)/sources/cxx-stl/stlport/stlport \
		-I$(ANDROID_NDK_ROOT)/sources/cxx-stl/system/include \
		$(C_CXX_FLAGS)'" \
	CFLAGS="$(C_CXX_FLAGS)" \
	LDFLAGS='-nostdlib -Bstatic -Wl,-z,nocopyreloc' \
	LIBS='$(ANDROID_NDK_ROOT)/toolchains/arm-linux-androideabi-4.4.3/prebuilt/linux-x86/lib/gcc/arm-linux-androideabi/4.4.3/libgcc.a \
		$(ANDROID_NDK_ROOT)/platforms/android-9/arch-arm/usr/lib/libc.a \
		$(ANDROID_NDK_ROOT)/platforms/android-9/arch-arm/usr/lib/libstdc++.a \
		$(ANDROID_NDK_ROOT)/platforms/android-9/arch-arm/usr/lib/libm.a \
		-Wl,-z,noexecstack \
		-Wl,-rpath-link=$(ANDROID_NDK_ROOT)/platforms/android-9/arch-arm/usr/lib \
		$(ANDROID_NDK_ROOT)/platforms/android-9/arch-arm/usr/lib/crtend_android.o' \
#end of list

BUILD_JAVA_PROTOBUF = defined
#BUILD_PYTHON_PROTOBUF = defined
