ccflags-y += -std=gnu99 -Wno-declaration-after-statement -Werror -fno-stack-protector
# ccflags-y += -DHYP_PAE

-include $(src)/Subdir.mk

SUBDIR_CFILES   = $(filter %.c,   $(SUBDIR_FILES))
SUBDIR_ASFILES  = $(filter %.S,   $(SUBDIR_FILES))

SUBDIR_OBJS = $(SUBDIR_CFILES:%.c=%.o) $(SUBDIR_ASFILES:%.S=%.o)

OBJS = $(SUBDIR_OBJS) \
       qdivrem.o \
       udivdi3.o \
       umoddi3.o

MOD_OBJS = os.o \
       hyp.o \
	kprintf.o \
       dmar.o \
       page.o \
       alloc.o \
       $(OBJS)

obj-m = hyprintmod.o
hyprintmod-objs := hyprint.o

ifneq ($(BUILD_HYPMOD),)
obj-m += hypmod.o
hypmod-objs := $(MOD_OBJS)
endif

default: exports

main: main.o
	gcc -o main main.o

main.o: main.c vmcs.h
	gcc -O2 -std=gnu99 -DDUMP_ENCODINGS -c main.c

vmcall: vmcall.o
	gcc -m32 -o vmcall vmcall.o

vmcall.o: vmcall.c
	gcc -m32 -O2 -std=gnu99 -c vmcall.c

vmcall.exe: vmcall.c
	/opt/gvm/mingw32/bin/i686-mingw32-gcc -std=gnu99 -o $@ $<

vmtest: vmtest.o
	gcc -o vmtest vmtest.o

vmtest.o: vmtest.c
	gcc -O2 -std=gnu99 -c vmtest.c

vmcopytest: vmcopytest.o
	gcc -o vmcopytest vmcopytest.o

vmcopytest.o: vmcopytest.c
	gcc -O2 -std=gnu99 -c vmcopytest.c

LOCAL_KBUILD := make -C /lib/modules/$(shell uname -r)/build BUILD_HYPMOD=1 M=$(CURDIR)

local-modules:
	$(LOCAL_KBUILD) modules
local-clean:
	$(LOCAL_KBUILD) clean
local-modules_install:
	$(LOCAL_KBUILD) modules_install

-include $(ROOT)/usr/include/make/GVMKernelrules
