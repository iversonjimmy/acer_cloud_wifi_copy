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
/// Packet.cpp
///
/// Tunnel Service Packet Defs

#include "vplu_types.h"
#include "vpl_th.h"
#include "vpl_conv.h"
#include "vplex_trace.h"
#include "vplex_assert.h"
#include "vplex_strings.h"
#include "Packet.hpp"
#include "gvm_errors.h"
#include "log.h"

#include "cslsha.h" // SHA1 HMAC
#include "aes.h"    // AES128 encrypt/decrypt

#include <cassert>
#include <iostream>
#include <iomanip>
#include <sstream>

using namespace std;
using namespace Ts2;

#ifdef _MSC_VER
#pragma pack(push,1)
#endif
struct
#ifdef __GNUC__
    __attribute__((__packed__))
#endif
UserDataIvInput {
    // all values are in network order
    u32 dst_vt_id;
    u32 src_vt_id;
    u64 seq_num;
};
#ifdef _MSC_VER
#pragma pack(pop)
#endif

static u8 zeroIv[CSL_AES_IVSIZE_BYTES] = {0};

/*
static void hexdump(const char *bytes, size_t nbytes)
{
    std::ostringstream oss;
    oss << std::endl;
    for (size_t i = 0; i < nbytes; i++) {
        if (i % 16 == 0) {
            oss << std::setfill('0') << std::setw(4) << std::hex << i << ": ";
        }
        oss << std::setfill('0') << std::setw(2) << std::hex << ((u32)bytes[i] & 0xff) << " ";
        if (i % 16 == 15) {
            oss << std::endl;
        }
    }
    oss << std::endl;
    std::string line(oss.str());
    LOG_INFO("%s", line.c_str());
}
*/

static void compute_hmac(const u8* const array_buf[], const size_t array_len[], int array_size,
                         const string& key, u8* hmac, int hmac_size)
{
    CSL_HmacContext context;
    u8 calc_hmac[CSL_SHA1_DIGESTSIZE] = {0};

    CSL_ResetHmac(&context, (const u8*)key.data());

    for(int i = 0; i < array_size; i++) {
        CSL_InputHmac(&context, array_buf[i], array_len[i]);
    }

    CSL_ResultHmac(&context, calc_hmac);

    if (hmac_size > CSL_SHA1_DIGESTSIZE) {
        hmac_size = CSL_SHA1_DIGESTSIZE;
    }
    memcpy(hmac, calc_hmac, hmac_size);
}

static void encrypt_data(u8* dest, const u8* src, size_t len,
                         const u8 iv[CSL_AES_IVSIZE_BYTES], const string& key)
{
    int rc;

    ASSERT((len & (CSL_AES_BLOCKSIZE_BYTES - 1)) == 0);

    rc = aes_SwEncrypt((const u8*)key.data(), iv, src, len, dest);
    if (rc != 0) {
        LOG_ERROR("Failed to encrypt data. - %d", rc);
    }
}

static void decrypt_data(u8* dest, const u8* src, size_t len,
                         const u8 iv[CSL_AES_IVSIZE_BYTES], const string& key)
{
    int rc;

    ASSERT((len & (CSL_AES_BLOCKSIZE_BYTES - 1)) == 0);

    rc = aes_SwDecrypt((const u8*)key.data(), iv, src, len, dest);
    if (rc != 0) {
        LOG_ERROR("Failed to decrypt data - %d.", rc);
    }
}

// *__no means network order
static void computeIv__no(u64 seqNum__no, u32 srcVtId__no, u32 dstVtId__no, const std::string &encKey,
                          u8 iv[CSL_AES_IVSIZE_BYTES])
{
    UserDataIvInput ivInput;
    ivInput.dst_vt_id = dstVtId__no;
    ivInput.src_vt_id = srcVtId__no;
    ivInput.seq_num = seqNum__no;
    encrypt_data(iv, (u8*)&ivInput, sizeof(ivInput), zeroIv, encKey);
}

static void computeIv(u64 seqNum, u32 srcVtId, u32 dstVtId, const std::string &encKey,
                      u8 iv[CSL_AES_IVSIZE_BYTES])
{
    computeIv__no(VPLConv_hton_u64(seqNum), VPLConv_hton_u32(srcVtId), VPLConv_hton_u32(dstVtId), encKey, iv);
}

// Utility function used to fill space left in cipher block with random-looking bytes.
// The code is taken from libvssi.
static void fill_with_random(u8 *buf, size_t size)
{
    // Fill allocated pad space with random data -- need not be secure random,
    // message will be encrypted afterwards.
    while (size > 0) {
        // Random number generator good for 16 bits of randomness.
        static int randomness = 99999; // seed
        randomness = randomness * 1103515245 + 12345;
        buf[0] = randomness & 0xff;
        buf++;
        size--;
        if(size > 0) {
            buf[0] = (randomness >> 8) & 0xff;
            size--;
            buf++;
        }
    }
}

ssize_t Packet::serialize(const u8 *body, size_t bodySize,
                          u8 sign_mode, u8 enc_mode,
                          const string& sign_key, const string& enc_key,
                          u8 *buffer, size_t bufferSize) const
{
    u8 pkt_type = GetPktType();

    size_t hash_len = 0;
    size_t pad_len = 0;
    size_t serializedPacketSize = 0;

    const size_t headerSize = sizeof(HeaderOnWire);

    u8 flag0 = 0;

    if (sign_mode != Ts2::Security::SIGNING_MODE_NONE) {
        hash_len = TS_HMAC_SIZE;
        if (sign_mode == Ts2::Security::SIGNING_MODE_HEADER_ONLY) {
            flag0 |= TS_PKT_FLAG0_SIGNED_HEADER;
        } else {
            flag0 |= TS_PKT_FLAG0_SIGNED_ALL;
        }
    }

    // Add pad data to tail
    if (bodySize > 0 && enc_mode == Ts2::Security::ENCRYPT_MODE_BODY &&
        pkt_type == TS_PKT_DATA) {
       pad_len = (CSL_AES_BLOCKSIZE_BYTES - (bodySize % CSL_AES_BLOCKSIZE_BYTES)) % CSL_AES_BLOCKSIZE_BYTES;
       flag0 |= TS_PKT_FLAG0_ENCRYPTED_DATA;
    }

    serializedPacketSize = headerSize + bodySize + pad_len + hash_len;
    if (!buffer || (bufferSize < serializedPacketSize)) {
        LOG_ERROR("Buffer not supplied or too short: buffer %p supplied "FMT_size_t" needed "FMT_size_t, 
                  buffer, bufferSize, serializedPacketSize);
        return -(ssize_t)serializedPacketSize;  // return needed buffer size as negative number
    }

    {
        HeaderOnWire *how = (HeaderOnWire*)buffer;
        how->version = VPLConv_hton_u8(TS_PKT_VERSION);
        how->type = VPLConv_hton_u8(pkt_type);
        how->flag[0] = VPLConv_hton_u8(flag0);
        how->flag[1] = VPLConv_hton_u8(0);
        how->packetSize = VPLConv_hton_u32(serializedPacketSize);
        how->srcDevId = VPLConv_hton_u64(srcDevId);
        how->srcInstId = VPLConv_hton_u32(srcInstId);
        how->srcVtId = VPLConv_hton_u32(srcVtId);
        how->dstDevId = VPLConv_hton_u64(dstDevId);
        how->dstInstId = VPLConv_hton_u32(dstInstId);
        how->dstVtId = VPLConv_hton_u32(dstVtId);
        how->seqNum = VPLConv_hton_u64(seqNum);
        how->ackNum = VPLConv_hton_u64(ackNum);
        how->windowSize = VPLConv_hton_u32(windowSize);
        how->dataSize = VPLConv_hton_u32(bodySize);
        how->reserved = 0;
    }

    // Sign it
    // Do not modify header or body after this point
    if (sign_mode != Ts2::Security::SIGNING_MODE_NONE) {
        const u8* const array_buf[] = {(u8 *)buffer, (u8 *)body};
        const size_t array_len[] = {headerSize, bodySize};
        compute_hmac(array_buf, array_len, sign_mode == Ts2::Security::SIGNING_MODE_HEADER_ONLY ? 1 : 2,
                     sign_key, buffer + (serializedPacketSize - hash_len), hash_len);
    }

    if (bodySize > 0) {
        // encryption
        if (enc_mode == Ts2::Security::ENCRYPT_MODE_BODY &&
            pkt_type == TS_PKT_DATA) {
            // http://www.ctbg.acer.com/wiki/index.php/User:Fokushi/Ts2/DataPacketEncryption

            u8 iv[CSL_AES_IVSIZE_BYTES];
            computeIv(seqNum, srcVtId, dstVtId, enc_key, iv);

            // First, encrypt all the way up to the last block boundary.
            const size_t dataSizeToLastBoundary = (bodySize / CSL_AES_BLOCKSIZE_BYTES) * CSL_AES_BLOCKSIZE_BYTES;
            assert(dataSizeToLastBoundary <= bodySize);
            assert(bodySize - dataSizeToLastBoundary < CSL_AES_BLOCKSIZE_BYTES);
            if (dataSizeToLastBoundary > 0) {
                encrypt_data(buffer + headerSize,
                             body, dataSizeToLastBoundary,
                             iv, enc_key);
            }

            // Second, if there is a partial block left, work on it.
            if (dataSizeToLastBoundary < bodySize) {  // partial block at end
                u8 buf[CSL_AES_BLOCKSIZE_BYTES];
                memcpy(buf,
                       body + dataSizeToLastBoundary, 
                       bodySize - dataSizeToLastBoundary);
                fill_with_random(buf + CSL_AES_BLOCKSIZE_BYTES - pad_len, pad_len);
                encrypt_data(buffer + headerSize + dataSizeToLastBoundary,
                             buf, CSL_AES_BLOCKSIZE_BYTES,
                             dataSizeToLastBoundary > 0
                             ? (buffer + headerSize + dataSizeToLastBoundary - CSL_AES_BLOCKSIZE_BYTES)
                             : iv, enc_key);
            }
        }
        else {
            memcpy(buffer + headerSize, body, bodySize);
        }
    }

    return serializedPacketSize;
}

static TSError_t Packet_verify_pkt(u8 sign_mode, u8 enc_mode,
                                   const std::string& sign_key, const std::string& enc_key,
                                   const u8 *header, size_t headerSize, 
                                   u8 pktType, u8 flag0, u64 seqNum__no, u32 srcVtId__no, u32 dstVtId__no, size_t dataSize,
                                   u8 *payload, size_t payloadSize)
{
    TSError_t err = TS_OK;

    size_t pad_len = 0;

    if (flag0 & TS_PKT_FLAG0_ENCRYPTED_DATA) {
        pad_len = (CSL_AES_BLOCKSIZE_BYTES - (dataSize % CSL_AES_BLOCKSIZE_BYTES)) % CSL_AES_BLOCKSIZE_BYTES;

        u8 iv[CSL_AES_IVSIZE_BYTES];
        computeIv__no(seqNum__no, srcVtId__no, dstVtId__no, enc_key, iv);

        decrypt_data(payload, payload, dataSize + pad_len, iv, enc_key);
        // set pad to zero
        memset(payload + dataSize, 0, pad_len);
    }

    if (flag0 & TS_PKT_FLAG0_SIGNED_ALL) {
        u8* hash_data = payload + dataSize + pad_len;
        u8 hmac_calc[TS_HMAC_SIZE];
        const u8* array_buf[2];
        size_t array_len[2];
        int numElements = 0;
        if (flag0 & TS_PKT_FLAG0_SIGNED_HEADER) {
            array_buf[numElements] = header;
            array_len[numElements] = headerSize;
            numElements++;
        }
        if (flag0 & TS_PKT_FLAG0_SIGNED_DATA) {
            array_buf[numElements] = payload;
            array_len[numElements] = dataSize;
            numElements++;
        }

        compute_hmac(array_buf, array_len, numElements,
                     sign_key, hmac_calc, sizeof(hmac_calc));

        if (memcmp(hash_data, hmac_calc, sizeof(hmac_calc)) != 0) {
            LOG_ERROR("Packet hash miscompare: dataSize "FMTu_size_t", pad_len "FMTu_size_t, dataSize, pad_len);
            err = TS_ERR_BAD_SIGN;
        }
    }
    return err;
}

TSError_t Ts2::Packet_verify_pkt(u8 sign_mode, u8 enc_mode,
                                 const std::string &sign_key, const std::string &enc_key,
                                 const HeaderOnWire &hdr,
                                 u8 *payload, size_t payloadSize)
{
    return ::Packet_verify_pkt(sign_mode, enc_mode,
                               sign_key, enc_key,
                               (const u8*)&hdr, sizeof(hdr),
                               hdr.type, hdr.flag[0], hdr.seqNum, hdr.srcVtId, hdr.dstVtId, VPLConv_ntoh_u32(hdr.dataSize),
                               payload, payloadSize);
}

// class method
Ts2::Packet *Ts2::Packet::Deserialize(const HeaderOnWire &hdr,
                                      u8 *data, size_t dataSize)
{
    Packet *p = NULL;

    switch (hdr.type) {
    case TS_PKT_SYN:
        {
            std::string serviceName((char*)data, dataSize);
            p = new (std::nothrow) PacketSyn(hdr, serviceName);
        }
        break;
    case TS_PKT_SYN_ACK:
        p = new (std::nothrow) PacketSynAck(hdr);
        break;
    case TS_PKT_ACK:
        p = new (std::nothrow) PacketAck(hdr);
        break;
    case TS_PKT_DATA:
        p = new (std::nothrow) PacketData(hdr, data, dataSize, /*delete_body*/true);
        data = NULL;  // ownership transfers to PacketData obj
        break;
    case TS_PKT_FIN:
        p = new (std::nothrow) PacketFin(hdr);
        break;
    case TS_PKT_FIN_ACK:
        p = new (std::nothrow) PacketFinAck(hdr);
        break;
    default: // unexpected packet type
        LOG_ERROR("Unknown packet type %d", hdr.type);
        return NULL;
    }
    if (!p) {
        LOG_ERROR("Failed to create Packet obj (type %d)", hdr.type);
        return NULL;
    }

    if (data) {
        delete [] data;
    }

    return p;
}
