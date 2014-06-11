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

#define _XOPEN_SOURCE 600

#include "managed_dataset.hpp"
#include "DatasetDB.hpp"
#include "utf8.hpp"
#include "vplex_assert.h"
#include "gvm_file_utils.hpp"
#include "gvm_rm_utils.hpp"
#include "vplex_vs_directory.h"
#include "ccdi.hpp"

#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <stdio.h>
#include <fcntl.h>

#ifndef _MSC_VER
#include <unistd.h>
#include <utime.h>
#endif

#include <iostream>
#include <sstream>
#include <map>
#include <set>
#include <stack>
#include <algorithm>

#include "vpl_conv.h"
#include "vpl_fs.h"
#include "vplex_file.h"
#include "vplex_trace.h"

#include "vss_comm.h"

#define DATASETDB_FILENAME "db"

#define ATTRS_SET_MASK  (VSSI_ATTR_READONLY | VSSI_ATTR_SYS | \
    VSSI_ATTR_HIDDEN | VSSI_ATTR_ARCHIVE)
#define ATTRS_CLR_MASK  (ATTRS_SET_MASK)

#define MD_SYNC_TIME_SECS   (1)
#define MD_BACKUP_SECS   (15*60) 
#define MD_FORCE_BACKUP  (60*60) 
#define FSCK_DELAY        VPLTIME_FROM_SEC(120)    // 2 minute delay

// This file indicates that the database was completely removed
// during the recovery process. If we fail to create a new db
// we give up rather than enter an infinite loop of trying to create
// new databases.
#define MD_NO_DB_MARKER         "no-db"
#define MD_DB_BACKUP_MARKER     "db-needs-backup"
#define MD_DB_NEEDS_FSCK        "db-needs-fsck"
#define MD_DB_IS_OPEN           "db-is-open"

// If we want to support N failures this should be re-written
// to compute the name of the file. This is hard-coded to support
// only 2.
#define MD_DB_BACKUP_ERR1_MARKER    "db-backup-fail-1"
#define MD_DB_BACKUP_ERR2_MARKER    "db-backup-fail-2"

// If we want to support N failures this should be re-written
// to compute the name of the file. This is hard-coded to support
// only 2.
#define MD_DB_FSCK_ERR1_MARKER    "db-fsck-fail-1"
#define MD_DB_FSCK_ERR2_MARKER    "db-fsck-fail-2"

// If we fail to open twice, including following a restart
// then we force a restore
#define MD_DB_OPEN_ERR1_MARKER  "db-open-fail-1"

// This file indicates that the system "lost+found" folder
// has already been created
#define MD_LOST_AND_FOUND_SYSTEM    "lost-and-found-system"


using namespace std;

// Basic directories for all save states.
static const std::string base_dirs[] = {"data/", // user data
                                        "changes/", // change logs
                                        "references/", // reference files
                                        ""/*terminator*/};
enum {
    DATA = 0,
    CHANGES,
    REFERENCES,
};
#define BASE_DIR(i) (base_dirs[(i)])

static const int MD_WORKER_STACK_SIZE = (32*1024);

managed_dataset::managed_dataset(dataset_id& id, int type, std::string& storage_base, vss_server* server) :
    dataset(id, type, server),
    version(0),
    mini_tran_open(false),
    tran_is_open(false),
    tran_do_commit(false),
    tran_needs_commit(false),
    dbase_needs_backup(false),
    backup_done(false),
    fsck_done(false),
    fsck_needed(false),
    shutdown_sync(true),
    remaining_space_size(0)
{
    stringstream dataset_name;

    dataset_name << hex << setfill('0') << setw(16) << uppercase << id.uid << "/";
    dataset_name << hex << setfill('0') << setw(16) << uppercase << id.did << "/";
    dir_name = storage_base + dataset_name.str();
    tmp_to_delete_dir = storage_base + "to_delete";

    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      "Dataset %"PRIx64":%"PRIx64" dir name: %s.",
                      id.uid, id.did, dir_name.c_str());

    datasetdb_options = 0;
    if (is_case_insensitive()) {
        datasetdb_options |= DATASETDB_OPTION_CASE_INSENSITIVE;
    }

    VPLMutex_Init(&sync_mutex);
    VPLMutex_Init(&access_mutex);
    VPLMutex_Init(&fd_mutex);
    VPLCond_Init(&per_sync_cond);

    dbase_activate();

    //update the remaining_space_size for init.
    u64 disk_size = 0; // dummy
    int err = get_remaining_space(dir_name, disk_size, remaining_space_size);
    if(err) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed update remaining space %d", err);
    }
}

static std::string normalizePath(const std::string& path)
{
    std::string rv = path;

    // Component names must not have trailing NULL characters.
    while(rv.size() > 0 && rv[rv.size() - 1] == '\0') {
        rv.erase(rv.size() - 1, 1);
    }

    // Component names must not have multiple consecutive,
    // leading, or trailing slashes.
    while(rv.find("//") != std::string::npos) {
        rv.replace(rv.find("//"), 2, "/");
    }
    while(rv.size() > 0 && rv[0] == '/') {
        rv.erase(0, 1);
    }
    while(rv.size() > 0 && rv[rv.size() - 1] == '/') {
        rv.erase(rv.size() - 1, 1);
    }

    return rv;
}

static bool location_exists(const std::string &data_path, const std::string &path)
{
    bool is_present;
    string full_path;

    full_path = data_path + path;
    is_present = (VPLFile_CheckAccess(full_path.c_str(), VPLFILE_CHECKACCESS_EXISTS) == VPL_OK);

    return is_present;
}

static VPLThread_return_t per_sync_helper(VPLThread_arg_t dataset)
{
    managed_dataset* md = (managed_dataset*)dataset;
    md->file_periodic_sync();
    return VPLTHREAD_RETURN_VALUE_UNUSED;
}

void managed_dataset::period_thread_start(void)
{
    int rv;
    VPLThread_attr_t thread_attr;
    VPLThread_AttrInit(&thread_attr);
    VPLThread_AttrSetStackSize(&thread_attr, MD_WORKER_STACK_SIZE);
    VPLThread_AttrSetDetachState(&thread_attr, false);

    // start up the periodic sync function
    // This thread can go up and down multiple times because of
    // delete_all() support. Reset the sync flag prior to launch.
    shutdown_sync = false;
    rv = VPLThread_Create(&per_sync_thread, per_sync_helper, VPL_AS_THREAD_FUNC_ARG(this), &thread_attr, "per_sync worker");
    if ( rv != VPL_OK ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLThread_Create(per_sync) - %d", rv);
        // The sync thread is vital. If it doesn't come up the database is
        // unusable. Mark the dataset invalid.
        invalid = true;
        // indicates the thread isn't running.
        shutdown_sync = true;
        // Attempting to restart CCD to clear error.
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLThread_Create(per_sync) - %d", rv);
        stop_ccd();
        goto done;
    }

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "per_sync_worker started");
done:
    return;
}

void managed_dataset::period_thread_stop(void)
{
    VPLMutex_Lock(&sync_mutex);
    if ( !shutdown_sync ) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Shutting down per_sync thread");
        shutdown_sync = true;
        VPLCond_Broadcast(&per_sync_cond);
        VPLMutex_Unlock(&sync_mutex);

        // Wait for the sync thread to shutdown
        VPLThread_Join(&per_sync_thread, NULL);
        VPLTRACE_LOG_INFO(TRACE_BVS, 0, "joined with per_sync thread");
    }
    else {
        VPLMutex_Unlock(&sync_mutex);
    }
}

void managed_dataset::dbase_activate()
{
    DatasetDBError dbrv;
    bool open_transaction = false;
    int rc;
    int ec;
    bool no_restart = false;
    bool had_backup = false;
    bool lost_and_found_system = false;

    VPLMutex_Lock(&sync_mutex);
    fsck_needed = test_marker(MD_DB_NEEDS_FSCK) || test_marker(MD_DB_IS_OPEN);

    // clear the open marker
    clear_marker(MD_DB_IS_OPEN);


    rc = make_dataset_directory();
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "make_dataset_directory - %d", rc);
        invalid = true;
        goto exit;
    }

    if ( test_marker(MD_DB_FSCK_ERR2_MARKER) ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "2 failed FSCKs - forcing restore.");
        invalid = true;
        goto force_restore;
    }

    if ( test_marker(MD_DB_BACKUP_ERR2_MARKER) ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "2 failed backups - forcing restore.");
        invalid = true;
        goto force_restore;
    }

    // Try twice to open the database. If that fails, well, try a restore.
    for( int i = 0 ; i < 2 ; i++ ) {
        VPLTime_t start = VPLTime_GetTimeStamp();
        VPLTime_t dur;
        dbrv = datasetDB.OpenDB(dir_name + DATASETDB_FILENAME,
                                datasetdb_options);
        dur = VPLTime_DiffClamp(VPLTime_GetTimeStamp(), start);
        VPLTRACE_LOG_INFO(TRACE_BVS, 0, "datasetDB.OpenDB() took "FMTu64, dur);
        
        if ( dbrv == DATASETDB_OK ) {
            clear_marker(MD_DB_OPEN_ERR1_MARKER);
            set_marker(MD_DB_IS_OPEN);
            invalid = false;
            break;
        }

        ec = db_err_to_class(dbrv);
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Dataset "FMTu64":"FMTu64" error %d class %d",
                         id.uid, id.did, dbrv, ec);
        switch (ec) {
        case DB_ERR_CLASS_SCHEMA:
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Dataset "FMTu64":"FMTu64" schema mismatch.",
                             id.uid, id.did);
            no_restart = true;
            break;
        case DB_ERR_CLASS_USER:
        case DB_ERR_CLASS_SWHW:
        case DB_ERR_CLASS_DATABASE:
        case DB_ERR_CLASS_SYSTEM:
            invalid = true;
            no_restart = false;
            break;
        default:
            // This can't really happen at present
            FAILED_ASSERT("Unexpected error class %d", ec);
            break;
        }

        dbrv = datasetDB.CloseDB();
        if ( dbrv != DATASETDB_OK ) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Dataset "FMTu64":"FMTu64": CloseDB(). Error:%d.",
                             id.uid, id.did, dbrv);
        }
    }

    // Force a restart the first time we fail to open.
    // The second time we need to perform a restore
    if ( invalid && !no_restart && !test_marker(MD_DB_OPEN_ERR1_MARKER) ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Dataset "FMTu64":"FMTu64": OpenDB first strike.",
                         id.uid, id.did);
        set_marker(MD_DB_OPEN_ERR1_MARKER);
        goto exit_restart;
    }

force_restore:
    if ( invalid ) {
        bool is_new_db;

        is_new_db = test_marker(MD_NO_DB_MARKER);
        if ( no_restart ) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Dataset "FMTu64":"FMTu64": db bad - no restart",
                             id.uid, id.did);
            is_failed = true;
            goto exit_fail;
        }
        // Attempt a restore
        dbrv = datasetDB.Restore(dir_name + DATASETDB_FILENAME, had_backup);
        if ( dbrv ) {
            // If this happens do not restart. No point.
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Dataset "FMTu64":"FMTu64": db.Restore() - %d",
                             id.uid, id.did, dbrv);
            is_failed = true;
            goto exit_fail;
        }
        else {
            clear_dbase_err();
        }
        if ( !had_backup ) {
            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                              "Dataset "FMTu64":"FMTu64": No backup db",
                              id.uid, id.did);
            // If we deleted the database last time around, don't bother
            // retrying. Things are really bad and we need to let the
            // user know rather than sit in an infinite loop trying to
            // create the database.
            if ( is_new_db ) {
                is_failed = true;
                goto exit_fail;
            }
            set_marker(MD_NO_DB_MARKER);
        }
        else {
            // We just restored from a backup, losing the backup in the
            // process. Trigger a backup to be made.
            set_marker(MD_DB_BACKUP_MARKER);
        }
        goto exit_restart;
    }

    // Dataset merge version is in the database.
    // Dataset version is greater of merge version and unmerged change version.
    dbrv = datasetDB.GetDatasetFullVersion(version);
    if (dbrv != DATASETDB_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Dataset "FMTu64":"FMTu64": Failed to get version. Error:%d.",
                         id.uid, id.did, dbrv);
        invalid = true;
        goto exit;
    }

    // May need to modify dataset.
    dbrv = datasetDB.BeginTransaction();
    if (dbrv != DATASETDB_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Dataset "FMTu64":"FMTu64": Failed to begin transaction:%d.",
                         id.uid, id.did, dbrv);
        invalid = true;
        goto exit;
    }
    open_transaction = true;
    dbrv = datasetDB.TestAndCreateComponent("", DATASETDB_COMPONENT_TYPE_DIRECTORY, 0, version, VPLTime_GetTime());
    if (dbrv != DATASETDB_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Dataset "FMTu64":"FMTu64": Failed to test/create root component in DB. Error:%d.",
                         id.uid, id.did, dbrv);
        invalid = true;
        goto exit;
    }

    // Since there are existing Orbes out there, the users could have already created
    // their own "lost+found" folder. To resolve this naming conflict, we add a marker
    // to distinguish between the case where the system itself has created the folder vs
    // the case where the user has created the folder. If the user has created the folder,
    // then move its contents to "/lost+found/lost+found-(user)-<timestamp>"
    lost_and_found_system = test_marker(MD_LOST_AND_FOUND_SYSTEM);

    dbrv = datasetDB.TestAndCreateLostAndFound(version, lost_and_found_system, VPLTime_GetTime());
    if (dbrv != DATASETDB_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Dataset "FMTu64":"FMTu64": Failed to test/create lost+found directory in DB. Error:%d.",
                         id.uid, id.did, dbrv);
        invalid = true;
        goto exit;
    }
    set_marker(MD_LOST_AND_FOUND_SYSTEM);

 exit:
    // Commit database changes from activation.
    if(invalid) {
        if(open_transaction) {
            // rollback just in case transaction was left in a funny state.
            datasetDB.RollbackTransaction();
        }
    }
    else {
        dbrv = datasetDB.CommitTransaction();
        if (dbrv != DATASETDB_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Dataset "FMTu64":"FMTu64": Failed to commit transaction:%d.",
                             id.uid, id.did, dbrv);
            invalid = true;
        }
    }
    // clear the mod marker since we were successful
    if ( !invalid ) {
        clear_marker(MD_NO_DB_MARKER);
    }

 exit_restart:
    period_thread_start();

 exit_fail:
    VPLMutex_Unlock(&sync_mutex);
}

managed_dataset::~managed_dataset()
{ 
    DatasetDBError dbrv;

    // Tell the periodic sync thread to shutdown
    // Prevent a backup at shutdown as this can delay shutdown by many
    // minutes.
    period_thread_stop();

    // We have to set this marker before the database has been closed.
    // So that the mod marker will be created
    if ( invalid || fsck_needed ) {
        set_marker(MD_DB_NEEDS_FSCK);
    }

    dbrv = datasetDB.CloseDB();
    dbrv = db_err_classify(dbrv);
    if ( dbrv != DATASETDB_OK ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Dataset "FMTu64":"FMTu64": CloseDB(). Error:%d.",
                         id.uid, id.did, dbrv);
    }
    else {
        clear_marker(MD_DB_IS_OPEN);
    }

    if ( open_handles.begin() != open_handles.end()
         || file_id_to_handle.begin() != file_id_to_handle.end() ) {

        FAILED_ASSERT("Shutting down and there are open file handles!");
    }

    VPLMutex_Destroy(&sync_mutex);
    VPLMutex_Destroy(&access_mutex);
    VPLMutex_Destroy(&fd_mutex);
    VPLCond_Destroy(&per_sync_cond);
}

void managed_dataset::dataset_check()
{
    vector<string> paths;
    int rc;
    DatasetDBError dbrc;
    ContentAttrs content_attr;
    string disk_path;
    VPLFS_stat_t stats;
    string current;
    VPLFS_dir_t directory;
    bool dir_open = false;
    VPLFS_dirent_t entry;
    string root_dir = dir_name + BASE_DIR(DATA);
    int err_cnt = 0;
    u64 component_check_offset = 0;
    u64 num_deleted = 0;
    u64 num_missing_parents = 0;
    VPLTime_t component_check_dir_time = 0;
    int content_file_lost_and_found_cnt = 0;
    int content_rec_del_cnt = 0;
    int content_size_cnt = 0;
    int content_file_lost_and_found_mod_cnt;
    VPLTime_t content_file_lost_and_found_dir_time = 0;
    VPLTime_t content_file_lost_and_found_time;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      "Dataset "FMTu64":"FMTu64": Walking content tree.",
                      id.uid, id.did);
    // Walk the data tree to reset incorrect component sizes.
    paths.push_back("");
    while(!paths.empty()) {
        current = paths.back();
        paths.pop_back();
        string path = dir_name + BASE_DIR(DATA) + current;
#ifdef FSCK_DEBUG
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Dataset "FMTu64":"FMTu64": Walking content tree - %s.",
                          id.uid, id.did, path.c_str());
#endif // FSCK_DEBUG
        rc = VPLFS_Opendir(path.c_str(), &directory);
        if(rc != VPL_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Dataset "FMTu64":"FMTu64": Open directory {%s} Failed: %d.",
                             id.uid, id.did, path.c_str(), rc);
            goto exit;
        }
        dir_open = true;

        do {
            bool access_lock_held = false;

            hold_off_fsck();

            if ( shutdown_sync || invalid ) {
                VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                                  "Dataset "FMTu64":"FMTu64": Database invalid or shutdown",
                                  id.uid, id.did);
                goto exit;
            }
            rc = VPLFS_Readdir(&directory, &entry);
            if(rc == VPL_ERR_MAX) {
                break;
            }
            if(rc != VPL_OK) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "Dataset "FMTu64":"FMTu64": Read directory {%s} Failed: %d.",
                                 id.uid, id.did, path.c_str(), rc);
                err_cnt++;
                break;
            }

            // Ignore "." and ".."
            if(entry.filename[0] == '.' &&
               (entry.filename[1] == '\0' || (entry.filename[1] == '.' &&
                                              entry.filename[2] == '\0'))) {
                continue;
            }

            string new_path = current.empty() ? entry.filename : current + "/" + entry.filename;
            
#ifdef FSCK_DEBUG
            VPLTRACE_LOG_INFO(TRACE_BVS, 0, "testing %s", new_path.c_str());
#endif // FSCK_DEBUG
            access_lock_held = false;
            switch(entry.type) {
            case VPLFS_TYPE_FILE: {
                string content_path;
                std::map<string, vss_file*>::iterator fd_it;

                // For each file, check if there's a matching content file entry.
                disk_path = path + "/" + entry.filename;
                
                rc = VPLFS_Stat(disk_path.c_str(), &stats);
                if(rc != VPL_OK) {
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                     "Dataset "FMTu64":"FMTu64": Stat content {%s} failed: %d.",
                                     id.uid, id.did, new_path.c_str(), rc);
                    break;
                }

                // If the file is currently open and modified via open_file
                // skip this component as it's expected for the size to be wrong.
                content_path = disk_path.substr(root_dir.length());
                VPLMutex_Lock(&access_mutex);
                access_lock_held = true;
                VPLMutex_Lock(&fd_mutex);
                fd_it = open_handles.find(content_path);
                if(fd_it != open_handles.end()) {
#ifdef FSCK_DEBUG
                    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                                      "Dataset "FMTu64":"FMTu64": skipping open content %s",
                                      id.uid, id.did, content_path.c_str());
#endif // FSCK_DEBUG
                    VPLMutex_Unlock(&fd_mutex);
                    break;
                }
                VPLMutex_Unlock(&fd_mutex);

                dbrc = datasetDB.GetContentByLocation(new_path, content_attr);
                dbrc = db_err_classify(dbrc);
                if(dbrc == DATASETDB_ERR_UNKNOWN_CONTENT) {

#ifdef FSCK_DEBUG
                    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Moving unknown content %s to lost+found", new_path.c_str());
#endif // FSCK_DEBUG

                    // Move the content to "lost+found"
                    // The dir_time and count are needed to construct the directory path
                    // The version and time are needed to set the component fields
                    content_file_lost_and_found_mod_cnt = content_file_lost_and_found_cnt % MAXIMUM_COMPONENTS;
                    if (content_file_lost_and_found_mod_cnt == 0) {
                        // The dir_time is used for setting the directory names of
                        // "MM-DD-YYYY" and "unreferenced-files-<timestamp>"
                        content_file_lost_and_found_dir_time = VPLTime_GetTime();
                    }
                    content_file_lost_and_found_cnt++;
                    content_file_lost_and_found_time = VPLTime_GetTime();
                    dbrc = datasetDB.MoveContentToLostAndFound(content_path,
                                                               content_file_lost_and_found_mod_cnt,
                                                               content_file_lost_and_found_dir_time,
                                                               stats.size,
                                                               version,
                                                               content_file_lost_and_found_time);
                    dbrc = db_err_classify(dbrc);
                    if(dbrc != DATASETDB_OK) {
                        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                         "Dataset "FMTu64":"FMTu64": Move Content {%s} to lost+found failed: %d.",
                                         id.uid, id.did, disk_path.c_str(), dbrc);
                        err_cnt++;
                    }
                }
                else if(dbrc == DATASETDB_OK) {
                    // Make sure DB size matches file size. Update DB if needed.
                    if(stats.size != content_attr.size) {
                        content_size_cnt++;
                        VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                                         "Dataset "FMTu64":"FMTu64": Content {%s} size incorrect. Fixing.",
                                         id.uid, id.did, new_path.c_str());
                        dbrc = datasetDB.SetContentSizeByLocation(new_path, stats.size);
                        dbrc = db_err_classify(dbrc);
                        if(dbrc != DATASETDB_OK) {
                            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                             "Dataset "FMTu64":"FMTu64": Set Content {%s} size failed: %d.",
                                             id.uid, id.did, new_path.c_str(), dbrc);
                            err_cnt++;
                        }
                    }
                }
                else {
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                     "Dataset "FMTu64":"FMTu64": Check Content {%s} Exists failed: %d.",
                                     id.uid, id.did, new_path.c_str(), dbrc);
                    err_cnt++;
                }
                break;
            }    
            case VPLFS_TYPE_DIR:
                // Dig deeper here.
                paths.push_back(new_path);
                break;
                
            default:
            case VPLFS_TYPE_OTHER:
                // Ignore others
                break;
            }
            if ( access_lock_held ) {
                VPLMutex_Unlock(&access_mutex);
                access_lock_held = false;
            }
        } while(rc == VPL_OK);
        VPLFS_Closedir(&directory);
        dir_open = false;
    }
    
    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      "Dataset "FMTu64":"FMTu64": Walking the db for bad content.",
                      id.uid, id.did);
    // Remove content and component records where the content-file is missing.
    content_attr.compid = 0; // begin from the beginning
    do {

        hold_off_fsck();

        if ( shutdown_sync || invalid ) {
            VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                              "Dataset "FMTu64":"FMTu64": Database invalid or shutdown",
                              id.uid, id.did);
            goto exit;
        }
        dbrc = datasetDB.GetNextContent(content_attr);
        dbrc = db_err_classify(dbrc);
        if(dbrc == DATASETDB_ERR_UNKNOWN_CONTENT) {
            // No more contents. Done.
            break;
        }
        else if(dbrc != DATASETDB_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Dataset "FMTu64":"FMTu64": Get Next Content from content ID "FMTu64" failed: %d.",
                             id.uid, id.did, content_attr.compid, dbrc);
            err_cnt++;
        }
        else {
            bool is_busy = false;
            std::map<string, vss_file*>::iterator fd_it;

#ifdef FSCK_DEBUG
            VPLTRACE_LOG_INFO(TRACE_BVS, 0, "testing %s", content_attr.location.c_str());
#endif // FSCK_DEBUG
            // Make sure this content isn't being used by an open file
            VPLMutex_Lock(&access_mutex);
            VPLMutex_Lock(&fd_mutex);
            fd_it = open_handles.find(content_attr.location);
            if(fd_it != open_handles.end()) {
                VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                                  "Dataset "FMTu64":"FMTu64": skipping open content %s",
                                  id.uid, id.did, content_attr.location.c_str());
                is_busy = true;
            }
            VPLMutex_Unlock(&fd_mutex);

            // Make sure content file exists. If it doesn't, remove the content.
            if ( !is_busy ) {
                disk_path = dir_name + BASE_DIR(DATA) + content_attr.location;
                rc = VPLFS_Stat(disk_path.c_str(), &stats);

                switch(rc) {
                case VPL_ERR_NOENT:
                case VPL_ERR_NOTDIR:
                    VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                                      "Dataset "FMTu64":"FMTu64": Content {%s} file missing. Delete from DB.",
                                      id.uid, id.did, content_attr.location.c_str());
                    content_rec_del_cnt++;
                    dbrc = datasetDB.DeleteContentComponentByLocation(content_attr.location);
                    dbrc = db_err_classify(dbrc);
                    if(dbrc != DATASETDB_OK) {
                        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                         "Dataset "FMTu64":"FMTu64": Delete Content {%s} failed: %d.",
                                         id.uid, id.did, content_attr.location.c_str(), dbrc);
                        err_cnt++;
                    }
                    break;
                default:
                    if ( rc != 0 ) {
                        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                         "Dataset "FMTu64":"FMTu64": stat {%s} failed: %d.",
                                         id.uid, id.did, disk_path.c_str(), rc);
                    }
                    break;
                }
            }
            VPLMutex_Unlock(&access_mutex);
        }
    } while(dbrc == DATASETDB_OK);

    // Delete any file-type component that has no content entry.
    component_check_offset = 0;
    num_missing_parents = 0;
    do {
        u64 num_deleted_this_round = 0;

        // Grab lock to prevent race condition with open_file
        VPLMutex_Lock(&access_mutex);
        dbrc = datasetDB.CheckComponentConsistency(component_check_offset,
                                                   num_deleted_this_round,
                                                   num_missing_parents,
                                                   component_check_dir_time,
                                                   MAXIMUM_COMPONENTS,
                                                   version);
        VPLMutex_Unlock(&access_mutex);
        num_deleted += num_deleted_this_round;
        dbrc = db_err_classify(dbrc);

        hold_off_fsck();

        if ( shutdown_sync || invalid ) {
            VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                              "Dataset "FMTu64":"FMTu64": Database invalid or shutdown",
                              id.uid, id.did);
            goto exit;
        }
    } while(dbrc == DATASETDB_OK);

    if(dbrc != DATASETDB_ERR_REACH_COMPONENT_END) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Dataset "FMTu64":"FMTu64": Delete Components Without Content failed: %d.",
                         id.uid, id.did, dbrc);
        err_cnt++;
    }

    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      "Dataset "FMTu64":"FMTu64": Completed successfully.",
                      id.uid, id.did);

    // Successfully done with check.
    if ( !invalid && !shutdown_sync ) {
        fsck_needed = false;
        clear_marker(MD_DB_NEEDS_FSCK);
        clear_marker(MD_DB_FSCK_ERR1_MARKER);
        clear_marker(MD_DB_FSCK_ERR2_MARKER);
    }
 exit:
    if ( invalid ) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Dataset "FMTu64":"FMTu64": fsck failed.",
                          id.uid, id.did);
        mark_fsck_err();
    }
    if(dir_open) {
        VPLFS_Closedir(&directory);
    }

    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      "Dataset "FMTu64":"FMTu64": fsck Done. err_cnt = %d",
                     id.uid, id.did, err_cnt);
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "files lost+found %d, file sizes adjusted %d, "
                      "file recs deleted %d, component entries deleted "FMTu64", "
                      "missing parents "FMTu64,
                      content_file_lost_and_found_cnt, content_size_cnt,
                      content_rec_del_cnt, num_deleted, num_missing_parents);

    // Wake up per_sync_thread to reap us.
    fsck_done = true;
    VPLMutex_Lock(&sync_mutex);
    VPLCond_Broadcast(&per_sync_cond);
    VPLMutex_Unlock(&sync_mutex);
    return;
}

void managed_dataset::close_unused_files()
{
}

u64 managed_dataset::get_version()
{
    return version;
}

s16 managed_dataset::delete_all()
{
    u8 rv = VSSI_SUCCESS;
    string database_name = dir_name + DATASETDB_FILENAME;
    DatasetDBError dbrv;
    u64 size = 0;

    VPLMutex_Lock(&access_mutex);

    // Shut down the periodic sync thread while we delete everything.
    period_thread_stop();

    VPLMutex_Lock(&sync_mutex);

    // If current version is 0, do nothing.
    if(version == 0) {
        goto finish;
    }

    // Close database
    dbrv = datasetDB.CloseDB();
    dbrv = db_err_classify(dbrv);
    if ( dbrv != DATASETDB_OK ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Dataset "FMTu64":"FMTu64": CloseDB(). Error:%d.",
                         id.uid, id.did, dbrv);
        goto finish;
    }
    clear_marker(MD_DB_IS_OPEN);

    // Delete all data
    (void)Util_rmRecursive(dir_name, tmp_to_delete_dir);
    make_dataset_directory();

    // Open database
    // Do not use activate here. Activate should *only* happen from within
    // the constructor to force it all happening at startup of CCD.
    dbrv = datasetDB.OpenDB(database_name, datasetdb_options);
    dbrv = db_err_classify(dbrv);
    if(dbrv != DATASETDB_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Dataset "FMTu64":"FMTu64" failed database open: %d",
                         id.uid, id.did, dbrv);
        goto finish;
    }

    clear_marker(MD_DB_OPEN_ERR1_MARKER);
    set_marker(MD_DB_IS_OPEN);

    // Bump version
    // Sync thread is stopped so no need for the lock
    version++;

    // Set version, make root directory.
    dbrv = datasetDB.BeginTransaction();
    dbrv = db_err_classify(dbrv);
    if (dbrv != DATASETDB_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Dataset "FMTu64":"FMTu64": Failed to begin transaction:%d.",
                         id.uid, id.did, dbrv);
        goto finish;
    }
    dbrv = datasetDB.TestAndCreateComponent("", DATASETDB_COMPONENT_TYPE_DIRECTORY, 0, version, VPLTime_GetTime());
    dbrv = db_err_classify(dbrv);
    if ( dbrv != DATASETDB_OK ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Dataset "FMTu64":"FMTu64": TestAndCreateComponent(). Error:%d.",
                         id.uid, id.did, dbrv);
        goto finish;
    }
    dbrv = datasetDB.TestAndCreateLostAndFound(version, false, VPLTime_GetTime());
    dbrv = db_err_classify(dbrv);
    if ( dbrv != DATASETDB_OK ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Dataset "FMTu64":"FMTu64": TestAndCreateLostAndFound(). Error:%d.",
                         id.uid, id.did, dbrv);
        goto finish;
    }
    set_marker(MD_LOST_AND_FOUND_SYSTEM);
    dbrv = datasetDB.SetDatasetFullVersion(version);
    dbrv = db_err_classify(dbrv);
    if ( dbrv != DATASETDB_OK ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Dataset "FMTu64":"FMTu64": SetDatasetFullVersion(version). Error:%d.",
                         id.uid, id.did, dbrv);
        goto finish;
    }
    dbrv = datasetDB.CommitTransaction();
    dbrv = db_err_classify(dbrv);
    if (dbrv != DATASETDB_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Dataset "FMTu64":"FMTu64": Failed to commit transaction:%d.",
                         id.uid, id.did, dbrv);
        goto finish;
    }
    
    // Update dataset status (mainly update size).
    dbrv = datasetDB.GetComponentSize("", size);
    dbrv = db_err_classify(dbrv);
    if ( dbrv != DATASETDB_OK ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Dataset "FMTu64":"FMTu64": GetComponentSize(). Error:%d.",
                         id.uid, id.did, dbrv);
        goto finish;
    }

    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0, "Add dataset event, datasetid "FMTu64", size "FMTu64", version "FMTu64,
                        id.uid, size, version);
    server->add_dataset_stat_update(id, size, version);

 finish:
    // we just blew away everything, nothing to fsck
    fsck_needed = false;

    period_thread_start();

    VPLMutex_Unlock(&sync_mutex);
    VPLMutex_Unlock(&access_mutex);
    return rv;
}

bool managed_dataset::component_is_directory(const std::string &name)
{
    DatasetDBError dberr;
    int type;
    dberr = datasetDB.GetComponentType(name, type);
    dberr = db_err_classify(dberr);
    return dberr == DATASETDB_OK && type == DATASETDB_COMPONENT_TYPE_DIRECTORY;
}

DatasetDBError managed_dataset::append_dirent(const ComponentInfo &info,
                                              std::ostringstream& data_out,
                                              bool json)
{
    std::string basename;
    // compute the base name (without any slashes)
    size_t pos = info.name.find_last_of('/');
    if (pos != std::string::npos) {
        basename.assign(info.name, pos + 1, std::string::npos);
    }
    else {
        basename.assign(info.name);
    }

    if (json) {
        data_out << "{\"name\":\"" << basename << "\",\"type\":\"";
        if (info.type == DATASETDB_COMPONENT_TYPE_DIRECTORY)
            data_out << "dir";
        else
            data_out << "file";
        data_out << "\",\"lastChanged\":" << VPLTime_ToSec(info.mtime) << ",\"size\":" << info.size << "}";
    }
    else {
        char vss_dirent[VSS_DIRENT_BASE_SIZE];
        string metadata;
        std::vector<std::pair<int, std::string> > metadata_vec;
        std::vector<std::pair<int, std::string> >::const_iterator it;

        vss_dirent_set_size(vss_dirent, info.size);
        vss_dirent_set_ctime(vss_dirent, info.ctime);
        vss_dirent_set_mtime(vss_dirent, info.mtime);
        if(info.type == DATASETDB_COMPONENT_TYPE_DIRECTORY) {
            vss_dirent_set_is_dir(vss_dirent, true);
        }
        else {
            vss_dirent_set_is_dir(vss_dirent, false);
        }

        vss_dirent_set_name_len(vss_dirent, basename.length() + 1); // account for NUL char at the end
        vss_dirent_set_reserved(vss_dirent);
        vss_dirent_set_signature(vss_dirent, info.hash.data());

        // gather metadata for this component
        for (it = info.metadata.begin(); it != info.metadata.end(); it++) {
            metadata.append(1, (char)it->first);
            metadata.append(1, (char)it->second.length());
            metadata.append(it->second);
        }

        vss_dirent_set_meta_size(vss_dirent, metadata.length());
        vss_dirent_set_change_ver(vss_dirent, info.version);

        data_out.write(vss_dirent, sizeof(vss_dirent));
        data_out << basename;
        data_out.put('\0'); // NUL char at the end of basename
        data_out << metadata;
    }

    return DATASETDB_OK;
}

DatasetDBError managed_dataset::append_dirent2(const ComponentInfo &info,
                                               std::ostringstream& data_out,
                                               bool json)
{
    std::string basename;
    // compute the base name (without any slashes)
    size_t pos = info.name.find_last_of('/');
    if (pos != std::string::npos) {
        basename.assign(info.name, pos + 1, std::string::npos);
    }
    else {
        basename.assign(info.name);
    }

    if (json) {
        data_out << "{\"name\":\"" << basename << "\",\"type\":\"";
        if (info.type == DATASETDB_COMPONENT_TYPE_DIRECTORY)
            data_out << "dir";
        else
            data_out << "file";
        data_out << "\",\"lastChanged\":" << VPLTime_ToSec(info.mtime) << ",\"size\":" << info.size << "}";

        // TODO: Add attributes
    }
    else {
        // to solve memory alignment, we use u64 (8-byte alignment)
        // instead of char (1-byte alignment) for buffer allocation on stack.
        u64 vss_dirent[VSS_DIRENT2_BASE_SIZE / sizeof(u64) + 1];
        string metadata;
        std::vector<std::pair<int, std::string> > metadata_vec;
        std::vector<std::pair<int, std::string> >::const_iterator it;

        vss_dirent2_set_size((char *)vss_dirent, info.size);
        vss_dirent2_set_ctime((char *)vss_dirent, info.ctime);
        vss_dirent2_set_mtime((char *)vss_dirent, info.mtime);
        if(info.type == DATASETDB_COMPONENT_TYPE_DIRECTORY) {
            vss_dirent2_set_is_dir((char *)vss_dirent, true);
        }
        else {
            vss_dirent2_set_is_dir((char *)vss_dirent, false);
        }

        vss_dirent2_set_name_len((char *)vss_dirent, basename.length() + 1); // account for NUL char at the end
        vss_dirent2_set_reserved((char *)vss_dirent);
        vss_dirent2_set_signature((char *)vss_dirent, info.hash.data());
        vss_dirent2_set_attrs((char *)vss_dirent, info.perm);

        // gather metadata for this component
        for (it = info.metadata.begin(); it != info.metadata.end(); it++) {
            metadata.append(1, (char)it->first);
            metadata.append(1, (char)it->second.length());
            metadata.append(it->second);
        }

        vss_dirent2_set_meta_size((char *)vss_dirent, metadata.length());
        vss_dirent2_set_change_ver((char *)vss_dirent, info.version);

        data_out.write((char *)vss_dirent, VSS_DIRENT2_BASE_SIZE);
        data_out << basename;
        data_out.put('\0'); // NUL char at the end of basename
        data_out << metadata;
    }

    return DATASETDB_OK;
}

DatasetDBError managed_dataset::append_dirent(const std::string& component,
                                              std::ostringstream& data_out,
                                              bool json)
{
    DatasetDBError dberr;
    ComponentInfo info;

    dberr = datasetDB.GetComponentInfo(component, info);
    dberr = db_err_classify(dberr);
    if (dberr != DATASETDB_OK) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                          "Couldn't get info on component %s from DB: %d. Skipping entry.",
                          component.c_str(), dberr);
        goto exit;
    }

    dberr = append_dirent(info, data_out, json);

 exit:
    return dberr;
}

DatasetDBError managed_dataset::append_dirent2(const std::string& component,
                                               std::ostringstream& data_out,
                                               bool json)
{
    std::map<string, vss_file*>::iterator fd_it;
    DatasetDBError dberr;
    ComponentInfo info;


    dberr = datasetDB.GetComponentInfo(component, info);
    dberr = db_err_classify(dberr);
    if (dberr != DATASETDB_OK) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                          "Couldn't get info on component %s from DB: %d. Skipping entry.",
                          component.c_str(), dberr);
        goto exit;
    }

    // See if this bad boy's in the cache.
    // Note: we don't check the write_data cache
    // because that shouldn't take effect until commit.
    VPLMutex_Lock(&fd_mutex);
    fd_it = open_handles.find(info.path);
    if(fd_it != open_handles.end()) {
        // override values from the database
        info.size = fd_it->second->get_size();
        info.perm = fd_it->second->get_attrs();
    }
    VPLMutex_Unlock(&fd_mutex);

    dberr = append_dirent2(info, data_out, json);

 exit:
    return dberr;
}

void managed_dataset::update_component_info_to_cache(ComponentInfo& info)
{
    std::map<string, vss_file*>::iterator fd_it;

    // Update ComponentInfo to match cache

    // See if this bad boy's in the cache.
    // Note: we don't check the write_data cache
    // because that shouldn't take effect until commit.
    VPLMutex_Lock(&fd_mutex);
    fd_it = open_handles.find(info.path);
    if(fd_it != open_handles.end()) {
        // override values from the database
        info.size = fd_it->second->get_size();
        info.perm = fd_it->second->get_attrs();
    }
    VPLMutex_Unlock(&fd_mutex);
}

void managed_dataset::append_dirent2_pagination(const ComponentInfo& info,
                                                std::vector< std::pair<std::string, ComponentInfo> >& filelist)
{
    std::string basename;
    size_t pos;

    // compute the base name (without any slashes)
    pos = info.name.find_last_of('/');
    if (pos != std::string::npos) {
        basename.assign(info.name, pos + 1, std::string::npos);
    } else {
        basename.assign(info.name);
    }

    filelist.push_back(std::make_pair(basename, info));
}



/* To the DB, a component is either known or unknown (DATASETDB_ERR_UNKNOWN_COMPONENT).
 * However, in case of a non-existing component, VSSI protocol makes the distinction between 
 * (1) component simply didn't exist, and all ancestor components were directory components (VSSI_NOTFOUND), and
 * (2) component couldn't have existed because some ancestor component was not a directory component (VSSI_NOTDIR).
 * Thus, in case of DATASETDB_ERR_UNKNOWN_COMPONENT, further error analaysis is needed.
 */
// assumption: component is DATASETDB_ERR_UNKNOWN_COMPONENT
s16 managed_dataset::elaborate_unknown_component_error(const std::string &name)
{
    s16 rv = VSSI_NOTFOUND;  // assume case (1) and prove otherwise

    size_t pos = name.find_first_of('/');
    while (pos != std::string::npos) {
        int type;
        DatasetDBError dberr = datasetDB.GetComponentType(name.substr(0, pos), type);
        dberr = db_err_classify(dberr);
        if (dberr == DATASETDB_OK && type != DATASETDB_COMPONENT_TYPE_DIRECTORY) {
            rv = VSSI_NOTDIR;
            break;
        }
        pos = name.find_first_of('/', pos+1);
    }

    return rv;
}

bool managed_dataset::component_exists(const std::string &name)
{
    return datasetDB.TestExistComponent(name) == DATASETDB_OK;
}

s16 managed_dataset::read_dir(const std::string& component_in,
                              std::string& data_out,
                              bool json,
                              bool pagination,
                              const std::string& sort,
                              u32 index,
                              u32 max)
{
    s16 rv = VSSI_SUCCESS;
    DatasetDBError dberr;
    std::vector<std::string> names;
    std::vector<std::string>::const_iterator it;
    std::ostringstream oss;
    int totalItems = 0;
    std::string component = normalizePath(component_in);

    data_out.erase();

    VPLMutex_Lock(&access_mutex);
    dberr = datasetDB.ListComponents(component, names);
    dberr = db_err_classify(dberr);
    if (dberr != DATASETDB_OK) {
        if ( dberr == VSSI_ACCESS ) {
            rv = VSSI_ACCESS;
        }
        else if (dberr == DATASETDB_ERR_NOT_DIRECTORY) {
            rv = VSSI_NOTDIR;
        }
        else {
            // The client makes the distinction between
            // (1) all ancestor components were directory components but the component simply didn't exist, and
            // (2) some ancestor component was not a directory component.
            // So in case of an error, we will need to check the ancestor components.

            rv = VSSI_NOTFOUND;  // assume case (1) and prove otherwise

            if (dberr == DATASETDB_ERR_UNKNOWN_COMPONENT) {
                rv = elaborate_unknown_component_error(component);
            }

            if (rv == VSSI_NOTFOUND) {
                VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                                  "Component %s not found in DB. Error %d",
                                  component.c_str(), dberr);
            }
        }
        goto exit;
    }

    for (it = names.begin(); it != names.end(); it++) {
        std::string subcomponent = component.empty() ? *it : component + "/" + *it;
        if (json && totalItems > 0)
            oss << ",";
        dberr = append_dirent(subcomponent, oss, json);
        if (dberr != DATASETDB_OK) {
            continue;
        }
        totalItems++;
        VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                          "Add directory %s entry for %s.",
                          component.c_str(), it->c_str());
    }

    if (json) {
        oss << "],\"numOfFiles\":" << totalItems << "}";
        data_out.assign("{\"fileList\":[" + oss.str());
    }
    else {
        data_out.assign(oss.str());
    }

 exit:
    // Errors reading object root must be ignored.
    // Return success with no data instead (object is empty).
    if(rv != VSSI_SUCCESS && component.empty()) {
        rv = VSSI_SUCCESS;
    }

    last_action = VPLTime_GetTimeStamp();
    VPLMutex_Unlock(&access_mutex);
    return rv;
}

static bool sort_by_time(std::pair<std::string, ComponentInfo> i,
                         std::pair<std::string, ComponentInfo> j) {
    return i.second.mtime > j.second.mtime;
}

static bool sort_by_size(std::pair<std::string, ComponentInfo> i,
                         std::pair<std::string, ComponentInfo> j) {
    return i.second.size > j.second.size;
}

static bool sort_by_name(std::pair<std::string, ComponentInfo> i,
                         std::pair<std::string, ComponentInfo> j) {
    return i.first < j.first;
}

s16 managed_dataset::read_dir2(const std::string& component_in,
                               std::string& data_out,
                               bool json,
                               bool pagination,
                               const std::string& sort,
                               u32 index,
                               u32 max)
{
    s16 rv = VSSI_SUCCESS;
    DatasetDBError dberr;
    std::vector<ComponentInfo> components_info;
    std::vector<ComponentInfo>::iterator it;
    std::ostringstream oss;
    int totalItems = 0;
    std::string component = normalizePath(component_in);

    data_out.erase();

    VPLMutex_Lock(&access_mutex);
    dberr = datasetDB.ListComponentsInfo(component, components_info);
    dberr = db_err_classify(dberr);
    if (dberr != DATASETDB_OK) {
        if ( dberr == VSSI_ACCESS ) {
            rv = VSSI_ACCESS;
        }
        else if (dberr == DATASETDB_ERR_NOT_DIRECTORY) {
            rv = VSSI_NOTDIR;
        }
        else {
            // The client makes the distinction between
            // (1) all ancestor components were directory components but the component simply didn't exist, and
            // (2) some ancestor component was not a directory component.
            // So in case of an error, we will need to check the ancestor components.

            rv = VSSI_NOTFOUND;  // assume case (1) and prove otherwise

            if (dberr == DATASETDB_ERR_UNKNOWN_COMPONENT) {
                rv = elaborate_unknown_component_error(component);
            }

            if (rv == VSSI_NOTFOUND) {
                VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                                  "Component %s not found in DB. Error %d",
                                  component.c_str(), dberr);
            }
        }
        goto exit;
    }

    if (pagination == true && json == true) {
        std::vector< std::pair<std::string, ComponentInfo> > filelist;
        bool needComma = false;
        u32 total = 0;

        // get everything under the requested directory
        for (it = components_info.begin(); it != components_info.end(); it++) {
            update_component_info_to_cache(*it);
            append_dirent2_pagination(*it, filelist);
        }

        // Release the lock for the following long lasting sort so that the server
        // thread can get in and service clients without really long delays.
        VPLMutex_Unlock(&access_mutex);

        // do the sorting. index starts from 1
        if (filelist.empty() || filelist.size() < index) {
            // do nothing
        } else if (sort == "alpha") {
            std::sort(filelist.begin(), filelist.end(), sort_by_name);
        } else if (sort == "size") {
            std::sort(filelist.begin(), filelist.end(), sort_by_size);
        } else {
            // default sorting is "time"
            std::sort(filelist.begin(), filelist.end(), sort_by_time);
        }

        // grab the lock again to make the code simple and make it easier to remove
        // this when the sort happens in the database code.
        VPLMutex_Lock(&access_mutex);

        // generate JSON response
        for (u32 i = index-1; i < filelist.size() && total < max; i++, total++) {
            if (needComma) {
                oss << ",";
            }
            oss << "{\"name\":\"" << filelist[i].first << "\",\"type\":\"";
            if (filelist[i].second.type == DATASETDB_COMPONENT_TYPE_DIRECTORY)
                oss << "dir";
            else
                oss << "file";
            oss << "\",\"lastChanged\":" << VPLTime_ToSec(filelist[i].second.mtime)
                     << ",\"size\":" << filelist[i].second.size << "}";

            needComma = true;
        }

        oss << "],\"numOfFiles\":" << total << "}";
        data_out.assign("{\"fileList\":[" + oss.str());
    }
    else {
        for (it = components_info.begin(); it != components_info.end(); it++) {
            if (json && totalItems > 0) {
                oss << ",";
            }
            update_component_info_to_cache(*it);
            dberr = append_dirent2(*it, oss, json);
            if (dberr != DATASETDB_OK) {
                continue;
            }
            totalItems++;
            VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                    "Add directory %s entry for %s.",
                    component.c_str(), it->name.c_str());
        }

        if (json) {
            oss << "],\"numOfFiles\":" << totalItems << "}";
            data_out.assign("{\"fileList\":[" + oss.str());
        }
        else {
            data_out.assign(oss.str());
        }
    }

 exit:
    // Errors reading object root must be ignored.
    // Return success with no data instead (object is empty).
    if(rv != VSSI_SUCCESS && component.empty()) {
        rv = VSSI_SUCCESS;
    }

    last_action = VPLTime_GetTimeStamp();
    VPLMutex_Unlock(&access_mutex);
    return rv;
}

int managed_dataset::stat_component(const std::string& component_in,
                                    std::string& data_out)
{
    s16 rv = VSSI_NOTFOUND;
    DatasetDBError dberr;
    std::ostringstream oss;
    std::string component = normalizePath(component_in);

    data_out.clear();

    VPLTRACE_LOG_FINER(TRACE_BVS, 0,
                       "Dataset "FMTu64":"FMTu64" stat {%s}.",
                       id.uid, id.did, component.c_str());

    VPLMutex_Lock(&access_mutex);
    dberr = append_dirent(component, oss, /*json*/false);
    if (dberr == DATASETDB_OK) {
        rv = VSSI_SUCCESS;
    }
    data_out.assign(oss.str());

    // Errors reading object root must be ignored.
    // Return success with no data instead (object is empty).
    if(rv != VSSI_SUCCESS && component.empty()) {
        rv = VSSI_SUCCESS;
    }

    VPLMutex_Unlock(&access_mutex);
    return rv;
}

int managed_dataset::stat2_component(const std::string& component_in,
                                     std::string& data_out)
{
    s16 rv = VSSI_NOTFOUND;
    DatasetDBError dberr;
    std::ostringstream oss;
    std::string component = normalizePath(component_in);

    data_out.clear();

    VPLTRACE_LOG_FINER(TRACE_BVS, 0,
                       "Dataset "FMTu64":"FMTu64" stat2 {%s}.",
                       id.uid, id.did, component.c_str());

    VPLMutex_Lock(&access_mutex);
    dberr = append_dirent2(component, oss, /*json*/false);
    if (dberr == DATASETDB_OK) {
        rv = VSSI_SUCCESS;
    }
    data_out.assign(oss.str());

    // Errors reading object root must be ignored.
    // Return success with no data instead (object is empty).
    if(rv != VSSI_SUCCESS && component.empty()) {
        rv = VSSI_SUCCESS;
    }

    last_action = VPLTime_GetTimeStamp();
    VPLMutex_Unlock(&access_mutex);
    return rv;
}

int managed_dataset::stat_component(const std::string& component_in,
                                    VPLFS_stat_t& stat_out)
{
    int rv = VPL_OK;
    DatasetDBError dberr;
    ComponentInfo info;
    std::string component = normalizePath(component_in);

    VPLMutex_Lock(&access_mutex);
    dberr = datasetDB.GetComponentInfo(component, info);
    dberr = db_err_classify(dberr);
    if (dberr != DATASETDB_OK) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                          "Couldn't get info on component %s from DB: %d. Skipping entry.",
                          component.c_str(), dberr);
        if ( dberr == VSSI_ACCESS ) {
            rv = VSSI_ACCESS;
        }
        else {
            rv = VSSI_NOTFOUND;
        }
        goto exit;
    }

    stat_out.size = info.size;
    stat_out.type = info.type == DATASETDB_COMPONENT_TYPE_DIRECTORY ? VPLFS_TYPE_DIR : VPLFS_TYPE_FILE;
    stat_out.atime = (time_t)VPLTime_ToSec(info.mtime);  // no last-access time in ComponentInfo; use mtime instead
    stat_out.mtime = (time_t)VPLTime_ToSec(info.mtime);
    stat_out.ctime = (time_t)VPLTime_ToSec(info.ctime);
    stat_out.isHidden = false;
    stat_out.isSymLink = false;
 exit:
    VPLMutex_Unlock(&access_mutex);
    return rv;
}

int managed_dataset::stat_shortcut(const std::string& component,
                                   std::string& path,
                                   std::string& type,
                                   std::string& args)
{
    // managed_dataset is only for Acer Orbe usage.
    // Currently, only Windows PC supports shortcut feature.
    // Reserve the function for possible future requirements.
    return VSSI_SUCCESS;
}

int managed_dataset::merge_timestamp(const std::string& component,
                                     u64 ctime, u64 mtime,
                                     u64 version, u32 index,
                                     u64 origin_device, VPLTime_t time)
{
    int rv = VSSI_SUCCESS;
    DatasetDBError dberr = DATASETDB_ERR_UNKNOWN_COMPONENT; // No commit if mtime and ctime both 0
    u64 nversion;

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        FMTu64" "FMTu64":"FMTu64" set timestamp component {%s} recv vers "FMTu64".",
                        origin_device, id.uid, id.did,
                        component.c_str(), version);

    mini_trans_start("merge_timestamp");
    // Only apply timestamp changes if the component exists. 
    nversion = version + 1;
    if(ctime != 0) {
        dberr = datasetDB.SetComponentCreationTime(component, nversion, ctime);
        dberr = db_err_classify(dberr);
        if(dberr == DATASETDB_ERR_UNKNOWN_COMPONENT) {
            VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                              FMTu64" "FMTu64":"FMTu64" Set ctime for {%s}: component doesn't exist.",
                              origin_device, id.uid, id.did,
                              component.c_str());
            goto exit;
        }
        else if(dberr != DATASETDB_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             FMTu64" "FMTu64":"FMTu64" Set ctime for {%s} in DB fail:%d.",
                             origin_device, id.uid, id.did,
                             component.c_str(), dberr);
            rv = VSSI_BADCMD;
            goto exit;
        }
    }
    if(mtime != 0) {
        dberr = datasetDB.SetComponentLastModifyTime(component, nversion, mtime);
        dberr = db_err_classify(dberr);
        if(dberr == DATASETDB_ERR_UNKNOWN_COMPONENT) {
            VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                              FMTu64" "FMTu64":"FMTu64" Set mtime for {%s}: component doesn't exist.",
                              origin_device, id.uid, id.did,
                              component.c_str());
            goto exit;
        }
        else if(dberr != DATASETDB_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             FMTu64" "FMTu64":"FMTu64" Set mtime for {%s} in DB fail:%d.",
                             origin_device, id.uid, id.did,
                             component.c_str(), dberr);
            rv = VSSI_BADCMD;
            goto exit;
        }
    }

 exit:
    mini_trans_end("merge_timestamp", dberr == DATASETDB_OK);
    return rv;
}

// check each prefix of the component to make sure that it is a directory component.
// if not, trash
s16 managed_dataset::create_directory(const std::string& component,
                                      VPLTime_t time, u32 attrs)
{
    int rv = VSSI_SUCCESS;
    string new_component;
    DatasetDBError dberr;
    u64 nversion;
    int components_count = 0;

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "Dataset "FMTu64":"FMTu64" make directory {%s} attrs 0x%08x.",
                        id.uid, id.did, component.c_str(), attrs);

    if(component.empty()) {
        goto exit;
    }

    attrs &= ATTRS_SET_MASK;
    mini_trans_start("create_directory");
    nversion = version + 1;

    dberr = datasetDB.GetSiblingComponentsCount(component, components_count);
    dberr = db_err_classify(dberr);
    if (dberr != DATASETDB_OK) {
        VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                            "Failed to get sibling components count for component %s in DB: Error %d", 
                         component.c_str(), dberr);
        rv = VSSI_ACCESS;
    }

    if(components_count < MAXIMUM_COMPONENTS) {
        dberr = datasetDB.TestAndCreateComponent(component, DATASETDB_COMPONENT_TYPE_DIRECTORY, attrs, nversion, time);
        dberr = db_err_classify(dberr);
        if (dberr != DATASETDB_OK) {
            VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                                "Dataset "FMTu64":"FMTu64" make directory {%s} fail:%d.",
                                id.uid, id.did,
                                component.c_str(), dberr);
            rv = VSSI_ACCESS;
        }
    } else {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Exceed MAXIMUM_COMPONENTS: %d for component %s.",
                         MAXIMUM_COMPONENTS,
                         component.c_str());
        rv = VSSI_MAXCOMP;
    }
    mini_trans_end("create_directory", dberr == DATASETDB_OK);
 exit:
    return rv;
}

int managed_dataset::delete_component(const std::string& component,
                                      VPLTime_t time)
{
    int rv = VSSI_SUCCESS;
    DatasetDBError dberr;
    std::string path;
    std::string fullpath;
    int count;
    int type;
    u64 nversion;

    mini_trans_start("delete_component");
    nversion = version + 1;

    dberr = datasetDB.GetComponentType(component, type);
    dberr = db_err_classify(dberr);
    if (dberr != DATASETDB_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Dataset "FMTu64":"FMTu64" delete {%s} fail get type: %d.",
                         id.uid, id.did,
                         component.c_str(), dberr);
        if (dberr == DATASETDB_ERR_UNKNOWN_COMPONENT) {
            rv = VSSI_SUCCESS;
            goto exit;
        }
        rv = VSSI_INVALID;
        goto exit;
    }

    if ( type == DATASETDB_COMPONENT_TYPE_FILE ) {
        dberr = datasetDB.GetComponentPath(component, path);
        dberr = db_err_classify(dberr);
        if (dberr != DATASETDB_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Dataset "FMTu64":"FMTu64" delete {%s} fail get path: %d.",
                             id.uid, id.did,
                             component.c_str(), dberr);
            rv = VSSI_INVALID;
            goto exit;
        }

        dberr = datasetDB.GetContentRefCount(path, count);
        dberr = db_err_classify(dberr);
        if (dberr != DATASETDB_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Dataset "FMTu64":"FMTu64" delete {%s} fail ref count: %d.",
                             id.uid, id.did,
                             component.c_str(), dberr);
            rv = VSSI_INVALID;
            goto exit;
        }
    }

    dberr = datasetDB.DeleteComponent(component, nversion, time);
    dberr = db_err_classify(dberr);
    if(dberr != DATASETDB_OK) {
        if(dberr == DATASETDB_ERR_UNKNOWN_COMPONENT) {
            // There was nothing to do.
        }
        else {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Dataset "FMTu64":"FMTu64" delete {%s} fail: %d.",
                             id.uid, id.did,
                             component.c_str(), dberr);
            rv = VSSI_BADCMD;
        }
    }
    else if ( (type == DATASETDB_COMPONENT_TYPE_FILE) && (count == 1) ) {
        fullpath = dir_name + BASE_DIR(DATA) + path;
        VPLTRACE_LOG_FINE(TRACE_BVS, 0, "Removing content file %s for component %s",
            fullpath.c_str(), component.c_str());
        rv = VPLFile_Delete(fullpath.c_str());
        if ( rv ) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Delete failed for content file %s (component %s) = %d",
                fullpath.c_str(), component.c_str(), rv);
        }
        rv = VSSI_SUCCESS;
    }
exit:
    mini_trans_end("delete_component", dberr == DATASETDB_OK);
    return rv;
}

int managed_dataset::rename_component(const std::string& source,
                                      const std::string& destination,
                                      VPLTime_t time,
                                      u32 flags)
{
    int rv = VSSI_SUCCESS;
    ComponentInfo info;
    DatasetDBError dberr;
    u64 nversion;
    bool replace_existing = ((flags & VSSI_RENAME_REPLACE_EXISTING)!=0);
    std::string upsource;
    std::string updest;
    std::map<string, vss_file*>::iterator fd_it;
    int type;

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                        "Renaming {%s} to {%s} within {%s} (%sreplace_existing).",
                        source.c_str(), destination.c_str(), dir_name.c_str(),
                        replace_existing ? "" : "!");

    if(!component_exists(source)) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                          "Dataset "FMTu64":"FMTu64" rename {%s} to {%s} fail: source not found.",
                          id.uid, id.did,
                          source.c_str(), destination.c_str());
        rv = VSSI_NOTFOUND;
        goto exit;
    }

    //
    // Don't allow if source is a subpath of destination or vice versa.
    //
    // This logic also checks for rename self -> self which is allowed.
    // This needs to be checked before the check for clobbering an
    // existing target in order for the rename self case to work.
    //
    if (this->datasetdb_options & DATASETDB_OPTION_CASE_INSENSITIVE) {
        if((utf8_casencmp(source.size(), destination.substr(0, source.size()).c_str(), source.size(), source.c_str()) == 0 &&
            destination[source.size()] == '/') ||
           (utf8_casencmp(destination.size(), source.substr(0, destination.size()).c_str(), destination.size(), destination.c_str()) == 0 &&
            source[destination.size()] == '/')) {

            VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                          "Dataset "FMTu64":"FMTu64" rename {%s} to {%s} fail: intersecting directories.",
                          id.uid, id.did,
                          source.c_str(), destination.c_str());
            // Impossible instruction.
            rv = VSSI_INVALID;
            goto exit;
        }
        else {
            // If source and dest are the same modulo case, allow the rename.
            // Note that this is not a NOP in this case, since it may change the way
            // the file is listed by readdir (different case).  It also changes
            // the version number of the target.
            if(utf8_casencmp(source.size(), source.c_str(), destination.size(), destination.c_str()) == 0) {
                VPLTRACE_LOG_FINEST(TRACE_BVS, 0, "rename self (case_insensitive)");
                goto do_rename;
            }
        }
    } else {
        // If source and dest are the same, allow the rename.  Even in this
        // case (exact same path), it is not a NOP since it updates the version.
        if(destination.compare(0, source.size(), source) == 0 &&
           source.size() == destination.size()) {
            VPLTRACE_LOG_FINEST(TRACE_BVS, 0, "rename self (case_sensitive)");
            goto do_rename;
        }
        if((destination.compare(0, source.size(), source) == 0 &&
            destination[source.size()] == '/') ||
           (source.compare(0, destination.size(), destination) == 0 &&
            source[destination.size()] == '/')) {

            VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                          "Dataset "FMTu64":"FMTu64" rename {%s} to {%s} fail: intersecting directories.",
                          id.uid, id.did,
                          source.c_str(), destination.c_str());
            // Impossible instruction.
            rv = VSSI_INVALID;
            goto exit;
        }
    }

    // Disallow clobbering an existing target, unless it is a file
    // and the REPLACE_EXISTING flag is passed.
    if(component_exists(destination)) {
        if(component_is_directory(destination)) {
            rv = VSSI_ISDIR;
        }
        else {
            if(replace_existing) {
                // Need to delete target to get rid of content file and
                // handle the case that it is currently open.
                rv = remove_iw(destination, true);
                if(rv != VSSI_SUCCESS) {
                    goto exit;
                }
                goto do_rename;
            }
            rv = VSSI_FEXISTS;
        }
        goto exit;
    }
    else {
        // Check that the target directory path exists.
        size_t slash = destination.find_last_of('/');
        if(slash != string::npos) {
            string dir;
            dir = destination.substr(0, slash);
            if(!component_is_directory(dir)) {
                VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                                  "Dataset "FMTu64":"FMTu64" rename {%s} to {%s} fail: no destination directory.",
                                  id.uid, id.did,
                                  source.c_str(), destination.c_str());
                rv = VSSI_BADPATH;
                goto exit;
            }
        }
    }

 do_rename:
    mini_trans_start("rename_component");
    nversion = version + 1;
    dberr = datasetDB.MoveComponent(source, destination, 0, nversion, time);
    mini_trans_end("rename_component", dberr == DATASETDB_OK);
    dberr = db_err_classify(dberr);
    if (dberr != DATASETDB_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Dataset "FMTu64":"FMTu64" rename {%s} to {%s} DB error: %d",
                         id.uid, id.did,
                         source.c_str(), destination.c_str(), dberr);
        rv = VSSI_BADCMD;
        goto exit;
    }
    //
    // If the source is an open file, then find the open file descriptor and change the
    // component path so that the open file handle matches the database entry.
    //
    dberr = datasetDB.GetComponentType(destination, type);
    dberr = db_err_classify(dberr);
    if(dberr != DATASETDB_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                        "Dataset "FMTu64":"FMTu64" {%s} fail get type: %d.",
                        id.uid, id.did,
                        destination.c_str(), dberr);
        rv = VSSI_BADCMD;
        goto exit;
    }
    if(type == DATASETDB_COMPONENT_TYPE_FILE) {
        if(is_case_insensitive()) {
            utf8_upper(source.c_str(), upsource);
            utf8_upper(destination.c_str(), updest);
        }
        else {
            upsource = source;
            updest = destination;
        }
        // Search open file map for a match
        VPLMutex_Lock(&fd_mutex);
        for(fd_it = open_handles.begin();
            fd_it != open_handles.end();
            fd_it++) {

            if(upsource.compare(fd_it->second->component) == 0) {
                VPLTRACE_LOG_FINEST(TRACE_BVS, 0, "source {%s} is open as a file"
                                    "renaming file handle to {%s}",
                                    upsource.c_str(), updest.c_str());
                fd_it->second->component = updest;
                // TODO: Can there be more than one matching file handle?
            }
        }
        VPLMutex_Unlock(&fd_mutex);
    }

 exit:
    return rv;
}

int managed_dataset::make_dataset_directory()
{
    int rc;
    int i;

    // Make the dataset directory.
    if(VPLFile_CheckAccess(dir_name.c_str(), VPLFILE_CHECKACCESS_EXISTS) != VPL_OK) {
        rc = Util_CreatePath(dir_name.c_str(), true);
        if(rc != 0) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Failed to create directory %s. Error:%d",
                             dir_name.c_str(), rc);
            return rc;
        }
    }

    // Additionally create basic directories.
    for(i = 0; !base_dirs[i].empty(); i++) {
        string new_dir = dir_name + base_dirs[i];
        if(VPLFile_CheckAccess(new_dir.c_str(), VPLFILE_CHECKACCESS_EXISTS) != VPL_OK) {
            rc = VPLDir_Create(new_dir.c_str(), 0777);
            if(rc != VPL_OK) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "Failed to create directory %s. Error:%d",
                                 new_dir.c_str(), rc);
                return VSSI_ACCESS;
            }
        }
    }
    return VSSI_SUCCESS;
}

int managed_dataset::get_total_used_size(u64& size)
{
    int rv = VSSI_SUCCESS;
    DatasetDBError dberr;

    // get_total_used_size() must not hold off managed dataset db backup or fsck: Bug 9515

    VPLMutex_Lock(&access_mutex);
    dberr = datasetDB.GetComponentSize("", size);
    dberr = db_err_classify(dberr);
    if (dberr != DATASETDB_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to get dataset size in DB: Error %d", dberr);
        rv = VSSI_NOTFOUND;
        goto exit;
    }

 exit:
    VPLMutex_Unlock(&access_mutex);
    return rv;
}

void managed_dataset::mini_trans_start(const char* str)
{
    DatasetDBError dbrv;

    VPLMutex_Lock(&sync_mutex);
    if (mini_tran_open) {
        FAILED_ASSERT("already open");
        return;
    }

    if ( !tran_is_open ) {
        dbrv = datasetDB.BeginTransaction();
        dbrv = db_err_classify(dbrv);
        if (dbrv != DATASETDB_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Dataset "FMTu64":"FMTu64": BeginTransaction(%s):%d.",
                             id.uid, id.did, str, dbrv);
        }
        else {
            tran_is_open = true;
        }
    }
    mini_tran_open = true;
}

void managed_dataset::mini_trans_end(const char* str, bool trigger_commit)
{
    if (!mini_tran_open) {
        FAILED_ASSERT("Not open");
        return;
    }

    mini_tran_open = false;

    if (trigger_commit) {
        // Note: This thing is called with the wr_lock held
        // Seems that on any file close or commit we need to bump
        // the version or we could end up with multiple committed
        // changes going to a file under the same version number
        // Needs to be bumped with the dataset lock
        version++;

        // Set the timestamp now so that we don't immediately fire
        // off a backup when the per_sync thread wakes up.
        last_action = VPLTime_GetTimeStamp();
        if ( !tran_needs_commit ) {
            // Wake up the periodic sync
            tran_needs_commit = true;
            VPLCond_Broadcast(&per_sync_cond);
        }
    }
    VPLMutex_Unlock(&sync_mutex);
}

s16 managed_dataset::commit_iw(void)
{
    s16 rv = VSSI_SUCCESS;

    VPLTRACE_LOG_FINE(TRACE_BVS, 0, "commit");
    
    return rv;
}

s16 managed_dataset::set_metadata_iw(const std::string& component_in,
                                     u8 type,
                                     u8 length,
                                     const char* data)
{
    s16 rv = VSSI_SUCCESS;
    DatasetDBError dberr;
    VPLTime_t change_time;
    string metadata(data, length);
    std::string component = normalizePath(component_in);

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      "set %d metadata component {%s} for %d bytes",
                      type, component.c_str(), length);
    
    VPLMutex_Lock(&access_mutex);
    change_time = VPLTime_GetTime();

    mini_trans_start("set_metadata_iw");
    if(length == 0) {
        dberr = datasetDB.DeleteComponentMetadata(component, type);
        dberr = db_err_classify(dberr);
        if(dberr == DATASETDB_ERR_UNKNOWN_COMPONENT) {
            VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                              FMTu64":"FMTu64" clear metadata type %d for {%s}: component doesn't exist.",
                              id.uid, id.did,
                              type, component.c_str());
            rv = VSSI_BADCMD;
            goto exit;
        }
        else if(dberr != DATASETDB_OK && dberr != DATASETDB_ERR_UNKNOWN_METADATA) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             FMTu64":"FMTu64" clear metadata type %d for {%s} in DB fail:%d.",
                             id.uid, id.did,
                             type, component.c_str(), dberr);
            rv = VSSI_BADCMD;
            goto exit;
        }
    }
    else {
        dberr = datasetDB.SetComponentMetadata(component, type, metadata);
        dberr = db_err_classify(dberr);
        if(dberr == DATASETDB_ERR_UNKNOWN_COMPONENT) {
            VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                              FMTu64":"FMTu64" set metadata type %d for {%s}: component doesn't exist.",
                              id.uid, id.did,
                              type, component.c_str());
            rv = VSSI_BADCMD;
            goto exit;
        }
        else if(dberr != DATASETDB_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             FMTu64":"FMTu64" set metadata type %d for {%s} in DB fail:%d.",
                             id.uid, id.did,
                             type, component.c_str(), dberr);
            rv = VSSI_BADCMD;
            goto exit;
        }
    }
 exit:
    mini_trans_end("set_metadata_iw", dberr == DATASETDB_OK);
    VPLMutex_Unlock(&access_mutex);
    return rv;
}

//
// Check whether a component can be removed.  Reasons for failure
// include:  file is currently open or directory non-empty
//
bool managed_dataset::remove_allowed(const std::string& component, 
                                     vss_file*& fp,
                                     bool rename_target)
{
    bool allowed = true;
    DatasetDBError dberr;
    int type;
    std::string path;
    std::map<string, vss_file*>::iterator fd_it;
    std::vector<std::string> names;

    do {
        dberr = datasetDB.GetComponentType(component, type);
        dberr = db_err_classify(dberr);
        if(dberr != DATASETDB_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                            "Dataset "FMTu64":"FMTu64" {%s} fail get type: %d.",
                            id.uid, id.did,
                            component.c_str(), dberr);
            break;
        }

        if(type == DATASETDB_COMPONENT_TYPE_FILE) {
            dberr = datasetDB.GetComponentPath(component, path);
            dberr = db_err_classify(dberr);
            if(dberr != DATASETDB_OK) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                "Dataset "FMTu64":"FMTu64" {%s} fail get path: %d.",
                                id.uid, id.did,
                                component.c_str(), dberr);
                break;
            }
    
            // See if there is an open file handle for this path
            VPLMutex_Lock(&fd_mutex);
            fd_it = open_handles.find(path);
            if(fd_it != open_handles.end()) {
                fp = fd_it->second;
                //
                // There are two cases:  normal remove and the
                // case that we are removing the target of a
                // rename with the REPLACE_EXISTING mode.
                //
                // In the normal rename case, only look at the
                // access modes to check OPEN_DELETE bit.
                //
                // In the rename replace target case, we ignore
                // the access modes, but make sure that exactly
                // one client FD points at the file.  Believe it
                // or not, this is what MSoft MoveFileEx semantics
                // require.
                //
                if(rename_target) {
                    if(fp->get_refcount() != 1) {
                        allowed = false;
                    }
                }
                else {
                    u32 access_mode = fp->get_access_mode();
                    if(!(access_mode & VSSI_FILE_OPEN_DELETE)) {
                        allowed = false;
                    }
                }
            }
            VPLMutex_Unlock(&fd_mutex);
            VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                              "%sallow delete of %s file {%s}",
                              allowed?"":"dis",
                              allowed?"unreferenced":"open",
                              component.c_str());
            break;
        }

        if(type == DATASETDB_COMPONENT_TYPE_DIRECTORY) {
            dberr = datasetDB.ListComponents(component, names);
            dberr = db_err_classify(dberr);
            if (dberr != DATASETDB_OK) {
                break;
            }
            if(!names.empty()) {
                allowed = false;
            }
            VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                              "%sallow delete of %sempty dir {%s}",
                              allowed?"":"dis",
                              allowed?"":"non-",
                              component.c_str());
        }

    } while (0);

    return allowed;
}

bool managed_dataset::check_open_conflict(const std::string& component)
{
    bool conflict = false;
    DatasetDBError dberr;
    int type;
    std::string upper;
    std::map<string, vss_file*>::iterator fd_it;

    if(is_case_insensitive()) {
        utf8_upper(component.c_str(), upper);
    }
    else {
        upper = component;
    }

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0, "component {%s}", upper.c_str());

    do {
        dberr = datasetDB.GetComponentType(upper, type);
        dberr = db_err_classify(dberr);
        if(dberr != DATASETDB_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                            "Dataset "FMTu64":"FMTu64" {%s} fail get type: %d.",
                            id.uid, id.did,
                            component.c_str(), dberr);
            break;
        }

        VPLMutex_Lock(&fd_mutex);
        for(fd_it = open_handles.begin();
            fd_it != open_handles.end();
            fd_it++) {
            //
            // If the "from" path is a file, then it only conflicts if it
            // matches fully with the filename of the file handle &&
            // the file is not opened with OPEN_DELETE.
            //
            if(type == DATASETDB_COMPONENT_TYPE_FILE) {
                if(upper.compare(fd_it->second->component) == 0) {
                    u32 access_mode = fd_it->second->get_access_mode();
                    if(!(access_mode & VSSI_FILE_OPEN_DELETE)) {
                        conflict = true;
                    }
                    VPLTRACE_LOG_FINEST(TRACE_BVS, 0, "component {%s} is open as a file"
                                        " and OPEN_DELETE is %s",
                                        upper.c_str(),
                                        conflict?"false":"true");
                    break;
                }
            }
            //
            // If the "from" path is a directory, then it conflicts if
            // the file handle points to a file below that directory.
            //
            else if(type == DATASETDB_COMPONENT_TYPE_DIRECTORY) {
                size_t findpos;
                findpos = fd_it->second->component.find(upper);
                if(findpos == 0) {
                    VPLTRACE_LOG_FINEST(TRACE_BVS, 0, "dir {%s} is parent of {%s}", 
                                        upper.c_str(), fd_it->second->component.c_str());
                    conflict = true;
                    break;
                }
                VPLTRACE_LOG_FINEST(TRACE_BVS, 0, "dir {%s} comp {%s} findpos %d", 
                                    upper.c_str(), fd_it->second->component.c_str(), findpos);
            }
        }
        VPLMutex_Unlock(&fd_mutex);

    } while (0);
    
    VPLTRACE_LOG_FINEST(TRACE_BVS, 0, "component {%s}: %sconflict", upper.c_str(), conflict?"":"no ");

    return conflict;
}

s16 managed_dataset::remove_iw(const std::string& component_in,
                               bool rename_target)
{
    s16 rv = VSSI_SUCCESS;
    VPLTime_t change_time;
    DatasetDBError dberr;
    ComponentInfo info;
    vss_file *fp = NULL;
    std::string component = normalizePath(component_in);

    VPLTRACE_LOG_FINE(TRACE_BVS, 0, "remove component {%s} (%srename_target)",
                      component.c_str(),
                      rename_target ? "" : "!");

    VPLMutex_Lock(&access_mutex);
    //
    // The remove will be disallowed in the following cases:
    //
    // 1) the component is a non-empty directory
    // 2) there is an open file handle to the component which
    //    is not opened with OPEN_DELETE mode set OR the
    //    operation in progress is a rename replace_existing
    //    and the file handle has a refcount of 1
    //
    // This code needs to support deleting an open file and
    // then allowing subsequent IO to it with the actual delete
    // deferred until the last close.  This is required since
    // Windows supports this classic temp file behavior.
    //
    if(!remove_allowed(component, fp, rename_target)) {
        rv = VSSI_BUSY;
        goto exit;
    }

    dberr = datasetDB.GetComponentInfo(component, info);
    dberr = db_err_classify(dberr);
    if (dberr != DATASETDB_OK) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                          "Couldn't get info on component %s from DB: %d.",
                          component.c_str(), dberr);
        rv = VSSI_NOTFOUND;
        goto exit;
    }

    //
    // Handle the case of an open file.  Permission to delete
    // was checked above, so mark it for delete on close and
    // remove the database entries.  The file will stay open
    // and can be accessed, but the name can be recycled after
    // this point.  Also need to remove the filename to vss_file
    // map entry.  The file_id map entry remains.
    //
    if(info.type == DATASETDB_COMPONENT_TYPE_FILE && fp != NULL) {
        std::map<string, vss_file*>::iterator fd_it;
        VPLMutex_Lock(&fd_mutex);
        fd_it = open_handles.find(fp->filename);
        if(fd_it != open_handles.end()) {
            open_handles.erase(fd_it);
        }
        VPLMutex_Unlock(&fd_mutex);
        VPLTRACE_LOG_FINE(TRACE_BVS, 0, 
                          "defer delete of component {%s} until last close",
                          component.c_str());
        fp->set_delete_pending();
    }

    change_time = VPLTime_GetTime();

    rv = delete_component(component, change_time);
    if ( rv != VSSI_SUCCESS ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Dataset "FMTu64":"FMTu64": Failed remove {%s} - %d",
                         id.uid, id.did, component.c_str(), rv);
        goto exit;
    }

 exit:
    VPLMutex_Unlock(&access_mutex);
    return rv;
}

s16 managed_dataset::set_size_iw(const std::string& component_in, u64 new_size)
{
    s16 rv = VSSI_SUCCESS;
    vss_file* handle = NULL;
    std::string component = normalizePath(component_in);

    VPLMutex_Lock(&access_mutex);
    rv = open_file(component, 0, VSSI_FILE_OPEN_WRITE | VSSI_FILE_OPEN_CREATE,
                   0, handle);
    if ( rv && (rv != VSSI_EXISTS) ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
            "Dataset "FMTu64":"FMTu64 " open_file(%s) - %d",
            id.uid, id.did, component.c_str(), rv);
        goto done;
    }

    rv = truncate_file(handle, NULL, 0, new_size);
    if ( rv ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
            "Dataset "FMTu64":"FMTu64 "truncate_file(%s, "FMTu64") - %d",
            id.uid, id.did, component.c_str(), new_size, rv);
        goto done;
    }

    rv = close_file(handle, NULL, 0);
    if ( rv ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
            "Dataset "FMTu64":"FMTu64 "close_file(%s) - %d",
            id.uid, id.did, component.c_str(), rv);
        goto done;
    }
    handle = NULL;

done:
    if ( handle ) {
        (void)close_file(handle, NULL, 0);
    }
    VPLMutex_Unlock(&access_mutex);
    return rv;
}

s16 managed_dataset::rename_iw(const std::string& component_in,
                               const std::string& new_name_in,
                               u32 flags)
{
    s16 rv = VSSI_SUCCESS;
    VPLTime_t change_time;
    std::string component = normalizePath(component_in);
    std::string new_name = normalizePath(new_name_in);

    VPLTRACE_LOG_FINE(TRACE_BVS, 0, "rename component {%s} to {%s} "
                      "(%sreplace_existing).",
                      component.c_str(), new_name.c_str(),
                      (flags & VSSI_RENAME_REPLACE_EXISTING) ? "" : "!");

    VPLMutex_Lock(&access_mutex);
    if(check_open_conflict(component)) {
        rv = VSSI_BUSY;
        goto done;
    }

    change_time = VPLTime_GetTime();
    rv = rename_component(component, new_name, change_time, flags);
    if(rv != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Dataset "FMTu64":"FMTu64": Failed rename {%s}: %d",
                         id.uid, id.did, component.c_str(), rv);
        goto done;
    }

 done:
    VPLMutex_Unlock(&access_mutex);
    return rv;
}

s16 managed_dataset::set_times_iw(const std::string& component_in,
                                  u64 ctime, u64 mtime)
{
    s16 rv = VSSI_SUCCESS;
    VPLTime_t change_time;
    u64 nversion;
    std::string component = normalizePath(component_in);

    VPLMutex_Lock(&access_mutex);
    nversion = version + 1;

    VPLTRACE_LOG_FINE(TRACE_BVS, 0, "set_times component {%s}",
                      component.c_str());

    change_time = VPLTime_GetTime();

    // The change index is always 0 since we don't have journals
    rv = merge_timestamp(component,
                         ctime, mtime,
                         nversion,
                         0,
                         0, // origin_device - used in log messages
                         change_time);
    if ( rv != VSSI_SUCCESS ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Dataset "FMTu64":"FMTu64
                            ": Failed timestamp record:%d.",
                         id.uid, id.did, rv);
        goto done;
    }

    // If we are updating the mtime and if the file is open then also need to reset its new_modify_time flag.
    // Otherwise, we just need the db update above.
    if (mtime != 0) {
        std::string path;
        DatasetDBError dberr = datasetDB.GetComponentPath(component, path);
        dberr = db_err_classify(dberr);
        // Setting mtime on a nonexistent file should not return a failure (see merge_timestamp).
        // We proceed here if the component actually exists, but ignore any error
        if (dberr == DATASETDB_OK) {
            std::map<string, vss_file*>::iterator handle_it;
            VPLMutex_Lock(&fd_mutex);
            handle_it = open_handles.find(path);
            if (handle_it != open_handles.end()) {    
                VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Cancel new_modify_time");
                handle_it->second->set_new_modify_time(false);
            }
            VPLMutex_Unlock(&fd_mutex);       
        }
    }

 done:
    VPLMutex_Unlock(&access_mutex);
    return rv;
}

s16 managed_dataset::make_directory_iw(const std::string& component_in, u32 attrs)
{
    s16 rv = VSSI_SUCCESS;
    VPLTime_t change_time;
    std::string component = normalizePath(component_in);

    VPLTRACE_LOG_FINE(TRACE_BVS, 0, "make_directory component {%s}"
                      " attrs 0x%08x version "FMTu64".", component.c_str(), attrs, version);

    VPLMutex_Lock(&access_mutex);
    //
    // Check whether the containing directory path exists.  In order to avoid "mkdir -p"
    // behavior, the error needs to be detected at this level.  Calling create_directory
    // will create the parent tree, but that's not the desired behavior.  Changing the
    // lower level datasetDB code is too scary, so do it here.
    //
    size_t slash = component.find_last_of('/');
    if(slash != string::npos) {
        string dir = component.substr(0, slash);
        if(!component_is_directory(dir)) {
            rv = VSSI_BADPATH;
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Parent directory not found for directory %s.",
                             component.c_str());
            goto done;
        }
    }

    // Check that the target does not already exist either as a file or directory
    if(component_exists(component)) {
        if(component_is_directory(component)) {
            rv = VSSI_ISDIR;
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                            "Directory already exists at %s.",
                            component.c_str());
        }
        else {
            rv = VSSI_FEXISTS;
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                            "File already exists at %s.",
                            component.c_str());
        }
        goto done;
    }

    change_time = VPLTime_GetTime();

    rv = create_directory(component, change_time, attrs);
    if(rv != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Dataset "FMTu64":"FMTu64": Failed mkdir record:%d.",
                         id.uid, id.did, rv);
        goto done;
    }

 done:
    VPLMutex_Unlock(&access_mutex);
    return rv;
}

s16 managed_dataset::chmod_iw(const std::string& component_in, u32 attrs, u32 attrs_mask)
{
    s16 rv = VSSI_SUCCESS;
    VPLTime_t change_time;
    u64 nversion = version + 1;
    DatasetDBError dberr = DATASETDB_OK;
    std::string path;
    std::map<string, vss_file*>::iterator fd_it;
    ComponentInfo info;
    std::string component = normalizePath(component_in);

    VPLTRACE_LOG_FINE(TRACE_BVS, 0, "chmod component {%s} - 0x%08x - 0x%08x",
                      component.c_str(), attrs, attrs_mask);

    VPLMutex_Lock(&access_mutex);

    attrs &= ATTRS_SET_MASK;
    attrs_mask &= ATTRS_CLR_MASK;

    dberr = datasetDB.GetComponentInfo(component, info);
    dberr = db_err_classify(dberr);
    if (dberr != DATASETDB_OK) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                          "Couldn't get info on component %s from DB: %d.",
                          component.c_str(), dberr);
        rv = VSSI_NOTFOUND;
        goto done_cached;
    }

    // Check if component is a file. Chmod is only for directories
    if (info.type != DATASETDB_COMPONENT_TYPE_DIRECTORY) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Attempting VSSI_Chmod on file %s. Use VSSI_ChmodFile for files.", component.c_str());
        rv = VSSI_NOTDIR;
        goto done_cached;
    }

    mini_trans_start("chmod_iw");
    change_time = VPLTime_GetTime();
    attrs = (info.perm & ~(attrs_mask & ATTRS_CLR_MASK)) | 
        (attrs & (attrs_mask & ATTRS_SET_MASK));
    dberr = datasetDB.SetComponentPermission(component, attrs, nversion);
    dberr = db_err_classify(dberr);
    if (dberr != DATASETDB_OK) {
        goto done;
    }

 done:
    mini_trans_end("chmod_iw", dberr == DATASETDB_OK);

    if (dberr != DATASETDB_OK) {
        if (dberr == DATASETDB_ERR_UNKNOWN_COMPONENT || dberr == DATASETDB_ERR_UNKNOWN_CONTENT) {
            rv = VSSI_NOTFOUND;
        }
        else {
            // For lack of a better choice
            rv = VSSI_INVALID;
        }
    }
done_cached:
    VPLMutex_Unlock(&access_mutex);
    return rv;
}

s16 managed_dataset::open_file(const std::string& component_in,
                               u64 iversion,
                               u32 flags,
                               u32 attrs,
                               vss_file*& ret_handle)
{
    s16 rv = VSSI_SUCCESS;
    DatasetDBError dberr;
    std::string path;
    std::string fullpath;
    std::string data_path;
    std::map<string, vss_file*>::iterator fd_it;
    vss_file* handle = NULL;
    std::map<u32, vss_file*>::iterator fh_it;
    u32 file_id;
    VPLTime_t curtime;
    VPLFile_handle_t fd;
    bool writeable;
    bool createable;
    bool create_exists = false;
    bool truncate_file = false;
    bool is_created = false;
    bool open_mini_trans = false;
    int mode = 0;
    ComponentInfo info;
    int components_count = 0;
    std::string component = normalizePath(component_in);

    writeable = ((flags & VSSI_FILE_OPEN_WRITE) != 0);
    createable = 
        ((flags & (VSSI_FILE_OPEN_OPEN_ALWAYS|VSSI_FILE_OPEN_CREATE_ALWAYS|VSSI_FILE_OPEN_CREATE_NEW)) != 0);

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0, "open file component {%s} (%screateable & %scase-sensitive)",
                        component.c_str(), createable?"":"!", is_case_insensitive()?"!":"");

    VPLMutex_Lock(&access_mutex);
    // Open File not valid for directories
    if(component_is_directory(component)) {
        rv = VSSI_ISDIR;
        goto done;
    }

    path.clear();
    dberr = datasetDB.GetComponentPath(component, path);
    dberr = db_err_classify(dberr);
    if (dberr != DATASETDB_OK) {
        if (dberr == DATASETDB_ERR_UNKNOWN_COMPONENT || dberr == DATASETDB_ERR_UNKNOWN_CONTENT) {
            //
            // Check whether the containing directory path exists.  In order to avoid "mkdir -p"
            // behavior, the error needs to be detected at this level.  Calling SetComponentPath
            // will create the parent tree, but that's not the desired behavior.  Changing the
            // lower level datasetDB code is too scary, so do it here.
            //
            size_t slash = component.find_last_of('/');
            if(slash != string::npos) {
                string dir = component.substr(0, slash);
                if(!component_is_directory(dir)) {
                    rv = VSSI_BADPATH;
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                     "Parent directory not found for file {%s}.",
                                     component.c_str());
                    goto done;
                }
            }
            //
            // Parent directory exists, but file is not found.  Openfile needs to return a
            // different error to distinguish the "path not complete" case from "file not found".
            //
            if (!createable) {
                rv = VSSI_NOTFOUND;
                goto done;
            }
            mini_trans_start("open_file");
            is_created = true;
            open_mini_trans = true;
            curtime = VPLTime_GetTime();
            attrs |= VSSI_ATTR_ARCHIVE;
            attrs &= ATTRS_SET_MASK;

            dberr = datasetDB.GetSiblingComponentsCount(component, components_count);
            dberr = db_err_classify(dberr);
            if (dberr != DATASETDB_OK) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "Failed to get sibling components count for component %s in DB: Error %d", 
                                 component.c_str(), dberr);
                rv = VSSI_NOTFOUND;
                goto done;
            }

            if(components_count < MAXIMUM_COMPONENTS) {
                data_path = dir_name + BASE_DIR(DATA);
                dberr = datasetDB.TestAndCreateComponent(component,
                                                         DATASETDB_COMPONENT_TYPE_FILE,
                                                         attrs,
                                                         version + 1,
                                                         curtime,
                                                         &path,
                                                         &location_exists,
                                                         &data_path);
                dberr = db_err_classify(dberr);
                if (dberr != DATASETDB_OK) {
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                     "Failed to test/create component %s in DB: Error %d",
                                     component.c_str(), dberr);
                    rv = VSSI_NOTFOUND;
                    goto done;
                }
            } else {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "Exceed MAXIMUM_COMPONENTS: %d for component %s.",
                                 MAXIMUM_COMPONENTS,
                                 component.c_str());
                rv = VSSI_MAXCOMP;
                goto done;
            }
            mini_trans_end("open_file", true);
            open_mini_trans = false;
        }
    }
    else {
        // Component already exists

        // CREATE_NEW fails if the file exists
        if((flags & VSSI_FILE_OPEN_CREATE_NEW) != 0) {
            rv = VSSI_PERM;
            goto done;
        }
        //
        // This is a tricky case:  if CREATE_ALWAYS and the
        // file exists, the operation succeeds, but returns
        // VSSI_EXISTS to indicate the file existed.
        //
        if((flags & VSSI_FILE_OPEN_CREATE_ALWAYS) != 0) {
            truncate_file = true;
            create_exists = true;
        }
        //
        // OPEN_CREATE is equivalent to OPEN_ALWAYS in Win32
        // which has similar behavior to CREATE_ALWAYS, but
        // does not truncate
        //
        if((flags & VSSI_FILE_OPEN_OPEN_ALWAYS) != 0) {
            create_exists = true;
        }
        if((flags & VSSI_FILE_OPEN_TRUNCATE_ALWAYS) != 0) {
            truncate_file = true;
        }
    }
    dberr = datasetDB.GetComponentInfo(component, info);
    dberr = db_err_classify(dberr);
    if(dberr != DATASETDB_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "GetComponentInfo{%s}, failed - %d",
                          component.c_str(), dberr);
        rv = VSSI_NOTFOUND;
        goto done;
    }

    // File exists, check whether there is already a handle open using the real disk path
    // to avoid ambiguity about case insensitive component names
    VPLMutex_Lock(&fd_mutex);
    fd_it = open_handles.find(path);
    if(fd_it == open_handles.end()) {
        // No handle found
        VPLTRACE_LOG_FINE(TRACE_BVS, 0, "no handle found for {%s}, make one",
                          path.c_str());
        if(!is_created && writeable && (info.perm & VSSI_ATTR_READONLY)) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "open of readonly file {%s} for "
                             "write", component.c_str());
            rv = VSSI_PERM;
            goto release_done;
        }

        // Make sure the whole directory path exists.
        // Note: this is for the actual content file, not the component in the database.
        if(createable) {
            size_t slash = path.find_last_of('/');
            if(slash != string::npos) {
                string dir = dir_name + BASE_DIR(DATA) + path.substr(0, slash);
                rv = Util_CreatePath(dir.c_str(), true);
                if(rv != 0) {
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                    "Failed to create parent directory for file %s.",
                                    path.c_str());
                    goto release_done;
                }
            }
        }
        //
        // Always open for R/W at the VPL level.  This is needed
        // because there are sharing combinations that allow WRITE
        // to be added on a subsequent open and it's too complicated
        // to close and reopen the VPL FD.  The access modes are
        // enforced based on the vss_file level access_mode.
        //
        mode = VPLFILE_OPENFLAG_READWRITE;
        if(createable) {
            mode |= VPLFILE_OPENFLAG_CREATE;
        }
        fullpath = dir_name + BASE_DIR(DATA) + path;
        fd = VPLFile_Open(fullpath.c_str(), mode, 0777);
        if(!VPLFile_IsValidHandle(fd)) {
            VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                              "Failed to open file {%s} with mode %x. Error:%d",
                              fullpath.c_str(), mode, fd);
            handle = NULL;
            if (fd == VPL_ERR_MAX) {
                rv = VSSI_HLIMIT;
            }
            else {
                rv = VSSI_BADOBJ;
            }
        }
        else {
            VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                              "Opened file {%s} mode %x with fd %d.",
                              fullpath.c_str(), mode, fd);
            // Pull out the attributes and size

            // Normalize the component name to help with path matching
            // to avoid open files getting renamed
            std::string upper;
            if (is_case_insensitive()) {
                utf8_upper(component.c_str(), upper);
            }
            else {
                upper = component;
            }

            // Create the new file handle and add it to the map
            handle = new vss_file(this, fd, upper, path, flags, 
                                  info.perm, info.size, is_created);
            open_handles[path] = handle;
            //
            // Use unique file_id to pass back to the client and keep
            // track of the mapping of the file_id to the vss_file pointer.
            // There is a small probability that the file_id calculated
            // in the vss_file constructor is not unique, so check the
            // map to insure uniqueness.
            //
            file_id = handle->get_file_id();
            fh_it = file_id_to_handle.find(file_id);
            while(fh_it != file_id_to_handle.end()) {
#define __FILE_ID_DEBUG__
#ifdef __FILE_ID_DEBUG__
                VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0, "FileID Collision on %08x:"
                    "v1 %08x t1 %08x v2 %08x t2 %08x",
                    file_id,
                    (u32) handle, file_id ^ (u32)handle,
                    (u32) fh_it->second, file_id ^ (u32) fh_it->second);
#endif
                file_id++;
                handle->set_file_id(file_id);
                fh_it = file_id_to_handle.find(file_id);
            }
            file_id_to_handle[file_id] = handle;
        }
    }
    else {
        // Handle exists, add reference if it doesn't conflict
        if(!is_created && writeable && 
                (fd_it->second->get_attrs() & VSSI_ATTR_READONLY)) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "open of readonly file {%s} for "
                             "write", component.c_str());
            rv = VSSI_PERM;
            goto release_done;
        }
        handle = fd_it->second;
        // If the file is already open but deleted, don't allow the open
        if(handle->delete_is_pending()) {
            VPLTRACE_LOG_FINE(TRACE_BVS, 0, "handle found for {%s}, but delete pending",
                              path.c_str());
            rv = VSSI_BUSY;
            handle = NULL;
            goto release_done;
        }
        VPLTRACE_LOG_FINEST(TRACE_BVS, 0, "handle found for {%s}, add reference",
                            path.c_str());
        rv = handle->add_reference(flags);
        if(rv != VSSI_SUCCESS) {
            handle = NULL;
        }
    }

 release_done:
    VPLMutex_Unlock(&fd_mutex);

    // This is a little sketchy to do a truncate without checking for locks,
    // but we don't have quite enough context to do that correctly.
    // Let the buyer beware if they are using CREATE_ALWAYS in combination
    // with pre-existing files and file locking.
    // TODO: a simple solution might be to check if there are any locks
    // at all on the file and just reject the truncate (silently?) if so.
    if(truncate_file && handle != NULL) {
        s32 vpl_rc;
        vpl_rc = VPLFile_TruncateAt(handle->get_fd(), 0);
        if (vpl_rc != VPL_OK) {
            VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                              "TruncateAt() failed on file %s. %d.",
                              handle->component.c_str(), vpl_rc);
        }
   
        // Set file length and mark the file as having changed since
        // last backup.
        handle->set_attrs(handle->get_attrs() | VSSI_ATTR_ARCHIVE);
        handle->set_size(0);
    }

 done:
    if(open_mini_trans) {
        mini_trans_end("open_file");
    }
    if(rv == VSSI_SUCCESS) {
        ret_handle = handle;
        if(create_exists) {
            rv = VSSI_EXISTS;
        }
    }
    else {
        ret_handle = NULL;
    }
    VPLMutex_Unlock(&access_mutex);
    return rv;
}

s16 managed_dataset::read_file(vss_file* file,
                               vss_object* object,
                               u64 origin,
                               u64 offset,
                               u32& length,
                               char* data)
{
    s16 rv = VSSI_SUCCESS;
    s32 bytes_read = 0;

    VPLMutex_Lock(&access_mutex);
    // If the read conflicts with a byte range lock, return error.
    if(file->check_lock_conflict(origin, VSSI_FILE_LOCK_READ_SHARED, offset, offset+length)) {
        rv = VSSI_LOCKED;
        goto done;
    }

    // Check for conflicting oplocks, which may generate oplock breaks.
    if(file->check_oplock_conflict(object, false)) {
        // If there are conflicting oplocks, the read needs to wait for them to clear
        VPLTRACE_LOG_FINE(TRACE_BVS, 0, "Oplock breaks were generated. Queue and retry.");
        // This error code will cause the request to queued for later handling
        rv = VSSI_RETRY;
        goto done;
    }

    bytes_read = VPLFile_ReadAt(file->get_fd(), data, length, offset);
    if(bytes_read != length) {
        VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                          "Failed to read full %u bytes requested from file %s. Return: %d.",
                          length, file->component.c_str(), bytes_read);
    }
    if(bytes_read < 0) {
        rv = bytes_read;
    } else {
        length = bytes_read;
    }
 done:
    last_action = VPLTime_GetTimeStamp();
    VPLMutex_Unlock(&access_mutex);
    return rv;
}

s16 managed_dataset::write_file(vss_file* file,
                                vss_object* object,
                                u64 origin,
                                u64 offset,
                                u32& length,
                                const char* data)
{
    s16 rv = VSSI_SUCCESS;
    s32 bytes_written = 0;

    VPLMutex_Lock(&access_mutex);
    if(file->check_lock_conflict(origin, VSSI_FILE_LOCK_WRITE_EXCL, offset, offset+length)) {
        rv = VSSI_LOCKED;
        goto done;
    }

    // check space available
    if(remaining_space_size < length) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "no available space for writing");
        rv = VSSI_NOSPACE; 
        goto done;
    }

    // Check for conflicting oplocks, which may generate oplock breaks.
    if(file->check_oplock_conflict(object, true)) {
        // If there are conflicting oplocks, the write needs to wait for them to clear
        VPLTRACE_LOG_FINE(TRACE_BVS, 0, "Oplock breaks were generated. Queue and retry.");
        // This error code will cause the request to queued for later handling
        rv = VSSI_RETRY;
        goto done;
    }

    bytes_written = VPLFile_WriteAt(file->get_fd(), data, length, offset);
    if(bytes_written != length) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to write full %u byte entry to file %s. Return: %d.",
                         length, file->component.c_str(), bytes_written);
        if(bytes_written < 0) {
            rv = bytes_written;
            goto done;
        }
    }
    length = bytes_written;

    // Mark the file as having changed since last backup.
    VPLMutex_Lock(&fd_mutex);
    file->set_attrs(file->get_attrs() | VSSI_ATTR_ARCHIVE);
    file->set_size_if_larger(length + offset);
    VPLMutex_Unlock(&fd_mutex);

 done:
    VPLMutex_Unlock(&access_mutex);
    return rv;
}

s16 managed_dataset::truncate_file(vss_file* file,
                                   vss_object* object,
                                   u64 origin,
                                   u64 offset)
{
    s16 rv = VSSI_SUCCESS;
    s32 vpl_rc;
    u64 start;
    u64 end;

    VPLMutex_Lock(&access_mutex);
    // For Truncate, check for lock conflict from the new EOF to infinity
    start = offset;
    end = 0xffffffffffffffffULL;
    if(file->check_lock_conflict(origin, VSSI_FILE_LOCK_WRITE_EXCL, start, end)) {
        rv = VSSI_LOCKED;
        goto done;
    }

    // Check for conflicting oplocks, which may generate oplock breaks.
    if(file->check_oplock_conflict(object, true)) {
        // If there are conflicting oplocks, the truncate needs to wait for them to clear
        VPLTRACE_LOG_FINE(TRACE_BVS, 0, "Oplock breaks were generated. Queue and retry.");
        // This error code will cause the request to queued for later handling
        rv = VSSI_RETRY;
        goto done;
    }

    vpl_rc = VPLFile_TruncateAt(file->get_fd(), offset);
    if (vpl_rc != VPL_OK) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                          "TruncateAt() failed on file %s. %d.",
                          file->component.c_str(), vpl_rc);
        goto done;
    }

    // Mark the file as having changed since last backup.
    VPLMutex_Lock(&fd_mutex);
    file->set_attrs(file->get_attrs() | VSSI_ATTR_ARCHIVE);
    file->set_size(offset);
    VPLMutex_Unlock(&fd_mutex);

 done:
    VPLMutex_Unlock(&access_mutex);
    return rv;
}

s16 managed_dataset::chmod_file(vss_file* file,
                                vss_object* object,
                                u64 origin,
                                u32 attrs,
                                u32 attrs_mask)
{
    s16 rv = VSSI_SUCCESS;

    VPLMutex_Lock(&access_mutex);
    // Set the new attributes
    // Note: We don't even try and keep in sync with the VSSI_1 API.
    VPLMutex_Lock(&fd_mutex);
    attrs = (file->get_attrs() & ~(attrs_mask & ATTRS_CLR_MASK)) | 
        (attrs & (attrs_mask & ATTRS_SET_MASK));
    file->set_attrs(attrs);
    VPLMutex_Unlock(&fd_mutex);

    VPLMutex_Unlock(&access_mutex);
    return rv;
}

void managed_dataset::file_update_database(vss_file* file)
{
    u32 attrs;
    u64 size;
    VPLTime_t change_time;
    DatasetDBError dberr = DATASETDB_OK;
    u64 nversion;

    // If delete is pending, then the database entries are already gone
    // so there is nothing to do.
    if ( !file->get_is_modified() || file->delete_is_pending() ) {
        goto done;
    }

    change_time = VPLTime_GetTime();
    mini_trans_start("close_file");
    nversion = version + 1;
    if ( file->sync_attrs(attrs) ) {
        dberr = datasetDB.SetComponentPermission(file->component, attrs,
                                                 nversion);
        dberr = db_err_classify(dberr);
        if (dberr != DATASETDB_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "SetComponentPermission(%s, 0x%08x) - %d",
                             file->component.c_str(), attrs, dberr);
            goto fail;
        }
    }

    // Always set the size to force a modified file to have its version
    // number increased.
    file->sync_size(size);
    dberr = datasetDB.SetComponentSize(file->component, size,
                                       nversion, change_time, file->get_new_modify_time());
    dberr = db_err_classify(dberr);
    if (dberr != DATASETDB_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "SetComponentSize(%s, "FMTu64") - %d",
                         file->component.c_str(), size, dberr);
        goto fail;
    }

    file->set_new_modify_time(false);
    file->set_is_modified(false);
fail:
    mini_trans_end("close_file", dberr == DATASETDB_OK);
done:
    return;
}

//
// release_file implements "CleanUpIRP" semantics for Windows,
// which means the descriptor is no longer usable by the application,
// but there may still be IO triggered by the VM system for mapped
// files.  This could include both reads for page misses and writes
// for dirty pages.  The main thing that release_file does is to release
// any byte range locks held by the caller.
//
// close_file means all the mapped pages are gone and no further IO
// is possible. 
//
s16 managed_dataset::release_file(vss_file* file,
                                  vss_object* object,
                                  u64 origin)
{
    s16 rv = VSSI_SUCCESS;
    
    VPLTRACE_LOG_FINE(TRACE_BVS, 0, "release file for {%s}",
                      file->component.c_str());
    VPLMutex_Lock(&access_mutex);
    //
    // Release byte range locks and open modes
    //
    file->release_origin(origin);

    VPLMutex_Unlock(&access_mutex);
    return rv;
}

s16 managed_dataset::close_file(vss_file* file,
                                vss_object* object,
                                u64 origin)
{
    s16 rv = VSSI_SUCCESS;
    std::map<string, vss_file*>::iterator fd_it;

    VPLMutex_Lock(&access_mutex);

    VPLMutex_Lock(&fd_mutex);
    if (!file->validate_origin(origin)) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "origin "FMTu64" does not exist for vss_file %p", origin, file);
        rv = VSSI_INVALID;
        goto done;
    }
    VPLMutex_Unlock(&fd_mutex);

    // flush out attributes and file size
    // update version numbers, notify vsds, etc.
    file_update_database(file);

    VPLMutex_Lock(&fd_mutex);

    // The ref count on the vss_file is protected by the dataset fd_mutex, but
    // the flocks are protected by the vss_file flock_mutex
    file->remove_open_origin(origin);
    file->remove_reference(object, origin);
    if(file->referenced()) {
        goto done;
    }

    //
    // This is the last close
    //
    // Remove from the content filename -> handle map, unless
    // the file has already been deleted
    //
    if(!file->delete_is_pending()) {
        fd_it = open_handles.find(file->filename);
        if(fd_it == open_handles.end()) {
            // No map entry found
            VPLTRACE_LOG_WARN(TRACE_BVS, 0, "closing handle not found for {%s}",
                              file->filename.c_str());
            rv = VSSI_INVALID;
        }
        else {
            VPLTRACE_LOG_FINE(TRACE_BVS, 0, "handle found for {%s}, close it",
                              file->filename.c_str());
            open_handles.erase(file->filename);
        }
    }

    // Remove from the file ID map and destroy the vss_file, which closes
    // the actual VPL FD
    file_id_to_handle.erase(file->get_file_id());
    delete file;

 done:
    VPLMutex_Unlock(&fd_mutex);

    VPLMutex_Unlock(&access_mutex);
    return rv;
}

vss_file* managed_dataset::map_file_id(u32 file_id)
{
    vss_file* handle = NULL;
    std::map<u32, vss_file*>::iterator fh_it;

    VPLMutex_Lock(&access_mutex);
    VPLMutex_Lock(&fd_mutex);

    fh_it = file_id_to_handle.find(file_id);
    if(fh_it == file_id_to_handle.end()) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "file_id %x not in file_id map", file_id);
    }
    else {
        handle = fh_it->second;
    }

    VPLMutex_Unlock(&fd_mutex);
    VPLMutex_Unlock(&access_mutex);

    return handle;
}

void managed_dataset::object_release(vss_object* object)
{
    std::map<string, vss_file*>::iterator fh_it;
    std::map<u32, vss_file*>::iterator fid_it;

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      "object %p file id to handle map size %d", object, file_id_to_handle.size());
    
    VPLMutex_Lock(&access_mutex);
    VPLMutex_Lock(&fd_mutex);

    //
    // There are two maps to search for vss_file structs referenced by the object, but the
    // open_handles map will not show open files that are already deleted.
    //
    fid_it = file_id_to_handle.begin();
    while(fid_it != file_id_to_handle.end()) {
        fid_it->second->remove_object(object);
        // If that was the last reference to the vss_file ...
        if(!fid_it->second->referenced()) {
            if(!fid_it->second->delete_is_pending()) {
                // If not already deleted, there should be an entry in
                // the open_handles map
                fh_it = open_handles.find(fid_it->second->filename);
                if(fh_it != open_handles.end()) {
                    open_handles.erase(fh_it);
                }
            }
            VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                                "free file handle %p fileId %x", fid_it->second, fid_it->second->get_file_id());
            delete fid_it->second;
            file_id_to_handle.erase(fid_it);
            // TODO: would fid_it++ work in this case?
            fid_it = file_id_to_handle.begin();
        }
        else {
            fid_it++;
        }
    }

    VPLMutex_Unlock(&fd_mutex);
    VPLMutex_Unlock(&access_mutex);
}

void managed_dataset::tran_commit(void)
{
    DatasetDBError dbrv;
    u64 size;

    dbrv = datasetDB.TestAndCreateComponent("", 
        DATASETDB_COMPONENT_TYPE_DIRECTORY, 0, version, VPLTime_GetTime());
    dbrv = db_err_classify(dbrv);
    if (dbrv != DATASETDB_OK) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0,
            "Failed to test/create root component in DB. Error %d", dbrv);
        goto fail;
    }

    dbrv = datasetDB.SetDatasetFullVersion(version);
    dbrv = db_err_classify(dbrv);
    if ( dbrv != DATASETDB_OK ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Dataset "FMTu64":"FMTu64": SetDatasetFullVersion(). Error:%d.",
                         id.uid, id.did, dbrv);
        goto fail;
    }

    dbrv = datasetDB.CommitTransaction();
    dbrv = db_err_classify(dbrv);
    if ( dbrv != DATASETDB_OK ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Dataset "FMTu64":"FMTu64": CommitTransaction(). Error:%d.",
                         id.uid, id.did, dbrv);
        goto fail;
    }

    // Report dataset status
    dbrv = datasetDB.GetComponentSize("", size);
    dbrv = db_err_classify(dbrv);
    if ( dbrv != DATASETDB_OK ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Dataset "FMTu64":"FMTu64": GetComponentSize(). Error:%d.",
                         id.uid, id.did, dbrv);
        goto fail;
    }

    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0, "Add dataset event, datasetid "FMTu64", size "FMTu64", version "FMTu64,
                        id.uid, size, version);
    server->add_dataset_stat_update(id, size, version);

    // prevent backups for another N minutes
    last_action = VPLTime_GetTimeStamp();

    // update the reserved space size
    {
        u64 disk_size = 0; // dummy
        int err = get_remaining_space(dir_name, disk_size, remaining_space_size);
        if(err) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed update remaining space %d",
                err);
        }
    }

fail:
    last_commit = VPLTime_GetTimeStamp();
    return;
}

void managed_dataset::tran_rollback(void)
{
    DatasetDBError dbrv;

    dbrv = datasetDB.RollbackTransaction();
    dbrv = db_err_classify(dbrv);
    if ( dbrv != DATASETDB_OK ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Dataset "FMTu64":"FMTu64": RollbackTransaction(). Error:%d.",
                         id.uid, id.did, dbrv);
        goto done;
    }

done:
    last_commit = VPLTime_GetTimeStamp();
    return;
}

void managed_dataset::mark_backup_err(void)
{
    // Indicate we've seen a backup error 
    if ( test_marker(MD_DB_BACKUP_ERR2_MARKER) ) {
        return;
    }

    if ( test_marker(MD_DB_BACKUP_ERR1_MARKER) ) {
        set_marker(MD_DB_BACKUP_ERR2_MARKER);
    }
    else {
        set_marker(MD_DB_BACKUP_ERR1_MARKER);
    }
}

void managed_dataset::mark_fsck_err(void)
{
    // Indicate we've seen an fsck error 
    if ( test_marker(MD_DB_FSCK_ERR2_MARKER) ) {
        return;
    }

    if ( test_marker(MD_DB_FSCK_ERR1_MARKER) ) {
        set_marker(MD_DB_FSCK_ERR2_MARKER);
    }
    else {
        set_marker(MD_DB_FSCK_ERR1_MARKER);
    }
}

void managed_dataset::clear_dbase_err(void)
{
    clear_marker(MD_DB_BACKUP_ERR1_MARKER);
    clear_marker(MD_DB_BACKUP_ERR2_MARKER);
    clear_marker(MD_DB_FSCK_ERR1_MARKER);
    clear_marker(MD_DB_FSCK_ERR2_MARKER);
    clear_marker(MD_DB_OPEN_ERR1_MARKER);
}

void managed_dataset::dbase_backup(void)
{
    DatasetDBError dbrv;
    VPLTime_t end_time;
    bool was_interrupted = false;
    int rv;
    int ec;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "backing up dbase - begin");

    // do not lock this operation as we want it to be interruptable
    // by other write operations.
    dbrv = datasetDB.Backup(was_interrupted, end_time);
    VPLMutex_Lock(&sync_mutex);
    rv = db_err_classify(dbrv);
    if ( rv || invalid ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "datasetDB.backup() - %d", dbrv);
        ec = db_err_to_class(dbrv);
        if ( ec == DB_ERR_CLASS_DATABASE ) {
            mark_backup_err();
        }
        goto done;
    }
    if ( was_interrupted ) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0, "datasetDB.backup() was interrupted");
        goto done;
    }

    // Performed a successful backup so clear any errors.
    clear_marker(MD_DB_BACKUP_ERR1_MARKER);
    clear_marker(MD_DB_BACKUP_ERR2_MARKER);

    // See if the database has changed since the backup completed.
    // possible due to locking windows.
    if ( tran_is_open || (last_commit > end_time) ) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Backup already too old to swap in.");
        goto done;
    }

    // Indicate we no longer need a backup.
    // otherwise something got added before we could swap that wasn't backed
    // up meaning we need another backup.
    dbase_needs_backup = false;
    clear_marker(MD_DB_BACKUP_MARKER);

#ifdef SWAP_IN_BACKUP
    // Swap the backed up version of the database for the real one.
    dbrv = datasetDB.SwapInBackup();
    dbrv = db_err_classify(dbrv);
    if ( dbrv ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "SwapInBackup() - %d", dbrv);
        goto done;
    }
#endif // SWAP_IN_BACKUP

done:
    backup_done = true;
    last_backup = VPLTime_GetTimeStamp();

    VPLCond_Broadcast(&per_sync_cond);
    VPLMutex_Unlock(&sync_mutex);

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "backing up dbase - end");
}


void managed_dataset::stop_ccd(void)
{
    int rv;
    ccd::UpdateSystemStateInput request;
    ccd::UpdateSystemStateOutput response;
    bool shutdown_failed = true;

    request.set_do_shutdown(true);
    rv = CCDIUpdateSystemState(request, response);
    if(rv != CCD_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, 
            "CCDIUpdateSystemState() failed with code %d.", rv);
    }
    else if ( (rv = response.do_shutdown()) != 0 ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, 
            "do_shutdown() failed with code %d.", rv);
    }
    else {
        shutdown_failed = false;
    }
    if ( shutdown_failed ) {
        // Force an exit the hard way
        FAILED_ASSERT("Failed to shutdown CCD!");
#if defined(CLOUDNODE)
        _Exit(1);
#endif // CLOUDNODE
    }
}

static VPLThread_return_t backup_helper(VPLThread_arg_t dataset)
{
    managed_dataset* md = (managed_dataset*)dataset;
    md->dbase_backup();
    return VPLTHREAD_RETURN_VALUE_UNUSED;
}


int managed_dataset::dbase_backup_start(void)
{
    int rv = 0;
    VPLThread_attr_t thread_attr;
    VPLThread_AttrInit(&thread_attr);
    VPLThread_AttrSetStackSize(&thread_attr, MD_WORKER_STACK_SIZE);
    VPLThread_AttrSetDetachState(&thread_attr, false);

    // start up the backup thread
    rv = VPLThread_Create(&backup_thread, backup_helper, VPL_AS_THREAD_FUNC_ARG(this), &thread_attr, "db backup worker");
    if ( rv != VPL_OK ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLThread_Create(db backup) - %d", rv);
        goto done;
    }
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "backup_worker started");

done:
    return rv;
}

static VPLThread_return_t fsck_helper(VPLThread_arg_t dataset)
{
    managed_dataset* md = (managed_dataset*)dataset;
    md->dataset_check();
    return VPLTHREAD_RETURN_VALUE_UNUSED;
}

int managed_dataset::dbase_fsck_start(void)
{
    int rv = 0;
    VPLThread_attr_t thread_attr;
    VPLThread_AttrInit(&thread_attr);
    VPLThread_AttrSetStackSize(&thread_attr, MD_WORKER_STACK_SIZE);
    VPLThread_AttrSetDetachState(&thread_attr, false);

    // start up the periodic sync function
    rv = VPLThread_Create(&fsck_thread, fsck_helper, VPL_AS_THREAD_FUNC_ARG(this), &thread_attr, "db fsck worker");
    if ( rv != VPL_OK ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLThread_Create(db fsck) - %d", rv);
        goto done;
    }
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "fsck_worker started");

done:
    return rv;
}

void managed_dataset::dbase_backup_end(bool force_end)
{
    if ( force_end ) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Stopping backup.");
        datasetDB.BackupCancel();
    }

    // wait for the backup thread to finish
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Reaping backup worker.");
    // Must release this lock so the backup thread can complete and
    // exit.
    VPLMutex_Unlock(&sync_mutex);
    VPLThread_Join(&backup_thread, NULL);
    VPLMutex_Lock(&sync_mutex);

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Backup reaped.");
}

void managed_dataset::dbase_fsck_end(void)
{
    // It should be monitoring the shutdown flag directly and quitting
    // on its own without help from us.
    // wait for the fsck thread to finish
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Reaping fsck worker.");
    VPLMutex_Unlock(&sync_mutex);
    VPLThread_Join(&fsck_thread, NULL);
    VPLMutex_Lock(&sync_mutex);

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Fsck reaped.");
}

void managed_dataset::file_periodic_sync(void)
{
    VPLTime_t delay;
    bool done = false;
    bool dbase_do_backup = false;
    bool backup_in_progress = false;
    bool fsck_in_progress = false;
    bool force_backup = false;

    last_commit = last_action = last_backup = VPLTime_GetTimeStamp();

    dbase_needs_backup = test_marker(MD_DB_BACKUP_MARKER);

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Database backup marker %sseen.",
        dbase_needs_backup ? "" : "not ");

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Ready");

    // If the database needs an fsck, start it going in the background
    if ( fsck_needed && !invalid ) {
        int rc;

        VPLTRACE_LOG_INFO(TRACE_BVS, 0, "fsck of db "FMTu64":"FMTu64" required.",
            id.uid, id.did);
        rc = dbase_fsck_start();
        if ( rc ) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "dbase_fsck_start("FMTu64":"FMTu64") - %d",
                id.uid, id.did, rc);
        }
        else {
            fsck_in_progress = true;
        }
    }

    // The sync thread is mostly idle and asleep. It is woken up by
    // someone requiring a commit of data to be scheduled. On the first commit
    // a database backup is also scheduled for 15 minutes after activity.
    // dies off. This thread also wakes up whenever the database has some
    // kind of fatal error and then deals with starting the recovery process
    // This is usually triggering a restart of CCD. Potentially triggering
    // a restart of the orbe itself. First pass we'll just trigger the restart
    // of CCD and handle recovery activate.

    VPLMutex_Lock(&sync_mutex);
    while (!done) {

        // This thread strictly performs commits, in theory it should also
        // be the one that opens transactions. But it doesn't know enough
        // to do that, so it owns the big flush.
        // It also owns backup of the database, but that's performed by
        // a thread that it owns.
        // Note that database fsck() is performed by a separate thread as well.

        if ( tran_needs_commit ) {
            tran_do_commit = tran_needs_commit;
            delay = VPLTIME_FROM_SEC(MD_SYNC_TIME_SECS);
        }
        else if ( dbase_needs_backup && !fsck_needed ) {
            VPLTime_t cur_time = VPLTime_GetTimeStamp();
            VPLTime_t elapsed_time = cur_time - last_action;

            delay = (elapsed_time < VPLTIME_FROM_SEC(MD_BACKUP_SECS)) ?
                VPLTIME_FROM_SEC(MD_BACKUP_SECS) - elapsed_time :
                VPLTIME_FROM_SEC(1);
        }
        else {
            // enter deep sleep
            delay = VPL_TIMEOUT_NONE;
        }

        // if we need a commit and we don't have do_commit
        // schedule a commit for the near future, otherwise sleep
        // so that we can do the commit when we wakeup.
        if ( !shutdown_sync && !invalid &&
                !(fsck_in_progress && fsck_done) &&
                !(backup_in_progress && backup_done)) {
            VPLCond_TimedWait(&per_sync_cond, &sync_mutex, delay);
        }

        if ( fsck_in_progress && fsck_done ) {
            fsck_in_progress = false;

            // reap the fsck worker.
            dbase_fsck_end();
        }

        if ( backup_in_progress && backup_done ) {
            backup_in_progress = false;

            // reap the backup worker.
            dbase_backup_end(false);

            // If we're successful clear the marker and
            // reset the attempt count
            if ( !invalid && !shutdown_sync ) {
                VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Backup completed.");
            }
            else {
                VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Backup did not complete.");
            }
        }

        // Handle the database going bad
        if ( invalid && !shutdown_sync ) {
            VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Database has gone invalid.");
            
            // If there's an active transaction roll it back.
            if ( tran_needs_commit) {
                tran_rollback();
                tran_needs_commit = false;
            }

            // Tell CCD to exit
            stop_ccd();

            // Loop around and handle shutdown processing.
            shutdown_sync = true;
            continue;
        }

        // if not already scheduled, schedule a backup
        if ( tran_needs_commit && !invalid ) {
            // If no backup is indicated, mark it.
            // Do this even if a backup is in progress to close a window
            // caused by backup being performed in a separate thread.
            if ( !dbase_needs_backup ) {
                dbase_needs_backup = true;
                set_marker(MD_DB_BACKUP_MARKER);
            }
        }

        // Check if we need to force a backup to happen this pass.
        // Even if last_action keeps getting pushed back, this ensures
        // a backup will happen eventually
        VPLTime_t time_since_backup = VPLTime_GetTimeStamp() - last_backup;
        if ( time_since_backup >= VPLTIME_FROM_SEC(MD_FORCE_BACKUP) ){
            VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Forcing backup, last backup was "FMTu64" second ago.", time_since_backup);
            force_backup = true;
        }

        if ( dbase_needs_backup && !backup_in_progress && !fsck_needed ) {
            // see if it's time;
            // Force out any pending transactions before a backup.
            dbase_do_backup = ((VPLTime_GetTimeStamp() - last_action) >=
                    VPLTIME_FROM_SEC(MD_BACKUP_SECS));

            if ( force_backup ){
                dbase_do_backup = true;
                force_backup = false;
            }
            if ( dbase_do_backup ) {
                tran_do_commit = tran_needs_commit;
            }
        }

        // If we're shutting down, force anything pending out now.
        if ( shutdown_sync ) {
            VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Shutdown wakeup.");

            // cancel any running backup
            if ( backup_in_progress ) {
                dbase_backup_end(true);
                backup_in_progress = false;
            }

            // cancel any running fsck
            if ( fsck_in_progress ) {
                dbase_fsck_end();
                fsck_in_progress = false;
            }

            done = true;
            tran_do_commit = tran_needs_commit;
        }

        if ( tran_do_commit && !invalid ) {
            if ( !tran_is_open ) {
                FAILED_ASSERT("Need commit but no open transaction?");
            }

            tran_commit();
            tran_needs_commit = tran_do_commit = tran_is_open = false;
        }

        // we don't perform backups during shutdown because CCD has time     
        // constraints on how long it can take shutting down.
        if ( dbase_do_backup && !shutdown_sync && !invalid && !fsck_needed) {
            int rv;

            backup_done = false;

            rv = dbase_backup_start();
            if ( rv ) {
                // TODO: If this happens we should probably mark the
                // database as invalid so that we can restart
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "dbase_backup_start() - %d", rv);
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Forcing a restart to clear");
                invalid = true;
            }
            else {
                backup_in_progress = true;
                dbase_do_backup = false;
            }
        }
    }
    VPLMutex_Unlock(&sync_mutex);

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Shutting down.");
}

int managed_dataset::get_space(const std::string& path, u64& disk_size, u64& 
avail_size)
{
    // get_space() must not hold off managed dataset db backup or fsck: Bug 9515

    return get_remaining_space(path, disk_size, avail_size);
}

int managed_dataset::get_remaining_space(const std::string& path, u64& disk_size, u64& avail_size)
{
    int rv;

    VPLFS_stat_t statbuf;
    u64 current_db_size = 0, reserved_size = 0;
    std::string dbpath = dir_name + DATASETDB_FILENAME;

    // get_remaining_space() must not hold off managed dataset db backup or fsck: Bug 9515

    if( (rv = VPLFS_Stat(dbpath.c_str(), &statbuf)) == VPL_OK) {
        current_db_size = statbuf.size;
        rv = VPLFS_GetSpace(path.c_str(), &disk_size, &avail_size);
        if(rv == VSSI_SUCCESS) {
            // max(disksize * reserved%, times*dbsize);
            reserved_size = (u64)(disk_size * RESERVED_PERCENT_OF_SPACE);
            if(current_db_size * RESERVED_TIMES_OF_DBSIZE > reserved_size) {
                reserved_size = current_db_size * RESERVED_TIMES_OF_DBSIZE;
            }

            disk_size -= (u64)(disk_size * RESERVED_PERCENT_OF_SPACE);
            if(reserved_size >= avail_size) {
                avail_size = 0;
            } else {
                avail_size -= reserved_size;
            }

            VPLTRACE_LOG_FINE(TRACE_BVS, 0, "Remaining space "FMTu64" Reserved size "FMTu64" Disk size "FMTu64, 
                                             remaining_space_size, reserved_size, disk_size);
        } else {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to get disk size %d", rv);
        }
    } else {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to get db size %d", rv);
    }
    return rv;
}

int managed_dataset::check_access_right(const std::string& path, u32 access_mask) 
{
    return VPL_OK;
}

int managed_dataset::get_sibling_components_count(const std::string& component_in, int& count)
{
    int rv = 0;
    DatasetDBError dberr;
    int components_count = 0;
    count = 0;
    std::string component = normalizePath(component_in);

    if(component.empty()) {
        goto exit;
    }

    dberr = datasetDB.GetSiblingComponentsCount(component, components_count);
    dberr = db_err_classify(dberr);
    if (dberr != DATASETDB_OK) {
        VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                            "Failed to get sibling components count for component %s in DB: Error %d",
                         component.c_str(), dberr);
        rv = VSSI_ACCESS;
    }

    count = components_count;

exit:
    return rv;
}

s16 managed_dataset::get_component_path(const std::string &component, std::string &path)
{
    return VPL_OK;
}

int managed_dataset::db_err_to_class(DatasetDBError dbrv)
{
    int rv;
    
    switch (dbrv) {
    // USER errors are errors in something the client is doing.
    case DATASETDB_OK:
    case DATASETDB_ERR_IS_DIRECTORY:
    case DATASETDB_ERR_NOT_DIRECTORY:
    case DATASETDB_ERR_BAD_DATA:
    case DATASETDB_ERR_BAD_REQUEST:
    case DATASETDB_ERR_UNKNOWN_VERSION:
    case DATASETDB_ERR_UNKNOWN_COMPONENT:
    case DATASETDB_ERR_UNKNOWN_CONTENT:
    case DATASETDB_ERR_UNKNOWN_METADATA:
    case DATASETDB_ERR_UNKNOWN_TRASH:
    case DATASETDB_ERR_WRONG_COMPONENT_TYPE:
    case DATASETDB_ERR_SQLITE_NOTFOUND:
    case DATASETDB_ERR_REACH_COMPONENT_END:
    // not sure about these:
    case DATASETDB_ERR_SQLITE_INTERRUPT:
    case DATASETDB_ERR_SQLITE_PROTOCOL:
    case DATASETDB_ERR_SQLITE_TOOBIG:
    case DATASETDB_ERR_SQLITE_CONSTRAINT:
    case DATASETDB_ERR_SQLITE_MISMATCH:
    case DATASETDB_ERR_SQLITE_NOLFS:
    case DATASETDB_ERR_SQLITE_FORMAT:
    case DATASETDB_ERR_SQLITE_RANGE:
    case DATASETDB_ERR_SQLITE_NOTADB:
        rv = DB_ERR_CLASS_USER;
        break;
    // SWHW indicates something wrong with CCD or physical hardware
    case DATASETDB_ERR_DB_ALREADY_OPEN:
    case DATASETDB_ERR_NOT_OPEN:
    case DATASETDB_ERR_BUSY:
    case DATASETDB_ERR_RESTART:
    case DATASETDB_ERR_SQLITE_ERROR:
    case DATASETDB_ERR_SQLITE_INTERNAL:
    case DATASETDB_ERR_SQLITE_ABORT:
    case DATASETDB_ERR_SQLITE_BUSY:
    case DATASETDB_ERR_SQLITE_LOCKED:
    case DATASETDB_ERR_SQLITE_NOMEM:
    case DATASETDB_ERR_SQLITE_IOERR:
    case DATASETDB_ERR_SQLITE_MISUSE:
        rv = DB_ERR_CLASS_SWHW;
        break;
    // DATABASE indicates issues with the database
    case DATASETDB_ERR_CORRUPTED:
    case DATASETDB_ERR_TABLES_MISSING:
    case DATASETDB_ERR_SQLITE_CORRUPT:
    case DATASETDB_ERR_SQLITE_CANTOPEN:
    case DATASETDB_ERR_SQLITE_EMPTY:
        rv = DB_ERR_CLASS_DATABASE;
        break;
    // SYSTEM indicates issues with the environment, like a read-only db.
    case DATASETDB_ERR_DB_OPEN_FAIL:
    case DATASETDB_ERR_SQLITE_PERM:
    case DATASETDB_ERR_SQLITE_READONLY:
    case DATASETDB_ERR_SQLITE_FULL:
    case DATASETDB_ERR_SQLITE_AUTH:
        rv = DB_ERR_CLASS_SYSTEM;
        break;
    // SCHEMA indicates a version mismatch of some sort
    case DATASETDB_ERR_SQLITE_SCHEMA:
    case DATASETDB_ERR_FUTURE_SCHEMA_VERSION:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Dataset "FMTu64":"FMTu64" schema is a future version. Reinstall latest CCD.",
                         id.uid, id.did);
        rv = DB_ERR_CLASS_SCHEMA;
        break;
    default:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Unexpected error %d", dbrv);
        rv = DB_ERR_CLASS_SWHW;
        break;
    }

    return rv;
}

int managed_dataset::db_err_classify(DatasetDBError dbrv)
{
    int rv = dbrv;
    int rc;
    bool needs_recovery = false;

    // figure out the error class
    rc = db_err_to_class(dbrv);
    switch (rc) {
    case DB_ERR_CLASS_USER:
        break;
    case DB_ERR_CLASS_SCHEMA:
    case DB_ERR_CLASS_SWHW:
    case DB_ERR_CLASS_DATABASE:
    case DB_ERR_CLASS_SYSTEM:
        needs_recovery = true;
        break;
    default:
        // Note that this really "can't happen" since db_err_to_class()
        // returns DB_ERR_CLASS_SWHW in the default case
        FAILED_ASSERT("Unexpected error class %d", rc);
        needs_recovery = true;
        break;
    }

    // If the error type indicates recovery is needed we wake
    // up the sync thread to get it started
    if ( needs_recovery ) {
        // NOTE: This takes advantage of the fact that mutex locks
        // can be held recursively by the same thread. I know, gross.
        VPLMutex_Lock(&sync_mutex);
        if ( !invalid ) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Error %d class %d - marking db bad.",
                             dbrv, rc);
            invalid = true;
            VPLCond_Broadcast(&per_sync_cond);
        }
        VPLMutex_Unlock(&sync_mutex);
    }

    // If the database went invalid we return ACCESS errors from here out
    if ( invalid ) {
        rv = VSSI_ACCESS;
    }

    return rv;
}

void managed_dataset::set_marker(const std::string& path)
{
    std::string marker_path;
    FILE *fp;

    marker_path = dir_name + path;
    if ( (fp = VPLFile_FOpen(marker_path.c_str(), "w")) == NULL ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to create marker %s",
            marker_path.c_str());
        goto done;
    }

    (void)fclose(fp);
done:
    return;
}

bool managed_dataset::test_marker(const std::string& path)
{
    std::string marker_path;
    bool is_present;

    marker_path = dir_name + path;
    is_present = (VPLFile_CheckAccess(marker_path.c_str(),
                                      VPLFILE_CHECKACCESS_READ) == 0);

    return is_present;
}

void managed_dataset::clear_marker(const std::string& path)
{
    std::string marker_path;

    marker_path = dir_name + path;
    (void)VPLFile_Delete(marker_path.c_str());
}

// This function polls last_action time and blocks until the system goes idle for 2 minutes.
// If the system is going to shutdown, condvar causes it to wake up immediately and does not block system shutdown.
void managed_dataset::hold_off_fsck()
{
    VPLMutex_Lock(&sync_mutex);
    VPLTime_t elapsed_time;
    while ( (elapsed_time = VPLTime_GetTimeStamp() - last_action ) < FSCK_DELAY
           && !shutdown_sync
           && !invalid) {
#ifdef FSCK_DEBUG
        VPLTime_t hold_elapsed_time = VPLTime_GetTimeStamp();
        VPLTRACE_LOG_INFO(TRACE_BVS, 0, "We hold off fsck for "FMTu64" secs.", VPLTIME_TO_SEC(FSCK_DELAY - elapsed_time));
#endif // FSCK_DEBUG
        VPLCond_TimedWait(&per_sync_cond, &sync_mutex, FSCK_DELAY - elapsed_time);
#ifdef FSCK_DEBUG
        VPLTRACE_LOG_INFO(TRACE_BVS, 0, "We have held off fsck for "FMTu64" secs.", VPLTIME_TO_SEC(VPLTime_GetTimeStamp() - hold_elapsed_time));
#endif // FSCK_DEBUG
    }
    VPLMutex_Unlock(&sync_mutex);
}
