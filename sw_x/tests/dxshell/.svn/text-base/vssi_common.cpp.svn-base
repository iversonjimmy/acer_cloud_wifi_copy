//
//  Copyright (C) 2009, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

/// BroadOn Virtual Storage Unit Test - client side libbvs poll loop
#include "vssi_common.hpp"

#include "vpl_th.h"
#include "vpl_socket.h"
#include "vplex_trace.h"

#include "vssi.h"
#include "vssi_error.h"

#include <errno.h>
#include <fcntl.h>

static VPLThread_t select_thread;
static int select_thread_should_run = 1;
static bool thread_created = false;

int do_vssi_setup(u64 deviceId)
{
    int rv = 0;

    rv = vssi_select_thread_init();
    if(rv != 0) {
    VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Initialization of libvssi select thread.");
    goto fail;
    }
    rv = VPLThread_Create(&select_thread,
                          vssi_select_thread,
                          &select_thread_should_run,
                          0, // default VPLThread thread-attributes: priority, stack-size, etc.
                          "libvssi_select_thread");
    if (rv != VPL_OK) {
    VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                     "creating libvssi_select_thread failed!");
    goto fail;
    }
    thread_created = true;

    if(VSSI_Init(deviceId, wakeup_vssi_select_thread) != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                    "VSSI init failed!");
        rv++;
        goto fail;
    }

 fail:
    return rv;
}

void do_vssi_cleanup(void)
{
    VPLThread_return_t dontcare;
   
    select_thread_should_run = 0;
    if(thread_created) {
        wakeup_vssi_select_thread();
        VPLThread_Join(&select_thread, &dontcare);
    }

    VSSI_Cleanup();
}

static int pipefds[2] = { -1, -1 };
#define MAX_BVS_SOCKETS 10

struct fdset_info{
    fd_set send_fdset;
    fd_set recv_fdset;
#if defined(WIN32)
    SOCKET maxfd;
#else
    int maxfd;
#endif
};

/// Helper function to build fdset from libbvs.
static void vstest_add_vplsock(VPLSocket_t vpl_sock, int recv, int send, void* ctx)
{
    struct fdset_info* fdset_info_p = (struct fdset_info*)(ctx);

    if(recv) {
#if defined(WIN32)
        FD_SET(vpl_sock.s, &fdset_info_p->recv_fdset);
        if(fdset_info_p->maxfd < vpl_sock.s) {
            fdset_info_p->maxfd = vpl_sock.s;
        }
    }
    if(send) {
        FD_SET(vpl_sock.s, &fdset_info_p->send_fdset);
        if(fdset_info_p->maxfd < vpl_sock.s) {
            fdset_info_p->maxfd = vpl_sock.s;
        }
    }
    VPLTRACE_LOG_FINEST(TRACE_APP, 0,
                        "Added socket %d for %s.\n",
                        vpl_sock.s,
                        (recv && send) ? "send and receive" :
                        (recv) ? "receive only" :
                        (send) ? "send only" :
                        "neither send not receive");
#else
        FD_SET(vpl_sock.fd, &fdset_info_p->recv_fdset);
        if(fdset_info_p->maxfd < vpl_sock.fd) {
            fdset_info_p->maxfd = vpl_sock.fd;
        }
    }
    if(send) {
        FD_SET(vpl_sock.fd, &fdset_info_p->send_fdset);
        if(fdset_info_p->maxfd < vpl_sock.fd) {
            fdset_info_p->maxfd = vpl_sock.fd;
        }
    }
    VPLTRACE_LOG_FINEST(TRACE_APP, 0,
                        "Added socket %d for %s.",
                        vpl_sock.fd,
                        (recv && send) ? "send and receive" :
                        (recv) ? "receive only" :
                        (send) ? "send only" :
                        "neither send not receive");
#endif
}

/// Helper functions so libbvs can determine active sockets
static int vstest_vplsock_recv_ready(VPLSocket_t vpl_sock, void* ctx)
{
    int rv;
    struct fdset_info* fdset_info_p = (struct fdset_info*)(ctx);

#if defined(WIN32)
    rv = FD_ISSET(vpl_sock.s, &(fdset_info_p->recv_fdset));
    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "Socket %d %s for receive.", 
                        vpl_sock.s, rv ? "ready" : "not ready");
#else
    rv = FD_ISSET(vpl_sock.fd, &(fdset_info_p->recv_fdset));
    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "Socket %d %s for receive.", 
                        vpl_sock.fd, rv ? "ready" : "not ready");
#endif
    return rv;
}
static int vstest_vplsock_send_ready(VPLSocket_t vpl_sock, void* ctx)
{
    int rv;
    struct fdset_info* fdset_info_p = (struct fdset_info*)(ctx);

#if defined(WIN32)
    rv = FD_ISSET(vpl_sock.s, &(fdset_info_p->send_fdset));
    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "Socket %d %s for send.",
                        vpl_sock.s, rv ? "ready" : "not ready");    
#else
    rv = FD_ISSET(vpl_sock.fd, &(fdset_info_p->send_fdset));
    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "Socket %d %s for send.",
                        vpl_sock.fd, rv ? "ready" : "not ready");    
#endif
    return rv;
}

int vssi_select_thread_init(void)
{
    int i;
    int rc = pipe(pipefds);
    int rv = -1;

    if(rc != 0) {
        int err = errno;
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "FAIL:"
                         "Failed to create notify pipe: (%d)",
                         err);
        goto exit;
    }
#if defined(WIN32)
    for(i = 0; i < 2; i++) {
        // mode, nonzero value means nonblocking mode
        u_long mode = 1;
        rc = ioctlsocket(pipefds[i], FIONBIO, &mode);
        if(rc == SOCKET_ERROR) {
            int err = WSAGetLastError();
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "FAILED: "
                             "Failed to set nonblocking flag for end %d of pipe: %d\n",
                             i, err);
            goto close_pipe;
        }
    }
#else
    for(i = 0; i < 2; i++) {
        int flags = fcntl(pipefds[i], F_GETFL);
        if (flags == -1) {
            int err = errno;
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "FAIL:"
                             "Failed to get flags for end %d of pipe: (%d)",
                             i, err);
            goto close_pipe;
        }
        rc = fcntl(pipefds[i], F_SETFL, flags | O_NONBLOCK);
        if(rc == -1) {
            int err = errno;
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "FAIL:"
                             "Failed to set nonblocking flag for end %d of pipe: (%d)",
                             i, err);
            goto close_pipe;  
        }
    }
#endif
    rv = 0;
    goto exit;
 close_pipe:
    close(pipefds[0]);
    close(pipefds[1]);
 exit:
    return rv;
}

VPLThread_return_t vssi_select_thread(VPLThread_arg_t arg)
{
    fdset_info master_fdset_info;
    fdset_info response_fdset_info;
    int *e = (int *)arg;

    FD_ZERO(&master_fdset_info.recv_fdset);
    FD_ZERO(&master_fdset_info.send_fdset);
    master_fdset_info.maxfd = 0;
    FD_ZERO(&response_fdset_info.recv_fdset);
    FD_ZERO(&response_fdset_info.send_fdset);

    FD_SET(pipefds[0], &master_fdset_info.recv_fdset);
    master_fdset_info.maxfd = pipefds[0];

    while (*e) {
        int rv;
        char cmd = '\0';
        char wake_up = 0;
        int err = 0;
        int rc;

        // Determine how long to wait.
        struct timeval timeout_val;
        VPLTime_t timeout_us = VSSI_HandleSocketsTimeout();
        if(timeout_us != VPL_TIMEOUT_NONE) {
            timeout_val.tv_sec = (long)VPLTime_ToSec(timeout_us);
            timeout_val.tv_usec = (long)VPLTime_ToMicrosec(timeout_us - VPLTime_FromSec(timeout_val.tv_sec));
        }

        VPLTRACE_LOG_FINEST(TRACE_APP, 0,
                            "Waiting for activity "FMT_VPLTime_t"us...",
                            timeout_us);

        response_fdset_info.recv_fdset = master_fdset_info.recv_fdset;
        response_fdset_info.send_fdset = master_fdset_info.send_fdset;
        rv = select(master_fdset_info.maxfd + 1,
                    &(response_fdset_info.recv_fdset),
                    &(response_fdset_info.send_fdset),
                    NULL, 
                    timeout_us == VPL_TIMEOUT_NONE ? NULL : &timeout_val);
        if (rv < 0) {
#if defined(WIN32)
            err = WSAGetLastError();
            if(err == WSAENOTSOCK) {
#else
            if(errno == EBADF) {
#endif
                VPLTRACE_LOG_FINE(TRACE_APP, 0,
                                  "Bad file descriptor in fdset. Wait for rebuild signal on pipe.");
                FD_ZERO(&response_fdset_info.recv_fdset);
                FD_ZERO(&response_fdset_info.send_fdset);
                FD_SET(pipefds[0], &response_fdset_info.recv_fdset);
            }
            else {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "Select returns with error (%d)",
                                 errno);
                VPLThread_Sleep(VPLTime_FromSec(10));
                continue;
            }
        }

        // Let VSSI handle its sockets
        VSSI_HandleSockets(vstest_vplsock_recv_ready, 
                           vstest_vplsock_send_ready,
                           &response_fdset_info);

        wake_up = 0;
        err = 0;
        if(FD_ISSET(pipefds[0], &(response_fdset_info.recv_fdset))) {
            cmd = '\0';
            do {
                VPLTRACE_LOG_FINEST(TRACE_APP, 0,
                                    "Reading net handle pipe...");
                rc = read(pipefds[0], &cmd, 1);
#if defined(WIN32)
                if(rc == SOCKET_ERROR) err = WSAGetLastError();
#else
                if(rc == -1) err = errno;
#endif
                VPLTRACE_LOG_FINEST(TRACE_APP, 0,
                                    "Read net handler pipe. rc:%d(%d), cmd:%c",
                                    rc, err, cmd);
                if(rc == 0) {
                    // no more messages at EOF.
                    break;
                }
                else if(rc == 1) {
                    // Got a message.
                    wake_up = 1;
                }
                else {
                    // Got an error.
#if defined(WIN32)
                    if(err == WSAEWOULDBLOCK) {
#else
                    if(err == EAGAIN || err == EINTR) {
#endif
                        // no big deal
                        break;
                    }
                    else {
                        VPLTRACE_LOG_ERR(TRACE_APP, 0,
                                         "Got error (%d) reading notify pipe.",
                                         err);
                    }
                }
            } while(rc == 1);
        }

        if(wake_up) {
            VPLTRACE_LOG_FINEST(TRACE_APP, 0,
                                "libbvs socket roster changed.");
            FD_ZERO(&master_fdset_info.recv_fdset);
            FD_ZERO(&master_fdset_info.send_fdset);
            FD_SET(pipefds[0], &master_fdset_info.recv_fdset);
            master_fdset_info.maxfd = pipefds[0];
            
            VSSI_ForEachSocket(vstest_add_vplsock,
                               &master_fdset_info);
        }
    }

    close(pipefds[0]);
    close(pipefds[1]);

    return NULL;
}

void wakeup_vssi_select_thread(void)
{
    char c = '0';
    ssize_t ignore = write(pipefds[1], &c, sizeof c);
    (void)ignore;
}
