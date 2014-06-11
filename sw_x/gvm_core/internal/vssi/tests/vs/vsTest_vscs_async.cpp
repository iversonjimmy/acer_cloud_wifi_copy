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

#include <ctype.h>
#include <string>

#include "vpl_time.h"
#include "vpl_th.h"
#include "vplex_assert.h"
#include "vplex_trace.h"
#include "vplex_user.h"

#include "vssi.h"
#include "vssi_error.h"
//#include "vss_comm.h"

#include "vsTest_vscs_common.hpp"
#include "vsTest_vscs_async.hpp"
#include "vsTest_personal_cloud_data.hpp"
#include "vsTest_personal_cloud_http.hpp"
#include "vsTest_camera_roll.hpp"

using namespace std;

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
                   "Read back uncommitted changed block. Confirm it is not changed.");
    filedata = '2';
    rv += test_vscs_read_and_verify_async(handle, BVD_BLOCK_DATA,
                                          &filedata, 1, 1,
                                          SAVE_BLOCKSIZE);
    if(rv > 0) goto fail;

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                   "Close and reopen. Confirm version not changed, data not changed. Close again.");
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
                   "Read first block. Confirm it's still all '2'");
    filedata = '2';
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

#if 0
static const char vsTest_vscs_premature_close_on_commit [] = "VSCS Close While Committing Test";
static int test_vscs_premature_close_on_commit(VSSI_Session session,
                                               const char* save_description,
                                               u64 user_id,
                                               u64 dataset_id,
                                               const VSSI_RouteInfo& route_info,
                                               bool use_xml_api)
{
    int rv = 0;
    int rc;
    VSSI_Object handle;
    vscs_test_context_t test_context[2];
    char filedata;
    u64 version;
    int i;

    for(i = 0; i < 2; i++) {
        VPL_SET_UNINITIALIZED(&(test_context[i].sem));
    }

    vsTest_curTestName = vsTest_vscs_premature_close_on_commit;
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Starting test: %s.",
                        vsTest_curTestName);

    // REQUIRES: valid save_description and registered session
    if(save_description == NULL) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                    "No save object description. Aborting test.");
        return rv+1;
    }
    if(vssi_session == 0) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                    "No VSSI session registered.");
        return rv+1;
    }

    for(i = 0; i < 2; i++) {
        if(VPLSem_Init(&(test_context[i].sem), 1, 0) != VPL_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Failed to create semaphore.");
            return rv+1;
        }
    }

    if ( use_xml_api ) {
        VSSI_OpenObject(vssi_session, save_description,
                        VSSI_READWRITE, &handle,
                        &test_context[0], vscs_test_callback);
    }
    else {
        VSSI_OpenObject2(vssi_session, user_id, dataset_id, &route_info,
                         VSSI_READWRITE, &handle,
                         &test_context[0], vscs_test_callback);
    }
    VPLSem_Wait(&(test_context[0].sem));
    rc = test_context[0].rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                    "Open object %s failed: %d.",
                    save_description, rc);
        rv++;
        goto fail_open;
    }
    version = VSSI_GetVersion(handle);

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                   "Write data to the save game (number of 32k blocks)");
    filedata = 'A';
    rv += test_vscs_write_object_async(handle, &filedata, 32, SAVE_TESTBLOCKS,
                                       SAVE_BLOCKSIZE);

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                   "Commit the data. Close while committing.");
    VSSI_Commit(handle, &test_context, vscs_test_callback);

    VPLThread_Sleep(100);

    VSSI_CloseObject(handle,
                        &test_context[1], vscs_test_callback);
    VPLSem_Wait(&(test_context[1].sem));
    rc = test_context[1].rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Close object %s failed: %d.",
                         save_description, rc);
        rv++;
    }

    VPLSem_Wait(&(test_context[0].sem));
    rc = test_context[0].rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Failed to commit object %s: %d",
                         save_description, rc);
        rv++;
    }

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Reopen object to check data, server condition.");
    if ( use_xml_api ) {
        VSSI_OpenObject(vssi_session, save_description,
                        VSSI_READWRITE, &handle,
                        &test_context[0], vscs_test_callback);
    }
    else {
        VSSI_OpenObject2(vssi_session, user_id, dataset_id, &route_info,
                         VSSI_READWRITE, &handle,
                         &test_context[0], vscs_test_callback);
    }
    VPLSem_Wait(&(test_context[0].sem));
    rc = test_context[0].rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Open object %s failed: %d.",
                         save_description, rc);
        rv++;
        goto fail_open;
    }

    if(VSSI_GetVersion(handle) != version + 1) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Object version after commit is "FMTu64". Expected "FMTu64".",
                         VSSI_GetVersion(handle), version + 1);
        rv++;
    }

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Read one block to confirm server has sync'd save data.");
    rv += test_vscs_read_and_verify_async(handle, BVD_BLOCK_DATA,
                                          &filedata, 1, 1,
                                          SAVE_BLOCKSIZE);

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                   "Verify object data.");
    rv += test_vscs_read_and_verify_async(handle, BVD_BLOCK_DATA,
                                          &filedata, 32, SAVE_TESTBLOCKS,
                                          SAVE_BLOCKSIZE);

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Close the object.");
    VSSI_CloseObject(handle, &test_context[0], vscs_test_callback);
    VPLSem_Wait(&(test_context[0].sem));
    rc = test_context[0].rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Close object %s failed: %d.",
                         save_description, rc);
        rv++;
    }

 fail_open:
    for(i = 0; i < 2; i++) {
        VPLSem_Destroy(&(test_context[i].sem));
    }
    return rv;

}
#endif

static const char vsTest_vss_user_access_control[] = "VSS user Access Control Test";
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
int test_vss_session_recognition(const vplex::vsDirectory::SessionInfo& loginSession,
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

    serviceTicket = loginSession.serviceticket();
    sessionHandle = loginSession.sessionhandle();
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

    serviceTicket = loginSession.serviceticket();
    sessionHandle = loginSession.sessionhandle();
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

    // Re-establish the legit session.
    vssi_session = VSSI_RegisterSession(loginSession.sessionhandle(),
                                        loginSession.serviceticket().c_str());

    return rv;
}

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

static const char vsTest_test_vssi_dataset_access[] = "VSSI Dataset Access Test";
int test_vssi_dataset_access(const std::string& location,
                             u64 user_id,
                             u64 dataset_id,
                             const VSSI_RouteInfo& route_info,
                             bool use_xml_api,
                             vplex::vsDirectory::DatasetType type)
{
    int rv = 0;

    vsTest_curTestName = vsTest_test_vssi_dataset_access;
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                   "Starting test: %s.",
                   vsTest_curTestName);

    rv += test_personal_cloud(vssi_session, location.c_str(), user_id,
                              dataset_id, route_info, use_xml_api, type);
    if(rv) goto exit;

    rv += test_vscs_access_save_bvd_async(vssi_session, location.c_str(), user_id,
                                          dataset_id, route_info, use_xml_api);
    if(rv) goto exit;

#if 0 // Can't predict outcome.
    rv += test_vscs_premature_close_on_commit(vssi_session, location.c_str(), user_id,
                                              dataset_id, route_info, use_xml_api);
    if(rv) goto exit;
#endif

    rv += test_vss_user_access_control(vssi_session, location.c_str(), user_id,
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

static const char vsTest_test_camera_roll[] = "VSSI CameraRoll Test";
int test_camera_roll(const std::string& upload_location,
                     const std::string& download_location,
                     u64 user_id,
                     u64 upload_dataset_id,
                     u64 download_dataset_id,
                     const VSSI_RouteInfo& route_info,
                     bool use_xml_api)
{
    int rv = 0;

    vsTest_curTestName = vsTest_test_camera_roll;
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Starting test: %s.",
                        vsTest_curTestName);

    rv += test_camera_roll_feature(vssi_session,
                                   upload_location.c_str(),
                                   download_location.c_str(),
                                   user_id, upload_dataset_id, download_dataset_id,
                                   route_info, use_xml_api);
    if(rv) goto exit;

 exit:
    return rv;
}
