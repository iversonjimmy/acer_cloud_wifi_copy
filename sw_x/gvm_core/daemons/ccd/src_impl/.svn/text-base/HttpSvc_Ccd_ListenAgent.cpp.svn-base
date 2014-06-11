#include "HttpSvc_Ccd_ListenAgent.hpp"

#include "HttpSvc_Ccd_SocketAgent.hpp"
#include "HttpSvc_Ccd_Server.hpp"
#include "HttpSvc_Utils.hpp"

#include <gvm_errors.h>
#include <gvm_thread_utils.h>
#include <log.h>

#include <vpl_net.h>
#include <vpl_socket.h>
#include <vplex_socket.h>
#include <vplu_format.h>
#include <vplu_mutex_autolock.hpp>

#include <sstream>
#include <string>

// TODO for 2.7, Bug 13079:
// see if we can lower the stack requirement 16KB (UTIL_DEFAULT_THREAD_STACK_SIZE) for non-Android.
static const size_t ListenAgent_StackSize = 
#ifdef ANDROID
    UTIL_DEFAULT_THREAD_STACK_SIZE
#else
    128 * 1024
#endif
    ;

#ifdef DEBUG
#define LOG_STATE LOG_INFO("ListenAgent[%p]: State %s", this, getStateStr().c_str())
#else
#define LOG_STATE LOG_INFO("ListenAgent[%p]: State %x", this, getStateNum())
#endif

HttpSvc::Ccd::ListenAgent::ListenAgent(Server *server, u64 userId, VPLNet_addr_t addrSpec, VPLNet_port_t portSpec)
    : Agent(server, userId), addrSpec(addrSpec), portSpec(portSpec),
      socketState(SocketState_NoSocket), threadState(Utils::ThreadState_NoThread), cancelState(Utils::CancelState_NoCancel)
{
    LOG_INFO("ListenAgent[%p]: Created for User["FMTu64"]", this, userId);
}

HttpSvc::Ccd::ListenAgent::~ListenAgent()
{
    if (RefCounter > 0) {
        LOG_WARN("ListenAgent[%p]: RefCount %d", this, int(RefCounter));
    }

    LOG_INFO("ListenAgent[%p]: Destroyed", this);
}

int HttpSvc::Ccd::ListenAgent::Start()
{
    int err = 0;

    err = openSocket(addrSpec, portSpec);
    if (err && err != CCD_ERROR_ALREADY_INIT) {
        LOG_ERROR("ListenAgent[%p]: Failed to open service socket at "FMT_VPLNet_addr_t":%u: error %d.",
                  this, VAL_VPLNet_addr_t(addrSpec), portSpec, err);
        goto end;
    }

    do {
        {
            MutexAutoLock lock(&mutex);
            if (threadState != Utils::ThreadState_NoThread) {
                LOG_WARN("ListenAgent[%p]: Thread already spawning/running", this);
                break;
            }
            threadState = Utils::ThreadState_Spawning;
            LOG_STATE;
        }

        ListenAgent *copyOf_this = Utils::CopyPtr(this);  // REFCOUNT(ListenAgent,ForThread)
        err = Util_SpawnThread(thread_main, (void*)copyOf_this, ListenAgent_StackSize, /*isJoinable*/VPL_FALSE, &main_thread);
        if (err) {
            LOG_ERROR("ListenAgent[%p]: Failed to spawn thread: err %d", this, err);
            Utils::DestroyPtr(copyOf_this);  // REFCOUNT(ListenAgent,ForThread)
            goto end;
        }
    } while (0);

    {
        VPLNet_addr_t addr;
        VPLNet_port_t port;
        addr = VPLSocket_GetAddr(svcSocket);
        port = VPLSocket_GetPort(svcSocket);
        LOG_INFO("ListenAgent[%p]: Listening at "FMT_VPLNet_addr_t":%u on socket "FMT_VPLSocket_t,
                 this, VAL_VPLNet_addr_t(addr), VPLNet_port_ntoh(port), VAL_VPLSocket_t(svcSocket));
    }

 end:
    if (err) {
        AsyncStop();
    }

    return err;
}

int HttpSvc::Ccd::ListenAgent::AsyncStop()
{
    int err = 0;

    {
        MutexAutoLock lock(&mutex);
        cancelState = Utils::CancelState_Canceling;
        LOG_STATE;
    }

    closeSocket();
    // This should have the side effect of causing the listener thread to return from VPLSocket_Accept().

    return err;
} 

VPLNet_port_t HttpSvc::Ccd::ListenAgent::GetServicePort() const
{
    MutexAutoLock lock(&mutex);
    if (VPLSocket_Equal(svcSocket, VPLSOCKET_INVALID)) {
        return VPLNET_PORT_INVALID;
    }
    else {
        return VPLNet_port_ntoh(VPLSocket_GetPort(svcSocket));
    }
}

VPLTHREAD_FN_DECL HttpSvc::Ccd::ListenAgent::thread_main(void* param)
{
    ListenAgent *listener = static_cast<ListenAgent*>(param);

    listener->thread_main();

    Utils::DestroyPtr(listener);  // REFCOUNT(ListenAgent,ForThread)

    return VPLTHREAD_RETURN_VALUE;
}

void HttpSvc::Ccd::ListenAgent::thread_main()
{
    int err = 0;

    {
        MutexAutoLock lock(&mutex);
        threadState = Utils::ThreadState_Running;
        LOG_STATE;
    }

    while (cancelState != Utils::CancelState_Canceling) {
        VPLSocket_t connSocket;
        VPLSocket_addr_t addr;
        err = VPLSocket_Accept(svcSocket, &addr, sizeof(addr), &connSocket);
        if (cancelState == Utils::CancelState_Canceling) {
            break;
        }
        if (err) {
            LOG_ERROR("ListenAgent[%p]: Failed to accept connection: err %d", this, err);
#ifdef IOS
            // On iOS, recreated the socket.  See Bug 2329 for discussions.
            if (err == VPL_ERR_CONNABORT) {
                VPLNet_port_t curPort = 0;
                {
                    // Getting the port of the current listening socket rather than using the
                    // member variable portSpec because portSpec may be 0 to let the OS decide
                    // the port.  It is important that the same listening port is used, otherwise
                    // the change of listening port would need to be communicated to the App.
                    MutexAutoLock lock(&mutex);
                    if (!VPLSocket_Equal(svcSocket, VPLSOCKET_INVALID)) {
                        curPort = VPLNet_port_ntoh(VPLSocket_GetPort(svcSocket));
                    }
                }
                closeSocket();
                err = openSocket(addrSpec, curPort);
                if (err && err != CCD_ERROR_ALREADY_INIT) {
                    // no recovery - go into canceling
                    LOG_ERROR("Unrecoverable error openSocket("
                              FMT_VPLNet_addr_t":"FMT_VPLNet_port_t"):%d. "
                              "Program needs restart.",
                              VAL_VPLNet_addr_t(addrSpec), curPort, err);

                    MutexAutoLock lock(&mutex);
                    cancelState = Utils::CancelState_Canceling;
                    LOG_STATE;
                }
            }
#endif
            continue;
        }

        LOG_INFO("ListenAgent[%p]: Accepted connection from "FMT_VPLNet_addr_t":"FMT_VPLNet_port_t" on socket["FMT_VPLSocket_t"]",
                 this, VAL_VPLNet_addr_t(addr.addr), addr.port, VAL_VPLSocket_t(connSocket));
        err = handleNewConnection(connSocket);
        if (err) {
            LOG_ERROR("ListenAgent[%p]: Failed to handle new connection: %d", this, err);
            VPLSocket_Close(connSocket);
        }
    }

    {
        MutexAutoLock lock(&mutex);
        threadState = Utils::ThreadState_NoThread;
        cancelState = Utils::CancelState_NoCancel;
        LOG_STATE;
    }
}


int HttpSvc::Ccd::ListenAgent::handleNewConnection(VPLSocket_t connSocket)
{
    SocketAgent *agent = Utils::CopyPtr(new SocketAgent(server, userId, connSocket));  // REFCOUNT(SocketAgent,InitialCopy)

    if (server) {
        server->AddAgent(agent);
    }

    int err = agent->Start();
    if (err) {
        if (server) {
            server->DropAgent(agent);
        }
    }

    Utils::DestroyPtr(agent);  // REFCOUNT(SocketAgent,InitialCopy)

    return 0;
}


int HttpSvc::Ccd::ListenAgent::openSocket(VPLNet_addr_t addr, VPLNet_port_t port)
{
    MutexAutoLock lock(&mutex);

    if (socketState != SocketState_NoSocket) {
        LOG_ERROR("ListenAgent[%p]: Service socket already open", this);
        return CCD_ERROR_ALREADY_INIT;
    }

    int err = 0;
    VPLSocket_addr_t sin;
    VPLSocket_addr_t sin_tmp;
    int yes = 1;

    svcSocket = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_STREAM, /*nonblock*/VPL_FALSE);
    if (VPLSocket_Equal(svcSocket, VPLSOCKET_INVALID)) {
        LOG_ERROR("ListenAgent[%p]: Failed to create service socket", this);
        goto end;
    }

    err = VPLSocket_SetSockOpt(svcSocket, VPLSOCKET_SOL_SOCKET, VPLSOCKET_SO_REUSEADDR, &yes, sizeof(int));
    if (err) {
        LOG_ERROR("ListenAgent[%p]: Failed to set REUSEADDR on socket["FMT_VPLSocket_t"]: err %d",
                  this, VAL_VPLSocket_t(svcSocket), err);
        goto end;
    }
    
    err = VPLSocket_SetSockOpt(svcSocket, VPLSOCKET_IPPROTO_TCP, VPLSOCKET_TCP_NODELAY, &yes, sizeof(int));
    if (err) {
        LOG_ERROR("ListenAgent[%p]: Failed to set TCP_NODELAY on socket["FMT_VPLSocket_t"]: err %d",
                  this, VAL_VPLSocket_t(svcSocket), err);
        goto end;
    }

    sin.family = VPL_PF_INET;
    sin.addr = addr;
    sin.port = VPLConv_hton_u16(port);
    memcpy(&sin_tmp, &sin, sizeof(sin));

    err = VPLSocket_Bind(svcSocket, &sin_tmp, sizeof(sin));
    if (err) {
        LOG_ERROR("ListenAgent[%p]: Failed to bind socket["FMT_VPLSocket_t"]: err %d",
                  this, VAL_VPLSocket_t(svcSocket), err);
        goto end;
    }

    err = VPLSocket_Listen(svcSocket, 10);
    if (err) {
        LOG_ERROR("ListenAgent[%p]: Failed to listen socket["FMT_VPLSocket_t"]: err %d",
                  this, VAL_VPLSocket_t(svcSocket), err);
        goto end;
    }

    socketState = SocketState_Listening;
    LOG_STATE;

 end:
    return err;
}

int HttpSvc::Ccd::ListenAgent::closeSocket()
{
    MutexAutoLock lock(&mutex);
    if (!VPLSocket_Equal(svcSocket, VPLSOCKET_INVALID)) {
        VPLSocket_Close(svcSocket);
        LOG_INFO("ListenAgent[%p]: Closed socket["FMT_VPLSocket_t"]", this, VAL_VPLSocket_t(svcSocket));
        svcSocket = VPLSOCKET_INVALID;
        socketState = SocketState_NoSocket;
        LOG_STATE;
    }
    return 0;
}

const std::string &HttpSvc::Ccd::ListenAgent::getSocketStateStr() const
{
    static const std::string strNoSocket("NoSocket");
    static const std::string strListening("Listening");
    static const std::string strUnknown("Unknown");

    switch (socketState) {
    case SocketState_NoSocket:
        return strNoSocket;
    case SocketState_Listening:
        return strListening;
    default:
        return strUnknown;
    }
}

std::string HttpSvc::Ccd::ListenAgent::getStateStr() const
{
    MutexAutoLock lock(&mutex);
    std::ostringstream oss;

    oss << "<"
        << getSocketStateStr()
        << ","
        << Utils::GetThreadStateStr(threadState)
        << ","
        << Utils::GetCancelStateStr(cancelState)
        << ">";

    return oss.str();
}

int HttpSvc::Ccd::ListenAgent::getStateNum() const
{
    MutexAutoLock lock(&mutex);
    return (socketState << 8) | (threadState << 4) | cancelState;
}

