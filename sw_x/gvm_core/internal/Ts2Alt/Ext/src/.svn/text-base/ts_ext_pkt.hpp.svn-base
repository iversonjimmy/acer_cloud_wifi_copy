/*
 *  Copyright 2014 Acer Cloud Technology Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF 
 *  ACER CLOUD TECHNOLOGY INC.
 *
 */

///
/// ts_ext_pkt.hpp
///
/// Tunnel Service External Packet Defs


#ifndef __TS_EXT_PKT_HPP__
#define __TS_EXT_PKT_HPP__


#include "vplu_types.h"
#include "vpl_time.h"
#include "vpl_conv.h"
#include "string"
#include "vplex_trace.h"
#include "ts_ext_client.hpp"

using namespace std;


// Maximum read/write buffer size
#define TS_EXT_WRITE_MAX_BUF_SIZE       (128 * 1024)
#define TS_EXT_READ_MAX_BUF_SIZE        (128 * 1024)

// Common timeout value
#define TS_EXT_TIMEOUT_IN_SEC           (60)

#define TS_EXT_PKT_VERSION              1

#ifndef _MSC_VER
typedef struct __attribute__((__packed__)) ts_ext_pkt_hdr_s {
#else
#pragma pack(push,1)
typedef struct ts_ext_pkt_hdr_s {
#endif
    u8      protocol;
    u8      pkt_type;
    u8      reserved[2];
    u32     xid;
    u32     pkt_len;
} ts_ext_pkt_hdr_t;
#ifdef _MSC_VER
#pragma pack(pop)
#endif

#define TS_EXT_PKT_SVCNM_LEN        20
#ifndef _MSC_VER
typedef struct __attribute__((__packed__)) ts_ext_open_req_pkt_s {
#else
#pragma pack(push,1)
typedef struct ts_ext_open_req_pkt_s {
#endif
    u64     user_id;
    u64     device_id;
    u8      service_name[TS_EXT_PKT_SVCNM_LEN];
} ts_ext_open_req_pkt_t;
#ifdef _MSC_VER
#pragma pack(pop)
#endif

#ifndef _MSC_VER
typedef struct __attribute__((__packed__)) ts_ext_open_resp_pkt_s {
#else
#pragma pack(push,1)
typedef struct ts_ext_open_resp_pkt_s {
#endif
    TSError_t   error;
} ts_ext_open_resp_pkt_t;
#ifdef _MSC_VER
#pragma pack(pop)
#endif

#ifndef _MSC_VER
typedef struct __attribute__((__packed__)) ts_ext_close_resp_pkt_s {
#else
#pragma pack(push,1)
typedef struct ts_ext_close_resp_pkt_s {
#endif
    TSError_t   error;
} ts_ext_close_resp_pkt_t;
#ifdef _MSC_VER
#pragma pack(pop)
#endif

#ifndef _MSC_VER
typedef struct __attribute__((__packed__)) ts_ext_read_req_pkt_s {
#else
#pragma warning( push )
#pragma warning( disable : 4200 )
#pragma pack(push,1)
typedef struct ts_ext_read_req_pkt_s {
#endif
    u32     max_data_len;
} ts_ext_read_req_pkt_t;
#ifdef _MSC_VER
#pragma pack(pop)
#pragma warning( pop )
#endif

#ifndef _MSC_VER
typedef struct __attribute__((__packed__)) ts_ext_read_resp_pkt_s {
#else
#pragma pack(push,1)
typedef struct ts_ext_read_resp_pkt_s {
#endif
    TSError_t   error;
} ts_ext_read_resp_pkt_t;
#ifdef _MSC_VER
#pragma pack(pop)
#endif

#ifndef _MSC_VER
typedef struct __attribute__((__packed__)) ts_ext_write_resp_pkt_s {
#else
#pragma pack(push,1)
typedef struct ts_ext_write_resp_pkt_s {
#endif
    TSError_t   error;
} ts_ext_write_resp_pkt_t;
#ifdef _MSC_VER
#pragma pack(pop)
#endif

enum {
    TS_EXT_PKT_OPEN_REQ,
    TS_EXT_PKT_OPEN_RESP,
    TS_EXT_PKT_CLOSE_REQ,
    TS_EXT_PKT_CLOSE_RESP,
    TS_EXT_PKT_READ_REQ,
    TS_EXT_PKT_READ_RESP,
    TS_EXT_PKT_WRITE_REQ,
    TS_EXT_PKT_WRITE_RESP,
};

#define MAX_TS_EXT_PKT_TYPE_HDR_SIZE (sizeof(ts_ext_open_req_pkt_t))

TSError_t ts_ext_pkt_type_hdr_len(u8 pkt_type, size_t& len_out);


class ts_ext_pkt {
public:
    ts_ext_pkt(u8 pkt_type, u32 xid) :
        pkt_type(pkt_type),
        xid(xid) {};
    ts_ext_pkt(ts_ext_pkt_hdr_t& hdr) : 
        pkt_type(hdr.pkt_type),
        xid(hdr.xid),
        pkt_len(VPLConv_ntoh_u32(hdr.pkt_len)) {};
    virtual ~ts_ext_pkt() {};

    virtual void serialize(const u8* &pkt_hdrs, u32& hdrs_len,
                           const u8* &pkt_data, u32& pkt_data_len);
    void serialize_pkt_hdr(u32 body_len);

    u8                  pkt_type;
    u32                 xid;
    u32                 pkt_len;
    ts_ext_pkt_hdr_t    pkt_hdr;
};

class ts_ext_open_req_pkt: public ts_ext_pkt {
public:
    ts_ext_open_req_pkt(u32 xid,
                        const u64 user_id,
                        const u64 device_id,
                        const string& service_name) :
        ts_ext_pkt(TS_EXT_PKT_OPEN_REQ, xid),
        user_id(user_id),
        device_id(device_id),
        service_name(service_name) {};
    ts_ext_open_req_pkt(ts_ext_pkt_hdr_t& hdr, ts_ext_open_req_pkt_t& open_req_body) :
        ts_ext_pkt(hdr),
        user_id(VPLConv_ntoh_u64(open_req_body.user_id)),
        device_id(VPLConv_ntoh_u64(open_req_body.device_id))
        {
            service_name.assign((char *)open_req_body.service_name,
                                strnlen((char*)open_req_body.service_name,
                                TS_EXT_PKT_SVCNM_LEN));
        };
    virtual ~ts_ext_open_req_pkt() {};

    void serialize(const u8* &pkt_hdrs, u32& hdrs_len,
                   const u8* &pkt_data, u32& pkt_data_len);

    // Packets consist of fixed len hdrs optionally followed by variable len data.
    // Most packet types (like this one) don't have variable len data.
    u8 serialized_hdrs[sizeof(ts_ext_pkt_hdr_t) + sizeof(ts_ext_open_req_pkt_t)];

    u64     user_id;
    u64     device_id;
    string  service_name;
};

class ts_ext_open_resp_pkt: public ts_ext_pkt {
public:
    ts_ext_open_resp_pkt(u32 xid, int error) :
        ts_ext_pkt(TS_EXT_PKT_OPEN_RESP, xid),
        error(error) {};
    ts_ext_open_resp_pkt(ts_ext_pkt_hdr_t& hdr, ts_ext_open_resp_pkt_t& open_resp_body) :
        ts_ext_pkt(hdr),
        error(VPLConv_ntoh_s32(open_resp_body.error)) {};
    virtual ~ts_ext_open_resp_pkt() {};

    void serialize(const u8* &pkt_hdrs, u32& hdrs_len,
                   const u8* &pkt_data, u32& pkt_data_len);

    // Packets consist of fixed len hdrs optionally followed by variable len data.
    // Most packet types (like this one) don't have variable len data.
    u8 serialized_hdrs[sizeof(ts_ext_pkt_hdr_t) + sizeof(ts_ext_open_resp_pkt_t)];

    s32                     error;
};

class ts_ext_close_req_pkt: public ts_ext_pkt {
public:
    ts_ext_close_req_pkt(u32 xid) :
        ts_ext_pkt(TS_EXT_PKT_CLOSE_REQ, xid) {};
    ts_ext_close_req_pkt(ts_ext_pkt_hdr_t& hdr, void* unused) :
        ts_ext_pkt(hdr) {};
    virtual ~ts_ext_close_req_pkt() {};

    void serialize(const u8* &pkt_hdrs, u32 &hdrs_len,
                   const u8* &pkt_data, u32 &pkt_data_len);

    // Packets consist of fixed len hdrs optionally followed by variable len data.
    // Most packet types (like this one) don't have variable len data.
    u8 serialized_hdrs[sizeof(ts_ext_pkt_hdr_t)];
};

class ts_ext_close_resp_pkt: public ts_ext_pkt {
public:
    ts_ext_close_resp_pkt(u32 xid, int error):
        ts_ext_pkt(TS_EXT_PKT_CLOSE_RESP, xid),
        error(error) {};
    ts_ext_close_resp_pkt(ts_ext_pkt_hdr_t& hdr, ts_ext_close_resp_pkt_s& close_resp_body) :
        ts_ext_pkt(hdr),
        error(VPLConv_ntoh_s32(close_resp_body.error)) {};
    virtual ~ts_ext_close_resp_pkt() {};

    void serialize(const u8* &pkt_hdrs, u32& hdrs_len,
                   const u8* &pkt_data, u32& pkt_data_len);

    // Packets consist of fixed len hdrs optionally followed by variable len data.
    // Most packet types (like this one) don't have variable len data.
    u8 serialized_hdrs[sizeof(ts_ext_pkt_hdr_t) + sizeof(ts_ext_close_resp_pkt_t)];

    s32                     error;
};

class ts_ext_read_req_pkt: public ts_ext_pkt {
public:
    ts_ext_read_req_pkt(u32 xid, u32 max_data_len) :
        ts_ext_pkt(TS_EXT_PKT_READ_REQ, xid),
        max_data_len(max_data_len) {};
    ts_ext_read_req_pkt(ts_ext_pkt_hdr_t& hdr, ts_ext_read_req_pkt_s& read_req_body) :
        ts_ext_pkt(hdr),
        max_data_len(VPLConv_ntoh_u32(read_req_body.max_data_len)) {};
    virtual ~ts_ext_read_req_pkt() {}

    void serialize(const u8* &pkt_hdrs, u32& hdrs_len,
                   const u8* &pkt_data, u32& pkt_data_len);

    // Packets consist of fixed len hdrs optionally followed by variable len data.
    // Most packet types don't have variable len data.
    u8 serialized_hdrs [sizeof(ts_ext_pkt_hdr_t) + sizeof(ts_ext_read_req_pkt_t)];

    u32       max_data_len;
};

class ts_ext_read_resp_pkt: public ts_ext_pkt {
public:
    ts_ext_read_resp_pkt(u32 xid, int error, const char* data_buf, u32 data_buf_len) :
        ts_ext_pkt(TS_EXT_PKT_READ_RESP, xid),
        error(error),
        data((const u8*) data_buf),
        data_len(data_buf_len) {};
    virtual ~ts_ext_read_resp_pkt() {}

    void serialize(const u8* &pkt_hdrs, u32& hdrs_len,
                   const u8* &pkt_data, u32& pkt_data_len);

    // Packets consist of fixed len hdrs optionally followed by variable len data.
    // Most packet types don't have variable len data.  This one does.
    // This object does not own the data, just points to it; so doesn't delete it.
    u8 serialized_hdrs[sizeof(ts_ext_pkt_hdr_t) + sizeof(ts_ext_read_resp_pkt_t)];

    u32       error;
    const u8* data;
    u32       data_len;
};

class ts_ext_write_req_pkt: public ts_ext_pkt {
public:
    ts_ext_write_req_pkt(u32 xid, const char* data_buf, u32 data_buf_len) :
        ts_ext_pkt(TS_EXT_PKT_WRITE_REQ, xid),
        data((const u8*) data_buf),
        data_len(data_buf_len) {};
    virtual ~ts_ext_write_req_pkt() {}

    void serialize(const u8* &pkt_hdrs, u32& hdrs_len,
                   const u8* &pkt_data, u32& pkt_data_len);

    // Packets consist of fixed len hdrs optionally followed by variable len data.
    // Most packet types don't have variable len data.  This one does.
    // This object does not own the data, just points to it; so doesn't delete it.
    u8 serialized_hdrs[sizeof(ts_ext_pkt_hdr_t)];

    const u8* data;
    u32       data_len;
};

class ts_ext_write_resp_pkt: public ts_ext_pkt {
public:
    ts_ext_write_resp_pkt(u32 xid, int error):
        ts_ext_pkt(TS_EXT_PKT_WRITE_RESP, xid),
        error(error) {};
    ts_ext_write_resp_pkt(ts_ext_pkt_hdr_t& hdr, ts_ext_write_resp_pkt_s& write_resp_body) :
        ts_ext_pkt(hdr),
        error(VPLConv_ntoh_s32(write_resp_body.error)) {};
    virtual ~ts_ext_write_resp_pkt() {};

    void serialize(const u8* &pkt_hdrs, u32& hdrs_len,
                   const u8* &pkt_data, u32& pkt_data_len);

    // Packets consist of fixed len hdrs optionally followed by variable len data.
    // Most packet types (like this one) don't have variable len data.
    u8 serialized_hdrs[sizeof(ts_ext_pkt_hdr_t) + sizeof(ts_ext_write_resp_pkt_t)];

    s32                     error;
};


#endif // include guard
