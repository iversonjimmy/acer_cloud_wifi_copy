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

#ifndef __TS2_LINK_ENDPOINT_HPP__
#define __TS2_LINK_ENDPOINT_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>
#include <vpl_socket.h>
#include <vpl_th.h>
#include <vpl_thread.h>
#include <vpl_time.h>

#include <string>

#include "RouteType.hpp"
#include "LinkProtocol.hpp"

namespace Ts2 {
class Packet;
#ifndef __TS2_PACKETHANDLER_DEFINED__
// TODO: Move to a more appropriate place.
typedef void (*PacketHandler)(Packet*, void *context);
#define __TS2_PACKETHANDLER_DEFINED__
#endif

class LocalInfo;

namespace Link {

class EndPoint;
typedef void (*NotifyEndPointStatusChange)(const EndPoint *ep, void *context);

class EndPoint {
public:
    EndPoint(VPLSocket_t socket,
             u64 remoteUserId, u64 remoteDeviceId, u32 remoteInstanceId,
             const std::string *sessionKey, bool isServer,
             PacketHandler rcvdPacketHandler, void *rcvdPacketHandler_context,
             NotifyEndPointStatusChange notifyEndPointStatusChange, void *notifyEndPointStatusChange_context,
             RouteType routeType, LocalInfo *localInfo);
    ~EndPoint();

    u64 GetRemoteUserId() const;
    u64 GetRemoteDeviceId() const;
    u32 GetRemoteInstanceId() const;

    // Start activity.
    int Start();
    // Instruct the object to stop all activities.
    int Stop();

    bool IsValid() const;

    // Block until all activity is terminated.
    // When this function returns, it is safe to destroy this object.
    int WaitStopped();

    // When the obj is invalid, this functions tells the reason it became invalid.
    // At this time, "voluntary exit" means idle timeout had been reached.
    bool IsVoluntaryExit() const;

    // Send the packet using caller's thread.
    // Ownership of the packet remains with the caller.
    int Send(Packet *packet);

    VPLTime_t GetLastSentTimeStamp() const;
    VPLTime_t GetLastRcvdTimeStamp() const;

    VPLTime_t GetEstablishTimeStamp() const;

private:
    VPL_DISABLE_COPY_AND_ASSIGN(EndPoint);

    InterruptibleSocket *socket;
    const u64 remoteUserId;
    const u64 remoteDeviceId;
    const u32 remoteInstanceId;
    const PacketHandler rcvdPacketHandler;
    void *const rcvdPacketHandler_context;
    const NotifyEndPointStatusChange notifyEndPointStatusChange;
    void *const notifyEndPointStatusChange_context;
    const RouteType routeType;
    LocalInfo *const localInfo;

    mutable VPLMutex_t mutex;
    mutable VPLCond_t cond_state_noClients;
    mutable VPLMutex_t sockWrite_mutex;

    VPLTime_t idleTimeout;

    enum SocketState {
        SOCKET_STATE_OPEN = 0,  // socket is connected at TCP level
        SOCKET_STATE_READY,     // TS session is negotiated
        SOCKET_STATE_SHUTDOWN,  // TCP connection shut down
        SOCKET_STATE_CLOSED,    // socket is closed
    };
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
        State() : socket(SOCKET_STATE_OPEN), thread(THREAD_STATE_NOTHREAD), stop(STOP_STATE_NOSTOP), clients(0), keepalives(0) {}
        SocketState socket;
        ThreadState thread;
        StopState stop;
        int clients;  // num of client threads inside the obj
        int keepalives;  // num of keepalives sent
    } state;

    // Last time socket was used to send/receive any packet (either TS or LCP).
    VPLTime_t lastSentTimeStamp;
    VPLTime_t lastRcvdTimeStamp;

    // Last time socket was used to send or receive a TS packet.
    VPLTime_t lastTsTimeStamp;

    // The time this endpoint was created.
    VPLTime_t establishedTimeStamp;

    static const VPLTime_t SocketRecvTimeout;
    static const VPLTime_t SocketSendTimeout;

    // Maximum time the link can have no incoming packets.
    // After this much time has elapsed, the link will be considered disconnected.
    static const VPLTime_t NoInPacketTimeout;

    static const VPLTime_t KeepAlive_Time;
    static const VPLTime_t KeepAlive_Interval;
    static const int KeepAlive_Retry;

    bool exitReason_idleTimerExpired;
    bool exitReason_shutdownNoticeReceived;
    bool exitReason_connBad;

    static const size_t ThreadStackSize;
    VPLDetachableThreadHandle_t thread;
    void threadMain();
    static VPLTHREAD_FN_DECL threadMain(void *param);

    enum NegotiationState {
        NEGOTIATION_STATE_BEGIN = 0,
        NEGOTIATION_STATE_BEGINWAIT,
        NEGOTIATION_STATE_DONE,
    } negotiationState;

    const std::string *sessionKey;
    bool isServer;

    u8 signMode;
    u8 encMode;
    std::string signKey;
    std::string encKey;

    mutable VPLCond_t condNegotiation;
    u8 getSigningCapability();
    u8 getEncryptionCapability();
    void prepareNegotiation(Link::Protocol::LcpNegotiateSessionData& data);
    void revNegotiation(const Link::Protocol::LcpNegotiateSessionData& data);
};
} // end namespace Link
} // end namespace Ts2

#endif // incl guard
