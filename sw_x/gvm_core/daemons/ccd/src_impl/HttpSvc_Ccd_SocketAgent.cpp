#include "HttpSvc_Ccd_SocketAgent.hpp"

#include "HttpSvc_Ccd_Dispatcher.hpp"
#include "HttpSvc_Ccd_Server.hpp"
#include "HttpSvc_Utils.hpp"

#include <gvm_errors.h>
#include <gvm_thread_utils.h>
#include <HttpSocketStream.hpp>
#include <log.h>
#include <SocketStream.hpp>

#include <vpl_socket.h>
#include <vplu_mutex_autolock.hpp>

#include <sstream>

#ifdef DEBUG
#define LOG_STATE LOG_INFO("SocketAgent[%p]: State %s", this, getStateStr().c_str())
#else
#define LOG_STATE LOG_INFO("SocketAgent[%p]: State %x", this, getStateNum())
#endif

#define SOCKET_RECV_BUFFER_SIZE (32*1024)

// TODO for 2.7, Bug 13079:
// see if we can lower the stack requirement 16KB (UTIL_DEFAULT_THREAD_STACK_SIZE) for non-Android.
static const size_t SocketAgent_StackSize =
#ifdef ANDROID
    UTIL_DEFAULT_THREAD_STACK_SIZE
#else
    128 * 1024
#endif
    ;

HttpSvc::Ccd::SocketAgent::SocketAgent(Server *server, u64 userId, VPLSocket_t socket)
    : Agent(server, userId), socket(socket), isSocketClosed(false),
      threadState(Utils::ThreadState_NoThread), cancelState(Utils::CancelState_NoCancel)
{
    LOG_INFO("SocketAgent[%p]: Created for socket "FMT_VPLSocket_t, this, VAL_VPLSocket_t(socket));
}

HttpSvc::Ccd::SocketAgent::~SocketAgent()
{
    if (RefCounter > 0) {
        LOG_WARN("SocketAgent[%p]: RefCount %d", this, int(RefCounter));
    }

    closeSocket();

    LOG_INFO("SocketAgent[%p]: Destroyed", this);
}

int HttpSvc::Ccd::SocketAgent::closeSocket()
{
    if (!VPLSocket_Equal(socket, VPLSOCKET_INVALID) && !isSocketClosed) {
        VPLSocket_Close(socket);
        isSocketClosed = true;
        LOG_INFO("SocketAgent[%p]: Closed socket "FMT_VPLSocket_t, this, VAL_VPLSocket_t(socket));
    }
    return 0;
}

int HttpSvc::Ccd::SocketAgent::Start()
{
    int err = 0;

    do {
        {
            MutexAutoLock lock(&mutex);
            if (threadState != Utils::ThreadState_NoThread) {
                LOG_WARN("Listener[%p]: Thread already spawning/running", this);
                break;
            }
            threadState = Utils::ThreadState_Spawning;
            LOG_STATE;
        }

        SocketAgent *copyOf_this = Utils::CopyPtr(this);  // REFCOUNT(SocketAgent,ForThread)
        err = Util_SpawnThread(threadMain, (void*)copyOf_this, SocketAgent_StackSize, /*isJoinable*/VPL_FALSE, &thread);
        if (err) {
            LOG_ERROR("SocketAgent[%p]: Failed to spawn thread: err %d", this, err);
            Utils::DestroyPtr(copyOf_this);  // REFCOUNT(SocketAgent,ForThread)
            goto end;
        }
    } while (0);

 end:
    if (err) {
        AsyncStop();
    }

    return err;
}

int HttpSvc::Ccd::SocketAgent::AsyncStop()
{
    int err = 0;

    {
        MutexAutoLock lock(&mutex);
        if (cancelState != Utils::CancelState_Canceling) {
            cancelState = Utils::CancelState_Canceling;
            LOG_STATE;
        }
    }

    closeSocket();
    // The above will cause recv thread to unblock.

    server->DropAgent(this);

    return err;
}

VPLTHREAD_FN_DECL HttpSvc::Ccd::SocketAgent::threadMain(void* param)
{
    HttpSvc::Ccd::SocketAgent *sagent = static_cast<HttpSvc::Ccd::SocketAgent*>(param);

    sagent->threadMain();

    Utils::DestroyPtr(sagent);  // REFCOUNT(SocketAgent,ForThread)

    return VPLTHREAD_RETURN_VALUE;
}

void HttpSvc::Ccd::SocketAgent::threadMain()
{
    {
        MutexAutoLock lock(&mutex);
        threadState = Utils::ThreadState_Running;
        LOG_STATE;
    }

    SocketStream *ss = new (std::nothrow) SocketStream(socket);
    if (!ss) {
        LOG_ERROR("SocketAgent[%p]: No memory to create SocketStream obj", this);
    }

    while (ss) {
        HttpSocketStream *hss = new (std::nothrow) HttpSocketStream(ss);
        if (!hss) {
            LOG_ERROR("SocketAgent[%p]: No memory to create HttpSocketStream obj", this);
            break;
        }
        LOG_INFO("SocketAgent[%p]: Created HttpSocketStream[%p]", this, hss);
        hss->SetUserId(userId);
        int err = HttpSvc::Ccd::Dispatcher::Dispatch(hss);

        delete hss;

        if (err) break;
    }

    if (ss)
        delete ss;

    AsyncStop();

    {
        MutexAutoLock lock(&mutex);
        threadState = Utils::ThreadState_NoThread;
        LOG_STATE;
    }
}

std::string HttpSvc::Ccd::SocketAgent::getStateStr() const
{
    MutexAutoLock lock(&mutex);
    std::ostringstream oss;

    oss << "<"
        << "Recv" << Utils::GetThreadStateStr(threadState)
        << ","
        << Utils::GetCancelStateStr(cancelState)
        << ">";

    return oss.str();
}

int HttpSvc::Ccd::SocketAgent::getStateNum() const
{
    MutexAutoLock lock(&mutex);
    return (threadState << 4) | cancelState;
}

