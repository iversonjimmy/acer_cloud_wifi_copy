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

#include "Ts2_RouteManager.hpp"
#include "Packet.hpp"

#include <gvm_errors.h>
#include <gvm_thread_utils.h>
#include <log.h>

#include <scopeguard.hpp>
#include <vpl_time.h>
#include <vplu_format.h>
#include <vplu_mutex_autolock.hpp>

#include <EndPoint.hpp>

#include <cassert>
#include <cmath>

const size_t Ts2::Network::RouteManager::ThreadStackSize = UTIL_DEFAULT_THREAD_STACK_SIZE;
const VPLTime_t Ts2::Network::RouteManager::ValidateEndPointMaxInterval = VPLTime_FromSec(10);
const VPLTime_t Ts2::Network::RouteManager::ConnectionEstablishTimeout = VPLTime_FromSec(10);

namespace Ts2 {
namespace Network {
void SafeDestroyEndPoint(Link::EndPoint *ep);
void SafeDestroyEndPoint(Link::EndPoint *ep)
{
    int err = ep->Stop();
    if (err) {
        LOG_ERROR("Stop failed on EndPoint[%p]: err %d", ep, err);
        // We could not tell this obj to stop.
        // Since we don't know if it is safe to destroy it, we will let it leak.
        return;
    }

    err = ep->WaitStopped();
    if (err) {
        LOG_ERROR("WaitStopped failed on EndPoint[%p]: err %d", ep, err);
        // We could not confirm if this obj stopped.
        // Since we don't know if it is safe to destroy it, we will let it leak.
        return;
    }

    delete ep;
}
} // end namespace Network
} // end namespace Ts2

Ts2::Network::RouteManager::RouteManager(u64 remoteUserId, u64 remoteDeviceId, u32 remoteInstanceId,
                                         PacketHandler rcvdPacketHandler, void *rcvdPacketHandler_context,
                                         LocalInfo *localInfo, bool isServer)
    : remoteUserId(remoteUserId), remoteDeviceId(remoteDeviceId), remoteInstanceId(remoteInstanceId),
      rcvdPacketHandler(rcvdPacketHandler), rcvdPacketHandler_context(rcvdPacketHandler_context),
      localInfo(localInfo), isServer(isServer),
      isNetworkUpdated(false),
      forceMakeConn(false),
      endPointStatusChangePending(false),
      unexpectedLossOfEndPoint(false),
      retryOnce(false),
      resetRetryTimeout(false),
      packetThreadSending(false)
{
    assert(rcvdPacketHandler);
    assert(localInfo);

    VPLMutex_Init(&mutex);
    VPLCond_Init(&cond_state_noClients);

    VPLCond_Init(&cond_packetThreadHasWork);

    VPLCond_Init(&cond_connThreadHasWork);

    VPLMutex_Init(&mutex_endPoints);
    VPLCond_Init(&cond_someEndPointReady);

    VPLMutex_Init(&mutex_packetThreadSending);
    VPLCond_Init(&cond_packetThreadSending);

    LOG_INFO("RouteManager[%p]: Created for UDI<"FMTu64","FMTu64","FMTu32">",
             this, remoteUserId, remoteDeviceId, remoteInstanceId);
}

Ts2::Network::RouteManager::~RouteManager()
{
    if (!packetQueue.empty()) {
        LOG_INFO("RouteManager[%p]: Non-empty PacketQueue", this);
        while (!packetQueue.empty()) {
            std::pair<Packet*, VPLTime_t> entry = packetQueue.front();
            packetQueue.pop_front();
            delete entry.first;
        }
    }

    if (endPointMap.size() > 0) {
        LOG_WARN("RouteManager[%p]: "FMTu_size_t" EndPoint objs detected", this, endPointMap.size());
    }
    {
        int numConnHelpers = activeConnHelperSet.size() + doneConnHelperQ.size();
        if (numConnHelpers > 0) {
            LOG_WARN("RouteManager[%p]: %d ConnHelper objs detected", this, numConnHelpers);
        }
    }

    VPLCond_Destroy(&cond_packetThreadSending);
    VPLMutex_Destroy(&mutex_packetThreadSending);

    VPLCond_Destroy(&cond_someEndPointReady);
    VPLMutex_Destroy(&mutex_endPoints);

    VPLCond_Destroy(&cond_packetThreadHasWork);

    VPLCond_Destroy(&cond_connThreadHasWork);

    VPLCond_Destroy(&cond_state_noClients);
    VPLMutex_Destroy(&mutex);

    LOG_INFO("RouteManager[%p]: Destroyed", this);
}

u64 Ts2::Network::RouteManager::GetRemoteUserId() const
{
    MutexAutoLock lock(&mutex);
    if (state.stop == STOP_STATE_STOPPING) {
        LOG_WARN("RouteManager[%p]: Method called after Stop", this);
    }
    return remoteUserId;
}

u64 Ts2::Network::RouteManager::GetRemoteDeviceId() const
{
    MutexAutoLock lock(&mutex);
    if (state.stop == STOP_STATE_STOPPING) {
        LOG_WARN("RouteManager[%p]: Method called after Stop", this);
    }
    return remoteDeviceId;
}

u32 Ts2::Network::RouteManager::GetRemoteInstanceId() const
{
    MutexAutoLock lock(&mutex);
    if (state.stop == STOP_STATE_STOPPING) {
        LOG_WARN("RouteManager[%p]: Method called after Stop", this);
    }
    return remoteInstanceId;
}

int Ts2::Network::RouteManager::Start()
{
    int err = 0;

    {
        MutexAutoLock lock(&mutex);
        if (state.stop != STOP_STATE_NOSTOP) {
            LOG_ERROR("RouteManager[%p]: Wrong stop state %d", this, state.stop);
            err = TS_ERR_WRONG_STATE;
            goto end;
        }
        if (state.packetThread != THREAD_STATE_NOTHREAD) {
            LOG_ERROR("RouteManager[%p]: Wrong packet thread state %d", this, state.packetThread);
            err = TS_ERR_WRONG_STATE;
            goto end;
        }

        if (state.connThread != THREAD_STATE_NOTHREAD) {
            LOG_ERROR("RouteManager[%p]: Wrong connection thread state %d", this, state.connThread);
            err = TS_ERR_WRONG_STATE;
            goto end;
        }

        state.packetThread = THREAD_STATE_SPAWNING;
        err = Util_SpawnThread(packetThreadMain, this, ThreadStackSize, /*isJoinable*/VPL_TRUE, &packetThread);
        if (err) {
            LOG_ERROR("RouteManager[%p]: Failed to spawn packet thread: err %d", this, err);
            state.packetThread = THREAD_STATE_NOTHREAD;
        }

        state.connThread = THREAD_STATE_SPAWNING;
        err = Util_SpawnThread(connThreadMain, this, ThreadStackSize, /*isJoinable*/VPL_TRUE, &connThread);
        if (err) {
            LOG_ERROR("RouteManager[%p]: Failed to spawn connection thread: err %d", this, err);
            state.connThread = THREAD_STATE_NOTHREAD;
        }
    }

 end:
    return err;
}

int Ts2::Network::RouteManager::Stop()
{
    int err = 0;

    LOG_INFO("RouteManager[%p]: Stop request received", this);

    {
        MutexAutoLock lock(&mutex);
        if (state.stop == STOP_STATE_STOPPING) {
            LOG_WARN("RouteManager[%p]: Stop state is already %d", this, state.stop);
            return 0;
        }
        state.stop = STOP_STATE_STOPPING;
    }

    {
        MutexAutoLock lock(&mutex_endPoints);
        std::map<Link::RouteType, Link::EndPoint*>::iterator it;
        for (it = endPointMap.begin(); it != endPointMap.end(); it++) {
            Link::EndPoint *ep = it->second;
            err = ep->Stop();
            if (err) {
                LOG_ERROR("RouteManager[%p]: Stop failed on EndPoint[%p]: err %d", this, ep, err);
                err = 0;  // Reset error and continue.
            }
        }
        // Unblock any wait status
        VPLCond_Broadcast(&cond_someEndPointReady);
    }

    {
        // Wake up the packet thread.
        MutexAutoLock lock(&mutex);
        VPLCond_Signal(&cond_packetThreadHasWork);
    }

    {
        // Wake up the connection thread.
        MutexAutoLock lock(&mutex);
        VPLCond_Signal(&cond_connThreadHasWork);
    }

    LOG_INFO("RouteManager[%p]: Stop request propagated: err %d", this, err);

    return err;
}

int Ts2::Network::RouteManager::WaitStopped()
{
    int err = 0;
    std::list<Link::ConnectHelper*> deleteList;

    LOG_INFO("RouteManager[%p]: Waiting for obj to Stop", this);

    {
        MutexAutoLock lock(&mutex);
        if (state.stop != STOP_STATE_STOPPING) {
            LOG_ERROR("RouteManager[%p]: Wrong stop state %d", this, state.stop);
            err = TS_ERR_WRONG_STATE;
            goto end;
        }

        // Wait for all external threads to leave the object.
        while (state.clients > 0) {
            LOG_INFO("RouteManager[%p]: Waiting for %d client threads to leave", this, state.clients);
            err = VPLCond_TimedWait(&cond_state_noClients, &mutex, VPL_TIMEOUT_NONE);
            if (err) {
                LOG_ERROR("RouteManager[%p]: CondVar failed: err %d", this, err);
                err = 0;  // Reset error and continue.
                break;
            }
        }
    }

    // Join threads first, to prevent any race
    {
        VPLMutex_Lock(&mutex);
        if (state.packetThread != THREAD_STATE_NOTHREAD) {
            VPLMutex_Unlock(&mutex);
            err = VPLDetachableThread_Join(&packetThread);
            VPLMutex_Lock(&mutex);
            if (err) {
                LOG_ERROR("RouteManager[%p]: Failed to join packet thread: err %d", this, err);
                // Do not reset error state.
                // We don't know the state of the thread state, so it is not safe to destroy this object.
            }
            else {
                state.packetThread = THREAD_STATE_NOTHREAD;
            }
        }
        VPLMutex_Unlock(&mutex);
    }

    {
        VPLMutex_Lock(&mutex);
        if (state.connThread != THREAD_STATE_NOTHREAD) {
            VPLMutex_Unlock(&mutex);
            err = VPLDetachableThread_Join(&connThread);
            VPLMutex_Lock(&mutex);
            if (err) {
                LOG_ERROR("RouteManager[%p]: Failed to join connection thread: err %d", this, err);
                // Do not reset error state.
                // We don't know the state of the thread state, so it is not safe to destroy this object.
            }
            else {
                state.connThread = THREAD_STATE_NOTHREAD;
            }
        }
        VPLMutex_Unlock(&mutex);
    }

    // Send WaitDone on all known ConnectHelper objects and destroy them.
    {
        VPLMutex_Lock(&mutex_endPoints);
        std::set<Link::ConnectHelper*>::iterator it;
        while ((it = activeConnHelperSet.begin()) != activeConnHelperSet.end()) {
            Link::ConnectHelper *ch = *it;
            activeConnHelperSet.erase(it);
            VPLMutex_Unlock(&mutex_endPoints);
            // Force close any outgoing socket attempts
            ch->ForceClose();
            err = ch->WaitDone();
            VPLMutex_Lock(&mutex_endPoints);
            if (err) {
                LOG_ERROR("RouteManager[%p]: WaitDone failed on ConnectHelper[%p]: err %d", this, ch, err);
                err = 0;  // Reset error and continue;
                // We don't know the state of the object, so we won't destroy it and let it leak.
            }
            else {
                deleteList.push_back(ch);
            }
        }

        while (doneConnHelperQ.size() > 0) {
            Link::ConnectHelper *ch = doneConnHelperQ.front();
            doneConnHelperQ.pop();
            VPLMutex_Unlock(&mutex_endPoints);
            err = ch->WaitDone();
            VPLMutex_Lock(&mutex_endPoints);
            if (err) {
                LOG_ERROR("RouteManager[%p]: WaitDone failed on ConnectHelper[%p]: err %d", this, ch, err);
                err = 0;  // Reset error and continue;
                // We don't know the state of the object, so we won't destroy it and let it leak.
            }
            else {
                deleteList.push_back(ch);
            }
        }
        VPLMutex_Unlock(&mutex_endPoints);
    }

    deleteList.sort();
    deleteList.unique();
    for(std::list<Link::ConnectHelper*>::iterator i = deleteList.begin();
            i != deleteList.end(); i++) {
        delete *i;
    }

    // Send WaitStopped for all EndPoint objects and destroy them.
    {
        VPLMutex_Lock(&mutex_endPoints);
        while (endPointMap.size() > 0) {

            Link::EndPoint *ep = endPointMap.begin()->second;
            endPointMap.erase(endPointMap.begin());

            VPLMutex_Unlock(&mutex_endPoints);
            err = ep->WaitStopped();
            VPLMutex_Lock(&mutex_endPoints);

            if (err) {
                LOG_ERROR("RouteManager[%p]: WaitStopped failed on EndPoint[%p]: err %d", this, ep, err);
                err = 0;  // Reset error and continue;
                // We don't know the state of the object, so we won't destroy it and let it leak.
            }
            else {
                delete ep;
            }
        }
        VPLMutex_Unlock(&mutex_endPoints);
    }

 end:
    LOG_INFO("RouteManager[%p]: Obj has Stopped: err %d", this, err);

    return err;
}

int Ts2::Network::RouteManager::Enqueue(Packet *packet, VPLTime_t timeout)
{
    int err = 0;

    {
        MutexAutoLock lock(&mutex);
        state.clients++;
        if (state.stop == STOP_STATE_STOPPING) {
            LOG_WARN("RouteManager[%p]: Method called after Stop", this);
        }
        if (state.packetThread != THREAD_STATE_SPAWNING && state.packetThread != THREAD_STATE_RUNNING) {
            LOG_ERROR("RouteManager[%p]: Wrong thread state %d", this, state.packetThread);
            err = TS_ERR_WRONG_STATE;
            goto end;
        }
    }
#if 0   // Temporarily disable the logic at Network layer for testing
    // Check that the remote device is authorized to connect with the local device.
    {
        // Because of overhead of testing, we will only test once every minute.
        static VPLTime_t clientLastAuthTimeStamp = 0;
        bool expired = clientLastAuthTimeStamp + VPLTime_FromSec(60) < VPLTime_GetTimeStamp();
        clientLastAuthTimeStamp = VPLTime_GetTimeStamp();
        if (expired) {
            LOG_DEBUG("RouteManager[%p]: Checking client's authorization", this);
            if (!localInfo->IsAuthorizedClient(remoteDeviceId)) {
                LOG_ERROR("RouteManager[%p]: Device "FMTu64" is not authorized to connect.", this, remoteDeviceId);
                err = TS_ERR_NO_AUTH;
                goto end;
            }
        }
    }
#endif
    {
        MutexAutoLock lock(&mutex);
        packetQueue.push_back(std::make_pair(packet, VPLTime_GetTimeStamp() + timeout));
        LOG_DEBUG("RouteManager[%p]: Enqueued Packet[%p]", this, packet);
        VPLCond_Signal(&cond_packetThreadHasWork);
    }

 end:
    {
        MutexAutoLock lock(&mutex);
        state.clients--;
        if (state.clients == 0) {
            VPLCond_Broadcast(&cond_state_noClients);
        }
    }

    return err;
}

int Ts2::Network::RouteManager::CancelDataPackets(u32 srcVtId)
{
    int err = 0;

    {
        MutexAutoLock lock(&mutex);
        state.clients++;
        if (state.stop == STOP_STATE_STOPPING) {
            LOG_WARN("RouteManager[%p]: Method called after Stop", this);
        }

        // Removing an element from the middle of a queue maybe expensive.
        // Thus, instead of removing it, we will simply replace the pointer to a Packet obj with a NULL pointer.
        // Dequeue code (in packetThreadMain()) will check the pointer value and ignore if NULL.
        std::deque<std::pair<Packet*, VPLTime_t> >::iterator it;
        for (it = packetQueue.begin(); it != packetQueue.end(); it++) {
            Packet *packet = it->first;
            if (packet && (packet->GetPktType() == TS_PKT_DATA) && (packet->GetSrcVtId() == srcVtId)) {
                delete packet;     // destroy Packet obj
                it->first = NULL;  // replace pointer with NULL pointer
            }
        }
    }

    {
        // If the worker thread is trying to send, wait until it is done.
        // This is necessary, since it may be currently processing a packet that
        // would have been cancelled by the loop above.
        MutexAutoLock lock(&mutex_packetThreadSending);
        while (packetThreadSending) {
            err = VPLCond_TimedWait(&cond_packetThreadSending, &mutex_packetThreadSending, VPL_TIMEOUT_NONE);
            if (err) {
                LOG_ERROR("RouteManager[%p]: CondVar failed: err %d", this, err);
                err = 0;  // Reset error and exit.
                break;
            }
        }
    }

    {
        MutexAutoLock lock(&mutex);
        state.clients--;
        if (state.clients == 0) {
            VPLCond_Broadcast(&cond_state_noClients);
        }
    }

    return err;
}

int Ts2::Network::RouteManager::AddConnectedSocket(VPLSocket_t socket, Link::RouteType routeType, const std::string *sessionKey, bool isServer)
{
    int err = 0;

    if (routeType != Link::ROUTE_TYPE_DIN && routeType != Link::ROUTE_TYPE_DEX &&
        routeType != Link::ROUTE_TYPE_P2P && routeType != Link::ROUTE_TYPE_PRX) {
        LOG_ERROR("RouteManager[%p]: Ignoring socket with invalid RouteType %d", this, routeType);
        return TS_ERR_INVALID;
    }

    if (routeType == Link::ROUTE_TYPE_PRX) {
        // pxd client library does not seem to set TCP NODELAY option on the PRX socket, so do it here.
        int yes = 1;
        err = VPLSocket_SetSockOpt(socket, VPLSOCKET_IPPROTO_TCP, VPLSOCKET_TCP_NODELAY, &yes, sizeof(yes));
        if (err) {
            LOG_WARN("RouteManager[%p]: Failed to set TCP no-delay on Socket["FMT_VPLSocket_t"]", this, VAL_VPLSocket_t(socket));
            err = 0;  // reset error
        }
    }
    {
        int yes = 0;
        err = VPLSocket_GetSockOpt(socket, VPLSOCKET_IPPROTO_TCP, VPLSOCKET_TCP_NODELAY, &yes, sizeof(yes));
        if (err) {
            LOG_WARN("RouteManager[%p]: Failed to get TCP no-delay state of Socket["FMT_VPLSocket_t"]", this, VAL_VPLSocket_t(socket));
            err = 0;  // reset error
        }
        else {
            LOG_INFO("RouteManager[%p]: Socket["FMT_VPLSocket_t"] TCP no-delay state %d", this, VAL_VPLSocket_t(socket), yes);
        }
    }

    // set/get send buffer size
    {
        int sendBufSize = localInfo->GetSendBufSize();
        if (sendBufSize > 0) {  // change send buffer size to a non-default value
            err = VPLSocket_SetSockOpt(socket, VPLSOCKET_SOL_SOCKET, VPLSOCKET_SO_SNDBUF, &sendBufSize, sizeof(sendBufSize));
            if (err) {
                LOG_WARN("RouteManager[%p]: Failed to set send buffer size to %d on Socket["FMT_VPLSocket_t"]", this, sendBufSize, VAL_VPLSocket_t(socket));
                err = 0;  // reset error
            }
        }

        sendBufSize = 0;
        err = VPLSocket_GetSockOpt(socket, VPLSOCKET_SOL_SOCKET, VPLSOCKET_SO_SNDBUF, &sendBufSize, sizeof(sendBufSize));
        if (err) {
            LOG_WARN("RouteManager[%p]: Failed to get send buffer size of Socket["FMT_VPLSocket_t"]", this, VAL_VPLSocket_t(socket));
            err = 0;  // reset error
        }
        else {
            LOG_INFO("RouteManager[%p]: Send buffer size of Socket["FMT_VPLSocket_t"] is %d", this, VAL_VPLSocket_t(socket), sendBufSize);
        }
    }

    Link::EndPoint *newEndPoint = NULL;
    Link::EndPoint *oldEndPoint = NULL;
    newEndPoint = new (std::nothrow) Link::EndPoint(socket, remoteUserId, remoteDeviceId, remoteInstanceId,
                                                 sessionKey, isServer, rcvdPacketHandler, rcvdPacketHandler_context,
                                                 notifyEndPointStatusChange, this,
                                                 routeType, localInfo);
    if (!newEndPoint) {
        LOG_ERROR("RouteManager[%p]: No memory to create EndPoint obj", this);
        err = TS_ERR_NO_MEM;
        goto end;
    }

    err = newEndPoint->Start();
    if (err) {
        LOG_ERROR("RouteManager[%p]: Start failed on EndPoint[%p]: err %d", this, newEndPoint, err);
        goto end;
    }
    {
        MutexAutoLock lock(&mutex_endPoints);
        std::map<Link::RouteType, Link::EndPoint*>::iterator it = endPointMap.find(routeType);
        if (it != endPointMap.end()) {  // slot is occupied
            // We always remove the old one, and use the latest route,
            // Since this might mean the peer on the other side is aware of a connection is bad and establish a new one
            LOG_WARN("RouteManager[%p]: Already has EndPoint[%p] for RouteType %d, we are going to close old one", this, it->second, routeType);
            oldEndPoint = it->second;
            endPointMap.erase(it);
        }

        if(unexpectedLossOfEndPoint) {
            unexpectedLossOfEndPoint = false;
        }

        endPointMap[routeType] = newEndPoint;
        LOG_INFO("RouteManager[%p]: Added Socket["FMT_VPLSocket_t"] as EndPoint[%p] for RouteType %d", this, VAL_VPLSocket_t(socket), newEndPoint, routeType);
        newEndPoint = NULL;  // Ownership of EndPoint object transferred to the endPointMap object.

        if(retryCheaperRoute()) {
            resetRetryTimeout = true;
        }

        VPLCond_Broadcast(&cond_someEndPointReady);
    }  // MutexAutoLock block

    if(!oldEndPoint) {
        notifyEndPointStatusChange(NULL/*For now, we won't use "ep"*/);
    } else {
        SafeDestroyEndPoint(oldEndPoint);
    }

    {
        // Wake up packet thread to check if we have some packets
        MutexAutoLock lock(&mutex);
        VPLCond_Broadcast(&cond_packetThreadHasWork);
    }

 end:
    if (newEndPoint) {
        SafeDestroyEndPoint(newEndPoint);
    }

    return err;
}

// class method
void Ts2::Network::RouteManager::networkConnCB(void* instance)
{
    Ts2::Network::RouteManager* rm = (Ts2::Network::RouteManager*)instance;
    rm->reportNetworkConnected();
}

void Ts2::Network::RouteManager::reportNetworkConnected()
{
    LOG_INFO("RouteManager[%p]: Receive network connected events", this);
    MutexAutoLock lock(&mutex);
    isNetworkUpdated = true;
    VPLCond_Signal(&cond_connThreadHasWork);
}

VPLTHREAD_FN_DECL Ts2::Network::RouteManager::connThreadMain(void *param)
{
    RouteManager *routeMgr = static_cast<RouteManager*>(param);
    routeMgr->connThreadMain();
    return VPLTHREAD_RETURN_VALUE;
}

void Ts2::Network::RouteManager::connThreadMain()
{
    // Initialization
    VPLTime_t pollingTimeout = VPLTime_FromSec(60);
    VPLTime_t timeout = ValidateEndPointMaxInterval < pollingTimeout ? ValidateEndPointMaxInterval : pollingTimeout;
    VPLTime_t timeLastMakeConn = VPLTime_GetTimeStamp();
    VPLTime_t timeNextMakeConn = timeLastMakeConn + pollingTimeout;
    u32 tryAttempts = 0;

    {
        MutexAutoLock lock(&mutex);
        if (state.connThread != THREAD_STATE_SPAWNING) {
            LOG_ERROR("RouteManager[%p]: Wrong thread state %d", this, state.connThread);
            goto end;
        }
        state.connThread = THREAD_STATE_RUNNING;
    }

    localInfo->RegisterNetworkConnCB(this, networkConnCB);

    // Connection handler loop
    while (1) {
        bool _isNetworkUpdated = false;
        bool _forceMakeConn = false;
        // Wait for network available events from applications
        {
            MutexAutoLock lock(&mutex);
            while (state.stop != STOP_STATE_STOPPING &&
                   !isNetworkUpdated &&
                   !forceMakeConn &&
                   !endPointStatusChangePending) {
                int err = VPLCond_TimedWait(&cond_connThreadHasWork, &mutex, timeout);
                if (err == VPL_ERR_TIMEOUT) {
                    break; // Break the loop to do polling or maintenance
                }
                else if (err) {
                    LOG_ERROR("RouteManager[%p]: CondVar failed: err %d", this, err);
                    goto end;
                }
            }

            if(endPointStatusChangePending) {
                endPointStatusChangePending = false;
            }

            if(isNetworkUpdated) {
                // We had used this "dirty flag", clean it up
                // We don't want to miss any events so we can't modify this after release lock
                _isNetworkUpdated = isNetworkUpdated;
                isNetworkUpdated = false;
            }

            if(forceMakeConn) {
                _forceMakeConn = forceMakeConn;
                forceMakeConn = false;
            }

            if (state.stop == STOP_STATE_STOPPING) {
                LOG_INFO("RouteManager[%p]: Stopping state detected in connection thread", this);
                goto end;
            }
        }

        {
            MutexAutoLock lock(&mutex_endPoints);
            if(resetRetryTimeout) {
                tryAttempts = 0;
                pollingTimeout = VPLTime_FromSec(60);
                LOG_INFO("RouteManager[%p]: Reset retry timeout to "FMTu64" secs", this, VPLTime_ToSec(pollingTimeout));
                resetRetryTimeout = false;
            }
        }

        {
            bool forceDropAll = (localInfo->GetTestNetworkEnv() != 0) && _isNetworkUpdated;

            // Validate routes
            validateEndPoints(forceDropAll);
        }

        // Only on client side, it does purge costly routes.
        if(!isServer) {
            purgeCostlyRoutes();
        }

        VPLMutex_Lock(&mutex_endPoints);
        if (// Server side always waits for client to establish connection.
            !isServer && (
                /// There are many conditions that we need to make new connections
                /// 1) Forced by packet thread
                _forceMakeConn ||
                /// 2) We get network changed events and it's possible that we have cheaper routes
                (_isNetworkUpdated && retryCheaperRoute()) ||

                /// 3) We get network changed events, endPointMap is empty, and there is an unexpected endPoint loss

                ( (_isNetworkUpdated || !retryOnce) && endPointMap.empty() && unexpectedLossOfEndPoint) ||

                /// 4) If a client side doesn't have any network changed, we probably stay on more expensive routes forever
                ///    even server(like Orbe) has a better interface for using.
                ///    In this case, we use polling mechanism on client to reestablish connections
                (retryCheaperRoute() && VPLTime_GetTimeStamp() >= timeNextMakeConn)

            )) {

            LOG_INFO("RouteManager[%p]: Connecting...", this);
            timeLastMakeConn = VPLTime_GetTimeStamp();

            // Exponential back-off
            tryAttempts = tryAttempts + 1 > 20 ? 20 : tryAttempts + 1; // 20 attempts make 62914560 secs..long enough
            pollingTimeout = VPLTime_FromSec((u64)(std::pow(2.0, (double)tryAttempts))*60);
            LOG_INFO("RouteManager[%p]: Set retry timeout to "FMTu64" secs", this, VPLTime_ToSec(pollingTimeout));

            // Connect ...
            // Try DIN first
            tryTcpDinConn();

            bool prxp2pAttempted = false;
            // We need to delay PRX connection attempting
            VPLTime_t establishTimeout = VPLTime_FromSec(localInfo->GetAttemptPrxP2PDelay());

            if(endPointMap.empty() && activeConnHelperSet.size() == 0) {
                // DEBUG:
                // If it hits here, this mean we disable it in debug mode
                tryTcpPrxP2pConns();
                prxp2pAttempted = true;
            }

            // TODO: bug 16484, we need to separate PRX and P2P connection function...
            // Delay PrxP2P retry
            while (endPointMap.empty() &&
                    !prxp2pAttempted) {
                int err = VPLCond_TimedWait(&cond_someEndPointReady, &mutex_endPoints, establishTimeout);
                if (err != VPL_OK && err != VPL_ERR_TIMEOUT) {
                    LOG_ERROR("RouteManager[%p]: CondVar failed: err %d", this, err);
                    VPLMutex_Unlock(&mutex_endPoints);
                    goto end;
                }

                if (state.stop == STOP_STATE_STOPPING) {
                    LOG_INFO("RouteManager[%p]: Stopping state detected", this);
                    VPLMutex_Unlock(&mutex_endPoints);
                    goto end;
                }

                // Try PrxP2P, since we got timeout
                if(endPointMap.empty() &&
                   (err == VPL_ERR_TIMEOUT || activeConnHelperSet.size() == 0/*DIN failed*/) &&
                   !prxp2pAttempted) {
                    tryTcpPrxP2pConns();
                    prxp2pAttempted = true;
                }
            }

            if(!retryOnce) {
                retryOnce = true;
            }
        }

        if(retryCheaperRoute() && !isServer) {
            VPLTime_t timediff = VPLTime_DiffClamp(timeNextMakeConn, VPLTime_GetTimeStamp());
            timeNextMakeConn = timeLastMakeConn + pollingTimeout;
            timeout = ValidateEndPointMaxInterval <  timediff ? ValidateEndPointMaxInterval : timediff;
        } else {
            timeout = ValidateEndPointMaxInterval;
        }

        VPLMutex_Unlock(&mutex_endPoints);

        purgeDoneConnHelpers();
    }

end:
    {
        MutexAutoLock lock(&mutex);
        state.connThread = THREAD_STATE_EXITING;
    }
    localInfo->DeregisterNetworkConnCB(this);
    LOG_INFO("RouteManager[%p]: Connection service thread exiting", this);
}

VPLTHREAD_FN_DECL Ts2::Network::RouteManager::packetThreadMain(void *param)
{
    RouteManager *routeMgr = static_cast<RouteManager*>(param);
    routeMgr->packetThreadMain();
    return VPLTHREAD_RETURN_VALUE;
}

void Ts2::Network::RouteManager::packetThreadMain()
{
    {
        MutexAutoLock lock(&mutex);
        if (state.packetThread != THREAD_STATE_SPAWNING) {
            LOG_ERROR("RouteManager[%p]: Wrong thread state %d", this, state.packetThread);
            goto end;
        }
        state.packetThread = THREAD_STATE_RUNNING;
    }

    while (1) {
        // Get next Packet.
        // Block until there is one.
        bool hasEndPoint = false;
        Packet *packet = NULL;
        {
            MutexAutoLock lock(&mutex);
            while (packetQueue.size() == 0 &&
                   state.stop != STOP_STATE_STOPPING) {
                int err = VPLCond_TimedWait(&cond_packetThreadHasWork, &mutex, VPL_TIMEOUT_NONE);
                if (err) {
                    LOG_ERROR("RouteManager[%p]: CondVar failed: err %d", this, err);
                    goto end;
                }
            }

            if (state.stop == STOP_STATE_STOPPING) {
                LOG_INFO("RouteManager[%p]: Stopping state detected in packet thread", this);
                goto end;
            }

            if (packetQueue.size() > 0) {
                VPLMutex_Unlock(&mutex);
                // Check if any routes available
                // checkAndWaitRoutes doesn't guarantee an EndPoint must be available when processing a packet since we release lock here
                // It gives a suggestion and force connectThread to make a new one if there is no EndPoint
                hasEndPoint = checkAndWaitRoutes();
                VPLMutex_Lock(&mutex);
            }

            // checkAndWaitRoutes might have been waiting for awhile, and we release a lock, we need to check if it is shutdown again
            if (state.stop == STOP_STATE_STOPPING) {
                LOG_INFO("RouteManager[%p]: Stopping state detected in packet thread", this);
                goto end;
            }

            if (packetQueue.size() > 0) {
                LOG_DEBUG("RouteManager[%p]: Found "FMT_size_t" packets in queue", this, packetQueue.size());
                std::pair<Packet*, VPLTime_t> entry = packetQueue.front();

                if (entry.first == NULL) {  // packet got canceled
                    packetQueue.pop_front();
                    continue;
                }

                VPLTime_t now = VPLTime_GetTimeStamp();
                if (now > entry.second) {  // past deadline
                    LOG_INFO("RouteManager[%p]: Dropping Packet[%p] for being stale: age "FMTu64"us", this, entry.first, now - entry.second);
                    delete entry.first;
                    packetQueue.pop_front();
                    continue;
                }

                if (hasEndPoint) {
                    packet = entry.first;
                    packetQueue.pop_front(); // now we could pop it out

                    {
                        // NOTE: Nested lock, CanceldataPackets would wait for packetThread sending this packet
                        MutexAutoLock lock(&mutex_packetThreadSending);
                        packetThreadSending = true;
                    }
                }
            }
        } // MutexAutoLock block

        if (packet) {
            // This could fail, Transport layer would handle failure case
            if (!trySendViaBestRoute(packet)) {
                // Something is wrong, give up
                LOG_WARN("RouteManager[%p]: Fail to send packets", this);
            }

            delete packet;
            packet = NULL;

            {
                MutexAutoLock lock(&mutex_packetThreadSending);
                if (packetThreadSending) {
                    packetThreadSending = false;
                    VPLCond_Broadcast(&cond_packetThreadSending);
                }
            }
        }
    } // while (1)

 end:
    {
        MutexAutoLock lock(&mutex_packetThreadSending);
        if (packetThreadSending) {
            packetThreadSending = false;
            VPLCond_Broadcast(&cond_packetThreadSending);
        }
    }
    {
        MutexAutoLock lock(&mutex);
        state.packetThread = THREAD_STATE_EXITING;
    }
    LOG_INFO("RouteManager[%p]: Packet service thread exiting", this);
}

// class method
void Ts2::Network::RouteManager::notifyEndPointStatusChange(const Link::EndPoint *ep, void *context)
{
    RouteManager *routeManager = static_cast<RouteManager*>(context);
    routeManager->notifyEndPointStatusChange(ep);
}

void Ts2::Network::RouteManager::notifyEndPointStatusChange(const Link::EndPoint *ep)
{
    // For now, we won't use "ep".

    MutexAutoLock lock(&mutex);
    endPointStatusChangePending = true;
    VPLCond_Signal(&cond_connThreadHasWork);
}

// class method
void Ts2::Network::RouteManager::connectedSocketHandler(VPLSocket_t socket, Link::RouteType routeType,
                                                        const std::string *sessionKey, Link::ConnectHelper *connectHelper, void *context)
{
    RouteManager *routeManager = static_cast<RouteManager*>(context);
    routeManager->connectedSocketHandler(socket, routeType, sessionKey, connectHelper);
}

void Ts2::Network::RouteManager::connectedSocketHandler(VPLSocket_t socket, Link::RouteType routeType,
                                                        const std::string *sessionKey, Link::ConnectHelper *connectHelper)
{
    if (routeType == Link::ROUTE_TYPE_INVALID) {  // means no more connected sockets.
        MutexAutoLock lock(&mutex_endPoints);
        activeConnHelperSet.erase(connectHelper);
        doneConnHelperQ.push(connectHelper);
        VPLCond_Broadcast(&cond_someEndPointReady);
        return;
    }
    assert(routeType == Link::ROUTE_TYPE_DIN ||
           routeType == Link::ROUTE_TYPE_DEX ||
           routeType == Link::ROUTE_TYPE_P2P ||
           routeType == Link::ROUTE_TYPE_PRX);

    int err = AddConnectedSocket(socket, routeType, sessionKey, false);
    if (err) {
        // ErrMsg logged by AddConnectionSocket().
        VPLSocket_Close(socket);
    }
}

void Ts2::Network::RouteManager::validateEndPoints(bool forceDropAll)
{
    VPLMutex_Lock(&mutex_endPoints);
    for (int rt = Link::ROUTE_TYPE_LOWEST_COST; rt <= Link::ROUTE_TYPE_HIGHEST_COST; rt++) {
        std::map<Link::RouteType, Link::EndPoint*>::iterator it = endPointMap.find(static_cast<Link::RouteType>(rt));
        if (it == endPointMap.end()) {  // no EndPoint obj for given RouteType
            continue;
        }

        if (!it->second->IsValid() || forceDropAll) {
            LOG_INFO("RouteManager[%p]: Purging invalid EndPoint[%p]", this, it->second);
            Link::EndPoint *ep = it->second;
            endPointMap.erase(it);
            if (!ep->IsVoluntaryExit() || forceDropAll) {
                unexpectedLossOfEndPoint = true;
                retryOnce = false;
            }

            VPLMutex_Unlock(&mutex_endPoints);
            SafeDestroyEndPoint(ep);
            VPLMutex_Lock(&mutex_endPoints);
        }
    } // for each routetype
    VPLMutex_Unlock(&mutex_endPoints);
}

void Ts2::Network::RouteManager::tryTcpDinConn()
{
    int disabledRouteTypes = localInfo->GetDisabledRouteTypes();
    if (disabledRouteTypes & LocalInfo::DISABLED_ROUTE_TYPE_DIN) {
        return;
    }

    Link::TcpDinConnHelper *tdch = new (std::nothrow) Link::TcpDinConnHelper(remoteUserId, remoteDeviceId, remoteInstanceId,
                                                                             connectedSocketHandler, this, localInfo);
    if (!tdch) {
        LOG_ERROR("RouteManager[%p]: No memory to create TcpDinConnHelper obj", this);
        return;
    }

    {
        // Put helper into track list first, in case a connect attempt finished too fast
        MutexAutoLock lock(&mutex_endPoints);
        activeConnHelperSet.insert(tdch);
    }

    int err = tdch->Connect();
    if (err) {
        LOG_ERROR("RouteManager[%p]: Connect failed on TcpDinConnHelper[%p]: err %d", this, tdch, err);
        {
            MutexAutoLock lock(&mutex_endPoints);
            activeConnHelperSet.erase(tdch);
        }
        err = tdch->WaitDone();
        if (err) { 
            LOG_ERROR("RouteManager[%p]: WaitDone failed on TcpDinConnHelper[%p]: err %d", this, tdch, err);
            // We could not confirm if this obj stopped.
            // Since we don't know if it is safe to destroy it, we will let it leak.
            return;
        }
        delete tdch;
    }
}

void Ts2::Network::RouteManager::tryTcpPrxP2pConns()
{
#if !defined(TS2_NO_PXD) && !defined(TS2_NO_PRX)
    int disabledRouteTypes = localInfo->GetDisabledRouteTypes();
    int p2pPrxMask = (LocalInfo::DISABLED_ROUTE_TYPE_P2P | LocalInfo::DISABLED_ROUTE_TYPE_PRX);
    if ((disabledRouteTypes & p2pPrxMask) == p2pPrxMask) {
        // All disabled
        return;
    }

    Link::PrxP2PConnHelper *prxch = new (std::nothrow) Link::PrxP2PConnHelper(remoteUserId, remoteDeviceId, remoteInstanceId,
                                                                             connectedSocketHandler, this, localInfo);
    if (!prxch) {
        LOG_ERROR("RouteManager[%p]: No memory to create PrxP2PConnHelper obj", this);
        return;
    }

    {
        // Put helper into track list first, in case a connect attempt finished too fast
        MutexAutoLock lock(&mutex_endPoints);
        activeConnHelperSet.insert(prxch);
    }

    int err = prxch->Connect();
    if (err) {
        LOG_ERROR("RouteManager[%p]: Connect failed on PrxP2PConnHelper[%p]: err %d", this, prxch, err);
        {
            MutexAutoLock lock(&mutex_endPoints);
            activeConnHelperSet.erase(prxch);
        }
        err = prxch->WaitDone();
        if (err) {
            // We could not determine if it is safe to destroy or not, so we will let it leak.
            LOG_ERROR("RouteManager[%p]: WaitDone failed on PrxP2PConnHelper[%p]: err %d", this, prxch, err);
            return;
        }
        delete prxch;
    }
#endif
}

inline bool Ts2::Network::RouteManager::checkAndWaitRoutes()
{
    MutexAutoLock lock(&mutex_endPoints);
    // No routes, we need helps
    if(endPointMap.empty()) {
        VPLMutex_Unlock(&mutex_endPoints);
        {
            MutexAutoLock lock(&mutex);
            forceMakeConn = true;
            VPLCond_Signal(&cond_connThreadHasWork);
        }
        VPLMutex_Lock(&mutex_endPoints);
        while(endPointMap.empty() &&
              state.stop != STOP_STATE_STOPPING) {
            int err = VPLCond_TimedWait(&cond_someEndPointReady, &mutex_endPoints, ConnectionEstablishTimeout);
            if (err != VPL_OK && err != VPL_ERR_TIMEOUT) {
                LOG_ERROR("RouteManager[%p]: CondVar failed: err %d", this, err);
                goto end;
            }
            if (err == VPL_ERR_TIMEOUT) {
                LOG_WARN("RouteManager[%p]: No available routes after "FMTu64" secs", this, VPLTime_ToSec(ConnectionEstablishTimeout));
                goto end;
            }
        }
        if (state.stop == STOP_STATE_STOPPING) {
            LOG_INFO("RouteManager[%p]: Stopping state detected", this);
            goto end;
        }
    }

    {
        std::map<Link::RouteType, Link::EndPoint*>::iterator it = endPointMap.find(Link::ROUTE_TYPE_PRX);
        // We don't want to leverage infra if it's possible.
        // Wait for an while, if there is no more new routes, we have no choice.
        if(endPointMap.size() == 1 &&
           it != endPointMap.end()) {
            VPLTime_t createTime = it->second->GetEstablishTimeStamp();
            VPLTime_t duration = VPLTime_DiffClamp(VPLTime_GetTimeStamp(), createTime);
            VPLTime_t prxDelay = VPLTime_FromSec(localInfo->GetUsePrxDelay());

            while (state.stop != STOP_STATE_STOPPING &&
                   endPointMap.size() == 1 &&
                   endPointMap.find(Link::ROUTE_TYPE_PRX) != endPointMap.end() &&
                   prxDelay > duration) {
                // We need to wait
                int err = VPLCond_TimedWait(&cond_someEndPointReady, &mutex_endPoints, VPLTime_DiffAbs(prxDelay, duration));
                if (err != VPL_OK && err != VPL_ERR_TIMEOUT) {
                    LOG_ERROR("RouteManager[%p]: CondVar failed: err %d", this, err);
                    goto end;
                }
                duration = VPLTime_DiffClamp(VPLTime_GetTimeStamp(), createTime);
            }
        }
    }
end:
    return !endPointMap.empty();
}

inline bool Ts2::Network::RouteManager::trySendViaBestRoute(Packet *packet)
{
    bool packetSent = false;
    VPLMutex_Lock(&mutex_endPoints);

    // If no routes are available, just discard the packet and let Transport layer do re-transmit
    if(endPointMap.empty()) {
        LOG_WARN("RouteManager[%p]: No available routes", this);
        goto end;
    }

    for (int rt = Link::ROUTE_TYPE_LOWEST_COST; rt <= Link::ROUTE_TYPE_HIGHEST_COST; rt++) {
        std::map<Link::RouteType, Link::EndPoint*>::iterator it = endPointMap.find(static_cast<Link::RouteType>(rt));
        if (it == endPointMap.end()) {  // no EndPoint obj for given RouteType
            continue;
        }

        if(!it->second->IsValid()) {
            continue;
        }

        int err = it->second->Send(packet);
        if (err) {
            LOG_ERROR("RouteManager[%p]: Send failed: err %d", this, err);
        }
        else {
            // This packet was successfully sent, so break out of the loop.
            packetSent = true;
            break;
        }
    } // for each routetype
end:
    VPLMutex_Unlock(&mutex_endPoints);
    if(!packetSent) {
        // Notify connection thread something wrong
        MutexAutoLock lock(&mutex);
        endPointStatusChangePending = true;
        VPLCond_Signal(&cond_connThreadHasWork);
    }
    return packetSent;
}

void Ts2::Network::RouteManager::purgeDoneConnHelpers()
{
    std::list<Link::ConnectHelper*> deleteList;
    {
        MutexAutoLock lock(&mutex_endPoints);
        while (doneConnHelperQ.size() > 0) {
            Link::ConnectHelper *ch = doneConnHelperQ.front();
            doneConnHelperQ.pop();
            deleteList.push_back(ch);
        }
    }

    for(std::list<Link::ConnectHelper*>::iterator i = deleteList.begin();
            i != deleteList.end(); i++) {
        Link::ConnectHelper *ch = *i;
        int err = ch->WaitDone();  // Make sure it is really done.
        if (err) {
            LOG_ERROR("RouteManager[%p]: WaitDone failed on ConnectHelper[%p]: err %d", this, ch, err);
            // We could not confirm if this obj stopped.
            // Since we don't know if it is safe to destroy it, we will let it leak.
            continue;
        }
        delete ch;
    }
}

void Ts2::Network::RouteManager::purgeCostlyRoutes()
{
    // Find the cheapest route.
    int cheapestRt = Link::ROUTE_TYPE_INVALID;
    {
        MutexAutoLock lock(&mutex_endPoints);

        // If there is no more than one EndPoint obj, then there's nothing to purge, so go back.
        if (endPointMap.size() <= 1) {
            return;
        }

        for (int rt = Link::ROUTE_TYPE_LOWEST_COST; rt <= Link::ROUTE_TYPE_HIGHEST_COST; rt++) {
            std::map<Link::RouteType, Link::EndPoint*>::iterator it = endPointMap.find(static_cast<Link::RouteType>(rt));
            if (it != endPointMap.end()) {
                cheapestRt = rt;
                break;
            }
        }
        if (cheapestRt == Link::ROUTE_TYPE_INVALID) {
            // This should not happen, because of the (endPointMap.size() <= 1) check 
            // at the beginning of the function.
            // However, if it does happen, there's nothing we need to do, so just go back.
            return;
        }
    }

    // Destroy any route that's more costly than the cheapest route.
    VPLMutex_Lock(&mutex_endPoints);
    for (int rt = cheapestRt + 1; rt <= Link::ROUTE_TYPE_HIGHEST_COST; rt++) {
        std::map<Link::RouteType, Link::EndPoint*>::iterator it = endPointMap.find(static_cast<Link::RouteType>(rt));
        if (it == endPointMap.end()) {  // no EndPoint obj for given RouteType
            continue;
        }

        Link::EndPoint *ep = it->second;
        endPointMap.erase(it);

        VPLMutex_Unlock(&mutex_endPoints);
        SafeDestroyEndPoint(ep);
        VPLMutex_Lock(&mutex_endPoints);
    } // for each routetype
    VPLMutex_Unlock(&mutex_endPoints);
}

inline bool Ts2::Network::RouteManager::retryCheaperRoute() const
{
    MutexAutoLock lock(&mutex_endPoints);
    if (endPointMap.empty()) {
        return false;
    }
    return endPointMap.find(Link::ROUTE_TYPE_LOWEST_COST) == endPointMap.end();
}

