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

#ifndef __TS2_NETWORK_ROUTE_MANAGER_HPP__
#define __TS2_NETWORK_ROUTE_MANAGER_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>
#include <vpl_net.h>
#include <vpl_socket.h>
#include <vpl_th.h>
#include <vpl_thread.h>
#include <vpl_time.h>

#include <TcpDinConnHelper.hpp>
#ifndef TS2_NO_PXD
#include <PrxP2PConnHelper.hpp>
#endif
#include <EndPoint.hpp>
#include <LocalInfo.hpp>

#include <deque>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <utility>

namespace Ts2 {
class Packet;
#ifndef __TS2_PACKETHANDLER_DEFINED__
typedef void (*PacketHandler)(Packet*, void *context);
#define __TS2_PACKETHANDLER_DEFINED__
#endif

namespace Network {
class RouteManager {
public:
    RouteManager(u64 remoteUserId, u64 remoteDeviceId, u32 remoteInstanceId,
                 PacketHandler rcvdPacketHandler, void *rcvdPacketHandler_context,
                 LocalInfo *localInfo, bool isServer);
    ~RouteManager();

    u64 GetRemoteUserId() const;
    u64 GetRemoteDeviceId() const;
    u32 GetRemoteInstanceId() const;

    int Start();
    int Stop();

    // When this returns successfully, it is safe to destroy this object.
    int WaitStopped();

    // Onwership of the packet object transfers to Enqueue, unless the function returns an error.
    int Enqueue(Packet *packet, VPLTime_t timeout);

    // Remove data packets with the specified srcVtId.
    int CancelDataPackets(u32 srcVtId);

    int AddConnectedSocket(VPLSocket_t socket, Link::RouteType routeType, const std::string *sessionKey, bool isServer);

private:
    VPL_DISABLE_COPY_AND_ASSIGN(RouteManager);

    const u64 remoteUserId;
    const u64 remoteDeviceId;
    const u32 remoteInstanceId;
    const PacketHandler rcvdPacketHandler;
    void *const rcvdPacketHandler_context;
    LocalInfo *const localInfo;
    const bool isServer;

    mutable VPLMutex_t mutex;
    mutable VPLCond_t cond_state_noClients;

    enum ThreadState {
        THREAD_STATE_NOTHREAD = 0,
        THREAD_STATE_SPAWNING,
        THREAD_STATE_RUNNING,
        THREAD_STATE_EXITING,
    };
    enum StopState {
        STOP_STATE_NOSTOP = 0,
        STOP_STATE_STOPPING,
    };
    struct State {
        State() : packetThread(THREAD_STATE_NOTHREAD), connThread(THREAD_STATE_NOTHREAD),
                  stop(STOP_STATE_NOSTOP), clients(0) {}
        ThreadState packetThread;
        ThreadState connThread;
        StopState stop;
        int clients;  // num of client threads inside the obj
    } state;

    std::deque<std::pair<Packet*, VPLTime_t> > packetQueue;

    static const size_t ThreadStackSize;

    // Connection thread is responsible for EndPoint management
    VPLDetachableThreadHandle_t connThread;
    void connThreadMain();
    static VPLTHREAD_FN_DECL connThreadMain(void *param);
    mutable VPLCond_t cond_connThreadHasWork;  // associated with "mutex"

    static void networkConnCB(void* instance);
    void reportNetworkConnected();

    // Indicated network changed events occur (protected by mutex)
    bool isNetworkUpdated;

    // Incoming packet, force make connection (protected by mutex)
    bool forceMakeConn;

    // Indicated an endpoint status is changed (protected by mutex)
    bool endPointStatusChangePending;

    // A EndPoint is gone unwillingly (protected by mutex_endPoints)
    bool unexpectedLossOfEndPoint;

    // Once an endpoint is gone unwillingly,
    // we need to retry once to test all other interfaces (protected by mutex_endPoints)
    bool retryOnce;

    // If we got a new EndPoints, we need to reset timer (protected by mutex_endPoints)
    bool resetRetryTimeout;

    // Packet thread is responsible for packet delivering
    VPLDetachableThreadHandle_t packetThread;
    void packetThreadMain();
    static VPLTHREAD_FN_DECL packetThreadMain(void *param);
    mutable VPLCond_t cond_packetThreadHasWork;  // associated with "mutex"

    std::map<Link::RouteType, Link::EndPoint*> endPointMap;
    std::set<Link::ConnectHelper*> activeConnHelperSet;
    std::queue<Link::ConnectHelper*> doneConnHelperQ;
    mutable VPLMutex_t mutex_endPoints;
    mutable VPLCond_t cond_someEndPointReady;  // associated with mutex_endPoints

    void notifyEndPointStatusChange(const Link::EndPoint *ep);
    static void notifyEndPointStatusChange(const Link::EndPoint *ep, void *context);

    void connectedSocketHandler(VPLSocket_t socket, Link::RouteType routeType,
                                const std::string *sessionKey, Link::ConnectHelper *connectHelper);
    static void connectedSocketHandler(VPLSocket_t socket, Link::RouteType routeType,
                                       const std::string *sessionKey, Link::ConnectHelper *connectHelper, void *context);

    bool packetThreadSending;
    mutable VPLMutex_t mutex_packetThreadSending;
    mutable VPLCond_t cond_packetThreadSending;

    // forceDropAll is a debug parameter for simulating network disconnected
    void validateEndPoints(bool forceDropAll = false);

    void tryTcpDinConn();
    void tryTcpPrxP2pConns();
    inline bool checkAndWaitRoutes();
    inline bool trySendViaBestRoute(Packet *packet);
    void purgeDoneConnHelpers();
    void purgeCostlyRoutes();

    // To tell if it is possible to have a cheaper route
    // This function return false if there is no route
    inline bool retryCheaperRoute() const;

    // Maximum time between successive calls to validateEndPoints().
    static const VPLTime_t ValidateEndPointMaxInterval;
    // Maximum time for a valid packet
    static const VPLTime_t ValidatePacketMaxInterval;
    // Timeout for connection setup
    static const VPLTime_t ConnectionEstablishTimeout;
};
} // end namespace Network
} // end namespace Ts2

#endif // incl guard
