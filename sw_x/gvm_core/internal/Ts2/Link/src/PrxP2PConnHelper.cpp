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

#include "PrxP2PConnHelper.hpp"
#include "ObjectTracker.hpp"

#include <gvm_errors.h>
#include <gvm_thread_utils.h>
#include <log.h>

#include <vpl_string.h>
#include <vplu_format.h>
#include <vplu_mutex_autolock.hpp>
#include "PrxP2PRoutines.hpp"

static const VPLTime_t condWaitTimeout_pxdSupplyExternalCb = VPLTime_FromSec(10);
static const VPLTime_t condWaitTimeout_pxdConnectDoneCb = VPLTime_FromSec(30);
static const VPLTime_t condWaitTimeout_p2pDoneCb = VPLTime_FromSec(30);
static const VPLTime_t condWaitTimeout_pxdLoginCb = VPLTime_FromSec(10);

const size_t Ts2::Link::PrxP2PConnHelper::ThreadStackSize = UTIL_DEFAULT_THREAD_STACK_SIZE;

namespace Ts2 {
namespace Link {
// Callback context for Pxd Client Libaray in ConnHelper
class PclConnCtxt {
public:
    PclConnCtxt(PrxP2PConnHelper* h) : helper(h), result(TS_ERR_TIMEOUT), socket(VPLSOCKET_INVALID) {
    }
    ~PclConnCtxt() {
    }
    PrxP2PConnHelper* helper;
    int result;
    VPLSocket_t socket;
};
} // end namespace Link
} // end namespace Ts2

static Ts2::Link::ObjectTracker<Ts2::Link::PclConnCtxt> pclConnCtxtTracker;

Ts2::Link::PrxP2PConnHelper::PrxP2PConnHelper(u64 remoteUserId, u64 remoteDeviceId, u32 remoteInstanceId,
                                              ConnectedSocketHandler connectedSocketHandler, void *connectedSocketHandler_context,
                                              LocalInfo *localInfo)
    : ConnectHelper(remoteUserId, remoteDeviceId, remoteInstanceId, connectedSocketHandler, connectedSocketHandler_context, localInfo),
      connState(IN_PRX),
      prxSocket(VPLSOCKET_INVALID),
      p2pSocket(VPLSOCKET_INVALID),
      p2pHandle(0),
      pxdClient(NULL)
{
    localExtAddr.ip_address = NULL;
    localExtAddr.type = 0;
    localExtAddr.ip_length = 0;
    localExtAddr.port = 0;
    remoteExtAddr.ip_address = NULL;
    remoteExtAddr.type = 0;
    remoteExtAddr.ip_length = 0;
    remoteExtAddr.port = 0;

    VPLCond_Init(&cond);
}

Ts2::Link::PrxP2PConnHelper::~PrxP2PConnHelper()
{
    if (localExtAddr.ip_address != NULL) {
        delete[] localExtAddr.ip_address;
        localExtAddr.ip_address = NULL;
    }
    if (remoteExtAddr.ip_address != NULL) {
        delete[] remoteExtAddr.ip_address;
        remoteExtAddr.ip_address = NULL;
    }

    VPLCond_Destroy(&cond);
}

int Ts2::Link::PrxP2PConnHelper::Connect()
{
    int err = 0;

    {
        MutexAutoLock lock(&mutex);
        // state check and transition
        if (state.thread != THREAD_STATE_NOTHREAD) {
            LOG_ERROR("PrxP2PConnHelper[%p]: Wrong thread state %d", this, state.thread);
            err = TS_ERR_WRONG_STATE;
            goto end;
        }
        state.thread = THREAD_STATE_SPAWNING;
    }

    err = Util_SpawnThread(threadMain, this, ThreadStackSize, /*isJoinable*/VPL_TRUE, &prxThread);
    if (err) {
        LOG_ERROR("PrxP2PConnHelper[%p]: Failed to spawn thread: err %d", this, err);
        {
            MutexAutoLock lock(&mutex);
            state.thread = THREAD_STATE_NOTHREAD;
        }
        goto end;
    }

 end:
    return err;
}

int Ts2::Link::PrxP2PConnHelper::WaitDone()
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
            LOG_ERROR("PrxP2PConnHelper[%p]: Failed to Join thread: err %d", this, err);
            goto end;
        }
        LOG_INFO("PrxP2PConnHelper[%p]: Joined PRX thread", this);
        if (connState == IN_P2P) {
            err = VPLDetachableThread_Join(&p2pThread);
            if (err) {
                LOG_ERROR("PrxP2PConnHelper[%p]: Failed to Join thread: err %d", this, err);
                goto end;
            }
            LOG_INFO("PrxP2PConnHelper[%p]: Joined P2P thread", this);
        }
    }
    {
        MutexAutoLock lock(&mutex);
        state.thread = THREAD_STATE_NOTHREAD;
    }

 end:
    return err;
}

void Ts2::Link::PrxP2PConnHelper::ForceClose()
{
    MutexAutoLock lock(&mutex);
    if (state.stop != STOP_STATE_NOSTOP) {
        LOG_WARN("PrxP2PConnHelper[%p]: Wrong stop state %d", this, state.stop);

    } else {
        state.stop = STOP_STATE_STOPPING;
        if (pxdClient != NULL) {
            // Stop pxd lib
            pxd_error_t pxdErr;
            LOG_INFO("PrxP2PConnHelper[%p]: Force to close socket", this);
            pxd_close(&pxdClient, 0, &pxdErr);
            pxdClient = NULL;
        }
        if (p2pHandle != 0) {
            // Stop p2p library
            LOG_INFO("PrxP2PConnHelper[%p]: Force to close p2p library", this);
            p2p_stop(p2pHandle);
            p2pHandle = 0;
        }
        if (!VPLSocket_Equal(p2pSocket, VPLSOCKET_INVALID)) {
            // Stop pxd_login
            LOG_INFO("PrxP2PConnHelper[%p]: Force to close socket", this);
            VPLSocket_Shutdown(p2pSocket, VPLSOCKET_SHUT_RDWR);
            VPLSocket_Close(p2pSocket);
            p2pSocket = VPLSOCKET_INVALID;
        }
    }
    VPLCond_Signal(&cond);
}

VPLTHREAD_FN_DECL Ts2::Link::PrxP2PConnHelper::threadMain(void *param)
{
    PrxP2PConnHelper *helper = static_cast<PrxP2PConnHelper*>(param);
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

void Ts2::Link::PrxP2PConnHelper::prxThreadMain()
{
    int err = 0;
    pxd_callback_t pxdCallbacks =
        {
            supplyLocal,
            supplyExternal,
            connectDone,
            lookupDone,
            incomingRequest,
            incomingLogin,
            rejectCcdCreds,
            rejectPxdCreds
        };
    pxd_open_t pxdOpen;
    pxd_connect_t pxdConnect;

    pxd_id_t localId;
    pxd_id_t remoteId;

    pxd_cred_t pxdCred;
    pxd_cred_t ccdCred;

    pxd_error_t pxdErr;

    char region[] = ""; // Dummy
    char localInstanceIdStr[16];  // large enough for u32 in decimal
    VPL_snprintf(localInstanceIdStr, ARRAY_SIZE_IN_BYTES(localInstanceIdStr), FMTu32, localInfo->GetInstanceId());
    char remoteInstanceIdStr[16];  // large enough for u32 in decimal
    VPL_snprintf(remoteInstanceIdStr, ARRAY_SIZE_IN_BYTES(remoteInstanceIdStr), FMTu32, remoteInstanceId);
    std::string pxdSessionKey;
    std::string pxdLoginBlob;
    std::string ccdSessionKey;
    std::string ccdLoginBlob;
    std::string *duplicateSessionKey = NULL;

    PclConnCtxt openCtxt(this);
    PclConnCtxt connCtxt(this);

    // Prepare local id
    localId.device_id = localInfo->GetDeviceId();
    localId.user_id = localInfo->GetUserId();
    localId.instance_id = localInstanceIdStr;
    localId.region = region;

    err = localInfo->GetPxdSessionKey(pxdSessionKey, pxdLoginBlob);
    if (err != 0) {
        LOG_ERROR("PrxP2PConnHelper[%p]: GetPxdSessionKey failed: err %d",
                  this, err);
        goto end;
    }
    //mangleKey(pxdSessionKey);  // test case: bad pxd session key to pxd_login()
    pxdCred.id = &localId;
    pxdCred.key = (void*)pxdSessionKey.data();
    pxdCred.key_length  = static_cast<int>(pxdSessionKey.size());
    pxdCred.opaque = (void*)pxdLoginBlob.data();
    pxdCred.opaque_length = static_cast<int>(pxdLoginBlob.size());

    // PXD open
    pxdOpen.cluster_name = (char*)localInfo->GetPxdSvrName().c_str();
    pxdOpen.is_incoming = false; // This is a client
    pxdOpen.opaque = reinterpret_cast<void*>(pclConnCtxtTracker.Add(&openCtxt));
    pxdOpen.credentials = &pxdCred;
    pxdOpen.callback = &pxdCallbacks;

    LOG_INFO("PrxP2PConnHelper[%p]: Calling pxd_open", this);

    {
        MutexAutoLock lock(&mutex);

        if (state.stop == STOP_STATE_STOPPING) {
            LOG_INFO("PrxP2PConnHelper[%p]: Force closing..", this);
            err = VPL_ERR_NOT_RUNNING; // err out
            goto end;
        }

        pxdClient = pxd_open(&pxdOpen, &pxdErr);

        if (pxdClient == NULL ||
           pxdErr.error != 0) {
            LOG_ERROR("PrxP2PConnHelper[%p]: pxd_open failed: err %d, Msg: %s",
                      this, pxdErr.error, pxdErr.message);
            err = pxdErr.error;
            pclConnCtxtTracker.Remove(reinterpret_cast<u32>(pxdOpen.opaque));
            goto end;
        }

        // Wait for supply_external
        while (openCtxt.result == TS_ERR_TIMEOUT &&
               state.stop != STOP_STATE_STOPPING) {
            err = VPLCond_TimedWait(&cond, &mutex, condWaitTimeout_pxdSupplyExternalCb);
            if (err) {
                if (err == VPL_ERR_TIMEOUT) {
                    LOG_WARN("PrxP2PConnHelper[%p]: Timed out waiting for supply_external", this);
                }
                else {
                    LOG_WARN("PrxP2PConnHelper[%p]: Failed waiting for supply_external: err %d", this, err);
                }
                break;
            }
        }
        pclConnCtxtTracker.Remove(reinterpret_cast<u32>(pxdOpen.opaque));
        if(err) {
            goto end;
        }

        if (state.stop == STOP_STATE_STOPPING) {
            LOG_INFO("PrxP2PConnHelper[%p]: Force closing..", this);
            err = VPL_ERR_NOT_RUNNING; // err out
            goto end;
        }
    }
    if (openCtxt.result != TS_OK) {
        LOG_ERROR("PrxP2PConnHelper[%p]: supply_external failed: err %d",
                          this, openCtxt.result);
        err = openCtxt.result;
        goto end;
    }

    // Prepare remote id
    remoteId.region = region;
    remoteId.user_id = remoteUserId;
    remoteId.device_id = remoteDeviceId;
    remoteId.instance_id = remoteInstanceIdStr;

    err = localInfo->GetCcdSessionKey(remoteUserId, remoteDeviceId, remoteInstanceId, ccdSessionKey, ccdLoginBlob);
    if (err != 0) {
        LOG_ERROR("PrxP2PConnHelper[%p]: GetCcdSessionKey failed: err %d",
                  this, err);
        goto end;
    }

    //mangleKey(ccdSessionKey);  // test case: bad ccd session key to pxd_connect()
    ccdCred.id = &localId;
    ccdCred.key = (void*)ccdSessionKey.data();
    ccdCred.key_length  = static_cast<int>(ccdSessionKey.size());
    ccdCred.opaque = (void*)ccdLoginBlob.data();
    ccdCred.opaque_length = static_cast<int>(ccdLoginBlob.size());

    // PXD connet
    pxdConnect.creds = &ccdCred;

    pxdConnect.target = &remoteId;
    pxdConnect.pxd_dns = (char*)localInfo->GetPxdSvrName().c_str();

    pxdConnect.address_count = 1;
    pxdConnect.addresses[0].ip_address = localExtAddr.ip_address;
    pxdConnect.addresses[0].ip_length  = localExtAddr.ip_length;
    pxdConnect.addresses[0].port = localExtAddr.port;
    pxdConnect.addresses[0].type = localExtAddr.type;
    pxdConnect.opaque = reinterpret_cast<void*>(pclConnCtxtTracker.Add(&connCtxt));

    {
        MutexAutoLock lock(&mutex);

        if (state.stop == STOP_STATE_STOPPING) {
            LOG_INFO("PrxP2PConnHelper[%p]: Force closing..", this);
            goto end;
        }

        LOG_INFO("PrxP2PConnHelper[%p]: Calling pxd_connect", this);
        pxd_connect(pxdClient, &pxdConnect, &pxdErr);
        if (pxdErr.error != 0) {
            LOG_ERROR("PrxP2PConnHelper[%p]: pxd_connect failed: err %d, Msg: %s",
                              this, pxdErr.error, pxdErr.message);
            err = pxdErr.error;
            pclConnCtxtTracker.Remove(reinterpret_cast<u32>(pxdConnect.opaque));
            goto end;
        }

        // Wait for connect_done
        while (connCtxt.result == TS_ERR_TIMEOUT &&
               state.stop != STOP_STATE_STOPPING) {
            err = VPLCond_TimedWait(&cond, &mutex, condWaitTimeout_pxdConnectDoneCb);
            if (err) {
                if (err == VPL_ERR_TIMEOUT) {
                    LOG_WARN("PrxP2PConnHelper[%p]: Timed out waiting for connect_done", this);
                }
                else {
                    LOG_WARN("PrxP2PConnHelper[%p]: Failed waiting for connect_done: err %d", this, err);
                }
                break;
            }
        }
        pclConnCtxtTracker.Remove(reinterpret_cast<u32>(pxdConnect.opaque));
        if(err) {
            goto end;
        }

        if (state.stop == STOP_STATE_STOPPING) {
            LOG_INFO("PrxP2PConnHelper[%p]: Force closing..", this);
            err = VPL_ERR_NOT_RUNNING; // err out
            goto end;
        }
    }
    if (connCtxt.result != TS_OK) {
        LOG_ERROR("PrxP2PConnHelper[%p]: connect_done failed: err %d",
                          this, connCtxt.result);
        err = connCtxt.result;
        goto end;
    }

    // Duplicate session key
    duplicateSessionKey =  new (std::nothrow) std::string(ccdSessionKey);
    if (duplicateSessionKey == NULL) {
        LOG_ERROR("PrxP2PConnHelper[%p]: Copy sessionKey failed, out of memory.", this);
        err = TS_ERR_NO_MEM;
    }

 end:
    if (err) {
        if (!VPLSocket_Equal(connCtxt.socket, VPLSOCKET_INVALID)) {
            VPLSocket_Close(connCtxt.socket);
            connCtxt.socket = VPLSOCKET_INVALID;
        }
        if (pxdClient != NULL) {
            pxd_close(&pxdClient, 0, &pxdErr);
            pxdClient = NULL;
        }
    }

    if (!VPLSocket_Equal(connCtxt.socket, VPLSOCKET_INVALID)) {
        if (!(localInfo->GetDisabledRouteTypes() & LocalInfo::DISABLED_ROUTE_TYPE_P2P)) {
            // Fork P2P handler thread
            prxSocket = connCtxt.socket;
            connState = IN_P2P;
            err = Util_SpawnThread(threadMain, this, ThreadStackSize, /*isJoinable*/VPL_TRUE, &p2pThread);
            if (err) {
                LOG_ERROR("PrxP2PConnHelper[%p]: Failed to spawn thread: err %d", this, err);
            }
        }

        if (!(localInfo->GetDisabledRouteTypes() & LocalInfo::DISABLED_ROUTE_TYPE_PRX)) {
            (*connectedSocketHandler)(connCtxt.socket, ROUTE_TYPE_PRX, duplicateSessionKey, this, connectedSocketHandler_context);
        }
        // Ownership of socket transfers to connectedSocketHandler.
    }

    if (err||
       (localInfo->GetDisabledRouteTypes() & LocalInfo::DISABLED_ROUTE_TYPE_P2P)) {
        // Notify no more sockets from this helper.
        (*connectedSocketHandler)(VPLSOCKET_INVALID, ROUTE_TYPE_INVALID, NULL, this, connectedSocketHandler_context);

        {
            MutexAutoLock lock(&mutex);
            state.stop = STOP_STATE_STOPPING;
        }
    }

    LOG_INFO("PrxP2PConnHelper[%p]: PRX thread exiting", this);
}

void Ts2::Link::PrxP2PConnHelper::p2pThreadMain()
{
    int err = 0;
    pxd_login_t pxdLogin;
    pxd_cred_t ccdCred;
    pxd_id_t localId;
    pxd_error_t pxdErr;
    char region[] = ""; // Dummy
    char localInstanceIdStr[16];  // large enough for u32 in decimal
    VPL_snprintf(localInstanceIdStr, ARRAY_SIZE_IN_BYTES(localInstanceIdStr), FMTu32, localInfo->GetInstanceId());

    std::string ccdSessionKey;
    std::string ccdLoginBlob;
    std::string *duplicateSessionKey = NULL;

    PclConnCtxt p2pCtxt(this);
    u32 p2pCtxt_index = 0;  // Note, 0 is an invalid ObjectTracker index.

    // Try P2P
    VPLSocket_addr_t remoteAddr;
    if (remoteExtAddr.ip_address == NULL) {
        LOG_ERROR("PrxP2PConnHelper[%p]: Address of remoteExtAddr is NULL", this);
        goto end;
    }
    PXD_ADDR_2_VPL_ADDR(remoteExtAddr, remoteAddr);
    LOG_INFO("PrxP2PConnHelper[%p]: Calling p2p_connect", this);
    p2pCtxt_index = pclConnCtxtTracker.Add(&p2pCtxt);

    {
        MutexAutoLock lock(&mutex);
        if (state.stop == STOP_STATE_STOPPING) {
            LOG_INFO("PrxP2PConnHelper[%p]: Force closing..", this);
            err = VPL_ERR_NOT_RUNNING; // err out
            goto end;
        }
        err = p2p_connect(p2pHandle, prxSocket, &remoteAddr, sizeof(remoteAddr), reinterpret_cast<void*>(p2pCtxt_index), p2pDoneCb);

        if (err != VPL_OK) {
            LOG_ERROR("PrxP2PConnHelper[%p]: p2p_connect failed: err %d", this, err);
            pclConnCtxtTracker.Remove(p2pCtxt_index);
            goto end;
        }

        // Wait for P2P done
        while (p2pCtxt.result == TS_ERR_TIMEOUT &&
               state.stop != STOP_STATE_STOPPING) {
            err = VPLCond_TimedWait(&cond, &mutex, condWaitTimeout_p2pDoneCb);
            if (err) {
                if (err == VPL_ERR_TIMEOUT) {
                    LOG_WARN("PrxP2PConnHelper[%p]: Timed out waiting for p2p to succeed", this);
                }
                else {
                    LOG_WARN("PrxP2PConnHelper[%p]: Failed waiting for p2p to succeed: err %d", this, err);
                }
                break;
            }
        }
        pclConnCtxtTracker.Remove(p2pCtxt_index);
        if(err) {
            goto end;
        }

        if (state.stop == STOP_STATE_STOPPING) {
            LOG_INFO("PrxP2PConnHelper[%p]: Force closing..", this);
            err = VPL_ERR_NOT_RUNNING; // err out
            goto end;
        }
    }
    if (p2pCtxt.result != VPL_OK) {
        LOG_WARN("PrxP2PConnHelper[%p]: p2p done failed: err %d", this, p2pCtxt.result);
        err = p2pCtxt.result;
        goto end;
    }

    {
        MutexAutoLock lock(&mutex);
        if (state.stop == STOP_STATE_STOPPING) {
            LOG_INFO("PrxP2PConnHelper[%p]: Force closing..", this);
            goto end;
        }
        p2pSocket = p2pCtxt.socket;
    }

    // Prepare local id
    localId.device_id = localInfo->GetDeviceId();
    localId.user_id = localInfo->GetUserId();
    localId.instance_id = localInstanceIdStr;
    localId.region = region;

    err = localInfo->GetCcdSessionKey(remoteUserId, remoteDeviceId, remoteInstanceId, ccdSessionKey, ccdLoginBlob);
    if (err != 0) {
        LOG_ERROR("PrxP2PConnHelper[%p]: GetCcdSessionKey failed: err %d",
                  this, err);
        goto end;
    }
    ccdCred.id = &localId;
    ccdCred.key = (void*)ccdSessionKey.data();
    ccdCred.key_length  = static_cast<int>(ccdSessionKey.size());
    ccdCred.opaque = (void*)ccdLoginBlob.data();
    ccdCred.opaque_length = static_cast<int>(ccdLoginBlob.size());

    pxdLogin.callback = loginDone;
    pxdLogin.is_incoming = false;
    pxdLogin.credentials = &ccdCred;
    pxdLogin.socket = p2pSocket;
    pxdLogin.opaque = reinterpret_cast<void*>(pclConnCtxtTracker.Add(&p2pCtxt));

    {
        MutexAutoLock lock(&mutex);
        LOG_INFO("PrxP2PConnHelper[%p]: Calling pxd_login", this);
        pxd_login(&pxdLogin, &pxdErr);
        if (pxdErr.error != 0) {
            LOG_ERROR("PrxP2PConnHelper[%p]: pxd_login failed: err %d, Msg: %s",
                              this, pxdErr.error, pxdErr.message);
            err = pxdErr.error;
            pclConnCtxtTracker.Remove(reinterpret_cast<u32>(pxdLogin.opaque));
            goto end;
        }

        // Wait for P2P done
        p2pCtxt.result = TS_ERR_TIMEOUT;
        while (p2pCtxt.result == TS_ERR_TIMEOUT &&
               state.stop != STOP_STATE_STOPPING) {
            err = VPLCond_TimedWait(&cond, &mutex, condWaitTimeout_pxdLoginCb);
            if (err) {
                if (err == VPL_ERR_TIMEOUT) {
                    LOG_WARN("PrxP2PConnHelper[%p]: Timed out waiting for pxd_login to succeed", this);
                }
                else {
                    LOG_WARN("PrxP2PConnHelper[%p]: Failed waiting for pxd_login to succeed: err %d", this, err);
                }
                break;
            }
        }
        pclConnCtxtTracker.Remove(reinterpret_cast<u32>(pxdLogin.opaque));
        if(err) {
            goto end;
        }

        if (state.stop == STOP_STATE_STOPPING) {
            LOG_INFO("PrxP2PConnHelper[%p]: Force closing..", this);
            err = VPL_ERR_NOT_RUNNING; // err out
            goto end;
        }
    }
    if (p2pCtxt.result != TS_OK) {
        LOG_ERROR("PrxP2PConnHelper[%p]: login_done failed: err %d",
                          this, p2pCtxt.result);
        err = p2pCtxt.result;
        goto end;
    }

    // Duplicate session key
    duplicateSessionKey =  new (std::nothrow) std::string(ccdSessionKey);
    if (duplicateSessionKey == NULL) {
        LOG_ERROR("PrxP2PConnHelper[%p]: Copy sessionKey failed, out of memory.", this);
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
        (*connectedSocketHandler)(p2pSocket, ROUTE_TYPE_P2P, duplicateSessionKey, this, connectedSocketHandler_context);
        // Ownership of socket transfers to connectedSocketHandler.
        {
            MutexAutoLock lock(&mutex);
            p2pSocket = VPLSOCKET_INVALID;
        }
    }

    // Notify no more sockets from this helper.
    (*connectedSocketHandler)(VPLSOCKET_INVALID, ROUTE_TYPE_INVALID, NULL, this, connectedSocketHandler_context);

    {
        MutexAutoLock lock(&mutex);
        state.stop = STOP_STATE_STOPPING;
    }

    LOG_INFO("PrxP2PConnHelper[%p]: P2P thread exiting", this);
}

// class method
void Ts2::Link::PrxP2PConnHelper::p2pDoneCb(int result, VPLSocket_t socket, void* ctxt)
{
    PclConnCtxt *opCtxt = pclConnCtxtTracker.Lookup(reinterpret_cast<u32>(ctxt));
    if (!opCtxt) {
        LOG_WARN("Ignoring late callback");
        return;
    }
    MutexAutoLock lock(&(opCtxt->helper->mutex));
    LOG_INFO("PrxP2PConnHelper[%p]: p2p done: %d", opCtxt->helper, result);
    if (result == VPL_OK) {
        opCtxt->result = TS_OK;
        opCtxt->socket = socket;
    } else {
        opCtxt->result = TS_ERR_INTERNAL;
        LOG_ERROR("PrxP2PConnHelper[%p]: p2p done failed: %d", opCtxt->helper, result);
    }
    VPLCond_Signal(&(opCtxt->helper->cond));
}
// class method
void Ts2::Link::PrxP2PConnHelper::supplyExternal(pxd_cb_data_t *cb_data)
{
    PclConnCtxt *opCtxt = pclConnCtxtTracker.Lookup(reinterpret_cast<u32>(cb_data->client_opaque));
    if (!opCtxt) {
        LOG_WARN("Ignoring late callback");
        return;
    }
    MutexAutoLock lock(&(opCtxt->helper->mutex));
    LOG_INFO("PrxP2PConnHelper[%p]: supplyExternal done: "FMTu64, opCtxt->helper, cb_data->result);
    if (cb_data->address_count > 0) {

        if (cb_data->address_count != 1) {
            LOG_WARN("PrxP2PConnHelper[%p]: Address count is %d, greater than 1", opCtxt->helper, cb_data->address_count);
            for(int i = 0; i < cb_data->address_count; i++) {
                if (cb_data->addresses[i].ip_length != 4) {
                    LOG_WARN("PrxP2PConnHelper[%p]: IP Length are %d bytes", opCtxt->helper, cb_data->addresses[i].ip_length);
                    continue;
                }
                LOG_WARN("PrxP2PConnHelper[%p]: Address[%d]: %s", opCtxt->helper, i, PXD_ADDR_2_CSTR(cb_data->addresses[i]));
            }
        }

        if (cb_data->addresses[0].ip_length != 4) {
            opCtxt->result = TS_ERR_INTERNAL;
            LOG_ERROR("PrxP2PConnHelper[%p]: IP Length are %d bytes", opCtxt->helper, cb_data->addresses[0].ip_length);
            goto end;
        }

        if (opCtxt->helper->localExtAddr.ip_address == NULL) {
            // Need to be destroyed
            // NOTE: No Ipv6
            opCtxt->helper->localExtAddr.ip_address = new (std::nothrow) char[4];
            if (opCtxt->helper->localExtAddr.ip_address == NULL) {
                opCtxt->result = TS_ERR_INTERNAL;
                LOG_ERROR("PrxP2PConnHelper[%p]: Allocate localExtAddr, out of memory.", opCtxt->helper);
                goto end;
            }
        }
        memcpy(opCtxt->helper->localExtAddr.ip_address, cb_data->addresses[0].ip_address, 4);
        opCtxt->helper->localExtAddr.ip_length = cb_data->addresses[0].ip_length;
        opCtxt->helper->localExtAddr.port = cb_data->addresses[0].port;
        opCtxt->helper->localExtAddr.type = cb_data->addresses[0].type;
        opCtxt->result = TS_OK; // Tell waiting thread, everything seems ok
        LOG_INFO("PrxP2PConnHelper[%p]: Local external address %s", opCtxt->helper, PXD_ADDR_2_CSTR(opCtxt->helper->localExtAddr));
    } else {
        opCtxt->result = TS_ERR_INTERNAL;
        LOG_ERROR("PrxP2PConnHelper[%p]: supplyExternal done failed: "FMTu64, opCtxt->helper, cb_data->result);
    }
end:
    VPLCond_Signal(&(opCtxt->helper->cond));
}
// class method
void Ts2::Link::PrxP2PConnHelper::loginDone(pxd_cb_data_t *cb_data)
{
    PclConnCtxt *opCtxt = pclConnCtxtTracker.Lookup(reinterpret_cast<u32>(cb_data->op_opaque));
    if (!opCtxt) {
        LOG_WARN("Ignoring late callback");
        return;
    }
    MutexAutoLock lock(&(opCtxt->helper->mutex));
    LOG_INFO("PrxP2PConnHelper[%p]: loginDone done: "FMTu64, opCtxt->helper, cb_data->result);
    opCtxt->socket = cb_data->socket;
    if (cb_data->result == pxd_op_successful) {
        opCtxt->result = TS_OK;
    } else {
        opCtxt->result = TS_ERR_BAD_CRED;
        LOG_ERROR("PrxP2PConnHelper[%p]: loginDone failed: "FMTu64, opCtxt->helper, cb_data->result);
        if (cb_data->result == pxd_op_failed) {
            // We are not sure it is because of client credentials are bad or server key is bad.
            // We destroy ccd credentials on client side also.
            opCtxt->helper->localInfo->ResetCcdSessionKey(opCtxt->helper->remoteUserId,
                                                          opCtxt->helper->remoteDeviceId,
                                                          opCtxt->helper->remoteInstanceId);
        }
    }
    VPLCond_Signal(&(opCtxt->helper->cond));
}
// class method
void Ts2::Link::PrxP2PConnHelper::connectDone(pxd_cb_data_t *cb_data)
{
    PclConnCtxt *opCtxt = pclConnCtxtTracker.Lookup(reinterpret_cast<u32>(cb_data->op_opaque));
    if (!opCtxt) {
        LOG_WARN("Ignoring late callback");
        return;
    }
    MutexAutoLock lock(&(opCtxt->helper->mutex));
    LOG_INFO("PrxP2PConnHelper[%p]: connectDone done: "FMTu64, opCtxt->helper, cb_data->result);
    opCtxt->socket = cb_data->socket;
    if (cb_data->result == pxd_op_successful &&
       cb_data->address_count > 0) {

        if (cb_data->address_count != 1) {
            LOG_WARN("PrxP2PConnHelper[%p]: Address count is %d, greater than 1", opCtxt->helper, cb_data->address_count);
            for(int i = 0; i < cb_data->address_count; i++) {
                if (cb_data->addresses[i].ip_length != 4) {
                    LOG_WARN("PrxP2PConnHelper[%p]: IP Length are %d bytes", opCtxt->helper, cb_data->addresses[i].ip_length);
                    continue;
                }
                LOG_WARN("PrxP2PConnHelper[%p]: Address[%d]: %s", opCtxt->helper, i, PXD_ADDR_2_CSTR(cb_data->addresses[i]));
            }
        }

        if (cb_data->addresses[0].ip_length != 4) {
            opCtxt->result = TS_ERR_INTERNAL;
            LOG_ERROR("PrxP2PConnHelper[%p]: IP Length are %d bytes", opCtxt->helper, cb_data->addresses[0].ip_length);
            goto end;
        }
        if (opCtxt->helper->remoteExtAddr.ip_address == NULL) {
            // Need to be destroyed
            // NOTE: No Ipv6
            opCtxt->helper->remoteExtAddr.ip_address = new (std::nothrow) char[4];
            if (opCtxt->helper->remoteExtAddr.ip_address == NULL) {
                opCtxt->result = TS_ERR_INTERNAL;
                LOG_ERROR("PrxP2PConnHelper[%p]: Allocate remoteExtAddr, out of memory.", opCtxt->helper);
                goto end;
            }
        }
        memcpy(opCtxt->helper->remoteExtAddr.ip_address, cb_data->addresses[0].ip_address, 4);
        opCtxt->helper->remoteExtAddr.ip_length = cb_data->addresses[0].ip_length;
        opCtxt->helper->remoteExtAddr.port = cb_data->addresses[0].port;
        opCtxt->helper->remoteExtAddr.type = cb_data->addresses[0].type;
        opCtxt->result = TS_OK;
        LOG_INFO("PrxP2PConnHelper[%p]: Remote external address %s", opCtxt->helper, PXD_ADDR_2_CSTR(opCtxt->helper->remoteExtAddr));
    } else {
        opCtxt->result = TS_ERR_INTERNAL;
        LOG_ERROR("PrxP2PConnHelper[%p]: connectDone failed: "FMTu64", address count: %d", opCtxt->helper, cb_data->result, cb_data->address_count);
    }
end:
    VPLCond_Signal(&(opCtxt->helper->cond));
}
// class method
void Ts2::Link::PrxP2PConnHelper::rejectCcdCreds(pxd_cb_data_t *cb_data)
{
    PclConnCtxt *opCtxt = pclConnCtxtTracker.Lookup(reinterpret_cast<u32>(cb_data->op_opaque));
    if (!opCtxt) {
        LOG_WARN("Ignoring late callback");
        return;
    }
    MutexAutoLock lock(&(opCtxt->helper->mutex));
    opCtxt->result = TS_ERR_BAD_CRED;
    // We are not sure it is because of client credentials are bad or server key is bad.
    // We destroy ccd credentials on client side also.
    opCtxt->helper->localInfo->ResetCcdSessionKey(opCtxt->helper->remoteUserId,
                                                  opCtxt->helper->remoteDeviceId,
                                                  opCtxt->helper->remoteInstanceId);
    LOG_ERROR("PrxP2PConnHelper[%p]: rejectCcdCreds: "FMTu64, opCtxt->helper, cb_data->result);
    VPLCond_Signal(&(opCtxt->helper->cond)); // This is for pxd_connect
}
// class method
void Ts2::Link::PrxP2PConnHelper::rejectPxdCreds(pxd_cb_data_t *cb_data)
{
    PclConnCtxt *opCtxt = pclConnCtxtTracker.Lookup(reinterpret_cast<u32>(cb_data->client_opaque));
    if (!opCtxt) {
        LOG_WARN("Ignoring late callback");
        return;
    }
    MutexAutoLock lock(&(opCtxt->helper->mutex));
    opCtxt->helper->localInfo->ResetPxdSessionKey();
    opCtxt->result = TS_ERR_BAD_CRED;
    LOG_ERROR("PrxP2PConnHelper[%p]: rejectPxdCreds: "FMTu64, opCtxt->helper, cb_data->result);
    VPLCond_Signal(&(opCtxt->helper->cond)); // This is for pxd_open
}


