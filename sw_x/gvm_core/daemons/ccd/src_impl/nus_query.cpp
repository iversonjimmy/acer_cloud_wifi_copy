//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include "nus_query.hpp"

#include "vpl_th.h"
#include "vplex_nus.hpp"

#include "cache.h"
#include "config.h"
#include "query.h"

using namespace std;
using namespace ccd;

static VPLNus_ProxyHandle_t __nus;
static char __nusHostname[HOSTNAME_MAX_LENGTH];
static VPLMutex_t __nusMutex; // proxy is not currently threadsafe

s32 Query_InitNUS(const char* nusHostname, u16 nusPort)
{
    int rv = 0;

    rv = VPLMutex_Init(&__nusMutex);
    if (rv != 0) {
        LOG_ERROR("VPLMutex_Init returned %d", rv);
        rv = CCD_ERROR_INTERNAL;
        goto out;
    }

    Query_UpdateNusHostname(nusHostname);

    rv = VPLNus_CreateProxy(__nusHostname, nusPort, &__nus);
    if (rv != 0) {
        LOG_ERROR("VPLNus_CreateProxy returned %d.", rv);
        goto out;
    }

 out:
    return rv;
}

s32 Query_QuitNUS(void)
{
    return VPLNus_DestroyProxy(__nus);
}

void Query_UpdateNusHostname(const char* nusHostname)
{
    VPLMutex_Lock(&__nusMutex);
    strncpy(__nusHostname, nusHostname, ARRAY_SIZE_IN_BYTES(__nusHostname));
    VPLMutex_Unlock(&__nusMutex);
}

void Query_privSetNusAbstractRequestFields(vplex::nus::AbstractRequestType& inherited)
{
    inherited.set_version("1.0");
    //inherited.set_countrycode("us");
    //inherited.set_language("en");
    //inherited.set_regionid("USA");
    inherited.set_messageid(Util_MakeMessageId());
}

VPLNus_ProxyHandle_t Query_GetNusProxy(void)
{
    VPLMutex_Lock(&__nusMutex);
    return __nus;
}

void Query_ReleaseNusProxy(VPLNus_ProxyHandle_t proxy)
{
    UNUSED(proxy); // nus proxy is not currently threadsafe
    VPLMutex_Unlock(&__nusMutex);
}
