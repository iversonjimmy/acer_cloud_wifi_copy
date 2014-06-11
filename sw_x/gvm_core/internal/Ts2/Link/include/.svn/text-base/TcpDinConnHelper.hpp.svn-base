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

#ifndef __TS2_LINK_TCP_DIN_CONN_HELPER_HPP__
#define __TS2_LINK_TCP_DIN_CONN_HELPER_HPP__

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

#include "ConnectHelper.hpp"

#include <map>
#include <string>

#ifdef IOS
#define TS2_LINK_TCP_DIN_CONN_HELPER_USE_CMD_SOCKET 1
#endif

namespace Ts2 {
namespace Link {
class TcpDinConnHelper : public ConnectHelper {
public:
    TcpDinConnHelper(u64 remoteUserId, u64 remoteDeviceId, u32 remoteInstanceId,
                     ConnectedSocketHandler connectedSocketHandler, void *connectedSocketHandler_context,
                     LocalInfo *localInfo);
    virtual ~TcpDinConnHelper();

    virtual int Connect();
    virtual int WaitDone();
    virtual void ForceClose();

private:
    VPL_DISABLE_COPY_AND_ASSIGN(TcpDinConnHelper);

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

    VPLSocket_t socket;
#ifdef TS2_LINK_TCP_DIN_CONN_HELPER_USE_CMD_SOCKET
    VPLSocket_t cmdSocket;
#endif // TS2_LINK_TCP_DIN_CONN_HELPER_USE_CMD_SOCKET
#ifndef TS2_NO_PXD
    mutable VPLCond_t cond;
    static void pxdLoginDone(pxd_cb_data_t *data);
    void setRemoteUdi(pxd_cb_data_t *data);
    int pxdLogin_result;
#endif

    int sendClientId(VPLSocket_t socket);
};
} // end namespace Link
} // end namespace Ts2

#endif // incl guard
