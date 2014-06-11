##########################################################################
# Global defs referencing any external servers or storage go here.
##########################################################################


# Port to use when retrieving data over HTTP
HTTP_PORT = 8001


# Used when accessing the server that stores built outputs
# When using wget, the path would be:
#   wget http://$(STORE_HOST):$(HTTP_PORT)/$(STORE_PATH)/third_party/platform_linux/curl-7.20.0.tar.gz .
# When using scp, the path would be:
#   scp $(STORE_USER)@$(STORE_HOST):$(FS_STORE_PATH)/third_party/platform_linux/curl-7.20.0.tar.gz .
STORE_USER ?= build
STORE_HOST ?= pcstore.ctbg.acer.com
ifdef IS_MTV_BUILD
    STORE_PATH ?= pc/builds_by_MTV
else
    STORE_PATH ?= pc/builds_by_TPE
endif
FS_STORE_PATH := a/$(STORE_PATH)
FS_STORE_TEST_PATH ?= a/pc/test_outputs


# FS_PC_INFRA_STORE_PATH is for the few infra things we keep on the host that CTBG can see
FS_PC_INFRA_STORE_PATH ?= /a/pc/builds_by_MTV/$(BRANCH_NAME)/infra


# We have occasion to put/get stuff on/from the infra store host, which is different than the client store host
INFRA_STORE_USER ?= build
INFRA_STORE_HOST ?= gvmstore.routefree.com
INFRA_STORE_PATH ?= gvm/builds
FS_INFRA_STORE_PATH := a/$(INFRA_STORE_PATH)
FS_INFRA_STORE_TEST_PATH ?= a/gvm/test_outputs


# Used when retrieving test data stored outside of the source tree.
# Typically retrieved using wget; such as:
#   wget http://$(TESTDATA_HOST):$(HTTP_PORT)/$(TESTDATA_PATH)/android/bvd_test_data.tar
# When using scp, the path would be:
#   scp $(TESTDATA_USER)@$(TESTDATA_HOST):$(FS_TESTDATA_PATH)/android/bvd_test_data.tar
TESTDATA_USER ?= build
TESTDATA_HOST ?= pcstore.ctbg.acer.com
TESTDATA_PATH ?= pc/test_data
FS_TESTDATA_PATH := a/$(TESTDATA_PATH)


# Used when retrieving tools or third-party packages stored outside of the source tree.
# Typically retrieved using wget; such as:
#   wget http://$(TOOL_HOST):$(HTTP_PORT)/$(TOOL_PATH)/third_party/platform_linux/curl-7.20.0.tar.gz .
# When using scp, the path would be:
#   scp $(TOOL_USER)@$(TOOL_HOST):$(FS_TOOL_PATH)/third_party/platform_linux/curl-7.20.0.tar.gz .
TOOL_USER ?= build
TOOL_HOST ?= pcstore.ctbg.acer.com
TOOL_PATH ?= pc/tools
TAGEDIT_TOOL_PATH ?= pc/pc_tests/tools
FS_TOOL_PATH := a/$(TOOL_PATH)
