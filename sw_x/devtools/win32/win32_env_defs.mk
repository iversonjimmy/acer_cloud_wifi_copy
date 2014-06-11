# Definitions shared by all PRODUCTs targeted for Win32.

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

PREBUILT_TOOLS ?= $(HOME)/prebuilt_tools
##############################################

WIN32_TOOLS := $(PREBUILT_TOOLS)/win32
WIN32_TOOLS_DOWNLOAD := $(WIN32_TOOLS)/download

WIN32_PREBUILT_DOWNLOAD_URL := http://$(TOOL_HOST):$(HTTP_PORT)/$(TOOL_PATH)/win32_prebuilt

override WIN32API := w32api-3.17-2-mingw32-dev.tar.lzma
$(eval $(call OVERRIDE_PREBUILT_TOOL,WIN32API_ROOT,$(WIN32_TOOLS)/$(WIN32API:.tar.lzma=)))
