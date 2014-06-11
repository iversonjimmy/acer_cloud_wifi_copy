##########################################################################
# Build defs shared by all platforms go here.
# Changes to this file will cause all objects and libraries to be rebuilt.
##########################################################################

# When this file changes, rebuild all objects and libraries.
DEFS_FILES += $(SRCROOT)/make/common_defs.mk

PROTOC = $(TOOLS_ROOT)/usr/bin/protoc$(TOOL_EXECUTABLE_SUFFIX)
PROTORPCGEN = $(TOOLS_ROOT)/gvm_core/devtools/protorpcgen/protorpcgen$(TOOL_EXECUTABLE_SUFFIX)
# Note: only supported when (HOST_PLATFORM is cygwin)
PROTOC_DOTNET = $(TOOLS_ROOT)/external/protobuf_dotnet/protobuf/protogen.exe

# Developers can set VERBOSE=1 to get additional information from certain build steps.
ifdef VERBOSE
ANT_FLAGS += -verbose
endif

#####################################################
# makegen hooks (documentation only)
# The following variables are automatically $(call)'ed after certain things are built.
# The <platform>_defs.mk can set these variables to perform platform-specific steps.
#####################################################
# POST_PROCESS_BINARY is called after linking a binary executable.
# Params:
# $1: executable output file path/filename
###############################
# POST_PROCESS_SHARED_LIB is called after linking a shared library.
# Params:
# $1: shared library file path/filename
###############################
# $(POST_PROCESS_C_OBJ) is called after compiling a C source file.
# Params:
# $1: source file (compiler input)
# $2: the new output object file
# $3: the new output dependency file
###############################
# $(POST_PROCESS_CXX_OBJ) is called after compiling a C++ source file.
# Params:
# $1: source file (compiler input)
# $2: the new output object file
# $3: the new output dependency file
###############################

#####################################################
# Utility functions
#####################################################

###############################
# Command to update .cvsignore from $files variable.
# Now that sw_x uses SVN, this is a no-op (so run 'true' to return success).
# When a new generated file is added, someone will need to manually tell SVN.
###############################
CVSIGNORE = true

###############################
# FIX_DEPENDENCY_FILE_* definitions
###############################
# $(call FIX_DEPENDENCY_FILE_<type>) to produce a command that
# fixes up dependency files.
# Intended for use as a $(POST_PROCESS_C_OBJ) or $(POST_PROCESS_CXX_OBJ) step.
# Params:
# $1: .c/.cpp filename
# $2: .o filename
# $3: .d filename

# mwcc's (Metrowerks) dependency files have several issues that aren't fixable via compiler flags.
# Steps taken by sed:
#   1. replace all backslashes with forward-slashes
#   2. restore the end-of-line backslashes
#   3. change the input file to be an absolute path and add the opening 'wildcard' call (to deal with removed header files)
#   4. add a ')' on the last line (to end the 'wildcard' call)
FIX_DEPENDENCY_FILE_MWCC = sed 's|\\|/|g;  s|/$$|\\|g;  s|\(^[[:alpha:]].*\):.*\([ \\]\)$$|\1: $$(wildcard $1 \2|1;  s| $$| )|g' $3 > $3.temp && rm -f $3 && mv $3.temp $3

# GCC has an issue that isn't fixable via compiler flags.
# Strip out any mention of the source file from the .d file.  The makefiles are already responsible for expressing
# that the .o file depends on the .c/.cpp file; having it in the .d file breaks the build when the source file is moved or extension changed between C/C++.
FIX_DEPENDENCY_FILE_GCC = sed 's|[-./_\\A-Za-z0-9]*$(notdir $1) ||g;' $3 > $3.temp && rm -f $3 && mv $3.temp $3

###############################
# SEPARATE_DEBUG_INFO_* definitions
###############################
# $(call SEPARATE_DEBUG_INFO_<type>) to produce a command that
# extracts debug info to a separate file and removes it from the original binary.
# Intended for use as a $(POST_PROCESS_CPP_BINARY) step.
# Params:
# $1: binary filename

# The debug info is extracted to <binary>.debug and a "gnu-debuglink" is added
# to the stripped binary.
SEPARATE_DEBUG_INFO_BINUTILS = \
	$(OBJCOPY) --only-keep-debug $1 $1.debug && \
	$(STRIP) --strip-debug --strip-unneeded $1 && \
	$(OBJCOPY) --add-gnu-debuglink=$1.debug $1

###############################
