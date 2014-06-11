//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __IAS_QUERY_HPP__
#define __IAS_QUERY_HPP__

#include "cache.h"
#include "vplex_ias.hpp"

/// @param iasHostname The buffer will be copied.
s32 Query_InitIAS(const char* iasHostname, u16 iasPort);

s32 Query_QuitIAS(void);

/// @param iasHostname The buffer will be copied.
void Query_UpdateIasHostname(const char* iasHostname);

VPLIas_ProxyHandle_t Query_GetIasProxy(void);

void Query_ReleaseIasProxy(VPLIas_ProxyHandle_t proxy);

static const VPLTime_t IAS_TIMEOUT = VPLTIME_FROM_SEC(30);

void Query_privSetIasAbstractRequestFields(vplex::ias::AbstractRequestType& inherited);

template<class RequestT>
static void
Query_SetIasAbstractRequestFields(RequestT& request)
{
    Query_privSetIasAbstractRequestFields(*request.mutable__inherited());
}

///
/// Call a function from vplex_ias.hpp.
/// Example usage:
///  <pre>
///    vplex::ias::RegisterVirtualDeviceRequestType registerRequest;
///    registerRequest.mutable__inherited()->set_version("2.0");
///    registerRequest.set_username(username);
///    ... <set other request fields> ...
///    vplex::ias::RegisterVirtualDeviceResponseType registerResponse;
///    rv = QUERY_IAS(VPLIas_RegisterVirtualDevice, registerRequest, registerResponse);
///  </pre>
#define QUERY_IAS(vplFunc_, req_in_, resp_out_) \
        Query_Ias(vplFunc_, #vplFunc_, req_in_, resp_out_)
template<typename InT, typename OutT>
static s32 Query_Ias(
        int (*vplex_func)(VPLIas_ProxyHandle_t, VPLTime_t, const InT&, OutT&),
        const char* funcName,
        const InT& in,
        OutT& out)
{
    LOG_DEBUG("Calling %s", funcName);
    s32 rv;
    VPLIas_ProxyHandle_t proxy = Query_GetIasProxy();
    rv = vplex_func(proxy, IAS_TIMEOUT, in, out);
    Query_ReleaseIasProxy(proxy);
    if (rv != 0) {
        LOG_ERROR("%s returned %d", funcName, rv);
    }
    return rv;
}

#endif // include guard
