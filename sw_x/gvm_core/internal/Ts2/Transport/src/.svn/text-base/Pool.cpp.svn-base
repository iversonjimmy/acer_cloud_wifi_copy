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

#include "Pool.hpp"

#include "vplu_format.h"

static const size_t threadStackSize = UTIL_DEFAULT_THREAD_STACK_SIZE;

Ts2::Transport::Pool::Pool(bool isServer, Ts2::LocalInfo* localInfo)
  : state(TS_POOL_NULL),
    isServer(isServer),
    localInfo(localInfo),
    removeWaitCount(0),
    eventHandle(0),
    isEventThrdStarted(false),
    isEventThrdStop(false),
    connRequestHandler(NULL),
    networkFrontEnd(NULL)
{
    VPLMutex_Init(&mutex);
    VPLCond_Init(&removeCond);

    //
    // Initialize vtId counter with a value that should be different
    // for each instance.  Take the low 32 bits of the current TimeStamp.
    // Absolute uniqueness is not really required, but this should be
    // "pretty unique".
    //
    idCounter = (u32) (VPLTime_GetTimeStamp() & 0xffffffff);
    LOG_INFO("Pool[%p]: Created", this);
}

Ts2::Transport::Pool::~Pool()
{
    VPLMutex_Destroy(&mutex);
    VPLCond_Destroy(&removeCond);

    LOG_INFO("Pool[%p]: Destroyed", this);
    if (!vtunnelMap.empty()) {
        LOG_INFO("Pool[%p]: vtunnelMap is not empty ("FMTu_size_t" entries)", this, vtunnelMap.size());
    }
    // Upper layer is responsible for localInfo's lifecycle
    if (localInfo != NULL) {
        localInfo = NULL;
    }
}

int Ts2::Transport::Pool::Start()
{
    int err = TS_OK;

    // Initialize the lower layers and start the EventHandler thread

    LOG_INFO("Pool[%p]: Start", this);

    MutexAutoLock lock(&mutex);
    if (state != TS_POOL_NULL) {
        LOG_ERROR("Pool[%p]: invalid state %u", this, state);
        err = TS_ERR_WRONG_STATE;
        goto end;
    }
    assert(localInfo);
    if (!isEventThrdStarted) {
        err = localInfo->CreateQueue(&eventHandle);
        if (err != VPL_OK) {
            LOG_ERROR("Pool[%p]: CreateQueue for events fails: %d", this, err);
            err = TS_ERR_INTERNAL;
            goto end;
        }
        err = Util_SpawnThread(eventHandlerThread, this, threadStackSize, /*isJoinable*/VPL_TRUE, &eventThrdHandle);
        if (err != VPL_OK) {
            LOG_ERROR("Pool[%p]: Spawn event handler thread fails: %d", this, err);
            err = TS_ERR_INTERNAL;
            goto end;
        }
        isEventThrdStarted = true;
    }
    if (isServer) {
        networkFrontEnd = new (std::nothrow) Ts2::Network::FrontEnd(recvPacketHandler, this, VPLNET_ADDR_ANY, VPLNET_PORT_ANY, localInfo);
    } else {
        networkFrontEnd = new (std::nothrow) Ts2::Network::FrontEnd(recvPacketHandler, this, localInfo);
    }
    if (networkFrontEnd == NULL) {
        LOG_ERROR("Pool[%p]: unable to allocate memory", this);
        err = TS_ERR_NO_MEM;
        goto end;
    }
    err = networkFrontEnd->Start();
    if (err != TS_OK) {
        LOG_ERROR("Pool[%p]: networkFrontEnd Start fails: %d", this, err);
        goto end;
    }

    state = TS_POOL_STARTED;

end:
    return err;
}

VPLTHREAD_FN_DECL Ts2::Transport::Pool::eventHandlerThread(void* arg)
{
    Ts2::Transport::Pool *pool = (Ts2::Transport::Pool*)arg;

    pool->eventHandlerThread();

    return VPLTHREAD_RETURN_VALUE_UNUSED;
}

void Ts2::Transport::Pool::eventHandlerThread()
{
    int rv = CCD_OK;

    LOG_INFO("Pool[%p]: Event handler thread starts", this);

    google::protobuf::RepeatedPtrField<ccd::CcdiEvent> events;
    while (!isEventThrdStop) {
        rv = localInfo->GetEvents(eventHandle, 10, (int)VPLTime_ToMillisec(localInfo->GetEventsTimeout()), &events);
        if (rv == CCD_ERROR_NO_QUEUE) {
            LOG_WARN("No queue to get events: %d", rv);
            goto done;
        }
        if (rv != CCD_OK) {
            LOG_ERROR("Failed to dequeue events: %d", rv);
            goto done;
        }
        if (events.size() == 0) {
            LOG_DEBUG("No events");
            continue;
        }
        for (int i = 0; i < events.size(); i++) {
            LOG_DEBUG("Events: %s", events.Get(i).DebugString().c_str());
            if (events.Get(i).has_device_info_change()) {
                if (events.Get(i).device_info_change().change_type() == ccd::DEVICE_INFO_CHANGE_TYPE_UNLINK) {
                    if (events.Get(i).device_info_change().has_device_id()) {
                        u64 deviceId = events.Get(i).device_info_change().device_id();
                        VtunnelDrop(deviceId);
                    }
                }
            }

        }
        events.Clear();
    }

done:
    events.Clear();
    LOG_INFO("Pool[%p]: Event handler thread exits: %d", this, rv);
    return;
}

int Ts2::Transport::Pool::VtunnelDrop(u64 deviceId)
{
    int rv = TS_OK;

    // Traverse vtunnelMap looking for vtunnels to an unlinked device and drop the connections
    // Make sure TS APIs for the vtunnel return TS_ERR_INVALID_DEVICE
    map<u32, Vtunnel*>::iterator mit;

    MutexAutoLock lock(&mutex);
    // FIXME: change log level
    LOG_INFO("Dropping any active vtunnels for deviceId: "FMTu64, deviceId);
    for (mit = vtunnelMap.begin(); mit != vtunnelMap.end(); mit++) {
        if (mit->second->IsConnectingDevice(deviceId)) {
            // FIXME: change log level
            LOG_INFO("Vtunnel with deviceId = "FMTu64" found!", deviceId);
            //
            // Note that we do not need to grab a reference to the Vtunnel
            // at this point, since the pool mutex is held throughout this
            // entire loop. That means that no TS callers can look up the
            // tunnel until we finish here. Note also that ForceClose must
            // be non-blocking (does not wait for FIN_ACKs).
            //

            // call Vtunnel::ForceClose() to drop vtunnel even if opening/half open
            rv = mit->second->ForceClose();
            if (rv != TS_OK) {
                // TODO: how to handle Close error?
                LOG_ERROR("Failed to force close vtId "FMTu32" : %d", mit->first, rv);
            }
            // Note that we do not delete the map entry or the vtunnel object at this
            // point. That will be left to the pre-existing threads using this tunnel.
        }
    }

    return rv;
}

VPLNet_port_t Ts2::Transport::Pool::GetListenPort()
{
    MutexAutoLock lock(&mutex);
    if (state != TS_POOL_STARTED) {
        LOG_ERROR("Pool[%p]: invalid state %u", this, state);
        return VPLNET_PORT_INVALID;
    }
    if (!isServer || networkFrontEnd == NULL) {
        return VPLNET_PORT_INVALID;
    }
    return networkFrontEnd->GetTcpDinListenerPort();
}

// class method (declared static in header)
void Ts2::Transport::Pool::recvPacketHandler(Ts2::Packet* pkt, void* ctx)
{
    assert(ctx);
    Pool *pool = (Pool *)ctx;
    pool->recvPacketHandler(pkt);
    return;
}

//
// Handler for packets being sent up from the Network Layer
//
void Ts2::Transport::Pool::recvPacketHandler(Ts2::Packet* pkt)
{
    do {
        //
        // Extract local vtId to identify the Vtunnel that should get this packet
        //
        u32 local_vtId = pkt->GetDstVtId();
        Vtunnel *vt;

        //
        // Zero is not a valid vtId.  If this is the server pool, then this
        // identifies an incoming connection request.
        //
        if (local_vtId == 0) {
            {
                MutexAutoLock lock(&mutex);
                if (!isServer) {
                    LOG_WARN("Pool[%p]: client drop packet %p with zero vtId", this, pkt);
                    delete pkt;
                    break;
                }
                if (connRequestHandler == NULL) {
                    LOG_WARN("Pool[%p]: drop packet %p, no conn request handler", this, pkt);
                    delete pkt;
                    break;
                }
                //
                // If Pool::Stop called, don't allow any new connections, but continue to
                // process packets for established connections to allow Close() processing.
                //
                if (state != TS_POOL_STARTED) {
                    LOG_INFO("Pool[%p]: %s, new conn request dropped", this,
                        (state == TS_POOL_STOPPING) ? "shutdown in progress" : "invalid pool state");
                    delete pkt;
                    break;
                }
            }
            //
            // Must release Pool mutex before the following call, since it will acquire
            // the higher level mutex of the TSS server layer (to avoid ABBA deadlocks
            // with service layer calls to the Pool class). See the comment in
            // Pool::SetConnRequestHandler explaining why it is safe to deference this
            // pointer without holding the mutex.
            //
            LOG_DEBUG("Pool[%p]: conn request pkt %p", this, pkt);
            connRequestHandler(pkt);
            break;
        }
        //
        // For established vtunnels, look up the vtId in the map.  This logic is the same
        // for client and server instances.
        //
        int err = VtunnelFind(local_vtId, vt);
        if (err != 0) {
            LOG_DEBUG("Pool[%p]: vtId %d not found, dropping packet %p type %d", this, local_vtId, pkt, pkt->GetPktType());
            delete pkt;
            break;
        }
        LOG_DEBUG("Pool[%p]: pkt %p for vt "FMTu32, this, pkt, local_vtId);
        vt->ReceivePacket(pkt);
        VtunnelRelease(vt);

    } while (ALWAYS_FALSE_CONDITIONAL);

    return;
}

void Ts2::Transport::Pool::SetConnRequestHandler(void (*handler)(Packet *))
{
    //
    // The recvPacket handler dereferences connRequestHandler without holding
    // the mutex, so make sure it only gets set once and never transitions from
    // non-NULL to NULL.
    //
    MutexAutoLock lock(&mutex);
    if (connRequestHandler == NULL) {
        connRequestHandler = handler;
    } else {
        LOG_ERROR("Pool[%p]: ignore attempt to overwrite connRequestHandler %p with %p", 
                   this, connRequestHandler, handler);
    }
}

int Ts2::Transport::Pool::VtunnelAdd(Vtunnel* vt, u32& retVtId)
{
    int err = TS_ERR_INVALID;

    //
    // Assign a local virtual tunnel ID and insert the new entry
    // into the vtId -> Vtunnel map.
    //
    do {
        u32 vtId;
        map<u32, Vtunnel*>::iterator mit;

        MutexAutoLock lock(&mutex);
        // Only need to check this once, since the lock is held
        if (state != TS_POOL_STARTED) {
            LOG_ERROR("Pool[%p]: invalid state %u", this, state);
            err = TS_ERR_WRONG_STATE;
            break;
        }
        // check whether dst device id is valid or not
        if (!localInfo->IsAuthorizedClient(vt->GetDstDeviceId())) {
            LOG_ERROR("Device "FMTu64" is not authorized to connect.", vt->GetDstDeviceId());
            err = TS_ERR_INVALID_DEVICE;
            break;
        }
        assert(vt->GetRefCount() == 0);
        // Assign a vtId to the new tunnel
        for (int tries = 10; tries > 0; tries--) {
            vtId = ++idCounter;
            // Check for wrap:  zero is not a valid vtId
            if (vtId == 0) continue;
            //
            // New ID better not already be mapped
            //
            mit = vtunnelMap.find(vtId);
            if (mit == vtunnelMap.end()) {
                vtunnelMap[vtId] = vt;
#ifdef TS2_PKT_RETRY_ENABLE
                // add to vtIdMap only if SYN sequence number is not 0
                if (vt->GetDstStartSeqNum() != 0) {
                    dstInfo_t dst = {vt->GetDstDeviceId(), vt->GetDstInstanceId(), vt->GetDstVtId(), vt->GetDstStartSeqNum()};
                    vtIdMap[dst] = vtId;
                }
#endif
                vt->SetVtId(vtId);
                retVtId = vtId;
                // Pass pointer to network layer
                vt->SetFrontEnd(networkFrontEnd);
                err = TS_OK;
                break;
            } else {
                // Collision detected, try again.
                // FIXME: reset idCounter completely?
                LOG_ALWAYS("Found vtId "FMTu32" already in use", vtId);
            }
        }
        if (err == TS_OK) {
            // Add reference for the Open in progress
            vt->AddRefCount(+1);
            LOG_INFO("New vtId "FMTu32" added to map (entries "FMTu_size_t")", vtId, vtunnelMap.size());
        } else {
            LOG_ERROR("Pool[%p]: Unable to assign new vtId ("FMTu_size_t" map entries)", this, vtunnelMap.size());
        }
    } while (ALWAYS_FALSE_CONDITIONAL);

    return err;
}

//
// Delete both the VtunnelMap entry and the actual Vtunnel object
// if there are no other active threads using the Vtunnel
//
int Ts2::Transport::Pool::VtunnelRemove(u32 vtId)
{
    int err = TS_OK;
    int tries = 0;

    while (true) {
        map<u32, Vtunnel*>::iterator mit;

        MutexAutoLock lock(&mutex);
        mit = vtunnelMap.find(vtId);
        if (mit == vtunnelMap.end()) {
            LOG_DEBUG("Cannot find vtId "FMTu32" in vtunnel map (entries "FMTu_size_t")", vtId, vtunnelMap.size());
            err = TS_ERR_BAD_HANDLE;
            break;
        }
        // If there are active references, then wait and retry.
        if (mit->second->GetRefCount() > 0) {
            if (++tries > 10) {
                //
                // Tried multiple times with timeout, but still in use.  Note that there
                // is only one "remove" condvar for the whole pool, so a wakeup may not be
                // intended for the current vtunnel entry.  Eventually give up and remove
                // the map entry but do not delete the actual vtunnel object, since there
                // is an active reference.
                //
                LOG_WARN("vtId "FMTu32" still in use, erase from vtunnel map (entries "FMTu_size_t")", vtId, vtunnelMap.size());
#ifdef TS2_PKT_RETRY_ENABLE
                // delete from vtIdMap only if SYN sequence number is not 0
                if (mit->second->GetDstStartSeqNum() != 0) {
                    dstInfo_t dst = {mit->second->GetDstDeviceId(), mit->second->GetDstInstanceId(), mit->second->GetDstVtId(), mit->second->GetDstStartSeqNum()};
                    vtIdMap.erase(dst);
                }
#endif
                vtunnelMap.erase(mit);
                break;
            }
            LOG_DEBUG("vtId "FMTu32" in use ... wait to remove (%d tries)", vtId, tries);
            removeWaitCount++;
            err = VPLCond_TimedWait(&removeCond, &mutex, VPLTime_FromMillisec(20));
            removeWaitCount--;
            assert(removeWaitCount >= 0);
            if (err != 0 && err != VPL_ERR_TIMEOUT) {
                // Some kind of failure, better to bail than retry
                LOG_ERROR("Wait on remove condVar fails with %d for vtId "FMTu32, err, vtId);
                break;
            }
            // Loop back and recheck
            continue;
        }
        // All clear, delete both the vtunnel object and the map entry
#ifdef TS2_PKT_RETRY_ENABLE
        // delete from vtIdMap only if SYN sequence number is not 0
        if (mit->second->GetDstStartSeqNum() != 0) {
            dstInfo_t dst = {mit->second->GetDstDeviceId(), mit->second->GetDstInstanceId(), mit->second->GetDstVtId(), mit->second->GetDstStartSeqNum()};
            vtIdMap.erase(dst);
            LOG_INFO("Erased ["FMTu64","FMTu32","FMTu32","FMTu64"]vtId "FMTu32" from vtIdMap (entries "FMTu_size_t")", 
                mit->second->GetDstDeviceId(),
                mit->second->GetDstInstanceId(),
                mit->second->GetDstVtId(),
                mit->second->GetDstStartSeqNum(),
                vtId, 
                vtIdMap.size());
        }
#endif
        delete mit->second;
        vtunnelMap.erase(mit);
        LOG_INFO("Erased vtId "FMTu32" from vtunnel map (entries "FMTu_size_t")", vtId, vtunnelMap.size());
        break;
    } // end of AutoLock scope

    return err;
}

//
// Uses the vtunnelMap of the Pool to translate a local vtId
// into a pointer to the actual vtunnel object.  Grab a reference
// to the Vtunnel to insure that it doesn't get deleted until the
// caller is finished with the pointer.
//
int Ts2::Transport::Pool::VtunnelFind(u32 vtId, Vtunnel*& vt)
{
    int err = TS_OK;
    int refCount;

    do {
        map<u32, Vtunnel*>::iterator mit;

        MutexAutoLock lock(&mutex);
        mit = vtunnelMap.find(vtId);
        if (mit == vtunnelMap.end()) {
            vt = NULL;
            err = TS_ERR_BAD_HANDLE;
            LOG_DEBUG("No map entry for vtId "FMTu32, vtId);
            break;
        }
        vt = mit->second;
        refCount = vt->AddRefCount(+1);
        if (refCount > 1) {
            LOG_DEBUG("Vtunnel[%p]: refCount %d for vtId "FMTu32, vt, refCount, vtId);
        }

    } while(ALWAYS_FALSE_CONDITIONAL);

    return err;
}

int Ts2::Transport::Pool::VtunnelFind(const dstInfo_t& dst, Vtunnel*& vt)
{
    int err = TS_OK;

    do {
        map<dstInfo_t, u32>::iterator vit;
        // find vtId from vtIdMap by deviceId, instanceId, and seqNum 
        MutexAutoLock lock(&mutex);

        // for backward compatibility, always return TS_ERR_BAD_HANDLE so Pool creates new vtunnel when SYN seqNum is 0
        if (dst.seqNum == 0) {
            LOG_DEBUG("No map entry for deviceId:"FMTu64", instance id:"FMTu32", vt id:"FMTu32", seq:"FMTu64, dst.deviceId, dst.instId, dst.vtId, dst.seqNum);
            vt = NULL;
            err = TS_ERR_BAD_HANDLE;
            break;
        }

        vit = vtIdMap.find(dst);
        if (vit == vtIdMap.end()) {
            LOG_DEBUG("No map entry for deviceId:"FMTu64", instance id:"FMTu32", vt id:"FMTu32", seq:"FMTu64, dst.deviceId, dst.instId, dst.vtId, dst.seqNum);
            vt = NULL;
            err = TS_ERR_BAD_HANDLE;
            break;
        }

        err = VtunnelFind(vit->second, vt);

    } while(ALWAYS_FALSE_CONDITIONAL);

    return err;
}

int Ts2::Transport::Pool::VtunnelRelease(Vtunnel* vt)
{
    int refCount;

    MutexAutoLock lock(&mutex);
    refCount = vt->AddRefCount(-1);
    //
    // If this just released the last reference on this particular Vtunnel, check
    // if any Vtunnel is waiting to be removed and wakeup the waiters in that case.
    // Note that there is only one remove condvar for the whole pool, so this is
    // a little inefficient, but it should be pretty rare to have more than one
    // removal happening at once and is not in the performance path in any case.
    // If there is more than one, then some of the wakeups are wasted.
    //
    if (refCount == 0 && removeWaitCount > 0) {
        LOG_DEBUG("Vtunnel[%p]: signal removeCond for vtId "FMTu32, vt, vt->GetVtId());
        VPLCond_Broadcast(&removeCond);
    }

    return refCount;
}

// Return the number of entries in the VtunnelMap
size_t Ts2::Transport::Pool::GetPoolSize()
{
    size_t mapSize;

    MutexAutoLock lock(&mutex);
    mapSize = vtunnelMap.size();
    return mapSize;
}

//
// Called at shutdown time on both server and client side to
// clean up any remaining open Vtunnels.
//
int Ts2::Transport::Pool::Stop()
{
    int err = TS_OK;
    int stuckThreads = 0;
    map<u32, Vtunnel *> busyTunnels;

    // Close and delete any existing Vtunnels
    VPLMutex_Lock(&mutex);
    if (state != TS_POOL_STARTED) {
        LOG_ERROR("Pool[%p]: invalid state %u", this, state);
        goto end;
    }
    state = TS_POOL_STOPPING;

    // Stop event handler thread if necessary
    if (isEventThrdStarted) {
        isEventThrdStop = true;
        // Destroy queue so event handler thread will exit immediately
        localInfo->DestroyQueue(eventHandle);
        // Have to release the pool mutex to prevent deadlock
        VPLMutex_Unlock(&mutex);
        VPLDetachableThread_Join(&eventThrdHandle);
        // Lock the pool mutex again for following vtunnelMap cleanup
        VPLMutex_Lock(&mutex);
        isEventThrdStarted = false;
    }

    do {
        map<u32, Vtunnel*>::iterator mit;
        Vtunnel *vt;
        u32 vtId;

        // Note that a Vtunnel map entry is deleted every time through this loop, so that
        // progress is made.
        LOG_INFO("Stop with "FMTu_size_t" entries remaining in vtunnelMap", vtunnelMap.size());
        mit = vtunnelMap.begin();
        if (mit == vtunnelMap.end()) {
            // The map is empty, so we are done
            LOG_INFO("Stop processing complete");
            break;
        }

        vtId = mit->first;
        vt = mit->second;
        // To prevent normal close from deleting it out from under us
        vt->AddRefCount(+1);

        // Have to release the pool mutex in order for incoming
        // packets to be handled
        VPLMutex_Unlock(&mutex);

        LOG_INFO("Stopping vt "FMTu32, vtId);
        err = vt->ForceClose();

        // Since the lock was released above, we need to recheck things.
        // Note that the iterator gets restarted everytime.
        VPLMutex_Lock(&mutex);
        mit = vtunnelMap.find(vtId);
        if (mit != vtunnelMap.end()) {
            //
            // If there are other reference counts outstanding, then it is not
            // safe to delete the Vtunnel object.
            //
            if (mit->second->GetRefCount() == 1) {
                delete mit->second;
            } else {
                //
                // Keep the ref count and try again soon in the hopes
                // that the ForceClose above will have ejected the other
                // active threads by the time this loop finishes.
                //
                LOG_INFO("Another thread active for vt "FMTu32, vtId);
                busyTunnels[vtId] = mit->second;
            }
            // Erase the map entry to insure we make progress
#ifdef TS2_PKT_RETRY_ENABLE
            // delete from vtIdMap only if SYN sequence number is not 0
            if (vt->GetDstStartSeqNum() != 0) {
                dstInfo_t dst = {mit->second->GetDstDeviceId(), mit->second->GetDstInstanceId(), mit->second->GetDstVtId(), mit->second->GetDstStartSeqNum()};
                vtIdMap.erase(dst);
            }
#endif
            vtunnelMap.erase(mit);
        }
    } while (true);

    // Let any threads waiting in VtunnelRemove go free
    if (removeWaitCount > 0) {
        LOG_INFO("Wait for VtunnelRemove threads (%d remaining)", removeWaitCount);
        for (int tries = 0; tries < 5 && removeWaitCount > 0; tries++) {
            VPLCond_Broadcast(&removeCond);
            VPLMutex_Unlock(&mutex);
            VPLThread_Sleep(VPLTime_FromMillisec(20));
            VPLMutex_Lock(&mutex);
        }
        if (removeWaitCount > 0) {
            LOG_WARN("Threads still waiting in VtunnelRemove (%d)", removeWaitCount);
            stuckThreads++;
        }
    } else {
        //
        // If did not have to wait for VtunnelRemove, then take a short
        // nap here to allow threads awakened by the ForceClose loop above
        // to have a chance to run and unwind from their calls.
        //
        VPLMutex_Unlock(&mutex);
        VPLThread_Sleep(VPLTime_FromMillisec(20));
        VPLMutex_Lock(&mutex);
    }

    //
    // If any tunnels were busy above, wait for the threads to exit and free them
    //
    if (!busyTunnels.empty()) {
        map<u32, Vtunnel *>::iterator mit;
        LOG_INFO("Wait for active tunnels to close ("FMTu_size_t" remaining)", busyTunnels.size());
        // Check if we can free any of the previously busy tunnels now
        while (!busyTunnels.empty()) {
            mit = busyTunnels.begin();
            if (mit->second->GetRefCount() == 1) {
                LOG_INFO("Vtunnel for vt "FMTu32" is free now, delete it", mit->first);
                delete mit->second;
            } else {
                LOG_WARN("Vtunnel for vt "FMTu32" still busy, skip delete", mit->first);
                stuckThreads++;
            }
            busyTunnels.erase(mit);
        }
    }

    // Stop the lower network layers
    assert(networkFrontEnd);
    // Can't hold Pool mutex here, since EndPoints may need to recv packets
    VPLMutex_Unlock(&mutex);
    err = networkFrontEnd->Stop();
    if (err == TS_OK) {
        networkFrontEnd->WaitStopped();
    }
    delete networkFrontEnd;

    VPLMutex_Lock(&mutex);
    networkFrontEnd = NULL;

    state = TS_POOL_STOPPED;

    //
    // Be careful to return an error if we have reason to suspect that we
    // did not get all the active threads to quit.  The caller should not
    // actually delete the Pool in that case to avoid bad memory references.
    //
    if (stuckThreads > 0) {
        if (err == 0) {
            err = TS_ERR_WRONG_STATE;
        }
        LOG_WARN("Pool[%p]: Stop failed to clear all active threads", this);
    }

end:
    VPLMutex_Unlock(&mutex);

    return err;
}
