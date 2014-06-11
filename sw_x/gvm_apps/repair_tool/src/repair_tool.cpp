/*
 *  Copyright 2013 Acer Cloud Technology Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF 
 *  ACER CLOUD TECHNOLOGY INC.
 *
 */

#define _XOPEN_SOURCE 600

#include "managed_dataset.hpp"
#include "DatasetDB.hpp"
#include "utf8.hpp"
#include "vplex_assert.h"
#include "vplex_vs_directory.h"

#include <stdlib.h>
#include <string>

#include <iomanip>
#include <stdio.h>
#include <fcntl.h>

#include <setjmp.h>
#include <signal.h>
#include <getopt.h>
#include <unistd.h>
#include <utime.h>

#include <iostream>
#include <sstream>
#include <map>
#include <set>
#include <stack>

#include "vpl_conv.h"
#include "vpl_fs.h"
#include "vplex_file.h"
#include "vplex_trace.h"

using namespace std;

typedef struct {
    int         files_present;
    int         files_lost_and_found;
    int         db_entries_resized;
    int         db_entries_deleted;
    int         component_entries_deleted;
    int         missing_parents;
    int         errors;
    VPLTime_t   ds_repair_time;
} DatasetCheckStats;

// Basic directories for all save states.
static const std::string base_dirs[] = {"data",       // user data
                                        "changes",    // change logs
                                        "references", // reference files
                                        "db",         // database
                                        ""/*terminator*/};
enum {
    DATA = 0,
    CHANGES,
    REFERENCES,
    DB,
};
#define BASE_DIR(i) (base_dirs[(i)])

// Remember to update clear_all_markers if any new markers are added
#define MAX_BACKUP_COPY     1
#define MD_DB_BACKUP_MARKER         "db-needs-backup"
#define MD_LOST_AND_FOUND_SYSTEM    "lost-and-found-system"

static std::string storage_base = "/home/user0/.ccd/0/cc/sn/";

// No need to specify uid or did, just go through them all
// But definitely do have a help message explaining what is going on!

static void usage(int argc, char* argv[])
{
    // Dump original command
    // Print usage message
    printf("------------------------\n");
    printf("Storage Node Repair Tool\n");
    printf("------------------------\n");
    printf("Usage: %s [options]\n", argv[0]);
    printf("Options:\n");
    printf(" -s --storage_base <storage base> Base directory for datasets. Include trailing '/' Default is /home/user0/.ccd/0/cc/sn/\n");
    printf(" -h --help                        Print this message.\n");

    printf("\n\nThis tool repairs the Orbe's filesystem.\n");
    printf("It scans every users' dataset and checks against its respective database for accuracy.\n");
    printf("It will:\n");
    printf(" - delete any file not in db\n");
    printf(" - correct any wrongly sized files in db\n");
    printf(" - remove from the db any file missing on device\n");
    printf(" - delete all file-type components without a content entry\n");

    printf("For addition information, please go to:\n");
    printf(" http://www.ctbg.acer.com/wiki/index.php/Dataset_Fsck_Tool_Usage\n");
}

static int parse_args(int argc, char* argv[])
{
    int rv = 0;

    static struct option long_options[] = {
        {"storage_base", required_argument, 0, 's'},
        {"help", no_argument, 0, 'h'},
        {NULL,0,0,0}
    };

    for(;;) {
        int option_index = 0;
        
        int c = getopt_long(argc, argv, "s:h", 
                            long_options, &option_index);

        if (c == -1)
            break;

        switch (c) {
        case 's': 
            storage_base = optarg;
            break;
        case 'h': 
            usage(argc, argv); 
            rv = -1;
            break;
        default:
            break;
        }
    }

    return rv;
}

static void set_marker(const std::string& path)
{
    FILE *fp;

    if ( (fp = VPLFile_FOpen(path.c_str(), "w")) == NULL ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to create marker %s",
            path.c_str());
        goto done;
    }

    (void)fclose(fp);
done:
    return;
}

static bool test_marker(const std::string& path)
{
    bool is_present;

    is_present = (VPLFile_CheckAccess(path.c_str(), VPLFILE_CHECKACCESS_EXISTS) == 0);
    return is_present;
}

static void clear_marker(const std::string& path)
{
    (void)VPLFile_Delete(path.c_str());
}

static void clear_all_markers(const std::string& base_path)
{
    string path;
    const char *markers[] = {
        "no-db",
        "db-needs-backup",
        "db-needs-fsck",
        "db-is-open",
        "db-backup-fail-1",
        "db-backup-fail-2",
        "db-fsck-fail-1",
        "db-fsck-fail-2",
        "db-open-fail-1",
        "lost-and-found-system",
    };

    for (int i = 0; i < (sizeof(markers) / sizeof(markers[0])); i++) {
        path = base_path + markers[i];
        clear_marker(path);
    }
}

// This function tests a directory is a valid dataset or not. A directory is considered
// a valid dataset if there's a db or a back-up db. Otherwise, there's really nothing to
// do anyways
static int is_valid_dataset(const std::string& path)
{
    bool is_valid = false;
    string db_path = path + BASE_DIR(DB);
    string backup_db_path;

    if (VPLFile_CheckAccess(db_path.c_str(), VPLFILE_CHECKACCESS_EXISTS) != VPL_OK) {
        for (int i = 1; i <= MAX_BACKUP_COPY; i++) {
            std::stringstream name_tmp;

            name_tmp << i;
            backup_db_path = path + BASE_DIR(DB) + "-" + name_tmp.str();
            if (VPLFile_CheckAccess(backup_db_path.c_str(), VPLFILE_CHECKACCESS_EXISTS) == VPL_OK) {
                is_valid = true;
                break;
            }
        }
    } else {
        is_valid = true;
    }

    return is_valid;
}

static int db_err_to_class(DatasetDBError dbrv)
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
        rv = DB_ERR_CLASS_SCHEMA;
        break;
    default:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Unexpected error %d", dbrv);
        rv = DB_ERR_CLASS_SWHW;
        break;
    }

    return rv;
}

static int db_err_classify(DatasetDBError dbrv, bool &needs_recovery)
{
    int rv = dbrv;
    int rc;

    needs_recovery = false;

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
        needs_recovery = true;
        break;
    }

    return rv;
}

static int dataset_check(string uid, string did, bool force_create_db, bool lost_and_found_system, DatasetCheckStats &check_stats)
{
    int rv = 0;
    int rc;

    vector<string> paths;
    string current;
    string disk_path;

    DatasetDB datasetDB;
    DatasetDBError dbrc;

    ContentAttrs content_attr;
    VPLFS_stat_t stats;

    u64 component_check_offset = 0;
    u64 version;
    int content_file_lost_and_found_mod_cnt;
    VPLTime_t content_file_lost_and_found_dir_time = 0;
    VPLTime_t content_file_lost_and_found_time;
    u64 num_missing_parents = 0;
    VPLTime_t component_check_dir_time = 0;

    bool dir_open = false;
    bool db_open = false;
    bool needs_recovery;

    string dir_name = storage_base + uid + "/" + did + "/";

    string data_dir = dir_name + BASE_DIR(DATA) + "/";
    string db_path = dir_name + BASE_DIR(DB);

    VPLFS_dir_t directory;
    VPLFS_dirent_t entry;

    VPLTime_t start = VPLTime_GetTimeStamp();
    VPLTime_t dur;

    memset(&check_stats, 0, sizeof(check_stats));

    // If force_create_db is not set, then we return error if the db file
    // doesn't exist

    if (!force_create_db) {
        rc = VPLFile_CheckAccess(db_path.c_str(), VPLFILE_CHECKACCESS_EXISTS);
        if (rc != VPL_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Database file does not exist %s", db_path.c_str());
            check_stats.errors++;
            rv = -1;
            goto exit;
        }
    }

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Opening/Creating database...");
    
    dbrc = datasetDB.OpenDB(db_path);
    if (dbrc != DATASETDB_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to open/create database");
        check_stats.errors++;
        rv = -1;
        goto exit;
    } else {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Opened/Created database");
        db_open = true;
    }

    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      "Dataset %s:%s: Test and create lost+found directory",
                      uid.c_str(), did.c_str());

    dbrc = datasetDB.GetDatasetFullVersion(version);
    if (dbrc != DATASETDB_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Dataset %s:%s: Failed to get version. Error: %d.",
                         uid.c_str(), did.c_str(), dbrc);
        check_stats.errors++;
        rv = -1;
        goto exit;
    }

    dbrc = datasetDB.TestAndCreateComponent("", DATASETDB_COMPONENT_TYPE_DIRECTORY, 0, version, VPLTime_GetTime());
    if(dbrc != DATASETDB_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Dataset %s:%s: Failed to test/create root directory in DB. Error: %d.",
                         uid.c_str(), did.c_str(), dbrc);
        check_stats.errors++;
        rv = -1;
        goto exit;
    }

    dbrc = datasetDB.TestAndCreateLostAndFound(version, lost_and_found_system, VPLTime_GetTime());
    if(dbrc != DATASETDB_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Dataset %s:%s: Failed to test/create lost+found in DB. Error: %d.",
                         uid.c_str(), did.c_str(), dbrc);
        check_stats.errors++;
        rv = -1;
        goto exit;
    }

    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      "Dataset %s:%s: Walking content tree.",
                      uid.c_str(), did.c_str());

    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      "  database                       filesystem");
    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      "   entry:                         content:   result:");

    // For non-database errors, we would just increment the check_stats.errors count.
    // For database errors, we would try to classify it. For certain non-recoverable
    // errors, we would return -1 so the caller can attempt a restore operation.
    // Otherwise, we would just increment the check_stats.errors count

    // Check filesystem for consistency
    // Walk the files system and delete any files not present in the database. Also update files size in database if has changes on the filesystem.
    paths.push_back("");
    while(!paths.empty()) {
        current = paths.back();
        paths.pop_back();
        string path = data_dir + current;

        rc = VPLFS_Opendir(path.c_str(), &directory);
        if(rc != VPL_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Open directory {%s} failed. Error: %d.",
                              path.c_str(), rc);
            check_stats.errors++;
            continue;
        }
        dir_open = true;

        do {
            rc = VPLFS_Readdir(&directory, &entry);
            if(rc == VPL_ERR_MAX) {
                break;
            }
            if(rc != VPL_OK) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "Read directory {%s} failed. Error: %d.",
                                  path.c_str(), rc);
                check_stats.errors++;
                break;
            }

            // Ignore "." and ".."
            if(entry.filename[0] == '.' &&
               (entry.filename[1] == '\0' || (entry.filename[1] == '.' &&
                                              entry.filename[2] == '\0'))) {
                continue;
            }

            string new_path = current.empty() ? entry.filename : current + "/" + entry.filename;
            
            switch(entry.type) {
            case VPLFS_TYPE_FILE: {
                string content_path;

                // For each file, check if there's a matching content file entry.
                disk_path = path + "/" + entry.filename;
                
                rc = VPLFS_Stat(disk_path.c_str(), &stats);
                if(rc != VPL_OK) {
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                     "Stat content {%s} failed. Error: %d.",
                                      new_path.c_str(), rc);
                    check_stats.errors++;
                    break;
                }
                content_path = disk_path.substr(data_dir.length());
                
                dbrc = datasetDB.GetContentByLocation(new_path, content_attr);
                dbrc = db_err_classify(dbrc, needs_recovery);
                if(dbrc == DATASETDB_ERR_UNKNOWN_CONTENT) {
                    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                        "[  absent  ]<-------------------[%*s%s] Moving content to lost+found", 10 - new_path.length(), " ", new_path.c_str());

                    // Move the content to "lost+found"
                    // The dir_time and count are needed to construct the directory path
                    // The version and time are needed to set the component fields
                    content_file_lost_and_found_mod_cnt = check_stats.files_lost_and_found % MAXIMUM_COMPONENTS;
                    if (content_file_lost_and_found_mod_cnt == 0) {
                        // The dir_time is used for setting the directory names of
                        // "MM-DD-YYYY" and "unreferenced-files-<timestamp>"
                        content_file_lost_and_found_dir_time = VPLTime_GetTime();
                    }
                    content_file_lost_and_found_time = VPLTime_GetTime();
                    dbrc = datasetDB.MoveContentToLostAndFound(content_path,
                                                               content_file_lost_and_found_mod_cnt,
                                                               content_file_lost_and_found_dir_time,
                                                               stats.size,
                                                               version,
                                                               content_file_lost_and_found_time);
                    dbrc = db_err_classify(dbrc, needs_recovery);
                    if(dbrc != DATASETDB_OK) {
                        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                         "Dataset %s:%s: Move Content {%s} to lost+found failed: %d.",
                                         uid.c_str(), did.c_str(), disk_path.c_str(), dbrc);
                        check_stats.errors++;
                        if (needs_recovery) {
                            rv = -1;
                            goto exit;
                        }
                    } else {
                        check_stats.files_lost_and_found++;
                    }
                }
                else if(dbrc == DATASETDB_OK) {
                    // File found, update size in db if necessary 
                    check_stats.files_present++;

                    if(stats.size != content_attr.size) {
                        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                                         "[%s%*s]<------------------>[%*s%s] updating size in database",
                                          new_path.c_str(), 10 - new_path.length(), " ", 10 - new_path.length(), " ", new_path.c_str());

                        dbrc = datasetDB.SetContentSizeByLocation(new_path, stats.size);
                        dbrc = db_err_classify(dbrc, needs_recovery);
                        if(dbrc != DATASETDB_OK) {
                            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                             "Set content {%s} size failed. Error: %d.",
                                              new_path.c_str(), dbrc);
                            check_stats.errors++;
                            if (needs_recovery) {
                                rv = -1;
                                goto exit;
                            }
                        } else {
                            check_stats.db_entries_resized++;
                        }
                    }
                }
                else {
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                     "Check content {%s} exists failed. Error: %d.",
                                      new_path.c_str(), dbrc);
                    check_stats.errors++;
                    if (needs_recovery) {
                        rv = -1;
                        goto exit;
                    }
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
        } while(rc == VPL_OK);

        VPLFS_Closedir(&directory);
        dir_open = false;
    }

    // Remove content and component records where the content-file is missing.
    content_attr.compid = 0; // begin from the beginning
    do {
        dbrc = datasetDB.GetNextContent(content_attr);
        dbrc = db_err_classify(dbrc, needs_recovery);
        if(dbrc == DATASETDB_ERR_UNKNOWN_CONTENT) {
            // Reached end
            break;
        }
        else if(dbrc != DATASETDB_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Get next content from content ID "FMTu64" failed. Error: %d.",
                              content_attr.compid, dbrc);
            check_stats.errors++;
            if (needs_recovery) {
                rv = -1;
                goto exit;
            }
        }
        else {
            // For each content entry, check that its file is found on filesystem. Remove if it does not.
            disk_path = data_dir + content_attr.location;

            rc = VPLFS_Stat(disk_path.c_str(), &stats);
            switch(rc) {
            case VPL_ERR_NOENT:
            case VPL_ERR_NOTDIR:
                VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                    "[%s%*s]------------------->[  absent  ] deleting database entry", content_attr.location.c_str(), 10 - content_attr.location.length(), " ");

                dbrc = datasetDB.DeleteContentComponentByLocation(content_attr.location);
                dbrc = db_err_classify(dbrc, needs_recovery);
                if(dbrc != DATASETDB_OK) {
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                     "Delete content {%s} failed. Error: %d.",
                                      content_attr.location.c_str(), dbrc);
                    check_stats.errors++;
                    if (needs_recovery) {
                        rv = -1;
                        goto exit;
                    }
                } else {
                    check_stats.db_entries_deleted++;
                }
                break;
            default:
                if ( rc != 0 ) {
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                     "Stat {%s} failed. Error: %d.",
                                      disk_path.c_str(), rc);
                    check_stats.errors++;
                } else {
                    //VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Content found.");
                }
                break;
            }   
        }
    } while(dbrc == DATASETDB_OK);

    // Delete file component entries without a corresponding entry, and move
    // component entries without a valid parent to "lost+found"
    component_check_offset = 0;
    num_missing_parents = 0;
    do {
        u64 num_deleted_this_round = 0;

        dbrc = datasetDB.CheckComponentConsistency(component_check_offset,
                                                   num_deleted_this_round,
                                                   num_missing_parents,
                                                   component_check_dir_time,
                                                   MAXIMUM_COMPONENTS,
                                                   version);
        check_stats.component_entries_deleted += num_deleted_this_round;
    } while(dbrc == DATASETDB_OK);
    check_stats.missing_parents = num_missing_parents;

    dbrc = db_err_classify(dbrc, needs_recovery);
    if(dbrc != DATASETDB_ERR_REACH_COMPONENT_END) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Dataset %s:%s: DeleteComponentFilesWithoutContent failed. Error: %d.",
                         uid.c_str(), did.c_str(), dbrc);
        check_stats.errors++;
        if (needs_recovery) {
            rv = -1;
            goto exit;
        }
    }

    if ( (check_stats.files_lost_and_found + check_stats.db_entries_resized + check_stats.db_entries_deleted + check_stats.component_entries_deleted + check_stats.missing_parents) == 0) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                    "[   good   ]<------------------>[   good   ] ok - dataset had no inconsistencies ");
    }

exit:
    if(dir_open) {
        VPLFS_Closedir(&directory);
        dir_open = false;
    }
    if(db_open) {
        dbrc = datasetDB.CloseDB();
        db_open = false;
    }

    dur = VPLTime_DiffClamp(VPLTime_GetTimeStamp(), start);
    check_stats.ds_repair_time = dur;

    return rv;
}

static int dataset_repair(string uid, string did)
{
    int rv = 0;
    DatasetDBError dbrc;
    DatasetDB datasetDB;
    DatasetCheckStats check_stats;
    string base_path, db_path, backup_needed_path, lost_and_found_system_path;
    bool force_create_db = false, restore_ok = true, had_backup = false;
    bool lost_and_found_system = false;

    // Since we are repairing the dataset, it would make sense to clear all markers
    // and re-start everything all over again. Before cleaning though, get the
    // state of the system "lost+found" directory
    base_path = storage_base + uid + "/" + did + "/";
    lost_and_found_system_path = base_path + MD_LOST_AND_FOUND_SYSTEM;
    lost_and_found_system = test_marker(lost_and_found_system_path);
    clear_all_markers(base_path);

    do {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Performing dataset check for uid:%s, did:%s",
                          uid.c_str(), did.c_str());

        rv = dataset_check(uid, did, force_create_db, lost_and_found_system, check_stats);
        if (rv != 0) {
            VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Dataset check failed for uid:%s, did:%s, rv=%d",
                              uid.c_str(), did.c_str(), rv);

            if (restore_ok) {
                VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Restoring back-up database");

                db_path = base_path + BASE_DIR(DB);
                dbrc = datasetDB.Restore(db_path, had_backup);
                if (dbrc != DATASETDB_OK) {
                    // This really shouldn't happen
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Database restore failed %s, rv=%d\n",
                                     db_path.c_str(), dbrc);
                    rv = -1;
                    goto exit;
                }

                if (!had_backup) {
                    // In this case, no back-up restore has happened, but the offending
                    // db should have been deleted. Re-try, with force_create_db set to
                    // true, but restore_ok set to false
                    force_create_db = true;
                    restore_ok = false;
                    continue;
                } else {
                    // Back-up has been restored, just re-try
                    continue;
                }
            } else {
                // Exhausted all tries
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Exhausted all restore tries %s\n",
                                 db_path.c_str());
                rv = -1;
                goto exit;
            }
        } else {
            // Successfully completed dataset_check()
            break;
        }
    } while (true);

    if (rv == 0) {
        // A bit hard to tell whether a back-up is needed after this point. The
        // safest way is to schedule one regardless
        backup_needed_path = base_path + MD_DB_BACKUP_MARKER;
        set_marker(backup_needed_path);

        // By this point, the "lost+found" directory should have been set up
        set_marker(lost_and_found_system_path);
    }

exit:
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Repair %s.", (rv != 0) ? "failed" : "complete");
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Files present:             %d", check_stats.files_present);
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Files lost+found:        %d", check_stats.files_lost_and_found);
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Content entries resized:  %d", check_stats.db_entries_resized);
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Database entries deleted:  %d", check_stats.db_entries_deleted);
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Component entries deleted: %d", check_stats.component_entries_deleted);
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Missing parents:           %d", check_stats.missing_parents);
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Errors:                    %d", check_stats.errors); 
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Dataset repair time:       "FMTu64" seconds\n", VPLTime_ToSec(check_stats.ds_repair_time));

    return rv;
}

static int repair_storage_node(int &nSuccess, int &nFail, int &nSkip)
{
    int rv = 0;
    bool is_base_open = false, is_uid_open = false;
    string uid_dir_path, did_dir_path;
    VPLFS_dir_t base_dir;
    VPLFS_dirent_t base_dir_entry;
    VPLFS_dir_t uid_dir;
    VPLFS_dirent_t uid_dir_entry;

    nSuccess = 0;
    nFail = 0;
    nSkip = 0;

    // Below the storage base, the db files are hidden under two directory levels
    // (i.e. /uid/did/)

    rv = VPLFS_Opendir(storage_base.c_str(), &base_dir);
    if (rv != VPL_OK) {
        // Not much to do if can't read base directory
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Open %s failed", storage_base.c_str());
        goto exit;
    }
    is_base_open = true;

    do {
        rv = VPLFS_Readdir(&base_dir, &base_dir_entry);
        if (rv == VPL_ERR_MAX) {
            rv = 0;
            break;
        }
        if (rv != VPL_OK) {
            // Not a normal case, break out of the loop
            rv = 0;
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Read %s failed", storage_base.c_str());
            break;
        }

        // Ignore "." and ".."
        if ((base_dir_entry.filename[0] == '.') &&
            ((base_dir_entry.filename[1] == '\0') || ((base_dir_entry.filename[1] == '.') &&
                                                      (base_dir_entry.filename[2] == '\0')))) {
            continue;
        }

        uid_dir_path = storage_base + base_dir_entry.filename + "/";
        rv = VPLFS_Opendir(uid_dir_path.c_str(), &uid_dir);
        if (rv != VPL_OK) {
            // Not a normal case, but let's just ignore the error and continue
            rv = 0;
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Open %s failed", uid_dir_path.c_str());
            continue;
        }
        is_uid_open = true;

        do {
            rv = VPLFS_Readdir(&uid_dir, &uid_dir_entry);
            if (rv == VPL_ERR_MAX) {
                rv = 0;
                break;
            }
            if (rv != VPL_OK) {
                // Not a normal case, break out of the loop
                rv = 0;
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Read %s failed", uid_dir_path.c_str());
                break;
            }

            // Ignore "." and ".."
            if ((uid_dir_entry.filename[0] == '.') &&
                ((uid_dir_entry.filename[1] == '\0') || ((uid_dir_entry.filename[1] == '.') &&
                                                         (uid_dir_entry.filename[2] == '\0')))) {
                continue;
            }

            // Skip any junk directory that may happen to be there
            did_dir_path = uid_dir_path + uid_dir_entry.filename + "/";
            if (!is_valid_dataset(did_dir_path)) {
                VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Skipping %s as there are no db files",
                                  did_dir_path.c_str());
                nSkip++;
                continue;
            }

            rv = dataset_repair(base_dir_entry.filename, uid_dir_entry.filename);
            if (rv != 0) {
                // Record the error, but continue anyways
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Dataset repair failed for uid:%s, did:%s, rv=%d",
                                 base_dir_entry.filename, uid_dir_entry.filename, rv);
                nFail++;
                rv = 0;
                continue;
            }

            nSuccess++;
        } while (1);

        VPLFS_Closedir(&uid_dir);
        is_uid_open = false;
    } while (1); 

    VPLFS_Closedir(&base_dir);
    is_base_open = false;

exit:
    if (is_base_open) {
        VPLFS_Closedir(&base_dir);
    }
    if (is_uid_open) {
        VPLFS_Closedir(&uid_dir);
    }

    return rv;
}

int main(int argc, char *argv[]) {

    int rv = 0;
    int nSuccess, nFail, nSkip;
    
    if (parse_args(argc, argv) != 0){
        return 0;
    }

    VPLTrace_Init(NULL);
    VPLTrace_SetShowTimeAbs(true);
    VPLTrace_SetShowThread(false);
    VPLTrace_SetShowFunction(false);

    printf("------------------------\n");
    printf("Storage Node Repair Tool\n");
    printf("------------------------\n");

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Storage base is %s", storage_base.c_str());
    rv = repair_storage_node(nSuccess, nFail, nSkip);
    if (rv != 0) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Unable to repair storage node, rv=%d", rv);
    } else {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Success: %d datasets, Failed: %d datasets, Skipped: %d directories", nSuccess, nFail, nSkip);
    }
	
    return 0;
}

