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
/// ts_ext_client.cpp
///
/// Tunnel Services External Client Support.


#include "vplu_types.h"
#include "ts_ext_client.hpp"
#include "ts_ext_pkt.hpp"
#include "vpl_th.h"
#include "vpl_lazy_init.h"
#include "vplex_trace.h"
#include "vpl_socket.h"
#include "vplex_socket.h"
#include "vpl_net.h"
#include "gvm_errors.h"
#include <stdio.h>
#include <sstream>
#include <ccdi.hpp>

using namespace std;


// Request types used by tse_request_t
enum {
    TS_EXT_REQ_OPEN,
    TS_EXT_REQ_CLOSE,
    TS_EXT_REQ_READ,
    TS_EXT_REQ_WRITE,
};

typedef struct tse_request_s {
    bool        done;
    TSError_t   error;
    u32         xid;
    u32         type;
    char*       data_buf;
    size_t      data_buf_len;
} tse_request_t;

class tse_handle {
public:
    tse_handle(u32 handle_id);
    ~tse_handle();

    TSError_t init(void);
    TSError_t shutdown(void);
    u32 get_handle_id(void);

    TSError_t open(u64 user_id, u64 device_id, const string& service_name);
    TSError_t close(void);
    TSError_t read(char* buf, size_t& len);
    TSError_t write(const char* buf, size_t len);

    void handle_resp(void);

private:
    TSError_t make_data_connection(void);
    TSError_t make_signal_connections(void);
    TSError_t accept_signal_connections(void);
    TSError_t send_signals(void);

    TSError_t queue_request(u32 req_type, char* data_buf, size_t data_buf_len, tse_request_t*& req_out);
    TSError_t write_pkt(ts_ext_pkt* pkt);
    TSError_t write_to_sock(const char* buf, size_t len);

    TSError_t read_pkt_hdrs(ts_ext_pkt_hdr_t* pkt_hdr,
                            char* pkt_type_hdr_buffer);
    TSError_t read_from_sock(char* buffer,
                             size_t& buffer_len);

    // is_handle_down is used to signal Poll() to stop looping
    volatile bool               is_handle_down;
    volatile bool               is_reader_out;
    u32                         handle_id;
    u32                         xid_next;
    VPLSocket_t                 sockfd;
    map<u32, tse_request_t*>    req_map;

    // Use signal connections to unblock VPLSocket_Poll()
    VPLNet_port_t               signal_port;
    VPLSocket_t                 signal_sockfd;
    VPLSocket_t                 client_read_signal_sockfd;
    VPLSocket_t                 client_write_signal_sockfd;
    VPLSocket_t                 server_read_signal_sockfd;
    VPLSocket_t                 server_write_signal_sockfd;

    // After connecting client_write_signal_sockfd, needs to wait until the accept
    // is done so that write_to_sock() can poll on a valid server_write_signal_sockfd
    VPLCond_t                   accept_cond;
    volatile bool               is_signal_conn_accepted;

    // write_mutex is dedicated to blocking writes.  The rule is that while holding
    // the general mutex, one should not try to acquire the write_mutex as well to
    // to prevent deadlock

    VPLMutex_t                  mutex;
    VPLMutex_t                  write_mutex;    // Separate mutex for blocking write
    VPLCond_t                   open_cond;
    VPLCond_t                   close_cond;
    VPLCond_t                   read_cond;
    VPLCond_t                   write_cond;
    VPLThread_t                 worker_thread;
    volatile bool               is_worker_thread_started;
};

typedef struct tse_href_s {
    tse_handle*             handle;
    u32                     ref_cnt;
} tse_href_t;

class tse_pool {
public:
    tse_pool();
    ~tse_pool();

    TSError_t init(void);
    TSError_t shutdown(void);

    tse_handle* handle_get(void);
    tse_handle* handle_get(u32 handle_id);
    void handle_put(tse_handle*& handle);

private:
    volatile bool           is_pool_down;
    u32                     handle_id_next;
    map<u32, tse_href_t*>   handle_map;

    VPLMutex_t              mutex;
    VPLCond_t               cond;
};


static const u32 TS_EXT_WORKER_STACK_SIZE = (192 * 1024);

static volatile bool is_tse_init = false;
static VPLLazyInitMutex_t tse_mutex = VPLLAZYINITMUTEX_INIT;
static VPLCond_t tse_cond;
static volatile u32 pool_ref_cnt = 0;
static tse_pool* pool = NULL;

static VPLThread_return_t worker_thread_start(VPLThread_arg_t handlev)
{
    tse_handle* handle = (tse_handle*) handlev;

    handle->handle_resp();

    return VPLTHREAD_RETURN_VALUE_UNUSED;
}
 
       
namespace TS_EXT {

TSError_t TS_Init(string& error_msg)
{
    TSError_t err = TS_OK;
    int rv;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Calling TS_Init");

    error_msg.clear();

    if (is_tse_init) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0, "TS-EXT library already initialized");
        goto done;
    }

    VPL_SET_UNINITIALIZED(&tse_cond);
    if ((rv = VPLCond_Init(&tse_cond)) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to initialize cond");
        err = TS_ERR_NO_MEM;
        error_msg = "Failed to initialize cond";
        goto done;
    }

    pool = new (std::nothrow) tse_pool;
    if (pool == NULL) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0, "Unable to allocate tse_pool");
        err = TS_ERR_NO_MEM;
        error_msg = "Unable to allocate tse_pool";
        goto done;
    }

    err = pool->init();
    if (err != TS_OK) {
        error_msg = "Unable to initialize tse_pool";
        goto done;
    }

    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&tse_mutex));
    pool_ref_cnt = 0;
    is_tse_init = true;
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&tse_mutex));

done:
    if (err != TS_OK) {
        if (pool != NULL) {
            delete pool;
        }

        if (VPL_IS_INITIALIZED(&tse_cond)) {
            VPLCond_Destroy(&tse_cond);
        }
    }

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "TS_Init() done");
    return err;
}

void TS_Shutdown(void)
{
    TSError_t err = TS_OK;
    int rv;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Calling TS_Shutdown");

    if (!is_tse_init) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0, "TS-EXT library not initialized");
        err = TS_ERR_NOT_INIT;
        goto done;
    }

    pool->shutdown();

    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&tse_mutex));
    is_tse_init = false;
    while (pool_ref_cnt > 0) {
        rv = VPLCond_TimedWait(&tse_cond, VPLLazyInitMutex_GetMutex(&tse_mutex), VPLTIME_FROM_SEC(TS_EXT_TIMEOUT_IN_SEC));
        if (rv != VPL_OK) {
            // This is unexpected, just break the loop and continue the shutdown.  If
            // it crashes, at least we have a log statement
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Pool shutdown failed timed wait, rv=%d", rv);
            break;
        }
    }
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&tse_mutex));

    delete pool;

    if (VPL_IS_INITIALIZED(&tse_cond)) {
        VPLCond_Destroy(&tse_cond);
    }

done:
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "TS_Shutdown() done");
    return;
}

TSError_t TS_Open(const TSOpenParms_t& parms,
                  TSIOHandle_t& ret_io_handle,
                  string& error_msg)
{
    TSError_t err = TS_OK;
    int rv;
    tse_handle* handle = NULL;
    bool need_decrement = false;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Calling TS_Open() "FMTu64":"FMTu64":%s",
                      parms.user_id, parms.device_id, parms.service_name.c_str());

    error_msg.clear();

    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&tse_mutex));
    if (!is_tse_init) {
        VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&tse_mutex));

        VPLTRACE_LOG_WARN(TRACE_BVS, 0, "TS-EXT library not initialized");
        err = TS_ERR_NOT_INIT;
        error_msg = "TS-EXT library not initialized";
        goto done;
    }
    pool_ref_cnt++;
    need_decrement = true;
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&tse_mutex));

    handle = pool->handle_get();
    if (handle == NULL) {
        err = TS_ERR_NO_MEM;
        error_msg = "Unable to create tse_handle";
        goto done;
    }
    ret_io_handle = (void*) handle->get_handle_id();
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "TS_Open() handle "FMTu32, (u32) ret_io_handle);

    err = handle->open(parms.user_id, parms.device_id, parms.service_name);
    if (err != TS_OK) {
        error_msg = "Failed tse_handle open request";
        goto done;
    }

done:
    if (err != TS_OK) {
        if (handle != NULL) {
            pool->handle_put(handle);
        }
        ret_io_handle = NULL;
    }

    if (need_decrement) {
        VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&tse_mutex));
        pool_ref_cnt--;
        if (pool_ref_cnt == 0) {
            rv = VPLCond_Signal(&tse_cond);
            if (rv != VPL_OK) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to signal, rv=%d", rv);
            }
        }
        VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&tse_mutex));
    }

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "TS_Open() done");
    return err;
}

TSError_t TS_Close(TSIOHandle_t& io_handle,
                   string& error_msg)
{
    TSError_t err = TS_OK;
    int rv;
    tse_handle* handle = NULL;
    bool need_decrement = false;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Calling TS_Close() with handle "FMTu32, (u32) io_handle);

    error_msg.clear();

    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&tse_mutex));
    if (!is_tse_init) {
        VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&tse_mutex));

        VPLTRACE_LOG_WARN(TRACE_BVS, 0, "TS-EXT library not initialized");
        err = TS_ERR_NOT_INIT;
        error_msg = "TS-EXT library not initialized";
        goto done;
    }
    pool_ref_cnt++;
    need_decrement = true;
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&tse_mutex));

    handle = pool->handle_get((u32) io_handle);
    if (handle == NULL) {
        err = TS_ERR_BAD_HANDLE;
        error_msg = "Unable to get tse_handle reference";
        goto done;
    }

    err = handle->close();
    if (err != TS_OK) {
        error_msg = "Failed tse_handle close request";
        goto done;
    }

done:
    if (handle != NULL) {
        // Double handle_put() here, one to free the handle_get() obtained here,
        // and the other to free the handle_get() obtained in TS_Open()
        pool->handle_put(handle);
        pool->handle_put(handle);
        io_handle = NULL;
    }

    if (need_decrement) {
        VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&tse_mutex));
        pool_ref_cnt--;
        if (pool_ref_cnt == 0) {
            rv = VPLCond_Signal(&tse_cond);
            if (rv != VPL_OK) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to signal, rv=%d", rv);
            }
        }
        VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&tse_mutex));
    }

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "TS_Close() done");
    return err;
}

TSError_t TS_Read(TSIOHandle_t io_handle,
                  char* buffer,
                  size_t& buffer_len,
                  string& error_msg)
{
    TSError_t err = TS_OK;
    int rv;
    tse_handle* handle = NULL;
    bool need_decrement = false;

    VPLTRACE_LOG_FINE(TRACE_BVS, 0, "Calling TS_Read() with handle "FMTu32, (u32) io_handle);

    error_msg.clear();

    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&tse_mutex));
    if (!is_tse_init) {
        VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&tse_mutex));

        VPLTRACE_LOG_WARN(TRACE_BVS, 0, "TS-EXT library not initialized");
        err = TS_ERR_NOT_INIT;
        error_msg = "TS-EXT library not initialized";
        goto done;
    }
    pool_ref_cnt++;
    need_decrement = true;
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&tse_mutex));

    handle = pool->handle_get((u32) io_handle);
    if (handle == NULL) {
        err = TS_ERR_BAD_HANDLE;
        error_msg = "Unable to get tse_handle reference";
        goto done;
    }

    err = handle->read(buffer, buffer_len);
    if (err != TS_OK) {
        error_msg = "Failed tse_handle read request";
        goto done;
    }

done:
    if (handle != NULL) {
        pool->handle_put(handle);
    }

    if (need_decrement) {
        VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&tse_mutex));
        pool_ref_cnt--;
        if (pool_ref_cnt == 0) {
            rv = VPLCond_Signal(&tse_cond);
            if (rv != VPL_OK) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to signal, rv=%d", rv);
            }
        }
        VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&tse_mutex));
    }

    VPLTRACE_LOG_FINE(TRACE_BVS, 0, "TS_Read() done");
    return err;
}

TSError_t TS_Write(TSIOHandle_t io_handle,
                   const char* buffer,
                   size_t buffer_len,
                   string& error_msg)
{
    TSError_t err = TS_OK;
    int rv;
    tse_handle* handle = NULL;
    bool need_decrement = false;
    size_t written = 0, nbytes;

    VPLTRACE_LOG_FINE(TRACE_BVS, 0, "Calling TS_Write() with handle "FMTu32, (u32) io_handle);

    error_msg.clear();

    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&tse_mutex));
    if (!is_tse_init) {
        VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&tse_mutex));

        VPLTRACE_LOG_WARN(TRACE_BVS, 0, "TS-EXT library not initialized");
        err = TS_ERR_NOT_INIT;
        error_msg = "TS-EXT library not initialized";
        goto done;
    }
    pool_ref_cnt++;
    need_decrement = true;
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&tse_mutex));

    handle = pool->handle_get((u32) io_handle);
    if (handle == NULL) {
        err = TS_ERR_BAD_HANDLE;
        error_msg = "Unable to get tse_handle reference";
        goto done;
    }

    while (written < buffer_len) {
        nbytes = buffer_len - written;
        if (nbytes > TS_EXT_WRITE_MAX_BUF_SIZE) {
            nbytes = TS_EXT_WRITE_MAX_BUF_SIZE;
        }

        err = handle->write(&buffer[written], nbytes);
        if (err != TS_OK) {
            error_msg = "Failed tse_handle write request";
            goto done;
        }

        written += nbytes;
    }

done:
    if (handle != NULL) {
        pool->handle_put(handle);
    }

    if (need_decrement) {
        VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&tse_mutex));
        pool_ref_cnt--;
        if (pool_ref_cnt == 0) {
            rv = VPLCond_Signal(&tse_cond);
            if (rv != VPL_OK) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to signal, rv=%d", rv);
            }
        }
        VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&tse_mutex));
    }

    VPLTRACE_LOG_FINE(TRACE_BVS, 0, "TS_Write() done");
    return err;
}
} // TS_EXT guard


tse_handle::tse_handle(u32 handle_id) :
    is_handle_down(false),
    is_reader_out(false),
    handle_id(handle_id),
    xid_next(0),
    sockfd(VPLSOCKET_INVALID),
    signal_port(0),
    signal_sockfd(VPLSOCKET_INVALID),
    client_read_signal_sockfd(VPLSOCKET_INVALID),
    client_write_signal_sockfd(VPLSOCKET_INVALID),
    server_read_signal_sockfd(VPLSOCKET_INVALID),
    server_write_signal_sockfd(VPLSOCKET_INVALID),
    is_signal_conn_accepted(false),
    is_worker_thread_started(false)
{
    VPL_SET_UNINITIALIZED(&accept_cond);
    VPL_SET_UNINITIALIZED(&mutex);
    VPL_SET_UNINITIALIZED(&write_mutex);
    VPL_SET_UNINITIALIZED(&open_cond);
    VPL_SET_UNINITIALIZED(&close_cond);
    VPL_SET_UNINITIALIZED(&read_cond);
    VPL_SET_UNINITIALIZED(&write_cond);
}

tse_handle::~tse_handle()
{
    map<u32, tse_request_t*>::iterator it;

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

    // This shouldn't actually happen, since when the handle can be
    // freed, all the requests should have been returned
    for (it = req_map.begin(); it != req_map.end(); req_map.erase(it++)) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Request map not empty");
        delete it->second;
    }

    if (VPL_IS_INITIALIZED(&accept_cond)) {
        VPLCond_Destroy(&accept_cond);
    }

    if (VPL_IS_INITIALIZED(&mutex)) {
        VPLMutex_Destroy(&mutex);
    }

    if (VPL_IS_INITIALIZED(&write_mutex)) {
        VPLMutex_Destroy(&write_mutex);
    }

    if (VPL_IS_INITIALIZED(&open_cond)) {
        VPLCond_Destroy(&open_cond);
    }

    if (VPL_IS_INITIALIZED(&close_cond)) {
        VPLCond_Destroy(&close_cond);
    }

    if (VPL_IS_INITIALIZED(&read_cond)) {
        VPLCond_Destroy(&read_cond);
    }

    if (VPL_IS_INITIALIZED(&write_cond)) {
        VPLCond_Destroy(&write_cond);
    }
}

TSError_t tse_handle::init(void)
{
    TSError_t err = TS_OK;
    int rv;
    int yes = 1;
    VPLThread_attr_t thread_attr;
    VPLThread_AttrInit(&thread_attr);
    VPLThread_AttrSetStackSize(&thread_attr, TS_EXT_WORKER_STACK_SIZE);
    VPLThread_AttrSetDetachState(&thread_attr, false);

    if ((rv = VPLCond_Init(&accept_cond)) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to initialize accept_cond, rv=%d", err);
        err = TS_ERR_NO_MEM;
        goto done;
    }

    if ((rv = VPLMutex_Init(&mutex)) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to initialize mutex, rv=%d", err);
        err = TS_ERR_NO_MEM;
        goto done;
    }

    if ((rv = VPLMutex_Init(&write_mutex)) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to initialize write mutex, rv=%d", err);
        err = TS_ERR_NO_MEM;
        goto done;
    }

    if ((rv = VPLCond_Init(&open_cond)) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to initialize open_cond, rv=%d", err);
        err = TS_ERR_NO_MEM;
        goto done;
    }

    if ((rv = VPLCond_Init(&close_cond)) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to initialize close_cond, rv=%d", err);
        err = TS_ERR_NO_MEM;
        goto done;
    }

    if ((rv = VPLCond_Init(&read_cond)) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to initialize read_cond, rv=%d", err);
        err = TS_ERR_NO_MEM;
        goto done;
    }

    if ((rv = VPLCond_Init(&write_cond)) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to initialize write_cond, rv=%d", err);
        err = TS_ERR_NO_MEM;
        goto done;
    }

    // Create data connection to TS-EXT server
    err = make_data_connection();
    if (err != TS_OK) {
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
    rv = VPLThread_Create(&worker_thread, worker_thread_start,
                          VPL_AS_THREAD_FUNC_ARG(this), &thread_attr,
                          "TS-EXT client worker thraed");
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to create worker thread");
        err = TS_ERR_NO_MEM;
        goto done;
    }
    is_worker_thread_started = true;

    // Try to connect to the signal port, and the worker thread will do the accept
    err = make_signal_connections();
    if (err != TS_OK) {
        goto done;
    }

done:
    if (err != TS_OK) {
        // No need to destroy mutexes/conds and close sockets because the destructor
        // will take care of them
        if (is_worker_thread_started) {
            // If this function fails with worker thread started, then it means
            // the connections couldn't be established, and the worker thread
            // should error out as well
            VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Handle init failed, trying to join worker thread");
            rv = VPLThread_Join(&worker_thread, NULL);
            if (rv != VPL_OK) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to join worker thread");
            }
        }
        is_worker_thread_started = false;
    }

    return err;
}

TSError_t tse_handle::shutdown(void)
{
    TSError_t err = TS_OK;
    int rv;
    bool do_join = false;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Shutting down handle");

    // Note that shutdown() could be called twice for the same handle, once by
    // TS_Shutdown(), and the other by TS_Close()
    //
    // shutdown() does not need to undo everything init() does, because the
    // destructor will handle most of it
    VPLMutex_Lock(&mutex);
    is_handle_down = true;

    if (is_worker_thread_started) {
        do_join = true;
    }
    is_worker_thread_started = false;
    VPLMutex_Unlock(&mutex);

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Sending signals");
    send_signals();

    if (do_join) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Joining worker thread");
        rv = VPLThread_Join(&worker_thread, NULL);
        if (rv != VPL_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to join worker thread");
        }
    }

    return err;
}

u32 tse_handle::get_handle_id(void)
{
    return handle_id;
}

TSError_t tse_handle::open(u64 user_id, u64 device_id, const string& service_name)
{
    TSError_t err = TS_OK;
    int rv;
    ts_ext_open_req_pkt* pkt = NULL;
    tse_request_t* req = NULL;
    bool queued = false;

    err = queue_request(TS_EXT_REQ_OPEN, NULL, 0, req);
    if (err != TS_OK) {
        goto done;
    }
    queued = true;

    // Now we have to perform the open req...
    pkt = new (std::nothrow) ts_ext_open_req_pkt(req->xid, user_id, device_id, service_name);
    if (pkt == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate packet");
        err = TS_ERR_NO_MEM;
        goto done;
    }

    err = write_pkt(pkt);
    if (err != TS_OK) {
        goto done;
    }

    VPLMutex_Lock(&mutex);

    while (!is_reader_out && !req->done) {
        rv = VPLCond_TimedWait(&open_cond, &mutex, VPLTIME_FROM_SEC(TS_EXT_TIMEOUT_IN_SEC));
        if (rv != VPL_OK) {
            VPLMutex_Unlock(&mutex);

            // This is unexpected.  Error out, but set req to NULL to avoid de-allocating
            // it.  The req_map will be cleaned in the destructor
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Operation failed timed wait, rv=%d", rv);
            err = TS_ERR_INTERNAL;
            req = NULL;
            goto done;
        }
    }

    if (req->done) {
        err = req->error;
    } else {
        err = TS_ERR_CLOSED;
    }
    req_map.erase(req->xid);
    queued = false;

    VPLMutex_Unlock(&mutex);

done:
    if (pkt != NULL) {
        delete pkt;
    }
    if (req != NULL) {
        if (queued) {
            VPLMutex_Lock(&mutex);
            req_map.erase(req->xid);
            VPLMutex_Unlock(&mutex);
        }
        delete req;
    }

    return err;
}

TSError_t tse_handle::close(void)
{
    TSError_t err = TS_OK;
    int rv;
    ts_ext_close_req_pkt* pkt = NULL;
    tse_request_t* req = NULL;
    bool queued = false;

    err = queue_request(TS_EXT_REQ_CLOSE, NULL, 0, req);
    if (err != TS_OK) {
        goto done;
    }
    queued = true;

    // Now we have to perform the close req...
    pkt = new (std::nothrow) ts_ext_close_req_pkt(req->xid);
    if (pkt == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate packet");
        err = TS_ERR_NO_MEM;
        goto done;
    }

    err = write_pkt(pkt);
    if (err != TS_OK) {
        goto done;
    }

    VPLMutex_Lock(&mutex);

    while (!is_reader_out && !req->done) {
        rv = VPLCond_TimedWait(&close_cond, &mutex, VPLTIME_FROM_SEC(TS_EXT_TIMEOUT_IN_SEC));
        if (rv != VPL_OK) {
            VPLMutex_Unlock(&mutex);

            // This is unexpected.  Error out, but set req to NULL to avoid de-allocating
            // it.  The req_map will be cleaned in the destructor
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Operation failed timed wait, rv=%d", rv);
            err = TS_ERR_INTERNAL;
            req = NULL;
            goto done;
        }
    }

    if (req->done) {
        err = req->error;
    } else {
        err = TS_ERR_CLOSED;
    }
    req_map.erase(req->xid);
    queued = false;

    VPLMutex_Unlock(&mutex);

done:
    if (pkt != NULL) {
        delete pkt;
    }
    if (req != NULL) {
        if (queued) {
            VPLMutex_Lock(&mutex);
            req_map.erase(req->xid);
            VPLMutex_Unlock(&mutex);
        }
        delete req;
    }

    return err;
}

TSError_t tse_handle::read(char* buf, size_t& len)
{
    TSError_t err = TS_OK;
    int rv;
    ts_ext_read_req_pkt* pkt = NULL;
    tse_request_t* req = NULL;
    bool queued = false;

    err = queue_request(TS_EXT_REQ_READ, buf, len, req);
    if (err != TS_OK) {
        goto done;
    }
    queued = true;

    // Now we have to perform the read req...
    pkt = new (std::nothrow) ts_ext_read_req_pkt(req->xid, (u32) len);
    if (pkt == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate packet");
        err = TS_ERR_NO_MEM;
        goto done;
    }

    err = write_pkt(pkt);
    if (err != TS_OK) {
        goto done;
    }

    VPLMutex_Lock(&mutex);

    while (!is_reader_out && !req->done) {
        rv = VPLCond_TimedWait(&read_cond, &mutex, VPLTIME_FROM_SEC(TS_EXT_TIMEOUT_IN_SEC));
        if (rv != VPL_OK) {
            VPLMutex_Unlock(&mutex);

            // This is unexpected.  Error out, but set req to NULL to avoid de-allocating
            // it.  The req_map will be cleaned in the destructor
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Operation failed timed wait, rv=%d", rv);
            err = TS_ERR_INTERNAL;
            req = NULL;
            goto done;
        }
    }

    if (req->done) {
        err = req->error;
        len = req->data_buf_len;   
    } else {
        err = TS_ERR_CLOSED;
    }
    req_map.erase(req->xid);
    queued = false;

    VPLMutex_Unlock(&mutex);

done:
    if (pkt != NULL) {
        delete pkt;
    }
    if (req != NULL) {
        if (queued) {
            VPLMutex_Lock(&mutex);
            req_map.erase(req->xid);
            VPLMutex_Unlock(&mutex);
        }
        delete req;
    }

    return err;
}

TSError_t tse_handle::write(const char* buf, size_t len)
{
    TSError_t err = TS_OK;
    int rv;
    ts_ext_write_req_pkt* pkt = NULL;
    tse_request_t* req = NULL;
    bool queued = false;

    err = queue_request(TS_EXT_REQ_WRITE, NULL, 0, req);
    if (err != TS_OK) {
        goto done;
    }
    queued = true;

    // Now we have to perform the write req...
    pkt = new (std::nothrow) ts_ext_write_req_pkt(req->xid, buf, (u32) len);
    if (pkt == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate packet");
        err = TS_ERR_NO_MEM;
        goto done;
    }

    err = write_pkt(pkt);
    if (err != TS_OK) {
        goto done;
    }

    VPLMutex_Lock(&mutex);

    while (!is_reader_out && !req->done) {
        rv = VPLCond_TimedWait(&write_cond, &mutex, VPLTIME_FROM_SEC(TS_EXT_TIMEOUT_IN_SEC));
        if (rv != VPL_OK) {
            VPLMutex_Unlock(&mutex);

            // This is unexpected.  Error out, but set req to NULL to avoid de-allocating
            // it.  The req_map will be cleaned in the destructor
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Operation failed timed wait, rv=%d", rv);
            err = TS_ERR_INTERNAL;
            req = NULL;
            goto done;
        }
    }

    if (req->done) {
        err = req->error;
    } else {
        err = TS_ERR_CLOSED;
    }
    req_map.erase(req->xid);
    queued = false;

    VPLMutex_Unlock(&mutex);

done:
    if (pkt != NULL) {
        delete pkt;
    }
    if (req != NULL) {
        if (queued) {
            VPLMutex_Lock(&mutex);
            req_map.erase(req->xid);
            VPLMutex_Unlock(&mutex);
        }
        delete req;
    }

    return err;
}

TSError_t tse_handle::make_data_connection(void)
{
    TSError_t err = TS_OK;
    CCDIError ccdi_rv;
    int rv;
    ccd::GetSystemStateInput gssInput;
    ccd::GetSystemStateOutput gssOutput;
    VPLSocket_addr_t sin;
    u32 ts_port;
    int yes = 1;

    // Get TS-EXT port of local CCD
    gssInput.set_get_network_info(true);
    ccdi_rv = CCDIGetSystemState(gssInput, gssOutput);
    if (ccdi_rv != CCD_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "FAIL:CCDIGetSystemState() %d", ccdi_rv);
        err = TS_ERR_COMM;
        goto done;
    }

    if (!gssOutput.has_network_info() ||
            !gssOutput.network_info().has_ext_tunnel_service_port()) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:CCDIGetSystemState() missing TS_EXT port");
        err = TS_ERR_COMM;
        goto done;
    }

    ts_port = gssOutput.network_info().ext_tunnel_service_port();
    VPLTRACE_LOG_INFO(TRACE_APP, 0, "ts port: "FMTu32, ts_port);

    // Connect to the TS-EXT server
    sockfd = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_STREAM, VPL_TRUE /* non-block */);
    if (VPLSocket_Equal(sockfd, VPLSOCKET_INVALID)) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLSocket_Create() failed");
        err = TS_ERR_COMM;
        goto done;
    }

    yes = 1;
    rv = VPLSocket_SetSockOpt(sockfd, VPLSOCKET_IPPROTO_TCP,
                              VPLSOCKET_TCP_NODELAY, &yes, sizeof(int));
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "Failed to set TCP_NODELAY, rv=%d", rv);
        err = TS_ERR_COMM;
        goto done;
    }

    // Use VPLSocket_Connect()'s default timeouts
    sin.family = VPL_PF_INET;
    sin.addr = VPLNET_ADDR_LOOPBACK;
    sin.port = ts_port;
    rv = VPLSocket_Connect(sockfd, &sin, sizeof(sin));
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLSocket_Connect failed, rv=%d", rv);
        err = TS_ERR_COMM;
        goto done;
    }

done:
    // No need to close socket as the destructor would handle it

    return err;
}

TSError_t tse_handle::make_signal_connections(void)
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

    VPLMutex_Lock(&mutex);
    while (!is_signal_conn_accepted) {
        rv = VPLCond_TimedWait(&accept_cond, &mutex, VPLTIME_FROM_SEC(TS_EXT_TIMEOUT_IN_SEC));
        if (rv != VPL_OK) {
            VPLMutex_Unlock(&mutex);

            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Connect() failed timed wait, rv=%d", rv);
            err = TS_ERR_INTERNAL;
            goto done;
        }
    }
    VPLMutex_Unlock(&mutex);

done:
    return err;
}

TSError_t tse_handle::accept_signal_connections(void)
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

    // Accepted signal connections
    VPLMutex_Lock(&mutex);
    is_signal_conn_accepted = true;
    rv = VPLCond_Signal(&accept_cond);
    if (rv != VPL_OK) {
        VPLMutex_Unlock(&mutex);

        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to signal, rv=%d", rv);
        err = TS_ERR_INTERNAL;
        goto done;
    }
    VPLMutex_Unlock(&mutex);

done:
    return err;

}

TSError_t tse_handle::send_signals(void)
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

TSError_t tse_handle::queue_request(u32 req_type, char *data_buf, size_t data_buf_len, tse_request_t*& req_out)
{
    TSError_t err = TS_OK;
    tse_request_t* req = NULL;

    req = new (std::nothrow) tse_request_t;
    if (req == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Unable to allocate request");
        err = TS_ERR_NO_MEM;
        goto done;
    }

    VPLMutex_Lock(&mutex);

    do {
        xid_next++;
        if (xid_next == 0) {
            xid_next = 1;
        }
    } while (req_map.find(xid_next) != req_map.end());

    req->done = false;
    req->error = TS_OK;
    req->xid = xid_next;
    req->type = req_type;
    req->data_buf = data_buf;
    req->data_buf_len = data_buf_len;

    req_map[req->xid] = req;
    req_out = req;

    VPLMutex_Unlock(&mutex);

done:
    if (err != TS_OK) {
        if (req != NULL) {
            delete req;
        }
    }

    return err;
}

// write_pkt() writes a pkt from an ext client to the local client ccd
TSError_t tse_handle::write_pkt(ts_ext_pkt* pkt)
{
    TSError_t err = TS_OK;
    const u8* pkt_hdrs;
    const u8* data; // optional variable len pkt data
    size_t hdrs_len;
    size_t data_len = 0;

    VPLMutex_Lock(&write_mutex);

    // Packets consist of fixed len hdrs optionally followed by variable len data.
    // Most packet types don't have variable len data.
    pkt->serialize(pkt_hdrs, (u32&) hdrs_len, data, (u32&) data_len);

    // write the packet hdrs and data (if pkt type has any)
    err = write_to_sock((const char*) pkt_hdrs, hdrs_len);
    if (err != TS_OK) {
        goto done;
    }

    if (data_len != 0) {
        err = write_to_sock((const char*) data, data_len);
        if (err != TS_OK) {
            goto done;
        }
    }

done:
    VPLMutex_Unlock(&write_mutex);

    return err;
}

TSError_t tse_handle::write_to_sock(const char* buffer,
                                    size_t buffer_len)
{
    TSError_t err = TS_OK;
    int rv;
    size_t sent = 0;
    int xferred = 0;

    // Multiple threads can call write_to_sock(), but the write_mutex guarantees
    // only one thread can enter at a time.  If is_handle_down is already set,
    // error out here before the blocking VPLSocket_Poll() since another thread
    // may have already received the signal
    if (is_handle_down) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Handle closed detected");
        err = TS_ERR_CLOSED;
        goto done;
    }

    for( ; ; ) {
        VPLSocket_poll_t pollspec[2];

        pollspec[0].socket = server_write_signal_sockfd;
        pollspec[0].events = VPLSOCKET_POLL_RDNORM;

        pollspec[1].socket = sockfd;
        pollspec[1].events = VPLSOCKET_POLL_OUT; 

        rv = VPLSocket_Poll(pollspec, 2, VPLTIME_FROM_SEC(TS_EXT_TIMEOUT_IN_SEC));
        if (rv == 0) {
            // This is unexpected.  The socket write should not block for an extended
            // period of time, as there can be only one outstanding TS_Read() or
            // TS_Write() at a time and there are dedicated threads on the ts_ext_server
            // for each of TS_Read() and TS_Write()
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
            default: // Something unexpected, exit
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
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Socket error, revents=0x"FMTx32":"FMTx32, pollspec[0].revents, pollspec[1].revents);
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
            if (is_handle_down) {
                VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Handle closed detected");
                err = TS_ERR_CLOSED;
                goto done;
            }

            if (!(pollspec[1].revents & VPLSOCKET_POLL_OUT)) {
                continue;
            }

            // If pollspec[1] is set to POLL_OUT, then try to send
            xferred = VPLSocket_Send(sockfd, &buffer[sent], buffer_len - sent);
            if (xferred <= 0) {
                if (xferred == VPL_ERR_AGAIN) {
                    continue;
                }
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Send error, rv=%d", xferred);
                err = TS_ERR_COMM;
                goto done;
            } else {
                sent += xferred;
                if (sent == buffer_len) {
                    break;
                }
            }
        }
    }

done:
    return err;
}

void tse_handle::handle_resp(void)
{
    TSError_t err = TS_OK;
    int rv;
    ts_ext_pkt_hdr_t pkt_hdr;
    char  pkt_type_hdr_buffer[MAX_TS_EXT_PKT_TYPE_HDR_SIZE];
    char* pkt_type_hdr = pkt_type_hdr_buffer;
    map<u32, tse_request_t*>::iterator it;
    tse_request_t* req = NULL;
    ts_ext_read_resp_pkt_t* read_resp_pkt = NULL;
    size_t read_data_len;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "tse_handle handle_resp() starts");

    // Accept the signal connections
    err = accept_signal_connections();
    if (err != TS_OK) {
        goto done;
    }

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Accepted signal connections");

    while (true) {
        err = read_pkt_hdrs(&pkt_hdr, pkt_type_hdr);
        if (err != TS_OK) {
            goto done;
        }

        VPLMutex_Lock(&mutex);
        it = req_map.find(VPLConv_ntoh_u32(pkt_hdr.xid));
        if (it != req_map.end()) {
            // The req entry will be removed by the request function.  Note that
            // there's no reference count around "req", so only this thread should
            // set req->done or is_reader_out to free the requester
            req = it->second;
        } else {
            // Shouldn't happen in real life
            VPLMutex_Unlock(&mutex);

            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Unexpected xid:" FMTu32, VPLConv_ntoh_u32(pkt_hdr.xid));
            err = TS_ERR_INTERNAL;
            goto done;
        }
        VPLMutex_Unlock(&mutex);

        switch (pkt_hdr.pkt_type) {
        case TS_EXT_PKT_OPEN_RESP:
            VPLMutex_Lock(&mutex);
            req->error = VPLConv_ntoh_s32(((ts_ext_open_resp_pkt_t*) pkt_type_hdr)->error);
            req->done = true;
            rv = VPLCond_Signal(&open_cond);
            if (rv != VPL_OK) {
                VPLMutex_Unlock(&mutex);

                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to signal, rv=%d", rv);
                err = TS_ERR_INTERNAL;
                goto done;
            }
            VPLMutex_Unlock(&mutex);
            break;
        case TS_EXT_PKT_CLOSE_RESP:
            VPLMutex_Lock(&mutex);
            req->error = VPLConv_ntoh_s32(((ts_ext_close_resp_pkt_t*) pkt_type_hdr)->error);
            req->done = true;
            rv = VPLCond_Signal(&close_cond);
            if (rv != VPL_OK) {
                VPLMutex_Unlock(&mutex);

                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to signal, rv=%d", rv);
                err = TS_ERR_INTERNAL;
                goto done;
            }
            VPLMutex_Unlock(&mutex);

            // After receiving close response, no need to continue;
            VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Received close resp");
            goto done;
        case TS_EXT_PKT_READ_RESP:
            read_resp_pkt = ((ts_ext_read_resp_pkt_t*) pkt_type_hdr);
            if (VPLConv_ntoh_s32(read_resp_pkt->error) != TS_OK) {
                VPLMutex_Lock(&mutex);
                req->error = VPLConv_ntoh_s32(read_resp_pkt->error);
                req->done = true;
                rv = VPLCond_Signal(&read_cond);
                if (rv != VPL_OK) {
                    VPLMutex_Unlock(&mutex);

                    VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to signal, rv=%d", rv);
                    err = TS_ERR_INTERNAL;
                    goto done;
                }
                VPLMutex_Unlock(&mutex);
                break;
            }

            read_data_len = VPLConv_ntoh_u32(pkt_hdr.pkt_len) - sizeof(ts_ext_pkt_hdr_t) - sizeof(ts_ext_read_resp_pkt_t);
            if (read_data_len > req->data_buf_len) {
                // This shouldn't happen in real life
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Read data length longer than acceptable: "FMTu_size_t":"FMTu_size_t, read_data_len, req->data_buf_len);
                err = TS_ERR_INTERNAL;
                goto done;
            }

            err = read_from_sock(req->data_buf, read_data_len);
            if (err != TS_OK) {
                goto done;
            }

            VPLMutex_Lock(&mutex);
            req->error = VPLConv_ntoh_s32(read_resp_pkt->error);
            req->data_buf_len = read_data_len;
            req->done = true;
            rv = VPLCond_Signal(&read_cond);
            if (rv != VPL_OK) {
                VPLMutex_Unlock(&mutex);

                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to signal, rv=%d", rv);
                err = TS_ERR_INTERNAL;
                goto done;
            }
            VPLMutex_Unlock(&mutex);
            break;
        case TS_EXT_PKT_WRITE_RESP:
            VPLMutex_Lock(&mutex);
            req->error = VPLConv_ntoh_s32(((ts_ext_write_resp_pkt_t*) pkt_type_hdr)->error);
            req->done = true;
            rv = VPLCond_Signal(&write_cond);
            if (rv != VPL_OK) {
                VPLMutex_Unlock(&mutex);

                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to signal, rv=%d", rv);
                err = TS_ERR_INTERNAL;
                goto done;
            }
            VPLMutex_Unlock(&mutex);
            break;
        default:
            // This shouldn't happen in real life
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Unexpected resp type: "FMTu32, pkt_hdr.pkt_type);
            err = TS_ERR_INTERNAL;
            goto done;
        }
    }

done:
    // Unblock everyone
    VPLMutex_Lock(&mutex);
    is_reader_out = true;

    rv = VPLCond_Broadcast(&open_cond);
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to signal, rv=%d", rv);
    }

    rv = VPLCond_Broadcast(&close_cond);
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to signal, rv=%d", rv);
    }

    rv = VPLCond_Broadcast(&read_cond);
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to signal, rv=%d", rv);
    }

    rv = VPLCond_Broadcast(&write_cond);
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to signal, rv=%d", rv);
    }

    VPLMutex_Unlock(&mutex);

    return;
}

// read_pkt_hdrs reads the pkt hdr and pkt type specific hdr sent from the client ccd to the ext client.
TSError_t tse_handle::read_pkt_hdrs(ts_ext_pkt_hdr_t* pkt_hdr,
                                    char* pkt_type_hdr_buffer)
{
    TSError_t err = TS_OK;
    size_t recv_len = sizeof(ts_ext_pkt_hdr_t);

    // Packets consist of fixed len hdrs optionally followed by variable len data.
    //      ts_ext_pkt_hdr_t,
    //      [optional packet type specific header],
    //      [optional variable len data]
    // Most packet types don't have variable len data.
    // The max hdrs_len is 25 = (ts_ext_pkt_hdr_t(8) + ts_ext_open_req_pkt_t(17))

    // read tx_ext packet header
    err = read_from_sock((char*)pkt_hdr, recv_len);
    if (err != TS_OK) {
        goto done;
    }

    // Read the fixed length packet type header. For all types except
    // data pkt the packet type header contains all remaing packet data.
    err = ts_ext_pkt_type_hdr_len(pkt_hdr->pkt_type, recv_len);
    if (err != TS_OK) {
        goto done;
    }

    if (recv_len) {
        err = read_from_sock(pkt_type_hdr_buffer, recv_len);
        if (err != TS_OK) {
            goto done;
        }
    }

done:
    return err;
}

TSError_t tse_handle::read_from_sock(char* buffer,
                                     size_t& buffer_len)
{
    TSError_t err = TS_OK;
    int xferred = 0;
    size_t can_recv;
    size_t recv_so_far = 0;
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
            default: // Something unexpected, exit
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
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Socket error, revents=0x"FMTx32":"FMTx32, pollspec[0].revents, pollspec[1].revents);
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
            if (is_handle_down) {
                VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Handle closed detected");
                err = TS_ERR_CLOSED;
                goto done;
            }

            if (!(pollspec[1].revents & VPLSOCKET_POLL_RDNORM)) {
                continue;
            }

            // If pollspec[1] is set to POLL_RDNORM, then try to read
            can_recv = buffer_len - recv_so_far;
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
                if (recv_so_far == buffer_len) {
                    break;
                }
            }
        }
    }

done:
    buffer_len = recv_so_far;
    return err;
}


tse_pool::tse_pool() :
    is_pool_down(false),
    handle_id_next(0)
{
    VPL_SET_UNINITIALIZED(&mutex);
    VPL_SET_UNINITIALIZED(&cond);
}

tse_pool::~tse_pool()
{
    map<u32, tse_href_t*>::iterator it;

    // It's possible to have handles left in the handle map if
    // the caller does not call TS_Close() before calling
    // TS_Shutdown().  Clear them all here
    for (it = handle_map.begin(); it != handle_map.end(); handle_map.erase(it++)) {
        if (it->second->ref_cnt != 1) {
            // This is unexpected
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "handle has a ref_cnt of more than 1 during clean-up: "FMTu32, it->second->ref_cnt);
        }
        VPLTRACE_LOG_WARN(TRACE_BVS, 0, "Kill outstanding handle: "FMTu32, it->second->handle->get_handle_id());

        delete(it->second->handle);
        delete(it->second);
    }

    if (VPL_IS_INITIALIZED(&mutex)) {
        VPLMutex_Destroy(&mutex);
    }

    if (VPL_IS_INITIALIZED(&cond)) {
        VPLCond_Destroy(&cond);
    }
}

TSError_t tse_pool::init(void)
{
    TSError_t err = TS_OK;
    int rv;

    if ((rv = VPLMutex_Init(&mutex)) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to initialize mutex, rv=%d", rv);
        err = TS_ERR_NO_MEM;
        goto done;
    }

    if ((rv = VPLCond_Init(&cond)) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to initialize cond, rv=%d", rv);
        VPLMutex_Destroy(&mutex);
        err = TS_ERR_NO_MEM;
        goto done;
    }

done:
    return err;
}

TSError_t tse_pool::shutdown(void)
{
    TSError_t err = TS_OK;
    map<u32, tse_href_t*>::iterator it, it_next;
    bool reached_end = false;
    u32 next_handle_id = 0;
    tse_handle* handle = NULL;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Shutting down pool");

    VPLMutex_Lock(&mutex);

    // After is_pool_down is set, no more new handles can be added to the map
    is_pool_down = true;

    for (it = handle_map.begin(); it != handle_map.end(); ) {
        // Since we need to unlock, it's tricky to get this right since other threads
        // could cause handles to be removed from the handle map, including the current
        // one and the one next to it

        it_next = it;
        it_next++;
        if (it_next == handle_map.end()) {
            // If end is reached, can exit the loop after this iteration
            reached_end = true;
        } else {
            // Store next handle ID to check whether it's removed
            next_handle_id = it_next->second->handle->get_handle_id();
        }

        VPLMutex_Unlock(&mutex);

        // Need to call handle_get to increment the reference count before calling
        // handle->shutdown()
        handle = handle_get(it->second->handle->get_handle_id());
        if (handle != NULL) {
            it->second->handle->shutdown();
            handle_put(it->second->handle);
        } else {
            // Handle already freed, nothing to do
        }

        VPLMutex_Lock(&mutex);

        if (reached_end) {
            break;
        }

        // Check whether the next handle is still valid
        it = handle_map.find(next_handle_id);
        if (it == handle_map.end()) {
            // Handle already freed, start from the beginning.  This should not
            // result in an infinite loop because the map is getting smaller.
            // Also, it's okay to call shutdown() on the same handle multiple times
            VPLTRACE_LOG_WARN(TRACE_BVS, 0, "Re-start handle map shutdown from the beginning");
            it = handle_map.begin();
        }
    }

    VPLMutex_Unlock(&mutex);

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Pool shutdown done");

    return err;
}

tse_handle* tse_pool::handle_get(void)
{
    TSError_t err = TS_OK;
    tse_handle* handle = NULL;
    tse_href_t *href = NULL;
    bool initialized = false;
    u32 new_handle_id;

    VPLMutex_Lock(&mutex);

    if (is_pool_down) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0, "Pool already down");
        err = TS_ERR_NOT_INIT;
        goto unlock;
    }

    // Find a new handle_id
    do {
        map<u32, tse_href_t*>::iterator it;

        handle_id_next++;
        if (handle_id_next == 0) {
            handle_id_next++;
        }
    } while (handle_map.find(handle_id_next) != handle_map.end());
    new_handle_id = handle_id_next;

    handle = new (std::nothrow) tse_handle(new_handle_id);
    if (handle == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate handle");
        err = TS_ERR_NO_MEM;
        goto unlock;
    }

    href = new (std::nothrow) tse_href_t;
    if (href == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate handle reference");
        err = TS_ERR_NO_MEM;
        goto unlock;
    }

    href->handle = handle;
    href->ref_cnt = 1;

    // Don't want to call handle->init() with lock held because it tries to make
    // a connection to the TS-EXT server
    VPLMutex_Unlock(&mutex);

    err = handle->init();
    if (err != TS_OK) {
        goto done;
    }
    initialized = true;

    VPLMutex_Lock(&mutex);

    // This case is very unlikely to happen
    if (handle_map.find(new_handle_id) != handle_map.end()) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "handle_id re-used unexpectedly "FMTu32, new_handle_id);
        err = TS_ERR_INTERNAL;
        goto unlock;
    }
    handle_map[new_handle_id] = href;

unlock:
    VPLMutex_Unlock(&mutex);

done:
    if (err != TS_OK) {
        if (href) {
            delete href;
            href = NULL;
        }

        if (handle) {
            if (initialized) {
                handle->shutdown();
            }

            delete handle;
            handle = NULL;
        }

    }

    return handle;
}

tse_handle* tse_pool::handle_get(u32 handle_id)
{
    map<u32, tse_href_t*>::iterator it;
    tse_handle* handle = NULL;

    VPLMutex_Lock(&mutex);
    it = handle_map.find(handle_id);
    if (it != handle_map.end()) {
        it->second->ref_cnt++;
        handle = it->second->handle;
    }
    VPLMutex_Unlock(&mutex);

    return handle;
}

void tse_pool::handle_put(tse_handle*& handle)
{
    map<u32, tse_href_t*>::iterator it;
    tse_href_t* href = NULL;
    bool is_free = false;

    VPLMutex_Lock(&mutex);
    it = handle_map.find(handle->get_handle_id());
    if (it == handle_map.end()) {
        goto done;
    }

    href = it->second;
    href->ref_cnt--;
    if (href->ref_cnt != 0) {
        goto done;
    }

    /* Delete handle if its reference count reaches 0 */
    handle_map.erase(it);
    is_free = true;

done:
    VPLMutex_Unlock(&mutex);

    if (is_free) {
        href->handle->shutdown();
        delete href->handle;
        delete href;
        handle = NULL;
    }

    return;
}
