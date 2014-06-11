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
/// Packet.hpp
///
/// Tunnel Service Packet Defs

#ifndef __TS2_PACKET_HPP__
#define __TS2_PACKET_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>
#include "vpl_time.h"
#include "vpl_conv.h"
#include "vplex_trace.h"
#include "ts_types.hpp"

#include "cslsha.h" // SHA1 HMAC
#include "aes.h"    // AES128 encrypt/decrypt

#include <cassert>

namespace Ts2 {

// Ts2 packet version number
#define TS_PKT_VERSION               1
#define TS_HMAC_SIZE                10
#define TS_CODEC_UNIT               16

namespace Security {
/// Signing mode of a tunnel
/// A u8 value in LcpNegotiateSessionData is used to present the signing capability of a device
enum {
    SIGNING_MODE_NONE = 0,
    SIGNING_MODE_HEADER_ONLY = 1,
    SIGNING_MODE_FULL = 2
};

// Encryption mode of a tunnel
/// A u8 value in LcpNegotiateSessionData is used to present the encryption capability of a device
enum {
    ENCRYPT_MODE_NONE = 0,
    ENCRYPT_MODE_BODY = 1
};

/// Key Length
enum {
    SIGNING_KEY_LENGTH = CSL_SHA1_DIGESTSIZE,
    ENCRYPT_KEY_LENGTH = CSL_AES_KEYSIZE_BYTES // AES 128 bit
};
} // namespace Security

/***
 * Current Packet Format: A packet would be serialized into the following format on the wire
 *
 * | HEADER(68) | DATA(optional) | PADDING(0 ~ 15, optional) | SIGNATURE(10, optional) |
 *
 * Total size cannot exceed U32_MAX.
 */

// http://www.ctbg.acer.com/wiki/index.php/User:Fokushi/Ts2/PacketHeaderFormat
#ifndef _MSC_VER
struct __attribute__((__packed__)) HeaderOnWire {
#else
#pragma pack(push,1)
struct HeaderOnWire {
#endif
    u8 version;
    u8 type;

    u8     flag[2];
    // flag[0]:
    //   xxxxxxx1 = header is included in MAC using HMAC-SHA1
    //   xxxxxx1x = data is included in MAC using HMAC-SHA1
    //  (xxxxxx11 = header and data is included in MAC using HMAC-SHA1)
    //   xxxxx1xx = data is encrypted using AES-128-CBC
    // flag[1]:
    //   unassigned

    u32 packetSize;  // packet len = header size + data section size + padding data + hash size
    u64 srcDevId;
    u32 srcInstId; 
    u32 srcVtId;
    u64 dstDevId;
    u32 dstInstId;
    u32 dstVtId;
    u64 seqNum;
    u64 ackNum;
    u32 windowSize;
    u32 dataSize;  // actual data size (without any padding); 0 if none
    u32 reserved;
};
#ifdef _MSC_VER
#pragma pack(pop)
#endif

enum {
    TS_PKT_SYN = 1,
    TS_PKT_SYN_ACK = 2,
    TS_PKT_ACK = 3,
    TS_PKT_DATA = 4,
    TS_PKT_FIN = 5,
    TS_PKT_FIN_ACK = 6,
};

// Packet flags definition.
enum {
    TS_PKT_FLAG0_SIGNED_HEADER = 1,
    TS_PKT_FLAG0_SIGNED_DATA = 2,
    TS_PKT_FLAG0_SIGNED_ALL = TS_PKT_FLAG0_SIGNED_HEADER | TS_PKT_FLAG0_SIGNED_DATA,
    TS_PKT_FLAG0_ENCRYPTED_DATA = 4,
};

// TODO: integrate into Deserialize().
TSError_t Packet_verify_pkt(u8 sign_mode, u8 enc_mode,
                            const std::string &sign_key, const std::string &enc_key,
                            const HeaderOnWire &hdr,
                            u8 *payload, size_t payloadSize);

class Packet {
public:
    Packet(u64 src_dev_id, u32 src_inst_id, u32 src_vt_id,
           u64 dst_dev_id, u32 dst_inst_id, u32 dst_vt_id,
           u64 seq_num, u64 ack_num, u32 window_size) 
        : srcDevId(src_dev_id), srcInstId(src_inst_id), srcVtId(src_vt_id),
          dstDevId(dst_dev_id), dstInstId(dst_inst_id), dstVtId(dst_vt_id),
          seqNum(seq_num), ackNum(ack_num), windowSize(window_size) {}
    Packet(const HeaderOnWire& hdr)
        : srcDevId(VPLConv_ntoh_u64(hdr.srcDevId)), srcInstId(VPLConv_ntoh_u32(hdr.srcInstId)), srcVtId(VPLConv_ntoh_u32(hdr.srcVtId)),
          dstDevId(VPLConv_ntoh_u64(hdr.dstDevId)), dstInstId(VPLConv_ntoh_u32(hdr.dstInstId)), dstVtId(VPLConv_ntoh_u32(hdr.dstVtId)),
          seqNum(VPLConv_ntoh_u64(hdr.seqNum)), ackNum(VPLConv_ntoh_u64(hdr.ackNum)), windowSize(VPLConv_ntoh_u32(hdr.windowSize)) {}
    virtual ~Packet() {}

    // factory method
    // On success (non-NULL return value), ownership of "body" transfers to Deserialize.
    // "body" must have been allocated using new[] operator.
    static Packet *Deserialize(const HeaderOnWire &hdr, u8 *body, size_t bodySize);

    u64 GetSeqNum(void) const { return seqNum; }
    u64 GetAckNum(void) const { return ackNum; }
    u64 GetSrcDeviceId(void) const { return srcDevId; }
    u64 GetDstDeviceId(void) const { return dstDevId; }
    u32 GetSrcInstId(void) const { return srcInstId; }
    u32 GetDstInstId(void) const { return dstInstId; }
    u32 GetSrcVtId(void) const { return srcVtId; }
    u32 GetDstVtId(void) const { return dstVtId; }
    u64 GetWindowSize(void) const { return windowSize; }

    virtual u8 GetPktType(void) const = 0;

    // Returns the size of buffer that will need to be passed into Serialize().
    // Note that it does not necessarily return the size of the serialized packet.
    virtual size_t GetSerializeBufSize() const = 0;

    // Serialize packet for transmission over the network.
    // Buffer must be allocated by the caller and passed into Serialize().
    // Returns the size of the serialized packet in the buffer, starting at offset 0.
    // Negative return value means NULL buffer, or insufficient buffer; 
    // negate the value to get the required buffer size.
    virtual ssize_t Serialize(u8 sign_mode, u8 enc_mode,
                              const std::string& sign_key, const std::string& enc_key,
                              u8 *buffer, size_t bufferSize) const = 0;

private:
    VPL_DISABLE_COPY_AND_ASSIGN(Packet);

    const u64 srcDevId;
    const u32 srcInstId;
    const u32 srcVtId;
    const u64 dstDevId;
    const u32 dstInstId;
    const u32 dstVtId;

    const u64 seqNum;
    const u64 ackNum;
    const u32 windowSize;

protected:
    // Helper function to serialize the packet.
    // The expectation is for the subclass' Serialize() method to call this function.
    ssize_t serialize(const u8 *body, size_t bodySize,
                      u8 sign_mode, u8 enc_mode,
                      const std::string& sign_key, const std::string& enc_key,
                      u8 *buffer, size_t bufferSize) const;
};

class PacketAck : public Packet {
public:
    PacketAck(u64 src_dev_id, u32 src_inst_id, u32 src_vt_id,
              u64 dst_dev_id, u32 dst_inst_id, u32 dst_vt_id,
              u64 seq_num, u64 ack_num, u32 window_size)
        : Packet(src_dev_id, src_inst_id, src_vt_id,
                 dst_dev_id, dst_inst_id, dst_vt_id,
                 seq_num, ack_num, window_size) {}
    PacketAck(const HeaderOnWire& hdr) : Packet(hdr) {}
    virtual ~PacketAck() {}

    virtual u8 GetPktType() const { return TS_PKT_ACK; }
    virtual size_t GetSerializeBufSize() const { return sizeof(HeaderOnWire) + TS_HMAC_SIZE; }
    virtual ssize_t Serialize(u8 sign_mode, u8 enc_mode,
                              const std::string& sign_key, const std::string& enc_key,
                              u8 *buffer, size_t bufferSize) const {
        return serialize(/*body*/NULL, /*bodysize*/0,
                         sign_mode, enc_mode,
                         sign_key, enc_key,
                         buffer, bufferSize);
    }

private:
    VPL_DISABLE_COPY_AND_ASSIGN(PacketAck);
};

class PacketSyn: public Packet {
public:
    PacketSyn(u64 src_dev_id, u32 src_inst_id, u32 src_vt_id,
              u64 dst_dev_id, u32 dst_inst_id, u32 dst_vt_id,
              u64 seq_num, u64 ack_num, const std::string& service_name)
        : Packet(src_dev_id, src_inst_id, src_vt_id,
                 dst_dev_id, dst_inst_id, dst_vt_id,
                 seq_num, ack_num, /*windowsize*/0),
          serviceName(service_name) {}
    PacketSyn(const HeaderOnWire& hdr, const std::string& service_name)
        : Packet(hdr), serviceName(service_name) {}
    virtual ~PacketSyn() {}

    virtual u8 GetPktType() const { return TS_PKT_SYN; }
    virtual size_t GetSerializeBufSize() const { return sizeof(HeaderOnWire) + serviceName.size() + TS_HMAC_SIZE; }
    virtual ssize_t Serialize(u8 sign_mode, u8 enc_mode,
                              const std::string& sign_key, const std::string& enc_key,
                              u8 *buffer, size_t bufferSize) const {
        return serialize((u8*)serviceName.data(), serviceName.size(),
                         sign_mode, enc_mode,
                         sign_key, enc_key,
                         buffer, bufferSize);
    }

    void GetSvcName(std::string& svcName) const { svcName = serviceName; }

private:
    VPL_DISABLE_COPY_AND_ASSIGN(PacketSyn);

    const std::string serviceName;
};

class PacketSynAck: public Packet {
public:
    PacketSynAck(u64 src_dev_id, u32 src_inst_id, u32 src_vt_id,
                 u64 dst_dev_id, u32 dst_inst_id, u32 dst_vt_id,
                 u64 seq_num, u64 ack_num, u32 window_size)
        : Packet(src_dev_id, src_inst_id, src_vt_id,
                 dst_dev_id, dst_inst_id, dst_vt_id,
                 seq_num, ack_num, window_size) {}
    PacketSynAck(const HeaderOnWire& hdr) : Packet(hdr) {}
    virtual ~PacketSynAck() {}

    virtual u8 GetPktType() const { return TS_PKT_SYN_ACK; }
    virtual size_t GetSerializeBufSize() const { return sizeof(HeaderOnWire) + TS_HMAC_SIZE; }
    virtual ssize_t Serialize(u8 sign_mode, u8 enc_mode,
                              const std::string& sign_key, const std::string& enc_key,
                              u8 *buffer, size_t bufferSize) const {
        return serialize(/*body*/NULL, /*bodysize*/0,
                         sign_mode, enc_mode,
                         sign_key, enc_key,
                         buffer, bufferSize);
    }

private:
    VPL_DISABLE_COPY_AND_ASSIGN(PacketSynAck);
};

class PacketFin: public Packet {
public:
    PacketFin(u64 src_dev_id, u32 src_inst_id, u32 src_vt_id,
              u64 dst_dev_id, u32 dst_inst_id, u32 dst_vt_id,
              u64 seq_num, u64 ack_num)
        : Packet(src_dev_id, src_inst_id, src_vt_id,
                 dst_dev_id, dst_inst_id, dst_vt_id,
                 seq_num, ack_num, /*windowsize*/0) {}
    PacketFin(const HeaderOnWire& hdr) : Packet(hdr) {}
    virtual ~PacketFin() {}

    virtual u8 GetPktType() const { return TS_PKT_FIN; }
    virtual size_t GetSerializeBufSize() const { return sizeof(HeaderOnWire) + TS_HMAC_SIZE; }
    virtual ssize_t Serialize(u8 sign_mode, u8 enc_mode,
                              const std::string& sign_key, const std::string& enc_key,
                              u8 *buffer, size_t bufferSize) const {
        return serialize(/*body*/NULL, /*bodysize*/0,
                         sign_mode, enc_mode,
                         sign_key, enc_key,
                         buffer, bufferSize);
    }

private:
    VPL_DISABLE_COPY_AND_ASSIGN(PacketFin);
};

class PacketFinAck: public Packet {
public:
    PacketFinAck(u64 src_dev_id, u32 src_inst_id, u32 src_vt_id,
                 u64 dst_dev_id, u32 dst_inst_id, u32 dst_vt_id,
                 u64 seq_num, u64 ack_num, u32 window_size)
        : Packet(src_dev_id, src_inst_id, src_vt_id,
                 dst_dev_id, dst_inst_id, dst_vt_id,
                 seq_num, ack_num, window_size) {}
    PacketFinAck(const HeaderOnWire& hdr) : Packet(hdr) {}
    virtual ~PacketFinAck() {}

    virtual u8 GetPktType() const { return TS_PKT_FIN_ACK; }
    virtual size_t GetSerializeBufSize() const { return sizeof(HeaderOnWire) + TS_HMAC_SIZE; }
    virtual ssize_t Serialize(u8 sign_mode, u8 enc_mode,
                              const std::string& sign_key, const std::string& enc_key,
                              u8 *buffer, size_t bufferSize) const {
        return serialize(/*body*/NULL, /*bodysize*/0,
                         sign_mode, enc_mode,
                         sign_key, enc_key,
                         buffer, bufferSize);
    }
private:
    VPL_DISABLE_COPY_AND_ASSIGN(PacketFinAck);
};

class PacketData: public Packet {
public:
    PacketData(u64 src_dev_id, u32 src_inst_id, u32 src_vt_id,
               u64 dst_dev_id, u32 dst_inst_id, u32 dst_vt_id,
               u64 seq_num, u64 ack_num,
               const u8* data, u32 data_size, u32 window_size, bool delete_body)
        : Packet(src_dev_id, src_inst_id, src_vt_id,
                 dst_dev_id, dst_inst_id, dst_vt_id,
                 seq_num, ack_num, window_size),
          deleteDataOnDestroy(delete_body), data(data), datasize(data_size) {}
    PacketData(const HeaderOnWire& hdr, const u8* data_body, u32 data_size, bool delete_body)
        : Packet(hdr), deleteDataOnDestroy(delete_body), data(data_body), datasize(data_size) {}
    virtual ~PacketData() {
        // 1. data from app to send => app is responsible for data's cleanup, do nothing but erase the pointer
        // 2. data from link layer to receive => cleanup data_pkt (including "u8* data")
        if (data != NULL && deleteDataOnDestroy) {
            delete[] data;
        }
    }

    virtual u8 GetPktType() const { return TS_PKT_DATA; }
    virtual size_t GetSerializeBufSize() const { return sizeof(HeaderOnWire) + datasize + TS_CODEC_UNIT + TS_HMAC_SIZE; }
    virtual ssize_t Serialize(u8 sign_mode, u8 enc_mode,
                              const std::string& sign_key, const std::string& enc_key,
                              u8 *buffer, size_t bufferSize) const {
        return serialize(data, datasize,
                         sign_mode, enc_mode,
                         sign_key, enc_key,
                         buffer, bufferSize);
    }

    const u8* GetData() const { return data; }
    u32 GetDataSize() const { return datasize; }

private:
    VPL_DISABLE_COPY_AND_ASSIGN(PacketData);

    const bool      deleteDataOnDestroy;
    const u8 *const data;
    const u32       datasize;
};

}  // namespace Ts2

#endif // include guard
