#include "InterruptibleSocket_SigBySockShutdown.hpp"
#include "InterruptibleSocket_SigByCmdPipe.hpp"
#include "InterruptibleSocket_SigByCmdSocket.hpp"
#include "ConnectedSocketPair.hpp"

#include <log.h>

#include <cassert>
#include <iostream>

static void runSimpleIoTest(InterruptibleSocket *is[2])
{
    std::string testdata("1234567890");

    ssize_t numBytesWritten = is[0]->Write((const u8*)testdata.data(), testdata.size(), VPLTime_FromSec(10));
    assert(numBytesWritten == testdata.size());
    LOG_INFO("Wrote testdata");

    u8 buf[128];
    ssize_t numBytesRead = is[1]->Read(buf, ARRAY_ELEMENT_COUNT(buf), VPLTime_FromSec(10));
    assert(numBytesRead == testdata.size());
    assert(testdata.compare(0, std::string::npos, (const char*)buf, numBytesRead) == 0);
    LOG_INFO("Read testdata");
}

int main(int argc, char *argv[])
{
    LOGInit("SimpleIoTest", NULL);
    LOGSetMax(0);
    VPL_Init();

#define test(type)                                                      \
    {                                                                   \
        ConnectedSocketPair csp;                                        \
        VPLSocket_t sock[2];                                            \
        csp.GetSockets(sock);                                           \
        InterruptibleSocket *is[2] = { new InterruptibleSocket_ ## type (sock[0]), \
                                       new InterruptibleSocket_ ## type (sock[1]) }; \
        LOG_INFO("Running SimpleIoTest using InterruptibleSocket_" #type " objs"); \
        runSimpleIoTest(is);                                            \
        delete is[0];                                                   \
        delete is[1];                                                   \
    }
    test(SigBySockShutdown);
    test(SigByCmdPipe);
    test(SigByCmdSocket);
#undef test

    exit(0);
}
