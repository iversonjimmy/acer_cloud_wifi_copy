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

#include "PrxP2PListener.hpp"

#include <gvm_errors.h>
#include <gvm_thread_utils.h>
#include <log.h>

#include <scopeguard.hpp>
#include <vpl_string.h>
#include <vpl_time.h>
#include <vplex_socket.h>
#include <vplu_format.h>
#include <vplu_mutex_autolock.hpp>

#include <cassert>
#include <sstream>
#include <string>

const size_t Ts2::Network::PrxP2PListener::ThreadStackSize = UTIL_DEFAULT_THREAD_STACK_SIZE;

Ts2::Network::PrxP2PListener::PrxP2PListener(Link::AcceptedSocketHandler frontEndIncomingSocket, void *frontEndIncomingSocket_context,
                                             LocalInfo *localInfo)
    : frontEndIncomingSocket(frontEndIncomingSocket), frontEndIncomingSocket_context(frontEndIncomingSocket_context),
      localInfo(localInfo),
      needToInitPxd(true),
      pxdClient(NULL),
      openCtxt(this)  // C4355 warning
{
    assert(frontEndIncomingSocket);
    assert(localInfo);

    VPLMutex_Init(&mutex);
    VPLCond_Init(&cond);

    LOG_INFO("PrxP2PListener[%p]: Created", this);
}

Ts2::Network::PrxP2PListener::~PrxP2PListener()
{
    if (pxdClient) {
        LOG_WARN("PrxP2PListener[%p]: pxdClient still open", this);
        closePxdClient();
    }

    // Clear it
    localInfo->SetAnsNotifyCB(NULL, NULL);
    localInfo->SetAnsDeviceOnlineCB(NULL, NULL);

    VPLMutex_Destroy(&mutex);
    VPLCond_Destroy(&cond);

    LOG_INFO("PrxP2PListener[%p]: Destroyed", this);
}

int Ts2::Network::PrxP2PListener::Start()
{
    int err = 0;

    {
        MutexAutoLock lock(&mutex);
        if (state.stop != STOP_STATE_NOSTOP) {
            LOG_ERROR("PrxP2PListener[%p]: Wrong stop state %d", this, state.stop);
            err = TS_ERR_WRONG_STATE;
            goto end;
        }

        if (state.pxdState != PXD_STATE_NOLISTEN) {
            LOG_ERROR("PrxP2PListener[%p]: Wrong pxd state %d", this, state.pxdState);
            err = TS_ERR_WRONG_STATE;
            goto end;
        }

        if (state.thread != THREAD_STATE_NOTHREAD) {
            LOG_ERROR("PrxP2PListener[%p]: Wrong thread state %d", this, state.thread);
            err = TS_ERR_WRONG_STATE;
            goto end;
        }
        state.thread = THREAD_STATE_SPAWNING;

    }

    localInfo->SetAnsDeviceOnlineCB(this, ansDeviceOnlineCB);

    {
        MutexAutoLock lock(&mutex);
        err = Util_SpawnThread(threadMain, this, ThreadStackSize, /*isJoinable*/VPL_TRUE, &thread);
        if (err) {
            LOG_ERROR("PrxP2PListener[%p]: Failed to spawn thread: err %d", this, err);
            state.thread = THREAD_STATE_NOTHREAD;
            goto end;
        }
    }

    localInfo->SetAnsNotifyCB(ansNotifyCB, this);

 end:
    return err;
}

int Ts2::Network::PrxP2PListener::Stop()
{
    int err = 0;

    MutexAutoLock lock(&mutex);
    if (state.stop != STOP_STATE_NOSTOP) {
        LOG_ERROR("PrxP2PListener[%p]: Wrong stop state %d", this, state.stop);
        err = TS_ERR_WRONG_STATE;
        goto end;
    }
    state.stop = STOP_STATE_STOPPING;

    err = closePxdClient();
    if (err) {
        // ErrMsg logged by closePxdClient().
        goto end;
    }

 end:
    VPLCond_Signal(&cond);
    return err;
}

int Ts2::Network::PrxP2PListener::WaitStopped()
{
    int err = 0;

    {
        MutexAutoLock lock(&mutex);
        if (state.stop != STOP_STATE_STOPPING) {
            LOG_ERROR("PrxP2PListener[%p]: Wrong stop state %d", this, state.stop);
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
            LOG_ERROR("PrxP2PListener[%p]: Failed to join thread: err %d", this, err);
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
                LOG_ERROR("PrxP2PListener[%p]: WaitDone failed on AcceptHelper[%p]: err %d", this, ah, err);
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
                LOG_ERROR("PrxP2PListener[%p]: WaitDone failed on AcceptHelper[%p]: err %d", this, ah, err);
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

VPLTHREAD_FN_DECL Ts2::Network::PrxP2PListener::threadMain(void *param)
{
    PrxP2PListener *agent = static_cast<PrxP2PListener*>(param);
    agent->threadMain();
    return VPLTHREAD_RETURN_VALUE;
}

void Ts2::Network::PrxP2PListener::threadMain()
{
    {
        MutexAutoLock lock(&mutex);
        if (state.thread != THREAD_STATE_SPAWNING) {
            LOG_WARN("PrxP2PListener[%p]: Unexpected thread state %d", this, state.thread);
        }
        state.thread = THREAD_STATE_RUNNING;
        LOG_INFO("PrxP2PListener[%p]: Thread running", this);
    }

    int err = 0;

    while (1) {
        ConnRequest* connReq = NULL;
        bool _needToInitPxd = false;
        {
            MutexAutoLock lock(&mutex);
            while ((state.stop == STOP_STATE_NOSTOP) && connReqQueue.empty() && doneAcceptHelperQ.empty() && !needToInitPxd) {
                // Wait for ANS notification
                err = VPLCond_TimedWait(&cond, &mutex, VPL_TIMEOUT_NONE);
                if (err) {
                    LOG_ERROR("PrxP2PListener[%p]: CondVar failed: err %d", this, err);
                    goto end;
                }
            }

            if (state.stop != STOP_STATE_NOSTOP) {  // shutdown in progress
                goto end;
            }

            if (needToInitPxd) {
                _needToInitPxd = true;
                needToInitPxd = false;
            }

            // Cleanup AccpetHelper
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
                    LOG_INFO("PrxP2PListener[%p]: Delete one PrxP2PAcceptHelper[%p]", this, ah);
                    delete ah;
                }
            }

            if (!connReqQueue.empty()) {
                // If we get here, it is because something in queue
                connReq = connReqQueue.front();
                connReqQueue.pop();
            }

            // Test if we have works to do
            if(!connReq && !_needToInitPxd) {
                continue;
            }
        }

        if (_needToInitPxd) {
            err = openPxdClient();
            if (err) {
                // ErrMsg logged by openPxdClient().
                // This could fail because of network is not ready
                // In this case, we still want "Listen" to it
                LOG_WARN("PrxP2PListener[%p]: Open pxd client failed %d", this, err);
                err = VPL_OK;
            }
        }

        if (connReq) {
            LOG_INFO("PrxP2PListener[%p]: Get connection request from client", this);
            Link::PrxP2PAcceptHelper *prxah = new (std::nothrow) Link::PrxP2PAcceptHelper(acceptedSocketHandler, this,
                                                                                          pxdClient,
                                                                                          connReq->buffer,
                                                                                          connReq->bufferLength, localInfo);
            if (!prxah) {
                LOG_ERROR("PrxP2PListener[%p]: No memory to create PrxP2PListener obj", this);
                continue;
            }

            err = prxah->Accept();
            if (err) {
                // ErrMsg logged by Accept().
            } else {
                // Put AcceptHelper into tracking set
                MutexAutoLock lock(&mutex);
                activeAcceptHelperSet.insert(prxah);
            }

            delete connReq;
        }
    }

 end:
    {
        MutexAutoLock lock(&mutex);
        while(!connReqQueue.empty()) {
            delete connReqQueue.front();
            connReqQueue.pop();
        }
        state.thread = THREAD_STATE_EXITING;
        LOG_INFO("PrxP2PListener[%p]: Thread exiting", this);
    }
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

int Ts2::Network::PrxP2PListener::openPxdClient()
{
    int err = 0;
    pxd_callback_t pxdCallbacks =
        {
            supplyLocal,
            supplyExternal,
            Ts2::Link::PrxP2PAcceptHelper::ConnectDone,
            Ts2::Link::PrxP2PAcceptHelper::LookupDone,
            Ts2::Link::PrxP2PAcceptHelper::IncomingRequest,
            Ts2::Link::PrxP2PAcceptHelper::IncomingLogin,
            Ts2::Link::PrxP2PAcceptHelper::RejectCcdCreds,
            rejectPxdCreds
        };

    pxd_open_t pxdOpen;
    pxd_cred_t pxdCred;
    pxd_error_t pxdErr;
    pxd_id_t localId;

    char region[] = ""; // Dummy
    char localInstanceIdStr[16];  // large enough for u32 in decimal
    VPL_snprintf(localInstanceIdStr, ARRAY_SIZE_IN_BYTES(localInstanceIdStr), FMTu32, localInfo->GetInstanceId());
    std::string pxdSessionKey;
    std::string pxdLoginBlob;

    // Prepare local id
    localId.device_id = localInfo->GetDeviceId();
    localId.user_id = localInfo->GetUserId();
    localId.instance_id = localInstanceIdStr;
    localId.region = region;

    {
        MutexAutoLock lock(&mutex);
        if (state.pxdState != PXD_STATE_NOLISTEN) {
            // We probably got here because of ans login even,
            // We don't want to change state or do anything if it is already initialized or it is initializing , just return it
            return 0;
        }
        state.pxdState = PXD_STATE_INITIALIZING;
    }

    err = localInfo->GetPxdSessionKey(pxdSessionKey, pxdLoginBlob);
    if(err != 0) {
        LOG_ERROR("PrxP2PListener[%p]: GetPxdSessionKey failed: err %d",
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
    pxdOpen.is_incoming = true; // This is a server
    pxdOpen.opaque = &openCtxt;
    pxdOpen.credentials = &pxdCred;
    pxdOpen.callback = &pxdCallbacks;

    {
        pxd_client_t *_pxdClient = pxd_open(&pxdOpen, &pxdErr);
        MutexAutoLock lock(&mutex);
        pxdClient = _pxdClient;
    }
    if(pxdClient == NULL ||
       pxdErr.error != 0) {
        LOG_ERROR("PrxP2PListener[%p]: pxd_open failed: err %d, Msg: %s",
                  this, pxdErr.error, pxdErr.message);
        err = pxdErr.error;
        goto end;
    }

 end:
    if (err) {
        if (pxdClient) {
            closePxdClient();
        }
        MutexAutoLock lock(&mutex);
        if (state.pxdState != PXD_STATE_NOLISTEN) {
            state.pxdState = PXD_STATE_NOLISTEN;
        }
    }
    else {
        MutexAutoLock lock(&mutex);
        if (state.pxdState != PXD_STATE_INITIALIZING) {
            LOG_WARN("PrxP2PListener[%p]: Unexpected pxd state %d", this, state.pxdState);
        }
        state.pxdState = PXD_STATE_LISTENING;
    }
    return err;
}

int Ts2::Network::PrxP2PListener::closePxdClient()
{
    int err = 0;

    MutexAutoLock lock(&mutex);

    if (pxdClient) {
        pxd_error_t pxdErr;
        pxd_close(&pxdClient, 0, &pxdErr);
        if(pxdErr.error != 0) {
            LOG_ERROR("PrxP2PListener[%p]: pxd_close failed: err %d, Msg: %s",
                              this, pxdErr.error, pxdErr.message);
            err = pxdErr.error;
        }

        pxdClient = NULL;
        state.pxdState = PXD_STATE_NOLISTEN;
    }

    return err;
}

void Ts2::Network::PrxP2PListener::rejectPxdCreds(pxd_cb_data_t *cb_data)
{
    static bool retryOnce = false;
    PclListenerCtxt *opCtxt = (PclListenerCtxt *)cb_data->client_opaque;
    LOG_ERROR("PrxP2PListener[%p]: rejectPxdCreds: "FMTu64, opCtxt->helper, cb_data->result);
    opCtxt->helper->localInfo->ResetPxdSessionKey();
    // Close pxd client to avoid infinite retry
    opCtxt->helper->closePxdClient();

    {
        MutexAutoLock lock(&opCtxt->helper->mutex);
        if (opCtxt->helper->state.stop != STOP_STATE_STOPPING &&
            opCtxt->helper->state.thread == THREAD_STATE_RUNNING &&
            !retryOnce) {
            retryOnce = true;
            opCtxt->helper->needToInitPxd = true;
            VPLCond_Signal(&opCtxt->helper->cond);
            LOG_INFO("PrxP2PListener[%p]: RejectPxdCreds, retry init", opCtxt->helper);
        }
    }
}

void Ts2::Network::PrxP2PListener::ansIncomingClient(const char* buffer, u32 bufferLength)
{
    MutexAutoLock lock(&mutex);
    if(state.thread != THREAD_STATE_RUNNING) {
        LOG_ERROR("PrxP2PListener[%p]: Wrong thread state %d", this, state.thread);
        return;
    }
    ConnRequest* connReq = new (std::nothrow) ConnRequest(buffer, bufferLength);
    if(connReq == NULL) {
        LOG_ERROR("PrxP2PListener[%p]: Not enough memory", this);
        return;
    }

    LOG_INFO("PrxP2PListener[%p]: Enqueue connection request", this);
    connReqQueue.push(connReq);
    VPLCond_Signal(&cond);
}

// class method
void Ts2::Network::PrxP2PListener::ansNotifyCB(const char* buffer, u32 bufferLength, void* context)
{
    Ts2::Network::PrxP2PListener* listener = (Ts2::Network::PrxP2PListener*)context;
    listener->ansIncomingClient(buffer, bufferLength);
}

void Ts2::Network::PrxP2PListener::ansDeviceOnline(u64 deviceId)
{
    MutexAutoLock lock(&mutex);
    if (state.stop != STOP_STATE_STOPPING &&
        state.thread == THREAD_STATE_RUNNING &&
        (deviceId == localInfo->GetDeviceId())) {
        needToInitPxd = true;
        VPLCond_Signal(&cond);
        LOG_INFO("PrxP2PListener[%p]: Ans device online, retry init", this);
    }
}

// class method
void Ts2::Network::PrxP2PListener::ansDeviceOnlineCB(void* instance, u64 deviceId)
{
    Ts2::Network::PrxP2PListener* listener = (Ts2::Network::PrxP2PListener*)instance;
    listener->ansDeviceOnline(deviceId);
}

// class method
void Ts2::Network::PrxP2PListener::acceptedSocketHandler(VPLSocket_t socket, Link::RouteType routeType,
                                                         u64 remoteUserId, u64 remoteDeviceId, u32 remoteInstanceId,
                                                         const std::string *sessionKey,
                                                         Ts2::Link::AcceptHelper *acceptHelper, void *context)
{
    PrxP2PListener *helper = static_cast<PrxP2PListener*>(context);
    helper->acceptedSocketHandler(socket, routeType, remoteUserId, remoteDeviceId, remoteInstanceId, sessionKey, acceptHelper);
}

void Ts2::Network::PrxP2PListener::acceptedSocketHandler(VPLSocket_t socket, Link::RouteType routeType,
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
            VPLCond_Signal(&cond);
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
