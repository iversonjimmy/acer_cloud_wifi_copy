//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include "vpl_net.h"

#include <unistd.h>
#include <netdb.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdlib.h>

#ifdef IOS
# include <ifaddrs.h>
# include <sys/sysctl.h>
#endif

#include "vplu.h"
#include "vpl_socket.h"

// Sanity-check our kludge for the botched refactoring of
// <netinet/in.h> inline into vpl__conv.h!
#if (VPLNET_ADDRSTRLEN != INET_ADDRSTRLEN)
#error VPLNET_ADDRSTRLEN != INET_ADDRSTRLEN
#endif


VPLNet_addr_t 
VPLNet_GetLocalAddr(void)
{
    char szBuffer[VPLNET_MAX_INTERFACES * sizeof(struct ifreq)];
    VPLSocket_t sockfd;
    struct ifconf ifConf;
    struct ifreq ifReq;
    struct sockaddr_in local_addr;
    int rc, i;

    // Create an unbound datagram socket to do the SIOCGIFADDR ioctl on.
    sockfd.fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd.fd == -1) {
        return VPLNET_ADDR_INVALID;
    }

    // Get the interface configuration information...
    ifConf.ifc_len = sizeof szBuffer;
    ifConf.ifc_ifcu.ifcu_buf = (caddr_t) szBuffer;
    rc = ioctl(sockfd.fd, SIOCGIFCONF, &ifConf);

    if (rc < 0) {
        close(sockfd.fd);
        return VPLNET_ADDR_INVALID;
    }

    memset(&local_addr, 0, sizeof(local_addr));
    // Cycle through the list of interfaces looking for IP addresses.
    for (i = 0; i < ifConf.ifc_len;) {
        struct ifreq* pifReq =
            (struct ifreq*)((caddr_t) ifConf.ifc_req + i);
        i += sizeof(*pifReq);

        // See if this is the sort of interface we want to deal with.
        strcpy(ifReq.ifr_name, pifReq->ifr_name);
        if (ioctl(sockfd.fd, SIOCGIFFLAGS, &ifReq) < 0) {
            // Can't get interface flags. Can't choose this address.
            continue;
        }

        // Skip loopback, point-to-point and down interfaces,
        // except don't skip down interfaces
        // if we're trying to get a list of configurable interfaces.
        if ((ifReq.ifr_flags & IFF_LOOPBACK)
            || (!(ifReq.ifr_flags & IFF_UP))) {
            continue;
        }
        else if(pifReq->ifr_addr.sa_family == AF_INET) {
            // Get a pointer to the address...
            memcpy(&local_addr, &pifReq->ifr_addr,
                    sizeof pifReq->ifr_addr);

            // Stop if it's not the loopback interface.
            if (local_addr.sin_addr.s_addr &&
                local_addr.sin_addr.s_addr != htonl(INADDR_LOOPBACK)) {
                break;
            }

        }
    }

    close(sockfd.fd);
    return local_addr.sin_addr.s_addr;
}

#ifdef IOS
static int getDefaultGateway(in_addr_t* addr)
{
    int mib[] = {
        CTL_NET,
        PF_ROUTE,
        0,
        AF_INET,
        NET_RT_FLAGS,
        RTF_GATEWAY
    };
    size_t l;
    char* buf;
    char* p;
    struct rt_msghdr* rt;
    struct sockaddr* sa;
    struct sockaddr* sa_tab[RTAX_MAX];
    int i;
    int rc = -1;
    
    if (sysctl(mib, sizeof(mib) / sizeof(int), 0, &l, 0, 0) < 0) {
        return -1;
    }
    
    if (l > 0) {
        buf = malloc(l);
        if (sysctl(mib, sizeof(mib) / sizeof(int), buf, &l, 0, 0) < 0) {
            return -1;
        }
        
        for (p = buf; p < buf + l; p += rt->rtm_msglen) {
            rt = (struct rt_msghdr *)p;
            sa = (struct sockaddr *)(rt + 1);
            for (i = 0; i < RTAX_MAX; i++) {
                if (rt->rtm_addrs & (1 << i)) {
                    sa_tab[i] = sa;
                    sa = (struct sockaddr *)((char *)sa + sa->sa_len);
                } else {
                    sa_tab[i] = NULL;
                }
            }
            
            if( ((rt->rtm_addrs & (RTA_DST|RTA_GATEWAY)) == (RTA_DST|RTA_GATEWAY)) &&
               sa_tab[RTAX_DST]->sa_family == AF_INET &&
               sa_tab[RTAX_GATEWAY]->sa_family == AF_INET) {
                
                unsigned char octet[4]  = {0, 0, 0, 0};
                for (int i = 0; i < 4; i++) {
                    octet[i] = ( ((struct sockaddr_in *)(sa_tab[RTAX_GATEWAY]))->sin_addr.s_addr >> (i*8) ) & 0xFF;
                }
                
                if (((struct sockaddr_in *)sa_tab[RTAX_DST])->sin_addr.s_addr == 0) {
                    *addr = ((struct sockaddr_in *)(sa_tab[RTAX_GATEWAY]))->sin_addr.s_addr;
                    rc = VPL_OK;
                    break;
                }
            }
        }
        
        free(buf);
    }
    return rc;
}


VPLNet_addr_t 
VPLNet_GetDefaultGateway(void) 
{
    VPLNet_addr_t rv = VPLNET_ADDR_INVALID;
    in_addr_t addr;
    if (getDefaultGateway(&addr) == VPL_OK) {
        rv = addr;
    }
    return rv;
}

int 
VPLNet_GetLocalAddrList(VPLNet_addr_t* ppAddrs,
                        VPLNet_addr_t* ppNetmasks)
{
    int rv = VPL_OK;
    int ctr= 0;
    int tempresults[VPLNET_MAX_INTERFACES];
    int tempnetmasks[VPLNET_MAX_INTERFACES];
    struct ifaddrs *ifa;
    struct ifaddrs *ifList;
    struct sockaddr_in LocalAddr;
    
    if (ppAddrs == NULL) {
        return VPLNET_ADDR_INVALID;
    }
    
    rv = getifaddrs(&ifList); // should check for errors
    
    if (rv < 0) {
        return VPLError_XlatErrno(rv);
    }
    
    for (ifa = ifList; ifa != NULL; ifa = ifa->ifa_next) {
        //ifa->ifa_addr // interface address
        //ifa->ifa_netmask // subnet mask
        //ifa->ifa_dstaddr // broadcast address, NOT router address
        //tempresults[ctr] = ifa->ifa_addr
        
        if (ifa->ifa_addr->sa_family == AF_INET) {
            memcpy (&LocalAddr, &ifa->ifa_addr, sizeof ifa->ifa_addr);
            if (LocalAddr.sin_addr.s_addr && LocalAddr.sin_addr.s_addr != htonl (INADDR_LOOPBACK)) {
                tempresults[ctr] = LocalAddr.sin_addr.s_addr;
                
                if (ppNetmasks) {
                    if (ifa->ifa_netmask->sa_family == AF_INET) {
                        memcpy (&LocalAddr, &ifa->ifa_netmask, sizeof ifa->ifa_netmask);
                        tempnetmasks[ctr] = LocalAddr.sin_addr.s_addr;
                    } else {
                        tempnetmasks[ctr] = 0xffffffff;
                    }
                }
                
                ctr++;
            }
        }
    }
    
    freeifaddrs(ifList);
    
    memcpy(ppAddrs, tempresults, sizeof(VPLNet_addr_t)*ctr);
    
    if (ppNetmasks) {
        memcpy(ppNetmasks, tempnetmasks, sizeof(VPLNet_addr_t)*ctr);
    }
    
    return rv;
}

#else

VPLNet_addr_t 
VPLNet_GetDefaultGateway(void) 
{
    VPLNet_addr_t rv = VPLNET_ADDR_INVALID;

    /* Linux: read from /proc/net/route */
    FILE *in;
    char line[256];

    if (!(in = fopen("/proc/net/route", "r"))) {
        return rv;
    }

    while (fgets(line, sizeof(line), in)) {
        struct rtentry  rtent;
        struct rtentry  *rt;
        char            rtent_name[32];
        int             refcount, flags, metric;
        unsigned        use;
        struct sockaddr_in rt_dest, rt_gw, rt_mask;

        memset(&rtent, (0), sizeof(rtent));
        rt = &rtent;
        rt->rt_dev = rtent_name;

        memset(&rt_dest, 0, sizeof(rt_dest));
        memset(&rt_gw, 0, sizeof(rt_gw));
        memset(&rt_mask, 0, sizeof(rt_mask));

        /*
         * as with /proc/net/route in Linux kernel 1.99.14 and later:
         * Iface Dest GW Flags RefCnt Use Metric Mask MTU Win IRTT
         * eth0 0A0A0A0A 00000000 05 0 0 0 FFFFFFFF 1500 0 0
         */
        if (8 != sscanf(line, "%s %x %x %x %u %d %d %x %*d %*d %*d\n",
                        rt->rt_dev,
                        &rt_dest.sin_addr.s_addr,
                        &rt_gw.sin_addr.s_addr,
                        /*
                         * XXX: fix type of the args
                         */
                        &flags, &refcount, &use, &metric,
                        &rt_mask.sin_addr.s_addr)) {
            continue;
        }

        rt->rt_flags = flags;
        rt->rt_metric = metric;

        if (rt_gw.sin_addr.s_addr) {
            rv = rt_gw.sin_addr.s_addr;
        }
    }

    if (in) {
        fclose(in);
    }

    return rv;
}

int 
VPLNet_GetLocalAddrList(VPLNet_addr_t* ppAddrs,
        VPLNet_addr_t* ppNetmasks) 
{
    int rv = VPL_OK;
    int tempresults[VPLNET_MAX_INTERFACES];
    int tempnetmasks[VPLNET_MAX_INTERFACES];
    int ctr=0;

    //
    // Posix will use an Ioctl call to get the IPAddress List
    //
    char szBuffer[VPLNET_MAX_INTERFACES*sizeof(struct ifreq)];
    struct ifconf ifConf;
    struct ifreq ifReq;
    int nResult;
    int LocalSock;
    struct sockaddr_in LocalAddr;
    int i = 0;

    if (ppAddrs == NULL) {
        return VPLNET_ADDR_INVALID;
    }

    /* Create an unbound datagram socket to do the SIOCGIFADDR ioctl on. */
    if ((LocalSock = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        return VPLError_XlatErrno(errno);
    }

    /* Get the interface configuration information... */
    ifConf.ifc_len = sizeof szBuffer;
    ifConf.ifc_ifcu.ifcu_buf = (caddr_t)szBuffer;
    nResult = ioctl(LocalSock, SIOCGIFCONF, &ifConf);
    if (nResult < 0) {
        rv = VPLError_XlatErrno(nResult);
        close(LocalSock);
        return rv;
    }

    /* Cycle through the list of interfaces looking for IP addresses. */
    while (ctr < VPLNET_MAX_INTERFACES && i < ifConf.ifc_len) {
        struct ifreq* pifReq = (struct ifreq*)((caddr_t)ifConf.ifc_req + i);
        i += sizeof(*pifReq);
        /* See if this is the sort of interface we want to deal with. */
        strcpy (ifReq.ifr_name, pifReq -> ifr_name);
        rv = ioctl (LocalSock, SIOCGIFFLAGS, &ifReq);
        if (rv < 0) {
            close(LocalSock);
            return VPLError_XlatErrno(rv);
        }

        /* Skip loopback, point-to-point and down interfaces, */
        /* except don't skip down interfaces */
        /* if we're trying to get a list of configurable interfaces. */
        if ((ifReq.ifr_flags & IFF_LOOPBACK) ||
            (!(ifReq.ifr_flags & IFF_UP))) {
            continue;
        }

        if (ppNetmasks) {
            rv = ioctl (LocalSock, SIOCGIFNETMASK, &ifReq);
            if(rv < 0) {
                close(LocalSock);
                return VPLError_XlatErrno(rv);
            }
        }

        if (pifReq->ifr_addr.sa_family == AF_INET) {
            /* Get a pointer to the address... */
            memcpy (&LocalAddr, &pifReq -> ifr_addr, sizeof pifReq -> ifr_addr);
            if (LocalAddr.sin_addr.s_addr &&
                LocalAddr.sin_addr.s_addr != htonl (INADDR_LOOPBACK)) {
                tempresults[ctr] = LocalAddr.sin_addr.s_addr;

                if (ppNetmasks) {
                    if (ifReq.ifr_netmask.sa_family == AF_INET) {
                        memcpy (&LocalAddr, &ifReq.ifr_netmask,
                                sizeof ifReq.ifr_netmask);
                        tempnetmasks[ctr] = LocalAddr.sin_addr.s_addr;
                    }
                    else {
                        tempnetmasks[ctr] = 0xffffffff;
                    }
                }

                ++ctr;
            }
        }
    }
    close(LocalSock);

    memcpy(ppAddrs, tempresults, sizeof(VPLNet_addr_t)*ctr);

    if (ppNetmasks) {
        memcpy(ppNetmasks, tempnetmasks, sizeof(VPLNet_addr_t)*ctr);
    }

    return(ctr);
}

#endif

VPLNet_addr_t 
VPLNet_GetAddr(const char* hostName) 
{
    struct addrinfo ai_hints;
    struct addrinfo* ai_res = NULL, *ai = NULL;
    int rc;
    VPLNet_addr_t addr = VPLNET_ADDR_INVALID;
    int addrcnt = 0;
    int match;
    int i;
    VPLNet_addr_t  *choices;

    if (hostName == NULL) {
        return addr;
    }

    memset(&ai_hints, 0, sizeof(ai_hints));
    ai_hints.ai_family = AF_INET;

    rc = getaddrinfo(hostName, NULL, &ai_hints, &ai_res);
    if (rc != 0) {
        return VPLNET_ADDR_INVALID;
    }

    // Select the address to return at random from those retrieved, 
    // excluding loopback address.
    // Randomness doesn't have to be very strong.

    ai = ai_res;
    while(ai != NULL) {
        if(ai->ai_family == AF_INET) {
            struct sockaddr_in * sockAddrPtr =
                (struct sockaddr_in *) ai->ai_addr;
            
            if(sockAddrPtr != NULL) {
                addr = sockAddrPtr->sin_addr.s_addr;
                if(addr != htonl(INADDR_LOOPBACK)) {
                    addrcnt++;
                }
            }
        }
        
        ai = ai->ai_next;
    }

    // If no non-loopack addresses found, return either loopback
    // (only address found) or invalid address.
    if(addrcnt > 0) {
        choices = malloc(addrcnt * sizeof(VPLNet_addr_t));

        if (choices == NULL) {
            freeaddrinfo(ai_res);
            return addr;
        }

        ai = ai_res;
        addrcnt = 0;

        while (ai != NULL) {
            if (ai->ai_family == AF_INET) {
                struct sockaddr_in * sockAddrPtr =
                    (struct sockaddr_in *) ai->ai_addr;
                
                if (sockAddrPtr != NULL) {
                    addr = sockAddrPtr->sin_addr.s_addr;
                    if (addr != htonl(INADDR_LOOPBACK)) {
                        match = 0;

                        for (i = 0; i < addrcnt && !match; i++) {
                            match = addr == choices[i];
                        }

                        if (!match) {
                            choices[addrcnt++] = addr;
                        }
                    }
                }
            }

            ai = ai->ai_next;
        }

        addr = choices[(rand() ^ (VPLTime_GetTimeStamp() >> 7)) % addrcnt];

        free(choices);
    }

    if (ai_res != NULL) {
        freeaddrinfo(ai_res);
    }

    return addr;
}

const char* 
VPLNet_Ntop(const VPLNet_addr_t* src, char* dest, size_t count) 
{
    const char* rv = NULL;

    if (src == NULL) {
        return NULL;
    }
    if (dest == NULL) {
        return NULL;
    }
    if (count == 0) {
        return NULL;
    }

    rv = inet_ntop(AF_INET, src, dest, count);
    if (rv == NULL) {
        memset(dest, 0, count);
    }

    return rv;
}

