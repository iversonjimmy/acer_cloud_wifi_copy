/*
 *  Copyright 2013 Acer Cloud Technology, Inc.
 *  All Rights Reserved.
 *
 *  This software contains confidential information and
 *  trade secrets of Acer Cloud Technology, Inc.
 *  Use, disclosure or reproduction is prohibited without
 *  the prior express written permission of Acer Cloud
 *  Technology, Inc.
 */

#ifndef __TS2_NETWORK_TCP_DIN_LISTENER_HPP__
#define __TS2_NETWORK_TCP_DIN_LISTENER_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>
#include <vpl_net.h>
#include <vpl_socket.h>
#include <vpl_th.h>
#include <vpl_thread.h>

#include <TcpDinAcceptHelper.hpp>
#include <LocalInfo.hpp>

#include <string>
#include <queue>
#include <set>

namespace Ts2 {

namespace Network {

typedef void (*AcceptHandler)(VPLSocket_t socket, void *context);

class TcpDinListener {
public:
    TcpDinListener(VPLNet_addr_t addrSpec, VPLNet_port_t portSpec, 
                   Link::AcceptedSocketHandler frontEndIncomingSocket, void *fronEndIncomingSocket_context,
                   LocalInfo *localInfo);
    ~TcpDinListener();

    int Start();
    int Stop();
    int WaitStopped();

    VPLNet_port_t GetPort() const;

private:
    VPL_DISABLE_COPY_AND_ASSIGN(TcpDinListener);

    struct AddrPortSpec {
        AddrPortSpec() : addr(VPLNET_ADDR_ANY), port(VPLNET_PORT_ANY) {}
        AddrPortSpec(VPLNet_addr_t addr, VPLNet_port_t port) : addr(addr), port(port) {}
        VPLNet_addr_t addr;
        VPLNet_port_t port;
    };
    AddrPortSpec addrPortSpec;

    const Link::AcceptedSocketHandler frontEndIncomingSocket;
    void *const frontEndIncomingSocket_context;

    void acceptedSocketHandler(VPLSocket_t socket, Link::RouteType routeType,
                               u64 remoteUserId, u64 remoteDeviceId, u32 remoteInstanceId,
                               const std::string *sessionKey,
                               Ts2::Link::AcceptHelper *acceptHelper);
    static void acceptedSocketHandler(VPLSocket_t socket, Link::RouteType routeType,
                                      u64 remoteUserId, u64 remoteDeviceId, u32 remoteInstanceId,
                                      const std::string *sessionKey,
                                      Ts2::Link::AcceptHelper *acceptHelper, void *acceptedSocketHandler_context);

    mutable VPLMutex_t mutex;

    enum SocketState {
        SOCKET_STATE_NOLISTEN = 0,
        SOCKET_STATE_LISTENING,
    };
    enum ThreadState {
        THREAD_STATE_NOTHREAD = 0,
        THREAD_STATE_SPAWNING,
        THREAD_STATE_RUNNING,
        THREAD_STATE_EXITING,
    };
    enum StopState {
        STOP_STATE_NOSTOP = 0,
        STOP_STATE_STOPPING,
    };
    struct State {
        State() : socket(SOCKET_STATE_NOLISTEN), thread(THREAD_STATE_NOTHREAD), stop(STOP_STATE_NOSTOP) {}
        SocketState socket;
        ThreadState thread;
        StopState stop;
    } state;

    LocalInfo *localInfo;

    static const size_t ThreadStackSize;
    VPLDetachableThreadHandle_t thread;
    void threadMain();
    static VPLTHREAD_FN_DECL threadMain(void *param);

    VPLSocket_t listenSocket;
    int openSocket();
    int waitConnectionAttempt();
    int closeSocket();

    std::set<Link::AcceptHelper*> activeAcceptHelperSet;
    std::queue<Link::AcceptHelper*> doneAcceptHelperQ;
};
} // end namespace Network
} // end namespace Ts2

#endif // incl guard
