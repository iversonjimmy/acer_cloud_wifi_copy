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

MY_DIR = gvm_core/external/libexif
PKG_LIBEXIF = libexif-0.6.21

INSTALL_DIR = $(PBROOT)/usr
LIBEXIF_DIR = $(PBROOT)/$(MY_DIR)/$(PKG_LIBEXIF)

PREPARE_TIMESTAMP = $(PBROOT)/$(MY_DIR)/prepare-libexif.timestamp
BUILD_LIBEXIF_TIMESTAMP = $(PBROOT)/$(MY_DIR)/build-libexif.timestamp
INSTALL_LIBEXIF_TIMESTAMP = $(INSTALL_DIR)/install-libexif.timestamp


.PHONY: default
default: $(INSTALL_LIBEXIF_TIMESTAMP)
	@echo "==> Building $(LIBEXIF_DIR) on $(PLATFORM)"

$(PREPARE_TIMESTAMP): $(EXTRA_FILE_DEPENDENCIES) 
	$(MAKE) prepare_$(PLATFORM)
	touch $@

$(BUILD_LIBEXIF_TIMESTAMP): $(EXTRA_FILE_DEPENDENCIES) $(PREPARE_TIMESTAMP)
	$(MAKE) build_$(PLATFORM)
	touch $@

$(INSTALL_LIBEXIF_TIMESTAMP): $(EXTRA_FILE_DEPENDENCIES) $(BUILD_LIBEXIF_TIMESTAMP)
	$(MAKE) install_$(PLATFORM)
	touch $@

EXTRA_FILE_DEPENDENCIES = Makefile

prepare_linux: $(EXTRA_FILE_DEPENDENCIES) platform_linux.mk
	rm -rf $(PBROOT)/$(MY_DIR)/*
	@echo "==> Preparing $(LIBEXIF_DIR) build"
	mkdir -p $(PBROOT)/$(MY_DIR)
	cp $(PKG_LIBEXIF).tar.gz $(PBROOT)/$(MY_DIR)
	cd $(PBROOT)/$(MY_DIR) && \
		tar xzvf $(PKG_LIBEXIF).tar.gz

prepare_linux_emb: $(EXTRA_FILE_DEPENDENCIES) platform_linux.mk
	rm -rf $(PBROOT)/$(MY_DIR)/*
	@echo "==> Preparing $(LIBEXIF_DIR) build"
	mkdir -p $(PBROOT)/$(MY_DIR)
	cp $(PKG_LIBEXIF).tar.gz $(PBROOT)/$(MY_DIR)
	cd $(PBROOT)/$(MY_DIR) && \
		tar xzvf $(PKG_LIBEXIF).tar.gz

prepare_cloudnode: $(EXTRA_FILE_DEPENDENCIES) platform_cloudnode.mk
	rm -rf $(PBROOT)/$(MY_DIR)/*
	@echo "==> Preparing $(LIBEXIF_DIR) build"
	mkdir -p $(PBROOT)/$(MY_DIR)
	cp $(PKG_LIBEXIF).tar.gz $(PBROOT)/$(MY_DIR)
	cd $(PBROOT)/$(MY_DIR) && \
		tar xzvf $(PKG_LIBEXIF).tar.gz

build_linux: $(EXTRA_FILE_DEPENDENCIES) $(PREPARE_TIMESTAMP)
	@echo "==> Building $(LIBEXIF_DIR)"
	(cd $(LIBEXIF_DIR) && \
		$(LIBEXIF_CONF_OPTS) ./configure \
			--prefix=$(INSTALL_DIR) $(LIBEXIF_CONFIG_OPTS) \
			--build=$(shell uname -m) \
			--enable-static \
			--disable-docs \
			--disable-nls \
			--without-libintl-prefix \
			--without-libiconv-prefix && \
		$(MAKE))

build_linux_emb: $(EXTRA_FILE_DEPENDENCIES) $(PREPARE_TIMESTAMP)
	@echo "==> Building $(LIBEXIF_DIR)"
	(cd $(LIBEXIF_DIR) && \
		$(LIBEXIF_CONF_OPTS) ./configure \
			--prefix=$(INSTALL_DIR) $(LIBEXIF_CONFIG_OPTS) \
			--host=mips-linux \
			--enable-static \
			--disable-docs \
			--disable-nls \
			--without-libintl-prefix \
			--without-libiconv-prefix && \
		$(MAKE))

build_cloudnode: $(EXTRA_FILE_DEPENDENCIES) $(PREPARE_TIMESTAMP)
	@echo "==> Building $(LIBEXIF_DIR)"
	(cd $(LIBEXIF_DIR) && \
		./configure \
			--prefix=$(INSTALL_DIR) $(LIBEXIF_CONFIG_OPTS) \
			--host=arm-mv5sft-linux-gnueabi \
			--enable-static \
			--disable-docs \
			--disable-nls \
			--without-libintl-prefix \
			--without-libiconv-prefix && \
		$(MAKE))

install_linux: $(EXTRA_FILE_DEPENDENCIES) $(BUILD_LIBEXIF_TIMESTAMP)
	(cd $(LIBEXIF_DIR) && $(MAKE) install);

install_linux_emb: $(EXTRA_FILE_DEPENDENCIES) $(BUILD_LIBEXIF_TIMESTAMP)
	(cd $(LIBEXIF_DIR) && $(MAKE) install);

install_cloudnode: $(EXTRA_FILE_DEPENDENCIES) $(BUILD_LIBEXIF_TIMESTAMP)
	(cd $(LIBEXIF_DIR) && $(MAKE) install);

.PHONY: clean
clean:
	rm -rf $(LIBEXIF_DIR) \
		$(PREPARE_TIMESTAMP) \
		$(BUILD_LIBEXIF_TIMESTAMP) \
		$(INSTALL_LIBEXIF_TIMESTAMP) \
