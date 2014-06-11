#include "mdd_iface.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <pthread.h>

#include "mdd_utils.h"
#include "log.h"

#define MAX_MSGSIZE 4096

int(*g_iface_changed_cb)(MDDIface_t iface, MDDIfaceState_t state, void *arg) = 0;

int MDDIface_Equal(MDDIface_t *a, MDDIface_t *b)
{
    if (a == NULL || b == NULL) {
        return MDD_FALSE;
    }

    if (a->ifindex != b->ifindex) {
        return MDD_FALSE;
    } else {
        if (a->addr.family == MDDNet_INET) {
            if (a->addr.family == b->addr.family &&
                    a->addr.ip.ipv4 == b->addr.ip.ipv4) {
                return MDD_TRUE;
            }
        } else {
            if (a->addr.family == b->addr.family &&
                    !memcmp(a->addr.ip.data, b->addr.ip.data, sizeof(a->addr.ip))) {
                return MDD_TRUE;
            }
        }
        return MDD_FALSE;
    }
}

int MDDIface_Listall(MDDIface_t **iflist, int *count)
{
    MDDIface_t *outlist = NULL;
    int n = 0;
    struct ifaddrs *ifalist = NULL;
    struct ifaddrs *ifa = NULL;

    if (iflist == NULL) {
        return MDD_ERROR;
    }

    if (getifaddrs(&ifalist) < 0) {
        return MDD_ERROR;
    }

    for (ifa = ifalist; ifa != NULL; ifa = ifa->ifa_next) {
        if (!(ifa->ifa_flags & IFF_UP) ||
                !(ifa->ifa_flags & IFF_MULTICAST) ||
                (ifa->ifa_flags & IFF_LOOPBACK) ||
                ifa->ifa_addr == NULL) {
            LOG_DEBUG("MDDIface got un-support interface, name: %s\n", ifa->ifa_name);
            continue;
        }

        if (ifa->ifa_addr->sa_family == AF_INET) { // for IPv4 address
            struct sockaddr_in *in = (struct sockaddr_in *)ifa->ifa_addr;
            struct sockaddr_in *netmask = (struct sockaddr_in *)ifa->ifa_netmask;
            // allocate memory space
            if (n == 0) {
                outlist = malloc(sizeof(MDDIface_t));
            } else {
                outlist = realloc(outlist, sizeof(MDDIface_t) * (n + 1));
            }
            outlist[n].ifindex = if_nametoindex(ifa->ifa_name);
            outlist[n].addr.family = MDDNet_INET;
            outlist[n].addr.ip.ipv4 = in->sin_addr.s_addr;
            outlist[n].netmask.family = MDDNet_INET;
            outlist[n].netmask.ip.ipv4 = netmask->sin_addr.s_addr;
            n++;
        } else if (ifa->ifa_addr->sa_family == AF_INET6) { // for IPv6 address
            struct sockaddr_in6 *in6 = (struct sockaddr_in6 *)ifa->ifa_addr;
            struct sockaddr_in6 *netmask6 = (struct sockaddr_in6 *)ifa->ifa_netmask;
            // allocate memory space
            if (n == 0) {
                outlist = malloc(sizeof(MDDIface_t));
            } else {
                outlist = realloc(outlist, sizeof(MDDIface_t) * (n + 1));
            }
            outlist[n].ifindex = if_nametoindex(ifa->ifa_name);
            outlist[n].addr.family = MDDNet_INET6;
            memcpy(outlist[n].addr.ip.ipv6, in6->sin6_addr.s6_addr, 16);
            outlist[n].netmask.family = MDDNet_INET6;
            memcpy(outlist[n].netmask.ip.ipv6, netmask6->sin6_addr.s6_addr, 16);
            n++;
        }
    }
    if (ifalist != NULL)
        freeifaddrs(ifalist);

    *iflist = outlist;
    *count = n;

    return MDD_OK;
}

static int netlink_send_req(int sock, int type)
{
    char req[1024];
    struct nlmsghdr *nlh;
    struct rtgenmsg *gen;

    memset(req, 0, sizeof(req));
    nlh = (struct nlmsghdr*) req;
    nlh->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtgenmsg));
    nlh->nlmsg_type = type;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP | NLM_F_ACK;
    nlh->nlmsg_pid = 0;
    nlh->nlmsg_seq = 0;

    gen = NLMSG_DATA(nlh);
    memset(gen, 0, sizeof(struct rtgenmsg));
    gen->rtgen_family = AF_UNSPEC;

    if (send(sock, nlh, nlh->nlmsg_len, 0) < 0) {
        LOG_ERROR("error when send %d request to netlink\n", type);
        return -1;
    }
    return 0;
}

static void *monitor_iface_thread(void *arg)
{
    struct ifaddr_list {
        MDDIface_t iface;
        struct ifaddr_list *prev;
        struct ifaddr_list *next;
    };

    struct sockaddr_nl addr;
    int sock, len, ioctl_sock;
    unsigned char *buffer;
    struct nlmsghdr *nlh;
    struct iovec iov;
    struct msghdr msg;
    int stop_report = 0;
    int if_valid = 0;

    // linked-list for address list
    struct ifaddr_list *ifaddr_cur = NULL;
    struct ifaddr_list *ifaddr_head = NULL;
    struct ifaddr_list *ifaddr_tail = NULL;

    if (g_iface_changed_cb == NULL) {
        return NULL;
    }

    if ((sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)) < 0) {
        LOG_ERROR("couldn't open NETLINK_ROUTE socket, error: %s\n", strerror(errno));
        return NULL;
    }

    if ((ioctl_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        LOG_ERROR("couldn't open SOCK_DGRAM socket, error: %s\n", strerror(errno));
        return NULL;
    }


    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_groups = RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR;
    addr.nl_pid = 0;

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        LOG_ERROR("couldn't bind socket for netlink, error: %s\n", strerror(errno));
        close(sock);
        return NULL;
    }

    // query ip address
    if (netlink_send_req(sock, RTM_GETADDR) < 0) {
        LOG_ERROR("error when send GETADDR request to netlink\n");
    }

    buffer = malloc(NLMSG_SPACE(MAX_MSGSIZE));

    while (1) {
        memset(buffer, 0, NLMSG_SPACE(MAX_MSGSIZE));
        iov.iov_base = (void *)buffer;
        iov.iov_len = NLMSG_SPACE(MAX_MSGSIZE);
        msg.msg_name = NULL;
        msg.msg_namelen = 0;
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        if ((len = recvmsg(sock, &msg, 0)) < 0) {
            LOG_ERROR("error when recv from netlink, error: %s\n", strerror(errno));
            break;
        }

        nlh = (struct nlmsghdr *)buffer;
        for (; len > 0 && NLMSG_OK(nlh, len); nlh = NLMSG_NEXT(nlh, len)) {
            if (nlh->nlmsg_type == RTM_NEWADDR || nlh->nlmsg_type == RTM_DELADDR) {
                struct ifaddrmsg *ifa = (struct ifaddrmsg *) NLMSG_DATA(nlh);
                struct rtattr *rth = IFA_RTA(ifa);
                int rtl = IFA_PAYLOAD(nlh);

                // only support ipv4 and ipv6
                if (ifa->ifa_family != AF_INET && ifa->ifa_family != AF_INET6) {
                    continue;
                }

                while (rtl && RTA_OK(rth, rtl)) {
                    //if (rth->rta_type == IFA_LOCAL || rth->rta_type == IFA_ADDRESS) {
                    if (rth->rta_type == IFA_ADDRESS) {
                        if (ifa->ifa_family == AF_INET || ifa->ifa_family == AF_INET6) {

                            if ((ifa->ifa_family == AF_INET && RTA_PAYLOAD(rth) != 4) ||
                                    (ifa->ifa_family == AF_INET6 && RTA_PAYLOAD(rth) != 16)) {
                                LOG_WARN("Got ip address but length is not valid, length: %d, index: %u\n", RTA_PAYLOAD(rth), ifa->ifa_index);
                                rth = RTA_NEXT(rth, rtl);
                                continue;
                            }

                            // check whether the address is already in the list
                            ifaddr_cur = ifaddr_head;
                            while (ifaddr_cur) {
                                if (ifaddr_cur->iface.ifindex == ifa->ifa_index &&
                                        ifaddr_cur->iface.addr.family == MDDNet_af2mdd(ifa->ifa_family) &&
                                        !memcmp(ifaddr_cur->iface.addr.ip.data, RTA_DATA(rth), RTA_PAYLOAD(rth))) {
                                    LOG_DEBUG("Got duplicate ip address on interface: %d\n", ifa->ifa_index);
                                    break;
                                }
                                ifaddr_cur = ifaddr_cur->next;
                            }

                            // add/remove to/from address list
                            if (ifaddr_cur != NULL && nlh->nlmsg_type == RTM_DELADDR) {
                                // call interface removed callback
                                if (g_iface_changed_cb(ifaddr_cur->iface, MDDIfaceState_Remove, arg) != MDD_OK) {
                                    stop_report = 1;
                                    break;
                                }

                                // remove from address list
                                if (ifaddr_cur == ifaddr_head && ifaddr_cur == ifaddr_tail) {
                                    ifaddr_head = NULL;
                                    ifaddr_tail = NULL;
                                } else if (ifaddr_cur == ifaddr_tail) {
                                    ifaddr_tail = ifaddr_cur->prev;
                                    ifaddr_tail->next = NULL;
                                } else if (ifaddr_cur == ifaddr_head) {
                                    ifaddr_head = ifaddr_cur->next;
                                    ifaddr_head->prev = NULL;
                                } else {
                                    ifaddr_cur->prev->next = ifaddr_cur->next;
                                    ifaddr_cur->next->prev = ifaddr_cur->prev;
                                }
                                free(ifaddr_cur);
                                ifaddr_cur = NULL;

                            } else if (ifaddr_cur == NULL && nlh->nlmsg_type == RTM_NEWADDR) {
                                struct ifreq ifr;
                                // get if flags
                                memset(&ifr, 0, sizeof(struct ifreq));
                                if_indextoname(ifa->ifa_index, ifr.ifr_name);
                                ioctl(ioctl_sock, SIOCGIFFLAGS, &ifr);
                                LOG_DEBUG("MDDIface interface name: %s, flags: %u\n", ifr.ifr_name, ifr.ifr_flags);

                                // check if the address from a valid interface
                                if_valid = 0;
                                if ((ifr.ifr_flags & IFF_MULTICAST) &&
                                        !(ifr.ifr_flags & IFF_LOOPBACK)) {
                                    if_valid = 1;
                                }

                                if (!if_valid) {
                                    LOG_DEBUG("MDDIface got address from un-support interface, interface index: %d\n", ifa->ifa_index);
                                    rth = RTA_NEXT(rth, rtl);
                                    continue;
                                }

                                ifaddr_cur = malloc(sizeof(struct ifaddr_list));
                                memset(ifaddr_cur, 0, sizeof(struct ifaddr_list));
                                ifaddr_cur->iface.ifindex = ifa->ifa_index;
                                ifaddr_cur->iface.addr.family = MDDNet_af2mdd(ifa->ifa_family);
                                memcpy(ifaddr_cur->iface.addr.ip.data, RTA_DATA(rth), RTA_PAYLOAD(rth));
                                // fill netmask
                                memset(ifaddr_cur->iface.netmask.ip.data, 0xff, (int)(ifa->ifa_prefixlen / 8));
                                if (ifa->ifa_prefixlen % 8) {
                                    char last_prefix = 0x00;
                                    switch ((ifa->ifa_prefixlen % 8) % 4) {
                                        case 1:
                                            last_prefix = 0x80;
                                            break;
                                        case 2:
                                            last_prefix = 0xc0;
                                            break;
                                        case 3:
                                            last_prefix = 0xe0;
                                            break;
                                        default:
                                            break;
                                    }
                                    if ((ifa->ifa_prefixlen % 8) >= 4) {
                                        last_prefix >>= 4;
                                        last_prefix |= 0xf0;
                                    }
                                    ifaddr_cur->iface.netmask.ip.data[(int)(ifa->ifa_prefixlen / 8)] = last_prefix;
                                }
                                ifaddr_cur->prev = NULL;
                                ifaddr_cur->next = NULL;
                                if (ifaddr_head == NULL) {
                                    ifaddr_head = ifaddr_cur;
                                    ifaddr_tail = ifaddr_cur;
                                } else {
                                    ifaddr_head->prev = ifaddr_cur;
                                    ifaddr_cur->next = ifaddr_head;
                                    ifaddr_head = ifaddr_cur;
                                }

                                // call interface changed callback
                                if (g_iface_changed_cb(ifaddr_cur->iface, MDDIfaceState_Add, arg) != MDD_OK) {
                                    stop_report = 1;
                                    break;
                                }

                            } else {
                                if (nlh->nlmsg_type == RTM_NEWADDR) {
                                    LOG_DEBUG("Netlink got duplicate address from interface: %d\n", ifa->ifa_index);
                                } else {
                                    LOG_WARN("Netlink got unsupport address removed from interface: %d\n", ifa->ifa_index);
                                }
                            }
                        }
                    }
                    rth = RTA_NEXT(rth, rtl);
                }

                if (stop_report) {
                    break;
                }

            } else if (nlh->nlmsg_type == NLMSG_DONE) {
                break;
            }
        }

        if (stop_report) {
            break;
        }
    }

    // free buffer
    free(buffer);

    // clear all data
    close(sock);
    close(ioctl_sock);
    // free address list
    while (ifaddr_head) {
        ifaddr_cur = ifaddr_head->next;
        free(ifaddr_head);
        ifaddr_head = ifaddr_cur;
    }

    return NULL;
}

int MDDIface_Monitoriface(int(*iface_changed)(MDDIface_t iface, MDDIfaceState_t state, void *arg), void *arg)
{
    pthread_t thread;

    if (iface_changed == NULL) {
        return MDD_ERROR;
    }

    g_iface_changed_cb = iface_changed;
    if (pthread_create(&thread, NULL, monitor_iface_thread, arg) != 0) {
        return MDD_ERROR;
    }
    pthread_detach(thread);
    return MDD_OK;
}

MDDIfaceType_t MDDIface_Gettype(MDDIface_t iface)
{
    unsigned int ifindex = iface.ifindex;
    char ifName[IF_NAMESIZE];
    //FIXME: determine the interface type with better mechanism
    if (ifindex > 0 && if_indextoname(ifindex, ifName)) {
        if (strstr(ifName, "eth")) {
            return MDDIfaceType_Ethernet;
        } else if (strstr(ifName, "wlan")) {
            return MDDIfaceType_Wifi;
        } else if (strstr(ifName, "ra")) {
            return MDDIfaceType_Wifi;
        } else if (strstr(ifName, "p2p")) {
            return MDDIfaceType_Wifi;
        }
    }
    return MDDIfaceType_Unknown;
}
