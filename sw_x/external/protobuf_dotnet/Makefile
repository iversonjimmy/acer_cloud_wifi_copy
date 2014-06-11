ifndef PBROOT
$(error Need to run this from a higher level Makefile)
endif

ifndef SRCROOT
$(error Need to run this from a higher level Makefile)
endif

include $(SRCROOT)/make/external_locations.mk

EXT_PKG_SRC ?= http://$(TOOL_HOST):$(HTTP_PORT)/$(TOOL_PATH)/third_party/platform_win32
PROTOBUF_DOTNET_FILE = protobuf.net30.r282.zip
PROTOBUF_DOTNET_SUBDIR = protobufnet.30.r282

# TODO get this programatically
MY_DIR = external/protobuf_dotnet

DEST_DIR = $(PBROOT)/$(MY_DIR)

EXTRACT_TIMESTAMP = $(DEST_DIR)/extracted.stamp

.PHONY: default
default: $(EXTRACT_TIMESTAMP) 

#
# The prepare step takes the external source, unpacks it,
# patches it up and prepares it for the usual
# 'configure, make, make install' flow.
#
$(EXTRACT_TIMESTAMP): Makefile
	rm -rf $(DEST_DIR)
	mkdir -p $(DEST_DIR)
	cd $(DEST_DIR) && \
		wget --progress=dot:mega $(EXT_PKG_SRC)/$(PROTOBUF_DOTNET_FILE) \
			-O $(DEST_DIR)/$(PROTOBUF_DOTNET_FILE) && \
		unzip $(DEST_DIR)/$(PROTOBUF_DOTNET_FILE)
	mv $(DEST_DIR)/$(PROTOBUF_DOTNET_SUBDIR) $(DEST_DIR)/protobuf
	chmod u+x $(DEST_DIR)/protobuf/*.dll
	chmod u+x $(DEST_DIR)/protobuf/*.exe
	rm -f $(DEST_DIR)/$(PROTOBUF_DOTNET_FILE)
	echo "$(PROTOBUF_DOTNET_FILE)" > $@
