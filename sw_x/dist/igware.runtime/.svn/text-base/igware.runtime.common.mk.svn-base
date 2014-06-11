# Definitions shared by all of the different igware.runtime PRODUCTs.

ifndef PRODUCT
$(error PRODUCT was not defined yet!)
endif

ifeq ($(HOST_PLATFORM),cygwin)
override AS_UNIX_PATH = $(shell cygpath -u '$1')
else
override AS_UNIX_PATH = $1
endif

export WORKDIR := $(shell while [ ! -e sw_x -a `pwd` != '/' ]; do cd .. ; done; pwd)
ifeq ($(WORKDIR),/)
$(error This file must be checked-out beneath sw_x)
endif

ifdef BUILDROOT
override BUILDROOT := $(call AS_UNIX_PATH,$(BUILDROOT))
else
BUILDROOT := $(WORKDIR)/buildroot
endif
export BUILDROOT
$(info BUILDROOT="$(BUILDROOT)")

ifdef PREBUILT_TOOLS
override PREBUILT_TOOLS := $(call AS_UNIX_PATH,$(PREBUILT_TOOLS))
else
PREBUILT_TOOLS := $(HOME)/prebuilt_tools/$(PRODUCT)
endif
export PREBUILT_TOOLS

export TMPDIR := $(BUILDROOT)/tmp

# Fix the quotes in GCC error messages.
export LC_CTYPE=C

PRODUCT_VERSION_NUMBER_FILE := product.version.number
BUILD_NUMBER_FILE           := product.build.number
PRODUCT_VERSION_NUMBER      := $(shell cat $(PRODUCT_VERSION_NUMBER_FILE))
BUILD_NUMBER                 = $(shell cat $(BUILD_NUMBER_FILE))
BUILD_VERSION                = $(PRODUCT_VERSION_NUMBER).$(BUILD_NUMBER)


.PHONY: no_default
no_default:
	@echo "ERROR: Must specify an explicit make target!"
	@false
