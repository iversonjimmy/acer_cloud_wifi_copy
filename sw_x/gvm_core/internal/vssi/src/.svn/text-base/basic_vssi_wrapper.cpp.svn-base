//
//  Copyright 2013 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER CLOUD
//  TECHNOLOGY INC.
//
#include "basic_vssi_wrapper.h"
#include "vpl_lazy_init.h"
#include "vpl_socket.h"
#include "vplex_socket.h"
#include "vpl_th.h"
#include "vplu_format.h"
#include "vplu_mutex_autolock.hpp"
#include "vssi.h"
#include "vssi_error.h"
#include <deque>
#include <vector>
#include <log.h>

struct SocketState {
    VPLSocket_t socket;

    // Possible states:
    // VPLSOCKET_POLL_RDNORM
    // VPLSOCKET_POLL_OUT
    uint16_t eventsBitmask;
};

/// Local socket that can be used to wake the net handler thread (client side).
static VPLSocket_t priv_socket_client;
/// Local socket that the net handler thread listens on (to allow other threads to wake it).
static VPLSocket_t priv_socket_server_listen;
static VPLSocket_t priv_socket_server_connected;

static VPLThread_t vssi_wrap_handler_thread;
static bool is_init = false;

static std::deque<char> cmdq;
static const int CMDQ_MAX_SIZE = 256;
static VPLLazyInitMutex_t cmdq_mutex = VPLLAZYINITMUTEX_INIT;
static VPLLazyInitMutex_t is_init_mutex = VPLLAZYINITMUTEX_INIT;
static inline int cmdq_is_full(void)
{
    return static_cast<int>(cmdq.size()) >= CMDQ_MAX_SIZE;
}

/// Enqueue a command to be handled by the handler thread.
/// Commands are dequeued and processed only by the #vsd_net_handler function loop.
static void cmdq_enqueue(char new_req)
{
    LOG_DEBUG("cmdq request: %c", new_req);
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&cmdq_mutex));
    if (cmdq_is_full()) {
        LOG_ERROR("Net handler new req notice failed: command queue full.");
    }
    else {
        cmdq.push_back(new_req);
    }
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&cmdq_mutex));
    // Tell the handler thread to wake up now.
    const int bytesToWrite = 1;
    int temp_rv = VPLSocket_Write(priv_socket_client,
                                  " ", bytesToWrite,
                                  static_cast<VPLTime_t>(VPL_TIMEOUT_NONE));
    if (temp_rv != bytesToWrite) {
        LOG_ERROR("VPLSocket_Write:%d", temp_rv);
    } else {
        LOG_DEBUG("Wrote byte to priv socket: %d", temp_rv);
    }
}

// Arbitrary characters for the notifications pipe to indicate what's needed.
#define NET_HANDLER_STOP 'X'
#define NET_HANDLER_UPDATE 'U'

/// Helper function to build socket set from VSSI.
static void vssi_wrap_collect_sockets(VPLSocket_t vpl_sock,
                                      int recv, int send, void* ctx)
{
    std::vector<SocketState>* set = (std::vector<SocketState>*)(ctx);
    if(send || recv) {
        SocketState temp;
        temp.socket = vpl_sock;
        temp.eventsBitmask = 0;
        if(recv) {
            temp.eventsBitmask |= VPLSOCKET_POLL_RDNORM;
        }
        if(send) {
            temp.eventsBitmask |= VPLSOCKET_POLL_OUT;
        }
        set->push_back(temp);
        LOG_DEBUG("Added net socket "FMT_VPLSocket_t" to master FD set for %s.",
                  VAL_VPLSocket_t(vpl_sock),
                  (recv && send) ? "send and receive" :
                  (recv) ? "receive only" :
                  (send) ? "send only" : "neither send nor receive");
    }
}

static int create_private_sockets(VPLSocket_t& priv_socket_server_listen,
                                  VPLSocket_t& priv_socket_client,
                                  VPLSocket_t& priv_socket_server_connected)
{
    int rv;
    int connect_retry_count;
    int accept_retry_count;
    VPLSocket_addr_t privSockAddr;
    VPLNet_port_t privSockPort;
    int yes;
    priv_socket_server_listen = VPLSocket_CreateTcp(VPLNET_ADDR_LOOPBACK, VPLNET_PORT_ANY);
    if (VPLSocket_Equal(priv_socket_server_listen, VPLSOCKET_INVALID)) {
        LOG_ERROR("VPLSocket_CreateTcp (receiving-side) failed.");
        rv = VPL_ERR_SOCKET;
        goto fail_server_sock_create;
    }
    rv = VPLSocket_Listen(priv_socket_server_listen, 100);
    if (rv != VPL_OK) {
        LOG_ERROR("VPLSocket_Listen failed: %d.", rv);
        goto fail_server_sock_listen;
    }
    privSockPort = VPLSocket_GetPort(priv_socket_server_listen);
    if (privSockPort == VPLNET_PORT_INVALID) {
        LOG_ERROR("VPLSocket_GetPort failed.");
        rv = VPL_ERR_SOCKET;
        goto fail_server_sock_get_port;
    }
    priv_socket_client = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_STREAM, VPL_TRUE);
    if (VPLSocket_Equal(priv_socket_client, VPLSOCKET_INVALID)) {
        LOG_ERROR("VPLSocket_CreateTcp (sending-side) failed.");
        rv = VPL_ERR_SOCKET;
        goto fail_client_sock_create;
    }
    yes = 1;
    rv = VPLSocket_SetSockOpt(priv_socket_client, VPLSOCKET_IPPROTO_TCP,
                              VPLSOCKET_TCP_NODELAY, &yes, sizeof(int));
    if (rv != VPL_OK) {
        LOG_ERROR("VPLSocket_SetSockOpt(TCP_NODELAY) failed: %d.", rv);
        goto fail_client_sock_set_nodelay;
    }
    privSockAddr.family = VPL_PF_INET;
    privSockAddr.addr = VPLNET_ADDR_LOOPBACK;
    privSockAddr.port = privSockPort;
    connect_retry_count = 0;
 retry_connect:
    rv = VPLSocket_Connect(priv_socket_client, &privSockAddr, sizeof(privSockAddr));
    if (rv != VPL_OK) {
        if (connect_retry_count < 16) {
            VPLThread_Sleep(VPLTime_FromMillisec(500));
            connect_retry_count++;
            goto retry_connect;
        }
        LOG_ERROR("VPLSocket_Connect failed: %d.", rv);
        goto fail_client_sock_connect;
    }
    accept_retry_count = 0;
 retry_accept:
    rv = VPLSocket_Accept(priv_socket_server_listen, NULL, 0, &priv_socket_server_connected);
    if (rv == VPL_ERR_AGAIN) {
        if (accept_retry_count < 200) {
            VPLThread_Sleep(VPLTime_FromMillisec(50));
            accept_retry_count++;
            goto retry_accept;
        }
    }
    if (rv != VPL_OK) {
        LOG_ERROR("VPLSocket_Accept failed: %d.", rv);
        goto fail_server_sock_accept;
    }
    goto out;
 fail_server_sock_accept:
 fail_client_sock_connect:
 fail_client_sock_set_nodelay:
    VPLSocket_Close(priv_socket_client);
    priv_socket_client = VPLSOCKET_INVALID;
 fail_client_sock_create:
 fail_server_sock_get_port:
 fail_server_sock_listen:
    VPLSocket_Close(priv_socket_server_listen);
    priv_socket_server_listen = VPLSOCKET_INVALID;
 fail_server_sock_create:
 out:
    return rv;
}

/// Helper function so VSSI can determine active sockets
static int vssi_wrap_socket_recv_ready(VPLSocket_t vpl_sock, void* ctx)
{
    int rv = 0;
    VPLSocket_poll_t* pollspec = (VPLSocket_poll_t*)(ctx);
    int i = 0;

    while(!VPLSocket_Equal(pollspec[i].socket, VPLSOCKET_INVALID)) {
        if(VPLSocket_Equal(pollspec[i].socket, vpl_sock)) {
            break;
        }
        LOG_DEBUG("Net socket "FMT_VPLSocket_t" !=pollspec[%d] socket "FMT_VPLSocket_t".",
                  VAL_VPLSocket_t(vpl_sock), i, VAL_VPLSocket_t(pollspec[i].socket));
        i++;
    }

    if(!VPLSocket_Equal(pollspec[i].socket, vpl_sock)) {
        LOG_DEBUG("Net socket "FMT_VPLSocket_t" not in pollspec!",
                  VAL_VPLSocket_t(vpl_sock));
    }
    else if(VPLSocket_Equal(pollspec[i].socket, vpl_sock) &&
            (pollspec[i].revents &
             (VPLSOCKET_POLL_RDNORM |
              VPLSOCKET_POLL_ERR |
              VPLSOCKET_POLL_HUP |
              VPLSOCKET_POLL_SO_INVAL |
              VPLSOCKET_POLL_EV_INVAL)))
    {
        rv = 1;
    }
    LOG_DEBUG("Net socket "FMT_VPLSocket_t" %s for receive.",
              VAL_VPLSocket_t(vpl_sock), rv ? "ready" : "not ready");
    return rv;
}

static int vssi_wrap_socket_send_ready(VPLSocket_t vpl_sock, void* ctx)
{
    int rv = 0;
    VPLSocket_poll_t* pollspec = (VPLSocket_poll_t*)(ctx);
    int i = 0;

    while(!VPLSocket_Equal(pollspec[i].socket, VPLSOCKET_INVALID)) {
        if(VPLSocket_Equal(pollspec[i].socket, vpl_sock)) {
            break;
        }
        LOG_DEBUG("Net socket "FMT_VPLSocket_t" !=pollspec[%d] socket "FMT_VPLSocket_t".",
                  VAL_VPLSocket_t(vpl_sock), i, VAL_VPLSocket_t(pollspec[i].socket));
        i++;
    }

    if(!VPLSocket_Equal(pollspec[i].socket, vpl_sock)) {
        LOG_DEBUG("Net socket "FMT_VPLSocket_t" not in pollspec!",
                  VAL_VPLSocket_t(vpl_sock));
    }
    else if(VPLSocket_Equal(pollspec[i].socket, vpl_sock) &&
            (pollspec[i].revents & VPLSOCKET_POLL_OUT)) {
        rv = 1;
    }
    LOG_DEBUG("Net socket "FMT_VPLSocket_t" %s for send.",
              VAL_VPLSocket_t(vpl_sock), rv ? "ready" : "not ready");

    return rv;
}

static VPLThread_return_t vssi_wrap_net_handler_run(VPLThread_arg_t unused)
{
    (void) unused;
    int rc;
    std::vector<SocketState> socket_set;

    while(1) {
        {
            // Determine time to wait for events.
            VPLTime_t timeout = VSSI_HandleSocketsTimeout();
            size_t i;
            // Build pollspec.
            // Add an extra pollspec for the private socket.
            // Add another extra pollspec as a terminator (required by the #vsd_net_handler_vplsock_recv_ready
            // and #vsd_net_handler_vplsock_send_ready callbacks).
            VPLSocket_poll_t* pollspec = (VPLSocket_poll_t*)calloc(sizeof(VPLSocket_poll_t),
                                                                   socket_set.size() + 2);
            pollspec[0].socket = priv_socket_server_connected;
            pollspec[0].events = VPLSOCKET_POLL_RDNORM;
            for(i = 0; i < socket_set.size(); i++) {
                pollspec[i+1].socket = socket_set[i].socket;
                pollspec[i+1].events = socket_set[i].eventsBitmask;
                LOG_DEBUG("pollspec["FMTu_size_t"].socket = "FMT_VPLSocket_t,
                          i+1, VAL_VPLSocket_t(pollspec[i+1].socket));
            }
            pollspec[i+1].socket = VPLSOCKET_INVALID; // terminator

            LOG_DEBUG("Polling "FMTu_size_t" sockets, timeout="FMT_VPLTime_t,
                      socket_set.size() + 1, timeout);
            rc = VPLSocket_Poll(pollspec, socket_set.size() + 1, timeout);
            LOG_DEBUG("VPLSocket_Poll returned %d", rc);
            if(rc < 0) {
                switch(rc) {
                case VPL_ERR_AGAIN:
                case VPL_ERR_INVALID: // Sudden connection closure.
                case VPL_ERR_INTR: // Debugger interrupt.
                case VPL_ERR_NOTSOCK:
                    LOG_WARN("Poll() error: %d. Continuing.",
                             rc);
                    break;
                default: // Something unexpected. Crashing!
                    LOG_ERROR("Poll() error: %d.",
                              rc);
                    free(pollspec);
                    goto exit;
                }
            }
            else if(rc == 0) {
                // The timeout was reached.  Check for socket timeouts.
                VSSI_HandleSockets(vssi_wrap_socket_recv_ready,
                                   vssi_wrap_socket_send_ready,
                                   pollspec);
            }
            else if (rc > 0) {
                LOG_DEBUG("Got socket activity...");

                if (pollspec[0].revents &
                    (VPLSOCKET_POLL_RDNORM | VPLSOCKET_POLL_ERR | VPLSOCKET_POLL_HUP |
                     VPLSOCKET_POLL_SO_INVAL | VPLSOCKET_POLL_EV_INVAL)) {
                    char tempBuf[128];
                    int temp_rv;
                    LOG_DEBUG("activity on private socket: "FMTu16, pollspec[0].revents);
                    temp_rv = VPLSocket_Recv(pollspec[0].socket, tempBuf, ARRAY_SIZE_IN_BYTES(tempBuf));
                    LOG_DEBUG("VPLSocket_Recv on private socket: %d", temp_rv);
#ifdef IOS
                    if (temp_rv == VPL_ERR_NOTCONN) {
                        rc = create_private_sockets(priv_socket_server_listen,
                                                    priv_socket_client,
                                                    priv_socket_server_connected);
                        if (rc != VPL_OK) {
                            LOG_ERROR("Failed to re-establish private sockets , rv = %d.", rc);
                        }
                    }
#endif
                }

                // Let VSSI handle its sockets
                VSSI_HandleSockets(vssi_wrap_socket_recv_ready,
                                   vssi_wrap_socket_send_ready,
                                   pollspec);
            }

            free(pollspec);
        }

        // Check command queue for new commands.
        // Quit loop on NET_HANDLER_STOP order.
        // Anything else, just eat it.
        VPL_BOOL wake_up = VPL_FALSE;
        // Dequeue and process all commands:
        VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&cmdq_mutex));
        while (!cmdq.empty()) {
            char cmd = cmdq.front();
            cmdq.pop_front();

            LOG_DEBUG("Got command: %c", cmd);

            switch(cmd) {
            case NET_HANDLER_STOP:
                LOG_INFO("Net handler stop. time to go...");
                VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&cmdq_mutex));
                goto exit;

            case NET_HANDLER_UPDATE:
                // handle updated sockets after pipe is empty.
                wake_up = VPL_TRUE;
                break;
            }
        }
        VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&cmdq_mutex));

        if(wake_up) {
            LOG_DEBUG("VSSI socket roster changed.");
            // Collect information on sockets
            socket_set.clear();
            VSSI_ForEachSocket(vssi_wrap_collect_sockets, &socket_set);
        }
    }

 exit:
    return NULL;
}

/// Function to send the thread a wake-up notice
static void vssi_wrap_update_sockets(void)
{
    LOG_DEBUG("net handler being told to update sockets. Something changed.");
    cmdq_enqueue(NET_HANDLER_UPDATE);
}

int vssi_wrap_start(u64 device_id)
{
    int rv = 0;
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&is_init_mutex));
    if(is_init) {
        LOG_ERROR("Already initialized, returning OK.");
        return 0;
    }

    // Initialize VSSI
    rv = VSSI_Init(device_id, vssi_wrap_update_sockets);
    if(rv != VSSI_SUCCESS) {
        LOG_ERROR("Failed to initialize VSSI: %d.", rv);
        goto fail_vssi;
    }

    // Set up a socket for waking up the worker thread.
    rv = create_private_sockets(priv_socket_server_listen,
                                priv_socket_client,
                                priv_socket_server_connected);
    if (rv != VPL_OK) {
        LOG_ERROR("create_private_sockets:%d", rv);
        goto fail_create_priv_sockets;
    }

    // spawn worker thread
    rv = VPLThread_Create(&vssi_wrap_handler_thread,
                          vssi_wrap_net_handler_run,
                          NULL, NULL, "vssi_wrapper_thread");
    if(rv != VPL_OK) {
        LOG_ERROR("Failed to init net handler thread: %d.",
                  rv);
        goto fail_thread;
    }

    is_init = true;
    LOG_INFO("Started vssi_wrapper.");

    goto done;
 fail_thread:
 {
    int rc;
    rc = VPLSocket_Close(priv_socket_server_connected);
    if(rc != 0) {
        LOG_ERROR("VPLSocket_Close(server_connected:"FMT_VPLSocket_t"):%d",
                  VAL_VPLSocket_t(priv_socket_server_connected), rc);
    }
    priv_socket_server_connected = VPLSOCKET_INVALID;
    rc = VPLSocket_Close(priv_socket_client);
    if(rc != 0) {
        LOG_ERROR("VPLSocket_Close(client:"FMT_VPLSocket_t"):%d",
                  VAL_VPLSocket_t(priv_socket_client), rc);
    }
    priv_socket_client = VPLSOCKET_INVALID;
    rc = VPLSocket_Close(priv_socket_server_listen);
    if(rc != 0) {
        LOG_ERROR("VPLSocket_Close(server_listen:"FMT_VPLSocket_t"):%d",
                  VAL_VPLSocket_t(priv_socket_server_listen), rc);
    }
    priv_socket_server_listen = VPLSOCKET_INVALID;
 }
 fail_create_priv_sockets:
    VSSI_Cleanup();
 fail_vssi:
 done:
    return rv;
}

void vssi_wrap_stop(void)
{
    LOG_INFO("vssi_wrap_stop being told to stop.");
    int rc;
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&is_init_mutex));
    if(!is_init) {
        LOG_WARN("Not initialized.");
        return;
    }

    cmdq_enqueue(NET_HANDLER_STOP);
    rc = VPLThread_Join(&vssi_wrap_handler_thread, NULL);
    if(rc != 0) {
        LOG_ERROR("VPLThread_Join:%d", rc);
    }
    LOG_INFO("vssi_wrap thread stopped.");

    // private socket.
    rc = VPLSocket_Close(priv_socket_server_connected);
    if(rc != 0) {
        LOG_ERROR("VPLSocket_Close(server_connected:"FMT_VPLSocket_t"):%d",
                  VAL_VPLSocket_t(priv_socket_server_connected), rc);
    }
    priv_socket_server_connected = VPLSOCKET_INVALID;
    rc = VPLSocket_Close(priv_socket_client);
    if(rc != 0) {
        LOG_ERROR("VPLSocket_Close(client:"FMT_VPLSocket_t"):%d",
                  VAL_VPLSocket_t(priv_socket_client), rc);
    }
    priv_socket_client = VPLSOCKET_INVALID;
    rc = VPLSocket_Close(priv_socket_server_listen);
    if(rc != 0) {
        LOG_ERROR("VPLSocket_Close(server_listen:"FMT_VPLSocket_t"):%d",
                  VAL_VPLSocket_t(priv_socket_server_listen), rc);
    }
    priv_socket_server_listen = VPLSOCKET_INVALID;

    // clean-up VSSI.
    VSSI_Cleanup();
    is_init = false;
}
