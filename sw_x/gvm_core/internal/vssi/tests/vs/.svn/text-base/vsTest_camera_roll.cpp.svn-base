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

#include "vsTest_camera_roll.hpp"

#include "vplex_trace.h"
#include "vpl_th.h"
#include "vssi_error.h"

#include <string>

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>


#define CR_TEST_FILE 0 // DO NOT CHECK IN if 1.

extern const char* vsTest_curTestName;

// callback and context to make async commands synchronous for testing.
typedef struct  {
    VPLSem_t sem;
    int rv;
} crtest_context_t;
static void crtest_callback(void* ctx, VSSI_Result rv)
{
    crtest_context_t* context = (crtest_context_t*)ctx;

    context->rv = rv;
    VPLSem_Post(&(context->sem));
}
static crtest_context_t test_context;

// Upload a file into a dataset.
// @param handle Handle to open dataset
// @param name Name of file to upload
// @param data Data to upload.
static int camera_roll_upload(VSSI_Object handle,
                              const std::string& name,
                              const std::string& data)
{
    u32 length = data.size();

    VPLTRACE_LOG_FINE(TRACE_APP, 0,
                      "Writing file %s.",
                      name.c_str());

    // TODO: Write in chunks for larger files.
    VSSI_Write(handle, name.c_str(), 0, &length, data.data(),
               &test_context, crtest_callback);
    VPLSem_Wait(&(test_context.sem));
    return test_context.rv;
}

#if CR_TEST_FILE
static int camera_roll_upload_file(VSSI_Object handle,
                                   const std::string& component,
                                   const std::string& filename)
{
    // Open file and upload to CameraRoll dataset.
    int fd = -1;
    int rv = -1;
    char* buf = NULL;
    u32 length;
    u64 offset = 0;

    fd = open(filename.c_str(), O_RDONLY);
    if(fd == -1) {
        goto exit;
    }

    buf = (char*)malloc(32*1024);
    if(buf == NULL) {
        goto exit;
    }

    do {
        rv = read(fd, buf, 32*1024);
        if(rv <= 0) {
            goto exit;
        }
        length = rv;
        VSSI_Write(handle, component.c_str(), offset, &length, buf,
                   &test_context, crtest_callback);
        VPLSem_Wait(&(test_context.sem));
        offset += length;

        rv = test_context.rv;
    } while(rv == VSSI_SUCCESS);

 exit:
    if(fd != -1) {
        close(fd);
    }
    if(buf != NULL) {
        free(buf);
    }

    return rv;
}
#endif // CR_TEST_FILE

static int camera_roll_commit(VSSI_Object handle)
{
    VPLTRACE_LOG_FINE(TRACE_APP, 0,
                      "Committing changes.");

    VSSI_Commit(handle, &test_context, crtest_callback);
    VPLSem_Wait(&(test_context.sem));
    return test_context.rv;
}

int test_camera_roll_feature(VSSI_Session session,
                             const char* upload_location,
                             const char* download_location,
                             u64 user_id,
                             u64 upload_dataset_id,
                             u64 download_dataset_id,
                             const VSSI_RouteInfo& route_info,
                             bool use_xml_api)
{
    int rv = 0;
    int rc = 0;
    VSSI_Object handles[2];

    VPL_SET_UNINITIALIZED(&(test_context.sem));

    if(VPLSem_Init(&(test_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0,
                         "Failed to create sempahore.");
        return -1;
    }

    // Clear both datasets for initial conditions.
    if ( use_xml_api ) {
        VSSI_Delete(session, upload_location,
                    &test_context, crtest_callback);
    }
    else {
        VSSI_Delete2(session, user_id, upload_dataset_id, &route_info,
                     &test_context,crtest_callback);
    }
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Delete object %s failed: %d.",
                         upload_location, rc);
        rv++;
        goto fail_open;
    }
    if ( use_xml_api ) {
        VSSI_Delete(session, download_location,
                    &test_context, crtest_callback);
    }
    else {
        VSSI_Delete2(session, user_id, download_dataset_id, &route_info,
                     &test_context, crtest_callback);
    }
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Delete object %s failed: %d.",
                         download_location, rc);
        rv++;
        goto fail_open;
    }

    // Open both datasets
    if ( use_xml_api ) {
        VSSI_OpenObject(session, upload_location,
                        VSSI_READWRITE, &handles[0],
                        &test_context, crtest_callback);
    }
    else {
        VSSI_OpenObject2(session, user_id, upload_dataset_id, &route_info,
                         VSSI_READWRITE, &handles[0],
                         &test_context, crtest_callback);
    }
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Open object %s: %d.",
                         upload_location, rc);
        rv++;
        goto fail_open;
    }
    if ( use_xml_api ) {
        VSSI_OpenObject(session, download_location,
                        VSSI_READWRITE, &handles[1],
                        &test_context, crtest_callback);
    }
    else {
        VSSI_OpenObject2(session, user_id, download_dataset_id, &route_info,
                         VSSI_READWRITE, &handles[1],
                         &test_context, crtest_callback);
    }
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Open object %s: %d.",
                         download_location, rc);
        rv++;
        goto fail_open;
    }

    // Write a file into the upload dataset.
    do {
        if(rc == VSSI_CONFLICT) {
            VSSI_ClearConflict(handles[0]);
        }

        rc = camera_roll_upload(handles[0],
                                 "test_file.dat",
                                 "This is a quick test1.");
        if(rc == VSSI_CONFLICT) { continue; }
        else if(rc != VSSI_SUCCESS) { rv++; goto fail; }
        rc = camera_roll_commit(handles[0]);
        if(rc == VSSI_CONFLICT) { continue; }
        else if(rc != VSSI_SUCCESS) { rv++; goto fail; }
    } while(rc == VSSI_CONFLICT);

    // Confirm file exists in download dataset.
    // Confirm upload dataset empty, no trash records.

    // Write multiple files into the upload dataset in one batch.
    // Confirm files exist in download dataset.
    // Confirm upload dataset empty, no trash records.
    do {
        if(rc == VSSI_CONFLICT) {
            VSSI_ClearConflict(handles[0]);
        }

        rc = camera_roll_upload(handles[0],
                                "device/test_file.dat",
                                "This is a quick test2.");
        if(rc == VSSI_CONFLICT) { continue; }
        else if(rc != VSSI_SUCCESS) { rv++; goto fail; }
        rc = camera_roll_upload(handles[0],
                                "device.bogusext/test_file.dat",
                                "This is a quick test2.");
        if(rc == VSSI_CONFLICT) { continue; }
        else if(rc != VSSI_SUCCESS) { rv++; goto fail; }
        rc = camera_roll_upload(handles[0],
                                "deeper/directory/tree/test_file.dat",
                                "This is a quick test3.");
        if(rc == VSSI_CONFLICT) { continue; }
        else if(rc != VSSI_SUCCESS) { rv++; goto fail; }
        rc = camera_roll_upload(handles[0],
                                "device/test_file2",
                                "This is a quick test4.");
        if(rc == VSSI_CONFLICT) { continue; }
        else if(rc != VSSI_SUCCESS) { rv++; goto fail; }
        rc = camera_roll_upload(handles[0],
                                "device.bogusext/test_file2",
                                "This is a quick test4.");
        if(rc == VSSI_CONFLICT) { continue; }
        else if(rc != VSSI_SUCCESS) { rv++; goto fail; }
        rc = camera_roll_upload(handles[0],
                                "device/test_file3",
                                "This is a quick test5.");
        if(rc == VSSI_CONFLICT) { continue; }
        else if(rc != VSSI_SUCCESS) { rv++; goto fail; }
        rc = camera_roll_upload(handles[0],
                                "device/test_file4.01020304.doc",
                                "This is a quick test6.");
        if(rc == VSSI_CONFLICT) { continue; }
        else if(rc != VSSI_SUCCESS) { rv++; goto fail; }
        rc = camera_roll_upload(handles[0],
                                "device/test_file5.0000.doc",
                                "This is a quick test7.");
        if(rc == VSSI_CONFLICT) { continue; }
        else if(rc != VSSI_SUCCESS) { rv++; goto fail; }
        rc = camera_roll_commit(handles[0]);
        if(rc == VSSI_CONFLICT) { continue; }
        else if(rc != VSSI_SUCCESS) { rv++; goto fail; }
    } while(rc == VSSI_CONFLICT);

    // Write multiple files into upload dataset in separate batches.
    // Confirm files exist in download dataset.
    // Confirm upload dataset empty, no trash records.

    // Write multiple files into upload dataset in separate spaced-out batches.
    // Confirm files exist in download dataset.
    // Confirm upload dataset empty, no trash records.

#if CR_TEST_FILE
    // Write actual photo files. Tests timestamp parsing.
    do {
        if(rc == VSSI_CONFLICT) {
            VSSI_ClearConflict(handles[0]);
        }

        rc = camera_roll_upload_file(handles[0],
                                     "some_device/D089-0027.JPG",
                                     "/home/david/exif/D089-0027.JPG");
        if(rc == VSSI_CONFLICT) { continue; }
        else if(rc != VSSI_SUCCESS) { rv++; goto fail; }
        rc = camera_roll_commit(handles[0]);
        if(rc == VSSI_CONFLICT) { continue; }
        else if(rc != VSSI_SUCCESS) { rv++; goto fail; }
    } while(rc == VSSI_CONFLICT);

    do {
        if(rc == VSSI_CONFLICT) {
            VSSI_ClearConflict(handles[0]);
        }

        rc = camera_roll_upload_file(handles[0],
                                     "some_device/phone.jpg",
                                     "/home/david/exif/phone.jpg");
        if(rc == VSSI_CONFLICT) { continue; }
        else if(rc != VSSI_SUCCESS) { rv++; goto fail; }
        rc = camera_roll_commit(handles[0]);
        if(rc == VSSI_CONFLICT) { continue; }
        else if(rc != VSSI_SUCCESS) { rv++; goto fail; }
    } while(rc == VSSI_CONFLICT);

    do {
        if(rc == VSSI_CONFLICT) {
            VSSI_ClearConflict(handles[0]);
        }

        rc = camera_roll_upload_file(handles[0],
                                     "some_device/gwar.jpg",
                                     "/home/david/exif/002427.jpg");
        if(rc == VSSI_CONFLICT) { continue; }
        else if(rc != VSSI_SUCCESS) { rv++; goto fail; }
        rc = camera_roll_commit(handles[0]);
        if(rc == VSSI_CONFLICT) { continue; }
        else if(rc != VSSI_SUCCESS) { rv++; goto fail; }
    } while(rc == VSSI_CONFLICT);


#endif

 fail:
    // Close both datasets.
    VSSI_CloseObject(handles[0],
                     &test_context, crtest_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Close object %s failed: %d.",
                         upload_location, rc);
        rv++;
    }
    VSSI_CloseObject(handles[1],
                     &test_context, crtest_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Close object %s failed: %d.",
                         download_location, rc);
        rv++;
    }

 fail_open:
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Finished test: %s. Result: %s",
                        vsTest_curTestName,
                        (rv == 0) ? "PASS" : "FAIL");
    return rv;
}
