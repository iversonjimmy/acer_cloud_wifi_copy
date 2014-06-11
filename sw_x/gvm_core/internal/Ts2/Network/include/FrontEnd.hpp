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

#ifndef __TS2_NETWORK_FRONTEND_HPP__
#define __TS2_NETWORK_FRONTEND_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>
#include <vpl_net.h>
#include <vpl_socket.h>
#include <vpl_th.h>
#include <vpl_thread.h>

#include <AcceptHelper.hpp>
#include <RouteType.hpp>

#include <map>
#include <queue>
#include <string>

#include "TcpDinListener.hpp"
#ifndef TS2_NO_PXD
#include "PrxP2PListener.hpp"
#endif
#include <LocalInfo.hpp>

namespace Ts2 {
class Packet;
#ifndef __TS2_PACKETHANDLER_DEFINED__
typedef void (*PacketHandler)(Packet*, void *context);
#define __TS2_PACKETHANDLER_DEFINED__
#endif

namespace Network {
class RouteManager;
class FrontEnd {
public:
    FrontEnd(PacketHandler rcvdPacketHandler, void *rcvdPacketHandler_context,
             LocalInfo *localInfo);  // for client
    FrontEnd(PacketHandler rcvdPacketHandler, void *rcvdPacketHandler_context, 
             VPLNet_addr_t addrSpec, VPLNet_port_t portSpec,
             LocalInfo *localInfo);  // for server
    ~FrontEnd();

    int Start();
    int Stop();

    // When this function returns successfully, it is safe to destroy this object.
    int WaitStopped();

    // Ownership of the packet object transfers to Enqueue, unless the function returns an error.
    int Enqueue(Packet *packet, VPLTime_t timeout);

    // Remove data packets with the specified srcVtId.
    int CancelDataPackets(u64 dstDeviceId, u32 dstInstanceId, u32 srcVtId);

    VPLNet_port_t GetTcpDinListenerPort() const;

private:
    VPL_DISABLE_COPY_AND_ASSIGN(FrontEnd);

    const PacketHandler rcvdPacketHandler;
    void *const rcvdPacketHandler_context;

    struct TcpDinListenerSpec {
        TcpDinListenerSpec()
            : addr(VPLNET_ADDR_INVALID), port(VPLNET_PORT_INVALID) {}
        TcpDinListenerSpec(VPLNet_addr_t addr, VPLNet_port_t port)
            : addr(addr), port(port) {}
        VPLNet_addr_t addr;
        VPLNet_port_t port;
    };
    const TcpDinListenerSpec tcpDinListenerSpec;

    const bool isServer;

    mutable VPLMutex_t mutex;
    mutable VPLCond_t cond_state_noClients;

    enum StopState {
        STOP_STATE_NOSTOP = 0,
        STOP_STATE_STOPPING,
    };
    struct State {
        State() : stop(STOP_STATE_NOSTOP), clients(0) {}
        StopState stop;
        int clients;  // num of client threads inside the obj
    } state;

    class RouteManagerMap {
    public:
        RouteManagerMap(PacketHandler rcvdPacketHandler, void *rcvdPacketHandler_context,
                        LocalInfo *localInfo, bool isServer);
        ~RouteManagerMap();
        RouteManager *FindOrCreate(u64 userId, u64 deviceId, u32 instanceId);
        int VisitAll(int (RouteManager::*visitor)());

        // For each RouteManager obj, send WaitStopped and destroy it.
        int WaitStopped();

    private:
        VPL_DISABLE_COPY_AND_ASSIGN(RouteManagerMap);

        const PacketHandler rcvdPacketHandler;
        void *const rcvdPacketHandler_context;
        LocalInfo *const localInfo;
        const bool isServer;

        VPLMutex_t mutex;

        std::map<std::pair<u64, u32>, RouteManager*> diMap;
    };
    RouteManagerMap routeManagerMap;

    LocalInfo *const localInfo;
    TcpDinListener *tcpDinListener;
#ifndef TS2_NO_PXD
    PrxP2PListener *prxP2PListener;
#endif

    void incomingSocket(VPLSocket_t socket, Link::RouteType routeType,
                        u64 remoteUserId, u64 remoteDeviceId, u32 remoteInstanceId,
                        const std::string *sessionKey);
    static void incomingSocket(VPLSocket_t socket, Link::RouteType routeType,
                               u64 remoteUserId, u64 remoteDeviceId, u32 remoteInstanceId,
                               const std::string *sessionKey, Link::AcceptHelper *acceptHelper, void *incomingSocket_context);

    u64 getUserIdFromDeviceId(u64 deviceId);
};
} // end namespace Network
} // end namespace Ts2

#endif // incl guard
