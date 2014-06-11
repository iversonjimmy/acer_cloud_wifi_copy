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

#include "vssts.hpp"
#include "vssts_error.hpp"
// #include "vss_comm.h"

// #include "vsTest_vscs_common.hpp"
#include "vsTest_vscs_async.hpp"

#include <stack>

using namespace std;

extern const char* vsTest_curTestName;

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

typedef struct  {
    int io_cnt;
    int ios_complete;
    VPLSem_t sem;
    int rv;
} async_context_t;

static void async_callback(void* ctx, VSSI_Result rv)
{
    async_context_t* context = (async_context_t*)ctx;

    if ( (context->rv == 0) && (rv != 0) ) {
        context->rv = rv;
    }
    context->ios_complete++;
    if ( context->ios_complete == context->io_cnt ) {
        VPLSem_Post(&(context->sem));
    }
    else if ( context->ios_complete > context->io_cnt ) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Too many IOs %d:%d",
                         context->ios_complete, context->io_cnt);
    }
}

char myBigBuf[2 * 1024 * 1024];

#define IO_SIZE_K   (128)
#define IO_SIZE     (IO_SIZE_K*1024)
static int create_file(VSSI_Object handle,
                       vscs_test_context_t& test_context,
                       async_context_t& async_context,
                       const string& name, int file_size)
{
    u32 flags;
    u32 attrs;
    VSSI_File fileHandle1;
    u32 wrLen;
    int rv = 0;
    int rc;
    VPLTime_t start;
    VPLTime_t end;
    VPLTime_t elapsed;
    u32 io_cnt;
    u64 offset = 0;

    // file_size is in MiB, round up to nearest mult of IO_SIZE
    io_cnt = ((file_size * 1024) + (IO_SIZE_K - 1)) / IO_SIZE_K;

    flags = VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_CREATE_ALWAYS;
    attrs = 0;

VPLTRACE_LOG_INFO(TRACE_APP, 0, "Create file %s", name.c_str());
    start = VPLTime_GetTimeStamp();
    VSSI_OpenFile(handle, name.c_str(), flags, attrs, &fileHandle1,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc && (rc != VSSI_EXISTS)) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: VSSI_OpenFile returns %d.", rc);
        rv++;
        goto done;
    }

    // fire off io_cnt requests
    async_context.io_cnt = io_cnt;
    async_context.ios_complete = 0;
    async_context.rv = 0;
    for( int i = 0 ; i < io_cnt ; i++ ) {
        wrLen = IO_SIZE;
        VSSI_WriteFile(fileHandle1, offset, &wrLen, myBigBuf,
                              &async_context, async_callback);
        if ( wrLen != IO_SIZE ) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "VSSI_WriteFile change IO size");
            rv++;
        }
        offset += IO_SIZE;
    }

    VPLSem_Wait(&(async_context.sem));

    rc = async_context.rv;
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

done:
    if ( rv == 0 ) {
        end = VPLTime_GetTimeStamp();
        elapsed = VPLTime_DiffClamp(end, start);
#ifdef NOTDEF
        VPLTRACE_LOG_INFO(TRACE_APP, 0, "Write of %d MiB took "FMTu64,
            file_size, elapsed);
#endif // NOTDEF
    }
    return rv;
}

static int read_file(VSSI_Object handle,
                     const string& name,
                     vscs_test_context_t& test_context,
                     async_context_t& async_context,
                     int file_size)
{
    u32 flags;
    u32 attrs;
    VSSI_File fileHandle1;
    u32 rdLen;
    int rv = 0;
    int rc;
    VPLTime_t start;
    VPLTime_t end;
    VPLTime_t elapsed;
    u32 io_cnt;
    u64 offset = 0;

    // file_size is in MiB, round up to nearest mult of IO_SIZE
    io_cnt = ((file_size * 1024) + (IO_SIZE_K - 1)) / IO_SIZE_K;

    flags = VSSI_FILE_OPEN_READ;
    attrs = 0;

VPLTRACE_LOG_INFO(TRACE_APP, 0, "Create file %s", name.c_str());
    start = VPLTime_GetTimeStamp();
    VSSI_OpenFile(handle, name.c_str(), flags, attrs, &fileHandle1,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc && (rc != VSSI_EXISTS)) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: VSSI_OpenFile returns %d.", rc);
        rv++;
        goto done;
    }

    // fire off io_cnt requests
    async_context.io_cnt = io_cnt;
    async_context.ios_complete = 0;
    async_context.rv = 0;
    for( int i = 0 ; i < io_cnt ; i++ ) {
        rdLen = IO_SIZE;
        VSSI_ReadFile(fileHandle1, offset, &rdLen, myBigBuf,
                              &async_context, async_callback);
        if ( rdLen != IO_SIZE ) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "VSSI_ReadFile change IO size");
            rv++;
        }
        offset += IO_SIZE;
    }

    VPLSem_Wait(&(async_context.sem));

    rc = async_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_ReadFile returned %d.", rc);
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

done:
    if ( rv == 0 ) {
        end = VPLTime_GetTimeStamp();
        elapsed = VPLTime_DiffClamp(end, start);
#ifdef NOTDEF
        VPLTRACE_LOG_INFO(TRACE_APP, 0, "Write of %d MiB took "FMTu64,
            file_size, elapsed);
#endif // NOTDEF
    }
    return rv;
}


static int create_dir(VSSI_Object handle, vscs_test_context_t& test_context,
                      const string& path)
{
    int rv = 0;
    int rc;

VPLTRACE_LOG_INFO(TRACE_APP, 0, "Create path %s", path.c_str());
    // Now try to create the first directory again, which should fail
    VSSI_MkDir2(handle, path.c_str(), 0,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc && (rc != VSSI_ISDIR)) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_MkDir2 %s returns %d.",
                         path.c_str(), rc);
        rv++;
    }

    return rv;
}

static int make_tree(VSSI_Object handle, vscs_test_context_t& test_context,
                     async_context_t& async_context,
                     int num_dirs, int num_files, int num_levels,
                     int file_size, int cur_level, const string& cur_path)
{
    int rv = 0;

    if ( cur_level == num_levels ) {
        for( int i = 0 ; i < num_files ; i++ ) {
            char buf[20];
            snprintf(buf, sizeof(buf),"f%08x", i);
            rv = create_file(handle, test_context, async_context, cur_path + buf, file_size);
            if ( rv ) {
                break;
            }
        }
        return rv;
    }

    for( int i = 0 ; i < num_dirs ; i++ ) {
        string new_path;
        char buf[20];
        snprintf(buf, sizeof(buf),"d%08x", i);
        new_path = cur_path + buf;
        rv = create_dir(handle, test_context, new_path);
        if ( rv ) {
            break;
        }
        rv = make_tree(handle, test_context, 
                       async_context, num_dirs, num_files, num_levels,
                       file_size, cur_level + 1, new_path + "/");
        if ( rv ) {
            break;
        }
    }

    return rv;
}

static void dump_dirent(VSSI_Dirent2* de, const std::string& path)
{
    VPLTRACE_LOG_INFO(TRACE_APP, 0,
        "%s {%s%s} size:"FMTu64" ctime:"FMTu64" mtime:"FMTu64" ver:"FMTu64,
            de->isDir ? "Dir" : "File", path.c_str(), de->name, de->size,
            de->ctime, de->mtime, de->changeVer);
}

static int walk_tree(VSSI_Object handle, vscs_test_context_t& test_context,
                     async_context_t& async_context, const string& cur_path, int file_size)
{
    int rv = 0;
    int rc;
    VSSI_Dir2 dir = NULL;
    VSSI_Dirent2* dirent = NULL;
    stack<string> dirs;

VPLTRACE_LOG_INFO(TRACE_APP, 0, "Walking dir %s", cur_path.c_str());
    // Read the current directory
    VSSI_OpenDir2(handle, cur_path.c_str(), &dir,
                  &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenDir2(%s) returns %d.",
                         cur_path.c_str(), rc);
        rv++;
        goto done;
    }

    // walk the entries
    while( dir && (dirent = VSSI_ReadDir2(dir)) != NULL ) {
        dump_dirent(dirent, cur_path);
        if ( dirent->isDir ) {
            dirs.push(dirent->name);
        }
        else {
            string new_path;
            VSSI_Dirent2* stats = NULL;

            new_path = cur_path + dirent->name;
            rv = read_file(handle, new_path.c_str(), test_context, async_context, file_size);
            if(rc != VSSI_SUCCESS) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: read_file(%s) - %d",
                    new_path.c_str(), rc);
            }

            if(stats != NULL) {
                free(stats);
            }
        }
    }

    VSSI_CloseDir2(dir);
    dir = NULL;

    while (!dirs.empty()) {
        string new_path = cur_path + dirs.top() + "/";
        dirs.pop();
        walk_tree(handle, test_context, async_context, new_path, file_size);
    }

done:
    return 0;
}

static const char vsTest_async[] = "PSN Async Test";
static int test_async(u64 user_id,
                      u64 dataset_id,
                      int num_dirs,
                      int num_files,
                      int num_levels,
                      int file_size,
                      bool do_create,
                      const std::string& root_dir,
                      bool open_only)
{
    int rv = 0;
    int rc;
    VSSI_Object handle;
    vscs_test_context_t test_context;
    async_context_t async_context;
    std::string new_root;

    VPL_SET_UNINITIALIZED(&(test_context.sem));
    VPL_SET_UNINITIALIZED(&(async_context.sem));
    vsTest_curTestName = vsTest_async;

    new_root = root_dir;
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Starting test: %s root %s. open_only %s",
                        vsTest_curTestName, new_root.c_str(),
                        open_only ? "true" : "false");

    if(VPLSem_Init(&(test_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        return rv+1;
    }
    if(VPLSem_Init(&(async_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        return rv+1;
    }

    VSSI_OpenObjectTS(user_id, dataset_id,
                     VSSI_READWRITE | VSSI_FORCE, &handle,
                     &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Open object failed: %d.",
                         rc);
        rv++;
        goto fail_open;
    }

    if ( open_only ) {
        goto done;
    }

    if ( do_create ) {

        if ( new_root.compare("/") != 0 ) {
            rv = create_dir(handle, test_context, root_dir);
            if ( rv ) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: create_dir(%s) %d.", 
                    new_root.c_str(), rv);
                goto done;
            }
            new_root = new_root + "/";
        }

        rv = make_tree(handle, test_context, async_context, num_dirs, num_files, num_levels,
                       file_size, 0, new_root);
        if ( rv ) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: make_tree() %d.", rv);
            goto done;
        }
    }
    else {
        if ( new_root.compare("/") != 0 ) {
            new_root = new_root + "/";
        }
VPLTRACE_LOG_INFO(TRACE_APP, 0, "About to Walk dir %s", new_root.c_str());
        rv = walk_tree(handle, test_context, async_context, new_root, file_size);
        if ( rv ) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: walk_tree() %d.", rv);
            goto done;
        }
    }

done:
    // Close the VSS Object
    VSSI_CloseObject(handle, &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Failed to close object : %d",
                         rc);
        rv++;
    }

fail_open:
    VPLSem_Destroy(&(test_context.sem));
    VPLSem_Destroy(&(async_context.sem));

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                   "Test %s result: %d.",
                   vsTest_curTestName, rv);
    return rv;
}

#ifdef NOTDEF
static const char vsTest_vss_file_create[] = "VSS File Create Test";
static int test_vss_file_create(u64 user_id,
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

    VSSI_Commit(handle, &test_context, vscs_test_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Commit on Remove component %s: %d.",
                         name, rc);
        goto done;
    }

done:
    return rc;
}
#endif // NOTDEF

static const char vsTest_test_vssi_dataset_access[] = "VSSI Dataset Access Test";
int test_vssi_dataset_access(u64 user_id,
                             u64 dataset_id,
                             int num_dirs,
                             int num_files,
                             int num_levels,
                             int file_size,
                             bool do_create,
                             const std::string& root_dir,
                             bool open_only)
{
    int rv = 0;
    vsTest_curTestName = vsTest_test_vssi_dataset_access;
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                   "Starting test: %s.",
                   vsTest_curTestName);

    rv += test_async(user_id,
                     dataset_id,
                     num_dirs, num_files,
                     num_levels, file_size,
                     do_create, root_dir, open_only);

    return rv;
}
