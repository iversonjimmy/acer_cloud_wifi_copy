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
#include "vpl_plat.h"
#include "vpl_types.h"
#include "vplex_file.h"
#include "vplex_trace.h"
#include "vplex_assert.h"

#include "vssi_error.h"
#include "vssi_types.h"

#include "vss_object.hpp"
#include "vss_cmdproc.hpp"

#include <map>
#include <iostream>
#include <sstream>

#include <stdlib.h>
#include <errno.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <fcntl.h>



using namespace std;


// this happens on object open
vss_object::vss_object(dataset* obj_dataset, 
                       u64 origin_device,
                       u64 user_id,
                       u32 flags, 
                       u32 handle,
                       u64 session_handle) :
    commands_processed(1), // open command
    bytes_written(0),
    bytes_read(0),
    obj_dataset(obj_dataset),
    user_id(user_id),
    origin_device(origin_device),
    handle(handle),
    flags(flags),
    refcount(1),
    closed(false),
    notify_mask(0),
    client(NULL),
    vssts_id(0),
    session_handle(session_handle)
{
    // Apply flags for this object. (all but read-lock, already held)
    obj_dataset->open(this);
    open_version = obj_dataset->get_version();
    open_time = VPLTime_GetTimeStamp();
    // Inherited reference to the dataset. Release it when destroyed.
    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      FMTu64" "FMTu64":"FMTu64" #%u mode %u open (object %p).",
                      origin_device, obj_dataset->get_id().uid, obj_dataset->get_id().did,
                      handle, flags, this);
    
    VPLMutex_Init(&mutex);
    VPLCond_Init(&cond);

    last_client_access = VPLTime_GetTimeStamp();
}

// this happens on object close
vss_object::~vss_object()
{
    VPLTime_t close_time = VPLTime_GetTimeStamp();

    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      FMTu64" "FMTu64":"FMTu64" #%u mode %u dealloc (object %p).",
                      origin_device, obj_dataset->get_id().uid, obj_dataset->get_id().did,
                      handle, flags, this);
    if(refcount > 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         FMTu64" "FMTu64":"FMTu64" #%u mode %u dealloc: %d references outstanding.",
                         origin_device, obj_dataset->get_id().uid, obj_dataset->get_id().did,
                         handle, flags, refcount);
    }
    if(!closed) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                          FMTu64" "FMTu64":"FMTu64" #%u mode %u dealloc: not marked closed.",
                          origin_device, obj_dataset->get_id().uid, obj_dataset->get_id().did,
                          handle, flags);
    }
    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      FMTu64" "FMTu64":"FMTu64" summary: cmd:%u read:"FMTu64"b write:"FMTu64"b time:"FMT_VPLTime_t"us.",
                      origin_device, obj_dataset->get_id().uid, obj_dataset->get_id().did,
                      commands_processed, bytes_read, bytes_written,
                      close_time - open_time);

    if(obj_dataset) {
        obj_dataset->close(this); // Clean up any state owned by this object (file opens, locks ...)
        obj_dataset->release();   // Release object's reference on the dataset
    }

    set_notify_mask(0, NULL, 0);

    VPLMutex_Destroy(&mutex);
    VPLCond_Destroy(&cond);
}

void vss_object::lock()
{
    VPLMutex_Lock(&mutex);
}

void vss_object::unlock()
{
    VPLMutex_Unlock(&mutex);
}

void vss_object::close()
{
    if(closed) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         FMTu64" "FMTu64":"FMTu64" #%u mode %u already closed (refcount %d).",
                         origin_device, obj_dataset->get_id().uid, obj_dataset->get_id().did,
                         handle, flags, refcount);
    }
    else {
        closed = true;
        // TODO: need to lock here?  Or maybe we know we are the last ref ...
        refcount--; // removes reference made on open
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          FMTu64" "FMTu64":"FMTu64" #%u mode %u closed (refcount %d).",
                          origin_device, obj_dataset->get_id().uid, obj_dataset->get_id().did,
                          handle, flags, refcount);
    }
}

bool vss_object::is_closed()
{
    return closed;
}

void vss_object::reserve(bool doAccess)
{
    lock();
    refcount++;
    if(doAccess)
        last_client_access = VPLTime_GetTimeStamp();
    unlock();
    VPLTRACE_LOG_FINER(TRACE_BVS, 0,
                       FMTu64" "FMTu64":"FMTu64" #%u mode %u refcount now %u.",
                       origin_device, obj_dataset->get_id().uid, obj_dataset->get_id().did,
                       handle, flags, refcount);
}

void vss_object::release()
{
    lock();
    if(refcount == 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         FMTu64" "FMTu64":"FMTu64" #%u mode %u released without references.",
                         origin_device, obj_dataset->get_id().uid, obj_dataset->get_id().did,
                         handle, flags);
    }
    else {
        refcount--;
        VPLTRACE_LOG_FINER(TRACE_BVS, 0,
                           FMTu64" "FMTu64":"FMTu64" #%u mode %u refcount now %u.",
                           origin_device, obj_dataset->get_id().uid, obj_dataset->get_id().did,
                           handle, flags, refcount);
    }
    unlock();
}

VPLTime_t vss_object::get_last_client_access_time()
{
    return last_client_access;
}

u32 vss_object::get_refcount()
{
    return refcount;
}

dataset* vss_object::get_dataset() 
{
    return obj_dataset;
}

u64 vss_object::get_origin_device() 
{
    return origin_device;
}

u64 vss_object::get_user_id() 
{
    return user_id;
}

u32 vss_object::get_flags() 
{
    return flags;
}

u32 vss_object::get_handle() 
{
    return handle;
}

u64 vss_object::get_version()
{
    u64 rv;

    if(flags & VSS_OPEN_MODE_LOCK) {
        rv = open_version;
    }
    else {
        rv = obj_dataset->get_version();
    }

    return rv;
}

void vss_object::set_notify_mask(VSSI_NotifyMask mask,
                                 vss_client *client,
                                 u32 vssts_id)
{
    // A mask of 0 means there's no notification.
    lock();
    if ( (mask != 0) && (client == NULL) && (vssts_id == 0) ) {
        FAILED_ASSERT("Invalid params: mask set, client not set.");
    }
    if( (notify_mask != 0) && (this->client) ) {
        // If the mask is non-zero then client must be non-zero.
        this->client->disassociate_object(this);
    }
    if(mask != 0x0) {
        if ( client != NULL ) {
            client->associate_object(this);
        }
        this->client = client;
        this->vssts_id = vssts_id;
        notify_mask = mask;
    }
    else {
        this->client = NULL;
        this->vssts_id = 0;
        notify_mask = 0;
    }
    unlock();
}

VSSI_NotifyMask vss_object::get_notify_mask(vss_client* client, u32 vssts_id)
{
    VSSI_NotifyMask mask;

    lock();
    // If the client is NULL then just return the current mask.
    // This is used by the vss_server in determining whether
    // to send an event for this object. It does not have the client
    // information available to it at that point nor should it need
    // it.
    if ( ((client == NULL) && (vssts_id == 0)) ||
            (client == this->client) || (vssts_id == this->vssts_id) ) {
        mask = notify_mask;
    }
    else {
        mask = 0;
    }
    unlock();

    return mask;
}

u64 vss_object::get_session_handle()
{
    return session_handle;
}

vss_client * vss_object::get_client()
{
    return client;
}

u32 vss_object::get_vssts_id()
{
    return vssts_id;
}
