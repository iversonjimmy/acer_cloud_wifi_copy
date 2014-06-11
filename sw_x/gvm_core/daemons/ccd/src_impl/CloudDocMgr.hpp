/*
 *  Copyright 2012 Acer Cloud Technology, Inc.
 *  All Rights Reserved.
 *
 *  This software contains confidential information and
 *  trade secrets of Acer Cloud Technology, Inc.
 *  Use, disclosure or reproduction is prohibited without
 *  the prior express written permission of Acer Cloud
 *  Technology, Inc.
 */

/*
 *  Copyright 2011 iGware Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF IGWARE INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
 *
 */

#ifndef CLOUDDOC_MGR_HPP_
#define CLOUDDOC_MGR_HPP_

#include <vplu_types.h>

#include <sqlite3.h>

#include <vector>
#include <string>

#include "vplex_file.h"
#include "vpl_th.h"
#include <vplex_http2.hpp>
#include <log.h>

#include "ccd_features.h"
#include "vssi_types.h"
#include "vssi.h"

#include "ccdi_rpc.pb.h"

#ifdef __cplusplus

// Forward declaration
class DocSNGTicket;

// Start SyncDown engine to start.
// Note that part of the actual work is done asynchronously; 
// thus, successful return does not necessarily mean the engine has actually started.
int DocSyncDown_Start(u64 userId);

// Stop SyncDown engine.
// If "purge" is true, all outstanding jobs will be removed.
// Note that most of the actual work is done asynchronously;
// thus, successful return does not necessarily mean the engine has actually stopped.
int DocSyncDown_Stop(bool purge);

// Notify dataset content change.  Download only happens when ans is available,
// unless, overrideAnsCheck is true.
int DocSyncDown_NotifyDatasetContentChange(u64 datasetid, bool overrideAnsCheck);

// Reset the last-checked version number of the given dataset.
int DocSyncDown_ResetDatasetLastCheckedVersion(u64 datasetid);

// Remove all jobs for the given dataset.
int DocSyncDown_RemoveJobsByDataset(u64 datasetId);

int DocSyncDown_QueryStatus(u64 userId,
                         u64 datasetId,
                         const std::string& path,
                         ccd::FeatureSyncStateSummary& syncState_out);

int DocSyncDown_QueryStatus(u64 userId,
                         ccd::SyncFeature_t syncFeature,
                         ccd::FeatureSyncStateSummary& syncState_out);

int CountUnfinishedJobsinDB(std::string name, u64 compId, u32 &count);

int DocSNGQueue_NewRequest(u64 uid,
                           u64 did,
                           const char *dloc,
                           int change_type,
                           const char *docname,
                           const char *name,
                           const char *newname,
                           VPLTime_t modtime,
                           const char *contentType,
                           u64 compid, u64 baserev,
                           const char *thumbpath,
                           const char *thumbdata, u32 thumbsize,
                           u64 *requestid);
int DocSNGQueue_VisitTickets(int (*visitor)(void *ctx,
                                            int change_type,
                                            int in_progress,
                                            const char *name,
                                            const char *newname,
                                            u64 modify_time,
                                            u64 upload_size,
                                            u64 upload_progress),
                             void *ctx);
int DocSNGQueue_EngineIsRunning();
int DocSNGQueue_Enable(u64 userId);
int DocSNGQueue_Disable(int purge);
int DocSNGQueue_Init(const char *rootdir,
                     int (*completionCb)(int change_type,
                                         const char *name,
                                         const char *newname,
                                         u64 modify_time,
                                         int result,
                                         const char *docname,
                                         u64 comp_id,
                                         u64 revision),
                     int (*engineStateChangeCb)(bool engine_started));
int DocSNGQueue_Release();
int DocSNGQueue_SetVcsHandlerFunction(VPLDetachableThread_fn_t handler);
int DocSNGQueue_GetRequestListInJson(char **json_out);
int DocSNGQueue_GetRequestStatusInJson(u64 requestId, char **json_out);
int DocSNGQueue_CancelRequest(u64 requestId);

class DocSNGQueue {
public:
    DocSNGQueue();
    ~DocSNGQueue();

    int init(const std::string &rootdir,
             int (*completionCb)(int change_type,
                                 const char *name,
                                 const char *newname,
                                 u64 modify_time,
                                 int result,
                                 const char *docname,
                                 u64 comp_id,
                                 u64 revision),
             int (*engineStateChangeCb)(bool engine_started));
    int release();

    int setVcsHandlerFunction(VPLDetachableThread_fn_t handler);

    /// Enqueue new request for processing.
    /// If running, queue task to actively process tickets.
    int enqueue(u64 uid,
                u64 did,
                const char *dloc,
                int change_type,
                const char *docname,
                const char *name,
                const char *newname,
                u64 modtime,
                const char *contentType,
                u64 compid, u64 baserev,
                const char *thumbpath,
                const char *thumbdata, u32 thumbsize,
                u64 *requestid);

    /// Dequeue completed ticket (called by ticket destructor)
    int dequeue(DocSNGTicket* tp);

    /// Remove all requests from the db.
    int removeAllRequests();
    /// Remove all tmp files.
    int removeAllTmpFiles();

    /// Reset failed ticket. This causes the ticket to be retained in the queue.
    int resetTicket(DocSNGTicket* tp);

    /// Complete the ticket. This causes the ticket to be removed from the queue.
    int completeTicket(DocSNGTicket* tp);

    /// Return head-most not-in-progress ticket and mark it in-progress.
    /// Return NULL if no not-in-progress tickets in queue.
    DocSNGTicket* activateNextTicket();

    /// Enable ticket processing. Queue task to actively process tickets.
    int startProcessing(u64 userId);

    /// Disable ticket processing. Any active processing will stop.
    int stopProcessing(bool purge);

    /// Check to see if tickets are actively being processed.
    bool isProcessing();

    /// Check to see if processing is enabled.
    bool isEnabled();

    int visitTickets(int (*visitor)(void *ctx,
                                    int change_type,
                                    int in_progress,
                                    const char *name,
                                    const char *newname,
                                    u64 modify_time,
                                    u64 upload_size,
                                    u64 upload_progress),
                     void *ctx);

    bool isAvailable(DocSNGTicket* tp); //check the tickets before activate
    int checkTicketsAfterEnqueue(); //check the tickets after enqueue

    int getRequestListInJson(char **json_out);
    int getRequestStatusInJson(u64 requestId, char **json_out);
    int cancelRequest(u64 requestId);

private:
    VPLMutex_t m_mutex;
    std::vector<sqlite3_stmt*> dbstmts;
    bool isRunning; /// Able to process queued tickets
    bool isActive;  /// Actively processing one or more queued tickets
    std::string rootdir;  // root dir for DocSNG
    std::string tmpdir;   // dir to hold temp files
    std::string dbpath;  // path to sqlite file
    sqlite3 *db;  // handle to db
    DocSNGTicket *activeTicket;
    int (*completionCb)(int change_type,
                        const char *name,
                        const char *newname,
                        u64 modify_time,
                        int result,
                        const char *docname,
                        u64 comp_id,
                        u64 revision);
    int (*engineStateChangeCb)(bool engine_started);
    u64 currentUserId;
    bool isReady;
    VPLDetachableThread_fn_t vcs_handler;

    int openDB();
    int migrateDB();
    int updateSchema();
    int getVersion(int versionId, u64 &version);
    int closeDB();

    int beginTransaction();
    int commitTransaction();
    int rollbackTransaction();
#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
    int writeComponentToDB(DocSNGTicket *tp);
    int writeRevisionToDB(DocSNGTicket *tp);
    int deleteComponentFromDB(DocSNGTicket *tp);
    int deleteRevisionFromDB(DocSNGTicket *tp);
    int getCompid_by_name(DocSNGTicket *tp);
    std::string calculateRowChecksum(std::string input);
    int getHash_by_compid(u64 compid, std::string &hashval);
#endif // CCD_ENABLE_SYNCDOWN_CLOUDDOC
};


typedef enum {
    DOC_SNG_UPLOAD = 1,
    DOC_SNG_RENAME = 2,
    DOC_SNG_DELETE = 3,
} DocSNGChangeType;


class DocSNGTicket {
public:
    // Back-pointer to container of this ticket
    DocSNGQueue* parentQ;

    // ticket id in queue
    u64 ticket_id;

    // Dataset to change
    u64 uid;              // user id
    u64 did;              // dataset id
    std::string dloc;     // [v1.1 ONLY] dataset-location
    u64 compid;           // [v1.1 ONLY] component id: 0 means not set
    u64 baserev;          // [v1.1 ONLY] base revision: 0 is a valid value
    bool baserev_valid;   // [v1.1 ONLY] whether value in baserev is valid or not
    std::string docname;  // [v1.1 ONLY] docname = <origin-devid>/<abs-path-on-origin-dev>
    std::string contentType; // [v1.1 ONLY] MIME content-type: "" means not set

#if CCD_ENABLE_SYNCDOWN_CLOUDDOC
    // Information for DocTrackerDB
    u64 origin_device;
    u64 size;
    u64 update_device;
    //char hashval[40];
    std::string hashval;
#endif // CCD_ENABLE_SYNCDOWN_CLOUDDOC

    // Operation details
    DocSNGChangeType operation;
    std::string srcPath;   // Upload: File data location (temp file)
    std::string thumbPath; // Upload: Thumbnail data location (temp file)
    std::string origName;  // All: Original source file name (includes path)
    std::string newName;   // Rename: new name (includes path)
    u64 modifyTime; 

    // Runtime progress
    bool inProgress; // Actively working on this ticket
    u64 uploadFileSize; // Upload: File size (bytes)
    u64 uploadFileProgress; // Upload: File uploaded so far (bytes)
    u64 uploadThumbSize; // Upload: Thumbnail size (bytes)
    u64 uploadThumbProgress; // Upload: Thumbnail uploaded so far (bytes)

    // Runtime state
    VPLFile_handle_t srcFile; // Upload: Open file handle for uploading data
    std::string filename; // Current destination name.
    bool out_of_date;

    // End result
    int result;

    // 
    VPLHttp2 *http_handle;  
    // Mutex for http_handle access
    VPLMutex_t http_handle_mutex;

    DocSNGTicket(DocSNGQueue* parentQ) :
        parentQ(parentQ),
        inProgress(false),
        uploadFileSize(0),
        uploadFileProgress(0),
        uploadThumbSize(0),
        uploadThumbProgress(0),
        srcFile(VPLFILE_INVALID_HANDLE),
        out_of_date(false),
        result(0),
        http_handle(NULL)
    {
        int err = VPLMutex_Init(&http_handle_mutex);
        if (err) {
            LOG_ERROR("Failed to initialize mutex: %d", err);
        }
    }

    ~DocSNGTicket()
    {
        // Reset ticket (releasing resources)
        reset();
    }

    // returns true if ticket processing must stop.
    bool stop()
    {
        return !parentQ->isEnabled();
    }

    // Reset this ticket's progress, result, etc.
    void reset()
    {
        // Reset progress
        // TODO: Have parentQ lock?
        inProgress = false;
        uploadFileProgress = 0;
        uploadThumbProgress = 0;
        result = 0;

        // Clean-up external resources.
        if(VPLFile_IsValidHandle(srcFile)) {
            VPLFile_Close(srcFile);
            srcFile = VPLFILE_INVALID_HANDLE;
        }        

        int err = VPLMutex_Destroy(&http_handle_mutex);
        if (err) {
            LOG_ERROR("Failed to destroy mutex: %d", err);
        }
    }

    void set_out_of_date(bool out_of_date)
    {
        this->out_of_date = out_of_date;
    }

    bool is_out_of_date()
    {
        return out_of_date;
    }
};

#endif /* __cplusplus */

#endif /* include guard */
