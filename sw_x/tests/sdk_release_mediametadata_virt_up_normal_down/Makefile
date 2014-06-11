override MY_TEST_NAME=sdk_release_mediametadata

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

# Common test target rules
include ../make/test_rules.mk

# Specialization rules for this test suite.
CCDCONF_ENABLE_UPLOAD_VIRTUAL_SYNC ?= 2
CCDCONF_ENABLE_ARCHIVE_DOWNLOAD ?= 1
CCDCONF_ENABLETS ?= 13
CCDCONF_CLEARFIMODE ?= 14

# --mode-virtual-up-archive-down:mediaMetadataUploadVirtualSync = 2, mediaMetadataSyncDownloadFromArchiveDevice = 1.
#		1. Pure virtual sync (post to VCS only).  For testing only.
#		2. Upload without actually uploading the content.  Depending on being able
#		   to access the content directly on the cloud pc.
#   	3. enables download directly from cloudPC rather than through ACS.
# --mode-hybrid-up-archive-down: mediaMetadataUploadVirtualSync = 1, mediaMetadataSyncDownloadFromArchiveDevice = 1.
#		1. Hybrid sync (post to VCS, then upload file, then update VCS).
#	    2. Upload without actually uploading the content.  Depending on being able
#    	   to access the content directly on the cloud pc.
#       3. enables download directly from cloudPC rather than through ACS
# --mode-hybrid-up-normal-down: mediaMetadataUploadVirtualSync = 1, mediaMetadataSyncDownloadFromArchiveDevice = 0.
#		1. Hybrid sync (post to VCS, then upload file, then update VCS).
#       2. Upload without actually uploading the content.  Depending on being able
#          to access the content directly on the cloud pc.
#       3. disables download directly from cloudPC, ensures "normal down" from ACS

.PHONY: sdk_release_mediametadata
sdk_release_mediametadata:
ifdef RUN_FULL_TEST
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); ./dxshell$(EXE) AutoTest SdkMediaMetadataRelease $(LAB_DOMAIN_NAME) $(CCD_TEST_ACCOUNT) password -f'
else
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); ./dxshell$(EXE) AutoTest SdkMediaMetadataRelease $(LAB_DOMAIN_NAME) $(CCD_TEST_ACCOUNT) password'
endif

.PHONY: extra_cloudpc_config
extra_cloudpc_config:
	$(REMOTE_RUN) "cd $(TARGET_TESTROOT); ./dxshell$(EXE) -m $(CLOUDPC_ALIAS) CCDConfig Set mediaMetadataUploadVirtualSync $(CCDCONF_ENABLE_UPLOAD_VIRTUAL_SYNC)"
	$(REMOTE_RUN) "cd $(TARGET_TESTROOT); ./dxshell$(EXE) -m $(CLOUDPC_ALIAS) CCDConfig Set mediaMetadataSyncDownloadFromArchiveDevice $(CCDCONF_ENABLE_ARCHIVE_DOWNLOAD)"
	$(REMOTE_RUN) "cd $(TARGET_TESTROOT); ./dxshell$(EXE) -m $(CLOUDPC_ALIAS) CCDConfig Set enableTs $(CCDCONF_ENABLETS)"

.PHONY: extra_md_config
extra_md_config:
	$(REMOTE_RUN) "cd $(TARGET_TESTROOT); ./dxshell$(EXE) -m $(MD_ALIAS) CCDConfig Set mediaMetadataUploadVirtualSync $(CCDCONF_ENABLE_UPLOAD_VIRTUAL_SYNC)"
	$(REMOTE_RUN) "cd $(TARGET_TESTROOT); ./dxshell$(EXE) -m $(MD_ALIAS) CCDConfig Set mediaMetadataSyncDownloadFromArchiveDevice $(CCDCONF_ENABLE_ARCHIVE_DOWNLOAD)"
	$(REMOTE_RUN) "cd $(TARGET_TESTROOT); ./dxshell$(EXE) -m $(MD_ALIAS) CCDConfig Set enableTs $(CCDCONF_ENABLETS)"
	$(REMOTE_RUN) "cd $(TARGET_TESTROOT); ./dxshell$(EXE) -m $(MD_ALIAS) CCDConfig Set clearfiMode $(CCDCONF_CLEARFIMODE)"

.PHONY: extra_client_config
extra_client_config:
	$(REMOTE_RUN) "cd $(TARGET_TESTROOT); ./dxshell$(EXE) -m $(CLIENT_ALIAS) CCDConfig Set mediaMetadataUploadVirtualSync $(CCDCONF_ENABLE_UPLOAD_VIRTUAL_SYNC)"
	$(REMOTE_RUN) "cd $(TARGET_TESTROOT); ./dxshell$(EXE) -m $(CLIENT_ALIAS) CCDConfig Set mediaMetadataSyncDownloadFromArchiveDevice $(CCDCONF_ENABLE_ARCHIVE_DOWNLOAD)"
	$(REMOTE_RUN) "cd $(TARGET_TESTROOT); ./dxshell$(EXE) -m $(CLIENT_ALIAS) CCDConfig Set enableTs $(CCDCONF_ENABLETS)"

.PHONY: prepare_test_data
prepare_test_data:

.PHONY: clean_test_data
clean_test_data:
