#include "mdd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "mdnsd.h"
#include "sdtxt.h"

#include "mdd.h"
#include "mdd_fdutil.h"
#include "mdd_iface.h"
#include "mdd_mutex.h"
#include "mdd_net.h"
#include "mdd_socket.h"
#include "mdd_utils.h"

#include "vplex_assert.h"
#include "log.h"

#define DEFAULT_IF_RETRY_TIMEOUT 500 // 500 ms
#define DEFAULT_MDNS_TTL 300 // 5 minutes
#define LOCAL_IPV4_ADDR "127.0.0.1" // loopback ipv4 address
#define RETRY_TO_TTL_FACTOR 2 // ttl = max repeat interval x 2

typedef enum {
    MDDEvent_IfAdded = 0,
    MDDEvent_IfRemoved,
    MDDEvent_IfRetry,
    MDDEvent_ResetRPublish,
} MDDEventType;

typedef struct {
    MDDIface_t iface;
    MDDSocket_t msocket;
    MDDSocket_t usocket;
} MDDIfaceSocket;

typedef struct {
    MDDIface_t iface;
} MDDEventIfData;

typedef struct MDDEvent_struct {
    MDDInstance instance;
    MDDEventType type;
    void *data;
    struct MDDEvent_struct *next;
} MDDEvent;

typedef struct MDDIfaceSocketList_struct {
    MDDIfaceSocket *ifsocket;
    struct MDDIfaceSocketList_struct *prev;
    struct MDDIfaceSocketList_struct *next;
} MDDIfaceSocketList;

typedef struct MDDIfacelist_struct {
    MDDIface_t iface;
    struct MDDIfacelist_struct *prev;
    struct MDDIfacelist_struct *next;
} MDDIfacelist;

struct MDDInstance_struct {
    MDDSocket_t socket_wakeup;
    MDDIfaceSocketList *if_list;
    mdnsd mdnsd;
    int restart;
    int shutdown;
    MDDIfacelist *mcast_if_list;
    // event queue
    MDDEvent *event_head;
    MDDEvent *event_tail;
    MDDMutex_t event_lock;
    // used for publish
    char *publish_origin_host_name;
    char *publish_host_name;
    char *publish_service_name;
    unsigned short publish_service_port;
    char *publish_connection_type_key;
    MDDXht publish_txt_xht;
    int publish_max_rinterval;
    int (*append_txt_cb)(MDDInstance, MDDXht, void *);
    void *append_txt_cb_arg;
    // used for resolve
    char *query_service_name;
    int query_rinterval;
    char *resolve_connection_type_key;
    void (*resolve_cb)(MDDInstance, MDDResolve *, void *);
    void *resolve_cb_arg;
};

struct MDDXht_wrap {
    xht xht;
};

MDDSocket_t msock_ipv4();
int add_to_event_queue(MDDInstance instance, MDDEvent *event);
void add_to_iflist(MDDInstance instance, MDDIface_t iface);
void remove_from_iflist(MDDInstance instance, MDDIface_t iface);
int add_to_multicast_group(MDDInstance instance, MDDIface_t iface);
int remove_from_multicast_group(MDDInstance instance, MDDIface_t iface);
void query_on_interface(MDDInstance instance, MDDIface_t iface);
void publish_on_interface(MDDInstance instance, MDDIface_t iface);
char *hostname_with_random(const char *host_name);
int wakeup_main_thread(MDDInstance instance);

static int _mdd_resolve_cb(mdnsda a, MDDIface_t iface, void *arg)
{
    MDDInstance instance = (MDDInstance)arg;
    MDDResolve *resolve_info;
    MDDNetAddr_t addr;
    unsigned int i = 0;
    MDDIfaceType_t iface_type = MDDIfaceType_Unknown;

    if (!instance || !instance->resolve_cb) {
        LOG_ERROR("error on resolve callback, instance or resolve callback is null\n");
        return MDD_ERROR;
    }

    resolve_info = (MDDResolve *)malloc(sizeof(MDDResolve));
    memset(resolve_info, 0, sizeof(MDDResolve));

    resolve_info->domain = a->name;
    resolve_info->host = a->rdname;
    resolve_info->srv.priority = a->srv.priority;
    resolve_info->srv.weight = a->srv.weight;
    resolve_info->srv.port = a->srv.port;
    resolve_info->txt_xht = (MDDXht) a->txt_xht;
    resolve_info->ttl = a->ttl;
    resolve_info->ifindex = iface.ifindex;

    LOG_DEBUG("MDD resolve_cb: domain: %s host: %s ttl: %u, ifindex: %u\n",
              resolve_info->domain, resolve_info->host,
              resolve_info->ttl, resolve_info->ifindex);
    // fill ipv4 address
    if (a->ip > 0) {
        memset(&addr, 0, sizeof(MDDNetAddr_t));
        addr.family = MDDNet_INET;
        addr.ip.ipv4 = MDDNet_Htonl(a->ip);
        resolve_info->ipv4 = MDDNet_Inet_ntop(MDDNet_INET, &addr);
        LOG_DEBUG("ipv4: %s\n", resolve_info->ipv4);
    }
    // fill ipv6 address
    if (a->ipv6_count > 0) {
        resolve_info->ipv6_count = a->ipv6_count;
        resolve_info->ipv6 = malloc(sizeof(char *) * a->ipv6_count);
        for (i = 0; i < a->ipv6_count; i++) {
            char *addr_str = NULL;
            memset(&addr, 0, sizeof(MDDNetAddr_t));
            addr.family = MDDNet_INET6;
            memcpy(addr.ip.ipv6, a->ipv6[i], 16);
            addr_str = MDDNet_Inet_ntop(MDDNet_INET6, &addr);
            if (addr_str != NULL) {
                if (strstr(addr_str, "fe80") == addr_str) {
                    char *ll_addr_str = (char *)malloc(INET6_ADDRSTRLEN + 10); // The format for link-local addr would be addr%ifindex
                    memset(ll_addr_str, 0, INET6_ADDRSTRLEN + 10);
                    sprintf(ll_addr_str, "%s%%%u", addr_str, iface.ifindex);
                    free(addr_str);
                    addr_str = NULL;
                    resolve_info->ipv6[i] = ll_addr_str;
                } else {
                    resolve_info->ipv6[i] = addr_str;
                }
            } else {
                resolve_info->ipv6[i] = NULL;
            }
            LOG_DEBUG("ipv6: %s\n", resolve_info->ipv6[i]);
        }
    }

    // set connection type for remote
    resolve_info->remote_conn_type = a->remote_conn_type;

    // set connection type for self
    iface_type = MDDIface_Gettype(iface);
    switch (iface_type) {
        case MDDIfaceType_USB3:
            resolve_info->self_conn_type = MDDConnType_USB3;
            break;
        case MDDIfaceType_Ethernet:
            resolve_info->self_conn_type = MDDConnType_ETH;
            break;
        case MDDIfaceType_Wifi:
            resolve_info->self_conn_type = MDDConnType_WIFI;
            break;
        case MDDIfaceType_Unknown:
            resolve_info->self_conn_type = MDDConnType_UNKNOWN;
            break;
    }
    LOG_DEBUG("remote type: %d, self type: %d\n", resolve_info->remote_conn_type, resolve_info->self_conn_type);

    // call callback
    instance->resolve_cb(instance, resolve_info, instance->resolve_cb_arg);

    // free the report
    if (resolve_info->ipv4 != NULL) {
        free(resolve_info->ipv4);
    }
    if (resolve_info->ipv6 != NULL) {
        for (i = 0; i < resolve_info->ipv6_count; i++) {
            free(resolve_info->ipv6[i]);
        }
        free(resolve_info->ipv6);
    }
    free(resolve_info);

    return MDD_OK;
}

static int _mdd_answer_cb(mdnsda a, void *arg)
{
    // do not need to do anything on answer callback now
    return MDD_OK;
}

static int _mdd_append_txt_cb(xht txt_xht, MDDIface_t *iface, void *arg)
{
    MDDInstance instance = (MDDInstance)arg;
    int changed = 0;

    if (!instance) {
        LOG_ERROR("error on append txt callback, instance is null\n");
        return changed;
    }

    if (instance->publish_connection_type_key) {
        MDDIfaceType_t if_type = MDDIfaceType_Unknown;
        char value[8];
        char *origValue = NULL;

        if (iface) {
            if_type = MDDIface_Gettype(*iface);
        }
        memset(value, 0, 8);
        sprintf(value, "%d", if_type);
        origValue = MDD_XhtGet((MDDXht)txt_xht, instance->publish_connection_type_key);
        if (origValue == NULL || strcmp(origValue, value)) {
            MDD_XhtSet((MDDXht)txt_xht, instance->publish_connection_type_key, value);
            changed = 1;
        }
    }

    if (!instance->append_txt_cb) {
        return changed;
    }

    if (instance->append_txt_cb(instance, (MDDXht)txt_xht, instance->append_txt_cb_arg)) {
        changed = 1;
    }

    return changed;
}

static void _mdd_publish_conflict_cb(char *name, int type, void *arg)
{
    MDDInstance instance = (MDDInstance)arg;

    if (!instance) {
        return;
    }

    LOG_WARN("Conflicting name detected %s, type %d\n", name, type);
    if (instance->publish_host_name) {
        free(instance->publish_host_name);
    }
    instance->publish_host_name = hostname_with_random(instance->publish_origin_host_name);
    LOG_WARN("Retry with host name: %s\n", instance->publish_host_name);
    instance->restart = 1;
}

// the callback will be called by another thread
static int _mdd_iface_changed_cb(MDDIface_t iface, MDDIfaceState_t state, void *arg)
{
    MDDInstance instance = (MDDInstance)arg;
    MDDEventIfData *data = NULL;
    MDDEvent *mdd_event = NULL;

    if (!instance) {
        LOG_ERROR("error on interface changed callback, instance is null\n");
        return MDD_ERROR;
    }

    LOG_DEBUG("interface changed, state: %d, ifindex: %d, family: %d\n", state, iface.ifindex, iface.addr.family);
    if (iface.addr.family == MDDNet_INET) {
        LOG_DEBUG("ipv4 address: %lx\n", iface.addr.ip.ipv4);
    }

    // create new if changed event
    data = (MDDEventIfData *)malloc(sizeof(MDDEventIfData));
    memset(data, 0, sizeof(MDDEventIfData));
    data->iface = iface;

    mdd_event = (MDDEvent *)malloc(sizeof(MDDEvent));
    memset(mdd_event, 0, sizeof(MDDEvent));
    mdd_event->instance = instance;
    mdd_event->data = data;

    if (state == MDDIfaceState_Add) {
        mdd_event->type = MDDEvent_IfAdded;
    } else if (state == MDDIfaceState_Remove) {
        mdd_event->type = MDDEvent_IfRemoved;
    } else {
        LOG_WARN("interface changed, but got unknown state: %d\n", state);
        free(data);
        free(mdd_event);
        return MDD_ERROR;
    }

    add_to_event_queue(instance, mdd_event);
    // wake up main thread to handle the event
    wakeup_main_thread(instance);
    return MDD_OK;
}

static void _mdd_timeout_cb(void *arg)
{
    MDDEvent *mdd_event = (MDDEvent *)arg;
    MDDInstance instance = NULL;

    if (mdd_event == NULL) {
        LOG_ERROR("error on timeout callback, event is null\n");
        return;
    }

    instance = mdd_event->instance;
    if (instance == NULL) {
        LOG_ERROR("error on timeout callback, instance is null\n");
        return;
    }

    if (mdd_event->type == MDDEvent_IfRetry) {
        // add event to main thread
        add_to_event_queue(instance, mdd_event);
        // wake up main thread to handle the event
        wakeup_main_thread(instance);
    } else {
        LOG_ERROR("error on timeout callback, unknown event type %d\n", mdd_event->type);
    }
}

static int _mdd_connection_type_resolver(xht txt_xht, void *arg)
{
    MDDInstance instance = (MDDInstance)arg;
    MDDIfaceType_t if_type = MDDIfaceType_Unknown;

    if (!instance) {
        LOG_ERROR("error on connection type resolver, instance is null\n");
        return if_type;
    }

    if (instance->resolve_connection_type_key) {
        char *value = MDD_XhtGet((MDDXht)txt_xht, instance->resolve_connection_type_key);
        if (value) {
            int type = atoi(value);
            switch (type) {
                case MDDIfaceType_USB3:
                    if_type = MDDConnType_USB3;
                    break;
                case MDDIfaceType_Ethernet:
                    if_type = MDDConnType_ETH;
                    break;
                case MDDIfaceType_Wifi:
                    if_type = MDDConnType_WIFI;
                    break;
                default:
                    if_type = MDDConnType_UNKNOWN;
                    break;
            }
        }
    }
    return if_type;
}

static MDDSocket_t get_socket_for_iface(MDDInstance instance, MDDIface_t iface, int multicast)
{
    MDDIfaceSocketList *cur = NULL;

    if (instance == NULL) {
        return MDDSocket_INVALID;
    }

    for (cur = instance->if_list; cur != NULL; cur = cur->next) {
        if (MDDIface_Equal(&cur->ifsocket->iface, &iface)) {
            if (multicast) {
                return cur->ifsocket->msocket;
            } else {
                return cur->ifsocket->usocket;
            }
        }
    }
    return MDDSocket_INVALID;
}

int add_to_event_queue(MDDInstance instance, MDDEvent *mdd_event)
{
    if (!instance) {
        LOG_ERROR("error on add to event queue, instance is null\n");
        return MDD_ERROR;
    }

    // acquire lock
    MDDMutex_Lock(&instance->event_lock);

    // append to event queue
    if (instance->event_head == NULL) {
        instance->event_head = mdd_event;
    } else {
        instance->event_tail->next = mdd_event;
    }
    mdd_event->next = NULL;
    instance->event_tail = mdd_event;

    // release lock
    MDDMutex_Unlock(&instance->event_lock);

    return MDD_OK;
}

// create a local socket for wakeup main loop
static MDDSocket_t sock_wakeup()
{
    MDDSocket_t s;
    MDDSocketAddr_t in;

    // create ipv4 local address
    in.port = 0; //bind a random port
    in.family = MDDNet_INET;
    if (MDDNet_Inet_pton(MDDNet_INET, LOCAL_IPV4_ADDR, &in.addr) < 0) {
        LOG_ERROR("can't convert ipv4 address, addr: %s\n", LOCAL_IPV4_ADDR);
        return MDDSocket_INVALID;
    }

    if ((s = MDDSocket_Create(MDDNet_INET, MDDSocket_SOCK_DGRAM)) == MDDSocket_INVALID) {
        LOG_ERROR("Fail to create ipv4 wakeup socket\n");
        return MDDSocket_INVALID;
    }

    if (MDDSocket_Bind(s, &in) != MDD_OK) {
        LOG_ERROR("Fail to bind port for ipv4 wakeup socket\n");
        MDDSocket_Close(s);
        return MDDSocket_INVALID;
    }

    MDDSocket_Setnonblock(s);
    return s;
}

// create multicast socket for ipv4
MDDSocket_t msock_ipv4()
{
    MDDSocket_t s;
    MDDSocketAddr_t in;
    int flag = 1;
    int ittl = 255;
    char ttl = 255;
    int yes = 1;

    in.family = MDDNet_INET;
    in.port = MDDNet_Htons(MDNS_MULTICAST_PORT);
    in.addr.ip.ipv4 = 0; // Any address

    if ((s = MDDSocket_Create(MDDNet_INET, MDDSocket_SOCK_DGRAM)) == MDDSocket_INVALID) {
        LOG_ERROR("Fail to create ipv4 socket\n");
        return MDDSocket_INVALID;
    }

    // Set PKTINFO
    yes = 1;
    if (MDDSocket_Setsockopt(s, MDDSocket_IPPROTO_IP, MDDSocket_IP_PKTINFO, &yes, sizeof(yes)) != MDD_OK) {
        LOG_WARN("Fail to set PKTINFO\n");
    }

#ifdef SO_REUSEPORT
    MDDSocket_Setsockopt(s, MDDSocket_SOL_SOCKET, MDDSocket_SO_REUSEPORT, (char*)&flag, sizeof(flag));
#endif
    MDDSocket_Setsockopt(s, MDDSocket_SOL_SOCKET, MDDSocket_SO_REUSEADDR, (char*)&flag, sizeof(flag));

    if (MDDSocket_Bind(s, &in) != MDD_OK) {
        LOG_ERROR("Fail to bind port\n");
        MDDSocket_Close(s);
        return MDDSocket_INVALID;
    }

    MDDSocket_Setsockopt(s, MDDSocket_IPPROTO_IP, MDDSocket_IP_MULTICAST_TTL, &ttl, sizeof(ttl));
    MDDSocket_Setsockopt(s, MDDSocket_IPPROTO_IP, MDDSocket_IP_TTL, &ittl, sizeof(ittl));

    MDDSocket_Setnonblock(s);

    return s;
}

// create multicast socket for ipv6
static MDDSocket_t msock_ipv6()
{
    MDDSocket_t s;
    MDDSocketAddr_t in;
    int flag = 1;
    int ttl = 255;
    int yes = 1;

    in.family = MDDNet_INET6;
    in.port = MDDNet_Htons(MDNS_MULTICAST_PORT);
    memset(in.addr.ip.ipv6, 0, 16); // Any address

    if ((s = MDDSocket_Create(MDDNet_INET6, MDDSocket_SOCK_DGRAM)) == MDDSocket_INVALID) {
        LOG_ERROR("Fail to create ipv6 socket\n");
        return MDDSocket_INVALID;
    }

    // Set PKTINFO
    yes = 1;
    if (MDDSocket_Setsockopt(s, MDDSocket_IPPROTO_IPV6, MDDSocket_IPV6_RECVPKTINFO, &yes, sizeof(yes)) != MDD_OK) {
        LOG_WARN("Fail to set PKTINFO\n");
    }

#ifdef SO_REUSEPORT
    MDDSocket_Setsockopt(s, MDDSocket_SOL_SOCKET, MDDSocket_SO_REUSEPORT, (char*)&flag, sizeof(flag));
#endif
    MDDSocket_Setsockopt(s, MDDSocket_SOL_SOCKET, MDDSocket_SO_REUSEADDR, (char*)&flag, sizeof(flag));

    if (MDDSocket_Bind(s, &in) != MDD_OK) {
        LOG_ERROR("Fail to bind port\n");
        MDDSocket_Close(s);
        return MDDSocket_INVALID;
    }

    MDDSocket_Setsockopt(s, MDDSocket_IPPROTO_IPV6, MDDSocket_IPV6_MULTICAST_HOPS, &ttl, sizeof(ttl));
    MDDSocket_Setsockopt(s, MDDSocket_IPPROTO_IPV6, MDDSocket_IPV6_UNICAST_HOPS, &ttl, sizeof(ttl));
    MDDSocket_Setnonblock(s);

    return s;
}

// create unicast socket for ipv4
static MDDSocket_t usock_ipv4()
{
    MDDSocket_t s;
    MDDSocketAddr_t in;
    int yes = 1;

    in.family = MDDNet_INET;
    in.port = MDDNet_Htons(0); // Bind a random port
    in.addr.ip.ipv4 = 0; // Any address

    if ((s = MDDSocket_Create(MDDNet_INET, MDDSocket_SOCK_DGRAM)) == MDDSocket_INVALID) {
        LOG_ERROR("Fail to create ipv4 socket\n");
        return MDDSocket_INVALID;
    }

    // Set PKTINFO
    yes = 1;
    if (MDDSocket_Setsockopt(s, MDDSocket_IPPROTO_IP, MDDSocket_IP_PKTINFO, &yes, sizeof(yes)) != MDD_OK) {
        LOG_WARN("Fail to set PKTINFO\n");
    }

    if (MDDSocket_Bind(s, &in) != MDD_OK) {
        LOG_ERROR("Fail to bind port for unicast\n");
        MDDSocket_Close(s);
        return MDDSocket_INVALID;
    }

    MDDSocket_Setnonblock(s);

    return s;
}

// create unicast socket for ipv6
static MDDSocket_t usock_ipv6()
{
    MDDSocket_t s;
    MDDSocketAddr_t in;
    int yes = 1;

    in.family = MDDNet_INET6;
    in.port = MDDNet_Htons(0); // Bind a random port
    memset(in.addr.ip.ipv6, 0, 16); // Any address

    if ((s = MDDSocket_Create(MDDNet_INET6, MDDSocket_SOCK_DGRAM)) == MDDSocket_INVALID) {
        LOG_ERROR("Fail to create ipv6 socket\n");
        return MDDSocket_INVALID;
    }

    // Set PKTINFO
    yes = 1;
    if (MDDSocket_Setsockopt(s, MDDSocket_IPPROTO_IPV6, MDDSocket_IPV6_RECVPKTINFO, &yes, sizeof(yes)) != MDD_OK) {
        LOG_WARN("Fail to set PKTINFO\n");
    }

    if (MDDSocket_Bind(s, &in) != MDD_OK) {
        LOG_ERROR("Fail to bind port for unicast\n");
        MDDSocket_Close(s);
        return MDDSocket_INVALID;
    }

    MDDSocket_Setnonblock(s);

    return s;
}

static int is_if_in_iflist(MDDInstance instance, MDDIface_t iface)
{
    MDDIfaceSocketList *cur = NULL;

    if (instance == NULL) {
        return MDD_FALSE;
    }

    for (cur = instance->if_list; cur != NULL; cur = cur->next) {
        if (iface.ifindex == cur->ifsocket->iface.ifindex &&
                iface.addr.family == cur->ifsocket->iface.addr.family) {
            if (iface.addr.family == MDDNet_INET &&
                    iface.addr.ip.ipv4 == cur->ifsocket->iface.addr.ip.ipv4) {
                return MDD_TRUE;
            } else if (iface.addr.family == MDDNet_INET6 &&
                    !memcmp(iface.addr.ip.ipv6, cur->ifsocket->iface.addr.ip.ipv6, 16)) {
                return MDD_TRUE;
            }
        }
    }

    return MDD_FALSE;
}

void add_to_iflist(MDDInstance instance, MDDIface_t iface)
{
    MDDIfaceSocket *ifsocket = NULL;
    MDDIfaceSocketList *new_if = NULL;
    MDDIfaceSocketList *last = NULL;

    if (instance == NULL) {
        return;
    }

    ifsocket = malloc(sizeof(MDDIfaceSocket));
    ifsocket->iface = iface;
    if (iface.addr.family == MDDNet_INET) {
        ifsocket->msocket = msock_ipv4();
        ifsocket->usocket = usock_ipv4();
    } else {
        ifsocket->msocket = msock_ipv6();
        ifsocket->usocket = usock_ipv6();
    }
    if (ifsocket->msocket == MDDSocket_INVALID || ifsocket->usocket == MDDSocket_INVALID) {
        if (ifsocket->msocket != MDDSocket_INVALID) {
            LOG_ERROR("can't create multicast socket for interface: %u, family: %d\n", iface.ifindex, iface.addr.family);
            MDDSocket_Close(ifsocket->msocket);
        }
        if (ifsocket->usocket != MDDSocket_INVALID) {
            LOG_ERROR("can't create unicast socket for interface: %u, family: %d\n", iface.ifindex, iface.addr.family);
            MDDSocket_Close(ifsocket->usocket);
        }
        free(ifsocket);
        return;
    }

    new_if = malloc(sizeof(MDDIfacelist));
    memset(new_if, 0, sizeof(MDDIfacelist));
    new_if->ifsocket = ifsocket;

    if (instance->if_list == NULL) {
        instance->if_list = new_if;
    } else {
        // append to tail
        last = instance->if_list;
        while (last->next != NULL) {
            last = last->next;
        }
        last->next = new_if;
        new_if->prev = last;
    }
}

void remove_from_iflist(MDDInstance instance, MDDIface_t iface)
{
    MDDIfaceSocketList *cur = NULL;
    MDDIfaceSocketList *del = NULL;

    if (instance == NULL) {
        return;
    }

    cur = instance->if_list;
    while (cur != NULL) {
        if (MDDIface_Equal(&cur->ifsocket->iface, &iface)) {
            if (cur == instance->if_list) {
                instance->if_list = cur->next;
                if (cur->next) {
                    cur->next->prev = NULL;
                }
            } else {
                if (cur->prev) {
                    cur->prev->next = cur->next;
                }
                if (cur->next) {
                    cur->next->prev = cur->prev;
                }
            }
            del = cur;
            cur = cur->next;
            if (del->ifsocket->msocket != MDDSocket_INVALID) {
                MDDSocket_Close(del->ifsocket->msocket);
            }
            if (del->ifsocket->usocket != MDDSocket_INVALID) {
                MDDSocket_Close(del->ifsocket->usocket);
            }
            free(del->ifsocket);
            free(del);
            continue;
        }
        cur = cur->next;
    }
}

static int find_local_address(MDDInstance instance, MDDSocketAddr_t *from_addr, MDDIface_t *from_iface, MDDNetAddr_t *local_addr)
{
    MDDIfaceSocketList *cur = NULL;
    int j = 0;

    if (from_addr == NULL || from_iface == NULL) {
        return MDD_ERROR;
    }

    for (cur = instance->if_list; cur != NULL; cur = cur->next) {
        if (cur->ifsocket->iface.ifindex == from_iface->ifindex &&
                cur->ifsocket->iface.addr.family == from_iface->addr.family) {
            if (from_iface->addr.family == MDDNet_INET) {
                if ((cur->ifsocket->iface.addr.ip.ipv4 & cur->ifsocket->iface.netmask.ip.ipv4) ==
                        (from_addr->addr.ip.ipv4 & cur->ifsocket->iface.netmask.ip.ipv4)) {
                    local_addr->family = MDDNet_INET;
                    local_addr->ip.ipv4 = cur->ifsocket->iface.addr.ip.ipv4;
                    return MDD_OK;
                }
            } else if (from_iface->addr.family == MDDNet_INET6) {
                for (j = 0; j < 16; j++) {
                    if ((cur->ifsocket->iface.addr.ip.ipv6[j] & cur->ifsocket->iface.netmask.ip.ipv6[j]) ==
                            (from_addr->addr.ip.ipv6[j] & cur->ifsocket->iface.netmask.ip.ipv6[j])) {
                        continue;
                    } else {
                        break;
                    }
                }
                if (j == 16) {
                    local_addr->family = MDDNet_INET6;
                    memcpy(local_addr->ip.ipv6, cur->ifsocket->iface.addr.ip.ipv6, 16);
                    return MDD_OK;
                }
            }
        }
    }
    return MDD_ERROR;
}

static int create_query_and_publish(MDDInstance instance, MDDIface_t iface)
{
    if (!instance) {
        LOG_ERROR("error on create query and publish, instance is null\n");
        return MDD_ERROR;
    }

    // check for query
    if (instance->query_service_name != NULL) {
        query_on_interface(instance, iface);
    }

    // check for publish
    if (instance->publish_host_name != NULL && instance->publish_service_name != NULL) {
        //ASSERT(instance->publish_service_port >= 0);  //Warning: this comparison is always true
        publish_on_interface(instance, iface);
    }

    return MDD_OK;
}

static int handle_interface_changed(MDDInstance instance, MDDIface_t iface, MDDIfaceState_t state)
{
    if (!instance) {
        LOG_ERROR("error on interface changed callback, instance is null\n");
        return MDD_ERROR;
    }

    if (state == MDDIfaceState_Add) {
        LOG_DEBUG("New address added, family: %d\n", iface.addr.family);

        // check if the interface alrealy exists in list
        if (is_if_in_iflist(instance, iface) == MDD_TRUE) {
            LOG_WARN("Interface already in list, skip it, ifindex: %u\n", iface.ifindex);
            return MDD_OK;
        }

        // add to interface list
        add_to_iflist(instance, iface);
        // add this interface to multicast group
        if (add_to_multicast_group(instance, iface) != MDD_OK) {
            // fail to add interface to multicast group, try again later
            MDDEventIfData *data = NULL;
            MDDEvent *retry_event = NULL;

            LOG_WARN("Failed to add to multicast group, try again later, ifindex: %u\n", iface.ifindex);

            data = (MDDEventIfData *)malloc(sizeof(MDDEventIfData));
            memset(data, 0, sizeof(MDDEventIfData));
            data->iface = iface;

            retry_event = (MDDEvent *)malloc(sizeof(MDDEvent));
            memset(retry_event, 0, sizeof(MDDEvent));
            retry_event->instance = instance;
            retry_event->data = data;
            retry_event->type = MDDEvent_IfRetry;

            MDDTime_AddTimeoutCallback(DEFAULT_IF_RETRY_TIMEOUT, _mdd_timeout_cb, retry_event);
        } else {
            create_query_and_publish(instance, iface);
        }

    } else if (state == MDDIfaceState_Remove) {
        LOG_DEBUG("Address removed, family: %d\n", iface.addr.family);
        // remove from multicast group
        remove_from_multicast_group(instance, iface);
        // remove from interface list
        remove_from_iflist(instance, iface);
        // set PTR caches from the interface to expiration, then it will be report through resolve callback
        if (instance->query_service_name != NULL) {
            mdnsd_expire_cache_by_interface(instance->mdnsd, instance->query_service_name, QTYPE_PTR, iface);
        }
        // remove from query list
        mdnsd_remove_query_by_interface(instance->mdnsd, iface);
        // remove from publishing list
        mdnsd_remove_publish_by_interface(instance->mdnsd, iface);
    }
    return MDD_OK;
}

static int handle_event(MDDInstance instance)
{
    if (instance->event_head == NULL) {
        return MDD_OK;
    }

    // acquire lock
    MDDMutex_Lock(&instance->event_lock);

    while (instance->event_head) {
        MDDEvent *cur = instance->event_head;
        if (cur->type == MDDEvent_IfAdded) {
            MDDEventIfData *if_data = cur->data;
            handle_interface_changed(instance, if_data->iface, MDDIfaceState_Add);
        } else if (cur->type == MDDEvent_IfRemoved) {
            MDDEventIfData *if_data = cur->data;
            handle_interface_changed(instance, if_data->iface, MDDIfaceState_Remove);
        } else if (cur->type == MDDEvent_IfRetry) {
            MDDEventIfData *if_data = cur->data;
            if (is_if_in_iflist(instance, if_data->iface) == MDD_TRUE) {
                if (add_to_multicast_group(instance, if_data->iface) != MDD_OK) {
                    // still failed, try again later
                    MDDEventIfData *data = NULL;
                    MDDEvent *retry_event = NULL;

                    LOG_WARN("Failed to add to multicast group, try again later, ifindex: %u\n", if_data->iface.ifindex);

                    data = (MDDEventIfData *)malloc(sizeof(MDDEventIfData));
                    memset(data, 0, sizeof(MDDEventIfData));
                    data->iface = if_data->iface;

                    retry_event = (MDDEvent *)malloc(sizeof(MDDEvent));
                    memset(retry_event, 0, sizeof(MDDEvent));
                    retry_event->instance = instance;
                    retry_event->data = data;
                    retry_event->type = MDDEvent_IfRetry;

                    MDDTime_AddTimeoutCallback(DEFAULT_IF_RETRY_TIMEOUT, _mdd_timeout_cb, retry_event);
                } else {
                    create_query_and_publish(instance, if_data->iface);
                }
            }
        } else if (cur->type == MDDEvent_ResetRPublish) {
            // reset ttl and repeat publish interval
            mdnsd_reset_all_publish_ttl(instance->mdnsd, instance->publish_max_rinterval * RETRY_TO_TTL_FACTOR);
            mdnsd_repeat_publish(instance->mdnsd, instance->publish_max_rinterval);
        } else {
            LOG_WARN("Got unknow event, type: %d\n", cur->type);
        }

        // delete the event
        instance->event_head = cur->next;
        free(cur->data);
        free(cur);
    }
    instance->event_head = NULL;
    instance->event_tail = NULL;

    // release lock
    MDDMutex_Unlock(&instance->event_lock);

    return MDD_OK;
}

static int handle_pkt_in_wakeup(MDDInstance instance)
{
    int rsize = 0;
    unsigned char buf[8] = {0};
    MDDSocketAddr_t from_addr;

    // recv the data for wakeup
    from_addr.family = MDDNet_INET;
    while ((rsize = MDDSocket_Recvfrom(instance->socket_wakeup, buf, 8, &from_addr, NULL)) > 0);

    return MDD_OK;
}

static int handle_pkt_in(MDDInstance instance, MDDSocket_t socket, MDDSocketAddr_t *from_addr, struct message *m)
{
    int rsize = 0;
    unsigned char buf[MAX_PACKET_LEN] = {0};
    MDDIface_t from_iface;

    while ((rsize = MDDSocket_Recvfrom(socket, buf, MAX_PACKET_LEN, from_addr, &from_iface)) > 0) {
        if (find_local_address(instance, from_addr, &from_iface, &from_iface.addr) == MDD_OK) {
            memset(m, 0, sizeof(struct message));
            message_parse(m, buf, rsize);
            mdnsd_in(instance->mdnsd, m, from_addr, &from_iface);
        } else {
            /*
            LOG_DEBUG("Can't find any local address to the message, skip it, ifindex: %u, packet from addr: ", from_iface.ifindex);
            if (from_addr->family == MDDNet_INET) {
                LOG_DEBUG("%lx\n", from_addr->addr.ip.ipv4);
            } else {
                unsigned char *ipv6 = from_addr->addr.ip.ipv6;
                LOG_DEBUG("%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x\n",
                    ipv6[0], ipv6[1], ipv6[2], ipv6[3],
                    ipv6[4], ipv6[5], ipv6[6], ipv6[7],
                    ipv6[8], ipv6[9], ipv6[10], ipv6[11],
                    ipv6[12], ipv6[13], ipv6[14], ipv6[15]);
            }
            */
        }
    }
    if (rsize == MDD_ERROR) {
        LOG_ERROR("Can't read from socket\n");
        return MDD_ERROR;
    }
    return MDD_OK;
}

static int is_multicast_addr(MDDSocketAddr_t *addr)
{
    if (addr == NULL) {
        return MDD_FALSE;
    }

    if (addr->family == MDDNet_INET) {
        MDDNetAddr_t mcast_addr;
        if (MDDNet_Inet_pton(MDDNet_INET, MDNS_MULTICAST_IPV4_ADDR, &mcast_addr) != MDD_OK) {
            LOG_ERROR("Fail to convert addr to MDDNetAddr_t, addr: %s\n", MDNS_MULTICAST_IPV4_ADDR);
            return MDD_FALSE;
        }
        if (mcast_addr.ip.ipv4 == addr->addr.ip.ipv4) {
            return MDD_TRUE;
        }
        return MDD_FALSE;
    } else if (addr->family == MDDNet_INET6) {
        MDDNetAddr_t mcast_addr;
        if (MDDNet_Inet_pton(MDDNet_INET6, MDNS_MULTICAST_IPV6_ADDR, &mcast_addr) != MDD_OK) {
            LOG_ERROR("Fail to convert addr to MDDNetAddr_t, addr: %s\n", MDNS_MULTICAST_IPV6_ADDR);
            return MDD_ERROR;
        }
        if (!memcmp(mcast_addr.ip.ipv6, addr->addr.ip.ipv6, 16)) {
            return MDD_TRUE;
        }
        return MDD_FALSE;
    } else {
        return MDD_FALSE;
    }
}

static int is_if_in_multicast_group(MDDInstance instance, MDDIface_t iface)
{
    MDDIfacelist *cur = NULL;

    if (instance == NULL) {
        return MDD_FALSE;
    }

    for (cur = instance->mcast_if_list; cur != NULL; cur = cur->next) {
        if (cur->iface.addr.family == iface.addr.family) {
            if ((cur->iface.addr.family == MDDNet_INET &&  // compare ipv4 address for ipv4 interface
                    cur->iface.addr.ip.ipv4 == iface.addr.ip.ipv4) ||
                    (cur->iface.addr.family == MDDNet_INET6 && // compare ipv6 address for ipv6 interface
                    !memcmp(cur->iface.addr.ip.ipv6, iface.addr.ip.ipv6, 16))) {
                // the address already joined to multicast group
                return MDD_TRUE;
            }
        }
    }

    return MDD_FALSE;
}

int add_to_multicast_group(MDDInstance instance, MDDIface_t iface)
{
    MDDIfacelist *cur = NULL;
    MDDSocket_t socket = MDDSocket_INVALID;
    MDDNetAddr_t mcast_addr;

    if (instance == NULL) {
        return MDD_ERROR;
    }

    // check if the interface already join the multicast group
    if (is_if_in_multicast_group(instance, iface) == MDD_TRUE) {
        return MDD_OK;
    }

    socket = get_socket_for_iface(instance, iface, 1);
    if (socket == MDDSocket_INVALID) {
        LOG_ERROR("Fail to add to multicast group, no socket found for iface, index: %u, family: %u\n", iface.ifindex, iface.addr.family);
        return MDD_ERROR;
    }

    if (iface.addr.family == MDDNet_INET) {
        if (MDDNet_Inet_pton(MDDNet_INET, MDNS_MULTICAST_IPV4_ADDR, &mcast_addr) != MDD_OK) {
            LOG_ERROR("Fail to convert addr to MDDNetAddr_t, addr: %s\n", MDNS_MULTICAST_IPV4_ADDR);
            return MDD_ERROR;
        }
    } else if (iface.addr.family == MDDNet_INET6) {
        if (MDDNet_Inet_pton(MDDNet_INET6, MDNS_MULTICAST_IPV6_ADDR, &mcast_addr) != MDD_OK) {
            LOG_ERROR("Fail to convert addr to MDDNetAddr_t, addr: %s\n", MDNS_MULTICAST_IPV6_ADDR);
            return MDD_ERROR;
        }
    } else {
        LOG_ERROR("Fail to add to multicast group, unknown addr family: %u\n", iface.addr.family);
        return MDD_ERROR;
    }

    if (MDDSocket_Jointomulticast(socket, 1, &mcast_addr, &iface) != MDD_OK) {
        LOG_WARN("Fail to join multicast group for interface index: %u, family: %u\n", iface.ifindex, iface.addr.family);
        return MDD_ERROR;
    }

    LOG_DEBUG("Success to join multicast group for interface index: %u, family: %u\n", iface.ifindex, iface.addr.family);

    // prepend to list
    cur = malloc(sizeof(MDDIfacelist));
    memset(cur, 0, sizeof(MDDIfacelist));
    cur->iface = iface;
    if (instance->mcast_if_list != NULL) {
        instance->mcast_if_list->prev = cur;
        cur->next = instance->mcast_if_list;
    }
    instance->mcast_if_list = cur;

    return MDD_OK;
}

int remove_from_multicast_group(MDDInstance instance, MDDIface_t iface)
{
    MDDIfacelist *cur = NULL;
    MDDSocket_t socket = MDDSocket_INVALID;
    MDDNetAddr_t mcast_addr;

    if (instance == NULL) {
        return MDD_ERROR;
    }

    // check if the interface in the multicast group or not
    if (is_if_in_multicast_group(instance, iface) != MDD_TRUE) {
        return MDD_OK;
    }

    socket = get_socket_for_iface(instance, iface, 1);
    if (socket == MDDSocket_INVALID) {
        LOG_ERROR("Fail to remove from multicast group, no socket found for iface, index: %u, family: %u\n", iface.ifindex, iface.addr.family);
        return MDD_ERROR;
    }

    if (iface.addr.family == MDDNet_INET) {
        if (MDDNet_Inet_pton(MDDNet_INET, MDNS_MULTICAST_IPV4_ADDR, &mcast_addr) != MDD_OK) {
            LOG_ERROR("Fail to convert addr to MDDNetAddr_t, addr: %s\n", MDNS_MULTICAST_IPV4_ADDR);
            return MDD_ERROR;
        }
    } else if (iface.addr.family == MDDNet_INET6) {
        if (MDDNet_Inet_pton(MDDNet_INET6, MDNS_MULTICAST_IPV6_ADDR, &mcast_addr) != MDD_OK) {
            LOG_ERROR("Fail to convert addr to MDDNetAddr_t, addr: %s\n", MDNS_MULTICAST_IPV6_ADDR);
            return MDD_ERROR;
        }
    } else {
        LOG_ERROR("Fail to remove from multicast group, unknown addr family: %u\n", iface.addr.family);
        return MDD_ERROR;
    }

    if (MDDSocket_Jointomulticast(socket, 0, &mcast_addr, &iface) != MDD_OK) {
        // treat the fail for droping membership is not fatal error, just log the error
        LOG_WARN("Fail to remove multicast group for interface index: %u, family: %u\n", iface.ifindex, iface.addr.family);
    }

    // remove from list
    for (cur = instance->mcast_if_list; cur != NULL; cur = cur->next) {
        if (cur->iface.addr.family == iface.addr.family) {
            if ((cur->iface.addr.family == MDDNet_INET &&  // compare ipv4 address for ipv4 interface
                    cur->iface.addr.ip.ipv4 == iface.addr.ip.ipv4) ||
                    (cur->iface.addr.family == MDDNet_INET6 && // compare ipv6 address for ipv6 interface
                    !memcmp(cur->iface.addr.ip.ipv6, iface.addr.ip.ipv6, 16))) {
                if (cur == instance->mcast_if_list) {
                    instance->mcast_if_list = cur->next;
                    if (instance->mcast_if_list) {
                        instance->mcast_if_list->prev = NULL;
                    }
                } else {
                    if (cur->next) {
                        cur->next->prev = cur->prev;
                    }
                    if (cur->prev) {
                        cur->prev->next = cur->next;
                    }
                }
                free(cur);
                return MDD_OK;
            }
        }
    }

    return MDD_ERROR;
}

void query_on_interface(MDDInstance instance, MDDIface_t iface)
{
    if (instance == NULL) {
        return;
    }

    mdnsd_query(instance->mdnsd, instance->query_service_name, QTYPE_PTR, iface, instance->query_rinterval, _mdd_answer_cb, _mdd_resolve_cb, instance);
}

void publish_on_interface(MDDInstance instance, MDDIface_t iface)
{
    mdnsd d = instance->mdnsd;
    mdnsdr r;
    char *host_local = NULL;
    char *service_local = NULL;
    char *domain_local = NULL;
    unsigned char *packet = NULL;
    int ttl = DEFAULT_MDNS_TTL;
    int len = 0;

    len = strlen(instance->publish_host_name) + strlen(".local.") + 1;
    host_local = malloc(len);
    sprintf(host_local, "%s.local.", instance->publish_host_name);
    host_local[len - 1] = '\0';

    len = strlen("_") + strlen(instance->publish_service_name) + strlen("._tcp.local.") + 1;
    service_local = malloc(len);
    sprintf(service_local, "_%s._tcp.local.", instance->publish_service_name);
    service_local[len - 1] = '\0';

    len = strlen(instance->publish_host_name) + strlen(".") + strlen(service_local) + 1;
    domain_local = malloc(len);
    sprintf(domain_local, "%s.%s", instance->publish_host_name, service_local);
    domain_local[len - 1] = '\0';

    if (instance->publish_max_rinterval > 0) {
        ttl = (instance->publish_max_rinterval * RETRY_TO_TTL_FACTOR);
    }

    // add PTR
    r = mdnsd_shared(d, service_local, QTYPE_PTR, ttl, iface, _mdd_append_txt_cb, instance);
    mdnsd_set_host(d, r, domain_local);

    // add SRV
    r = mdnsd_unique(d, domain_local, QTYPE_SRV, ttl, iface, _mdd_publish_conflict_cb, instance);
    mdnsd_set_srv(d, r, 0, 0, instance->publish_service_port, host_local); // Priority = 0, Weight = 0

    // add ip address (A, AAAA)
    if (iface.addr.family == MDDNet_INET) {
        r = mdnsd_unique(d, host_local, QTYPE_A, ttl, iface, _mdd_publish_conflict_cb, instance);
        mdnsd_set_raw(d, r, (char *)&iface.addr.ip.ipv4, 4); // Set ipv4 address
    } else if (iface.addr.family == MDDNet_INET6) {
        r = mdnsd_unique(d, host_local, QTYPE_AAAA, ttl, iface, _mdd_publish_conflict_cb, instance);
        mdnsd_set_raw(d, r, (char *)&iface.addr.ip.ipv6, 16); // Set ipv6 address
    }

    // add TXT
    if (instance->publish_txt_xht != NULL) {
        r = mdnsd_unique(d, domain_local, QTYPE_TXT, ttl, iface, _mdd_publish_conflict_cb, instance);
        packet = sd2txt((xht)instance->publish_txt_xht, &len);
        mdnsd_set_raw(d, r, (char *)packet, len);
        free(packet);
    } else {
        // create a null txt entry
        r = mdnsd_unique(d, domain_local, QTYPE_TXT, ttl, iface, _mdd_publish_conflict_cb, instance);
        mdnsd_set_raw(d, r, NULL, 0);
    }

    free(host_local);
    free(service_local);
    free(domain_local);
}

static char rand_09_az_AZ() {
    char val = rand() % 62; // 0-9: 10, a-z: 26, A-Z: 26
    if (val < 10) {
        val += '0';
    } else if (val < 36) {
        val -= 10;
        val += 'a';
    } else {
        val -= 36;
        val += 'A';
    }
    return val;
}

char *hostname_with_random(const char *host_name)
{
    char *name_gen;

    if (host_name == NULL) {
        return NULL;
    }

    name_gen = malloc(strlen(host_name) + 7); // NAME_XXXXX, with 5 random number
    sprintf(name_gen, "%s_%c%c%c%c%c", host_name,
            rand_09_az_AZ(), rand_09_az_AZ(), rand_09_az_AZ(), rand_09_az_AZ(), rand_09_az_AZ());
    name_gen[strlen(host_name) + 7 - 1] = '\0';
    return name_gen;
}

static void clear_up(MDDInstance instance)
{
    MDDIfaceSocketList *cur_ifsocket = NULL;
    MDDIfaceSocketList *next_ifsocket = NULL;
    MDDIfacelist *cur_iface = NULL;
    MDDIfacelist *next_iface = NULL;

    if (!instance->shutdown) {
        return;
    }

    if (instance->publish_origin_host_name) {
        free(instance->publish_origin_host_name);
        instance->publish_origin_host_name = NULL;
    }
    if (instance->publish_host_name) {
        free(instance->publish_host_name);
        instance->publish_host_name = NULL;
    }
    if (instance->publish_service_name) {
        free(instance->publish_service_name);
        instance->publish_service_name = NULL;
    }
    if (instance->publish_connection_type_key) {
        free(instance->publish_connection_type_key);
        instance->publish_connection_type_key = NULL;
    }
    if (instance->query_service_name) {
        free(instance->query_service_name);
        instance->query_service_name = NULL;
    }
    if (instance->resolve_connection_type_key) {
        free(instance->resolve_connection_type_key);
        instance->resolve_connection_type_key = NULL;
    }

    // free interface list
    cur_ifsocket = instance->if_list;
    while (cur_ifsocket) {
        next_ifsocket = cur_ifsocket->next;
        // close socket if needed
        if (cur_ifsocket->ifsocket->msocket != MDDSocket_INVALID) {
            MDDSocket_Close(cur_ifsocket->ifsocket->msocket);
        }
        if (cur_ifsocket->ifsocket->usocket != MDDSocket_INVALID) {
            MDDSocket_Close(cur_ifsocket->ifsocket->usocket);
        }
        free(cur_ifsocket->ifsocket);
        free(cur_ifsocket);
        cur_ifsocket = next_ifsocket;
    }
    instance->if_list = NULL;

    // free multicast interface list
    cur_iface = instance->mcast_if_list;
    while (cur_iface) {
        next_iface = cur_iface->next;
        free(cur_iface);
        cur_iface = next_iface;
    }
    instance->mcast_if_list = NULL;

    // close wakeup socket
    MDDSocket_Close(instance->socket_wakeup);
    // release socket
    MDDSocket_Quit();

    // release mdnsd
    mdnsd_shutdown(instance->mdnsd);
    mdnsd_free(instance->mdnsd);

    // destroy event lock
    MDDMutex_Destroy(&instance->event_lock);
}

static void restart_mdnsd(MDDInstance instance)
{
    int temp_shutdown_state = 0;

    temp_shutdown_state = mdnsd_in_temp_shutdown_state(instance->mdnsd);
    mdnsd_shutdown(instance->mdnsd);
    mdnsd_free(instance->mdnsd);

    instance->mdnsd = mdnsd_new(1, 1000);
    // recover temp shutdown state
    mdnsd_temp_shutdown_service(instance->mdnsd, temp_shutdown_state);
}

int wakeup_main_thread(MDDInstance instance)
{
    MDDSocketAddr_t local_addr;

    if (instance == NULL || instance->socket_wakeup < 0) {
        return MDD_ERROR;
    }

    local_addr.family = MDDNet_INET;
    if (MDDSocket_Getsockname(instance->socket_wakeup, &local_addr) != MDD_OK) {
        LOG_WARN("can't get socket name for wakeup socket\n");
        return MDD_ERROR;
    }

    // send "1" to wakeup the main thread
    if (MDDSocket_Sendto(instance->socket_wakeup, (unsigned char *)"1", 1, &local_addr, NULL) != 1) {
        LOG_WARN("fail to send message to wake up main thread\n");
        return MDD_ERROR;
    }
    return MDD_OK;
}

MDDInstance MDD_Init()
{
    MDDInstance instance;
    mdnsd d;
    MDDSocket_t s_wakeup;
    int if_count = 0;
    MDDIface_t *if_list = NULL;
    int i = 0;

    if (MDDSocket_Init() != MDD_OK) {
        LOG_ERROR("can't init socket\n");
        return NULL;
    }

    if ((s_wakeup = sock_wakeup()) == MDDSocket_INVALID) {
        LOG_ERROR("can't create local socket for wakeup\n");
        return NULL;
    }

    instance = (MDDInstance)malloc(sizeof(struct MDDInstance_struct));
    memset(instance, 0, sizeof(struct MDDInstance_struct));

    instance->socket_wakeup = s_wakeup;

    if (MDDIface_Monitoriface(_mdd_iface_changed_cb, instance) < 0) {
        LOG_ERROR("can't monitor network interface\n");
        MDDSocket_Close(s_wakeup);
        free(instance);
        return NULL;
    }

    d = mdnsd_new(1, 1000);
    instance->mdnsd = d;
    instance->shutdown = 0;

    // add all interface to multicast group
    if (MDDIface_Listall(&if_list, &if_count) != MDD_OK) {
        LOG_ERROR("can't get interface list\n");
    } else {
        for (i = 0; i < if_count; i++) {
            add_to_iflist(instance, if_list[i]);
            add_to_multicast_group(instance, if_list[i]);
        }
        free(if_list);
    }

    // init event lock
    MDDMutex_Init(&instance->event_lock);

    return instance;
}

void MDD_Shutdown(MDDInstance instance)
{
    if (instance == NULL) {
        return;
    }

    instance->shutdown = 1;
    wakeup_main_thread(instance);
}

int MDD_Resolve(MDDInstance instance, const char *service_name, unsigned int repeat_interval, void (*resolve_callback)(MDDInstance inc, MDDResolve *resolve, void *arg), void *arg)
{
    MDDIfaceSocketList *cur = NULL;

    if (instance == NULL || resolve_callback == NULL) {
        return MDD_ERROR;
    }

    mdnsd_set_conn_type_resolver(instance->mdnsd, _mdd_connection_type_resolver, instance);
    if (instance->query_service_name) {
        free(instance->query_service_name);
    }
    instance->query_service_name = MDDUtils_Strdup(service_name);
    instance->query_rinterval = repeat_interval;
    instance->resolve_cb = resolve_callback;
    instance->resolve_cb_arg = arg;

    for (cur = instance->if_list; cur != NULL; cur = cur->next) {
        query_on_interface(instance, cur->ifsocket->iface);
    }

    return MDD_OK;
}

int MDD_ResolveImmediately(MDDInstance instance)
{
    MDDIfaceSocketList *cur = NULL;

    if (instance == NULL || instance->query_service_name == NULL) {
        return MDD_ERROR;
    }

    for (cur = instance->if_list; cur != NULL; cur = cur->next) {
        mdnsd_restart_query_interval(instance->mdnsd, instance->query_service_name, QTYPE_PTR, cur->ifsocket->iface);
    }
    wakeup_main_thread(instance);

    return MDD_OK;
}

int MDD_ResolveConnectionType(MDDInstance instance, const char *key)
{
    if (instance->resolve_connection_type_key) {
        free(instance->resolve_connection_type_key);
        instance->resolve_connection_type_key = NULL;
    }
    if (key != NULL) {
        instance->resolve_connection_type_key = MDDUtils_Strdup(key);
    }
    return MDD_OK;
}

int MDD_Publish(MDDInstance instance, const char *host_name, const char *service_name, unsigned short service_port, MDDXht txt_xht, int max_repeat_interval, int (*append_txt_callback)(MDDInstance inc, MDDXht txt_xht, void *arg), void *arg)
{
    MDDIfaceSocketList *cur = NULL;

    // ASSERT(!service_port<0)  // Warning, comparison is always false
    if (instance == NULL || host_name == NULL || service_name == NULL) {
        return MDD_ERROR;
    }

    // origin host name, it's saved only on the first time
    if (instance->publish_origin_host_name == NULL) {
        instance->publish_origin_host_name = MDDUtils_Strdup(host_name);
    }

    LOG_DEBUG("Announcing _%s._tcp.local site named '%s' to all ip, port %u, max repeat interval: %d\n", service_name, host_name, service_port, max_repeat_interval);

    if (host_name != instance->publish_host_name) {
        if (instance->publish_host_name)
            free(instance->publish_host_name);
        instance->publish_host_name = MDDUtils_Strdup(host_name);
    }
    if (service_name != instance->publish_service_name) {
        if (instance->publish_service_name)
            free(instance->publish_service_name);
        instance->publish_service_name = MDDUtils_Strdup(service_name);
    }
    instance->publish_service_port = service_port;
    instance->publish_txt_xht = txt_xht;
    instance->append_txt_cb = append_txt_callback;
    instance->append_txt_cb_arg = arg;

    // set for publishing repeatly
    instance->publish_max_rinterval = max_repeat_interval;
    mdnsd_repeat_publish(instance->mdnsd, max_repeat_interval);
    for (cur = instance->if_list; cur != NULL; cur = cur->next) {
        publish_on_interface(instance, cur->ifsocket->iface);
    }

    return MDD_OK;
}

int MDD_PublishImmediately(MDDInstance instance)
{
    mdnsd_restart_repeat_publish(instance->mdnsd);
    wakeup_main_thread(instance);
    return MDD_OK;
}

int MDD_SetPublishRepeatInterval(MDDInstance instance, unsigned int max_repeat_interval)
{
    MDDEvent *mdd_event;

    mdd_event = (MDDEvent *)malloc(sizeof(MDDEvent));
    memset(mdd_event, 0, sizeof(MDDEvent));
    mdd_event->instance = instance;
    mdd_event->data = NULL;
    mdd_event->type = MDDEvent_ResetRPublish;

    instance->publish_max_rinterval = max_repeat_interval;
    add_to_event_queue(instance, mdd_event);
    wakeup_main_thread(instance);
    return MDD_OK;
}

int MDD_TempShutdownService(MDDInstance instance, int shutdown)
{
    mdnsd_temp_shutdown_service(instance->mdnsd, shutdown);
    return MDD_PublishImmediately(instance);
}

int MDD_PublishConnectionType(MDDInstance instance, const char *key)
{
    if (instance->publish_connection_type_key) {
        free(instance->publish_connection_type_key);
        instance->publish_connection_type_key = NULL;
    }
    if (key != NULL) {
        instance->publish_connection_type_key = MDDUtils_Strdup(key);
    }
    return MDD_OK;
}

int MDD_Start(MDDInstance instance)
{
    struct message m;
    struct timeval *tv;
    MDDSocketAddr_t to_addr, from_addr;
    MDDIface_t to_iface;
    MDDFDSet_t fds;
    mdnsd mdnsd;
    MDDSocket_t socket_max = 0;
    MDDSocket_t socket_wakeup;

    if (instance == NULL) {
        return MDD_ERROR;
    }

    mdnsd = instance->mdnsd;
    socket_wakeup = instance->socket_wakeup;

    srand((unsigned)time(0));

    while (1) {
        MDDIfaceSocketList *cur = NULL;

        tv = mdnsd_sleep(mdnsd);
        MDDFDUtil_FDZero(&fds);
        MDDFDUtil_FDSet(socket_wakeup, &fds);
        socket_max = socket_wakeup;
        for (cur = instance->if_list; cur != NULL; cur = cur->next) {
            if (cur->ifsocket->msocket != MDDSocket_INVALID) {
                MDDFDUtil_FDSet(cur->ifsocket->msocket, &fds);
                if (socket_max < cur->ifsocket->msocket) {
                    socket_max = cur->ifsocket->msocket;
                }
            }
            if (cur->ifsocket->usocket != MDDSocket_INVALID) {
                MDDFDUtil_FDSet(cur->ifsocket->usocket, &fds);
                if (socket_max < cur->ifsocket->usocket) {
                    socket_max = cur->ifsocket->usocket;
                }
            }
        }

        MDDFDUtil_FDSelect(socket_max + 1, &fds, 0, 0, tv);

        // handle event queue first
        handle_event(instance);

        // handle incoming packet
        if (MDDFDUtil_FDIsset(socket_wakeup, &fds)) {
            handle_pkt_in_wakeup(instance);
        }

        for (cur = instance->if_list; cur != NULL; cur = cur->next) {
            // handle multicast socket
            if (cur->ifsocket->msocket != MDDSocket_INVALID) {
                if (MDDFDUtil_FDIsset(cur->ifsocket->msocket, &fds)) {
                    from_addr.family = cur->ifsocket->iface.addr.family;
                    if (handle_pkt_in(instance, cur->ifsocket->msocket, &from_addr, &m) != MDD_OK) {
                        LOG_ERROR("Fail to receive data from multicast socket interface, index: %u, family: %u\n", cur->ifsocket->iface.ifindex, cur->ifsocket->iface.addr.family);
                    }
                }
            }
            // handle unicast socket
            if (cur->ifsocket->usocket != MDDSocket_INVALID) {
                if (MDDFDUtil_FDIsset(cur->ifsocket->usocket, &fds)) {
                    from_addr.family = cur->ifsocket->iface.addr.family;
                    if (handle_pkt_in(instance, cur->ifsocket->usocket, &from_addr, &m) != MDD_OK) {
                        LOG_ERROR("Fail to receive data from unicast socket interface, index: %u, family: %u\n", cur->ifsocket->iface.ifindex, cur->ifsocket->iface.addr.family);
                    }
                }
            }
        }

        // restart publish service if needed
        if (instance->restart) {
            struct timeval sleep_tv;

            restart_mdnsd(instance);
            mdnsd = instance->mdnsd;

            // FIXME:
            // There is a bug that causes the domain name conflicts forever.
            // But we don't know the root cause.
            // Just put a sleep here to see if the issue could be prevented.
            LOG_WARN("Sleep 10 seconds before restart the publish.\n");
            sleep_tv.tv_sec = 10;
            sleep_tv.tv_usec = 0;
            MDDFDUtil_FDSelect(1, 0, 0, 0, &sleep_tv);

            MDD_Publish(instance, instance->publish_host_name, instance->publish_service_name, instance->publish_service_port, instance->publish_txt_xht, instance->publish_max_rinterval, instance->append_txt_cb, instance->append_txt_cb_arg);

            instance->restart = 0;
        }

        // handle outgoing packet
        memset(&m, 0, sizeof(struct message));
        while (mdnsd_out(mdnsd, &m, &to_addr, &to_iface)) {
            MDDSocket_t to_msocket = get_socket_for_iface(instance, to_iface, 1);
            int is_multicast = is_multicast_addr(&to_addr);
            // send by unicast socket for query
            if (m.header.qr == 0 && m.qdcount > 0 && m.nscount == 0) {
                MDDSocket_t to_usocket = get_socket_for_iface(instance, to_iface, 0);
                if (to_usocket == MDDSocket_INVALID) {
                    LOG_ERROR("can't find unicast socket for interface, ifindex: %u, family: %u\n", to_iface.ifindex, to_iface.addr.family);
                } else if (MDDSocket_Sendto(to_usocket, message_packet(&m), message_packet_len(&m), &to_addr, &to_iface) != message_packet_len(&m)) {
                    LOG_ERROR("can't write to unicast socket, ifindex: %u, family: %u\n", to_iface.ifindex, to_iface.addr.family);
                }
                // do not send unicast query through multicast socket
                if (!is_multicast) {
                    continue;
                }
            }

            // send by multicast socket
            if (to_msocket == MDDSocket_INVALID) {
                LOG_ERROR("can't find multicast socket for interface, ifindex: %u, family: %u\n", to_iface.ifindex, to_iface.addr.family);
            } else if (MDDSocket_Sendto(to_msocket, message_packet(&m), message_packet_len(&m), &to_addr, &to_iface) != message_packet_len(&m)) {
                LOG_ERROR("can't write to multicast socket, ifindex: %u, family: %u\n", to_iface.ifindex, to_iface.addr.family);
            }

        }

        if (instance->shutdown) {
            clear_up(instance);
            break;
        }
    }
    free(instance);
    return MDD_OK;
}

MDDXht MDD_XhtCreate()
{
    return (MDDXht)xht_new(23);
}

void MDD_XhtSet(MDDXht txt_xht, const char *key, const char *value)
{
    if (txt_xht == NULL) {
        return;
    }

    xht_store((xht)txt_xht, key, strlen(key), (void *)value, strlen(value));
}

char *MDD_XhtGet(MDDXht txt_xht, const char *key)
{
    if (txt_xht == NULL) {
        return NULL;
    }

    return (char *)xht_get((xht)txt_xht, key);
}

void MDD_XhtDelete(MDDXht txt_xht, const char *key)
{
    if (txt_xht == NULL) {
        return;
    }

    xht_set((xht)txt_xht, key, NULL);
}

unsigned char *MDD_XhtToStr(MDDXht txt_xht, int *len)
{
    if (txt_xht == NULL) {
        return NULL;
    }

    return sd2txt((xht)txt_xht, len);
}

void MDD_XhtFree(MDDXht txt_xht)
{
    if (txt_xht == NULL) {
        return;
    }

    xht_free((xht)txt_xht);
}
