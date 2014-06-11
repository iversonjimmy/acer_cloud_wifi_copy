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

#include "PrxP2PAcceptHelper.hpp"
#include "ObjectTracker.hpp"

#include <gvm_errors.h>
#include <gvm_thread_utils.h>
#include <log.h>

#include <vpl_string.h>
#include <vplu_format.h>
#include <vplu_mutex_autolock.hpp>
#include "PrxP2PRoutines.hpp"

static const VPLTime_t condWaitTimeout_pxdIncomingRequestCb = VPLTime_FromSec(10);
static const VPLTime_t condWaitTimeout_pxdIncomingLoginCb = VPLTime_FromSec(30);
static const VPLTime_t condWaitTimeout_p2pDoneCb = VPLTime_FromSec(30);
static const VPLTime_t condWaitTimeout_pxdLoginCb = VPLTime_FromSec(10);

const size_t Ts2::Link::PrxP2PAcceptHelper::ThreadStackSize = UTIL_DEFAULT_THREAD_STACK_SIZE;

namespace Ts2 {
namespace Link {
// Callback context for Pxd Client Libaray in AcceptHelper
class PclAcceptCtxt {
public:
    PclAcceptCtxt(PrxP2PAcceptHelper* h) : helper(h), result(TS_ERR_TIMEOUT), resultIncomingRequest(TS_ERR_TIMEOUT), socket(VPLSOCKET_INVALID) {
    }
    ~PclAcceptCtxt() {
    }
    PrxP2PAcceptHelper* helper;
    int result;
    int resultIncomingRequest;
    VPLSocket_t socket;
};
} // end namespace Link
} // end namespace Ts2

static Ts2::Link::ObjectTracker<Ts2::Link::PclAcceptCtxt> pclAcceptCtxtTracker;

Ts2::Link::PrxP2PAcceptHelper::PrxP2PAcceptHelper(AcceptedSocketHandler acceptedSocketHandler, void *acceptedSocketHandler_context,
    pxd_client_t* pxdClient, const char* _buffer, u32 bufferLength,
    LocalInfo *localInfo) :
        AcceptHelper(acceptedSocketHandler, acceptedSocketHandler_context, localInfo),
        connState(IN_PRX),
        prxSocket(VPLSOCKET_INVALID),
        p2pSocket(VPLSOCKET_INVALID),
        pxdClient(pxdClient),
        buffer(NULL),
        bufferLength(bufferLength),
        remoteUserId(0),
        remoteDeviceId(0),
        remoteInstanceId(0),
        p2pHandle(0)
{
    remoteExtAddr.ip_address = NULL;
    remoteExtAddr.type = 0;
    remoteExtAddr.ip_length = 0;
    remoteExtAddr.port = 0;

    buffer = (char*)malloc(bufferLength);
    if (buffer) {
        memcpy(buffer, _buffer, bufferLength);
    } else {
        LOG_ERROR("Out of memory!");
    }

    VPLCond_Init(&cond);
}

Ts2::Link::PrxP2PAcceptHelper::~PrxP2PAcceptHelper()
{
    if (remoteExtAddr.ip_address != NULL) {
        delete[] remoteExtAddr.ip_address;
        remoteExtAddr.ip_address = NULL;
    }
    if (buffer != NULL) {
        free(buffer);
        buffer = NULL;
    }

    VPLCond_Destroy(&cond);
}

int Ts2::Link::PrxP2PAcceptHelper::Accept()
{
    int err = 0;

    {
        MutexAutoLock lock(&mutex);
        // state check and transition
        if (state.thread != THREAD_STATE_NOTHREAD) {
            LOG_ERROR("PrxP2PAcceptHelper[%p]: Wrong thread state %d", this, state.thread);
            err = TS_ERR_WRONG_STATE;
            goto end;
        }
        state.thread = THREAD_STATE_SPAWNING;
    }

    err = Util_SpawnThread(threadMain, this, ThreadStackSize, /*isJoinable*/VPL_TRUE, &prxThread);
    if (err) {
        LOG_ERROR("PrxP2PAcceptHelper[%p]: Failed to spawn thread: err %d", this, err);
        {
            MutexAutoLock lock(&mutex);
            state.thread = THREAD_STATE_NOTHREAD;
        }
        goto end;
    }

 end:
    return err;
}

int Ts2::Link::PrxP2PAcceptHelper::WaitDone()
{
    int err = 0;

    bool threadPresent = false;
    {
        MutexAutoLock lock(&mutex);
        threadPresent = (state.thread != THREAD_STATE_NOTHREAD);
    }

    if (threadPresent) {
        err = VPLDetachableThread_Join(&prxThread);
        if (err) {
            LOG_ERROR("PrxP2PAcceptHelper[%p]: Failed to Join thread: err %d", this, err);
            goto end;
        }
        if (connState == IN_P2P) {
            err = VPLDetachableThread_Join(&p2pThread);
            if (err) {
                LOG_ERROR("PrxP2PAcceptHelper[%p]: Failed to Join thread: err %d", this, err);
                goto end;
            }
        }
    }
    {
        MutexAutoLock lock(&mutex);
        state.thread = THREAD_STATE_NOTHREAD;
    }

 end:
    return err;
}

void Ts2::Link::PrxP2PAcceptHelper::ForceClose()
{
    MutexAutoLock lock(&mutex);
    if (state.stop != STOP_STATE_NOSTOP) {
        LOG_WARN("PrxP2PAcceptHelper[%p]: Wrong stop state %d", this, state.stop);

    } else {
        state.stop = STOP_STATE_STOPPING;
        // pxd lib should be stopped by PrxP2PListener already
        if (p2pHandle != 0) {
            // Stop p2p library
            LOG_INFO("PrxP2PAcceptHelper[%p]: Force to close p2p library", this);
            p2p_stop(p2pHandle);
            p2pHandle = 0;
        }
        if (!VPLSocket_Equal(p2pSocket, VPLSOCKET_INVALID)) {
            // Stop pxd_login
            LOG_INFO("PrxP2PAcceptHelper[%p]: Force to close socket", this);
            VPLSocket_Shutdown(p2pSocket, VPLSOCKET_SHUT_RDWR);
            VPLSocket_Close(p2pSocket);
            p2pSocket = VPLSOCKET_INVALID;
        }
    }
    VPLCond_Signal(&cond);
}

VPLTHREAD_FN_DECL Ts2::Link::PrxP2PAcceptHelper::threadMain(void *param)
{
    PrxP2PAcceptHelper *helper = static_cast<PrxP2PAcceptHelper*>(param);
    if (helper->connState == IN_PRX) {
        helper->prxThreadMain();
    } else if (helper->connState == IN_P2P) {
        helper->p2pThreadMain();
    }
    return VPLTHREAD_RETURN_VALUE;
}

#if 0
// test code to mangle key
static void mangleKey(std::string &key)
{
    char c = key[0];
    key[0] = key[1];
    key[1] = c;
}
#endif

void Ts2::Link::PrxP2PAcceptHelper::prxThreadMain()
{
    int err = 0;
    pxd_id_t localId;
    pxd_error_t pxdErr;
    pxd_receive_t pxdReceive;

    char region[] = ""; // Dummy
    char localInstanceIdStr[16];  // large enough for u32 in decimal
    VPL_snprintf(localInstanceIdStr, ARRAY_SIZE_IN_BYTES(localInstanceIdStr), FMTu32, localInfo->GetInstanceId());
    std::string ccdServerKey;
    std::string *duplicateSessionKey = NULL;

    PclAcceptCtxt receiveCtxt(this);

    // Prepare local id
    localId.device_id = localInfo->GetDeviceId();
    localId.user_id = localInfo->GetUserId();
    localId.instance_id = localInstanceIdStr;
    localId.region = region;

    err = localInfo->GetCcdServerKey(ccdServerKey);
    if (err != 0) {
        LOG_ERROR("PrxP2PAcceptHelper[%p]: GetCcdServerKey failed: err %d",
                  this, err);
        goto end;
    }

    // PXD receive
    //mangleKey(ccdServerKey);  // test case: bad ccd server key to pxd_receive()
    pxdReceive.client = pxdClient;
    pxdReceive.buffer = buffer;
    pxdReceive.buffer_length = bufferLength;
    pxdReceive.server_key = (char*)ccdServerKey.data();
    pxdReceive.server_key_length  = static_cast<int>(ccdServerKey.size());
    pxdReceive.opaque = reinterpret_cast<void*>(pclAcceptCtxtTracker.Add(&receiveCtxt));

    {
        MutexAutoLock lock(&mutex);

        if (state.stop == STOP_STATE_STOPPING) {
            LOG_INFO("PrxP2PAcceptHelper[%p]: Force closing..", this);
            err = VPL_ERR_NOT_RUNNING; // err out
            goto end;
        }

        LOG_INFO("PrxP2PAcceptHelper[%p]: Calling pxd_receive", this);
        pxd_receive(&pxdReceive, &pxdErr);

        if (pxdErr.error != 0) {
            LOG_ERROR("PrxP2PAcceptHelper[%p]: pxd_receive failed: err %d, Msg: %s",
                              this, pxdErr.error, pxdErr.message);
            err = pxdErr.error;
            pclAcceptCtxtTracker.Remove(reinterpret_cast<u32>(pxdReceive.opaque));
            goto end;
        }

        // Wait for incomingRequest
        // According to sepc, only valid msg has a callback delivered
        while (receiveCtxt.resultIncomingRequest == TS_ERR_TIMEOUT &&
                state.stop != STOP_STATE_STOPPING) {
            err = VPLCond_TimedWait(&cond, &mutex, condWaitTimeout_pxdIncomingRequestCb);
            if (err) {
                if (err == VPL_ERR_TIMEOUT) {
                    LOG_WARN("PrxP2PAcceptHelper[%p]: Timed out waiting for incoming_request", this);
                }
                else {
                    LOG_WARN("PrxP2PAcceptHelper[%p]: Failed waiting for incoming_request: err %d", this, err);
                }
                break;
            }
        }
        if(err) {
            pclAcceptCtxtTracker.Remove(reinterpret_cast<u32>(pxdReceive.opaque));
            goto end;
        }

        if (state.stop == STOP_STATE_STOPPING) {
            LOG_INFO("PrxP2PAcceptHelper[%p]: Force closing..", this);
            err = VPL_ERR_NOT_RUNNING; // err out
            pclAcceptCtxtTracker.Remove(reinterpret_cast<u32>(pxdReceive.opaque));
            goto end;
        }

        if (receiveCtxt.resultIncomingRequest != TS_OK) {

            LOG_ERROR("PrxP2PAcceptHelper[%p]: incomingRequest failed: err %d",
                              this, receiveCtxt.resultIncomingRequest);
            err = receiveCtxt.resultIncomingRequest;
            pclAcceptCtxtTracker.Remove(reinterpret_cast<u32>(pxdReceive.opaque));
            goto end;
        }

        // Wait for incomingLogin
        // According to sepc, only valid login has a callback delivered
        while (receiveCtxt.result == TS_ERR_TIMEOUT &&
               state.stop != STOP_STATE_STOPPING) {
            err = VPLCond_TimedWait(&cond, &mutex, condWaitTimeout_pxdIncomingLoginCb);
            if (err) {
                if (err == VPL_ERR_TIMEOUT) {
                    LOG_WARN("PrxP2PAcceptHelper[%p]: Timed out waiting for incoming_login", this);
                    // There is not much we could do, we don't know if client credentails are bad or server key is bad
                    // or there are some problems in connection. We could only drop the server key.
                    localInfo->ResetCcdServerKey();
                }
                else {
                    LOG_WARN("PrxP2PAcceptHelper[%p]: Failed waiting for incoming_login: err %d", this, err);
                }
                break;
            }
        }
        pclAcceptCtxtTracker.Remove(reinterpret_cast<u32>(pxdReceive.opaque));
        if(err) {
            goto end;
        }

        if (state.stop == STOP_STATE_STOPPING) {
            LOG_INFO("PrxP2PAcceptHelper[%p]: Force closing..", this);
            err = VPL_ERR_NOT_RUNNING; // err out
            goto end;
        }
    }

    if (receiveCtxt.result != TS_OK) {
        LOG_ERROR("PrxP2PAcceptHelper[%p]: incomingLogin failed: err %d",
                          this, receiveCtxt.result);
        err = receiveCtxt.result;
        goto end;
    }

    if (!localInfo->IsAuthorizedClient(remoteDeviceId)) {
        LOG_ERROR("PrxP2PAcceptHelper[%p]: Device "FMTu64" is not authorized to connect", this, remoteDeviceId);
        err = TS_ERR_NO_AUTH;
        goto end;
    }

    // Duplicate session key
    duplicateSessionKey =  new (std::nothrow) std::string(sessionKey);
    if (duplicateSessionKey == NULL) {
        LOG_ERROR("PrxP2PAcceptHelper[%p]: Copy sessionKey failed, out of memory.", this);
        err = TS_ERR_NO_MEM;
    }

 end:
    if (err) {
        if (!VPLSocket_Equal(receiveCtxt.socket, VPLSOCKET_INVALID)) {
            VPLSocket_Close(receiveCtxt.socket);
            receiveCtxt.socket = VPLSOCKET_INVALID;
        }
    }

    if (!VPLSocket_Equal(receiveCtxt.socket, VPLSOCKET_INVALID)) {
        // Fork P2P handler thread
        prxSocket = receiveCtxt.socket;
        connState = IN_P2P;
        err = Util_SpawnThread(threadMain, this, ThreadStackSize, /*isJoinable*/VPL_TRUE, &p2pThread);
        if (err) {
            LOG_ERROR("PrxP2PAcceptHelper[%p]: Failed to spawn thread: err %d", this, err);
        }
        (*acceptedSocketHandler)(receiveCtxt.socket, ROUTE_TYPE_PRX,
                                 remoteUserId, remoteDeviceId, remoteInstanceId,
                                 duplicateSessionKey, this, acceptedSocketHandler_context);
        // Ownership of socket transfers to connectedSocketHandler.
    }

    if (err) {
        // Notify no more sockets from this helper.
        (*acceptedSocketHandler)(VPLSOCKET_INVALID, ROUTE_TYPE_INVALID,
                                 remoteUserId, remoteDeviceId, remoteInstanceId,
                                 NULL, this, acceptedSocketHandler_context);
        {
            MutexAutoLock lock(&mutex);
            state.stop = STOP_STATE_STOPPING;
        }
    }
}

void Ts2::Link::PrxP2PAcceptHelper::p2pThreadMain()
{
    int err = 0;
    pxd_login_t pxdLogin;
    pxd_id_t localId;
    pxd_error_t pxdErr;
    char region[] = ""; // Dummy
    char localInstanceIdStr[16];  // large enough for u32 in decimal
    VPL_snprintf(localInstanceIdStr, ARRAY_SIZE_IN_BYTES(localInstanceIdStr), FMTu32, localInfo->GetInstanceId());

    std::string ccdServerKey;
    std::string *duplicateSessionKey = NULL;

    PclAcceptCtxt p2pCtxt(this);
    u32 p2pCtxt_index = 0;  // Note, 0 is an invalid ObjectTracker index.

    // Try P2P
    VPLSocket_addr_t remoteAddr;
    if (remoteExtAddr.ip_address == NULL) {
        LOG_ERROR("PrxP2PAcceptHelper[%p]: Address of remoteExtAddr is NULL", this);
        goto end;
    }
    PXD_ADDR_2_VPL_ADDR(remoteExtAddr, remoteAddr);
    p2pCtxt_index = pclAcceptCtxtTracker.Add(&p2pCtxt);

    {
        MutexAutoLock lock(&mutex);

        if (state.stop == STOP_STATE_STOPPING) {
            LOG_INFO("PrxP2PAcceptHelper[%p]: Force closing..", this);
            err = VPL_ERR_NOT_RUNNING; // err out
            goto end;
        }

        err = p2p_connect(p2pHandle, prxSocket, &remoteAddr, sizeof(remoteAddr), reinterpret_cast<void*>(p2pCtxt_index), p2pDoneCb);

        if (err != VPL_OK) {
            LOG_ERROR("PrxP2PAcceptHelper[%p]: p2p_connect failed: err %d", this, err);
            pclAcceptCtxtTracker.Remove(p2pCtxt_index);
            goto end;
        }

        // Wait for P2P done
        while (p2pCtxt.result == TS_ERR_TIMEOUT &&
               state.stop != STOP_STATE_STOPPING) {
            err = VPLCond_TimedWait(&cond, &mutex, condWaitTimeout_p2pDoneCb);
            if (err) {
                if (err == VPL_ERR_TIMEOUT) {
                    LOG_WARN("PrxP2PAcceptHelper[%p]: Timed out waiting for p2p to succeed", this);
                }
                else {
                    LOG_WARN("PrxP2PAcceptHelper[%p]: Failed waiting for p2p to succeed: err %d", this, err);
                }
                break;
            }
        }
        pclAcceptCtxtTracker.Remove(p2pCtxt_index);

        if(err) {
            goto end;
        }

        if (state.stop == STOP_STATE_STOPPING) {
            LOG_INFO("PrxP2PAcceptHelper[%p]: Force closing..", this);
            err = VPL_ERR_NOT_RUNNING; // err out
            goto end;
        }
    }

    if (p2pCtxt.result != TS_OK) {
        LOG_WARN("PrxP2PAcceptHelper[%p]: p2p done failed: err %d", this, p2pCtxt.result);
        err = p2pCtxt.result;
        goto end;
    }

    {
        MutexAutoLock lock(&mutex);
        if (state.stop == STOP_STATE_STOPPING) {
            LOG_INFO("PrxP2PAcceptHelper[%p]: Force closing..", this);
            goto end;
        }
        p2pSocket = p2pCtxt.socket;
    }

    // Prepare local id
    localId.device_id = localInfo->GetDeviceId();
    localId.user_id = localInfo->GetUserId();
    localId.instance_id = localInstanceIdStr;
    localId.region = region;

    err = localInfo->GetCcdServerKey(ccdServerKey);
    if (err != 0) {
        LOG_ERROR("PrxP2PAcceptHelper[%p]: GetCcdServerKey failed: err %d",
                  this, err);
        goto end;
    }

    pxdLogin.callback = loginDone;
    pxdLogin.is_incoming = true;
    pxdLogin.server_id = &localId;
    pxdLogin.server_key = (void *)ccdServerKey.data();
    pxdLogin.server_key_length  = static_cast<int>(ccdServerKey.size());
    pxdLogin.socket = p2pSocket;
    pxdLogin.opaque = reinterpret_cast<void*>(pclAcceptCtxtTracker.Add(&p2pCtxt));

    {
        MutexAutoLock lock(&mutex);

        if (state.stop == STOP_STATE_STOPPING) {
            LOG_INFO("PrxP2PAcceptHelper[%p]: Force closing..", this);
            err = VPL_ERR_NOT_RUNNING; // err out
            goto end;
        }

        LOG_INFO("PrxP2PAcceptHelper[%p]: Calling pxd_login", this);
        pxd_login(&pxdLogin, &pxdErr);
        if (pxdErr.error != 0) {
            LOG_ERROR("PrxP2PAcceptHelper[%p]: pxd_login failed: err %d, Msg: %s",
                              this, pxdErr.error, pxdErr.message);
            err = pxdErr.error;
            pclAcceptCtxtTracker.Remove(reinterpret_cast<u32>(pxdLogin.opaque));
            goto end;
        }

        p2pCtxt.result = TS_ERR_TIMEOUT;
        while (p2pCtxt.result == TS_ERR_TIMEOUT &&
               state.stop != STOP_STATE_STOPPING) {
            err = VPLCond_TimedWait(&cond, &mutex, condWaitTimeout_pxdLoginCb);
            if (err) {
                if (err == VPL_ERR_TIMEOUT) {
                    LOG_WARN("PrxP2PAcceptHelper[%p]: Timed out waiting for pxd_login to succeed", this);
                }
                else {
                    LOG_WARN("PrxP2PAcceptHelper[%p]: Failed waiting for pxd_login to succeed: err %d", this, err);
                }
                break;
            }
        }
        pclAcceptCtxtTracker.Remove(reinterpret_cast<u32>(pxdLogin.opaque));
        if(err) {
            goto end;
        }

        if (state.stop == STOP_STATE_STOPPING) {
            LOG_INFO("PrxP2PAcceptHelper[%p]: Force closing..", this);
            err = VPL_ERR_NOT_RUNNING; // err out
            goto end;
        }
    }

    if (p2pCtxt.result != TS_OK) {
        LOG_ERROR("PrxP2PAcceptHelper[%p]: login_done failed: err %d",
                          this, p2pCtxt.result);
        err = p2pCtxt.result;
        goto end;
    }

    if (!localInfo->IsAuthorizedClient(remoteDeviceId)) {
        LOG_ERROR("PrxP2PAcceptHelper[%p]: Device "FMTu64" is not authorized to connect", this, remoteDeviceId);
        err = TS_ERR_NO_AUTH;
        goto end;
    }

    // Duplicate session key
    duplicateSessionKey =  new (std::nothrow) std::string(sessionKey);
    if (duplicateSessionKey == NULL) {
        LOG_ERROR("PrxP2PAcceptHelper[%p]: Copy sessionKey failed, out of memory.", this);
        err = TS_ERR_NO_MEM;
    }

 end:
    if (err) {
        if (!VPLSocket_Equal(p2pCtxt.socket, VPLSOCKET_INVALID)) {
            VPLSocket_Close(p2pCtxt.socket);
            p2pCtxt.socket = VPLSOCKET_INVALID;
        }
    }

    if (!VPLSocket_Equal(p2pCtxt.socket, VPLSOCKET_INVALID)) {
        (*acceptedSocketHandler)(p2pSocket, ROUTE_TYPE_P2P,
                                 remoteUserId, remoteDeviceId, remoteInstanceId,
                                 duplicateSessionKey, this, acceptedSocketHandler_context);
        // Ownership of socket transfers to connectedSocketHandler.
        {
            MutexAutoLock lock(&mutex);
            p2pSocket = VPLSOCKET_INVALID;
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
// class method
void Ts2::Link::PrxP2PAcceptHelper::p2pDoneCb(int result, VPLSocket_t socket, void* ctxt)
{
    PclAcceptCtxt *opCtxt = pclAcceptCtxtTracker.Lookup(reinterpret_cast<u32>(ctxt));
    if (!opCtxt) {
        LOG_WARN("Ignoring late callback");
        return;
    }
    MutexAutoLock lock(&(opCtxt->helper->mutex));
    LOG_INFO("PrxP2PAcceptHelper[%p]: p2p done: %d", opCtxt->helper, result);
    if (result == VPL_OK) {
        opCtxt->result = TS_OK;
        opCtxt->socket = socket;
    } else {
        opCtxt->result = TS_ERR_INTERNAL;
        LOG_ERROR("PrxP2PAcceptHelper[%p]: p2p done failed: %d", opCtxt->helper, result);
    }
    VPLCond_Signal(&(opCtxt->helper->cond));
}
// class method
void Ts2::Link::PrxP2PAcceptHelper::loginDone(pxd_cb_data_t *cb_data)
{
    PclAcceptCtxt *opCtxt = pclAcceptCtxtTracker.Lookup(reinterpret_cast<u32>(cb_data->op_opaque));
    if (!opCtxt) {
        LOG_WARN("Ignoring late callback");
        return;
    }
    MutexAutoLock lock(&(opCtxt->helper->mutex));
    LOG_INFO("PrxP2PAcceptHelper[%p]: loginDone done: "FMTu64, opCtxt->helper, cb_data->result);
    if (cb_data->result == pxd_op_successful) {
        opCtxt->result = TS_OK;
        opCtxt->socket = cb_data->socket;
    } else {
        opCtxt->result = TS_ERR_BAD_CRED;
        LOG_ERROR("PrxP2PAcceptHelper[%p]: loginDone failed: "FMTu64, opCtxt->helper, cb_data->result);
        if (cb_data->result == pxd_credentials_rejected) {
            // We are not sure it is because of client credentials are bad or server key is bad.
            // We destroy server key on server side also.
            opCtxt->helper->localInfo->ResetCcdServerKey();
        }
    }
    VPLCond_Signal(&(opCtxt->helper->cond));
}
// class method
void Ts2::Link::PrxP2PAcceptHelper::IncomingRequest(pxd_cb_data_t *cb_data)
{
    PclAcceptCtxt *opCtxt = pclAcceptCtxtTracker.Lookup(reinterpret_cast<u32>(cb_data->op_opaque));
    if (!opCtxt) {
        LOG_WARN("Ignoring late callback");
        return;
    }
    MutexAutoLock lock(&(opCtxt->helper->mutex));
    LOG_INFO("PrxP2PAcceptHelper[%p]: IncomingRequest done: "FMTu64, opCtxt->helper, cb_data->result);
    if (cb_data->address_count > 0) {

        if (cb_data->address_count != 1) {
            LOG_WARN("PrxP2PAcceptHelper[%p]: Address count is %d, greater than 1", opCtxt->helper, cb_data->address_count);
            for(int i = 0; i < cb_data->address_count; i++) {
                if (cb_data->addresses[i].ip_length != 4) {
                    LOG_WARN("PrxP2PAcceptHelper[%p]: IP Length are %d bytes", opCtxt->helper, cb_data->addresses[i].ip_length);
                    continue;
                }
                LOG_WARN("PrxP2PAcceptHelper[%p]: Address[%d]: %s", opCtxt->helper, i, PXD_ADDR_2_CSTR(cb_data->addresses[i]));
            }
        }

        if (cb_data->addresses[0].ip_length != 4) {
            opCtxt->resultIncomingRequest = TS_ERR_INTERNAL;
            LOG_ERROR("PrxP2PAcceptHelper[%p]: IP Length are %d bytes", opCtxt->helper, cb_data->addresses[0].ip_length);
            goto end;
        }

        if (opCtxt->helper->remoteExtAddr.ip_address == NULL) {
            // Need to be destroyed
            // NOTE: No Ipv6
            opCtxt->helper->remoteExtAddr.ip_address = new (std::nothrow) char[4];
            if (opCtxt->helper->remoteExtAddr.ip_address == NULL) {
                opCtxt->resultIncomingRequest = TS_ERR_INTERNAL;
                LOG_ERROR("PrxP2PAcceptHelper[%p]: Allocate remoteExtAddr, out of memory.", opCtxt->helper);
                goto end;
            }
        }
        memcpy(opCtxt->helper->remoteExtAddr.ip_address, cb_data->addresses[0].ip_address, 4);
        opCtxt->helper->remoteExtAddr.ip_length = cb_data->addresses[0].ip_length;
        opCtxt->helper->remoteExtAddr.port = cb_data->addresses[0].port;
        opCtxt->helper->remoteExtAddr.type = cb_data->addresses[0].type;
        opCtxt->resultIncomingRequest = TS_OK;
        LOG_INFO("PrxP2PAcceptHelper[%p]: Remote external address %s", opCtxt->helper, PXD_ADDR_2_CSTR(opCtxt->helper->remoteExtAddr));
    } else {
        opCtxt->resultIncomingRequest = TS_ERR_INTERNAL;
        LOG_ERROR("PrxP2PAcceptHelper[%p]: IncomingRequest failed: "FMTu64, opCtxt->helper, cb_data->result);
    }
end:
    VPLCond_Signal(&(opCtxt->helper->cond));
}
// class method
void Ts2::Link::PrxP2PAcceptHelper::IncomingLogin(pxd_cb_data_t *cb_data)
{
    PclAcceptCtxt *opCtxt = pclAcceptCtxtTracker.Lookup(reinterpret_cast<u32>(cb_data->op_opaque));
    if (!opCtxt) {
        LOG_WARN("Ignoring late callback");
        return;
    }
    MutexAutoLock lock(&(opCtxt->helper->mutex));
    LOG_INFO("PrxP2PAcceptHelper[%p]: IncomingLogin done: "FMTu64, opCtxt->helper, cb_data->result);
    opCtxt->socket = cb_data->socket;
    if (cb_data->blob != NULL) {
        opCtxt->helper->remoteUserId = cb_data->blob->client_user;
        opCtxt->helper->remoteDeviceId = cb_data->blob->client_device;
        int nmatch = sscanf(cb_data->blob->client_instance, FMTu32, &opCtxt->helper->remoteInstanceId);
        if (nmatch != 1) {
            opCtxt->result = TS_ERR_INTERNAL;
            LOG_ERROR("PrxP2PAcceptHelper[%p]: Failed to parse instanceId string", opCtxt->helper);
            goto end;
        }
    } else {
        opCtxt->result = TS_ERR_INTERNAL;
        LOG_ERROR("PrxP2PAcceptHelper[%p]: IncomingLogin failed: "FMTu64, opCtxt->helper, cb_data->result);
        goto end;
    }

    opCtxt->helper->sessionKey.assign(cb_data->blob->key, cb_data->blob->key_length);

    opCtxt->result = TS_OK;
end:
    VPLCond_Signal(&(opCtxt->helper->cond));
}

