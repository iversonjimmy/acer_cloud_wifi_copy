//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#ifdef IOS

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <poll.h>
#include <limits.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include "vpl_socket.h"
#include "vplu.h"
#include "vplu_types.h"
#include "vplu_debug.h"
#include "vpl_time.h"

#else

#include "vpl_socket.h"

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <poll.h>
#include <limits.h>
#include <sys/ioctl.h>

#include <linux/sockios.h>

#include "vplu.h"
#include "vplu_types.h"
#include "vplu_debug.h"
#include "vpl_time.h"

#endif

int 
VPLSocket_Init(void) {
    return VPL_OK;
}

int 
VPLSocket_Quit(void) {
    return VPL_OK;
}
 
// open a socket of a given type.
VPLSocket_t 
VPLSocket_Create(int family, int type, int nonblock)
{
    VPLSocket_t sockfd;
#ifdef IOS
    int opt_value = 1;
#endif
    sockfd.fd = socket(family, type, 0);
    if (sockfd.fd == -1) {
        goto fail;
    }

    /* Set socket to nonblocking */
    if (nonblock != 0) {
        int status;
        int oldopts;
        oldopts = fcntl(sockfd.fd, F_GETFL, 0);
        if (oldopts == -1) {
            // ??
            // fcntl failed, socket is dead. Avoid leaking open socket.
            (void) close(sockfd.fd);
            goto fail;
        }
        status = fcntl(sockfd.fd, F_SETFL, oldopts | O_NONBLOCK);
        if (status == -1) {
            (void)close(sockfd.fd);
            goto fail;
        }
    }

#ifdef IOS
    setsockopt(sockfd.fd, SOL_SOCKET, SO_NOSIGPIPE, (void *)&opt_value, sizeof(int));
#endif

    return sockfd;

 fail:
    return VPLSOCKET_INVALID;
}

int 
VPLSocket_Close(VPLSocket_t sockfd)
{
    int rv = VPL_OK;
    int rc;
    int value_in, value_out;

    if ( VPLSocket_Equal(sockfd, VPLSOCKET_INVALID) ) {
        return VPL_ERR_BADF;
    }

#ifdef IOS
    /*
     * Refers to: http://cf.ccmr.cornell.edu/cgi-bin/w3mman2html.cgi?tcp(7)
     *
     * FIONREAD or TIOCINQ
     *     Returns the amount of queued  unread  data  in  the
     *     receive  buffer.  Argument is a pointer to an integer.
     *
     * TIOCOUTQ
     *     Returns  the  amount  of  unsent data in the socket
     *     send queue in the  passed  integer  value  pointer.
     *     Unfortunately,  the implementation of this ioctl is
     *     buggy in all known versions of  Linux  and  instead
     *     returns  the  free  space  (effectively buffer size
     *     minus bytes used including metadata)  in  the  send
     *     queue. This will be fixed in future Linux versions.
     *     If you use TIOCOUTQ, please include a runtime  test
     *     for  both  behaviors for correct function on future
     *     releases and other Unixes.
     */
    rc = ioctl(sockfd.fd, FIONREAD, &value_in);
    if(rc != 0) { value_in = 0; } // ignore value if there's an error.
    rc = ioctl(sockfd.fd, TIOCOUTQ, &value_out);
    if(rc != 0) { value_out = 0; } // ignore value if there's an error.
#else
    // Check the incoming and outgoing queue sizes. Report if nonzero.
    // Sockets opened to listen will return EINVAL on these calls.
    rc = ioctl(sockfd.fd, SIOCINQ, &value_in);
    if(rc != 0) { value_in = 0; } // ignore value if there's an error.
    rc = ioctl(sockfd.fd, SIOCOUTQ, &value_out);
    if(rc != 0) { value_out = 0; } // ignore value if there's an error.
#endif

    if(value_in || value_out) { // log only if queues not both empty
        VPL_REPORT_INFO("Closing socket %d with %d bytes pending send and %d bytes pending receive.",
                        sockfd.fd, value_out, value_in);
    }

    shutdown(sockfd.fd, SHUT_RDWR);

    rc = close(sockfd.fd);
    if(rc) {
        int err_code = errno;
        rv = VPLError_XlatErrno(err_code);
    }

    return rv;
}

int 
VPLSocket_Shutdown(VPLSocket_t sockfd, int how)
{
    int rv = VPL_OK;
    int rc;

    if ( VPLSocket_Equal(sockfd, VPLSOCKET_INVALID) ) {
        return VPL_ERR_BADF;
    }

    rc = shutdown(sockfd.fd, how);
    if (rc != 0) {
	int err = errno;
        rv = VPLError_XlatErrno(err);
    }
    return rv;
}

// Bind a socket to an address and port
int 
VPLSocket_Bind(VPLSocket_t sockfd, const VPLSocket_addr_t* addr, size_t addrSize)
{
    int rv = VPL_OK;
    struct sockaddr_in sin;
    struct sockaddr sin_tmp;

    if (VPLSocket_Equal(sockfd, VPLSOCKET_INVALID)) {
        return VPL_ERR_BADF;
    }
    if (addr == NULL) {
        return VPL_ERR_INVALID;
    }
    UNUSED(addrSize);

    sin.sin_family = PF_INET;
    sin.sin_port = addr->port;
#ifdef IOS
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
#else
    if (addr->addr == VPLNET_ADDR_INVALID) {
        sin.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else {
        sin.sin_addr.s_addr = addr->addr;
    }
#endif
    memcpy(&sin_tmp, &sin, sizeof(sin));
    rv = bind(sockfd.fd, &sin_tmp, sizeof(sin));

    if (rv == -1) {
        int const err = errno;
        rv = VPLError_XlatErrno(err);
    }

    return rv;
}

int 
VPLSocket_SetSockOpt(VPLSocket_t sockfd, int level, int optionName,
        const void* optionValue, unsigned int optionLen)
{
    int rv;
    rv = setsockopt(sockfd.fd, level, optionName, (const char*)optionValue, optionLen);
    if (rv == 0) {
        return VPL_OK;
    }
    else {
        int const err = errno;
        return VPLError_XlatErrno(err);
    }
}

int 
VPLSocket_GetSockOpt(VPLSocket_t sockfd, int level, int optionName,
        void *optionValue, unsigned int optionLen)
{
    socklen_t nativeOptionLen = optionLen;
    int rv;
    rv = getsockopt(sockfd.fd, level, optionName, optionValue, &nativeOptionLen);
    if (rv == 0) {
        if(optionName == VPLSOCKET__SO_ERROR) {
            int realValue = VPLError_XlatErrno(*((int*)(optionValue)));
            *((int*)(optionValue)) = realValue;
        }
        return VPL_OK;
    }
    else {
        int const err = errno;
        return VPLError_XlatErrno(err);
    }
}

#ifdef IOS

int
VPLSocket_SetKeepAlive(VPLSocket_t sockfd, int enable, int wait, int interval, int count)
{
    return -1; //TODO: Porting for iOS.
}

int
VPLSocket_GetKeepAlive(VPLSocket_t sockfd, int* enable, int* wait, 
                       int* interval, int* count)
{
    return -1; //TODO: Porting for iOS.
}

#else

int
VPLSocket_SetKeepAlive(VPLSocket_t sockfd, int enable, int wait, int interval, int count)
{
    int rv;
    rv = setsockopt(sockfd.fd, SOL_SOCKET, SO_KEEPALIVE, &enable, sizeof(enable));
    if (rv != 0) {
        int const err = errno;
        return VPLError_XlatErrno(err);
    }

    if (!enable) {
        return VPL_OK;
    }

    rv = setsockopt(sockfd.fd, SOL_TCP, TCP_KEEPIDLE, &wait, sizeof(wait));
    if (rv != 0) {
        int const err = errno;
        return VPLError_XlatErrno(err);
    }

    rv = setsockopt(sockfd.fd, SOL_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));
    if (rv != 0) {
        int const err = errno;
        return VPLError_XlatErrno(err);
    }

    rv = setsockopt(sockfd.fd, SOL_TCP, TCP_KEEPCNT, &count, sizeof(count));
    if (rv != 0) {
        int const err = errno;
        return VPLError_XlatErrno(err);
    }

    return 0;
}

int
VPLSocket_GetKeepAlive(VPLSocket_t sockfd, int* enable, int* wait, 
                       int* interval, int* count)
{
    int rv;
    socklen_t nativeOptionLen = sizeof(enable);
    rv = getsockopt(sockfd.fd, SOL_SOCKET, SO_KEEPALIVE, (void*)enable, &nativeOptionLen);
    if (rv != 0) {
        int const err = errno;
        return VPLError_XlatErrno(err);
    }

    nativeOptionLen = sizeof(wait);
    rv = getsockopt(sockfd.fd, SOL_TCP, TCP_KEEPIDLE, (void*)wait, &nativeOptionLen);
    if (rv != 0) {
        int const err = errno;
        return VPLError_XlatErrno(err);
    }

    nativeOptionLen = sizeof(interval);
    rv = getsockopt(sockfd.fd, SOL_TCP, TCP_KEEPINTVL, (void*)interval, &nativeOptionLen);
    if (rv != 0) {
        int const err = errno;
        return VPLError_XlatErrno(err);
    }

    nativeOptionLen = sizeof(count);
    rv = getsockopt(sockfd.fd, SOL_TCP, TCP_KEEPCNT, (void*)count, &nativeOptionLen);
    if (rv != 0) {
        int const err = errno;
        return VPLError_XlatErrno(err);
    }

    return 0;
}

#endif

static int 
connect_with_timeouts(VPLSocket_t sockfd, const VPLSocket_addr_t* addr, size_t addrSize,
                      VPLTime_t timeout_nonroutable, VPLTime_t timeout_routable)
{
    int rv = VPL_OK;
    int rc;
    struct sockaddr_in sin;
    struct sockaddr sa;
    socklen_t sin_size = sizeof(sin);
    int err;
    VPLTime_t start = VPLTime_GetTimeStamp();
    VPLTime_t end, timeout_remaining;

    if (VPLSocket_Equal(sockfd, VPLSOCKET_INVALID)) {
        return VPL_ERR_BADF;
    }
    if (addr == NULL) {
        return VPL_ERR_INVALID;
    }
    UNUSED(addrSize);
    
    memset(&sin, 0, sin_size);
    sin.sin_family = AF_INET;
    sin.sin_port = addr->port;
    sin.sin_addr.s_addr = addr->addr;
    memcpy(&sa, &sin, sin_size);

    // For routable addresses, wait 5 seconds max for response.
    // For non-routable addresses, wait 1 second max for response.
    // Non-routable address ranges:
    // * 10.0.0.0 - 10.255.255.255
    // * 172.16.0.0.- 172.31.255.255
    // * 192.168.0.0 - 192.168.255.255
    // * 169.254.0.0 - 169.254.255.255
    if(!VPLNet_IsRoutableAddress(addr->addr)) {
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

        rc = connect(sockfd.fd, (struct sockaddr*) &sa, sin_size);
        if(rc != 0) {
            err = errno;
            if(err == EISCONN) {
                break;
            }
            else if (err == EINPROGRESS || err == EALREADY) {
                // For non-blocking socket, need to check for completion
                struct pollfd sockets[1];
                socklen_t optlen = sizeof(err);
                int timeout;
                
                sockets[0].fd = sockfd.fd;
                sockets[0].events = POLLOUT;
                sockets[0].revents = 0;
                
                if(end == VPL_TIMEOUT_NONE) {
                    timeout = -1;
                }
                else {
                    timeout = VPLTime_ToMillisec(timeout_remaining);
                }

                rc = poll(sockets, 1, timeout);
                if(rc == 0) {
                    err = ETIMEDOUT;
                }
                else if(rc == 1 && (sockets[0].revents & POLLOUT)) {
                    rc = getsockopt(sockfd.fd, SOL_SOCKET, SO_ERROR,
                                    (void*) &err, &optlen);
                    if (rc != 0) {
                        err = errno;
                    }
                }
                else {
                    err = errno;
                }
            }
            
            if (err) {        
                rv = VPLError_XlatErrno(err);
            }
        }
    } while((rv == VPL_ERR_TIMEOUT) && ((end == VPL_TIMEOUT_NONE) || (VPLTime_GetTimeStamp() < end)));
    
    return rv;
}

int 
VPLSocket_Connect(VPLSocket_t sockfd, const VPLSocket_addr_t* addr, size_t addrSize)
{
    return connect_with_timeouts(sockfd, addr, addrSize, VPLTime_FromSec(1), VPLTime_FromSec(5));
}

int 
VPLSocket_ConnectWithTimeout(VPLSocket_t sockfd, const VPLSocket_addr_t* addr, size_t addrSize, VPLTime_t timeout)
{
    return connect_with_timeouts(sockfd, addr, addrSize, timeout, timeout);
}

int 
VPLSocket_ConnectWithTimeouts(VPLSocket_t sockfd, const VPLSocket_addr_t* addr, size_t addrSize, VPLTime_t timeout_nonroutable, VPLTime_t timeout_routable)
{
    return connect_with_timeouts(sockfd, addr, addrSize, timeout_nonroutable, timeout_routable);
}

int 
VPLSocket_Listen(VPLSocket_t sockfd, int backlog)
{
    int rv = VPL_OK;
    int rc;

    if (VPLSocket_Equal(sockfd, VPLSOCKET_INVALID)) {
        return VPL_ERR_BADF;
    }

    rc = listen(sockfd.fd, backlog);
    if (rc == -1) {
        int err = errno;
        rv = VPLError_XlatErrno(err);
    }

    return rv;
}

int 
VPLSocket_Accept(VPLSocket_t sockfd, VPLSocket_addr_t* addr, size_t addrSize,
        VPLSocket_t* connfd)
{
#ifdef IOS
    // Magically using VPLSocket_GetAddr to avoid VPL_ERR_BADF error when calling VPLSocket_Accept on an iPhone 4s device
    // Also log the returned value for observation
    VPL_REPORT_INFO("VPLSocket_Accept Address: %d.", VPLSocket_GetAddr(sockfd));
    VPL_REPORT_INFO("VPLSocket_Accept Port: %d.", VPLSocket_GetPort(sockfd));
#endif
    
    struct sockaddr_in cliaddr;
    socklen_t clilen;
    struct sockaddr sa;
    int oldopts;
   
    if (connfd == NULL) {
        return VPL_ERR_INVALID;
    } 
    if (VPLSocket_Equal(sockfd, VPLSOCKET_INVALID)) {
        return VPL_ERR_BADF;
    }

    memset(&cliaddr, 0, sizeof(cliaddr));
    clilen = sizeof(cliaddr);
    
    connfd->fd = accept(sockfd.fd, (struct sockaddr*) &sa, &clilen);
    if (connfd->fd == -1) {
        return VPLError_XlatErrno(errno);
    }

    /// Inherit nonblocking property from original socket
    oldopts = fcntl(sockfd.fd, F_GETFL, 0);
    if(oldopts & O_NONBLOCK) {
        oldopts = fcntl(connfd->fd, F_GETFL, 0);
        fcntl(connfd->fd, F_SETFL, oldopts | O_NONBLOCK);
    }
    else {
        oldopts = fcntl(connfd->fd, F_GETFL, 0);
        fcntl(connfd->fd, F_SETFL, oldopts & ~O_NONBLOCK);
    }
    
    memcpy(&cliaddr, &sa, clilen);

    if (addr && addrSize == sizeof(VPLSocket_addr_t)) {
        addr->family = VPL_PF_INET;
        addr->addr = cliaddr.sin_addr.s_addr;
        addr->port = cliaddr.sin_port;
    }

    return VPL_OK;
}

#if SHRT_MAX != 32767
#error "VPLSocket_Poll assumes 16-bit short"
#endif
//% TODO: check sizeof(VPLSocket_t) == sizeof(int)?

static short 
VPLSocket_FlagsVplToGvm(u16 flags)
{
    short out = 0;
    if (flags & VPLSOCKET_POLL_RDNORM)
        out |= POLLIN;
    if (flags & VPLSOCKET_POLL_RDPRI)
        out |= POLLPRI;
    if (flags & VPLSOCKET_POLL_OUT)
        out |= POLLOUT;
    return out;
}

static u16 
VPLSocket_FlagsGvmToVpl(short flags)
{
    short out = 0;
    if (flags & POLLIN)
        out |= VPLSOCKET_POLL_RDNORM;
    if (flags & POLLPRI)
        out |= VPLSOCKET_POLL_RDPRI;
    if (flags & POLLOUT)
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

int 
VPLSocket_Poll(VPLSocket_poll_t* sockets, int numSockets, VPLTime_t timeout)
{
    int i, rv;
    int plat_timeout;
    if (timeout == VPL_TIMEOUT_NONE) {
        plat_timeout = -1;
    } else {
        VPLTime_t timeout_ms = timeout / 1000;
        if (timeout_ms > INT_MAX) {
            plat_timeout = INT_MAX;
        } else {
            plat_timeout = (int)timeout_ms;
        }
    }
    static u16 VALID_FLAGS = VPLSOCKET_POLL_RDNORM | VPLSOCKET_POLL_RDPRI
            | VPLSOCKET_POLL_OUT;

    if (sockets == NULL) {
        return VPL_ERR_INVALID;
    }

    for (i = 0; i < numSockets; i++) {
        sockets[i].events = VPLSocket_FlagsVplToGvm(sockets[i].events);
        sockets[i].revents = 0;
    }
    rv = poll((struct pollfd *)sockets, numSockets, plat_timeout);
    for (i = 0; i < numSockets; i++) {
        sockets[i].events = VPLSocket_FlagsGvmToVpl(sockets[i].events);
        sockets[i].revents = VPLSocket_FlagsGvmToVpl(sockets[i].revents);
        if (sockets[i].events & ~VALID_FLAGS) {
            sockets[i].revents |= VPLSOCKET_POLL_EV_INVAL;
        }
    }
    if (rv < 0) {
        rv = VPLError_XlatErrno(errno);
    }
    return rv;
}

int 
VPLSocket_Send(VPLSocket_t sockfd, const void* msg, int len)
{
    int rv = VPL_OK;
    int rc;

    if (VPLSocket_Equal(sockfd, VPLSOCKET_INVALID) ) {
        return VPL_ERR_BADF;
    }
#ifdef IOS
    rc = send(sockfd.fd, ((const char*)msg), len, 0);
#else
    // Use MSG_NOSIGNAL to prevent SIGPIPE if the socket was shut down.
    rc = send(sockfd.fd, ((const char*)msg), len, MSG_NOSIGNAL);
#endif
    if (rc == -1) {
        int err_code = errno;
        rv = VPLError_XlatErrno(err_code);
    }
    else {
        // Return number of bytes sent if no error.
        rv = rc;
    }

    return rv;
}

int 
VPLSocket_SendTo(VPLSocket_t sockfd, const void* buf, int len,
        const VPLSocket_addr_t* addr, size_t addrSize)
{
    int rv = VPL_OK;
    struct sockaddr_in sin;
    struct sockaddr sa;

    if (VPLSocket_Equal(sockfd, VPLSOCKET_INVALID)) {
        return VPL_ERR_BADF;
    }
    if (addr == NULL) {
        return VPL_ERR_INVALID;
    }
    UNUSED(addrSize);
    
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = addr->port;
    sin.sin_addr.s_addr = addr->addr;
    memcpy(&sa, &sin, sizeof(sin));

#ifdef IOS
    rv = sendto(sockfd.fd, buf, len, 0,
                (struct sockaddr*) &sa, sizeof(sin));
#else
    // Use MSG_NOSIGNAL to prevent SIGPIPE if the socket was shut down.
    rv = sendto(sockfd.fd, buf, len, MSG_NOSIGNAL,
                (struct sockaddr*) &sa, sizeof(sin));
#endif

    if (rv == -1) {
        int err_code = errno;
        rv = VPLError_XlatErrno(err_code);
    }

    return rv;
}

int 
VPLSocket_Recv(VPLSocket_t sockfd, void* buf, int len) 
{
    int rv = VPL_OK;

    if (VPLSocket_Equal(sockfd, VPLSOCKET_INVALID)) {
        return VPL_ERR_BADF;
    }

    rv = recv(sockfd.fd, buf, len, 0);

    if (rv == -1) {
        int err_code = errno;
        rv = VPLError_XlatErrno(err_code);
    }
#ifdef IOS
    // Special case for iOS: When buf is null, recv()/recvfrom() will not return error.
    // EAGAIN doesn't make much sense to me, but that's what Linux does.
    else if (buf == NULL) {
        return VPL_ERR_AGAIN;
    }
#endif
    return rv;
}

int 
VPLSocket_RecvFrom(VPLSocket_t sockfd, void* buf, int len,
        VPLSocket_addr_t* senderAddr, size_t senderAddrSize)
{
    int rv = VPL_OK;
    struct sockaddr_in sin;
    struct sockaddr sa;
    socklen_t size = sizeof(sin);
    char addr[VPLNET_ADDRSTRLEN];

    if ( VPLSocket_Equal(sockfd, VPLSOCKET_INVALID) ) {
        return VPL_ERR_BADF;
    }

    memset(&sa, 0, sizeof(sa));

    rv = recvfrom(sockfd.fd, buf, len, 0, (struct sockaddr*) &sa, &size);
    memcpy(&sin, &sa, size);

    if (rv == -1) {
        int err_code = errno;
        rv = VPLError_XlatErrno(err_code);
    }
#ifdef IOS
    // Special case for iOS: When buf is null, recv()/recvfrom() will not return error.
    // EAGAIN doesn't make much sense to me, but that's what Linux does.
    else if (buf == NULL) {
        return VPL_ERR_AGAIN;
    }
#endif
    else {
        VPLNet_addr_t temp_ipaddr = (VPLNet_addr_t) sin.sin_addr.s_addr;

        VPLNet_Ntop(&temp_ipaddr, addr, sizeof(addr));
        
        if (senderAddr != NULL && senderAddrSize == sizeof(VPLSocket_addr_t)) {
            senderAddr->family = VPL_PF_INET;
            senderAddr->addr = sin.sin_addr.s_addr;
            senderAddr->port = sin.sin_port;
        }
    }

    return rv;
}

VPLNet_port_t 
VPLSocket_GetPort(VPLSocket_t sockfd) 
{
    struct sockaddr sa;
    socklen_t socklen;
    int rc;
    struct sockaddr_in sin;

    memset(&sa, 0, sizeof(sa));
    socklen = sizeof(sa);

    rc = getsockname(sockfd.fd, (struct sockaddr*) &sa, &socklen);
    if (rc == -1) {
        return VPLNET_PORT_INVALID;
    }

    memcpy(&sin, &sa, socklen);
    return sin.sin_port;
}

VPLNet_addr_t 
VPLSocket_GetAddr(VPLSocket_t sockfd) 
{
    struct sockaddr sa;
    socklen_t socklen;
    int rc;
    struct sockaddr_in sin;

    memset(&sa, 0, sizeof(sa));
    socklen = sizeof(sa);

    rc = getsockname(sockfd.fd, (struct sockaddr*) &sa, &socklen);
    if (rc == -1) {
        return VPLNET_ADDR_INVALID;
    }

    memcpy(&sin, &sa, socklen);
    return sin.sin_addr.s_addr;
}

VPLNet_addr_t
VPLSocket_GetPeerAddr(VPLSocket_t socket)
{
    struct sockaddr *   name;
    struct sockaddr_in  tcp_name;
    socklen_t           length;

    char  buffer[256];
    int   error;

    name   = (struct sockaddr *) buffer;
    length = sizeof(buffer);
    error  = getpeername(socket.fd, name, &length);

    if (error) {
        return VPLNET_ADDR_INVALID;
    }

    memcpy(&tcp_name, buffer, sizeof(tcp_name));
    return tcp_name.sin_addr.s_addr;
}

VPLNet_port_t
VPLSocket_GetPeerPort(VPLSocket_t socket)
{
    struct sockaddr *   name;
    struct sockaddr_in  tcp_name;
    socklen_t           length;

    char  buffer[256];
    int   error;

    name   = (struct sockaddr *) buffer;
    length = sizeof(buffer);
    error  = getpeername(socket.fd, name, &length);

    if (error) {
        return VPLNET_PORT_INVALID;
    }

    memcpy(&tcp_name, buffer, sizeof(tcp_name));
    return tcp_name.sin_port;
}
