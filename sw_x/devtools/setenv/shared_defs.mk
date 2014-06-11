# Definitions shared by all PRODUCTs.

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
