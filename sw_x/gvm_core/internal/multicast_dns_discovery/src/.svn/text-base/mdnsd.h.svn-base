#ifndef mdnsd_h
#define mdnsd_h
#include "1035.h"
#include "xht.h"
#include "mdd_time.h"
#include "mdd_socket.h"

#define MDNS_MULTICAST_PORT 5353
#define MDNS_MULTICAST_IPV4_ADDR "224.0.0.251"
#define MDNS_MULTICAST_IPV6_ADDR "ff02::fb"

typedef struct mdnsd_struct *mdnsd; // main daemon data
typedef struct mdnsdr_struct *mdnsdr; // record entry
// answer data
typedef struct mdnsda_struct
{
    char *name;
    unsigned short int type;
    unsigned long int ttl;
    unsigned short int rdlen;
    unsigned char *rdata;
    unsigned long int ip; // A
    char *rdname; // NS/CNAME/PTR/SRV
    xht txt_xht; // TXT
    unsigned int ipv6_count;
    unsigned char **ipv6; // AAAA
    struct { unsigned short int priority, weight, port; } srv; // SRV
    int remote_conn_type;
} *mdnsda;

///////////
// Global functions
//
// create a new mdns daemon for the given class of names (usually 1) and maximum frame size
mdnsd mdnsd_new(int class_, int frame);
//
// gracefully shutdown the daemon, use mdnsd_out() to get the last packets
void mdnsd_shutdown(mdnsd d);
//
// flush all cached records (network/interface changed)
void mdnsd_flush(mdnsd d);
//
// free given mdnsd (should have used mdnsd_shutdown() first!)
void mdnsd_free(mdnsd d);
//
///////////

///////////
// I/O functions
//
// incoming message from host (to be cached/processed)
void mdnsd_in(mdnsd d, struct message *m, MDDSocketAddr_t *from_addr, MDDIface_t *from_iface);
//
// outgoing messge to be delivered to host, returns >0 if one was returned and m/ip/port set
int mdnsd_out(mdnsd d, struct message *m, MDDSocketAddr_t *to_addr, MDDIface_t *to_iface);
//
// returns the max wait-time until mdnsd_out() needs to be called again 
struct timeval *mdnsd_sleep(mdnsd d);
//
////////////

///////////
// Q/A functions
// 
// register a new query
// answer(record, arg) is called whenever one is found/changes/expires (immediate or anytime after, mdnsda valid until ->ttl==0)
// either answer returns -1, or another mdnsd_query with a NULL answer will remove/unregister this query
// if repeat_interval larger than 0, the query will be sent periodically by the interval
void mdnsd_query(mdnsd d, const char *host, int type, MDDIface_t iface, unsigned int repeat_interval, int (*answer)(mdnsda a, void *arg), int (*resolve)(mdnsda a, MDDIface_t iface, void *arg), void *arg);
//
// set the resolver to resolve the connection type on the publishing info
// the connection type of remote will be appended on resolve callback
void mdnsd_set_conn_type_resolver(mdnsd d, int (*connection_type_resolver)(xht txt_xht, void *arg), void *arg);
//
// restart the repeat query interval, and the first query will be sent immediately
void mdnsd_restart_query_interval(mdnsd d, const char *host, int type, MDDIface_t iface);
//
// expire the caches which received from the interface
void mdnsd_expire_cache_by_interface(mdnsd d, const char *host, int type, MDDIface_t iface);
//
// remove the query by interface
void mdnsd_remove_query_by_interface(mdnsd d, MDDIface_t removed_iface);
//
// returns the first (if last == NULL) or next answer after last from the cache
// mdnsda only valid until an I/O function is called
mdnsda mdnsd_list(mdnsd d, char *host, int type, mdnsda last, MDDIface_t iface);
//
///////////

///////////
// Publishing functions
//
// set a interval to publish periodically
// the interval would exponentially backoff to maximum interval
// and finally use maximum interval to publish repeatly
// set 0 to stop publish repeatly
void mdnsd_repeat_publish(mdnsd d, unsigned int max_interval);
//
// reset the ttl of all publish to the specific value
void mdnsd_reset_all_publish_ttl(mdnsd d, long int ttl);
//
// restart the repeat publish timer, and the first publishing will be sent immediately
void mdnsd_restart_repeat_publish(mdnsd d);
//
// remove the publishing by interface
void mdnsd_remove_publish_by_interface(mdnsd d, MDDIface_t removed_iface);
//
// temporary shuwdown service
// set shotdown to 1 to send out PTR with ttl = 0
void mdnsd_temp_shutdown_service(mdnsd d, int shutdown);
//
// return 1 is in temp shutdown state otherwise 0
int mdnsd_in_temp_shutdown_state(mdnsd d);
//
// create a new unique record (try mdnsda_list first to make sure it's not used)
// conflict(arg) called at any point when one is detected and unable to recover
// after the first data is set_*(), any future changes effectively expire the old one and attempt to create a new unique record
mdnsdr mdnsd_unique(mdnsd d, char *host, int type, long int ttl, MDDIface_t iface, void (*conflict)(char *host, int type, void *arg), void *arg);
// 
// create a new shared record
// append_txt only for PTR, it will be called before reply any PTR message
// if append_txt return 1, then the new value will be applied
mdnsdr mdnsd_shared(mdnsd d, char *host, int type, long int ttl, MDDIface_t iface, int (*append_txt)(xht txt_xht, MDDIface_t *iface, void *arg), void *arg);
//
// de-list the given record
void mdnsd_done(mdnsd d, mdnsdr r);
//
// these all set/update the data for the given record, nothing is published until they are called
void mdnsd_set_raw(mdnsd d, mdnsdr r, char *data, int len);
void mdnsd_set_host(mdnsd d, mdnsdr r, char *name);
void mdnsd_set_ip(mdnsd d, mdnsdr r, unsigned long int ip);
void mdnsd_set_srv(mdnsd d, mdnsdr r, int priority, int weight, int port, char *name);
//
///////////


#endif
