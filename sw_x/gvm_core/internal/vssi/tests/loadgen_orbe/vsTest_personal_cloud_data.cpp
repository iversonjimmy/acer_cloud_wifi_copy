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

#include <stdio.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <glob.h>

#include <sstream>
#include <iomanip>

#include "vsTest_personal_cloud_data.hpp"

#include "vsTest_worker.h"

#include "cslsha.h"

using namespace std;

typedef struct  {
    VPLSem_t sem;
    int rv;
} pctest_context_t;

static void common_callback(void* ctx, VSSI_Result rv)
{
    pctest_context_t* context = (pctest_context_t*)ctx;

    context->rv = rv;
    VPLSem_Post(&(context->sem));
}

static int common_open_file(VSSI_Object handle, VSSI_File &file_handle, std::string filename)
{
    pctest_context_t test_context;
    int rv = 0;
    if(VPLSem_Init(&(test_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0,
                         "Failed to create sempahore.");
        rv = -1;
    } else {
        VSSI_OpenFile(handle, filename.c_str(),
                      VSSI_FILE_OPEN_READ | VSSI_FILE_OPEN_WRITE | VSSI_FILE_OPEN_CREATE |
                      VSSI_FILE_SHARE_READ | VSSI_FILE_SHARE_WRITE | VSSI_FILE_SHARE_DELETE,
                      0, &file_handle,
                      &test_context, common_callback);
        VPLSem_Wait(&(test_context.sem));
        rv = test_context.rv;
        if(rv != VSSI_SUCCESS && rv != VSSI_EXISTS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0,
                             "Failed to open file %s, result %d.", 
                             filename.c_str(), test_context.rv);
        }
        VPLSem_Destroy(&(test_context.sem));
    }
    return rv;
}

static int common_close_file(VSSI_File &handle)
{
    int rv = 0;
    pctest_context_t test_context;
    if(handle != NULL) {
        if(VPLSem_Init(&(test_context.sem), 1, 0) != VPL_OK) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0,
                             "Failed to create sempahore.");
            rv = -1;
        } else {
            VSSI_CloseFile(handle,
                           &test_context, common_callback);
            VPLSem_Wait(&(test_context.sem));
            if((rv = test_context.rv) != VSSI_SUCCESS) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "CloseFile failed %d", test_context.rv);
            }
            handle = NULL;
            VPLSem_Destroy(&(test_context.sem));
        }
    }
    return rv;
}

static int common_mkdir(VSSI_Object handle, std::string& dirPath)
{
    int rv = VSSI_SUCCESS;
    u32 attrs = 0;
    pctest_context_t test_context;

    do {
        VPL_SET_UNINITIALIZED(&(test_context.sem));
        if(VPLSem_Init(&(test_context.sem), 1, 0) != VPL_OK) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "Failed to create semaphore.");
            rv = VSSI_INVALID;
            break;
        } 
        
        VSSI_Dirent2 *dir_ent = NULL;
        VSSI_Stat2(handle, dirPath.c_str(), &dir_ent,
                   &test_context, common_callback);
        VPLSem_Wait(&(test_context.sem));
        rv = test_context.rv;

        if( rv == VSSI_NOTFOUND) {
            VSSI_MkDir2(handle, dirPath.c_str(), attrs,
                        &test_context, common_callback);
            VPLSem_Wait(&(test_context.sem));
            rv = test_context.rv;
        }

        VPLSem_Destroy(&(test_context.sem));

    } while(0);
    VPLTRACE_LOG_FINE(TRACE_APP, 0, "Mkdir %s rv %d", dirPath.c_str(), rv);

    return rv;
}

static int common_mkdirp(VSSI_Object handle, std::string& dirPath)
{
    int rv = VSSI_SUCCESS;
    size_t slash;

    VPLTRACE_LOG_FINE(TRACE_APP, 0,"MkdirP %s", dirPath.c_str());

    slash = dirPath.find_first_of('/');
    while(slash != std::string::npos) {
        std::string beginPath = dirPath.substr(0, slash);
        rv = common_mkdir(handle, beginPath);
        if(rv != VSSI_SUCCESS && rv != VSSI_ISDIR) break;
        slash = dirPath.find_first_of('/', slash + 1);
    }
    rv = common_mkdir(handle, dirPath);
    if(rv == VSSI_ISDIR) rv = VSSI_SUCCESS;

    return rv;
}

int cloud_clear_dataset(VSSI_Session session, u64 uid, u64 did, VSSI_RouteInfo *route)
{
    int rv = 0;
    pctest_context_t test_context;

    if(VPLSem_Init(&(test_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create sempahore.");
        return -1;
    }

    // Issue Delete command to clear all dataset contents. Wait for result.
    VSSI_Delete2(session, uid, did, route,
                &test_context, common_callback);
    VPLSem_Wait(&(test_context.sem));
    rv = test_context.rv;

    return rv;
}

void cloud_load_test(void* params)
{
    load_task *task = (load_task*)(params);
    pctest_context_t test_context;

    if(VPLSem_Init(&(test_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create sempahore.");
        return;
    }
    // Open dataset as ordered.
    VPLTRACE_LOG_FINE(TRACE_APP, 0,
                      "Task[%s] Opening object: %s session: %p",
                      task->taskName.c_str(),
                      task->datasetLocation.c_str(),
                      task->session);

    task->start = VPLTime_GetTime();

    // Current callback based design is not for single object open and close,
    // Use static object handle and mutex here for single object.
    VPLMutex_Lock(&(task->obj_mutex));
    if(task->handle == NULL) {
        VSSI_OpenObject2(task->session,
                         task->userId,
                         task->datasetId,
                         &(task->routeInfo),
                         task->taskType == TASK_READER ? VSSI_READONLY :
                         VSSI_READWRITE | VSSI_FORCE,
                         &(task->handle),
                         &test_context, common_callback);
        VPLSem_Wait(&(test_context.sem));

        if(test_context.rv) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Task[%s] Open object /%s: %d.",
                             task->taskName.c_str(), task->datasetLocation.c_str(), test_context.rv);
            task->rv = test_context.rv;
        }

        VPLTRACE_LOG_FINE(TRACE_APP, 0,
                          "Task[%s] Opened object: %s session: %p result:%d",
                          task->taskName.c_str(), task->datasetLocation.c_str(), task->session, test_context.rv);
    }
    VPLMutex_Unlock(&(task->obj_mutex));

    vsTest_worker_add_task(cloud_open_done, task);
}

// Current callback based design is not for single object open and close,
#if 0
void cloud_open_done_helper(void* ctx, VSSI_Result result)
{
    load_task *task = (load_task*)(ctx);

    if(result) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Task[%s] Open object /%s: %d.",
                         task->taskName.c_str(), task->datasetLocation.c_str(), result);
        task->rv = result;
    }

    VPLTRACE_LOG_FINE(TRACE_APP, 0,
                      "Task[%s] Opened object: %s session: %p result:%d",
                      task->taskName.c_str(), task->datasetLocation.c_str(), task->session, result);

    vsTest_worker_add_task(cloud_open_done, ctx);
}
#endif

void cloud_open_done(void* ctx)
{
    load_task *task = (load_task*)(ctx);

    if(task->rv > 0) {
        task->failed = true;

        cloud_done(ctx);
    }
    else {
        // Launch read or write task as appropriate.
        cloud_do_work(ctx);
    }
}

void cloud_do_work(void* ctx)
{
    load_task *task = (load_task*)(ctx);
    cloud_io_task* ioTask = NULL;
    cloud_io_task* thumbIoTask = NULL;
    bool finished = false;
    bool do_verify_merge = false;

    VPLMutex_Lock(&(task->mutex));

    // Check if failed.
    if(task->failed) {
        if(task->io_processing == 0) {
            task->complete = true;
        }
        goto done;
    }

    // Check if done.
    if(task->taskType == TASK_WRITER) {
        // Stop writers when last file written.
        if(task->files_started >= task->parameters->file_cnt) {
            if(task->io_processing == 0) {
                task->complete = true;
            }
            goto done;
        }
    }
    else {
        // Reader: stop when last scan complete.
        if(task->directories.empty() && task->files.empty()) {
            if(!task->parameters->wait_for_writers && task->last_scan) {
                if(task->io_processing == 0) {
                    task->complete = true;
                }
                goto done;
            }
            else if(task->io_processing != 0) {
                // Still work in progress. Don't restart yet.
                goto done;
            }
            else {
                string base_dir = task->parameters->read_dir + "/";
                // Need to scan dataset again.
                VPLTRACE_LOG_FINER(TRACE_APP, 0,
                                  "Task[%s] Scanning dataset once more.",
                                  task->taskName.c_str());
                task->directories.push_back(base_dir);
                task->files_done = 0;
                if(!task->parameters->wait_for_writers) {
                VPLTRACE_LOG_FINER(TRACE_APP, 0,
                                  "Task[%s] Performing last dataset scan.",
                                  task->taskName.c_str());
                    task->last_scan = true;
                }
            }
        }
    }

    // Check if there's room for another I/O in progress.
    if(task->io_processing >= task->io_limit) {
        goto done;
    }

    // Check if commit should be done (on file count or volume).
    if(task->taskType == TASK_WRITER &&
       (task->commit_active ||
        (task->parameters->commit_interval != 0 &&
         task->files_in_batch >= task->parameters->commit_interval) ||
        (task->parameters->commit_size != 0 &&
         task->bytes_in_batch >= task->parameters->commit_size))) {
        // Must wait for files to finish.
        goto done;
    }

    ioTask = new (nothrow) cloud_io_task();
    if(ioTask == NULL) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0,
                         "Task[%s] done on fail to alloc IO context.",
                         task->taskName.c_str());
        goto done;
    }

    // Choose I/O operation.
    if(task->taskType == TASK_WRITER) {
        ioTask->filename = "/" + task->taskName + "/";
        if(task->parameters->write_thumb) {
            thumbIoTask = new (nothrow) cloud_io_task();
            if(thumbIoTask == NULL) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0,
                                 "Task[%s] done on fail to alloc thumbnail IO context.",
                                 task->taskName.c_str());
                delete ioTask;
                goto done;
            }
            thumbIoTask->thumbnail = true;
            thumbIoTask->filename = ioTask->filename;
            thumbIoTask->filename += task->parameters->thumb_dir + "/";
            ioTask->filename += task->parameters->base_dir + "/";
        }

        if(task->parameters->use_manifest) {
            ioTask->filename.append(task->parameters->manifest.array[task->files_started]->path);
            ioTask->filesize = task->parameters->manifest.array[task->files_started]->size;
            if(task->parameters->write_thumb) {
                thumbIoTask->filename.append(task->parameters->manifest.array[task->files_started]->path);
            }
        }
        else {
            // Names are formulated to create nice-size directories with 1000 files
            // and 1000 directories in each one maximum.
            // We mod the file_cnt for the case where readers are waiting
            // for writers to finish.
            stringstream tmp;
            u32 filenumber = (task->files_started % task->parameters->file_cnt);
            u32 stepsize = 1000;
            // Create directory per hundred files.
            while(filenumber >= stepsize) {
                tmp << "files_" << setw(3) << setfill('0')
                    << filenumber % stepsize << "/";
                filenumber = filenumber / stepsize;
            }
            // Create file
            tmp << "file_" << setw(3) << setfill('0') << filenumber;
            ioTask->filename.append(tmp.str());
            ioTask->filesize = task->parameters->file_size;
            if(task->parameters->write_thumb) {
                thumbIoTask->filename.append(tmp.str());
            }
        }

        ioTask->read = false;
        ioTask->file = true;
        task->files_in_batch++;
        task->bytes_in_batch += ioTask->filesize;
        if(task->parameters->write_thumb) {
            thumbIoTask->filesize = task->parameters->thumb_size;
            thumbIoTask->read = false;
            thumbIoTask->file = true;
            task->files_in_batch++;
            task->bytes_in_batch += ioTask->filesize;
        }
    }
    else {
        // Choose next file in the files queue.
        // If none, choose to read a directory instead.
        if(task->files.empty()) {
            ioTask->filename = task->directories.front();
            ioTask->filesize = 0;
            task->directories.pop_front();
            ioTask->file = false;
        }
        else {
            pair<string, u64> readFile = task->files.front();
            ioTask->filename = readFile.first;
            ioTask->filesize = readFile.second;
            task->files.pop_front();
            ioTask->file = true;
        }

        ioTask->read = true;
    }

    task->files_started++;
    task->io_processing++;
    if(thumbIoTask != NULL) {
        task->io_processing++;
    }

    VPLMutex_Unlock(&(task->mutex));
    
    // If writing or verifying, compute file pattern.
    if(task->parameters->source_data == NULL && ioTask->file &&
       (!ioTask->read || task->parameters->verify_read)) {
        // Set file data patern: SHA1(userID + base_filename) bytes 0..15 repeated to fill.
        // base_filename is the file name with the worker identity stripped.
        CSL_ShaContext context;
        char calc_hash[CSL_SHA1_DIGESTSIZE] = {0};
        string base_filename = ioTask->filename.substr(ioTask->filename.find_first_of('/', 1));

        CSL_ResetSha(&context);
        CSL_InputSha(&context, (void*)&(task->userId), sizeof(task->userId));
        CSL_InputSha(&context, (void*)(base_filename.data()), base_filename.size());
        CSL_ResultSha(&context, (u8*)calc_hash);
        memcpy(ioTask->src_pattern, calc_hash, 16);
        if(thumbIoTask) {
            base_filename = thumbIoTask->filename.substr(thumbIoTask->filename.find_first_of('/', 1));
            CSL_ResetSha(&context);
            CSL_InputSha(&context, (void*)&(task->userId), sizeof(task->userId));
            CSL_InputSha(&context, (void*)(base_filename.data()), base_filename.size());
            CSL_ResultSha(&context, (u8*)calc_hash);
            memcpy(thumbIoTask->src_pattern, calc_hash, 16);
        }
    }

    // Set callback for completion.
    ioTask->main_task = task;
    ioTask->callback = cloud_task_done;
    ioTask->context = ioTask;
    ioTask->rv = 0;
    if(thumbIoTask != NULL) {
        thumbIoTask->main_task = task;
        thumbIoTask->callback = cloud_task_done;
        thumbIoTask->context = thumbIoTask;
        thumbIoTask->rv = 0;
    }

    VPLTRACE_LOG_FINE(TRACE_APP, 0,
                      "Task[%s] starting IO: %s [%s] size "FMTu64".",
                      task->taskName.c_str(),
                      !ioTask->read ? "write file" : ioTask->file ?
                      "read file" : "read directory",
                      ioTask->filename.c_str(), ioTask->filesize);

    if(ioTask->read) {
        if(ioTask->file) {
            cloud_read_file(ioTask);
        }
        else {
            cloud_read_directory(ioTask);
        }
    } else {
        cloud_write_file(ioTask);
        if(thumbIoTask != NULL) {
            cloud_write_file(thumbIoTask);
        }
    }

    // Repeat work to launch next I/O task (if any).
    vsTest_worker_add_task(cloud_do_work, ctx);
    
    return;
 done:
    if(task->complete && !task->finishing) {
        task->stop = VPLTime_GetTime(); // stop clock on read/write activity
        finished = true;
        if(task->taskType == TASK_WRITER && !task->verified_merge) {
            do_verify_merge = true;
            task->verified_merge = true;
        } 
        else {
            task->finishing = true;
        }
    }
    VPLMutex_Unlock(&(task->mutex));
    
    if(finished) {
        if(task->taskType == TASK_WRITER && do_verify_merge) {
            cloud_do_stat(ctx);
        }
        else {
            VPLTRACE_LOG_FINER(TRACE_APP, 0,
                               "Task[%s] Done.",
                               task->taskName.c_str());
            cloud_done(ctx);
        }
    }
}

void cloud_task_done(void* ctx)
{
    cloud_io_task *ioTask = (cloud_io_task*)(ctx);
    load_task *task = ioTask->main_task;
    bool do_commit = false;
    bool do_next_io = true;

    VPLMutex_Lock(&(task->mutex));

    common_close_file(ioTask->file_handle);
    task->io_processing--;

    if(ioTask->rv == 0) {
        if(!ioTask->thumbnail) {
            task->files_done++;
        }
        if(ioTask->read) {
            if(ioTask->file) {
                task->read_time += ioTask->accumulated_time;
                task->reads++;
                task->data_read += ioTask->filesize;
            }
            else {
                task->dir_read_time += ioTask->accumulated_time;
                task->dir_reads++;
            }
        }
        else {
            task->commit_pending = true;
            task->write_time += ioTask->accumulated_time;
            task->writes++;
            task->data_write += ioTask->filesize;
        }
    }
    else if(ioTask->rv == VSSI_CONFLICT){
        // Need to have all I/O ended to reset conflict and restart.
        // Mainly applies to read.
        if(task->io_processing == 0) {
            VSSI_ClearConflict(task->handle);
            task->files_started = task->files_done;
            task->files_in_batch = 0;
            task->bytes_in_batch = 0;
            task->last_scan = false;
            task->commit_pending = false;
            task->directories.clear();
            task->files.clear();
            VPLTRACE_LOG_INFO(TRACE_APP, 0, 
                              "Task[%s] user_id "FMTu64" Read conflict. Reset and repeat.",
                              task->taskName.c_str(), task->userId);
        }
        else {
            // Must wait for outstanding I/O to complete.
            VPLTRACE_LOG_INFO(TRACE_APP, 0, 
                              "Task[%s] user_id "FMTu64" Read conflict. Wait for %d other tasks.",
                              task->taskName.c_str(), task->userId,
                              task->io_processing);
            do_next_io = false;
        }
    }
    else {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: Task[%s] user_id "FMTu64" %s %s: %d\n",
                         task->taskName.c_str(), task->userId,
                         !ioTask->read ? "write file" :
                         ioTask->file ? "read file" : "read directory",
                         ioTask->filename.c_str(), ioTask->rv);
        task->failed = true;
    }

    delete ioTask;

    // Check if commit needed.
    if(!task->failed &&
       task->taskType == TASK_WRITER && !task->commit_active &&
       task->commit_pending && task->io_processing == 0 &&
       (task->files_done == task->parameters->file_cnt ||
        (task->parameters->commit_interval > 0 &&
         task->parameters->commit_interval <= task->files_in_batch) ||
        (task->parameters->commit_size > 0 &&
         task->parameters->commit_size <= task->bytes_in_batch))) {
        do_commit = true;
        task->commit_active = false;
    }

    VPLMutex_Unlock(&task->mutex);

    if(do_commit) {
        task->op_start = VPLTime_GetTime();
        VPLTRACE_LOG_FINER(TRACE_APP, 0,
                          "Task[%s] Commit changes.",
                          task->taskName.c_str());
        VSSI_Commit(task->handle, task, cloud_do_commit_done_helper);
    }
    else if(do_next_io) {
        // Next sub-task starts now.
        cloud_do_work(task);
    }
}

void cloud_do_commit_done_helper(void* ctx, VSSI_Result result)
{
    load_task *task = (load_task*)(ctx);
    task->op_end = VPLTime_GetTime();

    task->rv = result;

    vsTest_worker_add_task(cloud_do_commit_done, ctx);

    VPLTRACE_LOG_FINER(TRACE_APP, 0,
                      "Task[%s] Commit changes done:%d.",
                      task->taskName.c_str(), result);
}

void cloud_do_commit_done(void* ctx)
{
    load_task *task = (load_task*)(ctx);

    VPLMutex_Lock(&task->mutex);
    task->commit_time += task->op_end - task->op_start;
    task->files_in_batch = 0;
    task->bytes_in_batch = 0;
    task->commit_pending = false;
    task->commit_active = false;

    if(task->rv) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Task[%s] Commit: %d.",
                         task->taskName.c_str(), task->rv);
        task->failed = true;
        vsTest_worker_add_task(cloud_do_work, ctx);
    }
    else {
        task->commits++;
        if(task->parameters->verify_write) {
            // At least stat root directory to confirm commit done.
            vsTest_worker_add_task(cloud_do_stat, ctx);
        }
        else {
            // Start next task.
            vsTest_worker_add_task(cloud_do_work, ctx);
        }
    }

    VPLMutex_Unlock(&task->mutex);
}

void cloud_do_stat(void* ctx) {
    load_task *task = (load_task*)(ctx);

    task->op_start = VPLTime_GetTime();
    // Stat non-existent file to confirm changes have been merged.
    // Read-type commands can only complete when all change logs are merged.
    VSSI_Stat(task->handle, "/nonexistent",
              &task->stats,
              ctx, cloud_do_stat_done_helper);
}

void cloud_do_stat_done_helper(void* ctx, VSSI_Result result)
{
    load_task *task = (load_task*)(ctx);

    task->op_end = VPLTime_GetTime();
    if(result == VSSI_CONFLICT) {
        task->rv = result;
        // Clear conflict and continue on.
        VSSI_ClearConflict(task->handle);
    }
    // Any result is OK.
    vsTest_worker_add_task(cloud_do_stat_done, ctx);
}

void cloud_do_stat_done(void* ctx)
{
    load_task *task = (load_task*)(ctx);

    VPLMutex_Lock(&(task->mutex));

    task->verify_time += task->op_end - task->op_start;
    if(task->rv == VSSI_CONFLICT) {
        // Clear conflict and continue on.
        VSSI_ClearConflict(task->handle);
    }

    if(task->stats) {
        free(task->stats);
        task->stats = NULL;
    }

    VPLMutex_Unlock(&(task->mutex));

    VPLTRACE_LOG_FINER(TRACE_APP, 0,
                      "Task[%s] Commit changes confirmed:%d.",
                       task->taskName.c_str(), task->rv);
    vsTest_worker_add_task(cloud_do_work, ctx);
}

void cloud_done(void* ctx)
{
    load_task *task = (load_task*)(ctx);
    pctest_context_t test_context;

    if(VPLSem_Init(&(test_context.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create sempahore.");
        return;
    }
    // Close object
    VPLTRACE_LOG_INFO(TRACE_APP, 0,
                      "Task[%s] Closing object.",
                      task->taskName.c_str());

    // TODO: Close signle Object
    // Current callback based design is not for single object open and close,
    // This shoud not affect test result.
#if 0
    VPLMutex_Lock(&(task->obj_mutex));
    if(task->handle != NULL) {
        VSSI_CloseObject(task->handle, &test_context, common_callback);
        VPLSem_Wait(&(test_context.sem));
        task->handle = NULL;
    }
    VPLMutex_Unlock(&(task->obj_mutex));
#endif
    vsTest_worker_add_task(cloud_close_done, ctx);
}

// Current callback based design is not for single object open and close,
#if 0
void cloud_close_done_helper(void* ctx, VSSI_Result result)
{
    // Don't care about close result.
    vsTest_worker_add_task(cloud_close_done, ctx);
}
#endif

void cloud_close_done(void* ctx) {
    load_task *task = (load_task*)(ctx);

    // Call callback on task complete.
    VPLTRACE_LOG_INFO(TRACE_APP, 0,
                      "Task[%s] Over. file progress: %d/%d/%d result:%d",
                      task->taskName.c_str(),
                      task->files_started, task->files_done, 
                      task->parameters->file_cnt, task->rv);
    (task->parameters->callback)(task->context);
}

void cloud_write_file(void* ctx)
{
    cloud_io_task *ioTask = (cloud_io_task*)(ctx);
    load_task *task = ioTask->main_task;
    u32 num_blocks;
    u32 block_tasks;
    u64 block_offset = 0;
    int op_rv = 0;
    std::string parent_dir_name;

    parent_dir_name = ioTask->filename.substr(0, ioTask->filename.find_last_of('/'));

    common_mkdirp(task->handle, parent_dir_name);

    op_rv = common_open_file(task->handle, ioTask->file_handle, ioTask->filename);

    // Allocate block contexts sufficient to write this file.
    // Each context takes a fraction of the file.
    // Max contexts: lesser of block_limit or (file size / block size).
    num_blocks = ((ioTask->filesize + 
                   ioTask->main_task->parameters->xfer_size - 1) / 
                  ioTask->main_task->parameters->xfer_size);
    block_tasks = MIN(ioTask->block_limit, num_blocks);
    
    VPLTRACE_LOG_FINEST(TRACE_APP, 0,
                        "Task[%s] Write file [%s] size "FMTu64" in %u blocks by %u tasks.",
                        ioTask->main_task->taskName.c_str(),
                        ioTask->filename.c_str(), ioTask->filesize,
                        num_blocks, block_tasks);
    
    VPLMutex_Lock(&ioTask->mutex);
    
    // Launch block writers
    for(u32 i = 0; i < block_tasks; i++) {
        u32 buf_offset = 0;
        // Assign fractional share to each task.
        u64 length = (ioTask->main_task->parameters->xfer_size * 
                      (num_blocks / (block_tasks - i)));
        num_blocks -= (num_blocks / (block_tasks - i));
        length = MIN(length, (ioTask->filesize - block_offset));

        VPLTRACE_LOG_FINEST(TRACE_APP, 0,
                            "Task[%s] Write file [%s] size "FMTu64" task %u from offset "FMTu64" size "FMTu64".",
                            ioTask->main_task->taskName.c_str(),
                            ioTask->filename.c_str(), ioTask->filesize,
                            i, block_offset, length);
        cloud_block_io_task* blockTask = 
            new cloud_block_io_task(ioTask, block_offset, length);
        block_offset += length;

        // Allocate transfer block and populate with pattern.
        // Don't bother if source file provided.
        if(ioTask->main_task->parameters->source_data == NULL) {
            blockTask->buffer = new char[ioTask->main_task->parameters->xfer_size];
            while(buf_offset < ioTask->main_task->parameters->xfer_size) {
                memcpy(blockTask->buffer + buf_offset, ioTask->src_pattern, 16);
                buf_offset += 16;
            }
        }       
        ioTask->reqs_pending++;
        if(op_rv == VSSI_SUCCESS || op_rv == VSSI_EXISTS) {
            vsTest_worker_add_task(cloud_write_block, blockTask);
        } else {
            blockTask->rv = op_rv;
            vsTest_worker_add_task(cloud_write_block_done, blockTask);
        }
    }

    // Launch set size, set metadata as needed. (Skip metadata on thumbnails.)
    ioTask->reqs_pending++;
    vsTest_worker_add_task(cloud_write_size, ioTask);
    if(ioTask->main_task->parameters->use_metadata &&
       !ioTask->thumbnail) {
        ioTask->reqs_pending++;
        vsTest_worker_add_task(cloud_write_metadata, ioTask);
    }
    VPLMutex_Unlock(&ioTask->mutex);
}

void cloud_write_block(void* ctx)
{
    cloud_block_io_task* blockTask = (cloud_block_io_task*)(ctx);
    cloud_io_task *ioTask = blockTask->parent;
    load_task *task = ioTask->main_task;

    // TODO: Check if throttling delay required before write.
    //       (Requires timed-delay thread.)
    // FORNOW: Throttle disabled.

    u64 offset = blockTask->offset + blockTask->offset_so_far;
    blockTask->req_len = MIN(blockTask->length - blockTask->offset_so_far,
                             task->parameters->xfer_size);

    VPLTRACE_LOG_FINEST(TRACE_APP, 0,
                        "Task[%s] Write block from offset "FMTu64" file [%s] size "FMTu32".",
                        task->taskName.c_str(), offset,
                        ioTask->filename.c_str(), blockTask->req_len);

    // TODO: Should source data be modified when used?

    blockTask->start_time = VPLTime_GetTime();
    VSSI_WriteFile(ioTask->file_handle,
                   offset, &blockTask->req_len,
                   task->parameters->source_data == NULL ? blockTask->buffer :
                   task->parameters->source_data + offset,
                   ctx, cloud_write_block_done_helper);
}

void cloud_write_block_done_helper(void* ctx, VSSI_Result result)
{
    cloud_block_io_task* blockTask = (cloud_block_io_task*)(ctx);

    blockTask->end_time = VPLTime_GetTime();
    blockTask->rv = result;
    vsTest_worker_add_task(cloud_write_block_done, ctx);
}

void cloud_write_block_done(void* ctx)
{
    cloud_block_io_task* blockTask = (cloud_block_io_task*)(ctx);
    cloud_io_task *ioTask = blockTask->parent;

    blockTask->accumulated_time += blockTask->end_time - blockTask->start_time;

    if(blockTask->rv) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Task[%s]. Write [%s]: %d.",
                         blockTask->parent->main_task->taskName.c_str(),
                         blockTask->parent->filename.c_str(), 
                         blockTask->rv);
        cloud_write_done(ioTask, blockTask->rv, blockTask->accumulated_time);
        delete blockTask;
    }
    else {
        blockTask->offset_so_far += blockTask->req_len;
        VPLTRACE_LOG_FINEST(TRACE_APP, 0,
                            "Task[%s] Write block file [%s] done. "FMTu64"/"FMTu64" complete.",
                            ioTask->main_task->taskName.c_str(), 
                            ioTask->filename.c_str(),
                            blockTask->offset_so_far, blockTask->length);

        if(blockTask->offset_so_far >= blockTask->length) {
            cloud_write_done(ioTask, blockTask->rv, blockTask->accumulated_time);
            delete blockTask;
        }
        else {
            cloud_write_block(blockTask);
        }
    }
}

void cloud_write_metadata(void* ctx)
{
    cloud_io_task *ioTask = (cloud_io_task*)(ctx);
    VPLTime_t write_time = VPLTime_GetTime();

    ioTask->start_meta_time = VPLTime_GetTime();
    VSSI_SetMetadata(ioTask->main_task->handle, ioTask->filename.c_str(),
                     0,
                     sizeof(write_time), (const char*)&write_time,
                     ctx, cloud_write_metadata_done_helper);
    VPLTRACE_LOG_FINEST(TRACE_APP, 0,
                        "Task[%s] Set metadata for file [%s].",
                        ioTask->main_task->taskName.c_str(), ioTask->filename.c_str());    
}

void cloud_write_metadata_done_helper(void* ctx, VSSI_Result result)
{
    cloud_io_task *ioTask = (cloud_io_task*)(ctx);

    ioTask->end_meta_time = VPLTime_GetTime();
    switch(ioTask->rv) {
    case VSSI_SUCCESS:
        // failure is always an option
        ioTask->rv = result;
        break;
    case VSSI_CONFLICT:
        // conflicts can become errors
        if(result != VSSI_SUCCESS) {
            ioTask->rv = result;
        }
        break;
    default:
        // errors are sticky
        break;
    }

    if(result) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Task[%s] Write metadata [%s]: %d.",
                         ioTask->main_task->taskName.c_str(), ioTask->filename.c_str(), result);
    }
    vsTest_worker_add_task(cloud_write_metadata_done, ctx);
}

void cloud_write_metadata_done(void* ctx)
{
    cloud_io_task *ioTask = (cloud_io_task*)(ctx);

    VPLTRACE_LOG_FINEST(TRACE_APP, 0,
                        "Task[%s] Write metadata [%s] done.",
                        ioTask->main_task->taskName.c_str(), 
                        ioTask->filename.c_str());

    cloud_write_done(ioTask, ioTask->rv,
                     ioTask->end_meta_time - ioTask->start_meta_time);
}

void cloud_write_size(void* ctx)
{
    cloud_io_task *ioTask = (cloud_io_task*)(ctx);

    ioTask->start_size_time = VPLTime_GetTime();
    VSSI_TruncateFile(ioTask->file_handle,
                      ioTask->filesize,
                      ctx, cloud_write_size_done_helper);
    VPLTRACE_LOG_FINEST(TRACE_APP, 0,
                        "Task[%s] Set size for file [%s] size "FMTu64".",
                        ioTask->main_task->taskName.c_str(), ioTask->filename.c_str(), ioTask->filesize);
}

void cloud_write_size_done_helper(void* ctx, VSSI_Result result)
{
    cloud_io_task *ioTask = (cloud_io_task*)(ctx);

    ioTask->end_size_time = VPLTime_GetTime();
    switch(ioTask->rv) {
    case VSSI_SUCCESS:
        // failure is always an option
        ioTask->rv = result;
        break;
    case VSSI_CONFLICT:
        // conflicts can become errors
        if(result != VSSI_SUCCESS) {
            ioTask->rv = result;
        }
        break;
    default:
        // errors are sticky
        break;
    }

    if(result) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Task[%s] Write size [%s]: %d.",
                         ioTask->main_task->taskName.c_str(), ioTask->filename.c_str(), result);
    }

    vsTest_worker_add_task(cloud_write_size_done, ctx);
}

void cloud_write_size_done(void* ctx)
{
    cloud_io_task *ioTask = (cloud_io_task*)(ctx);

    VPLTRACE_LOG_FINEST(TRACE_APP, 0,
                        "Task[%s] Write size [%s] done.",
                        ioTask->main_task->taskName.c_str(), 
                        ioTask->filename.c_str());

    cloud_write_done(ioTask, ioTask->rv,
                     ioTask->end_size_time - ioTask->start_size_time);
}

void cloud_write_done(cloud_io_task *ioTask, int result, VPLTime_t time)
{
    bool done = false;

    VPLMutex_Lock(&ioTask->mutex);

    switch(ioTask->rv) {
    case VSSI_SUCCESS:
        // failure is always an option
        ioTask->rv = result;
        break;
    case VSSI_CONFLICT:
        // conflicts can become errors
        if(result != VSSI_SUCCESS) {
            ioTask->rv = result;
        }
        break;
    default:
        // errors are sticky
        break;
    }
     
    ioTask->reqs_pending--;
    ioTask->accumulated_time += time;
    if(ioTask->reqs_pending == 0) {
        done = true;
    }

    VPLMutex_Unlock(&ioTask->mutex);

    if(done) {
        VPLTRACE_LOG_FINER(TRACE_APP, 0,
                           "Task[%s] Write file [%s] size "FMTu64" complete.",
                           ioTask->main_task->taskName.c_str(), ioTask->filename.c_str(), ioTask->filesize);
        
        (ioTask->callback)(ioTask->context);
    }
}

void cloud_read_file(void* ctx)
{
    cloud_io_task *ioTask = (cloud_io_task*)(ctx);
    load_task *task = ioTask->main_task;
    u32 num_blocks;
    u32 block_tasks;
    u64 block_offset = 0;
    int op_rv = 0;
    std::string parent_dir_name;

    parent_dir_name = ioTask->filename.substr(0, ioTask->filename.find_last_of('/'));

    common_mkdirp(task->handle, parent_dir_name);

    op_rv = common_open_file(task->handle, ioTask->file_handle, ioTask->filename);

    // Allocate block contexts sufficient to write this file.
    // Each context takes a fraction of the file.
    // Max contexts: lesser of block_limit or (file size / block size).
    num_blocks = ((ioTask->filesize + 
                   ioTask->main_task->parameters->xfer_size - 1) / 
                  ioTask->main_task->parameters->xfer_size);
    block_tasks = MIN(ioTask->block_limit, num_blocks);
    
    if(block_tasks > 0) {
        VPLTRACE_LOG_FINEST(TRACE_APP, 0,
                            "Task[%s] Read file [%s] size "FMTu64" in %u blocks by %u tasks.",
                            ioTask->main_task->taskName.c_str(),
                            ioTask->filename.c_str(), ioTask->filesize,
                            num_blocks, block_tasks);
        
        VPLMutex_Lock(&ioTask->mutex);
        
        // Launch block readers
        for(u32 i = 0; i < block_tasks; i++) {
            // Assign fractional share to each task.
            u64 length = (ioTask->main_task->parameters->xfer_size * 
                          (num_blocks / (block_tasks - i)));
            length = MIN(length, (ioTask->filesize - block_offset));
            num_blocks -= (num_blocks / (block_tasks - i));
            
            VPLTRACE_LOG_FINEST(TRACE_APP, 0,
                                "Task[%s] Read file [%s] size "FMTu64" block %u from offset "FMTu64" size "FMTu64".",
                                ioTask->main_task->taskName.c_str(),
                                ioTask->filename.c_str(), ioTask->filesize,
                                i, block_offset, length);
            cloud_block_io_task* blockTask = 
                new cloud_block_io_task(ioTask, block_offset, length);
            block_offset += length;
            
            // Allocate transfer block.
            blockTask->buffer = new char[ioTask->main_task->parameters->xfer_size];
        
            ioTask->reqs_pending++;
            if(op_rv == VSSI_SUCCESS || op_rv == VSSI_EXISTS) {
                vsTest_worker_add_task(cloud_read_block, blockTask);
            } else {
                blockTask->rv = op_rv;
                vsTest_worker_add_task(cloud_read_block_done, blockTask);
            }
        }

        VPLMutex_Unlock(&ioTask->mutex);
    }
    else {
        // Nothing to read -> done.
        (ioTask->callback)(ioTask->context);
    }
}

void cloud_read_block(void* ctx)
{
    cloud_block_io_task* blockTask = (cloud_block_io_task*)(ctx);
    cloud_io_task *ioTask = blockTask->parent;
    load_task *task = ioTask->main_task;

    // TODO: Check if throttling delay required before read.
    //       (Requires timed-delay thread.)
    // FORNOW: Throttle disabled.

    u64 offset = blockTask->offset + blockTask->offset_so_far;
    blockTask->req_len = MIN(blockTask->length - blockTask->offset_so_far,
                             task->parameters->xfer_size);

    VPLTRACE_LOG_FINEST(TRACE_APP, 0,
                        "Task[%s] Read block from offset "FMTu64" file [%s] size "FMTu32".",
                        task->taskName.c_str(), offset,
                        ioTask->filename.c_str(), blockTask->req_len);

    blockTask->start_time = VPLTime_GetTime();
    VSSI_ReadFile(ioTask->file_handle,
                  offset, &blockTask->req_len,
                  blockTask->buffer,
                  ctx, cloud_read_block_done_helper);
}

void cloud_read_block_done_helper(void* ctx, VSSI_Result result)
{
    cloud_block_io_task* blockTask = (cloud_block_io_task*)(ctx);

    blockTask->end_time = VPLTime_GetTime();
    blockTask->rv = result;
    vsTest_worker_add_task(cloud_read_block_done, ctx);
}

void cloud_read_block_done(void* ctx)
{
    cloud_block_io_task* blockTask = (cloud_block_io_task*)(ctx);
    cloud_io_task *ioTask = blockTask->parent;

    blockTask->accumulated_time += blockTask->end_time - blockTask->start_time;

    if(blockTask->rv) {
        cloud_read_done(ioTask, blockTask->rv, blockTask->accumulated_time);
        delete blockTask;
    }
    else {
        blockTask->offset_so_far += blockTask->req_len;
        if(ioTask->main_task->parameters->verify_read) {
            cloud_read_confirm_block(blockTask);
        }
        else {
            if(blockTask->offset_so_far >= blockTask->length) {
                cloud_read_done(ioTask, blockTask->rv, blockTask->accumulated_time);
                delete blockTask;
            }
            else {
                cloud_read_block(blockTask);
            }
        }
    }
}

void cloud_read_confirm_block(cloud_block_io_task* blockTask)
{
    size_t cmp_offset = 0;
    cloud_io_task *ioTask = blockTask->parent;

    while(blockTask->rv == 0 &&
          cmp_offset < blockTask->req_len) {
        size_t cmp_len = MIN(16, blockTask->req_len - cmp_offset);
        if(memcmp(blockTask->parent->src_pattern, 
                  blockTask->buffer + cmp_offset, cmp_len) != 0) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Task[%s] Read [%s] compare at offset "FMTu64" fails.",
                             blockTask->parent->main_task->taskName.c_str(),
                             blockTask->parent->filename.c_str(),
                             blockTask->confirm_offset_so_far);
            blockTask->rv = VSSI_INVALID;
            break;
        }
        cmp_offset += 16;
        blockTask->confirm_offset_so_far += 16;
    }
    
    if(blockTask->rv != 0) {
        cloud_read_done(blockTask->parent, blockTask->rv, blockTask->accumulated_time);
    }
    else {
        if(blockTask->offset_so_far >= blockTask->length) {
            cloud_read_done(ioTask, blockTask->rv, blockTask->accumulated_time);
            delete blockTask;
        }
        else {
            cloud_read_block(blockTask);
        }
    }
}

void cloud_read_done(cloud_io_task *ioTask, int result, VPLTime_t time)
{
    bool done = false;

    VPLMutex_Lock(&ioTask->mutex);

    switch(ioTask->rv) {
    case VSSI_SUCCESS:
        // failure is always an option
        ioTask->rv = result;
        break;
    case VSSI_CONFLICT:
        // conflicts can become errors
        if(result != VSSI_SUCCESS) {
            ioTask->rv = result;
        }
        break;
    default:
        // errors are sticky
        break;
    }

    ioTask->reqs_pending--;
    ioTask->accumulated_time += time;
    if(ioTask->reqs_pending == 0) {
        done = true;
    }

    VPLMutex_Unlock(&ioTask->mutex);

    if(done) {
        VPLTRACE_LOG_FINER(TRACE_APP, 0,
                           "Task[%s] Read file [%s] size "FMTu64" complete.",
                           ioTask->main_task->taskName.c_str(), ioTask->filename.c_str(), ioTask->filesize);
        
        (ioTask->callback)(ioTask->context);
    }
}

void cloud_read_directory(void* ctx)
{
    cloud_io_task *ioTask = (cloud_io_task*)(ctx);
    load_task *task = ioTask->main_task;

    if(ioTask->rv == VSSI_CONFLICT) {
        VPLTRACE_LOG_FINER(TRACE_APP, 0,
                           "Task[%s] Read Dir [%s] conflict detected. Repeating.",
                           ioTask->main_task->taskName.c_str(), ioTask->filename.c_str());
        VSSI_ClearConflict(task->handle);
        task->last_scan = false;
    }

    ioTask->dir = NULL;
    ioTask->start_time = VPLTime_GetTime();

    VPLTRACE_LOG_FINER(TRACE_APP, 0,
                       "Task[%s] Read Dir [%s].",
                       ioTask->main_task->taskName.c_str(), ioTask->filename.c_str());
    VSSI_OpenDir(task->handle, ioTask->filename.c_str(),
                 &(ioTask->dir),
                 ctx, cloud_read_directory_done);
}

void cloud_read_directory_done(void* ctx, VSSI_Result result)
{
    cloud_io_task *ioTask = (cloud_io_task*)(ctx);

    VPLTRACE_LOG_FINER(TRACE_APP, 0,
                       "Task[%s] Read Dir [%s] done:%d.",
                       ioTask->main_task->taskName.c_str(),
                       ioTask->filename.c_str(), result);

    ioTask->end_time = VPLTime_GetTime();

    switch(ioTask->rv) {
    case VSSI_SUCCESS:
        // failure is always an option
        ioTask->rv = result;
        break;
    case VSSI_CONFLICT:
        // conflicts can become errors
        if(result != VSSI_SUCCESS) {
            ioTask->rv = result;
        }
        break;
    default:
        // errors are sticky
        break;
    }

    if(ioTask->rv == VSSI_SUCCESS) {
        vsTest_worker_add_task(cloud_read_directory_process, ctx);
    }
    else {
        vsTest_worker_add_task(cloud_read_directory_end, ctx);
    }
}

void cloud_read_directory_process(void* ctx)
{
    cloud_io_task *ioTask = (cloud_io_task*)(ctx);
    load_task *task = ioTask->main_task;
    VSSI_Dirent* dirent;

    VPLMutex_Lock(&(task->mutex));

    ioTask->accumulated_time += ioTask->end_time - ioTask->start_time;

    // For each file and directory, check if known.
    // If unknown or changed version, add to appropriate queue.
    while((dirent = VSSI_ReadDir(ioTask->dir)) != NULL) {
        string name = ioTask->filename + dirent->name;
        if(dirent->isDir) {
            name += "/";
        }

        VPLTRACE_LOG_FINER(TRACE_APP, 0,
                          "Task[%s] Read Dir [%s] checking entry [%s].",
                          ioTask->main_task->taskName.c_str(), 
                          ioTask->filename.c_str(), name.c_str());

        map<string, u64>::iterator it = task->read_files.find(name);
        if(it == task->read_files.end() ||
           it->second != dirent->changeVer) {
            if(dirent->isDir) {
                task->directories.push_back(name);
            }
            else {
                task->files.push_back(make_pair(name, dirent->size));
            }
        }
        task->read_files[name] = dirent->changeVer;
    }

    VPLMutex_Unlock(&(task->mutex));

    cloud_read_directory_end(ctx);
}

void cloud_read_directory_end(void* ctx)
{
    cloud_io_task *ioTask = (cloud_io_task*)(ctx);

    VPLTRACE_LOG_FINER(TRACE_APP, 0,
                      "Task[%s] Read Dir [%s] complete.",
                      ioTask->main_task->taskName.c_str(),
                      ioTask->filename.c_str());

    (ioTask->callback)(ioTask->context);
}
