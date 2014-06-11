#include "mdd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *specificUUID = NULL;

static void resolve_cb(MDDInstance inc, MDDResolve *resolve, void *arg)
{
    unsigned char *raw_txt;
    unsigned int i = 0;

    if (specificUUID != NULL) {
        if (!strcmp(specificUUID, MDD_XhtGet(resolve->txt_xht, "UUID"))) {
            if (resolve->ipv4 != NULL) {
                printf("IPv4: %s\n", resolve->ipv4);
                MDD_Shutdown(inc);
            }
            if (resolve->ipv6_count > 0) {
                for (i = 0; i < resolve->ipv6_count; i++) {
                    printf("IPv6: %s\n", resolve->ipv6[i]);
                }
            }
        }
        return;
    }

    printf("-------------\n");
    printf("Remote connection type: %s\n", resolve->remote_conn_type == MDDConnType_USB3 ? "USB 3.0" : resolve->remote_conn_type == MDDConnType_ETH ? "Ethernet" : resolve->remote_conn_type == MDDConnType_WIFI ? "WiFi" : "Unknown");
    printf("Self connection type: %s\n", resolve->self_conn_type == MDDConnType_USB3 ? "USB 3.0" : resolve->self_conn_type == MDDConnType_ETH ? "Ethernet" : resolve->self_conn_type == MDDConnType_WIFI ? "WiFi" : "Unknown");
    printf("Interface index: %u\n", resolve->ifindex);
    printf("Host: %s, domain: %s, TTL: %u\n", resolve->host, resolve->domain, resolve->ttl);
    printf("Port: %u\n", resolve->srv.port);
    if (resolve->ipv4 != NULL) {
        printf("IPv4: %s\n", resolve->ipv4);
    }
    if (resolve->ipv6_count > 0) {
        for (i = 0; i < resolve->ipv6_count; i++) {
            printf("IPv6: %s\n", resolve->ipv6[i]);
        }
    }
    raw_txt = MDD_XhtToStr(resolve->txt_xht, NULL);
    printf("TXT: %s\n", raw_txt);
    free(raw_txt);
    printf("TXT version: %s\n", MDD_XhtGet(resolve->txt_xht, "version"));
    printf("-------------\n");
}

int main(int argc, char* argv[])
{
    MDDInstance i = MDD_Init();
    MDD_ResolveConnectionType(i, "ConnType");
    if (argc == 2 && argv[1] != NULL) {
        specificUUID = strdup(argv[1]);
        printf("Launch with specific UUID: %s\n", specificUUID);
    }

    if (i != NULL) {
        MDD_Resolve(i, "_simpletest._tcp.local.", 60, resolve_cb, NULL); // repeat every 1 minute
        //MDD_Resolve(i, "_vss._tcp.local.", 60, resolve_cb, NULL); // repeat every 1 minute
        //MDD_Resolve(i, "_acss._tcp.local.", 60, resolve_cb, NULL); // repeat every 1 minute
        //MDD_Resolve(i, "_rcs._tcp.local.", resolve_cb, NULL);
        //MDD_Resolve(i, "_http._tcp.local.", resolve_cb, NULL);
        MDD_Start(i);
    } else {
        printf("Fail to init mdd\n");
    }

    return 0;
}
