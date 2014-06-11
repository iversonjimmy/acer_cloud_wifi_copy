/*
 *                Copyright (C) 2008, BroadOn Communications Corp.
 *
 *   These coded instructions, statements, and computer programs contain
 *   unpublished proprietary information of BroadOn Communications Corp.,
 *   and are protected by Federal copyright law. They may not be disclosed
 *   to third parties or copied or duplicated in any form, in whole or in
 *   part, without the prior written consent of BroadOn Communications Corp.
 *
 */

#include "vpl_socket.h"
#include "vplu.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>  /* For SIO_UDP_CONNRESET */
#include <iphlpapi.h> /* For GetIpForwardTable */

#include "vpl_error.h"
#include "vpl_types.h"
#include "vpl_time.h"
#include "vpl_th.h" /// for VPLThread_Yield

//----------------
// These definitions are not available in MinGW:
//----------------
#define SIO_KEEPALIVE_VALS  0x98000004

/* Argument structure for SIO_KEEPALIVE_VALS */
struct tcp_keepalive {
    u_long  onoff;
    u_long  keepalivetime;
    u_long  keepaliveinterval;
};
//----------------

int VPLSocket_Init(void) {
    WORD requestedVersion = 2 | (2 << 8); // request v 2.2
    WSADATA implDetails;
    if (WSAStartup(requestedVersion, &implDetails) != 0) {
        return VPL_ERR_FAIL;
    }
    if (implDetails.wVersion != requestedVersion) {
        WSACleanup();
        return VPL_ERR_FAIL;
    }
    return VPL_OK;
}

int VPLSocket_Quit(void) {
    if (WSACleanup() == 0) {
        return VPL_OK;
    } else {
        return VPL_ERR_FAIL;
    }
}
 
// open a socket of a given type.
VPLSocket_t VPLSocket_Create(int family, int type, int nonblock)
{
    VPLSocket_t sockfd = VPLSOCKET_INVALID;

    sockfd.s = socket(family, type, 0);
    if (VPLSocket_Equal(sockfd, VPLSOCKET_INVALID)) {
        return sockfd;
    }

    /* From: http://support.microsoft.com/kb/263823
     * Disable "new behavior" using IOCTL: SIO_UDP_CONNRESET. Without this call
     * recvfrom() can fail, repeatedly, after a bad sendto() call.
     */
#ifndef SIO_UDP_CONNRESET
#define SIO_UDP_CONNRESET       (IOC_IN | IOC_VENDOR | 12)
#endif
    if (type == VPLSOCKET_DGRAM) {
        /* TODO: Need to check window version?
                 Problem introduced in Windows 2000 and fix was added
                 for Windows 2000 SP2 and above. */
        DWORD dwBytesReturned = 0;
        BOOL  bNewBehavior = FALSE;
        DWORD status = WSAIoctl(sockfd.s, SIO_UDP_CONNRESET,
                                &bNewBehavior, sizeof(bNewBehavior),
                                NULL, 0, &dwBytesReturned,NULL,NULL);
        if (status != 0) {
            return VPLSOCKET_INVALID;
        }

    }

    /* Set socket to nonblocking */
    if (nonblock) {
        u_long myNonblock = nonblock;
        ioctlsocket(sockfd.s, FIONBIO, (u_long FAR*) &myNonblock);
    }

    return sockfd;
}

int VPLSocket_Close(VPLSocket_t sockfd)
{
    int rv = VPL_OK;
    int rc;

    if (VPLSocket_Equal(sockfd, VPLSOCKET_INVALID)) {
        return VPL_ERR_BADF;
    }

    shutdown(sockfd.s, SD_BOTH);
    rc = closesocket(sockfd.s);

    if(rc) {
        rv = VPLError_XlatSockErrno(WSAGetLastError());
    }

    return rv;
}

int VPLSocket_Shutdown(VPLSocket_t sockfd, int how)
{
    int rv = VPL_OK;
    int rc;

    if (VPLSocket_Equal(sockfd, VPLSOCKET_INVALID)) {
        return VPL_ERR_BADF;
    }
    rc = shutdown(sockfd.s, how);
    if (rc != 0) {
        rv = VPLError_XlatSockErrno(WSAGetLastError());
    }
    return rv;
}

// Bind a socket to an address and port
int VPLSocket_Bind(VPLSocket_t sockfd, const VPLSocket_addr_t *in_addr, size_t in_addr_size)
{
    int rv = VPL_OK;
    int rc;
    struct sockaddr_in sin;

    if (VPLSocket_Equal(sockfd, VPLSOCKET_INVALID)) {
        return VPL_ERR_BADF;
    }
    if ( in_addr == NULL ) {
        return VPL_ERR_INVALID;
    }

    sin.sin_family = PF_INET;
    sin.sin_port = in_addr->port;
    if (in_addr->addr == VPLNET_ADDR_INVALID) {
        sin.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else {
        sin.sin_addr.s_addr = in_addr->addr;
    }

    rc = bind(sockfd.s, (struct sockaddr*) &sin, sizeof(sin));
    if(rc == SOCKET_ERROR) {
        rv = VPLError_XlatSockErrno(WSAGetLastError());
    }

    return rv;
}

int VPLSocket_SetSockOpt(VPLSocket_t sockfd, int level, int option_name,
                         const void *option_value, unsigned int option_len)
{
    int rv;
    rv = setsockopt( sockfd.s, level, option_name, (const char*)option_value, option_len );
    if ( rv == 0 ) {
        return VPL_OK;
    } else {
        DWORD err = WSAGetLastError();
        return VPLError_XlatSockErrno(err);
    }
}

int VPLSocket_GetSockOpt(VPLSocket_t sockfd, int level, int option_name,
                         void *option_value, unsigned int option_len)
{
    int rv;
    int tmpOptionLen = option_len;
    rv = getsockopt( sockfd.s, level, option_name, (char*)option_value, &tmpOptionLen );
    if ( rv == 0 ) {
        if(option_name == VPLSOCKET__SO_ERROR) {
            int real_value = VPLError_XlatErrno(*((int*)(option_value)));
            *((int*)(option_value)) = real_value;
        }
        return VPL_OK;
    } else {
        DWORD err = WSAGetLastError();
        return VPLError_XlatErrno(err);
    }
}

int
VPLSocket_SetKeepAlive(VPLSocket_t socket, int enable, int waitSec, int intervalSec, int count)
{
    int rv = VPL_OK;
    DWORD dwBytes; // unused dummy value
    UNUSED(count); // retransmissions is always 10 on Vista and later
    struct tcp_keepalive params = { (enable != 0), waitSec * 1000/*ms*/, intervalSec * 1000/*ms*/ };
    if (WSAIoctl(socket.s, SIO_KEEPALIVE_VALS, &params, sizeof(params), NULL, 0, &dwBytes, NULL, NULL) != 0) {
        rv = VPLError_XlatSockErrno(WSAGetLastError());
    }
    return rv;
}

static struct timeval VPLTimeToTimeVal(VPLTime_t vplTime)
{
    struct timeval result;
    VPLTime_t seconds = VPLTime_ToSec(vplTime);
    if (seconds >= LONG_MAX) {
        result.tv_sec = LONG_MAX;
    } else {
        result.tv_sec = (long)(seconds);
    }
    result.tv_usec = (long)VPLTime_ToMicrosec(vplTime - VPLTime_FromSec(seconds));
    return result;
}

static int connect_with_timeouts(VPLSocket_t sockfd, const VPLSocket_addr_t *in_addr, size_t in_addr_size,
                                 VPLTime_t timeout_nonroutable, VPLTime_t timeout_routable)
{
    int rv = VPL_OK;
    int rc;
    struct sockaddr_in sin;
    socklen_t sin_size = sizeof(sin);
    VPLTime_t start = VPLTime_GetTimeStamp();
    VPLTime_t end, timeout_remaining;

    if (VPLSocket_Equal(sockfd, VPLSOCKET_INVALID)) {
        return VPL_ERR_BADF;
    }
    if ( in_addr == NULL ) {
        return VPL_ERR_INVALID;
    }

    memset(&sin, 0, sin_size);
    sin.sin_family = AF_INET;
    sin.sin_port = in_addr->port;
    sin.sin_addr.s_addr = in_addr->addr;

    // For routable addresses, wait "timeout_routable" seconds max for response.
    // For non-routable addresses, wait "timeout_nonroutable" seconds max for response.
    // Non-routable address ranges:
    // * 10.0.0.0 - 10.255.255.255
    // * 172.16.0.0.- 172.31.255.255
    // * 192.168.0.0 - 192.168.255.255
    // * 169.254.0.0 - 169.254.255.255
    if(!VPLNet_IsRoutableAddress(in_addr->addr)) {
        // Non-routable address.
        if(timeout_nonroutable == VPL_TIMEOUT_NONE) {
            end = VPL_TIMEOUT_NONE;
        }
        else {
            end = start + timeout_nonroutable;
        }
    }
    else {
        // Routable address. may take longer.
        if(timeout_routable == VPL_TIMEOUT_NONE) {
            end = VPL_TIMEOUT_NONE;
        }
        else {
            end = start + timeout_routable;
        }
    }

    do {
        rv = VPL_OK;
        timeout_remaining = VPLTime_DiffClamp(end, VPLTime_GetTimeStamp());
        
        // CAUTION: connect() can return WSAEINVAL (which gets mapped to VPL_ERR_INVALID) if a
        // connection is already pending!
        // See https://bugs.ctbg.acer.com/show_bug.cgi?id=14711#c23 for details.
        // From MSDN:
        // "Due to ambiguities in version 1.1 of the Windows Sockets specification, error
        // codes returned from connect while a connection is already pending may vary among
        // implementations. As a result, it is not recommended that applications use
        // multiple calls to connect to detect connection completion. If they do, they must
        // be prepared to handle WSAEINVAL and WSAEWOULDBLOCK error values the same way that
        // they handle WSAEALREADY, to assure robust operation."
        rc = connect(sockfd.s, (struct sockaddr*) &sin, sin_size);
        if(rc == SOCKET_ERROR) {
            DWORD err = WSAGetLastError();

            if(err == WSAEISCONN) {
                break;
            }
            else if (err == WSAEWOULDBLOCK || err == WSAEALREADY) {
                /* For non-blocking socket, need to check for completion */
                fd_set writefds;
                FD_ZERO(&writefds);
                FD_SET(sockfd.s, &writefds);

                if (end == VPL_TIMEOUT_NONE) {
                    // Note that FD_SET and select() on Windows do not have a problem with large-numbered file
                    // descriptors (although there is still a limit of 64 different file descriptors per fd_set).
                    // See https://bugs.ctbg.acer.com/show_bug.cgi?id=14711 for more information.
                    rc = select(0 /*nfds is ignored on Win32*/, NULL, &writefds, NULL, NULL);
                } else {
                    struct timeval tv = VPLTimeToTimeVal(timeout_remaining);
                    rc = select(0 /*nfds is ignored on Win32*/, NULL, &writefds, NULL, &tv);
                }
                
                if(rc == 0) {
                    // select() timed out.
                    VPLTime_t now = VPLTime_GetTimeStamp();
                    if (now < end) {
                        VPL_REPORT_WARN("select() returned early ("FMT_VPLTime_t"us remaining).", end - now);
                    }
                    rv = VPL_ERR_TIMEOUT;
                    break;
                } else if(rc == 1 && FD_ISSET(sockfd.s, &writefds)) {
                    socklen_t optlen = sizeof(err);
                    rc = getsockopt(sockfd.s, SOL_SOCKET, SO_ERROR,
                            (char*) &err, &optlen);
                    if (rc == SOCKET_ERROR) {
                        err = WSAGetLastError();
                        VPL_REPORT_WARN("getsockopt() failed: "FMT_DWORD, err);
                    }
                } else {
                    err = WSAGetLastError();
                }
            }
            else if (err == WSAETIMEDOUT) {
                VPL_REPORT_INFO("connect() timed out ("FMT_VPLTime_t"us elapsed).", VPLTime_GetTimeStamp() - start);
            }
            else {
                VPL_REPORT_INFO("connect() failed: "FMT_DWORD, err);
            }
            
            if (err) {
                rv = VPLError_XlatSockErrno(err);
            }
        }

      // Bug 14375: Retry if connect() returns WSAETIMEDOUT (which makes rv=VPL_ERR_TIMEOUT).
    } while((rv == VPL_ERR_TIMEOUT) && ((end == VPL_TIMEOUT_NONE) || (VPLTime_GetTimeStamp() < end)));

    return rv;
}

int VPLSocket_Connect(VPLSocket_t sockfd, const VPLSocket_addr_t *in_addr, size_t in_addr_size)
{
    return connect_with_timeouts(sockfd, in_addr, in_addr_size, VPLTime_FromSec(1), VPLTime_FromSec(5));
}

int VPLSocket_ConnectWithTimeout(VPLSocket_t sockfd, const VPLSocket_addr_t* in_addr, size_t in_addr_size, VPLTime_t timeout)
{
    return connect_with_timeouts(sockfd, in_addr, in_addr_size, timeout, timeout);
}

int VPLSocket_ConnectWithTimeouts(VPLSocket_t sockfd, const VPLSocket_addr_t* in_addr, size_t in_addr_size, VPLTime_t timeout_nonroutable, VPLTime_t timeout_routable)
{
    return connect_with_timeouts(sockfd, in_addr, in_addr_size, timeout_nonroutable, timeout_routable);
}

int VPLSocket_Listen(VPLSocket_t sockfd, int backlog)
{
    int rv = VPL_OK;
    int rc;

    if (VPLSocket_Equal(sockfd, VPLSOCKET_INVALID)) {
        return VPL_ERR_BADF;
    }

    rc = listen(sockfd.s, backlog);
    if (rc == SOCKET_ERROR) {
        rv = VPLError_XlatSockErrno(WSAGetLastError());
    }

    return rv;
}

int VPLSocket_Accept(VPLSocket_t sockfd,
                             VPLSocket_addr_t *in_addr, size_t in_addr_size,
                             VPLSocket_t *connfd)
{
    struct sockaddr_in cliaddr;
    socklen_t clilen;

    if (connfd == NULL)
        return VPL_ERR_INVALID;

    *connfd = VPLSOCKET_INVALID;
    
    if (VPLSocket_Equal(sockfd, VPLSOCKET_INVALID)) {
        return VPL_ERR_BADF;
    }
    
    memset(&cliaddr, 0, sizeof(cliaddr));
    clilen = sizeof(cliaddr);
    
    connfd->s = accept(sockfd.s, (struct sockaddr*) &cliaddr, &clilen);
    if(connfd->s == SOCKET_ERROR) {
        return VPLError_XlatSockErrno(WSAGetLastError());
    }
    else {
        // connfd automatically inherits properties of sockfd
        if (in_addr && in_addr_size == sizeof(VPLSocket_addr_t)) {
            in_addr->addr = cliaddr.sin_addr.s_addr;
            in_addr->port = cliaddr.sin_port;
        }
    }

    return VPL_OK;
}

// TODO: need to test the alternative first, then remove "|| 1"
#if WINVER < 0x0600 || 1
//% WSAPoll not defined in versions before Vista
// And we may want to avoid it - see http://daniel.haxx.se/blog/2012/10/10/wsapoll-is-broken/
int VPLSocket_Poll(VPLSocket_poll_t *sockets, int num_sockets, VPLTime_t timeout)
{
    int i, rv;
    fd_set readfds, writefds, exceptfds;
    static uint16_t VALID_EVENTS = VPLSOCKET_POLL_RDNORM | VPLSOCKET_POLL_RDPRI
            | VPLSOCKET_POLL_OUT;

    if (sockets == NULL)
        return VPL_ERR_INVALID;

    if(num_sockets > FD_SETSIZE) {
        VPL_REPORT_WARN("num_sockets is %d > %d.", 
                         num_sockets, FD_SETSIZE);
    }

    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_ZERO(&exceptfds);
    for (i = 0; i < num_sockets; i++) {
        if (sockets[i].events & VPLSOCKET_POLL_RDNORM)
            FD_SET(sockets[i].socket.s, &readfds);
        if (sockets[i].events & VPLSOCKET_POLL_RDPRI)
            FD_SET(sockets[i].socket.s, &exceptfds);
        if (sockets[i].events & VPLSOCKET_POLL_OUT)
            FD_SET(sockets[i].socket.s, &writefds);
        sockets[i].revents = 0;
    }

    if (timeout == VPL_TIMEOUT_NONE) {
        // Note that FD_SET and select() on Windows do not have a problem with large-numbered file
        // descriptors (although there is still a limit of 64 different file descriptors per fd_set).
        // See https://bugs.ctbg.acer.com/show_bug.cgi?id=14711 for more information.
        rv = select(0 /*nfds is ignored on Win32*/, &readfds, &writefds, &exceptfds, NULL);
    } else {
        struct timeval tv = VPLTimeToTimeVal(timeout);
        rv = select(0 /*nfds is ignored on Win32*/, &readfds, &writefds, &exceptfds, &tv);
    }
    if (rv == SOCKET_ERROR) {
        DWORD err_code = WSAGetLastError();
        rv = VPLError_XlatSockErrno(err_code);

        return rv;
    }

    // TODO: Detect invalid sockets and return error as VPLSOCKET_POLL_SO_INVAL
    for (i = 0, rv = 0; i < num_sockets; i++) {
        if (sockets[i].events & ~VALID_EVENTS) {
            sockets[i].revents |= VPLSOCKET_POLL_EV_INVAL;
        }
        if (FD_ISSET(sockets[i].socket.s, &readfds)) {
            unsigned long avail;
            // Get number of bytes available
            ioctlsocket(sockets[i].socket.s, FIONREAD, &avail);
            if (avail > 0) {
                sockets[i].revents |= VPLSOCKET_POLL_RDNORM;
            } else {
                DWORD sock_err;
                int sock_err_len = sizeof(sock_err);
                getsockopt(sockets[i].socket.s, SOL_SOCKET, SO_ERROR,
                        (char *)&sock_err, &sock_err_len);
                if (sock_err == 0) {
                    BOOL listening;
                    int listening_len = sizeof(listening);
                    getsockopt(sockets[i].socket.s, SOL_SOCKET, SO_ACCEPTCONN,
                            (char *)&listening, &listening_len);
                    if (listening) {
                        // Pending connection on listening socket
                        sockets[i].revents |= VPLSOCKET_POLL_RDNORM;
                    } else {
                        // Connection closed gracefully by remote peer
                        sockets[i].revents |= VPLSOCKET_POLL_HUP;
                    }
                } else if (sock_err == WSAENETRESET
                        || sock_err == WSAECONNRESET) {
                    // Connection closed forcibly
                    sockets[i].revents |= VPLSOCKET_POLL_ERR | VPLSOCKET_POLL_HUP;
                } else {
                    // Unspecified error
                    sockets[i].revents |= VPLSOCKET_POLL_ERR;
                }
            }
        }
        if (FD_ISSET(sockets[i].socket.s, &writefds)) {
            sockets[i].revents |= VPLSOCKET_POLL_OUT;
        }
        if (FD_ISSET(sockets[i].socket.s, &exceptfds)) {
            unsigned long avail = 0;
            // Determine if OOB data available
            ioctlsocket(sockets[i].socket.s, SIOCATMARK, &avail);
            if (avail) {
                sockets[i].revents |= VPLSOCKET_POLL_RDPRI;
            } else {
                sockets[i].revents |= VPLSOCKET_POLL_ERR;
            }
        }
        if (sockets[i].revents) {
            rv++;
        }
    }
    return rv;
}

#else

#if SHRT_MAX != 32767
#error VPLSocket_Poll assumes 16-bit short
#endif
//% TODO: check sizeof(VPLSocket_t) == sizeof(SOCKET)?

static short VPLSocket_FlagsVplToWinsock(u16 flags)
{
    short out = 0;
    if (flags & VPLSOCKET_POLL_RDNORM)
        out |= POLLRDNORM;
    if (flags & VPLSOCKET_POLL_OUT)
        out |= POLLWRNORM;
    if (flags & VPLSOCKET_POLL_RDPRI)
        out |= POLLRDBAND;
    return out;
}

static u16 VPLSocket_FlagsWinsockToVpl(short flags)
{
    u16 out = 0;
    if (flags & POLLRDNORM)
        out |= VPLSOCKET_POLL_RDNORM;
    if (flags & POLLRDBAND)
        out |= VPLSOCKET_POLL_RDPRI;
    if (flags & POLLWRNORM)
        out |= VPLSOCKET_POLL_OUT;
    if (flags & POLLERR)
        out |= VPLSOCKET_POLL_ERR;
    if (flags & POLLHUP)
        out |= VPLSOCKET_POLL_HUP;
    if (flags & POLLNVAL)
        // Can't be the event flags since they were generated internally
        out |= VPLSOCKET_POLL_SO_INVAL;
    return out;
}

//% Untested!
int VPLSocket_Poll(VPLSocket_poll_t *sockets, int num_sockets, VPLTime_t timeout)
{
    int i, rv;
    int plat_timeout;
    if (timeout == VPL_TIMEOUT_NONE) {
        plat_timeout = -1;
    } else {
        VPLTime_t temp = timeout / 1000;
        if (temp > INT_MAX) {
            plat_timeout = INT_MAX;
        } else {
            plat_timeout = (int)temp;
        }
    }

    if (sockets == NULL)
        return VPL_ERR_INVALID;

    static u16 VALID_FLAGS = VPLSOCKET_POLL_RDNORM | VPLSOCKET_POLL_RDPRI
            | VPLSOCKET_POLL_OUT;

    WSAPOLLFD* fdarray = (WSAPOLLFD*)calloc(num_sockets, sizeof(WSAPOLLFD));
    if (fdarray == NULL) {
        return VPL_ERR_NOMEM;
    }

    for (i = 0; i < num_sockets; i++) {
        fdarray[i].fd = sockets[i].socket.s;
        fdarray[i].events = VPLSocket_FlagsVplToWinsock(sockets[i].events);
    }
    rv = WSAPoll(fdarray, num_sockets, plat_timeout);
    for (i = 0; i < num_sockets; i++) {
        sockets[i].revents = VPLSocket_FlagsWinsockToVpl(fdarray[i].revents);
        if (sockets[i].events & ~VALID_FLAGS) {
            sockets[i].revents |= VPLSOCKET_POLL_EV_INVAL;
        }
    }
    if (rv == SOCKET_ERROR) {
        DWORD err_code = WSAGetLastError();
        rv = VPLError_XlatSockErrno(err_code);
    }

    free(fdarray);

    return rv;
}
#endif  // WINVER

int VPLSocket_Send(VPLSocket_t sockfd, const void* msg, int len)
{
    int rv = VPL_OK;
    int rc;

    if (VPLSocket_Equal(sockfd, VPLSOCKET_INVALID)) {
        return VPL_ERR_BADF;
    }

    rc = send(sockfd.s, (char*)msg, len, 0);
    if (rc == SOCKET_ERROR) {
        DWORD err_code = WSAGetLastError();
        rv = VPLError_XlatSockErrno(err_code);
    }
    else {
        // Return number of bytes sent if no error.
        rv = rc;
    }

    return rv;
}

int VPLSocket_SendTo(VPLSocket_t sockfd, const void* buf, int len,
                     const VPLSocket_addr_t *in_addr, size_t in_addr_size)
{
    int rv = VPL_OK;
    struct sockaddr_in sin;

    if (VPLSocket_Equal(sockfd, VPLSOCKET_INVALID)) {
        return VPL_ERR_BADF;
    }
    if ( in_addr == NULL ) {
        return VPL_ERR_INVALID;
    }

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = in_addr->port;
    sin.sin_addr.s_addr = in_addr->addr;

    rv = sendto(sockfd.s, (char*)buf, (int)len, 0, (struct sockaddr*) &sin,
                sizeof(sin));
    
    if (rv == SOCKET_ERROR) {
        DWORD err_code = WSAGetLastError();
        rv = VPLError_XlatSockErrno(err_code);
    }

    return rv;
}

int VPLSocket_Recv(VPLSocket_t sockfd, void* buf, int len) {
    int rv = VPL_OK;

    if (VPLSocket_Equal(sockfd, VPLSOCKET_INVALID)) {
        return VPL_ERR_BADF;
    }

    rv = recv(sockfd.s, (char*)buf, len, 0);

    if (rv == SOCKET_ERROR) {
        DWORD err_code = WSAGetLastError();
        rv = VPLError_XlatSockErrno(err_code);
    }

    return rv;
}

int VPLSocket_RecvFrom(VPLSocket_t sockfd, void* buf, int len,
                       VPLSocket_addr_t *in_addr, size_t in_addr_size)
{
    int rv = VPL_OK;
    struct sockaddr_in sin;
    socklen_t size = sizeof(sin);

    if (VPLSocket_Equal(sockfd, VPLSOCKET_INVALID)) {
        return VPL_ERR_BADF;
    }

    memset(&sin, 0, sizeof(sin));
    rv = recvfrom(sockfd.s, (char*)buf, len, 0, (struct sockaddr*) &sin, &size);

    if (rv == SOCKET_ERROR) {
        DWORD err_code = WSAGetLastError();
        rv = VPLError_XlatSockErrno(err_code);
    }
    else {
        if (in_addr && in_addr_size == sizeof(VPLSocket_addr_t)) {
            in_addr->addr = sin.sin_addr.s_addr;
            in_addr->port = sin.sin_port;
        }
    }

    return rv;
}

VPLNet_port_t VPLSocket_GetPort(VPLSocket_t sockfd) 
{
    socklen_t socklen;
    int rc;
    struct sockaddr_in sin;

    memset(&sin, 0, sizeof(sin));
    socklen = sizeof(sin);

    rc = getsockname(sockfd.s, (struct sockaddr*) &sin, &socklen);
    if (rc == SOCKET_ERROR) {
        return VPLNET_PORT_INVALID;
    }

    return sin.sin_port;
}

VPLNet_addr_t VPLSocket_GetAddr(VPLSocket_t sockfd) 
{
    socklen_t socklen;
    int rc;
    struct sockaddr_in sin;

    memset(&sin, 0, sizeof(sin));
    socklen = sizeof(sin);

    rc = getsockname(sockfd.s, (struct sockaddr*) &sin, &socklen);
    if (rc == SOCKET_ERROR) {
        return VPLNET_ADDR_INVALID;
    }

    return sin.sin_addr.s_addr;
}

VPLNet_port_t VPLSocket_GetPeerPort(VPLSocket_t sockfd) 
{
    socklen_t socklen;
    int rc;
    struct sockaddr_in sin;

    memset(&sin, 0, sizeof(sin));
    socklen = sizeof(sin);

    rc = getpeername(sockfd.s, (struct sockaddr*) &sin, &socklen);
    if (rc == SOCKET_ERROR) {
        return VPLNET_PORT_INVALID;
    }

    return sin.sin_port;
}

VPLNet_addr_t VPLSocket_GetPeerAddr(VPLSocket_t sockfd) 
{
    socklen_t socklen;
    int rc;
    struct sockaddr_in sin;

    memset(&sin, 0, sizeof(sin));
    socklen = sizeof(sin);

    rc = getpeername(sockfd.s, (struct sockaddr*) &sin, &socklen);
    if (rc == SOCKET_ERROR) {
        return VPLNET_ADDR_INVALID;
    }

    return sin.sin_addr.s_addr;
}
