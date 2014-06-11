# TODO: this is temporary; we need to figure out if we are going to build these libs or get them from somewhere else

ifndef PBROOT
$(error Need to define PBROOT (or run this from a higher level Makefile))
endif

INSTALL_DIR = $(PBROOT)/usr
INSTALL_TIMESTAMP = $(INSTALL_DIR)/install-pthread_win32.timestamp

.PHONY: default
default: $(INSTALL_TIMESTAMP)

$(INSTALL_TIMESTAMP): $(CURDIR)/pthread.tar.gzip
	mkdir -p $(PBROOT)/usr
	cd $(PBROOT)/usr && tar xvf $(CURDIR)/pthread.tar.gzip
	touch $@
