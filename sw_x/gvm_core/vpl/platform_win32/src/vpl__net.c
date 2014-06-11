/*
 *                Copyright (C) 2008, BroadOn Communications Corp.
 *
 *   These coded instructions, statements, and computer programs contain
 *   unpublished proprietary information of BroadOn Communications Corp.,
 *   and are protected by Federal copyright law. They may not be disclosed
 *   to third parties or copied or duplicated in any form, in whole or in
 *   part, without the prior written consent of BroadOn Communications Corp.
 *
 */

#include "vpl_net.h"
#include "vplu.h"
#include "vplu_debug.h"

#ifndef VPL_PLAT_IS_WINRT
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>  /* For SIO_UDP_CONNRESET */
#include <iphlpapi.h> /* For GetIpForwardTable */
#endif
#include <assert.h>
#include <time.h>

/* Call gethostname() (defined in winsock2) and cache the result.
 * Refresh every 5 seconds.
 *
 * The motivation for this function is an observation that calling gethostname() is quite expensive.
 * Details can be found in Remark section of http://msdn.microsoft.com/en-us/library/ms738527%28VS.85%29.aspx.
 */
static int VPLNet_GetHostName(char *hostname_out)
{
#ifdef VPL_PLAT_IS_WINRT
    return VPL_ERR_NOOP;
#else
    static char hostname[256];  // http://msdn.microsoft.com/en-us/library/ms738527%28VS.85%29.aspx says 256 "will always be adequate".
    static __time64_t last_refreshed = 0;
    static volatile LONG lock = 0;

    int rv = 0;
    __time64_t curtime;
    _time64(&curtime);

    // acquire lock using atomic test-and-set
    while (InterlockedCompareExchange(&lock, (LONG)1, (LONG)0) == (LONG)1) {
        Sleep(0);  // yield to other threads
    }

    if (curtime > last_refreshed + 5) {  // if at least 5 seconds have elapsed since last refresh
        rv = gethostname(hostname, (int)sizeof(hostname));
        if (rv == 0) {
            last_refreshed = curtime;
        }
        else {
            if (WSAGetLastError() == WSANOTINITIALISED) {
                // Winsock should have been initialized via VPL_Init
                VPL_REPORT_WARN("VPLNet_GetLocalAddr called before VPL_Init");
            }
        }
    }

    strcpy(hostname_out, hostname);

    // release lock
    lock = 0;

    return rv;
#endif
}

VPLNet_addr_t VPLNet_GetLocalAddr(void)
{
    char hostname[256];
    int rv = VPLNet_GetHostName(hostname);
    if (rv == 0) {
        return VPLNet_GetAddr(hostname);
    }
    else {
        return VPLNET_ADDR_INVALID;
    }
}

VPLNet_addr_t VPLNet_GetDefaultGateway(void) {
    VPLNet_addr_t rv = VPLNET_ADDR_INVALID;

    MIB_IPFORWARDTABLE*   pft;
    DWORD  tableSize = 0;

    GetIpForwardTable (NULL, &tableSize, FALSE);
    pft = (MIB_IPFORWARDTABLE*) malloc(tableSize);
    if (pft) {
        if (GetIpForwardTable (pft, &tableSize, TRUE) == NO_ERROR) {
            unsigned int i;
            for (i = 0; i < pft->dwNumEntries; i++) {
                if (pft->table[i].dwForwardDest == 0) {
                    rv = pft->table[i].dwForwardNextHop;
                }
            }
        }
        free(pft);
    }

    return rv;
}

int VPLNet_GetLocalAddrList(VPLNet_addr_t* pp_addrs,
                            VPLNet_addr_t* pp_netmasks) {
    int tempresults[VPLNET_MAX_INTERFACES];
    int tempnetmasks[VPLNET_MAX_INTERFACES];
    int ctr=0;

    MIB_IPADDRTABLE*   pft;
    DWORD  tableSize = 0;

    if (pp_addrs == NULL) {
        return VPLNET_ADDR_INVALID;
    }

    GetIpAddrTable (NULL, &tableSize, FALSE);
    pft = (MIB_IPADDRTABLE*) malloc(tableSize);
    if (pft) {
        if (GetIpAddrTable (pft, &tableSize, TRUE) == NO_ERROR) {
            unsigned int i;

            for (i = 0; i < pft->dwNumEntries && ctr < VPLNET_MAX_INTERFACES;
                    i++) {
                if (pft->table[i].dwAddr &&
                    pft->table[i].dwAddr != htonl( INADDR_LOOPBACK )) {
                    tempresults[ctr] = pft->table[i].dwAddr;
                    tempnetmasks[ctr] = pft->table[i].dwMask;
                    ctr++;
                }
            }
        }
        free(pft);
    }

    memcpy(pp_addrs,tempresults,sizeof(VPLNet_addr_t)*ctr);

    if (pp_netmasks != NULL) {
        memcpy(pp_netmasks,tempnetmasks,sizeof(VPLNet_addr_t)*ctr);
    }

    return(ctr);
}

VPLNet_addr_t VPLNet_GetAddr(const char* hostname) {
    struct addrinfo ai_hints;
    struct addrinfo *ai_res = NULL, *ai = NULL;
    int rc;
    VPLNet_addr_t addr = VPLNET_ADDR_INVALID;
    int addrcnt = 0;
    int addrselect;

    if (hostname == NULL) {
        return addr;
    }

    memset(&ai_hints, 0, sizeof(ai_hints));
    ai_hints.ai_family = PF_INET;

    rc = getaddrinfo(hostname, "", &ai_hints, &ai_res);
    if (rc != 0) {
        if (WSAGetLastError() == WSANOTINITIALISED) {
            // Winsock should have been initialized via VPL_Init
            VPL_REPORT_WARN("VPLNet_GetLocalAddr called before VPL_Init");
        }
        return VPLNET_ADDR_INVALID;
    }

    // Select the address to return at random from those retrieved, 
    // excluding loopback address.
    // Randomness doesn't have to be very strong.

    ai = ai_res;
    while (ai != NULL) {
        if(ai->ai_family == AF_INET) {
            struct sockaddr_in * sockAddrPtr =
                (struct sockaddr_in *) ai->ai_addr;

            if (sockAddrPtr) {
                addr = sockAddrPtr->sin_addr.s_addr;
                if (addr != htonl( INADDR_LOOPBACK )) {
                    addrcnt++;
                }
            }
        }
        ai = ai->ai_next;
    }

    // If no non-loopack addresses found, return either loopback
    // (only address found) or invalid address.
    if(addrcnt > 1) {
        addrselect = rand() % addrcnt;
        
        ai = ai_res;
        addrcnt = 0;
        while (ai != NULL) {
            if(ai->ai_family == AF_INET) {
                struct sockaddr_in * sockAddrPtr =
                    (struct sockaddr_in *) ai->ai_addr;
                
                if (sockAddrPtr) {
                    addr = sockAddrPtr->sin_addr.s_addr;
                    if (addr != htonl( INADDR_LOOPBACK )) {
                        if(addrselect == addrcnt) {
                            break;
                        }
                        addrcnt++;
                    }
                }
            }
            ai = ai->ai_next;
        }
    }

    if (ai_res != VPL_NULL) {
        freeaddrinfo(ai_res);
    }

    return addr;
}

const char* VPLNet_Ntop(const VPLNet_addr_t *src, char *dst, size_t cnt) {
    const char* rv = NULL;

    if (src == NULL) {
        return NULL;
    }
    if (dst == NULL) {
        return NULL;
    }
    if (cnt < 1) {
        return NULL;
    }

    /* Windows don't have support for inet_ntop yet */
    /* Use inet_ntoa - okay since we only do IPv4 */
    rv = inet_ntoa(* ((struct in_addr*) src));
    if (rv == NULL) {
        memset(dst, 0, cnt);
        rv = NULL;
    }
    else {
#ifdef _MSC_VER
#pragma warning( push )
        // We explicitly add the null-terminator, so strncpy_s is not needed.
#pragma warning( disable : 4996 )
#endif

        strncpy(dst, rv, cnt);
        dst[cnt-1] = '\0'; /* make sure dst null terminated */
        rv = dst;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    }

    return rv;
}


