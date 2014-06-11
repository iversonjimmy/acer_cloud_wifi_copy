# Definitions shared by all PRODUCTs targeted for Cloudnode.

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

CLOUDNODE_TOOLS := $(PREBUILT_TOOLS)/cloudnode
CLOUDNODE_TOOLS_DOWNLOAD := $(CLOUDNODE_TOOLS)/download

CLOUDNODE_PREBUILT_DOWNLOAD_URL := http://$(TOOL_HOST):$(HTTP_PORT)/$(TOOL_PATH)/cloudnode_prebuilt/
CLOUDNODE_PREBUILT_SCP_SRC := $(TOOL_HOST):/$(FS_TOOL_PATH)/cloudnode_prebuilt/

override CLOUDNODE_CROSS_PACKAGE := cloudnode_x86-2-arm_20121030.tar.bz2
$(eval $(call OVERRIDE_PREBUILT_TOOL,CLOUDNODE_CROSS_ROOT,$(CLOUDNODE_TOOLS)/$(CLOUDNODE_CROSS_PACKAGE:.tar.bz2=)))

