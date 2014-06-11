TESTDIR ?= $(ROOT)/gvm/usr/local/tests/vpl/microbench
include ../../gvm.prog.mk

LCFLAGS += -g3 -I../lib

LLDLIBS  += -lvplex -lvpl-microbench
