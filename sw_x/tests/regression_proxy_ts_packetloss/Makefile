override MY_TEST_NAME=regression_proxy_ts_packetloss

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

# Need Client only when running cloudnode test.
ifdef TEST_TARGET_IS_CLOUDNODE
# just a dummy expression for syntax
override TEST_TARGET_IS_CLOUDNODE=1
else
ifeq ($(PRODUCT), cloudnode)
# another dummy expression for syntax
override TEST_TARGET_IS_CLOUDNODE=1
else
# Not cloudnode or cloudnode_linux. No Client needed.
export NO_CLIENT_DEVICE=1
endif
endif

# makefile included by all test makefiles
include ../make/test_env.mk

# Common definitions for tests.
include ../make/test_defs.mk

# Common test target rules
include ../make/test_rules.mk

# Specialization rules for this test suite.

CCDCONF_ENABLETS ?= 13
CCDCONF_CLEARFIMODE ?= 7

# enable TS packet loss and pass loss rate is about 2% (dropping 1 packet every 50 packets)
CCDCONF_ENABLEPACKETDROP ?= 1
CCDCONF_PACKETDROPRATE ?= 50

.PHONY: regression_proxy_ts_packetloss
regression_proxy_ts_packetloss:
ifdef RUN_FULL_TEST
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); ./dxshell$(EXE) AutoTest RegressionStreamingProxy $(LAB_DOMAIN_NAME) $(CCD_TEST_ACCOUNT) password 10 10'
else
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); ./dxshell$(EXE) AutoTest RegressionStreamingProxy $(LAB_DOMAIN_NAME) $(CCD_TEST_ACCOUNT) password 4 4'
endif

.PHONY: extra_cloudpc_config
extra_cloudpc_config:
	$(REMOTE_RUN) "cd $(TARGET_TESTROOT); ./dxshell$(EXE) -m $(CLOUDPC_ALIAS) CCDConfig Set enableTs $(CCDCONF_ENABLETS)"
	$(REMOTE_RUN) "cd $(TARGET_TESTROOT); ./dxshell$(EXE) -m $(CLOUDPC_ALIAS) CCDConfig Set ts2DevAidParam $(CCDCONF_ENABLEPACKETDROP)"
	$(REMOTE_RUN) "cd $(TARGET_TESTROOT); ./dxshell$(EXE) -m $(CLOUDPC_ALIAS) CCDConfig Set ts2PktDropParam $(CCDCONF_PACKETDROPRATE)"


.PHONY: extra_md_config
extra_md_config:
	$(REMOTE_RUN) "cd $(TARGET_TESTROOT); ./dxshell$(EXE) -m $(MD_ALIAS) CCDConfig Set enableTs $(CCDCONF_ENABLETS)"
	$(REMOTE_RUN) "cd $(TARGET_TESTROOT); ./dxshell$(EXE) -m $(MD_ALIAS) CCDConfig Set clearfiMode $(CCDCONF_CLEARFIMODE)"
	$(REMOTE_RUN) "cd $(TARGET_TESTROOT); ./dxshell$(EXE) -m $(MD_ALIAS) CCDConfig Set ts2DevAidParam $(CCDCONF_ENABLEPACKETDROP)"
	$(REMOTE_RUN) "cd $(TARGET_TESTROOT); ./dxshell$(EXE) -m $(MD_ALIAS) CCDConfig Set ts2PktDropParam $(CCDCONF_PACKETDROPRATE)"

.PHONY: extra_client_config
extra_client_config:
	$(REMOTE_RUN) "cd $(TARGET_TESTROOT); ./dxshell$(EXE) -m $(CLIENT_ALIAS) CCDConfig Set enableTs $(CCDCONF_ENABLETS)"
	$(REMOTE_RUN) "cd $(TARGET_TESTROOT); ./dxshell$(EXE) -m $(CLIENT_ALIAS) CCDConfig Set ts2DevAidParam $(CCDCONF_ENABLEPACKETDROP)"
	$(REMOTE_RUN) "cd $(TARGET_TESTROOT); ./dxshell$(EXE) -m $(CLIENT_ALIAS) CCDConfig Set ts2PktDropParam $(CCDCONF_PACKETDROPRATE)"

.PHONY: prepare_test_data
prepare_test_data:

.PHONY: clean_test_data
clean_test_data:

