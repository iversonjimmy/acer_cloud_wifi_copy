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


#include <vpl_time.h>
#include <vpl_th.h>
#include "vplex_trace.h"
#include "vplex_assert.h"
#include "vplex_vs_directory.h"
#include "vssi.h"

#include "manifest.h"

#include <map>
#include <string>
#include <deque>
#include <set>

/// Types of task
enum {
    TASK_WRITER = 0,
    TASK_READER,
};

struct load_params {
    bool use_manifest;
    manifest_t manifest;
    u32 file_size;
    u32 file_cnt;

    bool use_metadata;
    u32 xfer_size;
    bool wait_for_writers;
    bool reset_dataset;
    u32 commit_interval;
    u32 commit_size;
    bool verify_read;
    bool verify_write;

    // doc save and go options
    u32 thumb_size;
    bool write_thumb;
    std::string base_dir;
    std::string thumb_dir;

    // Base directory for read operations ("/" if not specified)
    std::string read_dir;

    // When writing, file data to write (file_size length if not NULL)
    char* source_data;

    // On completion call this callback with task context.
    void (*callback)(void*);
};

struct load_task {
    int taskType;
    std::string taskName;

    // Context for task completion callback.
    void* context;

    load_params* parameters;

    u64 userId;
    std::string username;
    std::string datasetLocation;
    vplex::vsDirectory::SessionInfo sessionInfo;
    VSSI_Session session;

    VPLTime_t start;
    VPLTime_t stop;

    VPLMutex_t mutex;

    bool failed;
    u32 rv;

    u32 reads;
    u32 dir_reads;
    u32 writes;
    u32 commits;
    VPLTime_t read_time;
    VPLTime_t dir_read_time;
    VPLTime_t write_time;
    VPLTime_t commit_time;
    VPLTime_t verify_time;
    u64 data_read;
    u64 data_write;

    VPLTime_t op_start;
    VPLTime_t op_end;

    // While running, VSSI object in-use.
    VSSI_Object handle;

    // Read state
    std::deque<std::string> directories; // Directories to read.
    std::deque<std::pair<std::string, u64> > files; // Files & sizes to read.
    std::map<std::string, u64> read_files; // Files/dirs read, version read.
    bool last_scan;

    // Write state
    u32 files_in_batch;
    u32 bytes_in_batch;
    bool verified_merge;

    VPLTime_t io_start;
    u32 io_xfered;

    u32 files_started;
    u32 files_done;
    bool complete;
    bool finishing;

#define TASK_IO_LIMIT 16
    int io_limit;
    int io_processing; // Number of I/O tasks in-progress
    bool commit_pending;
    bool commit_active;
    VSSI_Dirent* stats;

    load_task() :
        taskType(TASK_WRITER),
        context(NULL),
        userId(0),
        session(NULL),
        failed(false),
        rv(0),
        reads(0),
        dir_reads(0),
        writes(0),
        commits(0),
        read_time(0),
        dir_read_time(0),
        write_time(0),
        commit_time(0),
        verify_time(0),
        data_read(0),
        data_write(0),
        handle(NULL),
        last_scan(false),
        files_in_batch(0),
        bytes_in_batch(0),
        verified_merge(false),
        io_start(0),
        io_xfered(0),
        files_started(0),
        files_done(0),
        complete(false),
        finishing(false),
        io_limit(TASK_IO_LIMIT),
        io_processing(0),
        commit_pending(false),
        commit_active(false),
        stats(NULL)
    {
         VPLMutex_Init(&mutex);
    }

    ~load_task()
    {
        if(stats) {
            free(stats);
        }

        VPLMutex_Destroy(&mutex);
    }
};

/// Clear the contents of the dataset for a task (delete all contents).
int cloud_clear_dataset(VSSI_Session session, std::string& datasetLocation);

/// Main task entry point.
void cloud_load_test(void* params);
/// Main task stages
void cloud_open_done_helper(void* ctx, VSSI_Result result);
void cloud_open_done(void* ctx);
void cloud_do_work(void* ctx);
void cloud_task_done(void* ctx);
void cloud_do_commit_done_helper(void* ctx, VSSI_Result result);
void cloud_do_commit_done(void* ctx);
void cloud_do_stat(void* ctx);
void cloud_do_stat_done_helper(void* ctx, VSSI_Result result);
void cloud_do_stat_done(void* ctx);
void cloud_done(void* ctx);
void cloud_close_done_helper(void* ctx, VSSI_Result result);
void cloud_close_done(void* ctx);

struct cloud_io_task {
    std::string filename;
    u64 filesize;
    VPLTime_t accumulated_time; // sum(end-start) for all server ops
    bool read; // read or write
    bool file; // file or directory (read only)
    bool commit; // commit after write (write only)
    bool confirm; // confirm file contents (read only)
    bool thumbnail; // This operation is for a thumbnail.
    int rv;

    VPLTime_t start_time; // Start of latest server op
    VPLTime_t end_time; // End of latest server op
    VPLTime_t start_size_time; // Start of latest server op
    VPLTime_t end_size_time; // End of latest server op
    VPLTime_t start_meta_time; // Start of latest server op
    VPLTime_t end_meta_time; // End of latest server op

    char src_pattern[16];

    load_task* main_task;

    VPLMutex_t mutex;
    u32 reqs_pending;

    VSSI_Dirent* stats;
    VSSI_Dir dir;

#define TASK_BLOCK_LIMIT 4
    u32 block_limit;

    // On completion call this callback with this context.
    void (*callback)(void*);
    void* context;

    cloud_io_task() :
        filesize(0),
        accumulated_time(0),
        read(false),
        file(false),
        commit(false),
        confirm(false),
        thumbnail(false),
        rv(0),
        main_task(NULL),
        reqs_pending(0),
        stats(NULL),
        dir(NULL),
        block_limit(TASK_BLOCK_LIMIT),
        callback(NULL),
        context(NULL)
    {
         VPLMutex_Init(&mutex);
    }

    ~cloud_io_task()
    {
        VPLMutex_Destroy(&mutex);

        if(stats) {
            free(stats);
        }
        if(dir) {
            VSSI_CloseDir(dir);
        }            
    }
};

struct cloud_block_io_task {
    cloud_io_task* parent;

    VPLTime_t accumulated_time; // sum(end-start) for all server ops
    VPLTime_t start_time; // Start of latest server op
    VPLTime_t end_time; // End of latest server op

    // Initial offset and length for activity
    u64 offset;
    u64 length;

    // Length requested/completed
    u32 req_len;

    // Offset complete so far
    u64 offset_so_far;
    u64 confirm_offset_so_far;

    // Bufffer for read or write (size of transfer block)
    char* buffer;

    int rv;

    cloud_block_io_task(cloud_io_task* parent,
                        u64 offset,
                        u64 length) :
        parent(parent),
        accumulated_time(0),
        offset(offset),
        length(length),
        req_len(0),
        offset_so_far(0),
        confirm_offset_so_far(0),
        buffer(NULL),
        rv(0)
    {};

    ~cloud_block_io_task() 
    {
        if(buffer) {
            delete buffer;
        }
    };
};

/// Write task stages
void cloud_write_file(void* ctx);
void cloud_write_block(void* ctx);
void cloud_write_block_done_helper(void* ctx, VSSI_Result result);
void cloud_write_block_done(void* ctx);
void cloud_write_metadata(void* ctx);
void cloud_write_metadata_done_helper(void* ctx, VSSI_Result result);
void cloud_write_metadata_done(void* ctx);
void cloud_write_size(void* ctx);
void cloud_write_size_done_helper(void* ctx, VSSI_Result result);
void cloud_write_size_done(void* ctx);
void cloud_write_done(cloud_io_task *ioTask, int result, VPLTime_t time);

/// Read file task stages
void cloud_read_file(void* ctx);
void cloud_read_block(void* ctx);
void cloud_read_block_done_helper(void* ctx, VSSI_Result result);
void cloud_read_block_done(void* ctx);
void cloud_read_confirm_block(cloud_block_io_task* blockTask);
void cloud_read_done(cloud_io_task *ioTask, int result, VPLTime_t time);

/// Read directory task stages
void cloud_read_directory(void* ctx);
void cloud_read_directory_done(void* ctx, VSSI_Result result);
void cloud_read_directory_process(void* ctx);
void cloud_read_directory_end(void* ctx);

#endif // include guard
