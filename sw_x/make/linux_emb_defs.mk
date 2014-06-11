##########################################################################
# Global defs for linux go here
##########################################################################

# Include the platform-independent stuff.
include $(SRCROOT)/make/common_defs.mk

PIC_COMPILE_FLAGS = -fpic

#####################################################
# Tools
#####################################################
################
# Standard GCC + binutils
################
ifeq ($(ARCH), mips)
BINUTILS_PREFIX ?= /home/jimmy.tsai/prebuilt_tools/rtl819xD/rsdk-4.4.7-5281-EB-2.6.30-0.9.33-m32utb-140606/bin/mips-linux-
endif
ifeq ($(ARCH), x86)
$(error X86 architecture not supported for cloudnode target builds.)
endif

CC = $(BINUTILS_PREFIX)gcc
CXX = $(BINUTILS_PREFIX)g++
AR = $(BINUTILS_PREFIX)ar
LD = $(CXX)
OBJCOPY = $(BINUTILS_PREFIX)objcopy
STRIP = $(BINUTILS_PREFIX)strip
#####################################################
# C and C++ compiler flags
#####################################################
GCC_GXX_WARNINGS := \
    -Wall -Werror \
    -Wno-packed \
    -Wpointer-arith \
    -Wredundant-decls \
    -Wstrict-aliasing=3 \
    -Wswitch-enum \
    -Wundef \
    -Wwrite-strings \
    -Wextra -Wno-unused-parameter \
# end of list
#    -Wfloat-equal conflicts jballantyne's delta-t game-time
#    computation, which uses == 0.
#
# Note:  would prefer -Wpacked, but layout.h from GVM's VPLDisplay
# emits hundreds of lines of warnings...

# Our Red Hat environment uses an older version of gcc. These are workarounds for that.
ifeq ($(HOST_PLATFORM_SUB),redhat)
C_CXX_FLAGS +=-fno-strict-aliasing
else
GXX_WARNINGS += -Wmissing-declarations
endif
GCC_WARNINGS += -Wmissing-declarations

CINCLUDES += -I/home/jimmy.tsai/prebuilt_tools/rtl819xD/rsdk-4.4.7-5281-EB-2.6.30-0.9.33-m32utb-140606/usr/include \
#CINCLUDES += -I/home/jimmy.tsai/prebuilt_tools/rtl819xD/rsdk-4.4.7-5281-EB-2.6.30-0.9.33-m32utb-140606/include \
#
#CINCLUDES += -I/home/jimmy.tsai/prebuilt_tools/rtl819xD/rsdk-1.5.5-5281-EB-2.6.30-0.9.30.3-110714/include \
# end of list
# # #
CXXINCLUDES += -I/home/jimmy.tsai/prebuilt_tools/rtl819xD/rsdk-4.4.7-5281-EB-2.6.30-0.9.33-m32utb-140606/usr/include \
#CXXINCLUDES += -I/home/jimmy.tsai/prebuilt_tools/rtl819xD/rsdk-4.4.7-5281-EB-2.6.30-0.9.33-m32utb-140606/include \
#
#CXXINCLUDES += -I/home/jimmy.tsai/prebuilt_tools/rtl819xD/rsdk-1.5.5-5281-EB-2.6.30-0.9.30.3-110714/include \
# # end of list
#

# MP makes it so that the dependency file still works when a header file is moved/deleted.
#DEPFILE_USE_PHONY = -MP

POST_PROCESS_C_OBJ = $(FIX_DEPENDENCY_FILE_GCC)
POST_PROCESS_CXX_OBJ = $(FIX_DEPENDENCY_FILE_GCC)
POST_PROCESS_SHARED_LIB = # nothing to do

# These are not the real debug flags, just an example
ifeq ($(BUILDTYPE),debug)
OPTIMIZER ?= -O0
C_CXX_FLAGS += -g3 -D_DEBUG -DDEBUG
POST_PROCESS_BINARY = # nothing to do
else
OPTIMIZER ?= -O2
# It's still a good idea to put the -g3 here so that we can debug the release mode binaries.
# With our current code base and version of gcc, the compiler emits the same exact code independently of -g,
# and we can always strip the debug symbols from any binaries before releasing them.
C_CXX_FLAGS += -g3
POST_PROCESS_BINARY = $(SEPARATE_DEBUG_INFO_BINUTILS)
# This disables assert()s, which can result in unused variable warnings.
#C_CXX_FLAGS += -DNDEBUG
endif

C_CXX_FLAGS += $(OPTIMIZER) $(DEPFILE_USE_PHONY) $(GCC_GXX_WARNINGS)

# Flags from the old GVM build; some of these might not be needed anymore.
C_CXX_FLAGS += -DLINUX -DLINUX_EMB -DHOST_IS_BIG_ENDIAN -DGVM -D_FILE_OFFSET_BITS=64  -DGVM_BUILD_INTERN 

# This is needed for compiling things that use protobuf.
# Strangely, this didn't seem to be needed when building with the GVM toolchain.
# TODO: patch the protobuf .h file instead
C_CXX_FLAGS += -Wno-sign-compare

CFLAGS   += $(C_CXX_FLAGS) $(GCC_WARNINGS)
CXXFLAGS += $(C_CXX_FLAGS) $(GXX_WARNINGS)

LDFLAGS += -Wl,-rpath-link=/home/jimmy.tsai/prebuilt_tools/rtl819xD/rsdk-4.4.7-5281-EB-2.6.30-0.9.33-m32utb-140606/lib \
#
#LDFLAGS += -Wl,-rpath-link=/home/jimmy.tsai/prebuilt_tools/rtl819xD/rsdk-1.5.5-5281-EB-2.6.30-0.9.30.3-110714/external_libs \
# end of list

#####################################################
