###################################
# Common defs
###################################

# Files live at "/a/pc/tools/third_party/" on pcstore.ctbg.acer.com.
ARCHIVE_REPOSITORY = http://$(TOOL_HOST):$(HTTP_PORT)/$(TOOL_PATH)/third_party/platform_multi

# TODO get this programatically
MY_DIR = gvm_core/external/openssl

INSTALL_DIR = $(PBROOT)/usr
STATUS_DIR = $(INSTALL_DIR)/.status

ifndef PBROOT
$(error Need to define PBROOT (or run this from a higher level Makefile))
endif

ifndef PLATFORM
$(error Need to define PLATFORM (or run this from a higher level Makefile))
endif

ifndef SRCROOT
$(error Need to define SRCROOT (or run this from a higher level Makefile))
endif

include platform_$(PLATFORM).mk
include $(SRCROOT)/make/external_locations.mk

#########

PKG_NAME ?= openssl-0.9.8r

CURR_BUILD_ROOT = $(PBROOT)/$(MY_DIR)

CURR_BUILD_DIR = $(CURR_BUILD_ROOT)/$(PKG_NAME)

# Indicates when "download and unpack" completed.
PREP_STAMP_FILE = $(CURR_BUILD_ROOT)/$(PKG_NAME)-unpacked.timestamp

# Indicates when "configure and call external makefile" completed.
DONE_STAMP_FILE = $(CURR_BUILD_ROOT)/$(PKG_NAME)-built.timestamp

OPENSSL_CONFIG_OPTS += --prefix=$(INSTALL_DIR)

DEP_MAKEFILES = $(CURDIR)/Makefile $(CURDIR)/platform_$(PLATFORM).mk

###################################
# Shortcuts
###################################

# default needs to be install for the new build system
.PHONY: default
default: install

.PHONY: prep
prep: $(PREP_STAMP_FILE)

.PHONY: build
build: $(DONE_STAMP_FILE)

.PHONY: install
install: $(STATUS_DIR)/$(PKG_NAME)

.PHONY: clean
clean: $(PKG_NAME)-clean

###################################
# Prepare from external by unpacking.
###################################

$(PREP_STAMP_FILE): $(DEP_MAKEFILES)
	@echo "==> Preparing $(PKG_NAME) build"
	rm -rf $(CURR_BUILD_ROOT)
	mkdir -p $(CURR_BUILD_ROOT)
	cd $(CURR_BUILD_ROOT) && \
		wget --progress=dot:mega -O - $(ARCHIVE_REPOSITORY)/$(PKG_NAME).tar.gz | tar xzf -
	cd $(CURR_BUILD_DIR) && \
		patch -p1 < $(CURDIR)/platform_$(PLATFORM).patch
	touch $@

###################################
# Build does the usual configure/make, but we need to control
# the environment to pick the right tools.
###################################

$(DONE_STAMP_FILE): $(PREP_STAMP_FILE) $(DEP_MAKEFILES)
	@echo "==> Building $(PKG_NAME)"
	(cd $(CURR_BUILD_DIR) && CROSS_COMPILE=i586-mingw32msvc- ./Configure $(OPENSSL_CONFIG_OPTS))
	$(MAKE) -C $(CURR_BUILD_DIR)
	touch $@

###################################
# Install.
###################################

$(STATUS_DIR)/$(PKG_NAME): $(DONE_STAMP_FILE) $(STATUS_DIR)
	@echo "==> Installing in $(INSTALL_DIR)"
	$(MAKE) -C $(CURR_BUILD_DIR) install
	cp $(CURR_BUILD_DIR)/apps/cygcrypto-0.9.8.dll $(INSTALL_DIR)/bin/
	cp $(CURR_BUILD_DIR)/apps/cygssl-0.9.8.dll $(INSTALL_DIR)/bin/
	mkdir -p $(STATUS_DIR)
	touch $@

###################################
# Clean all the temporaries and targets.
###################################

.PHONY: $(PKG_NAME)-clean
$(PKG_NAME)-clean:
	rm -rf $(CURR_BUILD_ROOT)

###################################
# Common rules
###################################

$(STATUS_DIR):
	mkdir -p $@
