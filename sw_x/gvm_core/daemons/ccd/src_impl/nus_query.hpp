//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __NUS_QUERY_HPP__
#define __NUS_QUERY_HPP__

#include "cache.h"
#include "vplex_nus.hpp"

/// @param nusHostname The buffer will be copied.
s32 Query_InitNUS(const char* nusHostname, u16 nusPort);

s32 Query_QuitNUS(void);

/// @param nusHostname The buffer will be copied.
void Query_UpdateNusHostname(const char* nusHostname);

VPLNus_ProxyHandle_t Query_GetNusProxy(void);

void Query_ReleaseNusProxy(VPLNus_ProxyHandle_t proxy);

static const VPLTime_t NUS_TIMEOUT = VPLTIME_FROM_SEC(30);

void Query_privSetNusAbstractRequestFields(vplex::nus::AbstractRequestType& inherited);

template<class RequestT>
static void
Query_SetNusAbstractRequestFields(RequestT& request)
{
    Query_privSetNusAbstractRequestFields(*request.mutable__inherited());
}

///
/// Call a function from vplex_nus.hpp.
/// Example usage:
///  <pre>
///    vplex::nus::GetSystemUpdateRequestType getRequest;
///    getRequest.mutable__inherited()->set_version("2.0");
///    getRequest.set_deviceid(deviceid);
///    ... <set other request fields> ...
///    vplex::nus::GetSystemUpdateResponseType getResponse;
///    rv = QUERY_NUS(VPLNus_GetSystemUpdate, getRequest, getResponse);
///  </pre>
#define QUERY_NUS(vplFunc_, req_in_, resp_out_) \
        Query_Nus(vplFunc_, #vplFunc_, req_in_, resp_out_)
template<typename InT, typename OutT>
static s32 Query_Nus(
        int (*vplex_func)(VPLNus_ProxyHandle_t, VPLTime_t, const InT&, OutT&),
        const char* funcName,
        const InT& in,
        OutT& out)
{
    LOG_DEBUG("Calling %s", funcName);
    s32 rv;
    VPLNus_ProxyHandle_t proxy = Query_GetNusProxy();
    rv = vplex_func(proxy, NUS_TIMEOUT, in, out);
    Query_ReleaseNusProxy(proxy);
    if (rv != 0) {
        LOG_ERROR("%s returned %d", funcName, rv);
    }
    return rv;
}

#endif // include guard
