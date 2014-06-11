##########################################################################
# Global defs to build for Android go here.
##########################################################################

# Include the platform-independent stuff.
include $(SRCROOT)/make/common_defs.mk

# Get the location(s) of the prebuilt tools automatically.
include $(SRCROOT)/devtools/android/android_env_defs.mk

ifndef ANDROID_ANT_ROOT
$(error Must define ANDROID_ANT_ROOT)
endif
ifndef ANDROID_SDK_ROOT
$(error Must define ANDROID_SDK_ROOT)
endif
ifndef ANDROID_NDK_ROOT
$(error Must define ANDROID_NDK_ROOT)
endif

# Using android-8 because that's currently the latest "usable" x86 port.
ifeq ($(ARCH), arm)
ANDROID_NDK_SYS_ROOT = $(ANDROID_NDK_ROOT)/platforms/android-8/arch-arm
endif
ifeq ($(ARCH), x86)
ANDROID_NDK_SYS_ROOT = $(ANDROID_NDK_ROOT)/ndk/android-ndk-r4/darwin/platforms/android-8/arch-x86
endif

PIC_COMPILE_FLAGS = -fpic

#####################################################
# Tools
#####################################################
ANDROID_ANT := $(ANDROID_ANT_ROOT)/bin/ant

################
# Android SDK tools
################
ADB = $(ANDROID_SDK_ROOT)/tools/adb
AAPT = $(ANDROID_SDK_ROOT)/tools/aapt
DX = $(ANDROID_SDK_ROOT)/tools/dx
APKBUILDER = $(ANDROID_SDK_ROOT)/tools/apkbuilder

################
# Standard GCC + binutils
################
ifeq ($(ARCH), arm)
BINUTILS_PREFIX ?= $(ANDROID_NDK_ROOT)/toolchains/arm-linux-androideabi-4.4.3/prebuilt/linux-x86/bin/arm-linux-androideabi-
endif
ifeq ($(ARCH), x86)
X86_TOOL_BIN = $(ANDROID_NDK_ROOT)/linux-x86/toolchain/i686-unknown-linux-gnu-4.2.1/bin
BINUTILS_PREFIX ?= $(X86_TOOL_BIN)/i686-unknown-linux-gnu-
endif

CC = $(BINUTILS_PREFIX)gcc
CXX = $(BINUTILS_PREFIX)g++
AR = $(BINUTILS_PREFIX)ar
LD = $(CXX)
OBJCOPY = $(BINUTILS_PREFIX)objcopy
STRIP = $(BINUTILS_PREFIX)strip
# TODO: Uncomment --strip-unneeded once we have a system in place to automatically reconstruct the debugging symbols.
STRIPLIB = $(STRIP) # --strip-unneeded

#####################################################
# C and C++ compiler flags
#####################################################
GCC_GXX_WARNINGS := \
    -Wall -Werror \
    -Wmissing-declarations \
    -Wno-packed \
    -Wpointer-arith \
    -Wstrict-aliasing=3 \
    -Wswitch-enum \
    -Wwrite-strings \
    -Wfloat-equal \
    -Wextra -Wno-unused-parameter -Wno-missing-field-initializers \
# end of list
# TODO: should be able to restore -Wundef next time we update NDK (bug resolved)
# TODO: want -Wredundant-decls, but the standard headers provided with NDK currently fail this
# TODO: want -Wpacked, but layout.h from GVM's VPLDisplay
# emits hundreds of lines of warnings...

# Suppress the pointless "mangling of 'va_list' has changed in GCC 4.4" messages.
# GCC 4.5 will fix this, but until then, it's nothing to worry about.
GCC_GXX_WARNINGS += -Wno-psabi

# MP makes it so that the dependency file still works when a header file is moved/deleted.
DEPFILE_USE_PHONY = -MP

ifeq ($(ARCH), arm)
C_CXX_FLAGS += \
    -D__ARM_ARCH_5__ \
    -D__ARM_ARCH_5T__ \
    -D__ARM_ARCH_5E__ \
    -D__ARM_ARCH_5TE__ \
    -march=armv5te \
    -mtune=xscale \
    -msoft-float \
    -mthumb \
    -Os \
    -DANDROID \
    -DHOST_IS_LITTLE_ENDIAN \
    -Wa,--noexecstack \
# end of list
endif
ifeq ($(ARCH), x86)
# TODO: There are too many flags in here.  Things like -fPIC are handled per-make-target by makegen.
#     Warnings are handled above.  DEBUG vs NDEBUG is handled below.
C_CXX_FLAGS += \
    -DHOST_IS_LITTLE_ENDIAN \
    -Wa,--noexecstack -fno-stack-protector \
    -c  -fno-exceptions -Wno-multichar -march=i686 -m32  \
    -fPIC -D__ANDROID__ -DANDROID -fmessage-length=0 -W -Wall \
    -Wno-unused -Winit-self -Wpointer-arith -Werror=return-type \
    -Werror=non-virtual-dtor -Werror=address -Werror=sequence-point \
    -O2 -g -fno-strict-aliasing -DNDEBUG -UDEBUG \
    -fmessage-length=0 -W -Wall -Wno-unused -Winit-self -Wpointer-arith \
    -Werror=return-type -Werror=non-virtual-dtor \
    -Werror=address -Werror=sequence-point -MD \
# end of list
endif

# These must come *after* our own headers.
CINCLUDES += -I$(ANDROID_NDK_SYS_ROOT)/usr/include

CXXINCLUDES += \
    -I$(ANDROID_NDK_SYS_ROOT)/usr/include \
    -I$(ANDROID_NDK_ROOT)/sources/cxx-stl/stlport/stlport \
    -I$(ANDROID_NDK_ROOT)/sources/cxx-stl/system/include \
# end of list

CXX_ONLY_FLAGS = \
    -fno-rtti \
    -DGOOGLE_PROTOBUF_NO_RTTI \
    -fno-exceptions \
# end of list

POST_PROCESS_C_OBJ = $(FIX_DEPENDENCY_FILE_GCC)
POST_PROCESS_CXX_OBJ = $(FIX_DEPENDENCY_FILE_GCC)
POST_PROCESS_SHARED_LIB = # nothing to do

ifeq ($(BUILDTYPE),debug)
OPTIMIZER ?= -O0
C_CXX_FLAGS += -g3 -D_DEBUG -DDEBUG
POST_PROCESS_BINARY = # nothing to do
else
OPTIMIZER ?= -O2
# It's still a good idea to put the -g3 here, since the deployed version of the .so will always get
# the debug symbols stripped out when it is being packaged into the .apk, but the non-stripped
# version can help us debug release mode.
C_CXX_FLAGS += -g3 -DNDEBUG -ffunction-sections -funwind-tables -fomit-frame-pointer -fno-strict-aliasing -finline-limit=64
ifeq ($(ARCH), arm)
# X86 doesn't have the necessary support it its NDK libraries.
C_CXX_FLAGS += -fstack-protector
endif
POST_PROCESS_BINARY = $(SEPARATE_DEBUG_INFO_BINUTILS)
endif

C_CXX_FLAGS += $(OPTIMIZER) $(DEPFILE_USE_PHONY) $(GCC_GXX_WARNINGS)

# Flags from the old GVM build; some of these might not be needed anymore.
#C_CXX_FLAGS += -DLINUX -DHOST_IS_LITTLE_ENDIAN -DGVM -D_FILE_OFFSET_BITS=64  -DGVM_BUILD_INTERN -m32

# This is needed for compiling things that use protobuf.
# Strangely, this didn't seem to be needed when building with the GVM toolchain.
# TODO: patch the protobuf .h file instead
C_CXX_FLAGS += -Wno-sign-compare

# Temp hack for using -Wundef with NDK; see http://code.google.com/p/android/issues/detail?id=14627
C_CXX_FLAGS += -D__STDC_VERSION__=0

CFLAGS   += $(C_CXX_FLAGS)
CXXFLAGS += $(CXX_ONLY_FLAGS) $(C_CXX_FLAGS)

ifeq ($(ARCH), arm)
LDFLAGS += \
    --sysroot=$(ANDROID_NDK_SYS_ROOT) \
    -Wl,-rpath-link=$(ANDROID_NDK_SYS_ROOT)/usr/lib \
# end of list
endif
ifeq ($(ARCH), x86)
# TODO: this looks like it is specific to linking binaries; the basic LDFLAGS should apply to both binaries and .so files.
X86_NDK_LIBGCC = $(ANDROID_NDK_ROOT)/linux-x86/toolchain/i686-unknown-linux-gnu-4.2.1/lib/gcc/i686-unknown-linux-gnu/4.2.1
LDFLAGS += \
    -nostdlib -Bdynamic \
    -Wl,-dynamic-linker,/system/bin/linker -Wl,-z,nocopyreloc \
    -L $(ANDROID_NDK_SYS_ROOT)/usr/lib \
    -Wl,-rpath-link=$(ANDROID_NDK_SYS_ROOT)/usr/lib \
    -lc -lstdc++ -lm \
    $(ANDROID_NDK_SYS_ROOT)/usr/lib/crtbegin_dynamic.o \
    -Wl,--no-undefined \
    $(X86_NDK_LIBGCC)/libgcc.a \
    $(X86_NDK_LIBGCC)/libgcc_eh.a \
    $(ANDROID_NDK_SYS_ROOT)/usr/lib/crtend_android.o
# end of list
endif

#####################################################
