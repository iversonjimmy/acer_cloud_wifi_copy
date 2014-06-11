#include "LinkProtocol.hpp"
#include <Packet.hpp>

#include <gvm_errors.h>
//#include <gvm_thread_utils.h>
#include <log.h>

#include <scopeguard.hpp>
#include <vpl_time.h>
#include <vplu_format.h>
//#include <vplu_mutex_autolock.hpp>

#include <cassert>

// *__no means the value is in network order.

int Ts2::Link::Protocol::RecvProtocol(InterruptibleSocket *socket, u16 &protocol, VPLTime_t timeout)
{
    int err = 0;

    u16 protocol__no = 0;
    ssize_t numBytesRcvd = RecvUntilFull(socket, &protocol__no, sizeof(protocol__no), timeout);
    if (numBytesRcvd < (ssize_t)sizeof(protocol__no)) {  // error, or too short
        if (numBytesRcvd < 0) {  // error
            err = numBytesRcvd;
            if (err != TS_ERR_TIMEOUT) {
                LOG_ERROR("Failed to receive Protocol: err %d", err);
            }
        }
        else {  // too short
            LOG_ERROR("Short receive of Protocol: "FMT_ssize_t" < "FMT_size_t, numBytesRcvd, sizeof(protocol__no));
            err = TS_ERR_COMM;
        }
    }
    else {
        protocol = VPLConv_ntoh_u16(protocol__no);
    }

    return err;
}

int Ts2::Link::Protocol::SendTsPacket(InterruptibleSocket *socket, Packet *packet, VPLTime_t timeout,
                                      u8 sign_mode, u8 enc_mode,
                                      const std::string& sign_key, const std::string& enc_key)
{
    int err = 0;
    u16 *buf = NULL;

    LOG_DEBUG("About to send Packet[%p]", packet);

    {
        size_t serializeBufSize = packet->GetSerializeBufSize();
        size_t u16bufSize = 1 + (serializeBufSize + 1) / 2 ;  // 1+ for protocol, +1 to round up
        buf = new (std::nothrow) u16 [u16bufSize];
        if (!buf) {
            LOG_ERROR("No memory for temporary buffer (size "FMT_size_t")", u16bufSize * sizeof(u16));
            err = TS_ERR_NO_MEM;
            goto end;
        }

        buf[0] = VPLConv_hton_u16(PROTO_TS);
        ssize_t usedBufSize = packet->Serialize(sign_mode,
                                                enc_mode,
                                                sign_key,
                                                enc_key,
                                                (u8*)&buf[1],
                                                serializeBufSize);
        if (usedBufSize < 0) {  // buffer too short
            // This should not happen, because we called GetSerialziedBufSize() for buffer size.
            LOG_ERROR("Failed to serialize packet: "FMT_ssize_t, usedBufSize);
            err = TS_ERR_INTERNAL;
            goto end;
        }

        size_t totalSendSize = 2 + usedBufSize;  // 2 = protocol
        ssize_t numBytesSent = SendUntilDone(socket, buf, totalSendSize, timeout);
        if (numBytesSent < (ssize_t)totalSendSize) {  // error, or too short
            if (numBytesSent < 0) {
                err = numBytesSent;
                LOG_ERROR("Failed to send TS Packet: err %d", err);
            }
            else {
                LOG_ERROR("Short send of TS Packet: "FMT_ssize_t" < "FMT_size_t, numBytesSent, totalSendSize);
            }
            goto end;
        }
    }

 end:
    LOG_DEBUG("Sent Packet[%p]: err %d", packet, err);
    if (buf) {
        delete [] buf;
    }
    return err;
}

int Ts2::Link::Protocol::RecvTsPacket(InterruptibleSocket *socket, Packet *&packet, VPLTime_t timeout,
                                      u8 sign_mode, u8 enc_mode,
                                      const std::string& sign_key, const std::string& enc_key)
{
    int err = 0;

    u8 *payload = NULL;
    {
        HeaderOnWire pktHeader;
        ssize_t numBytesRcvd = Protocol::RecvUntilFull(socket, &pktHeader, sizeof(pktHeader), timeout);
        if (numBytesRcvd < (ssize_t)sizeof(pktHeader)) {  // error, or too short
            if (numBytesRcvd < 0) {
                err = numBytesRcvd;
                LOG_ERROR("Failed to receive TS Packet header: err %d", err);
            }
            else {
                LOG_ERROR("Short receive of TS Packet header: "FMT_ssize_t" < "FMT_size_t, numBytesRcvd, sizeof(pktHeader));
                err = TS_ERR_COMM;
            }
            goto end;
        }

        size_t payloadSize = (size_t)(VPLConv_ntoh_u32(pktHeader.packetSize) - sizeof(pktHeader));
        payload = new (std::nothrow) u8 [payloadSize];
        if (!payload) {
            LOG_ERROR("No memory for packet payload (payloadSize "FMT_size_t")", payloadSize);
            err = TS_ERR_NO_MEM;
            goto end;
        }

        numBytesRcvd = Protocol::RecvUntilFull(socket, payload, payloadSize, timeout);
        if (numBytesRcvd < (ssize_t)payloadSize) {  // error, or too short
            if (numBytesRcvd < 0) {
                err = numBytesRcvd;
                LOG_ERROR("Failed to receive TS packet payload: err %d", err);
            }
            else {
                LOG_ERROR("Short receive of TS Packet payload: "FMT_ssize_t" < "FMT_size_t, numBytesRcvd, payloadSize);
                err = TS_ERR_COMM;
            }
            goto end;
        }

        // Verify and decrypt packet, if it needs
        size_t dataSize = VPLConv_ntoh_u32(pktHeader.dataSize);

        err = Ts2::Packet_verify_pkt(sign_mode,
                                     enc_mode,
                                     sign_key,
                                     enc_key,
                                     pktHeader,
                                     payload,
                                     payloadSize);

        if(err != TS_OK) {
            LOG_ERROR("Body signature bad or decryption failed");
            goto end;
        }

        packet = Packet::Deserialize(pktHeader, (u8*)payload, dataSize);
        if (!packet) {
            LOG_ERROR("No memory for Packet object");
            err = TS_ERR_NO_MEM;
            goto end;
        }
        // Ownership of memory block for payload transferred to Packet object.
        // Tailing padding and hash would be destroyed in upper layer too
        payload = NULL;
    }

 end:
    if (payload) {
        delete [] payload;
    }
    LOG_DEBUG("TS Packet received: err %d", err);
    return err;
}

int Ts2::Link::Protocol::SendLcpPacket(InterruptibleSocket *socket, u8 code, u8 id, const std::string &data, VPLTime_t timeout)
{
    return SendLcpPacket(socket, code, id, data.data(), data.size(), timeout);
}

int Ts2::Link::Protocol::SendLcpPacket(InterruptibleSocket *socket, u8 code, u8 id, const void *data, size_t size, VPLTime_t timeout)
{
    int err = 0;
    u16 *buf = NULL;

    {
        size_t totalSendSize = 2 + 4 + size;  // 2 = protocol, 4 = LCP header
        size_t u16bufSize = totalSendSize / 2 + 1;  // +1 to round up
        buf = new (std::nothrow) u16 [u16bufSize];
        if (!buf) {
            LOG_ERROR("No memory for temporary buffer (size "FMT_size_t")", 2 * u16bufSize);
            err = TS_ERR_NO_MEM;
            goto end;
        }

        buf[0] = VPLConv_hton_u16(PROTO_LCP);       // protocol
        buf[1] = VPLConv_hton_u16(code << 8 | id);  // LCP header: code, id fields
        buf[2] = VPLConv_hton_u16(4 + size);        // LCP header: length field
        memcpy(&buf[3], data, size);

        ssize_t numBytesSent = SendUntilDone(socket, buf, totalSendSize, timeout);
        if (numBytesSent < (ssize_t)totalSendSize) {  // error, or too short
            if (numBytesSent < 0) {  // error
                err = numBytesSent;
                LOG_ERROR("Failed to send LCP Packet: err %d", err);
            }
            else {  // too short
                LOG_ERROR("Short send of LCP Packet: "FMT_ssize_t" < "FMT_size_t, numBytesSent, totalSendSize);
                err = TS_ERR_COMM;
            }
            goto end;
        }
    }

 end:
    if (buf) {
        delete [] buf;
    }
    return err;
}

int Ts2::Link::Protocol::RecvLcpPacket(InterruptibleSocket *socket, u8 &code, u8 &id, std::string &data, VPLTime_t timeout)
{
    int err = 0;

    u16 length = 0;  // This is the LCP packet total size.
    // Receive (1B) Code, (1B) Id, (2B) Length fields.
    {
        LcpHeader lcpHeader__no;
        ssize_t numBytesRcvd = RecvUntilFull(socket, &lcpHeader__no, sizeof(lcpHeader__no), timeout);
        if (numBytesRcvd < (ssize_t)sizeof(lcpHeader__no)) {  // error, or too short
            if (numBytesRcvd < 0) {  // error
                err = numBytesRcvd;
                LOG_ERROR("Failed to receive LCP Packet header: err %d", err);
            }
            else {  // too short
                LOG_ERROR("Short receive of LCP Packet header: "FMT_ssize_t " < "FMT_size_t, numBytesRcvd, sizeof(lcpHeader__no));
                err = TS_ERR_COMM;
            }
            goto end;
        }

        code = lcpHeader__no.code;
        id = lcpHeader__no.id;
        length = VPLConv_ntoh_u16(lcpHeader__no.length);
        if (length < sizeof(LcpHeader)) {  // too small
            LOG_ERROR("Invalid LCP Length field %d", length);
            err = TS_ERR_INVALID;
            goto end;
        }
    }

    // Receive Data.
    if (length > sizeof(LcpHeader)) {  // Data starts after the LCP Packet header.
        size_t size = length - sizeof(LcpHeader);
        char *buf = new (std::nothrow) char [size];
        if (!buf) {
            LOG_ERROR("No memory for temporary buffer (size "FMT_size_t")", size);
            err = TS_ERR_NO_MEM;
            goto end;
        }
        ON_BLOCK_EXIT(deleteArray<char>, buf);

        ssize_t numBytesRcvd = RecvUntilFull(socket, buf, size, timeout);
        if (numBytesRcvd < (ssize_t)size) {  // error, or too short
            if (numBytesRcvd < 0) {  // error
                err = numBytesRcvd;
                LOG_ERROR("Failed to receive LCP Packet data: err %d", err);
            }
            else {  // too short
                LOG_ERROR("Short receive of LCP Packet data: "FMT_ssize_t" < "FMT_size_t, numBytesRcvd, size);
                err = TS_ERR_COMM;
            }
            goto end;
        }
        data.assign(buf, size);
    }

 end:
    return err;
}

int Ts2::Link::Protocol::SendLcpEchoReq(InterruptibleSocket *socket, u8 id, const std::string &data, VPLTime_t timeout)
{
    return SendLcpPacket(socket, LCP_ECHO_REQ, id, data, timeout);
}

int Ts2::Link::Protocol::SendLcpEchoResp(InterruptibleSocket *socket, u8 id, const std::string &data, VPLTime_t timeout)
{
    return SendLcpPacket(socket, LCP_ECHO_RESP, id, data, timeout);
}


int Ts2::Link::Protocol::SendLcpQueryPeerReq(InterruptibleSocket *socket, u8 id, VPLTime_t timeout)
{
    std::string empty;
    return SendLcpPacket(socket, LCP_QUERY_PEER_REQ, id, empty, timeout);
}

int Ts2::Link::Protocol::SendLcpQueryPeerResp(InterruptibleSocket *socket, u8 id, u64 userId, u64 deviceId, const std::string &instanceId, VPLTime_t timeout)
{
    LcpQueryPeerData data__no;
    memset(&data__no, 0, sizeof(data__no));
    data__no.userId = VPLConv_hton_u64(userId);
    data__no.deviceId = VPLConv_hton_u64(deviceId);
    memcpy(data__no.instanceId, instanceId.data(), MIN(instanceId.size(), 16));
    return SendLcpPacket(socket, LCP_QUERY_PEER_RESP, id, &data__no, sizeof(data__no), timeout);
}

int Ts2::Link::Protocol::SendLcpNegotiateSessionReq(InterruptibleSocket *socket, u8 id, const LcpNegotiateSessionData* param, VPLTime_t timeout)
{
    return SendLcpPacket(socket, LCP_NEGOTIATE_SESSION_REQ, id, (const char*)param, sizeof(LcpNegotiateSessionData), timeout);
}

int Ts2::Link::Protocol::SendLcpNegotiateSessionResp(InterruptibleSocket *socket, u8 id, const LcpNegotiateSessionData* param, VPLTime_t timeout)
{
    return SendLcpPacket(socket, LCP_NEGOTIATE_SESSION_RESP, id, (const char*)param, sizeof(LcpNegotiateSessionData), timeout);
}

int Ts2::Link::Protocol::SendLcpNotifyShutdown(InterruptibleSocket *socket, u8 id, VPLTime_t timeout)
{
    std::string empty;
    return SendLcpPacket(socket, LCP_NOTIFY_SHUTDOWN, id, empty, timeout);
}


ssize_t Ts2::Link::Protocol::SendUntilDone(InterruptibleSocket *socket, const void *data, size_t datasize, VPLTime_t timeout)
{
    int err = 0;
    size_t numBytesSentSoFar = 0;
    while (numBytesSentSoFar < datasize) {
        int numBytesJustSent = socket->Write((u8*)data + numBytesSentSoFar, datasize - numBytesSentSoFar, timeout);
        if (numBytesJustSent < 0) {  // error
            if (numBytesJustSent == VPL_ERR_CANCELED) {  // treat as error
                err = TS_ERR_COMM;
                goto end;
            }
            else if (numBytesJustSent == VPL_ERR_TIMEOUT) {
                // Msg logged by Write().
                err = TS_ERR_TIMEOUT;
                goto end;
            }
            else {
                err = numBytesJustSent;
                LOG_ERROR("Send() failed: err %d", err);
                goto end;
            }
        }
        numBytesSentSoFar += numBytesJustSent;
    } // while

 end:
    return err ? err : (ssize_t)numBytesSentSoFar;
}

ssize_t Ts2::Link::Protocol::RecvUntilFull(InterruptibleSocket *socket, void *buf, size_t bufsize, VPLTime_t timeout)
{
    int err = 0;

    size_t numBytesRcvdSoFar = 0;
    while (numBytesRcvdSoFar < bufsize) {
        int numBytesJustRcvd = socket->Read((u8*)buf + numBytesRcvdSoFar, bufsize - numBytesRcvdSoFar, timeout);
        if (numBytesJustRcvd < 0) {  // error
            if (numBytesJustRcvd == VPL_ERR_CANCELED) {  // treat as EOF
                goto end;
            }
            else if (numBytesJustRcvd == VPL_ERR_TIMEOUT) {
                // Msg logged by Read().
                err = TS_ERR_TIMEOUT;
                goto end;
            }
            else {
                err = numBytesJustRcvd;
                LOG_ERROR("Recv() failed: err %d", err);
                goto end;
            }
        }
        if (numBytesJustRcvd == 0) {  // socket disconnected
            LOG_INFO("No bytes received from socket");
            goto end;
        }

        numBytesRcvdSoFar += numBytesJustRcvd;
    } // while

 end:
    return err ? err : (ssize_t)numBytesRcvdSoFar;
}
