SHELL=/bin/bash

override MY_TEST_NAME=es

# makefile included by all test makefiles
include ../make/test_env.mk

# xargs -r a GNU extension t
XARGS_CMDLINE = xargs -r

# auto detect
override WORKAREA=$(shell cd ../../..; pwd)

# override by env variable
export BUILDROOT ?= $(WORKAREA)/buildroot
export PRODUCT ?= linux
export BUILDTYPE ?= release
export USE_VALGRIND ?= 0

sinclude ../make/$(PRODUCT).makedefs

# derived
override TESTROOT=$(BUILDROOT)/$(BUILDTYPE)/$(PRODUCT)
override TESTDIR=$(TESTROOT)/tests/es

ifeq ($(PRODUCT), cloudnode)
ifndef CLOUDPC_NAME
    $(error CLOUDPC_NAME must be either the hostname or ip address of an orbe)
endif
export TARGET_USER ?= build
override TARGET_MACHINE=$(CLOUDPC_NAME)
override TESTPLAT=cloudnode
override TARGET_HOME=/home/$(TARGET_USER)
override TARGET_TESTROOT=$(TARGET_HOME)/temp/igware/testroot
override KILL=killall
override TEST_PACKAGE=tests_cloudnode.tar.gz
override TEST_PACKAGE_STORE_PATH=$(STORE_PATH)/$(TEST_BRANCH)/cloudnode/tests_cloudnode
override TARGET_TEST_DECRYPT_CONTENT=$(BUILDTYPE)/$(PRODUCT)/es_core/kernel/test_decrypt_content
override TARGET_TEST_DECRYPT_SECRET_CRED=$(BUILDTYPE)/$(PRODUCT)/es_core/kernel/test_decrypt_secret_cred
override BUILDROOT_TEST_FILES=$(TARGET_TEST_DECRYPT_CONTENT) $(TARGET_TEST_DECRYPT_SECRET_CRED)
override REMOTE_RUN_PRIVILEGED=/usr/bin/ssh root@$(TARGET_MACHINE)
override AS_TARGET_USER=su $(TARGET_USER) -c
else
override TESTPLAT=linux
endif
REMOTE_RUN=/usr/bin/ssh $(TARGET_USER)@$(TARGET_MACHINE)


TEST_DATA_TAR = es_test_data-20110330-1627.tar

# name of tests with initial test_ dropped
TEST_LIST = decrypt_content_ordered_all decrypt_content_ordered_64 decrypt_content_random_64 decrypt_secret_cred

.PHONY: default
default: all_tests

.PHONY: all all_tests
ifeq ($(TESTPLAT), cloudnode)
all all_tests:
	$(MAKE) setup_$(TESTPLAT)
	$(MAKE) runtests_$(TESTPLAT)
	$(MAKE) cleanup_$(TESTPLAT)
else
all all_tests: $(addprefix test_,$(TEST_LIST))
endif

.PHONY: get_test_data
get_test_data: download_test_data $(TESTDIR)/es_test_data/timestamp
	mkdir -p $(TESTDIR)

.PHONY: downlod_test_data
download_test_data:
	wget --progress=dot:mega --timestamping --directory-prefix=$(TESTDIR) http://$(TESTDATA_HOST):$(HTTP_PORT)/$(TESTDATA_PATH)/es/$(TEST_DATA_TAR)

$(TESTDIR)/es_test_data/timestamp: $(TESTDIR)/$(TEST_DATA_TAR)
	cd $(TESTDIR) && rm -fr es_test_data && tar xf $(TEST_DATA_TAR) && touch es_test_data/timestamp

.PHONY: list_tests
list_tests:
	@for t in $(TEST_LIST); do			\
		echo test_$$t;				\
	done


.PHONY: copy_win32_dlls
ifeq ($(shell uname -o),Cygwin)
copy_win32_dlls: $(addprefix $(TESTDIR)/,pthreadGC2.dll cygcrypto-0.9.8.dll libcurl-4.dll cygssl-0.9.8.dll)
$(addprefix $(TESTDIR)/,pthreadGC2.dll): $(TESTDIR)/%: $(TESTROOT)/usr/lib/%
	cp -f --preserve=timestamps $< $@
$(addprefix $(TESTDIR)/,cygcrypto-0.9.8.dll libcurl-4.dll cygssl-0.9.8.dll): $(TESTDIR)/%: $(TESTROOT)/usr/bin/%
	cp -f --preserve=timestamps $< $@
else
copy_win32_dlls:
endif


TEST_DECRYPT_CONTENT = $(TESTROOT)/es_core/kernel/test_decrypt_content
TEST_DECRYPT_CONTENT_ARGS = -c es_test_data/rootfs -f es_test_data/rootfs-cfm -t es_test_data/rootfs-tmd -T es_test_data/rootfs-tik -C es_test_data/rootfs-ca-cert -C es_test_data/rootfs-xs-cert

.PHONY: run_decrypt_content_ordered_all
run_decrypt_content_ordered_all: copy_win32_dlls
	cd $(TESTDIR) && $(TEST_DECRYPT_CONTENT) $(TEST_DECRYPT_CONTENT_ARGS)

.PHONY: test_decrypt_content_ordered_all
test_decrypt_content_ordered_all: get_test_data
	@if [[ ! -x $(TEST_DECRYPT_CONTENT) ]]; then		\
		echo ERROR $(TEST_DECRYPT_CONTENT) missing;	\
		exit 1;						\
	fi
ifeq ($(CONFIG_ES),km)
	$(MAKE) install_escmod
endif
	$(MAKE) run_decrypt_content_ordered_all
ifeq ($(CONFIG_ES),km)
	$(MAKE) remove_escmod
endif

.PHONY: run_decrypt_content_ordered_64
run_decrypt_content_ordered_64: copy_win32_dlls
	cd $(TESTDIR) && $(TEST_DECRYPT_CONTENT) $(TEST_DECRYPT_CONTENT_ARGS) -n 64

.PHONY: test_decrypt_content_ordered_64
test_decrypt_content_ordered_64: get_test_data
	@if [[ ! -x $(TEST_DECRYPT_CONTENT) ]]; then		\
		echo ERROR $(TEST_DECRYPT_CONTENT) missing;	\
		exit 1;						\
	fi
ifeq ($(CONFIG_ES),km)
	$(MAKE) install_escmod
endif
	$(MAKE) run_decrypt_content_ordered_64
ifeq ($(CONFIG_ES),km)
	$(MAKE) remove_escmod
endif

.PHONY: run_decrypt_content_random_64
run_decrypt_content_random_64: copy_win32_dlls
	cd $(TESTDIR) && $(TEST_DECRYPT_CONTENT) $(TEST_DECRYPT_CONTENT_ARGS) -n 64 -r

.PHONY: test_decrypt_content_random_64
test_decrypt_content_random_64: get_test_data
	@if [[ ! -x $(TEST_DECRYPT_CONTENT) ]]; then		\
		echo ERROR $(TEST_DECRYPT_CONTENT) missing;	\
		exit 1;						\
	fi
ifeq ($(CONFIG_ES),km)
	$(MAKE) install_escmod
endif
	$(MAKE) run_decrypt_content_random_64
ifeq ($(CONFIG_ES),km)
	$(MAKE) remove_escmod
endif


.PHONY: run_auto_cleanup
run_auto_cleanup: copy_win32_dlls
	cd $(TESTDIR) && $(TEST_DECRYPT_CONTENT) $(TEST_DECRYPT_CONTENT_ARGS) -n 32 -M

.PHONY: test_auto_cleanup
test_auto_cleanup: get_test_data
	@if [[ ! -x $(TEST_DECRYPT_CONTENT) ]]; then		\
		echo ERROR $(TEST_DECRYPT_CONTENT) missing;	\
		exit 1;						\
	fi
ifeq ($(CONFIG_ES),km)
	$(MAKE) install_escmod
endif
	$(MAKE) run_auto_cleanup
ifeq ($(CONFIG_ES),km)
	$(MAKE) remove_escmod
endif


TEST_DECRYPT_SECRET_CRED = $(TESTROOT)/es_core/kernel/test_decrypt_secret_cred
TEST_DECRYPT_SECRET_CRED_ARGS = -c es_test_data/dev_cred_clear -s es_test_data/dev_cred_secret

.PHONY: run_decrypt_secret_cred
run_decrypt_secret_cred: copy_win32_dlls
	cd $(TESTDIR) && $(TEST_DECRYPT_SECRET_CRED) $(TEST_DECRYPT_SECRET_CRED_ARGS)

.PHONY: test_decrypt_secret_cred
test_decrypt_secret_cred: get_test_data
	@if [[ ! -x $(TEST_DECRYPT_SECRET_CRED) ]]; then	\
		echo ERROR $(TEST_DECRYPT_SECRET_CRED) missing;	\
		exit 1;						\
	fi
ifeq ($(CONFIG_ES),km)
	$(MAKE) install_escmod
endif
	$(MAKE) run_decrypt_secret_cred
ifeq ($(CONFIG_ES),km)
	$(MAKE) remove_escmod
endif


.PHONY: setup_cloudnode runtests_cloudnode cleanup_cloudnode

setup_cloudnode:
	-mkdir -p $(LOGDIR)
	# TARGET_TESTROOT is the test root path on the test target
	# TARGET_MACHINE is the IP address, hostname, or localhost (i.e. on the same machine as test builder)
	@if [ "$(TARGET_MACHINE)" == "" ]; then \
	  echo '*** Must define "TARGET_MACHINE"'; \
	  false; \
	fi
	@if [ "$(TARGET_USER)" == "" ]; then \
	  echo '*** Must define "TARGET_USER"'; \
	  false; \
	fi
	@if [ -z "$(TARGET_TESTROOT)" ]; then echo "*** Bad variable TARGET_TESTROOT"; exit 2; fi
	-$(REMOTE_RUN) 'mkdir -p $(TARGET_TESTROOT)'
	-$(REMOTE_RUN) 'rm -fr $(TARGET_TESTROOT)/*'
	$(REMOTE_RUN) 'wget http://$(TESTDATA_HOST):$(HTTP_PORT)/$(TESTDATA_PATH)/es/$(TEST_DATA_TAR) -P $(TARGET_TESTROOT)'
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); tar -xvf $(TEST_DATA_TAR)';
	if [ "$(TEST_PACKAGE)" == "" ] || [ "$(USE_ARCHIVED_BUILD)" == "" ] || [ "$(USE_BUILDROOT_TESTS)" != "" ]; then \
	  cd $(BUILDROOT); rsync -pR  $(BUILDROOT_TEST_FILES) $(TARGET_USER)@$(TARGET_MACHINE):$(TARGET_TESTROOT); \
	else \
	  $(REMOTE_RUN) 'wget http://$(STORE_HOST):$(HTTP_PORT)/$(TEST_PACKAGE_STORE_PATH)/$(TEST_PACKAGE) -P $(TARGET_TESTROOT)'; \
	  $(REMOTE_RUN) 'cd $(TARGET_TESTROOT); tar -xzvf $(TEST_PACKAGE)'; \
	fi


runtests_cloudnode:
	-$(REMOTE_RUN) '$(KILL) test_decrypt_content test_decrypt_secret_cred'
	@if [ -z "$(LOGDIR)" ]; then echo "Bad variable"; exit 2; fi
	rm -fr $(LOGDIR)/*
	$(REMOTE_RUN_PRIVILEGED) ' \
	  { \
	    ulimit -c unlimited && cd $(TARGET_TESTROOT) && $(AS_TARGET_USER) " \
	      $(TARGET_TEST_DECRYPT_CONTENT)     $(TEST_DECRYPT_CONTENT_ARGS) && \
	      $(TARGET_TEST_DECRYPT_CONTENT)     $(TEST_DECRYPT_CONTENT_ARGS) -n 64 && \
	      $(TARGET_TEST_DECRYPT_CONTENT)     $(TEST_DECRYPT_CONTENT_ARGS) -n 64 -r && \
	      $(TARGET_TEST_DECRYPT_SECRET_CRED) $(TEST_DECRYPT_SECRET_CRED_ARGS) && \
	      echo \"ENTIRE TEST FINISHED\""; \
	  }' \
	2>&1 | tee $(LOGDIR)/$(MY_TEST_NAME).log | grep $(GREP_TEST_OUTPUT_REGEXP); \
	echo "PIPESTATUS[0] is $${PIPESTATUS[0]} PIPESTATUS[1] is $${PIPESTATUS[1]}";
	@if tail -n 1 $(LOGDIR)/$(MY_TEST_NAME).log | grep -q '^ENTIRE TEST FINISHED'; then \
	  echo "TC_RESULT = PASS ;;; TC_NAME = $(MY_TEST_NAME)_complete"; \
	else \
	  echo "*** $(MY_TEST_NAME) didn't complete!"; \
	  echo "TC_RESULT = FAIL ;;; TC_NAME = $(MY_TEST_NAME)_complete"; \
	fi


cleanup_cloudnode: 
	-$(REMOTE_RUN) '$(KILL) test_decrypt_content test_decrypt_secret_cred'


sinclude ../make/$(PRODUCT).makerules
