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
/// ts_internal.hpp
///
/// Tunnel Service internal definitions
///
#ifndef __TS_INTERNAL_HPP__
#define __TS_INTERNAL_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>
#include <ts_client.hpp>
#include <ts_server.hpp>
#include <string>
#include <list>
#include <queue>
#include <map>
#include <utility>
#include <scopeguard.hpp>
#include <log.h>
#include <vpl_net.h>
#include <vpl_socket.h>
#include <vpl_th.h>
#include <vplu_mutex_autolock.hpp>
#include <vplex_assert.h>
#include <vplex_safe_conversion.h>
#include <gvm_thread_utils.h>

using namespace std;

#define TS_FMT_TARG     "<"FMTu64","FMTu64","FMTu32">"

// 3-tuple that defines a TS endpoint
typedef struct {
    u64    userId;
    u64    deviceId;
    u32    instanceId;
} ts_udi_t;

#define TS_TEMP_PORT       0x1234
#define TS_TEMP_PORT_RANGE 10

enum {
    TS_CONN_LOOPBACK = 1,
    TS_CONN_SOCKET   = 2
};

#define MAX_CONN_TYPE       4
enum {
    TS_CONN_TYPE_DIN    = 1,
    TS_CONN_TYPE_DEX    = 2,
    TS_CONN_TYPE_P2P    = 3,
    TS_CONN_TYPE_PRX    = 4,
};

// Various TS timeout values in seconds
typedef struct ts_timeouts_s {
    int                         conn_idle;
    int                         vtunnel_idle;
    int                         tunnel_idle;
    int                         keep_alive_prime;
    int                         keep_alive_idle;
} ts_timeouts_t;

struct TSServiceHandle_s {
    string              protocol_name;
    list<string>        service_names;
    TSServiceHandlerFn_t  service_handler;
};

TSError_t TS_ServiceLookup(const string& service_name,
                           TSServiceHandle_t& service_handle,
                           string& error_msg);

#endif // include guard
