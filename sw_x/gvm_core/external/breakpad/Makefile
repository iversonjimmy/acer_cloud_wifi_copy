#
# Make rules common to all target platforms.
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

EXT_PKG_SRC ?= http://$(TOOL_HOST):$(HTTP_PORT)/$(TOOL_PATH)/third_party/platform_multi
PKG_BREAKPAD = google-breakpad-read-only

# TODO get this programatically
MY_DIR = gvm_core/external/breakpad

INSTALL_DIR = $(PBROOT)/usr
BREAKPAD_DIR = $(PBROOT)/$(MY_DIR)/$(PKG_BREAKPAD)

PREPARE_TIMESTAMP = $(PBROOT)/$(MY_DIR)/prepare.timestamp
BUILD_BREAKPAD_TIMESTAMP = $(PBROOT)/$(MY_DIR)/build-breakpad.timestamp
INSTALL_BREAKPAD_TIMESTAMP = $(INSTALL_DIR)/install-breakpad.timestamp

.PHONY: default
default: $(INSTALL_BREAKPAD_TIMESTAMP) 

EXTRA_FILE_DEPENDENCIES = Makefile 

#
# The prepare step takes the external source, unpacks it,
# patches it up and prepares it for the usual
# 'configure, make, make install' flow.
#
$(PREPARE_TIMESTAMP): $(EXTRA_FILE_DEPENDENCIES)
	make prepare_$(PLATFORM)
	touch $@

#
# The build-breakpad step does the 'configure, make'
# in the build output directory.
#
$(BUILD_BREAKPAD_TIMESTAMP): $(EXTRA_FILE_DEPENDENCIES) $(PREPARE_TIMESTAMP)
	make build_$(PLATFORM)
	touch $@

$(INSTALL_BREAKPAD_TIMESTAMP): $(EXTRA_FILE_DEPENDENCIES) $(BUILD_BREAKPAD_TIMESTAMP)
	make install_$(PLATFORM)
	touch $@

prepare_win: $(EXTRA_FILE_DEPENDENCIES) platform_win.mk 

prepare_android: $(EXTRA_FILE_DEPENDENCIES) platform_android.mk breakpad_android.patch
	rm -rf $(PBROOT)/$(MY_DIR)/*
	@echo "==> Preparing $(BREAKPAD_DIR) build"
	mkdir -p $(PBROOT)/$(MY_DIR)
	cd $(PBROOT)/$(MY_DIR) && \
		wget --progress=dot:mega $(EXT_PKG_SRC)/$(PKG_BREAKPAD).tar.gz -O - -q | tar xzvf -
	cd $(BREAKPAD_DIR) && \
		patch -p1 < $(CURDIR)/breakpad_android.patch
	files="$@ $(BREAKPAD_DIR)"; $(CVSIGNORE)

prepare_linux: $(EXTRA_FILE_DEPENDENCIES) platform_linux.mk
	rm -rf $(PBROOT)/$(MY_DIR)/*
	@echo "==> Preparing $(BREAKPAD_DIR) build"
	mkdir -p $(PBROOT)/$(MY_DIR)
	cd $(PBROOT)/$(MY_DIR) && \
		wget --progress=dot:mega $(EXT_PKG_SRC)/$(PKG_BREAKPAD).tar.gz -O - -q | tar xzvf -
	files="$@ $(BREAKPAD_DIR)"; $(CVSIGNORE)

build_win: $(EXTRA_FILE_DEPENDENCIES) $(PREPARE_TIMESTAMP)
	mkdir -vp $(INSTALL_DIR)/lib
	mkdir -vp $(INSTALL_DIR)/include
	touch $@

build_android: $(EXTRA_FILE_DEPENDENCIES) $(PREPARE_TIMESTAMP)
	mkdir -vp $(INSTALL_DIR)/lib
	@echo "==> Building $(BREAKPAD_DIR)"
	(cd $(BREAKPAD_DIR) && \
		./configure \
			--prefix=$(INSTALL_DIR) $(BREAKPAD_CONFIG_OPTS) && \
		make AM_CXXFLAGS=-fPIC AM_CFLAGS=-fPIC) 

build_linux: $(EXTRA_FILE_DEPENDENCIES) $(PREPARE_TIMESTAMP)
	mkdir -vp $(INSTALL_DIR)/lib
	@echo "==> Building $(BREAKPAD_DIR)"
	(cd $(BREAKPAD_DIR) && \
		./configure \
			--prefix=$(INSTALL_DIR) $(BREAKPAD_CONFIG_OPTS) && \
		make)

install_linux: $(EXTRA_FILE_DEPENDENCIES) $(BUILD_BREAKPAD_TIMESTAMP)
	(cd $(BREAKPAD_DIR) && make install);

install_win: $(EXTRA_FILE_DEPENDENCIES) $(BUILD_BREAKPAD_TIMESTAMP)

install_android: $(EXTRA_FILE_DEPENDENCIES) $(BUILD_BREAKPAD_TIMESTAMP)
	(cd $(BREAKPAD_DIR) && make install);


.PHONY: clean
clean:
	rm -rf $(BREAKPAD_DIR) \
		$(PREPARE_TIMESTAMP) \
		$(BUILD_BREAKPAD_TIMESTAMP) \
		$(INSTALL_BREAKPAD_TIMESTAMP) \
