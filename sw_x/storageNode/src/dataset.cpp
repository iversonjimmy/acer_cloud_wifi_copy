/*
 *  Copyright 2010 iGware Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF IGWARE INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
 *
 */

#include "dataset.hpp"

#include "vplex_trace.h"
#include "vplex_assert.h"

dataset::dataset(dataset_id& id, int type, vss_server* server) :
    server(server),
    reserve_count(0),
    is_failed(false),
    invalid(false)
{
    this->id = id;
    this->ds_type = type;

    VPLMutex_Init(&mutex);
}

dataset::~dataset()
{ 
    VPLMutex_Destroy(&mutex);
}

const dataset_id& dataset::get_id()
{
    return id;
}

bool dataset::is_case_insensitive()
{
    return ((ds_type == vplex::vsDirectory::VIRT_DRIVE) || (ds_type == vplex::vsDirectory::CLEARFI_MEDIA));
}

void dataset::reserve()
{
    lock();
    reserve_count++;
    unlock();
}

void dataset::release()
{
    bool try_delete = false;

    lock();

    if(reserve_count > 0) {
        reserve_count--;
        VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                            "Dataset "FMTu64":"FMTu64" reference count: %d.",
                            id.uid, id.did,
                            reserve_count);
        if(reserve_count == 0) {
            try_delete = true;
        }
    }
    else {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Dataset "FMTu64":"FMTu64" reference release with no references.",
                         id.uid, id.did);
    }

    unlock();

    if(try_delete) {
        server->removeDataset(id, this);
    }
}

int dataset::num_references()
{
    int rv;

    // TODO: what is the point of the mutex here?  reserve_count is a 32 bit word
    // which will be updated atomically by the cpu.  In a race, you'll get either
    // the before or the after value, no?  This just makes it more expensive.
    lock();
    rv = reserve_count;
    unlock();

    return rv;
}

void dataset::lock()
{
    int rc = VPLMutex_Lock(&mutex);
    if(rc != 0) {
        FAILED_ASSERT("Dataset "FMTu64":"FMTu64" lock fail: %d.",
                      id.uid, id.did, rc);
    }
}

void dataset::unlock()
{
    int rc = VPLMutex_Unlock(&mutex);
    if(rc != 0) {
        FAILED_ASSERT("Dataset "FMTu64":"FMTu64" unlock fail: %d.",
                      id.uid, id.did, rc);
    }
}

void dataset::open(vss_object* object)
{
    // Track object open against this dataset.
    lock();
    objects.insert(object);
    unlock();
}

void dataset::close(vss_object* object)
{
    // Call virtual function in derived classes to release any
    // resources owned by the object
    object_release(object);
    // Remove object from set of objects open on this dataset.
    lock();
    objects.erase(object);
    unlock();
}

#define _BUF_SIZE   32*1024
s16 dataset::copy_file(const std::string& src, const std::string& dst)
{
    s16 rv = 0;
    u64 offset;
    u32 length;
    char* buf = NULL;
    VPLFS_stat_t stat;
    vss_file* src_file_handle = NULL;
    vss_file* dst_file_handle = NULL;

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
        "Dataset "FMTu64":"FMTu64 " copy %s - %s",
        id.uid, id.did, src.c_str(), dst.c_str());

    if ( (rv = stat_component(src, stat)) ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
            "Dataset "FMTu64":"FMTu64 " Failed stat of %s - %d",
            id.uid, id.did, src.c_str(), rv);
        goto done;
    }
    if ( stat.type != VPLFS_TYPE_FILE ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
            "Dataset "FMTu64":"FMTu64 " %s not a file",
            id.uid, id.did, src.c_str());
        rv = VSSI_NOTFOUND;
        goto done;
    }

    // open src file handle as read-only
    rv = open_file(src, 0, VSSI_FILE_OPEN_READ | VSSI_FILE_SHARE_READ, 0, src_file_handle);
    if ( rv || (src_file_handle == NULL) ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
            "Dataset "FMTu64":"FMTu64 " fail to open src file %s",
            id.uid, id.did, src.c_str());
        goto done;
    }

    // open dst file handle as write and create.
    // VSSI_FILE_OPEN_CREATE_ALWAYS will truncate the existing file size to 0 (if exist)
    rv = open_file(dst, 0, VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_CREATE_ALWAYS, 0, dst_file_handle);
    if ( (rv && rv != VSSI_EXISTS) || (dst_file_handle == NULL) ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
            "Dataset "FMTu64":"FMTu64 " fail to open dst file %s",
            id.uid, id.did, dst.c_str());
        goto done;
    }

    buf = (char *)malloc(_BUF_SIZE);
    if ( buf == NULL ) {
        rv = VSSI_NOMEM;
        goto done;
    }

    offset = 0;
    for( ;; ) {
        length = _BUF_SIZE;
        rv = read_file(src_file_handle, NULL, 0, offset, length, buf);
        if ( rv ) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Dataset "FMTu64":"FMTu64 " Failed read of %s - %d",
                             id.uid, id.did, src.c_str(), rv);
            break;
        }
        if (length == 0) {
            break;
        }
        rv = write_file(dst_file_handle, NULL, 0, offset, length, buf);
        if ( rv ) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                "Dataset "FMTu64":"FMTu64 " Failed cleanup of %s - %d",
                id.uid, id.did, dst.c_str(), rv);
            break;
        }
        offset += length;
    }

done:
    if ( NULL != src_file_handle ) {
        close_file(src_file_handle, NULL, 0);
    }
    if ( NULL != dst_file_handle ) {
        close_file(dst_file_handle, NULL, 0);
    }
    if ( buf ) {
        free(buf);
    }
    // If there was an error and we created the new file
    // remove it.
    if ( rv && (dst_file_handle != NULL) ) {
        s16 srv;

        if ( (srv = remove_iw(dst)) ) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                "Dataset "FMTu64":"FMTu64 " Failed cleanup of %s - %d",
                id.uid, id.did, dst.c_str(), srv);
        } else {
            srv = commit_iw();
            if ( srv ) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "Dataset "FMTu64":"FMTu64 " Failed to commit cleanup %s - %d",
                                 id.uid, id.did, dst.c_str(), srv);
            }
        }

    }
    return rv;
}

