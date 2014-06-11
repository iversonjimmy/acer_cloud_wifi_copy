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

MY_DIR = gvm_core/external/magick++
PKG_MAGICK = GraphicsMagick-1.3.16

INSTALL_DIR = $(PBROOT)/usr
MAGICK_DIR = $(PBROOT)/$(MY_DIR)/$(PKG_MAGICK)

PREPARE_TIMESTAMP = $(PBROOT)/$(MY_DIR)/prepare-magick.timestamp
BUILD_MAGICK_TIMESTAMP = $(PBROOT)/$(MY_DIR)/build-magick.timestamp
INSTALL_MAGICK_TIMESTAMP = $(INSTALL_DIR)/install-magick.timestamp


.PHONY: default
default: $(INSTALL_MAGICK_TIMESTAMP)
	@echo "==> Building $(MAGICK_DIR) on $(PLATFORM)"

$(PREPARE_TIMESTAMP): $(EXTRA_FILE_DEPENDENCIES) 
	$(MAKE) prepare_$(PLATFORM)
	touch $@

$(BUILD_MAGICK_TIMESTAMP): $(EXTRA_FILE_DEPENDENCIES) $(PREPARE_TIMESTAMP)
	$(MAKE) build_$(PLATFORM)
	touch $@

$(INSTALL_MAGICK_TIMESTAMP): $(EXTRA_FILE_DEPENDENCIES) $(BUILD_MAGICK_TIMESTAMP)
	$(MAKE) install_$(PLATFORM)
	touch $@

EXTRA_FILE_DEPENDENCIES = Makefile

prepare_linux: $(EXTRA_FILE_DEPENDENCIES) platform_linux.mk
	rm -rf $(PBROOT)/$(MY_DIR)/*
	@echo "==> Preparing $(MAGICK_DIR) build"
	mkdir -p $(PBROOT)/$(MY_DIR)
	cp $(PKG_MAGICK).tar.gz $(PBROOT)/$(MY_DIR)
	cd $(PBROOT)/$(MY_DIR) && \
		tar xzvf $(PKG_MAGICK).tar.gz
	cd $(PBROOT)/$(MY_DIR)/$(PKG_MAGICK) && \
		patch -p1 < $(CURDIR)/0001-For-build-workaround.patch

prepare_cloudnode: $(EXTRA_FILE_DEPENDENCIES) platform_cloudnode.mk
	rm -rf $(PBROOT)/$(MY_DIR)/*
	@echo "==> Preparing $(MAGICK_DIR) build"
	mkdir -p $(PBROOT)/$(MY_DIR)
	cp $(PKG_MAGICK).tar.gz $(PBROOT)/$(MY_DIR)
	cd $(PBROOT)/$(MY_DIR) && \
		tar xzvf $(PKG_MAGICK).tar.gz
	cd $(PBROOT)/$(MY_DIR)/$(PKG_MAGICK) && \
		patch -p1 < $(CURDIR)/0001-For-build-workaround.patch

build_linux: $(EXTRA_FILE_DEPENDENCIES) $(PREPARE_TIMESTAMP)
	@echo "==> Building $(MAGICK_DIR)"
	(cd $(MAGICK_DIR) && \
		./configure \
			--prefix=$(INSTALL_DIR) $(MAGICK_CONFIG_OPTS) \
			--build=$(shell uname -m) \
			--enable-static \
			--without-threads \
			--without-wmf \
			--without-x \
			--without-bzlib \
			--without-dps \
			--without-jbig \
			--without-jp2 \
			--without-lcms \
			--without-lcms2 \
			--without-lzma \
			--without-trio \
			--without-ttf  \
			--without-wmf \
			--without-xml \
			--without-zlib \
			--with-jpeg \
			--with-png \
			--without-tiff \
			--with-magick-plus-plus \
			--disable-openmp && \
		$(MAKE))

build_cloudnode: $(EXTRA_FILE_DEPENDENCIES) $(PREPARE_TIMESTAMP)
	@echo "==> Building $(MAGICK_DIR)"
	(cd $(MAGICK_DIR) && \
		./configure \
			--prefix=$(INSTALL_DIR) $(MAGICK_CONFIG_OPTS) \
			--host=arm-mv5sft-linux-gnueabi \
			--enable-static \
			--without-threads \
			--without-wmf \
			--without-x \
			--without-bzlib \
			--without-dps \
			--without-jbig \
			--without-jp2 \
			--without-lcms \
			--without-lcms2 \
			--without-lzma \
			--without-trio \
			--without-ttf  \
			--without-wmf \
			--without-xml \
			--without-zlib \
			--with-jpeg \
			--with-png \
			--with-tiff \
			--with-magick-plus-plus \
			--disable-openmp && \
		$(MAKE))

install_linux: $(EXTRA_FILE_DEPENDENCIES) $(BUILD_MAGICK_TIMESTAMP)
	(cd $(MAGICK_DIR) && $(MAKE) install);

install_cloudnode: $(EXTRA_FILE_DEPENDENCIES) $(BUILD_MAGICK_TIMESTAMP)
	(cd $(MAGICK_DIR) && $(MAKE) install);

.PHONY: clean
clean:
	rm -rf $(MAGICK_DIR) \
		$(PREPARE_TIMESTAMP) \
		$(BUILD_MAGICK_TIMESTAMP) \
		$(INSTALL_MAGICK_TIMESTAMP) \
