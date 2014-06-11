VPL       := $(BUILDROOT)/debug/$(PRODUCT)/gvm_core/vpl/
VPLEX     := $(BUILDROOT)/debug/$(PRODUCT)/gvm_core/vplex/
CSL       := $(BUILDROOT)/debug/$(PRODUCT)/gvm_core/internal/csl
ANS       := $(BUILDROOT)/debug/$(PRODUCT)/gvm_core/internal/ans
LOG       := $(BUILDROOT)/debug/$(PRODUCT)/gvm_core/internal/log

includes  := -I../../../vpl/include                    \
             -I../../../vpl/platform_linux/include     \
             -I../../../vpl/posix_include              \
             -I../../../vplex/include                  \
             -I../../../vplex/platform_include         \
             -I../../../vplex/posix_include            \
             -I../../../vplex/platform_linux/include   \
             -I../../csl/include                       \
             -I../../../../es_core/km_include          \
             -I../../ans/include                       \
             -I../include

dincludes := $(includes) -I.

objects   := pxd_client.o pxd_event.o pxd_log.o pxd_mtwist.o pxd_packet.o \
             pxd_util.o

libs      := -L$(ANS) -lans \
             -L$(LOG) -llog \
             -L$(VPL) -lvplcore -L$(VPLEX) -lvplexcore -lssl \
             -lrt -lpthread -L$(CSL) -lcsl 

vocal     := -fno-strict-aliasing -g3 -O3 -Wall -Werror -Wno-packed \
             -Wpointer-arith -Wredundant-decls -Wstrict-aliasing=3  \
             -Wswitch-enum -Wundef -Wwrite-strings -Wextra          \
             -Wno-unused-parameter -Wno-sign-compare

configure := -DLINUX -DHOST_IS_LITTLE_ENDIAN -DGVM -D_FILE_OFFSET_BITS=64 \
             -DGVM_BUILD_INTERN -m32

%.o:   %.cpp
	g++ -m32 $(includes) -I../../log/include -c $(vocal) $(configure) $*.cpp -o $@

#
#  Define the "all" target, which is the default.
#
all:    setup lib gcov test pxd_circle pxd_triangle pxd_bw

#
#  Define the unit test target.
#
test unit_test:	pxd_client_test pxd_packet_test pxd_event_test $(objects)
	testing/run-test ./pxd_client_test
	testing/run-test ./pxd_packet_test
	testing/run-test ./pxd_event_test
	@echo "The tests passed."

setup:
	cd $$WORKAREA/sw_x && make libpxd
	cd $$WORKAREA/sw_i/infra/modules/pxd && make install debug.jar

lib libpxd.so:    $(objects)
	ld -m elf_i386 -o libpxd.so -r $^

always_remake:

#
#  test programs
#
pxd_client_test pxd_packet_test pxd_event_test pxd_circle pxd_triangle pxd_bw:    log.h always_remake
	g++ -m32 $(dincludes) -c $(vocal) $(configure) $@.cpp -o $@.o
	g++ -m32 -ggdb $(dincludes) -o $@ $@.cpp $(libs)

log.h:
	cp /dev/null log.h

gcov:  pxd_client.cpp log.h
	-mkdir total
	g++ -m32 --coverage -ggdb $(dincludes) -o pxd_client_test pxd_client_test.cpp $(libs)
	./pxd_client_test
	gcov pxd_client_test > gcov.out.1
	for i in *.cpp.gcov; do cp $$i total/; done
	g++ -m32 --coverage -ggdb $(dincludes) -o pxd_packet_test pxd_packet_test.cpp $(libs)
	./pxd_packet_test
	gcov pxd_packet_test > gcov.out.2
	for i in *.cpp.gcov; do ./gt $$i total/$$i > total/$$i.total; done
	g++ -m32 --coverage -ggdb $(dincludes) -o pxd_event_test pxd_event_test.cpp $(libs)
	./pxd_event_test
	gcov pxd_event_test > gcov.out.3
	for i in *.cpp.gcov; do ./gt $$i total/$$i > total/$$i.total; done
	rm -f total/*_test.*
	rm -f total/*.gcov
	make gcov_clean
	./tc

install:  pxd_circle
	rm -f ~/pxd-test/$^
	cp $^ ~/pxd-test

pxd_bw_prof: always_remake
	g++ -m32 -O1 -pg $(dincludes) -o pxd_bw pxd_bw.cpp $(libs)

ci:
	ci -l -m"New version" *.cpp pxd_test.h Makefile 2>&1 | grep -v "reverting to" | grep -v done
	cd ../include; ci -l -m"New version" *.h        2>&1 | grep -v "reverting to" | grep -v done

.PHONY: mostly_clean

mostly_clean:
	rm -f log.h
	rm -f libpxd.so $(objects)
	rm -f pxd_client_test pxd_client_test.o
	rm -f pxd_packet_test pxd_packet_test.o
	rm -f pxd_event_test  pxd_event_test.o
	rm -f vgcore.* core.*

.PHONY: gcov_clean

gcov_clean:
	rm -f gcov.out.[123] *.gcov *.gcda *.gcno

.PHONY: clean

clean:  mostly_clean gcov_clean
	rm  -f  pxd_circle      pxd_circle.o
	rm  -f  pxd_bw          pxd_bw.o
	rm  -f  pxd_triangle    pxd_triangle.o
	-rm -rf testing/conf    testing/jars    testing/libsetSockOpt.so
	-rm -rf total always_remake mostly_clean
