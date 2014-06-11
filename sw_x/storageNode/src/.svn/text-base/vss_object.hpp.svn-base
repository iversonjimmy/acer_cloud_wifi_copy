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

#ifndef STORAGE_NODE__VSS_OBJECT_HPP__
#define STORAGE_NODE__VSS_OBJECT_HPP__

/// @file
/// Virtual Storage data object class (save data, etc)

class vss_object;

#include <vpl_types.h>
#include <vpl_th.h>
#include <vpl_time.h>
#include <vplex_file.h>

#include <string>
#include <vector>

#include "vss_cmdproc.hpp"
#include "dataset.hpp"
#include "vss_client.hpp"

class vss_object
{
public:
    // Statistics for object access.
    u32 commands_processed;
    u64 bytes_written;
    u64 bytes_read;
    VPLTime_t open_time;

    // Construction implies open.
    vss_object(dataset* dataset,
               u64 origin_device,
               u64 user_id,
               u32 flags, 
               u32 handle,
               u64 session_handle);
    ~vss_object();

    // Close-out an object. Object should be deleted after this.
    void close();
    bool is_closed();

    // track object references, if client does access the object, change the last_client_access time
    void reserve(bool doAccess = false);
    void release();
    u32 get_refcount();

    // Get object properties
    dataset* get_dataset();
    u64 get_origin_device();
    u64 get_user_id();
    u32 get_flags();
    u32 get_handle();
    u64 get_version();

    void set_notify_mask(VSSI_NotifyMask mask, vss_client* client, u32 vssts_id);//set mask and associated client.
    VSSI_NotifyMask get_notify_mask(vss_client* client, u32 vssts_id);
    u64 get_session_handle();
    vss_client* get_client();
    u32 get_vssts_id();
    VPLTime_t get_last_client_access_time();

private:
    VPL_DISABLE_COPY_AND_ASSIGN(vss_object);

    dataset* obj_dataset; // Dataset for this object.
    u64 user_id; // User ID of user that opened this object.
    u64 origin_device; // Origin of object access.
    u32 handle; // Object handle, assigned for the client.
    u32 flags; // Object access flags. See vss_comm.h.
    u64 open_version; // Version at object open (for locked objects)
    u32 refcount; // Object references. Represents commands in progress.
    bool closed;

    VSSI_NotifyMask notify_mask;//notify event mask
    vss_client * client;//client
    u32 vssts_id;
    u64 session_handle;//parent session
    VPLTime_t last_client_access;

    VPLMutex_t mutex;
    VPLCond_t cond;

    void lock();
    void unlock();
};

#endif // include guard
