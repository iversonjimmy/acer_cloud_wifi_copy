override MY_TEST_NAME=vssExpireTest

# makefile included by all test makefiles
include ../make/test_env.mk
sinclude ../make/$(PRODUCT).makedefs

# derived

export IDT_TOOLS = $(SRC_SWI)/tools/idt

# Make sure sw_i can be located.
ERROR_CHECK := $(shell test -d "$(SRC_SWI)" && echo OK)
ifneq ($(ERROR_CHECK),OK)
$(error The sw_i source tree could not be found at $(SRC_SWI).\
 This test requires sw_i to be checked-out.\
 You must override SRC_SWI if your sw_i and sw_x trees are not siblings)
endif

CCD_TEST_ACCOUNT ?= defaultVsTester@igware.com

.PHONY: default setup cleanup runtests 

default:  
ifeq ($(strip $(wildcard $(WORKAREA)/sw_x/tests/vs/Makefile)),)
	$(error "can't locate sw_x/tests/vs/Makefile - current directory is $(shell pwd)")
endif
	$(MAKE) setup
	$(MAKE) runtests
	$(MAKE) cleanup

setup: cleanup
ifdef DEVELOP_BUILD_DEPS
	$(MAKE) -C $(WORKAREA)/sw_x \
		gvm_core/internal/vssi/tests/vs/vsTest \
	# end of list
endif
	-mkdir -p $(LOGDIR)
	cd $(IDT_TOOLS) && \
		./createUser -i ids.$(INTERNAL_LAB_DOMAIN_NAME) -u $(CCD_TEST_ACCOUNT) -p password

runtests: 
	$(MAKE) vsTest

vsTest:
	-$(CAPTURE_CORE) && $(VALGRIND) $(TESTROOT)/gvm_core/internal/vssi/tests/vss_auto_expire/vssExpireTest -u $(CCD_TEST_ACCOUNT) -p password \
		--lab-name www-c100.$(LAB_DOMAIN_NAME) &> $(LOGDIR)/1_test.log
	-grep $(GREP_TEST_OUTPUT_REGEXP) $(LOGDIR)/1_test.log

cleanup: 
	-killall vsExpireTest
	-cd $(IDT_TOOLS) && \
		./deleteUser -i ids.$(INTERNAL_LAB_DOMAIN_NAME) -u $(CCD_TEST_ACCOUNT)

sinclude ../make/$(PRODUCT).makerules

