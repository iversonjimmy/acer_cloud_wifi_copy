#ifndef __TS2_LINK_PROTOCOL_HPP__
#define __TS2_LINK_PROTOCOL_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>

#include <csltypes.h>
#include <InterruptibleSocket.hpp>

#include <string>

namespace Ts2 {
class Packet;
namespace Link {
class Protocol {
public:
    enum {
        PROTO_TS = 0x0001,
        PROTO_LCP = 0xc001,
    };

    enum {
        LCP_ECHO_REQ = 1,
        LCP_ECHO_RESP = 2,
        LCP_QUERY_PEER_REQ = 3,
        LCP_QUERY_PEER_RESP = 4,
        LCP_NEGOTIATE_SESSION_REQ = 5,
        LCP_NEGOTIATE_SESSION_RESP = 6,
        LCP_NOTIFY_SHUTDOWN = 7,
    };

#define LCP_NEGOTIATE_SESSION_KEY_MATERIAL_SIZE 64

#ifdef _MSC_VER
#pragma pack(push,1)
#endif
    struct 
#ifdef __GNUC__
    __attribute__((__packed__))
#endif
    LcpHeader {
        u8 code;
        u8 id;
        u16 length;
    };
#ifdef _MSC_VER
#pragma pack(pop)
#endif

#ifdef _MSC_VER
#pragma pack(push,1)
#endif
    struct 
#ifdef __GNUC__
    __attribute__((__packed__))
#endif
    LcpQueryPeerData {
        u64 userId;
        u64 deviceId;
        char instanceId[16];
    };
#ifdef _MSC_VER
#pragma pack(pop)
#endif

#ifdef _MSC_VER
#pragma pack(push,1)
#endif
    struct
#ifdef __GNUC__
    __attribute__((__packed__))
#endif
    LcpNegotiateSessionData {
        u8 signMode; // signing mode
        u8 encMode; // encryption mode
        u8 keyMaterialSize;
        u8 _pad;  // should be 0

        u8 iv[CSL_AES_IVSIZE_BYTES];
        u8 keyMaterial[LCP_NEGOTIATE_SESSION_KEY_MATERIAL_SIZE];
    };
#ifdef _MSC_VER
#pragma pack(pop)
#endif

#if !defined(__GNUC__) && !defined(_MSC_VER)
#error Unknown Compiler
#endif

    static int RecvProtocol(InterruptibleSocket *socket, u16 &protocol, VPLTime_t timeout);

    static int SendTsPacket(InterruptibleSocket *socket, Packet *packet, VPLTime_t timeout,
                            u8 sign_mode, u8 enc_mode,
                            const std::string& sign_key, const std::string& enc_key);
    static int RecvTsPacket(InterruptibleSocket *socket, Packet *&packet, VPLTime_t timeout,
                            u8 sign_mode, u8 enc_mode,
                            const std::string& sign_key, const std::string& enc_key);

    static int SendLcpPacket(InterruptibleSocket *socket, u8 code, u8 id, const std::string &data, VPLTime_t timeout);
    static int SendLcpPacket(InterruptibleSocket *socket, u8 code, u8 id, const void *data, size_t size, VPLTime_t timeout);
    static int RecvLcpPacket(InterruptibleSocket *socket, u8 &code, u8 &id, std::string &data, VPLTime_t timeout);

    static int SendLcpEchoReq(InterruptibleSocket *socket, u8 id, const std::string &data, VPLTime_t timeout);
    static int SendLcpEchoResp(InterruptibleSocket *socket, u8 id, const std::string &data, VPLTime_t timeout);

    static int SendLcpQueryPeerReq(InterruptibleSocket *socket, u8 id, VPLTime_t timeout);
    static int SendLcpQueryPeerResp(InterruptibleSocket *socket, u8 id, u64 userId, u64 deviceId, const std::string &instanceId, VPLTime_t timeout);

    static int SendLcpNegotiateSessionReq(InterruptibleSocket *socket, u8 id, const LcpNegotiateSessionData* param, VPLTime_t timeout);
    static int SendLcpNegotiateSessionResp(InterruptibleSocket *socket, u8 id, const LcpNegotiateSessionData* param, VPLTime_t timeout);

    static int SendLcpNotifyShutdown(InterruptibleSocket *socket, u8 id, VPLTime_t timeout);

    static ssize_t SendUntilDone(InterruptibleSocket *socket, const void *data, size_t datasize, VPLTime_t timeout);
    static ssize_t RecvUntilFull(InterruptibleSocket *socket, void *buf, size_t bufsize, VPLTime_t timeout);
};
}
}

#endif // incl guard
