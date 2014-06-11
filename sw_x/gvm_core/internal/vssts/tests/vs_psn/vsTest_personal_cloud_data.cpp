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

#include "vssts.hpp"
#include "vssts_error.hpp"

#include "vsTest_personal_cloud_data.hpp"

#include <iostream>
#include <openssl/sha.h>

using namespace std;

extern const char* vsTest_curTestName;

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

/// Objects to use personal cloud tests.
static VSSI_Object handles[2];
static vs_dir root;

static const char vsTest_personal_cloud_data [] = "Personal Cloud Data Test";
int test_personal_cloud(
                        u64 user_id,
                        u64 dataset_id,
                        bool run_cloudnode_tests)
{
    int rv = 0;
    int rc;
    int open_flags = VSSI_READWRITE | VSSI_FORCE;

    const string path = ""; // root path

    /// test step types
    enum {
        WRITE,
        TRUNCATE,
        MKDIR,
        READDIR,
        READ,
        RENAME,
        COPY,
        REMOVE,
        GET_SPACE,
        VERIFY,
        SET_CTIME, // set only ctime
        SET_MTIME, // set only mtime
        SET_CMTIME, // set both to the same time
        ABORT // stop testing immediately
    };
    static const char* step_names[] = {
        "Write File",
        "Truncate File",
        "Make Directory",
        "Read Directory",
        "Read File",
        "Rename",
        "Copy",
        "Remove",
        "Get Space",
        "Clear Conflict",
        "Verify",
        "Set Create Time",
        "Set Modify Time",
        "Set Create and Modify Times",
        "Abort"
    };

    /// test instructions
    struct test_instruction {
        int type; // type of step.
        int object; // which object handle to use
        int expect; // expected result
        const char* name; // component to effect.
        const char* data; // if writing a file, data to write.
        // If renaming a file or directory, the new name
        u64 size; // if truncating, size to set. Also timestamp for SET_*TIME.
    };
    struct test_instruction regular_steps[] = {
        {READDIR, 0, 0, "",      "",           0}, // Read empty root directory.
        {READDIR, 1, 0, "",      "",           0}, // Read empty root directory.
        {WRITE,   0, 0, "a",     "File A",     0}, // write file
        {READ,    1, 0, "a", "",   0},             // Detect change via conflict.
        {READ,    1, VSSI_NOTFOUND, "b", "",   0}, // Detect conflict before other errors.
        // {VERIFY,  1, 0, "",      "",           0}, // Verify via alt. object handle
        {READDIR, 1, 0, "",      "",           0}, // Detect change via conflict.
        {MKDIR,   0, 0, "b",     "",           0}, // make dir
        {WRITE,   0, 0, "b/a",   "File B/A",   0}, // write file in dir
        {MKDIR,   0, 0, "b/b",   "",           0}, // make dir
        {WRITE,   0, 0, "b/b/a", "File B/B/A", 0}, // implicit make dir for file. (pathname write is deprecated, which implicit mkdir -p)
        {MKDIR,   0, 0, "c",     "",           0}, // make dir
        {MKDIR,   0, 0, "c/a",   "",           0}, // make dir
        {WRITE,   0, 0, "c/a/a", "File C/A/A", 0}, // implicit make dir for file. (pathname write is deprecated, which implicit mkdir -p)
        {MKDIR,   0, 0, "d",     "",           0}, // make dir
        {MKDIR,   0, 0, "d/e",   "",           0}, // make dir
        {MKDIR,   0, 0, "d/e/f",   "",          0}, // make dir
        {MKDIR,   0, 0, "d/e/f/g", "",         0}, // mkdir dir
        {TRUNCATE,0, 0, "q/r/s", "",           1024},// set size without writing file first

        {MKDIR,   0, VSSI_BADPATH, "implicit/implicit", "", 0}, // neg: mkdir dir -p
        {MKDIR,   0, VSSI_BADPATH, "b/implicit/implicit", "", 0}, // neg: mkdir dir -p
        {READ,    0, VSSI_BADPATH, "b/a/x", "", 0}, // neg: file in path
        {READ,    0, VSSI_NOTFOUND, "b/x",   "", 0}, // neg: nonexistent file
        {READ,    0, VSSI_ISDIR, "b",     "", 0},  // neg: dir isn't file
        {READDIR, 0, VSSI_NOTDIR, "a",     "", 0}, // neg: file isn't dir
        {READDIR, 0, VSSI_NOTFOUND, "b/x",   "", 0}, // neg: nonexistent dir
        {READDIR, 0, VSSI_NOTDIR, "b/a/a", "", 0}, // neg: non-dir path
        {READDIR, 1, 0, "", "",   0}, // Detect change via conflict.
        {READDIR,    1, VSSI_NOTFOUND, "notadir", "", 0}, // Detect conflict before other errors.
        // {VERIFY,  1, 0, "",      "",           0}, // Verify via alt. object handle
        {REMOVE,  0, 0, "b",     "",           0}, // rm -rf a directory, recursive remove is done by the test function itself
        {REMOVE,  0, 0, "a",     "",           0}, // rm -rf a file
        {WRITE,   0, 0, "d/a",   "Short-lived file", 0}, // write
        {WRITE, 0, 0, "emptyfile", "", 0}, // write/verify empty files
        {MKDIR,   0, 0, "my",   "",           0}, // make dir
        {WRITE, 0, 0, "my/largefile", "Here is some base data for a large file. At least 80 bytes. 12345678901234567890123456789012345678901234567890123456789012345678901234567890", 0}, // write data for truncation test.
        {TRUNCATE, 0, 0, "my/largefile", "", 80}, // truncate file, leaving some data
        {MKDIR, 0, 0, "my/dir", "", 0}, // Make directory tree for further tests
        {MKDIR, 0, 0, "my/dir/and", "", 0}, // Make directory tree for further tests
        {MKDIR, 0, 0, "my/dir/and/file", "", 0}, // Make directory tree for further tests
        {REMOVE,  0, 0, "",      "",           0}, // erase all contents, recursive remove is done by the test function itself
        {MKDIR, 0, 0, "a", "", 0}, // data for rename tests
        {MKDIR, 0, VSSI_ISDIR, "a", "", 0}, // mkdir existing dir
        {MKDIR, 0, 0, "a/b", "", 0}, // data for rename tests
        {MKDIR, 0, 0, "a/b/c", "", 0}, // data for rename tests
        {WRITE, 0, 0, "a/b/file", "Some file", 0}, // data for rename tests. (pathname write is deprecated, which implicit mkdir -p)
        {WRITE, 0, 0, "a/b/other", "another file", 0}, // data for rename tests. (pathname write is deprecated, which implicit mkdir -p)
        {MKDIR, 0, VSSI_FEXISTS, "a/b/file", "", 0}, // mkdir of existing file
        {RENAME, 0, VSSI_NOTFOUND, "foo", "foo", 0}, // non-existent -> self
        {RENAME, 0, VSSI_NOTFOUND, "foo", "bar", 0}, // non-existent -> non-existent
        {RENAME, 0, VSSI_FEXISTS, "a/b/other", "a/b/file", 0}, // existing file -> existing file
        {RENAME, 0, VSSI_ISDIR, "a/b/other", "a/b/c", 0}, // existing file -> existing dir
        {RENAME, 0, VSSI_INVALID, "a/b", "a", 0}, // existing dir -> higher dir
        {RENAME, 0, VSSI_INVALID, "a/b", "a/b/d", 0}, // existing -> nested dir
        {RENAME, 0, 0, "a/b/file", "a/b/file", 0}, // existing file -> self
        {RENAME, 0, 0, "a/b/file", "bar", 0}, // existing file -> non-existent
        {RENAME, 0, VSSI_FEXISTS, "bar", "a/b/other", 0}, // existing file -> existing file
        {RENAME, 0, 0, "bar", "a/file", 0}, // existing file -> non-existing file
        {RENAME, 0, VSSI_ISDIR, "a/file", "a/b/c", 0}, // existing file -> existing dir
        {MKDIR, 0, 0, "a/b/dir", "", 0}, // data for rename tests
        {RENAME, 0, VSSI_ISDIR, "a/b/c", "a/b/dir", 0}, // existing dir -> existing dir
        {RENAME, 0, 0, "a/file", "a/b/dir/other", 0}, // existing file -> lower dir
        {RENAME, 0, VSSI_BADPATH, "a/b/other", "a/b/dir/c/other", 0}, // existing file -> lower non-existing dir
        {RENAME, 0, VSSI_FEXISTS, "a/b/other", "a/b/dir/other", 0}, //existing file  -> lower dir, existing file
        {RENAME, 0, VSSI_BADPATH, "a/b/other", "a/b/implicitdir/implicitdir/other", 0}, // neg: target implicit mkdir -p
        {MKDIR, 0, 0, "aa", "", 0}, // data for rename tests
        {MKDIR, 0, 0, "aa/b", "", 0}, // data for rename tests
        {MKDIR, 0, 0, "aa/b/c", "", 0}, // data for rename tests
        {MKDIR, 0, 0, "aa/b/c/d", "", 0}, // data for rename tests
        {WRITE, 0, 0, "aa/b/c/file", "Some file", 0}, // data for rename tests. (pathname write is deprecated, which implicit mkdir -p)
        {WRITE, 0, 0, "aa/b/c/other", "Another file", 0}, // data for rename tests. (pathname write is deprecated, which implicit mkdir -p)
        {RENAME, 0, 0, "aa/b/c", "bar", 0}, // existing dir -> non-existent dir
        {RENAME, 0, 0, "bar/other", "aa/b/other", 0}, // existing file -> non-existent file
        {RENAME, 0, 0, "bar", "aa/b/c", 0}, // existing dir -> non-existent dir
        {RENAME, 0, VSSI_NOTFOUND, "bar", "aa/b/c", 0}, // non-existing dir -> existing dir
        {RENAME, 0, VSSI_ISDIR, "aa/b/other", "aa/b/c/d", 0}, // existing file -> existing dir
        {RENAME, 0, VSSI_INVALID, "aa/b/c", "aa/b/c/d", 0}, // existing dir -> nested dir
        {RENAME, 0, VSSI_INVALID, "aa/b", "aa/b/c/file", 0}, // existing dir -> nested dir
        {RENAME, 0, VSSI_INVALID, "aa/b/c/d", "aa/b", 0}, //existing dir  -> higher dir
        {RENAME, 0, VSSI_INVALID, "aa/b/c/d", "aa/b", 0}, //existing dir  -> higher dir
        {RENAME, 0, VSSI_BADPATH, "aa/b/c/d", "aa/b/c/implicitdir/implicitdir/other", 0}, // neg: target implicit mkdir -p
        {RENAME, 0, VSSI_INVALID, "aa/b/c/d", "aa/b/c/d/implicitdir/implicitdir/other", 0}, // neg: move to self lower dir
        {MKDIR, 0, 0, "name", "", 0}, // data for rename tests
        {MKDIR, 0, 0, "name2", "", 0}, // data for rename tests
        {RENAME, 0, 0, "name", "name2/name", 0}, // move into dir, src is substring of dest
        {RENAME, 0, 0, "name2/name", "nam", 0}, // move into dir, dest is substring of source
        {REMOVE,  0, 0, "",      "",           0}, // erase all contents, recursive remove is done by the test function itself
        {WRITE, 0, 0, "a/b/file", "Some file", 0}, // data for copy tests. (pathname write is deprecated, which implicit mkdir -p)
        {WRITE, 0, 0, "a/b/other", "another file", 0}, // data for copy tests. (pathname write is deprecated, which implicit mkdir -p)
        {MKDIR, 0, 0, "a", "", 0}, // data for copy tests
        {MKDIR, 0, 0, "a/b", "", 0}, // data for copy tests
        {MKDIR, 0, 0, "a/b/c", "", 0}, // data for copy tests
#ifdef COPY_FIXED_METADATA
        {COPY, 0, VSSI_INVALID, "foo", "foo", 0}, // non-existent -> self (empty op)
        {COPY, 0, VSSI_NOTFOUND, "foo", "bar", 0}, // non-existent -> non-existent
        {COPY, 0, VSSI_NOTFOUND, "foo", "a/b/file", 0}, // non-existent -> existing file
        {COPY, 0, VSSI_NOTFOUND, "foo", "a/b", 0}, // non-existent -> existing dir
        {COPY, 0, VSSI_NOTFOUND, "foo/a/b", "foo/a", 0}, // non-existent -> higher dir
        {COPY, 0, VSSI_NOTFOUND, "foo/a/b", "foo/a/b/c", 0}, // non-existent -> nested dir
        {COPY, 0, VSSI_INVALID, "a/b/file", "a/b/file", 0}, // existing file -> self (empty op)
        {COPY, 0, 0, "a/b/file", "bar", 0}, // existing file -> non-existent
        {COPY, 0, 0, "bar", "a/b/other", 0}, // existing file -> existing file
        {COPY, 0, 0, "a/b/other", "a/b/c", 0}, // existing file -> existing dir
        {COPY, 0, 0, "a/b/c", "a/b/dir/c", 0}, // existing file -> lower dir
        {WRITE, 0, 0, "a/b/other", "this file again", 0}, // data for copy tests. (pathname write is deprecated, which implicit mkdir -p)
        {COPY, 0, 0, "a/b/other", "a/b/dir/c/other", 0}, // existing file -> lower dir, stomp file
        {COPY, 0, 0, "a/b/dir/c/other", "a/b/dir/other", 0}, //existing file  -> higher dir
        {WRITE, 0, 0, "aa/b/c/file", "Some file", 0}, // data for copy tests. (pathname write is deprecated, which implicit mkdir -p)
        {WRITE, 0, 0, "aa/b/c/other", "another file", 0}, // data for copy tests. (pathname write is deprecated, which implicit mkdir -p)
        {MKDIR, 0, 0, "aa/b/c/d", "", 0}, // data for copy tests. (pathname write is deprecated, which implicit mkdir -p)
        // {COPY, 0, VSSI_INVALID, "aa/b/c", "aa/b/c", 0}, // existing dir -> self (empty op)
        // {COPY, 0, 0, "aa/b/c", "bar", 0}, // existing dir -> non-existent
        /// Failed to copy metadata for bar/file

        // {COPY, 0, 0, "bar", "aa/b/c/other", 0}, // existing dir -> existing file
        // {COPY, 0, 0, "aa/b/c/other", "aa/b/c/d", 0}, // existing dir -> existing dir
        // {COPY, 0, VSSI_INVALID, "aa/b/c", "aa/b/c/d", 0}, // existing dir -> nested dir
        // {COPY, 0, VSSI_INVALID, "aa/b", "aa/b/c/file", 0}, // existing dir -> nested dir, stomp file
        // {COPY, 0, VSSI_INVALID, "aa/b/c/d", "aa/b", 0}, //existing dir  -> higher dir
#endif // COPY_FIXED_METADATA

        // Set-times APIs
        {WRITE, 0, 0, "dir/file", "A test file", 0}, // File and directory for set-times tests. (pathname write is deprecated, which implicit mkdir -p)
        {SET_CTIME, 0, 0, "notfound", "", 12345678}, // Non-existent file
        {SET_MTIME, 0, 0, "notfound", "", 12345678}, // Non-existent file
        {SET_CMTIME, 0, 0, "notfound", "", 12345678}, // Non-existent file
        {SET_CMTIME, 0, 0, "notfound", "", 0}, // Non-existent file, no times set
        {SET_CTIME, 0, 0, "dir/file", "", 22345678}, // Existing file
        {SET_MTIME, 0, 0, "dir/file", "", 32345678}, // Existing file
        {SET_CMTIME, 0, 0, "dir/file", "", 42345678}, // Existing file
        {SET_CMTIME, 0, 0, "dir/file", "", 0}, // Existing file, no times set
        {SET_CTIME, 0, 0, "dir", "", 52345678}, // Existing directory
        {SET_MTIME, 0, 0, "dir", "", 62345678}, // Existing directory
        {SET_CMTIME, 0, 0, "dir", "", 72345678}, // Existing directory
        {SET_CMTIME, 0, 0, "dir", "", 0}, // Existing directory, no times set

        // Shell quote characters in filenames
        {WRITE, 0, 0, "File with spaces", "data", 0},
        {REMOVE,  0, 0, "File with spaces",      "",           0},
        {WRITE, 0, 0, "FileWith.InMiddle", "data", 0},
        {REMOVE,  0, 0, "FileWith.InMiddle",      "",           0},

    };
    struct test_instruction cloudnode_steps[] = {
        {GET_SPACE, 0, 0, "",    "",           0}, // Get disk space information.
    };
    u32 num_regular_steps, num_cloudnode_steps;
    struct test_instruction* steps;
    bool running_cloudnode_steps = false;

    num_regular_steps = sizeof(regular_steps) / sizeof(regular_steps[0]);
    num_cloudnode_steps = sizeof(cloudnode_steps) / sizeof(cloudnode_steps[0]);

    vsTest_curTestName = vsTest_personal_cloud_data;
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Starting test: %s.",
                        vsTest_curTestName);

    VPL_SET_UNINITIALIZED(&(test_context.sem));

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "BEFORE VPL_Sem_Init ");

    if(VPLSem_Init(&(test_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create sempahore.");
        return -1;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "AFTER VPL_Sem_Init ");

    root.reset();

    VSSI_Delete_Deprecated(user_id, dataset_id,
                         &test_context, pctest_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_Delete: %d.",
                         rc);
        rv++;
        goto fail_open;
    }

    // Open dataset
    VSSI_OpenObjectTS(user_id, dataset_id,
                         open_flags, &handles[0],
                         &test_context, pctest_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenObjectTS: %d.",
                         rc);
        rv++;
        goto fail_open;
    }

    // Open alternate handle to same dataset
    VSSI_OpenObjectTS(user_id, dataset_id,
                         open_flags, &handles[1],
                         &test_context, pctest_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_OpenObjectTS: %d.",
                         rc);
        rv++;
        goto fail_open;
    }

    steps = regular_steps;
    for(int i = 0; i < (num_regular_steps + num_cloudnode_steps); i++) {
        if ((i >= num_regular_steps) && !run_cloudnode_tests) {
            break;
        }

        if (i == num_regular_steps) {
            steps = cloudnode_steps;
            running_cloudnode_steps = true;
            VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "Start cloudnode-specific tests");
        }

        if (running_cloudnode_steps) {
            i -= num_regular_steps;
        }

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

        case WRITE: {
            string data = steps[i].data;
            rc = root.write_file(handles[steps[i].object], path, name, data);
            if(rc != steps[i].expect) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "Write file %s: %d, expected %d.",
                                 name.c_str(), rc, steps[i].expect);
                rv++;
            }
        } break;

        case TRUNCATE: {
            u64 size = steps[i].size;
            rc = root.truncate_file(handles[steps[i].object], path, name, size);
            if(rc != steps[i].expect) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "Truncate file %s: %d, expected %d.",
                                 name.c_str(), rc, steps[i].expect);
                rv++;
            }
        } break;

        case SET_CTIME: {
            u64 size = steps[i].size;
            rc = root.set_times(handles[steps[i].object], name, size, 0);
            if(rc != steps[i].expect) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "Set-times file %s: %d, expected %d.",
                                 name.c_str(), rc, steps[i].expect);
                rv++;
            }
        } break;
        case SET_MTIME: {
            u64 size = steps[i].size;
            rc = root.set_times(handles[steps[i].object], name, 0, size);
            if(rc != steps[i].expect) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "Set-times file %s: %d, expected %d.",
                                 name.c_str(), rc, steps[i].expect);
                rv++;
            }
        } break;
        case SET_CMTIME: {
            u64 size = steps[i].size;
            rc = root.set_times(handles[steps[i].object], name, size, size);
            if(rc != steps[i].expect) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "Set-times file %s: %d, expected %d.",
                                 name.c_str(), rc, steps[i].expect);
                rv++;
            }
        } break;

        case MKDIR: {
            rc = root.add_dir(handles[steps[i].object], path, name);
            if(rc != steps[i].expect) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "Make directory %s: %d, expected %d.",
                                 name.c_str(), rc, steps[i].expect);
                rv++;
            }
        } break;
        case READDIR:
            rc = root.read_dir(handles[steps[i].object], path, name);
            if(rc != steps[i].expect) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "Read directory %s: %d, expect %d",
                                 name.c_str(), rc, steps[i].expect);
                rv++;
            }
            break;
        case READ:
            rc = root.read_file(handles[steps[i].object], path, name);
            if(rc != steps[i].expect) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "Read file %s: %d, expect %d",
                                 name.c_str(), rc, steps[i].expect);
                rv++;
            }
            break;
        case REMOVE:
            rc = root.remove(handles[steps[i].object], name);
            if(rc != steps[i].expect) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "Remove directory %s: %d, expect %d.",
                                 name.c_str(), rc, steps[i].expect);
                rv++;
            }
            break;

        case RENAME: {
            string new_name = steps[i].data;
            rc = root.rename(handles[steps[i].object], name, new_name);
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
            rc = root.copy(handles[steps[i].object], name, new_name);
            if(rc != steps[i].expect) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "copy %s to %s: %d, expect %d.",
                                 name.c_str(), new_name.c_str(),
                                 rc, steps[i].expect);
                rv++;
            }
        } break;

        case GET_SPACE: {
            rc = root.get_space(handles[steps[i].object]);
            if(rc != steps[i].expect) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "Get space: %d, expect %d.",
                                 rc, steps[i].expect);
                rv++;
            }
        } break;

        case VERIFY:
            break;
        }

        // For write-type instructions, verify server data is correct.
        switch(steps[i].type) {
        case READDIR:
        case READ:
        case GET_SPACE:
            // does not apply to read instructions.
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
            rc = root.verify(handles[steps[i].object], path,
                             VSSI_GetVersion(handles[steps[i].object]),
                             0, 0, VS_PSN_VERIFY_VERSION);
            if(rc != 0) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "Server data does not match: %d", rc);
                rv++;
            }
        }

        if(rv != 0) {
            break;
        }

        if (running_cloudnode_steps) {
            i += num_regular_steps; // To avoid confusing the for-loop
        }
    }

    // Close object, both handles
    VSSI_CloseObject(handles[0],
                     &test_context, pctest_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Close object failed: %d.",
                         rc);
        rv++;
    }
    VSSI_CloseObject(handles[1],
                     &test_context, pctest_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    // Accept BADOBJ result here in case the test runs slowly, timing out this handle.
    if(rc != VSSI_SUCCESS && rc != VSSI_BADOBJ) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Close object failed: %d.",
                         rc);
        rv++;
    }

    VSSI_Delete_Deprecated(user_id, dataset_id,
                         &test_context, pctest_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "VSSI_Delete: %d.",
                         rc);
        rv++;
        goto fail_open;
    }

 fail_open:
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Finished test: %s. Result: %s",
                        vsTest_curTestName,
                        (rv == 0) ? "PASS" : "FAIL");
    return rv;
}

static int check_stat(VSSI_Object handle,
                      const std::string name,
                      VSSI_Dirent2* dir_entry)
{
    VSSI_Dirent2* stats = NULL;
    int rv = 0;

    // Get file stats and make sure they match the dirent data.
    VPLTRACE_LOG_FINE(TRACE_APP, 0,
                      "Verifying stat for file %s.",
                      name.c_str());

    VSSI_Stat2(handle, name.c_str(), &stats,
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

 exit:

    if(stats != NULL) {
        free(stats);
    }

    return rv;
}

void vs_file::reset_verify()
{
    confirmed = false;
}

void vs_file::update_version(u64 change_version)
{
    version = change_version;
}

int vs_file::verify(VSSI_Object handle,
                    const std::string& name,
                    u64 received_version,
                    u64 received_ctime,
                    u64 received_mtime,
                    vs_psn_verify_flag flag)
{
    int rv = 0;
    u32 length = contents.size() + 1024; // extra space to detect long files
    char data[length];
    string server_contents;
    int rc;
    VSSI_File fh;
    bool file_is_open = false;

    VPLTRACE_LOG_FINE(TRACE_APP, 0,
                        "Verifying file %s "
                        "received version "FMTu64".",
                        name.c_str(), received_version);

    VSSI_OpenFile(handle, name.c_str(), VSSI_FILE_OPEN_READ, 0, &fh,
                  &test_context, pctest_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if ( rc != VSSI_SUCCESS ) {
        rv = rc;
        goto done;
    }
    file_is_open = true;

    VSSI_ReadFile(fh, 0, &length, data, &test_context, pctest_callback);
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
        if(contents != server_contents) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "File %s contents not matched at server."
                             "\n\tlocal:%s\n\tserver:%s",
                             name.c_str(), contents.c_str(),
                             server_contents.c_str());
            rv = 1;
        }
    }
    if((flag & VS_PSN_VERIFY_VERSION) &&
       version != received_version) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "File \"%s\" change version "FMTu64" does not match reported change version "FMTu64".",
                         name.c_str(), version, received_version);
        rv = 1;
    }
    else {
        VPLTRACE_LOG_FINE(TRACE_APP, 0,
                     "File \"%s\" change version "FMTu64" ok.",
                     name.c_str(), version);
    }
    if(ctime != 0 && (flag & VS_PSN_VERIFY_CTIME) && received_ctime != ctime) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "File \"%s\" creation time "FMTu64" does not match reported creation time "FMTu64".",
                         name.c_str(), ctime, received_ctime);
        rv = 1;
    }
    if(mtime != 0 && (flag & VS_PSN_VERIFY_MTIME) && received_mtime != mtime) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "File \"%s\" modification time "FMTu64" does not match reported modification time "FMTu64".",
                         name.c_str(), mtime, received_mtime);
        rv = 1;
    }

    if(rv == 0) {
        confirmed = true;
    }

done:
    if ( file_is_open ) {
        VSSI_CloseFile(fh, &test_context, pctest_callback);
        VPLSem_Wait(&(test_context.sem));
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
    VSSI_File fh = NULL;
    bool file_is_open = false;

    VPLTRACE_LOG_FINE(TRACE_APP, 0,
                        "Writing file %s.",
                        name.c_str());

    contents = data;

    VSSI_OpenFile(handle, name.c_str(), 
                  VSSI_FILE_OPEN_WRITE | VSSI_FILE_OPEN_CREATE, 0, &fh,
                  &test_context, pctest_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if ( rc && rc != VSSI_EXISTS) {
        rv = rc;
        goto exit;
    }
    file_is_open = true;

    VSSI_WriteFile(fh, 0, &length, data.data(),
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
        VSSI_TruncateFile(fh, length,
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

    VSSI_CloseFile(fh, &test_context, pctest_callback);
    VPLSem_Wait(&(test_context.sem));
    file_is_open = false;

    if(rv == 0) {
        version = VSSI_GetVersion(handle);
        //VPLTRACE_LOG_INFO(TRACE_APP, 0, "Current version: %llu.", version);
    }

 exit:
    if ( file_is_open ) {
        VSSI_CloseFile(fh, &test_context, pctest_callback);
        VPLSem_Wait(&(test_context.sem));
    }
    return rv;
}

int vs_file::truncate(VSSI_Object handle,
                      const std::string& name,
                      u64 size)
{
    int rv = 0;
    int rc;
    VSSI_File fh = NULL;
    bool file_is_open = false;

    VPLTRACE_LOG_FINE(TRACE_APP, 0,
                        "Truncating file %s.",
                        name.c_str());

    contents.resize(size);

    VSSI_OpenFile(handle, name.c_str(), 
                  VSSI_FILE_OPEN_WRITE | VSSI_FILE_OPEN_CREATE, 0, &fh,
                  &test_context, pctest_callback);
    VPLSem_Wait(&(test_context.sem));
    rc = test_context.rv;
    if ( rc && rc != VSSI_EXISTS) {
        rv = rc;
        goto exit;
    }
    file_is_open = true;
    rv = 0;

    VSSI_TruncateFile(fh, size,
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

    VSSI_CloseFile(fh, &test_context, pctest_callback);
    VPLSem_Wait(&(test_context.sem));
    file_is_open = false;

    if(rv == 0) {
        version = VSSI_GetVersion(handle);
    }

 exit:
    if(file_is_open) {
        VSSI_CloseFile(fh, &test_context, pctest_callback);
        VPLSem_Wait(&(test_context.sem));
    }
    return rv;
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
                   u64 received_mtime,
                   vs_psn_verify_flag flag)
{
    int rv;
    VSSI_Dir2 server_dir;
    VSSI_Dirent2* server_entry;
    string element_name;
    map<string, vs_dir>::iterator dir_it;
    map<string, vs_file>::iterator file_it;

    VPLTRACE_LOG_FINE(TRACE_APP, 0,
                        "Verifying directory %s "
                        "received version "FMTu64".",
                        name.c_str(), received_version);

    // reset state before verification (nothing confirmed yet)
    reset_verify();

    VSSI_OpenDir2(handle, name.c_str(), &server_dir,
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
        server_entry = VSSI_ReadDir2(server_dir);
        if(server_entry == NULL) {
            break;
        }

        element_name = server_entry->name;
        if(server_entry->isDir) {
            // skip the "lost+found" directory
            if (strcmp(element_name.c_str(), "lost+found")) {
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
                element_name = concat_paths(name, server_entry->name);
                rv = file_it->second.verify(handle,
                                            element_name,
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
        if((flag & VS_PSN_VERIFY_VERSION) &&
           version != received_version) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Dir change version "FMTu64" for \"%s\" does not match reported change version "FMTu64".",
                             version, name.c_str(), received_version);
            rv = 1;
            goto exit;
        }
        else {
            VPLTRACE_LOG_FINE(TRACE_APP, 0,
                         "Dir \"%s\" change version "FMTu64" ok.",
                         name.c_str(), version);
        }
        if(ctime != 0 && (flag & VS_PSN_VERIFY_CTIME) && received_ctime != ctime) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Directory \"%s\" creation time "FMTu64" does not match reported creation time "FMTu64".",
                             name.c_str(), ctime, received_ctime);
            rv = 1;
            goto exit;
        }
        if(mtime != 0 && (flag & VS_PSN_VERIFY_MTIME) && received_mtime != mtime) {
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
    VSSI_CloseDir2(server_dir);
 exit_no_free:
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

        map<string, vs_file>::iterator file_it = files.find(element_name);
        if(file_it == files.end()) {
            files[element_name]; // side-effect: create file
            file_it = files.find(element_name);
        }
        element_name = concat_paths(path, name);
        rv = file_it->second.write(handle, element_name, data);
    }
    else {

        map<string, vs_dir>::iterator dir_it = dirs.find(element_name);

        string newpath = concat_paths(path, element_name);
        string newname;
        if(slash != string::npos) {
            newname = name.substr(slash + 1);
        }
        if(dir_it != dirs.end()) {
            rv = dir_it->second.write_file(handle, newpath, newname, data);
        }
    }

    if(rv == 0) {
        version = VSSI_GetVersion(handle);
        //VPLTRACE_LOG_INFO(TRACE_APP, 0, "Current version: %llu.", version);
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
        map<string, vs_file>::iterator file_it = files.find(element_name);
        if(file_it == files.end()) {
            files[element_name]; // side-effect: create file
            file_it = files.find(element_name);
        }
        element_name = concat_paths(path, name);
        rv = file_it->second.truncate(handle, element_name, size);
    }
    else {
        map<string, vs_dir>::iterator dir_it = dirs.find(element_name);

        string newpath = concat_paths(path, element_name);
        string newname;
        if(slash != string::npos) {
            newname = name.substr(slash + 1);
        }
        if(dir_it != dirs.end()) {
            rv = dir_it->second.truncate_file(handle, newpath, newname, size);
        }
    }

    if(rv == 0) {
        version = VSSI_GetVersion(handle);
        //VPLTRACE_LOG_INFO(TRACE_APP, 0, "Current version: %llu.", version);
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

    if(rv == 0) {
        version = VSSI_GetVersion(handle);
        //VPLTRACE_LOG_INFO(TRACE_APP, 0, "Current version: %llu.", version);
        // Update timestamps for local state file/dir
        set_times(name, version, ctime, mtime);

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
        }
        if(mtime != 0) {
            this->mtime = mtime;
        }
    }
    else {
        filep = find_file("", full_name, change_version);
        if(filep == NULL) {
            dirp = find_dir("", full_name, change_version);
            if(dirp) {
                if(ctime != 0) {
                    dirp->ctime = ctime;
                }
                if(mtime != 0) {
                    dirp->mtime = mtime;
                }
            }
        }
        else {
            if(ctime != 0) {
                filep->ctime = ctime;
            }
            if(mtime != 0) {
                filep->mtime = mtime;
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
    u32 attrs = 0;

    VPLTRACE_LOG_FINE(TRACE_APP, 0,
                        "Adding directory (%s)/(%s).",
                        path.c_str(), name.c_str());

    if(name.empty()) {

        VSSI_MkDir2(handle, path.c_str(), attrs,
                   &test_context, pctest_callback);
        VPLSem_Wait(&(test_context.sem));
        rv = test_context.rv;
        if(rv != VSSI_SUCCESS && rv != VSSI_ISDIR) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Make directory %s: %d.",
                             path.c_str(), rv);
            goto exit;
        }
        if(rv == VSSI_ISDIR) {
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
            // TODO: this can not happen anymore, since the actual server
            // TODO: mkdir would have failed above.
            // File is in the way. Remove it.
            string s = concat_paths(path, file_it->first);
            VPLTRACE_LOG_FINE(TRACE_APP, 0,
                              "File %s prevents create directory.",
                              s.c_str());
            //string full_name = concat_paths(path, file_it->first);
            //root.remove(full_name, VSSI_GetVersion(handle) + 1);
            rv = VSSI_FEXISTS;
            goto exit;
        }
        
        bool new_added = false;
        map<string, vs_dir>::iterator dir_it = dirs.find(element_name);
        if(dir_it == dirs.end()) {
            new_added = true;
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
        if(rv != VSSI_SUCCESS && new_added) {
            dirs.erase(element_name);
        }
    }

    if(rv == 0) {
        version = VSSI_GetVersion(handle);
        //VPLTRACE_LOG_INFO(TRACE_APP, 0, "Current version: %llu.", version);
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
    // If the next directory is non-existent, issue the server command and exit.
    // Relay server response back up the chain.

    VPLTRACE_LOG_FINE(TRACE_APP, 0,
                        "Reading directory (%s)/(%s).",
                        path.c_str(), name.c_str());

    if(name.empty()) {
        // Verify this directory
        rv = verify(handle, path, 0, 0, 0, VS_PSN_VERIFY_NONE);
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
            rv = dummy_dir.verify(handle, element_name, 0, 0, 0, VS_PSN_VERIFY_NONE);
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
                                   0, 0, 0, VS_PSN_VERIFY_NONE);
        }
        else {
            // No such file.
            // Verify dummy file contents to get server result.
            vs_file dummy_file;
            rv = dummy_file.verify(handle, element_name, 0, 0, 0, VS_PSN_VERIFY_NONE);
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
            rv = dummy_file.verify(handle, element_name, 0, 0, 0, VS_PSN_VERIFY_NONE);
        }
    }

    return rv;
}

void vs_dir::remove(const std::string& name, u64 change_version)
{
    // Remove the element from the tree 
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
        // Make copy for the trash.
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
    size_t last_slash;
    string dir_name;
    map<string, vs_dir>::iterator dirtarget_it;

    // Chop slashes.
    while(full_name[0] == '/') {
        full_name.erase(0, 1);
    }
    while(full_name[full_name.size() - 1 ] == '/') {
        full_name.erase(full_name.size() - 1, 1);
    }

    last_slash = full_name.find_last_of('/');
    dir_name = full_name;
    if(last_slash != std::string::npos) {
        dir_name = full_name.substr(last_slash + 1 );
    } 

    dirtarget_it = dirs.find(dir_name);
    // remove all sub files
    map<string, vs_file> tmpFiles;
    map<string, vs_dir> tmpDirs;
    vs_dir* targetDir = NULL;
    if(dirtarget_it != dirs.end()) {
        tmpFiles = dirtarget_it->second.files;
        tmpDirs = dirtarget_it->second.dirs;
        targetDir = &(dirtarget_it->second);
    } else if(!dir_name.compare("")) {
        tmpFiles = files;
        tmpDirs = dirs;
        targetDir = this;
    }
    for(map<string, vs_file>::iterator file_it = tmpFiles.begin();
        file_it != tmpFiles.end();
        file_it ++) {
        targetDir->remove(handle, name + "/" + file_it->first);
    }
    //remove all sub dirs
    for(map<string, vs_dir>::iterator dir_it = tmpDirs.begin();
        dir_it != tmpDirs.end();
        dir_it ++) {
        targetDir->remove(handle, name + "/" + dir_it->first);
    }

    // issue server command and commit
    // no need to actually remove the root directory (will run into problem with "lost+found")
    if (strcmp(full_name.c_str(), "")) {
        VSSI_Remove(handle, full_name.c_str(),
                    &test_context, pctest_callback);
        VPLSem_Wait(&(test_context.sem));
        rv = test_context.rv;
    } else {
        rv = VSSI_SUCCESS;
    }
    if(rv != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Remove component %s: %d.",
                         full_name.c_str(), rv);
    }
    else {
        // Remove from local state.
        root.remove(name, VSSI_GetVersion(handle) + 1);
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

    // With the new post-file-handle behavior, rename fails on the
    // server side in a lot more cases.  Perform the server command
    // and then adjust the local model of the tree based on whether
    // it succeeded or not.

    VSSI_Rename2(handle, name.c_str(), new_name.c_str(), 0,
                &test_context, pctest_callback);
    VPLSem_Wait(&(test_context.sem));
    rv = test_context.rv;
    if(rv != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0,
                         "Rename component %s to %s: %d.",
                         name.c_str(), new_name.c_str(), rv);
    }
    else {
        // Rename succeeded, so adjust the local state
        version = VSSI_GetVersion(handle);
        //VPLTRACE_LOG_INFO(TRACE_APP, 0, "Current version: %llu.", version);

        // Find the named element.
        filep = get_file(root, name, version);
        if(filep == NULL) {
            dirp = get_dir(root, name, version);
        }

        // Place element back in tree at new name.
        if(filep) {
            put_file(filep, root, new_name, version);
            delete filep;
        }
        else if(dirp) {
            put_dir(dirp, root, new_name, version);
            delete dirp;
        }
        else {
            // Local tree is out of sync
            // TODO: can this actually happen?
            VPLTRACE_LOG_ERR(TRACE_APP, 0, 
                             "Can't find element in local tree: rename %s to %s: %d.",
                             name.c_str(), new_name.c_str(), rv);
        }

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
            // rv->update_version(change_version);
            rv->version = change_version;
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
