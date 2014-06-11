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

#ifndef __VSTEST_PERSONAL_CLOUD_DATA_HPP__
#define __VSTEST_PERSONAL_CLOUD_DATA_HPP__

#include <map>
#include <string>

#include <vpl_time.h>
#include <vpl_th.h>
#include "vplex_trace.h"
#include "vplex_assert.h"
#include "vplex_vs_directory.h"
#include "vssi.h"

#include "manifest.h"

void generate_path_prefix(char *, int, const char *, int);

struct tpc_parms_s {
    manifest_t *manifest;
    u32 xfer_delay_size;
    VPLTime_t xfer_delay_time;
    u32 file_size;
    u32 file_cnt;
    u32 rw_ratio;
    u32 xfer_size;
    u32 round_size_k;
    bool manifest_randomize;
    bool rw_only;
    bool writer_wait;

    bool test_local;
    bool direct_xfer;
    char *path_prefix;
    const char *dataset;
    const char *pc_namespace;
    int commit_interval;
};
typedef struct tpc_parms_s tpc_parms_t;

struct tpc_thread_s {
    u32 id;
    u64 user_id;

    VPLThread_t wrkr;
    VPLThread_return_t dontcare;

    time_t start;
    time_t stop;
    tpc_parms_t *parms;
    u32 reads;
    u32 writes;
    u32 seed;
    bool rw_reader;
    bool failed;
    bool commit_pending;
    u32 rv;
    clock_t read_time;
    clock_t write_time;
    u32 data_read_k;
    u32 data_write_k;
    u32 rate_underrun_cnt;
 
    char *xfer_buf;
    int xfer_buf_size;
};
typedef struct tpc_thread_s tpc_thread_t;

// callback and context to make async commands synchronous for testing.
// XXX This should probably be merged with the thread structure.
typedef struct  {
    VPLSem_t sem;
    int rv;
    VPLTime_t io_start;
    u32 io_xfered;
} pctest_context_t;


/// Main test entry point.
void test_personal_cloud(tpc_thread_t *thread);
int local_create_dirs(const char *path1, const char *path2);

bool thrd_ready_wait(bool is_ok, void *handle, void (*idle)(void *));
void thrd_writer_done(void);

extern bool pct_threads_stop;
extern bool use_metadata;
extern bool writers_are_done;
extern bool check_io;

/// File entry
class vs_file
{
public:
    vs_file() {};
    ~vs_file() {};

    /// Read data from server and compare to local data.
    /// @param name Name of the file
    /// @return #VSSI_SUCCESS File present and data matches expectations.
    /// @return 1 Data mismatches.
    /// @return other VSSI error Error occurred attempting to access data.
    int read(pctest_context_t& test_context,
             const std::string& name,
             u32 offset,
             u32 xfer_size,
             char *data);

    /// Write specified data to the file, replacing any previous data.
    /// @param name File path and name
    /// @param data Data to write
    /// @return #VSSI_SUCCESS File written to server.
    /// @return other VSSI error Error occurred attempting to write data.
    int write(pctest_context_t& test_context,
              const std::string& path,
              u32 offset,
              u32 xfer_size,
              char *data);
};

#endif // include guard
