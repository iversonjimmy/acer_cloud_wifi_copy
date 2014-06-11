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


#include "vpl_lazy_init.h"
#include "vssts_srvr.hpp"

#include <list> 

using namespace std;

#define VSSTS_SRVR_STOP_TIMEOUT_IN_SEC      (60)


class vssts_svc {
public:
    vssts_svc(u32 vssts_id, u64 user_id, u64 device_id, TSIOHandle_t ioh);
    ~vssts_svc();

    u32 get_id(void) {return vssts_id;};

    void start(void);
    void stop(void);
    void reader(void);
    void writer(void);

    void put_response(char* resp);

private:
    TSError_t read_msg(vss_req_proc_ctx* req, string& error_msg);
    TSError_t read_cnt(char* buf, size_t buf_size, string& error_msg);

    u32                         vssts_id;
    u64                         user_id;
    u64                         device_id;
    TSIOHandle_t                ioh;

    volatile bool               is_writer_stop;
    VPLThread_t                 thread;
    VPLMutex_t                  mutex;
    VPLCond_t                   cond;

    list<char*>                 response_list;
    VPLTime_t                   last_read;
};

typedef struct vssts_sref_s {
    u32             ref_cnt;
    vssts_svc*      svc;
} vssts_sref_t;

class vssts_svc_pool {
public:
    vssts_svc_pool();
    ~vssts_svc_pool();

    TSError_t init(void);
    TSError_t stop(void);

    vssts_svc* svc_get(u64 user_id, u64 device_id, TSIOHandle_t ioh);
    vssts_svc* svc_get(u32 vssts_id);
    void svc_put(vssts_svc*& svc);

private:
    u32                     vssts_id_next;
    VPLMutex_t              mutex;
    map<u32,vssts_sref_t*>  svc_map;
};


static volatile bool is_init = false;
static TSServiceHandle_t svc_handle;
static vss_server* server = NULL;
static vssts_svc_pool* pool = NULL;
static vss_session* vssts_session = NULL;

static volatile bool is_stopping = true;
static VPLLazyInitMutex_t stop_ctrl_mutex = VPLLAZYINITMUTEX_INIT;
static VPLCond_t stop_ctrl_cond;
static u32 handler_ref_cnt = 0;

vssts_svc_pool::vssts_svc_pool() :
    vssts_id_next(0)
{
    VPL_SET_UNINITIALIZED(&mutex);
}

vssts_svc_pool::~vssts_svc_pool()
{
    if (VPL_IS_INITIALIZED(&mutex)) {
        VPLMutex_Destroy(&mutex);
    }
}

TSError_t vssts_svc_pool::init(void)
{
    TSError_t err = TS_OK;
    int rv;

    if ((rv = VPLMutex_Init(&mutex)) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to initialize mutex, rv=%d\n", rv);
        err = TS_ERR_NO_MEM;
        goto done;
    }

done:
    return err;
}

TSError_t vssts_svc_pool::stop(void)
{
    TSError_t err = TS_OK;
    map<u32, vssts_sref_t*>::iterator it;

    // vssts_svc::stop() could potentially block, so it's undesirable to hold the
    // vssts_svc_pool lock while calling it.  However, current usage shouldn't
    // result in any deadlock, so the following code should be fine
    VPLMutex_Lock(&mutex);
    for (it = svc_map.begin(); it != svc_map.end(); it++) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Forcing service %p to stop", it->second->svc);
        it->second->svc->stop();
    }
    VPLMutex_Unlock(&mutex);

    return err;
}

vssts_svc* vssts_svc_pool::svc_get(u64 user_id, u64 device_id, TSIOHandle_t ioh)
{
    TSError_t err = TS_OK;
    vssts_sref_t* sref = NULL;

    VPLMutex_Lock(&mutex);
    do {
        vssts_id_next++;
        if ( vssts_id_next == 0 ) {
            vssts_id_next = 1;
        }
    } while (svc_map.find(vssts_id_next) != svc_map.end());

    sref = new (std::nothrow) vssts_sref_t;
    if (sref == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate sref");
        goto done;
    }

    sref->svc = new (std::nothrow) vssts_svc(vssts_id_next, user_id, device_id, ioh);
    if (sref->svc == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate svc");
        goto done;
    }
    sref->ref_cnt = 1;

    svc_map[vssts_id_next] = sref;

done:
    VPLMutex_Unlock(&mutex);

    if (err != TS_OK) {
        if (sref->svc != NULL) {
            delete sref->svc;
        }
        if (sref != NULL) {
            delete sref;
        }

        return NULL;
    } else {
        return sref->svc;
    }
}

vssts_svc* vssts_svc_pool::svc_get(u32 vssts_id)
{
    map<u32,vssts_sref_t*>::iterator it;
    vssts_svc* svc = NULL;

    VPLMutex_Lock(&mutex);
    it = svc_map.find(vssts_id);
    if (it == svc_map.end()) {
        goto done;
    }

    it->second->ref_cnt++;
    svc = it->second->svc;

done:
    VPLMutex_Unlock(&mutex);

    return svc;
}

void vssts_svc_pool::svc_put(vssts_svc*& svc)
{
    map<u32,vssts_sref_t*>::iterator it;
    vssts_sref_t* sref = NULL;

    VPLMutex_Lock(&mutex);
    it = svc_map.find(svc->get_id());
    if (it == svc_map.end()) {
        goto done;
    }

    sref = it->second;
    sref->ref_cnt--;
    if (sref->ref_cnt > 0) {
        goto done;
    }

    svc_map.erase(it);
    delete sref;
    delete svc;

done:
    VPLMutex_Unlock(&mutex);

    svc = NULL;
}


static VPLThread_return_t vssts_writer_start(VPLThread_arg_t vssts_svcv)
{
    vssts_svc* svc = (vssts_svc*)vssts_svcv;

    svc->writer();

    return VPLTHREAD_RETURN_VALUE_UNUSED;
}

vssts_svc::vssts_svc(u32 vssts_id, u64 user_id, u64 device_id, TSIOHandle_t ioh) :
    vssts_id(vssts_id),
    user_id(user_id),
    device_id(device_id),
    ioh(ioh),
    is_writer_stop(false)
{
    VPL_SET_UNINITIALIZED(&mutex);
    VPL_SET_UNINITIALIZED(&cond);
}

vssts_svc::~vssts_svc()
{
    if (VPL_IS_INITIALIZED(&mutex)) {
        VPLMutex_Destroy(&mutex);
    }

    if (VPL_IS_INITIALIZED(&cond)) {
        VPLCond_Destroy(&cond);
    }

    // Free up any queued, unsent responses
    while (response_list.size() != 0) {
        free((void*) response_list.front());
        response_list.pop_front();
    }
}

TSError_t vssts_svc::read_cnt(char* buf, size_t buf_size, string& error_msg)
{
    TSError_t err = TS_OK;
    size_t want;
    size_t got = 0;
    VPLTime_t cur_time;

    while (got < buf_size) {
        want = buf_size - got;

        err = TS::TSS_Read(ioh, &buf[got], want, error_msg);
        if (err == TS_OK) {
            got += want;
            last_read = VPLTime_GetTimeStamp();
        } else if (err == TS_ERR_TIMEOUT) {
            cur_time = VPLTime_GetTimeStamp();
            if (VPLTime_DiffClamp(cur_time, last_read) >
                    VPLTime_FromSec(vss_server::VSSTS_SERVER_READ_TIMEOUT_SEC)) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Exceeded read timeout");
                err = TS_ERR_TIMEOUT;
                goto done;
            }

            err = TS_OK;
        } else {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "TSS_Read() failed, rv=%d\n", err);
            goto done;
        }
    }

done:
    return err;
}

TSError_t vssts_svc::read_msg(vss_req_proc_ctx* req, string& error_msg)
{
    TSError_t err = TS_OK;
    u32 body_len;

    err = read_cnt(req->header, VSS_HEADER_SIZE, error_msg);
    if (err != TS_OK) {
        goto done;
    }

    body_len = vss_get_data_length(req->header);
    req->body = (char*) calloc(body_len, 1);
    if (req->body == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate request body");
        err = TS_ERR_NO_MEM;
        goto done;
    }

    err = read_cnt(req->body, body_len, error_msg);
    if (err != TS_OK) {
        goto done;
    }

done:
    return err;
}

void vssts_svc::reader(void)
{
    TSError_t err = TS_OK;
    string error_msg;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Starting reader()");

    for( ;; ) {
        vss_req_proc_ctx* req;

        req = new (std::nothrow) vss_req_proc_ctx(NULL);
        if (req == NULL) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate vss_req_proc_ctx");
            err = TS_ERR_NO_MEM;
            goto done;
        }
        req->vssts_id = vssts_id;
        req->server = server;
        req->session = vssts_session;

        err = read_msg(req, error_msg);
        if (err != TS_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "read_msg() %d:%s",
                             err, error_msg.c_str());
            delete req;
            goto done;
        }

        // We pretend to be a client in order to integrate with the
        // old VSS server
        server->notifyReqReceived(req);
    }

done:
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Ending reader() %d", err);
    return;
}

void vssts_svc::put_response(char* resp)
{
    int rv;
    bool stop_needed = false;

    VPLMutex_Lock(&mutex);
    response_list.push_back(resp);

    rv = VPLCond_Signal(&cond);
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to signal, rv=%d", rv);
        stop_needed = true;
    }
    VPLMutex_Unlock(&mutex);

    if (stop_needed) {
        stop();
    }
}

void vssts_svc::writer(void)
{
    TSError_t err = TS_OK;
    int rv;
    string error_msg;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Starting writer()");

    for( ;; ) {
        char* response = NULL;

        // Look for work
        VPLMutex_Lock(&mutex);
        while (!is_writer_stop && (response_list.size() == 0)) {
            rv = VPLCond_TimedWait(&cond, &mutex, VPL_TIMEOUT_NONE);
            if (rv != VPL_OK) {
                VPLMutex_Unlock(&mutex);
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "writer() failed timed wait, rv=%d", rv);

                // Close the tunnel to force the TSS_Read() to error out
                stop();
                goto done;
            }
        }

        if (is_writer_stop) {
            VPLMutex_Unlock(&mutex);
            VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Writer stopped");
            goto done;
        }

        // Pull a response
        response = response_list.front();
        response_list.pop_front();
        VPLMutex_Unlock(&mutex);

        {
            size_t resp_size = VSS_HEADER_SIZE + vss_get_data_length(response);
            err = TS::TSS_Write(ioh, response, resp_size, error_msg);
            free((void*) response);
        }
        if (err != TS_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "TSS_Write() %d:%s",
                             err, error_msg.c_str());

            // Close the tunnel to force the TSS_Read() to error out
            stop();
            goto done;
        }
    }

done:
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Ending writer() %d", err);
    return;
}

void vssts_svc::start(void)
{
    TSError_t err = TS_OK;
    int rv;
    bool writer_started = false;

    // Initialize any remaining variables like any mutexes/conds
    if ((rv = VPLMutex_Init(&mutex)) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to initialize mutex");
        err = TS_ERR_NO_MEM;
        goto done;
    }

    if (VPLCond_Init(&cond) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to initialize cond");
        err = TS_ERR_NO_MEM;
        goto done;
    }

    // Create a new thread for the writer
    {
        VPLThread_attr_t thread_attr;
        VPLThread_AttrInit(&thread_attr);
        VPLThread_AttrSetStackSize(&thread_attr, 32*1024);
        VPLThread_AttrSetDetachState(&thread_attr, false);

        rv = VPLThread_Create(&thread, vssts_writer_start,
                              VPL_AS_THREAD_FUNC_ARG(this), &thread_attr,
                              "TS service handler");
        if (rv != VPL_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed start of tunnel writer");
            err = TS_ERR_NO_MEM;
            goto done;
        }
        writer_started = true;
    }

    // Become the reader
    last_read = VPLTime_GetTimeStamp();
    reader();

    // Tell the writer to stop
    VPLMutex_Lock(&mutex);
    is_writer_stop = true;
    rv = VPLCond_Signal(&cond);
    if (rv != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to signal, rv=%d", rv);
    }
    VPLMutex_Unlock(&mutex);

done:
    // Join with the writer thread
    if (writer_started) {
        rv = VPLThread_Join(&thread, NULL);
        if (rv != VPL_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to join thread, rv=%d", rv);
        }
    }
}

void vssts_svc::stop(void)
{
    TSError_t err = TS_OK;
    string error_msg;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Stopping service");

    // It should be okay to call TSS_Close() more than once, so no locks
    // are needed here
    if (ioh != NULL) {
        err = TS::TSS_Close(ioh, error_msg);
        if (err != TS_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "TSS_Close() %d:%s",
                             err, error_msg.c_str());
        } else {
            ioh = NULL;
        }
    }

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Stopped service");
    return;
}

static void vssts_svc_handler(TSServiceRequest_t& request)
{
    int rv;
    vssts_svc* svc = NULL;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "svc_handle: %p client: "FMTu64" device: "
                      FMTu64" ioh: %p",
                      request.service_handle, request.client_user_id, 
                      request.client_device_id, request.io_handle);

    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&stop_ctrl_mutex));
    if (is_stopping) {
        VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&stop_ctrl_mutex));
        VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Already stopping, exit immediately");
        goto done;
    }

    handler_ref_cnt++;
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&stop_ctrl_mutex));

    svc = pool->svc_get(request.client_user_id,
                        request.client_device_id,
                        request.io_handle);
    if (svc == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to create new service");
        goto ref_cnt_decrement;
    }

    svc->start();

    // Release it
    pool->svc_put(svc);

ref_cnt_decrement:
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&stop_ctrl_mutex));
    handler_ref_cnt--;
    if (handler_ref_cnt == 0) {
        rv = VPLCond_Signal(&stop_ctrl_cond);
        if (rv != VPL_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to signal, rv=%d", rv);
        }
    }
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&stop_ctrl_mutex));

done:
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "svc_handle: %p client: "FMTu64" device: "
                      FMTu64" ioh: %p - exiting",
                      request.service_handle, request.client_user_id, 
                      request.client_device_id, request.io_handle);
}

TSError_t vssts_srvr_start(vss_server* old_vss_server,
                           vss_session* session,
                           string& error_msg)
{
    TSError_t err = TS_OK;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Starting vssts_srvr");

    if (is_init) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0, "vssts_srvr already started");
        goto done;
    }

    VPL_SET_UNINITIALIZED(&stop_ctrl_cond);
    if (VPLCond_Init(&stop_ctrl_cond) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Unable to initialize cond");
        err = TS_ERR_NO_MEM;
        goto done;
    }

    handler_ref_cnt = 0;
    is_stopping = false;

    vssts_session = session;
    server = old_vss_server;

    pool = new (std::nothrow) vssts_svc_pool;
    if (pool == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to allocate vssts_svc_pool");
        err = TS_ERR_NO_MEM;
        goto done;
    }

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Initializing pool");
    err = pool->init();
    if (err != TS_OK) {
        goto done;
    }

    // Register our service handler
    {
        TSServiceParms_t parms;

        parms.service_names.push_back("VSSI"); 
        parms.protocol_name = "VSSI"; 
        parms.service_handler = vssts_svc_handler;

        VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Registering VSSI service");
        err = TS::TS_RegisterService(parms, svc_handle, error_msg);
        if (err != TS_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                "TS_RegisterService() %d:%s", err, error_msg.c_str());
            goto done;
        }
    }

    is_init = true;

done:
    if (err != TS_OK) {
        vssts_session = NULL;
        server = NULL;

        if (VPL_IS_INITIALIZED(&stop_ctrl_cond)) {
            VPLCond_Destroy(&stop_ctrl_cond);
        }
    }

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Started vssts_srvr");
    return err;
}

void vssts_srvr_stop(void)
{
    TSError_t err = TS_OK;
    int rv;
    string error_msg;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Stopping vssts_srvr");

    if (!is_init) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "vssts_srvr not initialized");
        err = TS_ERR_NO_MEM;
        goto done;
    }

    // This will prevent new services from being started
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&stop_ctrl_mutex));
    is_stopping = true;
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&stop_ctrl_mutex));

    err = TS::TS_DeregisterService(svc_handle, error_msg);
    if (err != TS_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "DeregisterService() %d:%s", err,
                         error_msg.c_str());
    }

    // Force all handlers to error out
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Forcing pool to stop");
    pool->stop();

    // Wait for all handlers to be done
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Waiting for handlers");
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&stop_ctrl_mutex));
    while (handler_ref_cnt > 0) {
        rv = VPLCond_TimedWait(&stop_ctrl_cond, VPLLazyInitMutex_GetMutex(&stop_ctrl_mutex), VPLTIME_FROM_SEC(VSSTS_SRVR_STOP_TIMEOUT_IN_SEC));
        if (rv != VPL_OK) {
            // This is unexpected, just break the loop and continue the shutdown.  If
            // it crashes, at least we have a log statement
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Pool stop failed timed wait, rv=%d", rv);
            break;
        }
    }
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&stop_ctrl_mutex));

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Deleting pool");
    if (pool != NULL) {
        delete pool;
        pool = NULL;
    }
    server = NULL;
    vssts_session = NULL;

    if (VPL_IS_INITIALIZED(&stop_ctrl_cond)) {
        VPLCond_Destroy(&stop_ctrl_cond);
    }

    is_init = false;

done:
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Stopped vssts_srvr");
    return;
}

void vssts_srvr_put_response(u32 vssts_id, char* response)
{
    int rv;
    vssts_svc* svc = NULL;

    // This is not a handler, but for simplicity, let's just re-use the handler_ref_cnt
    // to keep track of outstanding pool reference to avoid use-after-free

    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&stop_ctrl_mutex));
    if (is_stopping) {
        VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&stop_ctrl_mutex));
        VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Already stopping, exit immediately");
        goto done;
    }

    handler_ref_cnt++;
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&stop_ctrl_mutex));

    // Queue the response for transmission over the tunnel
    svc = pool->svc_get(vssts_id);
    if (svc != NULL) {
        svc->put_response(response);
        pool->svc_put(svc);
    } else {
        // Since the response doesn't get onto the response list, need to free it here
        free((void*) response);
    }

    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&stop_ctrl_mutex));
    handler_ref_cnt--;
    if (handler_ref_cnt == 0) {
        rv = VPLCond_Signal(&stop_ctrl_cond);
        if (rv != VPL_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to signal, rv=%d", rv);
        }
    }
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&stop_ctrl_mutex));

done:
    return;
}
