######
# Check prereqs:
######
ifndef PRODUCT
$(error Must define PRODUCT)
endif
ifndef BUILDROOT
$(error Must define BUILDROOT)
endif

######
# Mention default first, so that it really is the default target
######
default:

######

override SRCROOT := $(abspath $(CURDIR))
export SRCROOT

######
# Detect our buildhost environment.
######
ifeq ($(wildcard /proc/version),)
# /proc/version doesn't exist.
ifneq ($(shell sw_vers | grep 'Mac OS X'),)
override HOST_PLATFORM := mac
override HOST_PLATFORM_SUB := osx
else
$(error Unable to detect platform from sw_vers (and /proc/version does not exist).  Are you sure this is a supported build environment?)
endif
else
# /proc/version exists.
ifneq ($(shell grep 'centos' /proc/version),) # check this before 'Red Hat'
override HOST_PLATFORM := linux
override HOST_PLATFORM_SUB := centos
# Make sure we get Python version >= 2.6
override PATH := /usr/local/bin:$(PATH)
export PATH
else
ifneq ($(shell grep 'Red Hat' /proc/version),)
override HOST_PLATFORM := linux
override HOST_PLATFORM_SUB := redhat
# Make sure we get Python version >= 2.6
override PATH := /usr/local/bin:$(PATH)
export PATH
else
ifneq ($(shell grep 'Ubuntu' /proc/version),)
override HOST_PLATFORM := linux
override HOST_PLATFORM_SUB := ubuntu
else
ifneq ($(shell grep 'CYGWIN' /proc/version),)
override HOST_PLATFORM := cygwin
override HOST_PLATFORM_SUB := any
override BUILDROOT := $(shell cygpath -u $(BUILDROOT))
else
$(error Unable to detect platform from /proc/version.  Are you sure this is a supported build environment?)
endif
endif
endif
endif
endif
######

$(info >>> Detected HOST_PLATFORM: $(HOST_PLATFORM) ($(HOST_PLATFORM_SUB)))

# Include Product Type definitions
# Both the list of Product Types and the list of product variables derived
# from the Product Type are maintained in make/product_defs.py and should
# not be duplicated anywhere else.  See make/product_defs.mk which is the
# output of the python script for functions that can be used to work with
# these lists.
include make/product_defs.mk
export PRODUCT_VARS
# Set product parameters
$(eval $(call setParamsFromProduct,$(PRODUCT),))
override TARGET_PLATFORM := $(PLATFORM)

ifdef DEBUG
BUILDTYPE := debug
else
BUILDTYPE := release
endif

# abspath removes trailing slash as well as any . or .. elements.
override BUILDROOT := $(abspath $(BUILDROOT))
override PBROOT := $(BUILDROOT)/$(BUILDTYPE)/$(PRODUCT)
# Note that the Visual Studio projects and sw_i/Makefile.setup all need to be updated if this
# is changed.
override TOOLS_ROOT := $(BUILDROOT)/tools/$(HOST_PLATFORM)_$(HOST_PLATFORM_SUB)

export BUILDROOT
export HOST_PLATFORM
export HOST_PLATFORM_SUB
export PBROOT
export TARGET_PLATFORM
export TOOLS_ROOT

$(info >>> BUILDROOT: "$(BUILDROOT)")

EXPORTED_PRODUCT_VARS = $(PRODUCT_VARS) BUILDTYPE
setupCmdLineEnviron = $(foreach VAR,$(EXPORTED_PRODUCT_VARS),$(VAR)=$($(VAR)))

ifeq ($(wildcard $(PBROOT)/Make.deps),)
$(info Make.deps does not exist (this is expected for a clean BUILDROOT).)
else
# This will also define BUILD_FILES.
include $(PBROOT)/Make.deps
endif

# These targets will never be passed to the generated Makefile.
PHONY_TOP_LEVEL_TARGETS := \
	__devtools \
	echo_pbr \
	gen_build_info \
	list \
	list_makegen \
	$(BUILD_FILES) \
# note that 'default' is a valid target to pass to the generated Makefile, so don't list it here.
# end of list

override ITEMS_TO_BUILD = $(filter-out $(PBROOT)/Makefile $(PHONY_TOP_LEVEL_TARGETS),$(MAKECMDGOALS))
ifeq ($(ITEMS_TO_BUILD),)
override ITEMS_TO_BUILD = default
endif

BUILD_INFO_INCLUDE_DIR := $(PBROOT)/build_info/include
BUILD_INFO_INCLUDE_FILE := $(BUILD_INFO_INCLUDE_DIR)/build_info.h

.PHONY: gen_build_info
gen_build_info:
ifneq ($(BUILD_DATE),)
	rm -f $(BUILD_INFO_INCLUDE_FILE)
	mkdir -p $(BUILD_INFO_INCLUDE_DIR)
	echo '#define BUILD_INFO "'$(PRODUCT) $(BUILD_DATE) $(BUILD_BRANCH)'"' > $(BUILD_INFO_INCLUDE_FILE);
	echo '#define SW_BUILD_INFO "'`cat $(SRCROOT)/dist/igware.runtime/product.version.number`:`cat $(SRCROOT)/dist/igware.runtime/product.build.number`:`hostname`'"' >> $(BUILD_INFO_INCLUDE_FILE);
else
	mkdir -p $(BUILD_INFO_INCLUDE_DIR)
	echo '#define BUILD_INFO "'$(PRODUCT) developer build on `hostname` `cat $(SRCROOT)/dist/igware.runtime/product.version.number`'"' > $(BUILD_INFO_INCLUDE_FILE).temp;
	echo '#define SW_BUILD_INFO "'`cat $(SRCROOT)/dist/igware.runtime/product.version.number`:0:developer'"' >> $(BUILD_INFO_INCLUDE_FILE).temp;
	@if ! (diff -q $(BUILD_INFO_INCLUDE_FILE).temp $(BUILD_INFO_INCLUDE_FILE)); then \
		echo '--- Updating build_info.h:'; \
		rm -f $(BUILD_INFO_INCLUDE_FILE); \
		mv $(BUILD_INFO_INCLUDE_FILE).temp $(BUILD_INFO_INCLUDE_FILE); \
	else \
		echo '--- build_info.h is up-to-date:'; \
		rm -f $(BUILD_INFO_INCLUDE_FILE).temp; \
	fi
endif
	cat $(BUILD_INFO_INCLUDE_FILE)

# Since building 'default' actually builds all ITEMS_TO_BUILD, we only need to run this
# recipe once.  Therefore, make each element in ITEMS_TO_BUILD depend on 'default' instead of adding
# them as additional targets for this rule.
.PHONY: default
default: $(PBROOT)/Makefile __devtools gen_build_info
	@echo '>>> Building $(BUILDTYPE)/$(PRODUCT):'
	$(MAKE) $(call setupCmdLineEnviron) -C $(PBROOT) $(ITEMS_TO_BUILD)

DEPEND_ON_DEFAULT = $(filter-out default,$(ITEMS_TO_BUILD))
.PHONY: $(DEPEND_ON_DEFAULT)
$(DEPEND_ON_DEFAULT): | default
# Without @true here, make would report a bunch of confusing "make: Nothing to be done for `XX`" messages when
# ITEMS_TO_BUILD has more than one element.
	@true

# Check to make sure that the SRCROOT hasn't been changed.
$(BUILDROOT)/SRCROOT_stamp_sw_x:
	mkdir -p $(BUILDROOT)
	echo "BOUND_SRCROOT=$(SRCROOT)" > $@
-include $(BUILDROOT)/SRCROOT_stamp_sw_x
ifdef BOUND_SRCROOT
ifneq ($(BOUND_SRCROOT),$(SRCROOT))
$(error BUILDROOT="$(BUILDROOT)" already has output from SRCROOT="$(BOUND_SRCROOT)", but you are attempting to build from SRCROOT="$(SRCROOT)".  Please choose a different BUILDROOT, or completely remove the contents of "$(BUILDROOT)")
endif
endif

# Simply echo $(PBROOT) to stdout.
.PHONY: echo_pbr
echo_pbr:
	@echo $(PBROOT)

#########
# "make list" prints a list of all the generated make targets that make knows how to build for
# this environment.  Very useful!
#########
.PHONY: list
list:
	$(call setupCmdLineEnviron) python -tt make/makegen.py --list_make_targets

#########
# List all of the targets, in makegen syntax.
#########
.PHONY: list_makegen
list_makegen:
	$(call setupCmdLineEnviron) python -tt make/makegen.py --list_makegen_targets

#########
# This allows you to "make [foo/bar/]BUILD" to list all of the targets beneath a particular BUILD file.
#########
# BUILD_FILES will get overwritten in $(TOOLS_ROOT)/Make.deps
BUILD_FILE_TO_LIST = $(filter $(BUILD_FILES),$(MAKECMDGOALS))
ifneq ($(BUILD_FILE_TO_LIST),)
.PHONY: $(BUILD_FILE_TO_LIST)
$(BUILD_FILE_TO_LIST):
	@echo Targets beneath \"$(subst BUILD,,$@)\":
	@$(call setupCmdLineEnviron) python -tt make/makegen.py --list_make_targets | grep "  $(subst BUILD,,$@)"
# Since the BUILD file needs to be PHONY to run this rule, make will forever consider this Makefile out-of-date
# and continuously rerun this recipe. Return false to break out of this loop now:
	@false
endif

#########
# While processing __devtools and $(TOOLS_ROOT)/Make.deps, temporarily switch PBROOT to be TOOLS_ROOT.
#########
override PBROOT := $(TOOLS_ROOT)
export PBROOT

.PHONY: __devtools
__devtools: $(TOOLS_ROOT)/Makefile
	@echo '>>> Building development tools:'
	$(MAKE) PBROOT=$(TOOLS_ROOT) PLATFORM=$(HOST_PLATFORM) -C $(TOOLS_ROOT) devtools/devtools_to_build_for_$(TARGET_PLATFORM)_on_$(HOST_PLATFORM_SUB)_$(HOST_PLATFORM)

-include $(TOOLS_ROOT)/Make.deps

# Restore PBROOT
override PBROOT := $(BUILDROOT)/$(BUILDTYPE)/$(PRODUCT)
export PBROOT
#########

$(PBROOT)/Make.deps: $(PBROOT)/Makefile

$(PBROOT)/Makefile: Makefile make/makegen.py make/makegen/*.py
	@echo '>>> Regenerating Makefile for $(BUILDTYPE)/$(PRODUCT):'
	mkdir -p $(PBROOT)
	$(call setupCmdLineEnviron) python -tt make/makegen.py --outdir $(PBROOT)

$(TOOLS_ROOT)/Makefile: Makefile make/makegen.py make/makegen/*.py
	@echo '>>> Regenerating devtools Makefile:'
	mkdir -p $(TOOLS_ROOT)
	PLATFORM=$(HOST_PLATFORM) python -tt make/makegen.py --startdir devtools --outdir $(TOOLS_ROOT)
