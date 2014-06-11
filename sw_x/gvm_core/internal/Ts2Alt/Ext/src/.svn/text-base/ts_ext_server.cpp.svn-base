/*
 *  Copyright 2014 Acer Cloud Technology Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF IGWARE INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
 *
 */


///
/// ts_ext_server.cpp
///
/// Tunnel Services External Client Server Support.


#ifndef IS_TS
#define IS_TS
#endif


#include "vplu_types.h"
#include "ts_client.hpp"
#include "vpl_th.h"
#include "vpl_lazy_init.h"
#include "vplex_trace.h"
#include "vplu_mutex_autolock.hpp"
#include "vpl_socket.h"
#include "vplex_socket.h"
#include "vpl_net.h"
#include "gvm_thread_utils.h"
#include "gvm_errors.h"
#include "ts_ext_server.hpp"
#include "ts_ext_pkt.hpp"
#include <stdio.h>
#include "map"
#include "list"

using namespace std;


typedef struct tse_write_request_s {
    bool active;
    u32 xid;
    u8 data[TS_EXT_WRITE_MAX_BUF_SIZE];
    size_t data_len;
} tse_write_request_t;

typedef struct tse_read_request_s {
    bool active;
    u32 xid;
    u8 data[TS_EXT_READ_MAX_BUF_SIZE];
    size_t max_data_len;
} tse_read_request_t;

class ts_ext_conn {
public:
    ts_ext_conn(VPLSocket_t sockfd, VPLSocket_addr_t addr);
    ~ts_ext_conn();

    TSError_t start(void);
    TSError_t stop(void);
    TSError_t force_close(void);
    bool is_conn_closed(void);

    void worker(void);
    void writer(void);
    void reader(void);

    TSError_t send_sock(const char* buffer, size_t buf_len);
    TSError_t send_pkt(ts_ext_pkt* pkt);
    TSError_t recv_sock(char* buffer, size_t& buf_len);
    TSError_t recv_pkt_hdrs(ts_ext_pkt_hdr_t& pkt_hdr,
                            char* pkt_type_hdr_buffer);

private:
    TSError_t make_signal_connections(void);
    TSError_t accept_signal_connections(void);
    TSError_t send_signals(void);

    VPLSocket_t         sockfd;
    VPLSocket_addr_t    addr;
    volatile bool       is_closed;
    volatile bool       is_forcing_close;
    TSIOHandle_t        ioh;

    // Use signal connections to unblock VPLSocket_Poll()
    VPLNet_port_t               signal_port;
    VPLSocket_t                 signal_sockfd;
    VPLSocket_t                 client_read_signal_sockfd;
    VPLSocket_t                 client_write_signal_sockfd;
    VPLSocket_t                 server_read_signal_sockfd;
    VPLSocket_t                 server_write_signal_sockfd;

    // write_mutex is dedicated to blocking writes.  The rule is that while holding
    // the general mutex, one should not try to acquire the write_mutex as well to
    // prevent deadlock

    VPLMutex_t          mutex;
    VPLMutex_t          write_mutex;
    VPLCond_t           write_cond;
    VPLCond_t           read_cond;

    tse_write_request_t write_req;
    tse_read_request_t  read_req;

    VPLDetachableThreadHandle_t worker_thread;
    VPLDetachableThreadHandle_t writer_thread;
    VPLDetachableThreadHandle_t reader_thread;
};

class ts_ext {
public:
    ts_ext();
    ~ts_ext();

    TSError_t start(void);
    TSError_t stop(void);

    void do_service(void);
    TSError_t signal_server(void);
    VPLNet_port_t getport(void) { return port; }

private:
    TSError_t make_conn(VPLSocket_t sockfd, VPLSocket_addr_t addr);
    TSError_t make_signal_connection(void);
    TSError_t accept_signal_connection(void);

    VPLMutex_t                  mutex;
    volatile bool               is_forcing_stop;
    VPLNet_port_t               port;
    VPLDetachableThreadHandle_t server_thread;
    map<u32, ts_ext_conn*>      conn_map;
    u32                         conn_id_next;

    VPLSocket_t                 sockfd; 
    VPLSocket_t                 server_signal_sockfd; 
    VPLSocket_t                 client_signal_sockfd; 
};


static const size_t workerStackSize = MAX(32*1024, UTIL_DEFAULT_THREAD_STACK_SIZE);

static volatile bool is_tse_init = false;
static ts_ext* ext_server = NULL;
static VPLLazyInitMutex_t ext_server_mutex = VPLLAZYINITMUTEX_INIT;

static volatile bool is_recovery_needed = false;
static volatile bool is_stopping_recovery_thread = false;
static VPLDetachableThreadHandle_t recovery_thread;
static VPLMutex_t recovery_mutex;
static VPLCond_t recovery_cond;


static VPLTHREAD_FN_DECL ext_server_recovery(void* arg)
{
    TSError_t err = TS_OK;
    int rv;
    bool do_recovery = false;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "ext_server_recovery() starts");

    VPLMutex_Lock(&recovery_mutex);
    while (!is_stopping_recovery_thread) {
        if (!is_recovery_needed) {
            rv = VPLCond_TimedWait(&recovery_cond, &recovery_mutex, VPL_TIMEOUT_NONE);
            if (rv != VPL_OK) {
                VPLMutex_Unlock(&recovery_mutex);

                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Recovery failed timed wait, rv=%d", rv);
                goto done;           
            }
        }

        if (is_stopping_recovery_thread) {
            VPLMutex_Unlock(&recovery_mutex);

            VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Stopping recovery thread");
            goto done;
        }

        do_recovery = is_recovery_needed;
        is_recovery_needed = false;
        VPLMutex_Unlock(&recovery_mutex);

        if (do_recovery) {
            VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&ext_server_mutex));

            // Stop the ext_server
            ext_server->stop();
            delete ext_server;
            ext_server = NULL;
            VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Stopped ext_server");

            // Re-start the ext_server
            ext_server = new (std::nothrow) ts_ext();
            if (ext_server == NULL) {
                VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&ext_server_mutex));

                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
                goto done;
            }

            err = ext_server->start();
            if (err != TS_OK) {
                delete ext_server;
                ext_server = NULL;

                VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&ext_server_mutex));
                goto done;
            }
            VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Re-started ext_server");

            VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&ext_server_mutex));
        }

        VPLMutex_Lock(&recovery_mutex);
    }

done:
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "ext_server_recovery() ends");

    return VPLTHREAD_RETURN_VALUE_UNUSED;
}

TSError_t TSExtServer_Init(string& error_msg)
{
    TSError_t err = TS_OK;
    int rv;
    bool is_server_started = false;

    error_msg.clear();

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Calling TSExtServer_Init()");

    if (is_tse_init) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0, "TS-EXT server already started");
        goto done;
    }

    VPL_SET_UNINITIALIZED(&recovery_mutex);
    VPL_SET_UNINITIALIZED(&recovery_cond);

    rv = VPLMutex_Init(&recovery_mutex);
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to initialize mutex");
        err = TS_ERR_NO_MEM;
        error_msg = "Failed to initialize mutex";
        goto done;
    }

    rv = VPLCond_Init(&recovery_cond);
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to initialize cond");
        err = TS_ERR_NO_MEM;
        error_msg = "Failed to initialize cond";
        goto done;
    }

    ext_server = new (std::nothrow) ts_ext();
    if (ext_server == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate memory");
        err = TS_ERR_NO_MEM;
        error_msg = "Failed to allocate ext_server";
        goto done;
    }

    err = ext_server->start();
    if (err != TS_OK) {
        error_msg = "Failed to start server";
        goto done;
    }
    is_server_started = true;

    is_stopping_recovery_thread = false;
    rv = Util_SpawnThread(ext_server_recovery, NULL, workerStackSize, /*isJoinable*/VPL_TRUE, &recovery_thread);
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLThread_Create(recovery) failed, rv=%d", rv);
        error_msg = "Failed to spawn recovery thread";
        err = TS_ERR_NO_MEM;
        goto done;
    }

    is_tse_init = true;
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "TS-EXT server started");

done:
    if (err != TS_OK) {
        if (ext_server != NULL) {
            if (is_server_started) {
                ext_server->stop();
            }
            delete ext_server;
            ext_server = NULL;

            if (VPL_IS_INITIALIZED(&recovery_mutex)) {
                VPLMutex_Destroy(&recovery_mutex);
            }
            if (VPL_IS_INITIALIZED(&recovery_cond)) {
                VPLCond_Destroy(&recovery_cond);
            }
        }
    }

    return err;
}

void TSExtServer_Stop(void)
{
    int rv;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Calling TSExtServer_Stop()");

    if (!is_tse_init) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0, "TS-EXT server already stopped");
        goto done;
    }

    // Join the recovery thread first
    VPLMutex_Lock(&recovery_mutex);
    is_stopping_recovery_thread = true;
    rv = VPLCond_Signal(&recovery_cond);
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to signal recovery thread, rv=%d", rv);
    }
    VPLMutex_Unlock(&recovery_mutex);

    rv = VPLDetachableThread_Join(&recovery_thread);
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to join recovery thread, rv=%d", rv);
    } else {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Joined recovery thread");
    }

    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&ext_server_mutex));

    // Stop the ext_server
    if (ext_server != NULL) {
        ext_server->stop();
        delete ext_server;
        ext_server = NULL;
    }

    if (VPL_IS_INITIALIZED(&recovery_mutex)) {
        VPLMutex_Destroy(&recovery_mutex);
    }
    if (VPL_IS_INITIALIZED(&recovery_cond)) {
        VPLCond_Destroy(&recovery_cond);
    }

    is_tse_init = false;
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Stopped ext_server");

    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&ext_server_mutex));

done:
    return;
}

TSError_t TSExtGetPort(int& port_out)
{
    TSError_t err = TS_OK;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Calling TSExtGetPort()");

    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&ext_server_mutex));
    if (!is_tse_init || (ext_server == NULL)) {
        port_out = 0;
        err = TS_ERR_NOT_INIT;
        goto done;
    }

    port_out = ext_server->getport();

done:
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&ext_server_mutex));

    return err;
}


static VPLTHREAD_FN_DECL ext_server_start(void* te_v)
{
    ts_ext* te = (ts_ext*)te_v;

    te->do_service();
    
    return VPLTHREAD_RETURN_VALUE_UNUSED;
}

ts_ext::ts_ext() :
    is_forcing_stop(false),
    port(0),
    conn_id_next(0)
{
    VPL_SET_UNINITIALIZED(&mutex);
    sockfd = VPLSOCKET_INVALID;
    server_signal_sockfd = VPLSOCKET_INVALID;
    client_signal_sockfd = VPLSOCKET_INVALID;
}

ts_ext::~ts_ext()
{
    if (conn_map.size() != 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Conn_map not empty.");
    }

    if (VPL_IS_INITIALIZED(&mutex)) {
        VPLMutex_Destroy(&mutex);
    }

    if (!VPLSocket_Equal(sockfd, VPLSOCKET_INVALID)) {
        VPLSocket_Close(sockfd);
    }

    if (!VPLSocket_Equal(server_signal_sockfd, VPLSOCKET_INVALID)) {
        VPLSocket_Close(server_signal_sockfd);
    }

    if (!VPLSocket_Equal(client_signal_sockfd, VPLSOCKET_INVALID)) {
        VPLSocket_Close(client_signal_sockfd);
    }
}

TSError_t ts_ext::start(void)
{
    TSError_t err = TS_OK;
    int rv;
    int yes = 1;
    bool thread_started = false;

    rv = VPLMutex_Init(&mutex);
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to initialize mutex");
        err = TS_ERR_NO_MEM;
        goto done;
    }

    // Create a port used for accepting connections
    sockfd = VPLSocket_CreateTcp(VPLNET_ADDR_LOOPBACK, VPLNET_PORT_ANY);
    if (VPLSocket_Equal(sockfd, VPLSOCKET_INVALID)) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create socket. error:"FMT_VPLSocket_t,
                         VAL_VPLSocket_t(sockfd));
        err = TS_ERR_COMM;
        goto done;
    }

    rv = VPLSocket_SetSockOpt(sockfd, VPLSOCKET_IPPROTO_TCP, VPLSOCKET_TCP_NODELAY, &yes, sizeof(int));
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to set TCP_NODELAY on socket. socket:"FMT_VPLSocket_t", error:%d",
                         VAL_VPLSocket_t(sockfd), rv);
        err = TS_ERR_COMM;
        goto done;
    }

    rv = VPLSocket_Listen(sockfd, 10);
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to listen socket. socket:"FMT_VPLSocket_t
                         ", error:%d", VAL_VPLSocket_t(sockfd), rv);
        err = TS_ERR_COMM;
        goto done;
    }

    port = VPLSocket_GetPort(sockfd);
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "ts-ext client port: "FMTu32, port);

    // start up a thread that will listen for connections and then start
    // handling requests.
    rv = Util_SpawnThread(ext_server_start, this, workerStackSize, /*isJoinable*/VPL_TRUE, &server_thread);
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLThread_Create(ext_server), rv=%d", rv);
        err = TS_ERR_NO_MEM;
        goto done;
    }
    thread_started = true;

    err = make_signal_connection();
    if (err != TS_OK) {
        goto done;
    }

done:
    if (err != TS_OK) {
        if (thread_started) {
            // This should only happen if establishing the signal connection failed.  In
            // this case, the worker thread should error out as well
            rv = VPLDetachableThread_Join(&server_thread);
            if (rv != VPL_OK) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to join thread, rv=%d", rv);
            }
        }
    }

    return err;
}

TSError_t ts_ext::stop(void)
{
    TSError_t err = TS_OK;
    int rv;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Stopping ts_ext_server");

    VPLMutex_Lock(&mutex);
    is_forcing_stop = true;
    VPLMutex_Unlock(&mutex);

    signal_server();

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Joining server thread");
    rv = VPLDetachableThread_Join(&server_thread);
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to join thread, rv=%d", rv);
    }

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "ts_ext_server stop done, err=%d", err);
    return err;
}

TSError_t ts_ext::make_signal_connection(void)
{
    TSError_t err = TS_OK;
    int rv;
    int yes = 1;
    VPLSocket_addr_t sin;

    // For simplicity, make this a blocking socket
    client_signal_sockfd = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_STREAM,
                                            VPL_FALSE /* nonblocking */);
    if (VPLSocket_Equal(client_signal_sockfd, VPLSOCKET_INVALID)) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLSocket_CreateTcp (sending-side) failed");
        err = TS_ERR_COMM;
        goto done;
    }

    rv = VPLSocket_SetSockOpt(client_signal_sockfd, VPLSOCKET_IPPROTO_TCP,
                              VPLSOCKET_TCP_NODELAY, &yes, sizeof(int));
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLSocket_SetSockOpt (sending-side) failed, rv=%d", rv);
        err = TS_ERR_COMM;
        goto done;
    }

    // Connect to the server
    sin.family = VPL_PF_INET;
    sin.addr = VPLNET_ADDR_LOOPBACK;
    sin.port = port;
    rv = VPLSocket_Connect(client_signal_sockfd, &sin, sizeof(sin));
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLSocket_Connect failed, rv=%d", rv);
        err = TS_ERR_COMM;
        goto done;
    }

done:
    return err;
}

TSError_t ts_ext::accept_signal_connection(void)
{
    TSError_t err = TS_OK;
    int rv;

    for (int i = 0 ; i < 200 ; i++) {
        rv = VPLSocket_Accept(sockfd, NULL, 0, &server_signal_sockfd);
        if (rv == VPL_OK) {
            break;
        }

        if ((rv == VPL_ERR_AGAIN) || (rv == VPL_ERR_BADF)) {
            VPLThread_Sleep(VPLTime_FromMillisec(50));
            continue;
        }

        break;
    }

    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLSocket_Accept failed, rv=%d", rv);
        err = TS_ERR_COMM;
        goto done;
    }

done:
    return err;
}

void ts_ext::do_service(void)
{
    TSError_t err = TS_OK;
    int rv;
    VPLSocket_poll_t pollspec[2];
    u32 poll_cnt;
    map<u32, ts_ext_conn*>::iterator it;
    bool do_recovery = false;

    // First create the signal connection
    err = accept_signal_connection();
    if (err != TS_OK) {
        goto done;
    }

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "EXT-TS server ready");

    for (;;) {
        // 1st the signal socket
        pollspec[0].socket = server_signal_sockfd;
        pollspec[0].events = VPLSOCKET_POLL_RDNORM;

        // Now the connection socket
        pollspec[1].socket = sockfd;
        pollspec[1].events = VPLSOCKET_POLL_RDNORM;

        poll_cnt = 2;

        rv = VPLSocket_Poll(pollspec, poll_cnt, VPL_TIMEOUT_NONE);
        if (rv < 0) {
            switch (rv) {
            case VPL_ERR_AGAIN:
            case VPL_ERR_INTR: // Debugger interrupt.
                VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                                  "Poll() error: %d. Continuing.",
                                  rv);
                break;
            default: // Something unexpected. Crashing!
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "Poll() error: %d.",
                                 rv);
                do_recovery = true;
                err = TS_ERR_COMM;
                goto done;
            }
        } else if (rv > 0) {
            // Note that ts-ext is single-threaded, so no locks are needed

            // If there's an unexpected error, close everything and error out
            if ((pollspec[0].revents &
                (VPLSOCKET_POLL_ERR | VPLSOCKET_POLL_HUP | VPLSOCKET_POLL_EV_INVAL | VPLSOCKET_POLL_SO_INVAL)) ||
                (pollspec[1].revents &
                (VPLSOCKET_POLL_ERR | VPLSOCKET_POLL_HUP | VPLSOCKET_POLL_EV_INVAL | VPLSOCKET_POLL_SO_INVAL))) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Socket error, revents=0x"FMTx32":"FMTx32, pollspec[0].revents, pollspec[1].revents);
                do_recovery = true;
                err = TS_ERR_COMM;
                goto done;
            }

            // Clean up signal
            if (pollspec[0].revents & VPLSOCKET_POLL_RDNORM) {
                char tmp;

                rv = VPLSocket_Recv(pollspec[0].socket, &tmp, 1);
                VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Cleaned up signal, rv=%d", rv);
            }

            // Stopping TS-EXT server, clean everything up
            if (is_forcing_stop) {
                VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Forced to stop");
                goto done;
            }
 
            // Check for connections that are already closed
            for (it = conn_map.begin(); it != conn_map.end(); ) {
                if (it->second->is_conn_closed()) {
                    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Reaping closed conn %p", it->second);
                    it->second->stop();
                    delete it->second;
                    conn_map.erase(it++);
                } else {
                    it++;
                }
            }

            // Accept new business
            if (pollspec[1].revents & VPLSOCKET_POLL_RDNORM) {
                VPLSocket_t client_sock;
                VPLSocket_addr_t addr;

                rv = VPLSocket_Accept(sockfd, &addr, sizeof(addr), &client_sock);
                if (rv != VPL_OK) {
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                     "Client connection accept error. socket:"
                                     FMT_VPLSocket_t", error:%d",
                                     VAL_VPLSocket_t(sockfd), rv);
                    continue;
                }

                err = make_conn(client_sock, addr);
                if (err == TS_OK) {
                    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                                      "Accepted new connection from client");
                }
            }
        }
    }

done:
    for (it = conn_map.begin(); it != conn_map.end(); conn_map.erase(it++)) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Reaping conn %p", it->second);
        if (!it->second->is_conn_closed()) {
            it->second->force_close();
        }
        it->second->stop();
        delete it->second;
    }

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Server done");

    // If the sockets error out, it would cause ts-ext-server to be completely
    // non-functional.  In particular, on iOS, the operating system can force
    // socket close when an app switches to run in the background.  To recover
    // from this type of problems, signal the recovery thread to do clean-up
    // and recovery
    if (do_recovery) {
        VPLMutex_Lock(&recovery_mutex);
        is_recovery_needed = true;

        VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Signaling recovery");
        rv = VPLCond_Signal(&recovery_cond);
        if (rv != VPL_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to signal recovery thread, rv=%d", rv);
        }
        VPLMutex_Unlock(&recovery_mutex);
    }

    return;
}

TSError_t ts_ext::signal_server(void)
{
    TSError_t err = TS_OK;
    int rv;

    while (true) {
        rv = VPLSocket_Write(client_signal_sockfd, " ", 1, VPLTIME_FROM_SEC(TS_EXT_TIMEOUT_IN_SEC));
        if (rv <= 0) {
            if (rv == VPL_ERR_AGAIN) {
                continue;
            }

            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLSocket_Write() failed, rv=%d", rv);
            err = TS_ERR_COMM;
            goto done;
        }

        break;
    }

done:
    return err;
}

TSError_t ts_ext::make_conn(VPLSocket_t sockfd, VPLSocket_addr_t addr)
{
    TSError_t err = TS_OK;
    ts_ext_conn* conn = NULL;
    map<int,ts_ext_conn*>::iterator it;

    conn = new (std::nothrow) ts_ext_conn(sockfd, addr);
    if (conn == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate ts_ext_conn");
        err = TS_ERR_NO_MEM;
        goto done;
    }

    err = conn->start();
    if (err != TS_OK) {
        goto done;
    }

    do {
        conn_id_next++;
        if (conn_id_next == 0) {
            conn_id_next = 1;
        }
    } while (conn_map.find(conn_id_next) != conn_map.end());
    conn_map[conn_id_next] = conn;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "new connection on fd %d",
                      VPLSocket_AsFd(sockfd));

done:
    if (err != TS_OK) {
        if (conn != NULL) {
            delete conn;
        }
    }

    return err;
}


static VPLTHREAD_FN_DECL ext_conn_worker(void* conn_v)
{
    ts_ext_conn* conn = (ts_ext_conn*)conn_v;

    conn->worker();

    return VPLTHREAD_RETURN_VALUE_UNUSED;
}

static VPLTHREAD_FN_DECL ext_conn_writer(void* conn_v)
{
    ts_ext_conn* conn = (ts_ext_conn*)conn_v;

    conn->writer();

    return VPLTHREAD_RETURN_VALUE_UNUSED;
}

static VPLTHREAD_FN_DECL ext_conn_reader(void* conn_v)
{
    ts_ext_conn* conn = (ts_ext_conn*)conn_v;

    conn->reader();

    return VPLTHREAD_RETURN_VALUE_UNUSED;
}

ts_ext_conn::ts_ext_conn(VPLSocket_t sockfd, VPLSocket_addr_t addr) :
        sockfd(sockfd),
        addr(addr),
        is_closed(false),
        is_forcing_close(false),
        ioh(NULL),
        signal_port(0),
        signal_sockfd(VPLSOCKET_INVALID),
        client_read_signal_sockfd(VPLSOCKET_INVALID),
        client_write_signal_sockfd(VPLSOCKET_INVALID),
        server_read_signal_sockfd(VPLSOCKET_INVALID),
        server_write_signal_sockfd(VPLSOCKET_INVALID)
{
    VPL_SET_UNINITIALIZED(&mutex);
    VPL_SET_UNINITIALIZED(&write_mutex);
    VPL_SET_UNINITIALIZED(&write_cond);
    VPL_SET_UNINITIALIZED(&read_cond);

    write_req.active = false;
    read_req.active = false;
}

ts_ext_conn::~ts_ext_conn()
{
    if (!VPLSocket_Equal(sockfd, VPLSOCKET_INVALID)) {
        VPLSocket_Close(sockfd);
    }

    if (!VPLSocket_Equal(signal_sockfd, VPLSOCKET_INVALID)) {
        VPLSocket_Close(signal_sockfd);
    }

    if (!VPLSocket_Equal(client_read_signal_sockfd, VPLSOCKET_INVALID)) {
        VPLSocket_Close(client_read_signal_sockfd);
    }

    if (!VPLSocket_Equal(client_write_signal_sockfd, VPLSOCKET_INVALID)) {
        VPLSocket_Close(client_write_signal_sockfd);
    }

    if (!VPLSocket_Equal(server_read_signal_sockfd, VPLSOCKET_INVALID)) {
        VPLSocket_Close(server_read_signal_sockfd);
    }

    if (!VPLSocket_Equal(server_write_signal_sockfd, VPLSOCKET_INVALID)) {
        VPLSocket_Close(server_write_signal_sockfd);
    }

    if (VPL_IS_INITIALIZED(&mutex)) {
        VPLMutex_Destroy(&mutex);
    }

    if (VPL_IS_INITIALIZED(&write_mutex)) {
        VPLMutex_Destroy(&write_mutex);
    }

    if (VPL_IS_INITIALIZED(&write_cond)) {
        VPLCond_Destroy(&write_cond);
    }

    if (VPL_IS_INITIALIZED(&read_cond)) {
        VPLCond_Destroy(&read_cond);
    }
}

TSError_t ts_ext_conn::start(void)
{
    TSError_t err = TS_OK;
    int rv;
    int yes = 1;
    bool thread_started = false;

    if ((rv = VPLMutex_Init(&mutex)) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Init Mutex fails: %d", rv);
        err = TS_ERR_NO_MEM;
        goto done;
    }

    if ((rv = VPLMutex_Init(&write_mutex)) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Init Mutex fails: %d", rv);
        err = TS_ERR_NO_MEM;
        goto done;
    }

    if ((rv = VPLCond_Init(&write_cond)) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Init Cond fails: %d", rv);
        err = TS_ERR_NO_MEM;
        goto done;
    }

    if ((rv = VPLCond_Init(&read_cond)) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Init Cond fails: %d", rv);
        err = TS_ERR_NO_MEM;
        goto done;
    }

    // Create listening sockets for the signal connections
    signal_sockfd = VPLSocket_CreateTcp(VPLNET_ADDR_LOOPBACK, VPLNET_PORT_ANY);
    if (VPLSocket_Equal(signal_sockfd, VPLSOCKET_INVALID)) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create socket. error:"FMT_VPLSocket_t,
                         VAL_VPLSocket_t(signal_sockfd));
        err = TS_ERR_COMM;
        goto done;
    }

    rv = VPLSocket_SetSockOpt(signal_sockfd, VPLSOCKET_IPPROTO_TCP, VPLSOCKET_TCP_NODELAY, &yes, sizeof(int));
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to set TCP_NODELAY on socket. socket:"FMT_VPLSocket_t", error:%d",
                         VAL_VPLSocket_t(signal_sockfd), rv);
        err = TS_ERR_COMM;
        goto done;
    }

    rv = VPLSocket_Listen(signal_sockfd, 10);
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to listen socket. socket:"FMT_VPLSocket_t
                         ", error:%d", VAL_VPLSocket_t(signal_sockfd), rv);
        err = TS_ERR_COMM;
        goto done;
    }

    signal_port = VPLSocket_GetPort(signal_sockfd);
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Signal listen port: "FMTu32, signal_port);

    // Create thread to read from the socket
    rv = Util_SpawnThread(ext_conn_worker, (void *)this, workerStackSize, /*isJoinable*/VPL_TRUE, &worker_thread);
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLThread_Create(worker) - %d", rv);
        err = TS_ERR_NO_MEM;
        goto done;
    }
    thread_started = true;

    // Try to connect to the signal port, and the worker thread will do the accept
    err = make_signal_connections();
    if (err != TS_OK) {
        goto done;
    }

done:
    if (err != TS_OK) {
        if (thread_started) {
            // If this function fails with worker thread started, then it means
            // the connections couldn't be established, and the worker thread
            // should error out as well
            rv = VPLDetachableThread_Join(&worker_thread);
            if (rv != VPL_OK) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to join thread, rv=%d", rv);
            }
        }
    }

    return err; 
}

TSError_t ts_ext_conn::stop(void)
{
    int rv;

    rv = VPLDetachableThread_Join(&worker_thread);
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to join thread, rv=%d", rv);
    }

    return TS_OK;
}

TSError_t ts_ext_conn::force_close(void)
{
    TSError_t err = TS_OK;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Forcing close on ts_ext_conn");

    VPLMutex_Lock(&mutex);
    is_forcing_close = true;
    VPLMutex_Unlock(&mutex);

    send_signals();

    return err;
}

bool ts_ext_conn::is_conn_closed(void)
{
    return is_closed;
}

void ts_ext_conn::worker(void)
{
    TSError_t err = TS_OK;
    int rv;
    string error_msg;
    ts_ext_pkt_hdr_t pkt_hdr;
    char pkt_type_hdr[MAX_TS_EXT_PKT_TYPE_HDR_SIZE];
    ts_ext_open_req_pkt_t* open_req_pkt = NULL;
    ts_ext_open_resp_pkt* open_resp_pkt = NULL;
    ts_ext_close_resp_pkt* close_resp_pkt = NULL;
    ts_ext_read_req_pkt_t* read_req_pkt = NULL;
    u64 user_id, device_id;
    size_t data_len;
    string service_name;
    bool need_close_response = false;
    bool writer_thread_started = false, reader_thread_started = false;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "ts_ext_conn worker() starts");

    // Accept the signal connections
    err = accept_signal_connections();
    if (err != TS_OK) {
        goto done;
    }

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Accepted signal connections");

    err = recv_pkt_hdrs(pkt_hdr, pkt_type_hdr);
    if (err != TS_OK) {
        goto done;
    }

    if (pkt_hdr.pkt_type != TS_EXT_PKT_OPEN_REQ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "The first received hdr type is wrong: "FMTu32,
                         pkt_hdr.pkt_type);
        err = TS_ERR_EXT_TYPE;
        goto done;
    }

    // Parse the body
    open_req_pkt = (ts_ext_open_req_pkt_t*) pkt_type_hdr;
    user_id = VPLConv_ntoh_u64(open_req_pkt->user_id);
    device_id = VPLConv_ntoh_u64(open_req_pkt->device_id);
    service_name.assign((char*) open_req_pkt->service_name,
                        strnlen((char*) open_req_pkt->service_name,
                        TS_EXT_PKT_SVCNM_LEN));
    
    // Received open req body, and should now contain the information needed to
    // open a tunnel
    {
        bool open_failed = false;
        TSOpenParms_t parms;

        parms.user_id = user_id;
        parms.device_id = device_id;
        parms.instance_id = 0UL;
        parms.service_name = service_name;

        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "user_id: "FMTu64" device_id: "FMTu64
                          " service: %s sockfd %d",
                          parms.user_id, parms.device_id,
                          parms.service_name.c_str(),
                          VPLSocket_AsFd(sockfd));

        err = TS::TS_Open(parms, ioh, error_msg);
        if (err != TS_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                FMTu64" "FMT_VPLNet_addr_t":%u "
                "TS_Open("FMTu64","FMTu64",%s) - %d:%s",
                device_id, VAL_VPLNet_addr_t(addr.addr),
                VPLNet_port_ntoh(addr.port),
                user_id, device_id, service_name.c_str(), err,
                error_msg.c_str());
            open_failed = true;
        }

        // Send back open resp packet
        open_resp_pkt = new (std::nothrow) ts_ext_open_resp_pkt(VPLConv_ntoh_u32(pkt_hdr.xid), err);
        if (open_resp_pkt == NULL) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate open resp packet");
            err = TS_ERR_NO_MEM;
            goto done;
        } else {
            err = send_pkt(open_resp_pkt);
            if (err != TS_OK) {
                goto done;
            }
        }

        if (open_failed) {
            // If open failed, no point to continue;
            goto done;
        }
    }

    // Create the writer thread
    rv = Util_SpawnThread(ext_conn_writer, this, workerStackSize, /*isJoinable*/VPL_TRUE, &writer_thread);
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Util_SpawnThread(writer), rv=%d", rv);
        err = TS_ERR_NO_MEM;
        goto done;
    }
    writer_thread_started = true;

    // Create the reader thread
    rv = Util_SpawnThread(ext_conn_reader, this, workerStackSize, /*isJoinable*/VPL_TRUE, &reader_thread);
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Util_SpawnThread(reader), rv=%d", rv);
        err = TS_ERR_NO_MEM;
        goto done;
    }
    reader_thread_started = true;

    while (true) {
        err = recv_pkt_hdrs(pkt_hdr, pkt_type_hdr);
        if (err != TS_OK) {
            goto done;
        }

        switch (pkt_hdr.pkt_type) {
        case TS_EXT_PKT_CLOSE_REQ:
            need_close_response = true;
            VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Received close request");
            goto done;
        case TS_EXT_PKT_WRITE_REQ:
            if (write_req.active) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Unexpected write request while another is active");
                err = TS_ERR_INTERNAL;
                goto done;
            }

            // The ts_ext_client should have already broken up the request
            data_len = VPLConv_ntoh_u32(pkt_hdr.pkt_len) - sizeof(ts_ext_pkt_hdr_t); 
            if (data_len > sizeof(write_req.data)) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Requested write size larger than expected "FMTu_size_t":"FMTu_size_t, data_len, sizeof(write_req.data));
                err = TS_ERR_INTERNAL;
                goto done;
            }

            err = recv_sock((char*) write_req.data, data_len);
            if (err != TS_OK) {
                goto done;
            }

            VPLMutex_Lock(&mutex);
            write_req.xid = VPLConv_ntoh_u32(pkt_hdr.xid);
            write_req.data_len = data_len;
            write_req.active = true;
            rv = VPLCond_Signal(&write_cond);
            if (rv != VPL_OK) {
                VPLMutex_Unlock(&mutex);

                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to signal, rv=%d", rv);
                err = TS_ERR_INTERNAL;
                goto done;
            }
            VPLMutex_Unlock(&mutex);

            break;
        case TS_EXT_PKT_READ_REQ:
            if (read_req.active) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Unexpected read request while another is active");
                err = TS_ERR_INTERNAL;
                goto done;
            }
            read_req_pkt = (ts_ext_read_req_pkt_t*) pkt_type_hdr;

            VPLMutex_Lock(&mutex);
            read_req.xid = VPLConv_ntoh_u32(pkt_hdr.xid);
            if (VPLConv_ntoh_u32(read_req_pkt->max_data_len) > sizeof(read_req.data)) {
                read_req.max_data_len = sizeof(read_req.data);
            } else {
                read_req.max_data_len = VPLConv_ntoh_u32(read_req_pkt->max_data_len);
            }
            read_req.active = true;
            rv = VPLCond_Signal(&read_cond);
            if (rv != VPL_OK) {
                VPLMutex_Unlock(&mutex);

                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to signal, rv=%d", rv);
                err = TS_ERR_INTERNAL;
                goto done;
            }
            VPLMutex_Unlock(&mutex);

            break;
        default:
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Invalid request type: "FMTu32, pkt_hdr.pkt_type);
            err = TS_ERR_INTERNAL;
            goto done;
        }
    }

done:
    if (open_resp_pkt != NULL) {
        delete open_resp_pkt;
    }

    if (ioh != NULL) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Calling TS::TS_Close()");
        err = TS::TS_Close(ioh, error_msg);
        if (err != TS_OK) {
            VPLTRACE_LOG_INFO(TRACE_BVS, 0, "TS_Close() - %d:%s", err, error_msg.c_str());
        }
        ioh = NULL;

        if (need_close_response) {
            close_resp_pkt = new (std::nothrow) ts_ext_close_resp_pkt(VPLConv_ntoh_u32(pkt_hdr.xid), err);
            if (close_resp_pkt == NULL) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate close resp packet");
            } else {
                send_pkt(close_resp_pkt);
                delete(close_resp_pkt);
            }
        }
    }

    force_close();

    VPLMutex_Lock(&mutex);
    is_closed = true;

    rv = VPLCond_Signal(&write_cond);
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to signal, rv=%d", rv);
    }

    rv = VPLCond_Signal(&read_cond);
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to signal, rv=%d", rv);
    }

    VPLMutex_Unlock(&mutex);

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Joining reader and writer threads");
    if (writer_thread_started) {
        rv = VPLDetachableThread_Join(&writer_thread);
        if (rv != VPL_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to join thread, rv=%d", rv);
        }
    }
    if (reader_thread_started) {
        rv = VPLDetachableThread_Join(&reader_thread);
        if (rv != VPL_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to join thread, rv=%d", rv);
        }
    }

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "ts_ext_conn worker() ends");

    // Pretty ugly to call back to ext_server here.  It should be safe to do
    // so without getting the ext_server_mutex, since ts_ext::stop() would
    // stop all threads before returning
    ext_server->signal_server();
}

// receive and process packets sent from ext client to client ccd
void ts_ext_conn::writer(void)
{
    TSError_t err = TS_OK;
    int rv;
    string error_msg;
    u32 xid;
    ts_ext_write_resp_pkt* write_resp_pkt = NULL;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "ts_ext_conn writer() starts");

    while (true) {
        VPLMutex_Lock(&mutex);
        while (!is_closed && !write_req.active) {
            rv = VPLCond_TimedWait(&write_cond, &mutex, VPL_TIMEOUT_NONE);
            if (rv != VPL_OK) {
                VPLMutex_Unlock(&mutex);

                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "writer() failed timed wait, rv=%d", rv);
                goto done;
            }
        }
        VPLMutex_Unlock(&mutex);

        if (is_closed) {
            VPLTRACE_LOG_INFO(TRACE_BVS, 0, "writer() detected close");
            goto done;
        }

        err = TS::TS_Write(ioh, (const char*) write_req.data, write_req.data_len, error_msg);
        if (err != TS_OK) {
            VPLTRACE_LOG_WARN(TRACE_BVS, 0, "TS_Write() failed, rv=%d:%s", err, error_msg.c_str());
        }
        xid = write_req.xid;

        // Done with the write request, can accept another request now
        VPLMutex_Lock(&mutex);
        write_req.active = false;
        VPLMutex_Unlock(&mutex);

        // Send back response
        write_resp_pkt = new (std::nothrow) ts_ext_write_resp_pkt(xid, err);
        if (write_resp_pkt == NULL) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate write resp");
            err = TS_ERR_NO_MEM;
            goto done;
        }

        err = send_pkt(write_resp_pkt);
        delete write_resp_pkt;
        write_resp_pkt = NULL;

        if (err != TS_OK) {
            goto done;
        }
    }

done:
    force_close();
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "ts_ext_conn writer() ends");
}

// Read from tunnel and write received bytes to ts ext client as data pkt
void ts_ext_conn::reader(void)
{
    TSError_t err = TS_OK;
    int rv;
    string error_msg;
    ts_ext_read_resp_pkt* read_resp_pkt = NULL;
    u32 xid;
    size_t max_data_len;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "ts_ext_conn reader() starts");

    while (true) {
        VPLMutex_Lock(&mutex);
        while (!is_closed && !read_req.active) {
            rv = VPLCond_TimedWait(&read_cond, &mutex, VPL_TIMEOUT_NONE);
            if (rv != VPL_OK) {
                VPLMutex_Unlock(&mutex);

                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "reader() failed timed wait, rv=%d", rv);
                goto done;
            }
        }
        VPLMutex_Unlock(&mutex);

        if (is_closed) {
            VPLTRACE_LOG_INFO(TRACE_BVS, 0, "reader() detected close");
            goto done;
        }

        err = TS::TS_Read(ioh, (char*) read_req.data, read_req.max_data_len, error_msg);
        if (err != TS_OK) {
            VPLTRACE_LOG_WARN(TRACE_BVS, 0, "TS_Read() failed, rv=%d:%s", err, error_msg.c_str());
        }

        xid = read_req.xid;
        if (err == TS_OK) {
            max_data_len = read_req.max_data_len;
        } else {
            max_data_len = 0;
        }

        // Ready for the next request
        VPLMutex_Lock(&mutex);
        read_req.active = false;
        VPLMutex_Unlock(&mutex);

        // Send back response
        read_resp_pkt = new (std::nothrow) ts_ext_read_resp_pkt(xid, err, (const char*) read_req.data, (u32) max_data_len);
        if (read_resp_pkt == NULL) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate read resp");
            err = TS_ERR_NO_MEM;
            goto done;
        }

        err = send_pkt(read_resp_pkt);
        delete read_resp_pkt;
        read_resp_pkt = NULL;

        if (err != TS_OK) {
            goto done;
        }
    }

done:
    force_close();
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "ts_ext_conn reader() ends");
}

TSError_t ts_ext_conn::send_sock(const char* buffer,
                                 size_t ubuf_len)
{
    TSError_t err = TS_OK;
    size_t sent, buf_len = ubuf_len;
    int xferred = 0;
    int rv;

    // Multiple threads can call send_to_sock(), but the write_mutex guarantees
    // only one thread can enter at a time.  If is_handle_down is already set,
    // error out here before the blocking VPLSocket_Poll() since another thread
    // may have already received the signal
    if (is_forcing_close) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0, "ts_ext_conn close detected");
        err = TS_ERR_CLOSED;
        goto done;
    }

    for (sent = 0; sent < buf_len; sent += xferred) {
        VPLSocket_poll_t pollspec[2];

        xferred = 0;

        pollspec[0].socket = server_write_signal_sockfd;
        pollspec[0].events = VPLSOCKET_POLL_RDNORM;

        pollspec[1].socket = sockfd;
        pollspec[1].events = VPLSOCKET_POLL_OUT;

        rv = VPLSocket_Poll(pollspec, 2, VPLTIME_FROM_SEC(TS_EXT_TIMEOUT_IN_SEC));
        if (rv == 0) {
            // This is unexpected.  The socket write should not block for an extended
            // period of time, as there can be only one outstanding TS_Read() or
            // TS_Write() at a time
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Poll() timed out");
            err = TS_ERR_INTERNAL;
            goto done;
        } else if (rv < 0) {
            switch (rv) {
            case VPL_ERR_AGAIN:
            case VPL_ERR_INTR: // Debugger interrupt.
                VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                                  "Poll() error: %d. Continuing.",
                                  rv);
                break;
            default: // Something unexpected. Crashing!
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "Poll() error: %d.",
                                 rv);
                err = TS_ERR_COMM;
                goto done;
            }
        } else if (rv > 0) {
            if ((pollspec[0].revents &
                (VPLSOCKET_POLL_ERR | VPLSOCKET_POLL_HUP | VPLSOCKET_POLL_EV_INVAL | VPLSOCKET_POLL_SO_INVAL)) ||
                (pollspec[1].revents &
                (VPLSOCKET_POLL_ERR | VPLSOCKET_POLL_HUP | VPLSOCKET_POLL_EV_INVAL | VPLSOCKET_POLL_SO_INVAL))) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Socket error, revents=0x"FMTx32, pollspec[0].revents);
                err = TS_ERR_COMM;
                goto done;
            }

            // Clean up signal
            if (pollspec[0].revents & VPLSOCKET_POLL_RDNORM) {
                char tmp;

                rv = VPLSocket_Recv(pollspec[0].socket, &tmp, 1);
                VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Cleaned up signal, rv=%d", rv);
            }

            // Check exit condition
            if (is_forcing_close) {
                VPLTRACE_LOG_INFO(TRACE_BVS, 0, "ts_ext_conn close detected");
                err = TS_ERR_CLOSED;
                goto done;
            }

            if (!(pollspec[1].revents & VPLSOCKET_POLL_OUT)) {
                continue;
            }

            xferred = VPLSocket_Send(sockfd, &buffer[sent], buf_len - sent); 
            if (xferred <= 0) {
                if (xferred == VPL_ERR_AGAIN) {
                    continue;
                }
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Send error, rv=%d", xferred);
                err = TS_ERR_COMM;
                goto done;
            }
        }
    }

done:
    return err;
}

// Send a packet from client ccd to ext client
TSError_t ts_ext_conn::send_pkt(ts_ext_pkt* pkt)
{
    TSError_t err = TS_OK;

    {
        // Packets consist of fixed len hdrs optionally followed by variable len data.
        //      ts_ext_pkt_hdr_t,
        //      [optional packet type specific header],
        //      [optional variable len data]
        // Most packet types don't have variable len data.
        // The max hdrs_len is 25 = (ts_ext_pkt_hdr_t(8) + ts_ext_open_req_pkt_t(17))
        // A data packet has hdrs_len 12 = (ts_ext_pkt_hdr_t(8) + ts_ext_data_pkt_t(4))

        const u8* pkt_hdrs;
        const u8* data; // optional variable len pkt data
        size_t hdrs_len;
        size_t data_len = 0;

        pkt->serialize(pkt_hdrs, (u32&) hdrs_len, data, (u32&) data_len);

        {
            // ts_ext_conn::send_pkt() is called from multiple threads,
            // so need to protect these two send_sock() calls
            MutexAutoLock lock(&write_mutex);

            // write the packet hdrs and data (if pkt type has any)
            err = send_sock((const char*) pkt_hdrs, hdrs_len);
            if (err != TS_OK) {
                goto done;
            }

            if (data_len != 0) {
                err = send_sock((const char*) data, data_len);
                if (err != TS_OK) {
                    goto done;
                }
            }
        }
    }

done:
    return err;
}

TSError_t ts_ext_conn::recv_sock(char* buffer,
                                 size_t& ubuf_len)
{
    TSError_t err = TS_OK;
    size_t can_recv;
    size_t recv_so_far = 0;
    int xferred = 0;
    int rv;

    for (;;) {
        VPLSocket_poll_t pollspec[2];

        pollspec[0].socket = server_read_signal_sockfd;
        pollspec[0].events = VPLSOCKET_POLL_RDNORM;

        pollspec[1].socket = sockfd;
        pollspec[1].events = VPLSOCKET_POLL_RDNORM;

        rv = VPLSocket_Poll(pollspec, 2, VPL_TIMEOUT_NONE);
        if (rv < 0) {
            switch (rv) {
            case VPL_ERR_AGAIN:
            case VPL_ERR_INTR: // Debugger interrupt.
                VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                                  "Poll() error: %d. Continuing.",
                                  rv);
                break;
            default: // Something unexpected. Crashing!
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "Poll() error: %d.",
                                 rv);
                err = TS_ERR_COMM;
                goto done;
            }
        } else if (rv > 0) {
            if ((pollspec[0].revents &
                (VPLSOCKET_POLL_ERR | VPLSOCKET_POLL_HUP | VPLSOCKET_POLL_EV_INVAL | VPLSOCKET_POLL_SO_INVAL)) ||
                (pollspec[1].revents &
                (VPLSOCKET_POLL_ERR | VPLSOCKET_POLL_HUP | VPLSOCKET_POLL_EV_INVAL | VPLSOCKET_POLL_SO_INVAL))) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Socket error, revents=0x"FMTx32, pollspec[0].revents);
                err = TS_ERR_COMM;
                goto done;
            }

            // Clean up signal
            if (pollspec[0].revents & VPLSOCKET_POLL_RDNORM) {
                char tmp;

                rv = VPLSocket_Recv(pollspec[0].socket, &tmp, 1);
                VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Cleaned up signal, rv=%d", rv);
            }

            // Check exit condition
            if (is_forcing_close) {
                VPLTRACE_LOG_INFO(TRACE_BVS, 0, "ts_ext_conn close detected");
                err = TS_ERR_CLOSED;
                goto done;
            }

            if (!(pollspec[1].revents & VPLSOCKET_POLL_RDNORM)) {
                continue;
            }

            can_recv = ubuf_len - recv_so_far;
            xferred = VPLSocket_Recv(sockfd, &buffer[recv_so_far], can_recv); 
            if (xferred <= 0) {
                if (xferred == VPL_ERR_AGAIN) {
                    continue;
                }
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Recv error, rv=%d\n", xferred);
                err = TS_ERR_COMM;
                goto done;
            }
            else {
                recv_so_far += xferred;
                if (recv_so_far == ubuf_len) {
                    break;
                }
            }
        }
    }

done:
    ubuf_len = recv_so_far;
    return err;
}

// Receive pkt hdrs sent from the ext client to the client ccd
TSError_t ts_ext_conn::recv_pkt_hdrs(ts_ext_pkt_hdr_t& pkt_hdr,
                                     char* pkt_type_hdr_buffer)
{
    TSError_t err = TS_OK;
    size_t recv_len = sizeof(ts_ext_pkt_hdr_t);

    // read tx_ext packet header
    err = recv_sock((char*)&pkt_hdr, recv_len);
    if (err != TS_OK) {
        goto done;
    }

    // Read the fixed length packet type header. For all types except
    // data pkt the packet type header contains all remaing packet data.
    // The close request packet has no packet type specfic data (i.e. recv_len is 0)
    err = ts_ext_pkt_type_hdr_len(pkt_hdr.pkt_type, recv_len);
    if (err != TS_OK) {
        goto done;
    }

    if (recv_len != 0) {
        err = recv_sock(pkt_type_hdr_buffer, recv_len);
        if (err != TS_OK) {
            goto done;
        }
    }

done:
    return err;
}

TSError_t ts_ext_conn::make_signal_connections(void)
{
    TSError_t err = TS_OK;
    int rv;
    int yes = 1;
    VPLSocket_addr_t sin;

    // For simplicity, make this a blocking socket
    client_read_signal_sockfd = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_STREAM, VPL_FALSE /* nonblock */);
    if (VPLSocket_Equal(client_read_signal_sockfd, VPLSOCKET_INVALID)) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLSocket_Create() failed");
        err = TS_ERR_COMM;
        goto done;
    }

    rv = VPLSocket_SetSockOpt(client_read_signal_sockfd, VPLSOCKET_IPPROTO_TCP,
                              VPLSOCKET_TCP_NODELAY, &yes, sizeof(int));
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLSocket_SetSockOpt() failed, rv=%d", rv);
        err = TS_ERR_COMM;
        goto done;
    }
 
    sin.family = VPL_PF_INET;
    sin.addr = VPLNET_ADDR_LOOPBACK;
    sin.port = signal_port;
    rv = VPLSocket_Connect(client_read_signal_sockfd, &sin, sizeof(sin));
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLSocket_Connect() failed, rv=%d", rv);
                         err = TS_ERR_COMM;
        goto done;
    }

    // Repeat for the write socket
    client_write_signal_sockfd = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_STREAM, VPL_FALSE /* nonblock */);
    if (VPLSocket_Equal(client_write_signal_sockfd, VPLSOCKET_INVALID)) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLSocket_Create() failed");
        err = TS_ERR_COMM;
        goto done;
    }

    rv = VPLSocket_SetSockOpt(client_write_signal_sockfd, VPLSOCKET_IPPROTO_TCP,
                              VPLSOCKET_TCP_NODELAY, &yes, sizeof(int));
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLSocket_SetSockOpt() failed, rv=%d", rv);
        err = TS_ERR_COMM;
        goto done;
    }

    sin.family = VPL_PF_INET;
    sin.addr = VPLNET_ADDR_LOOPBACK;
    sin.port = signal_port;
    rv = VPLSocket_Connect(client_write_signal_sockfd, &sin, sizeof(sin));
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLSocket_Connect() failed, rv=%d", rv);
                         err = TS_ERR_COMM;
        goto done;
    }

done:
    return err;
}

TSError_t ts_ext_conn::accept_signal_connections(void)
{
    TSError_t err = TS_OK;
    int rv;

    // Accept the read signal socket
    for (int i = 0 ; i < 200 ; i++) {
        rv = VPLSocket_Accept(signal_sockfd, NULL, 0, &server_read_signal_sockfd);
        if (rv == VPL_OK) {
            break;
        }

        if ((rv == VPL_ERR_AGAIN) || (rv == VPL_ERR_BADF)) {
            VPLThread_Sleep(VPLTime_FromMillisec(50));
            continue;
        }

        break;
    }

    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLSocket_Accept() for read signal socket failed, rv=%d", rv);
        err = TS_ERR_COMM;
        goto done;
    }

    // Repeat for the write signal socket
    for (int i = 0 ; i < 200 ; i++) {
        rv = VPLSocket_Accept(signal_sockfd, NULL, 0, &server_write_signal_sockfd);
        if (rv == VPL_OK) {
            break;
        }

        if ((rv == VPL_ERR_AGAIN) || (rv == VPL_ERR_BADF)) {
            VPLThread_Sleep(VPLTime_FromMillisec(50));
            continue;
        }

        break;
    }

    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLSocket_Accept() for write signal socket failed, rv=%d", rv);
        err = TS_ERR_COMM;
        goto done;
    }

done:
    return err;

}

TSError_t ts_ext_conn::send_signals(void)
{
    TSError_t err = TS_OK;
    int rv;

    // Signal the read socket
    while (true) {
        rv = VPLSocket_Write(client_read_signal_sockfd, " ", 1, VPLTIME_FROM_SEC(TS_EXT_TIMEOUT_IN_SEC));
        if (rv <= 0) {
            if (rv == VPL_ERR_AGAIN) {
                continue;
            }

            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLSocket_Write() failed, rv=%d", rv);
            err = TS_ERR_COMM;
            // Still try to signal the write socket
        }

        break;
    }

    // Repeat for the write socket
    while (true) {
        rv = VPLSocket_Write(client_write_signal_sockfd, " ", 1, VPLTIME_FROM_SEC(TS_EXT_TIMEOUT_IN_SEC));
        if (rv <= 0) {
            if (rv == VPL_ERR_AGAIN) {
                continue;
            }

            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLSocket_Write() failed, rv=%d", rv);
            err = TS_ERR_COMM;
            goto done;
        }

        break;
    }

done:
    return err;

}
