include $(ROOT)/usr/include/make/GVMdefs

#OPTIMIZER += -O2
CFILES = conf_test.c
LLDLIBS += -llog -lconf -lpthread

TARGETS = $(CFILES:.c=)

default: $(TARGETS)

#runtests: $(TARGETS)
#	for i in $(TARGETS); do \
#		LD_LIBRARY_PATH=/opt/gvm/root/lib ./$$i; \
#	done

default: $(TARGETS)

install: default
	$(INSTALL) -m 644 $(TARGETS) $(ROOT)/gvm/usr/local/tests

include $(GVMRULES)
