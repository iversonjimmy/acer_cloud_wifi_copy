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

#ifndef __TS2_LINK_TCP_DIN_ACCEPT_HELPER_HPP__
#define __TS2_LINK_TCP_DIN_ACCEPT_HELPER_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>
#include <vpl_net.h>
#include <vpl_socket.h>
#include <vpl_th.h>
#include <vpl_thread.h>

#ifndef TS2_NO_PXD
#include <pxd_client.h>
#endif

#include "AcceptHelper.hpp"

#include <string>


namespace Ts2 {
namespace Link {
class TcpDinAcceptHelper : public AcceptHelper {
public:
    TcpDinAcceptHelper(AcceptedSocketHandler acceptedSocketHandler, void *context,
                       VPLSocket_t socket,
                       LocalInfo *localInfo);
    virtual ~TcpDinAcceptHelper();

    virtual int Accept();
    virtual int WaitDone();

    // Force tear down any outgoing socket attempts
    // Caller also needs to call WaitDone() after this call to make sure every things are ok
    virtual void ForceClose();

private:
    VPL_DISABLE_COPY_AND_ASSIGN(TcpDinAcceptHelper);

    VPLSocket_t socket;

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
    VPLDetachableThreadHandle_t thread;
    void threadMain();
    static VPLTHREAD_FN_DECL threadMain(void *param);

#ifndef TS2_NO_PXD
    mutable VPLCond_t cond;
    static void pxdLoginDone(pxd_cb_data_t *data);
    void setRemoteUdi(pxd_cb_data_t *data);
    int pxdLogin_result;
    u64 pxd_remoteUserId;
    u64 pxd_remoteDeviceId;
    u32 pxd_remoteInstanceId;
#endif

    int recvClientId(VPLSocket_t socket, u64 &userId, u64 &deviceId, u32 &instanceId);
};
} // end namespace Link
} // end namespace Ts2

#endif // incl guard
