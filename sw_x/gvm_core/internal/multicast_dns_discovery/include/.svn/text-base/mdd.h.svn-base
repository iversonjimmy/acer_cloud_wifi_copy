#ifndef LIB_MDD_H
#define LIB_MDD_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MDDInstance_struct* MDDInstance;
typedef struct MDDXht_wrap* MDDXht;

typedef enum
{
    MDDConnType_UNKNOWN,
    MDDConnType_USB3,
    MDDConnType_ETH,
    MDDConnType_WIFI,
} MDDConnType;

typedef struct MDDResolve_struct
{
    char *domain;   // domain name
    char *host; // host name
    char *ipv4; // A
    unsigned int ipv6_count;
    char **ipv6; // AAAA, array for multiple address strings
    struct { unsigned short int priority, weight, port; } srv; // SRV
    MDDXht txt_xht; // TXT
    unsigned int ttl; // TTL, 0 means the service expired
    MDDConnType remote_conn_type; // connection type for remote
    MDDConnType self_conn_type; // connection type for self
    unsigned int ifindex;
} MDDResolve;

/// Return NULL if fail to init
MDDInstance MDD_Init();

/// Shutdown the deamon, after called, the MDD_Start() will return
void MDD_Shutdown(MDDInstance instance);

/// Resolve specific service, ex: _http._tcp.local.
/// if repeat_interval larger than 0, then the query will be sent periodically by the interval.
int MDD_Resolve(MDDInstance instance,
                const char *service_name,
                unsigned int repeat_interval,
                void (*resolve_callback)(MDDInstance inc, MDDResolve *resolve, void *arg),
                void *arg);

/// Send out resolve request immediately
/// Need to set resolve request by MDD_Resolve() before using this function
int MDD_ResolveImmediately(MDDInstance instance);

/// Resolve connection type of server from specific key on TXT
/// If not set, unknown type will be reported by resolve callback
/// Set NULL to disable it
int MDD_ResolveConnectionType(MDDInstance instance, const char *key);

/// Publish specific service with host name and specific port
/// the max_repeat_interval is used to send publishing periodically
/// if max_repeat_interval set to 0, the repeat publishing will be disabled
/// When reply the qurey, append_txt_callback will be called
/// if append_txt_callback return 1, then new values will be applied
int MDD_Publish(MDDInstance instance,
                const char *host_name,
                const char *service_name,
                unsigned short service_port,
                MDDXht txt_xht,
                int max_repeat_interval,
                int (*append_txt_callback)(MDDInstance inc, MDDXht txt_xht, void *arg),
                void *arg);

/// Send out publishing immediately
/// Need to set publishing by MDD_Publish() and enable repeat publishing before using this function
int MDD_PublishImmediately(MDDInstance instance);

/// Set interval to send publishing periodically
/// This call would restart publishing with the specific max_repeat interval
/// if max_repeat_interval set to 0, the repeat publishing will be disabled
int MDD_SetPublishRepeatInterval(MDDInstance instance, unsigned int max_repeat_interval);

/// Temporary shutdown service, means send out publishing with ttl = 0
/// Also trigger to send out publishing immediately
int MDD_TempShutdownService(MDDInstance instance, int shutdown);

/// Publish the connection type by the specific key on TXT
/// Set NULL to disable it, default is disable
int MDD_PublishConnectionType(MDDInstance instance, const char *key);

/// MDD_Start will block the thread and trigger callback if something find
int MDD_Start(MDDInstance instance);


/// TXT utils

/// Create a new MDDXht instance
MDDXht MDD_XhtCreate();
/// Set value with specific key
void MDD_XhtSet(MDDXht txt_xht, const char *key, const char *value);
/// Get to value for specific key
/// if find, the value will be returned, and the momery of the value is owned by MDD
/// if not find, NULL will be returned
char *MDD_XhtGet(MDDXht txt_xht, const char *key);
/// Delete value for specific key if find
void MDD_XhtDelete(MDDXht txt_xht, const char *key);
/// dump all key/value to single string
/// the return value should be free by user
unsigned char *MDD_XhtToStr(MDDXht txt_xht, int *len);
/// Free the MDDXht
void MDD_XhtFree(MDDXht txt_xht);

#ifdef __cplusplus
}
#endif

#endif // LIB_MDD_H
