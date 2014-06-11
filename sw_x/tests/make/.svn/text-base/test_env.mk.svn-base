# Parts shared by all of the test Makefiles.

override WORKAREA := $(shell while [ ! -e sw_x -a `pwd` != '/' ]; do cd .. ; done; pwd)
ifeq ($(WORKAREA),/)
$(error This file must be checked-out beneath sw_x)
endif
$(info WORKAREA is "$(WORKAREA)")
export WORKAREA
export SRCROOT ?= $(WORKAREA)/sw_x

ifndef MY_TEST_NAME
$(error Please define MY_TEST_NAME before including this file)
endif

ifndef PRODUCT
$(error Please define PRODUCT before including this file)
endif

SHELL=/bin/bash

# -r is a GNU extension t
XARGS_CMDLINE = xargs -r

# for the purposes of running tests, set IS_MTV_BUILD=1 so that
# all items that are tested are retrieved from the builds_by_MTV directory
# normally, default for builds is that it goes to the builds_by_TPE directory
IS_MTV_BUILD ?= 1

# get all the external location info
include $(WORKAREA)/sw_x/make/external_locations.mk

########
# Can override by env variable or commandline arg:
########
export SRC_SWI ?= $(WORKAREA)/sw_i
export SRC_SWX ?= $(WORKAREA)/sw_x
export BUILDROOT ?= $(WORKAREA)/buildroot
export USE_VALGRIND ?= 0

#
# Supported configs:
#
# <default> => lab18 (pc-int.igware.net)
# "LAB=lab6 LAB_DOMAIN_NAME=pc-dev.igware.net" => lab6
# "LAB=lab10 LAB_DOMAIN_NAME=pc-test.igware.net" => lab10 (currently fails if test machine is not Acer device)
#
# You can set LAB in your environment, but please don't directly use $(LAB) from within the test Makefiles.
LAB ?= lab18
export INTERNAL_LAB_DOMAIN_NAME ?= $(LAB).routefree.com
export LAB_DOMAIN_NAME ?= pc-int.igware.net

########
# Derived stuff:
########

ifdef DEBUG
export BUILDTYPE := debug
else
export BUILDTYPE := release
endif

# Get PLATFORM from PRODUCT
include $(SRC_SWX)/make/product_defs.mk
$(eval $(call setParamsFromProduct,$(PRODUCT),))

# get ANDROID_SDK_ROOT
include $(SRC_SWX)/devtools/android/android_env_defs.mk

# This is the same as the PBROOT for a build machine.
override TESTROOT=$(BUILDROOT)/$(BUILDTYPE)/$(PRODUCT)
override PBROOT=$(BUILDROOT)/$(BUILDTYPE)/$(PRODUCT)

# The suite-specific directory specified by the test harness.
TEST_WORKDIR ?= $(TESTROOT)/test_log/$(MY_TEST_NAME)
TEST_TEMPDIR ?= $(TESTROOT)/test_temp/$(MY_TEST_NAME)

override LOGDIR=$(TEST_WORKDIR)

VALGRIND_SUPP ?= $(CURDIR)/valgrind.supp

VALGRIND_LOGFILE = $(LOGDIR)/valgrind.log

ifeq ($(shell uname -o),Cygwin)
export PRODUCT = win32_ul
override USE_VALGRIND = 0
else
ifeq ($(PRODUCT),cloudnode)
override USE_VALGRIND = 0
else
export PRODUCT ?= linux
export USE_VALGRIND ?= 1
endif
endif

ifeq ($(USE_VALGRIND), 0)
override VALGRIND=
else ifeq ($(USE_VALGRIND), 1)
override VALGRIND=CCD_FULL_SHUTDOWN=1 valgrind -v --log-file=$(VALGRIND_LOGFILE) --suppressions=$(VALGRIND_SUPP) --gen-suppressions=all --leak-check=full --track-origins=yes
else
# TODO valgrind.supp should be potentially shared among multiple tests.
override VALGRIND=CCD_FULL_SHUTDOWN=1 valgrind -v --tool=$(USE_VALGRIND) --log-file=$(VALGRIND_LOGFILE) --suppressions=$(VALGRIND_SUPP) --gen-suppressions=all
endif

ifeq ($(BUILD_BRANCH), )
export TEST_BRANCH=ACT_SDK_TOT
else
export TEST_BRANCH=$(BUILD_BRANCH)
endif

# Run the test output through this expression to print just the summary information.
# It will also highlight important parts if GREP_COLOR is configured in your environment.
# Uses "TC.RESULT" so that the testharness doesn't try to parse the command line.
GREP_TEST_OUTPUT_REGEXP = '^.*\(TEST FAILED\|FAILED CHECK\|FAILED ENSURE\|FAILED\|TC.RESULT\)'

CAPTURE_CORE = cd $(TEST_WORKDIR) && ulimit -c unlimited

# Make sure sw_i can be located.
ERROR_CHECK := $(shell test -d "$(SRC_SWI)" && echo OK)
ifneq ($(ERROR_CHECK),OK)
$(error The sw_i source tree could not be found at $(SRC_SWI).\
 This test requires sw_i to be checked-out.\
 You must override SRC_SWI if your sw_i and sw_x trees are not siblings)
endif

########
