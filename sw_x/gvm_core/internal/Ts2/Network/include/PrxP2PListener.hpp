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

#ifndef __TS2_NETWORK_PRXP2P_LISTENER_HPP__
#define __TS2_NETWORK_PRXP2P_LISTENER_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>
#include <vpl_net.h>
#include <vpl_socket.h>
#include <vpl_th.h>
#include <vpl_thread.h>
#include <log.h>

#include "pxd_client.h"
#include <PrxP2PAcceptHelper.hpp>
#include <LocalInfo.hpp>

#include <string>
#include <queue>
#include <set>

namespace Ts2 {

namespace Network {

class PrxP2PListener {
public:
    PrxP2PListener(Link::AcceptedSocketHandler frontEndIncomingSocket, void *frontEndIncomingSocket_context,
                   LocalInfo *localInfo);
    ~PrxP2PListener();

    int Start();
    int Stop();
    int WaitStopped();

private:
    VPL_DISABLE_COPY_AND_ASSIGN(PrxP2PListener);

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

    enum PxdState {
        PXD_STATE_NOLISTEN = 0,
        PXD_STATE_INITIALIZING,
        PXD_STATE_LISTENING,
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
        State() : pxdState(PXD_STATE_NOLISTEN), thread(THREAD_STATE_NOTHREAD), stop(STOP_STATE_NOSTOP) {}
        PxdState pxdState;
        ThreadState thread;
        StopState stop;
    } state;

    LocalInfo *localInfo;

    static const size_t ThreadStackSize;
    VPLDetachableThreadHandle_t thread;
    void threadMain();
    static VPLTHREAD_FN_DECL threadMain(void *param);

    // Callback context for Pxd Client Libaray in Listener
    class PclListenerCtxt {
    public:
        PclListenerCtxt(PrxP2PListener* h) : helper(h){
        };
        ~PclListenerCtxt() {
        }
        PrxP2PListener* helper;
    };

    // PXD callbacks
    static void supplyLocal(pxd_cb_data_t *cb_data){/*We don't care this one*/};
    static void rejectPxdCreds(pxd_cb_data_t *cb_data);
    static void supplyExternal(pxd_cb_data_t *cb_data){/*We don't care this one*/};

    bool needToInitPxd;
    pxd_client_t* pxdClient;
    PclListenerCtxt openCtxt;
    VPLCond_t cond;

    int openPxdClient();
    int closePxdClient();

    static void ansNotifyCB(const char* buffer, u32 bufferLength, void* context);
    void ansIncomingClient(const char* buffer, u32 bufferLength);

    class ConnRequest {
    public:
        ConnRequest(const char* _buffer, u32 _bufferLength) {
            bufferLength = _bufferLength;
            buffer = (char*)malloc(sizeof(char) * bufferLength);
            memcpy(buffer, _buffer, bufferLength);
        }
        ~ConnRequest() {
            if(buffer) {
                free(buffer);
                buffer = NULL;
            }
        }
        char* buffer;
        u32 bufferLength;
    };

    std::queue<ConnRequest*> connReqQueue;

    std::set<Link::AcceptHelper*> activeAcceptHelperSet;
    std::queue<Link::AcceptHelper*> doneAcceptHelperQ;

    // Callback for handling device online event
    static void ansDeviceOnlineCB(void* instance, u64 deviceId);
    void ansDeviceOnline(u64 deviceId);
};
} // end namespace Network
} // end namespace Ts2

#endif // incl guard
