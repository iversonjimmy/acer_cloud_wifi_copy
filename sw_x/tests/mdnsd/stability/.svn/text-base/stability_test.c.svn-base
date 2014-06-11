#include "mdd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "mdnsd.h"
#include "1035.h"
#include "sdtxt.h"

#include "mdd_fdutil.h"
#include "mdd_iface.h"
#include "mdd_net.h"
#include "mdd_socket.h"

#ifdef WIN32
#define snprintf _snprintf
#endif

#define KEY_NAME "Name"
#define KEY_SN   "SN"

#define VALUE_STABILITY_TEST "StabilityTest"
#define SERVICE_NAME "stest"

MDDSocket_t msock_ipv4();

typedef struct {
    mdnsd mdnsd;
    char *publish_host_name;
    char *publish_service_name;
    int publish_max_rinterval;
    unsigned int publish_service_port;
    MDDSocket_t socket_ipv4;
    xht publish_txt_xht;
    int max_sn;
    int sn;
} TestInstanceServer;

typedef struct {
    MDDInstance mdd_inc;
    int max_sn;
    int last_sn;
    int packet_received_count;
    int packet_loss_count;
    FILE *log_fp;
} TestInstanceClient;

static void getCurrentTime(struct tm *tm, int *msec)
{
    if (tm != NULL) {
#ifdef WIN32
        SYSTEMTIME wtm;
        GetLocalTime(&wtm);
        tm->tm_year = wtm.wYear - 1900;
        tm->tm_mon = wtm.wMonth - 1;
        tm->tm_mday = wtm.wDay;
        tm->tm_hour = wtm.wHour;
        tm->tm_min = wtm.wMinute;
        tm->tm_sec = wtm.wSecond;
        tm->tm_isdst = -1;
        if (msec != NULL) {
            *msec = wtm.wMilliseconds;
        }
#else
        struct timeval tv;
        gettimeofday(&tv, NULL);
        localtime_r(&tv.tv_sec, tm);
        if (msec != NULL) {
            *msec = (tv.tv_usec / 1000);
        }
#endif
    }
}

static void writeLog(TestInstanceClient *inc, const char *log)
{
    if (inc->log_fp) {
        struct tm tm;
        getCurrentTime(&tm, NULL);
        fprintf(inc->log_fp, "%02d:%02d:%02d %s", tm.tm_hour, tm.tm_min, tm.tm_sec, log);
    }
}

static int _append_txt_cb(xht txt_xht, MDDIface_t *iface, void *arg)
{
    char sn_char[10];
    TestInstanceServer *inc;

    if (arg == NULL) {
        printf("Failed to append txt, instance is NULL\n");
        return 0;
    }

    inc = arg;
    snprintf(sn_char, 10, "%d", inc->sn);
    xht_store(txt_xht, KEY_SN, strlen(KEY_SN), (void *)sn_char, strlen(sn_char));
    inc->sn++;
    return 1;
}

static void _publish_conflict_cb(char *name, int type, void *arg)
{
    printf("Conflict occur, name: %s, type: %d\n", name, type);
    exit(-1);
}

static void test_publish_on_interface(TestInstanceServer *instance, MDDIface_t iface)
{
    mdnsd d = instance->mdnsd;
    mdnsdr r;
    char *host_local = NULL;
    char *service_local = NULL;
    char *domain_local = NULL;
    unsigned char *packet = NULL;
    int ttl = 60;
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
        ttl = (instance->publish_max_rinterval * 2);
    }

    // add PTR
    r = mdnsd_shared(d, service_local, QTYPE_PTR, ttl, iface, _append_txt_cb, instance);
    mdnsd_set_host(d, r, domain_local);

    // add SRV
    r = mdnsd_unique(d, domain_local, QTYPE_SRV, ttl, iface, _publish_conflict_cb, instance);
    mdnsd_set_srv(d, r, 0, 0, instance->publish_service_port, host_local); // Priority = 0, Weight = 0

    // add ip address (A, AAAA)
    if (iface.addr.family == MDDNet_INET) {
        r = mdnsd_unique(d, host_local, QTYPE_A, ttl, iface, _publish_conflict_cb, instance);
        mdnsd_set_raw(d, r, (char *)&iface.addr.ip.ipv4, 4); // Set ipv4 address
    } else if (iface.addr.family == MDDNet_INET6) {
        r = mdnsd_unique(d, host_local, QTYPE_AAAA, ttl, iface, _publish_conflict_cb, instance);
        mdnsd_set_raw(d, r, (char *)&iface.addr.ip.ipv6, 16); // Set ipv6 address
    }

    // add TXT
    if (instance->publish_txt_xht != NULL) {
        r = mdnsd_unique(d, domain_local, QTYPE_TXT, ttl, iface, _publish_conflict_cb, instance);
        packet = sd2txt(instance->publish_txt_xht, &len);
        mdnsd_set_raw(d, r, (char *)packet, len);
        free(packet);
    } else {
        // create a null txt entry
        r = mdnsd_unique(d, domain_local, QTYPE_TXT, ttl, iface, _publish_conflict_cb, instance);
        mdnsd_set_raw(d, r, NULL, 0);
    }

    mdnsd_repeat_publish(instance->mdnsd, instance->publish_max_rinterval);

    free(host_local);
    free(service_local);
    free(domain_local);
}

static void resolve_cb(MDDInstance mdd_inc, MDDResolve *resolve, void *arg)
{
    int i = 0;
    char *sn_char = NULL;
    int sn;
    char buf[128];
    TestInstanceClient *inc = (TestInstanceClient *)arg;

    if (inc == NULL) {
        printf("Failed to do resolve_cb, instance is NULL\n");
        return;
    }

    printf("-------------\n");
    printf("Host: %s, domain: %s, TTL: %u\n", resolve->host, resolve->domain, resolve->ttl);
    if (resolve->ipv4 != NULL) {
        printf("IPv4: %s\n", resolve->ipv4);
    }
    if (resolve->ipv6_count > 0) {
        for (i = 0; i < resolve->ipv6_count; i++) {
            printf("IPv6: %s\n", resolve->ipv6[i]);
        }
    }

    sn_char = MDD_XhtGet(resolve->txt_xht, KEY_SN);
    if (sn_char != NULL && resolve->ttl != 0) {
        sn = atoi(sn_char);
        snprintf(buf, 128, "Received SN: %d, last SN: %d\n", sn, inc->last_sn);
        printf("%s", buf);
        writeLog(inc, buf);
        if (sn - inc->last_sn > 1) {
            int loss;
            loss = sn - inc->last_sn - 1;
            snprintf(buf, 128, "*** detecting packet loss, %d packet lost ***\n", loss);
            printf("%s", buf);
            writeLog(inc, buf);
            inc->packet_loss_count += loss;
        }
        inc->last_sn = sn;
    }
    inc->packet_received_count++;
    printf("-------------\n");

    if (inc->last_sn >= inc->max_sn) {
        MDD_Shutdown(inc->mdd_inc);
    }
}

static void timeout_cb(void *arg)
{
    TestInstanceClient *inc = (TestInstanceClient *)arg;
    if (inc == NULL) {
        printf("Failed to do timeout_cb, instance is NULL\n");
    } else {
        MDD_Shutdown(inc->mdd_inc);
    }
}

static void server_loop(TestInstanceServer *inc)
{
    struct message m;
    struct timeval *tv;
    MDDSocketAddr_t to_addr, from_addr;
    MDDIface_t to_iface, from_iface;
    MDDFDSet_t fds;
    mdnsd mdnsd;
    MDDSocket_t socket_ipv4;
    int rsize = 0;
    unsigned char buf[MAX_PACKET_LEN] = {0};

    mdnsd = inc->mdnsd;
    socket_ipv4 = inc->socket_ipv4;

    while (1) {
        tv = mdnsd_sleep(mdnsd);
        MDDFDUtil_FDZero(&fds);
        MDDFDUtil_FDSet(socket_ipv4, &fds);

        MDDFDUtil_FDSelect(socket_ipv4 + 1, &fds, 0, 0, tv);

        // handle incoming packet
        if (MDDFDUtil_FDIsset(socket_ipv4, &fds)) {
            from_addr.family = MDDNet_INET;
            while ((rsize = MDDSocket_Recvfrom(socket_ipv4, buf, MAX_PACKET_LEN, &from_addr, &from_iface)) > 0) {
                memset(&m, 0, sizeof(struct message));
                message_parse(&m, buf, rsize);
                mdnsd_in(inc->mdnsd, &m, &from_addr, &from_iface);
            }
        }

        // handle outgoing packet
        memset(&m, 0, sizeof(struct message));
        while (mdnsd_out(mdnsd, &m, &to_addr, &to_iface)) {
            if (to_addr.family == MDDNet_INET) {
                if (MDDSocket_Sendto(socket_ipv4, message_packet(&m), message_packet_len(&m), &to_addr, &to_iface) != message_packet_len(&m)) {
                    printf("can't write to ipv4 socket, ifindex: %u\n", to_iface.ifindex);
                }
            } else {
                printf("error when send packet, unsupport address family: %d\n", to_addr.family);
            }
        }
        if (inc->sn > inc->max_sn) {
            break;
        }
    }
}

static int getIfindex(unsigned long int ipv4, MDDIfaceIfindex_t *index)
{
    int i = 0;
    int if_count = 0;
    MDDIface_t *if_list = NULL;

    if (index == NULL) {
        return -1;
    }

    if (MDDIface_Listall(&if_list, &if_count) < 0) {
         printf("error when list all interface\n");
         return -1;
    } else {
        for (i = 0; i < if_count; i++) {
            if (if_list[i].addr.family == MDDNet_INET) {
                if (if_list[i].addr.ip.ipv4 == ipv4) {
                    *index = if_list[i].ifindex;
                    break;
                }
            }
        }
        free(if_list);
    }
    return 0;
}

static int add_to_multicast(TestInstanceServer *instance, MDDIface_t iface)
{
    MDDNetAddr_t mcast_addr;
    if (MDDNet_Inet_pton(MDDNet_INET, MDNS_MULTICAST_IPV4_ADDR, &mcast_addr) < 0) {
        printf("Fail to convert addr to MDDNetAddr_t, addr: %s\n", MDNS_MULTICAST_IPV4_ADDR);
        return -1;
    }

    // drop membership first
    MDDSocket_Jointomulticast(instance->socket_ipv4, 0, &mcast_addr, &iface);
    if (MDDSocket_Jointomulticast(instance->socket_ipv4, 1, &mcast_addr, &iface) < 0) {
        printf("Fail to join multicast group for ipv4 interface index: %lu\n", (unsigned long)iface.ifindex);
        return -1;
    }
    return 0;
}

static int server(int count, const char *ip)
{
    TestInstanceServer *inc;
    MDDIface_t iface;

    if (ip == NULL) {
        printf("IP is not specific\n");
        return -1;
    }

    inc = malloc(sizeof(TestInstanceServer));
    memset(inc, 0, sizeof(TestInstanceServer));

    inc->mdnsd = mdnsd_new(1, 1000);
    inc->socket_ipv4 = msock_ipv4();
    inc->publish_host_name = strdup("qazxswedc");
    inc->publish_service_name = strdup("stest");
    inc->publish_max_rinterval = 2;
    inc->publish_service_port = 9999;
    inc->publish_txt_xht = xht_new(23);
    inc->max_sn = count;
    inc->sn = 1;

    xht_store(inc->publish_txt_xht, KEY_NAME, strlen(KEY_NAME), (void *)VALUE_STABILITY_TEST, strlen(VALUE_STABILITY_TEST));

    if (MDDNet_Inet_pton(MDDNet_INET, ip, &iface.addr) < 0) {
        printf("Failed to convert ip, please make sure ip address is correct\n");
        return -1;
    }
    if (getIfindex(iface.addr.ip.ipv4, &iface.ifindex) < 0) {
        printf("Failed to find ifindex\n");
        return -1;
    }
    if (add_to_multicast(inc, iface) < 0) {
        printf("Failed to add to multicast group\n");
        return -1;
    }
    test_publish_on_interface(inc, iface);
    server_loop(inc);
    return 0;
}

static int client(int count, const char *path)
{
    TestInstanceClient *inc;
    int timeout = (count * 2000 + 60000);
    char *result;

    inc = malloc(sizeof(TestInstanceClient));
    memset(inc, 0, sizeof(TestInstanceClient));
    inc->mdd_inc = MDD_Init();
    inc->max_sn = count;

    if (path != NULL) {
        printf("Write log to %s\n", path);
        inc->log_fp = fopen(path, "w");
    } else {
        printf("Write log to result.log\n");
        inc->log_fp = fopen("result.log", "w");
    }

    printf("Set timeout to %d ms\n", timeout);
    MDDTime_AddTimeoutCallback(timeout, timeout_cb, inc);

    if (inc->mdd_inc != NULL) {
        MDD_Resolve(inc->mdd_inc, "_stest._tcp.local.", 0, resolve_cb, inc);
        MDD_Start(inc->mdd_inc);
    } else {
        printf("Fail to init mdd\n");
    }

    result = (char *)malloc(512);
    memset(result, 0, 512);
    snprintf(result, 512, "=== Result: Expect LastSN: %d, LastSN: %d, Received: %d, Loss: %d, LossRate: %.1f%% ===\n",
            inc->max_sn, inc->last_sn, inc->packet_received_count, inc->packet_loss_count,
            (((inc->max_sn - inc->packet_received_count) / (double)inc->max_sn) * 100));
    printf("%s", result);
    writeLog(inc, result);
    if (inc->log_fp) {
        fclose(inc->log_fp);
    }
    free(result);
    return 0;
}

static void print_help()
{
    printf("Usage:\n");
    printf("./stability_test -s COUNT IP\n");
    printf("./stability_test -c COUNT [FILE]\n");
}

int main(int argc, char* argv[])
{
    int count = 0;
    if (argc < 3) {
        print_help();
        return -1;
    }

    count = atoi(argv[2]);
    if (count <= 0) {
        printf("COUNT must larger than 0\n");
        print_help();
        return -1;
    }

    if (!strcmp(argv[1], "-s") && argc == 4) {
        return server(count, argv[3]);
    } else if (!strcmp(argv[1], "-c") && (argc == 3 || argc == 4)) {
        return client(count, argv[3]);
    } else {
        print_help();
        return -1;
    }

    return 0;
}
