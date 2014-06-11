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

///
/// Pool.hpp
///
/// Class Definition for Pool to manage Vtunnels
///
#ifndef __POOL_HPP__
#define __POOL_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>
#include <LocalInfo.hpp>
#include "Vtunnel.hpp"

namespace Ts2 {
class Packet;
namespace Transport {

typedef struct dstInfo_s {
    u64 deviceId;
    u32 instId;
    u32 vtId;
    u64 seqNum;

    bool operator < (const dstInfo_s target) const {
        return ((deviceId < target.deviceId) || 
               ((deviceId == target.deviceId) && (instId < target.instId)) ||
               ((deviceId == target.deviceId) && (instId == target.instId) && (vtId < target.vtId)) ||
               ((deviceId == target.deviceId) && (instId == target.instId) && (vtId == target.vtId) && (seqNum < target.seqNum)));
    }
} dstInfo_t;

class Pool {
public:
    Pool(bool isServer, LocalInfo *localInfo);
    ~Pool();

    int Start();
    int Stop();
    int VtunnelAdd(Vtunnel* vt, u32& retVtId);
    int VtunnelRemove(u32 vtId);
    int VtunnelFind(u32 vtId, Vtunnel*& vt);
    int VtunnelFind(const dstInfo_t& dst, Vtunnel*& vt);
    int VtunnelRelease(Vtunnel* vt);
    int VtunnelDrop(u64 deviceId);
    void SetConnRequestHandler(void (*handler)(Packet *));
    VPLNet_port_t GetListenPort();
    size_t GetPoolSize();

private:
    VPL_DISABLE_COPY_AND_ASSIGN(Pool);

    VPLMutex_t mutex;
    enum PoolState {
        TS_POOL_NULL = 0,
        TS_POOL_STARTED,
        TS_POOL_STOPPING,
        TS_POOL_STOPPED,
    };
    PoolState state;
    map<u32, Vtunnel *> vtunnelMap;
    map<dstInfo_t, u32> vtIdMap;
    u32 idCounter;
    const bool isServer;
    LocalInfo* localInfo;
    VPLCond_t removeCond;
    int removeWaitCount;

    u64 eventHandle;
    static VPLTHREAD_FN_DECL eventHandlerThread(void* arg);
    void eventHandlerThread();
    VPLDetachableThreadHandle_t eventThrdHandle;
    bool isEventThrdStarted;        // event handler thread is running
    bool isEventThrdStop;           // flag to stop event handler thread

    // Must be static to be called from anywhere
    static void recvPacketHandler(Packet* packet, void* context);
    void recvPacketHandler(Packet* packet);
    void (*connRequestHandler)(Packet* packet);
    Ts2::Network::FrontEnd* networkFrontEnd;
};

} // end namespace Transport
} // end namespace Ts2

#endif // include guard
