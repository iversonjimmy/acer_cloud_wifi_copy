/*
 *  Copyright 2012 Acer Cloud Technology Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF IGWARE INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
 *
 */

///Virtual Storage Offline Test
///
/// This test exercises the Virtual Storage offline usage.

//#include "vpl_net.h"
#include "vplex_trace.h"
#include "vplex_assert.h"
#include "vplex_vs_directory.h"
#include "vpl_conv.h"
#include "vpl_th.h"

#include <iostream>
#include <sstream>
#include <string>
#include <set>
#include <vector>

/// These are Linux specific non-abstracted headers.
/// TODO: provide similar functionality in a platform-abstracted way.
#include <setjmp.h>
#include <signal.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "vsTest_virtual_drive_test.hpp"

#include "vssts.hpp"
#include "vssts_error.hpp"
#include "ccdi.hpp"


using namespace std;

extern string vsTest_curTestName;

typedef struct  {
    VPLSem_t sem;
    int rv;
} virtual_drive_test_context_t;

static void virtual_drive_test_callback(void* ctx, VSSI_Result rv)
{
    virtual_drive_test_context_t* context = (virtual_drive_test_context_t*)ctx;

    context->rv = rv;
    VPLSem_Post(&(context->sem));
}

static const string vsTest_virtual_drive_test = "VS Test Virtual Drive Test";
int test_virtual_drive_access(const u64 userId,
                              const u64 datasetId,
                              const u64 deviceId,
                              const u64 handle
                              )
{
    int rv = 0; // pass until failed.
    int rc = 0;

    VSSI_File file_handle = NULL;
    vsTest_curTestName = vsTest_virtual_drive_test;

    //do open
    VSSI_Object datasetHandle;
    virtual_drive_test_context_t test_context;
    VPL_SET_UNINITIALIZED(&(test_context.sem));
    if(VPLSem_Init(&(test_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        rc = -1;
    } 
    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                        "Create semphore result %d", rc);
    if(rc != 0) {
        rv++;
        goto fail;
    }

    {
        VSSI_OpenObjectTS(userId, datasetId, 
                         VSSI_READWRITE | VSSI_FORCE, &datasetHandle,
                         &test_context, virtual_drive_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                        "Dataset Open result %d.", rc);
    if(rc != 0) {
        rv++;
        goto failtoop;
    }

    //do open file
    {
        VSSI_OpenFile(datasetHandle, "virtual_drive_file", 
                      VSSI_FILE_OPEN_WRITE |
                      VSSI_FILE_OPEN_READ |
                      VSSI_FILE_OPEN_CREATE |
                      VSSI_FILE_SHARE_READ |
                      VSSI_FILE_SHARE_WRITE, 
                      0,
                      &file_handle,
                      &test_context,
                      virtual_drive_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                        "Dataset OpenFile result %d.", rc);
    if(rc != 0 && rc != VSSI_EXISTS) {
        rv++;
        goto failtoop;
    }

    //do write
    {
        char buf[32] = "Test data";
        u32 length = 32;
        VSSI_WriteFile(file_handle,
                       0, &length, buf,
                       &test_context, virtual_drive_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                        "Dataset Write result %d.", rc);
    if(rc != 0) {
        rv++;
        goto failtoop;
    }

    //do read
    {
        char buf[32] = {0};
        u32 length = 32;
        VSSI_ReadFile(file_handle,
                      0, &length, buf,
                      &test_context, virtual_drive_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,"read buffer content RC: %d buf: %s.", rc, buf);
    }
    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                        "Dataset Read result %d.", rc);
    if(rc != 0) {
        rv++;
        goto failtoop;
    }

    //do close file
    {
        VSSI_CloseFile(file_handle, &test_context, virtual_drive_test_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                        "Dataset CloseFile result %d.", rc);

   //do close
    VSSI_CloseObject(datasetHandle,
                    &test_context, virtual_drive_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                        "Dataset Close result %d.", rc);
    if(rc != 0) {
        rv++;
        goto failtoop;
    }

    failtoop:
    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                        "Destroy semaphore result %d.", rc);
        VPLSem_Destroy(&(test_context.sem));

    fail:
    return (rv) ? 1 : 0;
}
