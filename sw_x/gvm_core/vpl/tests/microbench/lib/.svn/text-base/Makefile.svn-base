include $(ROOT)/usr/include/make/GVMdefs

CFILES = vpl_timing_parseargs.c microbench_profil.c

#HFILES = vpl_microbenchmark.h microbench_profil.h

OFILES = $(CFILES:.c=.o)

LLDLIBS += -lipc -lpthread -lm -lvpl

LCFLAGS += -g3 -I../lib



TARGETS = libvpl-microbench.a
default: exports

#(headers: $(HFILES)
#	$(INSTALL) $^ $(INCDIR)
#


exports: $(TARGETS)
	$(INSTALL) -m444 $(TARGETS) $(LIBDIR)


$(TARGETS): $(OFILES)
	ar cru $@ $(OFILES)



include $(GVMRULES)
