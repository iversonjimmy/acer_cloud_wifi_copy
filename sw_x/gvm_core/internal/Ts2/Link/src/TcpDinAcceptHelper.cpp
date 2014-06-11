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

#include "TcpDinAcceptHelper.hpp"
#include "ObjectTracker.hpp"

#include <gvm_errors.h>
#include <gvm_thread_utils.h>
#include <log.h>

#include <vpl_string.h>
#include <vplu_format.h>
#include <vplu_mutex_autolock.hpp>

static const VPLTime_t condWaitTimeout_pxdLogin = VPLTime_FromSec(10);

const size_t Ts2::Link::TcpDinAcceptHelper::ThreadStackSize = UTIL_DEFAULT_THREAD_STACK_SIZE;

static Ts2::Link::ObjectTracker<Ts2::Link::TcpDinAcceptHelper> tcpDinAcceptHelperTracker;

Ts2::Link::TcpDinAcceptHelper::TcpDinAcceptHelper(AcceptedSocketHandler acceptedSocketHandler, void *acceptedSocketHandler_context,
                                                  VPLSocket_t socket,
                                                  LocalInfo *localInfo)
    : AcceptHelper(acceptedSocketHandler, acceptedSocketHandler_context, localInfo),
      socket(socket)
#ifndef TS2_NO_PXD
      , pxdLogin_result(-1 /*init a value that does not belong to pxd*/)
#endif
{
#ifndef TS2_NO_PXD
    VPLCond_Init(&cond);
#endif
}

Ts2::Link::TcpDinAcceptHelper::~TcpDinAcceptHelper()
{
#ifndef TS2_NO_PXD
    VPLCond_Destroy(&cond);
#endif
}

int Ts2::Link::TcpDinAcceptHelper::Accept()
{
    int err = 0;

    if (VPLSocket_Equal(socket, VPLSOCKET_INVALID)) {
        LOG_ERROR("TcpDinAcceptHelper[%p]: Invalid socket", this);
        return TS_ERR_INVALID;
        goto end;
    }

    {
        MutexAutoLock lock(&mutex);

        if (state.thread != THREAD_STATE_NOTHREAD) {
            LOG_ERROR("TcpDinAcceptHelper[%p]: Wrong thread state %d", this, state.thread);
            err = TS_ERR_WRONG_STATE;
            goto end;
        }
        state.thread = THREAD_STATE_SPAWNING;

        err = Util_SpawnThread(threadMain, this, ThreadStackSize, /*isJoinable*/VPL_TRUE, &thread);
        if (err) {
            LOG_ERROR("TcpDinAcceptHelper[%p]: Failed to spawn thread: err %d", this, err);
            state.thread = THREAD_STATE_NOTHREAD;
        }
        goto end;
    }

 end:
    return err;
}

int Ts2::Link::TcpDinAcceptHelper::WaitDone()
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
            LOG_ERROR("TcpDinAcceptHelper[%p]: Failed to Join thread: err %d", this, err);
        }
        else {
            state.thread = THREAD_STATE_NOTHREAD;
        }
    }
    VPLMutex_Unlock(&mutex);

    return err;
}

void Ts2::Link::TcpDinAcceptHelper::ForceClose()
{
    MutexAutoLock lock(&mutex);
    if (state.stop != STOP_STATE_NOSTOP) {
        LOG_WARN("TcpDinAcceptHelper[%p]: Wrong stop state %d", this, state.stop);

    } else {
        state.stop = STOP_STATE_STOPPING;
        if (!VPLSocket_Equal(socket, VPLSOCKET_INVALID)) {
            LOG_INFO("TcpDinAcceptHelper[%p]: Force to close socket", this);
            VPLSocket_Shutdown(socket, VPLSOCKET_SHUT_RDWR);
            VPLSocket_Close(socket);
            socket = VPLSOCKET_INVALID;
        }
    }
    VPLCond_Signal(&cond);
}

VPLTHREAD_FN_DECL Ts2::Link::TcpDinAcceptHelper::threadMain(void *param)
{
    TcpDinAcceptHelper *helper = static_cast<TcpDinAcceptHelper*>(param);
    helper->threadMain();
    return VPLTHREAD_RETURN_VALUE;
}

void Ts2::Link::TcpDinAcceptHelper::threadMain()
{
    u64 remoteUserId = 0;
    u64 remoteDeviceId = 0;
    u32 remoteInstanceId = 0;
    std::string *duplicateSessionKey = NULL;
    int err = 0;

#ifdef TS2_NO_PXD
    err = recvClientId(socket, remoteUserId, remoteDeviceId, remoteInstanceId);
    // ErrMsg logged by recvClientId().
#else
    {
        std::string ccdServerKey;

        err = localInfo->GetCcdServerKey(ccdServerKey);
        if (err) {
            LOG_ERROR("TcpDinAcceptHelper[%p]: Failed to get CcdServerKey: err %d", this, err);
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

        pxd_login_t login;
        memset(&login, 0, sizeof(login));
        login.socket = socket;
        login.opaque = reinterpret_cast<void*>(tcpDinAcceptHelperTracker.Add(this));
        login.callback = pxdLoginDone;
        login.is_incoming = 1;
        login.server_id = &id;
        login.server_key = const_cast<char*>(ccdServerKey.data());
        login.server_key_length = ccdServerKey.size();

        {
            MutexAutoLock lock(&mutex);
            pxd_error_t pxd_err;

            if (state.stop == STOP_STATE_STOPPING) {
                LOG_INFO("TcpDinAcceptHelper[%p]: Force closing..", this);
                err = VPL_ERR_NOT_RUNNING; // err out
                goto end;
            }

            LOG_INFO("TcpDinAcceptHelper[%p]: Calling pxd_login", this);
            pxd_login(&login, &pxd_err);
            if (pxd_err.error) {
                LOG_ERROR("TcpDinAcceptHelper[%p]: pxd_login failed: err %s", this, pxd_err.message);
                err = pxd_err.error;
                tcpDinAcceptHelperTracker.Remove(reinterpret_cast<u32>(login.opaque));
                goto end;
            }

            while (pxdLogin_result == -1 &&
                   state.stop != STOP_STATE_STOPPING) {
                err = VPLCond_TimedWait(&cond, &mutex, condWaitTimeout_pxdLogin);
                if (err) {
                    if (err == VPL_ERR_TIMEOUT) {
                        LOG_WARN("TcpDinAcceptHelper[%p]: Timed out waiting for pxd_login to succeed", this);
                    }
                    else {
                        LOG_WARN("TcpDinAcceptHelper[%p]: Failed waiting for pxd_login to succeed: err %d", this, err);
                    }
                    break;
                }
            }
            tcpDinAcceptHelperTracker.Remove(reinterpret_cast<u32>(login.opaque));
            if (err) {
                goto end;
            }

            if (state.stop == STOP_STATE_STOPPING) {
                LOG_INFO("TcpDinAcceptHelper[%p]: Force closing..", this);
                err = VPL_ERR_NOT_RUNNING; // err out
                goto end;
            }
        }

        if (pxdLogin_result != pxd_op_successful) {
            err = TS_ERR_NO_AUTH;
            goto end;
        }

        remoteUserId = pxd_remoteUserId;
        remoteDeviceId = pxd_remoteDeviceId;
        remoteInstanceId = pxd_remoteInstanceId;

        if (!localInfo->IsAuthorizedClient(remoteDeviceId)) {
            LOG_ERROR("TcpDinAcceptHelper[%p]: Device "FMTu64" is not authorized to connect.", this, remoteDeviceId);
            err = TS_ERR_NO_AUTH;
            goto end;
        }
    }

    // Duplicate session key
    duplicateSessionKey =  new (std::nothrow) std::string(sessionKey);
    if (duplicateSessionKey == NULL) {
        LOG_ERROR("TcpDinAcceptHelper[%p]: Copy sessionKey failed, out of memory.", this);
        err = TS_ERR_NO_MEM;
    }
 end:
#endif

    if (!err) {
        (*acceptedSocketHandler)(socket, ROUTE_TYPE_DIN, 
                                 remoteUserId, remoteDeviceId, remoteInstanceId,
                                 duplicateSessionKey, this, acceptedSocketHandler_context);
        // Ownership of socket transferred to acceptedSocketHandler.
        {
            MutexAutoLock lock(&mutex);
            socket = VPLSOCKET_INVALID;
        }
    }

    // Notify no more sockets from this helper.
    (*acceptedSocketHandler)(VPLSOCKET_INVALID, ROUTE_TYPE_INVALID, 
                             remoteUserId, remoteDeviceId, remoteInstanceId,
                             NULL, this, acceptedSocketHandler_context);

    {
        MutexAutoLock lock(&mutex);
        state.stop = STOP_STATE_STOPPING;
    }
}

#ifndef TS2_NO_PXD
// class method
void Ts2::Link::TcpDinAcceptHelper::pxdLoginDone(pxd_cb_data_t *data)
{
    TcpDinAcceptHelper *helper = tcpDinAcceptHelperTracker.Lookup(reinterpret_cast<u32>(data->op_opaque));
    if (!helper) {
        LOG_WARN("Ignoring late callback");
        return;
    }
    helper->setRemoteUdi(data);
}

void Ts2::Link::TcpDinAcceptHelper::setRemoteUdi(pxd_cb_data_t *data)
{
    LOG_INFO("TcpDinAcceptHelper[%p]: pxd_login result "FMTu64, this, data->result);
    MutexAutoLock lock(&mutex);
    pxdLogin_result = data->result;
    if (data->result != pxd_op_successful) {
        LOG_ERROR("TcpDinAcceptHelper[%p]: Authentication failed: result "FMTu64, this, data->result);
        if (data->result == pxd_credentials_rejected) {
            // We are not sure it is because of client credentials are bad or server key is bad.
            // We destroy server key on server side also.
            localInfo->ResetCcdServerKey();
        }
        goto end;
    }

    LOG_INFO("TcpDinAcceptHelper[%p]: remote UDI is <"FMTu64","FMTu64",%s>", 
             this, data->blob->client_user, data->blob->client_device, data->blob->client_instance);

    pxd_remoteUserId = data->blob->client_user;
    pxd_remoteDeviceId = data->blob->client_device;
    {
        int nmatch = sscanf(data->blob->client_instance, FMTu32, &pxd_remoteInstanceId);
        if (nmatch != 1) {
            LOG_ERROR("TcpDinAcceptHelper[%p]: Failed to parse instanceId string.", this);
            goto end;
        }
    }

    sessionKey.assign(data->blob->key, data->blob->key_length);
 end:
    VPLCond_Signal(&cond);
}
#endif

int Ts2::Link::TcpDinAcceptHelper::recvClientId(VPLSocket_t socket, 
                                                u64 &userId, u64 &deviceId, u32 &instanceId)
{
    int err = 0;

    // wait for the ID packet to arrive
    {
        VPLSocket_poll_t pollspec[1];
        pollspec[0].socket = socket;
        pollspec[0].events = VPLSOCKET_POLL_RDNORM;
        int numSockets = VPLSocket_Poll(pollspec, 1, VPL_TIMEOUT_NONE);
        if (numSockets < 0) {  // error
            LOG_ERROR("TcpDinAcceptHelper[%p]: Poll() failed: err %d", this, numSockets);
            goto end;
        }
        else if (numSockets != 1) {
            // Unexpected outcome - should not happen - treat as error.
            LOG_ERROR("TcpDinAcceptHelper[%p]: Poll() returned %d - unexpected", this, numSockets);
            goto end;
        }
        else if (pollspec[0].revents != VPLSOCKET_POLL_RDNORM) {
            // Something bad must have happened to the socket - bail out.
            LOG_WARN("EndPoint[%p]: socket failed: revents %d", this, pollspec[0].revents);
            goto end;
        }
    }

    {
        struct {
            u64 userId;
            u64 deviceId;
            u32 instanceId;
        } idPacket;

        int bytesRcvd = VPLSocket_Recv(socket, &idPacket, sizeof(idPacket));
        if (bytesRcvd < 0) {
            LOG_ERROR("TcpDinAcceptHelper[%p]: Failed to recv ID packet: err %d", this, bytesRcvd);
            err = bytesRcvd;
            goto end;
        }
        else if (bytesRcvd != sizeof(idPacket)) {
            LOG_ERROR("TcpDinAcceptHelper[%p]: Unexpected ID packet size %d", this, bytesRcvd);
            err = TS_ERR_COMM;
            goto end;
        }
        LOG_INFO("TcpDinAcceptHelper[%p]: Received ID packet", this);

        userId = VPLConv_ntoh_u64(idPacket.userId);
        deviceId = VPLConv_ntoh_u64(idPacket.deviceId);
        instanceId = VPLConv_ntoh_u32(idPacket.instanceId);
    }

 end:
    return err;
}
