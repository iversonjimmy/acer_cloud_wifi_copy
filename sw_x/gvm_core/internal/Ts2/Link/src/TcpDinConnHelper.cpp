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

#include "TcpDinConnHelper.hpp"
#include "ObjectTracker.hpp"

#include <gvm_errors.h>
#include <gvm_thread_utils.h>
#include <log.h>
#ifndef TS2_NO_PXD
#include <pxd_client.h>
#endif

#include <vpl_string.h>
#include <vplu_format.h>
#include <vplu_mutex_autolock.hpp>
#ifdef TS2_LINK_TCP_DIN_CONN_HELPER_USE_CMD_SOCKET
#include <vplex_socket.h>
#endif // TS2_LINK_TCP_DIN_CONN_HELPER_USE_CMD_SOCKET

#include <cassert>

static const VPLTime_t sockConnTimeout_privateAddr = VPLTime_FromSec(4);
static const VPLTime_t sockConnTimeout_publicAddr = VPLTime_FromSec(5);

static const VPLTime_t condWaitTimeout_pxdLogin = VPLTime_FromSec(10);

const size_t Ts2::Link::TcpDinConnHelper::ThreadStackSize = UTIL_DEFAULT_THREAD_STACK_SIZE;

static Ts2::Link::ObjectTracker<Ts2::Link::TcpDinConnHelper> tcpDinConnHelperTracker;

Ts2::Link::TcpDinConnHelper::TcpDinConnHelper(u64 remoteUserId, u64 remoteDeviceId, u32 remoteInstanceId,
                                              ConnectedSocketHandler connectedSocketHandler, void *connectedSocketHandler_context,
                                              LocalInfo *localInfo)
    : ConnectHelper(remoteUserId, remoteDeviceId, remoteInstanceId, connectedSocketHandler, connectedSocketHandler_context, localInfo),
      socket(VPLSOCKET_INVALID)
#ifdef TS2_LINK_TCP_DIN_CONN_HELPER_USE_CMD_SOCKET
    , cmdSocket(VPLSOCKET_INVALID)
#endif // TS2_LINK_TCP_DIN_CONN_HELPER_USE_CMD_SOCKET
#ifndef TS2_NO_PXD
      , pxdLogin_result(-1 /*init a value that does not belong to pxd*/)
#endif
{
#ifndef TS2_NO_PXD
    VPLCond_Init(&cond);
#endif
    LOG_INFO("TcpDinConnHelper[%p]: Created", this);
}

Ts2::Link::TcpDinConnHelper::~TcpDinConnHelper()
{
#ifndef TS2_NO_PXD
    VPLCond_Destroy(&cond);
#endif
    LOG_INFO("TcpDinConnHelper[%p]: Destroyed", this);
}

int Ts2::Link::TcpDinConnHelper::Connect()
{
    int err = 0;

    {
        MutexAutoLock lock(&mutex);
        // state check and transition
        if (state.thread != THREAD_STATE_NOTHREAD) {
            LOG_ERROR("TcpDinConnHelper[%p]: Wrong thread state %d", this, state.thread);
            err = TS_ERR_WRONG_STATE;
            goto end;
        }
        state.thread = THREAD_STATE_SPAWNING;

        err = Util_SpawnThread(threadMain, this, ThreadStackSize, /*isJoinable*/VPL_TRUE, &thread);
        if (err) {
            LOG_ERROR("TcpDinConnHelper[%p]: Failed to spawn thread: err %d", this, err);
            state.thread = THREAD_STATE_NOTHREAD;
            goto end;
        }
    }

 end:
    return err;
}

int Ts2::Link::TcpDinConnHelper::WaitDone()
{
    int err = 0;

    // Join the thread.
    // Make sure not to hold the mutex while trying to Join.
    VPLMutex_Lock(&mutex);
    if (state.thread != THREAD_STATE_NOTHREAD) {
        VPLMutex_Unlock(&mutex);
        err = VPLDetachableThread_Join(&thread);
        VPLMutex_Lock(&mutex);
        if (err) {
            LOG_ERROR("TcpDinConnHelper[%p]: Failed to Join thread: err %d", this, err);
        }
        else {
            LOG_INFO("TcpDinConnHelper[%p]: Joined thread", this);
            state.thread = THREAD_STATE_NOTHREAD;
        }
        VPLMutex_Unlock(&mutex);
    }
    VPLMutex_Unlock(&mutex);

    return err;
}

void Ts2::Link::TcpDinConnHelper::ForceClose()
{
    MutexAutoLock lock(&mutex);
    if (state.stop != STOP_STATE_NOSTOP) {
        LOG_WARN("TcpDinConnHelper[%p]: Wrong stop state %d", this, state.stop);

    } else {
        state.stop = STOP_STATE_STOPPING;

#ifdef TS2_LINK_TCP_DIN_CONN_HELPER_USE_CMD_SOCKET
        if (!VPLSocket_Equal(cmdSocket, VPLSOCKET_INVALID)) {
            LOG_INFO("TcpDinConnHelper[%p]: Force to shutdown", this);
            int bytesWritten = VPLSocket_Send(cmdSocket, "Q", 1);
            if (bytesWritten < 0) {
                LOG_ERROR("TcpDinConnHelper[%p]: Failed to write into pipe: err %d", this, (int)bytesWritten);
            }
            else if (bytesWritten != 1) {
                LOG_ERROR("TcpDinConnHelper[%p]: Wrong number of bytes written into pipe: nbytes %d", this, (int)bytesWritten);
            }
        }
#endif // TS2_LINK_TCP_DIN_CONN_HELPER_USE_CMD_SOCKET

        // For pxd_login case
        if (!VPLSocket_Equal(socket, VPLSOCKET_INVALID)) {
            LOG_INFO("TcpDinConnHelper[%p]: Force to close socket", this);
            VPLSocket_Shutdown(socket, VPLSOCKET_SHUT_RDWR);
            VPLSocket_Close(socket);
            socket = VPLSOCKET_INVALID;
        }
    }
    VPLCond_Signal(&cond);
}

VPLTHREAD_FN_DECL Ts2::Link::TcpDinConnHelper::threadMain(void *param)
{
    TcpDinConnHelper *helper = static_cast<TcpDinConnHelper*>(param);
    helper->threadMain();
    return VPLTHREAD_RETURN_VALUE;
}

void Ts2::Link::TcpDinConnHelper::threadMain()
{
    int err = 0;
    VPLNet_addr_t addr = 0;
    VPLNet_port_t port = 0;
    std::string *duplicateSessionKey = NULL;
    std::string ccdSessionKey;
#ifdef TS2_LINK_TCP_DIN_CONN_HELPER_USE_CMD_SOCKET
    VPLSocket_t cmdSocketListener = VPLSOCKET_INVALID, cmdSocketServer = VPLSOCKET_INVALID, cmdSocketClient = VPLSOCKET_INVALID;
    VPLSocket_poll_t pollSockets[2];

    {
        VPLNet_addr_t addr = VPLNET_ADDR_LOOPBACK;  // NOTE: on iOS, VPLSocket_Create will treat this the same as VPLNET_ADDR_ANY.
        VPLNet_port_t port = VPLNET_PORT_ANY;
        cmdSocketListener = VPLSocket_CreateTcp(addr, port);
        if (VPLSocket_Equal(cmdSocketListener, VPLSOCKET_INVALID)) {
            LOG_ERROR("TcpDinConnHelper[%p]: Failed to create listen socket", this);
            goto end;
        }
    }

    err = VPLSocket_Listen(cmdSocketListener, 1);
    if (err) {
        LOG_ERROR("TcpDinConnHelper[%p]: VPLSocket_Listen() failed on socket["FMT_VPLSocket_t"]: err %d", this, VAL_VPLSocket_t(cmdSocketListener), err);
        goto end;
    }

    cmdSocketClient = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_STREAM, /*nonblocking*/VPL_TRUE);
    if (VPLSocket_Equal(cmdSocketClient, VPLSOCKET_INVALID)) {
        LOG_ERROR("TcpDinConnHelper[%p]: Failed to create client-end socket", this);
        goto end;
    }

    {
        VPLSocket_addr_t sin;
        sin.family = VPL_PF_INET;
        sin.addr = VPLNET_ADDR_LOOPBACK;
        sin.port = VPLSocket_GetPort(cmdSocketListener);
        err = VPLSocket_Connect(cmdSocketClient, &sin, sizeof(sin));
        if (err) {
            LOG_ERROR("TcpDinConnHelper[%p]: Failed to connect client socket to server: err %d", this, err);
            goto end;
        }
    }

    {
        VPLSocket_addr_t addr;
        int err = VPLSocket_Accept(cmdSocketListener, &addr, sizeof(addr), &cmdSocketServer);
        if (err) {
            LOG_ERROR("TcpDinConnHelper[%p]: VPLSocket_Accept() failed: err %d", this, err);
            goto end;
        }
    }
#endif // TS2_LINK_TCP_DIN_CONN_HELPER_USE_CMD_SOCKET

    {
        MutexAutoLock lock(&mutex);
        if (state.stop == STOP_STATE_STOPPING) {
            LOG_INFO("TcpDinConnHelper[%p]: Force closing..", this);
            goto end;
        }
        socket = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_STREAM, /*nonblock*/VPL_TRUE);
#ifdef TS2_LINK_TCP_DIN_CONN_HELPER_USE_CMD_SOCKET
        cmdSocket = cmdSocketClient;  // expose command socket
#endif // TS2_LINK_TCP_DIN_CONN_HELPER_USE_CMD_SOCKET
    }

    if (VPLSocket_Equal(socket, VPLSOCKET_INVALID)) {
        LOG_ERROR("TcpDinConnHelper[%p]: Failed to create socket: err %d", this, err);
        goto end;
    }

    {
        int yes = 1;
        err = VPLSocket_SetSockOpt(socket, VPLSOCKET_IPPROTO_TCP, VPLSOCKET_TCP_NODELAY, &yes, sizeof(int));
        if (err) {
            LOG_ERROR("TcpDinConnHelper[%p]: Failed to set socket opt: err %d", this, err);
            goto end;
        }
    }

    err = localInfo->GetServerTcpDinAddrPort(remoteUserId, remoteDeviceId, remoteInstanceId,
                                             addr, port);
    if (err) {
        LOG_ERROR("TcpDinConnHelper[%p]: Failed to get addr:port for UDI<"FMTu64","FMTu64","FMTu32">: err %d", 
                  this, remoteUserId, remoteDeviceId, remoteInstanceId, err);
        goto end;
    }
    LOG_INFO("TcpDinConnHelper[%p]: Connecting to "FMT_VPLNet_addr_t":"FMT_VPLNet_port_t" for UDI<"FMTu64","FMTu64","FMTu32">", 
             this,
             VAL_VPLNet_addr_t(addr), port, remoteUserId, remoteDeviceId, remoteInstanceId);

    {
        VPLSocket_addr_t sin;
        sin.family = VPL_PF_INET;
        sin.addr = addr;
        sin.port = VPLConv_hton_u16(port);
#ifdef TS2_LINK_TCP_DIN_CONN_HELPER_USE_CMD_SOCKET
        err = VPLSocket_ConnectNowait(socket, &sin, sizeof(sin));
        if(err == VPL_OK) {
            // we got connected
        } else if(err != VPL_ERR_BUSY && err != VPL_ERR_AGAIN) {
            LOG_ERROR("TcpDinConnHelper[%p]: Failed to connect: %d", this, err);
            goto end;
        } else {
            pollSockets[0].socket = cmdSocketServer;
            pollSockets[0].events = VPLSOCKET_POLL_RDNORM;
            pollSockets[1].socket = socket;
            pollSockets[1].events = VPLSOCKET_POLL_OUT;

            err = VPLSocket_Poll(pollSockets, 2, sockConnTimeout_publicAddr);

            if (err < 0) {  // poll error
                LOG_ERROR("TcpDinConnHelper[%p]: Poll() failed: err %d", this, err);
                goto end;
            }
            if (err == 0) {  // timeout
                LOG_ERROR("TcpDinConnHelper[%p]: Socket not ready for send after "FMT_VPLTime_t" secs", this, sockConnTimeout_publicAddr);
                err = VPL_ERR_TIMEOUT;
                goto end;
            }

            if (pollSockets[0].revents) {
                // FIXME: On iOS, because VPLNET_ADDR_LOOPBACK is treated the same as VPLNET_ADDR_ANY,
                //        we need to guard against an external attack.
                // TODO: Check internal state to make sure we really do want to exit.
                LOG_INFO("TcpDinConnHelper[%p]: cmd socket got events, treat as an abort signal, revents %d", this, pollSockets[0].revents);
                err = VPL_ERR_FAIL;
                goto end;
            }

            if (pollSockets[1].revents &&
                pollSockets[1].revents != VPLSOCKET_POLL_OUT) {  // bad socket - handle as error
                LOG_ERROR("TcpDinConnHelper[%p]: socket failed: revents %d", this, pollSockets[1].revents);
                err = VPL_ERR_IO;
                goto end;
            }
        }
#else
        err = VPLSocket_ConnectWithTimeouts(socket, &sin, sizeof(sin), sockConnTimeout_privateAddr, sockConnTimeout_publicAddr);
        if (err) {
            LOG_ERROR("TcpDinConnHelper[%p]: Failed to connect: %d", this, err);
            goto end;
        }
#endif // TS2_LINK_TCP_DIN_CONN_HELPER_USE_CMD_SOCKET
    }

    LOG_INFO("TcpDinConnHelper[%p]: Connected to UDI<"FMTu64","FMTu64","FMTu32">",
             this, remoteUserId, remoteDeviceId, remoteInstanceId);


#ifndef TS2_NO_PXD
    {
        std::string ccdLoginBlob;

        assert(localInfo);
        err = localInfo->GetCcdSessionKey(remoteUserId, remoteDeviceId, remoteInstanceId,
                                          ccdSessionKey, ccdLoginBlob);
        if (err) {
            LOG_ERROR("TcpDinConnHelper[%p]: Failed to get CcdSessionKey: %d", this, err);
            goto end;
        }

        pxd_id_t id;
        memset(&id, 0, sizeof(id));
        char region[] = "";
        char instanceId[16];  // large enough for u32 in decimal
        VPL_snprintf(instanceId, ARRAY_SIZE_IN_BYTES(instanceId), FMTu32, localInfo->GetInstanceId());
        id.region = region;
        id.user_id = localInfo->GetUserId();
        id.device_id = localInfo->GetDeviceId();
        id.instance_id = instanceId;

        pxd_cred_t cred;
        memset(&cred, 0, sizeof(cred));
        cred.id = &id;
        cred.opaque = const_cast<char*>(ccdLoginBlob.data());
        cred.opaque_length = ccdLoginBlob.size();
        cred.key = (void*)ccdSessionKey.data();
        cred.key_length  = static_cast<int>(ccdSessionKey.size());

        pxd_login_t login;
        memset(&login, 0, sizeof(login));
        login.socket = socket;
        login.opaque = reinterpret_cast<void*>(tcpDinConnHelperTracker.Add(this));
        login.callback = pxdLoginDone;
        login.credentials = &cred;
        login.is_incoming = 0;

        {
            MutexAutoLock lock(&mutex);
            pxd_error_t pxd_err;

            if (state.stop == STOP_STATE_STOPPING) {
                LOG_INFO("TcpDinConnHelper[%p]: Force closing..", this);
                err = VPL_ERR_NOT_RUNNING; // err out
                goto end;
            }

            LOG_INFO("TcpDinConnHelper[%p]: Calling pxd_login", this);
            pxd_login(&login, &pxd_err);
            if (pxd_err.error) {
                LOG_ERROR("TcpDinConnHelper[%p]: pxd_login failed: err %s", this, pxd_err.message);
                err = pxd_err.error;
                tcpDinConnHelperTracker.Remove(reinterpret_cast<u32>(login.opaque));
                goto end;
            }

            while (pxdLogin_result == -1 &&
                   state.stop != STOP_STATE_STOPPING) {
                err = VPLCond_TimedWait(&cond, &mutex, condWaitTimeout_pxdLogin);
                if (err) {
                    if (err == VPL_ERR_TIMEOUT) {
                        LOG_WARN("TcpDinConnHelper[%p]: Timed out waiting for pxd_login to succeed", this);
                    }
                    else {
                        LOG_WARN("TcpDinConnHelper[%p]: Failed waiting for pxd_login to succeed: err %d", this, err);
                    }
                    break;
                }
            }
            tcpDinConnHelperTracker.Remove(reinterpret_cast<u32>(login.opaque));
            if (err) {
                goto end;
            }

            if (state.stop == STOP_STATE_STOPPING) {
                LOG_INFO("TcpDinConnHelper[%p]: Force closing..", this);
                err = VPL_ERR_NOT_RUNNING; // err out
                goto end;
            }
        }

        if (pxdLogin_result != pxd_op_successful) {
            err = TS_ERR_NO_AUTH;
            goto end;
        }
        LOG_INFO("TcpDinConnHelper[%p]: Connection to UDI<"FMTu64","FMTu64","FMTu32"> authenticated", 
                 this, remoteUserId, remoteDeviceId, remoteInstanceId);
    }
#else
    sendClientId(socket);
#endif

    // Duplicate session key
    duplicateSessionKey =  new (std::nothrow) std::string(ccdSessionKey);
    if (duplicateSessionKey == NULL) {
        LOG_ERROR("TcpDinConnHelper[%p]: Copy sessionKey failed, out of memory.", this);
        err = TS_ERR_NO_MEM;
    }

 end:
    if (err) {
        {
            MutexAutoLock lock(&mutex);
            if (!VPLSocket_Equal(socket, VPLSOCKET_INVALID)) {
                VPLSocket_Close(socket);
                socket = VPLSOCKET_INVALID;
            }
        }
    }

    if (!VPLSocket_Equal(socket, VPLSOCKET_INVALID)) {
        (*connectedSocketHandler)(socket, ROUTE_TYPE_DIN, duplicateSessionKey, this, connectedSocketHandler_context);
        // Ownership of socket transferred to connectedSocketHandler.
        {
            MutexAutoLock lock(&mutex);
            socket = VPLSOCKET_INVALID;
        }
    }

    // Notify no more sockets from this helper.
    (*connectedSocketHandler)(VPLSOCKET_INVALID, ROUTE_TYPE_INVALID, NULL, this, connectedSocketHandler_context);

    {
        MutexAutoLock lock(&mutex);
        state.stop = STOP_STATE_STOPPING;
#ifdef TS2_LINK_TCP_DIN_CONN_HELPER_USE_CMD_SOCKET
        cmdSocket = VPLSOCKET_INVALID;  // hide command socket
#endif // TS2_LINK_TCP_DIN_CONN_HELPER_USE_CMD_SOCKET
    }

#ifdef TS2_LINK_TCP_DIN_CONN_HELPER_USE_CMD_SOCKET
    if(!VPLSocket_Equal(VPLSOCKET_INVALID, cmdSocketClient)) {
        VPLSocket_Close(cmdSocketClient);
    }
    if(!VPLSocket_Equal(VPLSOCKET_INVALID, cmdSocketServer)) {
        VPLSocket_Close(cmdSocketServer);
    }
    if(!VPLSocket_Equal(VPLSOCKET_INVALID, cmdSocketListener)) {
        VPLSocket_Close(cmdSocketListener);
    }
#endif // TS2_LINK_TCP_DIN_CONN_HELPER_USE_CMD_SOCKET

    LOG_INFO("TcpDinConnHelper[%p]: Thread exiting", this);
}

#ifndef TS2_NO_PXD
// class method
void Ts2::Link::TcpDinConnHelper::pxdLoginDone(pxd_cb_data_t *data)
{
    TcpDinConnHelper *helper = tcpDinConnHelperTracker.Lookup(reinterpret_cast<u32>(data->op_opaque));
    if (!helper) {
        LOG_WARN("Ignoring late callback");
        return;
    }
    helper->setRemoteUdi(data);
}

void Ts2::Link::TcpDinConnHelper::setRemoteUdi(pxd_cb_data_t *data)
{
    LOG_INFO("TcpDinConnHelper[%p]: pxd_login result "FMTu64, this, data->result);
    MutexAutoLock lock(&mutex);
    pxdLogin_result = data->result;
    if (data->result != pxd_op_successful) {
        LOG_ERROR("TcpDinConnHelper[%p]: Authentication failed: result "FMTu64, this, data->result);
        if (data->result == pxd_op_failed) {
            // We are not sure it is because client credentials are bad or server key is bad.
            // We destroy ccd credentials on client side also.
            localInfo->ResetCcdSessionKey(remoteUserId, remoteDeviceId, remoteInstanceId);
        }
        goto end;
    }

 end:
    VPLCond_Signal(&cond);
}
#endif

int Ts2::Link::TcpDinConnHelper::sendClientId(VPLSocket_t socket)
{
    struct {
        u64 userId;
        u64 deviceId;
        u32 instanceId;
    } idPacket;

    idPacket.userId = VPLConv_hton_u64(localInfo->GetUserId());
    idPacket.deviceId = VPLConv_hton_u64(localInfo->GetDeviceId());
    idPacket.instanceId = VPLConv_hton_u32(localInfo->GetInstanceId());

    int bytesSent = VPLSocket_Send(socket, &idPacket, sizeof(idPacket));
    if (bytesSent < 0) {
        LOG_ERROR("TcpDinConnHelper[%p]: Failed to send ID packet: err %d", this, bytesSent);
        return bytesSent;
    }
    else if (bytesSent != sizeof(idPacket)) {
        LOG_ERROR("TcpDinConnHelper[%p]: Unexpected ID packet size %d", this, bytesSent);
        return TS_ERR_COMM;
    }
    LOG_INFO("TcpDinConnHelper[%p]: Sent ID packet", this);
    return 0;
}
