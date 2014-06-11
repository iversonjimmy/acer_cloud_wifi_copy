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

#include "EndPoint.hpp"
#include "LinkProtocol.hpp"
#include "LocalInfo.hpp"
#include "Packet.hpp"

#include <gvm_errors.h>
#include <gvm_thread_utils.h>
#include <log.h>
#ifdef IOS
#include <InterruptibleSocket_SigByCmdSocket.hpp>
#else
#include <InterruptibleSocket_SigBySockShutdown.hpp>
#endif

#include <vpl_time.h>
#include <vplu_format.h>
#include <vplu_mutex_autolock.hpp>
#include <vplex_math.h>

#include "cslsha.h" // SHA1 HMAC
#include "aes.h"    // AES128 encrypt/decrypt

#include <cassert>

// These values must agree with CONFIG_TS2_DEV_AID_PARAM__* in config.h.
#define DEV_AID_PARAM__DROP_TS_PACKETS 1
#define DEV_AID_PARAM__DUP_TS_PACKETS  2

#if !defined(__GNUC__) && !defined(_MSC_VER)
#error Unknown Compiler
#endif

#if CSL_SHA1_DIGESTSIZE + CSL_AES_KEYSIZE_BYTES > LCP_NEGOTIATE_SESSION_KEY_MATERIAL_SIZE
#error Not enough key material
#endif

#ifdef _MSC_VER
#pragma pack(push,1)
#endif
struct
#ifdef __GNUC__
__attribute__((__packed__))
#endif
KeysInKeyMaterial {
    u8 sign[CSL_SHA1_DIGESTSIZE];
    u8 enc[CSL_AES_KEYSIZE_BYTES];
};
#ifdef _MSC_VER
#pragma pack(pop)
#endif

const size_t Ts2::Link::EndPoint::ThreadStackSize = UTIL_DEFAULT_THREAD_STACK_SIZE;
const VPLTime_t Ts2::Link::EndPoint::SocketRecvTimeout = VPLTime_FromSec(10);
const VPLTime_t Ts2::Link::EndPoint::SocketSendTimeout = VPLTime_FromSec(10);

// Following scheme is optimized for TS-over-TCP.
// When we switch to UDP, we will need to retry.
const VPLTime_t Ts2::Link::EndPoint::KeepAlive_Time = VPLTime_FromSec(10);
const VPLTime_t Ts2::Link::EndPoint::KeepAlive_Interval = VPLTime_FromSec(5);
const int Ts2::Link::EndPoint::KeepAlive_Retry = 1;
const VPLTime_t Ts2::Link::EndPoint::NoInPacketTimeout = VPLTime_FromSec(15);

Ts2::Link::EndPoint::EndPoint(VPLSocket_t socket,
                              u64 remoteUserId, u64 remoteDeviceId, u32 remoteInstanceId,
                              const std::string *sessionKey, bool isServer,
                              PacketHandler rcvdPacketHandler, void *rcvdPacketHandler_context,
                              NotifyEndPointStatusChange notifyEndPointStatusChange, void *notifyEndPointStatusChange_context,
                              RouteType routeType, LocalInfo *localInfo)
    : 
#ifdef IOS
    socket(new (std::nothrow) InterruptibleSocket_SigByCmdSocket(socket)),
#else
    socket(new (std::nothrow) InterruptibleSocket_SigBySockShutdown(socket)),
#endif
      remoteUserId(remoteUserId), remoteDeviceId(remoteDeviceId), remoteInstanceId(remoteInstanceId),
      rcvdPacketHandler(rcvdPacketHandler), rcvdPacketHandler_context(rcvdPacketHandler_context),
      notifyEndPointStatusChange(notifyEndPointStatusChange), notifyEndPointStatusChange_context(notifyEndPointStatusChange_context),
      routeType(routeType), localInfo(localInfo),
      lastSentTimeStamp(0), lastRcvdTimeStamp(0), lastTsTimeStamp(0), establishedTimeStamp(VPLTime_GetTimeStamp()),
      exitReason_idleTimerExpired(false), exitReason_shutdownNoticeReceived(false), exitReason_connBad(false),
      negotiationState(NEGOTIATION_STATE_BEGIN), sessionKey(sessionKey), isServer(isServer),
      signMode(Ts2::Security::SIGNING_MODE_NONE), encMode(Ts2::Security::ENCRYPT_MODE_NONE)
{
    if (!this->socket) {
        LOG_ERROR("EndPoint[%p]: Failed to create Socket obj", this);
        // TODO: Put obj in some invalid state so it cannot be used.
    }

    VPLMutex_Init(&mutex);
    VPLCond_Init(&cond_state_noClients);
    VPLMutex_Init(&sockWrite_mutex);
    VPLCond_Init(&condNegotiation);

    switch (routeType) {
    case ROUTE_TYPE_DIN:
        idleTimeout = VPLTime_FromSec(localInfo->GetIdleDinTimeout());
        break;
    case ROUTE_TYPE_DEX:
        idleTimeout = VPLTime_FromSec(localInfo->GetIdleDexTimeout());
        break;
    case ROUTE_TYPE_P2P:
        idleTimeout = VPLTime_FromSec(localInfo->GetIdleP2pTimeout());
        break;
    case ROUTE_TYPE_PRX:
        idleTimeout = VPLTime_FromSec(localInfo->GetIdlePrxTimeout());
        break;
    case ROUTE_TYPE_INVALID:
    default:
        LOG_ERROR("EndPoint[%p]: Invalid RouteType %d", this, routeType);
        idleTimeout = 0;
    }

    LOG_INFO("EndPoint[%p]: Created for socket["FMT_VPLSocket_t"], to UDI<"FMTu64","FMTu64","FMTu32">, by RouteType %d",
             this, VAL_VPLSocket_t(socket), remoteUserId, remoteDeviceId, remoteInstanceId, routeType);

    int yes = 1;
    // Enable TCP keep-alive.
    // After 30 seconds of inactivity, send at most 3 keep-alive packets, 10 seconds apart.
    int err = VPLSocket_SetKeepAlive(socket, yes, 30, 10, 3);
    if (err) {
        LOG_WARN("EndPoint[%p]: Failed to enable TCP KeepAlive: err %d", this, err);
    }
}

Ts2::Link::EndPoint::~EndPoint()
{
    if (state.socket != SOCKET_STATE_SHUTDOWN && state.socket != SOCKET_STATE_CLOSED) {
        socket->StopIo();
        state.socket = SOCKET_STATE_SHUTDOWN;
    }
    if (state.socket != SOCKET_STATE_CLOSED) {
        delete socket;  // Destroying the InterruptibleSocket has the side effect of closing the socket inside it.
        state.socket = SOCKET_STATE_CLOSED;
    }

    // Its EndPoint's responsibility to free memory
    if(sessionKey) {
        delete sessionKey;
    }

    VPLMutex_Destroy(&sockWrite_mutex);
    VPLCond_Destroy(&cond_state_noClients);
    VPLMutex_Destroy(&mutex);
    VPLCond_Destroy(&condNegotiation);

    LOG_INFO("EndPoint[%p]: Destroyed", this);
}

u64 Ts2::Link::EndPoint::GetRemoteUserId() const
{
    MutexAutoLock lock(&mutex);
    if (state.stop == STOP_STATE_STOPPING) {
        LOG_WARN("EndPoint[%p]: Method called after Stop", this);
    }
    return remoteUserId;
}

u64 Ts2::Link::EndPoint::GetRemoteDeviceId() const
{
    MutexAutoLock lock(&mutex);
    if (state.stop == STOP_STATE_STOPPING) {
        LOG_WARN("EndPoint[%p]: Method called after Stop", this);
    }
    return remoteDeviceId;
}

u32 Ts2::Link::EndPoint::GetRemoteInstanceId() const
{
    MutexAutoLock lock(&mutex);
    if (state.stop == STOP_STATE_STOPPING) {
        LOG_WARN("EndPoint[%p]: Method called after Stop", this);
    }
    return remoteInstanceId;
}

int Ts2::Link::EndPoint::Start()
{
    int err = 0;

    {
        MutexAutoLock lock(&mutex);

        // Check state and make transition.
        if (state.stop != STOP_STATE_NOSTOP) {
            LOG_ERROR("EndPoint[%p]: Wrong stop state %d", this, state.thread);
            err = TS_ERR_WRONG_STATE;
            goto end;
        }
        if (state.thread != THREAD_STATE_NOTHREAD) {
            LOG_ERROR("EndPoint[%p]: Wrong thread state %d", this, state.thread);
            err = TS_ERR_WRONG_STATE;
            goto end;
        }
        state.thread = THREAD_STATE_SPAWNING;

        err = Util_SpawnThread(threadMain, this, ThreadStackSize, /*isJoinable*/VPL_TRUE, &thread);
        if (err) {
            LOG_ERROR("EndPoint[%p]: Failed to spawn thread: err %d", this, err);
            state.thread = THREAD_STATE_NOTHREAD;
            goto end;
        }

        err = VPLCond_TimedWait(&condNegotiation, &mutex, SocketRecvTimeout);
        if(negotiationState != NEGOTIATION_STATE_DONE) {
            LOG_ERROR("EndPoint[%p]: Failed to negotiate: err %d, or it's being shut down", this, err);
            goto end;
        }
    }

 end:
    return err;
}

int Ts2::Link::EndPoint::Stop()
{
    int err = 0;

    LOG_INFO("EndPoint[%p]: Stop request received", this);

    {
        MutexAutoLock lock(&mutex);

        // Check state and make transition.
        if (state.stop == STOP_STATE_STOPPING) {
            LOG_WARN("EndPoint[%p]: Stop state is already %d", this, state.stop);
            return 0;
        }
        state.stop = STOP_STATE_STOPPING;

        if (state.socket == SOCKET_STATE_OPEN || state.socket == SOCKET_STATE_READY) {
            {
                MutexAutoLock lock(&sockWrite_mutex);
                if(!exitReason_connBad) {
                    err = Link::Protocol::SendLcpNotifyShutdown(socket, /*id*/0, SocketSendTimeout);
                    if (err) {
                        LOG_ERROR("EndPoint[%p]: Failed to send LCP NotifyShutdown: err %d", this, err);
                        err = 0;  // reset error
                        exitReason_connBad = true;
                    }
                    else {
                        LOG_INFO("EndPoint[%p]: Sent LCP NotifyShutdown", this);
                    }
                }
            }

            // This is necessary to unblock recv thread.
            socket->StopIo();
            state.socket = SOCKET_STATE_SHUTDOWN;
        }

        VPLCond_Broadcast(&condNegotiation);
    }

    LOG_INFO("EndPoint[%p]: Stop request propagated: err %d", this, err);

    return err;
}

bool Ts2::Link::EndPoint::IsValid() const
{
    MutexAutoLock lock(&mutex);
    return !(exitReason_idleTimerExpired || exitReason_shutdownNoticeReceived || exitReason_connBad) &&
        (state.socket == SOCKET_STATE_OPEN || state.socket == SOCKET_STATE_READY) &&
        (state.thread == THREAD_STATE_SPAWNING || state.thread == THREAD_STATE_RUNNING) &&
        (state.stop == STOP_STATE_NOSTOP);
}

bool Ts2::Link::EndPoint::IsVoluntaryExit() const
{
    MutexAutoLock lock(&mutex);
    return (exitReason_idleTimerExpired || exitReason_shutdownNoticeReceived) && !exitReason_connBad;
}

int Ts2::Link::EndPoint::WaitStopped()
{
    int err = 0;

    LOG_INFO("EndPoint[%p]: Waiting for obj to Stop", this);

    {
        MutexAutoLock lock(&mutex);

        // Check state.
        if (state.stop != STOP_STATE_STOPPING) {
            LOG_ERROR("EndPoint[%p]: Wrong stop state %d", this, state.stop);
            err = TS_ERR_WRONG_STATE;
            goto end;
        }

        while (state.clients > 0) {
            LOG_INFO("EndPoint[%p]: Waiting for %d client threads to leave", this, state.clients);
            int _err = VPLCond_TimedWait(&cond_state_noClients, &mutex, VPL_TIMEOUT_NONE);
            if (_err) {
                LOG_ERROR("EndPoint[%p]: CondVar failed: err %d", this, _err);
                // We could not determine how many threads are remaining inside this object.
                // This means it is not safe to destroy this object.
                // Action: Propagate error, indicating it is not safe to destroy this object.
                if (!err) {
                    err = _err;
                }
                break;
            }
        }
    }

    VPLMutex_Lock(&mutex);
    if (state.thread != THREAD_STATE_NOTHREAD) {
        LOG_INFO("EndPoint[%p]: Joining worker thread", this);
        VPLMutex_Unlock(&mutex);
        int _err = VPLDetachableThread_Join(&thread);
        VPLMutex_Lock(&mutex);
        if (_err) {
            LOG_ERROR("EndPoint[%p]: Failed to Join thread: err %d", this, err);
            // We could not join the receiver thread.
            // This means it is not safe to destroy this object.
            // Action: Propagate error, indicating it is not safe to destroy this object.
            if (!err) {
                err = _err;
            }
        }
        else {
            state.thread = THREAD_STATE_NOTHREAD;
        }
    }

 end:
    LOG_INFO("EndPoint[%p]: Obj has Stopped: err %d", this, err);

    return err;
}

int Ts2::Link::EndPoint::Send(Packet *packet)
{
    int err = 0;

    {
        MutexAutoLock lock(&mutex);
        state.clients++;
        // State check.
        if (state.stop == STOP_STATE_STOPPING) {
            LOG_WARN("EndPoint[%p]: Method called after Stop", this);
        }
        if (state.stop != STOP_STATE_NOSTOP) {
            LOG_ERROR("EndPoint[%p]: Wrong stop state %d", this, state.stop);
            err = TS_ERR_WRONG_STATE;
            goto end;
        }
        if (state.thread != THREAD_STATE_SPAWNING && state.thread != THREAD_STATE_RUNNING) {
            // Receiver thread is not available -> fail.
            // Rationale:
            // While the send operation is not impacted by the lack of receiver thread,
            // without the receiver thread, no response will ever be received.
            // Thus, it is better to fail now than later.
            LOG_ERROR("EndPoint[%p]: Wrong thread state %d", this, state.thread);
            err = TS_ERR_WRONG_STATE;
            goto end;
        }
    }

    do {
        if (localInfo->GetDevAidParam() & DEV_AID_PARAM__DROP_TS_PACKETS) {
            const int dropEveryNthPacket = localInfo->GetPktDropParam();  // simulate packet drop eveny certain packets
            static int counter = 0;
#if 0
            if ((++counter >= dropEveryNthPacket) &&
                ((packet->GetPktType() == TS_PKT_DATA) ||
                 ((packet->GetPktType() == TS_PKT_ACK) &&
                  (packet->GetAckNum() > 10)))) {
#else
            if (++counter >= dropEveryNthPacket) {
#endif
                LOG_INFO("DEVAID: dropping Packet[%p], pkt type:"FMTu8, packet, packet->GetPktType());
                counter = 0;
                break; // to skip SendTsPacket()
            }
        }

        {
            MutexAutoLock lock(&sockWrite_mutex);
            err = Protocol::SendTsPacket(socket, packet, SocketSendTimeout,
                                         signMode, encMode, signKey, encKey);
        }
        if (err) {
            LOG_ERROR("EndPoint[%p]: Failed to send TS packet: %d", this, err);
            MutexAutoLock lock(&mutex);
            exitReason_connBad = true;
            goto end;
        }

        if (localInfo->GetDevAidParam() & DEV_AID_PARAM__DUP_TS_PACKETS) {
            const int dupEveryNthPacket = 20;  // simulate 5% dup rate
            static int counter = 0;
            if ((++counter >= dupEveryNthPacket) &&
                ((packet->GetPktType() == TS_PKT_DATA) ||
                 ((packet->GetPktType() == TS_PKT_ACK) &&
                  (packet->GetAckNum() > 10)))) {
                LOG_INFO("DEVAID: duping Packet[%p]", packet);
                counter = 0;
                {
                    MutexAutoLock lock(&sockWrite_mutex);
                    err = Protocol::SendTsPacket(socket, packet, SocketSendTimeout,
                                                 signMode, encMode, signKey, encKey);
                }
                if (err) {
                    LOG_ERROR("EndPoint[%p]: Failed to send (duped) TS packet: %d", this, err);
                    err = 0;  // reset error
                }
            }
        }
    } while (0);
    {
        MutexAutoLock lock(&mutex);
        lastTsTimeStamp = lastSentTimeStamp = VPLTime_GetTimeStamp();
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

VPLTime_t Ts2::Link::EndPoint::GetLastSentTimeStamp() const
{
    MutexAutoLock lock(&mutex);
    if (state.stop == STOP_STATE_STOPPING) {
        LOG_WARN("EndPoint[%p]: Method called after Stop", this);
    }
    return lastSentTimeStamp;
}

VPLTime_t Ts2::Link::EndPoint::GetLastRcvdTimeStamp() const
{
    MutexAutoLock lock(&mutex);
    if (state.stop == STOP_STATE_STOPPING) {
        LOG_WARN("EndPoint[%p]: Method called after Stop", this);
    }
    return lastRcvdTimeStamp;
}

VPLTime_t Ts2::Link::EndPoint::GetEstablishTimeStamp() const
{
    return establishedTimeStamp;
}

VPLTHREAD_FN_DECL Ts2::Link::EndPoint::threadMain(void *param)
{
    EndPoint *socket = static_cast<EndPoint*>(param);
    socket->threadMain();
    return VPLTHREAD_RETURN_VALUE;
}

void Ts2::Link::EndPoint::threadMain()
{
    {
        MutexAutoLock lock(&mutex);
        state.thread = THREAD_STATE_RUNNING;
    }

    if(!isServer) {
        int err = TS_OK;
        Link::Protocol::LcpNegotiateSessionData localOffer;
        prepareNegotiation(localOffer);
        {
            MutexAutoLock lock(&sockWrite_mutex);
            err = Protocol::SendLcpNegotiateSessionReq(socket, /*id*/0, &localOffer, SocketSendTimeout);
        }
        if (err) {
            LOG_ERROR("EndPoint[%p]: Failed to send LCP NegotiateSession response: err %d", this, err);
            MutexAutoLock lock(&mutex);
            exitReason_connBad = true;
            goto end;
        }
    }
    {
        MutexAutoLock lock(&mutex);
        negotiationState = NEGOTIATION_STATE_BEGINWAIT;
        // A new established socket for reading (like repaired by Network layer), there might be nothing to send
        // Initiate a timestamp value at beginning
        lastTsTimeStamp = VPLTime_GetTimeStamp();
    }

    while (1) {
        u16 protocol = 0;
        VPLTime_t timeout = state.keepalives == 0 ? KeepAlive_Time : KeepAlive_Interval;
        int err = Protocol::RecvProtocol(socket, protocol, timeout);
        {
            MutexAutoLock lock(&mutex);
            if (state.stop == STOP_STATE_STOPPING) {
                LOG_INFO("EndPoint[%p]: Stopping state detected", this);
                goto end;
            }
        }
        if (err == TS_ERR_TIMEOUT) {
            VPLTime_t now = VPLTime_GetTimeStamp();
            // No incoming packets recently.
            // (1) If this EndPoint has not been used with a TS packet for time specified in ccd.conf, then exit.
            if (idleTimeout > 0 && now > lastTsTimeStamp + idleTimeout) {
                LOG_INFO("EndPoint[%p]: Idle for "FMTu64" secs - exiting", this, VPLTime_ToSec(now - lastTsTimeStamp));
                exitReason_idleTimerExpired = true;
                goto end;
            }
            // (2) If this EndPoint has not received any packet for NoInPacketTimeout,
            //     then consider the link dead and exit.
            if (now > lastRcvdTimeStamp + NoInPacketTimeout) {
                LOG_INFO("EndPoint[%p]: No incoming packet in "FMTu64" secs - considering disconnected", this, VPLTime_ToSec(now - lastRcvdTimeStamp));
                MutexAutoLock lock(&mutex);
                exitReason_connBad = true;
                goto end;
            }
            // (3) Otherwise, send out an EchoReq to test if the link is still good.
            {
                std::string data;
                {
                    MutexAutoLock lock(&sockWrite_mutex);
                    err = Link::Protocol::SendLcpEchoReq(socket, /*id*/0, data, SocketSendTimeout);
                }
                if (err) {
                    LOG_ERROR("EndPoint[%p]: Failed to send LCP Echo request: err %d", this, err);
                    MutexAutoLock lock(&mutex);
                    exitReason_connBad = true;
                    goto end;
                }
                {
                    MutexAutoLock lock(&mutex);
                    lastSentTimeStamp = VPLTime_GetTimeStamp();
                    state.keepalives++;
                }
                LOG_INFO("EndPoint[%p]: Sent LCP Echo request", this);
            }
            continue;
        }
        else if (err) {
            LOG_ERROR("EndPoint[%p]: Failed to receive protocol field: err %d", this, err);
            MutexAutoLock lock(&mutex);
            exitReason_connBad = true;
            goto end;
        }

        switch (protocol) {
        case Link::Protocol::PROTO_TS:
            {
                Ts2::Packet *packet = NULL;
                err = Protocol::RecvTsPacket(socket, packet, SocketRecvTimeout,
                                             signMode, encMode, signKey, encKey);
                {
                    MutexAutoLock lock(&mutex);
                    if (state.stop == STOP_STATE_STOPPING) {
                        LOG_INFO("EndPoint[%p]: Stopping state detected", this);
                        if (packet) {
                            delete packet;
                        }
                        goto end;
                    }
                }
                if (err) {
                    LOG_ERROR("EndPoint[%p]: Failed to receive TS packet: err %d", this, err);
                    MutexAutoLock lock(&mutex);
                    exitReason_connBad = true;
                    goto end;
                }
                assert(packet);
                {
                    MutexAutoLock lock(&mutex);
                    lastTsTimeStamp = lastRcvdTimeStamp = VPLTime_GetTimeStamp();
                }
                if (rcvdPacketHandler) {
                    (*rcvdPacketHandler)(packet, rcvdPacketHandler_context);
                    // Ownership of packet transferred to recvPacketHandler().
                    packet = NULL;
                }
                if (packet) {
                    delete packet;
                }
            }
            break;
        case Link::Protocol::PROTO_LCP:
            {
                u8 code = 0, id = 0;
                std::string data;
                err = Protocol::RecvLcpPacket(socket, code, id, data, SocketRecvTimeout);
                if (err) {
                    LOG_ERROR("EndPoint[%p]: Failed to receive LCP packet: err %d", this, err);
                    MutexAutoLock lock(&mutex);
                    exitReason_connBad = true;
                    goto end;
                }
                {
                    MutexAutoLock lock(&mutex);
                    lastRcvdTimeStamp = VPLTime_GetTimeStamp();
                }
                LOG_INFO("EndPoint[%p]: LCP Code %d Id %d", this, code, id);
                switch (code) {
                case Link::Protocol::LCP_ECHO_REQ:
                    LOG_INFO("EndPoint[%p]: Received LCP Echo request", this);
                    {
                        MutexAutoLock lock(&sockWrite_mutex);
                        err = Protocol::SendLcpEchoResp(socket, /*id*/0, data, SocketSendTimeout);
                    }
                    if (err) {
                        LOG_ERROR("EndPoint[%p]: Failed to send LCP Echo response: err %d", this, err);
                        MutexAutoLock lock(&mutex);
                        exitReason_connBad = true;
                        goto end;
                    }
                    LOG_INFO("EndPoint[%p]: Sent LCP Echo response", this);
                    break;
                case Link::Protocol::LCP_ECHO_RESP:
                    LOG_INFO("EndPoint[%p]: Received LCP Echo response", this);
                    {
                        MutexAutoLock lock(&mutex);
                        state.keepalives = 0;
                    }
                    break;
                case Link::Protocol::LCP_NEGOTIATE_SESSION_REQ:
                    LOG_INFO("EndPoint[%p]: Received LCP NegotiateSession request", this);
                    {
                        {
                            MutexAutoLock lock(&mutex);
                            if(negotiationState != NEGOTIATION_STATE_BEGINWAIT) {
                                LOG_WARN("EndPoint[%p]: Unexpected negotiation state: %d", this, negotiationState);
                                break;
                            }
                        }

                        const Link::Protocol::LcpNegotiateSessionData* remoteOffer =
                                                    (const Link::Protocol::LcpNegotiateSessionData*)data.data();
                        revNegotiation(*remoteOffer);

                        Link::Protocol::LcpNegotiateSessionData localOffer;
                        prepareNegotiation(localOffer);
                        {
                            MutexAutoLock lock(&sockWrite_mutex);
                            err = Protocol::SendLcpNegotiateSessionResp(socket, /*id*/0, &localOffer, SocketSendTimeout);
                        }
                    }
                    if (err) {
                        LOG_ERROR("EndPoint[%p]: Failed to send LCP NegotiateSession response: err %d", this, err);
                        MutexAutoLock lock(&mutex);
                        exitReason_connBad = true;
                        goto end;
                    }
                    LOG_INFO("EndPoint[%p]: Sent LCP NegotiateSession response", this);
                    {
                        MutexAutoLock lock(&mutex);
                        negotiationState = NEGOTIATION_STATE_DONE;
                        state.socket = SOCKET_STATE_READY;
                        VPLCond_Broadcast(&condNegotiation);
                    }
                    break;
                case Link::Protocol::LCP_NEGOTIATE_SESSION_RESP:
                    LOG_INFO("EndPoint[%p]: Received LCP NegotiateSession response", this);
                    {
                        MutexAutoLock lock(&mutex);
                        if(negotiationState != NEGOTIATION_STATE_BEGINWAIT) {
                            LOG_WARN("EndPoint[%p]: Unexpected negotiation state: %d", this, negotiationState);
                            break;
                        }

                        const Link::Protocol::LcpNegotiateSessionData* remoteOffer =
                            (const Link::Protocol::LcpNegotiateSessionData*)data.data();
                        revNegotiation(*remoteOffer);

                        negotiationState = NEGOTIATION_STATE_DONE;
                        state.socket = SOCKET_STATE_READY;
                        VPLCond_Broadcast(&condNegotiation);
                    }
                    break;
                case Link::Protocol::LCP_NOTIFY_SHUTDOWN:
                    LOG_INFO("EndPoint[%p]: Received LCP Shutdown Notice", this);
                    {
                        MutexAutoLock lock(&mutex);
                        exitReason_shutdownNoticeReceived = true;
                        goto end;
                    }
                    break;
                default:
                    LOG_WARN("EndPoint[%p]: Unexpected LCP packet ignored", this);
                }
            }
            break;
        default:
            LOG_ERROR("EndPoint[%p]: Unknown protocol %d", this, protocol);
            goto end;
        } // switch (protocol)
    } // while (1)

 end:
    LOG_INFO("EndPoint[%p]: Service thread exited main loop", this);

    {
        MutexAutoLock lock(&mutex);

        if (state.socket == SOCKET_STATE_OPEN || state.socket == SOCKET_STATE_READY) {
            if (!exitReason_shutdownNoticeReceived && !exitReason_connBad) {
                MutexAutoLock lock(&sockWrite_mutex);
                int err = Link::Protocol::SendLcpNotifyShutdown(socket, /*id*/0, SocketSendTimeout);
                if (err) {
                    LOG_ERROR("EndPoint[%p]: Failed to send LCP NotifyShutdown: err %d", this, err);
                    MutexAutoLock lock(&mutex);
                    exitReason_connBad = true;
                }
                else {
                    LOG_INFO("EndPoint[%p]: Sent LCP NotifyShutdown", this);
                }
            }

            socket->StopIo();
            state.socket = SOCKET_STATE_SHUTDOWN;
        }

        state.thread = THREAD_STATE_EXITING;
    }

    if (notifyEndPointStatusChange) {
        (*notifyEndPointStatusChange)(this, notifyEndPointStatusChange_context);
    }

    LOG_INFO("EndPoint[%p]: Service thread exiting", this);
}

u8 Ts2::Link::EndPoint::getSigningCapability()
{
    u8 mode = Ts2::Security::SIGNING_MODE_FULL;
    if(localInfo->GetPlatformType() == Ts2::LocalInfo::PLATFORM_TYPE_CLOUDNODE &&
       routeType == ROUTE_TYPE_DIN) {
        mode = Ts2::Security::SIGNING_MODE_HEADER_ONLY;
    }
    return mode;
}

u8 Ts2::Link::EndPoint::getEncryptionCapability()
{
    u8 mode = Ts2::Security::ENCRYPT_MODE_BODY;
    if(localInfo->GetPlatformType() == Ts2::LocalInfo::PLATFORM_TYPE_CLOUDNODE &&
       routeType == ROUTE_TYPE_DIN) {
        mode = Ts2::Security::ENCRYPT_MODE_NONE;
    }
    return mode;
}

static void encrypt_data(u8* dest, const u8* src, size_t len,
                         const u8* iv, const std::string& key)
{
    int rc;

    rc = aes_SwEncrypt((u8*)key.data(), const_cast<u8*>(iv), const_cast<u8*>(src), len, dest);
    if (rc != 0) {
        LOG_ERROR("Failed to encrypt data. - %d", rc);
    }
}

static void decrypt_data(u8* dest, const u8* src, size_t len,
                         const u8* iv, const std::string& key)
{
    int rc;

    rc = aes_SwDecrypt((u8*)key.data(), const_cast<u8*>(iv), const_cast<u8*>(src), len, dest);
    if (rc != 0) {
        LOG_ERROR("Failed to decrypt data - %d.", rc);
    }
}

static void randomGen(u8* buf, size_t len, u32 offset)
{
    static int init = VPLMath_InitRand();
    UNUSED(init);
    for(u32 i = offset, j = len; j > 0;) {
        buf[i] = VPLMath_Rand() % 0x100;
        i++;
        j--;
    }
}

void Ts2::Link::EndPoint::prepareNegotiation(Link::Protocol::LcpNegotiateSessionData& data)
{
    memset(&data, 0, sizeof(data));
    data.signMode = getSigningCapability();
    data.encMode = getEncryptionCapability();
    data.keyMaterialSize = ARRAY_SIZE_IN_BYTES(data.keyMaterial);

    randomGen(data.iv, CSL_AES_IVSIZE_BYTES, 0);

    if(!isServer) {
        MutexAutoLock lock(&mutex);
        u8 keyMaterial[LCP_NEGOTIATE_SESSION_KEY_MATERIAL_SIZE];
        randomGen(keyMaterial, ARRAY_SIZE_IN_BYTES(keyMaterial), 0);
        const KeysInKeyMaterial *keys = (KeysInKeyMaterial*)keyMaterial;
        signKey.assign((const char*)keys->sign, ARRAY_SIZE_IN_BYTES(keys->sign));
        encKey.assign((const char*)keys->enc, ARRAY_SIZE_IN_BYTES(keys->enc));

        // Encrypt it
        encrypt_data(data.keyMaterial, keyMaterial, ARRAY_SIZE_IN_BYTES(keyMaterial),
                     data.iv, *sessionKey);
    } else {
        randomGen(data.keyMaterial, ARRAY_SIZE_IN_BYTES(data.keyMaterial), 0);
    }
}

void Ts2::Link::EndPoint::revNegotiation(const Link::Protocol::LcpNegotiateSessionData& data)
{
    MutexAutoLock lock(&mutex);

    signMode = data.signMode < getSigningCapability() ? data.signMode : getSigningCapability();

    encMode = data.encMode < getEncryptionCapability() ? data.encMode : getEncryptionCapability();

    LOG_INFO("EndPoint[%p]: SignMode - %d, EncMode - %d", this, signMode, encMode);

    if(isServer) {
        u8 keyMaterial[LCP_NEGOTIATE_SESSION_KEY_MATERIAL_SIZE];
        decrypt_data(keyMaterial, data.keyMaterial, ARRAY_SIZE_IN_BYTES(data.keyMaterial), data.iv, *sessionKey);
        const KeysInKeyMaterial *keys = (KeysInKeyMaterial*)keyMaterial;
        // Get the key, if it needs
        if(signMode != Ts2::Security::SIGNING_MODE_NONE) {
            //signKey = data.signKey;
            signKey.assign((const char*)keys->sign, ARRAY_SIZE_IN_BYTES(keys->sign));
        }

        if(encMode != Ts2::Security::ENCRYPT_MODE_NONE) {
            //encKey = data.encKey;
            encKey.assign((const char*)keys->enc, ARRAY_SIZE_IN_BYTES(keys->enc));
        }
    }

    if(signMode == Ts2::Security::SIGNING_MODE_NONE) {
        signKey.clear();
    }

    if(encMode == Ts2::Security::ENCRYPT_MODE_NONE) {
        encKey.clear();
    }
}

