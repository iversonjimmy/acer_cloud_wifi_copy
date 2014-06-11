#ifndef mdd_net_h
#define mdd_net_h

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MDDNet_UNSPEC,
    MDDNet_INET,
    MDDNet_INET6
} MDDNetProto_t;

#include "mdd_net_platform.h"

typedef struct {
    MDDNetProto_t family;
    union {
        // ipv4 addr in network byte order
        unsigned long int ipv4;
        // ipv6 addr in network byte order
        unsigned char ipv6[16];
        // General data to access ip
        unsigned char data[1];
    } ip;
} MDDNetAddr_t;

/// MDD protocol family to AF protocol family
int MDDNet_mdd2af(MDDNetProto_t family);
/// AF protocol family to MDD protocol family
int MDDNet_af2mdd(int family);

/// Covert from printable address to MDDNetAddr_t
int MDDNet_Inet_pton(MDDNetProto_t family, const char *addr_str, MDDNetAddr_t *addr);
/// Convert MDDNetAddr_t to prinable address
/// If failed, NULL will be return
/// If successed, the newly allocate string will be return
char *MDDNet_Inet_ntop(MDDNetProto_t family, const MDDNetAddr_t *addr);

unsigned short int MDDNet_Htons(unsigned short int hostshort);
unsigned long int MDDNet_Htonl(unsigned long int hostlong);
unsigned short int MDDNet_Ntohs(unsigned short int netshort);
unsigned long int MDDNet_Ntohl(unsigned long int netlong);

#ifdef __cplusplus
}
#endif

#endif //mdd_time_h
