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

#include "vpl_th.h"
#include "vplex_trace.h"
#include "vplex_time.h"

#include "vssi.h"
#include "vssi_error.h"

#include "vsTest_expire_data.hpp"

#include <iostream>
#include <openssl/sha.h>

using namespace std;

extern const char* vsTest_curTestName;

/// Arbitrary metadata generator to use for tests.
#define TEST_METADATA_SIZE 10
static char test_metadata[TEST_METADATA_SIZE] = {0xaa, 0x55, 0xa5, 0x5a, 0x00,
                                                 0xaa, 0x55, 0xa5, 0x5a, 0x00};
static void update_metadata() {
    // permute the metadata in a pseudo-random fashion
    int i;
    for(i = 0; i < TEST_METADATA_SIZE; i++) {
        test_metadata[i] = (test_metadata[i] + test_metadata[((TEST_METADATA_SIZE + i - 1) % TEST_METADATA_SIZE)]) & 0xff;
    }
}

// callback and context to make async commands synchronous for testing.
typedef struct  {
    VPLSem_t sem;
    int rv;
} pctest_context_t;
static void pctest_callback(void* ctx, VSSI_Result rv)
{
    pctest_context_t* context = (pctest_context_t*)ctx;

    context->rv = rv;
    VPLSem_Post(&(context->sem));
}
pctest_context_t test_context;

/// Object to use personal cloud tests.
static VSSI_Object handle = 0;
static vs_dir root;

static const char vsTest_personal_cloud_data [] = "Data Expiration Test";
int test_data_expiration(VSSI_Session session,
                         u64 user_id,
                         u64 dataset_id,
                         const VSSI_RouteInfo& route_info)
{
    int rv = 0;
    int rc;

    const string path = ""; // root path

    /// test step types
    enum {
        OPEN, // Open object for testing
        CLOSE, // Close object
        WRITE,
        TRUNCATE,
        MKDIR,
        READDIR,
        READ,
        RENAME,
        COPY,
        COPY_MATCH,
        REMOVE,
        CLEAR_CONFLICT,
        VERIFY,
        SET_CTIME, // set only ctime
        SET_MTIME, // set only mtime
        SET_CMTIME, // set both to the same time
        PAUSE, // Pause for some time before next step.
        EXPIRE, // Expire fiels and directories around for more than 30 days (according to mtime)
        ABORT // stop testing immediately
    };
    static const char* step_names[] = {
        "Open Dataset",
        "Close Dataset",
        "Write File",
        "Truncate File",
        "Make Directory",
        "Read Directory",
        "Read File",
        "Rename",
        "Copy",
        "Copy Match",
        "Remove",
        "Clear Conflict",
        "Verify",
        "Set Create Time",
        "Set Modify Time",
        "Set Create and Modify Times",
        "Pause",
        "Expire",
        "Abort"
    };

    /// test instructions
    struct test_instruction {
        int type; // type of step.
        int expect; // expected result
        const char* name; // component to effect.
        const char* data; // if writing a file, data to write.
        // If renaming a file or directory, the new name
        u64 size; // if truncating, size to set. Also timestamp for SET_*TIME, time for PAUSE.
    };
    struct test_instruction steps[] = {
        {OPEN,     0, "",      "",           0},
        {REMOVE,   0, "",      "",           0}, // Remove all previous contents (initial condition)
        // Build a tree.
        {WRITE,    0, "file1", "abc",        0},
        {WRITE,    0, "file2", "abc",        0},
        {WRITE,    0, "file3", "abc",        0},
        {MKDIR,    0, "dir1",  "",           0},
        {WRITE,    0, "dir1/file1", "abc",   0},
        {WRITE,    0, "dir1/file2", "abc",   0},
        {WRITE,    0, "dir1/file3", "abc",   0},
        {MKDIR,    0, "dir2",  "",           0},
        {WRITE,    0, "dir2/file1", "abc",   0},
        {WRITE,    0, "dir2/file2", "abc",   0},
        {WRITE,    0, "dir2/file3", "abc",   0},
        {MKDIR,    0, "dir3",  "",           0},
        {WRITE,    0, "dir3/file1", "abc",   0},
        {WRITE,    0, "dir3/file2", "abc",   0},
        {WRITE,    0, "dir3/file3", "abc",   0},
        {MKDIR,    0, "dir3/dir4",  "",           0},
        {WRITE,    0, "dir3/dir4/file1", "abc",   0},
        {WRITE,    0, "dir3/dir4/file2", "abc",   0},
        {WRITE,    0, "dir3/dir4/file3", "abc",   0},
        {MKDIR,    0, "dir3/dir4/dir5",  "",           0},
        {WRITE,    0, "dir3/dir4/dir5/file1", "abc",   0},
        {WRITE,    0, "dir3/dir4/dir5/file2", "abc",   0},
        {WRITE,    0, "dir3/dir4/dir5/file3", "abc",   0},
        {MKDIR,    0, "dir5",  "",           0},
        {MKDIR,    0, "dir6",  "",           0},
        {MKDIR,    0, "dir6/dir7",  "",           0},
        {MKDIR,    0, "dir6/dir7/dir8",  "",           0},
        // Confirm tree is as-expected.
        {VERIFY,   0, "", "", 0},
        // Age some files (set mtime to 31 days ago)
        {SET_CMTIME,   0, "file1", "", VPLTime_GetTime() - VPLTime_FromSec(60*60*24*31)},
        {SET_CMTIME,   0, "dir1/file1", "", VPLTime_GetTime() - VPLTime_FromSec(60*60*24*31)},
        {SET_CMTIME,   0, "dir2/file1", "", VPLTime_GetTime() - VPLTime_FromSec(60*60*24*31)},
        {SET_CMTIME,   0, "dir3/file1", "", VPLTime_GetTime() - VPLTime_FromSec(60*60*24*31)},
        {SET_CMTIME,   0, "dir3/dir4/file1", "", VPLTime_GetTime() - VPLTime_FromSec(60*60*24*31)},
        {SET_CMTIME,   0, "dir3/dir4/dir5/file1", "", VPLTime_GetTime() - VPLTime_FromSec(60*60*24*31)},
        // Close/reopen, pausing 1 second in-between.
        {CLOSE, 0, "", "", 0},
        {PAUSE, 0, "", "", 1},
        {OPEN, 0, "", "", 0},
        // Verify expired files are gone.
        {EXPIRE, 0, "", "", 0}, // also does verification
        // Age some directories (set mtime to 30 days ago)
        {SET_CMTIME,   0, "dir1", "", VPLTime_GetTime() - VPLTime_FromSec(60*60*24*31)},
        {SET_CMTIME,   0, "dir1/file2", "", VPLTime_GetTime() - VPLTime_FromSec(60*60*24*31)},
        {SET_CMTIME,   0, "dir1/file3", "", VPLTime_GetTime() - VPLTime_FromSec(60*60*24*31)},
        {SET_CMTIME,   0, "dir2", "", VPLTime_GetTime() - VPLTime_FromSec(60*60*24*31)},
        {SET_CMTIME,   0, "dir3/dir4/dir5", "", VPLTime_GetTime() - VPLTime_FromSec(60*60*24*31)},
        {SET_CMTIME,   0, "dir3/dir4/dir5/file2", "", VPLTime_GetTime() - VPLTime_FromSec(60*60*24*31)},
        {SET_CMTIME,   0, "dir3/dir4/dir5/file3", "", VPLTime_GetTime() - VPLTime_FromSec(60*60*24*31)},
        {SET_CMTIME,   0, "dir5", "", VPLTime_GetTime() - VPLTime_FromSec(60*60*24*31)},
        {SET_CMTIME,   0, "dir6", "", VPLTime_GetTime() - VPLTime_FromSec(60*60*24*31)},
        // Close/reopen, pausing 1 second in-between.
        {CLOSE, 0, "", "", 0},
        {PAUSE, 0, "", "", 1},
        {OPEN, 0, "", "", 0},
        // Verify expired empty directories removed, all others still present (expired not empty, not expired empty)
        {EXPIRE, 0, "", "", 0}, // also does verification
        // Age files within an expired directory.
        {SET_CMTIME,   0, "dir2/file2", "", VPLTime_GetTime() - VPLTime_FromSec(60*60*24*31)},
        {SET_CMTIME,   0, "dir2/file3", "", VPLTime_GetTime() - VPLTime_FromSec(60*60*24*31)},
        // Close/reopen, pausing 1 second in-between.
        {CLOSE, 0, "", "", 0},
        {PAUSE, 0, "", "", 1},
        {OPEN, 0, "", "", 0},
        // Verify expired empty directories removed, all others still present (expired not empty, not expired empty)
        {EXPIRE, 0, "", "", 0}, // also does verification

        // Age all files, direcrtories in multi-level tree. Verify all removed.
        {SET_CTIME,    0, "dir3/file2", "", VPLTime_GetTime() - VPLTime_FromSec(60*60*24*31)},
        {SET_CTIME,    0, "dir3/file3", "", VPLTime_GetTime() - VPLTime_FromSec(60*60*24*31)},
        {SET_CTIME,    0, "dir3/dir4", "", VPLTime_GetTime() - VPLTime_FromSec(60*60*24*31)},
        {SET_CTIME,    0, "dir3/dir4/file2", "", VPLTime_GetTime() - VPLTime_FromSec(60*60*24*31)},
        {SET_CTIME,    0, "dir3/dir4/file3", "", VPLTime_GetTime() - VPLTime_FromSec(60*60*24*31)},
        {SET_CTIME,    0, "dir3/dir4/dir5/file2", "", VPLTime_GetTime() - VPLTime_FromSec(60*60*24*31)},
        {SET_CTIME,    0, "dir3/dir4/dir5/file3", "", VPLTime_GetTime() - VPLTime_FromSec(60*60*24*31)},
        // Close/reopen, pausing 1 second in-between.
        {CLOSE, 0, "", "", 0},
        {PAUSE, 0, "", "", 1},
        {OPEN, 0, "", "", 0},
        // Verify expired empty directories removed, all others still present (expired not empty, not expired empty)
        {EXPIRE, 0, "", "", 0}, // also does verification
        {CLOSE,    0, "",      "",           0},
    };
    size_t num_steps, i;
    num_steps = sizeof(steps)/sizeof(steps[0]);

    vsTest_curTestName = vsTest_personal_cloud_data;
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Starting test: %s.",
                        vsTest_curTestName);

    VPL_SET_UNINITIALIZED(&(test_context.sem));

    if(VPLSem_Init(&(test_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create sempahore.");
        return -1;
    }

    for(i = 0; i < num_steps; i++) {
        
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                            "Carrying out step (%d) %s on %s, expecting result %d.",
                            i, step_names[steps[i].type], steps[i].name,
                            steps[i].expect);
        string name = steps[i].name;

        // Carry out the instruction, verifying result.
        switch(steps[i].type) {
        case ABORT: // for use during test development
            exit(1);
            break;

        case PAUSE:
            VPLThread_Sleep(VPLTime_FromSec(steps[i].size));
            break;

        case OPEN:
            VSSI_OpenObject2(session, user_id, dataset_id, &route_info,
                             VSSI_READWRITE, &handle,
                             &test_context, pctest_callback);
            VPLSem_Wait(&(test_context.sem));
            rc = test_context.rv;
            if(rc != VSSI_SUCCESS) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "Open object for dataset "FMTu64":"FMTu64": %d.",
                                 user_id, dataset_id, rc);
                rv++;
            }
            break;
            
        case CLOSE:
            VSSI_CloseObject(handle,
                             &test_context, pctest_callback);
            VPLSem_Wait(&(test_context.sem));
            rc = test_context.rv;
            if(rc != VSSI_SUCCESS) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "Close object "FMTu64":"FMTu64" failed: %d.",
                                 user_id, dataset_id, rc);
                rv++;
            }            
            handle = 0;
            break;

        case WRITE: {
            string data = steps[i].data;
            rc = root.write_file(handle, path, name, data);
            if(rc != steps[i].expect) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "Write file %s: %d, expected %d.",
                                 name.c_str(), rc, steps[i].expect);
                rv++;
            }
        } break;

        case TRUNCATE: {
            u64 size = steps[i].size;
            rc = root.truncate_file(handle, path, name, size);
            if(rc != steps[i].expect) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "Truncate file %s: %d, expected %d.",
                                 name.c_str(), rc, steps[i].expect);
                rv++;
            }
        } break;

        case SET_CTIME: {
            u64 size = steps[i].size;
            rc = root.set_times(handle, name, size, 0);
            if(rc != steps[i].expect) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "Set-times file %s: %d, expected %d.",
                                 name.c_str(), rc, steps[i].expect);
                rv++;
            }
        } break;
        case SET_MTIME: {
            u64 size = steps[i].size;
            rc = root.set_times(handle, name, 0, size);
            if(rc != steps[i].expect) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "Set-times file %s: %d, expected %d.",
                                 name.c_str(), rc, steps[i].expect);
                rv++;
            }
        } break;
        case SET_CMTIME: {
            u64 size = steps[i].size;
            rc = root.set_times(handle, name, size, size);
            if(rc != steps[i].expect) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "Set-times file %s: %d, expected %d.",
                                 name.c_str(), rc, steps[i].expect);
                rv++;
            }
        } break;

        case MKDIR: {
            rc = root.add_dir(handle, path, name);
            if(rc != steps[i].expect) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "Make directory %s: %d, expected %d.",
                                 name.c_str(), rc, steps[i].expect);
                rv++;
            }
        } break;
        case READDIR:
            rc = root.read_dir(handle, path, name);
            if(rc != steps[i].expect) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "Read directory %s: %d, expect %d",
                                 name.c_str(), rc, steps[i].expect);
                rv++;
            }
            break;
        case READ:
            rc = root.read_file(handle, path, name);
            if(rc != steps[i].expect) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "Read file %s: %d, expect %d",
                                 name.c_str(), rc, steps[i].expect);
                rv++;
            }
            break;
        case REMOVE:
            rc = root.remove(handle, name);
            if(rc != steps[i].expect) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "Remove directory %s: %d, expect %d.",
                                 name.c_str(), rc, steps[i].expect);
                rv++;
            }
            break;

        case RENAME: {
            string new_name = steps[i].data;
            rc = root.rename(handle, name, new_name);
            if(rc != steps[i].expect) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "rename %s to %s: %d, expect %d.",
                                 name.c_str(), new_name.c_str(),
                                 rc, steps[i].expect);
                rv++;
            }
        } break;

        case COPY: {
            string new_name = steps[i].data;
            rc = root.copy(handle, name, new_name);
            if(rc != steps[i].expect) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "copy %s to %s: %d, expect %d.",
                                 name.c_str(), new_name.c_str(),
                                 rc, steps[i].expect);
                rv++;
            }
        } break;

        case COPY_MATCH: {
            string new_name = steps[i].data;
            rc = root.copy_match(handle, name, new_name);
            if(rc != steps[i].expect) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "copy match %s to %s: %d, expect %d.",
                                 name.c_str(), new_name.c_str(),
                                 rc, steps[i].expect);
                rv++;
            }
        } break;

        case CLEAR_CONFLICT: {
            rc = VSSI_ClearConflict(handle);
            if(rc != steps[i].expect) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "Clear conflict: %d, expect %d.",
                                 rc, steps[i].expect);
                rv++;
            }
        } break;

        case EXPIRE:
            // Expire any files and empty directories past expiration time (30 days).
            if(root.expire_data()) {
                root.version++;
            }
            break;

        case VERIFY:
            break;
        }

        // For write-type instructions, verify server data is correct.
        switch(steps[i].type) {
        case READDIR:
        case READ:
        case CLEAR_CONFLICT:
        case OPEN:
        case CLOSE:
            // does not apply to read, meta instructions.
            break;
        case WRITE:
        case TRUNCATE:
        case MKDIR:
        case REMOVE:
        case RENAME:
        case COPY:
        case SET_CTIME:
        case SET_MTIME:
        case SET_CMTIME:
        case VERIFY:
        case EXPIRE:
            rc = root.verify(handle, path,
                             VSSI_GetVersion(handle),
                             0, 0);
            if(rc != 0) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "Server data does not match: %d", rc);
                rv++;
            }
        }

        if(rv != 0) {
            break;
        }
    }

    if(handle != 0) {
        VSSI_CloseObject(handle,
                         &test_context, pctest_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Close object "FMTu64":"FMTu64" failed: %d.",
                             user_id, dataset_id, rc);
            rv++;
        }
    }        

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Finished test: %s. Result: %s",
                        vsTest_curTestName,
                        (rv == 0) ? "PASS" : "FAIL");
    return rv;
}

static int check_stat(VSSI_Object handle,
                      const std::string name,
                      VSSI_Dirent* dir_entry)
{
    VSSI_Dirent* stats = NULL;
    VSSI_Metadata* stat_meta;
    VSSI_Metadata* dirent_meta;
    int rv = 0;

    // Get file stats and make sure they match the dirent data.
    VPLTRACE_LOG_FINE(TRACE_APP, 0,
                      "Verifying stat for file %s.",
                      name.c_str());

    VSSI_Stat(handle, name.c_str(), &stats,
              &test_context, pctest_callback);
    VPLSem_Wait(&(test_context.sem));
    rv = test_context.rv;
    if(rv != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Stat call for %s failed: %d.",
                         name.c_str(), rv);
        rv = 1;
        goto exit;
    }

    if(stats->ctime == 0) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Stats for %s show ctime of 0.",
                         name.c_str());
        rv = 1;
        goto exit;
    }
    if(stats->mtime == 0) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Stats for %s show mtime of 0.",
                         name.c_str());
        rv = 1;
        goto exit;
    }

    if(stats->size != dir_entry->size ||
       stats->ctime != dir_entry->ctime ||
       stats->mtime != dir_entry->mtime ||
       stats->isDir != dir_entry->isDir ||
       stats->changeVer != dir_entry->changeVer ||
       strcmp(stats->name, dir_entry->name) != 0) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Stats for %s differ from directory entry.",
                         name.c_str());
        VPLTRACE_LOG_ERR(TRACE_APP, 0,
                         "\tsize: "FMTu64" vs. "FMTu64".",
                         stats->size, dir_entry->size);
        VPLTRACE_LOG_ERR(TRACE_APP, 0,
                         "\tctime: "FMTu64" vs. "FMTu64".",
                         stats->ctime, dir_entry->ctime);
        VPLTRACE_LOG_ERR(TRACE_APP, 0,
                         "\tmtime: "FMTu64" vs. "FMTu64".",
                         stats->mtime, dir_entry->mtime);
        VPLTRACE_LOG_ERR(TRACE_APP, 0,
                         "\tchangeVer: "FMTu64" vs. "FMTu64".",
                         stats->changeVer, dir_entry->changeVer);
        VPLTRACE_LOG_ERR(TRACE_APP, 0,
                         "\tisDir: %d vs. %d.",
                         stats->isDir, dir_entry->isDir);
        VPLTRACE_LOG_ERR(TRACE_APP, 0,
                         "\tname: %s vs. %s.",
                         stats->name, dir_entry->name);
        rv = 1;
        goto exit;
    }

    VSSI_RewindMetadata(dir_entry);
    do {
        stat_meta = VSSI_ReadMetadata(stats);
        dirent_meta = VSSI_ReadMetadata(dir_entry);
        if((stat_meta == NULL || dirent_meta == NULL) &&
           stat_meta != dirent_meta) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Stats metadata for %s differs in number of entries from directory entry metadata.",
                             name.c_str());
            rv = -1;
            goto exit;
        }

        if(stat_meta && dirent_meta) {
            if(stat_meta->type != dirent_meta->type ||
               stat_meta->length != dirent_meta->length ||
               memcmp(stat_meta->data, dirent_meta->data, stat_meta->length) != 0) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "Stats metadata for %s differs from directory entry metadata.",
                                 name.c_str());
            }
        }
    } while(stat_meta != NULL && dirent_meta != NULL);

 exit:
    VSSI_RewindMetadata(dir_entry);

    if(stats != NULL) {
        free(stats);
    }

    return rv;
}

vs_file::vs_file() : 
    ctime_explicit(false),
    mtime_explicit(false), 
    confirmed(false)
{
    ctime = mtime = VPLTime_GetTime();
}

void vs_file::reset_verify()
{
    confirmed = false;
}

void vs_file::compute_signature()
{
    // Compute SHA1 hash of the file.
    SHA_CTX context;

    SHA1_Init(&context);
    SHA1_Update(&context, contents.data(), contents.length());
    SHA1_Final((unsigned char*)signature, &context);
}

void vs_file::update_version(u64 change_version)
{
    version = change_version;
}

int vs_file::verify(VSSI_Object handle,
                    const std::string& name,
                    const char* received_signature,
                    const std::string* received_metadata,
                    u64 received_version,
                    u64 received_ctime,
                    u64 received_mtime)
{
    int rv = 0;
    u32 length = contents.size() + 1024; // extra space to detect long files
    char data[length];
    string server_contents;
    int rc;

    VPLTRACE_LOG_FINE(TRACE_APP, 0,
                        "Verifying file %s.",
                        name.c_str());

    VSSI_Read(handle, name.c_str(), 0, &length, data,
              &test_context, pctest_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc == VSSI_EOF) {
        // May be OK if file should be empty.
        length = 0;
    }
    else if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0,
                         "Read file %s: %d.",
                         name.c_str(), rc);
        rv = rc;
    }
    if(length > 0) {
        server_contents.assign(data, length);
    }

    if(rc == VSSI_EOF || rc == VSSI_SUCCESS) {
        if(received_metadata &&
           metadata != *received_metadata) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "File %s metadata not matched at server.",
                             name.c_str());
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "Expected %d bytes:",
                             metadata.length());
            VPLTRACE_DUMP_BUF_ERR(TRACE_BVS, 0,
                                  metadata.data(), metadata.length());
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "Received %d bytes:",
                             received_metadata->length());
            VPLTRACE_DUMP_BUF_ERR(TRACE_BVS, 0,
                                  received_metadata->data(), received_metadata->length());

            rv = 1;
        }

        if(received_signature) {
            compute_signature();
            
            if(memcmp(received_signature, signature, 20) != 0) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "File %s signature not matched at server.",
                                 name.c_str());
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "Expected:");           
                VPLTRACE_DUMP_BUF_ERR(TRACE_BVS, 0, signature, 20);
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "Received:");
                VPLTRACE_DUMP_BUF_ERR(TRACE_BVS, 0, received_signature, 20);
                rv = 1;
            }
        }

        if(contents != server_contents) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "File %s contents not matched at server."
                             "\n\tlocal:%s\n\tserver:%s",
                             name.c_str(), contents.c_str(),
                             server_contents.c_str());
            rv = 1;
        }
    }

    if(received_version != 0 &&
       version != received_version) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "File \"%s\" change version "FMTu64" does not match reported change version "FMTu64".",
                         name.c_str(), version, received_version);
        rv = 1;
    }

    if(ctime_explicit && received_ctime != 0 && received_ctime != ctime) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "File \"%s\" creation time "FMTu64" does not match reported creaton time "FMTu64".",
                         name.c_str(), ctime, received_ctime);
        rv = 1;
    }
    if(mtime_explicit && received_mtime != 0 && received_mtime != mtime) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "File \"%s\" modification time "FMTu64" does not match reported modification time "FMTu64".",
                         name.c_str(), mtime, received_mtime);
        rv = 1;
    }

    if(rv == 0) {
        confirmed = true;
    }

    return rv;
}

bool vs_file::is_expired()
{
    bool rv = false;

    if(mtime + VPLTime_FromSec(60*60*24*30) < VPLTime_GetTime()) {
        rv = true;
    }

    return rv;
}

bool vs_file::is_confirmed()
{
    return confirmed;
}

int vs_file::write(VSSI_Object handle,
                   const std::string& name,
                   const std::string& data)
{
    int rv = 0;
    int rc;
    u32 length = data.size();
    bool truncate = (data.size() < contents.size());

    VPLTRACE_LOG_FINE(TRACE_APP, 0,
                        "Writing file %s.",
                        name.c_str());

    contents = data;

    VSSI_Write(handle, name.c_str(), 0, &length, data.data(),
               &test_context, pctest_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Write file %s: %d.",
                         name.c_str(), rc);
        rv = rc;
        goto exit;
    }

    // set size if file shrunk
    if(truncate) {
        VSSI_Truncate(handle, name.c_str(), length,
                      &test_context, pctest_callback);
        VPLSem_Wait(&(test_context.sem));
        rc = test_context.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Truncate file %s: %d.",
                             name.c_str(), rc);
            rv = rc;
            goto exit;
        }
    }

    // Set metadata.
    VPLTRACE_LOG_FINE(TRACE_APP, 0,
                        "Setting file %s metadata.",
                        name.c_str());
    update_metadata();
    metadata.assign(test_metadata, TEST_METADATA_SIZE);
    VSSI_SetMetadata(handle, name.c_str(), 0,
                     metadata.length(), metadata.data(),
                     &test_context, pctest_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "SetMetatada file %s: %d.",
                         name.c_str(), rc);
        rv = rc;
        goto exit;
    }

    VSSI_Commit(handle, &test_context, pctest_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Commit after write file %s: %d.",
                         name.c_str(), rc);
        rv = rc;
    }

    if(rv == 0) {
        version = VSSI_GetVersion(handle);
    }

 exit:
    return rv;
}

int vs_file::truncate(VSSI_Object handle,
                      const std::string& name,
                      u64 size)
{
    int rv = 0;
    int rc;

    VPLTRACE_LOG_FINE(TRACE_APP, 0,
                        "Truncating file %s.",
                        name.c_str());

    contents.resize(size);

    VSSI_Truncate(handle, name.c_str(), size,
                  &test_context, pctest_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Truncate file %s: %d.",
                         name.c_str(), rc);
        rv = rc;
        goto exit;
    }

    // Set metadata.
    update_metadata();
    metadata.assign(test_metadata, TEST_METADATA_SIZE);
    VSSI_SetMetadata(handle, name.c_str(), 0,
                     metadata.length(), metadata.data(),
                     &test_context, pctest_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "SetMetadata file %s: %d.",
                         name.c_str(), rc);
        rv = rc;
        goto exit;
    }

    VSSI_Commit(handle, &test_context, pctest_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Commit after write file %s: %d.",
                         name.c_str(), rc);
        rv = rc;
    }

    if(rv == 0) {
        version = VSSI_GetVersion(handle);
    }

 exit:
    return rv;
}

vs_dir::vs_dir() : 
    ctime_explicit(false),
    mtime_explicit(false),
    confirmed(false) 
{
    ctime = mtime = VPLTime_GetTime();
}

void vs_dir::reset_verify()
{
    map<string, vs_dir>::iterator dir_it;
    for(dir_it = dirs.begin(); dir_it != dirs.end(); dir_it++) {
        dir_it->second.reset_verify();
    }
    map<string, vs_file>::iterator file_it;
    for(file_it = files.begin(); file_it != files.end(); file_it++) {
        file_it->second.reset_verify();
    }
    confirmed = false;
}

static string concat_paths(const std::string &path1,
                           const std::string &path2)
{
    string path;
    if (path1.empty()) {
        path = path2;
    }
    else {
        path = path1 + "/" + path2;
    }
    return path;
}

int vs_dir::verify(VSSI_Object handle,
                   const std::string& name,
                   u64 received_version,
                   u64 received_ctime,
                   u64 received_mtime)
{
    int rv;
    VSSI_Dir server_dir;
    VSSI_Dirent* server_entry;
    string element_name;
    map<string, vs_dir>::iterator dir_it;
    map<string, vs_file>::iterator file_it;

    VPLTRACE_LOG_FINE(TRACE_APP, 0,
                        "Verifying directory %s.",
                        name.c_str());

    // reset state before verification (nothing confirmed yet)
    reset_verify();

    VSSI_OpenDir(handle, name.c_str(), &server_dir,
                 &test_context, pctest_callback);
    VPLSem_Wait(&(test_context.sem));
    rv = test_context.rv;
    if(rv != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0,
                         "Read directory %s: %d.",
                         name.c_str(), rv);
        goto exit_no_free;
    }

    do {
        server_entry = VSSI_ReadDir(server_dir);
        if(server_entry == NULL) {
            break;
        }

        element_name = server_entry->name;
        if(server_entry->isDir) {
            dir_it = dirs.find(element_name);
            if(dir_it == dirs.end()) {
                string s = concat_paths(name, element_name);
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "Failed to find directory element: %s", 
                                 s.c_str());
                rv = 1;
                goto exit;
            }
            else {
                element_name = concat_paths(name, server_entry->name);
                rv = dir_it->second.verify(handle,
                                           element_name,
                                           server_entry->changeVer,
                                           server_entry->ctime,
                                           server_entry->mtime);
                if(rv != 0) {
                    VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                     "Failed to verify directory %s",
                                     element_name.c_str());
                    goto exit;
                }
            }
        }
        else {
            file_it = files.find(element_name);
            if(file_it == files.end()) {
                string s = concat_paths(name, element_name);
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "Failed to find file element: %s",
                                 s.c_str());
                rv = 1;
                goto exit;
            }
            else {
                string received_metadata;
                VSSI_Metadata* metadata_entry = VSSI_GetMetadataByType(server_entry, 0);
                if(metadata_entry != NULL) {
                    received_metadata.assign(metadata_entry->data,
                                             metadata_entry->length);
                }
                element_name = concat_paths(name, server_entry->name);
                rv = file_it->second.verify(handle,
                                            element_name,
                                            server_entry->signature,
                                            &received_metadata,
                                            server_entry->changeVer,
                                            server_entry->ctime,
                                            server_entry->mtime);
                if(rv != 0) {
                    VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                     "Failed to verify file %s",
                                     element_name.c_str());
                    goto exit;
                }
            }
        }

        // Make sure stat info matches dirent info.
        if(check_stat(handle, element_name, server_entry) != 0) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Stat info did not match directory info for %s.",
                             element_name.c_str());
            rv = 1;
            goto exit;
        }

        if(received_version != 0 &&
           version != received_version) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "File change version "FMTu64" for \"%s\" does not match reported change version "FMTu64".",
                             version, name.c_str(), received_version);
            rv = 1;
            goto exit;
        }

        if(ctime_explicit && received_ctime != 0 && received_ctime != ctime) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Directory \"%s\" creation time "FMTu64" does not match reported creaton time "FMTu64".",
                             name.c_str(), ctime, received_ctime);
            rv = 1;
            goto exit;
        }
        if(mtime_explicit && received_mtime != 0 && received_mtime != mtime) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Directory \"%s\" modification time "FMTu64" does not match reported modification time "FMTu64".",
                             name.c_str(), mtime, received_mtime);
            rv = 1;
            goto exit;
        }
    } while(server_entry != NULL);

    // Confirm all child elements are verified to verify this directory.
    for(dir_it = dirs.begin(); dir_it != dirs.end(); dir_it++) {
        if(!dir_it->second.is_confirmed()) {
            string s = concat_paths(name, dir_it->first);
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Directory %s not confirmed at server.",
                             s.c_str());
                rv = 1;
                goto exit;
        }
    }
    for(file_it = files.begin(); file_it != files.end(); file_it++) {
        if(!file_it->second.is_confirmed()) {
            string s = concat_paths(name, file_it->first);
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "File %s not confirmed at server.",
                             s.c_str());
                rv = 1;
                goto exit;
        }
    }

    confirmed = true;
    rv = 0;

 exit:
    VSSI_CloseDir(server_dir);
 exit_no_free:
    return rv;
}

bool vs_dir::expire_data()
{
    map<string, vs_dir>::iterator dir_it;
    map<string, vs_file>::iterator file_it;
    bool expired_data = false;

    // Delete expired files.
    for(file_it = files.begin(); file_it != files.end(); ) {
        map<string, vs_file>::iterator tmp = file_it++;
        if(tmp->second.is_expired()) {
            VPLTRACE_LOG_INFO(TRACE_APP, 0,
                              "Deleting file {%s} with mtime "FMT_VPLTime_t" vs. now time "FMT_VPLTime_t".",
                              tmp->first.c_str(), tmp->second.mtime, VPLTime_GetTime());
            files.erase(tmp);
            expired_data = true;
        }
    }

    // Expire data in directories.
    for(dir_it = dirs.begin(); dir_it != dirs.end(); dir_it++) {
        expired_data |= dir_it->second.expire_data();
    }

    // Delete expired directories.
    for(dir_it = dirs.begin(); dir_it != dirs.end(); ) {
        map<string, vs_dir>::iterator tmp = dir_it++;
        if(tmp->second.is_expired()) {
            VPLTRACE_LOG_INFO(TRACE_APP, 0,
                              "Deleting dir {%s} with mtime "FMT_VPLTime_t" vs. now time "FMT_VPLTime_t".",
                              tmp->first.c_str(), tmp->second.mtime, VPLTime_GetTime());
            dirs.erase(tmp);
            expired_data = true;
        }
    }

    return expired_data;
}

bool vs_dir::is_expired()
{
    bool rv = false;

    if(mtime + VPLTime_FromSec(60*60*24*30) < VPLTime_GetTime() &&
       dirs.empty() && files.empty()) {
        rv = true;
    }

    return rv;
}

bool vs_dir::is_confirmed()
{
    return confirmed;
}

int vs_dir::write_file(VSSI_Object handle,
                       const std::string& path,
                       const std::string& name,
                       const std::string& data)
{
    // When name has no path, write the named file, creating it as needed.
    // Otherwise, pass to the next directory in the path.
    // If a path element is a file, delete the file.
    // If a path element is missing, create it.
    // Relay server response back up the chain.

    int rv = 0;
    size_t slash;
    string element_name;

    VPLTRACE_LOG_FINE(TRACE_APP, 0,
                        "Writing file (%s)/(%s).",
                        path.c_str(), name.c_str());

    slash = name.find_first_of('/');
    element_name = name.substr(0, slash);
    if(slash == string::npos) {

        map<string, vs_dir>::iterator dir_it = dirs.find(element_name);
        if(dir_it != dirs.end()) {
            // remove directory in the way
            string full_name = concat_paths(path, dir_it->first);
            VPLTRACE_LOG_FINE(TRACE_APP, 0,
                              "Removing directory %s to create file.",
                              full_name.c_str());
            root.remove(full_name, VSSI_GetVersion(handle) + 1);
        }

        map<string, vs_file>::iterator file_it = files.find(element_name);
        if(file_it == files.end()) {
            files[element_name]; // side-effect: create file
            file_it = files.find(element_name);
        }
        element_name = concat_paths(path, name);
        rv = file_it->second.write(handle, element_name, data);
    }
    else {
        // Find next path element, removing any file in the way.
        map<string, vs_file>::iterator file_it = files.find(element_name);
        if(file_it != files.end()) {
            string full_name = concat_paths(path, file_it->first);
            VPLTRACE_LOG_FINE(TRACE_APP, 0,
                              "Removing file %s to create directory.",
                              full_name.c_str());
            root.remove(full_name, VSSI_GetVersion(handle) + 1);
        }

        map<string, vs_dir>::iterator dir_it = dirs.find(element_name);
        if(dir_it == dirs.end()) {
            dirs[element_name]; // side-effect: create directory
            dir_it = dirs.find(element_name);
        }

        string newpath = concat_paths(path, element_name);
        string newname;
        if(slash != string::npos) {
            newname = name.substr(slash + 1);
        }
        rv = dir_it->second.write_file(handle, newpath, newname, data);
    }

    if(rv == 0) {
        version = VSSI_GetVersion(handle);
    }

    return rv;
}

int vs_dir::truncate_file(VSSI_Object handle,
                          const std::string& path,
                          const std::string& name,
                          u64 size)
{
    // When name has no path, truncate the named file, creating it as needed.
    // Otherwise, pass to the next directory in the path.
    // If a path element is a file, delete the file.
    // If a path element is missing, create it.
    // Relay server response back up the chain.

    int rv = 0;
    size_t slash;
    string element_name;

    VPLTRACE_LOG_FINE(TRACE_APP, 0,
                        "Truncating file (%s)/(%s).",
                        path.c_str(), name.c_str());

    slash = name.find_first_of('/');
    element_name = name.substr(0, slash);
    if(slash == string::npos) {
        map<string, vs_dir>::iterator dir_it = dirs.find(element_name);
        if(dir_it != dirs.end()) {
            // remove directory in the way
            string s = concat_paths(path, dir_it->first);
            VPLTRACE_LOG_FINE(TRACE_APP, 0,
                              "Removing directory %s to create file.",
                              s.c_str());
            string full_name = concat_paths(path, dir_it->first);
            root.remove(full_name, VSSI_GetVersion(handle) + 1);
        }

        map<string, vs_file>::iterator file_it = files.find(element_name);
        if(file_it == files.end()) {
            files[element_name]; // side-effect: create file
            file_it = files.find(element_name);
        }
        element_name = concat_paths(path, name);
        rv = file_it->second.truncate(handle, element_name, size);
    }
    else {
        // Find next path element, removing any file in the way.
        map<string, vs_file>::iterator file_it = files.find(element_name);
        if(file_it != files.end()) {
            string s = concat_paths(path, file_it->first);
            VPLTRACE_LOG_FINE(TRACE_APP, 0,
                              "Removing file %s to create directory.",
                              s.c_str());
            string full_name = concat_paths(path, file_it->first);
            root.remove(full_name, VSSI_GetVersion(handle) + 1);
        }

        map<string, vs_dir>::iterator dir_it = dirs.find(element_name);
        if(dir_it == dirs.end()) {
            dirs[element_name]; // side-effect: create directory
            dir_it = dirs.find(element_name);
        }

        string newpath = concat_paths(path, element_name);
        string newname;
        if(slash != string::npos) {
            newname = name.substr(slash + 1);
        }
        rv = dir_it->second.truncate_file(handle, newpath, newname, size);
    }

    if(rv == 0) {
        version = VSSI_GetVersion(handle);
    }

    return rv;
}

int vs_dir::set_times(VSSI_Object handle,
                      const std::string& name,
                      u64 ctime, u64 mtime)
{
    int rv = 0;
    string full_name = name;

    // Chop slashes.
    while(full_name[0] == '/') {
        full_name.erase(0, 1);
    }
    while(full_name[full_name.size() - 1 ] == '/') {
        full_name.erase(full_name.size() - 1, 1);
    }

    // Update timestamps for local state file/dir
    set_times(name, VSSI_GetVersion(handle) + 1, ctime, mtime);

    // issue server command and commit
    VSSI_SetTimes(handle, full_name.c_str(),
                  ctime, mtime,
                  &test_context, pctest_callback);
    VPLSem_Wait(&(test_context.sem));
    rv = test_context.rv;
    if(rv != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "SetTimes component %s: %d.",
                         full_name.c_str(), rv);
    }
    else {
        VSSI_Commit(handle, &test_context, pctest_callback);
        VPLSem_Wait(&(test_context.sem));
        rv = test_context.rv;
        if(rv != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Commit on SetTimes component %s: %d.",
                             full_name.c_str(), rv);
        }
    }

    if(rv == 0) {
        version = VSSI_GetVersion(handle);
    }

    return rv;
}

void vs_dir::set_times(const std::string& name, u64 change_version,
                       u64 ctime, u64 mtime)
{
    // Update the element in the tree, if it exists.
    vs_file* filep = NULL;
    vs_dir* dirp = NULL;
    string full_name = name;

    if(ctime == 0 && mtime == 0) {
        // No times changed. Skip.
        return;
    }

    // Chop slashes.
    while(full_name[0] == '/') {
        full_name.erase(0, 1);
    }
    while(full_name[full_name.size() - 1 ] == '/') {
        full_name.erase(full_name.size() - 1, 1);
    }

    VPLTRACE_LOG_FINE(TRACE_APP, 0,
                      "SetTimes (%s).",
                      name.c_str());

    // Find the named element.
    if(full_name.empty()) {
        // Root directory.
        if(ctime != 0) {
            this->ctime = ctime;
            ctime_explicit = true;
        }
        if(mtime != 0) {
            this->mtime = mtime;
            mtime_explicit = true;
        }
    }
    else {
        filep = find_file("", full_name, change_version);
        if(filep == NULL) {
            dirp = find_dir("", full_name, change_version);
            if(dirp) {
                if(ctime != 0) {
                    dirp->ctime = ctime;
                    dirp->ctime_explicit = true;
                }
                if(mtime != 0) {
                    dirp->mtime = mtime;
                    dirp->mtime_explicit = true;
                }
            }
        }
        else {
            if(ctime != 0) {
                filep->ctime = ctime;
                filep->ctime_explicit = true;
            }
            if(mtime != 0) {
                filep->mtime = mtime;
                filep->mtime_explicit = true;
            }
        }
    }
}

int vs_dir::add_dir(VSSI_Object handle,
                    const std::string& path,
                    const std::string& name)
{
    // When name is empty, create the named directory.
    // Otherwise, pass to the next directory in the path.
    // Delete any files in the way.
    // Issue the command when name is empty (at directory to create)
    // Relay server response back up the chain.

    int rv = 0;

    VPLTRACE_LOG_FINE(TRACE_APP, 0,
                        "Adding directory (%s)/(%s).",
                        path.c_str(), name.c_str());

    if(name.empty()) {

        VSSI_MkDir(handle, path.c_str(),
                   &test_context, pctest_callback);
        VPLSem_Wait(&(test_context.sem));
        rv = test_context.rv;
        if(rv != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Make directory %s: %d.",
                             path.c_str(), rv);
            goto exit;
        }

        VSSI_Commit(handle, &test_context, pctest_callback);
        VPLSem_Wait(&(test_context.sem));
        rv = test_context.rv;
        if(rv != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Commit on make directory %s: %d.",
                             path.c_str(), rv);
            goto exit;
        }
    }
    else {
        size_t slash = name.find_first_of('/');
        string element_name;
        if(slash != string::npos) {
            element_name = name.substr(0, slash);
        }
        else {
            element_name = name;
        }

        map<string, vs_file>::iterator file_it = files.find(element_name);
        if(file_it != files.end()) {
            // File is in the way. Remove it.
            string s = concat_paths(path, file_it->first);
            VPLTRACE_LOG_FINE(TRACE_APP, 0,
                              "Removing file %s to create directory.",
                              s.c_str());
            string full_name = concat_paths(path, file_it->first);
            root.remove(full_name, VSSI_GetVersion(handle) + 1);
        }

        map<string, vs_dir>::iterator dir_it = dirs.find(element_name);
        if(dir_it == dirs.end()) {
            // Create new directory
            dirs[element_name]; // side-effect: create object
            dir_it = dirs.find(element_name);
        }

        // Pass to next directory.
        string newpath = concat_paths(path, element_name);
        string newname;
        if(slash != string::npos) {
            newname = name.substr(slash + 1);
        }
        rv = dir_it->second.add_dir(handle, newpath, newname);
    }

    if(rv == 0) {
        version = VSSI_GetVersion(handle);
    }

 exit:
    return rv;
}

int vs_dir::read_dir(VSSI_Object handle,
                     const std::string& path,
                     const std::string& name)
{
    int rv = 0;

    // When name is empty, verify this directory.
    // Otherwise, pass to the next directory in the path.
    // If the next diretory is non-existent, issue the server command and exit.
    // Relay server response back up the chain.

    VPLTRACE_LOG_FINE(TRACE_APP, 0,
                        "Reading directory (%s)/(%s).",
                        path.c_str(), name.c_str());

    if(name.empty()) {
        // Verify this directory
        rv = verify(handle, path, 0, 0, 0);
    }
    else {
        size_t slash = name.find_first_of('/');
        string element_name = name.substr(0, slash);;

        map<string, vs_dir>::iterator it = dirs.find(element_name);
        if(it != dirs.end()) {
            string newpath = concat_paths(path, element_name);
            string newname;
            if(slash != string::npos) {
                newname = name.substr(slash + 1);
            }
            rv = it->second.read_dir(handle, newpath, newname);
        }
        else {
            // Else no such directory. Might be a file or might not exist.
            // Verify dummy dir contents to get server result.
            vs_dir dummy_dir;
            element_name = concat_paths(path, name);
            rv = dummy_dir.verify(handle, element_name, 0, 0, 0);
        }
    }

    return rv;
}

int vs_dir::read_file(VSSI_Object handle,
                      const std::string& path,
                      const std::string& name)
{
    int rv = 0;
    size_t slash;
    string element_name;

    // When name has no path, read the named file.
    // Otherwise, pass to the next directory in the path.
    // If the next diretory is non-existent, issue the server command and exit.
    // Relay server response back up the chain.

    VPLTRACE_LOG_FINE(TRACE_APP, 0,
                        "Reading file (%s)/(%s).",
                        path.c_str(), name.c_str());

    slash = name.find_first_of('/');
    if(slash == string::npos) {
        element_name = concat_paths(path, name);
        // continue on to verify file contents
        map<string, vs_file>::iterator it = files.find(name);
        if(it != files.end()) {
            // verify the real file
            rv = it->second.verify(handle, element_name, 
                                   NULL, NULL, 0, 0, 0);
        }
        else {
            // No such file.
            // Verify dummy file contents to get server result.
            vs_file dummy_file;
            rv = dummy_file.verify(handle, element_name, NULL, NULL, 0, 0, 0);
        }
    }
    else {
        // Find next path element
        element_name = name.substr(0, slash);
        map<string, vs_dir>::iterator it = dirs.find(element_name);
        if(it != dirs.end()) {
            string newpath = concat_paths(path, element_name);
            string newname;
            if(slash != string::npos) {
                newname = name.substr(slash + 1);
            }
            rv = it->second.read_file(handle, newpath, newname);
        }
        else {
            // Else no such directory. Might be a file or might not exist.
            // Verify dummy file contents to get server result.
            vs_file dummy_file;
            element_name = concat_paths(path, name);
            rv = dummy_file.verify(handle, element_name, NULL, NULL, 0, 0, 0);
        }
    }

    return rv;
}

void vs_dir::remove(const std::string& name, u64 change_version)
{
    // Remove the element from the tree.
    vs_file* filep = NULL;
    vs_dir* dirp = NULL;
    string full_name = name;

    // Chop slashes.
    while(full_name[0] == '/') {
        full_name.erase(0, 1);
    }
    while(full_name[full_name.size() - 1 ] == '/') {
        full_name.erase(full_name.size() - 1, 1);
    }

    VPLTRACE_LOG_FINE(TRACE_APP, 0,
                      "Removing (%s).",
                      name.c_str());

    // If directory is root and has no contents, do not move.
    if(name.empty() && dirs.empty() && files.empty()) {
        return;
    }

    // Find the named element.
    if(full_name.empty()) {
        // Remove the whole dataset!
        dirp = new vs_dir;
        dirp->version = version;
        dirp->dirs = dirs;
        dirp->files = files;
        // Clear the root.
        dirs.clear();
        files.clear();
    }
    else {
        filep = get_file("", full_name, change_version);
        if(filep == NULL) {
            dirp = get_dir("", full_name, change_version);
        }
    }

    if(filep) {
        delete filep;
    }
    else if(dirp) {
        delete dirp;
    }
}

int vs_dir::remove(VSSI_Object handle,
                   const std::string& name)
{
    int rv = 0;
    string full_name = name;

    // Chop slashes.
    while(full_name[0] == '/') {
        full_name.erase(0, 1);
    }
    while(full_name[full_name.size() - 1 ] == '/') {
        full_name.erase(full_name.size() - 1, 1);
    }

    // Remove from local state.
    remove(name, VSSI_GetVersion(handle) + 1);

    // issue server command and commit
    VSSI_Remove(handle, full_name.c_str(),
                &test_context, pctest_callback);
    VPLSem_Wait(&(test_context.sem));
    rv = test_context.rv;
    if(rv != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Remove component %s: %d.",
                         full_name.c_str(), rv);
    }
    else {
        VSSI_Commit(handle, &test_context, pctest_callback);
        VPLSem_Wait(&(test_context.sem));
        rv = test_context.rv;
        if(rv != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Commit on Remove component %s: %d.",
                             full_name.c_str(), rv);
        }
    }

    if(rv == 0) {
        version = VSSI_GetVersion(handle);
    }

    return rv;
}

int vs_dir::rename(VSSI_Object handle,
                   const std::string& name,
                   const std::string& new_name)
{
    int rv = 0;
    string root = "";
    vs_file* filep = NULL;
    vs_dir* dirp = NULL;
    string temp_name = name + "/";
    string temp_new_name = new_name + "/";

    VPLTRACE_LOG_FINE(TRACE_APP, 0,
                        "Renaming %s to %s.",
                        name.c_str(), new_name.c_str());

    // If they are identical, take no local action.   This succeeds
    // but is a NOP on the server side.
    if(temp_name.compare(temp_new_name) == 0) goto issue_command;

    // If new_name and name have intersecting paths take no local action,
    if(temp_new_name.compare(0, temp_name.size(), temp_name) == 0 ||
        temp_name.compare(0, temp_new_name.size(), temp_new_name) == 0) {
        VPLTRACE_LOG_INFO(TRACE_APP, 0,
                          "Renaming %s to %s. will fail: intersect",
                          name.c_str(), new_name.c_str());
        goto issue_command;
    }

    // Find the named element.
    filep = get_file(root, name, VSSI_GetVersion(handle) + 1);
    if(filep == NULL) {
        dirp = get_dir(root, name, VSSI_GetVersion(handle) + 1);
    }

    // Place element back in tree at new name.
    if(filep) {
        put_file(filep, root, new_name, VSSI_GetVersion(handle) + 1);
        delete filep;
    }
    else if(dirp) {
        put_dir(dirp, root, new_name, VSSI_GetVersion(handle) + 1);
        delete dirp;
    }
    // else, name wasn't in tree, so no action.

 issue_command:
    // See what the server has to say.
    VSSI_Rename(handle, name.c_str(), new_name.c_str(),
                &test_context, pctest_callback);
    VPLSem_Wait(&(test_context.sem));
    rv = test_context.rv;
    if(rv != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0,
                         "Rename component %s to %s: %d.",
                         name.c_str(), new_name.c_str(), rv);
    }
    else {
        VSSI_Commit(handle, &test_context, pctest_callback);
        VPLSem_Wait(&(test_context.sem));
        rv = test_context.rv;
        if(rv != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Commit on rename %s to %s: %d.",
                             name.c_str(), new_name.c_str(), rv);
        }
    }

    if(rv == 0) {
        version = VSSI_GetVersion(handle);
    }

    return rv;
}

int vs_dir::copy(VSSI_Object handle,
                 const std::string& source,
                 const std::string& destination)
{
    int rv = 0;
    string root = "";
    vs_file* filep = NULL;
    vs_dir* dirp = NULL;
    string temp_name = source + "/";
    string temp_new_name = destination + "/";

    VPLTRACE_LOG_FINE(TRACE_APP, 0,
                      "Copying %s to %s.",
                      source.c_str(), destination.c_str());

    // If new_name and name have intersecting paths take no local action.
    if(temp_new_name.compare(0, temp_name.size(), temp_name) == 0 ||
       temp_name.compare(0, temp_new_name.size(), temp_new_name) == 0) {
        VPLTRACE_LOG_INFO(TRACE_APP, 0,
                          "Copying %s to %s. will fail: intersect",
                          source.c_str(), destination.c_str());
        goto issue_command;
    }

    // Find the named element.
    filep = copy_file(root, source);
    if(filep == NULL) {
        dirp = copy_dir(root, source, VSSI_GetVersion(handle) + 1);
    }

    // Place element back in tree at new name.
    if(filep) {
        put_file(filep, root, destination, VSSI_GetVersion(handle) + 1);
        delete filep;
    }
    else if(dirp) {
        put_dir(dirp, root, destination, VSSI_GetVersion(handle) + 1);
        delete dirp;
    }
    // else, name wasn't in tree, so no action.

 issue_command:
    // See what the server has to say.
    VSSI_Copy(handle, source.c_str(), destination.c_str(),
              &test_context, pctest_callback);
    VPLSem_Wait(&(test_context.sem));
    rv = test_context.rv;
    if(rv != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0,
                         "Copy component %s to %s: %d.",
                         source.c_str(), destination.c_str(), rv);
    }
    else {
        VSSI_Commit(handle, &test_context, pctest_callback);
        VPLSem_Wait(&(test_context.sem));
        rv = test_context.rv;
        if(rv != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Commit on copy %s to %s: %d.",
                             source.c_str(), destination.c_str(), rv);
        }
    }

    if(rv == 0) {
        version = VSSI_GetVersion(handle);
    }

    return rv;
}

int vs_dir::copy_match(VSSI_Object handle,
                       const std::string& source,
                       const std::string& destination)
{
    int rv = 0;
    string root = "";
    vs_file* filep = NULL;
    char signature[20] = "invalid signature..";
    u64 size = 0;

    VPLTRACE_LOG_FINE(TRACE_APP, 0,
                      "Copying %s to %s by hash/size reference.",
                      source.c_str(), destination.c_str());

    // Find the named element.
    filep = copy_file(root, source);
    if(filep == NULL) {
        // Can't do. Not a match.
        goto issue_command;
    }

    // Place element back in tree at new name. Remove any metadata.
    filep->metadata.clear();
    put_file(filep, root, destination, VSSI_GetVersion(handle) + 1);
    memcpy(signature, filep->signature, 20);
    size = filep->contents.size();

 issue_command:
    // See what the server has to say.
    VPLTRACE_LOG_INFO(TRACE_APP, 0,
                      "Copying %s to %s by hash/size "FMTu64" reference.",
                      source.c_str(), destination.c_str(), size);
    VPLTRACE_DUMP_BUF_INFO(TRACE_APP, 0,
                           signature, 20);
    VSSI_CopyMatch(handle, size, signature, destination.c_str(),
                   &test_context, pctest_callback);
    VPLSem_Wait(&(test_context.sem));
    rv = test_context.rv;
    if(rv != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0,
                         "Copy component %s to %s by hash/size: %d.",
                         source.c_str(), destination.c_str(), rv);
    }
    else {
        VSSI_Commit(handle, &test_context, pctest_callback);
        VPLSem_Wait(&(test_context.sem));
        rv = test_context.rv;
        if(rv != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Commit on copy %s to %s by hash/size: %d.",
                             source.c_str(), destination.c_str(), rv);
        }
    }

    if(rv == 0) {
        version = VSSI_GetVersion(handle);
    }

    return rv;
}

int vs_dir::get_space(VSSI_Object handle)
{
    int rv = 0;
    u64 disk_size, dataset_size, avail_size;

    VPLTRACE_LOG_FINE(TRACE_APP, 0,
                      "Getting disk space information.");

    VSSI_GetSpace(handle, &disk_size, &dataset_size, &avail_size,
                   &test_context, pctest_callback);
    VPLSem_Wait(&(test_context.sem));
    rv = test_context.rv;
    if(rv != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0,
                         "Getting disk space information: %d.", rv);
    } else {
        VPLTRACE_LOG_INFO(TRACE_APP, 0,
                          "Disk size: "FMTu64", datasize size: "FMTu64", avail size: "FMTu64".",
                          disk_size, dataset_size, avail_size);
    }

    return rv;
}

vs_file* vs_dir::get_file(const std::string& path,
                          const std::string& name,
                          u64 change_version)
{
    VPLTRACE_LOG_FINE(TRACE_APP, 0,
                        "Get file (%s)/(%s).",
                        path.c_str(), name.c_str());

    vs_file* rv = NULL;
    size_t slash;
    string element_name;

    slash = name.find_first_of('/');
    if(slash == string::npos) {
        map<string, vs_file>::iterator file_it = files.find(name);
        if(file_it != files.end()) {
            rv = new vs_file(file_it->second);
            files.erase(file_it);
        }
    }
    else {
        element_name = name.substr(0, slash);
        string newpath = concat_paths(path, element_name);
        string newname = name.substr(slash + 1);

        map<string, vs_dir>::iterator dir_it = dirs.find(element_name);
        if(dir_it != dirs.end()) {
            rv = dir_it->second.get_file(newpath, newname, change_version);
        }
    }

    if(rv != NULL) {
        version = change_version;
    }

    return rv;
}

vs_file* vs_dir::find_file(const std::string& path,
                           const std::string& name,
                           u64 change_version)
{
    VPLTRACE_LOG_FINE(TRACE_APP, 0,
                        "Find file (%s)/(%s).",
                        path.c_str(), name.c_str());

    vs_file* rv = NULL;
    size_t slash;
    string element_name;

    slash = name.find_first_of('/');
    if(slash == string::npos) {
        map<string, vs_file>::iterator file_it = files.find(name);
        if(file_it != files.end()) {
            rv = &(file_it->second);
            rv->version = change_version;
        }
    }
    else {
        element_name = name.substr(0, slash);
        string newpath = concat_paths(path, element_name);
        string newname = name.substr(slash + 1);

        map<string, vs_dir>::iterator dir_it = dirs.find(element_name);
        if(dir_it != dirs.end()) {
            rv = dir_it->second.find_file(newpath, newname, change_version);
        }
    }

    if(rv != NULL) {
        version = change_version;
    }

    return rv;
}

vs_file* vs_dir::copy_file(const std::string& path,
                           const std::string& name)
{
    VPLTRACE_LOG_FINE(TRACE_APP, 0,
                        "Copy file (%s)/(%s).",
                        path.c_str(), name.c_str());

    vs_file* rv = NULL;
    size_t slash;
    string element_name;

    slash = name.find_first_of('/');
    if(slash == string::npos) {
        map<string, vs_file>::iterator file_it = files.find(name);
        if(file_it != files.end()) {
            rv = new vs_file(file_it->second);
        }
    }
    else {
        element_name = name.substr(0, slash);
        string newpath = concat_paths(path, element_name);
        string newname = name.substr(slash + 1);

        map<string, vs_dir>::iterator dir_it = dirs.find(element_name);
        if(dir_it != dirs.end()) {
            rv = dir_it->second.copy_file(newpath, newname);
        }
    }

    return rv;
}

vs_dir* vs_dir::get_dir(const std::string& path,
                        const std::string& name,
                        u64 change_version)
{
    VPLTRACE_LOG_FINE(TRACE_APP, 0,
                        "Get dir (%s)/(%s).",
                        path.c_str(), name.c_str());

    vs_dir* rv = NULL;
    size_t slash;
    string element_name;

    slash = name.find_first_of('/');
    if(slash == string::npos) {
        map<string, vs_dir>::iterator dir_it = dirs.find(name);
        if(dir_it != dirs.end()) {
            rv = new vs_dir(dir_it->second);
            dirs.erase(dir_it);
            rv->update_version(change_version);
        }
    }
    else {
        element_name = name.substr(0, slash);
        string newpath = concat_paths(path, element_name);
        string newname = name.substr(slash + 1);

        map<string, vs_dir>::iterator dir_it = dirs.find(element_name);
        if(dir_it != dirs.end()) {
            rv = dir_it->second.get_dir(newpath, newname, change_version);
        }
    }

    if(rv != NULL) {
        version = change_version;
    }

    return rv;
}

vs_dir* vs_dir::find_dir(const std::string& path,
                        const std::string& name,
                        u64 change_version)
{
    VPLTRACE_LOG_FINE(TRACE_APP, 0,
                        "Find dir (%s)/(%s).",
                        path.c_str(), name.c_str());

    vs_dir* rv = NULL;
    size_t slash;
    string element_name;

    slash = name.find_first_of('/');
    if(slash == string::npos) {
        map<string, vs_dir>::iterator dir_it = dirs.find(name);
        if(dir_it != dirs.end()) {
            rv = &(dir_it->second);
            rv->version = change_version;
        }
    }
    else {
        element_name = name.substr(0, slash);
        string newpath = concat_paths(path, element_name);
        string newname = name.substr(slash + 1);

        map<string, vs_dir>::iterator dir_it = dirs.find(element_name);
        if(dir_it != dirs.end()) {
            rv = dir_it->second.find_dir(newpath, newname, change_version);
        }
    }

    if(rv != NULL) {
        version = change_version;
    }

    return rv;
}

vs_dir* vs_dir::copy_dir(const std::string& path,
                         const std::string& name,
                         u64 change_version)
{
    VPLTRACE_LOG_FINE(TRACE_APP, 0,
                        "Copy dir (%s)/(%s).",
                        path.c_str(), name.c_str());

    vs_dir* rv = NULL;
    size_t slash;
    string element_name;

    slash = name.find_first_of('/');
    if(slash == string::npos) {
        map<string, vs_dir>::iterator dir_it = dirs.find(name);
        if(dir_it != dirs.end()) {
            rv = new vs_dir(dir_it->second);
            rv->update_version(change_version);
        }
    }
    else {
        element_name = name.substr(0, slash);
        string newpath = concat_paths(path, element_name);
        string newname = name.substr(slash + 1);

        map<string, vs_dir>::iterator dir_it = dirs.find(element_name);
        if(dir_it != dirs.end()) {
            rv = dir_it->second.copy_dir(newpath, newname, change_version);
        }
    }

    return rv;
}

void vs_dir::put_file(vs_file* file,
                      const std::string& path,
                      const std::string& name,
                      u64 change_version)
{
    VPLTRACE_LOG_FINE(TRACE_APP, 0,
                      "Put file (%s)/(%s) with change version "FMTu64".",
                      path.c_str(), name.c_str(), change_version);

    size_t slash;
    string element_name;

    slash = name.find_first_of('/');
    if(slash == string::npos) {
        // remove any conflicting directory
        map<string, vs_dir>::iterator dir_it = dirs.find(name);
        if(dir_it != dirs.end()) {
            string full_name = concat_paths(path, dir_it->first);
            root.remove(full_name, change_version);
        }

        map<string, vs_file>::iterator file_it = files.find(name);
        if(file_it != files.end()) {
            string full_name = concat_paths(path, file_it->first);
            root.remove(full_name, change_version);
        }
        // add the file
        files[name] = *file;

        // Update version expectation for the file.
        files[name].version = change_version;
    }
    else {
        element_name = name.substr(0, slash);
        string newpath = concat_paths(path, element_name);
        string newname = name.substr(slash + 1);

        // remove any conflicting file
        map<string, vs_file>::iterator file_it = files.find(element_name);
        if(file_it != files.end()) {
            string full_name = concat_paths(path, file_it->first);
            root.remove(full_name, change_version);
        }

        // add any needed dir
        map<string, vs_dir>::iterator dir_it = dirs.find(element_name);
        if(dir_it == dirs.end()) {
            dirs[element_name]; // side-effect: create object
            dir_it = dirs.find(element_name);
        }
        // pass to next directory
        dir_it->second.put_file(file, newpath, newname, change_version);
    }

    version = change_version;
}

void vs_dir::put_dir(vs_dir* dir,
                     const std::string& path,
                     const std::string& name,
                     u64 change_version)
{
    VPLTRACE_LOG_FINE(TRACE_APP, 0,
                        "Put dir (%s)/(%s).",
                        path.c_str(), name.c_str());

    size_t slash;
    string element_name;

    slash = name.find_first_of('/');
    if(slash == string::npos) {
        // remove any conflicting file
        map<string, vs_file>::iterator file_it = files.find(name);
        if(file_it != files.end()) {
            string full_name = concat_paths(path, file_it->first);
            root.remove(full_name, change_version);
        }

        map<string, vs_dir>::iterator dir_it = dirs.find(name);
        if(dir_it != dirs.end()) {
            string full_name = concat_paths(path, dir_it->first);
            root.remove(full_name, change_version);
        }
        // add the directory
        dirs[name] = *dir;

        dirs[name].version = change_version;
    }
    else {
        element_name = name.substr(0, slash);
        string newpath = concat_paths(path, element_name);
        string newname = name.substr(slash + 1);

        // remove any conflicting file
        map<string, vs_file>::iterator file_it = files.find(element_name);
        if(file_it != files.end()) {
            root.remove(newpath, change_version);
        }

        // add any needed dir
        map<string, vs_dir>::iterator dir_it = dirs.find(element_name);
        if(dir_it == dirs.end()) {
            dirs[element_name]; // side-effect: create object
            dir_it = dirs.find(element_name);
        }
        // pass to next directory
        dir_it->second.put_dir(dir, newpath, newname, change_version);
    }

    version = change_version;
}

void vs_dir::update_version(u64 change_version)
{
    map<string, vs_dir>::iterator dir_it;
    for (dir_it = dirs.begin(); dir_it != dirs.end(); dir_it++) {
        dir_it->second.update_version(change_version);
    }
    map<string, vs_file>::iterator file_it;
    for (file_it = files.begin(); file_it != files.end(); file_it++) {
        file_it->second.update_version(change_version);
    }
    version = change_version;
}

void vs_dir::reset()
{
    dirs.clear();
    files.clear();
}
