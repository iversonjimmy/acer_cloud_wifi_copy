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
#include "vsTest_vscs_common.hpp"

#include "vpl_th.h"
#include "vpl_socket.h"
#include "vplex_trace.h"

#include "vssi.h"
#include "vssi_error.h"

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>


static VPLThread_t poll_thread;
static int poll_thread_should_run = 1;
static bool thread_created = false;

int do_vssi_setup(u64 deviceId, 
                  const vplex::vsDirectory::SessionInfo& loginSession,
                  VSSI_Session& session)
{
    int rv = 0;

    rv = vssi_poll_thread_init();
    if(rv != 0) {
	VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Initialization of libvssi poll thread.");
	goto fail;
    }
    rv = VPLThread_Create(&poll_thread,
                          vssi_poll_thread,
                          &poll_thread_should_run,
                          0, // default VPLThread thread-attributes: priority, stack-size, etc.
                          "libvssi_poll_thread");
    if (rv != VPL_OK) {
	VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "creating libvssi_poll_thread failed!");
	goto fail;
    }
    thread_created = true;

    if(VSSI_Init(deviceId, wakeup_vssi_poll_thread) != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI init failed!");
        rv++;
        goto fail;
    }

    // Register session
    session = VSSI_RegisterSession(loginSession.sessionhandle(),
                                   loginSession.serviceticket().c_str());
    if(session == 0) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Session registration failed!.");
        rv++;
        goto fail;
    }

 fail:
    return rv;
}

int do_vssi_setup(u64 deviceId)
{
    int rv = 0;

    rv = vssi_poll_thread_init();
    if(rv != 0) {
	VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Initialization of libvssi poll thread.");
	goto fail;
    }
    rv = VPLThread_Create(&poll_thread,
                          vssi_poll_thread,
                          &poll_thread_should_run,
                          0, // default VPLThread thread-attributes: priority, stack-size, etc.
                          "libvssi_poll_thread");
    if (rv != VPL_OK) {
	VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "creating libvssi_poll_thread failed!");
	goto fail;
    }
    thread_created = true;

    if(VSSI_Init(deviceId, wakeup_vssi_poll_thread) != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI init failed!");
        rv++;
        goto fail;
    }

 fail:
    return rv;
}

void do_vssi_cleanup(VSSI_Session& session)
{
    VPLThread_return_t dontcare;

    // End session
    if(session != 0) {
        VSSI_EndSession(session);
    }
   
    poll_thread_should_run = 0;
    if(thread_created) {
        wakeup_vssi_poll_thread();
        VPLThread_Join(&poll_thread, &dontcare);
    }

    VSSI_Cleanup();
}

void do_vssi_cleanup(void)
{
    VPLThread_return_t dontcare;
   
    poll_thread_should_run = 0;
    if(thread_created) {
        wakeup_vssi_poll_thread();
        VPLThread_Join(&poll_thread, &dontcare);
    }

    VSSI_Cleanup();
}

static int pipefds[2] = { -1, -1 };

struct poll_info {
    VPLSocket_poll_t* pollInfo;
    size_t infoSize; // size of allocated pollInfo array
    size_t infoCount; // Number of slots in-use for pollInfo array
};

/// Helper function to build select set from libbvs.
static void vstest_add_vplsock(VPLSocket_t vpl_sock, int recv, int send, void* ctx)
{
    struct poll_info* poll_info = (struct poll_info*)(ctx);

    if(send || recv) {
        // grow polInfo if full
        if(poll_info->infoSize == poll_info->infoCount) {
            VPLSocket_poll_t* tmp = (VPLSocket_poll_t*)realloc(poll_info->pollInfo, 
                                                               sizeof(VPLSocket_poll_t) * poll_info->infoSize * 2);
            if(tmp == NULL) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0,
                                 "FAILED TO REALLOC POLL INFO!");
                return;
            }
            poll_info->pollInfo = tmp;
            poll_info->infoSize *= 2;
        }

        poll_info->pollInfo[poll_info->infoCount].socket = vpl_sock;
        poll_info->pollInfo[poll_info->infoCount].events = 0;
        poll_info->pollInfo[poll_info->infoCount].revents = 0;
        if(recv) {
            poll_info->pollInfo[poll_info->infoCount].events |= VPLSOCKET_POLL_RDNORM;
        }
        if(send) {
            poll_info->pollInfo[poll_info->infoCount].events |= VPLSOCKET_POLL_OUT;
        }
        poll_info->infoCount++;
    }

    VPLTRACE_LOG_FINEST(TRACE_APP, 0,
                        "Added socket %d for %s.",
                        vpl_sock.fd,
                        (recv && send) ? "send and receive" :
                        (recv) ? "receive only" :
                        (send) ? "send only" :
                        "neither send not receive");
}

/// Helper functions so libbvs can determine active sockets
static int vstest_vplsock_recv_ready(VPLSocket_t vpl_sock, void* ctx)
{
    int rv = 0;
    struct poll_info* poll_info = (struct poll_info*)(ctx);

    for(int i = 0; i < poll_info->infoCount; i++) {
        if(VPLSocket_Equal(poll_info->pollInfo[i].socket, vpl_sock)) {
            if(poll_info->pollInfo[i].revents & 
               (VPLSOCKET_POLL_ERR | VPLSOCKET_POLL_HUP | VPLSOCKET_POLL_RDNORM)) {
                rv = 1;
            }
            break;
        }
    }

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "Socket %d %s for receive.", 
                        vpl_sock.fd, rv ? "ready" : "not ready");
    return rv;
}
static int vstest_vplsock_send_ready(VPLSocket_t vpl_sock, void* ctx)
{
    int rv = 0;
    struct poll_info* poll_info = (struct poll_info*)(ctx);

    for(int i = 0; i < poll_info->infoCount; i++) {
        if(VPLSocket_Equal(poll_info->pollInfo[i].socket, vpl_sock)) {
            if(poll_info->pollInfo[i].revents & 
               (VPLSOCKET_POLL_OUT)) {
                rv = 1;
            }
            break;
        }
    }

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "Socket %d %s for send.", 
                        vpl_sock.fd, rv ? "ready" : "not ready");
    return rv;
}

int vssi_poll_thread_init(void)
{
    int i;
    int rc = pipe(pipefds);
    int rv = -1;

    if(rc != 0) {
        int err = errno;
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "FAIL:"
                         "Failed to create notify pipe: %s(%d)",
                         strerror(err), err);
        goto exit;
    }

    for(i = 0; i < 2; i++) {
        int flags = fcntl(pipefds[i], F_GETFL);
        if (flags == -1) {
            int err = errno;
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "FAIL:"
                             "Failed to get flags for end %d of pipe: %s(%d)",
                             i, strerror(err), err);
            goto close_pipe;
        }
        rc = fcntl(pipefds[i], F_SETFL, flags | O_NONBLOCK);
        if(rc == -1) {
            int err = errno;
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "FAIL:"
                             "Failed to set nonblocking flag for end %d of pipe: %s(%d)",
                             i, strerror(err), err);
            goto close_pipe;  
        }
    }
    
    rv = 0;
    goto exit;
 close_pipe:
    close(pipefds[0]);
    close(pipefds[1]);
 exit:
    return rv;
}

VPLThread_return_t vssi_poll_thread(VPLThread_arg_t arg)
{
    struct poll_info poll_info;

    int *e = (int *)arg;

    poll_info.pollInfo = (VPLSocket_poll_t*)calloc(sizeof(VPLSocket_poll_t), 10); // enough for normal operation?
    poll_info.infoSize = 10;
    poll_info.infoCount = 1;
    poll_info.pollInfo[0].socket.fd = pipefds[0]; // will work for linux, at least
    poll_info.pollInfo[0].events = VPLSOCKET_POLL_RDNORM;

    while (*e) {
        int rv;
        char cmd = '\0';
        char wake_up = 0;
        int err = 0;
        int rc;

        // Determine how long to wait.
        VPLTime_t timeout_us = VSSI_HandleSocketsTimeout();
        int timeout_ms;
        if(timeout_us == VPL_TIMEOUT_NONE) {
            timeout_ms = -1;
        }
        else {
            timeout_ms = timeout_us / 1000;
        }

        VPLTRACE_LOG_FINEST(TRACE_APP, 0,
                            "Waiting for activity %dms...",
                            timeout_ms);

        rv = VPLSocket_Poll(poll_info.pollInfo,
                            poll_info.infoCount,
                            timeout_ms);
        if (rv < 0) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "Poll returns with error (%d):%s",
                             errno, strerror(errno));
            sleep(10);
            continue;
        }
        
        // Let VSSI handle its sockets
        VSSI_HandleSockets(vstest_vplsock_recv_ready, 
                           vstest_vplsock_send_ready,
                           &poll_info);

        wake_up = 0;
        err = 0;
        if(poll_info.pollInfo[0].revents & VPLSOCKET_POLL_RDNORM) {
            cmd = '\0';
            do {
                VPLTRACE_LOG_FINEST(TRACE_APP, 0,
                                    "Reading net handle pipe...");
                rc = read(pipefds[0], &cmd, 1);
                if(rc == -1) err = errno;
                VPLTRACE_LOG_FINEST(TRACE_APP, 0,
                                    "Read net handler pipe. rc:%d(%s:%d), cmd:%c",
                                    rc, strerror(err), err, cmd);
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
                    if(err == EAGAIN || err == EINTR) {
                        // no big deal
                        break;
                    }
                    else {
                        VPLTRACE_LOG_ERR(TRACE_APP, 0,
                                         "Got error %s(%d) reading notify pipe.",
                                         strerror(err), err);
                    }
                }
            } while(rc == 1);
        }

        if(wake_up) {
            VPLTRACE_LOG_FINEST(TRACE_APP, 0,
                                "libbvs socket roster changed.");
            poll_info.infoCount = 1;
            
            VSSI_ForEachSocket(vstest_add_vplsock,
                               &poll_info);
        }
    }

    close(pipefds[0]);
    close(pipefds[1]);
    free(poll_info.pollInfo);

    return NULL;
}

void wakeup_vssi_poll_thread(void)
{
    char c = '0';
    ssize_t ignore = write(pipefds[1], &c, sizeof c);
    (void)ignore;
}
