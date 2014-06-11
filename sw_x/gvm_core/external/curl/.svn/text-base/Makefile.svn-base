###################################
# Common defs
###################################

ifndef SRCROOT
$(error Need to define SRCROOT (or run this from a higher level Makefile))
endif

include $(SRCROOT)/make/external_locations.mk

# Files live at "/a/pc/tools/third_party/" on pcstore.
ARCHIVE_REPOSITORY = http://$(TOOL_HOST):$(HTTP_PORT)/$(TOOL_PATH)/third_party/platform_linux

# TODO get this programatically
MY_DIR = gvm_core/external/curl

INSTALL_DIR = $(PBROOT)/usr
STATUS_DIR = $(INSTALL_DIR)/.status

ifndef PBROOT
$(error Need to define PBROOT (or run this from a higher level Makefile))
endif

ifndef PLATFORM
$(error Need to define PLATFORM (or run this from a higher level Makefile))
endif

include platform_$(PLATFORM).mk

#########

PKG_CURL ?= curl-7.20.0

CURR_BUILD_ROOT = $(PBROOT)/$(MY_DIR)

CURR_BUILD_DIR = $(CURR_BUILD_ROOT)/$(PKG_CURL)

# Indicates when "download and unpack" completed.
PREP_STAMP_FILE = $(CURR_BUILD_ROOT)/$(PKG_CURL)-unpacked.timestamp

# Indicates when "configure and call external makefile" completed.
DONE_STAMP_FILE = $(CURR_BUILD_ROOT)/$(PKG_CURL)-built.timestamp

# TODO: bug 7579: enable c-ares
CURL_CONFIG_OPTS += --prefix=$(INSTALL_DIR) \
			--enable-shared \
			--enable-static \
			--enable-http \
			--enable-https \
			--disable-ftp \
			--disable-file \
			--disable-ldap \
			--disable-ldaps \
			--disable-dict \
			--disable-telnet \
			--disable-tftp \
			--disable-manual \
			--disable-libgcc \
			--enable-ipv6 \
			--enable-nonblocking \
			--enable-thread \
			--disable-ares \
			--disable-verbose \
			--disable-sspi \
			--disable-debug \
			--disable-crypto-auth \
			--disable-cookies \
			--enable-hidden-symbols \
			--without-libssh2 \
			--without-gnutls \
			--without-nss \
			--without-ca-bundle \
			--without-ca-path \
			--without-libidn \
# end of list

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
install: $(STATUS_DIR)/$(PKG_CURL)

.PHONY: clean
clean: $(PKG_CURL)-clean

###################################
# Prepare from external by unpacking.
###################################

$(PREP_STAMP_FILE): $(DEP_MAKEFILES)
	@echo "==> Preparing $(PKG_CURL) build"
	rm -rf $(CURR_BUILD_ROOT)
	mkdir -p $(CURR_BUILD_ROOT)
	cd $(CURR_BUILD_ROOT) && \
		wget --progress=dot:mega -O - $(ARCHIVE_REPOSITORY)/$(PKG_CURL).tar.gz | tar xzf -
	touch $@

###################################
# Build does the usual configure/make, but we need to control
# the environment to pick the right tools.
###################################

$(DONE_STAMP_FILE): $(PREP_STAMP_FILE) $(DEP_MAKEFILES)
	@echo "==> Building $(PKG_CURL)"
	(cd $(CURR_BUILD_DIR) && ./configure $(CURL_CONFIG_OPTS))
	# Make sure HTTPS is supported:
	grep HTTPS $(CURR_BUILD_DIR)/libcurl.pc || (echo 'Not configured for HTTPS!' && false)
	$(MAKE) -C $(CURR_BUILD_DIR)
	touch $@

###################################
# Install.
###################################

$(STATUS_DIR)/$(PKG_CURL): $(DONE_STAMP_FILE) $(STATUS_DIR)
	@echo "==> Installing in $(INSTALL_DIR)"
	$(MAKE) -C $(CURR_BUILD_DIR) install
	mkdir -p $(STATUS_DIR)
	touch $@

###################################
# Clean all the temporaries and targets.
###################################

.PHONY: $(PKG_CURL)-clean
$(PKG_CURL)-clean:
	rm -rf $(CURR_BUILD_ROOT)

###################################
# Common rules
###################################

$(STATUS_DIR):
	mkdir -p $@
