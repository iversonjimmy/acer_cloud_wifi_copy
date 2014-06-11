#include "mdd_socket.h"

#include <winSock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <Mswsock.h>


#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>

#pragma comment(lib, "ws2_32.lib")

#include "mdd_utils.h"
#include "log.h"

#define MALLOC(x) HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,x)
#define MSIZE(p) HeapSize(GetProcessHeap(),0,p)
#define CLOSESOCK(s) if(INVALID_SOCKET != s) {closesocket(s); s = INVALID_SOCKET;}

LPFN_WSASENDMSG fpWSASendMsg = NULL;
LPFN_WSARECVMSG fpWSARecvMsg = NULL;
SOCKET sendMsgSocket = INVALID_SOCKET;
SOCKET recvMsgSocket = INVALID_SOCKET;

LPFN_WSASENDMSG GetWSASendMsgFunctionPointer(SOCKET sock)
{
    LPFN_WSASENDMSG     lpfnWSASendMsg = NULL;
    GUID                guidWSASendMsg = WSAID_WSASENDMSG;
    DWORD               dwBytes = 0;

    if(SOCKET_ERROR == WSAIoctl(sock,
                                SIO_GET_EXTENSION_FUNCTION_POINTER,
                                &guidWSASendMsg,
                                sizeof(guidWSASendMsg),
                                &lpfnWSASendMsg,
                                sizeof(lpfnWSASendMsg),
                                &dwBytes,
                                NULL,
                                NULL
                                ))
    {
        LOG_ERROR("WSAIoctl SIO_GET_EXTENSION_FUNCTION_POINTER\n");
        return NULL;
    }

    return lpfnWSASendMsg;
}

LPFN_WSARECVMSG GetWSARecvMsgFunctionPointer(SOCKET sock)
{
    LPFN_WSARECVMSG     lpfnWSARecvMsg = NULL;
    GUID                guidWSARecvMsg = WSAID_WSARECVMSG;
    DWORD               dwBytes = 0;

    if(SOCKET_ERROR == WSAIoctl(sock,
                                SIO_GET_EXTENSION_FUNCTION_POINTER,
                                &guidWSARecvMsg,
                                sizeof(guidWSARecvMsg),
                                &lpfnWSARecvMsg,
                                sizeof(lpfnWSARecvMsg),
                                &dwBytes,
                                NULL,
                                NULL
                                ))
    {
        LOG_ERROR("WSAIoctl SIO_GET_EXTENSION_FUNCTION_POINTER\n");
        return NULL;
    }
    
    return lpfnWSARecvMsg;
}

int MDDSocket_Init(void)
{
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;

    wVersionRequested = MAKEWORD(2, 2);

    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        LOG_ERROR("MDDSocket, WSAStartup failed with error: %d\n", err);
        return MDD_ERROR;
    }

    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
        LOG_ERROR("MDDSocket, Could not find a usable version of Winsock.dll\n");
        WSACleanup();
        return MDD_ERROR;
    }

    sendMsgSocket = socket(AF_INET,SOCK_DGRAM,0);
    if (sendMsgSocket == INVALID_SOCKET)
    {
        LOG_ERROR("MDDSocket,socket function failed when create socket for sendMsg with error = %d\n", WSAGetLastError() );
        return MDD_ERROR;
    }

    if (NULL == (fpWSASendMsg = GetWSASendMsgFunctionPointer(sendMsgSocket)))
    {
        LOG_ERROR("GetWSASendMsgFunctionPointer returns null pointer.\n");
        return MDD_ERROR;
    }

    recvMsgSocket = socket(AF_INET,SOCK_DGRAM,0);
    if (recvMsgSocket == INVALID_SOCKET)
    {
        LOG_ERROR("MDDSocket,socket function failed when create socket for recvMsg with error = %d\n", WSAGetLastError() );
        return MDD_ERROR;
    }

    if (NULL == (fpWSARecvMsg = GetWSARecvMsgFunctionPointer(recvMsgSocket)))
    {
        LOG_ERROR("GetWSARecvMsgFunctionPointer returns null pointer.\n");
        return MDD_ERROR;
    }

    return MDD_OK;
}

int MDDSocket_Quit(void)
{
    closesocket(sendMsgSocket);
    closesocket(recvMsgSocket);

    WSACleanup();

    return MDD_OK;
}

MDDSocket_t MDDSocket_Create(MDDNetProto_t family, int type)
{
    SOCKET s = INVALID_SOCKET;

    s = socket(MDDNet_mdd2af(family), type, 0);
    if (s == INVALID_SOCKET)
    {
        LOG_ERROR("MDDSocket,socket function failed with error = %d\n", WSAGetLastError() );
        return MDDSocket_INVALID;
    }

    return s;
}

int MDDSocket_Setsockopt(MDDSocket_t socket, int level, int optname, const void *optval, int optlen)
{
    if (socket == INVALID_SOCKET) {
        return MDD_ERROR;
    }

    if (setsockopt(socket, level, optname, (const char *)optval, optlen) == SOCKET_ERROR) {
        LOG_ERROR("MDDSocket, setsockopt error: %d\n", WSAGetLastError());
        return MDD_ERROR;
    } else {
        return MDD_OK;
    }
}

int MDDSocket_Bind(MDDSocket_t socket, MDDSocketAddr_t *my_addr)
{
    if (socket == INVALID_SOCKET || my_addr == NULL) {
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

        if (bind(socket, (struct sockaddr*)&in, sizeof(in)) == SOCKET_ERROR) {
            LOG_ERROR("MDDSocket, bind error: %u\n", WSAGetLastError());
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

        if (bind(socket, (struct sockaddr*)&in, sizeof(in)) == SOCKET_ERROR) {
            LOG_ERROR("MDDSocket, bind error: %u\n", WSAGetLastError());
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
    if (socket == INVALID_SOCKET) {
        return MDD_ERROR;
    }

    if (closesocket(socket) == SOCKET_ERROR) {
        LOG_ERROR("MDDSocket, close error: %u\n", WSAGetLastError());
        return MDD_ERROR;
    } else {
        return MDD_OK;
    }
}

int MDDSocket_Sendto(MDDSocket_t socket, unsigned char *buf, int buf_len, MDDSocketAddr_t *to, MDDIface_t *iface)
{
    struct sockaddr_in sa;
    struct sockaddr_in6 sa6;

    WSAMSG msg;
    WSABUF io;

    DWORD sent_len = 0;
    INT nRet = 0;

    if (socket == INVALID_SOCKET) {
        return MDD_ERROR;
    }

    SecureZeroMemory(&io, sizeof(io));
    io.buf = (char *)buf;
    io.len = buf_len;

    SecureZeroMemory (&msg, sizeof(msg));
    msg.lpBuffers = &io;
    msg.dwBufferCount = 1;
    msg.dwFlags = 0;

    if (to->family == MDDNet_INET) {
        memset(&sa, 0, sizeof(sa));
        sa.sin_family = AF_INET;
        sa.sin_port = to->port;
        sa.sin_addr.s_addr = to->addr.ip.ipv4;

        msg.name = (LPSOCKADDR)(&sa);
        msg.namelen = sizeof(sa);
    } else if (to->family == MDDNet_INET6) {
        memset(&sa6, 0, sizeof(sa6));
        sa6.sin6_family = AF_INET6;
        sa6.sin6_port = to->port;
        memcpy(sa6.sin6_addr.s6_addr, to->addr.ip.ipv6, 16);

        msg.name = (LPSOCKADDR)(&sa6);
        msg.namelen = sizeof(sa6);
    } else {
        return MDD_ERROR;
    }

    if (iface != NULL && iface->ifindex > 0) {
        if (to->family == MDDNet_INET) {
            PIN_PKTINFO pkti;
            WSACMSGHDR *cmsg;
            char in[WSA_CMSG_SPACE(sizeof(struct in_pktinfo))];
            int sum = 0;

            memset(in, 0, sizeof(in));

            msg.Control.buf = (char *)&in;
            msg.Control.len = sizeof(in);
            
            cmsg = WSA_CMSG_FIRSTHDR(&msg);
            memset(cmsg, 0, WSA_CMSG_SPACE(sizeof(struct in_pktinfo)));
            cmsg->cmsg_level = IPPROTO_IP;
            cmsg->cmsg_type = IP_PKTINFO;
            
            cmsg->cmsg_len = WSA_CMSG_LEN(sizeof(struct in_pktinfo));

            pkti = (PIN_PKTINFO) WSA_CMSG_DATA(cmsg);

            if (iface->ifindex > 0)
            {
                pkti->ipi_ifindex = iface->ifindex;
                pkti->ipi_addr.S_un.S_addr = iface->addr.ip.ipv4;
                // MD_Tsai@acer.com.tw, 2012/07/24, workaround to resolve can't sent in release mode. DO NOT remove this printf until find solution
                printf("");
            }

            sum += WSA_CMSG_SPACE(sizeof(struct in_pktinfo));

            msg.Control.len = sum;
        } else if (to->family == MDDNet_INET6) {
            PIN6_PKTINFO pkti;
            WSACMSGHDR *cmsg;
            char in[WSA_CMSG_SPACE(sizeof(struct in6_pktinfo))];
            int sum = 0;

            memset(in, 0, sizeof(in));

            msg.Control.buf = (char *)&in;
            msg.Control.len = sizeof(in);

            cmsg = WSA_CMSG_FIRSTHDR(&msg);
            memset(cmsg, 0, WSA_CMSG_SPACE(sizeof(struct in6_pktinfo)));
            cmsg->cmsg_level = IPPROTO_IPV6;
            cmsg->cmsg_type = IPV6_PKTINFO;

            cmsg->cmsg_len = WSA_CMSG_LEN(sizeof(struct in6_pktinfo));

            pkti = (PIN6_PKTINFO) WSA_CMSG_DATA(cmsg);

            if (iface->ifindex > 0)
            {
                pkti->ipi6_ifindex = iface->ifindex;
                memcpy(&pkti->ipi6_addr.u.Byte, iface->addr.ip.ipv6, 16);
                // MD_Tsai@acer.com.tw, 2012/07/24, workaround to resolve can't sent in release mode. DO NOT remove this printf until find solution
                printf("");
            }

            sum += WSA_CMSG_SPACE(sizeof(struct in6_pktinfo));

            msg.Control.len = sum;
        }
    }

    if (SOCKET_ERROR == (nRet = fpWSASendMsg(socket,
                                           &msg,
                                           0,
                                           &sent_len,
                                           NULL,
                                           NULL)))
    {
        LOG_ERROR("MDDSocket_Sendto by WSASendMsg error %d\n", WSAGetLastError());
        return MDD_ERROR;
    } else {
        return sent_len;
    }
}

int MDDSocket_Recvfrom(MDDSocket_t sockets, unsigned char *buf, int buf_len, MDDSocketAddr_t *from, MDDIface_t *iface)
{
    struct sockaddr_in sa;
    struct sockaddr_in6 sa6;

    WSAMSG msg;
    WSABUF io;
    char aux[1024];
    DWORD recv_len = 0;

    if (sockets == INVALID_SOCKET) {
        return MDD_ERROR;
    }

    io.buf = (CHAR *)buf;
    io.len = buf_len;

    SecureZeroMemory (&msg, sizeof(msg));
    msg.lpBuffers = &io;
    msg.dwBufferCount = 1;
    msg.dwFlags = 0;
    msg.Control.buf = aux;
    msg.Control.len = sizeof(aux);

    if (from->family == MDDNet_INET) {
        msg.name = (LPSOCKADDR)&sa;
        msg.namelen = sizeof(sa);
    } else if (from->family == MDDNet_INET6) {
        msg.name = (LPSOCKADDR)&sa6;
        msg.namelen = sizeof(sa6);
    } else {
        LOG_ERROR("MDDSocket_Recvfrom error, need to specific family on from address\n");
        return MDD_ERROR;
    }

    if (SOCKET_ERROR == fpWSARecvMsg(sockets,
                                   &msg,
                                   &recv_len,
                                   NULL,
                                   NULL))
    {
        if (WSAGetLastError() != WSAEWOULDBLOCK)
        {
            LOG_ERROR("MDDSocket_Recvfrom fatal error, error: %d\n", WSAGetLastError());
            return MDD_ERROR;
        }

        return 0;
    }
    else {
        WSACMSGHDR *cmsg;

        if (from->family == MDDNet_INET) {
            from->port = sa.sin_port;
            from->addr.ip.ipv4 = (unsigned long int)sa.sin_addr.s_addr;
        } else if (from->family == MDDNet_INET6) {
            from->port = sa6.sin6_port;
            memcpy(from->addr.ip.ipv6, sa6.sin6_addr.u.Word, 16);
        }

        for (cmsg = WSA_CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = WSA_CMSG_NXTHDR(&msg, cmsg)) {
            if (cmsg->cmsg_level == IPPROTO_IP) {
                switch (cmsg->cmsg_type) {
                    case IP_TTL:
                        LOG_TRACE("MDDSocket_Recvfrom got ttl: %d\n", (*(int *) WSA_CMSG_DATA(cmsg)));
                        break;

                    case IP_PKTINFO: {
                        PIN_PKTINFO i = (PIN_PKTINFO) WSA_CMSG_DATA(cmsg);

                        LOG_TRACE("MDDSocket_Recvfrom got ifindex: %d\n", i->ipi_ifindex);
                        if (iface != NULL) {
                            iface->ifindex = i->ipi_ifindex;
                            iface->addr.family = MDDNet_INET;
                            iface->addr.ip.ipv4 = i->ipi_addr.S_un.S_addr;
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
                        LOG_TRACE("MDDSocket_Recvfrom got ttl: %d\n", (*(int *) WSA_CMSG_DATA(cmsg)));
                        break;

                    case IPV6_PKTINFO: {
                        PIN6_PKTINFO i = (PIN6_PKTINFO) WSA_CMSG_DATA(cmsg);

                        LOG_TRACE("MDDSocket_Recvfrom got ifindex: %d\n", i->ipi6_ifindex);
                        if (iface != NULL) {
                            iface->ifindex = i->ipi6_ifindex;
                            iface->addr.family = MDDNet_INET6;
                            memcpy(iface->addr.ip.ipv6, i->ipi6_addr.u.Byte, 16);
                        }
                        break;
                    }

                    default:
                        LOG_DEBUG("Unhandled cmsg_type: %d", cmsg->cmsg_type);
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
    int iResult = 0;
    u_long iMode = 1;

    if (socket == INVALID_SOCKET) {
        return MDD_ERROR;
    }

    iResult =  ioctlsocket(socket, FIONBIO, &iMode);
    if (iResult != NO_ERROR) {
        LOG_ERROR("MDDSocket, Setnonblock error: %u\n", iResult);
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
        if ((getsockname(socket, (struct sockaddr *)&in, &addr_len)) == SOCKET_ERROR) {
            LOG_ERROR("MDDSocket, getsockname error, error: %d\n", WSAGetLastError());
            return MDD_ERROR;
        }
        addr->port = in.sin_port;
        addr->addr.ip.ipv4 = (unsigned long int)in.sin_addr.s_addr;
        return MDD_OK;
    } else if (addr->family == MDDNet_INET6) {
        addr_len = sizeof(in6);
        if ((getsockname(socket, (struct sockaddr *)&in6, &addr_len)) == SOCKET_ERROR) {
            LOG_ERROR("MDDSocket, getsockname error, error: %d\n", WSAGetLastError());
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
