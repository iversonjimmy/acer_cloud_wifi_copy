#include <vplu_types.h>

#include "HttpSvc_Listener.hpp"

#include <log.h>

#include <vpl_net.h>
#include <vpl_socket.h>
#include <vpl_thread.h>
#include <vpl_time.h>

#include <iostream>
#include <cassert>

class TestHttpSvcListener {
public:
    void RunTests();
private:
    void testStartStop();
};

void TestHttpSvcListener::testStartStop()
{
    HttpSvc::Listener *listener = new HttpSvc::Listener(/*Server*/NULL, /*userId*/12345, /*deviceId*/11223344, VPLNET_ADDR_LOOPBACK, VPLNET_PORT_ANY);
    listener->Start();

    VPLThread_Sleep(VPLTime_FromSec(1));

    listener->Stop();

    VPLThread_Sleep(VPLTime_FromSec(1));
}

void TestHttpSvcListener::RunTests()
{
#define RUN_TEST(name)                                  \
    test##name();                                       \
    std::cout << "test" #name << ": OK" << std::endl

    RUN_TEST(StartStop);
#undef RUN_TEST
}

#ifdef WIN32
//static
#endif // WIN32
int main(int argc, char *argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " workdir" << std::endl;
        exit(0);
    }

    VPLSocket_Init();
    TestHttpSvcListener thsl;
    thsl.RunTests();
    VPLSocket_Quit();

    return 0;
}

#ifdef WIN32
int wmain(int argc, wchar_t *argv[])
{
    int exitcode = 0;
    char **argv_utf8 = new char* [argc + 1];
    for (int i = 0; i < argc; i++) {
        int rc = _VPL__wstring_to_utf8_alloc(argv[i], &argv_utf8[i]);
        if (rc != VPL_OK) {
            fprintf(stderr, "Failed to convert argv[%d]\n", i);  // our logger is not yet ready so call stdio
            exitcode = -1;
            goto end;
        }
    }
    argv_utf8[argc] = NULL;
    exitcode = main(argc, argv_utf8);

 end:
    delete [] argv_utf8;
    return exitcode;
}
#endif // WIN32
