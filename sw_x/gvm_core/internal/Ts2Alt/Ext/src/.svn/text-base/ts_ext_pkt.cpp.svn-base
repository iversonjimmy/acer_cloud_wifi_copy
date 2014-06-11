/*
 *  Copyright 2014 Acer Cloud Technology Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF IGWARE INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
 *
 */

///
/// ts_ext_pkt.cpp
///
/// Tunnel Service External Packet Support


#include "vplu_types.h"
#include "vpl_th.h"
#include "vpl_conv.h"
#include "vplex_trace.h"
#include "vplex_assert.h"
#include "ts_ext_pkt.hpp"
#include "gvm_errors.h"

using namespace std;


TSError_t ts_ext_pkt_type_hdr_len(u8 pkt_type, size_t& len_out)
{
    TSError_t err = TS_OK;

    switch (pkt_type) {
        case TS_EXT_PKT_OPEN_REQ:
            len_out = sizeof(ts_ext_open_req_pkt_t);
            break;
        case TS_EXT_PKT_OPEN_RESP:
            len_out = sizeof(ts_ext_open_resp_pkt_t);
            break;
        case TS_EXT_PKT_CLOSE_REQ:
            len_out = 0;
            break;
        case TS_EXT_PKT_CLOSE_RESP:
            len_out = sizeof(ts_ext_close_resp_pkt_t);
            break;
        case TS_EXT_PKT_READ_REQ:
            len_out = sizeof(ts_ext_read_req_pkt_t);
            break;
        case TS_EXT_PKT_READ_RESP:
            len_out = sizeof(ts_ext_read_resp_pkt_t);
            break;
        case TS_EXT_PKT_WRITE_REQ:
            len_out = 0;
            break;
        case TS_EXT_PKT_WRITE_RESP:
            len_out = sizeof(ts_ext_write_resp_pkt_t);
            break;
        default:
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Unknown pkt type %d", pkt_type);
            err = TS_ERR_EXT_TYPE;
            len_out = 0;
            break;
    }

    return err;
}


void ts_ext_pkt::serialize(const u8* &pkt_hdrs, u32& hdrs_len,
                           const u8* &pkt_data, u32& pkt_data_len)
{
    // Default implementation works for packet types
    // that consist of only ts_ext_pkt_hdr_t.
    // i.e. when the packet type is the only info needed.
    serialize_pkt_hdr(0);
    pkt_hdrs = (u8*) &pkt_hdr;
    hdrs_len = sizeof(pkt_hdr);
    pkt_data = NULL;
    pkt_data_len = 0;
};

void ts_ext_pkt::serialize_pkt_hdr(u32 body_len)
{
    pkt_len = sizeof(pkt_hdr) + body_len;

    memset(&pkt_hdr, 0, sizeof(pkt_hdr));
    pkt_hdr.protocol = TS_EXT_PKT_VERSION;
    pkt_hdr.pkt_type = pkt_type;
    pkt_hdr.xid = VPLConv_hton_u32(xid);
    pkt_hdr.pkt_len = VPLConv_hton_u32(pkt_len);
};

void ts_ext_open_req_pkt::serialize(const u8* &pkt_hdrs, u32& hdrs_len,
                                    const u8* &pkt_data, u32& pkt_data_len)
{
    // Serialize parameters added for specific packet type
    ts_ext_open_req_pkt_t open_req_body;

    open_req_body.user_id = VPLConv_hton_u64(user_id);
    open_req_body.device_id = VPLConv_hton_u64(device_id);
    memset(open_req_body.service_name, 0, sizeof(open_req_body.service_name));
    strncpy((char*) open_req_body.service_name,
            service_name.c_str(),
            sizeof(open_req_body.service_name));

    // Serialize values in ts_ext_pkt_hdr_t
    serialize_pkt_hdr(sizeof(open_req_body));

    memcpy(serialized_hdrs, &pkt_hdr, sizeof(pkt_hdr));
    memcpy(&serialized_hdrs[sizeof(pkt_hdr)], &open_req_body, sizeof(open_req_body));

    pkt_hdrs = serialized_hdrs;
    hdrs_len = sizeof(serialized_hdrs);

    pkt_data = NULL;
    pkt_data_len = 0;
};

void ts_ext_open_resp_pkt::serialize(const u8* &pkt_hdrs, u32& hdrs_len,
                                     const u8* &pkt_data, u32& pkt_data_len)
{
    // Serialize parameters added for specific packet type
    ts_ext_open_resp_pkt_t open_resp_body;

    open_resp_body.error = VPLConv_hton_s32(error);

    // Serialize values in ts_ext_pkt_hdr_t
    serialize_pkt_hdr(sizeof(open_resp_body));

    memcpy(serialized_hdrs, &pkt_hdr, sizeof(pkt_hdr));
    memcpy(&serialized_hdrs[sizeof(pkt_hdr)], &open_resp_body, sizeof(open_resp_body));

    pkt_hdrs = serialized_hdrs;
    hdrs_len = sizeof(serialized_hdrs);

    pkt_data = NULL;
    pkt_data_len = 0;
}

void ts_ext_close_req_pkt::serialize(const u8* &pkt_hdrs, u32& hdrs_len,
                                     const u8* &pkt_data, u32& pkt_data_len) {

    serialize_pkt_hdr(0);

    memcpy(serialized_hdrs, &pkt_hdr, sizeof(pkt_hdr));

    pkt_hdrs = serialized_hdrs;
    hdrs_len = sizeof(serialized_hdrs);

    pkt_data = NULL;
    pkt_data_len = 0;
};

void ts_ext_close_resp_pkt::serialize(const u8* &pkt_hdrs, u32& hdrs_len,
                                      const u8* &pkt_data, u32& pkt_data_len)
{
    // Serialize parameters added for specific packet type
    ts_ext_close_resp_pkt_t close_resp_body;

    close_resp_body.error = VPLConv_hton_s32(error);

    // Serialize values in ts_ext_pkt_hdr_t
    serialize_pkt_hdr(sizeof(close_resp_body));

    memcpy(serialized_hdrs, &pkt_hdr, sizeof(pkt_hdr));
    memcpy(&serialized_hdrs[sizeof(pkt_hdr)], &close_resp_body, sizeof(close_resp_body));

    pkt_hdrs = serialized_hdrs;
    hdrs_len = sizeof(serialized_hdrs);

    pkt_data = NULL;
    pkt_data_len = 0;
}

void ts_ext_read_req_pkt::serialize(const u8* &pkt_hdrs, u32& hdrs_len,
                                    const u8* &pkt_data, u32& pkt_data_len)
{
    // Serialize parameters added for specific packet type
    ts_ext_read_req_pkt_t read_req_body;

    read_req_body.max_data_len = VPLConv_hton_u32(max_data_len);

    // Serialize values in ts_ext_pkt_hdr_t
    serialize_pkt_hdr(sizeof(read_req_body));

    memcpy(serialized_hdrs, &pkt_hdr, sizeof(pkt_hdr));
    memcpy(&serialized_hdrs[sizeof(pkt_hdr)], &read_req_body, sizeof(read_req_body));

    pkt_hdrs = serialized_hdrs;
    hdrs_len = sizeof(serialized_hdrs);

    pkt_data = NULL;
    pkt_data_len = 0;
};

void ts_ext_read_resp_pkt::serialize(const u8* &pkt_hdrs, u32& hdrs_len,
                                     const u8* &pkt_data, u32& pkt_data_len)
{
    // Serialize parameters added for specific packet type
    ts_ext_read_resp_pkt_t read_resp_body;

    read_resp_body.error = VPLConv_hton_s32(error);

    // Serialize values in ts_ext_pkt_hdr_t
    serialize_pkt_hdr(sizeof(read_resp_body) + data_len);

    memcpy(serialized_hdrs, &pkt_hdr, sizeof(pkt_hdr));
    memcpy(&serialized_hdrs[sizeof(pkt_hdr)], &read_resp_body, sizeof(read_resp_body));

    pkt_hdrs = serialized_hdrs;
    hdrs_len = sizeof(serialized_hdrs);

    pkt_data = data;
    pkt_data_len = data_len;
};

void ts_ext_write_req_pkt::serialize(const u8* &pkt_hdrs, u32& hdrs_len,
                                     const u8* &pkt_data, u32& pkt_data_len) {

    serialize_pkt_hdr(data_len);

    memcpy(serialized_hdrs, &pkt_hdr, sizeof(pkt_hdr));

    pkt_hdrs = serialized_hdrs;
    hdrs_len = sizeof(serialized_hdrs);

    pkt_data = data;
    pkt_data_len = data_len;
};

void ts_ext_write_resp_pkt::serialize(const u8* &pkt_hdrs, u32& hdrs_len,
                                      const u8* &pkt_data, u32& pkt_data_len)
{
    // Serialize parameters added for specific packet type
    ts_ext_write_resp_pkt_t write_resp_body;

    write_resp_body.error = VPLConv_hton_s32(error);

    // Serialize values in ts_ext_pkt_hdr_t
    serialize_pkt_hdr(sizeof(write_resp_body));

    memcpy(serialized_hdrs, &pkt_hdr, sizeof(pkt_hdr));
    memcpy(&serialized_hdrs[sizeof(pkt_hdr)], &write_resp_body, sizeof(write_resp_body));

    pkt_hdrs = serialized_hdrs;
    hdrs_len = sizeof(serialized_hdrs);

    pkt_data = NULL;
    pkt_data_len = 0;
}
