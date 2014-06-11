override MY_TEST_NAME=sdk_release_transcode

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

export NO_CLIENT_DEVICE=1

# makefile included by all test makefiles
include ../make/test_env.mk

# Common definitions for tests.
include ../make/test_defs.mk
 
# Common test target rules
include ../make/test_rules.mk
 
# Specialization rules for this test suite.
CCDCONF_ENABLETS ?= 13
CCDCONF_CLEARFIMODE ?= 14

ifeq ($(TESTPLAT),win32)
export TRANSCODE_TEST_DATA ?= /cygdrive/c/Users/$(TARGET_USER)/AppData/Local/dxshell/GoldenTest
export TRANSCODE_TESTDATADIR ?= pc/test_data/dxdata
endif
ifeq ($(TESTPLAT),cloudnode)
export TRANSCODE_TEST_DATA ?= $(TARGET_HOME)/dxshell_root/GoldenTest
export TRANSCODE_TESTDATADIR ?= pc/test_data/dxdata
endif

.PHONY: sdk_release_transcode
sdk_release_transcode:
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); ./dxshell$(EXE) AutoTest SdkTranscodeStreamingPositive $(LAB_DOMAIN_NAME) $(CCD_TEST_ACCOUNT) password 3'

.PHONY: extra_cloudpc_config
extra_cloudpc_config:
	$(REMOTE_RUN) "cd $(TARGET_TESTROOT); ./dxshell$(EXE) -m $(CLOUDPC_ALIAS) CCDConfig Set enableTs $(CCDCONF_ENABLETS)"
 
.PHONY: extra_md_config
extra_md_config:
	$(REMOTE_RUN) "cd $(TARGET_TESTROOT); ./dxshell$(EXE) -m $(MD_ALIAS) CCDConfig Set enableTs $(CCDCONF_ENABLETS)"
	$(REMOTE_RUN) "cd $(TARGET_TESTROOT); ./dxshell$(EXE) -m $(MD_ALIAS) CCDConfig Set clearfiMode $(CCDCONF_CLEARFIMODE)"

.PHONY: extra_client_config
extra_client_config:
	$(REMOTE_RUN) "cd $(TARGET_TESTROOT); ./dxshell$(EXE) -m $(CLIENT_ALIAS) CCDConfig Set enableTs $(CCDCONF_ENABLETS)"

.PHONY: prepare_test_data
prepare_test_data:
ifeq ($(TESTPLAT), win32)
	$(REMOTE_RUN) 'if [ ! -d $(TRANSCODE_TEST_DATA)/image_transcode_positive_tests ]; then mkdir -p $(TRANSCODE_TEST_DATA); cd $(TRANSCODE_TEST_DATA); wget http://$(STORE_HOST):$(HTTP_PORT)/$(TRANSCODE_TESTDATADIR)/image_transcode_positive_tests.tar.gz; tar -zxvf image_transcode_positive_tests.tar.gz; fi'
	$(REMOTE_RUN) 'if [ ! -d $(TRANSCODE_TEST_DATA)/image_transcode_negative_test_1 ]; then mkdir -p $(TRANSCODE_TEST_DATA)/image_transcode_negative_test_1; cd $(TRANSCODE_TEST_DATA)/image_transcode_negative_test_1; wget http://$(STORE_HOST):$(HTTP_PORT)/$(TRANSCODE_TESTDATADIR)/test_invalid.jpg; fi'
else
ifeq ($(TESTPLAT), cloudnode)
	$(REMOTE_RUN) 'if [ -d $(TRANSCODE_TEST_DATA)/image_transcode_positive_tests.tar.gz ]; then rm -rf $(TRANSCODE_TEST_DATA)/image_transcode_positive_tests*; fi'
	$(REMOTE_RUN) 'if [ ! -d $(TRANSCODE_TEST_DATA)/image_transcode_positive_tests.tar.gz ]; then mkdir -p $(TRANSCODE_TEST_DATA); cd $(TRANSCODE_TEST_DATA); wget http://$(STORE_HOST):$(HTTP_PORT)/$(TRANSCODE_TESTDATADIR)/image_transcode_positive_tests.tar.gz; tar -zxvf image_transcode_positive_tests.tar.gz; fi'
	$(REMOTE_RUN) 'if [ -d $(TRANSCODE_TEST_DATA)/image_transcode_negative_test_1 ]; then rm -rf $(TRANSCODE_TEST_DATA)/image_transcode_negative_test_1*; fi'
	$(REMOTE_RUN) 'if [ ! -d $(TRANSCODE_TEST_DATA)/image_transcode_negative_test_1 ]; then mkdir -p $(TRANSCODE_TEST_DATA)/image_transcode_negative_test_1; cd $(TRANSCODE_TEST_DATA)/image_transcode_negative_test_1; wget http://$(STORE_HOST):$(HTTP_PORT)/$(TRANSCODE_TESTDATADIR)/test_invalid.jpg; fi'
endif
endif

.PHONY: clean_test_data
clean_test_data:
ifeq ($(TESTPLAT), cloudnode)
	-$(MD_REMOTE_RUN) 'rm -fr $(CCD_APPDATA_PATH_MD)/*'
endif
