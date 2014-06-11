#include "InterruptibleSocket_SigBySockShutdown.hpp"
#include "InterruptibleSocket_SigByCmdPipe.hpp"
#include "InterruptibleSocket_SigByCmdSocket.hpp"
#include "ConnectedSocketPair.hpp"

#include <gvm_thread_utils.h>
#include <log.h>

#include <cassert>
#include <iostream>

static VPLTime_t readThread_ReadCallDelay = VPLTime_FromSec(5);
static VPLTime_t readThread_ReadCallTimeout = VPLTime_FromSec(60);  // this must be a much much larger value
static VPLTime_t mainThread_StopIoCallDelay = VPLTime_FromSec(10);  // this must be larger than readThread_ReadCallDelay

static VPLTime_t readThread_expectedTimeInReadCall = mainThread_StopIoCallDelay - readThread_ReadCallDelay;

static VPLTHREAD_FN_DECL readThreadMain(void *param)
{
    InterruptibleSocket *is = (InterruptibleSocket*)param;

    VPLThread_Sleep(readThread_ReadCallDelay);

    {
        LOG_INFO("Calling Read() with 60 sec timeout");
        VPLTime_t timestamp_call = VPLTime_GetTimeStamp();

        u8 buf[16];
        int err = is->Read(buf, sizeof(buf), readThread_ReadCallTimeout);
        // On Linux, I see poll() return VPLSOCKET_POLL_HUP | VPLSOCKET_POLL_RDNORM

        VPLTime_t timestamp_return = VPLTime_GetTimeStamp();
        VPLTime_t timeInReadCall = VPLTime_DiffAbs(timestamp_call, timestamp_return);
        LOG_INFO("Returned from Read() after "FMT_VPLTime_t" ms: ret %d", VPLTime_ToMillisec(timeInReadCall), err);
        assert(VPLTime_DiffAbs(timeInReadCall, readThread_expectedTimeInReadCall) < VPLTime_FromMillisec(100));  // check that it was within 100ms of expected value
    }

    // make sure second attempt to read will immediately return
    {
        LOG_INFO("Calling Read() with 60 sec timeout");
        VPLTime_t timestamp_call = VPLTime_GetTimeStamp();

        u8 buf[16];
        int err = is->Read(buf, sizeof(buf), readThread_ReadCallTimeout);
        // On Linux, I see poll() return VPLSOCKET_POLL_HUP | VPLSOCKET_POLL_RDNORM

        VPLTime_t timestamp_return = VPLTime_GetTimeStamp();
        VPLTime_t timeInReadCall = VPLTime_DiffAbs(timestamp_call, timestamp_return);
        LOG_INFO("Returned from Read() after "FMT_VPLTime_t" ms: ret %d", VPLTime_ToMillisec(timeInReadCall), err);
        assert(timeInReadCall < VPLTime_FromMillisec(100));  // check that it was within 100ms of expected value
    }

    return VPLTHREAD_RETURN_VALUE;
}

static void runFreeBlockedReadTest(InterruptibleSocket *is[2])
{
    VPLDetachableThreadHandle_t readThread;
    {
        int err = Util_SpawnThread(readThreadMain, is[0], UTIL_DEFAULT_THREAD_STACK_SIZE, /*isJoirnable*/VPL_TRUE, &readThread);
        assert(!err);
    }

    VPLThread_Sleep(mainThread_StopIoCallDelay);

    LOG_INFO("Calling StopIo");
    is[0]->StopIo();
    LOG_INFO("Returned from StopIo");

    {
        int err = VPLDetachableThread_Join(&readThread);
        assert(!err);
    }
}

int main(int argc, char *argv[])
{
    LOGInit("FreeBlockedReadTest", NULL);
    LOGSetMax(0);
    VPL_Init();

#define test(type)                                                      \
    {                                                                   \
        ConnectedSocketPair csp;                                        \
        VPLSocket_t sock[2];                                            \
        csp.GetSockets(sock);                                           \
        InterruptibleSocket *is[2] = { new InterruptibleSocket_ ## type (sock[0]), \
                                       new InterruptibleSocket_ ## type (sock[1]) }; \
        LOG_INFO("Running FreeBlockedReadTest using InterruptibleSocket_" #type " objs"); \
        runFreeBlockedReadTest(is);                                     \
        delete is[0];                                                   \
        delete is[1];                                                   \
    }
    test(SigBySockShutdown);
    test(SigByCmdPipe);
    test(SigByCmdSocket);
#undef test

    exit(0);
}
