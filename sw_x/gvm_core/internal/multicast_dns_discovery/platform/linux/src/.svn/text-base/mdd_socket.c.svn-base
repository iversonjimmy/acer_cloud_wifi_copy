#include "mdd_socket.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <net/if.h>
#ifndef __USE_GNU
#  error
#endif
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "mdd_utils.h"
#include "log.h"

int MDDSocket_Init(void)
{
    // nothing to do on linux platform
    return MDD_OK;
}

int MDDSocket_Quit(void)
{
    // nothing to do on linux platform
    return MDD_OK;
}

MDDSocket_t MDDSocket_Create(MDDNetProto_t family, int type)
{
    MDDSocket_t s;
    if ((s = socket(MDDNet_mdd2af(family), type, 0)) < 0) {
        return MDDSocket_INVALID;
    }
    return s;
}

int MDDSocket_Setsockopt(MDDSocket_t socket, int level, int optname, const void *optval, int optlen)
{
    if (socket < 0) {
        return MDD_ERROR;
    }

    if (setsockopt(socket, level, optname, optval, optlen) < 0) {
        LOG_ERROR("MDDSocket, setsockopt error, error message: %s\n", strerror(errno));
        return MDD_ERROR;
    } else {
        return MDD_OK;
    }
}

int MDDSocket_Bind(MDDSocket_t socket, MDDSocketAddr_t *my_addr)
{
    if (socket < 0 || my_addr == NULL) {
        return MDD_ERROR;
    }

    if (my_addr->family == MDDNet_INET) {
        struct sockaddr_in in;

        memset(&in, 0, sizeof(in));
        in.sin_family = AF_INET;
        in.sin_port = my_addr->port;
        if (my_addr->addr.ip.ipv4 == 0) {
            in.sin_addr.s_addr = INADDR_ANY;
        } else {
            in.sin_addr.s_addr = my_addr->addr.ip.ipv4;
        }

        if (bind(socket, (struct sockaddr*)&in, sizeof(in)) < 0) {
            LOG_ERROR("MDDSocket, bind error, error message: %s\n", strerror(errno));
            return MDD_ERROR;
        } else {
            return MDD_OK;
        }
    } else if (my_addr->family == MDDNet_INET6) {
        int anyaddr = 1;
        int i = 0;
        struct sockaddr_in6 in;

        memset(&in, 0, sizeof(in));
        in.sin6_family = AF_INET6;
        in.sin6_port = my_addr->port;
        for (i = 0; i < 16; i++) {
            if (my_addr->addr.ip.ipv6[i] != 0x00) {
                anyaddr = 0;
            }
        }
        if (anyaddr) {
            in.sin6_addr = in6addr_any;
        } else {
            memcpy(in.sin6_addr.s6_addr, my_addr->addr.ip.ipv6, 16);
        }

        if (bind(socket, (struct sockaddr*)&in, sizeof(in)) < 0) {
            LOG_ERROR("MDDSocket, bind error, error message: %s\n", strerror(errno));
            return MDD_ERROR;
        } else {
            return MDD_OK;
        }
    } else {
        return MDD_ERROR;
    }
}

int MDDSocket_Close(MDDSocket_t socket)
{
    if (socket < 0) {
        return MDD_ERROR;
    }

    if (close(socket) < 0) {
        LOG_ERROR("MDDSocket, close error, error message: %s\n", strerror(errno));
        return MDD_ERROR;
    } else {
        return MDD_OK;
    }
}

int MDDSocket_Sendto(MDDSocket_t socket, unsigned char *buf, int buf_len, MDDSocketAddr_t *to, MDDIface_t *iface)
{
    struct sockaddr_in sa;
    struct sockaddr_in6 sa6;
    struct msghdr msg;
    struct iovec io;
    int sent_len = 0;

    if (socket < 0) {
        return MDD_ERROR;
    }

    memset(&io, 0, sizeof(io));
    io.iov_base = buf;
    io.iov_len = buf_len;

    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_flags = 0;
    msg.msg_control = NULL;
    msg.msg_controllen = 0;

    if (to->family == MDDNet_INET) {
        memset(&sa, 0, sizeof(sa));
        sa.sin_family = AF_INET;
        sa.sin_port = to->port;
        sa.sin_addr.s_addr = to->addr.ip.ipv4;
        msg.msg_name = &sa;
        msg.msg_namelen = sizeof(sa);
    } else if (to->family == MDDNet_INET6) {
        memset(&sa6, 0, sizeof(sa6));
        sa6.sin6_family = AF_INET6;
        sa6.sin6_port = to->port;
        memcpy(sa6.sin6_addr.s6_addr, to->addr.ip.ipv6, 16);
        msg.msg_name = &sa6;
        msg.msg_namelen = sizeof(sa6);
    } else {
        return MDD_ERROR;
    }

    if (iface != NULL && iface->ifindex > 0) {
        if (to->family == MDDNet_INET) {
            struct in_pktinfo *pkti;
            struct cmsghdr *cmsg;
            char cmsg_data[CMSG_SPACE(sizeof(struct in_pktinfo)) + 1];

            memset(cmsg_data, 0, sizeof(cmsg_data));
            msg.msg_control = cmsg_data;
            msg.msg_controllen = CMSG_LEN(sizeof(struct in_pktinfo));

            cmsg = (struct cmsghdr *)CMSG_FIRSTHDR(&msg);
            cmsg->cmsg_len = msg.msg_controllen;
            cmsg->cmsg_level = IPPROTO_IP;
            cmsg->cmsg_type = IP_PKTINFO;

            pkti = (struct in_pktinfo*) CMSG_DATA(cmsg);

            if (iface->ifindex > 0) {
                pkti->ipi_ifindex = iface->ifindex;
                pkti->ipi_spec_dst.s_addr = iface->addr.ip.ipv4;
            }
        } else if (to->family == MDDNet_INET6) {
            struct in6_pktinfo *pkti;
            struct cmsghdr *cmsg;
            char cmsg_data[CMSG_SPACE(sizeof(struct in6_pktinfo)) + 1];

            memset(cmsg_data, 0, sizeof(cmsg_data));
            msg.msg_control = cmsg_data;
            msg.msg_controllen = CMSG_LEN(sizeof(struct in6_pktinfo));

            cmsg = CMSG_FIRSTHDR(&msg);
            cmsg->cmsg_len = msg.msg_controllen;
            cmsg->cmsg_level = IPPROTO_IPV6;
            cmsg->cmsg_type = IPV6_PKTINFO;

            pkti = (struct in6_pktinfo*) CMSG_DATA(cmsg);

            if (iface->ifindex > 0) {
                pkti->ipi6_ifindex = iface->ifindex;
                memcpy(&pkti->ipi6_addr, iface->addr.ip.ipv6, 16);
            }
        }
    }

    if ((sent_len = sendmsg(socket, &msg, 0)) < 0) {
        LOG_ERROR("MDDSocket_Sendto error, error: %s\n", strerror(errno));
        return MDD_ERROR;
    } else {
        return sent_len;
    }
}

int MDDSocket_Recvfrom(MDDSocket_t socket, unsigned char *buf, int buf_len, MDDSocketAddr_t *from, MDDIface_t *iface)
{
    struct sockaddr_in sa;
    struct sockaddr_in6 sa6;
    struct msghdr msg;
    struct iovec io;
    size_t aux[1024 / sizeof(size_t)];
    int recv_len = 0;

    if (socket < 0) {
        return MDD_ERROR;
    }

    io.iov_base = buf;
    io.iov_len = buf_len;

    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_control = aux;
    msg.msg_controllen = sizeof(aux);
    msg.msg_flags = 0;

    if (from->family == MDDNet_INET) {
        msg.msg_name = &sa;
        msg.msg_namelen = sizeof(sa);
    } else if (from->family == MDDNet_INET6) {
        msg.msg_name = &sa6;
        msg.msg_namelen = sizeof(sa6);
    } else {
        LOG_ERROR("MDDSocket_Recvfrom error, need to specific family on from address\n");
        return MDD_ERROR;
    }

    if ((recv_len = recvmsg(socket, &msg, 0)) < 0) {
        if (errno != EAGAIN) {
            LOG_ERROR("MDDSocket_Recvfrom fatal error, error: %s\n", strerror(errno));
            return MDD_ERROR;
        }
        return 0;
    } else {
        struct cmsghdr *cmsg;

        if (from->family == MDDNet_INET) {
            from->port = sa.sin_port;
            from->addr.ip.ipv4 = (unsigned long int)sa.sin_addr.s_addr;
        } else if (from->family == MDDNet_INET6) {
            from->port = sa6.sin6_port;
            memcpy(from->addr.ip.ipv6, sa6.sin6_addr.s6_addr, 16);
        }

        for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
            if (cmsg->cmsg_level == IPPROTO_IP) {
                switch (cmsg->cmsg_type) {
                    case IP_TTL:
                        {
                            int ttlData;
                            memcpy(&ttlData, CMSG_DATA(cmsg), sizeof(ttlData));
                            LOG_TRACE("MDDSocket_Recvfrom got ttl: %d\n", ttlData);
                        }
                        break;

                    case IP_PKTINFO: {
                        struct in_pktinfo *i = (struct in_pktinfo*) CMSG_DATA(cmsg);

                        LOG_TRACE("MDDSocket_Recvfrom got ifindex: %d\n", i->ipi_ifindex);
                        if (iface != NULL) {
                            iface->ifindex = i->ipi_ifindex;
                            iface->addr.family = MDDNet_INET;
                            iface->addr.ip.ipv4 = i->ipi_addr.s_addr;
                        }
                        break;
                    }

                    default:
                        LOG_DEBUG("Unhandled cmsg_type: %d", cmsg->cmsg_type);
                        break;
                }
            } else if (cmsg->cmsg_level == IPPROTO_IPV6) {
                switch (cmsg->cmsg_type) {
                    case IPV6_HOPLIMIT:
                        {
                            int hopLimitData;
                            memcpy(&hopLimitData, CMSG_DATA(cmsg), sizeof(hopLimitData));
                            LOG_TRACE("MDDSocket_Recvfrom got ttl: %d\n", hopLimitData);
                        }
                        break;

                    case IPV6_PKTINFO: {
                        struct in6_pktinfo *i = (struct in6_pktinfo*) CMSG_DATA(cmsg);

                        LOG_TRACE("MDDSocket_Recvfrom got ifindex: %d\n", i->ipi6_ifindex);
                        if (iface != NULL) {
                            iface->ifindex = i->ipi6_ifindex;
                            iface->addr.family = MDDNet_INET6;
                            memcpy(iface->addr.ip.ipv6, i->ipi6_addr.s6_addr, 16);
                        }
                        break;
                    }

                    default:
                        LOG_TRACE("Unhandled cmsg_type: %d", cmsg->cmsg_type);
                        break;
                }
            }
        }
        LOG_TRACE("MDDSocket_Recvfrom recv_len: %d\n", recv_len);
        return recv_len;
    }
}

int MDDSocket_Setnonblock(MDDSocket_t socket)
{
    int flag = 0;

    if (socket < 0) {
        return MDD_ERROR;
    }

    flag =  fcntl(socket, F_GETFL, 0);
    flag |= O_NONBLOCK;
    if (fcntl(socket, F_SETFL, flag) < 0) {
        return MDD_ERROR;
    } else {
        return MDD_OK;
    }
}

int MDDSocket_Jointomulticast(MDDSocket_t socket, int join, MDDNetAddr_t *multicast_addr, MDDIface_t *iface)
{
    if (socket < 0 || multicast_addr == NULL) {
        return MDD_ERROR;
    }

    if (multicast_addr->family == MDDNet_INET) {
        struct ip_mreq mreq;

        memset(&mreq, 0, sizeof(mreq));
        mreq.imr_multiaddr.s_addr = multicast_addr->ip.ipv4;
        mreq.imr_interface.s_addr = iface->addr.ip.ipv4;
        if (MDDSocket_Setsockopt(socket, MDDSocket_IPPROTO_IP, join ? MDDSocket_IP_ADD_MEMBERSHIP : MDDSocket_IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
            LOG_ERROR("Error when %s to multicast group (ipv4)\n", join ? "add" : "remove");
            return MDD_ERROR;
        } else {
            return MDD_OK;
        }
    } else if (multicast_addr->family == MDDNet_INET6) {
        struct ipv6_mreq mreq6;

        memset(&mreq6, 0, sizeof(mreq6));
        memcpy(mreq6.ipv6mr_multiaddr.s6_addr, multicast_addr->ip.ipv6, 16);
        mreq6.ipv6mr_interface = iface->ifindex;
        if (MDDSocket_Setsockopt(socket, MDDSocket_IPPROTO_IPV6, join ? MDDSocket_IPV6_ADD_MEMBERSHIP : MDDSocket_IPV6_DROP_MEMBERSHIP, &mreq6, sizeof(mreq6)) < 0) {
            LOG_ERROR("Error when %s multicast group (ipv6)\n", join ? "add to" : "remove from");
            return MDD_ERROR;
        } else {
            return MDD_OK;
        }
    } else {
        return MDD_ERROR;
    }
}

int MDDSocket_Getsockname(MDDSocket_t socket, MDDSocketAddr_t *addr)
{
    struct sockaddr_in in;
    struct sockaddr_in6 in6;
    socklen_t addr_len = 0;

    if (socket < 0 || addr == NULL) {
        return MDD_ERROR;
    }

    if (addr->family == MDDNet_INET) {
        addr_len = sizeof(in);
        if ((getsockname(socket, (struct sockaddr *)&in, &addr_len)) < 0) {
            LOG_ERROR("MDDSocket, getsockname error, error message: %s\n", strerror(errno));
            return MDD_ERROR;
        }
        addr->port = in.sin_port;
        addr->addr.ip.ipv4 = (unsigned long int)in.sin_addr.s_addr;
        return MDD_OK;
    } else if (addr->family == MDDNet_INET6) {
        addr_len = sizeof(in6);
        if ((getsockname(socket, (struct sockaddr *)&in6, &addr_len)) < 0) {
            LOG_ERROR("MDDSocket, getsockname error, error message: %s\n", strerror(errno));
            return MDD_ERROR;
        }
        addr->port = in6.sin6_port;
        memcpy(addr->addr.ip.ipv6, in6.sin6_addr.s6_addr, 16);
        return MDD_OK;
    } else {
        LOG_ERROR("MDDSocket_Getsockname error, need to specific family on address\n");
        return MDD_ERROR;
    }
}
