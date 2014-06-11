MY_TEST_NAME ?= swupdate

export PRODUCT ?= linux
export USE_VALGRIND ?= 0
export TC ?= A

export VALGRIND_SUPP ?= $(CURDIR)/../ccd/valgrind.supp

# makefile included by all test makefiles
include ../make/test_env.mk
sinclude ../make/$(PRODUCT).makedefs


# derived
override CCD=$(TESTROOT)/gvm_core/daemons/ccd/ccd
override WAIT_FOR_CCD=$(TESTROOT)/gvm_core/daemons/ccd/tests/ccd_ready_test
export CCD_TEST_ACCOUNT ?= swupdateTester@igware.com
export IDT_TOOLS = $(SRC_SWI)/tools/idt

override C1_ROOT=$(TEST_WORKDIR)/c1Root
CLOUD_DIR=$(C1_ROOT)/Cloud

.PHONY: default setup cleanup runtests

default:  
ifeq ($(strip $(wildcard $(WORKAREA)/sw_x/tests/swupdate/Makefile)),)
	$(error "can't locate sw_x/swupdate/Makefile - current directory is $(shell pwd)")
endif
	$(MAKE) setup
	-$(MAKE) runtests
	$(MAKE) cleanup

setup:
	@if [ -d $(IDT_TOOLS) ]; then \
		echo 'IDT tool detected'; \
	else \
		echo 'IDT tool does not exist, check out the $(IDT_TOOLS)'; \
		false; \
	fi 
	-mkdir -p $(LOGDIR)
	-cd $(IDT_TOOLS) && \
		./deleteUser -i ids.$(INTERNAL_LAB_DOMAIN_NAME) -u $(CCD_TEST_ACCOUNT)
	cd $(IDT_TOOLS) && \
		./createUser -i ids.$(INTERNAL_LAB_DOMAIN_NAME) -u $(CCD_TEST_ACCOUNT) -p password
	-rm -fr $(LOGDIR)/*
	-rm -fr $(CLOUD_DIR)/*
	mkdir -p $(C1_ROOT)/conf
	export SRCROOT=$(SRC_SWX) ; $(SRC_SWX)/tests/tools/actool.sh $(LAB_DOMAIN_NAME) $(C1_ROOT) &> $(LOGDIR)/1_actool.log
	sleep 1

test:
	$(CAPTURE_CORE) && $(VALGRIND) $(CCD) $(C1_ROOT) &>> $(LOGDIR)/2_bg_ccd.log &
	$(CAPTURE_CORE) && $(WAIT_FOR_CCD) &>> $(LOGDIR)/3_wait_for_ccd.log # Allow ccd to start up
	../swupdate/syncWhitebox.py \
        --case $(TC) \
        --testName $(MY_TEST_NAME) \
        --cloudRoot $(CLOUD_DIR) \
        --c1Root $(C1_ROOT) \
        --testLog $(LOGDIR) \
        --account $(CCD_TEST_ACCOUNT) \
	echo "test case $(TC) complete."

runtests: 
	$(MAKE) test TC=A MY_TEST_NAME=$(MY_TEST_NAME)
	$(MAKE) test TC=B MY_TEST_NAME=$(MY_TEST_NAME)

cleanup: 
ifeq ($(USE_VALGRIND), 0)
	killall $(CCD) 2>&1 || true
else
	ps -o pid,cmd | grep valgrind | grep -v grep| cut -c1-6 | $(XARGS_CMDLINE) kill
endif
	-killall ccdSync &> /dev/null 2>&1
	-killall ccdLogin &> /dev/null 2>&1
	-killall ccdLogout &> /dev/null 2>&1
	-killall genSyncTest &> /dev/null 2>&1
	-killall swuTest &> /dev/null 2>&1
	-cd $(IDT_TOOLS) && \
		./deleteUser -i ids.$(INTERNAL_LAB_DOMAIN_NAME) -u $(CCD_TEST_ACCOUNT)

sinclude ../make/$(PRODUCT).makerules
