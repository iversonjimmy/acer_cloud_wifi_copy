/*
 *  Copyright 2013 Acer Cloud Technology Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF
 *  ACER CLOUD TECHNOLOGY INC.
 */

#ifndef __TS_TYPES_HPP__
#define __TS_TYPES_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>
#include <vpl_time.h>
#include <gvm_errors.h>
#include <string>
#include <list>

using namespace std;

typedef int32_t TSError_t;

#define TS_OK  0

typedef struct {
    u64         user_id;
    u64         device_id;
    u32         instance_id;
    string      service_name;
    string      credentials;
    u64         flags;
    VPLTime_t   timeout;
} TSOpenParms_t;

typedef void* TSIOHandle_t;

typedef struct TSServiceHandle_s* TSServiceHandle_t;

typedef struct {
    TSServiceHandle_t   service_handle;
    u64                 client_user_id;
    u64                 client_device_id;
    string              client_credentials;
    TSIOHandle_t        io_handle;
} TSServiceRequest_t;

typedef void (*TSServiceHandlerFn_t)(TSServiceRequest_t& request);

typedef struct {
    list<string>        service_names;
    string              protocol_name;
    TSServiceHandlerFn_t  service_handler;
} TSServiceParms_t;

enum TS_CRED_QUERY_TYPE {
  TS_CRED_QUERY_SVR_KEY = 1,
  TS_CRED_QUERY_PXD_CRED = 2,
  TS_CRED_QUERY_CCD_CRED = 3
};

typedef struct {
    // Query type,
    TS_CRED_QUERY_TYPE type;

    // Clear type of credentials in cache
    bool resetCred;

    // Required
    u64 user_id;

    // Optional, for TS_CRED_QUERY_CCD_CRED
    u64 target_svr_user_id;
    u64 target_svr_device_id;
    u32 target_svr_instance_id;

    // Query result
    int result;

    // Output:
    // server key for TS_CRED_QUERY_SVR_KEY
    // session key for TS_CRED_QUERY_PXD_CRED and TS_CRED_QUERY_CCD_CRED
    std::string resp_key;

    // Output:
    // session blob for TS_CRED_QUERY_PXD_CRED and TS_CRED_QUERY_CCD_CRED
    std::string resp_blob;

    // Output:
    // instance ID for TS_CRED_QUERY_PXD_CRED and TS_CRED_QUERY_CCD_CRED
    u32 resp_instance_id;
} TSCredQuery_t;

#endif  // include guard
