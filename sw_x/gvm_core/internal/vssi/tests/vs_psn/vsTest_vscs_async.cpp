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

#include <stdio.h>
#include <stdlib.h>

#include <ctype.h>
#include <string>

#include "vpl_time.h"
#include "vpl_th.h"
#include "vplex_assert.h"
#include "vplex_trace.h"
#include "vplex_user.h"
#include "vplex_math.h"

#include "vssi.h"
#include "vssi_error.h"
#include "vss_comm.h"

#include "vsTest_vscs_common.hpp"
#include "vsTest_vscs_async.hpp"
#include "vsTest_personal_cloud_data.hpp"
#include "vsTest_async_notification_events.hpp"

using namespace std;

#define TEST_DB_MAX_REMOVED 50

extern const char* vsTest_curTestName;
VSSI_Session vssi_session = 0;

/// Read/write size for VSCS
static const int CONTENT_BLOCKSIZE = (32*1024);
static const int SAVE_BLOCKSIZE = (4*1024);
static const int CONTENT_LONG_TESTBLOCKS = (2*1024);
static const int CONTENT_SHORT_TESTBLOCKS = (16);
static const int SAVE_TESTBLOCKS = (16*1024);

typedef struct  {
    VPLSem_t sem;
    int rv;
} vscs_test_context_t;

static void vscs_test_callback(void* ctx, VSSI_Result rv)
{
    vscs_test_context_t* context = (vscs_test_context_t*)ctx;

    context->rv = rv;
    VPLSem_Post(&(context->sem));
}

static int vss_chmod_file(vscs_test_context_t& test_context, VSSI_File& fh, u32 attrs, u32 attrs_mask);
static int vss_attrs_check(vscs_test_context_t& test_context, VSSI_Object handle, const char* name, u32 attrs);
static VPLTime_t get_mtime(VSSI_Object handle, const char* name, string event);

static int create_dir_tree(vscs_test_context_t& test_context, VSSI_Object handle, const char *dirname, int num_levels, int num_dirs, int num_files);
static int traverse_dir_tree(vscs_test_context_t& test_context, VSSI_Object handle, char *dirname, int num_levels, int num_dirs, int num_files, bool isdelete);
static int time_to_traverse(vscs_test_context_t& test_context, VSSI_Object handle, const char *dirname_base, const int num_top_level_trees, const int num_subtrees, int num_levels, int num_dirs, int num_files);

typedef struct  {
    int top_tree;
    int sub_tree;
} db_index;

static int dataset_events_count = 0;

typedef struct  {
    VPLSem_t sem;
    VSSI_NotifyMask mask;
} vscs_disconnect_context_t;

static vscs_disconnect_context_t disconnect_context;

static void vscs_disconnect_callback(void* ctx,
                                     VSSI_NotifyMask mask,
                                     const char* message,
                                     u32 message_length)
{
    vscs_disconnect_context_t* context = (vscs_disconnect_context_t*)ctx;

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Disconnect event received: "FMTx64, mask);
    VPLSem_Post(&(context->sem));
}

typedef struct  {
    VPLSem_t sem;
    VSSI_NotifyMask mask;
    VSSI_FileLockState lock_state;
    VSSI_ServerFileId server_file_id;
} vscs_notification_context_t;

struct vscs_obj_context_t {
    VSSI_Object                  handle;
    vscs_test_context_t          test_context;
    vscs_notification_context_t  notify_context;
};

// Need file handles global where notify callback can see them
static int numVssiFiles = 0;
static VSSI_File vssiFile[10];

static int oplock_break_count = 0;

static void oplock_notification_callback(void* ctx, 
                                         VSSI_NotifyMask mask,
                                         const char* message,
                                         u32 message_length)
{
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Mask: "FMTx64" msg %p msgLen %u",
                        mask, message, message_length);
    vscs_notification_context_t* context = (vscs_notification_context_t*)ctx;
    context->mask = mask;

    if(mask & VSSI_NOTIFY_DATASET_CHANGE_EVENT) {
        dataset_events_count++;
    }
    if(mask & VSSI_NOTIFY_OPLOCK_BREAK_EVENT) {
        oplock_break_count++;
        if(message_length < VSS_OPLOCK_BREAK_SIZE) {
            VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "notification message too short");
        }
        else {
            context->lock_state = vss_oplock_break_get_mode(message);
            context->server_file_id = vss_oplock_break_get_handle(message);
            VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "oplock break "FMTx64" handle %x",
                                context->lock_state, context->server_file_id);
            for (int i = 0; i < numVssiFiles; i++) {
                VSSI_ServerFileId fid = VSSI_GetServerFileId(vssiFile[i]);
                VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "ServerFid %x for File %p",
                                fid, vssiFile[i]);
                if(fid == context->server_file_id) {
                    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "ServerFid %x matches",
                                fid);
                }
            }
        }
    }

    VPLSem_Post(&(context->sem));
}

typedef struct {
    VSSI_Object handle;
    VPLSem_t* send_sem;
    VPLSem_t* done_sem;

    char* readval;

    u64 offset;
    u32 length;
    u32 total_length;
    u32 done_length;
    char name[5]; // Longest BVD component name is 4 characters, +1 for '\0'.
    char buf[CONTENT_BLOCKSIZE];
} vscs_test_io_context_t;

#ifdef NEEDS_CONVERSION
static int read_errors = 0;
static u64 read_bytes = 0;
static int hit_eof = 0;

static void vscs_test_read_callback(void* ctx, VSSI_Result rv)
{
    /// If readval is specified, verify the buffer contents match.
    /// Operation failure or buffer mismatch increase rv.
    vscs_test_io_context_t* context = (vscs_test_io_context_t*)ctx;

    if(rv == VSSI_EOF) {
        VPLTRACE_LOG_FINE(TRACE_APP, 0,
                          "Read "FMTu32" bytes at offset "FMTu64" past EOF.",
                          context->length, context->offset);
        hit_eof++;
        VPLSem_Post(context->send_sem);
        VPLSem_Post(context->done_sem);
        free(context);
    }
    else if(rv != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Read "FMTu32" bytes at offset "FMTu64" with result %d.",
                         context->length, context->offset, rv);
        read_errors++;
        VPLSem_Post(context->send_sem);
        VPLSem_Post(context->done_sem);
        free(context);
    }
    else {
        read_bytes += context->length;

        if(context->length + context->done_length < context->total_length) {
            // Short read. Issue read for the rest
            context->done_length += context->length;
            context->length = context->total_length - context->done_length;
            VSSI_Read(context->handle, context->name,
                      context->offset + context->done_length,
                      &(context->length),
                      context->buf + context->done_length,
                      context, vscs_test_read_callback);
            VPLTRACE_LOG_ERR(TRACE_APP, 0,
                             "Redo read for "FMTu32" bytes on short read reply.",
                             context->total_length - context->done_length);
        }
        else {
            // Check read data is correct
            if(context->readval != NULL) {
                for(u32 i = 0; i < context->total_length; i++) {
                    if(context->buf[i] != *(context->readval)) {
                        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                    "Mismatch data at offset "FMTu64". ("FMTu32" vs "FMTu32")",
                                    context->offset + i,
                                    context->buf[i],
                                    *(context->readval));
                        read_errors++;
                        break;
                    }
                }
            }
            VPLSem_Post(context->send_sem);
            VPLSem_Post(context->done_sem);
            free(context);
        }
    }
}

// Read object and verify its contents.
// Data in blocks file should all match readval when defined.
// Otherwise, blocks data is not verified, just consistency.
// Read the specified number of blocks, attempting to keep as many requests in-flight at a time as specified.
// TODO: Do a BVD consistency check for read data. (crypto, signatures, metadata, etc)
const char* vsTest_vscs_read_and_verify_async = "VSCS Read and verify (async) test";
static int test_vscs_read_and_verify_async(VSSI_Object handle,
                                           const char* name,
                                           char* readval,
                                           int inflight,
                                           u32 blocks,
                                           u32 blocksize)
{
    int rv = 0;

    VPLSem_t send_sem;
    VPLSem_t done_sem;

    u32 sent_cmds = 0;
    u32 complete_cmds = 0;

    VPLTime_t start;
    VPLTime_t end;

    vsTest_curTestName = vsTest_vscs_read_and_verify_async;
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                   "Starting test: %s.",
                   vsTest_curTestName);

    VPL_SET_UNINITIALIZED(&send_sem);
    VPL_SET_UNINITIALIZED(&done_sem);

    if(VPLSem_Init(&send_sem, VPLSEM_MAX_COUNT, inflight) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "Failed to create semaphore.");
        rv++;
        goto exit;
    }
    if(VPLSem_Init(&done_sem, VPLSEM_MAX_COUNT, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "Failed to create semaphore.");
        rv++;
        goto exit;
    }

    /// Issue block reads until enough are issued.
    read_errors = 0;
    read_bytes = 0;
    hit_eof = 0;
    start = VPLTime_GetTimeStamp();
    while(sent_cmds < blocks && !hit_eof && read_errors == 0) {
        vscs_test_io_context_t* context = (vscs_test_io_context_t*)
            malloc(sizeof(vscs_test_io_context_t));

        if(context == NULL) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                        "Failed to allocate read context.");
            rv++;
            break;
        }

        context->handle = handle;
        context->send_sem = &send_sem;
        context->done_sem = &done_sem;
        context->readval = readval;
        context->offset = sent_cmds * blocksize;
        context->length = context->total_length = blocksize;
        strncpy(context->name, name, 5);
        context->done_length = 0;
        VPLSem_Wait(&send_sem);
        VSSI_Read(handle, name,
                  context->offset, &(context->length),
                  context->buf,
                  context, vscs_test_read_callback);
        sent_cmds++;
    }

    /// Collect results. Done when enough results are collected.
    while(complete_cmds < sent_cmds) {
        VPLSem_Wait(&done_sem);
        complete_cmds++;
    }
    end = VPLTime_GetTimeStamp();

    rv += read_errors;
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                   "Test %s has "FMTu32" read errors and hit EOF "FMTu32" times.",
                        vsTest_curTestName, read_errors, hit_eof);

    /// Compute effective bandwidth
    if(rv == 0) {
        VPLTime_t elapsed = VPLTime_DiffClamp(end, start);
        if(end == start) {
            VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                           "Read "FMTu64" bytes in 0us. No Rate calculable",
                           read_bytes);
        }
        else {
            u64 mbps = (read_bytes * 8)/ elapsed;

            VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                           "Read "FMTu64" bytes in "FMT_VPLTime_t"us. Rate: ~"FMTu64" Mb/s",
                           read_bytes, elapsed, mbps);
        }
    }
    VPLSem_Destroy(&send_sem);
    VPLSem_Destroy(&done_sem);

 exit:
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                   "Test %s result: %d.",
                   vsTest_curTestName, rv);
    return rv;
}

static int write_errors = 0;
static void vscs_test_write_callback(void* ctx, VSSI_Result rv)
{
    vscs_test_io_context_t* context = (vscs_test_io_context_t*)ctx;

    if(rv != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                    "Write "FMTu32" bytes at offset "FMTu64" with result %d.",
                    context->length, context->offset, rv);
        write_errors++;
        VPLSem_Post(context->send_sem);
        VPLSem_Post(context->done_sem);
        free(context);
    }
    else {
        if(context->length + context->done_length < context->total_length) {
            // Short write. Issue write for the rest
            context->done_length += context->length;
            context->length = context->total_length - context->done_length;
            VSSI_Write(context->handle, BVD_BLOCK_DATA,
                       context->offset + context->done_length,
                       &(context->length),
                       context->buf + context->done_length,
                       context, vscs_test_write_callback);
        }
        else {
            VPLSem_Post(context->send_sem);
            VPLSem_Post(context->done_sem);
            free(context);
        }
    }
}

// Write some blocks filled with a given character value to the object.
// TODO: Do in a consistent way for BVD objects (encrypt, sign, etc.)
const char* vsTest_vscs_write_object_async = "VSCS Write object (async) test";
static int test_vscs_write_object_async(VSSI_Object handle, char* writeval,
                                        u32 inflight, u32 blocks, u32 blocksize)
{
    int rv = 0;

    VPLSem_t send_sem;
    VPLSem_t done_sem;

    u32 sent_cmds = 0;
    u32 complete_cmds = 0;

    VPLTime_t start;
    VPLTime_t end;

    vsTest_curTestName = vsTest_vscs_write_object_async;
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                   "Starting test: %s.",
                   vsTest_curTestName);

    VPL_SET_UNINITIALIZED(&send_sem);
    VPL_SET_UNINITIALIZED(&done_sem);

    if(VPLSem_Init(&send_sem, VPLSEM_MAX_COUNT, inflight) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "Failed to create semaphore.");
        rv++;
        goto exit;
    }
    if(VPLSem_Init(&done_sem, VPLSEM_MAX_COUNT, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "Failed to create semaphore.");
        rv++;
        goto exit;
    }

    /// Issue block writes until enough are issued.
    start = VPLTime_GetTimeStamp();
    write_errors = 0;

    while(sent_cmds < blocks && write_errors == 0) {
        vscs_test_io_context_t* context = (vscs_test_io_context_t*)
            malloc(sizeof(vscs_test_io_context_t));

        if(context == NULL) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                        "Failed to allocate read context.");
            rv++;
            break;
        }

        context->handle = handle;
        context->send_sem = &send_sem;
        context->done_sem = &done_sem;
        context->offset = sent_cmds * blocksize;
        context->length = context->total_length = blocksize;
        context->done_length = 0;
        memset(context->buf, *writeval, blocksize);

        VPLSem_Wait(&send_sem);
        VSSI_Write(handle, BVD_BLOCK_DATA,
                   context->offset, &(context->length),
                   context->buf,
                   context, vscs_test_write_callback);
        sent_cmds++;
    }

    /// Collect results. Done when enough results are collected.
    while(complete_cmds < sent_cmds) {
        VPLSem_Wait(&done_sem);
        complete_cmds++;
    }
    end = VPLTime_GetTimeStamp();
    rv += write_errors;
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                   "Test %s had "FMTu32" write errors",
                   vsTest_curTestName, write_errors);

    /// Compute effective bandwidth
    if(rv == 0) {
        VPLTime_t elapsed = VPLTime_DiffClamp(end, start);
        u64 bytes_written = sent_cmds * blocksize;
        if(end == start) {

            VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                           "Wrote "FMTu64" bytes in 0us. No Rate calculable",
                           bytes_written);
        }
        else {
            u64 mbps = (bytes_written * 8) / elapsed;

            VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                           "Wrote "FMTu64" bytes in "FMT_VPLTime_t"us. Rate: ~"FMTu64" Mb/s",
                           bytes_written, elapsed, mbps);
        }
    }

    VPLSem_Destroy(&send_sem);
    VPLSem_Destroy(&done_sem);

 exit:
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                   "Test %s result: %d.",
                   vsTest_curTestName, rv);
    return rv;
}

static const char vsTest_vscs_access_save_bvd_async [] = "VSCS User Save Test";
static int test_vscs_access_save_bvd_async(VSSI_Session session,
                                           const char* save_description,
                                           u64 user_id,
                                           u64 dataset_id,
                                           const VSSI_RouteInfo& route_info,
                                           bool use_xml_api)
{
    int rv = 0;
    int rc;
    VSSI_Object handle;
    vscs_test_context_t test_context;
    char filedata;
    u64 version;
    VPL_SET_UNINITIALIZED(&(test_context.sem));

    vsTest_curTestName = vsTest_vscs_access_save_bvd_async;
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Starting test: %s.",
                        vsTest_curTestName);

    // REQUIRES: valid save_description and registered session
    if(save_description == NULL) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                    "No save object description. Aborting download test.");
        return rv+1;
    }
    if(vssi_session == 0) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                    "No BVS session registered.");
        return rv+1;
    }

    if(VPLSem_Init(&(test_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "Failed to create semaphore.");
        return rv+1;
    }

    // Set initial conditions: delete object contents.
    if ( use_xml_api ) {
        VSSI_Delete(vssi_session, save_description,
                    &test_context, vscs_test_callback);
    }
    else {
        VSSI_Delete2(vssi_session, user_id, dataset_id, &route_info,
                     &test_context, vscs_test_callback);
    }
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Delete object %s failed: %d.",
                         save_description, rc);
        rv++;
        goto fail_open;
    }

    // Test of read/write objects.
    // (Open) Create a new save object (should be version 0)
    //    (if it exists, delete it and log a warning)
    if ( use_xml_api ) {
        VSSI_OpenObject(vssi_session, save_description,
                        VSSI_READWRITE | VSSI_FORCE, &handle,
                        &test_context, vscs_test_callback);
    }
    else {
        VSSI_OpenObject2(vssi_session, user_id, dataset_id, &route_info,
                         VSSI_READWRITE | VSSI_FORCE, &handle,
                         &test_context, vscs_test_callback);
    }
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                    "Open object %s failed: %d.",
                    save_description, rc);
        rv++;
        goto fail_open;
    }

    version = VSSI_GetVersion(handle);

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                   "Read the object. Get a not found error.");
    {
        char buf[CONTENT_BLOCKSIZE];
        u32 length = CONTENT_BLOCKSIZE;

        VSSI_Read(handle, BVD_BLOCK_DATA,
                  0, &length, buf,
                  &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_NOTFOUND) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Read of empty file returned result %d. Expected not found error %d.",
                             rc, VSSI_NOTFOUND);
            rv++;
            goto fail;
        }
    }

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                   "Write data to the save game (number of 32k blocks)");
    filedata = '1';
    rv += test_vscs_write_object_async(handle, &filedata, 32, SAVE_TESTBLOCKS,
                                       SAVE_BLOCKSIZE);
    if(rv > 0) goto fail;

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                   "Commit the data. Verify file version.");
    VSSI_Commit(handle, &test_context, vscs_test_callback);
    version++;
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;

    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Failed to commit object %s: %d",
                         save_description, rc);
        rv++;
        goto fail;
    }

    if(VSSI_GetVersion(handle) != version) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Object version after first commit is "FMTu64". Expected "FMTu64".",
                         VSSI_GetVersion(handle), version);
        rv++;
        goto fail;
    }

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Read one block to confirm server has sync'd save data.");
    rv += test_vscs_read_and_verify_async(handle, BVD_BLOCK_DATA,
                                          &filedata, 1, 1,
                                          SAVE_BLOCKSIZE);
    if(rv > 0) goto fail;

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                   "Verify object data.");
    rv += test_vscs_read_and_verify_async(handle, BVD_BLOCK_DATA,
                                          &filedata, 32, SAVE_TESTBLOCKS,
                                          SAVE_BLOCKSIZE);
    if(rv > 0) goto fail;

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                   "Overwrite new data for the same object.");
    filedata = '2';
    rv += test_vscs_write_object_async(handle, &filedata, 32, SAVE_TESTBLOCKS,
                                       SAVE_BLOCKSIZE);
    if(rv > 0) goto fail;

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                   "Commit the data. Verify object version.");
    VSSI_Commit(handle, &test_context, vscs_test_callback);
    version++;
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Failed to commit object %s: %d",
                         save_description, rc);
        rv++;
        goto fail;
    }
    if(VSSI_GetVersion(handle) != version) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Object version after second commit is "FMTu64". Expected "FMTu64".",
                         VSSI_GetVersion(handle), version);
        rv++;
        goto fail;
    }

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Read one block to confirm server has sync'd save data.");
    rv += test_vscs_read_and_verify_async(handle, BVD_BLOCK_DATA,
                                          &filedata, 1, 1,
                                          SAVE_BLOCKSIZE);
    if(rv > 0) goto fail;

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                   "Verify object data.");
    rv += test_vscs_read_and_verify_async(handle, BVD_BLOCK_DATA,
                                          &filedata, 32, SAVE_TESTBLOCKS,
                                          SAVE_BLOCKSIZE);
    if(rv > 0) goto fail;

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                   "Close the object.");
    VSSI_CloseObject(handle,
                     &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;

    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Failed to close object %s: %d",
                         save_description, rc);
        rv++;
        goto fail;
    }

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                   "Open object again, change first block.");
    if ( use_xml_api ) {
        VSSI_OpenObject(vssi_session, save_description,
                        VSSI_READWRITE, &handle,
                        &test_context, vscs_test_callback);
    }
    else {
        VSSI_OpenObject2(vssi_session, user_id, dataset_id, &route_info,
                         VSSI_READWRITE, &handle,
                         &test_context, vscs_test_callback);
    }
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Failed to open object %s: %d",
                         save_description, rc);
        rv++;
        goto fail_open;
    }
    if(VSSI_GetVersion(handle) != version) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Object version after reopen after second commit is "FMTu64". Expected "FMTu64".",
                         VSSI_GetVersion(handle), version);
        rv++;
        goto fail;
    }

    filedata = 'X';
    rv += test_vscs_write_object_async(handle, &filedata, 1, 1, SAVE_BLOCKSIZE);
    if(rv > 0) goto fail;

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                   "Read back uncommitted changed block. Confirm it has changed.");
    rv += test_vscs_read_and_verify_async(handle, BVD_BLOCK_DATA,
                                          &filedata, 1, 1,
                                          SAVE_BLOCKSIZE);
    if(rv > 0) goto fail;

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                   "Close and reopen. Confirm version changed, data changed. Close again.");
    VSSI_Commit(handle, &test_context, vscs_test_callback);
    version++;
    VPLSem_Wait(&(test_context.sem));
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "Version "FMTu64" after VSSI_Commit.", VSSI_GetVersion(handle));
    VSSI_CloseObject(handle,
                        &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Failed to close object %s: %d",
                         save_description, rc);
        rv++;
        goto fail;
    }

    if ( use_xml_api ) {
        VSSI_OpenObject(vssi_session, save_description,
                        VSSI_READWRITE, &handle,
                        &test_context, vscs_test_callback);
    }
    else {
        VSSI_OpenObject2(vssi_session, user_id, dataset_id, &route_info,
                         VSSI_READWRITE, &handle,
                         &test_context, vscs_test_callback);
    }
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Failed to open object %s: %d",
                         save_description, rc);
        rv++;
        goto fail_open;
    }
    if(VSSI_GetVersion(handle) != version) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Object version after reopen after second commit is "FMTu64". Expected "FMTu64".",
                         VSSI_GetVersion(handle), version);
        rv++;
        goto fail;
    }

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                   "Read first block. Confirm it's 'X'");
    rv += test_vscs_read_and_verify_async(handle, BVD_BLOCK_DATA,
                                          &filedata, 1, 1,
                                          SAVE_BLOCKSIZE);
    if(rv > 0) goto fail;

    VSSI_CloseObject(handle,
                        &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Failed to close object %s: %d",
                         save_description, rc);
        rv++;
        goto fail;
    }

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Delete the object.");
    if ( use_xml_api ) {
        VSSI_Delete(session, save_description,
                    &test_context, vscs_test_callback);
    }
    else {
        VSSI_Delete2(session, user_id, dataset_id, &route_info,
                     &test_context, vscs_test_callback);
    }
    version++;
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Failed to delete object %s: %d",
                         save_description, rc);
        rv++;
        goto fail;
    }

    if ( use_xml_api ) {
        VSSI_OpenObject(vssi_session, save_description,
                        VSSI_READWRITE, &handle,
                        &test_context, vscs_test_callback);
    }
    else {
        VSSI_OpenObject2(vssi_session, user_id, dataset_id, &route_info,
                         VSSI_READWRITE, &handle,
                         &test_context, vscs_test_callback);
    }
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Failed to open object %s: %d",
                         save_description, rc);
        rv++;
        goto fail_open;
    }
    if(VSSI_GetVersion(handle) != version) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Deleted object version is "FMTu64" reopen. Expected "FMTu64".",
                         VSSI_GetVersion(handle), version);
        rv++;
        goto fail;
    }

    VSSI_CloseObject(handle, &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Close object %s failed: %d.",
                         save_description, rc);
        rv++;
        goto fail;
    }

 fail_open:
 fail:
    VPLSem_Destroy(&(test_context.sem));
    return rv;
}
#endif // NEEDS_CONVERSION

static const char vsTest_vss_user_access_control[] = "VSS User Access Control Test";
static int test_vss_user_access_control(VSSI_Session session,
                                        const char* save_description,
                                        u64 user_id,
                                        u64 dataset_id,
                                        const VSSI_RouteInfo& route_info,
                                        bool use_xml_api)
{
    int rv = 0;
    int rc;
    VSSI_Object handle;
    vscs_test_context_t test_context;
    std::string bad_desc = save_description;

    VPL_SET_UNINITIALIZED(&(test_context.sem));

    vsTest_curTestName = vsTest_vss_user_access_control;
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Starting test: %s.",
                        vsTest_curTestName);

    // modify save description, changing the UID
    if(bad_desc.find("format=\"atomic\"") != string::npos) {
        // Find last '/', then modify character before it - last digit of UID.
        size_t index = bad_desc.rfind('/'); // now in closing XML tag
        index--;
        index = bad_desc.rfind('/', index); // now before TID
        index--;
        index = bad_desc.rfind('/', index); // now before type ID
        index--; // last character of UID
        if(isdigit(bad_desc[index])) {
            bad_desc.replace(index, 1, 1, 'A');
        }
        else {
            bad_desc.replace(index, 1, 1, '0');
        }
    }
    else if(bad_desc.find("format=\"dataset\"") != string::npos) {
        size_t index = bad_desc.find("<uid>") + 5;
        if(isdigit(bad_desc[index])) {
            bad_desc.replace(index, 1, 1, 'A');
        }
        else {
            bad_desc.replace(index, 1, 1, '1');
        }
    }
    else {
        // Can't run test. don't know the format.
        goto exit;
    }

    if(VPLSem_Init(&(test_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        return rv+1;
    }

    if ( use_xml_api ) {
        VSSI_OpenObject(vssi_session, bad_desc.c_str(),
                        VSSI_READWRITE, &handle,
                        &test_context, vscs_test_callback);
    }
    else {
        VSSI_OpenObject2(vssi_session, user_id ^ 0x555, dataset_id, &route_info,
                         VSSI_READWRITE, &handle,
                         &test_context, vscs_test_callback);
    }
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_ACCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSS access wrong user's save state returned %d. Expected %d.",
                         rc, VSSI_ACCESS);
        rv++;
    }
    else {
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "PASS:"
                            "VSS access wrong user's save state blocked.");
    }

    if ( use_xml_api ) {
        VSSI_Delete(vssi_session, bad_desc.c_str(),
                    &test_context, vscs_test_callback);
    }
    else {
        VSSI_Delete2(vssi_session, user_id ^ 0x5555, dataset_id, &route_info,
                    &test_context, vscs_test_callback);
    }
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_ACCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSS delete wrong user's save state returned %d. Expected %d.",
                         rc, VSSI_ACCESS);
        rv++;
    }
    else {
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "PASS:"
                            "VSS delete wrong user's save state blocked.");
    }

    VPLSem_Destroy(&(test_context.sem));

 exit:
    return rv;
}

char myBigBuf[2 * 1024 * 1024];

static const char vsTest_vss_file_handles[] = "VSS File Handle API Test";
static int test_vss_file_handles(VSSI_Session session,
                                  const char* save_description,
                                  u64 user_id,
                                  u64 dataset_id,
                                  const VSSI_RouteInfo& route_info,
                                  bool use_xml_api,
                                  bool use_close_to_release_locks)
{
    int rv = 0;
    int rc;
    VSSI_Object handle;
    vscs_test_context_t test_context;
    vscs_test_context_t unlock_context;
    vscs_test_context_t third_context;
    vscs_notification_context_t notify_context;
    static const char* name = "/fhTestDir1/fhTestDir2/foo.txt";
    static const char* dirname1 = "/fhTestDir1";
    static const char* dirname2 = "/fhTestDir1/fhTestDir2";
    VPLTime_t wait_time;
    u32 flags;
    u32 attrs;
    VSSI_File fileHandle1;
    VSSI_File fileHandle2;
    VSSI_ServerFileId fileId1;
    VSSI_NotifyMask mask;
    u32 rdLen;
    u32 wrLen;
    char myBuf[256];

    VPL_SET_UNINITIALIZED(&(test_context.sem));
    VPL_SET_UNINITIALIZED(&(unlock_context.sem));
    VPL_SET_UNINITIALIZED(&(third_context.sem));
    VPL_SET_UNINITIALIZED(&(notify_context.sem));

    vsTest_curTestName = vsTest_vss_file_handles;
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Starting test: %s.",
                        vsTest_curTestName);

    if(VPLSem_Init(&(test_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        return rv+1;
    }

    if(VPLSem_Init(&(unlock_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        return rv+1;
    }

    if(VPLSem_Init(&(third_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        return rv+1;
    }

    if(VPLSem_Init(&(notify_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        return rv+1;
    }

    // Set initial conditions: delete object contents.
    if(use_xml_api) {
        VSSI_Delete(vssi_session, save_description,
                    &test_context, vscs_test_callback);
    }
    else {
        VSSI_Delete2(vssi_session, user_id, dataset_id, &route_info,
                     &test_context, vscs_test_callback);
    }
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Delete object %s failed: %d.",
                         save_description, rc);
        rv++;
        goto fail_open;
    }

    if(use_xml_api) {
        VSSI_OpenObject(vssi_session, save_description,
                        VSSI_READWRITE | VSSI_FORCE, &handle,
                        &test_context, vscs_test_callback);
    }
    else {
        VSSI_OpenObject2(vssi_session, user_id, dataset_id, &route_info,
                         VSSI_READWRITE | VSSI_FORCE, &handle,
                         &test_context, vscs_test_callback);
    }
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Open object %s failed: %d.",
                         save_description, rc);
        rv++;
        goto fail_open;
    }

    //
    // Create the directories for the test
    //
    attrs = 0;

    // First create second level directory, which should fail
    // since VSSI_MkDir2 does not do "mkdir -p"
    VSSI_MkDir2(handle, dirname2, attrs,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_BADPATH) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_MkDir2 %s returns %d, expected %d.",
                         dirname2, rc, VSSI_BADPATH);
        rv++;
    }

    VSSI_Commit(handle, &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Commit on make directory %s: %d.",
                         dirname2, rc);
        rv++;
    }

    // Now create the parent directory first
    VSSI_MkDir2(handle, dirname1, attrs,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_MkDir2 %s returns %d.", dirname1, rc);
        rv++;
    }

    // Commit is a NOP now, but leave a few of them around just
    // to make sure they don't cause problems.
    VSSI_Commit(handle, &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                            "Commit on make directory %s: %d.",
                            dirname1, rc);
        rv++;
    }

    // Try creating a file in the non-existent second level directory
    // to make sure we get BADPATH instead of NOTFOUND in that case.
    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_CREATE;
    flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;
    attrs = 0;

    VSSI_OpenFile(handle, name, flags, attrs, &fileHandle1,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_BADPATH) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d, expected %d.",
                         rc, VSSI_BADPATH);
        rv++;
    }

    // Creating second level directory should now succeed
    VSSI_MkDir2(handle, dirname2, attrs,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_MkDir2 %s returns %d.", dirname2, rc);
        rv++;
    }

    VSSI_Commit(handle, &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Commit on make directory %s: %d.",
                         dirname2, rc);
        rv++;
    }

    // Now try to create the first directory again, which should fail
    VSSI_MkDir2(handle, dirname1, attrs,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_ISDIR) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_MkDir2 %s returns %d, expected %d.",
                         dirname1, rc, VSSI_ISDIR);
        rv++;
    }

    VSSI_Commit(handle, &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Commit on make directory %s: %d.",
                         dirname2, rc);
        rv++;
    }

    // OpenFile
    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_CREATE;
    flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;
    attrs = 0;

    VSSI_OpenFile(handle, name, flags, attrs, &fileHandle1,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS && rc != VSSI_EXISTS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.", rc);
        rv++;
    }

    fileId1 = VSSI_GetServerFileId(fileHandle1);
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                         "VSSI_OpenFile returns FH1 %p, FID1 %x", fileHandle1, fileId1);

    // Initialize the file
    memset(myBuf, 0x42, sizeof(myBuf));
    wrLen = sizeof(myBuf);
    VSSI_WriteFile(fileHandle1, 0, &wrLen, myBuf,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_WriteFile returned %d.", rc);
        rv++;
    }
    else if(wrLen != sizeof(myBuf)) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_WriteFile wrote %d, expected to write %d.",
                         wrLen, sizeof(myBuf));
        rv++;
    }

    memset(myBigBuf, 0x43, sizeof(myBigBuf));
    wrLen = sizeof(myBigBuf);
    VSSI_WriteFile(fileHandle1, sizeof(myBigBuf), &wrLen, myBigBuf,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_WriteFile returned %d.", rc);
        rv++;
    }
    else if(wrLen != MIN(1<<20, sizeof(myBigBuf))) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_WriteFile wrote %d, expected to write %d.",
                         wrLen, MIN(1<<20, sizeof(myBigBuf)));
        rv++;
    }

    VSSI_CloseFile(fileHandle1,
                   &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_CloseFile returns %d.", rc);
        rv++;
    }

    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_OPEN_ALWAYS;
    flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;

    VSSI_OpenFile(handle, name, flags, attrs, &fileHandle1,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_EXISTS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.", rc);
        rv++;
    }
    // Open a second handle at the same time
    VSSI_OpenFile(handle, name, flags, attrs, &fileHandle2,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_EXISTS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.", rc);
        rv++;
    }
    
    vssiFile[0] = fileHandle1;
    vssiFile[1] = fileHandle2;
    numVssiFiles = 2;

    if(fileId1 == VSSI_GetServerFileId(fileHandle1)) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0,
                         "VSSI_OpenFile returns FH1 %p with non-unique FID %x",
                         fileHandle1, VSSI_GetServerFileId(fileHandle1));
        rv++;
    }
    // Record FileId from new fileHandle1
    fileId1 = VSSI_GetServerFileId(fileHandle1);
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "VSSI_OpenFile returns FH1 %p, FID1 %x", fileHandle1, fileId1);

    if(fileId1 != VSSI_GetServerFileId(fileHandle2)) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0,
                         "VSSI_OpenFile returns FH2 %p with different FID %x than FH1 to same file",
                         fileHandle2, VSSI_GetServerFileId(fileHandle2));
        rv++;
    }

    // Test byte range locking
    VSSI_ByteRangeLock brLock;
    u32 lockFlags;

    brLock.lock_mask = VSSI_FILE_LOCK_READ_SHARED;
    brLock.offset = 0;
    brLock.length = 128;
    lockFlags = VSSI_RANGE_LOCK|VSSI_RANGE_NOWAIT;

    VSSI_SetByteRangeLock(fileHandle1, &brLock, lockFlags,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_SetByteRangeLock returns %d.", rc);
        rv++;
    }

    // Try a conflicting lock on FH2
    brLock.lock_mask = VSSI_FILE_LOCK_WRITE_EXCL;
    brLock.offset = 0;
    brLock.length = 128;
    lockFlags = VSSI_RANGE_LOCK|VSSI_RANGE_NOWAIT;

    VSSI_SetByteRangeLock(fileHandle2, &brLock, lockFlags,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_LOCKED) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_SetByteRangeLock returned %d, expected %d.", rc, VSSI_LOCKED);
        rv++;
    }

    // Try a non-conflicting lock on FH2
    brLock.lock_mask = VSSI_FILE_LOCK_WRITE_EXCL;
    brLock.offset = 128;
    brLock.length = 128;
    lockFlags = VSSI_RANGE_LOCK|VSSI_RANGE_NOWAIT;

    VSSI_SetByteRangeLock(fileHandle2, &brLock, lockFlags,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_SetByteRangeLock returned %d.", rc);
        rv++;
    }

    //
    // Windows LockFile semantics are that overlapping WREXCL locks are
    // not allowed even on the same file descriptor.
    //
    // FH2 should not be able to write lock a subrange of [128,256).
    //
    brLock.lock_mask = VSSI_FILE_LOCK_WRITE_EXCL;
    brLock.offset = 128;
    brLock.length = 16;
    lockFlags = VSSI_RANGE_LOCK|VSSI_RANGE_NOWAIT;

    VSSI_SetByteRangeLock(fileHandle2, &brLock, lockFlags,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_LOCKED) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_SetByteRangeLock returned %d, expected %d.", rc, VSSI_LOCKED);
        rv++;
    }

    // A new WREXCL lock can't overlap with any other lock, even from the same descriptor,
    // even if the pre-existing lock is READ_SHARED
    brLock.lock_mask = VSSI_FILE_LOCK_WRITE_EXCL;
    brLock.offset = 0;
    brLock.length = 128;
    lockFlags = VSSI_RANGE_LOCK|VSSI_RANGE_NOWAIT;

    VSSI_SetByteRangeLock(fileHandle1, &brLock, lockFlags,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_LOCKED) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_SetByteRangeLock returned %d, expected %d.", rc, VSSI_LOCKED);
        rv++;
    }

    // Try actual IO

    // FH2 should be able to write at 128
    wrLen = 128;
    VSSI_WriteFile(fileHandle2, 128, &wrLen, myBuf,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_WriteFile returned %d.", rc);
        rv++;
    }

    // FH1 write at 128 should fail
    wrLen = 128;
    VSSI_WriteFile(fileHandle1, 128, &wrLen, myBuf,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_LOCKED) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_WriteFile returned %d, expecting %d.", rc, VSSI_LOCKED);
        rv++;
    }

    // FH1 has a read shared lock on [0, 128), so write there through FH1 or FH2
    // should fail
    wrLen = 128;
    VSSI_WriteFile(fileHandle1, 0, &wrLen, myBuf,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_LOCKED) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_WriteFile returned %d, expecting %d.", rc, VSSI_LOCKED);
        rv++;
    }

    // FH1 has a read shared lock on [0, 128), so write there through FH1 or FH2
    // should fail
    wrLen = 128;
    VSSI_WriteFile(fileHandle2, 0, &wrLen, myBuf,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_LOCKED) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_WriteFile returned %d, expecting %d.", rc, VSSI_LOCKED);
        rv++;
    }

    // FH1 should not be able to read [128, 256), because FH2 has WREXCL
    rdLen = 128;
    VSSI_ReadFile(fileHandle1, 128, &rdLen, myBuf,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_LOCKED) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_ReadFile returned %d, expecting %d.", rc, VSSI_LOCKED);
        rv++;
    }

    // FH2 should be able to read [0, 256)
    rdLen = 256;
    VSSI_ReadFile(fileHandle2, 0, &rdLen, myBuf,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_ReadFile returned %d.", rc);
        rv++;
    }

    // Add read lock for FH2 over the whole range
    brLock.lock_mask = VSSI_FILE_LOCK_READ_SHARED;
    brLock.offset = 0;
    brLock.length = 256;
    lockFlags = VSSI_RANGE_LOCK|VSSI_RANGE_NOWAIT;

    VSSI_SetByteRangeLock(fileHandle2, &brLock, lockFlags,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_SetByteRangeLock returned %d.", rc);
        rv++;
    }

    // FH1 should still not be able to write to [0,128) (two read locks)
    wrLen = 128;
    VSSI_WriteFile(fileHandle1, 0, &wrLen, myBuf,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_LOCKED) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_WriteFile returned %d, expected %d.", rc, VSSI_LOCKED);
        rv++;
    }

    // FH2 should fail to write at 128 because of its own read lock
    wrLen = 128;
    VSSI_WriteFile(fileHandle2, 128, &wrLen, myBuf,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_LOCKED) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_WriteFile returned %d, expecting %d.", rc, VSSI_LOCKED);
        rv++;
    }

    // Release read lock for FH2 over the whole range
    brLock.lock_mask = VSSI_FILE_LOCK_READ_SHARED;
    brLock.offset = 0;
    brLock.length = 256;
    lockFlags = VSSI_RANGE_UNLOCK|VSSI_RANGE_NOWAIT;

    VSSI_SetByteRangeLock(fileHandle2, &brLock, lockFlags,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_SetByteRangeLock returned %d.", rc);
        rv++;
    }

    //
    // Test byte range locks with blocking behavior
    //

    // FH1 tries for READ on FH2 WREXCL segment, but asks to
    // block and be released when the lock releases
    brLock.lock_mask = VSSI_FILE_LOCK_READ_SHARED;
    brLock.offset = 240;
    brLock.length = 100;
    lockFlags = VSSI_RANGE_LOCK|VSSI_RANGE_WAIT;

    VSSI_SetByteRangeLock(fileHandle1, &brLock, lockFlags,
                          &test_context, vscs_test_callback);

    wait_time = VPLTime_FromMillisec(2000);
    // Timed semaphore wait for 2 seconds to confirm that it blocked
    rc = VPLSem_TimedWait(&(test_context.sem), wait_time);
    if(rc != VPL_ERR_TIMEOUT) {
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "FAIL:"
                            "VPLSem_TimedWait returned %d.", rc);
        rv++;
    }

    //
    // Now try another blocking lock from FH1 which overlaps
    // the FH2 WREXCL but does not conflict with the previous
    // FH1 READ [240,340).
    //
    brLock.lock_mask = VSSI_FILE_LOCK_WRITE_EXCL;
    brLock.offset = 144;
    brLock.length = 4;
    lockFlags = VSSI_RANGE_LOCK|VSSI_RANGE_WAIT;

    VSSI_SetByteRangeLock(fileHandle1, &brLock, lockFlags,
                          &third_context, vscs_test_callback);

    wait_time = VPLTime_FromMillisec(2000);
    // Confirm that it blocked
    rc = VPLSem_TimedWait(&(third_context.sem), wait_time);
    if(rc != VPL_ERR_TIMEOUT) {
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "FAIL:"
                            "VPLSem_TimedWait returned %d.", rc);
        rv++;
    }

    // Check that the first blocking lock is still blocked
    rc = VPLSem_TimedWait(&(test_context.sem), wait_time);
    if(rc != VPL_ERR_TIMEOUT) {
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "FAIL:"
                            "VPLSem_TimedWait returned %d.", rc);
        rv++;
    }

    if (use_close_to_release_locks) {
        //
        // Closing FH2 should release the WREXCL lock and free up
        // the blocked requests
        //
        VSSI_CloseFile(fileHandle2,
                    &unlock_context, vscs_test_callback);
        VPLSem_Wait(&(unlock_context.sem));
        rc = unlock_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                            "VSSI_CloseFile returns %d.", rc);
            rv++;
        }
    }
    else {
        //
        // Now FH2 releases WREXCL and the other two previous locks should retried
        // Note that WAIT is requested, but that is NOP for UNLOCK
        //
        brLock.lock_mask = VSSI_FILE_LOCK_WRITE_EXCL;
        brLock.offset = 128;
        brLock.length = 128;
        lockFlags = VSSI_RANGE_UNLOCK|VSSI_RANGE_WAIT;

        VSSI_SetByteRangeLock(fileHandle2, &brLock, lockFlags,
                            &unlock_context, vscs_test_callback);
        VPLSem_Wait(&(unlock_context.sem));
        rc = unlock_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                            "VSSI_SetByteRangeLock returned %d.", rc);
            rv++;
        }
    }

    // Now go back and wait for the FH1 READ LOCK request
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_SetByteRangeLock returned %d.", rc);
        rv++;
    }

    // Now go back and wait for the FH1 WREXCL LOCK request
    VPLSem_Wait(&(third_context.sem));
    rc = third_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_SetByteRangeLock returned %d.", rc);
        rv++;
    }

    // Release the FH1 READ lock
    brLock.lock_mask = VSSI_FILE_LOCK_READ_SHARED;
    brLock.offset = 240;
    brLock.length = 100;
    lockFlags = VSSI_RANGE_UNLOCK|VSSI_RANGE_WAIT;

    VSSI_SetByteRangeLock(fileHandle1, &brLock, lockFlags,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_SetByteRangeLock returned %d.", rc);
        rv++;
    }

    // Release the FH1 WREXCL [144,148) lock with the wrong
    // spec.  This should fail, since it doesn't match.
    brLock.lock_mask = VSSI_FILE_LOCK_READ_SHARED;
    brLock.offset = 144;
    brLock.length = 4;
    lockFlags = VSSI_RANGE_UNLOCK|VSSI_RANGE_WAIT;

    VSSI_SetByteRangeLock(fileHandle1, &brLock, lockFlags,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_INVALID) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_SetByteRangeLock returned %d, expected %d.", rc, VSSI_INVALID);
        rv++;
    }

    // Release the FH1 WREXCL [144,148) lock with the wrong
    // spec.  This should fail, since it doesn't match.
    // This time match the type, but not the range.
    brLock.lock_mask = VSSI_FILE_LOCK_WRITE_EXCL;
    brLock.offset = 144;
    brLock.length = 3;
    lockFlags = VSSI_RANGE_UNLOCK|VSSI_RANGE_WAIT;

    VSSI_SetByteRangeLock(fileHandle1, &brLock, lockFlags,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_INVALID) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_SetByteRangeLock returned %d, expected %d.", rc, VSSI_INVALID);
        rv++;
    }

    // Release the FH1 WREXCL [144,148) lock
    brLock.lock_mask = VSSI_FILE_LOCK_WRITE_EXCL;
    brLock.offset = 144;
    brLock.length = 4;
    lockFlags = VSSI_RANGE_UNLOCK|VSSI_RANGE_WAIT;

    VSSI_SetByteRangeLock(fileHandle1, &brLock, lockFlags,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_SetByteRangeLock returned %d.", rc);
        rv++;
    }

    // Close both file handles in FIFO order
    VSSI_CloseFile(fileHandle1,
                   &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_CloseFile returns %d.", rc);
        rv++;
    }
    if (!use_close_to_release_locks) {
        VSSI_CloseFile(fileHandle2,
                   &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_CloseFile returns %d.", rc);
            rv++;
        }
    }

    vssiFile[0] = NULL;
    vssiFile[1] = NULL;
    numVssiFiles = 0;

    // Exercise APIs using file handles
    {
        VSSI_Dirent2* stats = NULL;
        u32 truncLen1;
        u32 truncLen2;
        u32 i;
        static const char* name = "myfoo.txt";

        flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_CREATE;
        flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;
        attrs = 0;

        VSSI_OpenFile(handle, name, flags, attrs, &fileHandle1,
                      &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS && rc != VSSI_EXISTS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "VSSI_OpenFile returns %d.", rc);
            rv++;
        }

        // Initialize the file
        for (i = 0; i < sizeof(myBuf); ++i) {
            myBuf[i] = i;
        }

        // Write file with n bytes

        wrLen = sizeof(myBuf);
        VSSI_WriteFile(fileHandle1, 0, &wrLen, myBuf,
                              &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "VSSI_WriteFile returned %d.", rc);
            rv++;
        }

        VSSI_Stat2(handle, name, &stats,
                  &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Stat2 call for %s failed: %d.",
                             name, rc);
            rv++;
        }
        else if(stats->size != wrLen) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Stats size for %s differ from wrLen: "FMTu64" vs. %u.",
                             name, stats->size, wrLen);
            VPLTRACE_LOG_ERR(TRACE_APP, 0,
                             "\tctime: "FMTu64".",
                             stats->ctime);
            VPLTRACE_LOG_ERR(TRACE_APP, 0,
                             "\tmtime: "FMTu64".",
                             stats->mtime);
            VPLTRACE_LOG_ERR(TRACE_APP, 0,
                             "\tchangeVer: "FMTu64".",
                             stats->changeVer);
            VPLTRACE_LOG_ERR(TRACE_APP, 0,
                             "\tisDir: %d.",
                             stats->isDir);
            VPLTRACE_LOG_ERR(TRACE_APP, 0,
                             "\tname: %s.",
                             stats->name);
            rv++;
        }

        if(stats != NULL) {
            free(stats);
            stats = NULL;
        }


        // Just invoke VSSI_SetFileLockState() but don't check the effects
        {
            // Set a read oplock
            VSSI_FileLockState lock_state = VSSI_FILE_LOCK_CACHE_READ;
            VSSI_SetFileLockState(fileHandle1, lock_state,
                          &test_context, vscs_test_callback);
            VPLSem_Wait(&(test_context.sem));
            rc = test_context.rv;
            if(rc != VSSI_SUCCESS) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "VSSI_SetFileLockState returns %d.", rc);
                rv++;
            }

            // Read the state back
            VSSI_GetFileLockState(fileHandle1, &lock_state,
                          &test_context, vscs_test_callback);
            VPLSem_Wait(&(test_context.sem));
            rc = test_context.rv;
            if(rc != VSSI_SUCCESS) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "VSSI_GetFileLockState returns %d.", rc);
                rv++;
            }
            if(lock_state != VSSI_FILE_LOCK_CACHE_READ) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "lock_state "FMTx64", expected %x.", 
                                 lock_state, VSSI_FILE_LOCK_CACHE_READ);
                rv++;
            }

            // Set a write oplock - this should override (replace) the READ
            lock_state = VSSI_FILE_LOCK_CACHE_WRITE;
            VSSI_SetFileLockState(fileHandle1, lock_state,
                          &test_context, vscs_test_callback);
            VPLSem_Wait(&(test_context.sem));
            rc = test_context.rv;
            if(rc != VSSI_SUCCESS) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "VSSI_SetFileLockState returns %d.", rc);
                rv++;
            }

            // Read the state back
            VSSI_GetFileLockState(fileHandle1, &lock_state,
                          &test_context, vscs_test_callback);
            VPLSem_Wait(&(test_context.sem));
            rc = test_context.rv;
            if(rc != VSSI_SUCCESS) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "VSSI_GetFileLockState returns %d.", rc);
                rv++;
            }
            if(lock_state != VSSI_FILE_LOCK_CACHE_WRITE) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "lock_state "FMTx64", expected %x.", 
                                 lock_state, VSSI_FILE_LOCK_CACHE_WRITE);
                rv++;
            }

            // Clear the write oplock
            lock_state = VSSI_FILE_LOCK_NONE;
            VSSI_SetFileLockState(fileHandle1, lock_state,
                          &test_context, vscs_test_callback);
            VPLSem_Wait(&(test_context.sem));
            rc = test_context.rv;
            if(rc != VSSI_SUCCESS) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "VSSI_SetFileLockState returns %d.", rc);
                rv++;
            }

            // Read the state back
            VSSI_GetFileLockState(fileHandle1, &lock_state,
                          &test_context, vscs_test_callback);
            VPLSem_Wait(&(test_context.sem));
            rc = test_context.rv;
            if(rc != VSSI_SUCCESS) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "VSSI_GetFileLockState returns %d.", rc);
                rv++;
            }
            if(lock_state != VSSI_FILE_LOCK_NONE) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "lock_state "FMTx64", expected %x.", 
                                 lock_state, VSSI_FILE_LOCK_NONE);
                rv++;
            }

            // Set a read+write oplock
            lock_state = VSSI_FILE_LOCK_CACHE_READ | VSSI_FILE_LOCK_CACHE_WRITE;
            VSSI_SetFileLockState(fileHandle1, lock_state,
                          &test_context, vscs_test_callback);
            VPLSem_Wait(&(test_context.sem));
            rc = test_context.rv;
            if(rc != VSSI_SUCCESS) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "VSSI_SetFileLockState returns %d.", rc);
                rv++;
            }

            // Read the state back
            VSSI_GetFileLockState(fileHandle1, &lock_state,
                          &test_context, vscs_test_callback);
            VPLSem_Wait(&(test_context.sem));
            rc = test_context.rv;
            if(rc != VSSI_SUCCESS) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "VSSI_GetFileLockState returns %d.", rc);
                rv++;
            }
            if(lock_state != (VSSI_FILE_LOCK_CACHE_READ|VSSI_FILE_LOCK_CACHE_WRITE)) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "lock_state "FMTx64", expected %x.", 
                                 lock_state, VSSI_FILE_LOCK_CACHE_READ|VSSI_FILE_LOCK_CACHE_WRITE);
                rv++;
            }

            // Clear the read/write oplock
            lock_state = VSSI_FILE_LOCK_NONE;
            VSSI_SetFileLockState(fileHandle1, lock_state,
                          &test_context, vscs_test_callback);
            VPLSem_Wait(&(test_context.sem));
            rc = test_context.rv;
            if(rc != VSSI_SUCCESS) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "VSSI_SetFileLockState returns %d.", rc);
                rv++;
            }

            // Read the state back
            VSSI_GetFileLockState(fileHandle1, &lock_state,
                          &test_context, vscs_test_callback);
            VPLSem_Wait(&(test_context.sem));
            rc = test_context.rv;
            if(rc != VSSI_SUCCESS) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "VSSI_GetFileLockState returns %d.", rc);
                rv++;
            }
            if(lock_state != VSSI_FILE_LOCK_NONE) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "lock_state "FMTx64", expected %x.", 
                                 lock_state, VSSI_FILE_LOCK_NONE);
                rv++;
            }
        }

        // Truncate file to n/3 bytes

        truncLen1 = sizeof(myBuf)/3;
        VSSI_TruncateFile(fileHandle1, truncLen1,
                       &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "VSSI_TruncateFile returned %d.", rc);
            rv++;
        }


        VSSI_Stat2(handle, name, &stats,
                  &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Stat2 call for %s failed: %d.",
                             name, rc);
            rv++;
        }

        else if(stats->size != truncLen1) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Stats size for %s differ from truncLen1: "FMTu64" vs. %u.",
                             name, stats->size, truncLen1);
            VPLTRACE_LOG_ERR(TRACE_APP, 0,
                             "\tctime: "FMTu64".",
                             stats->ctime);
            VPLTRACE_LOG_ERR(TRACE_APP, 0,
                             "\tmtime: "FMTu64".",
                             stats->mtime);
            VPLTRACE_LOG_ERR(TRACE_APP, 0,
                             "\tchangeVer: "FMTu64".",
                             stats->changeVer);
            VPLTRACE_LOG_ERR(TRACE_APP, 0,
                             "\tisDir: %d.",
                             stats->isDir);
            VPLTRACE_LOG_ERR(TRACE_APP, 0,
                             "\tname: %s.",
                             stats->name);
            rv++;
        }

        if(stats != NULL) {
            free(stats);
            stats = NULL;
        }

        // Truncate file to increase size to  n * 2/3 bytes

        truncLen2 = (sizeof(myBuf)*2)/3;
        VSSI_TruncateFile(fileHandle1, truncLen2,
                       &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "VSSI_TruncateFile returned %d.", rc);
            rv++;
        }


        VSSI_Stat2(handle, name, &stats,
                  &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Stat2 call for %s failed: %d.",
                             name, rc);
            rv++;
        }
        else if(stats->size != truncLen2) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Stats size for %s differ from truncLen2: "FMTu64" vs. %u.",
                             name, stats->size, truncLen2);
            VPLTRACE_LOG_ERR(TRACE_APP, 0,
                             "\tctime: "FMTu64".",
                             stats->ctime);
            VPLTRACE_LOG_ERR(TRACE_APP, 0,
                             "\tmtime: "FMTu64".",
                             stats->mtime);
            VPLTRACE_LOG_ERR(TRACE_APP, 0,
                             "\tchangeVer: "FMTu64".",
                             stats->changeVer);
            VPLTRACE_LOG_ERR(TRACE_APP, 0,
                             "\tisDir: %d.",
                             stats->isDir);
            VPLTRACE_LOG_ERR(TRACE_APP, 0,
                             "\tname: %s.",
                             stats->name);
            rv++;
        }

        if(stats != NULL) {
            free(stats);
            stats = NULL;
        }

        rdLen = sizeof(myBuf);
        VSSI_ReadFile(fileHandle1, 0, &rdLen, myBuf,
                              &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "VSSI_ReadFile returned %d.", rc);
            rv++;
        }
        else if(rdLen != truncLen2) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "VSSI_ReadFile size for %s differ from truncLen2: %u vs. %u.",
                             name, rdLen, truncLen2);
            rv++;
        }
        else for (i = 0; i < rdLen; ++i) {
            if (i < truncLen1) {
                if ((u8)myBuf[i] != i) {
                    VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                        "VSSI_ReadFile %s myBuf[%u] expected 0x%02x got 0x%02x.",
                                        name, i, i, (u8)myBuf[i]);
                    rv++;
                    break;
                }
            }
            else {
                if (myBuf[i] != 0) {
                    VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                        "VSSI_ReadFile %s myBuf[%u] expected 0 got 0x%02x.",
                                        name, i, (u8)myBuf[i]);
                    rv++;
                    break;
                }
            }
        }

        VSSI_CloseFile(fileHandle1,
                       &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "VSSI_CloseFile returns %d.", rc);
            rv++;
        }

    }

    VPL_SET_UNINITIALIZED(&(disconnect_context.sem));
    if(VPLSem_Init(&(disconnect_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        return rv+1;
    }

    // Turn off notification for oplock breaks
    mask = VSSI_NOTIFY_DISCONNECTED_EVENT;
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Set VSSI_NOTIFY_DISCONNECT_EVENT Event, set mask "FMTx64, mask);
    VSSI_SetNotifyEvents(handle, &mask, &disconnect_context,
                         vscs_disconnect_callback,
                         &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Set Notify Events returned result %d.",
                         rc);
        rv++;
    }

    // Close the VSS Object
    VSSI_CloseObject(handle,
                        &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Failed to close object %s: %d",
                         save_description, rc);
        rv++;
    }

 fail_open:
    VPLSem_Destroy(&(test_context.sem));
    VPLSem_Destroy(&(unlock_context.sem));
    VPLSem_Destroy(&(third_context.sem));
    VPLSem_Destroy(&(notify_context.sem));
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                   "Test %s result: %d.",
                   vsTest_curTestName, rv);

    return rv;
}


static const char vsTest_vss_file_rename_remove[] = "VSS File Rename and Remove Test";
static int test_vss_file_rename_remove(VSSI_Session session,
                                       const char* save_description,
                                       u64 user_id,
                                       u64 dataset_id,
                                       const VSSI_RouteInfo& route_info,
                                       bool use_xml_api)
{
    int rv = 0;
    int rc;
    VSSI_Object handle;
    vscs_test_context_t test_context;
    vscs_test_context_t unlock_context;
    vscs_test_context_t third_context;
    static const char* name1 = "/renameTestDir1/bar.txt";
    static const char* NaMe1 = "/renameTestDir1/BaR.TXt";
    static const char* name2 = "/renameTestDir1/foo.txt";
    static const char* name3 = "/otherTestDir1/foo.txt";
    static const char* name4 = "/renameTestDir1/boohoo.txt";
    static const char* dirname1 = "/renameTestDir1";
    static const char* dirname2 = "/renameTestDirOther1";
    static const char* dirname3 = "/otherTestDir1";

    // in the following comments
    // XXX and YYY and ZZZ means string of Chinese UTF-8 characters
    // 'a' means LATIN SMALL LETTER A WITH DIAERESIS AND MACRON (UTF-8: c79fh), 'A' means LATIN CAPITAL LETTER A WITH DIAERESIS AND MACRON (UTF-8: c79eh)
    // 'b' means LATIN SMALL LETTER B WITH HOOK (UTF-8: c993h), 'B' means LATIN CAPITAL LETTER B WITH HOOK (UTF-8: c681h)
    static const char* dirnameUTF8_a = "\xe5\xae\x8f\xe7\xa2\x81\xc7\x9f\x0"; // /XXXa
    static const char* dirnameUTF8_A = "\xe5\xae\x8f\xe7\xa2\x81\xc7\x9e\x0"; // /XXXA
    static const char* nameUTF8_AA = "\xe5\xae\x8f\xe7\xa2\x81\xc7\x9e\x2f\xe9\x9b\xb2\xe7\xab\xaf\xe6\xb8\xac\xe8\xa9\xa6\xe8\xb3\x87\xe6\x96\x99\xc7\x9e\x0"; // /XXXA/YYYA
    static const char* nameUTF8_aa = "\xe5\xae\x8f\xe7\xa2\x81\xc7\x9f\x2f\xe9\x9b\xb2\xe7\xab\xaf\xe6\xb8\xac\xe8\xa9\xa6\xe8\xb3\x87\xe6\x96\x99\xc7\x9f\x0"; // /XXXa/YYYa
    static const char* nameUTF8_Aa = "\xe5\xae\x8f\xe7\xa2\x81\xc7\x9e\x2f\xe9\x9b\xb2\xe7\xab\xaf\xe6\xb8\xac\xe8\xa9\xa6\xe8\xb3\x87\xe6\x96\x99\xc7\x9f\x0"; // /XXXA/YYYa
    static const char* nameUTF8_ab = "\xe5\xae\x8f\xe7\xa2\x81\xc7\x9f\x2f\xe9\x87\x8d\xe6\x96\xb0\xe5\x91\xbd\xe5\x90\x8d\xc9\x93\x0"; // /XXXa/ZZZb
    static const char* nameUTF8_AB = "\xe5\xae\x8f\xe7\xa2\x81\xc7\x9e\x2f\xe9\x87\x8d\xe6\x96\xb0\xe5\x91\xbd\xe5\x90\x8d\xc6\x81\x0"; // /XXXA/ZZZB

    u32 flags;
    u32 attrs;
    VSSI_File fileHandle1;
    VSSI_File fileHandle2;
    VSSI_File fileHandle3;
    u32 rdLen;
    u32 wrLen;
    char myBuf[256];
    u64 diskSize;
    u64 datasetSize;
    u64 datasetFree;
    VPLTime_t start, end, elapsed;
    VSSI_FileLockState lock_state;

    VPL_SET_UNINITIALIZED(&(test_context.sem));
    VPL_SET_UNINITIALIZED(&(unlock_context.sem));
    VPL_SET_UNINITIALIZED(&(third_context.sem));

    vsTest_curTestName = vsTest_vss_file_rename_remove;
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Starting test: %s.",
                        vsTest_curTestName);

    if(VPLSem_Init(&(test_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        return rv+1;
    }

    if(VPLSem_Init(&(unlock_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        return rv+1;
    }

    if(VPLSem_Init(&(third_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        return rv+1;
    }

    if(use_xml_api) {
        VSSI_OpenObject(vssi_session, save_description,
                        VSSI_READWRITE | VSSI_FORCE, &handle,
                        &test_context, vscs_test_callback);
    }
    else {
        VSSI_OpenObject2(vssi_session, user_id, dataset_id, &route_info,
                         VSSI_READWRITE | VSSI_FORCE, &handle,
                         &test_context, vscs_test_callback);
    }
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Open object %s failed: %d.",
                         save_description, rc);
        rv++;
        goto fail_open;
    }

    //
    // Create the directory for the test
    //
    attrs = 0;
    VSSI_MkDir2(handle, dirname1, attrs,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_MkDir2 %s returns %d.", dirname1, rc);
        rv++;
    }

    // OpenFile
    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_CREATE;
    flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;
    attrs = 0;

    VSSI_OpenFile(handle, name1, flags, attrs, &fileHandle1,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS && rc != VSSI_EXISTS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.", rc);
        rv++;
    }

    // Initialize the file
    memset(myBuf, 0x42, sizeof(myBuf));
    wrLen = sizeof(myBuf);
    VSSI_WriteFile(fileHandle1, 0, &wrLen, myBuf,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_WriteFile returned %d.", rc);
        rv++;
    }

    VSSI_CloseFile(fileHandle1,
                   &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_CloseFile returns %d.", rc);
        rv++;
    }

    // Create a second file
    VSSI_OpenFile(handle, name2, flags, attrs, &fileHandle1,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS && rc != VSSI_EXISTS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.", rc);
        rv++;
    }

    // Initialize the file
    memset(myBuf, 0x43, sizeof(myBuf));
    wrLen = sizeof(myBuf);
    VSSI_WriteFile(fileHandle1, 0, &wrLen, myBuf,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_WriteFile returned %d.", rc);
        rv++;
    }

    VSSI_CloseFile(fileHandle1,
                   &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_CloseFile returns %d.", rc);
        rv++;
    }

    // With all the files closed, try renaming a non-existent
    // file.  This should fail.  Duh.
    VSSI_Rename(handle, name3, name1,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_NOTFOUND) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Rename component %s to %s: %d expected %d.",
                         name3, name1, rc, VSSI_NOTFOUND);
        rv++;
    }

    // With both files closed, try renaming and clobbering an
    // existing target.  This should fail.
    VSSI_Rename2(handle, name1, name2, 0,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_FEXISTS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Rename component %s to %s: %d expected %d.",
                         name1, name2, rc, VSSI_FEXISTS);
        rv++;
    }

    // With both files closed, try renaming into a non-existent
    // directory.  This should fail.
    VSSI_Rename(handle, name1, name3,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_BADPATH) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Rename component %s to %s: %d expected %d.",
                         name1, name3, rc, VSSI_BADPATH);
        rv++;
    }

    // With both files closed, try renaming and clobbering an
    // existing target with REPLACE_EXISTING.  This should succeed.
    VSSI_Rename2(handle, name1, name2, VSSI_RENAME_REPLACE_EXISTING,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Rename2 component %s to %s (replace existing): %d.",
                         name1, name2, rc);
        rv++;
    }

    // Rename it back (REPLACE_EXISTING, but it doesn't exist)
    VSSI_Rename2(handle, name2, name1, VSSI_RENAME_REPLACE_EXISTING,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Rename2 component %s to %s: %d.",
                         name2, name1, rc);
        rv++;
    }

    // Recreate a second file and hold it open
    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_CREATE;
    flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;
    attrs = 0;

    VSSI_OpenFile(handle, name2, flags, attrs, &fileHandle1,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS && rc != VSSI_EXISTS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.", rc);
        rv++;
    }

    // Initialize the file
    memset(myBuf, 0x44, sizeof(myBuf));
    wrLen = sizeof(myBuf);
    VSSI_WriteFile(fileHandle1, 0, &wrLen, myBuf,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_WriteFile returned %d.", rc);
        rv++;
    }

    // Open a second handle at the same time
    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE;
    flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;
    attrs = 0;

    VSSI_OpenFile(handle, name2, flags, attrs, &fileHandle2,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS && rc != VSSI_EXISTS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.", rc);
        rv++;
    }
   
    // With name2 open twice, try to rename over it with REPLACE_EXISTING.
    // This should fail.
    VSSI_Rename2(handle, name1, name2, VSSI_RENAME_REPLACE_EXISTING,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_BUSY) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Rename component %s to %s: %d expected %d.",
                         name1, name2, rc, VSSI_BUSY);
        rv++;
    }

    // Now close it one handle
    VSSI_CloseFile(fileHandle2,
                   &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_CloseFile returns %d.", rc);
        rv++;
    }

    // Now rename over the target and it should work this time
    VSSI_Rename2(handle, name1, name2, VSSI_RENAME_REPLACE_EXISTING,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Rename2 component %s to %s (replace existing): %d.",
                         name1, name2, rc);
        rv++;
    }

    // Rename it back (REPLACE_EXISTING, but it doesn't exist)
    VSSI_Rename2(handle, name2, name1, VSSI_RENAME_REPLACE_EXISTING,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Rename2 component %s to %s: %d.",
                         name2, name1, rc);
        rv++;
    }

    //
    // The file handle to the deleted file should still work until it
    // is closed. Check that and make sure the data is what we expect.
    //
    memset(myBuf, 0x55, sizeof(myBuf));
    rdLen = sizeof(myBuf);
    VSSI_ReadFile(fileHandle1, 0, &rdLen, myBuf,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_ReadFile returned %d.", rc);
        rv++;
    }
    if(rdLen != sizeof(myBuf)) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_ReadFile returned %d bytes, exp %d.",
                         rdLen, sizeof(myBuf));
        rv++;
    }
    if(myBuf[0] != 0x44) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_ReadFile data wrong: get %02x, exp %02x",
                         myBuf[0], 0x44);
        rv++;
    }

    // Now close the last handle to the deleted file
    VSSI_CloseFile(fileHandle1,
                   &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_CloseFile returns %d.", rc);
        rv++;
    }

    // Rename a file to itself.  This should succeed.
    VSSI_Rename(handle, name1, name1,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Rename component %s to %s: %d.",
                         name1, name1, rc);
        rv++;
    }

    // Rename a file to itself using different case.  This will
    // succeed in either case-sensitive or insensitive, but
    // the end result is different.
    VSSI_Rename(handle, name1, NaMe1,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Rename component %s to %s: %d.",
                         name1, NaMe1, rc);
        rv++;
    }

    // Rename it back, so that subsequent tests work if this is run
    // on a case-sensitive dataset.
    VSSI_Rename(handle, NaMe1, name1,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Rename component %s to %s: %d.",
                         NaMe1, name1, rc);
        rv++;
    }

    // Open the file again without OPEN_DELETE
    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_OPEN_ALWAYS;
    flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;

    VSSI_OpenFile(handle, name1, flags, attrs, &fileHandle1,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_EXISTS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.", rc);
        rv++;
    }

    // Open a second handle at the same time
    VSSI_OpenFile(handle, name1, flags, attrs, &fileHandle2,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_EXISTS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.", rc);
        rv++;
    }
   
    // Make the other directory required for name3
    attrs = 0;
    VSSI_MkDir2(handle, dirname3, attrs,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_MkDir2 %s returns %d.", dirname3, rc);
        rv++;
    }

    // Open a third handle to a different file in a different
    // directory
    VSSI_OpenFile(handle, name3, flags, attrs, &fileHandle3,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS && rc != VSSI_EXISTS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.", rc);
        rv++;
    }
    
    // The remove logic should prevent an open file from being deleted
    // unless OPEN_DELETE is specified
    VSSI_Remove(handle, name1,
                &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_BUSY) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_Remove component %s: %d (expected %d).",
                         name1, rc, VSSI_BUSY);
        rv++;
    }

    // The remove logic should prevent a non-empty directory being deleted
    VSSI_Remove(handle, dirname1,
                &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_BUSY) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_Remove component %s: %d (expected %d).",
                         dirname1, rc, VSSI_BUSY);
        rv++;
    }

    // Now try renaming the open file itself.  If OPEN_DELETE is false,
    // this should fail
    VSSI_Rename(handle, name1, name2,
                         &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_BUSY) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Rename component %s to %s: %d expected %d.",
                         name1, name2, rc, VSSI_BUSY);
        rv++;
    }

    // Now try renaming a directory that contains an open file
    // This should fail
    VSSI_Rename(handle, dirname1, dirname2,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_BUSY) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Rename component %s to %s: %d expected %d.",
                         dirname1, dirname2, rc, VSSI_BUSY);
        rv++;
    }

    // Close one of the files
    VSSI_CloseFile(fileHandle1,
                   &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_CloseFile returns %d.", rc);
        rv++;
    }

    // Check that the rename still fails
    VSSI_Rename(handle, dirname1, dirname2,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_BUSY) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Rename component %s to %s: %d expected %d.",
                         dirname1, dirname2, rc, VSSI_BUSY);
        rv++;
    }

    // Close the other file handle
    VSSI_CloseFile(fileHandle2,
                   &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_CloseFile returns %d.", rc);
        rv++;
    }

    // Now the rename should succeed
    VSSI_Rename(handle, dirname1, dirname2,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Rename component %s to %s: %d.",
                         dirname1, dirname2, rc);
        rv++;
    }

    // Rename it back
    VSSI_Rename(handle, dirname2, dirname1,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Rename component %s to %s: %d.",
                         dirname2, dirname1, rc);
        rv++;
    }

    // Reopen the files, but with OPEN_DELETE this time
    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_OPEN_ALWAYS;
    flags |= VSSI_FILE_OPEN_DELETE|VSSI_FILE_SHARE_DELETE;
    flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;
    VSSI_OpenFile(handle, name1, flags, attrs, &fileHandle1,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_EXISTS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.", rc);
        rv++;
    }

    // Open a second handle at the same time with the same flags
    VSSI_OpenFile(handle, name1, flags, attrs, &fileHandle2,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_EXISTS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.", rc);
        rv++;
    }

    // Rename directory should still fail
    VSSI_Rename(handle, dirname1, dirname2,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_BUSY) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Rename component %s to %s: %d expected %d.",
                         dirname1, dirname2, rc, VSSI_BUSY);
        rv++;
    }
    
    // Now renaming the file should succeed
    VSSI_Rename(handle, name1, name4,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Rename component %s to %s: %d.",
                         name1, name4, rc);
        rv++;
    }

    // Rename it back
    VSSI_Rename(handle, name4, name1,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Rename component %s to %s: %d.",
                         name4, name1, rc);
        rv++;
    }

    // The remove of the file should succeed this time
    VSSI_Remove(handle, name1,
                &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_Remove component %s: %d.",
                         name1, rc);
        rv++;
    }

    // Close extraneous fd so that we can try reopen
    VSSI_CloseFile(fileHandle3,
                   &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_CloseFile returns %d.", rc);
        rv++;
    }

    // Check that the file is actually gone from the namespace.
    // No CREATE or OPEN_ALWAYS to avoid creating a different file.
    flags = VSSI_FILE_OPEN_READ;
    flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;
    flags |= VSSI_FILE_SHARE_DELETE;
    VSSI_OpenFile(handle, name1, flags, attrs, &fileHandle3,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_NOTFOUND) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d, expected %d.",
                         rc, VSSI_NOTFOUND);
        rv++;
    }

    // Check available space
    // Unfortunately it appears that the underlying statvfs is not
    // predictable enough on NFS to create an actual test here.
    VSSI_GetSpace(handle, &diskSize, &datasetSize, &datasetFree,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_GetSpace returns %d.",
                         rc);
        rv++;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "VSSI_GetSpace before extending deleted file: "
                        "disk "FMTu64", dsSize "FMTu64", avail "FMTu64".",
                        diskSize, datasetSize, datasetFree);

    // The open file handles should still allow IO

    // IO should work on either descriptor for the deleted file
    memset(myBuf, 0x44, sizeof(myBuf));
    rdLen = sizeof(myBuf);
    VSSI_ReadFile(fileHandle1, 0, &rdLen, myBuf,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_ReadFile returned %d.", rc);
        rv++;
    }
    if(rdLen != sizeof(myBuf)) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_ReadFile returned %d bytes, exp %d.",
                         rdLen, sizeof(myBuf));
        rv++;
    }
    if(myBuf[0] != 0x42) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_ReadFile data wrong: get %02x, exp %02x",
                         myBuf[0], 0x42);
        rv++;
    }

    // Write through the other file handle
    memset(myBuf, 0x47, sizeof(myBuf));
    wrLen = sizeof(myBuf);
    VSSI_WriteFile(fileHandle2, 0, &wrLen, myBuf,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_WriteFile returned %d.", rc);
        rv++;
    }
    if(wrLen != sizeof(myBuf)) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_WriteFile returned %d bytes, exp %d.",
                         wrLen, sizeof(myBuf));
        rv++;
    }

    //
    // Test byte range locks with blocking behavior on the file
    // handles for the now-deleted file.
    //
    VSSI_ByteRangeLock brLock;
    u32 lockFlags;
    VPLTime_t wait_time;

    // Set WREXCL [128,256) on FH2
    brLock.lock_mask = VSSI_FILE_LOCK_WRITE_EXCL;
    brLock.offset = 128;
    brLock.length = 128;
    lockFlags = VSSI_RANGE_LOCK|VSSI_RANGE_NOWAIT;

    VSSI_SetByteRangeLock(fileHandle2, &brLock, lockFlags,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_SetByteRangeLock returned %d.", rc);
        rv++;
    }

    // Try to set a read oplock.  This should be rejected now
    // that there is a byte range lock on the file.
    lock_state = VSSI_FILE_LOCK_CACHE_READ;
    VSSI_SetFileLockState(fileHandle1, lock_state,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_LOCKED) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_SetFileLockState returns %d, expected %d.",
                         rc, VSSI_LOCKED);
        rv++;
    }

    // FH1 tries for READ on FH2 WREXCL segment, but asks to
    // block and be released when the lock releases
    brLock.lock_mask = VSSI_FILE_LOCK_READ_SHARED;
    brLock.offset = 240;
    brLock.length = 100;
    lockFlags = VSSI_RANGE_LOCK|VSSI_RANGE_WAIT;

    VSSI_SetByteRangeLock(fileHandle1, &brLock, lockFlags,
                          &test_context, vscs_test_callback);

    wait_time = VPLTime_FromMillisec(2000);
    // Timed semaphore wait for 2 seconds to confirm that it blocked
    rc = VPLSem_TimedWait(&(test_context.sem), wait_time);
    if(rc != VPL_ERR_TIMEOUT) {
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "FAIL:"
                            "VPLSem_TimedWait returned %d.", rc);
        rv++;
    }

    //
    // Now try another blocking lock from FH1 which overlaps
    // the FH2 WREXCL but does not conflict with the previous
    // FH1 READ [240,340).
    //
    brLock.lock_mask = VSSI_FILE_LOCK_WRITE_EXCL;
    brLock.offset = 144;
    brLock.length = 4;
    lockFlags = VSSI_RANGE_LOCK|VSSI_RANGE_WAIT;

    VSSI_SetByteRangeLock(fileHandle1, &brLock, lockFlags,
                          &third_context, vscs_test_callback);

    wait_time = VPLTime_FromMillisec(2000);
    // Confirm that it blocked
    rc = VPLSem_TimedWait(&(third_context.sem), wait_time);
    if(rc != VPL_ERR_TIMEOUT) {
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "FAIL:"
                            "VPLSem_TimedWait returned %d.", rc);
        rv++;
    }

    // Check that the first blocking lock is still blocked
    rc = VPLSem_TimedWait(&(test_context.sem), wait_time);
    if(rc != VPL_ERR_TIMEOUT) {
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "FAIL:"
                            "VPLSem_TimedWait returned %d.", rc);
        rv++;
    }

    //
    // VSSI_ReleaseFile on FH2 should release the WREXCL lock and free up
    // the blocked requests
    //
    VSSI_ReleaseFile(fileHandle2,
                   &unlock_context, vscs_test_callback);
    VPLSem_Wait(&(unlock_context.sem));
    rc = unlock_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                        "VSSI_ReleaseFile returns %d.", rc);
        rv++;
    }

    // Now go back and wait for the FH1 READ LOCK request
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_SetByteRangeLock returned %d.", rc);
        rv++;
    }

    // Now go back and wait for the FH1 WREXCL LOCK request
    VPLSem_Wait(&(third_context.sem));
    rc = third_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_SetByteRangeLock returned %d.", rc);
        rv++;
    }

    // Create a new file with the same name
    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_CREATE;
    flags |= VSSI_FILE_OPEN_DELETE|VSSI_FILE_SHARE_DELETE;
    flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;

    VSSI_OpenFile(handle, name1, flags, attrs, &fileHandle3,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.",
                         rc);
        rv++;
    }

    // Write through the new handle to the new file
    memset(myBuf, 0x5e, sizeof(myBuf));
    wrLen = sizeof(myBuf);
    VSSI_WriteFile(fileHandle3, 0, &wrLen, myBuf,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_WriteFile returned %d.", rc);
        rv++;
    }
    if(wrLen != sizeof(myBuf)) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_WriteFile returned %d bytes, exp %d.",
                         wrLen, sizeof(myBuf));
        rv++;
    }

    // Now close FH1 for the removed file, which releases locks it has
    VSSI_CloseFile(fileHandle1,
                   &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_CloseFile returns %d.", rc);
        rv++;
    }

    // Try IO, should still work through the remaining handle
    memset(myBuf, 0x44, sizeof(myBuf));
    rdLen = sizeof(myBuf);
    start = VPLTime_GetTimeStamp();
    VSSI_ReadFile(fileHandle2, 0, &rdLen, myBuf,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    end = VPLTime_GetTimeStamp();
    elapsed = VPLTime_DiffClamp(end, start);
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "VSSI_ReadFile() took "FMT_VPLTime_t"us to complete.", elapsed);
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_ReadFile returned %d.", rc);
        rv++;
    }
    if(rdLen != sizeof(myBuf)) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_ReadFile returned %d bytes, exp %d.",
                         rdLen, sizeof(myBuf));
        rv++;
    }
    if(myBuf[0] != 0x47) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_ReadFile data wrong: get %02x, exp %02x",
                         myBuf[0], 0x47);
        rv++;
    }

#define BIG_FILE_SIZE (10 * 1024 * 1024)
#define CHUNK_SIZE    (1024 * 1024)

    // Extend the file so that the space is noticeable
    memset(myBigBuf, 0x37, sizeof(myBigBuf));
    for (u64 bytesWritten = 0; bytesWritten < BIG_FILE_SIZE; ) {
        wrLen = CHUNK_SIZE;
        start = VPLTime_GetTimeStamp();
        VSSI_WriteFile(fileHandle2, bytesWritten, &wrLen, myBigBuf,
                          &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        end = VPLTime_GetTimeStamp();
        elapsed = VPLTime_DiffClamp(end, start);
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "VSSI_WriteFile() took "FMT_VPLTime_t"us to complete.", elapsed);
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                            "VSSI_WriteFile returned %d.", rc);
            rv++;
        }
        if(wrLen != CHUNK_SIZE) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                            "VSSI_WriteFile returned %d bytes, exp %d.",
                            wrLen, CHUNK_SIZE);
            rv++;
        }
        bytesWritten += wrLen;
    }
    
    // Check available space after extending the file
    VSSI_GetSpace(handle, &diskSize, &datasetSize, &datasetFree,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_GetSpace returns %d.",
                         rc);
        rv++;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "VSSI_GetSpace after extending deleted file: "
                        "disk "FMTu64", dsSize "FMTu64", avail "FMTu64".",
                        diskSize, datasetSize, datasetFree);

    // Close the remaining handle to the original deleted file
    VSSI_CloseFile(fileHandle2,
                   &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_CloseFile returns %d.", rc);
        rv++;
    }

    // Check available space.  The file should really have disappeared and
    // the disk space freed.
    VSSI_GetSpace(handle, &diskSize, &datasetSize, &datasetFree,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_GetSpace returns %d.",
                         rc);
        rv++;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "VSSI_GetSpace after closing deleted file: "
                        "disk "FMTu64", dsSize "FMTu64", avail "FMTu64".",
                        diskSize, datasetSize, datasetFree);

    // Close and reopen the new file and make sure it has the correct contents
    VSSI_CloseFile(fileHandle3,
                   &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_CloseFile returns %d.", rc);
        rv++;
    }

    // Open with no create
    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE;
    flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;

    VSSI_OpenFile(handle, name1, flags, attrs, &fileHandle3,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.",
                         rc);
        rv++;
    }

    // Read the new file again and make sure the data is
    // what we expect
    memset(myBuf, 0x61, sizeof(myBuf));
    rdLen = sizeof(myBuf);
    VSSI_ReadFile(fileHandle3, 0, &rdLen, myBuf,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_ReadFile returned %d.", rc);
        rv++;
    }
    if(rdLen != sizeof(myBuf)) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_ReadFile returned %d bytes, exp %d.",
                         rdLen, sizeof(myBuf));
        rv++;
    }
    if(myBuf[0] != 0x5e) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_ReadFile data wrong: get %02x, exp %02x",
                         myBuf[0], 0x5e);
        rv++;
    }

    // Extend the new file so that the space is noticeable
    memset(myBigBuf, 0x38, sizeof(myBigBuf));
    for (u64 bytesWritten = 0; bytesWritten < BIG_FILE_SIZE * 2; ) {
        wrLen = CHUNK_SIZE;
        VSSI_WriteFile(fileHandle3, bytesWritten, &wrLen, myBigBuf,
                          &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                            "VSSI_WriteFile returned %d.", rc);
            rv++;
        }
        if(wrLen != CHUNK_SIZE) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                            "VSSI_WriteFile returned %d bytes, exp %d.",
                            wrLen, CHUNK_SIZE);
            rv++;
        }
        bytesWritten += wrLen;
    }
    
    // Close the new file
    VSSI_CloseFile(fileHandle3,
                   &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_CloseFile returns %d.", rc);
        rv++;
    }

    // Check available space after extending the file
    VSSI_GetSpace(handle, &diskSize, &datasetSize, &datasetFree,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_GetSpace returns %d.",
                         rc);
        rv++;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "VSSI_GetSpace after extending new file: "
                        "disk "FMTu64", dsSize "FMTu64", avail "FMTu64".",
                        diskSize, datasetSize, datasetFree);

    // Remove the new version of name1 (no longer open)
    VSSI_Remove(handle, name1,
                &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_Remove component %s: %d.",
                         name1, rc);
        rv++;
    }

    // Check available space after removing the file
    VSSI_GetSpace(handle, &diskSize, &datasetSize, &datasetFree,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_GetSpace returns %d.",
                         rc);
        rv++;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "VSSI_GetSpace after removing extended new file: "
                        "disk "FMTu64", dsSize "FMTu64", avail "FMTu64".",
                        diskSize, datasetSize, datasetFree);

    // Test VSSI_ReleaseFile releasing open modes

    // Open the file without OPEN_DELETE and SHARE_DELETE
    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_OPEN_ALWAYS;
    flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;

    VSSI_OpenFile(handle, name1, flags, attrs, &fileHandle1,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.", rc);
        rv++;
    }

    // Write to the file
    memset(myBuf, 0x43, sizeof(myBuf));
    wrLen = sizeof(myBuf);
    VSSI_WriteFile(fileHandle1, 0, &wrLen, myBuf,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_WriteFile returned %d.", rc);
        rv++;
    }
    if(wrLen != sizeof(myBuf)) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_WriteFile wrote %d bytes, expected %d.",
                         wrLen, sizeof(myBuf));
        rv++;
    }

    //
    // VSSI_ReleaseFile on FH1, but don't close it
    //
    VSSI_ReleaseFile(fileHandle1,
                   &unlock_context, vscs_test_callback);
    VPLSem_Wait(&(unlock_context.sem));
    rc = unlock_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                        "VSSI_ReleaseFile returns %d.", rc);
        rv++;
    }

    // Now try opening it again with a conflicting open mode
    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_DELETE;
    flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE|VSSI_FILE_SHARE_DELETE;

    VSSI_OpenFile(handle, name1, flags, attrs, &fileHandle2,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.", rc);
        rv++;
    }

    // Now delete the file
    VSSI_Remove(handle, name1,
                &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_Remove component %s: %d.",
                         name1, rc);
        rv++;
    }

    // Close FH1
    VSSI_CloseFile(fileHandle1,
                   &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_CloseFile returns %d.", rc);
        rv++;
    }

    // Check that we can still read through FH2
    memset(myBuf, 0x61, sizeof(myBuf));
    rdLen = sizeof(myBuf);
    VSSI_ReadFile(fileHandle2, 0, &rdLen, myBuf,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_ReadFile returned %d.", rc);
        rv++;
    }
    if(rdLen != sizeof(myBuf)) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_ReadFile returned %d bytes, exp %d.",
                         rdLen, sizeof(myBuf));
        rv++;
    }
    if(myBuf[0] != 0x43) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_ReadFile data wrong: get %02x, exp %02x",
                         myBuf[0], 0x43);
        rv++;
    }

    // Close FH2 and the file should go away
    VSSI_CloseFile(fileHandle2,
                   &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_CloseFile returns %d.", rc);
        rv++;
    }

    // ** UTF8 with case insensitive  testing **
    //
    // Create the directory for the test
    //
    attrs = 0;
    VSSI_MkDir2(handle, dirnameUTF8_a, attrs,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_MkDir2 %s returns %d.", dirname1, rc);
        rv++;
    }

    // Open the file with nameUTF8_AA
    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_OPEN_ALWAYS|VSSI_FILE_OPEN_DELETE;
    flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE|VSSI_FILE_SHARE_DELETE;

    VSSI_OpenFile(handle, nameUTF8_AA, flags, attrs, &fileHandle1,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.", rc);
        rv++;
    }

    // Write to the file
    memset(myBuf, 0x43, sizeof(myBuf));
    wrLen = sizeof(myBuf);
    VSSI_WriteFile(fileHandle1, 0, &wrLen, myBuf,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_WriteFile returned %d.", rc);
        rv++;
    }
    if(wrLen != sizeof(myBuf)) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_WriteFile wrote %d bytes, expected %d.",
                         wrLen, sizeof(myBuf));
        rv++;
    }

    // Close FH1
    VSSI_CloseFile(fileHandle1,
                   &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_CloseFile returns %d.", rc);
        rv++;
    }

    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_DELETE;
    flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE|VSSI_FILE_SHARE_DELETE;
    // Open it again with nameUTF8_aa, try to read cotnent back
    VSSI_OpenFile(handle, nameUTF8_aa, flags, attrs, &fileHandle1,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.", rc);
        rv++;
    }

    memset(myBuf, 0x61, sizeof(myBuf));
    rdLen = sizeof(myBuf);
    VSSI_ReadFile(fileHandle1, 0, &rdLen, myBuf,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_ReadFile returned %d.", rc);
        rv++;
    }
    if(rdLen != sizeof(myBuf)) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_ReadFile returned %d bytes, exp %d.",
                         rdLen, sizeof(myBuf));
        rv++;
    }
    if(myBuf[0] != 0x43) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_ReadFile data wrong: get %02x, exp %02x",
                         myBuf[0], 0x43);
        rv++;
    }

    // Rename nameUTF8_Aa to nameUTF8_ab
    VSSI_Rename(handle, nameUTF8_Aa, nameUTF8_ab,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Rename component %s to %s: %d expected %d.",
                         dirname1, dirname2, rc, VSSI_BUSY);
        rv++;
    }

    // Close FH1
    VSSI_CloseFile(fileHandle1,
                   &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_CloseFile returns %d.", rc);
        rv++;
    }

    // Remove the file nameUTF8_AB
    VSSI_Remove(handle, nameUTF8_AB,
                &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_Remove component %s: %d.",
                         name1, rc);
        rv++;
    }

    // Now delete the dir dirnameUTF8_A
    VSSI_Remove(handle, dirnameUTF8_A,
                &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_Remove component %s: %d.",
                         name1, rc);
        rv++;
    }
    // ** UTF8 testing end **

    // Close the VSS Object
    VSSI_CloseObject(handle,
                        &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Failed to close object %s: %d",
                         save_description, rc);
        rv++;
    }

 fail_open:
    VPLSem_Destroy(&(test_context.sem));
    VPLSem_Destroy(&(unlock_context.sem));
    VPLSem_Destroy(&(third_context.sem));
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                   "Test %s result: %d.",
                   vsTest_curTestName, rv);

    return rv;
}


static const char vsTest_vss_file_remove_shared[] = "VSS File Remove Shared Open Test";
static int test_vss_file_remove_shared(VSSI_Session session,
                                       const char* save_description,
                                       u64 user_id,
                                       u64 dataset_id,
                                       const VSSI_RouteInfo& route_info,
                                       bool use_xml_api)
{
    int rv = 0;
    int rc;
    VSSI_Object handle;
    vscs_test_context_t test_context;
    static const char* name1 = "/removeTestDir1/boohoo.txt";
    static const char* dirname1 = "/removeTestDir1";
    u32 flags;
    u32 attrs;
    VSSI_File fileHandle1;
    VSSI_File fileHandle2;
    VSSI_File fileHandle3;
    u32 rdLen;
    u32 wrLen;
    char myBuf[256];

    VPL_SET_UNINITIALIZED(&(test_context.sem));

    vsTest_curTestName = vsTest_vss_file_remove_shared;
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Starting test: %s.",
                        vsTest_curTestName);

    if(VPLSem_Init(&(test_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        return rv+1;
    }

    if(use_xml_api) {
        VSSI_OpenObject(vssi_session, save_description,
                        VSSI_READWRITE | VSSI_FORCE, &handle,
                        &test_context, vscs_test_callback);
    }
    else {
        VSSI_OpenObject2(vssi_session, user_id, dataset_id, &route_info,
                         VSSI_READWRITE | VSSI_FORCE, &handle,
                         &test_context, vscs_test_callback);
    }
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Open object %s failed: %d.",
                         save_description, rc);
        rv++;
        goto fail_open;
    }

    //
    // Create the directory for the test
    //
    attrs = 0;
    VSSI_MkDir2(handle, dirname1, attrs,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_MkDir2 %s returns %d.", dirname1, rc);
        rv++;
    }

    // OpenFile
    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_CREATE;
    flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;
    attrs = 0;

    VSSI_OpenFile(handle, name1, flags, attrs, &fileHandle1,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS && rc != VSSI_EXISTS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.", rc);
        rv++;
    }

    // Initialize the file
    memset(myBuf, 0x42, sizeof(myBuf));
    wrLen = sizeof(myBuf);
    VSSI_WriteFile(fileHandle1, 0, &wrLen, myBuf,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_WriteFile returned %d.", rc);
        rv++;
    }

    VSSI_CloseFile(fileHandle1,
                   &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_CloseFile returns %d.", rc);
        rv++;
    }

    // Open the file, but without OPEN_DELETE
    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_OPEN_ALWAYS;
    flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;
    VSSI_OpenFile(handle, name1, flags, attrs, &fileHandle1,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_EXISTS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.", rc);
        rv++;
    }

    // Remove should fail if file is open without OPEN_DELETE
    VSSI_Remove(handle, name1,
                &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_BUSY) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_Remove component %s: %d (expected %d).",
                         name1, rc, VSSI_BUSY);
        rv++;
    }

    VSSI_CloseFile(fileHandle1,
                   &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_CloseFile returns %d.", rc);
        rv++;
    }

    // Bug 7967 scenario #1:  two descriptors

    // Open the file, but with SHARE_DELETE 
    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_OPEN_ALWAYS;
    flags |= VSSI_FILE_SHARE_DELETE;
    flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;
    VSSI_OpenFile(handle, name1, flags, attrs, &fileHandle1,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_EXISTS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.", rc);
        rv++;
    }

    // Remove should still fail if file is open without OPEN_DELETE
    VSSI_Remove(handle, name1,
                &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_BUSY) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_Remove component %s: %d (expected %d).",
                         name1, rc, VSSI_BUSY);
        rv++;
    }

    // Open a second handle that adds OPEN_DELETE.  This should
    // succeed since FH1 has SHARE_DELETE.
    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE;
    flags |= VSSI_FILE_OPEN_DELETE;
    flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;
    VSSI_OpenFile(handle, name1, flags, attrs, &fileHandle2,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.", rc);
        rv++;
    }

    // The remove of the file should succeed this time because
    // OPEN_DELETE got added to the FH
    VSSI_Remove(handle, name1,
                &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_Remove component %s: %d.",
                         name1, rc);
        rv++;
    } else {
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "Scenario #1 OK!");
    }

    // Check that the file is actually gone from the namespace.
    // No CREATE or OPEN_ALWAYS to avoid creating a different file.
    flags = VSSI_FILE_OPEN_READ;
    flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;
    flags |= VSSI_FILE_SHARE_DELETE;
    VSSI_OpenFile(handle, name1, flags, attrs, &fileHandle3,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_NOTFOUND) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d, expected %d.",
                         rc, VSSI_NOTFOUND);
        rv++;
    }

    // Create a new file with the same name
    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_CREATE;
    flags |= VSSI_FILE_OPEN_DELETE|VSSI_FILE_SHARE_DELETE;
    flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;

    VSSI_OpenFile(handle, name1, flags, attrs, &fileHandle3,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.",
                         rc);
        rv++;
    }

    // Write through the new handle to the new file
    memset(myBuf, 0x5e, sizeof(myBuf));
    wrLen = sizeof(myBuf);
    VSSI_WriteFile(fileHandle3, 0, &wrLen, myBuf,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_WriteFile returned %d.", rc);
        rv++;
    }
    if(wrLen != sizeof(myBuf)) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_WriteFile returned %d bytes, exp %d.",
                         wrLen, sizeof(myBuf));
        rv++;
    }

    // Now close FH1 for the removed file
    VSSI_CloseFile(fileHandle1,
                   &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_CloseFile returns %d.", rc);
        rv++;
    }

    // Now close FH2 for the removed file
    VSSI_CloseFile(fileHandle2,
                   &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_CloseFile returns %d.", rc);
        rv++;
    }

    // Close and reopen the new file and make sure it has the correct contents
    VSSI_CloseFile(fileHandle3,
                   &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_CloseFile returns %d.", rc);
        rv++;
    }

    // Open with no create
    flags = VSSI_FILE_OPEN_READ;
    flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;

    VSSI_OpenFile(handle, name1, flags, attrs, &fileHandle3,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.",
                         rc);
        rv++;
    }

    memset(myBuf, 0x61, sizeof(myBuf));
    rdLen = sizeof(myBuf);
    VSSI_ReadFile(fileHandle3, 0, &rdLen, myBuf,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_ReadFile returned %d.", rc);
        rv++;
    }
    if(rdLen != sizeof(myBuf)) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_ReadFile returned %d bytes, exp %d.",
                         rdLen, sizeof(myBuf));
        rv++;
    }
    if(myBuf[0] != 0x5e) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_ReadFile data wrong: get %02x, exp %02x",
                         myBuf[0], 0x5e);
        rv++;
    }

    VSSI_CloseFile(fileHandle3,
                   &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_CloseFile returns %d.", rc);
        rv++;
    }

    // Bug 7967 scenario #2:  two descriptors with OPEN_DELETE & SHARE_DELETE

    // Open the file with OPEN_DELETE and SHARE_DELETE 
    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_OPEN_ALWAYS;
    flags |= VSSI_FILE_OPEN_DELETE|VSSI_FILE_SHARE_DELETE;
    flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;
    VSSI_OpenFile(handle, name1, flags, attrs, &fileHandle1,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_EXISTS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.", rc);
        rv++;
    }

    // Open a second handle the same
    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE;
    flags |= VSSI_FILE_OPEN_DELETE|VSSI_FILE_SHARE_DELETE;
    flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;
    VSSI_OpenFile(handle, name1, flags, attrs, &fileHandle2,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.", rc);
        rv++;
    }

    // Removing the file should succeed
    VSSI_Remove(handle, name1,
                &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_Remove component %s: %d.",
                         name1, rc);
        rv++;
    } else {
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "Scenario #2 OK!");
    }

    // Now close FH1 for the removed file
    VSSI_CloseFile(fileHandle1,
                   &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_CloseFile returns %d.", rc);
        rv++;
    }

    // Now close FH2 for the removed file
    VSSI_CloseFile(fileHandle2,
                   &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_CloseFile returns %d.", rc);
        rv++;
    }

    // Bug 7967 scenario #3:  three descriptors

    // Open the file with SHARE_DELETE 
    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_CREATE;
    flags |= VSSI_FILE_SHARE_DELETE;
    flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;
    VSSI_OpenFile(handle, name1, flags, attrs, &fileHandle1,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.", rc);
        rv++;
    }

    // Open a second handle that adds OPEN_DELETE
    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE;
    flags |= VSSI_FILE_OPEN_DELETE|VSSI_FILE_SHARE_DELETE;
    flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;
    VSSI_OpenFile(handle, name1, flags, attrs, &fileHandle2,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.", rc);
        rv++;
    }

    // Open a third handle with OPEN_DELETE but not SHARE_DELETE.
    // This should fail.
    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE;
    flags |= VSSI_FILE_OPEN_DELETE;
    flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;
    VSSI_OpenFile(handle, name1, flags, attrs, &fileHandle3,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_ACCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d, expected %d.",
                         rc, VSSI_ACCESS);
        rv++;
    }

    // Open a third handle with OPEN_DELETE and SHARE_DELETE
    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE;
    flags |= VSSI_FILE_OPEN_DELETE|VSSI_FILE_SHARE_DELETE;
    flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;
    VSSI_OpenFile(handle, name1, flags, attrs, &fileHandle3,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.", rc);
        rv++;
    }

    // Removing the file should succeed
    VSSI_Remove(handle, name1,
                &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_Remove component %s: %d.",
                         name1, rc);
        rv++;
    } else {
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "Scenario #3 OK!");
    }

    // Now close FH1 for the removed file
    VSSI_CloseFile(fileHandle1,
                   &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_CloseFile returns %d.", rc);
        rv++;
    }

    // Now close FH2 for the removed file
    VSSI_CloseFile(fileHandle2,
                   &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_CloseFile returns %d.", rc);
        rv++;
    }

    // Now close FH3 for the removed file
    VSSI_CloseFile(fileHandle3,
                   &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_CloseFile returns %d.", rc);
        rv++;
    }

    // Close the VSS Object
    VSSI_CloseObject(handle,
                        &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Failed to close object %s: %d",
                         save_description, rc);
        rv++;
    }

 fail_open:
    VPLSem_Destroy(&(test_context.sem));
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                   "Test %s result: %d.",
                   vsTest_curTestName, rv);

    return rv;
}


static const char vsTest_vss_file_rename_attributes[] = "VSS File Rename with Attributes Test";
static int test_vss_file_rename_attributes(VSSI_Session session,
                                       const char* save_description,
                                       u64 user_id,
                                       u64 dataset_id,
                                       const VSSI_RouteInfo& route_info,
                                       bool use_xml_api)
{
    int rv = 0;
    int rc;
    VSSI_Object handle;
    vscs_test_context_t test_context;
    static const char* name1 = "/attrTestDir1/bar.txt";
    static const char* name2 = "/attrTestDir1/foo.txt";
    static const char* dirname1 = "/attrTestDir1";
    u32 flags;
    u32 attrs;
    VSSI_File fileHandle1;
    u32 wrLen;
    char myBuf[256];

    VPL_SET_UNINITIALIZED(&(test_context.sem));

    vsTest_curTestName = vsTest_vss_file_rename_attributes;
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Starting test: %s.",
                        vsTest_curTestName);

    if(VPLSem_Init(&(test_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        return rv+1;
    }

    if(use_xml_api) {
        VSSI_OpenObject(vssi_session, save_description,
                        VSSI_READWRITE | VSSI_FORCE, &handle,
                        &test_context, vscs_test_callback);
    }
    else {
        VSSI_OpenObject2(vssi_session, user_id, dataset_id, &route_info,
                         VSSI_READWRITE | VSSI_FORCE, &handle,
                         &test_context, vscs_test_callback);
    }
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Open object %s failed: %d.",
                         save_description, rc);
        rv++;
        goto fail_open;
    }

    //
    // Create the directory for the test
    //
    attrs = 0;
    VSSI_MkDir2(handle, dirname1, attrs,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_MkDir2 %s returns %d.", dirname1, rc);
        rv++;
    }

    // OpenFile
    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_CREATE;
    flags |= VSSI_FILE_OPEN_DELETE;
    attrs = 0;

    VSSI_OpenFile(handle, name1, flags, attrs, &fileHandle1,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS && rc != VSSI_EXISTS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.", rc);
        rv++;
    }

    // Initialize the file
    memset(myBuf, 0x42, sizeof(myBuf));
    wrLen = sizeof(myBuf);
    VSSI_WriteFile(fileHandle1, 0, &wrLen, myBuf,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_WriteFile returned %d.", rc);
        rv++;
    }

    // Set attributes through the file handle
    attrs = VSSI_ATTR_HIDDEN | VSSI_ATTR_SYS;
    rc = vss_chmod_file(test_context, fileHandle1, attrs, attrs);
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Chmod(clear archive) - %d", rc);
        rv++;
    }

    // With file open with OPEN_DELETE, rename it.
    // This should succeed.
    VSSI_Rename2(handle, name1, name2, 0,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Rename2 component %s to %s: %d.",
                         name1, name2, rc);
        rv++;
    }

    // Verify the attributes on the renamed file before close
    // The create on the file sets the archive bit.
    attrs |= VSSI_ATTR_ARCHIVE;
    rc = vss_attrs_check(test_context, handle, name2, attrs);
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "vss_attrs_check() - %d", rc);
        rv++;
    }

    // Now close it and check again
    VSSI_CloseFile(fileHandle1,
                   &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_CloseFile returns %d.", rc);
        rv++;
    }

    // Verify the attributes on the renamed file after close
    rc = vss_attrs_check(test_context, handle, name2, attrs);
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "vss_attrs_check() - %d", rc);
        rv++;
    }

    // Close the VSS Object
    VSSI_CloseObject(handle,
                        &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Failed to close object %s: %d",
                         save_description, rc);
        rv++;
    }

 fail_open:
    VPLSem_Destroy(&(test_context.sem));
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                   "Test %s result: %d.",
                   vsTest_curTestName, rv);
    return rv;
}


static const char vsTest_vss_file_create[] = "VSS File Create Test";
static int test_vss_file_create(VSSI_Session session,
                                const char* save_description,
                                u64 user_id,
                                u64 dataset_id,
                                const VSSI_RouteInfo& route_info,
                                bool use_xml_api)
{
    int rv = 0;
    int rc;
    VSSI_Object handle;
    vscs_test_context_t test_context;
    static const char* name = "/createTestDir1/bar.txt";
    static const char* topname = "/boo.txt";
    static const char* badname = "/boo.txt/foo.txt";
    static const char* dirname = "/createTestDir1";
    u32 flags;
    u32 attrs;
    VSSI_File fileHandle1;
    u32 rdLen;
    u32 wrLen;
    char myBuf[256];

    VPL_SET_UNINITIALIZED(&(test_context.sem));

    vsTest_curTestName = vsTest_vss_file_create;
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Starting test: %s.",
                        vsTest_curTestName);

    if(VPLSem_Init(&(test_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        return rv+1;
    }

    if(use_xml_api) {
        VSSI_OpenObject(vssi_session, save_description,
                        VSSI_READWRITE | VSSI_FORCE, &handle,
                        &test_context, vscs_test_callback);
    }
    else {
        VSSI_OpenObject2(vssi_session, user_id, dataset_id, &route_info,
                         VSSI_READWRITE | VSSI_FORCE, &handle,
                         &test_context, vscs_test_callback);
    }
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Open object %s failed: %d.",
                         save_description, rc);
        rv++;
        goto fail_open;
    }

    // OpenFile without creating directory
    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_CREATE;
    flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;
    attrs = 0;

    VSSI_OpenFile(handle, name, flags, attrs, &fileHandle1,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_BADPATH) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d, expected %d.",
                         rc, VSSI_BADPATH);
        rv++;
    }

    //
    // Create the directory for the test
    //
    attrs = 0;
    VSSI_MkDir2(handle, dirname, attrs,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_MkDir2 %s returns %d.", dirname, rc);
        rv++;
    }

    VSSI_Commit(handle, &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                            "Commit on make directory %s: %d.",
                            dirname, rc);
        rv++;
    }

    // OpenFile
    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_CREATE;
    flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;
    attrs = 0;

    VSSI_OpenFile(handle, name, flags, attrs, &fileHandle1,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.", rc);
        rv++;
    }

    // Initialize the file
    memset(myBuf, 0x42, sizeof(myBuf));
    wrLen = sizeof(myBuf);
    VSSI_WriteFile(fileHandle1, 0, &wrLen, myBuf,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_WriteFile returned %d.", rc);
        rv++;
    }

    VSSI_CloseFile(fileHandle1,
                   &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_CloseFile returns %d.", rc);
        rv++;
    }

    // CREATE_NEW of an existing file should fail
    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_CREATE_NEW;
    VSSI_OpenFile(handle, name, flags, attrs, &fileHandle1,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_PERM) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d, expected %d.",
                         rc, VSSI_PERM);
        rv++;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, 
                         "VSSI_OpenFile CREATE_NEW existing returns %d, fileHandle1 %p",
                         rc, fileHandle1);

    // CREATE_ALWAYS should succeed, but should return VSSI_EXISTS
    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_CREATE_ALWAYS;
    VSSI_OpenFile(handle, name, flags, attrs, &fileHandle1,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_EXISTS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d, expected %d.",
                         rc, VSSI_EXISTS);
        rv++;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, 
                         "VSSI_OpenFile CREATE_ALWAYS existing returns %d, fileHandle1 %p",
                         rc, fileHandle1);

    // Test that the returned file handle is actually valid
    memset(myBuf, 0, sizeof(myBuf));
    rdLen = sizeof(myBuf);
    VSSI_ReadFile(fileHandle1, 0, &rdLen, myBuf,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_ReadFile returned %d.", rc);
        rv++;
    }
    // The file should have been truncated
    if(rdLen != 0) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_ReadFile returned %d bytes, expected %d.",
                         rdLen, 0);
        rv++;
    }

    memset(myBuf, 0x43, sizeof(myBuf));
    wrLen = sizeof(myBuf);
    VSSI_WriteFile(fileHandle1, 0, &wrLen, myBuf,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_WriteFile returned %d.", rc);
        rv++;
    }
    if(wrLen != sizeof(myBuf)) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_WriteFile wrote %d bytes, expected %d.",
                         wrLen, sizeof(myBuf));
        rv++;
    }

    // Close file handle
    VSSI_CloseFile(fileHandle1,
                   &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_CloseFile returns %d.", rc);
        rv++;
    }

    // OPEN_ALWAYS should succeed, but should return VSSI_EXISTS
    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_OPEN_ALWAYS;
    VSSI_OpenFile(handle, name, flags, attrs, &fileHandle1,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_EXISTS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d, expected %d.",
                         rc, VSSI_EXISTS);
        rv++;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, 
                         "VSSI_OpenFile CREATE_ALWAYS existing returns %d, fileHandle1 %p",
                         rc, fileHandle1);

    // Test that the returned file handle is actually valid
    memset(myBuf, 0, sizeof(myBuf));
    rdLen = sizeof(myBuf);
    VSSI_ReadFile(fileHandle1, 0, &rdLen, myBuf,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_ReadFile returned %d.", rc);
        rv++;
    }
    if(rdLen != sizeof(myBuf)) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_ReadFile returned %d bytes, expected %d.",
                         rdLen, sizeof(myBuf));
        rv++;
    }

    // Close file handle
    VSSI_CloseFile(fileHandle1,
                   &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_CloseFile returns %d.", rc);
        rv++;
    }

    // Create a file in the top level directory
    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_CREATE;
    flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;
    attrs = 0;

    VSSI_OpenFile(handle, topname, flags, attrs, &fileHandle1,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.", rc);
        rv++;
    }

    // Close file handle
    VSSI_CloseFile(fileHandle1,
                   &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_CloseFile returns %d.", rc);
        rv++;
    }

    // Create a file that needs a parent directory colliding with a file
    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_CREATE;
    flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;
    attrs = 0;

    VSSI_OpenFile(handle, badname, flags, attrs, &fileHandle1,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_BADPATH) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d, expected %d.",
                         rc, VSSI_BADPATH);
        rv++;
    }

    // Close the VSS Object
    VSSI_CloseObject(handle,
                        &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Failed to close object %s: %d",
                         save_description, rc);
        rv++;
    }

 fail_open:
    VPLSem_Destroy(&(test_context.sem));
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                   "Test %s result: %d.",
                   vsTest_curTestName, rv);

    return rv;
}

static VPLTime_t get_mtime(VSSI_Object handle, const char* name, string event)
{
    int rc = 0;
    VPLTime_t mtime;
    vscs_test_context_t context;
    VSSI_Dirent2 *dir_ent;

    VPL_SET_UNINITIALIZED(&(context.sem));
    if(VPLSem_Init(&(context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
    }

    VSSI_Stat2(handle, name, &dir_ent, &context, vscs_test_callback);
    VPLSem_Wait(&(context.sem));
    if ( rc != VSSI_SUCCESS ) {
        // If Stat2 fails, return 0 as mtime to trigger the test to fail.
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                         "VSSI_Stat2 on %s returns %d.",
                         name, rc);
        mtime = 0;
    } else {
        mtime = dir_ent->mtime;
    }

    VPLTRACE_LOG_INFO(TRACE_APP, 0, "mtime: "FMTu64" File: %s Event: %s", mtime, name, event.c_str());
    free(dir_ent);
    return mtime;
}

static const char vsTest_vss_file_modify_time[] = "VSS File Modify Time Test";
static int test_vss_file_modify_time(VSSI_Session session,
                                const char* save_description,
                                u64 user_id,
                                u64 dataset_id,
                                const VSSI_RouteInfo& route_info,
                                bool use_xml_api)
{
    int rv = 0;
    int rc;
    VSSI_Object handle;
    VSSI_File fileHandle;

    vscs_test_context_t test_context;
    
    static const char* name =     "/test_dir_0/test_dir_1/mtime.txt";
    static const char* new_name = "/test_dir_0/test_dir_2/mtime.txt";
    static const char* dirname0 = "/test_dir_0";
    static const char* dirname1 = "/test_dir_0/test_dir_1";
    static const char* dirname2 = "/test_dir_0/test_dir_2";
    static const char* dirname3 = "/test_dir_0/test_dir_3";

    string event;
    string data = "asdfghjkl";
    u32 length = data.size();
    char* buf = new char[length];

    u32 flags;
    u32 attrs;

    bool file_is_open = false;

    VPLTime_t mtime_before;
    VPLTime_t mtime_after;
    VPLTime_t mtime_before_2;
    VPLTime_t mtime_after_2;
    
    VPL_SET_UNINITIALIZED(&(test_context.sem));

    vsTest_curTestName = vsTest_vss_file_modify_time;
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Starting test: %s.",
                        vsTest_curTestName);

    if(VPLSem_Init(&(test_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        return rv+1;
    }

    if(use_xml_api) {
        VSSI_OpenObject(vssi_session, save_description,
                        VSSI_READWRITE | VSSI_FORCE, &handle,
                        &test_context, vscs_test_callback);
    }
    else {
        VSSI_OpenObject2(vssi_session, user_id, dataset_id, &route_info,
                         VSSI_READWRITE | VSSI_FORCE, &handle,
                         &test_context, vscs_test_callback);
    }
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenObject %s failed: %d.",
                         save_description, rc);
        rv++;
        goto done;
    }

    // Create the directories for the test
    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_CREATE;
    flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;
    attrs = 0;

    VSSI_MkDir2(handle, dirname0, attrs,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_MkDir2 %s returns %d.", dirname0, rc);
        rv++;
    }
    VSSI_Commit(handle, &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                            "VSSI_Commit on make directory %s: %d.",
                            dirname0, rc);
        rv++;
    }

    event = "Creating child directory";
    mtime_before = get_mtime(handle, dirname0, event);

    VSSI_MkDir2(handle, dirname1, attrs,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_MkDir2 %s returns %d.", dirname1, rc);
        rv++;
    }
    VSSI_Commit(handle, &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                            "VSSI_Commit on make directory %s: %d.",
                            dirname1, rc);
        rv++;
    }

    event = "Created child directory";
    mtime_after = get_mtime(handle, dirname0, event);

    // Test: Directory mtime should increase after creating new directory within it
    if (mtime_after <= mtime_before) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: Creating directory did not increase parent mtime");
        rv = 1;
        goto done;
    }    
    VPLTRACE_LOG_INFO(TRACE_APP, 0, "PASS: Creating directory did increase parent mtime");

    VSSI_MkDir2(handle, dirname2, attrs,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_MkDir2 %s returns %d.", dirname2, rc);
        rv++;
    }
    VSSI_Commit(handle, &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                            "VSSI_Commit on make directory %s: %d.",
                            dirname2, rc);
        rv++;
    }

    // Check directory mitme after creating a file
    event = "Creating file";
    mtime_before = get_mtime(handle, dirname1, event);

    VSSI_OpenFile(handle, name, flags, attrs, &fileHandle,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                         "VSSI_OpenFile returns %d, expected %d.",
                         rc, VSSI_SUCCESS);
        rv++;
    }
    file_is_open = true;
    VSSI_CloseFile(fileHandle, &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if ( rc != VSSI_SUCCESS ) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "Fail: Close file %s: %d.",
                         name, rc);
        rv++;
        goto done;
    }
    file_is_open = false;

    event = "Created file";
    mtime_after = get_mtime(handle, dirname1, event);
    // Test: Directory mtime should increase after creating a file within it.
    if (mtime_after <= mtime_before) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: Directoy mtime did not increase after a file was created.");
        rv = 1;
        goto done;
    }
    VPLTRACE_LOG_INFO(TRACE_APP, 0, "PASS: Directory mtime increases after creating a file.");

    // Check file and directory mtime after a write
    event = "Writing to file";
    mtime_before = get_mtime(handle, name, event);
    event = "Writing to file in directory";
    mtime_before_2 = get_mtime(handle, dirname1, event);

    VSSI_OpenFile(handle, name, VSSI_FILE_OPEN_WRITE, attrs, &fileHandle,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                         "VSSI_OpenFile returns %d, expected %d.",
                         rc, VSSI_SUCCESS);
        rv++;
        goto done;
    }
    file_is_open = true;
    VSSI_WriteFile(fileHandle, 0, &length, data.c_str(), &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if ( rc != VSSI_SUCCESS ) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: Write file %s: %d.",
                         name, rc);
        rv++;
        goto done;
    }
    VSSI_CloseFile(fileHandle, &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if ( rc != VSSI_SUCCESS ) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "Fail: Close file %s: %d.",
                         name, rc);
        rv++;
        goto done;
    }

    file_is_open = false;
    event = "Wrote to file";
    mtime_after = get_mtime(handle, name, event);
    event = "Wrote to file in directory";
    mtime_after_2 = get_mtime(handle, dirname1, event);
    // Test: File mtime should increase after a write, but not directory mtime
    if (mtime_after <= mtime_before) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: File mtime did not increase after a write.");
        rv = 1;
        goto done;
    }
    VPLTRACE_LOG_INFO(TRACE_APP, 0, "PASS: File mtime did increase after a write.");
    if (mtime_after_2 != mtime_before_2) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: Directory mtime did increase after a write.");
        rv = 1;
        goto done;
    }
    VPLTRACE_LOG_INFO(TRACE_APP, 0, "PASS: Directory mtime did not increase after a write.");

    // Check SetTimes set file mtime and stops close from setting it.
    VSSI_OpenFile(handle, name, VSSI_FILE_OPEN_WRITE, attrs, &fileHandle,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                         "VSSI_OpenFile returns %d, expected %d.",
                         rc, VSSI_SUCCESS);
        rv++;
        goto done;
    }
    file_is_open = true;
    VSSI_WriteFile(fileHandle, 0, &length, data.c_str(), &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if ( rc != VSSI_SUCCESS ) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: Write file %s: %d.",
                         name, rc);
        rv++;
        goto done;
    }

    VSSI_SetTimes(handle, name, 0, VPLTime_GetTime(), &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if ( rc != VSSI_SUCCESS ) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "Fail: SetTimes file %s: %d.",
                         name, rc);
        rv++;
        goto done;
    }
    event = "Wrote to file, then set mtime of file";
    mtime_before = get_mtime(handle, name, event);

    VSSI_CloseFile(fileHandle, &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if ( rc != VSSI_SUCCESS ) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "Fail: Close file %s: %d.",
                         name, rc);
        rv++;
        goto done;
    }

    file_is_open = false;
    event = "Closed file";
    mtime_after = get_mtime(handle, name, event);
    // Test: File mtime should not change if set_times then close with no intervening writes
    if (mtime_after != mtime_before) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: File mtime did increase after a close that followed set_times.");
        rv++;
        goto done;
    }
    VPLTRACE_LOG_INFO(TRACE_APP, 0, "PASS: File mtime did not increase after a close that followed set_times");

    // Check file mtime does not change after read
    event = "Reading from file";
    mtime_before = get_mtime(handle, name, event);

    VSSI_OpenFile(handle, name, VSSI_FILE_OPEN_READ, attrs, &fileHandle,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if (rc != VSSI_SUCCESS && rc != VSSI_EXISTS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: Open file %s: %d.",
                         name, rc);
        rv++;
        goto done;
    }
    file_is_open = true;
    VSSI_ReadFile(fileHandle, 0, &length, buf, &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if (rc != VSSI_SUCCESS && rc != VSSI_EXISTS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: Read file %s: %d.",
                         name, rc);
        rv++;
        goto done;
    }
    VSSI_CloseFile(fileHandle, &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if ( rc != VSSI_SUCCESS ) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "Fail: Close file %s: %d.",
                         name, rc);
        rv++;
        goto done;
    }
    file_is_open = false;

    event = "Read from file";
    mtime_after = get_mtime(handle, name, event);
    // Test: File mtime should not change after read to file
    if (mtime_after != mtime_before) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: File mtime did increase after a read.");
        rv = 1;
        goto done;
    }    
    VPLTRACE_LOG_INFO(TRACE_APP, 0, "PASS: File mtime did not increase after a read.");

    // Check file mtime does not change after opening for write
    event = "Open file for write";
    mtime_before = get_mtime(handle, name, event);

    VSSI_OpenFile(handle, name, VSSI_FILE_OPEN_WRITE, attrs, &fileHandle,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if (rc != VSSI_SUCCESS && rc != VSSI_EXISTS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: Open file %s: %d.",
                         name, rc);
        rv++;
        goto done;
    }
    file_is_open = true;
    VSSI_CloseFile(fileHandle, &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if ( rc != VSSI_SUCCESS ) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "Fail: Close file %s: %d.",
                         name, rc);
        rv++;
        goto done;
    }
    file_is_open = false;

    event = "Opened file for write";
    mtime_after = get_mtime(handle, name, event); 
    // Test: file mtime should not change after open for write, but with no actual write
    if (mtime_after != mtime_before) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: File mtime does increase after opening for write.");
        rv = 1;
        goto done;
    }    
    VPLTRACE_LOG_INFO(TRACE_APP, 0, "PASS: File mtime does not increase after opening for write.");

    // Check that writing without growing the file set mtime.
    event = "Write to file without growing it";
    mtime_before = get_mtime(handle, name, event);

    VSSI_OpenFile(handle, name, VSSI_FILE_OPEN_WRITE, attrs, &fileHandle,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if (rc != VSSI_SUCCESS && rc != VSSI_EXISTS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: Open file %s: %d.",
                         name, rc);
        rv++;
        goto done;
    }
    file_is_open = true;
    VSSI_WriteFile(fileHandle, 0, &length, data.c_str(), &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if ( rc != VSSI_SUCCESS ) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: Write file %s: %d.",
                         name, rc);
        rv++;
        goto done;
    }
    VSSI_CloseFile(fileHandle, &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if ( rc != VSSI_SUCCESS ) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "Fail: Close file %s: %d.",
                         name, rc);
        rv++;
        goto done;
    }
    file_is_open = false;

    event = "Wrote to file without growing it";
    mtime_after = get_mtime(handle, name, event);
    // Test: file mtime should change after write even without growing the file
    if (mtime_after <= mtime_before) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: File mtime does not increase after write");
        rv = 1;
        goto done;
    } 
    VPLTRACE_LOG_INFO(TRACE_APP, 0, "PASS: File mtime does increase after write.");  

    // Check file and dir mtime after a change in file's attributes
    event = "Changing file attributes";
    mtime_before = get_mtime(handle, name, event);
    event = "Changing file attributes in directory";
    mtime_before_2 = get_mtime(handle, dirname1, event);

    VSSI_OpenFile(handle, name, VSSI_FILE_OPEN_READ | VSSI_FILE_OPEN_WRITE, attrs, &fileHandle,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if (rc != VSSI_SUCCESS && rc != VSSI_EXISTS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: Open file %s: %d.",
                         name, rc);
        rv++;
        goto done;
    }
    VSSI_ChmodFile(fileHandle, flags, attrs, &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if ( rc != VSSI_SUCCESS ) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: Chmod file %s: %d.",
                         name, rc);
        rv++;
        goto done;
    } 
    VSSI_CloseFile(fileHandle, &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if ( rc != VSSI_SUCCESS ) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "Fail: Close file %s: %d.",
                         name, rc);
        rv++;
        goto done;
    }
    file_is_open = false;

    event = "Changed file attributes";
    mtime_after = get_mtime(handle, name, event);
    event = "Changed file attributes in directory";
    mtime_after_2 = get_mtime(handle, dirname1, event);
    // Test: File and directory mtime after chmod should not change
    if (mtime_after != mtime_before) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: File mtime does increase after chmod.");
        rv++;
        goto done;
    }
    VPLTRACE_LOG_INFO(TRACE_APP, 0, "PASS: File mtime does not increase after chmod.");  
    if (mtime_after_2 != mtime_before_2) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: Directory mtime does increase after chmod in file.");
        rv++;
        goto done;
    }
    VPLTRACE_LOG_INFO(TRACE_APP, 0, "PASS: Directory mtime does not increase after chmod in file.");

    // Check file mtime after truncate
    event = "Truncating file";
    mtime_before = get_mtime(handle, name, event);

    VSSI_OpenFile(handle, name, VSSI_FILE_OPEN_READ | VSSI_FILE_OPEN_WRITE, attrs, &fileHandle,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if (rc != VSSI_SUCCESS && rc != VSSI_EXISTS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: Open file %s: %d.",
                         name, rc);
        rv++;
        goto done;
    }
    file_is_open = true;
    VSSI_TruncateFile(fileHandle, 32, &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if ( rc != VSSI_SUCCESS ) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: Truncate file %s: %d.",
                         name, rc);
        rv++;
        goto done;
    }
    VSSI_CloseFile(fileHandle, &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if ( rc != VSSI_SUCCESS ) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "Fail: Close file %s: %d.",
                         name, rc);
        rv++;
        goto done;
    }
    file_is_open = false;

    event = "Truncated file";
    mtime_after = get_mtime(handle, name, event);
    // Test: File mtime should incread after truncate
    if (mtime_after <= mtime_before) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: File mtime does not increase after truncate");
        rv = 1;
        goto done;
    } 
    VPLTRACE_LOG_INFO(TRACE_APP, 0, "PASS: File mtime does increase after truncate.");

    // Check directory mtimes after moving a file between them
    event = "Moving file from source";
    mtime_before = get_mtime(handle, dirname1, event);
    event = "Moving file to destination";
    mtime_before_2 = get_mtime(handle, dirname2, event);

    VSSI_Rename2(handle, name, new_name, VSSI_RENAME_REPLACE_EXISTING, &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    if ( rc != VSSI_SUCCESS ) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                         "VSSI_Rename2 on %s returns %d.",
                         new_name, rc);
        rv++;
        goto done;
    }

    event = "Moved file from source";
    mtime_after = get_mtime(handle, dirname1, event);
    event = "Moved file to destination";
    mtime_after_2 = get_mtime(handle, dirname2, event);
    // Test: Moving file should change both source and destination directory mtimes
    if (mtime_after <= mtime_before) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: Source directory mtime does not increase after move.");
        rv = 1;
        goto done;
    }
    VPLTRACE_LOG_INFO(TRACE_APP, 0, "PASS: Source directory mtime does increase after move.");
    if (mtime_after_2 <= mtime_before_2) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: Destination directory mtime does not increase after move.");
        rv = 1;
        goto done;
    }
    VPLTRACE_LOG_INFO(TRACE_APP, 0, "PASS: Destination directory mtime does increase after move.");

    // Check parent mtime changes after moving directory
    event = "Renaming directory";
    mtime_before = get_mtime(handle, dirname1, event);
    event = "Renaming child directouy";
    mtime_before_2 = get_mtime(handle, dirname0, event);

    VSSI_Rename2(handle, dirname1, dirname3, VSSI_RENAME_REPLACE_EXISTING, &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    if ( rc != VSSI_SUCCESS ) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                         "VSSI_Rename2 on %s returns %d.",
                         dirname3, rc);
        rv++;
        goto done;
    }

    event = "Renamed directory";
    mtime_after = get_mtime(handle, dirname3, event);
    event = "Renamed child directory";
    mtime_after_2 = get_mtime(handle, dirname0, event);
    // Test: Only the parent mtime should increase after renamimg directory 
    if (mtime_after != mtime_before) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: Directory mtime does increase after being renamed.");
        rv = 1;
        goto done;
    }    
    VPLTRACE_LOG_INFO(TRACE_APP, 0, "PASS: Directory mtime does not increase after being renamed.");
    if (mtime_after_2 <= mtime_before_2) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: Directory mtime does not increase after child renamed.");
        rv = 1;
        goto done;
    }    
    VPLTRACE_LOG_INFO(TRACE_APP, 0, "PASS: Directory mtime does increase after child renamed.");

    // Check parent mtime chanes after removing directory
    event = "Removing child directory";
    mtime_before = get_mtime(handle, dirname0, event);

    VSSI_Remove(handle, dirname3, &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    if ( rc != VSSI_SUCCESS ) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                         "VSSI_Remove on %s returns %d.",
                         dirname3, rc);
        rv++;
        goto done;
    }

    event = "Removed child directory";
    mtime_after = get_mtime(handle, dirname0, event);
    // Test: Parent mtime should increase after removing directory
    if (mtime_after <= mtime_before) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: Removing child directory did not increase parent mtime.");
        rv = 1;
        goto done;
    }    
    VPLTRACE_LOG_INFO(TRACE_APP, 0, "PASS: Removing child directory did increase parent mtime.");

    // Check parent mtime changes after removing a file, but its parent does not
    event = "Removing file";
    mtime_before = get_mtime(handle, dirname2, event);
    event = "Removing child's file";
    mtime_before_2 = get_mtime(handle, dirname0, event);

    VSSI_Remove(handle, new_name, &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    if ( rc != VSSI_SUCCESS ) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                         "VSSI_Remove on %s returns %d.",
                         new_name, rc);
        rv++;
        goto done;
    }

    event = "Removed file";
    mtime_after = get_mtime(handle, dirname2, event);
    event = "Removed child's file";
    mtime_after_2 = get_mtime(handle, dirname0, event);
    // Test: Directory mtime should have increased, bu not its parent
    if (mtime_after <= mtime_before) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: Parent of removed file did not increase.");
        rv++;
        goto done;
    }
    VPLTRACE_LOG_INFO(TRACE_APP, 0, "PASS: Parent of removed file increased.");
    if (mtime_after_2 != mtime_before_2) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: Parent of parent of removed file mtime increased.");
        rv++;
        goto done;
    }
    VPLTRACE_LOG_INFO(TRACE_APP, 0, "PASS: Parent of parent of removed file mtime did not increase.");

done:
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Ending test: %s.",
                        vsTest_curTestName);

    if ( file_is_open ) {
        VSSI_CloseFile(fileHandle, &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));        
    }
    return rv;
}

static int vss_chmod(vscs_test_context_t& test_context, VSSI_Object handle, const char* name, u32 attrs, u32 attrs_mask)
{
    int rc = 0;

    VSSI_Chmod(handle, name, attrs, attrs_mask,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_Chmod %s returns %d.", name, rc);
        goto done;
    }

    VSSI_Commit(handle, &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                            "Commit after VSSI_Chmod %s: %d.",
                            name, rc);
        goto done;
    }

done:
    return rc;
}

static int vss_chmod_file(vscs_test_context_t& test_context, VSSI_File& fh,
                          u32 attrs, u32 attrs_mask)
{
    int rc = 0;

    VSSI_ChmodFile(fh, attrs, attrs_mask,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_ChmodFile() returns %d.", rc);
        goto done;
    }

done:
    return rc;
}


// vss_isFileOrDir() returns:
//      VSSI_SUCCESS - File or directory exists. isDir and/or size can be NULL.
//      VSSI_NOTFOUND - Doesn't exist, no error logged
//      Other error - If VSSI_Stat2 returns other than success or not found
//                    error is logged and returned.

static int vss_isFileOrDir(vscs_test_context_t& test_context, VSSI_Object handle,
                           const char* name, bool *isDir, u64 *size)
{
    int rc;
    VSSI_Dirent2* stats = NULL;

    VSSI_Stat2(handle, name, &stats,
                &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;

    if(rc == VSSI_SUCCESS) {
        if (isDir) {
            *isDir = stats->isDir;
        }
        if (size) {
            *size = stats->size;
        }
    }
    else if(rc != VSSI_NOTFOUND){
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                            "Stat2 call for %s failed: %d.",
                            name, rc);
    }

    if(stats != NULL) {
        free(stats);
    }

    return rc;
}


static int vss_attrs_check(vscs_test_context_t& test_context, VSSI_Object handle, const char* name, u32 attrs)
{
    int rc;
    VSSI_Dirent2* stats = NULL;

    VSSI_Stat2(handle, name, &stats,
                &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                            "Stat2 call for %s failed: %d.",
                            name, rc);
        goto done;
    }

    if(stats->attrs != attrs) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                            "Stats2 attrs for %s differ from attrs: 0x%08x vs.0x%08x.",
                            name, stats->attrs, attrs);
        rc = -1;
    }

done:
    if(stats != NULL) {
        free(stats);
    }

    return rc;
}


static int vss_remove(vscs_test_context_t& test_context, VSSI_Object handle, const char* name, bool failIfNotFound)
{
    int rc = 0;

    VSSI_Remove(handle, name,
                &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        if (rc == VSSI_NOTFOUND && failIfNotFound == false) {
            rc = VSSI_SUCCESS;
        } else {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "VSSI_Remove component %s: %d.",
                             name, rc);
        }
        goto done;
    }

done:
    return rc;
}


static int create_dir_tree(vscs_test_context_t &test_context, VSSI_Object handle, const char *dirname, int num_levels, int num_dirs, int num_files)
{
    int rc = 0;
    char filename[50];
    char subdirname[50];
    const char *slash = "/";
    const char *nc = "\0";
    const char *file_base = "file";
    const char *dir_base = "dir";
    int f = 0;
    int d = 0;
    u32 flags;
    u32 attrs;
    VSSI_File fileHandle;
    
    if (num_levels < 0) {
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "Invalid number of levels :%d", num_levels);
        goto exit;
    }
//    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "number of levels :%d", num_levels);

    if (num_dirs < 0) {
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "Invalid number of dirs :%d", num_dirs);
        goto exit;
    }

    if (num_files < 0) {
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "Invalid number of files :%d", num_files);
        goto exit;
    }

    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_CREATE;
    attrs = 0;

    if (num_levels == 0) {
        goto exit;
    } else {
        memcpy (filename, dirname, strlen(dirname));
//        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "only copy dirname - filename is :"
//             "%s", filename);

        for (f = 0; f < num_files; ++f) {
            sprintf (filename+strlen(dirname), "%s%s%d%s", slash, file_base, f, nc);
            //VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "file name is :%s", filename);

            // Create file using Open file
            VSSI_OpenFile(handle, filename, flags, attrs, &fileHandle,
                      &test_context, vscs_test_callback);
            VPLSem_Wait(&(test_context.sem));
            rc = test_context.rv;
            if(rc != VSSI_SUCCESS) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "VSSI_OpenFile returns %d.", rc);
                goto exit;
            }
//            VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "RESULT:"
//                         "VSSI_OpenFile returns %d.", rc);

            // Close file handle
            VSSI_CloseFile(fileHandle,
                   &test_context, vscs_test_callback);
            VPLSem_Wait(&(test_context.sem));
            rc = test_context.rv;
            if(rc != VSSI_SUCCESS) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_CloseFile returns %d.", rc);
                goto exit;
            }
//            VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "RESULT:"
//                         "VSSI_CloseFile returns %d.", rc);
        }

        memcpy (subdirname, dirname, strlen(dirname));
//        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "only copy dirname - subdirname is :"
//             "%s", subdirname);

        for (d = 0; d < num_dirs; ++d) {
            sprintf (subdirname+strlen(dirname), "%s%s%d%s", slash, dir_base, d, nc);
            //VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "subdir name is :%s", subdirname);

            //
            // Create the subdirectories
            //
            attrs = 0;
            VSSI_MkDir2(handle, subdirname, attrs,
                  &test_context, vscs_test_callback);
            VPLSem_Wait(&(test_context.sem));
            rc = test_context.rv;
            if(rc != VSSI_SUCCESS) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_MkDir2 %s returns %d.", dirname, rc);
                goto exit;
            }
//            VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "RESULT:"
//                         "VSSI_MkDir2 returns %d.", rc);
            rc = create_dir_tree(test_context, handle, subdirname, num_levels - 1, num_dirs, num_files);
            if(rc != VSSI_SUCCESS) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "create_dir_tree returns %d. at level %d", rc, num_levels);
                goto exit;
            }
        }
    }

exit:
    return rc;
}

static int traverse_dir_tree(vscs_test_context_t &test_context, VSSI_Object handle, char *dirname, int num_levels, int num_dirs, int num_files, bool isdelete)
{
    int rc = 0;
    VSSI_Dir2 dir1 = NULL;
    VSSI_Dirent2* dirent = NULL;
    int file_count = 0;
    int dir_count = 0;
    char subdirname[50];
    const char *slash = "/";
    const char *nc = "\0";
   
    if (num_levels < 0) {
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "Invalid number of levels :%d", num_levels);
        goto exit;
    }
//    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "number of levels :%d", num_levels);

    if (num_dirs < 0) {
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "Invalid number of dirs :%d", num_dirs);
        goto exit;
    }

    if (num_files < 0) {
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "Invalid number of files :%d", num_files);
        goto exit;
    }

    if (num_levels == 0) {
        goto exit;
    } else {
        // Open 1st level dir, find 2nd level dir
        VSSI_OpenDir2(handle, dirname, &dir1,
                  &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenDir2 returns %d.", rc);
            goto exit;
        }
//        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "VSSI_OpenDir2 returns :%d", rc);
        
        //while ((dir_count < num_dirs) && (file_count < num_files)) {
        while (1) {
            dirent = VSSI_ReadDir2(dir1);
            if (dirent == NULL) {
                if (dir_count != num_dirs) {
                    VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Found %d dirs. Expected = %d.", dir_count, num_dirs);
                    goto exit;
                }
                if (file_count != num_files) {
                    VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Found %d files. Expected = %d.", file_count, num_files);
                    goto exit;
                }
//                VPLTRACE_LOG_ERR(TRACE_APP, 0, "VSSI_ReadDir2 returns NULL.");
                break;
//                rc = -1;
//                goto exit;
            } else if (dirent->isDir == true) {
                dir_count++;
//                VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "Found dir :%s", dirent->name);
                memcpy (subdirname, dirname, strlen(dirname));
                sprintf (subdirname+strlen(dirname), "%s%s%s", slash, dirent->name, nc);
                rc = traverse_dir_tree(test_context, handle, subdirname, num_levels-1, num_dirs, num_files, isdelete);
                if(rc != 0) {
                    VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "traverse_dir returns %d.", rc);
                    goto exit;
                }
                // if isdelete is set then delete the dir
                if (isdelete) {
                    VSSI_Remove(handle, subdirname,
                        &test_context, vscs_test_callback);
                    VPLSem_Wait(&(test_context.sem));
                    rc = test_context.rv;
                    if(rc != VSSI_SUCCESS) {
                        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                            "VSSI_Remove component %s: %d.",
                            subdirname, rc);
                        goto exit;
                    }
                }
            } else {
                file_count++;
//                VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "Found file :%s", dirent->name);
                // if isdelete is set then delete the file
                if (isdelete) {
                    memcpy (subdirname, dirname, strlen(dirname));
                    sprintf (subdirname+strlen(dirname), "%s%s%s", slash, dirent->name, nc);
                    VSSI_Remove(handle, subdirname,
                        &test_context, vscs_test_callback);
                    VPLSem_Wait(&(test_context.sem));
                    rc = test_context.rv;
                    if(rc != VSSI_SUCCESS) {
                        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                            "VSSI_Remove component %s: %d.",
                            subdirname, rc);
                        goto exit;
                    }
                }
            }
        }
        if (dir1) {
            VSSI_CloseDir2(dir1);
            dir1 = NULL;
        }
    } 

exit:
    if (dir1) {
        VSSI_CloseDir2(dir1);
        dir1 = NULL;
    }
    return rc;
}

static int time_to_traverse(vscs_test_context_t& test_context, VSSI_Object handle, const char *dirname_base, const int num_top_level_trees, const int num_subtrees, int num_levels, int num_dirs, int num_files)
{
    int rc = 0;
    int m = 0;
    int n = 0;
    static char dirname[50];
    static const char* us = "_";
    static const char* nc = "\0";
    bool isDir = true;   
    VPLTime_t start;
    VPLTime_t end;
    VPLTime_t elapsed;
    
    VPLMath_InitRand();

    start = VPLTime_GetTimeStamp();
   
    for (m = 0; m < num_top_level_trees; ++m) {
        for (n = 0; n < num_subtrees; ++n) {

            memcpy (dirname, dirname_base, strlen(dirname_base));
            sprintf (dirname+strlen(dirname_base), "%d%s%d%s", m, us, n, nc);
//            VPLTRACE_LOG_ERR(TRACE_APP, 0, "directory tree (%d %d) %s", m, n, dirname);

            rc = vss_isFileOrDir(test_context, handle, dirname, &isDir, 0);
            if (rc == VSSI_SUCCESS) {
                rc = traverse_dir_tree(test_context, handle, dirname, num_levels, num_dirs, num_files, false);
                if(rc != 0) {
                    VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "Error traversing directory tree (%d %d)", m, n);
                    goto exit;
                }
            }
        }
    }

    rc = 0;
 
    end = VPLTime_GetTimeStamp();
           
    elapsed = VPLTime_DiffClamp(end, start);
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,"Time to traverse is "FMT_VPLTime_t"us.", elapsed);
        
exit:
    return rc;

}

static const char vsTest_vss_file_attributes[] = "VSS File Attributes API Test";
static int test_vss_file_attributes (VSSI_Session session,
                                     const char* save_description,
                                     u64 user_id,
                                     u64 dataset_id,
                                     bool use_chmod_file,
                                     const VSSI_RouteInfo& route_info,
                                     bool use_xml_api)
{
    int rv = 0;
    int rc;
    VSSI_Object handle;
    vscs_test_context_t test_context;
    u32 flags;
    u32 attrs;
    u32 attrs1;
    u32 attrs2;
    VSSI_File fileHandle1 = NULL;
    VSSI_Dir2 dir1 = NULL;
    VSSI_Dirent2* dirent = NULL;
    const char* name = "foo1.txt";
    const char* name2 = "foo2.txt";
    const char* name3 = "foo3.txt";
    const char* dirname1 = use_chmod_file ? "/myDirCF1" : "/myDir1";
    const char* dirname2 = use_chmod_file ? "/myDirCF1/myDir2" : "/myDir1/myDir2";
    const char  dirname2Name[] = "myDir2";
    bool found = false;
    u32 i;
    u32 wrLen;
    u32 rdLen;
    char myBuf[256];

    VPL_SET_UNINITIALIZED(&(test_context.sem));

    vsTest_curTestName = vsTest_vss_file_attributes;
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Starting test: %s.",
                        vsTest_curTestName);

    if(VPLSem_Init(&(test_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        return rv+1;
    }

    // Set initial conditions: delete object contents.
    if ( use_xml_api ) {
        VSSI_Delete(vssi_session, save_description,
                    &test_context, vscs_test_callback);
    }
    else {
        VSSI_Delete2(vssi_session, user_id, dataset_id, &route_info,
                     &test_context, vscs_test_callback);
    }
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Delete object %s failed: %d.",
                         save_description, rc);
        rv++;
        goto fail_open;
    }

    if(use_xml_api) {
        VSSI_OpenObject(vssi_session, save_description,
                        VSSI_READWRITE | VSSI_FORCE, &handle,
                        &test_context, vscs_test_callback);
    }
    else {
        VSSI_OpenObject2(vssi_session, user_id, dataset_id, &route_info,
                         VSSI_READWRITE | VSSI_FORCE, &handle,
                         &test_context, vscs_test_callback);
    }
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Open object %s failed: %d.",
                         save_description, rc);
        rv++;
        goto fail_open;
    }

    // create a file with hidden and system, check attrs including archive,
    // write a pattern, check attrs, read and check,
    // close and check attributes

    // Make sure it doesn't pre-exist
    rc = vss_remove(test_context, handle, name, false);

    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_CREATE;
    flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;
    attrs = VSSI_ATTR_HIDDEN | VSSI_ATTR_SYS;

    VSSI_OpenFile(handle, name, flags, attrs, &fileHandle1,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.", rc);
        rv++;
    }

    // Verify the Archive bit is set
    attrs |= VSSI_ATTR_ARCHIVE;
    rc = vss_attrs_check(test_context, handle, name, attrs);
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "vss_attrs_check() - %d", rc);
        rv++;
    }

    // clear the archive bit
    attrs &= ~VSSI_ATTR_ARCHIVE;
    if ( use_chmod_file ) {
        rc = vss_chmod_file(test_context, fileHandle1, attrs, VSSI_ATTR_ARCHIVE);
    }
    else {
        rc = vss_chmod(test_context, handle, name, attrs, VSSI_ATTR_ARCHIVE);
    }
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Chmod(clear archive) - %d", rc);
        rv++;
    }

    // verify it's gone
    rc = vss_attrs_check(test_context, handle, name, attrs);
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "vss_attrs_check() - %d", rc);
        rv++;
    }

    // change in the spec, we need to be able to set the ARCHIVE bit.
    // Set it then clear it as before.
    attrs |= VSSI_ATTR_ARCHIVE;
    if ( use_chmod_file ) {
        rc = vss_chmod_file(test_context, fileHandle1, attrs, VSSI_ATTR_ARCHIVE);
    }
    else {
        rc = vss_chmod(test_context, handle, name, attrs, VSSI_ATTR_ARCHIVE);
    }
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Chmod(set archive) - %d", rc);
        rv++;
    }

    // Verify the Archive bit is set
    rc = vss_attrs_check(test_context, handle, name, attrs);
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "vss_attrs_check() - %d", rc);
        rv++;
    }

    // now clear the archive bit, again for the write test.
    attrs &= ~VSSI_ATTR_ARCHIVE;
    if ( use_chmod_file ) {
        rc = vss_chmod_file(test_context, fileHandle1, attrs, VSSI_ATTR_ARCHIVE);
    }
    else {
        rc = vss_chmod(test_context, handle, name, attrs, VSSI_ATTR_ARCHIVE);
    }
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Chmod(clear archive) - %d", rc);
        rv++;
    }

    // verify it's gone
    rc = vss_attrs_check(test_context, handle, name, attrs);
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "vss_attrs_check() - %d", rc);
        rv++;
    }

    // write pattern, check attrs, close file, check attrs,

    for (i = 0; i < sizeof(myBuf); ++i) {
        myBuf[i] = i;
    }
    wrLen = sizeof(myBuf);

    VSSI_WriteFile(fileHandle1, 0, &wrLen, myBuf,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_WriteFile returned %d.", rc);
        rv++;
    }

    // Verify the Archive bit is set
    attrs |= VSSI_ATTR_ARCHIVE;
    rc = vss_attrs_check(test_context, handle, name, attrs);
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "vss_attrs_check() - %d", rc);
        rv++;
    }

    VSSI_CloseFile(fileHandle1,
                   &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_CloseFile returns %d.", rc);
        rv++;
    }

    // attrs should not have changed since last check, archive bit still set

    rc = vss_attrs_check(test_context, handle, name, attrs);
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "vss_attrs_check() - %d", rc);
        rv++;
    }

    // open file, check attrs, read file, check pattern, close file, check attrs
    // attrs don't matter on open because file already exists

    VSSI_OpenFile(handle, name, flags, 0 /*attrs*/, &fileHandle1,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS && rc != VSSI_EXISTS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.", rc);
        rv++;
    }

    // attrs should not have changed since last check, archive bit still set

    rc = vss_attrs_check(test_context, handle, name, attrs);
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "vss_attrs_check() - %d", rc);
        rv++;
    }

    // before read set myBuf to an unexpected pattern
    for (i = 0; i < sizeof(myBuf); ++i) {
        myBuf[i] = sizeof(myBuf) - 1 - i;
    }
    rdLen = sizeof(myBuf);
    VSSI_ReadFile(fileHandle1, 0, &rdLen, myBuf,
                            &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                            "VSSI_ReadFile returned %d.", rc);
        rv++;
    }
    else if(rdLen != sizeof(myBuf)) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                            "VSSI_ReadFile size for %s differ from expected: %u vs. %u.",
                            name, rdLen, sizeof(myBuf));
        rv++;
    }
    else for (i = 0; i < rdLen; ++i) {
        if ((u8)myBuf[i] != i) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                "VSSI_ReadFile %s myBuf[%u] expected 0x%02x got 0x%02x.",
                                name, i, i, (u8)myBuf[i]);
            rv++;
            break;
        }
    }

    VSSI_CloseFile(fileHandle1,
                   &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_CloseFile returns %d.", rc);
        rv++;
    }

    // attrs should not have changed since last check, archive bit still set

    rc = vss_attrs_check(test_context, handle, name, attrs);
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "vss_attrs_check() - %d", rc);
        rv++;
    }

    // change file attrs to read only

    attrs |= VSSI_ATTR_READONLY;
    rc = vss_chmod(test_context, handle, name, attrs, VSSI_ATTR_READONLY);
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                            "Commit after VSSI_Chmod %s: %d.",
                            name, rc);
        rv++;
    }

    // verify read only attribute set

    rc = vss_attrs_check(test_context, handle, name, attrs);
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "vss_attrs_check() - %d", rc);
        rv++;
    }

    // try to write after changed mode to read only

    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE;
    // flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;
    // open attrs doesn't matter because file exists
    VSSI_OpenFile(handle, name, flags, 0 /*attrs*/, &fileHandle1,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc == VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "2nd VSSI_OpenFile %s returns %d.", name, rc);
        rv++;
    }

    // We have to be able to perform chmods on read-only files.
    if ( use_chmod_file ) {
        flags = VSSI_FILE_OPEN_READ;
        VSSI_OpenFile(handle, name, flags, 0 /*attrs*/, &fileHandle1,
                      &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "3rd VSSI_OpenFile %s returns %d.", name, rc);
            rv++;
        }
    }

    // re-set hidden and system attr
    attrs &= ~(VSSI_ATTR_HIDDEN | VSSI_ATTR_SYS);

    if ( use_chmod_file ) {
        rc = vss_chmod_file(test_context, fileHandle1, attrs, 
            (VSSI_ATTR_HIDDEN | VSSI_ATTR_SYS));
    }
    else {
        rc = vss_chmod(test_context, handle, name, attrs,
            (VSSI_ATTR_HIDDEN | VSSI_ATTR_SYS));
    }

    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Chmod(clear archive) - %d", rc);
        rv++;
    }

    // verify that they are reset
    rc = vss_attrs_check(test_context, handle, name, attrs);
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "vss_attrs_check() - %d", rc);
        rv++;
    }

    // re-set read only attr
    attrs &= ~VSSI_ATTR_READONLY;
    if ( use_chmod_file ) {
        rc = vss_chmod_file(test_context, fileHandle1, attrs, VSSI_ATTR_READONLY);
    }
    else {
        rc = vss_chmod(test_context, handle, name, attrs, VSSI_ATTR_READONLY);
    }
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Chmod(clear archive) - %d", rc);
        rv++;
    }

    // verify not read only
    rc = vss_attrs_check(test_context, handle, name, attrs);
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "vss_attrs_check() - %d", rc);
        rv++;
    }

    if ( use_chmod_file ) {
        VSSI_CloseFile(fileHandle1,
                       &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "VSSI_CloseFile returns %d.", rc);
            rv++;
        }
    }

    // create a file as read only, rename it and delete it
    // specify archive bit shouldn't cause any problem

    // make sure it doesn't pre-exist
    rc = vss_remove(test_context, handle, name2, false);

    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_CREATE;
    attrs = VSSI_ATTR_READONLY | VSSI_ATTR_ARCHIVE;

    VSSI_OpenFile(handle, name2, flags, attrs, &fileHandle1,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.", rc);
        rv++;
    }

    // Verify the read only and achive
    rc = vss_attrs_check(test_context, handle, name2, attrs);
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "vss_attrs_check() - %d", rc);
        rv++;
    }

    // This is now a valid operation with latest semantics.
    // If the file successfully opened with write, it's allowed to
    // continue writing.
    memset(myBuf, 0x42, sizeof(myBuf));
    wrLen = sizeof(myBuf);
    VSSI_WriteFile(fileHandle1, 0, &wrLen, myBuf,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_WriteFile %s returned %d.", name2, rc);
        rv++;
    }

    VSSI_CloseFile(fileHandle1,
                   &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_CloseFile %s returns %d.", name2, rc);
        rv++;
    }

    // verify can be renamed and deleted

    VSSI_Rename(handle, name2, name3,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Rename component %s to %s: %d.", name2, name3, rc);
        rv++;
    }

    // check attrs for new name
    rc = vss_attrs_check(test_context, handle, name3, attrs);
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "vss_attrs_check() - %d", rc);
        rv++;
    }

    // now delete

    rc = vss_remove(test_context, handle, name3, true);
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "vss_remove() - %d", rc);
        rv++;
    }

    // try to open deleted file

    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE;
    flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;
    // open attrs doesn't matter because not create

    VSSI_OpenFile(handle, name3, flags, 0 /* attrs */, &fileHandle1,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc == VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile for deleted file %s was successful.", name3);
        rv++;
    }


    // Make first level directory
    attrs1 = VSSI_ATTR_HIDDEN | VSSI_ATTR_SYS | VSSI_ATTR_READONLY;
    VSSI_MkDir2(handle, dirname1, attrs1,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_MkDir2 %s returns %d.", dirname1, rc);
        rv++;
    }

    // verify the attr
    rc = vss_attrs_check(test_context, handle, dirname1, attrs1);
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "vss_attrs_check() - %d", rc);
        rv++;
    }

    // Make 2nd level directory
    attrs2 = VSSI_ATTR_HIDDEN | VSSI_ATTR_SYS | VSSI_ATTR_READONLY | VSSI_ATTR_ARCHIVE;
    VSSI_MkDir2(handle, dirname2, attrs2,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_MkDir2 %s returns %d.", dirname2, rc);
        rv++;
    }

    // verify the attr
    rc = vss_attrs_check(test_context, handle, dirname2, attrs2);
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "vss_attrs_check() - %d", rc);
        rv++;
    }

    // Open 1st level dir, find 2nd level dir, check attrs

    VSSI_OpenDir2(handle, dirname1, &dir1,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenDir2 returns %d.", rc);
        rv++;
    }

    found = false;
    while ( dir1 && (dirent = VSSI_ReadDir2(dir1)) != NULL ) {
        if (strncmp(dirent->name, dirname2Name, sizeof dirname2Name) == 0) {
            found = true;
            if(dirent->attrs != attrs2) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                    "VSSI_ReadDir2 dirent attrs for %s differ from VSSI_MkDir2: %u vs. %u.",
                                    dirname2, dirent->attrs, attrs2);
                rv++;
            }
        }
        else {
            VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                                "VSSI_ReadDir2 dirent name %s  attrs %u.",
                                dirent->name, dirent->attrs);
        }
    }

    if (dir1 && !found) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                            "%s not found using VSSI_ReadDir2.", dirname2);
        rv++;
    }

    if (dir1) {
        VSSI_CloseDir2(dir1);
        dir1 = NULL;
    }

 fail_open:
    VPLSem_Destroy(&(test_context.sem));
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                   "Test %s result: %d.",
                   vsTest_curTestName, rv);

    return rv;
}


static const char vsTest_vss_mkdir_parents_and_existing[] = "VSS mkdir parents and existing Test";
static int test_vss_mkdir_parents_and_existing (VSSI_Session session,
                                     const char* save_description,
                                     u64 user_id,
                                     u64 dataset_id,
                                     const VSSI_RouteInfo& route_info,
                                     bool use_xml_api)
{
    int rv = 0;
    int rc;
    VSSI_Object handle;
    vscs_test_context_t test_context;
    u64 sizeFromStat2;
    bool isDir;
    u32 flags;
    u32 file2attrs = VSSI_ATTR_ARCHIVE;
    u32 file3attrs = VSSI_ATTR_ARCHIVE;
    u32 rdLen;
    u32 wrLen;
    u32 i;
    static char file3buf[300];

    VSSI_File fileHandle = NULL;

    const char file3FullPath[] = "/myDir1/myDir2/myDir3/Foo3.tmp";

    struct dirInfo {
        const char *name;
        const char *fullPath;
        VSSI_Dir2   handle;
        u32         attrs;
    };

    dirInfo dir [] =  {
        { "myDir1", "/myDir1", NULL, VSSI_ATTR_ARCHIVE },
        { "myDir2", "/myDir1/myDir2", NULL, VSSI_ATTR_SYS },
        { "myDir3", "/myDir1/myDir2/myDir3", NULL, VSSI_ATTR_HIDDEN },
    };

    u32 &dir1attrs = dir[0].attrs;
    u32 &dir2attrs = dir[1].attrs;
    u32 &dir3attrs = dir[2].attrs;
    u32  dir4attrs =  VSSI_ATTR_SYS | VSSI_ATTR_HIDDEN;

    const char* &dir1FullPath = dir[0].fullPath;

    const char* &dir2FullPath = dir[1].fullPath;

    const char* &dir3FullPath = dir[2].fullPath;


    VPL_SET_UNINITIALIZED(&(test_context.sem));

    vsTest_curTestName = vsTest_vss_mkdir_parents_and_existing;
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Starting test: %s.",
                        vsTest_curTestName);

    if(VPLSem_Init(&(test_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        return rv+1;
    }

    // Set initial conditions: delete object contents.
    if ( use_xml_api ) {
        VSSI_Delete(vssi_session, save_description,
                    &test_context, vscs_test_callback);
    }
    else {
        VSSI_Delete2(vssi_session, user_id, dataset_id, &route_info,
                     &test_context, vscs_test_callback);
    }
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Delete object %s failed: %d.",
                         save_description, rc);
        rv++;
        goto fail_open;
    }

    if(use_xml_api) {
        VSSI_OpenObject(vssi_session, save_description,
                        VSSI_READWRITE | VSSI_FORCE, &handle,
                        &test_context, vscs_test_callback);
    }
    else {
        VSSI_OpenObject2(vssi_session, user_id, dataset_id, &route_info,
                         VSSI_READWRITE | VSSI_FORCE, &handle,
                         &test_context, vscs_test_callback);
    }
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Open object %s failed: %d.",
                         save_description, rc);
        rv++;
        goto fail_open;
    }

    // Behavior to verify:
    //   1. try mkdir with multiple directory when parents don't exist
    //       expect fail
    //   2. create directories /1/2/3 and put file in dir3
    //   3. try to create file with same name as intermediate and final directory
    //       expect fail, leaving directory and file unharmed
    //   4. try mkdir where directory exists
    //       expect fail, leaving directory and file unharmed
    //   5. try mkdir with same pathname as a file
    //       expect fail, leaving file unharmed
    //   6. verify file contents is correct
    ////////////////////////////////////////////////////////////////

    //  1.  try mkdir with multiple directory when parents don't exist

    // make sure dir1 doesn't exist and then try mkdir -p /dir1/dir2/dir3

    rc = vss_isFileOrDir(test_context, handle, dir1FullPath, &isDir, 0);
    if (rc == VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                            "Did not expect to find %s %s.",
                            (isDir ? "directory":"file"), dir1FullPath);
        rv++;
        goto exit;
    }
    if (rc != VSSI_NOTFOUND) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                            "vss_isFileOrDir(%s) got %d expected VSSI_NOTFOUND.",
                            dir1FullPath, rc);
        rv++;
        goto exit;
    }

    VSSI_MkDir2(handle, dir3FullPath, dir3attrs,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_BADPATH) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_MkDir2 for non-existing parent dirs %s returns %d, expected %d.", 
                         dir3FullPath, rc, VSSI_BADPATH);
        rv++;
        goto exit;
    }

    // Make sure the directories don't exist even if attempt to create failed

    rc = vss_isFileOrDir(test_context, handle, dir1FullPath, &isDir, 0);
    if (rc == VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                            "Did not expect to find %s %s.",
                            (isDir ? "directory":"file"), dir1FullPath);
        rv++;
        goto exit;
    }
    if (rc != VSSI_NOTFOUND) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                            "vss_isFileOrDir(%s) got %d expected VSSI_NOTFOUND.",
                            dir1FullPath, rc);
        rv++;
        goto exit;
    }

    // 2. create the directories

    for (i = 0; i < 3; ++i) {
        VSSI_MkDir2(handle, dir[i].fullPath,  dir[i].attrs,
                      &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "VSSI_MkDir2 %s returns %d.", dir[i].fullPath, rc);
            rv++;
            goto exit;
        }

        VSSI_Commit(handle, &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                "Commit on make directory %s: %d.",
                                 dir[i].fullPath, rc);
            rv++;
            goto exit;
        }

        rc = vss_isFileOrDir(test_context, handle, dir[i].fullPath, &isDir, 0);
        if (rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                "vss_isFileOrDir(%s) got %d expected VSSI_SUCCESS.",
                                dir[i].fullPath, rc);
            rv++;
            goto exit;
        }
        if (!isDir) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                "vss_isFileOrDir(%s) iDir %d, expected true.",
                                dir[i].fullPath, isDir);
            rv++;
            goto exit;
        }
    }

    // add a file to dir3

    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_CREATE;

    VSSI_OpenFile(handle, file3FullPath, flags, file3attrs, &fileHandle,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.", rc);
        rv++;
        goto exit;
    }

    // write to it

    wrLen = sizeof file3buf;
    for (i = 0; i < sizeof file3buf; ++i) {
        file3buf[i] = (137 + i) % sizeof file3buf;
    }

    VSSI_WriteFile(fileHandle, 0, &wrLen, file3buf,
                   &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
            "VSSI_WriteFile: %d.", rc);
        rv++;
        goto exit;
    }

    // close it

    VSSI_CloseFile(fileHandle,
                   &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    fileHandle = NULL;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_CloseFile %s returns %d.", file3FullPath, rc);
        rv++;
        goto exit;
    }


    // 3. try to create a file with the same name as an existing intermediate directory

    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_CREATE_ALWAYS;

    VSSI_OpenFile(handle, dir2FullPath, flags, file2attrs, &fileHandle,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_ISDIR) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile %s returns %d.", dir2FullPath, rc);
        rv++;
        goto exit;
    }

    // Make sure the directory still exists even if attempt to create file failed

    rc = vss_isFileOrDir(test_context, handle, dir2FullPath, &isDir, 0);
    if (rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                            "vss_isFileOrDir(%s) got %d expected VSSI_SUCCESS.",
                            dir2FullPath, rc);
        rv++;
        goto exit;
    }
    if (!isDir) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
            "vss_isFileOrDir(%s) iDir %d, expected true.",
                            dir2FullPath, isDir);
        rv++;
        goto exit;
    }

    // Make sure the file in dir3 is unharmed

    rc = vss_isFileOrDir(test_context, handle, file3FullPath, &isDir, &sizeFromStat2);
    if (rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                            "vss_isFileOrDir(%s) got %d expected VSSI_SUCCESS.",
                            dir2FullPath, rc);
        rv++;
        goto exit;
    }
    if (isDir) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
            "vss_isFileOrDir(%s) iDir %d, expected false.",
                            file3FullPath, isDir);
        rv++;
        goto exit;
    }
    if (sizeFromStat2 != sizeof file3buf) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
            "vss_isFileOrDir(%s) sizeFromStat2 "FMTu64", expected %u.",
                            file3FullPath, sizeFromStat2, sizeof file3buf);
        rv++;
        goto exit;
    }


    // try to create a file with the same name as an existing final directory

    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_CREATE_ALWAYS;

    VSSI_OpenFile(handle, dir3FullPath, flags, file3attrs, &fileHandle,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_ISDIR) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile %s returns %d.", dir3FullPath, rc);
        rv++;
        goto exit;
    }

    // Make sure the directory still exists even if attempt to create file failed

    rc = vss_isFileOrDir(test_context, handle, dir3FullPath, &isDir, 0);
    if (rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                            "vss_isFileOrDir(%s) got %d expected VSSI_SUCCESS.",
                            dir3FullPath, rc);
        rv++;
        goto exit;
    }
    if (!isDir) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
            "vss_isFileOrDir(%s) iDir %d, expected true.",
                            dir3FullPath, isDir);
        rv++;
        goto exit;
    }

    // Make sure the file in dir3 is unharmed

    rc = vss_isFileOrDir(test_context, handle, file3FullPath, &isDir, &sizeFromStat2);
    if (rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                            "vss_isFileOrDir(%s) got %d expected VSSI_SUCCESS.",
                            dir2FullPath, rc);
        rv++;
        goto exit;
    }
    if (isDir) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
            "vss_isFileOrDir(%s) iDir %d, expected false.",
                            file3FullPath, isDir);
        rv++;
        goto exit;
    }
    if (sizeFromStat2 != sizeof file3buf) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
            "vss_isFileOrDir(%s) sizeFromStat2 "FMTu64", expected %u.",
                            file3FullPath, sizeFromStat2, sizeof file3buf);
        rv++;
        goto exit;
    }

    // 4. try to create a directory overtop of an existing directory with different atttrs

    VSSI_MkDir2(handle, dir3FullPath, dir4attrs,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_ISDIR) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_MkDir2 %s returns %d for existing directory.", dir3FullPath, rc);
        rv++;
        goto exit;
    }

    // check that the attrs of the directories and file did not change
    rc = vss_attrs_check(test_context, handle, dir1FullPath, dir1attrs);
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "vss_attrs_check() %s - %d", dir1FullPath, rc);
    }
    rc = vss_attrs_check(test_context, handle, dir2FullPath, dir2attrs);
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "vss_attrs_check() %s - %d", dir2FullPath, rc);
    }
    rc = vss_attrs_check(test_context, handle, dir3FullPath, dir3attrs);
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "vss_attrs_check() %s - %d", dir3FullPath, rc);
    }

    rc = vss_attrs_check(test_context, handle, file3FullPath, file3attrs);
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "vss_attrs_check(%s) - %d", file3FullPath, rc);
        rv++;
    }

    // Make sure the file in dir3 is unharmed

    rc = vss_isFileOrDir(test_context, handle, file3FullPath, &isDir, &sizeFromStat2);
    if (rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                            "vss_isFileOrDir(%s) got %d expected VSSI_SUCCESS.",
                            dir2FullPath, rc);
        rv++;
        goto exit;
    }
    if (isDir) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
            "vss_isFileOrDir(%s) iDir %d, expected false.",
                            file3FullPath, isDir);
        rv++;
        goto exit;
    }
    if (sizeFromStat2 != sizeof file3buf) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
            "vss_isFileOrDir(%s) sizeFromStat2 "FMTu64", expected %u.",
                            file3FullPath, sizeFromStat2, sizeof file3buf);
        rv++;
        goto exit;
    }



    // 5. try to create a directory overtop of the file

    VSSI_MkDir2(handle, file3FullPath, dir4attrs,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_FEXISTS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_MkDir2 %s returns %d for existing file.", file3FullPath, rc);
        rv++;
        goto exit;
    }

    // check that the attrs of the directories and file did not change
    rc = vss_attrs_check(test_context, handle, dir1FullPath, dir1attrs);
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "vss_attrs_check() %s - %d", dir1FullPath, rc);
    }
    rc = vss_attrs_check(test_context, handle, dir2FullPath, dir2attrs);
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "vss_attrs_check() %s - %d", dir2FullPath, rc);
    }
    rc = vss_attrs_check(test_context, handle, dir3FullPath, dir3attrs);
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "vss_attrs_check() %s - %d", dir3FullPath, rc);
    }

    rc = vss_attrs_check(test_context, handle, file3FullPath, file3attrs);
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "vss_attrs_check(%s) - %d", file3FullPath, rc);
        rv++;
    }

    // 6. Read and check the file contents

    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE;
    flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;
    // open attrs doesn't matter because not create

    VSSI_OpenFile(handle, file3FullPath, flags, 0/*attrs*/, &fileHandle,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile %s returns %d.", file3FullPath, rc);
        rv++;
        goto exit;
    }

    memset (file3buf, sizeof file3buf, 0x77);
    rdLen = sizeof file3buf;
    VSSI_ReadFile(fileHandle, 0, &rdLen, file3buf,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                            "VSSI_ReadFile %d.", rc);
        rv++;
        goto exit;
    }
    else if(rdLen != sizeof(file3buf)) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
            "VSSI_ReadFile size expect %u got %u.", sizeof file3buf, rdLen);
        rv++;
        goto exit;
    }
    else for (i = 0; i < rdLen; ++i) {
        u8 expected = (137 + i) % sizeof file3buf;
        if ((u8)file3buf[i] != expected) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                "VSSI_ReadFile buf[%u] expect 0x%02x got 0x%02x.",
                i, expected, (u8)file3buf[i]);
            rv++;
            goto exit;
        }
    }



exit:
    if (fileHandle) {
        VSSI_CloseFile(fileHandle,
                       &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "VSSI_CloseFile %s returns %d.", file3FullPath, rc);
            rv++;
        }
    }

    // Close the VSS Object
    VSSI_CloseObject(handle,
                        &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Failed to close object %s: %d",
                         save_description, rc);
        rv++;
    }

fail_open:
    VPLSem_Destroy(&(test_context.sem));

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                   "Test %s result: %d.",
                   vsTest_curTestName, rv);

    return rv;
}






static const char vsTest_vss_file_case_insensitivity[] = "VSS File case insensitivity Test";
static int test_vss_file_case_insensitivity (VSSI_Session session,
                                     const char* save_description,
                                     u64 user_id,
                                     u64 dataset_id,
                                     const VSSI_RouteInfo& route_info,
                                     bool use_xml_api)
{
    int rv = 0;
    int rc;
    VSSI_Object handle;
    vscs_test_context_t test_context;
    u32 flags;
    u32 file1attrs;
    u32 dir1attrs;
    u32 dir2attrs;
    u32 dir3attrs;

    bool found = false;
    bool found2 = false;
    u32 i, k;
    u32 rdLen;
    u32 wrLen;
    static char file1buf[1024];

    VSSI_Dirent2* dirent = NULL;

    const char file1NameCases[][12] = {
        { "foo1Abc.txt" },
        { "Foo1aBc.Txt" },
        { "foo1abc.txt" },
        { "FOO1ABC.TXT" },
    };

    const char file1FullPathCases[][33] = {
        { "/myDir1dEf/myDir2dEf/foo1Abc.txt" },
        { "/MydiR1DeF/MydiR2DeF/Foo1aBc.Txt" },
        { "/mydir1def/mydir2def/foo1abc.txt" },
        { "/MYDIR1DEF/MYDIR2def/FOO1ABC.TXT" },
    };
    const u32 file1FullPathCasesNum = sizeof file1FullPathCases / sizeof file1FullPathCases[0];
    static VSSI_File file1Handles[file1FullPathCasesNum];


    const char file2FullPathCases[][33] = {
        { "/myDir1dEf/myDir2dEf/Foo2xYZ.txT" },
        { "/MydiR1DeF/MydiR2DeF/foO2xyZ.tXt" },
        { "/mydir1def/mydir2def/foo2xyz.txt" },
        { "/MYDIR1DEF/MYDIR2def/FOO2XYZ.TXT" },
    };
    const u32 file2FullPathCasesNum = sizeof file2FullPathCases / sizeof file2FullPathCases[0];
    static VSSI_File file2Handles[file2FullPathCasesNum];


    const char dir1FullPathCases[][11] = {
        { "/myDir1dEf" },
        { "/MydiR1DeF" },
        { "/mydir1def" },
        { "/MYDIR1DEF" },
    };
    const u32 dir1FullPathCasesNum = sizeof dir1FullPathCases / sizeof dir1FullPathCases[0];
    static VSSI_Dir2 dir1Handles[dir1FullPathCasesNum];

    const char dir2NameCases[][10] = {
        { "myDir2dEf" },
        { "MydiR2DeF" },
        { "mydir2def" },
        { "MYDIR2DEF" },
    };
    const u32 dir2NameCasesNum = sizeof dir2NameCases / sizeof dir2NameCases[0];

    const char dir2FullPathCases[][21] = {
        { "/myDir1dEf/myDir2dEf" },
        { "/MydiR1DeF/MydiR2DeF" },
        { "/mydir1def/mydir2def" },
        { "/MYDIR1def/MYDIR2DEF" },
    };
    const u32 dir2FullPathCasesNum = sizeof dir2FullPathCases / sizeof dir2FullPathCases[0];
    static VSSI_Dir2 dir2Handles[dir2FullPathCasesNum];

    const char dir3NameCases[][10] = {
        { "myDir3dEf" },
        { "MydiR3DeF" },
        { "mydir3def" },
        { "MYDIR3DEF" },
    };
    const u32 dir3NameCasesNum = sizeof dir3NameCases / sizeof dir3NameCases[0];

    const char dir3FullPathCases[][31] = {
        { "/myDir1dEf/myDir2dEf/myDir3dEf" },
        { "/MydiR1DeF/MydiR2DeF/MydiR3DeF" },
        { "/mydir1def/mydir2def/mydir3def" },
        { "/MYDIR1def/MYDIR2def/MYDIR3DEF" },
    };
    const u32 dir3FullPathCasesNum = sizeof dir3FullPathCases / sizeof dir3FullPathCases[0];
    static VSSI_Dir2 dir3Handles[dir3FullPathCasesNum];

    const char dir4FullPathCases[][31] = {
        { "/myDir1dEf/myDir2dEf/myDir4gHi" },
        { "/MydiR1DeF/MydiR2DeF/MydiR4GhI" },
        { "/mydir1def/mydir2def/mydir4ghi" },
        { "/MYDIR1def/MYDIR2def/MYDIR4GHI" },
    };
    const u32 dir4FullPathCasesNum = sizeof dir4FullPathCases / sizeof dir4FullPathCases[0];
    static VSSI_Dir2 dir4Handles[dir4FullPathCasesNum];


    VPL_SET_UNINITIALIZED(&(test_context.sem));

    vsTest_curTestName = vsTest_vss_file_case_insensitivity;
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Starting test: %s.",
                        vsTest_curTestName);

    if(VPLSem_Init(&(test_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        return rv+1;
    }

    // Set initial conditions: delete object contents.
    if ( use_xml_api ) {
        VSSI_Delete(vssi_session, save_description,
                    &test_context, vscs_test_callback);
    }
    else {
        VSSI_Delete2(vssi_session, user_id, dataset_id, &route_info,
                     &test_context, vscs_test_callback);
    }
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Delete object %s failed: %d.",
                         save_description, rc);
        rv++;
        goto fail_open;
    }

    if(use_xml_api) {
        VSSI_OpenObject(vssi_session, save_description,
                        VSSI_READWRITE | VSSI_FORCE, &handle,
                        &test_context, vscs_test_callback);
    }
    else {
        VSSI_OpenObject2(vssi_session, user_id, dataset_id, &route_info,
                         VSSI_READWRITE | VSSI_FORCE, &handle,
                         &test_context, vscs_test_callback);
    }
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Open object %s failed: %d.",
                         save_description, rc);
        rv++;
        goto fail_open;
    }

     // Behavior to verify:
     //     case insensitive operations on mixed case direcories and files
     // Create mixed case directories and file.
     // Verify that can do operations regardles of case, but case is preserved.

    // Make first level directory
    dir1attrs = 0;
    VSSI_MkDir2(handle, dir1FullPathCases[0], dir1attrs,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_MkDir2 %s returns %d.", dir1FullPathCases[0], rc);
        rv++;
        goto exit;
    }

    VSSI_Commit(handle, &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                            "Commit on make directory %s: %d.",
                            dir1FullPathCases[0], rc);
        rv++;
        goto exit;
    }

    // verify the attrs via different case versions of dir full path
    for (i = 0;  i < dir1FullPathCasesNum;  ++i) {
        rc = vss_attrs_check(test_context, handle, dir1FullPathCases[i], dir1attrs);
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "vss_attrs_check() - %d", rc);
            rv++;
        }
    }

    // Make 2nd level directory
    dir2attrs = VSSI_ATTR_HIDDEN | VSSI_ATTR_SYS | VSSI_ATTR_READONLY | VSSI_ATTR_ARCHIVE;
    VSSI_MkDir2(handle, dir2FullPathCases[0], dir2attrs,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_MkDir2 %s returns %d.", dir2FullPathCases[0], rc);
        rv++;
        goto exit;
    }

    VSSI_Commit(handle, &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                            "Commit on make directory %s: %d.",
                            dir2FullPathCases[0], rc);
        rv++;
        goto exit;
    }

    // verify the attrs via different case versions of dir full path
    for (i = 0;  i < dir2FullPathCasesNum;  ++i) {
        rc = vss_attrs_check(test_context, handle, dir2FullPathCases[i], dir2attrs);
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "vss_attrs_check() - %d", rc);
            rv++;
        }
    }


    // Make 3rd level directory
    dir3attrs = VSSI_ATTR_SYS;
    VSSI_MkDir2(handle, dir3FullPathCases[0], dir3attrs,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_MkDir2 %s returns %d.", dir3FullPathCases[0], rc);
        rv++;
        goto exit;
    }

    VSSI_Commit(handle, &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                            "Commit on make directory %s: %d.",
                            dir3FullPathCases[0], rc);
        rv++;
        goto exit;
    }

    // verify the attrs via different case versions of dir full path
    (void) dir3FullPathCasesNum;
    for (i = 0;  i < dir3FullPathCasesNum;  ++i) {
        rc = vss_attrs_check(test_context, handle, dir3FullPathCases[i], dir3attrs);
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "vss_attrs_check() - %d", rc);
            rv++;
        }
    }


    // create a file, close it, check attributes with different cases
    // open with various cases, rename it, and delete it  with differeent cases

    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_CREATE;
    file1attrs = VSSI_ATTR_ARCHIVE | VSSI_ATTR_SYS;

    VSSI_OpenFile(handle, file1FullPathCases[0], flags, file1attrs, &file1Handles[0],
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.", rc);
        rv++;
        goto exit;
    }

    // write to it

    wrLen = sizeof file1buf;
    for (i = 0; i < sizeof file1buf; ++i) {
        file1buf[i] = (13 + i) % sizeof file1buf;
    }

    VSSI_WriteFile(file1Handles[0], 0, &wrLen, file1buf,
                            &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
            "VSSI_WriteFile: %d.", rc);
        rv++;
    }

    // close it

    VSSI_CloseFile(file1Handles[0],
                   &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_CloseFile %s returns %d.", file1FullPathCases[0], rc);
        rv++;
    }
    file1Handles[0] = NULL;

    // verify the attrs via different case versions of dir full path
    for (i = 0;  i < file1FullPathCasesNum;  ++i) {
        rc = vss_attrs_check(test_context, handle, file1FullPathCases[i], file1attrs);
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "vss_attrs_check(%s) - %d", file1FullPathCases[i], rc);
            rv++;
        }
    }

    // verify can open with various filename cases

    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE;
    flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;
    // open attrs doesn't matter because not create

    for (i = 0;  i < file1FullPathCasesNum;  ++i) {
        VSSI_OpenFile(handle, file1FullPathCases[i], flags, 0/*attrs*/, &file1Handles[i],
                      &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "VSSI_OpenFile %s returns %d.", file1FullPathCases[i], rc);
            rv++;
            goto exit;
        }
    }

    // read it with handles opened via various filename contents and check contents

    for (i = 0;  i < file1FullPathCasesNum;  ++i) {
        memset (file1buf, sizeof file1buf, 0x137);
        rdLen = sizeof file1buf;
        VSSI_ReadFile(file1Handles[i], 0, &rdLen, file1buf,
                                &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                                "VSSI_ReadFile %d.", rc);
            rv++;
        }
        else if(rdLen != sizeof(file1buf)) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                "VSSI_ReadFile size expect %u got %u.", sizeof file1buf, rdLen);
            rv++;
        }
        else for (i = 0; i < rdLen; ++i) {
            u8 expected = (13 + i) % sizeof file1buf;
            if ((u8)file1buf[i] != expected) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                    "VSSI_ReadFile buf[%u] expect 0x%02x got 0x%02x.",
                    i, expected, (u8)file1buf[i]);
                rv++;
                break;
            }
        }
    }


    // close file handles that were opened with different filename cases
    for (k = 0;  k < file1FullPathCasesNum;  ++k) {
        i = file1FullPathCasesNum - k - 1;
        VSSI_CloseFile(file1Handles[i],
                       &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "VSSI_CloseFile %s returns %d.", file1FullPathCases[i], rc);
            rv++;
        }
        file1Handles[i] = NULL;
    }



    // Open 1st level dir via different case versions of dir1 full path
    // find 2nd level dir, check attrs via different case versions of dir2 full path.

    for (i = 0;  i < dir1FullPathCasesNum;  ++i) {
        VSSI_OpenDir2(handle, dir1FullPathCases[i], &dir1Handles[i],
                      &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "VSSI_OpenDir2 %s (%s) returns %d.",
                             dir1FullPathCases[i], dir1FullPathCases[0], rc);
            rv++;
            continue;
        }

        for (k = 0;  k < dir2NameCasesNum; ++k) {

            if (k != 0) {
                VSSI_RewindDir2(dir1Handles[i]);
            }

            found = false;
            while ( (dirent = VSSI_ReadDir2(dir1Handles[i])) != NULL ) {
                if (strncasecmp(dirent->name, dir2NameCases[k], sizeof dir2NameCases[k]) == 0) {
                    found = true;
                    if(dirent->attrs != dir2attrs) {
                        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                            "VSSI_ReadDir2 %s dirent attrs for %s differ from VSSI_MkDir2: %u vs. %u.",
                                            dir1FullPathCases[i], dir2NameCases[k], dirent->attrs, dir2attrs);
                        rv++;
                    }
                }
                else {
                    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                                        "VSSI_ReadDir2 %s dirent name %s  attrs %u.",
                                        dir1FullPathCases[i], dirent->name, dirent->attrs);
                }
            }

            if (!found) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                    "%s (%u chars %s) not found using VSSI_ReadDir2 for %s.",
                                    dir2NameCases[k], sizeof dir2NameCases[k], dir2NameCases[0], dir1FullPathCases[i]);
                rv++;
            }
        }
    }

    for (i = 0;  i < dir1FullPathCasesNum;  ++i) {
        if (dir1Handles[i]) {
            VSSI_CloseDir2(dir1Handles[i]);
            dir1Handles[i] = NULL;
        }
    }

     
    // Open 2nd level dir via different case versions of dir1 full path
    // Find 3rd level dir and filename in 2nd level directory using
    // file handles opened with various path cases.
    //      Check that original dir and file name case is preserved
    //      check attrs

    for (i = 0;  i < dir2FullPathCasesNum;  ++i) {
        VSSI_OpenDir2(handle, dir2FullPathCases[i], &dir2Handles[i],
                      &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "VSSI_OpenDir2 %s (%s) returns %d.",
                             dir2FullPathCases[i], dir2FullPathCases[0], rc);
            rv++;
            continue;
        }

        for (k = 0;  k < dir3NameCasesNum; ++k) {

            if (k != 0) {
                VSSI_RewindDir2(dir2Handles[i]);
            }

            VSSI_OpenDir2(handle, dir2FullPathCases[i], &dir2Handles[i],
                          &test_context, vscs_test_callback);
            VPLSem_Wait(&(test_context.sem));
            rc = test_context.rv;
            if(rc != VSSI_SUCCESS) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "VSSI_OpenDir2 %s (%s) returns %d.",
                                 dir2FullPathCases[i], dir2FullPathCases[0], rc);
                rv++;
                continue;
            }

            found = found2 = false;
            while ( (dirent = VSSI_ReadDir2(dir2Handles[i])) != NULL ) {
                if (strncmp(dirent->name, dir3NameCases[0], sizeof dir3NameCases[0]) == 0) {
                    found = true;
                    if(dirent->attrs != dir3attrs) {
                        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                            "VSSI_ReadDir2 %s dirent attrs for %s differ from VSSI_MkDir2: %u vs. %u.",
                                            dir2FullPathCases[i], dir3NameCases[k], dirent->attrs, dir3attrs);
                        rv++;
                    }
                }
                else if (strncmp(dirent->name, file1NameCases[0], sizeof file1NameCases[0]) == 0) {
                    found2 = true;
                    if(dirent->attrs != file1attrs) {
                        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                            "VSSI_ReadDir2 %s dirent attrs for %s differ from VSSI_MkDir2: %u vs. %u.",
                                            dir2FullPathCases[i], file1NameCases[k], dirent->attrs, file1attrs);
                        rv++;
                    }
                }
                else {
                    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                                        "VSSI_ReadDir2 %s dirent name %s  attrs %u.",
                                        dir2FullPathCases[i], dirent->name, dirent->attrs);
                }
            }

            if (!found) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                    "%s not found using VSSI_ReadDir2 for %s.", dir3NameCases[k], dir2FullPathCases[i]);
                rv++;
            }

            if (!found2) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                    "%s not found using VSSI_ReadDir2 for %s.", file1NameCases[0], dir2FullPathCases[i]);
                rv++;
            }
        }
    }

    for (i = 0;  i < dir2FullPathCasesNum;  ++i) {
        if (dir2Handles[i]) {
            VSSI_CloseDir2(dir2Handles[i]);
            dir2Handles[i] = NULL;
        }
    }

    // verify files and directories can be renamed and deleted with differnt filename case

    VSSI_Rename(handle, file1FullPathCases[1], file2FullPathCases[0],
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Rename component %s to %s: %d.", file1FullPathCases[1], file2FullPathCases[0], rc);
        rv++;
    }
    else {
        VSSI_Commit(handle, &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Commit on rename %s to %s: %d.",
                             file1FullPathCases[1], file2FullPathCases[0], rc);
            rv++;
        }
    }

    // check attrs for new name and different cases
    for (i = 0;  i < file2FullPathCasesNum;  ++i) {
        rc = vss_attrs_check(test_context, handle, file2FullPathCases[i], file1attrs);
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "vss_attrs_check() %s - %d", file2FullPathCases[i], rc);
            rv++;
        }
    }

    // now delete
    rc = vss_remove(test_context, handle, file2FullPathCases[1], true);
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "vss_remove() - %d", rc);
        rv++;
    }

    // try to open deleted file

    flags = VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE|VSSI_FILE_SHARE_DELETE;
    // open attrs doesn't matter because not create

    for (i = 0;  i < file2FullPathCasesNum; ++i) {
        VSSI_OpenFile(handle, file2FullPathCases[i], flags, 0 /* attrs */, &file2Handles[i],
                      &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_NOTFOUND) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "VSSI_OpenFile for deleted file %s %d.", file2FullPathCases[i], rc);
            rv++;
        }
    }

    for (i = 0;  i < file1FullPathCasesNum; ++i) {
        VSSI_OpenFile(handle, file1FullPathCases[i], flags, 0 /* attrs */, &file1Handles[i],
                      &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_NOTFOUND) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "VSSI_OpenFile for deleted file %s %d.", file1FullPathCases[i], rc);
            rv++;
        }
    }


    // rename and delete a directory

    VSSI_Rename(handle, dir3FullPathCases[1], dir4FullPathCases[0],
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Rename component %s to %s: %d.", dir3FullPathCases[1], dir4FullPathCases[0], rc);
        rv++;
    }
    else {
        VSSI_Commit(handle, &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Commit on rename %s to %s: %d.",
                             dir3FullPathCases[1], dir4FullPathCases[0], rc);
            rv++;
        }
    }

    // check attrs for new name and different cases
    for (i = 0;  i < dir4FullPathCasesNum;  ++i) {
        rc = vss_attrs_check(test_context, handle, dir4FullPathCases[i], dir3attrs);
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "vss_attrs_check() %s - %d", dir4FullPathCases[i], rc);
            rv++;
        }
    }

    // now delete
    rc = vss_remove(test_context, handle, dir4FullPathCases[1], true);
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "vss_remove() - %d", rc);
        rv++;
    }

    // try to open deleted dir

    for (i = 0;  i < dir4FullPathCasesNum; ++i) {
        VSSI_OpenDir2(handle, dir4FullPathCases[i], &dir4Handles[i],
                      &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_NOTFOUND) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "VSSI_OpenDir2 for deleted directory %s %d.", dir4FullPathCases[i], rc);
            rv++;
        }
    }

    for (i = 0;  i < dir3FullPathCasesNum; ++i) {
        VSSI_OpenDir2(handle, dir3FullPathCases[i], &dir3Handles[i],
                      &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_NOTFOUND) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "VSSI_OpenFile for deleted directory %s %d.", dir3FullPathCases[i], rc);
            rv++;
        }
    }


exit:
fail_open:
    VPLSem_Destroy(&(test_context.sem));

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                   "Test %s result: %d.",
                   vsTest_curTestName, rv);

    return rv;
}






struct  AccessAndSharingModeTestCase {
    u32 flags;
    u32 open_read;
    u32 open_write;
    u32 open_delete;
    u32 share_read;
    u32 share_write;
    u32 share_delete;
    int expectOpen;
    int expectWrite;
    int expectRead;
    int expectDelete;
};


static const char vsTest_vss_file_access_and_share_modes[] = "VSS File access and share modes Test";
static int test_vss_file_access_and_share_modes (VSSI_Session session,
                                     const char* save_description,
                                     u64 user_id,
                                     u64 dataset_id,
                                     const VSSI_RouteInfo& route_info,
                                     bool use_xml_api)
{
    int rv = 0;
    int rc;
    int lastrv;
    int expectCreate;
    VSSI_Object handle;
    vscs_test_context_t test_context;
    u32 flags;
    u32 attrs;
    VSSI_File fileHandle1 = NULL;
    VSSI_File fileHandle2 = NULL;
    static const char* name1 = "testFile.tst";
    u32 i;
    u32 h1Num;
    u32 h2Num;
    u32 subStepNum;
    u32 numTested = 0;
    u32 numSkipped = 0;
    u32 numPassed = 0;
    u32 wrLen;
    u32 rdLen;
    const u32 bufSize = 256;
    char myBuf[bufSize];
    char current[bufSize];

    const u32 numFlagCombinations = 64;
    u32 flagCombinations[numFlagCombinations];


    VPL_SET_UNINITIALIZED(&(test_context.sem));

    vsTest_curTestName = vsTest_vss_file_access_and_share_modes;
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Starting test: %s.",
                        vsTest_curTestName);

    if(VPLSem_Init(&(test_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        return rv+1;
    }

    // Set initial conditions: delete object contents.
    if ( use_xml_api ) {
        VSSI_Delete(vssi_session, save_description,
                    &test_context, vscs_test_callback);
    }
    else {
        VSSI_Delete2(vssi_session, user_id, dataset_id, &route_info,
                     &test_context, vscs_test_callback);
    }
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Delete object %s failed: %d.",
                         save_description, rc);
        rv++;
        goto fail_open;
    }

    if(use_xml_api) {
        VSSI_OpenObject(vssi_session, save_description,
                        VSSI_READWRITE | VSSI_FORCE, &handle,
                        &test_context, vscs_test_callback);
    }
    else {
        VSSI_OpenObject2(vssi_session, user_id, dataset_id, &route_info,
                         VSSI_READWRITE | VSSI_FORCE, &handle,
                         &test_context, vscs_test_callback);
    }
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Open object %s failed: %d.",
                         save_description, rc);
        rv++;
        goto fail_open;
    }

    // initialize list of 64 access mode and sharing flag combinations

    {
        const u32 r_bit  = 1 << 0;
        const u32 w_bit  = 1 << 1;
        const u32 d_bit  = 1 << 2;
        const u32 sr_bit = 1 << 3;
        const u32 sw_bit = 1 << 4;
        const u32 sd_bit = 1 << 5;

        for (i = 0;  i < numFlagCombinations;  ++i) {
            flagCombinations[i] = 0;
            if (i & r_bit)  flagCombinations[i] |= VSSI_FILE_OPEN_READ;
            if (i & w_bit)  flagCombinations[i] |= VSSI_FILE_OPEN_WRITE;
            if (i & d_bit)  flagCombinations[i] |= VSSI_FILE_OPEN_DELETE;
            if (i & sr_bit) flagCombinations[i] |= VSSI_FILE_SHARE_READ;
            if (i & sw_bit) flagCombinations[i] |= VSSI_FILE_SHARE_WRITE;
            if (i & sd_bit) flagCombinations[i] |= VSSI_FILE_SHARE_DELETE;
        }
    }


    // try all combinations of access mode and sharing flags for 2 fileHandles to the same file

    for (h1Num = 0;  h1Num < numFlagCombinations;   ++h1Num) {
        AccessAndSharingModeTestCase h1;

        h1.flags = flagCombinations[h1Num];

        // Initialize individual h1 flag variables such that when or'ed
        // together, get a compact representation of access and share flags.

        h1.open_read    = (h1.flags & VSSI_FILE_OPEN_READ)    ? (1 << 0) : 0;
        h1.open_write   = (h1.flags & VSSI_FILE_OPEN_WRITE)   ? (1 << 1) : 0;
        h1.open_delete  = (h1.flags & VSSI_FILE_OPEN_DELETE)  ? (1 << 2) : 0;
        h1.share_read   = (h1.flags & VSSI_FILE_SHARE_READ)   ? (1 << 4) : 0;
        h1.share_write  = (h1.flags & VSSI_FILE_SHARE_WRITE)  ? (1 << 5) : 0;
        h1.share_delete = (h1.flags & VSSI_FILE_SHARE_DELETE) ? (1 << 6) : 0;

        // Compute expected outcomes.
        //
        // Expect all cases of open for h1 to be successful.
        // if h1 fails to open, don't even try h2

        h1.expectOpen = VSSI_SUCCESS;

        for (h2Num = 0, lastrv= rv;  
                h2Num < numFlagCombinations;
                    ++h2Num, lastrv = rv) {

            AccessAndSharingModeTestCase h2;

            u32 h1_ssaa;  // h1 compact flags  0xSSAA - S: share, A: access requested    
            u32 h2_ssaa;  // h2 compact flags  0xSSAA - S: share, A: access requested
            u32 testCase; // (h1_ssaa << 16) | h2_ssaa;

            h2.flags = flagCombinations[h2Num];

            // Initialize individual h2 flag variables such that when or'ed
            // together, get a compact representation of access and share flags.

            h2.open_read    = (h2.flags & VSSI_FILE_OPEN_READ)    ? (1 << 0) : 0;
            h2.open_write   = (h2.flags & VSSI_FILE_OPEN_WRITE)   ? (1 << 1) : 0;
            h2.open_delete  = (h2.flags & VSSI_FILE_OPEN_DELETE)  ? (1 << 2) : 0;
            h2.share_read   = (h2.flags & VSSI_FILE_SHARE_READ)   ? (1 << 4) : 0;
            h2.share_write  = (h2.flags & VSSI_FILE_SHARE_WRITE)  ? (1 << 5) : 0;
            h2.share_delete = (h2.flags & VSSI_FILE_SHARE_DELETE) ? (1 << 6) : 0;

            // The "testCase" variable is compact flags representation for h1 and h2.
            // It will be used in log messages to document the case being tried
            // <testCase>.0 means a problem when setting up testCase
            // <testCase>.1 means when opening or accessing h1
            // <testCase>.2 means when opening or accessing h2
            // Left 8 bits (MSBs) is h1 flags, Right 8 bits (LSBs) is h2 flags
            // Flags per handle are  0 sd sw sr 0 d r w
            // example h1 read and sharewrite, h2 write and share read: 0x02112

            h1_ssaa = h1.open_read|h1.open_write|h1.open_delete|
                      h1.share_read|h1.share_write|h1.share_delete;
            h2_ssaa = h2.open_read|h2.open_write|h2.open_delete|
                      h2.share_read|h2.share_write|h2.share_delete;

            testCase = (h1_ssaa << 8) | h2_ssaa;

            // set true to test only specific cases, false to test all cases
            if (false) {
                if (testCase != 0x2112 && testCase != 0x0001) {
                    continue;
                }
            }

            // Should the expected error be VSSI_PERM or VSSI_ACCESS ?

            // Need to check both if h2 access is allowed by h1
            // and h1 access is allowed by h2 or h2 should fail open

            h2.expectRead = VSSI_PERM;
            if (h2.open_read && h1.share_read) {
                h2.expectRead = VSSI_SUCCESS;
            }
            h2.expectWrite = VSSI_PERM;
            if (h2.open_write && h1.share_write) {
                h2.expectWrite = VSSI_SUCCESS;
            }
            h2.expectDelete = VSSI_PERM;
            if (h2.open_delete && h1.share_delete) {
                h2.expectDelete = VSSI_SUCCESS;
            }

            h1.expectRead = VSSI_PERM;
            if (h1.open_read && h2.share_read) {
                h1.expectRead = VSSI_SUCCESS;
            }
            h1.expectWrite = VSSI_PERM;
            if (h1.open_write && h2.share_write) {
                h1.expectWrite = VSSI_SUCCESS;
            }
            h1.expectDelete = VSSI_PERM;
            if (h1.open_delete && h2.share_delete) {
                h1.expectDelete = VSSI_SUCCESS;
            }

            if ( (h2.open_read   &&  h2.expectRead   != VSSI_SUCCESS) ||
                 (h2.open_write  &&  h2.expectWrite  != VSSI_SUCCESS) ||
                 (h2.open_delete &&  h2.expectDelete != VSSI_SUCCESS) ||
                 (h1.open_read   &&  h1.expectRead   != VSSI_SUCCESS) ||
                 (h1.open_write  &&  h1.expectWrite  != VSSI_SUCCESS) ||
                 (h1.open_delete &&  h1.expectDelete != VSSI_SUCCESS )) {
                 h2.expectOpen = VSSI_ACCESS;
            } else {
                 h2.expectOpen = VSSI_SUCCESS;
            }

            // if h2 can not be successfully opened,
            // h1 read/write expectation only depends on h1 access request
            if (h2.expectOpen != VSSI_SUCCESS) {
                h1.expectRead = VSSI_PERM;
                if (h1.open_read) {
                    h1.expectRead = VSSI_SUCCESS;
                }
                h1.expectWrite = VSSI_PERM;
                if (h1.open_write) {
                    h1.expectWrite = VSSI_SUCCESS;
                }
                h1.expectDelete = VSSI_PERM;
                if (h1.open_delete) {
                    h1.expectDelete = VSSI_SUCCESS;
                }
            }

            if (false) { // true to skip test cases not expected to open successfully
                if (h1.expectOpen != VSSI_SUCCESS || h2.expectOpen != VSSI_SUCCESS) {
                    ++numSkipped;
                    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Skipping: Case %04x  h1(%d,%d,%d,%d) h2(%d,%d,%d,%d) tsp(%u,%u,%u).",
                        testCase,
                        h1.expectOpen, h1.expectRead, h1.expectWrite, h1.expectDelete,
                        h2.expectOpen, h2.expectRead, h2.expectWrite, h2.expectDelete,
                        numTested, numSkipped, numPassed);
                    continue;
                }
            }

            ++numTested;
            if (false) { // true to log case being tested before it is tested
                VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                    "Testing: Case %04x  h1(%d,%d,%d,%d) h2(%d,%d,%d,%d) tsp(%u,%u,%u).",
                    testCase,
                    h1.expectOpen, h1.expectRead, h1.expectWrite, h1.expectDelete,
                    h2.expectOpen, h2.expectRead, h2.expectWrite, h2.expectDelete,
                    numTested, numSkipped, numPassed);
            }


            // remove file at start of case (unless disabled)
            // do 3 substeps for each test case
            //     0. remove then create file and write pattern to it
            //     1. open with h1.flags, try write/read,
            //     2. open 2nd filehande with h2.flags, try write/read,
            // close both filehandles
            //
            // If can't open fileHandle1, don't bother with fileHandle2

            if (fileHandle1) {
                    VSSI_CloseFile(fileHandle1,
                                   &test_context, vscs_test_callback);
                    VPLSem_Wait(&(test_context.sem));
                    rc = test_context.rv;
                    if(rc != VSSI_SUCCESS) {
                        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                            "Case %04x.0 VSSI_CloseFile 1 returns %d.", testCase, rc);
                        rv++;
                    }
                    fileHandle1 = NULL;
            }

            if (fileHandle2) {
                    VSSI_CloseFile(fileHandle2,
                                   &test_context, vscs_test_callback);
                    VPLSem_Wait(&(test_context.sem));
                    rc = test_context.rv;
                    if(rc != VSSI_SUCCESS) {
                        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                            "Case %04x.0 VSSI_CloseFile 2 returns %d.", testCase, rc);
                        rv++;
                    }
                    fileHandle2 = NULL;
            }

            if (true) { // true to remove the file each time and check create
                rc = vss_remove (test_context, handle, name1, false);
                if(rc != VSSI_SUCCESS) {
                    VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                        "Case %04x.0 vss_remove() - %d", testCase, rc);
                    rv++;
                    goto exit;
                }
                expectCreate = VSSI_SUCCESS;
            }
            else {
                if (h1Num + h2Num == 0) {
                    expectCreate = VSSI_SUCCESS;
                }
                else {
                    expectCreate = VSSI_EXISTS;
                }
            }

            // Open the file with a set of flags that will work regardless of
            // the h1 and h2 settings for this pass, so it is guaranteed to exist.
            flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_CREATE;
            flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;
            attrs = 0;
            VSSI_OpenFile(handle, name1, flags, attrs, &fileHandle1,
                          &test_context, vscs_test_callback);
            VPLSem_Wait(&(test_context.sem));
            rc = test_context.rv;
            if(rc != expectCreate) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                    "Case %04x.0 VSSI_OpenFile expect %d got %d when create file.",
                    testCase, expectCreate, rc);
                rv++;
            }

            // Set to true to test write and read back on every loop
            if (true) {

            // write pattern, read back and check, close file

            for (i = 0; i < sizeof current; ++i) {
                current[i] = i;
            }
            wrLen = sizeof current;

            VSSI_WriteFile(fileHandle1, 0, &wrLen, current,
                                  &test_context, vscs_test_callback);
            VPLSem_Wait(&(test_context.sem));
            rc = test_context.rv;
            if(rc != VSSI_SUCCESS) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                    "Case %04x.0 VSSI_WriteFile returned %d.", testCase, rc);
                rv++;
                continue;
            }

            // before read set buf to an unexpected pattern
            memset (myBuf, sizeof myBuf, 0x42);
            rdLen = sizeof(myBuf);
            VSSI_ReadFile(fileHandle1, 0, &rdLen, myBuf,
                                    &test_context, vscs_test_callback);
            VPLSem_Wait(&(test_context.sem));
            rc = test_context.rv;
            if(rc != VSSI_SUCCESS) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                    "Case %04x.0 VSSI_ReadFile returned %d.", testCase, rc);
                rv++;
                continue;
            }
            else if(rdLen != sizeof(myBuf)) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                    "Case %04x.0 VSSI_ReadFile size expect %u got %u.",
                    testCase, sizeof myBuf, rdLen);
                rv++;
                continue;
            }
            else {
                bool misMatch = false;
                for (i = 0; i < rdLen; ++i) {
                    if ((u8)myBuf[i] != (u8)current[i]) {
                        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                            "Case %04x.0 VSSI_ReadFile buf[%u] expected 0x%02x got 0x%02x.",
                            testCase, i, (u8)current[i], (u8)myBuf[i]);
                        rv++;
                        misMatch = true;
                        break;
                    }
                }
                if (misMatch) {
                    continue;
                }
            }
            }

            if (fileHandle1) {
                VSSI_CloseFile(fileHandle1,
                            &test_context, vscs_test_callback);
                VPLSem_Wait(&(test_context.sem));
                rc = test_context.rv;
                if(rc != VSSI_SUCCESS) {
                    VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                        "Case %04x.0 VSSI_CloseFile returns %d.", testCase, rc);
                    rv++;
                }
            }
            fileHandle1 = NULL;

            //
            // Actual test with the selected combination of modes
            //
            VSSI_OpenFile(handle, name1, h1.flags, attrs, &fileHandle1,
                          &test_context, vscs_test_callback);
            VPLSem_Wait(&(test_context.sem));
            rc = test_context.rv;
            if(rc != h1.expectOpen) {
                VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                    "Testing: Case %04x  %08x %08x  h1(%d,%d,%d,%d) h2(%d,%d,%d,%d) tsp(%u,%u,%u).",
                    testCase, h1.flags, h2.flags,
                    h1.expectOpen, h1.expectRead, h1.expectWrite, h1.expectDelete,
                    h2.expectOpen, h2.expectRead, h2.expectWrite, h2.expectDelete,
                    numTested, numSkipped, numPassed);

                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                    "Case %04x.1 VSSI_OpenFile expect %d got %d for existing file.",
                    testCase, h1.expectOpen, rc);
                rv++;
                continue; // don't even try 2nd open if first fails
            }

            fileHandle2 = NULL;
            VSSI_OpenFile(handle, name1, h2.flags, attrs, &fileHandle2,
                          &test_context, vscs_test_callback);
            VPLSem_Wait(&(test_context.sem));
            rc = test_context.rv;
            if(rc != h2.expectOpen) {
                VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                    "Testing: Case %04x  %08x %08x  h1(%d,%d,%d,%d) h2(%d,%d,%d,%d) tsp(%u,%u,%u).",
                    testCase, h1.flags, h2.flags,
                    h1.expectOpen, h1.expectRead, h1.expectWrite, h1.expectDelete,
                    h2.expectOpen, h2.expectRead, h2.expectWrite, h2.expectDelete,
                    numTested, numSkipped, numPassed);

                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                    "Case %04x.2 VSSI_OpenFile expect %d got %d for existing file.",
                    testCase, h2.expectOpen, rc);
                rv++;
            }

            if (true) // true to test expected result for read and write.
            for (subStepNum = 1; subStepNum < 3; ++subStepNum) {

                VSSI_File fileHandle = subStepNum == 1 ? fileHandle1 : fileHandle2;
                int expectWrite = subStepNum == 1 ? h1.expectWrite : h2.expectWrite;
                int expectRead  = subStepNum == 1 ? h1.expectRead : h2.expectRead;

                if (fileHandle == NULL) {
                    continue;
                }

                for (i = 0; i < sizeof myBuf; ++i) {
                    myBuf[i] = (subStepNum + i) % sizeof myBuf;
                }
                wrLen = sizeof myBuf;

                VSSI_WriteFile(fileHandle, 0, &wrLen, myBuf,
                                        &test_context, vscs_test_callback);
                VPLSem_Wait(&(test_context.sem));
                rc = test_context.rv;
                if(rc != expectWrite) {
                    VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                        "Case %04x.%u VSSI_WriteFile expect %d got %d.",
                        testCase, subStepNum, expectWrite, rc);
                    rv++;
                }

                if (rc == VSSI_SUCCESS) {
                    for (i = 0; i < sizeof current; ++i) {
                        current[i] = myBuf[i];
                    }
                }

                memset (myBuf, sizeof myBuf, 0x42);
                rdLen = sizeof myBuf;
                VSSI_ReadFile(fileHandle, 0, &rdLen, myBuf,
                                        &test_context, vscs_test_callback);
                VPLSem_Wait(&(test_context.sem));
                rc = test_context.rv;
                if(rc != expectRead) {
                    VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                                        "Case %04x.%u VSSI_ReadFile expect %d got %d.",
                                        testCase, subStepNum, expectRead, rc);
                    rv++;
                }
                else if (rc != VSSI_SUCCESS) {
                    continue;
                }
                else if(rdLen != sizeof(myBuf)) {
                    VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                        "Case %04x.%u VSSI_ReadFile size expect %u got %u.",
                        testCase, subStepNum, sizeof myBuf, rdLen);
                    rv++;
                    continue;
                }
                else for (i = 0; i < rdLen; ++i) {
                    if ((u8)myBuf[i] != (u8)current[i]) {
                        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                            "Case %04x.%u VSSI_ReadFile buf[%u] expect 0x%02x got 0x%02x.",
                            testCase, subStepNum, i, (u8)current[i], (u8)myBuf[i]);
                        rv++;
                        break;
                    }
                }
            }

            if (fileHandle1) {
                VSSI_CloseFile(fileHandle1,
                                &test_context, vscs_test_callback);
                VPLSem_Wait(&(test_context.sem));
                rc = test_context.rv;
                if(rc != VSSI_SUCCESS) {
                    VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                        "Case %04x.1 VSSI_CloseFile returns %d.", testCase, rc);
                    rv++;
                }
                fileHandle1 = NULL;
            }

            if (fileHandle2) {
                VSSI_CloseFile(fileHandle2,
                                &test_context, vscs_test_callback);
                VPLSem_Wait(&(test_context.sem));
                rc = test_context.rv;
                if(rc != VSSI_SUCCESS) {
                    VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                        "Case %04x.2 VSSI_CloseFile returns %d.", testCase, rc);
                    rv++;
                }
                fileHandle2 = NULL;
            }

            if (rv == lastrv) {
                ++numPassed;
                if (false) // true to log all cases that pass
                VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                    "Passed: Case %04x  h1(%d,%d,%d,%d) h2(%d,%d,%d,%d) tsp(%u,%u,%u).",
                    testCase,
                    h1.expectOpen, h1.expectRead, h1.expectWrite, h1.expectDelete,
                    h2.expectOpen, h2.expectRead, h2.expectWrite, h2.expectDelete,
                    numTested, numSkipped, numPassed);
            }
        }
    }

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
        "numTested %u numSkipped %u numPassed %u.", numTested, numSkipped, numPassed);

exit:

fail_open:
    VPLSem_Destroy(&(test_context.sem));
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                   "Test %s result: %d.",
                   vsTest_curTestName, rv);

    return rv;
}


#define TEST_NUM_FILES  1000
#define TEST_NUM_FILES_MIN 900

static const char vsTest_vss_file_open_many[] = "VSS File Open Many Test";
static int test_vss_file_open_many(VSSI_Session session,
                                const char* save_description,
                                u64 user_id,
                                u64 dataset_id,
                                const VSSI_RouteInfo& route_info,
                                bool use_xml_api)
{
    int rv = 0;
    int rc;
    VSSI_Object handle;
    vscs_test_context_t test_context;
    static const char* name = "/openManyTestDir1/file";
    static const char* dirname = "/openManyTestDir1";
    u32 flags;
    u32 attrs;
    static VSSI_File fileHandleMany[TEST_NUM_FILES];
    u32 rdLen;
    u32 wrLen;
    static char myBuf[256];
    static char readBuf[256];
    static char filename[50];
    static const char* fileext = ".txt";
    u32 i = 0;
    u32 max_opened = 0;

    VPL_SET_UNINITIALIZED(&(test_context.sem));

    vsTest_curTestName = vsTest_vss_file_open_many;
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Starting test: %s.",
                        vsTest_curTestName);

    if(VPLSem_Init(&(test_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        return rv+1;
    }

    if(use_xml_api) {
        VSSI_OpenObject(vssi_session, save_description,
                        VSSI_READWRITE | VSSI_FORCE, &handle,
                        &test_context, vscs_test_callback);
    }
    else {
        VSSI_OpenObject2(vssi_session, user_id, dataset_id, &route_info,
                         VSSI_READWRITE | VSSI_FORCE, &handle,
                         &test_context, vscs_test_callback);
    }
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Open object %s failed: %d.",
                         save_description, rc);
        rv++;
        goto fail_open;
    }

    //
    // Create the directory for the test
    //
    attrs = 0;
    VSSI_MkDir2(handle, dirname, attrs,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_MkDir2 %s returns %d.", dirname, rc);
        rv++;
        goto exit;
    }

    VSSI_Commit(handle, &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                            "Commit on make directory %s: %d.",
                            dirname, rc);
        rv++;
        goto exit;
    }

    // OpenFile
    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_CREATE;
    flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;
    attrs = 0;

    for (i = 0; i < TEST_NUM_FILES; ++i) {
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "i = %d", i);

        memcpy (filename, name, strlen(name));
        sprintf (filename+strlen(name), "%d%s", i, fileext);

        VSSI_OpenFile(handle, filename, flags, attrs, &fileHandleMany[i],
                      &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            if (i < TEST_NUM_FILES_MIN) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "VSSI_OpenFile returns %d.", rc);
                rv++;
                goto exit;
            }
            if (rc != VSSI_HLIMIT) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "VSSI_OpenFile returns %d.", rc);
                rv++;
                goto exit;
            } else {
                VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "VSSI_OpenFile returns %d for %dth file", rc, i + 1);

                max_opened = i;
                break;
            }
        }
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "VSSI_OpenFile returns %d for file %s.", rc, filename);

        // Initialize the file
        memset(myBuf, i, sizeof(myBuf));
        wrLen = sizeof(myBuf);
        VSSI_WriteFile(fileHandleMany[i], 0, &wrLen, myBuf,
                          &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_WriteFile returned %d.", rc);
            rv++;
            goto exit;
        }
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "VSSI_WriteFile returns %d.", rc);

        // Test that the returned file handle is actually valid
        memset(readBuf, 0, sizeof(myBuf));
        rdLen = sizeof(readBuf);
        VSSI_ReadFile(fileHandleMany[i], 0, &rdLen, readBuf,
                          &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_ReadFile returned %d.", rc);
            rv++;
            goto exit;
        }
        if(rdLen != sizeof(myBuf)) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_ReadFile returned %d bytes, expected %d.",
                         rdLen, sizeof(myBuf));
            rv++;
            goto exit;
        }
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "VSSI_ReadFile returns %d.", rc);

        // Verify the data read back is same as data written
        rc = memcmp (readBuf, myBuf, sizeof(myBuf));
        if (rc != 0) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_ReadFile data does not match VSSI_WriteFile.");
            rv++;
            goto exit;
        }
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "memcmp returns %d.", rc);
    }

    for (i = 0; i < max_opened; ++i) {
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "i = %d", i);

        memcpy (filename, name, strlen(name));
        sprintf (filename+strlen(name), "%d%s", i, fileext);

        // Close file handle
        VSSI_CloseFile(fileHandleMany[i],
                   &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_CloseFile returns %d.", rc);
            rv++;
            goto exit;
        }
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "VSSI_CloseFile returns %d for file %s.", rc, filename);

        // Remove the file
        VSSI_Remove(handle, filename,
                &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_Remove returns %d.", rc);
            rv++;
            goto exit;
        }
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "VSSI_Remove returns %d for file %s.", rc, filename);

    }

exit:
    // Close the VSS Object
    VSSI_CloseObject(handle,
                        &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Failed to close object %s: %d",
                         save_description, rc);
        rv++;
        goto fail_open;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "VSSI_CloseObject returns %d.", rc);

 fail_open:
    VPLSem_Destroy(&(test_context.sem));
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                   "Test %s result: %d.",
                   vsTest_curTestName, rv);

    return rv;
}

static const char vsTest_vss_file_oplocks[] = "VSS File Oplocks Test";
static int test_vss_file_oplocks(VSSI_Session session,
                                  const char* save_description,
                                  u64 user_id,
                                  u64 dataset_id,
                                  const VSSI_RouteInfo& route_info,
                                  bool use_xml_api)
{
    int rv = 0;
    int rc;
    u32 i;
    const u32 numObjects = 2;
    vscs_obj_context_t objects[numObjects];
    VSSI_Object& handle                          = objects[0].handle;
    VSSI_Object& handle2                         = objects[1].handle;
    vscs_test_context_t& test_context            = objects[0].test_context;
    vscs_test_context_t& test_context2           = objects[1].test_context;
    vscs_notification_context_t& notify_context  = objects[0].notify_context;
    vscs_notification_context_t& notify_context2 = objects[1].notify_context;
    vscs_test_context_t unlock_context;
    static const char* name = "/myDir1/myDir2/foo.txt";
    static const char* dirname1 = "/myDir1";
    static const char* dirname2 = "/myDir1/myDir2";
    u32 flags;
    u32 attrs;
    VSSI_File fileHandle1;
    VSSI_File fileHandle2;
    VSSI_NotifyMask mask;
    VSSI_FileLockState lock_state;
    VSSI_ServerFileId fileId;
    u32 rdLen;
    u32 wrLen;
    char myBuf[256];

    VPL_SET_UNINITIALIZED(&(test_context.sem));
    VPL_SET_UNINITIALIZED(&(test_context2.sem));
    VPL_SET_UNINITIALIZED(&(notify_context.sem));
    VPL_SET_UNINITIALIZED(&(notify_context2.sem));
    VPL_SET_UNINITIALIZED(&(unlock_context.sem));

    vsTest_curTestName = vsTest_vss_file_oplocks;
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Starting test: %s.",
                        vsTest_curTestName);

    if(VPLSem_Init(&(test_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        ++rv;
        goto exit1;
    }

    if(VPLSem_Init(&(test_context2.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        ++rv;
        goto exit2;
    }

    if(VPLSem_Init(&(unlock_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        ++rv;
        goto exit3;
    }

    if(VPLSem_Init(&(notify_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        ++rv;
        goto exit4;
    }

    if(VPLSem_Init(&(notify_context2.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        ++rv;
        goto exit5;
    }

    // Set initial conditions: delete object contents.
    if ( use_xml_api ) {
        VSSI_Delete(vssi_session, save_description,
                    &test_context, vscs_test_callback);
    }
    else {
        VSSI_Delete2(vssi_session, user_id, dataset_id, &route_info,
                     &test_context, vscs_test_callback);
    }
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Delete object %s failed: %d.",
                         save_description, rc);
        rv++;
        goto exit6;
    }

    if(use_xml_api) {
        VSSI_OpenObject(vssi_session, save_description,
                        VSSI_READWRITE | VSSI_FORCE, &handle,
                        &test_context, vscs_test_callback);
    }
    else {
        VSSI_OpenObject2(vssi_session, user_id, dataset_id, &route_info,
                         VSSI_READWRITE | VSSI_FORCE, &handle,
                         &test_context, vscs_test_callback);
    }
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Open object %s failed: %d.",
                         save_description, rc);
        rv++;
        goto exit6;
    }

    if(use_xml_api) {
        VSSI_OpenObject(vssi_session, save_description,
                        VSSI_READWRITE | VSSI_FORCE, &handle2,
                        &test_context2, vscs_test_callback);
    }
    else {
        VSSI_OpenObject2(vssi_session, user_id, dataset_id, &route_info,
                         VSSI_READWRITE | VSSI_FORCE, &handle2,
                         &test_context2, vscs_test_callback);
    }
    VPLSem_Wait(&(test_context2.sem));
    rc = test_context2.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Open object %s failed: %d.",
                         save_description, rc);
        rv++;
        goto exit7;
    }



    //
    // Create the directories for the test
    //
    attrs = 0;

    // Now the parent directory first
    VSSI_MkDir2(handle, dirname1, attrs,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_MkDir2 %s returns %d.", dirname1, rc);
        rv++;
        goto exit;
    }

    VSSI_Commit(handle, &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                            "Commit on make directory %s: %d.",
                            dirname1, rc);
        rv++;
        goto exit;
    }

    VSSI_MkDir2(handle, dirname2, attrs,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_MkDir2 %s returns %d.", dirname2, rc);
        rv++;
        goto exit;
    }

    VSSI_Commit(handle, &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Commit on make directory %s: %d.",
                         dirname2, rc);
        rv++;
        goto exit;
    }

    // OpenFile
    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_CREATE;
    flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;
    attrs = 0;

    VSSI_OpenFile(handle, name, flags, attrs, &fileHandle1,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS && rc != VSSI_EXISTS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.", rc);
        rv++;
        goto exit;
    }

    // Initialize the file
    memset(myBuf, 0x42, sizeof(myBuf));
    wrLen = sizeof(myBuf);
    VSSI_WriteFile(fileHandle1, 0, &wrLen, myBuf,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_WriteFile returned %d.", rc);
        rv++;
        goto exit;
    }

    VSSI_CloseFile(fileHandle1,
                   &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_CloseFile returns %d.", rc);
        rv++;
        goto exit;
    }
    fileHandle1 = NULL;

    VSSI_OpenFile(handle, name, flags, attrs, &fileHandle1,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_EXISTS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.", rc);
        rv++;
        goto exit;
    }

    fileId = VSSI_GetServerFileId(fileHandle1);
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "VSSI_OpenFile returns Server FID %x for FH1 %p.",
                        fileId, fileHandle1);

    // Open a second handle though a different object
    VSSI_OpenFile(handle2, name, flags, attrs, &fileHandle2,
                  &test_context2, vscs_test_callback);
    VPLSem_Wait(&(test_context2.sem));
    rc = test_context2.rv;
    if(rc != VSSI_EXISTS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.", rc);
        rv++;
        goto exit;
    }
    
    fileId = VSSI_GetServerFileId(fileHandle2);
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "VSSI_OpenFile returns Server FID %x for FH2 %p.",
                        fileId, fileHandle2);

    vssiFile[0] = fileHandle1;
    vssiFile[1] = fileHandle2;
    numVssiFiles = 2;


    //
    // Test setting of oplocks and generation of oplock break notifications
    //

    // Set up notification callback
    for (i = 0; i < numObjects; ++i) {
        mask = VSSI_NOTIFY_OPLOCK_BREAK_EVENT;
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                            "Set objects[%u] VSSI_NOTIFY_OPLOCK_BREAK_EVENT Event, set mask "FMTx64, i, mask);
        VSSI_SetNotifyEvents(objects[i].handle, &mask, &objects[i].notify_context,
                             oplock_notification_callback,
                             &objects[i].test_context, vscs_test_callback);
        VPLSem_Wait(&(objects[i].test_context.sem));
        rc = objects[i].test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Set Notify Events for objects[%u] returned result %d.",
                             i, rc);
            rv++;
            goto exit;
        }

        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                            "Set objects[%u] VSSI_NOTIFY_OPLOCK_BREAK_EVENT Event returned success for set mask "FMTx64,
                            i, mask);

        if(mask != VSSI_NOTIFY_OPLOCK_BREAK_EVENT) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Set objects[%u] Notify Events returned mask "FMTx64". Expected %x",
                             i, mask,
                             VSSI_NOTIFY_OPLOCK_BREAK_EVENT);
            rv++;
            goto exit;
        } 
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                            "Set objects[%u] VSSI_NOTIFY_OPLOCK_BREAK_EVENT mask == VSSI_NOTIFY_OPLOCK_BREAK_EVENT", i);
    }

    oplock_break_count = 0;
 
    // Test case 1:  Not oplocked, read oplock request, should succeed

    // FH1 cache for read
    VSSI_SetFileLockState(fileHandle1, VSSI_FILE_LOCK_CACHE_READ,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_SetFileLockState returned %d.", rc);
        rv++;
        goto exit;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "VSSI_SetFileLockState FH1 cache for read, success");

    // Test case 2:  One read oplocked, other object read oplock request, should succeed

    // FH2 cache for read, should succeed
    VSSI_SetFileLockState(fileHandle2, VSSI_FILE_LOCK_CACHE_READ,
                          &test_context2, vscs_test_callback);
    VPLSem_Wait(&(test_context2.sem));
    rc = test_context2.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_SetFileLockState returned %d.", rc);
        rv++;
        goto exit;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "VSSI_SetFileLockState FH2 cache for read, success");

    // Test case 3: read oplock, other object request write oplock, should reject

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "before VSSI_SetFileLockState FH2 cache for write");

    // FH2 cache for write, should be rejected
    VSSI_SetFileLockState(fileHandle2, VSSI_FILE_LOCK_CACHE_WRITE,
                          &test_context2, vscs_test_callback);
    VPLSem_Wait(&(test_context2.sem));
    rc = test_context2.rv;
    if(rc != VSSI_LOCKED) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_SetFileLockState returned %d expected VSSI_LOCKED.", rc);
        rv++;
        goto exit;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "VSSI_SetFileLockState FH2 cache for write, fails as expected (%d)",
                        rc);

    // Test case 4:  read oplocked, read request by other object, should succeed

    rdLen = sizeof(myBuf);
    VSSI_ReadFile(fileHandle2, 0, &rdLen, myBuf,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
            "VSSI_ReadFile returned %d.", rc);
        rv++;
        goto exit;
    }
    else if(rdLen != sizeof(myBuf)) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
            "VSSI_ReadFile size expect %u got %u.",
            sizeof(myBuf), rdLen );
        rv++;
        //goto exit;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "read successful");

    // Test case 5: read oplock, write request by other object

    // This write should trigger an oplock break for read or write
    // oplocks from other nodes.

    // now do the write

    // Get the oplock state and confirm it is READ
    VSSI_GetFileLockState(fileHandle1, &lock_state,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_GetFileLockState returns %d.", rc);
        rv++;
    }
    if(lock_state != VSSI_FILE_LOCK_CACHE_READ) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "lock_state "FMTx64", expected %x.", 
                         lock_state, VSSI_FILE_LOCK_CACHE_READ);
        rv++;
    }

    wrLen = 128;
    VSSI_WriteFile(fileHandle2, 128, &wrLen, myBuf,
                          &test_context2, vscs_test_callback);

    // Get the oplock state and confirm it is READ|BREAK, meaning that
    // there is a queued request for an oplock break
    VSSI_GetFileLockState(fileHandle1, &lock_state,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_GetFileLockState returns %d.", rc);
        rv++;
    }
    if(lock_state != (VSSI_FILE_LOCK_CACHE_READ|VSSI_FILE_LOCK_CACHE_BREAK)) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "lock_state "FMTx64", expected %x.", 
                         lock_state, (VSSI_FILE_LOCK_CACHE_READ|VSSI_FILE_LOCK_CACHE_BREAK));
        rv++;
    }

    // Wait for the oplock break notify event first to allow a test of the
    // full cycle without a multi-threaded client test program.

    VPLSem_Wait(&(notify_context.sem));
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "after VSSI_WriteFile that would trigger an oplock break");
    if(oplock_break_count > 0) {
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                            "oplock_break_count %u > 0", oplock_break_count);
        oplock_break_count = 0;

        // Release the other object read oplock and the write should finish

        VSSI_SetFileLockState(fileHandle1, VSSI_FILE_LOCK_NONE,
                              &unlock_context, vscs_test_callback);
        VPLSem_Wait(&(unlock_context.sem));
        if(unlock_context.rv != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                            "VSSI_SetFileLockState returned %d.", unlock_context.rv);
            rv++;
            goto exit;
        }
    }

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Now wait for the actual write to complete");
    //
    // Now wait for the actual write to complete
    //
    VPLSem_Wait(&(test_context2.sem));
    rc = test_context2.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_WriteFile returned %d.", rc);
        rv++;
        goto exit;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "write completed");

    // Setup lock state to none, should succeed even though it is already none

    // release FH1 oplocks
    VSSI_SetFileLockState(fileHandle1, VSSI_FILE_LOCK_NONE,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_SetFileLockState returned %d.", rc);
        rv++;
        goto exit;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "VSSI_SetFileLockState FH1 none, success");

    // release FH2 oplocks
    VSSI_SetFileLockState(fileHandle2, VSSI_FILE_LOCK_NONE,
                          &test_context2, vscs_test_callback);
    VPLSem_Wait(&(test_context2.sem));
    rc = test_context2.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_SetFileLockState returned %d.", rc);
        rv++;
        goto exit;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "VSSI_SetFileLockState FH2 none, success");

    // Test case 6:  Not oplocked, write oplock request, should succeed

    // no locks, FH1 cache for write, should succeed
    VSSI_SetFileLockState(fileHandle1, VSSI_FILE_LOCK_CACHE_WRITE,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_SetFileLockState returned %d.", rc);
        rv++;
        goto exit;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "VSSI_SetFileLockState FH1 write, success");

    // Test case 7: write oplocked, other object request read oplock, should reject

    // FH1 write locked, FH2 cache for read request, should be rejected
    VSSI_SetFileLockState(fileHandle2, VSSI_FILE_LOCK_CACHE_READ,
                          &test_context2, vscs_test_callback);
    VPLSem_Wait(&(test_context2.sem));
    rc = test_context2.rv;
    if(rc != VSSI_LOCKED) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "VSSI_SetFileLockState returned %d expected VSSI_LOCKED.", rc);
        rv++;
        goto exit;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "VSSI_SetFileLockState FH2 read lock request rejected as expected");

    // Test case 8: write oplock, other object request write oplock, should reject

    // FH1 write lock, FH2 cache for write request, should be rejected
    VSSI_SetFileLockState(fileHandle2, VSSI_FILE_LOCK_CACHE_WRITE,
                          &test_context2, vscs_test_callback);
    VPLSem_Wait(&(test_context2.sem));
    rc = test_context2.rv;
    if(rc != VSSI_LOCKED) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "VSSI_SetFileLockState returned %d expected VSSI_LOCKED.", rc);
        rv++;
        goto exit;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "VSSI_SetFileLockState FH2 write lock request rejected as expected");

    // Test case 9:  write oplocked, read request by other object

    // Read should trigger an oplock break for read or write
    // oplocks from other nodes.

    rdLen = sizeof(myBuf);
    VSSI_ReadFile(fileHandle2, 0, &rdLen, myBuf,
                  &test_context2, vscs_test_callback);

    // Wait for the oplock break notify event first to allow a test of the
    // full cycle without a multi-threaded client test program.

    VPLSem_Wait(&(notify_context.sem));
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "after VSSI_ReadFile that would trigger an oplock break");
    if(oplock_break_count > 0) {
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                            "oplock_break_count %u > 0", oplock_break_count);
        oplock_break_count = 0;

        // Release the other object write oplock and the read should finish

        VSSI_SetFileLockState(fileHandle1, VSSI_FILE_LOCK_NONE,
                              &unlock_context, vscs_test_callback);
        VPLSem_Wait(&(unlock_context.sem));
        if(unlock_context.rv != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                            "VSSI_SetFileLockState returned %d.", unlock_context.rv);
            rv++;
            goto exit;
        }
    }

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Now wait for the actual read to complete");

    // Now wait for the actual read to complete

    VPLSem_Wait(&(test_context2.sem));
    rc = test_context2.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_ReadFile returned %d.", rc);
        rv++;
        goto exit;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "read completed");

    // Restore FH1 write lock

    VSSI_SetFileLockState(fileHandle1, VSSI_FILE_LOCK_CACHE_WRITE,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_SetFileLockState returned %d.", rc);
        rv++;
        goto exit;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "VSSI_SetFileLockState FH1 write, success");

    // Test case 10: write oplock, write request by other object

    // Write should trigger an oplock break for read or write
    // oplocks from other nodes.

    wrLen = 128;
    VSSI_WriteFile(fileHandle2, 128, &wrLen, myBuf,
                          &test_context2, vscs_test_callback);
    //
    // Wait for the oplock break notify event first to allow a test of the
    // full cycle without a multi-threaded client test program.
    //
    VPLSem_Wait(&(notify_context.sem));
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "after VSSI_WriteFile that would trigger an oplock break");
    if(oplock_break_count > 0) {
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                            "oplock_break_count %u > 0", oplock_break_count);
        oplock_break_count = 0;

        // Release the other object write oplock and the write should finish

        VSSI_SetFileLockState(fileHandle1, VSSI_FILE_LOCK_NONE,
                              &unlock_context, vscs_test_callback);
        VPLSem_Wait(&(unlock_context.sem));
        if(unlock_context.rv != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                            "VSSI_SetFileLockState returned %d.", unlock_context.rv);
            rv++;
            goto exit;
        }
    }

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Now wait for the actual write to complete");

    // Now wait for the actual write to complete

    VPLSem_Wait(&(test_context2.sem));
    rc = test_context2.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_WriteFile returned %d.", rc);
        rv++;
        goto exit;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "write completed");


exit:
    if (fileHandle1) {
        VSSI_CloseFile(fileHandle1,
                       &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "VSSI_CloseFile returns %d.", rc);
            rv++;
        }
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                            "VSSI_CloseFile(fileHandle1) completed");
    }

    if (fileHandle2) {
        VSSI_CloseFile(fileHandle2,
                       &test_context2, vscs_test_callback);
        VPLSem_Wait(&(test_context2.sem));
        rc = test_context2.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "VSSI_CloseFile returns %d.", rc);
            rv++;
        }
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                            "VSSI_CloseFile(fileHandle2) completed");
    }

    vssiFile[0] = NULL;
    vssiFile[1] = NULL;
    numVssiFiles = 0;



    // Close the VSS Object
    VSSI_CloseObject(handle2,
                        &test_context2, vscs_test_callback);
    VPLSem_Wait(&(test_context2.sem));
    rc = test_context2.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Failed to close object %s: %d",
                         save_description, rc);
        rv++;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "VSSI_CloseObject(handle2) completed");


exit7:
    // Close the VSS Object
    VSSI_CloseObject(handle,
                        &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Failed to close object %s: %d",
                         save_description, rc);
        rv++;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "VSSI_CloseObject(handle) completed");

exit6:
    VPLSem_Destroy(&(notify_context2.sem));
exit5:
    VPLSem_Destroy(&(notify_context.sem));
exit4:
    VPLSem_Destroy(&(unlock_context.sem));
exit3:
    VPLSem_Destroy(&(test_context2.sem));
exit2:
    VPLSem_Destroy(&(test_context.sem));
exit1:
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                   "Test %s result: %d.",
                   vsTest_curTestName, rv);

    return rv;
}


static const char vsTest_vss_file_oplocks_brlocks[] = "VSS File Oplocks and Brlocks Test";
static int test_vss_file_oplocks_brlocks(VSSI_Session session,
                                  const char* save_description,
                                  u64 user_id,
                                  u64 dataset_id,
                                  const VSSI_RouteInfo& route_info,
                                  bool use_xml_api)
{
    int rv = 0;
    int rc;
    u32 i;
    const u32 numObjects = 2;
    vscs_obj_context_t objects[numObjects];
    VSSI_Object& handle                          = objects[0].handle;
    VSSI_Object& handle2                         = objects[1].handle;
    vscs_test_context_t& test_context            = objects[0].test_context;
    vscs_test_context_t& test_context2           = objects[1].test_context;
    vscs_notification_context_t& notify_context  = objects[0].notify_context;
    vscs_notification_context_t& notify_context2 = objects[1].notify_context;
    vscs_test_context_t unlock_context;
    static const char* name = "/testFolder1/testFolder2/foo.txt";
    static const char* dirname1 = "/testFolder1";
    static const char* dirname2 = "/testFolder1/testFolder2";
    u32 flags;
    u32 attrs;
    VSSI_File fileHandle1;
    VSSI_File fileHandle2;
    VSSI_NotifyMask mask;
    VSSI_FileLockState lock_state;
    VSSI_ServerFileId fileId;
    VSSI_ByteRangeLock brLock;
    u32 lockFlags;
    u32 wrLen;
    char myBuf[256];

    VPL_SET_UNINITIALIZED(&(test_context.sem));
    VPL_SET_UNINITIALIZED(&(test_context2.sem));
    VPL_SET_UNINITIALIZED(&(notify_context.sem));
    VPL_SET_UNINITIALIZED(&(notify_context2.sem));
    VPL_SET_UNINITIALIZED(&(unlock_context.sem));

    vsTest_curTestName = vsTest_vss_file_oplocks_brlocks;
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Starting test: %s.",
                        vsTest_curTestName);

    if(VPLSem_Init(&(test_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        ++rv;
        goto exit1;
    }

    if(VPLSem_Init(&(test_context2.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        ++rv;
        goto exit2;
    }

    if(VPLSem_Init(&(unlock_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        ++rv;
        goto exit3;
    }

    if(VPLSem_Init(&(notify_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        ++rv;
        goto exit4;
    }

    if(VPLSem_Init(&(notify_context2.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        ++rv;
        goto exit5;
    }

    // Set initial conditions: delete object contents.
    if ( use_xml_api ) {
        VSSI_Delete(vssi_session, save_description,
                    &test_context, vscs_test_callback);
    }
    else {
        VSSI_Delete2(vssi_session, user_id, dataset_id, &route_info,
                     &test_context, vscs_test_callback);
    }
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Delete object %s failed: %d.",
                         save_description, rc);
        rv++;
        goto exit6;
    }

    if(use_xml_api) {
        VSSI_OpenObject(vssi_session, save_description,
                        VSSI_READWRITE | VSSI_FORCE, &handle,
                        &test_context, vscs_test_callback);
    }
    else {
        VSSI_OpenObject2(vssi_session, user_id, dataset_id, &route_info,
                         VSSI_READWRITE | VSSI_FORCE, &handle,
                         &test_context, vscs_test_callback);
    }
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Open object %s failed: %d.",
                         save_description, rc);
        rv++;
        goto exit6;
    }

    if(use_xml_api) {
        VSSI_OpenObject(vssi_session, save_description,
                        VSSI_READWRITE | VSSI_FORCE, &handle2,
                        &test_context2, vscs_test_callback);
    }
    else {
        VSSI_OpenObject2(vssi_session, user_id, dataset_id, &route_info,
                         VSSI_READWRITE | VSSI_FORCE, &handle2,
                         &test_context2, vscs_test_callback);
    }
    VPLSem_Wait(&(test_context2.sem));
    rc = test_context2.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Open object %s failed: %d.",
                         save_description, rc);
        rv++;
        goto exit7;
    }

    //
    // Create the directories for the test
    //
    attrs = 0;

    // Now the parent directory first
    VSSI_MkDir2(handle, dirname1, attrs,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_MkDir2 %s returns %d.", dirname1, rc);
        rv++;
        goto exit;
    }

    VSSI_Commit(handle, &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                            "Commit on make directory %s: %d.",
                            dirname1, rc);
        rv++;
        goto exit;
    }

    VSSI_MkDir2(handle, dirname2, attrs,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_MkDir2 %s returns %d.", dirname2, rc);
        rv++;
        goto exit;
    }

    VSSI_Commit(handle, &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Commit on make directory %s: %d.",
                         dirname2, rc);
        rv++;
        goto exit;
    }

    // OpenFile
    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_CREATE;
    flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;
    attrs = 0;

    VSSI_OpenFile(handle, name, flags, attrs, &fileHandle1,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS && rc != VSSI_EXISTS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.", rc);
        rv++;
        goto exit;
    }

    // Initialize the file
    memset(myBuf, 0x42, sizeof(myBuf));
    wrLen = sizeof(myBuf);
    VSSI_WriteFile(fileHandle1, 0, &wrLen, myBuf,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_WriteFile returned %d.", rc);
        rv++;
        goto exit;
    }

    VSSI_CloseFile(fileHandle1,
                   &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_CloseFile returns %d.", rc);
        rv++;
        goto exit;
    }
    fileHandle1 = NULL;

    VSSI_OpenFile(handle, name, flags, attrs, &fileHandle1,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_EXISTS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.", rc);
        rv++;
        goto exit;
    }

    fileId = VSSI_GetServerFileId(fileHandle1);
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "VSSI_OpenFile returns Server FID %x for FH1 %p.",
                        fileId, fileHandle1);

    // Open a second handle though a different object
    VSSI_OpenFile(handle2, name, flags, attrs, &fileHandle2,
                  &test_context2, vscs_test_callback);
    VPLSem_Wait(&(test_context2.sem));
    rc = test_context2.rv;
    if(rc != VSSI_EXISTS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.", rc);
        rv++;
        goto exit;
    }
    
    fileId = VSSI_GetServerFileId(fileHandle2);
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "VSSI_OpenFile returns Server FID %x for FH2 %p.",
                        fileId, fileHandle2);

    vssiFile[0] = fileHandle1;
    vssiFile[1] = fileHandle2;
    numVssiFiles = 2;

    //
    // Test setting of oplocks and generation of oplock break notifications
    //

    // Set up notification callback
    for (i = 0; i < numObjects; ++i) {
        mask = VSSI_NOTIFY_OPLOCK_BREAK_EVENT;
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                            "Set objects[%u] VSSI_NOTIFY_OPLOCK_BREAK_EVENT Event, set mask "FMTx64, i, mask);
        VSSI_SetNotifyEvents(objects[i].handle, &mask, &objects[i].notify_context,
                             oplock_notification_callback,
                             &objects[i].test_context, vscs_test_callback);
        VPLSem_Wait(&(objects[i].test_context.sem));
        rc = objects[i].test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Set Notify Events for objects[%u] returned result %d.",
                             i, rc);
            rv++;
            goto exit;
        }

        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                            "Set objects[%u] VSSI_NOTIFY_OPLOCK_BREAK_EVENT Event returned success for set mask "FMTx64,
                            i, mask);

        if(mask != VSSI_NOTIFY_OPLOCK_BREAK_EVENT) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Set objects[%u] Notify Events returned mask "FMTx64". Expected %x",
                             i, mask,
                             VSSI_NOTIFY_OPLOCK_BREAK_EVENT);
            rv++;
            goto exit;
        } 
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                            "Set objects[%u] VSSI_NOTIFY_OPLOCK_BREAK_EVENT mask == VSSI_NOTIFY_OPLOCK_BREAK_EVENT", i);
    }

    oplock_break_count = 0;
 
    // Test case 1:  Not oplocked, read oplock request, should succeed

    // FH1 cache for read
    VSSI_SetFileLockState(fileHandle1, VSSI_FILE_LOCK_CACHE_READ,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_SetFileLockState returned %d.", rc);
        rv++;
        goto exit;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "VSSI_SetFileLockState FH1 cache for read, success");

    // Test case 2:  One read oplocked, other object read oplock request, should succeed

    // FH2 cache for read, should succeed
    VSSI_SetFileLockState(fileHandle2, VSSI_FILE_LOCK_CACHE_READ,
                          &test_context2, vscs_test_callback);
    VPLSem_Wait(&(test_context2.sem));
    rc = test_context2.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_SetFileLockState returned %d.", rc);
        rv++;
        goto exit;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "VSSI_SetFileLockState FH2 cache for read, success");

    // Test case 3: read oplock, set byte range request by other object

    // This write should trigger an oplock break for read or write
    // oplocks from other nodes.

    // Get the oplock state and confirm it is READ
    VSSI_GetFileLockState(fileHandle1, &lock_state,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_GetFileLockState returns %d.", rc);
        rv++;
    }
    if(lock_state != VSSI_FILE_LOCK_CACHE_READ) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "lock_state "FMTx64", expected %x.", 
                         lock_state, VSSI_FILE_LOCK_CACHE_READ);
        rv++;
    }

    // Ask for a byte range lock through FH2.  This should
    // trigger an OPlock break to FH1.
    brLock.lock_mask = VSSI_FILE_LOCK_READ_SHARED;
    brLock.offset = 0;
    brLock.length = 128;
    lockFlags = VSSI_RANGE_LOCK|VSSI_RANGE_NOWAIT;

    VSSI_SetByteRangeLock(fileHandle2, &brLock, lockFlags,
                          &test_context2, vscs_test_callback);

    // Can't wait for this call, since it will block

    // Get the oplock state and confirm it is READ|BREAK, meaning that
    // there is a queued request for an oplock break
    VSSI_GetFileLockState(fileHandle1, &lock_state,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_GetFileLockState returns %d.", rc);
        rv++;
    }
    if(lock_state != (VSSI_FILE_LOCK_CACHE_READ|VSSI_FILE_LOCK_CACHE_BREAK)) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "lock_state "FMTx64", expected %x.", 
                         lock_state, (VSSI_FILE_LOCK_CACHE_READ|VSSI_FILE_LOCK_CACHE_BREAK));
        rv++;
    }

    // Wait for the oplock break notify event first to allow a test of the
    // full cycle without a multi-threaded client test program.

    VPLSem_Wait(&(notify_context.sem));
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "after VSSI_SetByteRangeLock that would trigger an oplock break");
    if(oplock_break_count > 0) {
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                            "oplock_break_count %u > 0", oplock_break_count);
        oplock_break_count = 0;

        // Release the other object read oplock and the brlock should finish

        VSSI_SetFileLockState(fileHandle1, VSSI_FILE_LOCK_NONE,
                              &unlock_context, vscs_test_callback);
        VPLSem_Wait(&(unlock_context.sem));
        if(unlock_context.rv != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                            "VSSI_SetFileLockState returned %d.", unlock_context.rv);
            rv++;
            goto exit;
        }
    }

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Now wait for the actual brlock to complete");
    //
    // Now wait for the brlock call to complete
    //
    VPLSem_Wait(&(test_context2.sem));
    rc = test_context2.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_SetByteRangeLock returns %d.", rc);
        rv++;
        goto exit;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "brlock completed");

    // Setup lock state to none, should succeed even though it is already none

    // release FH1 oplocks
    VSSI_SetFileLockState(fileHandle1, VSSI_FILE_LOCK_NONE,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_SetFileLockState returned %d.", rc);
        rv++;
        goto exit;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "VSSI_SetFileLockState FH1 none, success");

    // Get the oplock state of FH2
    VSSI_GetFileLockState(fileHandle2, &lock_state,
                          &test_context2, vscs_test_callback);
    VPLSem_Wait(&(test_context2.sem));
    rc = test_context2.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_GetFileLockState returns %d.", rc);
        rv++;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "VSSI_GetFileLockState FH2 %llx", lock_state);

    // release FH2 oplocks
    VSSI_SetFileLockState(fileHandle2, VSSI_FILE_LOCK_NONE,
                          &test_context2, vscs_test_callback);
    VPLSem_Wait(&(test_context2.sem));
    rc = test_context2.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_SetFileLockState returned %d.", rc);
        rv++;
        goto exit;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "VSSI_SetFileLockState FH2 none, success");

    // Release the byte range lock, so that we can set other oplocks
    // for the next test
    brLock.lock_mask = VSSI_FILE_LOCK_READ_SHARED;
    brLock.offset = 0;
    brLock.length = 128;
    lockFlags = VSSI_RANGE_UNLOCK|VSSI_RANGE_NOWAIT;

    VSSI_SetByteRangeLock(fileHandle2, &brLock, lockFlags,
                          &test_context2, vscs_test_callback);
    VPLSem_Wait(&(test_context2.sem));
    rc = test_context2.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_SetByteRangeLock returns %d.", rc);
        rv++;
        goto exit;
    }

    // Test case 6:  Not oplocked, write oplock request, should succeed

    // no locks, FH1 cache for write, should succeed
    VSSI_SetFileLockState(fileHandle1, VSSI_FILE_LOCK_CACHE_WRITE,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_SetFileLockState returned %d.", rc);
        rv++;
        goto exit;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "VSSI_SetFileLockState FH1 write, success");

    // Test case 7:  write oplocked, brlock request by other object

    // Wait for the oplock break notify event first to allow a test of the
    // full cycle without a multi-threaded client test program.
    brLock.lock_mask = VSSI_FILE_LOCK_WRITE_EXCL;
    brLock.offset = 0;
    brLock.length = 128;
    lockFlags = VSSI_RANGE_LOCK|VSSI_RANGE_WAIT;

    VSSI_SetByteRangeLock(fileHandle2, &brLock, lockFlags,
                          &test_context2, vscs_test_callback);

    VPLSem_Wait(&(notify_context.sem));
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "after VSSI_SetByteRangeLock that would trigger an oplock break");
    if(oplock_break_count > 0) {
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                            "oplock_break_count %u > 0", oplock_break_count);
        oplock_break_count = 0;

        // Release the other object write oplock and the brlock should finish

        VSSI_SetFileLockState(fileHandle1, VSSI_FILE_LOCK_NONE,
                              &unlock_context, vscs_test_callback);
        VPLSem_Wait(&(unlock_context.sem));
        if(unlock_context.rv != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                            "VSSI_SetFileLockState returned %d.", unlock_context.rv);
            rv++;
            goto exit;
        }
    }

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Now wait for the actual brlock to complete");

    // Now wait for the actual brlock to complete

    VPLSem_Wait(&(test_context2.sem));
    rc = test_context2.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_SetByteRangeLock returned %d.", rc);
        rv++;
        goto exit;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "brlock completed");

exit:
    if (fileHandle1) {
        VSSI_CloseFile(fileHandle1,
                       &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "VSSI_CloseFile returns %d.", rc);
            rv++;
        }
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                            "VSSI_CloseFile(fileHandle1) completed");
    }

    if (fileHandle2) {
        VSSI_CloseFile(fileHandle2,
                       &test_context2, vscs_test_callback);
        VPLSem_Wait(&(test_context2.sem));
        rc = test_context2.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "VSSI_CloseFile returns %d.", rc);
            rv++;
        }
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                            "VSSI_CloseFile(fileHandle2) completed");
    }

    vssiFile[0] = NULL;
    vssiFile[1] = NULL;
    numVssiFiles = 0;

    // Close the VSS Object
    VSSI_CloseObject(handle2,
                        &test_context2, vscs_test_callback);
    VPLSem_Wait(&(test_context2.sem));
    rc = test_context2.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Failed to close object %s: %d",
                         save_description, rc);
        rv++;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "VSSI_CloseObject(handle2) completed");

exit7:
    // Close the VSS Object
    VSSI_CloseObject(handle,
                        &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Failed to close object %s: %d",
                         save_description, rc);
        rv++;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "VSSI_CloseObject(handle) completed");

exit6:
    VPLSem_Destroy(&(notify_context2.sem));
exit5:
    VPLSem_Destroy(&(notify_context.sem));
exit4:
    VPLSem_Destroy(&(unlock_context.sem));
exit3:
    VPLSem_Destroy(&(test_context2.sem));
exit2:
    VPLSem_Destroy(&(test_context.sem));
exit1:
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                   "Test %s result: %d.",
                   vsTest_curTestName, rv);
    return rv;
}


static const char vsTest_vss_object_teardown[] = "VSS Object Teardown Test";
static int test_vss_object_teardown(VSSI_Session session,
                                    const char* save_description,
                                    u64 user_id,
                                    u64 dataset_id,
                                    const VSSI_RouteInfo& route_info,
                                    bool use_xml_api)
{
    int rv = 0;
    int rc;
    const u32 numObjects = 2;
    vscs_obj_context_t objects[numObjects];
    VSSI_Object& handle                          = objects[0].handle;
    VSSI_Object& handle2                         = objects[1].handle;
    vscs_test_context_t& test_context            = objects[0].test_context;
    vscs_test_context_t& test_context2           = objects[1].test_context;
    vscs_notification_context_t& notify_context  = objects[0].notify_context;
    vscs_notification_context_t& notify_context2 = objects[1].notify_context;
    vscs_test_context_t unlock_context;
    vscs_test_context_t third_context;
    static const char* name = "/objDestDir1/objDestDir2/foo.txt";
    static const char* dirname1 = "/objDestDir1";
    static const char* dirname2 = "/objDestDir1/objDestDir2";
    VPLTime_t wait_time;
    u32 flags;
    u32 attrs;
    VSSI_File fileHandle1;
    VSSI_File fileHandle2;
    u32 wrLen;
    char myBuf[256];

    VPL_SET_UNINITIALIZED(&(test_context.sem));
    VPL_SET_UNINITIALIZED(&(test_context2.sem));
    VPL_SET_UNINITIALIZED(&(notify_context.sem));
    VPL_SET_UNINITIALIZED(&(notify_context2.sem));
    VPL_SET_UNINITIALIZED(&(unlock_context.sem));
    VPL_SET_UNINITIALIZED(&(third_context.sem));

    vsTest_curTestName = vsTest_vss_object_teardown;
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Starting test: %s.",
                        vsTest_curTestName);

    if(VPLSem_Init(&(test_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        return rv+1;
    }

    if(VPLSem_Init(&(test_context2.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        rv++;
        goto exit1;
    }

    if(VPLSem_Init(&(unlock_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        rv++;
        goto exit2;
    }

    if(VPLSem_Init(&(notify_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        rv++;
        goto exit3;
    }

    if(VPLSem_Init(&(notify_context2.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        rv++;
        goto exit4;
    }

    if(VPLSem_Init(&(third_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        rv++;
        goto exit5;
    }

    // Set initial conditions: delete object contents.
    if ( use_xml_api ) {
        VSSI_Delete(vssi_session, save_description,
                    &test_context, vscs_test_callback);
    }
    else {
        VSSI_Delete2(vssi_session, user_id, dataset_id, &route_info,
                     &test_context, vscs_test_callback);
    }
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Delete object %s failed: %d.",
                         save_description, rc);
        rv++;
        goto exit6;
    }

    if(use_xml_api) {
        VSSI_OpenObject(vssi_session, save_description,
                        VSSI_READWRITE | VSSI_FORCE, &handle,
                        &test_context, vscs_test_callback);
    }
    else {
        VSSI_OpenObject2(vssi_session, user_id, dataset_id, &route_info,
                         VSSI_READWRITE | VSSI_FORCE, &handle,
                         &test_context, vscs_test_callback);
    }
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Open object %s failed: %d.",
                         save_description, rc);
        rv++;
        goto exit6;
    }

    if(use_xml_api) {
        VSSI_OpenObject(vssi_session, save_description,
                        VSSI_READWRITE | VSSI_FORCE, &handle2,
                        &test_context2, vscs_test_callback);
    }
    else {
        VSSI_OpenObject2(vssi_session, user_id, dataset_id, &route_info,
                         VSSI_READWRITE | VSSI_FORCE, &handle2,
                         &test_context2, vscs_test_callback);
    }
    VPLSem_Wait(&(test_context2.sem));
    rc = test_context2.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Open object %s failed: %d.",
                         save_description, rc);
        rv++;
        goto fail_open;
    }

    //
    // Create the directories for the test
    //
    attrs = 0;

    VSSI_MkDir2(handle, dirname1, attrs,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_MkDir2 %s returns %d.", dirname1, rc);
        rv++;
    }

    VSSI_Commit(handle, &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                            "Commit on make directory %s: %d.",
                            dirname1, rc);
        rv++;
    }

    // Create second level directory
    VSSI_MkDir2(handle, dirname2, attrs,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_MkDir2 %s returns %d.", dirname2, rc);
        rv++;
    }

    VSSI_Commit(handle, &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Commit on make directory %s: %d.",
                         dirname2, rc);
        rv++;
    }

    // OpenFile
    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_CREATE;
    flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;
    attrs = 0;

    VSSI_OpenFile(handle, name, flags, attrs, &fileHandle1,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS && rc != VSSI_EXISTS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.", rc);
        rv++;
    }

    // Initialize the file
    memset(myBuf, 0x42, sizeof(myBuf));
    wrLen = sizeof(myBuf);
    VSSI_WriteFile(fileHandle1, 0, &wrLen, myBuf,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_WriteFile returned %d.", rc);
        rv++;
    }
    else if(wrLen != sizeof(myBuf)) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_WriteFile wrote %d, expected to write %d.",
                         wrLen, sizeof(myBuf));
        rv++;
    }

    VSSI_CloseFile(fileHandle1,
                   &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_CloseFile returns %d.", rc);
        rv++;
    }

    // Reopen the files with handles from the two different objects
    flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_OPEN_ALWAYS;
    flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;

    VSSI_OpenFile(handle, name, flags, attrs, &fileHandle1,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_EXISTS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.", rc);
        rv++;
    }

    // Open a second handle to the same file from the other object
    VSSI_OpenFile(handle2, name, flags, attrs, &fileHandle2,
                  &test_context2, vscs_test_callback);
    VPLSem_Wait(&(test_context2.sem));
    rc = test_context2.rv;
    if(rc != VSSI_EXISTS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenFile returns %d.", rc);
        rv++;
    }
    
    // Test byte range locking
    VSSI_ByteRangeLock brLock;
    u32 lockFlags;

    brLock.lock_mask = VSSI_FILE_LOCK_READ_SHARED;
    brLock.offset = 0;
    brLock.length = 128;
    lockFlags = VSSI_RANGE_LOCK|VSSI_RANGE_NOWAIT;

    VSSI_SetByteRangeLock(fileHandle1, &brLock, lockFlags,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_SetByteRangeLock returns %d.", rc);
        rv++;
    }

    // Set a non-conflicting lock on FH2
    brLock.lock_mask = VSSI_FILE_LOCK_WRITE_EXCL;
    brLock.offset = 128;
    brLock.length = 128;
    lockFlags = VSSI_RANGE_LOCK|VSSI_RANGE_NOWAIT;

    VSSI_SetByteRangeLock(fileHandle2, &brLock, lockFlags,
                          &test_context2, vscs_test_callback);
    VPLSem_Wait(&(test_context2.sem));
    rc = test_context2.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_SetByteRangeLock returned %d.", rc);
        rv++;
    }

    // Try actual IO

    // FH2 should be able to write at 128
    wrLen = 128;
    VSSI_WriteFile(fileHandle2, 128, &wrLen, myBuf,
                          &test_context2, vscs_test_callback);
    VPLSem_Wait(&(test_context2.sem));
    rc = test_context2.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_WriteFile returned %d.", rc);
        rv++;
    }

    //
    // Test byte range locks with blocking behavior
    //

    // FH1 tries for READ on FH2 WREXCL segment, but asks to
    // block and be released when the lock releases
    brLock.lock_mask = VSSI_FILE_LOCK_READ_SHARED;
    brLock.offset = 240;
    brLock.length = 100;
    lockFlags = VSSI_RANGE_LOCK|VSSI_RANGE_WAIT;

    VSSI_SetByteRangeLock(fileHandle1, &brLock, lockFlags,
                          &test_context, vscs_test_callback);

    wait_time = VPLTime_FromMillisec(2000);
    // Timed semaphore wait for 2 seconds to confirm that it blocked
    rc = VPLSem_TimedWait(&(test_context.sem), wait_time);
    if(rc != VPL_ERR_TIMEOUT) {
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "FAIL:"
                            "VPLSem_TimedWait returned %d.", rc);
        rv++;
    }

    //
    // Now try another blocking lock from FH1 which overlaps
    // the FH2 WREXCL but does not conflict with the previous
    // FH1 READ [240,340).
    //
    brLock.lock_mask = VSSI_FILE_LOCK_WRITE_EXCL;
    brLock.offset = 144;
    brLock.length = 4;
    lockFlags = VSSI_RANGE_LOCK|VSSI_RANGE_WAIT;

    VSSI_SetByteRangeLock(fileHandle1, &brLock, lockFlags,
                          &third_context, vscs_test_callback);

    wait_time = VPLTime_FromMillisec(2000);
    // Confirm that it blocked
    rc = VPLSem_TimedWait(&(third_context.sem), wait_time);
    if(rc != VPL_ERR_TIMEOUT) {
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "FAIL:"
                            "VPLSem_TimedWait returned %d.", rc);
        rv++;
    }

    // Check that the first blocking lock is still blocked
    rc = VPLSem_TimedWait(&(test_context.sem), wait_time);
    if(rc != VPL_ERR_TIMEOUT) {
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "FAIL:"
                            "VPLSem_TimedWait returned %d.", rc);
        rv++;
    }

    // Close the second VSS Object on which fileHandle2 is opened
    VSSI_CloseObject(handle2,
                     &unlock_context, vscs_test_callback);
    VPLSem_Wait(&(unlock_context.sem));
    rc = unlock_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Failed to close object %s: %d",
                         save_description, rc);
        rv++;
    }

    // The object destructor for Object2 should have released the
    // locks held by fileHandle2

    // Now go back and wait for the FH1 READ LOCK request
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_SetByteRangeLock returned %d.", rc);
        rv++;
    }

    // Now go back and wait for the FH1 WREXCL LOCK request
    VPLSem_Wait(&(third_context.sem));
    rc = third_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_SetByteRangeLock returned %d.", rc);
        rv++;
    }

    // Release the FH1 READ lock
    brLock.lock_mask = VSSI_FILE_LOCK_READ_SHARED;
    brLock.offset = 240;
    brLock.length = 100;
    lockFlags = VSSI_RANGE_UNLOCK|VSSI_RANGE_WAIT;

    VSSI_SetByteRangeLock(fileHandle1, &brLock, lockFlags,
                          &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_SetByteRangeLock returned %d.", rc);
        rv++;
    }

    // Close the first VSS Object, which should close fileHandle1
 fail_open:
    VSSI_CloseObject(handle,
                        &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Failed to close object %s: %d",
                         save_description, rc);
        rv++;
    }

 exit6:
    VPLSem_Destroy(&(third_context.sem));
 exit5:
    VPLSem_Destroy(&(notify_context2.sem));
 exit4:
    VPLSem_Destroy(&(notify_context.sem));
 exit3:
    VPLSem_Destroy(&(unlock_context.sem));
 exit2:
    VPLSem_Destroy(&(test_context2.sem));
 exit1:
    VPLSem_Destroy(&(test_context.sem));

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                   "Test %s result: %d.",
                   vsTest_curTestName, rv);
    return rv;
}


#ifdef TEST_BAD_OBJHANDLE
static const char vsTest_vss_bad_objhandle[] = "VSS Bad Objhandle Test";
static int test_vss_bad_objhandle(VSSI_Session session,
                                  const char* save_description,
                                  u64 user_id,
                                  u64 dataset_id,
                                  const VSSI_RouteInfo& route_info,
                                  bool use_xml_api)
{
    int rv = 0;
    int rc;
    VSSI_Object handle;
    vscs_test_context_t test_context;

    VPL_SET_UNINITIALIZED(&(test_context.sem));

    vsTest_curTestName = vsTest_vss_bad_objhandle;
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Starting test: %s.",
                        vsTest_curTestName);

    if(VPLSem_Init(&(test_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        return rv+1;
    }

    if ( use_xml_api ) {
        VSSI_OpenObject(vssi_session, save_description,
                        VSSI_READWRITE, &handle,
                        &test_context, vscs_test_callback);
    }
    else {
        VSSI_OpenObject2(vssi_session, user_id, dataset_id, &route_info,
                         VSSI_READWRITE, &handle,
                         &test_context, vscs_test_callback);
    }
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Open object %s failed: %d.",
                         save_description, rc);
        rv++;
        goto fail_open;
    }

    // Cause object handle to go bad - restart VSS (user intervention required)
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Restart VSS now... 5 seconds to comply.");
    VPLThread_Sleep(VPLTIME_FROM_SEC(5));

    // Access the object
    VSSI_Erase(handle,
               &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_BADOBJ) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Access to invalid object got %d. Expected %d.",
                         rc, VSSI_BADOBJ);
        rv++;
    }

 fail_open:
    VPLSem_Destroy(&(test_context.sem));

    return rv;
}
#endif // TEST_BAD_OBJHANDLE

static const char vsTest_vss_session_recognition[] = "VSS Session Recognition Test";
int test_vss_session_recognition(u64 access_handle,
                                 const std::string& ticket,
                                 const std::string& location,
                                 u64 user_id,
                                 u64 dataset_id,
                                 const VSSI_RouteInfo& route_info,
                                 bool use_xml_api)
{
    string serviceTicket;
    u64 sessionHandle;
    VSSI_Session testSession = 0;
    vscs_test_context_t test_context;
    VSSI_Object handle;
    int rv = 0;
    int rc;

    VPL_SET_UNINITIALIZED(&(test_context.sem));

    vsTest_curTestName = vsTest_vss_session_recognition;
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                   "Starting test: %s.",
                   vsTest_curTestName);


    if(VPLSem_Init(&(test_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        return rv+1;
    }

    serviceTicket = ticket;
    sessionHandle = access_handle;
    serviceTicket.replace(0, 1, 1, serviceTicket[0] ^ 0xff);

    // Must end the legit session first.
    VSSI_EndSession(vssi_session);

    testSession = VSSI_RegisterSession(sessionHandle, serviceTicket.data());
    if(testSession == 0) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                    "Session registration failed!.");
        rv++;
    }
    else {
        if ( use_xml_api ) {
            VSSI_OpenObject(testSession, location.c_str(),
                            VSSI_READWRITE, &handle,
                            &test_context, vscs_test_callback);
        }
        else {
            VSSI_OpenObject2(testSession, user_id, dataset_id, &route_info,
                             VSSI_READWRITE, &handle,
                             &test_context, vscs_test_callback);
        }
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_BADSIG) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "VSS access with bad session secret returned %d. Expected %d.",
                             rc, VSSI_BADSIG);
            rv++;
        }
        else {
            VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "PASS:"
                                "VSS recognized bad signature.");
        }
        VSSI_EndSession(testSession);
    }

    serviceTicket = ticket;
    sessionHandle = access_handle;
    sessionHandle++;

    testSession = VSSI_RegisterSession(sessionHandle, serviceTicket.data());
    if(testSession == 0) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                    "Session registration failed!.");
        rv++;
    }
    else {
        if ( use_xml_api ) {
            VSSI_OpenObject(testSession, location.c_str(),
                            VSSI_READWRITE, &handle,
                            &test_context, vscs_test_callback);
        }
        else {
            VSSI_OpenObject2(testSession, user_id, dataset_id, &route_info,
                             VSSI_READWRITE, &handle,
                             &test_context, vscs_test_callback);
        }
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        // This test may get either NOLOGIN or COMM since the server closes
        // connection after attempting to send response.
        if(rc != VSSI_NOLOGIN && rc != VSSI_COMM) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "VSS access with bad session handle returned %d. Expected %d.",
                             rc, VSSI_BADSIG);
            rv++;
        }
        else {
            VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "PASS:"
                                "VSS recognized bad session handle.");
        }
        VSSI_EndSession(testSession);
    }

    VPLSem_Destroy(&(test_context.sem));

    // Re-eastablish the legit session.
    vssi_session = VSSI_RegisterSession(access_handle,
                                        ticket.c_str());

    return rv;
}

static const char vsTest_db_error_handling[] = "DB error handling Test";
int test_db_error_handling(const char* save_description,
                           u64 user_id,
                           u64 dataset_id,
                           const VSSI_RouteInfo& route_info,
                           s32 db_err_hand_param,
                           const std::string& db_dir)
{
    int rv = 0;
    int rc;
    VSSI_Object handle;
    vscs_test_context_t test_context;
    static const char* dirname1 = "/dbErrTestDir1";
    static const char* dirname2 = "/dbErrTestDir1/dbfsckTestDir2";
    static const char  dirname2Name[] = "dbfsckTestDir2";
    static const char* name1 = "/dbErrTestDir1/file1.txt";
    static const char* name2 = "/dbErrTestDir1/dbfsckTestDir2/file2.txt";
    const char* name = NULL;
    const char* zeroSizeFileName1 = "/dbErrTestDir1/zeroSizeFile1.txt";
    const char* zeroSizeFileName2 = "/dbErrTestDir1/zeroSizeFile2.txt";
    const char* zeroSizeFileName = NULL;
    const char* dirname = NULL;
    bool found = false;
    u32 flags;
    u32 attrs;
    VSSI_File fileHandle1 = NULL;
    VSSI_File fileHandle2 = NULL;
    VSSI_Dir2 dir = NULL;
    VSSI_Dirent2* dirent = NULL;
    s32 i;
    u32 wrLen;
    u32 rdLen;
    u32 expectedReadLen;
    u64 expectedStatSize;
    u64 expectedZeroSizeFileSize;
    VSSI_Dirent2* stats = NULL;
    char myBuf[256];
    static char myReadBuf[16*1024];
    u64 disk_size, dataset_size, avail_size;
    VSSI_NotifyMask returned_mask = 0;

    // Constants for db fragmentation test     
    const int num_top_level_trees = 20;
    const int num_subtrees = 20;
    const int num_subtree_levels = 4;
    const int num_subtree_subdirs = 4;
    const int num_subtree_files = 4;
    const int subtrees_to_remove = 10;
    const int num_deletions = 10;
    static const char* dirname_base = "/traverseDBDir";

    std::string db_needs_fsck = db_dir + "/db-needs-fsck";
    std::string db_fsck_fail_1 = db_dir + "/db-fsck-fail-1";
    std::string db_fsck_fail_2 = db_dir + "/db-fsck-fail-2";
    std::string db_backup = db_dir + "/db-1";
    std::string db = db_dir + "/db";

    bool db_needs_fsck_existed = (access(db_needs_fsck.c_str(), F_OK) == 0);
    bool db_fsck_fail_1_existed = (access(db_fsck_fail_1.c_str(), F_OK) == 0);
    bool db_fsck_fail_2_existed = (access(db_fsck_fail_2.c_str(), F_OK) == 0);


    enum { CreateFileExpectSuccess = 1,
           ReadFileExpectSuccess = 2, ReadFileExpectNotFound = 3,
           ExpectDBCorruptionOnOpen = 4, ExpectDBIrrecoverablyFailed = 5,
           CreateFile2ExpectSuccess = 6, ReadDir2AndFile2ExpectSuccess = 7,
           ReadDir2AndFile2ExpectNotFound = 8, ReadDir2AndFile2ExpectFileNotFound = 9,
           ReadFileExpectSuccessButFileLarger = 10,
           ReadFileExpectSuccessButFileSmaller = 11,
           ReadFileExpectCCDStopped = 12, ExpectDBCorruptionOnfsck = 13,
           CheckDBBackupCreation = 14,
           SetupForDBFragmentationTest = 15,
           TestDBFragmentationEffects = 16
         };

    VPL_SET_UNINITIALIZED(&(test_context.sem));

    vsTest_curTestName = vsTest_db_error_handling;
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Starting test: %s.",
                        vsTest_curTestName);


    if(VPLSem_Init(&(test_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        return rv+1;
    }

    VPLThread_Sleep(VPLTIME_FROM_SEC(10));

    VSSI_OpenObject2(vssi_session, user_id, dataset_id, &route_info,
                     VSSI_READWRITE | VSSI_FORCE, &handle,
                     &test_context, vscs_test_callback);

    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "db_err_hand_param is %d    VSSI_OpenObject2 returned %d.",
                        db_err_hand_param, rc);

    switch (db_err_hand_param) {
        case CreateFileExpectSuccess:
        case ReadFileExpectSuccess:
        case ReadFileExpectNotFound:
        case ReadFileExpectSuccessButFileLarger:
        case ReadFileExpectSuccessButFileSmaller:
        case ReadFileExpectCCDStopped:
        case ExpectDBCorruptionOnfsck:
            name = name1;
            dirname = dirname1;
            zeroSizeFileName = zeroSizeFileName1;
            break;

        case CreateFile2ExpectSuccess:
        case ReadDir2AndFile2ExpectSuccess:
        case ReadDir2AndFile2ExpectNotFound:
        case ReadDir2AndFile2ExpectFileNotFound:
            name = name2;
            dirname = dirname2;
            zeroSizeFileName = zeroSizeFileName2;
            break;
    }


    switch (db_err_hand_param) {
        case CreateFileExpectSuccess:
        case CreateFile2ExpectSuccess:
        case ReadFileExpectSuccess:
        case ReadFileExpectNotFound:
        case ReadDir2AndFile2ExpectSuccess:
        case ReadDir2AndFile2ExpectNotFound:
        case ReadDir2AndFile2ExpectFileNotFound:
        case ReadFileExpectSuccessButFileLarger:
        case ReadFileExpectSuccessButFileSmaller:
        case ReadFileExpectCCDStopped:
        case SetupForDBFragmentationTest:
        case TestDBFragmentationEffects:
        case CheckDBBackupCreation:
            if ( rc != VSSI_SUCCESS ) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "VSSI_OpenObject2() %s failed: %d.",
                                 save_description, rc);
                rv++;
            }
            break;

        case ExpectDBCorruptionOnOpen:
            if (rc == VSSI_COMM) {
                VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                                    "VSSI_OpenObject2() got expected VSSI_COMM.");
            }
            else if (rc == VSSI_ACCESS) {
                VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                                    "VSSI_OpenObject2() got expected VSSI_ACCESS.");
            }
            else {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "VSSI_OpenObject2() expected VSSI_ACCESS got: %d.",
                                 rc);
                rv++;
            }
            if (rc == VSSI_SUCCESS) {
                goto exit;
            }
            break;

        case ExpectDBCorruptionOnfsck:
            // because fsck occurs in bg, could get success or VSSI_COMM
            if (rc == VSSI_SUCCESS) {
                VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                                    "VSSI_OpenObject2() got expected VSSI_SUCCESS.");
            }
            else if (rc == VSSI_COMM) {
                VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                                    "VSSI_OpenObject2() got expected VSSI_COMM.");
            }
            else if (rc == VSSI_ACCESS) {
                VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                                    "VSSI_OpenObject2() got expected VSSI_ACCESS.");
            }
            else {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "VSSI_OpenObject2() expected VSSI_ACCESS or VSSI_COMM got: %d.",
                                 rc);
                rv++;
            }
            break;

        case ExpectDBIrrecoverablyFailed:
            if (rc == VSSI_FAILED) {
                VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                                    "Open Object got expected vssi_failed.");
            }
            else {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "Open object expected vssi_failed got: %d.",
                                 rc);
                rv++;
            }
            if (rc == VSSI_SUCCESS) {
                goto exit;
            }
            break;

        default:
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Invalid deb_err_hand_param: %d.",
                             db_err_hand_param);
            if (rc == VSSI_SUCCESS) {
                goto exit;
            }
            break;
    }

    if (rc != VSSI_SUCCESS) {
        goto fail_open;
    }

    if (db_err_hand_param != ReadFileExpectNotFound) {
        // fsck occurs in background so give it time to finish
        // fsck only starts if no db access for 2 min.
        // Allow 10 min for fsck to start and finish (it is a small db and normally takes seconds).
        // Most of the time there is no fsck required, so only wait if needed
        // if at beginning db-needs-fsck does not exist,
        //     don't wait.
        // else if at beginning db-needs-fsck exists and db-fsck-fail-1 does not exist
        //     wait for either db-needs-fsck goes away or db-fsck-fail-1 exists
        // else if at beginning db-needs-fsck and db-fsck-fail-1 exists and db-fsck-fail-2 does not exist,
        //     wait for either db-needs-fsck goes away or db-fsck-fail-2 exists
        // else if at beginning db-needs-fsck, db-fsck-fail-1, and db-fsck-fail-2 exists
        //     wait for either db-needs-fsck goes away or 3 min

        int max_seconds_to_wait_for_fsck = 10*60;

        if ( db_needs_fsck_existed && db_fsck_fail_1_existed  && db_fsck_fail_2_existed ) {
            max_seconds_to_wait_for_fsck = 3*60;
        }

        for (i = 0;  (access(db_needs_fsck.c_str(), F_OK) == 0) && i < max_seconds_to_wait_for_fsck; ++i) {
            VPLThread_Sleep(VPLTIME_FROM_SEC(1));
            if ( ! db_fsck_fail_1_existed  &&  access(db_fsck_fail_1.c_str(), F_OK) == 0) {
                break;
            }
            else if ( ! db_fsck_fail_2_existed  &&  access(db_fsck_fail_2.c_str(), F_OK) == 0) {
                break;
            }
            if ( (i % 30) == 29 ) {
                // do VSSI_GetNotifyEvents() so object will not timeout (times out after 150 sec)

                VSSI_GetNotifyEvents(handle, &returned_mask,
                                     &test_context, vscs_test_callback);
                VPLSem_Wait(&(test_context.sem));
                rc = test_context.rv;
                if(rc != VSSI_SUCCESS) {
                    VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                     "Get Notify Events returned result %d.", rc);
                    rv++;
                    goto exit;
                }

                // Check that VSSI_GetSpace() does not delay fsck

                VSSI_GetSpace(handle, &disk_size, &dataset_size, &avail_size,
                               &test_context, vscs_test_callback);
                VPLSem_Wait(&(test_context.sem));
                rc = test_context.rv;
                if(rc != VSSI_SUCCESS) {
                    VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                     "Getting disk space information: %d.", rc);
                    rv++;
                    goto exit;
                }
            }
        }

        if (i == max_seconds_to_wait_for_fsck && (access(db_needs_fsck.c_str(), F_OK) == 0) && !db_fsck_fail_2_existed) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Expected fsck to finish in less than %d seconds", max_seconds_to_wait_for_fsck);
            rv++;
            goto exit;
        }

        if (i == 0 && !db_needs_fsck_existed) {
            VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                                "db-needs-fsck was not present.");
        } else {
            VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                                "fsck finished in %d seconds.", i);
        }
    }

    if (db_err_hand_param == CheckDBBackupCreation) {
        int minutes_idle_time_before_db_backup_created = 15;
        int max_minutes_to_wait_for_db_backup = minutes_idle_time_before_db_backup_created + 1;
        bool db_backup_exists = (access(db_backup.c_str(), F_OK) == 0);
        bool db_exists = (access(db.c_str(), F_OK) == 0);

        if ( !db_exists || db_backup_exists) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                "Expected db at %s and no backup db %s", db_dir.c_str(), db_backup.c_str());
            rv++;
            goto exit;
        }

        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "Wait for backup db to be created.  "
            "Fail if created in less than %d minutes or more than %d minutes.",
             minutes_idle_time_before_db_backup_created, max_minutes_to_wait_for_db_backup);

        i = 0;
        while (true) {
            VPLThread_Sleep(VPLTIME_FROM_SEC(60));
            ++i;
            db_backup_exists = (access(db_backup.c_str(), F_OK) == 0);

            if (i < minutes_idle_time_before_db_backup_created && db_backup_exists) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "DB backup created in %d minutes", i);
                rv++;
                goto exit;
            }

            if (db_backup_exists) {
                VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                                 "DB backup exists after %d minutes", i);
                goto exit;
            }

            if (i == max_minutes_to_wait_for_db_backup) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "DB backup not created after %d min", i);
                rv++;
                goto exit;
            }

            // do VSSI_GetNotifyEvents() so object will not timeout (times out after 150 sec)

            VSSI_GetNotifyEvents(handle, &returned_mask,
                                 &test_context, vscs_test_callback);
            VPLSem_Wait(&(test_context.sem));
            rc = test_context.rv;
            if(rc != VSSI_SUCCESS) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "Get Notify Events returned result %d.", rc);
                rv++;
                goto exit;
            }

            // Check that VSSI_GetSpace() does not delay backup

            VSSI_GetSpace(handle, &disk_size, &dataset_size, &avail_size,
                           &test_context, vscs_test_callback);
            VPLSem_Wait(&(test_context.sem));
            rc = test_context.rv;
            if(rc != VSSI_SUCCESS) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "Getting disk space information: %d.", rc);
                rv++;
                goto exit;
            }

            VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                "%d minutes after start waiting for backup.  "
                "Disk size: "FMTu64", datasize size: "FMTu64", avail size: "FMTu64".",
                 i, disk_size, dataset_size, avail_size);
        }
    }

    if ( db_err_hand_param == CreateFileExpectSuccess ||
            db_err_hand_param == CreateFile2ExpectSuccess ) {
        //
        // Create a directory for file 1 or 2
        //
        attrs = 0;
        VSSI_MkDir2(handle, dirname, attrs,
                      &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS && rc != VSSI_ISDIR) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "VSSI_MkDir2 %s returns %d.", dirname, rc);
            rv++;
            goto exit;
        }

        // Create file 1 or 2
        flags = VSSI_FILE_OPEN_READ|VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_CREATE;
        flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;
        attrs = 0;

        VSSI_OpenFile(handle, name, flags, attrs, &fileHandle1,
                      &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS && rc != VSSI_EXISTS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "VSSI_OpenFile returns %d.", rc);
            rv++;
            goto exit;
        }

        // Initialize the file
        for (i = 0; i < sizeof(myBuf); ++i) {
            myBuf[i] = i;
        }
        wrLen = sizeof(myBuf);
        VSSI_WriteFile(fileHandle1, 0, &wrLen, myBuf,
                              &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "VSSI_WriteFile returned %d.", rc);
            rv++;
            goto exit;
        }

        // Create zero size file
        VSSI_OpenFile(handle, zeroSizeFileName, flags, attrs, &fileHandle2,
                      &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS && rc != VSSI_EXISTS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "VSSI_OpenFile returns %d.", rc);
            rv++;
            goto exit;
        }
    }

    // check the files and/or directories

    if ( db_err_hand_param == ReadDir2AndFile2ExpectSuccess ||
            db_err_hand_param == ReadDir2AndFile2ExpectNotFound ||
                db_err_hand_param == ReadDir2AndFile2ExpectFileNotFound) {

        // Open 1st level dir, find 2nd level dir

        VSSI_OpenDir2(handle, dirname1, &dir,
                      &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "VSSI_OpenDir2 %s returns %d.", dirname1, rc);
            rv++;
            goto exit;
        }

        found = false;
        while ( (dirent = VSSI_ReadDir2(dir)) != NULL ) {
            if (strncmp(dirent->name, dirname2Name, sizeof dirname2Name) == 0) {
                found = true;
                VPLTRACE_LOG_ERR(TRACE_APP, 0,
                                    "VSSI_ReadDir2 found dirent name %s.",
                                    dirent->name);
                break;
            }
            else {
                VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                                    "VSSI_ReadDir2 dirent name %s.",
                                    dirent->name);
            }
        }

        if ( (db_err_hand_param == ReadDir2AndFile2ExpectSuccess ||
                db_err_hand_param == ReadDir2AndFile2ExpectFileNotFound) && !found ) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                "%s not found using VSSI_ReadDir2.", dirname2);
            rv++;
        }

        if ( db_err_hand_param == ReadDir2AndFile2ExpectNotFound && found ) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                "found %s using VSSI_ReadDir2, expected not found.", dirname2);
            rv++;
        }

        VSSI_CloseDir2(dir);
        dir = NULL;

        // Open 2nd level dir

        VSSI_OpenDir2(handle, dirname2, &dir,
                      &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if ( db_err_hand_param == ReadDir2AndFile2ExpectSuccess ||
                db_err_hand_param == ReadDir2AndFile2ExpectFileNotFound) {
            if(rc != VSSI_SUCCESS) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "VSSI_OpenDir2 %s returns %d.", dirname2, rc);
                rv++;
            }
        }
        else if(rc != VSSI_NOTFOUND) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "VSSI_OpenDir2 %s returns %d expected not found.", dirname2, rc);
            rv++;
        }

        if (rc == VSSI_SUCCESS) {
            VSSI_CloseDir2(dir);
            dir = NULL;
        }

        if (rv) {
            goto exit;
        }
    }

    if ( db_err_hand_param == ReadFileExpectSuccess ||
         db_err_hand_param == ReadFileExpectNotFound ||
         db_err_hand_param == ReadFileExpectCCDStopped ||
         db_err_hand_param == ExpectDBCorruptionOnfsck ||
         db_err_hand_param == ReadFileExpectSuccessButFileLarger ||
         db_err_hand_param == ReadFileExpectSuccessButFileSmaller ||
         db_err_hand_param == ReadDir2AndFile2ExpectSuccess ||
         db_err_hand_param == ReadDir2AndFile2ExpectNotFound  ||
         db_err_hand_param == ReadDir2AndFile2ExpectFileNotFound) {

        // OpenFile
        flags = VSSI_FILE_OPEN_READ;
        flags |= VSSI_FILE_SHARE_READ|VSSI_FILE_SHARE_WRITE;
        attrs = 0;

        VSSI_OpenFile(handle, name, flags, attrs, &fileHandle1,
                      &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;

        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "db_err_hand_param is %d    VSSI_OpenFile returned %d.",
                        db_err_hand_param, rc);

        if ( db_err_hand_param == ReadFileExpectCCDStopped ||
             db_err_hand_param == ExpectDBCorruptionOnfsck) {
            if (rc != VSSI_COMM && rc != VSSI_NOTFOUND) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "VSSI_OpenFile for read %s expected VSSI_COMM or VSSI_NOTFOUND, returns %d.",
                                 name, rc);
                rv++;
                if(rc == VSSI_SUCCESS) {
                    goto close_file;
                }
            }
            goto exit;
        }
        else if ( db_err_hand_param == ReadFileExpectNotFound ||
                    db_err_hand_param == ReadDir2AndFile2ExpectFileNotFound) {
            if (rc != VSSI_NOTFOUND) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "VSSI_OpenFile for read %s expected VSSI_NOTFOUND, returns %d.",
                                 name, rc);
                rv++;
                if(rc == VSSI_SUCCESS) {
                    goto close_file;
                }
            }
            goto exit;
        }
        else if ( db_err_hand_param == ReadDir2AndFile2ExpectNotFound) {
            if (rc != VSSI_BADPATH) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "VSSI_OpenFile for read %s expected VSSI_BADPATH, returns %d.",
                                 name, rc);
                rv++;
                if(rc == VSSI_SUCCESS) {
                    goto close_file;
                }
            }
            goto exit;
        }
        else if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "VSSI_OpenFile for read %s returns %d.", name, rc);
            rv++;
            goto exit;
        }
        else {
            VSSI_OpenFile(handle, zeroSizeFileName, flags, attrs, &fileHandle2,
                          &test_context, vscs_test_callback);
            VPLSem_Wait(&(test_context.sem));
            rc = test_context.rv;
            if(rc != VSSI_SUCCESS) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "VSSI_OpenFile for read %s returns %d.", zeroSizeFileName, rc);
                rv++;
                goto exit;
            }
        }
    }

    if (db_err_hand_param == SetupForDBFragmentationTest) {
        int m = 0;
        int n = 0;
        int k = 0;
        static char dirname[50];
        static const char* us = "_";
        static const char* nc = "\0";
        bool isDir = true;
                
        VPLMath_InitRand();

        for (m = 0; m < num_top_level_trees; ++m) {
            for (n = 0; n < num_subtrees; ++n) {
                VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "m = %d n = %d", m, n);

                memcpy (dirname, dirname_base, strlen(dirname_base));
                VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "only copy dirname base - dirname base is :"
                             "%s", dirname);

                sprintf (dirname+strlen(dirname_base), "%d%s%d%s", m, us, n, nc);
                VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "dirname is :"
                             "%s", dirname);

                // Create the directory [m,n] for the test
                attrs = 0;
                VSSI_MkDir2(handle, dirname, attrs,
                      &test_context, vscs_test_callback);
                VPLSem_Wait(&(test_context.sem));
                rc = test_context.rv;
                if(rc != VSSI_SUCCESS) {
                    VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_MkDir2 %s returns %d.", dirname, rc);
                    rv++;
                } else {
                    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, 
                         "VSSI_MkDir2 %s returns %d.", dirname, rc);
                }

                // Create a directory tree[m,n]
                rc = create_dir_tree(test_context, handle, dirname, num_subtree_levels, num_subtree_subdirs, num_subtree_files);
                if(rc != 0) {
                    VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                     "Error creating directory tree (%d %d)", m, n);
                    rv++;
                    goto exit;
                }
            }
        }
        // Delete a random number of subtrees (subtrees_to_remove) 
        for (k = 0; k < subtrees_to_remove; ++k) {
            while (1) {
                m = VPLMath_Rand() % num_top_level_trees; 
                //m = rand() % num_top_level_trees; 
                n = VPLMath_Rand() % num_subtrees; 
                //n = rand() % num_subtrees; 
                VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,"Random numbers are m = %d n = %d.", m,n);
                // delete subtree
                memcpy (dirname, dirname_base, strlen(dirname_base));
//                VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "only copy dirname base - dirname base is :"
//                             "%s", dirname);

                sprintf (dirname+strlen(dirname_base), "%d%s%d%s", m, us, n, nc);
//                VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "dirname is :"
//                             "%s", dirname);

                rc = vss_isFileOrDir(test_context, handle, dirname, &isDir, 0);
                if (rc == VSSI_SUCCESS) {
                    rc = traverse_dir_tree(test_context, handle, dirname, num_subtree_levels, num_subtree_subdirs, num_subtree_files, true);
                    if(rc != 0) {
                        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                     "Error traversing directory tree (%d %d)", m, n);
                        rv++;
                        goto exit;
                    }
                    // The remove of the file should succeed this time
                    VSSI_Remove(handle, dirname,
                        &test_context, vscs_test_callback);
                    VPLSem_Wait(&(test_context.sem));
                    rc = test_context.rv;
                    if(rc != VSSI_SUCCESS) {
                        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_Remove component %s: %d.",
                         dirname, rc);
                        rv++;
                    }

                    break;
                }
            }
        }

    } else if (db_err_hand_param == TestDBFragmentationEffects) {

        int m = 0;
        int n = 0;
        int i = 0;
        int s = 0;
        int c = 0;
        int num_iter = 0;
        const int num_max_iter = 500;
        static char dirname[50];
        static const char* us = "_";
        static const char* nc = "\0";
        bool isDir = true;
        db_index open_slot[TEST_DB_MAX_REMOVED];

        rc = time_to_traverse(test_context, handle, dirname_base, num_top_level_trees, num_subtrees, num_subtree_levels, num_subtree_subdirs, num_subtree_files);
        if (rc != 0) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                "Error timing traversing directory tree. rc = %d", rc);
            rv++;
            goto exit;
        }
        
        for (num_iter = 0; num_iter < num_max_iter; ++num_iter) {
            VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,"num_iter = %d.", num_iter);
            s = 0;
            // Mark empty slots
            for (m = 0; m < num_top_level_trees; ++m) {
                for (n = 0; n < num_subtrees; ++n) {

                    memcpy (dirname, dirname_base, strlen(dirname_base));
                    sprintf (dirname+strlen(dirname_base), "%d%s%d%s", m, us, n, nc);

                    rc = vss_isFileOrDir(test_context, handle, dirname, &isDir, 0);
                    if (rc != VSSI_SUCCESS) {
                        open_slot[s].top_tree = m;
                        open_slot[s].sub_tree = n;
                        s++;
                    }
                }
            }
 
            // Delete a random number of subtrees (num_deletions) 
            for (i = 0; i < num_deletions; ++i) {
                while (1) {
                    m = VPLMath_Rand() % num_top_level_trees; 
                    n = VPLMath_Rand() % num_subtrees; 
                    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,"Random numbers are m = %d n = %d.", m,n);
                
                    // delete subtree if it exists
                    memcpy (dirname, dirname_base, strlen(dirname_base));
                    sprintf (dirname+strlen(dirname_base), "%d%s%d%s", m, us, n, nc);

                    rc = vss_isFileOrDir(test_context, handle, dirname, &isDir, 0);
                    if (rc == VSSI_SUCCESS) {
                        rc = traverse_dir_tree(test_context, handle, dirname, num_subtree_levels, num_subtree_subdirs, num_subtree_files, true);
                        if(rc != 0) {
                            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                     "Error traversing directory tree (%d %d)", m, n);
                            rv++;
                            goto exit;
                        }
                        // Remove the top level directory
                        VSSI_Remove(handle, dirname,
                            &test_context, vscs_test_callback);
                        VPLSem_Wait(&(test_context.sem));
                        rc = test_context.rv;
                        if(rc != VSSI_SUCCESS) {
                            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "VSSI_Remove component %s: %d.",
                             dirname, rc);
                            rv++;
                        }

                        open_slot[s].top_tree = m;
                        open_slot[s].sub_tree = n;
                        s++;
                        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "Removed directory tree (%d %d)", m, n);
                        break;
                    }
                }
            }
            // Create the same number of subtrees (num_deletions) 
            for (i = 0; i < num_deletions; ++i) {
                VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,"s = %d.", s);
                while (1) {
                    c = VPLMath_Rand() % s; 

                    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,"Random number is c = %d.", c);
                    
                    memcpy (dirname, dirname_base, strlen(dirname_base));
                    sprintf (dirname+strlen(dirname_base), "%d%s%d%s", open_slot[c].top_tree, us, open_slot[c].sub_tree, nc);

                    // Create the directory [m,n] for the test
                    attrs = 0;
                    VSSI_MkDir2(handle, dirname, attrs,
                          &test_context, vscs_test_callback);
                    VPLSem_Wait(&(test_context.sem));
                    rc = test_context.rv;
                    if(rc != VSSI_SUCCESS) {
                        if (rc == VSSI_ISDIR) {
                            VPLTRACE_LOG_ERR(TRACE_APP, 0, "VSSI_MkDir2 %s returns %d. Continuing..", dirname, rc);
                            continue;
                        }
                        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "VSSI_MkDir2 %s returns %d.", dirname, rc);
                        rv++;
                        goto exit;
                    } else {
                        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, 
                             "VSSI_MkDir2 %s returns %d.", dirname, rc);
                    }

                    // Create a directory tree[m,n]
                    rc = create_dir_tree(test_context, handle, dirname, num_subtree_levels, num_subtree_subdirs, num_subtree_files);
                    if(rc != 0) {
                        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                         "Error creating directory tree (%d %d)", m, n);
                        rv++;
                        goto exit;
                    }
                    break;
                }
            }

            rc = time_to_traverse(test_context, handle, dirname_base, num_top_level_trees, num_subtrees, num_subtree_levels, num_subtree_subdirs, num_subtree_files);
            if (rc != 0) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                    "Error timing traversing directory tree.");
                rv++;
                goto exit;
            }
        
        }

    } else {

        // before read set myReadBuf to an unexpected pattern
        for (i = 0; i < sizeof(myReadBuf); ++i) {
            myReadBuf[i] = 256 - 1 - (i % 256);
        }

        expectedStatSize = expectedReadLen = sizeof(myBuf);
        expectedZeroSizeFileSize = 0;

        if (db_err_hand_param == ReadFileExpectSuccessButFileLarger) {
            expectedReadLen = sizeof(myReadBuf);
            expectedStatSize = expectedZeroSizeFileSize = 2097152;
        }
        else if (db_err_hand_param == ReadFileExpectSuccessButFileSmaller) {
            expectedStatSize = expectedReadLen = 128;
        }

        // check size returned by VSSI_Stat2()

        VSSI_Stat2(handle, name, &stats,
              &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Stat2 call for %s failed: %d.",
                         name, rc);
            rv++;
        }
        else if (stats->size != expectedStatSize) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0,
                       "File %s is "FMTu64" bytes, expected "FMTu64"",
                       name, stats->size, expectedStatSize);
            rv++;
        }
        else {
            VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                       "File %s is "FMTu64" bytes",
                       name, stats->size);
        }

        if(stats != NULL) {
            free(stats);
            stats = NULL;
        }

        VSSI_Stat2(handle, zeroSizeFileName, &stats,
                  &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Stat2 call for %s failed: %d.",
                         zeroSizeFileName, rc);
            rv++;
        }
        else if (stats->size != expectedZeroSizeFileSize) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0,
                       "File %s is "FMTu64" bytes, expected "FMTu64"",
                       zeroSizeFileName, stats->size, expectedZeroSizeFileSize);
            rv++;
        }
        else {
            VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                       "File %s is "FMTu64" bytes",
                       zeroSizeFileName, stats->size);
        }

        if(stats != NULL) {
            free(stats);
            stats = NULL;
        }

        // check that read expected size and content

        rdLen = sizeof(myReadBuf);
        VSSI_ReadFile(fileHandle1, 0, &rdLen, myReadBuf,
                            &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                            "VSSI_ReadFile returned %d.", rc);
            rv++;
        }
        else if(rdLen != expectedReadLen) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                            "VSSI_ReadFile %s returned %u bytes, expected %u.",
                            name, rdLen, expectedReadLen);
            rv++;
        }
        else {
            for (i = 0; i < rdLen; ++i) {
                if ((u8)myReadBuf[i] != i % 256) {
                    VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                    "VSSI_ReadFile %s myReadBuf[%u] expected 0x%02x got 0x%02x.",
                                    name, i, (i % 256), (u8)myReadBuf[i]);
                    rv++;
                    break;
                }
            }
            if (i == rdLen) {
                VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                           "Successfully read and checked %u bytes of %s",
                           rdLen, name);
            }
        }
    }

close_file:
    if (fileHandle1) {
        VSSI_CloseFile(fileHandle1,
                       &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "VSSI_CloseFile returns %d.", rc);
            rv++;
        }
    }

    if (fileHandle2) {
        VSSI_CloseFile(fileHandle2,
                       &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "VSSI_CloseFile returns %d.", rc);
            rv++;
        }
    }

exit:
    // Close the VSS Object
    VSSI_CloseObject(handle,
                        &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;

    if(rc == VSSI_COMM) {
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                         "VSSI_COMM on close object %s",
                         save_description);
    }
    else if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Failed to close object %s: %d",
                         save_description, rc);
        rv++;
    }

 fail_open:
    VPLSem_Destroy(&(test_context.sem));
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                   "Test %s result: %d.",
                   vsTest_curTestName, rv);

    return rv;
}


#ifdef NEEDS_CONVERSION
static const char vsTest_test_vssi_content_access[] = "VSSI Content Access Test";
int test_vssi_content_access(const std::string& location,
                             bool longTest)
{
    int rv = 0;
    u32 blocks = longTest ? CONTENT_LONG_TESTBLOCKS : CONTENT_SHORT_TESTBLOCKS;
    int rc;
    VSSI_Object handle;
    vscs_test_context_t test_context;
    VPL_SET_UNINITIALIZED(&(test_context.sem));

    vsTest_curTestName = vsTest_test_vssi_content_access;
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                   "Starting test: %s.",
                   vsTest_curTestName);

    // Testing libbvs and VSCS download of a content object.
    // TODO; Verify CFM (TOHX) file signature against TMD using eTicket
    // TODO: Verify data blocks (BLKS) againt signatures.

    if(vssi_session == 0) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                    "No VSSI session registered.");
        return rv+1;
    }

    if(VPLSem_Init(&(test_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "Failed to create semaphore.");
        return rv+1;
    }

    VSSI_OpenObject(vssi_session, location.c_str(),
                    VSSI_READONLY, &handle,
                    &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Open object %s failed: %d.",
                         location.c_str(), rc);
        rv++;
        goto fail_open;
    }

    // Check the version (should be 0)
    if(VSSI_GetVersion(handle) != 0) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Content object version is "FMTu64". Should be 0.",
                         VSSI_GetVersion(handle));
        rv++;
    }
    else {
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                       "Content object version is "FMTu64".",
                       VSSI_GetVersion(handle));
    }

    // Read the object metadata file. Ignore the contents.
    rv += test_vscs_read_and_verify_async(handle, BVD_CFM, NULL,
                                          1, blocks, CONTENT_BLOCKSIZE);

    // Read the object data. Ignore the contents.
    rv += test_vscs_read_and_verify_async(handle, BVD_BLOCK_DATA, NULL,
                                          32, blocks, CONTENT_BLOCKSIZE);

    VSSI_CloseObject(handle,
                        &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                    "Failed to close object %s: %d",
                    location.c_str(), rc);
        rv++;
    }

 fail_open:
    VPLSem_Destroy(&(test_context.sem));

    return rv;
}
#endif // NEEDS_CONVERSION
static const char vsTest_network_down[] = "VSSI Network Down Test";
static int test_network_down(VSSI_Session session,
                      const char* save_description,
                      u64 user_id,
                      u64 dataset_id,
                      const VSSI_RouteInfo& route_info,
                      bool use_xml_api)
{
    int rv = 0;
    int rc = 0;
    VSSI_Object handle;
    VSSI_File file_handle;
    vscs_test_context_t test_context;
    char buf[32] = "BeforeNetworkDown";
    char buf2[32] = "AfterNetworkDown";

    VPL_SET_UNINITIALIZED(&(test_context.sem));

    vsTest_curTestName = vsTest_network_down;
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Starting test: %s.",
                        vsTest_curTestName);

    if(VPLSem_Init(&(test_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0,
                    "Failed to create semaphore.");
        return rv++;
    }
    
    // Set initial conditions: delete object contents.
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "DeleteObject");
    if ( use_xml_api ) {
        VSSI_Delete(session, save_description,
                    &test_context, vscs_test_callback);
    }
    else {
        VSSI_Delete2(session, user_id, dataset_id, &route_info,
                     &test_context, vscs_test_callback);
    }
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Delete object %s failed: %d.",
                         save_description, rc);
        rv++;
        goto fail_open;
    }

    // Open dataset
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "OpenObject");
    if ( use_xml_api ) {
        VSSI_OpenObject(session, save_description,
                        VSSI_READWRITE, &handle,
                        &test_context, vscs_test_callback);
    }
    else {
        VSSI_OpenObject2(session, user_id, dataset_id, &route_info,
                         VSSI_READWRITE | VSSI_FORCE, &handle,
                         &test_context, vscs_test_callback);
    }
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                    "Open object %s failed: %d.",
                    save_description, rc);
        rv++;
        goto fail_open;
    }

    //do open file
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "OpenFile");
    {
        VSSI_OpenFile(handle, "network_done_file", 
                      VSSI_FILE_OPEN_WRITE |
                      VSSI_FILE_OPEN_READ |
                      VSSI_FILE_OPEN_CREATE |
                      VSSI_FILE_SHARE_READ |
                      VSSI_FILE_SHARE_WRITE, 
                      0,
                      &file_handle,
                      &test_context,
                      vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "OpenFile result %d.", rc);
    if(rc != VSSI_SUCCESS && rc != VSSI_EXISTS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "OpenFile result %d.", rc);
        rv++;
        goto fail;
    }

    //do write
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "WriteFile");
    {
        u32 length = 32;
        VSSI_WriteFile(file_handle,
                       0, &length, buf,
                       &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "WriteFile result %d.", rc);
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "WriteFile result %d.", rc);
        rv++;
        goto fail;
    }

    //do read
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "ReadFile");
    {
        char tmp_buf[32] = {0};
        u32 length = 32;
        VSSI_ReadFile(file_handle,
                      0, &length, tmp_buf,
                      &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(strcmp(tmp_buf, buf) != 0) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "ReadFile read data is mismach!");
            rv++;
            goto fail;
        }
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,"read buffer content RC: %d buf: %s.", rc, tmp_buf);
    }
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "ReadFile result %d.", rc);
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "ReadFile result %d.", rc);
        rv++;
        goto fail;
    }


    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Invoke VSSI_NetworkDown()");
    VSSI_NetworkDown();

    //do write
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "WriteFile");
    {
        u32 length = 32;
        VSSI_WriteFile(file_handle,
                       0, &length, buf2,
                       &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "WriteFile result %d.", rc);
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "WriteFile result %d.", rc);
        rv++;
        goto fail;
    }

    //do read
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "ReadFile");
    {
        char tmp_buf[32] = {0};
        u32 length = 32;
        VSSI_ReadFile(file_handle,
                      0, &length, tmp_buf,
                      &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(strcmp(tmp_buf, buf2) != 0) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "ReadFile read data is mismach!");
            rv++;
            goto fail;
        }
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,"read buffer content RC: %d buf: %s.", rc, tmp_buf);
    }
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "ReadFile result %d.", rc);
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "ReadFile result %d.", rc);
        rv++;
        goto fail;
    }

    //do close file
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "CloseFile");
    {
        VSSI_CloseFile(file_handle, &test_context, vscs_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "CloseFile result %d.", rc);
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "CloseFile result %d.", rc);
        rv++;
        goto fail;
    }


 fail:
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Close object ");
    VSSI_CloseObject(handle, &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Close object %s result %d.",
                         save_description, rc);
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Close object %s failed: %d.",
                         save_description, rc);
        rv++;
    }

 fail_open:
    VPLSem_Destroy(&(test_context.sem));
    return rv;
}

static const char vsTest_test_vssi_dataset_access[] = "VSSI Dataset Access Test";
int test_vssi_dataset_access(const std::string& location,
                             u64 user_id,
                             u64 dataset_id,
                             const VSSI_RouteInfo& route_info,
                             bool use_xml_api,
                             bool run_cloudnode_tests,
                             bool run_async_client_object_test,
                             bool run_file_access_and_share_modes_test,
                             bool run_many_file_open_test,
                             bool run_case_insensitivity_test)
{
    int rv = 0;
    vsTest_curTestName = vsTest_test_vssi_dataset_access;
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                   "Starting test: %s.",
                   vsTest_curTestName);

    rv += test_personal_cloud(vssi_session, location.c_str(), user_id,
                              dataset_id, route_info, use_xml_api, run_cloudnode_tests);
    if(rv) goto exit;

#ifdef NEEDS_CONVERSION
    rv += test_vscs_access_save_bvd_async(vssi_session, location.c_str(), user_id,
                                          dataset_id, route_info, use_xml_api);
    if(rv) goto exit;
#endif // NEEDS_CONVERSION

#if 0 // Can't predict outcome.
    rv += test_vscs_premature_close_on_commit(vssi_session, location.c_str(), user_id,
                                              dataset_id, route_info, use_xml_api);
    if(rv) goto exit;
#endif

    rv += test_vss_user_access_control(vssi_session, location.c_str(), user_id,
                                       dataset_id, route_info, use_xml_api);
    if(rv) goto exit;

    rv += test_async_notification_events(vssi_session, location.c_str(), user_id,
                                         dataset_id, route_info, use_xml_api, 
                                         run_async_client_object_test);
    if(rv) goto exit;

    rv += test_vss_file_handles(vssi_session, location.c_str(), user_id,
                                dataset_id, route_info, use_xml_api, false);
    if(rv) goto exit;

    rv += test_vss_file_handles(vssi_session, location.c_str(), user_id,
                                dataset_id, route_info, use_xml_api, true);
    if(rv) goto exit;

    rv += test_vss_file_rename_remove(vssi_session, location.c_str(), user_id,
                                      dataset_id, route_info, use_xml_api);
    if(rv) goto exit;

    rv += test_vss_file_remove_shared(vssi_session, location.c_str(), user_id,
                                      dataset_id, route_info, use_xml_api);
    if(rv) goto exit;

    rv += test_vss_file_create(vssi_session, location.c_str(), user_id,
                               dataset_id, route_info, use_xml_api);
    if(rv) goto exit;

    rv += test_vss_file_rename_attributes(vssi_session, location.c_str(), user_id,
                               dataset_id, route_info, use_xml_api);
    if(rv) goto exit;

    rv += test_vss_file_modify_time(vssi_session, location.c_str(), user_id,
                               dataset_id, route_info, use_xml_api);
    if(rv) goto exit;

    // Use VSSI1 CHMOD API
    rv += test_vss_file_attributes(vssi_session, location.c_str(), user_id,
                                   dataset_id, false, route_info, use_xml_api);
    if(rv) goto exit;

    // Use VSSI2 CHMOD API
    rv += test_vss_file_attributes(vssi_session, location.c_str(), user_id,
                                   dataset_id, true, route_info, use_xml_api);

    rv += test_vss_mkdir_parents_and_existing(vssi_session, location.c_str(), user_id,
                                              dataset_id, route_info, use_xml_api);
    if(rv) goto exit;

    if (run_case_insensitivity_test) {
        rv += test_vss_file_case_insensitivity(vssi_session, location.c_str(), user_id,
                                               dataset_id, route_info, use_xml_api);
        if(rv) goto exit;
    }

    if (run_file_access_and_share_modes_test) {
        rv += test_vss_file_access_and_share_modes(vssi_session, location.c_str(), user_id,
                                                   dataset_id, route_info, use_xml_api);
        if(rv) goto exit;
    }

    if (run_many_file_open_test) {
        rv += test_vss_file_open_many(vssi_session, location.c_str(), user_id,
                                                   dataset_id, route_info, use_xml_api);
        if(rv) goto exit;
    }

    rv += test_vss_file_oplocks(vssi_session, location.c_str(), user_id,
                                dataset_id, route_info, use_xml_api);
    if(rv) goto exit;

    rv += test_vss_file_oplocks_brlocks(vssi_session, location.c_str(), user_id,
                                dataset_id, route_info, use_xml_api);
    if(rv) goto exit;

    rv += test_vss_object_teardown(vssi_session, location.c_str(), user_id,
                                dataset_id, route_info, use_xml_api);
    if(rv) goto exit;

    rv += test_network_down(vssi_session, location.c_str(), user_id,
                            dataset_id, route_info, use_xml_api);
    if(rv) goto exit;

#ifdef TEST_BAD_OBJHANDLE
    // Manual test cases (require user intervention at server)
    rv += test_vss_bad_objhandle(vssi_session, location.c_str(), user_id,
                                 dataset_id, route_info, use_xml_api);
#endif

 exit:
    return rv;
}
