#include "mdd_net.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>

#include "mdd_utils.h"
#include "log.h"

int MDDNet_mdd2af(MDDNetProto_t family)
{
    switch (family) {
        case MDDNet_INET:
            return AF_INET;
        case MDDNet_INET6:
            return AF_INET6;
        case MDDNet_UNSPEC:
        default:
            return AF_UNSPEC;
    }
}

int MDDNet_af2mdd(int family)
{
    switch (family) {
        case AF_INET:
            return MDDNet_INET;
        case AF_INET6:
            return MDDNet_INET6;
        default:
            return MDDNet_UNSPEC;
    }
}

int MDDNet_Inet_pton(MDDNetProto_t family, const char *addr_str, MDDNetAddr_t *addr)
{
    if (addr_str == NULL || addr == NULL) {
        return MDD_ERROR;
    }

    if (family == MDDNet_INET) {
        struct in_addr ia;
        if (inet_pton(AF_INET, addr_str, &ia) <= 0) {
            return MDD_ERROR;
        }
        addr->family = MDDNet_INET;
        addr->ip.ipv4 = ia.s_addr;
        return MDD_OK;
    } else if (family == MDDNet_INET6) {
        struct in6_addr ia6;
        if (inet_pton(AF_INET6, addr_str, &ia6) <= 0) {
            return MDD_ERROR;
        }
        addr->family = MDDNet_INET6;
        memcpy(addr->ip.ipv6, ia6.s6_addr, 16);
        return MDD_OK;
    } else {
        return MDD_ERROR;
    }
}

char *MDDNet_Inet_ntop(MDDNetProto_t family, const MDDNetAddr_t *addr)
{
    if (addr == NULL) {
        return NULL;
    }

    if (family == MDDNet_INET) {
        struct in_addr ia;
        char *addr_str = malloc(INET_ADDRSTRLEN);

        ia.s_addr = addr->ip.ipv4;
        memset(addr_str, 0, INET_ADDRSTRLEN);

        if (inet_ntop(AF_INET, &ia, addr_str, INET_ADDRSTRLEN) == NULL) {
            LOG_WARN("MDDNet_Inet_ntop failed, error: %s\n", strerror(errno));
            free(addr_str);
            return NULL;
        }
        return addr_str;
    } else if (family == MDDNet_INET6) {
        struct in6_addr ia6;
        char *addr_str = malloc(INET6_ADDRSTRLEN);

        memcpy(ia6.s6_addr, addr->ip.ipv6, 16);
        memset(addr_str, 0, INET6_ADDRSTRLEN);

        if (inet_ntop(AF_INET6, &ia6, addr_str, INET6_ADDRSTRLEN) == NULL) {
            LOG_WARN("MDDNet_Inet_ntop failed, error: %s\n", strerror(errno));
            free(addr_str);
            return NULL;
        }
        return addr_str;
    } else {
        return NULL;
    }
}

unsigned short int MDDNet_Htons(unsigned short int hostshort)
{
    return htons(hostshort);
}

unsigned long int MDDNet_Htonl(unsigned long int hostlong)
{
    return htonl(hostlong);
}

unsigned short int MDDNet_Ntohs(unsigned short int netshort)
{
    return ntohs(netshort);
}

unsigned long int MDDNet_Ntohl(unsigned long int netlong)
{
    return ntohl(netlong);
}
