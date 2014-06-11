VPL   := $(BUILDROOT)/debug/$(PRODUCT)/gvm_core/vpl/
VPLEX := $(BUILDROOT)/debug/$(PRODUCT)/gvm_core/vplex/
CSL   := $(BUILDROOT)/debug/$(PRODUCT)/gvm_core/internal/csl

includes := -I../../../vpl/include                    \
            -I../../../vpl/platform_linux/include     \
            -I../../../vpl/posix_include              \
            -I../../../vplex/include                  \
            -I../../../vplex/platform_include         \
            -I../../../vplex/posix_include            \
            -I../../../vplex/platform_linux/include   \
            -I../../csl/include                       \
            -I../../../../es_core/km_include          \
            -I../include                              \
            -I.

config  := -DPOSIX
defines := -DLOG_INFO=printf -DLOG_ERROR=printf -DLOG_FUNC_ENTRY="" -DLOG_LEVEL_INFO=2 -DLOG_WARN=printf
warn    := -fno-strict-aliasing -O3 -Wall -Werror -Wno-packed -Wpointer-arith -Wredundant-decls -Wstrict-aliasing=3 -Wswitch-enum -Wundef -Wwrite-strings -Wextra -Wno-unused-parameter -DLINUX -DHOST_IS_LITTLE_ENDIAN -DGVM -D_FILE_OFFSET_BITS=64 -DGVM_BUILD_INTERN -DLOG_ALWAYS=printf -m32 -Wno-sign-compare
libs    := -L$(VPL) -lvplcore -L$(VPLEX) -lvplexcore -lssl -lrt -lpthread -L$(CSL) -lcsl
tests   := ans_device_test ans_keep_test ans_back ans_circle noconnect
objects := ans_device.o $(patsubst %, %.o, $(tests))

all:    setup $(tests) unit_test

always_remake:

setup:
	cd $$WORKAREA/sw_x && make libans es_core vplex vpl
	cd $$WORKAREA/sw_i/infra/modules/ans && make install debug.jar

ans_device_test:	ans_device_test.cpp ans_device.cpp log.h ../include/ans_device.h
	g++ -m32 $(defines) $(includes) -c $(warn) ans_device.cpp
	g++ -m32 $(defines) $(includes) -c $(warn) ans_device_test.cpp
	g++ -m32 -ggdb $(includes) -o $@ $@.cpp $(libs)

ans_keep_test:	ans_keep_test.cpp ans_device.cpp log.h ../include/ans_device.h
	g++ -m32 $(defines) $(includes) -c $(warn) ans_device.cpp
	g++ -m32 $(defines) $(includes) -c $(warn) ans_keep_test.cpp
	g++ -m32 -ggdb $(includes) -o $@ $@.cpp $(libs)

ans_circle:	ans_circle.cpp ans_device.cpp log.h ../include/ans_device.h
	g++ -m32 $(defines) $(includes) -c $(warn) $@.cpp
	g++ -m32 -c -ggdb $(config) $(includes) $@.cpp
	g++ -m32 -ggdb -o $@ $@.o $(objs) $(libs)

ans_back:	ans_back.cpp ans_device.cpp log.h ../include/ans_device.h
	g++ -m32 -c -ggdb $(config) $(includes) $@.cpp
	g++ -m32 -ggdb -o $@ $@.o $(objs) $(libs)

noconnect:	noconnect.cpp ans_device.cpp log.h ../include/ans_device.h
	g++ -m32 -c -ggdb $(config) $(includes) $@.cpp
	g++ -m32 -ggdb -o $@ $@.o $(objs) $(libs)

test unit_test:	ans_device_test ans_keep_test ans_circle
	valgrind --leak-check=full --show-reachable=yes ./ans_device_test
	valgrind --leak-check=full --show-reachable=yes ./ans_keep_test

cluster_test: ans_circle
	rm -f ~/java/ans_circle
	cp ans_circle ~/java
	cd ~/java; ./run-device-all

log.h:
	cp /dev/null log.h

gprof:  ans_device_test.cpp ans_device.cpp log.h
	g++ -m32 -pg -ggdb $(includes) -o ans_device_test ans_device_test.cpp $(libs)
	./ans_device_test
	gprof ans_device_test > gprof.out

gcov:  ans_device_test.cpp ans_keep_test.cpp ans_device.cpp log.h always_remake
	if [ ! -d gcov ]; then mkdir gcov; fi
	g++ -m32 --coverage -ggdb $(includes) -o   ans_device_test  ans_device_test.cpp $(libs)
	g++ -m32 --coverage -ggdb $(includes) -o   ans_circle       ans_circle.cpp      $(libs)
	g++ -m32 --coverage -ggdb $(includes) -o   ans_keep_test    ans_keep_test.cpp   $(libs)
	./ans_device_test
	gcov ans_device_test > summary
	cp ans_device.cpp.gcov gcov/gcov.1
	./ans_keep_test
	gcov ans_keep_test > summary
	cp ans_device.cpp.gcov gcov/gcov.2
	./gt gcov/gcov.[12] > gcov/subtotal
	rm -f ~/export/ans_circle
	cp ans_circle ~/export
	cd ~/export; ./run-device-all
	gcov ans_circle
	cp ans_device.cpp.gcov gcov/gcov.3
	./gt gcov/subtotal gcov/gcov.3 > gcov/total

clean:
	rm -f $(tests)
	rm -f $(objects)
	rm -f log.h
	rm -f ans_device.libans.o
	rm -f gprof.out gmon.out
	rm -f vgcore.* core.* vgcore.*
	rm -f summary gmon.out *.gcov *.gcda *.gcno
	rm -f always_remake
	rm -f testing/device_types testing/server_keys testing/libsetSockOpt.so
	rm -rf gcov testing/jars

install:    ans_circle
	rm -f ~/java/ans_circle ; cp ans_circle ~/java/
	rm -f ans_back; make ans_back

ci:
	ci -l -m"New version" Makefile
	ci -l -m"New version" *.cpp
	cd ../include; ci -l -m"New version" *.h
