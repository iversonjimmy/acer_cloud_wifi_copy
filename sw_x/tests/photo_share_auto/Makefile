override MY_TEST_NAME=photo_share_auto

# SDK RELEASE TESTS NOTE
# This test can run on Linux or Windows (on Cygwin), either remote or local
# - To run it locally:
#   make TARGET_USER=<local Linux/Windows username>
# - To run it remotely:
#   make TARGET_USER=<remote Linux/Windows username> TARGET_MACHINE=<remote machine name>
#
# By default, it downloads the dxshell package from the archive machine. Then it replaces the ccd and dxshell
# with the local copy built by the developer. When it runs on buildbot, it always gets the latest copy
# from the archive machine.
#
# To setup your machine for this test to run locally, you need to install cygwin and setup ssh
# Please follow the instruction here to set it up
# http://intwww.routefree.com/wiki/index.php/PersonalCloud_How_To_Setup_Windows_Test_Machine

# makefile included by all test makefiles
include ../make/test_env.mk

# Common definitions for tests.
include ../make/test_defs.mk

export CALL_EXTRA_PREPARE_ACCOUNT_TARGET=1

# Common test target rules
include ../make/test_rules.mk

# Specialization rules for this test suite.

.PHONY: sdk_release_basic
photo_share_auto:
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); \
            ./dxshell$(EXE) AutoTest PhotoShare $(LAB_DOMAIN_NAME)          \
                                     --acct1 $(CCD_TEST_ACCOUNT),password   \
                                     --acct2 $(CCD_TEST_ACCOUNT)_2,password \
                                     --acct3 $(CCD_TEST_ACCOUNT)_3,password'

.PHONY: extra_prepare_account
extra_prepare_account:
	-cd $(IDT_TOOLS) && \
		./deleteUser -i ids.$(INTERNAL_LAB_DOMAIN_NAME) -u $(CCD_TEST_ACCOUNT)_2
	@date +"%Y/%m/%d %k:%M:%S.%N"
	cd $(IDT_TOOLS) && \
		./createUser -i ids.$(INTERNAL_LAB_DOMAIN_NAME) -u $(CCD_TEST_ACCOUNT)_2 -p password
	@date +"%Y/%m/%d %k:%M:%S.%N"
	-cd $(IDT_TOOLS) && \
		./deleteUser -i ids.$(INTERNAL_LAB_DOMAIN_NAME) -u $(CCD_TEST_ACCOUNT)_3
	@date +"%Y/%m/%d %k:%M:%S.%N"
	cd $(IDT_TOOLS) && \
		./createUser -i ids.$(INTERNAL_LAB_DOMAIN_NAME) -u $(CCD_TEST_ACCOUNT)_3 -p password
	@date +"%Y/%m/%d %k:%M:%S.%N"

.PHONY: extra_cloudpc_config
extra_cloudpc_config:
	echo "extra_cloudpc_config null step"

.PHONY: extra_md_config
extra_md_config:
	echo "extra_md_config null step"

.PHONY: extra_client_config
extra_client_config:
	echo "extra_client_config null step"

.PHONY: prepare_test_data
prepare_test_data:

.PHONY: clean_test_data
clean_test_data:

