# -*-Makefile-*-
# Makefile that gets invoked by makegen

ifdef LOCAL_BUILD
BUILDDIR = $(PBROOT)/local_kernel_modules/escmod
else
BUILDDIR = $(PBROOT)/kernel_modules/escmod
endif

# KSRC is still a hack: I need a way to get it from makegen
ifeq ($(PLATFORM), linux)
ifdef LOCAL_BUILD
KSRC = /lib/modules/$(shell uname -r)/build
else
KSRC = $(PBROOT)/kernel_headers/linux-headers-2.6.32-21-generic-pae
endif
else
KSRC_TARBALL = swx_$(ARCH)_$(HW_PLAT)_$(KVERS).tar.bz2
KSRC = $(PBROOT)/kernel_headers/swx_$(ARCH)_$(HW_PLAT)_$(KVERS)
endif

# make sure we're using bash
SHELL = /bin/bash

# List of files that will need to be copied to the build directory.
# The files are relative to SW_X.
COPY_FILE_LIST =					\
	es_core/kernel/linux/esc.c			\
	es_core/kernel/linux/esc_ioc.c			\
	es_core/kernel/linux/escsvc.c			\
	es_core/kernel/linux/esc_mon.c			\
	es_core/kernel/linux/core_glue.c		\
	es_core/esc/core/ec/ec_cache.c			\
	es_core/esc/core/ec/ec_core.c			\
	es_core/esc/core/ec/ec_pers.c			\
	es_core/esc/core/ec/istorage_impl.c		\
	es_core/esc/core/es/es_container.c		\
	es_core/esc/core/es/es_storage.c		\
	es_core/esc/core/ioscrypto/crypto_impl.c	\
	es_core/esc/core/ioscrypto/mt19937-64.c		\
	es_core/esc/core/base/esi/esi_core.c		\
	es_core/esc/core/base/ioscrypto/bsl.c		\
	es_core/esc/core/base/ioscrypto/keystore.c	\
	es_core/esc/core/base/ioscrypto/wrapper.c	\
	gvm_core/internal/csl/src/aes.c			\
	gvm_core/internal/csl/src/aes_api.c		\
	gvm_core/internal/csl/src/aes_imp.c		\
	gvm_core/internal/csl/src/algorithms.c		\
	gvm_core/internal/csl/src/convecc.c		\
	gvm_core/internal/csl/src/convrsa.c		\
	gvm_core/internal/csl/src/cslecc.c		\
	gvm_core/internal/csl/src/csloaep.c		\
	gvm_core/internal/csl/src/cslrsa.c		\
	gvm_core/internal/csl/src/cslrsa_sign.c		\
	gvm_core/internal/csl/src/elliptic_math.c	\
	gvm_core/internal/csl/src/integer_math.c	\
	gvm_core/internal/csl/src/poly_math.c		\
	gvm_core/internal/csl/src/sha1.c		\
	gvm_core/internal/csl/src/sha256.c		\
	gvm_core/internal/csl/src/tomcrypt_sha256.c

KBUILDFILE = es_core/kernel/linux/Makefile.kmod
ifeq ($(PLATFORM), linux)
HDRSDIR = es_core/kernel/ubuntu
endif

include $(SRCROOT)/make/kernel_module.mk
