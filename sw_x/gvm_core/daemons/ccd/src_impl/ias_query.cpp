//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include "ias_query.hpp"

#include "vpl_th.h"
#include "vplex_ias.hpp"

#include "cache.h"
#include "config.h"
#include "query.h"

using namespace std;
using namespace ccd;

static VPLIas_ProxyHandle_t __ias;
static char __iasHostname[HOSTNAME_MAX_LENGTH];
static VPLMutex_t __iasMutex; // proxy is not currently threadsafe

s32 Query_InitIAS(const char* iasHostname, u16 iasPort)
{
    int rv = 0;

    rv = VPLMutex_Init(&__iasMutex);
    if (rv != 0) {
        LOG_ERROR("VPLMutex_Init returned %d", rv);
        rv = CCD_ERROR_INTERNAL;
        goto out;
    }

    Query_UpdateIasHostname(iasHostname);

    rv = VPLIas_CreateProxy(__iasHostname, iasPort, &__ias);
    if (rv != 0) {
        LOG_ERROR("VPLIas_CreateProxy returned %d.", rv);
        goto out;
    }

 out:
    return rv;
}

s32 Query_QuitIAS(void)
{
    return VPLIas_DestroyProxy(__ias);
}

void Query_UpdateIasHostname(const char* iasHostname)
{
    VPLMutex_Lock(&__iasMutex);
    strncpy(__iasHostname, iasHostname, ARRAY_SIZE_IN_BYTES(__iasHostname));
    VPLMutex_Unlock(&__iasMutex);
}

void Query_privSetIasAbstractRequestFields(vplex::ias::AbstractRequestType& inherited)
{
    inherited.set_version("2.0");
    //inherited.set_country("US");
    //inherited.set_language("en");
    //inherited.set_region("US");
    inherited.set_messageid(Util_MakeMessageId());
}

VPLIas_ProxyHandle_t Query_GetIasProxy(void)
{
    VPLMutex_Lock(&__iasMutex);
    return __ias;
}

void Query_ReleaseIasProxy(VPLIas_ProxyHandle_t proxy)
{
    UNUSED(proxy); // ias proxy is not currently threadsafe
    VPLMutex_Unlock(&__iasMutex);
}
