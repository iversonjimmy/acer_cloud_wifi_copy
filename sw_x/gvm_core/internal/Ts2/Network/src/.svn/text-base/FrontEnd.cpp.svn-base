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

#include "FrontEnd.hpp"
#include "Ts2_RouteManager.hpp"
#include "Packet.hpp"

#include <gvm_errors.h>
#include <gvm_thread_utils.h>
#include <log.h>

#include <scopeguard.hpp>
#include <vpl_time.h>
#include <vplu_format.h>
#include <vplu_mutex_autolock.hpp>

#include <RouteType.hpp>

#include <cassert>
#include <map>
#include <sstream>
#include <string>

Ts2::Network::FrontEnd::FrontEnd(PacketHandler rcvdPacketHandler, void *rcvdPacketHandler_context,
                                 LocalInfo *localInfo)
    : rcvdPacketHandler(rcvdPacketHandler), rcvdPacketHandler_context(rcvdPacketHandler_context),
      isServer(false), 
      routeManagerMap(rcvdPacketHandler, rcvdPacketHandler_context, localInfo, false),
      localInfo(localInfo), tcpDinListener(NULL)
#ifndef TS2_NO_PXD
    , prxP2PListener(NULL)
#endif
{
    assert(rcvdPacketHandler);
    assert(localInfo);

    VPLMutex_Init(&mutex);
    VPLCond_Init(&cond_state_noClients);

    LOG_INFO("FrontEnd[%p]: Created", this);
}

Ts2::Network::FrontEnd::FrontEnd(PacketHandler rcvdPacketHandler, void *rcvdPacketHandler_context,
                                 VPLNet_addr_t addrSpec, VPLNet_port_t portSpec,
                                 LocalInfo *localInfo)
    : rcvdPacketHandler(rcvdPacketHandler), rcvdPacketHandler_context(rcvdPacketHandler_context), tcpDinListenerSpec(addrSpec, portSpec),
      isServer(true), 
      routeManagerMap(rcvdPacketHandler, rcvdPacketHandler_context, localInfo, true),
      localInfo(localInfo), tcpDinListener(NULL)
#ifndef TS2_NO_PXD
    , prxP2PListener(NULL)
#endif
{
    assert(rcvdPacketHandler);
    assert(localInfo);

    VPLMutex_Init(&mutex);
    VPLCond_Init(&cond_state_noClients);

    LOG_INFO("FrontEnd[%p]: Created", this);
}

Ts2::Network::FrontEnd::~FrontEnd()
{
    if (tcpDinListener) {
        LOG_WARN("FrontEnd[%p]: TcpDinListener obj detected", this);
        // We don't know the state of the TcpDinListener obj.
        // Destroying it here may cause the process to crash, so we will let it leak.
    }

#ifndef TS2_NO_PXD
    if (prxP2PListener) {
        LOG_WARN("FrontEnd[%p]: PrxP2PListener obj detected", this);
        // We don't know the state of the PrxP2PListener obj.
        // Destroying it here could cause the process to crash, so we will let it leak.
    }
#endif

    VPLCond_Destroy(&cond_state_noClients);
    VPLMutex_Destroy(&mutex);

    LOG_INFO("FrontEnd[%p]: Destroyed", this);
}

int Ts2::Network::FrontEnd::Start()
{
    int err = 0;

    {
        MutexAutoLock lock(&mutex);

        if (state.stop != STOP_STATE_NOSTOP) {
            LOG_ERROR("FrontEnd[%p]: Wrong stop state %d", this, state.stop);
            err = TS_ERR_WRONG_STATE;
            goto end;
        }

        if (tcpDinListener) {
            LOG_ERROR("FrontEnd[%p]: TcpDinListener obj detected", this);
            err = TS_ERR_WRONG_STATE;
            goto end;
        }

#ifndef TS2_NO_PXD
        if (prxP2PListener) {
            LOG_ERROR("FrontEnd[%p]: PrxP2PListener obj detected", this);
            err = TS_ERR_WRONG_STATE;
            goto end;
        }
#endif

        if (isServer) {
            tcpDinListener = new (std::nothrow) TcpDinListener(tcpDinListenerSpec.addr, tcpDinListenerSpec.port, incomingSocket, this, localInfo);
            if (!tcpDinListener) {
                LOG_ERROR("FrontEnd[%p]: No memory to create TcpDinListener obj", this);
                err = TS_ERR_NO_MEM;
                goto end;
            }

            err = tcpDinListener->Start();
            if (err) {
                LOG_ERROR("FrontEnd[%p]: Start failed on TcpDinListener[%p]: err %d", this, tcpDinListener, err);
                goto end;
            }

#if !defined(TS2_NO_PXD) && !defined(TS2_NO_PRX)
            prxP2PListener = new (std::nothrow) PrxP2PListener(incomingSocket, this, localInfo);
            if (!prxP2PListener) {
                LOG_ERROR("FrontEnd[%p]: No memory to create PrxP2PListener obj", this);
                err = TS_ERR_NO_MEM;
                goto end;
            }

            err = prxP2PListener->Start();
            if (err) {
                LOG_ERROR("FrontEnd[%p]: Start failed on PrxP2PListener[%p]: err %d", this, prxP2PListener, err);
                goto end;
            }
#endif
        } // isServer
    } // MutexAutoLock block

 end:
    return err;
}

int Ts2::Network::FrontEnd::Stop()
{
    int err = 0;

    LOG_INFO("FrontEnd[%p]: Stop request received", this);

    {
        MutexAutoLock lock(&mutex);

        if (state.stop == STOP_STATE_STOPPING) {
            LOG_WARN("FrontEnd[%p]: Stop state is already %d", this, state.stop);
            return 0;
        }
        state.stop = STOP_STATE_STOPPING;

        if (tcpDinListener) {
            err = tcpDinListener->Stop();
            if (err) {
                LOG_ERROR("FrontEnd[%p]: Stop failed on TcpDinListener[%p]: err %d", this, tcpDinListener, err);
                err = 0;  // Reset error and continue.
            }
        }

#ifndef TS2_NO_PXD
        if (prxP2PListener) {
            err = prxP2PListener->Stop();
            if (err) {
                LOG_ERROR("FrontEnd[%p]: Stop failed on PrxP2PListener[%p]: err %d", this, prxP2PListener, err);
                err = 0;  // Reset error and continue.
            }
        }
#endif
    }

    routeManagerMap.VisitAll(&RouteManager::Stop);

    LOG_INFO("FrontEnd[%p]: Stop request propagated: err %d", this, err);

    return err;
}

int Ts2::Network::FrontEnd::WaitStopped()
{
    int err = 0;

    LOG_INFO("FrontEnd[%p]: Waiting for obj to Stop", this);

    {
        MutexAutoLock lock(&mutex);

        if (state.stop != STOP_STATE_STOPPING) {
            LOG_ERROR("FrontEnd[%p]: Wrong stop state %d", this, state.stop);
            err = TS_ERR_WRONG_STATE;
            goto end;
        }
    } // MutexAutoLock block

    VPLMutex_Lock(&mutex);
    if (tcpDinListener) {
        TcpDinListener *_tcpDinListener = tcpDinListener;
        tcpDinListener = NULL;
        VPLMutex_Unlock(&mutex);
        int _err = _tcpDinListener->WaitStopped();
        if (_err) {
            LOG_ERROR("FrontEnd[%p]: WaitStopped failed on TcpDinListener[%p]: err %d", this, _tcpDinListener, _err);
            // We could not determine whether the TcpDinListener object have stopped.
            // So we won't destroy this object (because doing so may cause a crash), and let the object leak.
            // Note however that this alone does not preclude us from destroying this (FrontEnd) object.
        }
        else {
            delete _tcpDinListener;
        }
        VPLMutex_Lock(&mutex);
    }
    VPLMutex_Unlock(&mutex);

#ifndef TS2_NO_PXD
    {
        VPLMutex_Lock(&mutex);
        if (prxP2PListener) {
            PrxP2PListener *_prxP2PListener = prxP2PListener;
            prxP2PListener = NULL;
            VPLMutex_Unlock(&mutex);
            err = _prxP2PListener->WaitStopped();
            if (err) {
                LOG_ERROR("FrontEnd[%p]: WaitStopped failed on PrxP2PListener[%p]: err %d", this, _prxP2PListener, err);
                err = 0;  // Reset error and continue.
                // We don't know the state of the PrxP2PListener obj, so we won't destroy it.
            }
            else {
                delete _prxP2PListener;
            }
            VPLMutex_Lock(&mutex);
        }
        VPLMutex_Unlock(&mutex);
    }
#endif

    {
        MutexAutoLock lock(&mutex);

        while (state.clients > 0) {
            int _err = VPLCond_TimedWait(&cond_state_noClients, &mutex, VPL_TIMEOUT_NONE);
            if (_err) {
                LOG_ERROR("FrontEnd[%p]: CondVar failed: err %d", this, _err);
                // This condvar failure will preclude us from determining whether all client threads have exited.
                // So when we return from this function, we will return an error so that the caller won't try to destroy this (FrontEnd) object.
                if (!err) {
                    err = _err;
                }
                break;
            }
        }
    } // MutexAutoLock block

    routeManagerMap.WaitStopped();

 end:
    LOG_INFO("FrontEnd[%p]: Obj has Stopped: err %d", this, err);

    return err;
}

int Ts2::Network::FrontEnd::Enqueue(Packet *packet, VPLTime_t timeout)
{
    int err = 0;

    {
        MutexAutoLock lock(&mutex);

        state.clients++;

        if (state.stop == STOP_STATE_STOPPING) {
            LOG_WARN("FrontEnd[%p]: Method called after Stop", this);
        }
        if (state.stop != STOP_STATE_NOSTOP) {
            LOG_ERROR("FrontEnd[%p]: Wrong stop state %d", this, state.stop);
            err = TS_ERR_WRONG_STATE;
            goto end;
        }
        u64 dstDeviceId = packet->GetDstDeviceId();
        u32 dstInstanceId = packet->GetDstInstId();
        u64 dstUserId = getUserIdFromDeviceId(dstDeviceId);
        RouteManager *routeManager = routeManagerMap.FindOrCreate(dstUserId, dstDeviceId, dstInstanceId);
        if (!routeManager) {
            LOG_ERROR("FrontEnd[%p]: No memory to create RouteManager obj", this);
            err = TS_ERR_NO_MEM;
            goto end;
        }

        err = routeManager->Enqueue(packet, timeout);
        if (err) {
            LOG_ERROR("FrontEnd[%p]: Enqueue failed on RouteManager[%p]: err %d", this, routeManager, err);
            // TODO: since this means RouteManager obj is disfunctional, we should proceed to destroy it.
            goto end;
        }
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

int Ts2::Network::FrontEnd::CancelDataPackets(u64 dstDeviceId, u32 dstInstanceId, u32 srcVtId)
{
    int err = 0;

    {
        MutexAutoLock lock(&mutex);

        state.clients++;

        if (state.stop == STOP_STATE_STOPPING) {
            LOG_WARN("FrontEnd[%p]: Method called after Stop", this);
        }
        if (state.stop != STOP_STATE_NOSTOP) {
            LOG_ERROR("FrontEnd[%p]: Wrong stop state %d", this, state.stop);
            err = TS_ERR_WRONG_STATE;
            goto end;
        }
        u64 dstUserId = getUserIdFromDeviceId(dstDeviceId);
        RouteManager *routeManager = routeManagerMap.FindOrCreate(dstUserId, dstDeviceId, dstInstanceId);
        if (!routeManager) {
            LOG_ERROR("FrontEnd[%p]: No memory to create RouteManager obj", this);
            err = TS_ERR_NO_MEM;
            goto end;
        }

        err = routeManager->CancelDataPackets(srcVtId);
        if (err) {
            LOG_ERROR("FrontEnd[%p]: CancelDataPackets failed on RouteManager[%p]: err %d", this, routeManager, err);
            // TODO: since this means RouteManager obj is disfunctional, we should proceed to destroy it.
            goto end;
        }
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

VPLNet_port_t Ts2::Network::FrontEnd::GetTcpDinListenerPort() const
{
    LOG_INFO("FrontEnd[%p]: TcpDin port is %d", this, tcpDinListener->GetPort());
    MutexAutoLock lock(&mutex);
    if (!isServer || !tcpDinListener) {
        return VPLNET_PORT_INVALID;
    }
    else {
        return tcpDinListener->GetPort();
    }
}

// class method
void Ts2::Network::FrontEnd::incomingSocket(VPLSocket_t socket, Link::RouteType routeType,
                                            u64 remoteUserId, u64 remoteDeviceId, u32 remoteInstanceId,
                                            const std::string *sessionKey, Link::AcceptHelper *acceptHelper, void *context)
{
    UNUSED(acceptHelper);// we don't use this
    FrontEnd *frontEnd = static_cast<FrontEnd*>(context);
    frontEnd->incomingSocket(socket, routeType, remoteUserId, remoteDeviceId, remoteInstanceId, sessionKey);
}

void Ts2::Network::FrontEnd::incomingSocket(VPLSocket_t socket, Link::RouteType routeType,
                                            u64 remoteUserId, u64 remoteDeviceId, u32 remoteInstanceId,
                                            const std::string *sessionKey)
{
    RouteManager *routeManager = routeManagerMap.FindOrCreate(remoteUserId, remoteDeviceId, remoteInstanceId);
    if (!routeManager) {
        // ErrMsg logged by FindOrCreate().
        VPLSocket_Close(socket);
        return;
    }

    int err = routeManager->AddConnectedSocket(socket, routeType, sessionKey, true);
    if (err) {
        // ErrMsg logged by AddConnectionSocket().
        VPLSocket_Close(socket);
        return;
    }
}

u64 Ts2::Network::FrontEnd::getUserIdFromDeviceId(u64 deviceId)
{
    // For now, it is always the same as the local userId.
    return localInfo->GetUserId();
}


Ts2::Network::FrontEnd::RouteManagerMap::RouteManagerMap(PacketHandler rcvdPacketHandler, void *rcvdPacketHandler_context,
                                                         LocalInfo *localInfo, bool isServer)
  : rcvdPacketHandler(rcvdPacketHandler), rcvdPacketHandler_context(rcvdPacketHandler_context),
    localInfo(localInfo), isServer(isServer)
{
    VPLMutex_Init(&mutex);
}

Ts2::Network::FrontEnd::RouteManagerMap::~RouteManagerMap()
{
    // TODO: log msg about how many RouteManager objs are still present.
    VPLMutex_Destroy(&mutex);
}

Ts2::Network::RouteManager *Ts2::Network::FrontEnd::RouteManagerMap::FindOrCreate(u64 userId, u64 deviceId, u32 instanceId)
{
    std::pair<u64, u32> key = std::make_pair(deviceId, instanceId);
    std::map<std::pair<u64, u32>, RouteManager*>::const_iterator it = diMap.find(key);
    if (it == diMap.end()) {
        RouteManager *rm = new (std::nothrow) RouteManager(userId, deviceId, instanceId, rcvdPacketHandler, rcvdPacketHandler_context, localInfo, isServer);
        if (!rm) {
            LOG_ERROR("FrontEnd[%p]: No memory to create RouteManager obj", this);
            return NULL;
        }
        int err = rm->Start();
        if (err) {
            LOG_ERROR("FrontEnd[%p]: Start failed on RouteManager[%p]: err %d", this, rm, err);
            rm->Stop();
            rm->WaitStopped();
            delete rm;
            return NULL;
        }
        diMap[key] = rm;
        return rm;
    }
    else {
        return it->second;
    }
}

int Ts2::Network::FrontEnd::RouteManagerMap::VisitAll(int (RouteManager::*visitor)())
{
    int err = 0;

    // TODO: accumulate error

    std::map<std::pair<u64, u32>, RouteManager*>::const_iterator it;
    for (it = diMap.begin(); it != diMap.end(); it++) {
        (it->second->*visitor)();
    }

    return err;
}

int Ts2::Network::FrontEnd::RouteManagerMap::WaitStopped()
{
    int err = 0;

    VPLMutex_Lock(&mutex);
    while (!diMap.empty()) {
        std::map<std::pair<u64, u32>, RouteManager*>::iterator it = diMap.begin();
        RouteManager *rm = it->second;
        diMap.erase(it);
        VPLMutex_Unlock(&mutex);
        err = rm->WaitStopped();
        if (err) {
            LOG_ERROR("FrontEnd[%p]: WaitStopped failed on RouteManager[%p]: err %d", this, rm, err);
            // We don't know the state of the obj.
            // Rather than risk crashing, we will let the obj leak.
            err = 0;  // Reset error and continue.
        }
        else {
            delete rm;
        }
        VPLMutex_Lock(&mutex);
    }
    VPLMutex_Unlock(&mutex);

    return err;
}
