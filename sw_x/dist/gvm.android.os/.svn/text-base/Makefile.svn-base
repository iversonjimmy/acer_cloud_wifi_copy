# THIS_DIR gives the directory containing this current Makefile
# THIS_DIR is a variable that's reliable regardless of
# whether we call 'make -C dir -f Makefile target' or 'make -f dir/Makefile target'
THIS_DIR=$(dir $(CURDIR)/$(lastword $(MAKEFILE_LIST)))

export SHELL=/bin/bash

.PHONY: build_tools default dist fetch_android_source check_packages
.DEFAULT_GOAL := default

.NOTPARALLEL:

# The GIT_DIR should be a sibling directory to the CVS sw_x directory.
# Inside the git directory should be the git version of sw_x directory that's manually set up prior to the build.
GIT_DIR ?= ../../../git

# accessing through /net via the autofs (a.k.a automounter)
DIST_TARGET_DIR=/net/grouper/a/GVM/buildbot_outputs/GVM-ANDROID

check_packages:
	echo check packages

fetch_android_source:
	cd $(THIS_DIR)/$(GIT_DIR)/sw_x && git pull
	cd $(THIS_DIR)/$(GIT_DIR)/sw_x && git reset --hard
	
	cd $(THIS_DIR)/$(GIT_DIR)/sw_x/android/2.2/x86 && make clobber
	cd $(THIS_DIR)/$(GIT_DIR)/sw_x/android/2.2/x86 && make update
	cd $(THIS_DIR)/$(GIT_DIR)/sw_x/android/2.2/x86 && make reset

build_tools: fetch_android_source

default:
	cd $(THIS_DIR) && GIT_DIR=$(GIT_DIR) bash makeAndroid.sh showcommands usb_img iso_img

dist:
	cp $(THIS_DIR)/$(GIT_DIR)/sw_x/android/2.2/x86/out/target/product/generic_x86/generic_x86_usb.img $(DIST_TARGET_DIR)/usb.img/generic_x86_usb.img-$(BUILD_DATE)
	ln -sf $(DIST_TARGET_DIR)/usb.img/generic_x86_usb.img-$(BUILD_DATE) $(DIST_TARGET_DIR)/usb.img/generic_x86_usb.img
	
	cp $(THIS_DIR)/$(GIT_DIR)/sw_x/android/2.2/x86/out/target/product/generic_x86/generic_x86.iso $(DIST_TARGET_DIR)/iso/generic_x86.iso-$(BUILD_DATE)
	ln -sf $(DIST_TARGET_DIR)/iso/generic_x86.iso-$(BUILD_DATE) $(DIST_TARGET_DIR)/iso/generic_x86.iso

