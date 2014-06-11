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
/// vssts.cpp
///
/// VSSI-TS replacement layer

#include "vplu_types.h"
#include "vpl_th.h"
#include "vplex_trace.h"
#include "vpl_lazy_init.h"
#include "vplu_mutex_autolock.hpp"

#define IS_VSSTS    1
#include "vssts.hpp"
#include "vssts_error.hpp"

#include "vssts_internal.hpp"

#include "ts_ext_client.hpp"

#include <stdlib.h>

using namespace std;

namespace vssts {

// Mutex to protect pool and objects
static VPLLazyInitMutex_t pool_mutex = VPLLAZYINITMUTEX_INIT;
static vssts_pool* pool = NULL;
static volatile u32 api_ref_cnt = 0;

VSSI_Result VSSI_Init(u64 app_id)
{
    VSSI_Result rv = VSSI_INIT;
    int rc;
    bool is_pool_init_done = false;

    // We will hold lock until VSSI_Init function return
    // VSSI_Init is the first call at beginning, any other APIs should wait for VSSI_Init returned
    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Calling VSSI_Init()");

    if(pool) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI already initialized. Not doing it twice.");
        rv = VSSI_SUCCESS;
        goto done;
    }

    rc = VSSI_InternalInit();
    if(rc != VSSI_SUCCESS) {
        rv = rc;
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VSSI_InternalInit() - %d", rv);
        goto done;
    }

    pool = new (std::nothrow) vssts_pool;
    if(!pool) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, " No memory to create vssts_pool obj");
        rv = VSSI_NOMEM;
        goto done;
    }

    rv = pool->init();
    if ( rv != VSSI_SUCCESS ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "pool->init() - %d", rv);
        goto done;
    }
    is_pool_init_done = true;

    {
        TSError_t err;
        string error_msg;

        err = TS_EXT::TS_Init(error_msg);
        if ( err != TS_OK ) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "TS_Init() %d:%s", err,
                             error_msg.c_str());
            rv = VSSI_COMM;
            goto done;
        }
    }

    rv = VSSI_SUCCESS;
done:
    if ( rv != VSSI_SUCCESS ) {
        VSSI_InternalCleanup();
        if(pool) {
            if (is_pool_init_done) {
                pool->shutdown();
            }
            delete pool;
            pool = NULL;
        }
    }

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "VSSI_Init() done");
    return rv;
}

void VSSI_Cleanup(void)
{
    // VSSI_Cleanup is going to destroy pool, any other APIs should wait for it returned
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
    u32 retry_cnt = 0;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Calling VSSI_Cleanup()");

    // Make sure there is no other thread referring VSSI APIs
    while (api_ref_cnt) {
        if (retry_cnt++ > 60) {
            // This is unexpected.  Continue to do the clean-up anyways since there's no
            // return value to indicate success or failure.  It may cause crashes, but at
            // least we have a log statement here for debugging
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VSSI_Cleanup() timed out, ref count: "FMTu32, api_ref_cnt);
            break;
        }
        VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        // Release lock, wait for other APIs returned
        VPLThread_Sleep(VPLTime_FromSec(1));
        VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
    }

    if(!pool) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to cleanup non-initialized VSSI.");
        goto done;
    }

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Shutting down the pool");
    pool->shutdown();
    delete pool;
    pool = NULL;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Shutting down TS-EXT");
    TS_EXT::TS_Shutdown();

    VSSI_InternalCleanup();

done:
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&pool_mutex));
    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      "Cleaning up VSSI...Done.");

}

u64 VSSI_GetVersion(VSSI_Object handle)
{
    vssts_object* object = NULL;
    u64 version = 0;

    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt++;
        if(!pool) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                        "VSSI is not initialized. Call VSSI_Init() first.");
            goto done;
        }
    }

    object = pool->object_get((u32)handle, true);
    if ( object == NULL ) {
        goto done;
    }

    version =  object->get_version();

    pool->object_put(object);

done:
    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt--;
    }
    return version;
}

// Roughly lifted from vssi.c
VSSI_Dirent2* VSSI_ReadDir2(VSSI_Dir2 dir)
{
    VSSI_DirectoryState* directory = (VSSI_DirectoryState*)(dir);
    VSSI_Dirent2* rv = NULL;

    // This is necessary, since every API should be allowed if and only if VSSI_Init() had been called.
    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        if(!pool) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                        "VSSI is not initialized. Call VSSI_Init() first.");
            return rv;
        }
    }

    if(directory == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Directory is NULL. Call VSSI_OpenDir2() first.");
    }
    else if(directory->offset >= directory->data_len) {
        VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                          "Walked off end of directory.");
    }
    else {
        // Copy next entry into cur_entry and advance offset.
        char* entry = directory->raw_data + directory->offset;
        directory->cur_entry2.size = vss_dirent2_get_size(entry);
        directory->cur_entry2.ctime = vss_dirent2_get_ctime(entry);
        directory->cur_entry2.mtime = vss_dirent2_get_mtime(entry);
        directory->cur_entry2.changeVer = vss_dirent2_get_change_ver(entry);
        directory->cur_entry2.isDir = vss_dirent2_get_is_dir(entry);
        directory->cur_entry2.attrs = vss_dirent2_get_attrs(entry);
        directory->cur_entry2.signature = (const char*)vss_dirent2_get_signature(entry);
        // name is NULL terminated from the server
        directory->cur_entry2.name = (const char*)vss_dirent2_get_name(entry);

        directory->offset += (VSS_DIRENT2_BASE_SIZE +
                              vss_dirent2_get_name_len(entry) + 
                              vss_dirent2_get_meta_size(entry));
        rv = &(directory->cur_entry2);
    }

    return rv;
}

void VSSI_RewindDir2(VSSI_Dir2 dir)
{
    VSSI_DirectoryState* directory = (VSSI_DirectoryState*)(dir);

    // This is necessary, since every API should be allowed if and only if VSSI_Init() had been called.
    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        if(!pool) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                        "VSSI is not initialized. Call VSSI_Init() first.");
            return;
        }
    }

    if(directory == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Directory is NULL. Call VSSI_OpenDir2() first.");
    }
    else {
        directory->offset = 0;
    }
}

void VSSI_CloseDir2(VSSI_Dir2 dir)
{
    VSSI_DirectoryState* directory = (VSSI_DirectoryState*)(dir);

    // This is necessary, since every API should be allowed if and only if VSSI_Init() had been called.
    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        if(!pool) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                        "VSSI is not initialized. Call VSSI_Init() first.");
            return;
        }
    }

    if(directory == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Directory is NULL. Call VSSI_OpenDir() first.");
    }
    else {
        delete [] directory->raw_data;
        delete directory;
    }
}

void VSSI_Delete_Deprecated(u64 user_id,
                  u64 dataset_id,
                  void* ctx,
                  VSSI_Callback callback)
{
    VSSI_Result rv = VSSI_SUCCESS;
    vssts_object* obj = NULL;

    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt++;
        if(!pool) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                        "VSSI is not initialized. Call VSSI_Init() first.");
            rv = VSSI_INIT;
            goto done;
        }
    }

    rv = pool->object_get(user_id, dataset_id, obj);
    if ( rv != VSSI_SUCCESS ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "pool->object_get("FMTu64","FMTu64") %d",
                         user_id, dataset_id, rv);
        goto done;
    }

    obj->delete2(ctx, callback);

done:
    if ( rv != VSSI_SUCCESS ) {
        if ( obj ) {
            pool->object_put(obj);
        }
        {
            MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
            api_ref_cnt--;
        }
        if ( callback != NULL ) {
            (callback)(ctx, rv);
        }
    } else {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt--;
    }
}

void VSSI_OpenObjectTS(u64 user_id,
                       u64 dataset_id,
                       u8 mode,
                       VSSI_Object* handle,
                       void* ctx,
                       VSSI_Callback callback)
{
    VSSI_Result rv = VSSI_SUCCESS;
    vssts_object* obj = NULL;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Calling VSSI_OpenObjectTS() "FMTu64":"FMTu64,
                      user_id, dataset_id);

    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt++;
        if(!pool) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                        "VSSI is not initialized. Call VSSI_Init() first.");
            rv = VSSI_INIT;
            goto done;
        }
    }

    rv = pool->object_get(user_id, dataset_id, obj);
    if ( rv != VSSI_SUCCESS ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "pool->object_get("FMTu64","FMTu64") %d",
                         user_id, dataset_id, rv);
        goto done;
    }

    rv = obj->open(mode, handle, ctx, callback);

done:
    if ( rv != VSSI_SUCCESS ) {
        if ( obj ) {
            pool->object_put(obj);
        }
        {
            MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
            api_ref_cnt--;
        }
        if ( callback != NULL ) {
            (callback)(ctx, rv);
        }
    } else {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt--;
    }

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "VSSI_OpenObjectTS() done");
}

void VSSI_CloseObject(VSSI_Object handle,
                      void* ctx,
                      VSSI_Callback callback)
{
    VSSI_Result rv = VSSI_SUCCESS;
    vssts_object* obj = NULL;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Calling VSSI_CloseObject()");

    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt++;
        if(!pool) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                        "VSSI is not initialized. Call VSSI_Init() first.");
            rv = VSSI_INIT;
            goto done;
        }
    }

    obj = pool->object_get((u32)handle, true);
    if (obj == NULL) {
        rv = VSSI_INVALID;
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI object state handle was null");
        goto done;
    }

    rv = obj->close(ctx, callback);

done:
    if ( obj != NULL ) {
        vssts_object* tmp = obj;

        pool->object_put(tmp);

        // CloseObject() still closes the object even when there's a failure,
        // so do an additional put() to offset the one done by OpenObjectTS()
        if ( rv != VSSI_SUCCESS ) {
            pool->object_put(obj);
        }
    }
    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt--;
    }
    if( (rv != VSSI_SUCCESS) && (callback != NULL) ) {
        (callback)(ctx, rv);
    }

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "VSSI_CloseObject() done");
}

void VSSI_MkDir2(VSSI_Object handle,
                 const char* name,
                 u32 attrs,
                 void* ctx,
                 VSSI_Callback callback)
{
    VSSI_Result rv = VSSI_SUCCESS;
    vssts_object* obj = NULL;

    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt++;
        if(!pool) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                        "VSSI is not initialized. Call VSSI_Init() first.");
            rv = VSSI_INIT;
            goto done;
        }
    }

    obj = pool->object_get((u32)handle, true);

    if (obj == NULL) {
        rv = VSSI_INVALID;
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI object state handle was null");
        goto done;
    }

    obj->mkdir2(name, attrs, ctx, callback);

done:
    if ( obj != NULL ) {
        pool->object_put(obj);
    }
    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt--;
    }
    if( (rv != VSSI_SUCCESS) && (callback != NULL) ) {
        (callback)(ctx, rv);
    }
}

void VSSI_OpenDir2(VSSI_Object handle,
                   const char* name,
                   VSSI_Dir2* dir,
                   void* ctx,
                   VSSI_Callback callback)
{
    VSSI_Result rv = VSSI_SUCCESS;
    vssts_object* obj = NULL;

    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt++;
        if(!pool) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                        "VSSI is not initialized. Call VSSI_Init() first.");
            rv = VSSI_INIT;
            goto done;
        }
    }

    obj = pool->object_get((u32)handle, true);

    if (obj == NULL) {
        rv = VSSI_INVALID;
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI object state handle was null");
        goto done;
    }

    obj->dir_read(name, dir, ctx, callback);

done:
    if ( obj != NULL ) {
        pool->object_put(obj);
    }
    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt--;
    }
    if( (rv != VSSI_SUCCESS) && (callback != NULL) ) {
        (callback)(ctx, rv);
    }
}

void VSSI_Stat2(VSSI_Object handle,
                const char* name,
                VSSI_Dirent2** stats,
                void* ctx,
                VSSI_Callback callback)
{
    VSSI_Result rv = VSSI_SUCCESS;
    vssts_object* obj = NULL;

    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt++;
        if(!pool) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                        "VSSI is not initialized. Call VSSI_Init() first.");
            rv = VSSI_INIT;
            goto done;
        }
    }

    obj = pool->object_get((u32)handle, true);

    if (obj == NULL) {
        rv = VSSI_INVALID;
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI object state handle was null");
        goto done;
    }

    obj->stat2(name, stats, ctx, callback);

done:
    if ( obj != NULL ) {
        pool->object_put(obj);
    }
    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt--;
    }
    if( (rv != VSSI_SUCCESS) && (callback != NULL) ) {
        (callback)(ctx, rv);
    }
}

void VSSI_Chmod(VSSI_Object handle,
                const char* name,
                u32 attrs,
                u32 attrs_mask,
                void* ctx,
                VSSI_Callback callback)
{
    VSSI_Result rv = VSSI_SUCCESS;
    vssts_object* obj = NULL;

    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt++;
        if(!pool) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                        "VSSI is not initialized. Call VSSI_Init() first.");
            rv = VSSI_INIT;
            goto done;
        }
    }

    obj = pool->object_get((u32)handle, true);

    if (obj == NULL) {
        rv = VSSI_INVALID;
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI object state handle was null");
        goto done;
    }

    obj->chmod(name, attrs, attrs_mask, ctx, callback);

done:
    if ( obj != NULL ) {
        pool->object_put(obj);
    }
    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt--;
    }
    if( (rv != VSSI_SUCCESS) && (callback != NULL) ) {
        (callback)(ctx, rv);
    }
}

void VSSI_Remove(VSSI_Object handle,
                 const char* name,
                 void* ctx,
                 VSSI_Callback callback)
{
    VSSI_Result rv = VSSI_SUCCESS;
    vssts_object* obj = NULL;

    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt++;
        if(!pool) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                        "VSSI is not initialized. Call VSSI_Init() first.");
            rv = VSSI_INIT;
            goto done;
        }
    }

    obj = pool->object_get((u32)handle, true);

    if (obj == NULL) {
        rv = VSSI_INVALID;
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI object state handle was null");
        goto done;
    }

    obj->remove(name, ctx, callback);

done:
    if ( obj != NULL ) {
        pool->object_put(obj);
    }
    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt--;
    }
    if( (rv != VSSI_SUCCESS) && (callback != NULL) ) {
        (callback)(ctx, rv);
    }
}

void VSSI_Rename2(VSSI_Object handle,
                  const char* name,
                  const char* new_name,
                  u32 flags,
                  void* ctx,
                  VSSI_Callback callback)
{
    VSSI_Result rv = VSSI_SUCCESS;
    vssts_object* obj = NULL;

    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt++;
        if(!pool) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                        "VSSI is not initialized. Call VSSI_Init() first.");
            rv = VSSI_INIT;
            goto done;
        }
    }

    obj = pool->object_get((u32)handle, true);

    if (obj == NULL) {
        rv = VSSI_INVALID;
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI object state handle was null");
        goto done;
    }

    obj->rename2(name, new_name, flags, ctx, callback);

done:
    if ( obj != NULL ) {
        pool->object_put(obj);
    }
    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt--;
    }
    if( (rv != VSSI_SUCCESS) && (callback != NULL) ) {
        (callback)(ctx, rv);
    }
}

void VSSI_SetTimes(VSSI_Object handle,
                   const char* name,
                   VPLTime_t ctime,
                   VPLTime_t mtime,
                   void* ctx,
                   VSSI_Callback callback)
{
    VSSI_Result rv = VSSI_SUCCESS;
    vssts_object* obj = NULL;

    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt++;
        if(!pool) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                        "VSSI is not initialized. Call VSSI_Init() first.");
            rv = VSSI_INIT;
            goto done;
        }
    }

    obj = pool->object_get((u32)handle, true);

    if (obj == NULL) {
        rv = VSSI_INVALID;
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI object state handle was null");
        goto done;
    }

    obj->set_times(name, ctime, mtime, ctx, callback);

done:
    if ( obj != NULL ) {
        pool->object_put(obj);
    }
    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt--;
    }
    if( (rv != VSSI_SUCCESS) && (callback != NULL) ) {
        (callback)(ctx, rv);
    }
}

void VSSI_GetSpace(VSSI_Object handle,
                   u64* disk_size,
                   u64* dataset_size,
                   u64* avail_size, 
                   void* ctx,
                   VSSI_Callback callback)
{
    VSSI_Result rv = VSSI_SUCCESS;
    vssts_object* obj = NULL;

    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt++;
        if(!pool) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                        "VSSI is not initialized. Call VSSI_Init() first.");
            rv = VSSI_INIT;
            goto done;
        }
    }

    obj = pool->object_get((u32)handle, true);

    if (obj == NULL) {
        rv = VSSI_INVALID;
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI object state handle was null");
        goto done;
    }

    obj->get_space(disk_size, dataset_size, avail_size, ctx, callback);

done:
    if ( obj != NULL ) {
        pool->object_put(obj);
    }
    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt--;
    }
    if( (rv != VSSI_SUCCESS) && (callback != NULL) ) {
        (callback)(ctx, rv);
    }

}

/// File Handle APIs

void VSSI_OpenFile(VSSI_Object handle,
                   const char* name,
                   u32 flags,
                   u32 attrs,
                   VSSI_File* file,
                   void* ctx,
                   VSSI_Callback callback)
{
    VSSI_Result rv = VSSI_SUCCESS;
    vssts_object* obj = NULL;

    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt++;
        if(!pool) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                        "VSSI is not initialized. Call VSSI_Init() first.");
            rv = VSSI_INIT;
            goto done;
        }
    }

    obj = pool->object_get((u32)handle, true);

    if (obj == NULL) {
        rv = VSSI_BADOBJ;
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI object state handle was null");
        goto done;
    }

    obj->file_open(name, flags, attrs, file, ctx, callback);

done:
    if ( obj != NULL ) {
        pool->object_put(obj);
    }
    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt--;
    }
    if( (rv != VSSI_SUCCESS) && (callback != NULL) ) {
        (callback)(ctx, rv);
    }
}

void VSSI_CloseFile(VSSI_File file_handle,
                    void* ctx,
                    VSSI_Callback callback)
{
    VSSI_Result rv = VSSI_SUCCESS;
    vssts_file_t* file = NULL;
    vssts_object* obj = NULL;

    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt++;
        if(!pool) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                        "VSSI is not initialized. Call VSSI_Init() first.");
            rv = VSSI_INIT;
            goto done;
        }
    }

    // TODO: the file could automatically have a ref to the object
    // that's released when the file is closed.
    file = pool->file_get((u32)file_handle);
    if ( file == NULL ) {
        rv = VSSI_NOTFOUND;
        goto done;
    }
    obj = pool->object_get((u32)file->object_id, true);
    if (obj == NULL) {
        rv = VSSI_BADOBJ;
        goto done;
    }

    rv = obj->file_close(file, ctx, callback);

done:
    if ( obj != NULL ) {
        pool->object_put(obj);
    }
    if ( file != NULL ) {
        vssts_file_t* tmp = file;

        pool->file_put(tmp);

        // If return value if successful, then the extra put will be done
        // by handle_reply().  If the return value is VSSI_COMM, the caller
        // would have the option to re-try.  For other error cases, close
        // the file handle
        if ( (rv != VSSI_SUCCESS) && (rv != VSSI_COMM) ) {
            pool->file_put(file);
        }
    }
    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt--;
    }
    if( (rv != VSSI_SUCCESS) && (callback != NULL) ) {
        (callback)(ctx, rv);
    }
}

void VSSI_ReadFile(VSSI_File file_handle,
                   u64 offset,
                   u32* length,
                   char* buf,
                   void* ctx,
                   VSSI_Callback callback)
{
    VSSI_Result rv = VSSI_SUCCESS;
    vssts_file_t* file = NULL;
    vssts_object* obj = NULL;

    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt++;
        if(!pool) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                        "VSSI is not initialized. Call VSSI_Init() first.");
            rv = VSSI_INIT;
            goto done;
        }
    }

    // TODO: the file could automatically have a ref to the object
    // that's released when the file is closed.
    file = pool->file_get((u32)file_handle);
    if ( file == NULL ) {
        rv = VSSI_NOTFOUND;
        goto done;
    }
    obj = pool->object_get((u32)file->object_id, true);
    if (obj == NULL) {
        rv = VSSI_BADOBJ;
        goto done;
    }

    // Check that file handle is opened for read
    if ((file->flags & VSSI_FILE_OPEN_READ) == 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "VSSI_ReadFile not permitted to handle (flags %x)", file->flags);
        rv = VSSI_PERM;
        goto done;
    }

    obj->file_read(file, offset, length, buf, ctx, callback);

done:
    if ( obj != NULL ) {
        pool->object_put(obj);
    }
    if ( file != NULL ) {
        pool->file_put(file);
    }
    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt--;
    }
    if( (rv != VSSI_SUCCESS) && (callback != NULL) ) {
        (callback)(ctx, rv);
    }
}

void VSSI_WriteFile(VSSI_File file_handle,
                    u64 offset,
                    u32* length,
                    const char* buf,
                    void* ctx,
                    VSSI_Callback callback)
{
    VSSI_Result rv = VSSI_SUCCESS;
    vssts_file_t* file = NULL;
    vssts_object* obj = NULL;

    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt++;
        if(!pool) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                        "VSSI is not initialized. Call VSSI_Init() first.");
            rv = VSSI_INIT;
            goto done;
        }
    }

    // TODO: the file could automatically have a ref to the object
    // that's released when the file is closed.
    file = pool->file_get((u32)file_handle);
    if ( file == NULL ) {
        rv = VSSI_NOTFOUND;
        goto done;
    }
    obj = pool->object_get((u32)file->object_id, true);
    if (obj == NULL) {
        rv = VSSI_BADOBJ;
        goto done;
    }

    // Check that file handle is opened for write
    if ((file->flags & VSSI_FILE_OPEN_WRITE) == 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "VSSI_WriteFile to readOnly handle (flags %x)",
                         file->flags);
        rv = VSSI_PERM;
        goto done;
    }

    //
    // Limit write length to a reasonable maximum of 1MB.  Note that this
    // does not fail the write, but will return a count less than the
    // caller requested.  The caller must then write the additional data
    // in a separate call.
    //
    // Note that this limitation has to happen here, since it's too late
    // by the time it gets to the server side.
    //
    if(*length > (1<<20)) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "write file %p limited from %u to %u bytes.",
                          file, *length, (1<<20));
        *length = (1<<20);
    }

    obj->file_write(file, offset, length, buf, ctx, callback);

done:
    if ( obj != NULL ) {
        pool->object_put(obj);
    }
    if ( file != NULL ) {
        pool->file_put(file);
    }
    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt--;
    }
    if ( (rv != VSSI_SUCCESS) && (callback != NULL) ) {
        (callback)(ctx, rv);
    }
}

void VSSI_TruncateFile(VSSI_File file_handle,
                       u64 offset,
                       void* ctx,
                       VSSI_Callback callback)
{
    VSSI_Result rv = VSSI_SUCCESS;
    vssts_file_t* file = NULL;
    vssts_object* obj = NULL;

    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt++;
        if(!pool) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                        "VSSI is not initialized. Call VSSI_Init() first.");
            rv = VSSI_INIT;
            goto done;
        }
    }

    // TODO: the file could automatically have a ref to the object
    // that's released when the file is closed.
    file = pool->file_get((u32)file_handle);
    if ( file == NULL ) {
        rv = VSSI_NOTFOUND;
        goto done;
    }
    obj = pool->object_get((u32)file->object_id, true);
    if (obj == NULL) {
        rv = VSSI_BADOBJ;
        goto done;
    }

    // Check that file handle is opened for write
    if ((file->flags & VSSI_FILE_OPEN_WRITE) == 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "VSSI_TruncateFile to readOnly handle (flags %x)", file->flags);
        rv = VSSI_PERM;
        goto done;
    }

    obj->file_truncate(file, offset, ctx, callback);

done:
    if ( obj != NULL ) {
        pool->object_put(obj);
    }
    if ( file != NULL ) {
        pool->file_put(file);
    }
    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt--;
    }
    if( (rv != VSSI_SUCCESS) && (callback != NULL) ) {
        (callback)(ctx, rv);
    }
}

void VSSI_ChmodFile(VSSI_File file_handle,
                    u32 attrs,
                    u32 attrs_mask,
                    void* ctx,
                    VSSI_Callback callback)
{
    VSSI_Result rv = VSSI_SUCCESS;
    vssts_file_t* file = NULL;
    vssts_object* obj = NULL;

    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt++;
        if(!pool) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                        "VSSI is not initialized. Call VSSI_Init() first.");
            rv = VSSI_INIT;
            goto done;
        }
    }

    // TODO: the file could automatically have a ref to the object
    // that's released when the file is closed.
    file = pool->file_get((u32)file_handle);
    if ( file == NULL ) {
        rv = VSSI_NOTFOUND;
        goto done;
    }
    obj = pool->object_get((u32)file->object_id, true);
    if (obj == NULL) {
        rv = VSSI_BADOBJ;
        goto done;
    }

    obj->file_chmod(file, attrs, attrs_mask, ctx, callback);

done:
    if ( obj != NULL ) {
        pool->object_put(obj);
    }
    if ( file != NULL ) {
        pool->file_put(file);
    }
    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt--;
    }
    if( (rv != VSSI_SUCCESS) && (callback != NULL) ) {
        (callback)(ctx, rv);
    }
}

void VSSI_ReleaseFile(VSSI_File file_handle,
                      void* ctx,
                      VSSI_Callback callback)
{
    VSSI_Result rv = VSSI_SUCCESS;
    vssts_file_t* file = NULL;
    vssts_object* obj = NULL;

    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt++;
        if(!pool) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                        "VSSI is not initialized. Call VSSI_Init() first.");
            rv = VSSI_INIT;
            goto done;
        }
    }

    // TODO: the file could automatically have a ref to the object
    // that's released when the file is closed.
    file = pool->file_get((u32)file_handle);
    if ( file == NULL ) {
        rv = VSSI_NOTFOUND;
        goto done;
    }
    obj = pool->object_get((u32)file->object_id, true);
    if (obj == NULL) {
        rv = VSSI_BADOBJ;
        goto done;
    }

    obj->file_release(file, ctx, callback);

done:
    if ( obj != NULL ) {
        pool->object_put(obj);
    }
    if ( file != NULL ) {
        pool->file_put(file);
    }
    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt--;
    }
    if( (rv != VSSI_SUCCESS) && (callback != NULL) ) {
        (callback)(ctx, rv);
    }
}

VSSI_ServerFileId VSSI_GetServerFileId(VSSI_File file_handle)
{
    VSSI_Result rv = VSSI_SUCCESS;
    vssts_file_t* file = NULL;
    VSSI_ServerFileId file_id = 0;

    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt++;
        if(!pool) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                        "VSSI is not initialized. Call VSSI_Init() first.");
            rv = VSSI_INIT;
            goto done;
        }
    }

    // TODO: the file could automatically have a ref to the object
    // that's released when the file is closed.
    file = pool->file_get((u32)file_handle);
    if ( file == NULL ) {
        rv = VSSI_NOTFOUND;
        goto done;
    }

    file_id = file->server_handle;

done:
    if ( file != NULL ) {
        pool->file_put(file);
    }
    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt--;
    }

    return file_id;
}

void VSSI_GetNotifyEvents(VSSI_Object handle,
                          VSSI_NotifyMask* mask_out,
                          void* ctx,
                          VSSI_Callback callback)
{
    VSSI_Result rv = VSSI_SUCCESS;
    vssts_object* obj = NULL;

    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt++;
        if(!pool) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                        "VSSI is not initialized. Call VSSI_Init() first.");
            rv = VSSI_INIT;
            goto done;
        }
    }

    obj = pool->object_get((u32)handle, true);

    if (obj == NULL) {
        rv = VSSI_INVALID;
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI object state handle was null");
        goto done;
    }

    obj->get_notify_events(mask_out, ctx, callback);

done:
    if ( obj != NULL ) {
        pool->object_put(obj);
    }
    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt--;
    }
    if( (rv != VSSI_SUCCESS) && (callback != NULL) ) {
        (callback)(ctx, rv);
    }
}

void VSSI_SetNotifyEvents(VSSI_Object handle,
                          VSSI_NotifyMask* mask_in_out,
                          void* notify_ctx,
                          VSSI_NotifyCallback notify_callback,
                          void* ctx,
                          VSSI_Callback callback)
{
    VSSI_Result rv = VSSI_SUCCESS;
    vssts_object* obj = NULL;

    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt++;
        if(!pool) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                        "VSSI is not initialized. Call VSSI_Init() first.");
            rv = VSSI_INIT;
            goto done;
        }
    }

    obj = pool->object_get((u32)handle, true);

    if (obj == NULL) {
        rv = VSSI_INVALID;
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "VSSI object state handle was null");
        goto done;
    }

    obj->set_notify_events(mask_in_out,
                           notify_ctx,
                           notify_callback,
                           ctx,
                           callback);

done:
    if ( obj != NULL ) {
        pool->object_put(obj);
    }
    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt--;
    }
    if( (rv != VSSI_SUCCESS) && (callback != NULL) ) {
        (callback)(ctx, rv);
    }
}
                                      
void VSSI_SetFileLockState(VSSI_File file_handle,
                           VSSI_FileLockState lock_state,
                           void* ctx,
                           VSSI_Callback callback)
{
    VSSI_Result rv = VSSI_SUCCESS;
    vssts_file_t* file = NULL;
    vssts_object* obj = NULL;

    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt++;
        if(!pool) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                        "VSSI is not initialized. Call VSSI_Init() first.");
            rv = VSSI_INIT;
            goto done;
        }
    }

    // TODO: the file could automatically have a ref to the object
    // that's released when the file is closed.
    file = pool->file_get((u32)file_handle);
    if ( file == NULL ) {
        rv = VSSI_NOTFOUND;
        goto done;
    }
    obj = pool->object_get((u32)file->object_id, true);
    if (obj == NULL) {
        rv = VSSI_BADOBJ;
        goto done;
    }

    obj->file_set_lock_state(file, lock_state, ctx, callback);

done:
    if ( obj != NULL ) {
        pool->object_put(obj);
    }
    if ( file != NULL ) {
        pool->file_put(file);
    }
    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt--;
    }
    if( (rv != VSSI_SUCCESS) && (callback != NULL) ) {
        (callback)(ctx, rv);
    }
}

void VSSI_GetFileLockState(VSSI_File file_handle,
                           VSSI_FileLockState* lock_state,
                           void* ctx,
                           VSSI_Callback callback)
{
    VSSI_Result rv = VSSI_SUCCESS;
    vssts_file_t* file = NULL;
    vssts_object* obj = NULL;

    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt++;
        if(!pool) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                        "VSSI is not initialized. Call VSSI_Init() first.");
            rv = VSSI_INIT;
            goto done;
        }
    }

    // TODO: the file could automatically have a ref to the object
    // that's released when the file is closed.
    file = pool->file_get((u32)file_handle);
    if ( file == NULL ) {
        rv = VSSI_NOTFOUND;
        goto done;
    }
    obj = pool->object_get((u32)file->object_id, true);
    if (obj == NULL) {
        rv = VSSI_BADOBJ;
        goto done;
    }

    obj->file_get_lock_state(file, lock_state, ctx, callback);

done:
    if ( obj != NULL ) {
        pool->object_put(obj);
    }
    if ( file != NULL ) {
        pool->file_put(file);
    }
    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt--;
    }
    if( (rv != VSSI_SUCCESS) && (callback != NULL) ) {
        (callback)(ctx, rv);
    }
}

void VSSI_SetByteRangeLock(VSSI_File file_handle,
                           VSSI_ByteRangeLock* br_lock,
                           u32 flags,
                           void* ctx,
                           VSSI_Callback callback)
{
    VSSI_Result rv = VSSI_SUCCESS;
    vssts_file_t* file = NULL;
    vssts_object* obj = NULL;

    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt++;
        if(!pool) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                        "VSSI is not initialized. Call VSSI_Init() first.");
            rv = VSSI_INIT;
            goto done;
        }
    }

    // TODO: the file could automatically have a ref to the object
    // that's released when the file is closed.
    file = pool->file_get((u32)file_handle);
    if ( file == NULL ) {
        rv = VSSI_NOTFOUND;
        goto done;
    }
    obj = pool->object_get((u32)file->object_id, true);
    if (obj == NULL) {
        rv = VSSI_BADOBJ;
        goto done;
    }

    obj->file_set_byte_range_lock(file, br_lock, flags, ctx, callback);

done:
    if ( obj != NULL ) {
        pool->object_put(obj);
    }
    if ( file != NULL ) {
        pool->file_put(file);
    }
    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt--;
    }
    if( (rv != VSSI_SUCCESS) && (callback != NULL) ) {
        (callback)(ctx, rv);
    }
}

void VSSI_NetworkDown(void)
{
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Calling VSSI_NetworkDown()");

    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt++;
        if(!pool) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                        "VSSI is not initialized. Call VSSI_Init() first.");
            goto done;
        }
    }
    pool->tunnel_release_all();
done:
    {
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&pool_mutex));
        api_ref_cnt--;
    }
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "VSSI_NetworkDown() done");
}

}
