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

#ifndef __TS2_LINK_PRXP2PCONNHELPER_HPP__
#define __TS2_LINK_PRXP2PCONNHELPER_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>
#include <vpl_net.h>
#include <vpl_socket.h>
#include <vpl_th.h>
#include <vpl_thread.h>
#include <gvm_errors.h>
#include <log.h>

#include "ConnectHelper.hpp"
#include "p2p.hpp"
#include "pxd_client.h"

#include <map>
#include <string>

namespace Ts2 {
namespace Link {
class PrxP2PConnHelper : public ConnectHelper {
public:
    PrxP2PConnHelper(u64 remoteUserId, u64 remoteDeviceId, u32 remoteInstanceId,
                     ConnectedSocketHandler connectedSocketHandler, void *connectedSocketHandler_context,
                     LocalInfo *localInfo);
    virtual ~PrxP2PConnHelper();

    // Connect to the remote server, using a background thread.
    // When connection is established (and authenticated), the socket will be delivered by callback function.
    virtual int Connect();

    // Block until all work is done.
    // When this function returns, it is safe to destroy this object.
    virtual int WaitDone();

    // Force tear down any outgoing socket attempts
    // Caller also needs to call WaitDone() after this call to make sure every things are ok
    virtual void ForceClose();

private:
    VPL_DISABLE_COPY_AND_ASSIGN(PrxP2PConnHelper);

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
        State() : thread(THREAD_STATE_NOTHREAD), stop(STOP_STATE_NOSTOP) {}
        ThreadState thread;
        StopState stop;
    } state;

    static const size_t ThreadStackSize;
    VPLDetachableThreadHandle_t prxThread;
    VPLDetachableThreadHandle_t p2pThread;
    void prxThreadMain();
    void p2pThreadMain();
    static VPLTHREAD_FN_DECL threadMain(void *param);

    // PXD callbacks
    static void supplyLocal(pxd_cb_data_t *cb_data){/*We don't care this one*/};
    static void lookupDone(pxd_cb_data_t *cb_data){/*We don't care this one*/};
    static void incomingRequest(pxd_cb_data_t *cb_data){/*We don't care this one*/};
    static void incomingLogin(pxd_cb_data_t *cb_data){/*We don't care this one*/};

    static void rejectCcdCreds(pxd_cb_data_t *cb_data);
    static void rejectPxdCreds(pxd_cb_data_t *cb_data);
    static void connectDone(pxd_cb_data_t *cb_data);
    static void supplyExternal(pxd_cb_data_t *cb_data);
    static void loginDone(pxd_cb_data_t *cb_data);
    static void p2pDoneCb(int result, VPLSocket_t socket, void* ctxt);

    // For P2P setup
    enum ConnectionState {
        IN_PRX = 0,
        IN_P2P,
    };
    ConnectionState connState;

    VPLSocket_t prxSocket;
    VPLSocket_t p2pSocket;
    pxd_address_t localExtAddr;
    pxd_address_t remoteExtAddr;
    P2PHandle p2pHandle;
    pxd_client_t* pxdClient;

    mutable VPLCond_t cond;
};
} // end namespace Link
} // end namespace Ts2


#endif /* __TS2_LINK_PRXP2PCONNHELPER_HPP__ */
