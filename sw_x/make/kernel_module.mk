# -*-Makefile-*-
# Common rules to build kernel modules.

include $(SRCROOT)/make/external_locations.mk

.PHONY: all headers copy_files build_module
all: headers copy_files build_module

ifeq ($(PLATFORM), linux)
headers:
	PBROOT=$(PBROOT) $(MAKE) -C $(SRCROOT)/$(HDRSDIR)
else
# ARM builds for android. The following will support both nexus one and the
# emulator. Linux build should be modified to conform with this.
ifeq ($(ARCH), arm)
export CROSS_COMPILE=$(ANDROID_NDK_ROOT)/toolchains/arm-eabi-4.4.0/prebuilt/linux-x86/bin/arm-eabi-
endif
ifeq ($(ARCH), x86)
# This is semi-retarded. The android-x86 build used to generate the kernel
# doesn't control its compiler, meaning it takes whatever is found in
# /usr/bin/gcc. In my case, that's 4.4.3. Since I have no control of the
# compiler version on anyone's build machine I'm forcing the build to a
# known compiler. There aren't many choices in the current ndk. I'm hoping
# this gets fixed with the real Android X86 support when released by google.
export CROSS_COMPILE=$(ANDROID_NDK_ROOT)/linux-x86/toolchain/i686-unknown-linux-gnu-4.2.1/bin/i686-unknown-linux-gnu-
endif
KSRC_LOC = http://$(TOOL_HOST):$(HTTP_PORT)/$(TOOL_PATH)/third_party/platform_gvm
headers: 
	@if [ -e $(PBROOT)/kernel_headers ] ; then \
		cd $(PBROOT)/kernel_headers ; \
		wget --progress=dot:mega $(KSRC_LOC)/$(KSRC_TARBALL).md5 ; \
		DIFF_OUT=`diff $(KSRC_TARBALL).md5 $(KSRC_TARBALL).cur.md5`; \
		if [ ! -z "$$DIFF_OUT" -o ! -e $(KSRC_TARBALL).cur.md5 ] ; then \
			echo "header mismatch - updating"; \
			rm -rf $(PBROOT)/kernel_headers ; \
		else \
			echo "Headers up to date"; \
			rm $(KSRC_TARBALL).md5; \
		fi ; \
	fi
	@if [ ! -e $(PBROOT)/kernel_headers ] ; then \
		mkdir -p $(PBROOT)/kernel_headers ; \
		cd $(PBROOT)/kernel_headers ; \
		wget --progress=dot:mega $(KSRC_LOC)/$(KSRC_TARBALL) && tar xjf $(KSRC_TARBALL) ; \
		wget --progress=dot:mega $(KSRC_LOC)/$(KSRC_TARBALL).md5 ;\
		mv $(KSRC_TARBALL).md5 $(KSRC_TARBALL).cur.md5 ; \
	fi

endif

copy_files: $(addprefix $(BUILDDIR)/sw_x/,$(COPY_FILE_LIST)) $(BUILDDIR)/Makefile

$(addprefix $(BUILDDIR)/sw_x/,$(COPY_FILE_LIST)): $(BUILDDIR)/sw_x/%: $(SRCROOT)/%
	mkdir -p $$(dirname $@)
	cp -f --preserve=timestamps $< $@

$(BUILDDIR)/Makefile: $(SRCROOT)/$(KBUILDFILE)
	cp --preserve=timestamps $< $@

build_module:
	cd $(BUILDDIR) && $(MAKE) -C $(KSRC) M=$(BUILDDIR) GVM_SRCROOT=$(SRCROOT)

