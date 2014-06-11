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

#ifndef __TS2_LINK_PRXP2PACCEPTHELPER_HPP__
#define __TS2_LINK_PRXP2PACCEPTHELPER_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>
#include <vpl_net.h>
#include <vpl_socket.h>
#include <vpl_th.h>
#include <vpl_thread.h>
#include <gvm_errors.h>
#include <log.h>

#include "AcceptHelper.hpp"
#include "p2p.hpp"
#include "pxd_client.h"

#include <map>
#include <string>

namespace Ts2 {
namespace Link {

class PrxP2PAcceptHelper : public AcceptHelper {
public:
    PrxP2PAcceptHelper(AcceptedSocketHandler acceptedSocketHandler,
                       void *acceptedSocketHandler_context,
                       pxd_client_t* pxdClient, const char* buffer, u32 bufferLength,
                       LocalInfo *localInfo);
    virtual ~PrxP2PAcceptHelper();

    virtual int Accept();

    virtual int WaitDone();

    // Force tear down any outgoing socket attempts
    // Caller also needs to call WaitDone() after this call to make sure every things are ok
    virtual void ForceClose();

    // PXD callbacks
    static void LookupDone(pxd_cb_data_t *cb_data){/*We don't care this one*/};
    static void ConnectDone(pxd_cb_data_t *cb_data) {/*We don't care this one*/};
    static void RejectCcdCreds(pxd_cb_data_t *cb_data){/*We don't care this one*/};

    static void IncomingRequest(pxd_cb_data_t *cb_data);
    static void IncomingLogin(pxd_cb_data_t *cb_data);

private:
    VPL_DISABLE_COPY_AND_ASSIGN(PrxP2PAcceptHelper);

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
    pxd_address_t remoteExtAddr;

    pxd_client_t* pxdClient;
    char* buffer;
    u32 bufferLength;

    u64 remoteUserId;
    u64 remoteDeviceId;
    u32 remoteInstanceId;
    P2PHandle p2pHandle;

    mutable VPLCond_t cond;
};
} // end namespace Link
} // end namespace Ts2


#endif /* __TS2_LINK_PRXP2PACCEPTHELPER_HPP__ */
