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

#include "TcpDinListener.hpp"

#include <gvm_errors.h>
#include <gvm_thread_utils.h>
#include <log.h>

#include <scopeguard.hpp>
#include <vpl_time.h>
#include <vplex_socket.h>
#include <vplu_format.h>
#include <vplu_mutex_autolock.hpp>

#include <cassert>
#include <sstream>
#include <string>

const size_t Ts2::Network::TcpDinListener::ThreadStackSize = UTIL_DEFAULT_THREAD_STACK_SIZE;

Ts2::Network::TcpDinListener::TcpDinListener(VPLNet_addr_t addrSpec, VPLNet_port_t portSpec, 
                                             Link::AcceptedSocketHandler frontEndIncomingSocket, void *frontEndIncomingSocket_context,
                                             LocalInfo *localInfo)
    : addrPortSpec(addrSpec, portSpec), 
      frontEndIncomingSocket(frontEndIncomingSocket), frontEndIncomingSocket_context(frontEndIncomingSocket_context),
      localInfo(localInfo),
      listenSocket(VPLSOCKET_INVALID)
{
    assert(frontEndIncomingSocket);
    assert(localInfo);

    VPLMutex_Init(&mutex);

    LOG_INFO("TcpDinListener[%p]: Created", this);
}

Ts2::Network::TcpDinListener::~TcpDinListener()
{
    if (!VPLSocket_Equal(listenSocket, VPLSOCKET_INVALID)) {
        LOG_WARN("TcpDinListener[%p]: ListenSocket still open", this);
        VPLSocket_Close(listenSocket);
    }

    VPLMutex_Destroy(&mutex);

    LOG_INFO("TcpDinListener[%p]: Destroyed", this);
}

int Ts2::Network::TcpDinListener::Start()
{
    int err = 0;

    {
        MutexAutoLock lock(&mutex);
        if (state.stop != STOP_STATE_NOSTOP) {
            LOG_ERROR("TcpDinListener[%p]: Wrong stop state %d", this, state.stop);
            err = TS_ERR_WRONG_STATE;
            goto end;
        }
    }

    {
        MutexAutoLock lock(&mutex);
        if (state.socket != SOCKET_STATE_NOLISTEN) {
            LOG_ERROR("TcpDinListener[%p]: Wrong socket state %d", this, state.socket);
            err = TS_ERR_WRONG_STATE;
            goto end;
        }
        if (!VPLSocket_Equal(listenSocket, VPLSOCKET_INVALID)) {
            LOG_ERROR("TcpDinListener[%p]: ListenSocket already opened", this);
            err = TS_ERR_WRONG_STATE;
            goto end;
        }
    }
    err = openSocket();
    if (err) {
        // ErrMsg logged by openSocket().
        goto end;
    }

    {
        MutexAutoLock lock(&mutex);
        if (state.thread != THREAD_STATE_NOTHREAD) {
            LOG_ERROR("TcpDinListener[%p]: Wrong thread state %d", this, state.thread);
            err = TS_ERR_WRONG_STATE;
            goto end;
        }
        state.thread = THREAD_STATE_SPAWNING;

        err = Util_SpawnThread(threadMain, this, ThreadStackSize, /*isJoinable*/VPL_TRUE, &thread);
        if (err) {
            LOG_ERROR("TcpDinListener[%p]: Failed to spawn thread: err %d", this, err);
            state.thread = THREAD_STATE_NOTHREAD;
            goto end;
        }
    }

 end:
    return err;
}

int Ts2::Network::TcpDinListener::Stop()
{
    int err = 0;

    {
        MutexAutoLock lock(&mutex);
        if (state.stop == STOP_STATE_STOPPING) {
            LOG_WARN("TcpDinListener[%p]: Stop state is already %d", this, state.stop);
            return 0;
        }
        state.stop = STOP_STATE_STOPPING;
    }

    err = closeSocket();
    if (err) {
        // ErrMsg logged by closeSocket().
        goto end;
    }

 end:
    return err;
}

int Ts2::Network::TcpDinListener::WaitStopped()
{
    int err = 0;

    {
        MutexAutoLock lock(&mutex);
        if (state.stop != STOP_STATE_STOPPING) {
            LOG_ERROR("TcpDinListener[%p]: Wrong stop state %d", this, state.stop);
            err = TS_ERR_WRONG_STATE;
            goto end;
        }
    }

    VPLMutex_Lock(&mutex);
    if (state.thread != THREAD_STATE_NOTHREAD) {
        VPLMutex_Unlock(&mutex);
        err = VPLDetachableThread_Join(&thread);
        VPLMutex_Lock(&mutex);
        if (err) {
            LOG_ERROR("TcpDinListener[%p]: Failed to join thread: err %d", this, err);
            // Do not reset the error.
            // We don't know the state of the thread, so it is not safe to destroy this object.
        }
        else {
            state.thread = THREAD_STATE_NOTHREAD;
        }
    }
    VPLMutex_Unlock(&mutex);

    {   // Stop all AcceptHelper
        MutexAutoLock lock(&mutex);
        std::set<Link::AcceptHelper*>::iterator it;
        while ((it = activeAcceptHelperSet.begin()) != activeAcceptHelperSet.end()) {
            Link::AcceptHelper *ah = *it;
            activeAcceptHelperSet.erase(it);
            VPLMutex_Unlock(&mutex);
            ah->ForceClose();
            err = ah->WaitDone();
            VPLMutex_Lock(&mutex);
            if (err) {
                LOG_ERROR("TcpDinListener[%p]: WaitDone failed on AcceptHelper[%p]: err %d", this, ah, err);
                err = 0;  // Reset error and continue;
                // We don't know the state of the object, so we won't destroy it and let it leak.
            }
            else {
                delete ah;
            }
        }

        while (doneAcceptHelperQ.size() > 0) {
            Link::AcceptHelper *ah = doneAcceptHelperQ.front();
            doneAcceptHelperQ.pop();
            VPLMutex_Unlock(&mutex);
            err = ah->WaitDone();
            VPLMutex_Lock(&mutex);
            if (err) {
                LOG_ERROR("TcpDinListener[%p]: WaitDone failed on AcceptHelper[%p]: err %d", this, ah, err);
                err = 0;  // Reset error and continue;
                // We don't know the state of the object, so we won't destroy it and let it leak.
            }
            else {
                delete ah;
            }
        }
    }

 end:
    return err;
}

VPLNet_port_t Ts2::Network::TcpDinListener::GetPort() const
{
    MutexAutoLock lock(&mutex);
    if (VPLSocket_Equal(listenSocket, VPLSOCKET_INVALID)) {
        return VPLNET_PORT_INVALID;
    }
    else {
        return VPLNet_port_ntoh(VPLSocket_GetPort(listenSocket));
    }
}

VPLTHREAD_FN_DECL Ts2::Network::TcpDinListener::threadMain(void *param)
{
    TcpDinListener *agent = static_cast<TcpDinListener*>(param);
    agent->threadMain();
    return VPLTHREAD_RETURN_VALUE;
}

void Ts2::Network::TcpDinListener::threadMain()
{
    {
        MutexAutoLock lock(&mutex);
        if (state.thread != THREAD_STATE_SPAWNING) {
            LOG_WARN("TcpDinListener[%p]: Unexpected thread state %d", this, state.thread);
        }
        state.thread = THREAD_STATE_RUNNING;
        LOG_INFO("TcpDinListener[%p]: Thread running", this);
    }

    int err = 0;

    while (1) {
        err = waitConnectionAttempt();

        if (err == TS_ERR_TIMEOUT) {
            // Clean up AcceptHelper
            MutexAutoLock lock(&mutex);
            while (!doneAcceptHelperQ.empty()) {
                Link::AcceptHelper *ah = NULL;
                ah = doneAcceptHelperQ.front();
                doneAcceptHelperQ.pop();
                VPLMutex_Unlock(&mutex);
                err = ah->WaitDone();
                VPLMutex_Lock(&mutex);
                if(err) {
                    // ErrMsg logged by WaitDone().
                } else {
                    LOG_INFO("TcpDinListener[%p]: Delete one TcpDinAcceptHelper[%p]", this, ah);
                    delete ah;
                }
            }
            continue;
        }

        if (err) {
            // ErrMsg logged by waitConnectionAttempt().
            goto end;
        }

        VPLSocket_t _socket = VPLSOCKET_SET_INVALID;
        VPLSocket_addr_t addr;
        err = VPLSocket_Accept(listenSocket, &addr, sizeof(addr), &_socket);
        if (err) {
            LOG_ERROR("TcpDinListener[%p]: Failed to accept on listenSocket: err %d", this, err);
            goto end;
        }
        LOG_INFO("TcpDinListener[%p]: Connection from "FMT_VPLNet_addr_t":"FMT_VPLNet_port_t,
                 this, VAL_VPLNet_addr_t(addr.addr), VPLNet_port_ntoh(addr.port));


        Link::TcpDinAcceptHelper *tdah = new (std::nothrow) Link::TcpDinAcceptHelper(acceptedSocketHandler, this,
                                                                                     _socket,
                                                                                     localInfo);
        if (!tdah) {
            LOG_ERROR("TcpDinListener[%p]: No memory to create TcpDinAcceptHelper obj", this);
            VPLSocket_Close(_socket);
            continue;
        }

        err = tdah->Accept();
        if (err) {
            // ErrMsg logged by Accept().
            delete tdah;
        } else {
            // Put AcceptHelper into tracking set
            MutexAutoLock lock(&mutex);
            activeAcceptHelperSet.insert(tdah);
        }
    }

 end:
    {
        MutexAutoLock lock(&mutex);
        state.thread = THREAD_STATE_EXITING;
        LOG_INFO("TcpDinListener[%p]: Thread exiting", this);
    }
}

int Ts2::Network::TcpDinListener::openSocket()
{
    int err = 0;

    VPLSocket_t _socket = VPLSocket_CreateTcp(addrPortSpec.addr, addrPortSpec.port);
    if (VPLSocket_Equal(_socket, VPLSOCKET_INVALID)) {
        LOG_ERROR("TcpDinListener[%p]: Failed to create service socket", this);
        err = TS_ERR_COMM;
        goto end;
    }

    err = VPLSocket_Listen(_socket, 10);
    if (err) {
        LOG_ERROR("TcpDinListener[%p]: Failed to listen socket["FMT_VPLSocket_t"]: err %d",
                  this, VAL_VPLSocket_t(_socket), err);
        goto end;
    }

 end:
    if (err) {
        if (!VPLSocket_Equal(_socket, VPLSOCKET_INVALID)) {
            VPLSocket_Close(_socket);
        }
    }
    else {
        MutexAutoLock lock(&mutex);
        if (state.socket != SOCKET_STATE_NOLISTEN) {
            LOG_WARN("TcpDinListener[%p]: Unexpected socket state %d", this, state.socket);
        }
        listenSocket = _socket;
        state.socket = SOCKET_STATE_LISTENING;
    }
    return err;
}

int Ts2::Network::TcpDinListener::waitConnectionAttempt()
{
    int err = 0;

    VPLSocket_poll_t pollspec[1];
    pollspec[0].socket = listenSocket;
    pollspec[0].events = VPLSOCKET_POLL_RDNORM;
    int numSockets = VPLSocket_Poll(pollspec, 1, VPLTime_FromSec(30));
    if (numSockets < 0) {
        LOG_ERROR("TcpDinListener[%p]: Poll failed: err %d", this, err);
        err = numSockets;
        goto end;
    }
    if (numSockets == 0) {
        err = TS_ERR_TIMEOUT;
        goto end;
    }
    if (numSockets != 1) {
        // THIS SHOULD NEVER HAPPEN.
        // > 1 is impossible, because we only supplied one socket.
        LOG_ERROR("TcpDinListener[%p]: Poll returned unexpected %d", this, numSockets);
        err = TS_ERR_INTERNAL;
        goto end;
    }
    if (pollspec[0].revents != VPLSOCKET_POLL_RDNORM) {
        LOG_ERROR("TcpDinListener[%p]: Poll returned unexpected event %d", this, pollspec[0].revents);
        err = TS_ERR_COMM;
        goto end;
    }

 end:
    return err;
}

int Ts2::Network::TcpDinListener::closeSocket()
{
    int err = 0;

    MutexAutoLock lock(&mutex);

    if (!VPLSocket_Equal(listenSocket, VPLSOCKET_INVALID)) {
        err = VPLSocket_Close(listenSocket);
        if (err) {
            LOG_ERROR("TcpDinListener[%p]: Failed to close socket: err %d", this, err);
            err = 0;  // reset error
        }

        // Even if close failed, assume it succeeded and so change state.
        listenSocket = VPLSOCKET_INVALID;
        state.socket = SOCKET_STATE_NOLISTEN;
    }

    return err;
}

// class method
void Ts2::Network::TcpDinListener::acceptedSocketHandler(VPLSocket_t socket, Link::RouteType routeType,
                                                         u64 remoteUserId, u64 remoteDeviceId, u32 remoteInstanceId,
                                                         const std::string *sessionKey,
                                                         Ts2::Link::AcceptHelper *acceptHelper, void *context)
{
    TcpDinListener *helper = static_cast<TcpDinListener*>(context);
    helper->acceptedSocketHandler(socket, routeType, remoteUserId, remoteDeviceId, remoteInstanceId, sessionKey, acceptHelper);
}

void Ts2::Network::TcpDinListener::acceptedSocketHandler(VPLSocket_t socket, Link::RouteType routeType,
                                                         u64 remoteUserId, u64 remoteDeviceId, u32 remoteInstanceId,
                                                         const std::string *sessionKey,
                                                         Ts2::Link::AcceptHelper *acceptHelper)
{
    if (routeType == Link::ROUTE_TYPE_INVALID) {
        // This means there are no more accepted sockets.
        MutexAutoLock lock(&mutex);
        if (state.stop != STOP_STATE_STOPPING ) {
            activeAcceptHelperSet.erase(acceptHelper);
            doneAcceptHelperQ.push(acceptHelper);
        }
        return;
    }
    assert(routeType == Link::ROUTE_TYPE_DIN ||
           routeType == Link::ROUTE_TYPE_DEX ||
           routeType == Link::ROUTE_TYPE_P2P ||
           routeType == Link::ROUTE_TYPE_PRX);

    if (VPLSocket_Equal(socket, VPLSOCKET_INVALID)) {
        LOG_ERROR("FrontEnd[%p]: Invalid socket", this);
        return;
    }

    frontEndIncomingSocket(socket, routeType, remoteUserId, remoteDeviceId, remoteInstanceId, sessionKey, acceptHelper, frontEndIncomingSocket_context);
}
