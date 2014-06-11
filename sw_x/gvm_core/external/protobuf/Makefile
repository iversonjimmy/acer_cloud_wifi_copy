#
# Make rules common to all target platforms.
#
# Expects the following variables defined in the platform_$(PLATFORM).mk:
# PROTO_CONF_OPTS goes on the command line before configure
# PROTO_CONF_FLAGS goes on the command line of configure
#

# Note that this is "Platform Build Root" not "Protocol Buffer Root".
ifndef PBROOT
$(error Need to run this from a higher level Makefile)
endif

ifndef SRCROOT
$(error Need to run this from a higher level Makefile)
endif

include platform_$(PLATFORM).mk
include $(SRCROOT)/make/external_locations.mk

###
# TODO: refactor to somewhere shared?
ifeq ($(HOST_PLATFORM),mac)
WGET_TO_STDOUT = curl -s -S
else
WGET_TO_STDOUT = wget --progress=dot:mega -O - -q
endif
###

PROTOC ?= $(TOOLS_ROOT)/usr/bin/protoc

EXT_PKG_SRC ?= http://$(TOOL_HOST):$(HTTP_PORT)/$(TOOL_PATH)/third_party/platform_multi
PKG_PROTOBUF = protobuf-2.4.0a

# TODO get this programatically
MY_DIR = gvm_core/external/protobuf

INSTALL_DIR = $(PBROOT)/usr
PROTOBUF_DIR = $(PBROOT)/$(MY_DIR)/$(PKG_PROTOBUF)
PROTOBUF_BUILD = $(PROTOBUF_DIR)-build

PREPARE_TIMESTAMP = $(PBROOT)/$(MY_DIR)/prepare.timestamp
BUILD_PROTOBUF_TIMESTAMP = $(PBROOT)/$(MY_DIR)/build-protobuf.timestamp
BUILD_PROTOBUF_JAVA_TIMESTAMP = $(PBROOT)/$(MY_DIR)/build-protobuf-java.timestamp
BUILD_PROTOBUF_PYTHON_TIMESTAMP = $(PBROOT)/$(MY_DIR)/build-protobuf-python.timestamp
INSTALL_PROTOBUF_TIMESTAMP = $(INSTALL_DIR)/install-protobuf.timestamp

.PHONY: default
default: $(INSTALL_PROTOBUF_TIMESTAMP) $(BUILD_PROTOBUF_JAVA_TIMESTAMP) $(BUILD_PROTOBUF_PYTHON_TIMESTAMP)

EXTRA_FILE_DEPENDENCIES = \
	Makefile \
	zero-copy-impl.patch \
	platform_$(PLATFORM).mk

#
# The prepare step takes the external source, unpacks it,
# patches it up and prepares it for the usual
# 'configure, make, make install' flow.
#
$(PREPARE_TIMESTAMP): $(EXTRA_FILE_DEPENDENCIES)
	rm -rf $(PROTOBUF_DIR)
	@echo "==> Preparing $(PROTOBUF_DIR) build"
	mkdir -p $(PBROOT)/$(MY_DIR)
	cd $(PBROOT)/$(MY_DIR) && \
		$(WGET_TO_STDOUT) $(EXT_PKG_SRC)/$(PKG_PROTOBUF).tar.bz2 | bunzip2 | tar xvf -
	(cd $(PROTOBUF_DIR)/src && \
		patch -p1 < $(SRCROOT)/$(MY_DIR)/zero-copy-impl.patch)
	(cd $(PROTOBUF_DIR) && \
		patch -p1 < $(SRCROOT)/$(MY_DIR)/java1.5.patch)
	cp $(SRCROOT)/$(MY_DIR)/config.sub $(PROTOBUF_DIR)/
	cp $(SRCROOT)/$(MY_DIR)/config.sub $(PROTOBUF_DIR)/gtest/build-aux/
	touch $@

#
# The build-protobuf step does the 'configure, make'
# in the build output directory.
#
$(BUILD_PROTOBUF_TIMESTAMP): $(EXTRA_FILE_DEPENDENCIES) $(PREPARE_TIMESTAMP)
	rm -rf $(PROTOBUF_BUILD)
	mkdir -vp $(PROTOBUF_BUILD)
	mkdir -vp $(INSTALL_DIR)/lib
	@echo "==> Building $(PROTOBUF_BUILD)"
ifneq ($(PLATFORM),android)
	(cd $(PROTOBUF_BUILD) && \
		$(PROTO_CONF_OPTS) $(PROTOBUF_DIR)/configure \
			--prefix=$(INSTALL_DIR) \
			--with-zlib=no \
			$(PROTO_CONF_FLAGS) && \
		$(MAKE))
endif
	touch $@

$(PROTOC): $(INSTALL_PROTOBUF_TIMESTAMP)

# See $(PROTOBUF_DIR)/java/pom.xml for list of classes to include in 'lite' version.
$(BUILD_PROTOBUF_JAVA_TIMESTAMP): $(PROTOC)
	if [ '$(BUILD_JAVA_PROTOBUF)' != '' ]; then \
		$(PROTOC) \
			--java_out=$(PROTOBUF_DIR)/java/src/main/java \
			-I$(PROTOBUF_DIR)/src \
			$(PROTOBUF_DIR)/src/google/protobuf/descriptor.proto && \
		(cd $(PROTOBUF_DIR)/java/src/main/java && \
			javac -source 1.5 -target 1.5 -g com/google/protobuf/*.java && \
			jar cf protobuf.jar com/google/protobuf/*.class && \
			jar cf protobuf-lite.jar \
				com/google/protobuf/AbstractMessageLite*.class \
				com/google/protobuf/ByteString*.class \
				com/google/protobuf/CodedInputStream*.class \
				com/google/protobuf/CodedOutputStream*.class \
				com/google/protobuf/ExtensionRegistryLite*.class \
				com/google/protobuf/FieldSet*.class \
				com/google/protobuf/GeneratedMessageLite*.class \
				com/google/protobuf/Internal*.class \
				com/google/protobuf/InvalidProtocolBufferException*.class \
				com/google/protobuf/LazyStringArrayList*.class \
				com/google/protobuf/LazyStringList*.class \
				com/google/protobuf/MessageLite*.class \
				com/google/protobuf/SmallSortedMap*.class \
				com/google/protobuf/UninitializedMessageException*.class \
				com/google/protobuf/UnmodifiableLazyStringList*.class \
				com/google/protobuf/WireFormat*.class \
				) && \
		cp $(PROTOBUF_DIR)/java/src/main/java/protobuf.jar \
			$(INSTALL_DIR)/lib/; \
		cp $(PROTOBUF_DIR)/java/src/main/java/protobuf-lite.jar \
			$(INSTALL_DIR)/lib/; \
	fi
	touch $@

$(BUILD_PROTOBUF_PYTHON_TIMESTAMP): $(BUILD_PROTOBUF_TIMESTAMP)
	if [ '$(BUILD_PYTHON_PROTOBUF)' != '' ]; then \
		PATH="$(PROTOBUF_BUILD)/src:$(PATH)" ; \
		cd $(PROTOBUF_DIR)/python && $(PROTO_PYTHON) setup.py install --install-lib=$(PROTO_PYTHON_LIB); \
	fi
	touch $@

$(INSTALL_PROTOBUF_TIMESTAMP): $(EXTRA_FILE_DEPENDENCIES) $(BUILD_PROTOBUF_TIMESTAMP)
ifneq ($(PLATFORM),android)
	(cd $(PROTOBUF_BUILD) && $(MAKE) install);
endif
	touch $@

.PHONY: clean
clean:
	rm -rf $(PROTOBUF_DIR) \
		$(PROTOBUF_BUILD) \
		$(PREPARE_TIMESTAMP) \
		$(BUILD_PROTOBUF_TIMESTAMP) \
		$(BUILD_PROTOBUF_JAVA_TIMESTAMP) \
		$(BUILD_PROTOBUF_PYTHON_TIMESTAMP) \
		$(INSTALL_PROTOBUF_TIMESTAMP) \
