//
//  Copyright 2013 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER CLOUD
//  TECHNOLOGY INC.
//

// NOTE: this file is only intended to be #include'd by SyncConfig.cpp

#include <vplex_http_util.hpp>
#include <google/protobuf/io/coded_stream.h>
#include "protobuf_file_reader.hpp"
#include "protobuf_file_writer.hpp"
#include "gvm_errors.h"

#define ASD_CLEANUP_DELAY_IN_SECS                               (15 * 60)
#define ASD_ORPHANED_METADATA_FILE_CLEANUP_DELAY_IN_SECS        (30 * 24 * 60 * 60) // Metadata files are small. Clean it up sparsely (~1 month) 
#define ASD_CLEANUP_META_FILE_EXT                               ".meta"
#define ASD_MAX_PATH_SIZE                                       1024

class SyncConfigTwoWayImpl : public SyncConfigImpl
{
private:
    SyncConfigTwoWayImpl(
            u64 user_id,
            const VcsDataset& dataset,
            SyncType type,
            const SyncPolicy& sync_policy,
            const std::string& local_dir,
            const std::string& server_dir,
            const DatasetAccessInfo& dataset_access_info,
            SyncConfigEventCallback event_cb,
            void* callback_context)
    :  SyncConfigImpl(user_id, dataset, type,
                      sync_policy, local_dir, server_dir,
                      dataset_access_info,
                      NULL, false,
                      event_cb, callback_context)
    {}

    // Allow the factory function to call our constructor.
    friend SyncConfig* CreateSyncConfig(
            u64 user_id,
            const VcsDataset& dataset,
            SyncType type,
            const SyncPolicy& sync_policy,
            const std::string& local_dir,
            const std::string& server_dir,
            const DatasetAccessInfo& dataset_access_info,
            SyncConfigThreadPool* thread_pool,
            bool make_dedicated_thread,
            SyncConfigEventCallback event_cb,
            void* callback_context,
            int& err_code__out,
            bool allow_create_db);


    /// @note If sync_policy.case_insensitive_compare is true, \a abs_path must
    ///     already be converted to uppercase for correct behavior.
    bool isSyncRoot(const std::string& abs_path) const
    {
        if (sync_policy.case_insensitive_compare) {
            return (abs_path.compare(local_dir_ci.str()) == 0);
        } else {
            return (abs_path.compare(local_dir.str()) == 0);
        }
    }

    /// Check if \a abs_path is *under* syncRoot.
    /// (Returns false when abs_path is the sync root itself.)
    /// @note If sync_policy.case_insensitive_compare is true, \a abs_path must
    ///     already be converted to uppercase for correct behavior.
    bool isInSyncRoot(const std::string& abs_path) const
    {
        std::string sync_root;
        if (sync_policy.case_insensitive_compare) {
            sync_root = local_dir_ci.str();
        } else {
            sync_root = local_dir.str();
        }
        if (abs_path.find(sync_root) == 0 &&
                (abs_path.size() > sync_root.size() &&
                 abs_path.substr(sync_root.size(), 1).compare("/") == 0)) {
            return true;
        }
        return false;
    }

    /// @note If sync_policy.case_insensitive_compare is true, \a rel_path must
    ///     already be converted to uppercase for correct behavior.
    bool isSyncTemp(const std::string& rel_path) const
    {
        return (rel_path.compare(convertCaseByPolicy(SYNC_TEMP_DIR)) == 0);
    }

    /// Check if \a rel_path is *under* syncTemp folder.
    /// (Returns false when rel_path is syncTemp itself.
    /// @note If sync_policy.case_insensitive_compare is true, \a rel_path must
    ///     already be converted to uppercase for correct behavior.
    bool isInSyncTemp(const std::string& rel_path) const
    {
        std::string sync_temp_path = convertCaseByPolicy(SYNC_TEMP_DIR);
        if (rel_path.find(sync_temp_path) == 0 &&
                (rel_path.size() > sync_temp_path.size() &&
                 rel_path.substr(sync_temp_path.size(), 1).compare("/") == 0)) {
            return true;
        }
        return false;
    }

    // must be called without SyncConfigImpl::mutex locked
    bool findFromIncrementalLocalScanPaths(const std::string& pathParent,
            const std::string& pathName)
    {
        bool rv = false;
        std::string path = pathParent + pathName;
        if (sync_policy.case_insensitive_compare) {
            path = Util_UTF8Upper(path.c_str());
        }
        std::string path_folder = path + "/";
        {
            MutexAutoLock lock(&mutex);
            for (std::set<string>::iterator it = incrementalLocalScanPaths.begin();
                    it != incrementalLocalScanPaths.end(); it++) {
                // check if we cannot find this file or folder.
                // If not, assum it is a folder and see if anything in this folder
                std::string cur_path = *it;
                if (sync_policy.case_insensitive_compare) {
                    cur_path = Util_UTF8Upper(cur_path.data());
                }
                if (cur_path.compare(path) == 0 || cur_path.find(path_folder) != string::npos) {
                    rv = true;
                }
            }
        }
        return rv;
    }

    static u32 convertToU32(u64 value)
    {
        if (value > UINT32_MAX) {
            LOG_WARN("Count exceeds u32 max!");
            return UINT32_MAX;
        }
        return (u32)value;
    }

    virtual void setDownloadScanRequested(bool value)
    {
        ASSERT(VPLMutex_LockedSelf(&mutex));
        download_scan_requested = value;
        // Trigger setStatus() to post a SyncConfigEventStatusChange
        // because remote_scan_pending may change
        setStatus(workerLoopStatus);
    }

    virtual void setDownloadScanError(bool local_downloadScanError)
    {
        ASSERT(VPLMutex_LockedSelf(&mutex));
        downloadScanError = local_downloadScanError;
        // Trigger setStatus() to post a SyncConfigEventStatusChange
        // because remote_scan_pending may change
        setStatus(workerLoopStatus);
    }

    virtual bool isRemoteScanPending()
    {
        ASSERT(VPLMutex_LockedSelf(&mutex));
        return ((download_scan_requested || downloadScanError));
    }

    virtual int updateUploadsRemaining()
    {
        // we don't report the number of incrementalLocalScanPaths
        // incrementalLocalScanPaths is reported by scan_in_progress
        u64 count = 0;
        int rv = localDb.uploadChangeLog_getRowCount(/*out*/count);
        if (rv != 0) {
            LOG_CRITICAL("%p: uploadChangeLog_getRowCount:%d", this, rv);
            HANDLE_DB_FAILURE();
        }
        workerUploadsRemaining = convertToU32(count);
        // Trigger setStatus() to post a SyncConfigEventStatusChange
        // because uploads_remaining may change
        SetStatus(workerLoopStatus);
        return rv;
    }

    virtual int updateDownloadsRemaining()
    {
        u64 count = 0;
        int rv = localDb.downloadChangeLog_getRowCount(/*out*/count);
        if (rv != 0) {
            LOG_CRITICAL("%p: downloadChangeLog_getRowCount:%d", this, rv);
            HANDLE_DB_FAILURE();
        }
        workerDownloadsRemaining = convertToU32(count);
        // Trigger setStatus() to post a SyncConfigEventStatusChange
        // because downloads_remaining may change
        SetStatus(workerLoopStatus);
        return rv;
    }

    virtual int GetSyncStateForPath(const std::string& abs_path_ts,
                                    SyncConfigStateType_t& state__out,
                                    u64& dataset_id__out,
                                    bool& is_sync_folder_root)
    {
        int rv = 0;
        SCRelPath rel_path = SCRelPath("");

        std::string abs_path;
        std::string abs_path_tmp;
        SCRow_uploadChangeLog up_entry_out;
        SCRow_downloadChangeLog down_entry_out;
        SCRow_syncHistoryTree sync_entry_out;
        is_sync_folder_root = false;

        state__out = SYNC_CONFIG_STATE_UNKNOWN;

        abs_path_tmp = Util_CleanupPath(abs_path_ts);
        if (sync_policy.case_insensitive_compare) {
            abs_path = Util_UTF8Upper(abs_path_tmp.data());
        } else {
            abs_path = abs_path_tmp;
        }

        if (isSyncRoot(abs_path)) {
            is_sync_folder_root = true;
        } else if (!isInSyncRoot(abs_path)) {
            state__out = SYNC_CONFIG_STATE_NOT_IN_SYNC_FOLDER;
            return 0;
        } else {
            // It is in SyncRoot, we can assume abs_path is
            // longer than local_dir here

            // rel_start include a slash right after syncRoot
            std::string sync_root;
            if (sync_policy.case_insensitive_compare) {
                sync_root = local_dir_ci.str();
            } else {
                sync_root = local_dir.str();
            }
            int rel_start = sync_root.size() + 1;
            rel_path = abs_path.substr(rel_start);
        }
        dataset_id__out = dataset.id;

        const std::string pathName = rel_path.getName(); // pathName can be a directory
        const std::string pathParent = rel_path.getParent().str();

        if (!is_sync_folder_root &&
                (!isValidDirEntry(pathName) || isSyncTemp(pathParent) ||
                 isInSyncTemp(pathParent))) {
            state__out = SYNC_CONFIG_STATE_FILTERED;
            return rv;
        }

        // Possible optimization: We could skip some of these queries if we know that we are already in-sync.

        MutexAutoLock lock(&getStateDbMutex);
        if (getStateDbInit) {
            // Query downloadChangeLog table to know if any items under
            // pathParent + pathName
            int download_rc = getStateDb.downloadChangeLog_getState(
                    pathParent, pathName, down_entry_out);
            if (download_rc == 0) {
                // If output path is not equal input path, which means
                // input name is a directory, because output is one
                // entry of the input directory.
                if (down_entry_out.parent_path.compare(pathParent) == 0 &&
                        down_entry_out.name.compare(pathName) == 0 &&
                        !down_entry_out.is_dir) {
                    // If the exact (pathParent/pathName) found, and
                    // is_dir is true, means abs_path is a file.
                    state__out = SYNC_CONFIG_STATE_NEED_TO_DOWNLOAD;
                } else {
                    // If abs_path is a directory in downloadChangeLog,
                    //  also need to check if it contains upload state
                    int upload_rc = getStateDb.uploadChangeLog_getState(
                            pathParent, pathName, up_entry_out);
                    if (upload_rc == 0) {
                        state__out = SYNC_CONFIG_STATE_NEED_TO_UPLOAD_AND_DOWNLOAD;
                    } else if (upload_rc != SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) {
                        LOG_CRITICAL("%p: getStateDb.uploadChangeLog_getState(%s, %s) failed: %d",
                                this, pathParent.c_str(), pathName.c_str(), upload_rc);
                        HANDLE_DB_FAILURE();
                    } else if (findFromIncrementalLocalScanPaths(pathParent, pathName)){
                        state__out = SYNC_CONFIG_STATE_NEED_TO_UPLOAD_AND_DOWNLOAD;
                    } else {
                        state__out = SYNC_CONFIG_STATE_NEED_TO_DOWNLOAD;
                    }
                }
            } else if (download_rc == SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) {
                // Cannot find any entry in downloadChangeLog table,
                //  now try uploadChangeLog

                // Query downloadChangeLog table to know if any items
                //  under pathParent + pathName
                int upload_rc = getStateDb.uploadChangeLog_getState(
                        pathParent, pathName, up_entry_out);
                if (upload_rc == 0) {
                    state__out = SYNC_CONFIG_STATE_NEED_TO_UPLOAD;
                } else if (upload_rc != SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) {
                    LOG_CRITICAL("%p: getStateDb.uploadChangeLog_getState(%s, %s) failed: %d",
                            this, pathParent.c_str(), pathName.c_str(), upload_rc);
                    HANDLE_DB_FAILURE();
                } else if (findFromIncrementalLocalScanPaths(pathParent, pathName)){
                    state__out = SYNC_CONFIG_STATE_NEED_TO_UPLOAD;
                } else {
                    // This is the worst case, query DB 3 times.
                    // Cannot find any entry in uploadChangeLog table and
                    //  downloadChangeLog table, now try SyncHistoryTree
                    int sync_rc = getStateDb.syncHistoryTree_get(
                            pathParent, pathName, sync_entry_out);
                    if (sync_rc == 0) {
                        state__out = SYNC_CONFIG_STATE_UP_TO_DATE;
                    } else if (sync_rc == SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) {
                        state__out = SYNC_CONFIG_STATE_UNKNOWN;
                    } else {
                        LOG_CRITICAL("%p: getStateDb.syncHistoryTree_get(%s, %s) failed: %d",
                                this, pathParent.c_str(), pathName.c_str(), sync_rc);
                        HANDLE_DB_FAILURE();
                    }
                }
            } else {
                LOG_CRITICAL("%p: getStateDb.downloadChangeLog_getState(%s, %s) failed: %d",
                        this, pathParent.c_str(), pathName.c_str(), download_rc);
                HANDLE_DB_FAILURE();
            }
        } else {
            LOG_ERROR("getStateDB not init");
            rv = CCD_ERROR_NOT_INIT;
        }

        return rv;
    }

    int uploadChangeLog_remove_and_updateRemaining(u64 row_id)
    {
        int rc = 0;
        rc = localDb.uploadChangeLog_remove(row_id);
        if (rc < 0) {
            LOG_CRITICAL("%p: uploadChangeLog_remove("FMTu64") failed: %d",
                    this, row_id, rc);
            HANDLE_DB_FAILURE();
        } else {
            rc = updateUploadsRemaining();
            if (rc != 0) {
                LOG_ERROR("updateUploadsRemaining:%d", rc);
            }
        }
        return rc;
    }
    
    int downloadChangeLog_remove_and_updateRemaining(u64 row_id)
    {
        int rc = 0;
        rc = localDb.downloadChangeLog_remove(row_id);
        if (rc < 0) {
            LOG_CRITICAL("%p: downloadChangeLog_remove("FMTu64") failed: %d",
                    this, row_id, rc);
            HANDLE_DB_FAILURE();
        } else {
            rc = updateDownloadsRemaining();
            if (rc != 0) {
                LOG_ERROR("downdateDownloadsRemaining:%d", rc);
            }
        }
        return rc;
    }
    

    void setErrorNeedsRemoteScan()
    {
        MutexAutoLock lock(&mutex);
        setDownloadScanError(true);
        setErrorTimeout(sync_policy.error_retry_interval);
    }

    std::string getTempSyncboxMetadataFilePath()
    {
        ostringstream temp;
        temp << local_dir.str() << "/" << SYNC_TEMP_DIR << "/tmpSyncbox/"; 
        return temp.str();
    }


    // Protobuf file format for staging file metadata file
    // Field 1: u32 hasCompId;   
    // If hasCompId == 1
    //   Field 2: u64 component id
    //   Field 3: u64 revision id
    //   Field 4: path of the component relative to dataset root
    // If hasCompId == 0
    //   Field 2: path of the component relative to dataset root
    int writeStagingMetadataFile(google::protobuf::io::CodedOutputStream& tempStream, u64 componentId, u64 revisionId, const std::string& relativePath)
    {
        int rv = 0;
        tempStream.WriteVarint32(1);    // hasCompId
        if (tempStream.HadError()) {
            LOG_ERROR("Failed to write hasCompId field");
            rv = -1;
            goto out;
        }
        tempStream.WriteVarint64(componentId);
        if (tempStream.HadError()) {
            LOG_ERROR("Failed to write componentId field");
            rv = -1;
            goto out;
        }
        tempStream.WriteVarint64(revisionId);
        if (tempStream.HadError()) {
            LOG_ERROR("Failed to write revisiontId field");
            rv = -1;
            goto out;
        }
        tempStream.WriteString(relativePath);
        if (tempStream.HadError()) {
            LOG_ERROR("Failed to write relativePath field");
            rv = -1;
            goto out;
        }
out:
        return rv;
    }

    int writeStagingMetadataFile(google::protobuf::io::CodedOutputStream &tempStream, const std::string& relativePath)
    {
        int rv = 0;
        tempStream.WriteVarint32(0);    // hasCompId
        if (tempStream.HadError()) {
            LOG_ERROR("Failed to write hasCompId field");
            rv = -1;
            goto out;
        }
        tempStream.WriteString(relativePath);
        if (tempStream.HadError()) {
            LOG_ERROR("Failed to write relativePath field");
            rv = -1;
            goto out;
        }
out:
        return rv;
    }

    int readStagingMetadataFile(google::protobuf::io::CodedInputStream& tempStream, bool& hasCompId, u64& componentId, u64& revisionId, std::string& relativePath)
    {
        int rv = 0;
        u32 hasComponentId;

        if (!tempStream.ReadVarint32(&hasComponentId)) {
            LOG_ERROR("Failed to read hasCompId Field");
            rv = -1;
            goto out;
        }
        hasCompId = (bool)(hasComponentId == 1);
        if (hasCompId) {
            if (!tempStream.ReadVarint64(&componentId)) {
                LOG_ERROR("Failed to read componentId Field");
                rv = -1;
                goto out;
            }
            if (!tempStream.ReadVarint64(&revisionId)) {
                LOG_ERROR("Failed to read revisionId Field");
                rv = -1;
                goto out;
            }
#if 0 
            // TODO: Bug 17347 Not sure why ReadString always return false??
            if (!tempStream.ReadString(&relativePath, ASD_MAX_PATH_SIZE)) {
                LOG_ERROR("Failed to read revisionId Field");
                rv = -1;
                goto out;
            }
#else
            tempStream.ReadString(&relativePath, ASD_MAX_PATH_SIZE);
#endif
        } else {
#if 0
            // Not sure why ReadString always return false??
            if (!tempStream.ReadString(&relativePath, ASD_MAX_PATH_SIZE)) {
                LOG_ERROR("Failed to read relativePath Field %s", relativePath.c_str());
                rv = -1;
                goto out;
            }
#else
            tempStream.ReadString(&relativePath, ASD_MAX_PATH_SIZE);
#endif
        }
                
out:
        return rv;
    }

    void addFileDeleteToUploadChangeLog(const SCRow_syncHistoryTree& fileToDelete)
    {
        ASSERT(!fileToDelete.is_dir);
        ASSERT(fileToDelete.revision_exists); // Should only upload a deletion if the file was previously downloaded/uploaded to/from this device.
        ASSERT(fileToDelete.local_mtime_exists);
        // append "remove file <entry>, LocalDB[entry].vcs_comp_id, LocalDB[entry].vcs_revision" to UploadChangeLog
        LOG_INFO("%p: Decision UploadDelete(%s,%s),compid(%d,"FMTu64"),rev(%d,"FMTu64")",
                 this,
                 fileToDelete.parent_path.c_str(),
                 fileToDelete.name.c_str(),
                 fileToDelete.comp_id_exists, fileToDelete.comp_id,
                 fileToDelete.revision_exists, fileToDelete.revision);
        localDb_uploadChangeLog_add_and_setSyncStatus(
                                    fileToDelete.parent_path,
                                    fileToDelete.name,
                                    fileToDelete.is_dir,
                                    UPLOAD_ACTION_DELETE,
                                    fileToDelete.comp_id_exists,
                                    fileToDelete.comp_id,
                                    fileToDelete.revision_exists,
                                    fileToDelete.revision,
                                    fileToDelete.hash_value,
                                    fileToDelete.file_size_exists,
                                    fileToDelete.file_size);
    }

    void addDirDeleteToUploadChangeLog(const SCRow_syncHistoryTree& directory)
    {
        // Recursively add all children files and directories of entry to
        // UploadChangeLog as "remove X" (add them depth-first, to ensure that
        // child files are removed before their parent directory).
        int rc;
        std::vector<SCRow_syncHistoryTree> traversedDirsStack;
        {
            std::deque<SCRow_syncHistoryTree> dirsToTraverseQ;
            dirsToTraverseQ.push_back(directory);
            while(!dirsToTraverseQ.empty())
            {
                SCRow_syncHistoryTree toTraverse = dirsToTraverseQ.front();
                dirsToTraverseQ.pop_front();

                std::vector<SCRow_syncHistoryTree> dirEntries_out;
                SCRelPath currDirPath = getRelPath(toTraverse);
                rc = localDb.syncHistoryTree_getChildren(currDirPath.str(), dirEntries_out);
                if ((rc != 0) && (rc != SYNC_AGENT_DB_ERR_ROW_NOT_FOUND)) {
                    LOG_CRITICAL("%p: syncHistoryTree_get(%s) failed: %d",
                            this, currDirPath.c_str(), rc);
                    HANDLE_DB_FAILURE();
                    continue;
                }
                for(std::vector<SCRow_syncHistoryTree>::iterator it = dirEntries_out.begin();
                    it != dirEntries_out.end();
                    ++it)
                {
                    SCRow_syncHistoryTree& currDirEntry = *it;
                    if(isRoot(currDirEntry)){ continue; } // Skip root exception case

                    if(currDirEntry.is_dir) {
                        dirsToTraverseQ.push_back(currDirEntry);
                    }else{
                        // No need to delete files in depth first order.  Just delete now.

                        // It's a requirement that files to be deleted MUST have
                        // a local modified time and revision, otherwise this
                        // could be a newly discovered file by the download scan.
                        if(currDirEntry.local_mtime_exists &&
                           currDirEntry.revision_exists) {
                            addFileDeleteToUploadChangeLog(currDirEntry);
                        }else{
                            LOG_INFO("%p: revision/local_mtime does not exist for (%s,%s)."
                                     "Cannot upload delete for file that has never uploaded or downloaded.",
                                     this,
                                     currDirEntry.parent_path.c_str(),
                                     currDirEntry.name.c_str());
                        }
                    }
                }
                traversedDirsStack.push_back(toTraverse);
            }  // while(!dirsToTraverseQ.empty())
        }

        // Delete the directories;
        while(!traversedDirsStack.empty())
        {
            SCRow_syncHistoryTree dirToDelete = traversedDirsStack.back();
            traversedDirsStack.pop_back();
            ASSERT(dirToDelete.is_dir);
            LOG_INFO("%p: Decision UploadDelete dir (%s,%s),compId(%d,"FMTu64")",
                     this,
                     dirToDelete.parent_path.c_str(),
                     dirToDelete.name.c_str(),
                     dirToDelete.comp_id_exists,
                     dirToDelete.comp_id);
            // append "remove directory <entry>, LocalDB[entry].vcs_comp_id" to UploadChangeLog
            localDb_uploadChangeLog_add_and_setSyncStatus(
                                        dirToDelete.parent_path,
                                        dirToDelete.name,
                                        dirToDelete.is_dir,
                                        UPLOAD_ACTION_DELETE,
                                        dirToDelete.comp_id_exists,
                                        dirToDelete.comp_id,
                                        false, 0,
                                        dirToDelete.hash_value,
                                        dirToDelete.file_size_exists,
                                        dirToDelete.file_size);
        }  // while(!traversedDirsStack.empty())
    }

    /// Populates the UploadChangeLog by detecting adds/updates/deletes from the local filesystem.
    void PerformFullLocalScan()
    {
        int fsOpCount = 0; // We will check for pause/stop every FILESYS_OPERATIONS_PER_STOP_CHECK operations.
        LOG_INFO("%p: Begin full local scan; clearing uploadChangeLog", this);
        int rc = localDb.uploadChangeLog_clear();
        if (rc < 0) {
            LOG_CRITICAL("%p: uploadChangeLog_clear() failed: %d", this, rc);
            HANDLE_DB_FAILURE();
            return;
        }
        deque<SCRelPath> relDirQ;
        // Add SyncConfig root to DirectoryQ
        relDirQ.push_back(SCRelPath(""));
        while (!relDirQ.empty()) {
            if (fsOpCount++ >= FILESYS_OPERATIONS_PER_STOP_CHECK) {
                if (checkForPauseStop()) {
                    return;
                }
                fsOpCount = 0;
            }
            SCRelPath currRelDir = relDirQ.front();
            relDirQ.pop_front();
            AbsPath currAbsLocalDir = getAbsPath(currRelDir);
            LOG_DEBUG("%p: Scanning "FMTu64":%s (%s)",
                      this, dataset.id, currRelDir.c_str(), currAbsLocalDir.c_str());
            set<string> pathExists; // There should be no "/" characters in here.
            VPLFS_dir_t dirStream;
            int rc = VPLFS_Opendir(currAbsLocalDir.c_str(), &dirStream);
            if (rc < 0) {
                // Dropping the directory and everything beneath it until we do another local scan.
                if (rc == VPL_ERR_NOENT) {
                    // The directory was probably deleted just now.
                    // There should be a file monitor notification (or app notification)
                    // coming soon to force us to scan again.
                    LOG_INFO("%p: Directory (%s) no longer exists", this, currAbsLocalDir.c_str());
                } else {
                    LOG_WARN("%p: VPLFS_Opendir(%s) failed: %d", this, currAbsLocalDir.c_str(), rc);
                    // TODO: Instead of dropping, we could set uploadScanError and setErrorTimeout (to try
                    //   again automatically), but that seems more dangerous since the directory may
                    //   never become readable, and we'd keep doing a scan every 15 minutes.
                    //   If we had some way to track the specific directories
                    //   causing problems and backoff the retry time, it might be better.
                }
                continue;
            }
            ON_BLOCK_EXIT(VPLFS_Closedir, &dirStream);

            // For each dirEntry in currDir:
            VPLFS_dirent_t dirEntry;
            while ((rc = VPLFS_Readdir(&dirStream, &dirEntry)) == VPL_OK) {
                // Ignore special directories and filenames that contain unsupported characters.
                if(!isValidDirEntry(dirEntry.filename)) {
                    continue;
                }
                // Only increase the count, but don't call clearHttpHandleAndCheckForPauseStop,
                // since we still have dirStream open.
                fsOpCount++;

                // Add to "pathExists" set.
                if (sync_policy.case_insensitive_compare) {
                    std::string filename_ci;
                    Util_UTF8Upper(dirEntry.filename, filename_ci);
                    pathExists.insert(filename_ci);
                } else {
                    pathExists.insert(dirEntry.filename);
                }

                SCRelPath currEntryRelPath = currRelDir.getChild(dirEntry.filename);
                bool currEntryIsDir = (dirEntry.type == VPLFS_TYPE_DIR);
                if (currEntryIsDir) {
                    relDirQ.push_back(currEntryRelPath);
                }
                SCRow_syncHistoryTree currDbEntry;
                rc = localDb.syncHistoryTree_get(currRelDir.str(), dirEntry.filename, /*OUT*/currDbEntry);
                if (rc == SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) { // dirEntry not present in LocalDB.
                    LOG_INFO("%p: Decision UploadCreate (%s,%s), %d",
                            this, currRelDir.c_str(), dirEntry.filename, currEntryIsDir);
                    localDb_uploadChangeLog_add_and_setSyncStatus(
                            currRelDir.str(), dirEntry.filename, currEntryIsDir,
                            UPLOAD_ACTION_CREATE, false, 0, false, 0, currDbEntry.hash_value,
                            currDbEntry.file_size_exists, currDbEntry.file_size);
                } else if (rc < 0) {
                    LOG_CRITICAL("%p: syncHistoryTree_get(%s, %s) failed: %d",
                              this, currRelDir.c_str(), dirEntry.filename, rc);
                    HANDLE_DB_FAILURE();
                    return;
                } else { // dirEntry present in LocalDB; currDbEntry is valid.
                    // Compare the previous record of our local FS with what actually exists now.
                    if (currEntryIsDir != currDbEntry.is_dir) {
                        // Local FS entry was changed from file to dir (or vice-versa).
                        if(currDbEntry.is_dir) {
                            ASSERT(currDbEntry.comp_id_exists);
                            LOG_INFO("%p: Decision UploadDelete/UploadCreate dir changed to file (%s,%s),%d,%d",
                                     this, currRelDir.c_str(), dirEntry.filename,
                                     currDbEntry.is_dir, currDbEntry.comp_id_exists);
                            // Adds entries depth first to be deleted.
                            // Parent directory final entry expected to be overwritten
                            //   by UPLOAD_ACTION_CHANGE_FILE_OR_DIR_TYPE below
                            addDirDeleteToUploadChangeLog(currDbEntry);
                        } else {
                            ASSERT(currDbEntry.comp_id_exists);
                            if (!currDbEntry.revision_exists) {
                                // It's a file on VCS, but we haven't downloaded it yet (probably
                                // ran into a transient error in the ApplyDownloads phase).
                                // This is a conflict case, which will be resolved when the file
                                // is successfully downloaded.
                                LOG_INFO("%p: Conflict; file on VCS, dir on local (%s,%s), skip upload",
                                         this, currRelDir.c_str(), dirEntry.filename);
                                continue;
                            }
                            LOG_INFO("%p: Decision UploadDelete/UploadCreate file changed to dir (%s,%s),%d,%d",
                                     this, currRelDir.c_str(), dirEntry.filename,
                                     currDbEntry.is_dir, currDbEntry.comp_id_exists);
                        }
                        localDb_uploadChangeLog_add_and_setSyncStatus(
                                currRelDir.str(), dirEntry.filename, currDbEntry.is_dir,
                                UPLOAD_ACTION_CHANGE_FILE_OR_DIR_TYPE,
                                currDbEntry.comp_id_exists, currDbEntry.comp_id,
                                currDbEntry.revision_exists, currDbEntry.revision,
                                currDbEntry.hash_value, currDbEntry.file_size_exists, currDbEntry.file_size);
                    } else if (!currEntryIsDir) {
                        AbsPath currEntryAbsPath = getAbsPath(currEntryRelPath);
                        // Stat the file to get its modified time.
                        VPLFS_stat_t statBuf;
                        rc = VPLFS_Stat(currEntryAbsPath.c_str(), &statBuf);
                        if (rc < 0) {
                            // Dropping the file until we do another local scan.
                            if (rc == VPL_ERR_NOENT) {
                                // The file was probably deleted just now.
                                // There should be a file monitor notification (or app notification)
                                // coming soon to force us to scan again.
                                LOG_INFO("%p: File (%s) no longer exists", this, currEntryAbsPath.c_str());
                            } else {
                                LOG_WARN("%p: VPLFS_Stat(%s) failed: %d", this, currEntryAbsPath.c_str(), rc);
                                // TODO: Instead of dropping, we could set uploadScanError and setErrorTimeout (to try
                                //   again automatically), but that seems more dangerous since the directory may
                                //   never become readable, and we'd keep doing a scan every 15 minutes.
                                //   If we had some way to track the specific directories
                                //   causing problems and backoff the retry time, it might be better.
                            }
                            continue;
                        }
                        VPLTime_t localModifiedTime = fsTimeToNormLocalTime(currEntryAbsPath, statBuf.vpl_mtime);
                        if (localModifiedTime != currDbEntry.local_mtime) {
                            LOG_INFO("%p: Decision UploadUpdate(%s):"FMTu64"!="FMTu64" mTimeNanosec:"FMTu64", mtimeSec:"FMTu64,
                                    this,
                                    currEntryAbsPath.c_str(),
                                    localModifiedTime,
                                    currDbEntry.local_mtime,
                                    statBuf.vpl_mtime,
                                    (u64)statBuf.mtime);
                            localDb_uploadChangeLog_add_and_setSyncStatus(
                                    currRelDir.str(), dirEntry.filename, currEntryIsDir,
                                    UPLOAD_ACTION_UPDATE, false, 0, false, 0, currDbEntry.hash_value,
                                    currDbEntry.file_size_exists, currDbEntry.file_size);
                        } else {
                            // File was not modified, do nothing.
                        }
                    } else {
                        // Entry is a directory and was previously a directory.
                        if (!currDbEntry.comp_id_exists) {
                            // If remote compId is missing for some reason, upload it again.
                            // This is to recover from Case J.
                            LOG_WARN("%p: LocalDB entry(%s,%s) has no compId.  Recovering.",
                                    this, currRelDir.c_str(), dirEntry.filename);
                            localDb_uploadChangeLog_add_and_setSyncStatus(
                                    currRelDir.str(), dirEntry.filename, currEntryIsDir,
                                    UPLOAD_ACTION_CREATE, false, 0, false, 0, currDbEntry.hash_value,
                                    currDbEntry.file_size_exists, currDbEntry.file_size);
                        }
                    }
                }
            } // for each dirEntry in currDir

            // Detect local files/dirs that have been deleted:
            // For each entry in currDir in LocalDB that are not in the "pathExists" Set:
            std::vector<SCRow_syncHistoryTree> previousEntries;
            rc = localDb.syncHistoryTree_getChildren(currRelDir.str(), previousEntries);
            if ((rc != 0) && (rc != SYNC_AGENT_DB_ERR_ROW_NOT_FOUND)) {
                LOG_CRITICAL("%p: syncHistoryTree_get(%s) failed: %d", this, currRelDir.c_str(), rc);
                HANDLE_DB_FAILURE();
                return;
            }
            for(std::vector<SCRow_syncHistoryTree>::iterator it = previousEntries.begin();
                it != previousEntries.end();
                ++it)
            {
                SCRow_syncHistoryTree& currToDelete = *it;
                std::string currToDeleteKey;

                if(isRoot(currToDelete)){ continue; } // Skip root exception case

                if (sync_policy.case_insensitive_compare) {
                    Util_UTF8Upper(currToDelete.name.data(), currToDeleteKey);
                } else {
                    currToDeleteKey = currToDelete.name;
                }

                if (contains(pathExists, currToDeleteKey)) {
                    LOG_DEBUG("%p: %s still exists.", this, currToDelete.name.c_str());
                }else {
                    LOG_DEBUG("%p: %s no longer exists.", this, currToDelete.name.c_str());
                    if (currToDelete.is_dir) {
                        // Because we create the local directory as soon as we learn of it, it is
                        // most likely that the local user removed it.
                        // (Search this file for "NOTE-130326").
                        // It is also possible that the local create failed, but in this case, we
                        // shouldn't have been able to download any children, so we won't issue
                        // the VCS deletes and VCS will prevent us from incorrectly deleting this
                        // directory here (as long as there is at least one file within the directory).
                        addDirDeleteToUploadChangeLog(currToDelete);
                    } else {
                        // If the file was seen in a download scan but not actually downloaded
                        // (due to an error or due to CCD being shut down), we will still have an
                        // entry in localDb.syncHistoryTree (to record the compId), but there will
                        // be no mtime or revision information.  Since we never downloaded the file,
                        // we should not consider this as a local deletion.
                        if (currToDelete.local_mtime_exists && currToDelete.revision_exists) {
                            addFileDeleteToUploadChangeLog(currToDelete);
                        }
                    }
                }
            }
        } // while (!relDirQ.empty())
    }

    int computeMD5(const char* file_path, std::string& hash__out)
    {
        MD5Context hashCtx;
        u8 hashVal[MD5_DIGESTSIZE];

        VPLFile_handle_t fh;
        int rv;
        unsigned char buf[1024];

        fh = VPLFile_Open(file_path, VPLFILE_OPENFLAG_READONLY, 0666);
        if (!VPLFile_IsValidHandle(fh)) {
            LOG_ERROR("Error opening file: %s", file_path);
            return -1;
        } else {
            rv = MD5Reset(&hashCtx);
            if (rv != 0) {
                LOG_ERROR("MD5Reset error, errno:%d", rv);
                VPLFile_Close(fh);
                return rv;
            }
            ssize_t n;
            while((n = VPLFile_Read(fh, buf, sizeof(buf))) > 0) {
                rv = MD5Input(&hashCtx, buf, n);
                if (rv != 0) {
                    LOG_ERROR("MD5Input error, errno:%d", rv);
                    VPLFile_Close(fh);
                    return rv;
                }
            }
            rv = MD5Result(&hashCtx, hashVal);
            if (rv != 0) {
                LOG_ERROR("MD5Result error, errno:%d", rv);
                VPLFile_Close(fh);
                return rv;
            }
        }

        hash__out.clear();
        for(int hashIndex = 0; hashIndex<MD5_DIGESTSIZE; hashIndex++) {
            char byteStr[4];
            snprintf(byteStr, sizeof(byteStr), "%02"PRIx8, hashVal[hashIndex]);
            hash__out.append(byteStr);
        }

        VPLFile_Close(fh);
        return 0;
    }

    int compareHash(u64 dbFileSize, const std::string& dbHashValue, u64 localFileSize, const AbsPath& localFilePath)
    {
        std::string localFileHash;
        LOG_INFO("db file size:"FMTu64", local localFileSize:"FMTu64", fileName:%s",
                dbFileSize, localFileSize, localFilePath.c_str());

        if (dbFileSize == localFileSize && !dbHashValue.empty()) {
            VPLTime_t time_start, time_end;
            time_start = VPLTime_GetTimeStamp();
            int hash_status =  computeMD5(localFilePath.c_str(), localFileHash);
            time_end = VPLTime_GetTimeStamp();
            LOG_INFO("Spent "FMT_VPLTime_t"usec to compute hash for: %s", VPLTime_DiffClamp(time_end, time_start), localFilePath.c_str());

            if (hash_status != 0) {
                LOG_ERROR("computeMD5 error, errno:%d", hash_status);
            } else {
                if (dbHashValue.compare(localFileHash) == 0) {
                    // Hash is the same
                    LOG_INFO("hash compare is the same, File: %s, hash:%s",
                            localFilePath.c_str(), localFileHash.c_str());
                    return 0;
                } else {
                    LOG_INFO("hash compare is different, File: %s, hash in database:%s, local hash:%s",
                            localFilePath.c_str(), dbHashValue.c_str(), localFileHash.c_str());
                }
            }
        }
        return -1;
    }

    bool touchTempTimestampFile()
    {
        // Find out the current mtime for the filesystem:
        // TODO: This would probably be more efficient if we had VPLFile_Touch(), but I'm not sure how to implement it for WinRT.
        VPLFile_handle_t tempHandle = VPLFile_Open(getTempTouchTimestampFile().c_str(),
                VPLFILE_OPENFLAG_CREATE | VPLFILE_OPENFLAG_TRUNCATE | VPLFILE_OPENFLAG_WRITEONLY, 0666);
        if (tempHandle < 0) {
            LOG_WARN("Failed to open %s: %d", getTempTouchTimestampFile().c_str(), tempHandle);
            return false;
        }
        ON_BLOCK_EXIT(VPLFile_Close, tempHandle);

        ssize_t written = VPLFile_Write(tempHandle, "", 1);
        if (written != 1) {
            LOG_WARN("Failed to write to %s: "FMT_ssize_t, getTempTouchTimestampFile().c_str(), written);
            return false;
        }

        return true;
    }

    // Input:
    //   currUploadEntry - A single entry to perform upload/update to ACS/storage
    // Returns:
    //  errorInc_out - An error occurred in this function, and the operation should
    //                 be retried sometime later
    //  skip_out - Any operations related to the currUploadEntry should be skipped
    //             when this function returns (currently success case and skip are
    //             identical, but this may not always be the case)
    //  endFunction_out - The entire ApplyChangeLog step should be abandoned
    //                    Perhaps the user is shutting down or logging out.
    // TODO: name is misleading; this will not involve ACS for Syncbox sync.
    void ApplyUploadCreateOrUpdateToAcs(SCRow_uploadChangeLog& currUploadEntry,
                                        bool& errorInc_out,
                                        bool& skip_out,
                                        bool& endFunction_out)
    {
        int rc;
        ASSERT(!VPLMutex_LockedSelf(&mutex));

        // Get the current compId, revision, and hash for the file from the localDB.
        bool currUploadEntryHasCompId = false;
        u64 currUploadEntryCompId = -1;
        u64 currUploadEntryRevisionToUpload = 1;
        u64 logOnlyCurrUploadMtime = 0;
        bool hasAccessUrl = true;

        SCRelPath currUploadEntryRelPath = getRelPath(currUploadEntry);
        {
            SCRow_syncHistoryTree currHistoryEntry;
            rc = syncHistoryTree_getEntry(currUploadEntryRelPath, currHistoryEntry);
            if (rc == 0) {
                if (currHistoryEntry.comp_id_exists) {
                    currUploadEntryHasCompId = true;
                    currUploadEntryCompId = currHistoryEntry.comp_id;
                }
                if (currHistoryEntry.revision_exists) {
                    currUploadEntryRevisionToUpload = currHistoryEntry.revision + 1;
                }
                if (currHistoryEntry.local_mtime_exists) {
                    logOnlyCurrUploadMtime = currHistoryEntry.local_mtime;
                }
            } else if (rc == SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) {
                // OK.  This just means that we are adding without a previous record.
            } else {
                LOG_CRITICAL("%p: syncHistoryTree_getEntry(%s) failed: %d", this, currUploadEntryRelPath.c_str(), rc);
                HANDLE_DB_FAILURE();
                skip_out = true;  // skip, retry doesn't have much hope to recover.
                return;
            }
        }

        AbsPath currEntryAbsPath = getAbsPath(getRelPath(currUploadEntry));
        VPLTime_t fileModifyTime;
        VPLTime_t fileCreateTime;
        VPLFS_file_size_t fileSize;
        VPLFS_file_type_t fileType;
        {
            // Stat the file to get its modified time.
            VPLFS_stat_t statBuf;
            rc = VPLFS_Stat(currEntryAbsPath.c_str(), &statBuf);
            if (rc < 0) {
                // Dropping the file until we do another local scan.
                if (rc == VPL_ERR_NOENT) {
                    // The file was probably deleted just now.
                    // There should be a file monitor notification (or app notification)
                    // coming soon to force us to scan again.
                    LOG_INFO("%p: File (%s) no longer exists", this, currEntryAbsPath.c_str());
                } else {
                    LOG_WARN("%p: VPLFS_Stat(%s) failed: %d", this, currEntryAbsPath.c_str(), rc);
                    // TODO: Bug 11588: Instead of dropping, we could set uploadScanError and setErrorTimeout (to try
                    //   again automatically), but that seems more dangerous since the directory may
                    //   never become readable, and we'd keep doing a scan every 15 minutes.
                    //   If we had some way to track the specific entities
                    //   causing problems and backoff the retry time, it might be better.
                }
                skip_out = true;
                return;
            }
            fileCreateTime = fsTimeToNormLocalTime(currEntryAbsPath, statBuf.vpl_ctime);
            fileModifyTime = fsTimeToNormLocalTime(currEntryAbsPath, statBuf.vpl_mtime);
            fileSize = statBuf.size;
            fileType = statBuf.type;
            // FAT32 correctness case: If it's possible for the user to make another change to the
            // file yet keep the same modification timestamp, we need to delay until we are past the
            // current timestamp interval.  2 seconds (for FAT32) seems to be the coarsest granularity
            // that we care about.
            {
                // Find out the current mtime for the filesystem:
                if (!touchTempTimestampFile()) {
                    LOG_WARN("Failed to touch %s; sleep for 2 seconds", getTempTouchTimestampFile().c_str());
                    VPLThread_Sleep(VPLTime_FromSec(2));
                } else {
                    VPLFS_stat_t tempTouchStat;
                    int temp_rc = VPLFS_Stat(getTempTouchTimestampFile().c_str(), &tempTouchStat);
                    if (temp_rc < 0) {
                        LOG_WARN("Failed to stat %s: %d, sleep for 2 seconds", getTempTouchTimestampFile().c_str(), temp_rc);
                        VPLThread_Sleep(VPLTime_FromSec(2));
                    } else {
                        LOG_INFO("mtime for \"%s\"="FMTu64", curr_mtime="FMTu64,
                                currEntryAbsPath.c_str(), statBuf.vpl_mtime, tempTouchStat.vpl_mtime);
                        if (statBuf.vpl_mtime == tempTouchStat.vpl_mtime) {
                            LOG_INFO("File to upload has changed too recently; sleep for 2 seconds");
                            VPLThread_Sleep(VPLTime_FromSec(2));
                        }
                        // else, no need to sleep
                    }
                }
            }
        }

        LOG_INFO("%p: ACTION UP upload begin:%s mtimes("FMTu64","FMTu64"),"
                 "ctime("FMTu64"),compId("FMTu64"),revUp("FMTu64")",
                 this,
                 currEntryAbsPath.c_str(),
                 logOnlyCurrUploadMtime,
                 fileModifyTime,
                 fileCreateTime,
                 currUploadEntryHasCompId?currUploadEntryCompId:0,
                 currUploadEntryRevisionToUpload);

        // check file size between UploadChangeLog table and local file,
        // if size is the same, then compare hash value, then only upload file if hash is different
        int hashRet = -1;
        if (sync_policy.how_to_compare == SyncPolicy::FILE_COMPARE_POLICY_USE_HASH && 
                fileSize >= 0 && fileType == VPLFS_TYPE_FILE) {
            hashRet = compareHash(currUploadEntry.file_size, currUploadEntry.hash_value, (u64)fileSize, currEntryAbsPath);
        }
        if (hashRet == 0){
            // We don't post Metadata to VCS. But need to update timestamp into syncHistoryTree here,
            // or it will always generate a conflict file in ScanRemoteChange if even user just
            // touch a file. Ex:
            // 1. Sync Client1 & Client2
            // 2. Client1 touch file1.txt (if not update mtime into syncHistoryTree)
            // 3. Client1 sync to server (actually nothing updated)
            // 3. Client2 modify file1.txt -> sync to server
            // 4. Then in Client1's download phase, we will find mtime is different between
            //    syncHistoryTree and local file, so conflict generated.

            // We don't want it always generate conflict files if users touch a file,
            // so update mtime to syncHistoryTree here
            rc = localDb.syncHistoryTree_updateLocalTime(currUploadEntry.parent_path,
                currUploadEntry.name, true, fileModifyTime);
            if (rc != 0) {
                LOG_CRITICAL("%p: syncHistoryTree_updateLocalTime:%d", this, rc);
                HANDLE_DB_FAILURE();
            }
            LOG_INFO("%p: ACTION UP upload skipped due to hash match:%s, new mtime("FMTu64")",
                    this, currEntryAbsPath.c_str(), fileModifyTime);
            return;
        }

        // Compute local file hash.
        // (optional optimization 1) Call VCS GET filemetadata.  If VCS hash == local file hash,
        //     (optional optimization 1) Update LocalDB with new timestamp, hash, set vcs_comp_id = compId,
        //                               and set vcs_revision = revision (for case when LocalDB was lost).
        //     (optional optimization 1) Proceed to next entry in ChangeLog.
        // (optional optimization 2) If (ChangeLog.vcs_comp_id != compId) || (ChangeLog.vcs_revision != revision),
        //     (optional optimization 2) Do conflict resolution.
        //     (optional optimization 2) Proceed to next entry in ChangeLog.

        std::string postFileMetadataUrl;
        std::string fileHashValue;
        if (type != SYNC_TYPE_TWO_WAY_HOST_ARCHIVE_STORAGE)
        {
            // Get the 3rd party storage URL (accessUrl) from VCS GET accessinfo (pass method=PUT).
            VcsAccessInfo accessInfoResp;
#if 0
            // TODO: P2: optional?
            string contentHash = "?";
            string contentType = "?";
#endif
            for(u32 retries=0; retries < NUM_IMMEDIATE_TRANSIENT_RETRY; ++retries)
            {
                VPLHttp2 httpHandle;
                if (setHttpHandleAndCheckForStop(httpHandle)) { endFunction_out=true; return; }
                rc = vcs_access_info_for_file_put(vcs_session,
                                                  httpHandle,
                                                  dataset,
                                                  verbose_http_log,
                                                  accessInfoResp);
                if (clearHttpHandleAndCheckForPauseStop(httpHandle)) { endFunction_out=true; return; }
                if (!isTransientError(rc)) { break; }
                LOG_WARN("%p: Transient Error:%d RetryIndex:%d", this, rc, retries);
                if (retries+1 < NUM_IMMEDIATE_TRANSIENT_RETRY) {  // pause between immediate retries
                    if(checkForPauseStop(true, QUICK_RETRY_INTERVAL)) {
                        endFunction_out = true;
                        return;
                    }
                }
            }
            if (rc < 0) {
                if (isTransientError(rc)) {
                    // Transient error; retry later.
                    errorInc_out = true;
                    LOG_WARN("%p: vcs_access_info_for_file_put failed: %d", this, rc);
                } else {
                    // Non-retryable; need to do another server scan.
                    setErrorNeedsRemoteScan();
                    LOG_ERROR("%p: vcs_access_info_for_file_put failed: %d", this, rc);
                }
                skip_out = true;
                return;
            }

            ////////  Upload file to archive storage ////////
            
            if (accessInfoResp.accessUrl.find("acer-ts://")==0) {
                // For the case of virtual sync upload to archive storage, first upload a 
                // metadata file. The metadata file contains information required 
                // for later ASD (Archive Storage Device) staging area cleanup process.
                VCSFileUrlAccess* vcsFileUrlAccess = dataset_access_info.fileUrlAccess;
                VcsAccessInfo metaAccessInfo = accessInfoResp;
                // The ASD metadata filename uses the original staging filename + .meta extension.
                // There is no chance for conflict, since the staging filenames are always
                // numeric filenames assigned by VCS.
                metaAccessInfo.accessUrl.append(ASD_CLEANUP_META_FILE_EXT);  
                std::string metafilePath = getTempSyncboxMetadataFilePath();

                {
                    // metadata filename uses the original staging filename with .meta extension
                    std::string uri_prefix = ARCHIVE_ACCESS_URI_SCHEME;
                    uri_prefix += ":/";
                    std::string uri = metaAccessInfo.accessUrl.substr(uri_prefix.length());
                    std::vector<std::string> uri_tokens;
                    VPLHttp_SplitUri(uri, uri_tokens);
                    metafilePath.append(uri_tokens[(int)uri_tokens.size() - 1]);
                }

                // Ensure the parent folder is there
                rc = Util_CreateParentDir(metafilePath.c_str());
                if (rc < 0) {
                    LOG_ERROR("%p: Util_CreateParentDir(%s) failed: %d", this, metafilePath.c_str(), rc);
                    errorInc_out = true; 
                    skip_out = true;
                    return;
                }

                {
                    ProtobufFileWriter writer;
                    rc = writer.open(metafilePath.c_str(), VPLFILE_MODE_IRUSR | VPLFILE_MODE_IWUSR);
                    if (rc < 0) {
                        LOG_ERROR("[%p]: Failed to open asd metadata file %s", this, metafilePath.c_str());
                        // This error is not expected. Skip this round, retry later and increment error count.
                        errorInc_out = true; 
                        skip_out = true;
                        return;
                    }

                    google::protobuf::io::CodedOutputStream tempStream(writer.getOutputStream());
                    if (currUploadEntryHasCompId) { 
                        rc = writeStagingMetadataFile(tempStream, 
                                                      currUploadEntryCompId, 
                                                      currUploadEntryRevisionToUpload, 
                                                      currUploadEntryRelPath.str());
                    } else {
                        rc = writeStagingMetadataFile(tempStream, currUploadEntryRelPath.str());
                    }

                    if (rc < 0) {
                        LOG_ERROR("[%p]: Failed to write asd metadata file %s", this, metafilePath.c_str());
                        errorInc_out = true;
                        skip_out = true;
                        return;
                    }
                }

                for (u32 retries=0; retries < NUM_IMMEDIATE_TRANSIENT_RETRY; ++retries)
                {
                    VCSFileUrlOperation* vcsFileUrlOperation = 
                        vcsFileUrlAccess->createOperation(metaAccessInfo,
                                                          verbose_http_log,
                                                          /*OUT*/ rc);
                    if (vcsFileUrlOperation == NULL) {
                        if (rc == CCD_ERROR_ARCHIVE_DEVICE_OFFLINE) {
                            LOG_WARN("vcsFileUrlOperation: archive storage offline");
                        } else {
                            LOG_WARN("vcsFileUrlOperation: error(%d). Continuing.", rc);
                        }
                    } else {
                        registerForAsyncCancel(vcsFileUrlOperation);
                        rc = vcsFileUrlOperation->PutFile(metafilePath);
                        unregisterForAsyncCancel(vcsFileUrlOperation);
                        vcsFileUrlAccess->destroyOperation(vcsFileUrlOperation);
                    }
                    if (!isTransientError(rc)) { break; }
                    LOG_WARN("%p: Archive Transient Error:%d RetryIndex:%d", this, rc, retries);
                    if (retries+1 < NUM_IMMEDIATE_TRANSIENT_RETRY) {  // pause between immediate retries
                        if (checkForPauseStop(true, QUICK_RETRY_INTERVAL)) {
                            endFunction_out=true;
                            return;
                        }
                    }
                }
                if (rc != 0) {
                    // We consider all errors from storage to be transient; we need to check with VCS
                    // GET accessinfo again later to determine if it is retryable or not.
                    LOG_WARN("%p: vcsFileUrlOperation->PutFile returned %d", this, rc);
                    // Retry the upload again later.
                    errorInc_out = true;
                    skip_out = true;
                    return;
                }
                // Remove the local temp metadata file
                rc = VPLFile_Delete(metafilePath.c_str());
                if (rc != 0 && rc != VPL_ERR_NOENT) {
                    // No reason to fail. Just log the error for now.
                    LOG_ERROR("%p: VPLFile_Delete failed %d for %s", this, rc, metafilePath.c_str());
                }
            }

            VCSFileUrlAccess* vcsFileUrlAccess = dataset_access_info.fileUrlAccess;
            for (u32 retries=0; retries < NUM_IMMEDIATE_TRANSIENT_RETRY; ++retries)
            {
                VCSFileUrlOperation* vcsFileUrlOperation =
                    vcsFileUrlAccess->createOperation(accessInfoResp,
                                                      verbose_http_log,
                                                      /*OUT*/ rc);
                if(vcsFileUrlOperation == NULL) {
                    if (rc == CCD_ERROR_ARCHIVE_DEVICE_OFFLINE) {
                        LOG_WARN("vcsFileUrlOperation: archive storage offline");
                    } else {
                        LOG_WARN("vcsFileUrlOperation: error(%d). Continuing.", rc);
                    }
                } else {
                    registerForAsyncCancel(vcsFileUrlOperation);
                    if (sync_policy.how_to_compare == SyncPolicy::FILE_COMPARE_POLICY_USE_HASH) {
                        rc = vcsFileUrlOperation->PutFile(currEntryAbsPath.str(), fileHashValue);
                        if (rc == 0) {
                            LOG_INFO("Got Hash %s of file %s", fileHashValue.c_str(), currEntryAbsPath.c_str());
                        }
                    } else {
                        rc = vcsFileUrlOperation->PutFile(currEntryAbsPath.str());
                    }
                    unregisterForAsyncCancel(vcsFileUrlOperation);
                    vcsFileUrlAccess->destroyOperation(vcsFileUrlOperation);
                }
                if (!isTransientError(rc)) { break; }
                LOG_WARN("%p: Archive Transient Error:%d RetryIndex:%d", this, rc, retries);
                if (retries+1 < NUM_IMMEDIATE_TRANSIENT_RETRY) {  // pause between immediate retries
                    if (checkForPauseStop(true, QUICK_RETRY_INTERVAL)) {
                        endFunction_out=true;
                        return;
                    }
                }
            }
            if (rc != 0) {
                // We consider all errors from storage to be transient; we need to check with VCS
                // GET accessinfo again later to determine if it is retryable or not.
                LOG_WARN("%p: vcsFileUrlOperation->PutFile returned %d, errorInc_out = true", this, rc);
                // Retry the upload again later.
                errorInc_out = true;
                skip_out = true;
                return;
            }
            {
                postFileMetadataUrl = accessInfoResp.accessUrl;
                // We've use timestamp to check if file is modified while file uploading, so don't need to compare rolling hash here.
                // So simply compare last modified time.
                VPLTime_t modifyTime;
                // Stat the file to get its modified time.
                VPLFS_stat_t statBuf;
                rc = VPLFS_Stat(currEntryAbsPath.c_str(), &statBuf);
                if (rc < 0) {
                    // Dropping the file until we do another local scan.
                    if (rc == VPL_ERR_NOENT) {
                        // The file was probably deleted just now.
                        // There should be a file monitor notification (or app notification)
                        // coming soon to force us to scan again.
                        LOG_INFO("%p: File (%s) no longer exists", this, currEntryAbsPath.c_str());
                    } else {
                        LOG_WARN("%p: VPLFS_Stat(%s) failed: %d", this, currEntryAbsPath.c_str(), rc);
                        // TODO: Bug 11588: Instead of dropping, we could set uploadScanError and setErrorTimeout (to try
                        //   again automatically), but that seems more dangerous since the directory may
                        //   never become readable, and we'd keep doing a scan every 15 minutes.
                        //   If we had some way to track the specific entities
                        //   causing problems and backoff the retry time, it might be better.
                    }
                    skip_out = true;
                    return;
                }
                modifyTime = fsTimeToNormLocalTime(currEntryAbsPath, statBuf.vpl_mtime);
                if (modifyTime != fileModifyTime ||
                    statBuf.size != fileSize)
                {
                    LOG_INFO("%p: %s file changed during upload. Upload safely abandoned. "
                             "("FMTu64","FMTu64")->("FMTu64","FMTu64")",
                             this, currEntryAbsPath.c_str(),
                             fileModifyTime, (u64)fileSize,
                             modifyTime, (u64)statBuf.size);
                    skip_out = true;
                    return;
                }
            }
        } else {
            // If local device is syncbox archive storage device, do not pass accessUrl 
            // (i.e. VCS componentUri is NULL)
            hasAccessUrl = false;
            // Due to there is no file transferring, we need to compute hash here
            if (sync_policy.how_to_compare == SyncPolicy::FILE_COMPARE_POLICY_USE_HASH) {
                LOG_INFO("No PutFile to remote, so start to compute local file's hash value.");
                VPLTime_t time_start = VPLTime_GetTimeStamp();
                int hash_status =  computeMD5(currEntryAbsPath.c_str(), /*out*/fileHashValue);
                VPLTime_t time_end = VPLTime_GetTimeStamp();
                LOG_INFO("End of compute local file's hash value.");
                LOG_INFO("Spent "FMT_VPLTime_t"usec to compute hash for: %s",
                        VPLTime_DiffClamp(time_end, time_start), currEntryAbsPath.c_str());

                if (hash_status != 0) {
                    LOG_ERROR("computeMD5 errno:%d", hash_status);
                    fileHashValue.clear();
                }
            }
        }
        // Update VCS with the uploaded file data. Call VCS POST filemetadata.
        {
            DatasetRelPath currUploadEntryDatasetRelPath = getDatasetRelPath(currUploadEntry);
            u64 parentCompId;
            rc = getParentCompId(currUploadEntry, parentCompId);
            if (rc == SYNC_AGENT_ERR_STOPPING) {
                endFunction_out=true;
                return;
            }
            if (isTransientError(rc)) {
                // Transient error; retry this upload later.
                LOG_WARN("%p: getParentCompId, (%s,%s): %d",
                         this,
                         currUploadEntry.parent_path.c_str(),
                         currUploadEntry.name.c_str(), rc);
                errorInc_out = true;
                skip_out = true;
                return;
            } else if (rc != 0) {
                // Non-retryable; need to do another server scan.
                setErrorNeedsRemoteScan();
                LOG_ERROR("%p: getParentCompId, (%s,%s): %d",
                          this,
                          currUploadEntry.parent_path.c_str(),
                          currUploadEntry.name.c_str(), rc);
                skip_out = true;
                return;
            }
            rc = vcsPostFileMetadataAfterUpload(currUploadEntryDatasetRelPath.str(),
                                                parentCompId,
                                                currUploadEntryHasCompId,
                                                currUploadEntryCompId,
                                                currUploadEntryRevisionToUpload,
                                                normLocalTimeToVcsTime(fileModifyTime),
                                                normLocalTimeToVcsTime(fileCreateTime),
                                                fileSize,
                                                hasAccessUrl,
                                                postFileMetadataUrl, 
                                                currUploadEntry,
                                                fileHashValue);
            if (rc == 0) {
                // ACTION UP log printout already done in vcsPostFileMetadataAfterUpload
            } else if (rc == SYNC_AGENT_ERR_STOPPING) {
                endFunction_out=true;
                return;
            } else if (rc == VCS_ERR_CANT_CREATE_UPLOADREVISION) {
                // VCS_ERR_CANT_CREATE_UPLOADREVISION means the file has since
                // been deleted on VCS by another client.
                // We can automatically resolve this case by uploading our file.
                rc = conflictResolutionVcsFileDeletedDuringUpload(
                                   currUploadEntryDatasetRelPath.str(),
                                   parentCompId,
                                   normLocalTimeToVcsTime(fileModifyTime),
                                   normLocalTimeToVcsTime(fileCreateTime),
                                   fileSize,
                                   postFileMetadataUrl,
                                   currUploadEntry,
                                   fileHashValue);
                if (rc == 0) {
                    LOG_INFO("%p: ACTION UP upload done:%s", this, currEntryAbsPath.c_str());
                } else if (rc == SYNC_AGENT_ERR_STOPPING) {
                    endFunction_out=true;
                    return;
                } else if (isTransientError(rc)) {
                    // Transient error; retry this upload later.
                    LOG_WARN("%p: vcs_post_file_metadata(%d) parentCompId("FMTu64"),postFileMetadata(%s),compId("FMTu64")",
                            this, rc,
                            parentCompId, currUploadEntryDatasetRelPath.c_str(),
                            currUploadEntryHasCompId?currUploadEntryCompId:0);
                    errorInc_out = true;
                    skip_out = true;
                    return;
                } else {
                    // Non-retryable; need to do another server scan.
                    setErrorNeedsRemoteScan();
                    LOG_ERROR("%p: vcs_post_file_metadata(%d) parentCompId("FMTu64"),postFileMetadata(%s),compId("FMTu64")",
                            this, rc,
                            parentCompId, currUploadEntryDatasetRelPath.c_str(),
                            currUploadEntryHasCompId?currUploadEntryCompId:0);
                    skip_out = true;
                    return;
                }
            } else if (isTransientError(rc)) {
                // Transient error; retry this upload later.
                LOG_WARN("%p: vcs_post_file_metadata(%d) parentCompId("FMTu64"),postFileMetadata(%s),compId("FMTu64")",
                        this, rc,
                        parentCompId, currUploadEntryDatasetRelPath.c_str(),
                        currUploadEntryHasCompId?currUploadEntryCompId:0);
                errorInc_out = true;
                skip_out = true;
                return;
            } else if(rc == VCS_ERR_UPLOADREVISION_NOT_HWM_PLUS_1 ||
                      rc == VCS_ERR_BASEREVISION_NOT_FOUND ||
                      rc == VCS_ERR_COMPONENT_ALREADY_EXISTS_AT_PATH ||
                      rc == VCS_ERR_COMPONENT_NOT_FOUND ||
                      rc == VCS_ERR_PATH_DOESNT_POINT_TO_KNOWN_COMPONENT ||
                      rc == SYNC_AGENT_ERR_FAIL)
            {
                LOG_WARN("%p: vcs_post_file_metadata(%d) Conflict? parentCompId("FMTu64"),postFileMetadata(%s),compId("FMTu64")",
                        this, rc,
                        parentCompId, currUploadEntryDatasetRelPath.c_str(),
                        currUploadEntryHasCompId?currUploadEntryCompId:0);

                // TODO: If VCS POST filemetadata fails with CONFLICT_DETECTED (or an equivalent-for-our-purposes error
                // TODO:    do conflict resolution

                // TODO: handle it
                // Possible cases:
                // - parent directory was removed by another client.
                // - parent directory was removed and re-added by other clients.
                // - ??
                skip_out = true; // retry would not help here.
                return;
            } else {
                // Unexpected non-retryable error; need to do another server scan.
                setErrorNeedsRemoteScan();
                LOG_ERROR("%p: vcs_post_file_metadata(%d): parentCompId("FMTu64"),postFileMetadata(%s),compId("FMTu64")",
                        this, rc,
                        parentCompId, currUploadEntryDatasetRelPath.c_str(),
                        currUploadEntryHasCompId?currUploadEntryCompId:0);
                skip_out = true;
                return;
            }
        }
    }

    int getNextUploadChangeLog(bool isErrorMode,
                               u64 errorModeMaxRowId,
                               SCRow_uploadChangeLog& entry_out)
    {
        int rc = 0;
        if(isErrorMode) {
            rc = localDb.uploadChangeLog_getErr(errorModeMaxRowId, entry_out);
        }else {
            rc = localDb.uploadChangeLog_get(entry_out);
        }
        return rc;
    }

    /// Applies the local changes to the server.
    void ApplyUploadChangeLog(bool isErrorMode)
    {
        int rc;
        SCRow_uploadChangeLog currUploadEntry;
        u64 errorModeMaxRowId = 0;  // Only valid when isErrorMode
        ASSERT(!VPLMutex_LockedSelf(&mutex));

        if (isErrorMode) {
            rc = localDb.uploadChangeLog_getMaxRowId(errorModeMaxRowId);
            if(rc != 0) {
                LOG_CRITICAL("%p: Should never happen:uploadChangeLog_getMaxRowId,%d", this, rc);
                HANDLE_DB_FAILURE();
                return;
            }
        }
        // For each entry in UploadChangeLog:
        while ((rc = getNextUploadChangeLog(isErrorMode,
                                            errorModeMaxRowId,
                                            currUploadEntry)) == 0)
        {
            bool skip_ErrInc = false;  // When true, appends to error.
            if (!isValidDirEntry(currUploadEntry.name)) {
                FAILED_ASSERT("Bad entry added to upload change log: \"%s\"", currUploadEntry.name.c_str());
                goto skip;
            }
            if (checkForPauseStop()) {
                return;
            }

            switch(currUploadEntry.upload_action)
            {
                case UPLOAD_ACTION_CHANGE_FILE_OR_DIR_TYPE:
                case UPLOAD_ACTION_CREATE:
                case UPLOAD_ACTION_UPDATE:
                case UPLOAD_ACTION_DELETE:
                    break;
                default:
                    FAILED_ASSERT("Unexpected upload action:"FMTu64, currUploadEntry.upload_action);
            }

            // Case Remove File or Dir:
            if (currUploadEntry.upload_action == UPLOAD_ACTION_DELETE ||
                currUploadEntry.upload_action == UPLOAD_ACTION_CHANGE_FILE_OR_DIR_TYPE)
            {
                DatasetRelPath currEntryDatasetRelPath = getDatasetRelPath(currUploadEntry);
                // If changelog entry is dir; call VCS DELETE dir (compId).
                if (currUploadEntry.is_dir) {
                    ASSERT(currUploadEntry.comp_id_exists);
                    LOG_INFO("%p: ACTION UP delete begin (dir):%s", this, currEntryDatasetRelPath.c_str());
                    for (u32 retries=0; retries < NUM_IMMEDIATE_TRANSIENT_RETRY; ++retries)
                    {
                        VPLHttp2 httpHandle;
                        if (setHttpHandleAndCheckForStop(httpHandle)) { return; }
                        rc = vcs_delete_dir(vcs_session, httpHandle, dataset,
                                            currEntryDatasetRelPath.str(),
                                            currUploadEntry.comp_id,
                                            false,    // Not recursive
                                            false, 0, // No dataset version
                                            verbose_http_log);
                        if (clearHttpHandleAndCheckForPauseStop(httpHandle)) { return; }
                        if (!isTransientError(rc)) { break; }
                        LOG_WARN("%p: Transient Error:%d RetryIndex:%d", this, rc, retries);
                        if (retries+1 < NUM_IMMEDIATE_TRANSIENT_RETRY) {  // pause between immediate retries
                            if(checkForPauseStop(true, QUICK_RETRY_INTERVAL)) {
                                return;
                            }
                        }
                    }
                } else { // Entry is file; call VCS DELETE file (compId, revision).
                    LOG_INFO("%p: ACTION UP delete begin (file):%s,compId("FMTu64"),rev("FMTu64")",
                             this,
                             currEntryDatasetRelPath.c_str(),
                             currUploadEntry.comp_id,
                             currUploadEntry.revision);
                    ASSERT(currUploadEntry.comp_id_exists);
                    ASSERT(currUploadEntry.revision_exists);
                    for (u32 retries=0; retries < NUM_IMMEDIATE_TRANSIENT_RETRY; ++retries)
                    {
                        VPLHttp2 httpHandle;
                        if (setHttpHandleAndCheckForStop(httpHandle)) { return; }
                        rc = vcs_delete_file(vcs_session, httpHandle, dataset,
                                             currEntryDatasetRelPath.str(),
                                             currUploadEntry.comp_id,
                                             currUploadEntry.revision,
                                             verbose_http_log);
                        if (clearHttpHandleAndCheckForPauseStop(httpHandle)) { return; }
                        if (!isTransientError(rc)) { break; }
                        LOG_WARN("%p: Transient Error:%d RetryIndex:%d", this, rc, retries);
                        if (retries+1 < NUM_IMMEDIATE_TRANSIENT_RETRY) {  // pause between immediate retries
                            if(checkForPauseStop(true, QUICK_RETRY_INTERVAL)) {
                                return;
                            }
                        }
                    }
                }
                // Expected errors from DELETE dir:
                // 5316 - VCS_PROVIDED_PATH_DOESNT_MATCH_PROVIDED_COMPID: The <path> in the requestURI and the <compId> in the request do not match
                // - This case is ambiguous.  It means either:
                //   1. We asked to delete a dir that doesn't exist (which is what we want anyway), or
                //   2. We asked to delete a dir that currently exists, but we passed the wrong compId.
                // 5317 - VCS_TARGET_FOLDER_NOT_EMPTY: Cannot delete a folder that is not empty.  (Please delete children first)
                // - Implies that another client added something within the directory.
                //
                // Expected errors from DELETE file:
                // 5316 - VCS_PROVIDED_PATH_DOESNT_MATCH_PROVIDED_COMPID: The <path> in the requestURI and the <compId> in the request do not match
                // - This case is ambiguous.  It means either:
                //   1. We asked to delete a file that doesn't exist (which is what we want anyway), or
                //   2. We asked to delete a file that currently exists, but we passed the wrong compId.
                //   TODO: If we care about this case, we should rescan VCS so that we can actually remove
                //     the file.
                // 5325 - VCS_REVISION_NOT_FOUND: The <revision> in the request parameter is not found
                // - Implies that another client modified the file.
                //   TODO: If we care about this case, we should rescan VCS so that we can actually remove
                //     the file.
                if (rc == 0)
                { // It succeeded.
                    // Remove entry from ChangeLog and remove entry from LocalDB.
                    rc = localDb.syncHistoryTree_remove(currUploadEntry.parent_path,
                                                        currUploadEntry.name);
                    if(rc != 0) {
                        if (rc == SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) {
                            // TODO: Is this case ever expected?
                            LOG_ERROR("%p: (%s,%s) was not found in localDb",
                                    this,
                                    currUploadEntry.parent_path.c_str(),
                                    currUploadEntry.name.c_str());
                        } else {
                            LOG_CRITICAL("%p: syncHistoryTree_remove(%s,%s) failed: %d",
                                    this,
                                    currUploadEntry.parent_path.c_str(),
                                    currUploadEntry.name.c_str(),
                                    rc);
                            HANDLE_DB_FAILURE();
                        }
                    }
                    LOG_INFO("%p: ACTION UP delete done:%s", this, currEntryDatasetRelPath.c_str());
                    reportFileUploaded(true, getRelPath(currUploadEntry),
                            currUploadEntry.is_dir, false);
                    rc = updateUploadsRemaining();
                    if (rc != 0) {
                        LOG_ERROR("updateUploadsRemaining:%d", rc);
                    }
                } else if (rc == VCS_ERR_PROVIDED_PATH_DOESNT_MATCH_PROVIDED_COMPID ||
                           rc == VCS_ERR_TARGET_FOLDER_NOT_EMPTY ||
                           rc == VCS_ERR_PATH_DOESNT_POINT_TO_KNOWN_COMPONENT ||
                           rc == VCS_ERR_REVISION_NOT_FOUND)
                {
                    // TODO: If CONFLICT_DETECTED error... still incomplete.
                    // TODO: Perform conflict resolution during applyDownloadChangeLog.
                    // For "First uploaded wins, preserve loser":
                    // - Skip, take no action. The download phase will handle this conflict.
                    LOG_WARN("%p: Conflict:%d, %s", this, rc, currEntryDatasetRelPath.c_str());
                    goto skip;
                } else if (isTransientError(rc)) {
                    // Transient error; retry this delete action later.
                    LOG_WARN("%p: Transient err during UPLOAD_ACTION_DELETE:%d, %s",
                            this, rc, currEntryDatasetRelPath.c_str());
                    skip_ErrInc = true;
                    goto skip;
                } else {
                    // Non-retryable; need to do another server scan.
                    setErrorNeedsRemoteScan();
                    LOG_ERROR("%p: Unexpected err during UPLOAD_ACTION_DELETE:%d, %s",
                            this, rc, currEntryDatasetRelPath.c_str());
                    goto skip;
                }

                // Case Change from "File to Dir" or "Dir to File"
                if (currUploadEntry.upload_action == UPLOAD_ACTION_CHANGE_FILE_OR_DIR_TYPE) {
                    currUploadEntry.upload_action = UPLOAD_ACTION_CREATE;
                    currUploadEntry.is_dir = !currUploadEntry.is_dir;  // Flip the type
                    // Because we're creating anew, component and revision ID not needed.
                    currUploadEntry.comp_id_exists = false;
                    currUploadEntry.revision_exists = false;
                } // Fall through, expect to Create.
            }

            // Case Add Directory:
            if ((currUploadEntry.upload_action == UPLOAD_ACTION_CREATE) && currUploadEntry.is_dir) {
                // Call VCS POST dir.
                u64 parentCompId;
                rc = getParentCompId(currUploadEntry, parentCompId);
                if (checkForPauseStop()) {
                    return;
                }
                if (isTransientError(rc)) {
                    LOG_WARN("%p: getParentCompId, (%s,%s): %d",
                             this,
                             currUploadEntry.parent_path.c_str(),
                             currUploadEntry.name.c_str(), rc);
                    skip_ErrInc = true;
                    goto skip;
                } else if (rc != 0) {
                    // This is a non-retryable error.  We should scan VCS again later.
                    LOG_ERROR("%p: getParentCompId, (%s,%s): %d",
                              this,
                              currUploadEntry.parent_path.c_str(),
                              currUploadEntry.name.c_str(), rc);
                    setErrorNeedsRemoteScan();
                    goto skip;
                }
                VcsMakeDirResponse vcsMakeDirOut;
                DatasetRelPath currUploadEntryPath = getDatasetRelPath(currUploadEntry);
                LOG_INFO("%p: ACTION UP mkdir %s", this, currUploadEntryPath.c_str());
                for (u32 retries=0; retries < NUM_IMMEDIATE_TRANSIENT_RETRY; ++retries)
                {
                    VPLHttp2 httpHandle;
                    if (setHttpHandleAndCheckForStop(httpHandle)) { return; }
                    rc = vcs_make_dir(vcs_session, httpHandle, dataset,
                            currUploadEntryPath.str(),
                            parentCompId,
                            DEFAULT_MYSQL_DATE, //infoLastChanged
                            DEFAULT_MYSQL_DATE, //infoCreateDate
                            my_device_id(),
                            verbose_http_log,
                            vcsMakeDirOut);
                    if (clearHttpHandleAndCheckForPauseStop(httpHandle)) { return; }
                    if (!isTransientError(rc)) { break; }
                    LOG_WARN("%p: Transient Error:%d RetryIndex:%d", this, rc, retries);
                    if (retries+1 < NUM_IMMEDIATE_TRANSIENT_RETRY) {  // pause between immediate retries
                        if(checkForPauseStop(true, QUICK_RETRY_INTERVAL)) {
                            return;
                        }
                    }
                }
                if (rc == 0) {
                    // On success, add directory to LocalDB and set vcs_comp_id = compId returned by VCS.
                    SCRow_syncHistoryTree newEntry;
                    newEntry.parent_path = currUploadEntry.parent_path;
                    newEntry.name = currUploadEntry.name;
                    newEntry.is_dir = currUploadEntry.is_dir;
                    newEntry.comp_id_exists = true;
                    newEntry.comp_id = vcsMakeDirOut.compId;
                    rc = localDb.syncHistoryTree_add(newEntry);
                    if (rc < 0) {
                        LOG_CRITICAL("%p: syncHistoryTree_add(%s) failed: %d", this, currUploadEntryPath.c_str(), rc);
                        HANDLE_DB_FAILURE();
                        goto skip;  // skip, retry doesn't have much hope to recover.
                    }
                    reportFileUploaded(false, getRelPath(newEntry),
                            newEntry.is_dir, true);
                } else if (isTransientError(rc)) {
                    LOG_WARN("%p: vcs_post_dir transient error: %d", this, rc);
                    // Transient error; retry later.
                    skip_ErrInc = true;
                    goto skip;
                } else {
                    // Non-retryable; need to do another server scan.
                    if (rc == VCS_PROVIDED_FOLDER_PATH_DOESNT_MATCH_PROVIDED_PARENTCOMPID){
                        LOG_INFO("%p: vcs_post_dir returned %d", this, rc);
                        // Conflict!
                        // 1. Parent dir may have been removed and recreated by another client.
                        //    Drop this, and rescan the server.
                        // TODO: technically, we probably shouldn't wait 15 minutes, but there's
                        //   a risk in trying again too soon.
                        setErrorNeedsRemoteScan();
                    } else {
                        // Non-retryable; need to do another server scan.
                        setErrorNeedsRemoteScan();
                        LOG_ERROR("%p: vcs_post_dir returned %d", this, rc);
                    }
                    goto skip;
                }
            }
            // Case Add File or Update File:
            else if (!currUploadEntry.is_dir &&
                     ( (currUploadEntry.upload_action == UPLOAD_ACTION_CREATE) ||
                       (currUploadEntry.upload_action == UPLOAD_ACTION_UPDATE)))
            {
                bool skip = false;
                bool endFunction = false;

                ApplyUploadCreateOrUpdateToAcs(currUploadEntry,
                                               /*OUT*/ skip_ErrInc,
                                               /*OUT*/ skip,
                                               /*OUT*/ endFunction);
                if (endFunction) {
                    return;
                } else if (skip) {
                    // Don't perform any additional steps for the currUploadEntry,
                    // if there are any.  Currently there are no additional steps.
                    goto skip;
                }
            }
          skip:
            if(skip_ErrInc) {
                if(currUploadEntry.upload_err_count > sync_policy.max_error_retry_count) {
                    LOG_ERROR("%p: Abandoning error:%s,%s,"FMTu64",%d,"FMTu64
                              " -- Attempted "FMTu64" times",
                              this,
                              currUploadEntry.parent_path.c_str(),
                              currUploadEntry.name.c_str(),
                              currUploadEntry.comp_id,
                              currUploadEntry.revision_exists,
                              currUploadEntry.revision,
                              currUploadEntry.upload_err_count);
                    rc = uploadChangeLog_remove_and_updateRemaining(currUploadEntry.row_id);
                    if (rc < 0) {
                        LOG_ERROR("%p: uploadChangeLog_remove_and_updateRemaining("
                                FMTu64") failed: %d", this, currUploadEntry.row_id, rc);
                        return;
                    }
                } else {
                    LOG_INFO("%p: uploadChangeLog_incErr for (%s,%s)",
                             this, currUploadEntry.parent_path.c_str(), currUploadEntry.name.c_str());
                    rc = localDb.uploadChangeLog_incErr(currUploadEntry.row_id);
                    if (rc < 0) {
                        LOG_ERROR("%p: uploadChangeLog_incErr("FMTu64") failed: %d",
                                  this, currUploadEntry.row_id, rc);
                        HANDLE_DB_FAILURE();
                        return;
                    }

                    MutexAutoLock lock(&mutex);
                    setErrorTimeout(sync_policy.error_retry_interval);
                }
            } else {  // uploadChangeLog succeeded.
                rc = uploadChangeLog_remove_and_updateRemaining(currUploadEntry.row_id);
                if (rc < 0) {
                    LOG_ERROR("%p: uploadChangeLog_remove_and_updateRemaining("
                            FMTu64") failed: %d", this, currUploadEntry.row_id, rc);
                    return;
                }
            }
        } // while ((rc = uploadChangeLog_get(currEntry)) == 0)
        if (rc != SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) {
            LOG_CRITICAL("%p: uploadChangeLog_get() failed: %d", this, rc);
            HANDLE_DB_FAILURE();
        }
    }

    /// Caller is responsible for handling error.
    int vcsPostFileMetadataAfterUpload(const std::string& currUploadEntryDatasetRelPath,
                                       u64 parentCompId,
                                       bool currUploadEntryHasCompId,
                                       u64 currUploadEntryCompId,
                                       u64 currUploadEntryRevisionToUpload,
                                       VPLTime_t modifyTime,
                                       VPLTime_t createTime,
                                       u64 fileSize,
                                       bool hasAccessUrl,
                                       const std::string& accessUrl,
                                       const SCRow_uploadChangeLog& currUploadEntry,
                                       const std::string& fileHashValue)
    {
        int rv = 0;
        VcsFileMetadataResponse fileMetadataResp;
        for (u32 retries=0; retries < NUM_IMMEDIATE_TRANSIENT_RETRY; ++retries)
        {
            VPLHttp2 httpHandle;
            if (setHttpHandleAndCheckForStop(httpHandle)) { return SYNC_AGENT_ERR_STOPPING; }
            rv = vcs_post_file_metadata(vcs_session,
                                        httpHandle,
                                        dataset,
                                        currUploadEntryDatasetRelPath,
                                        parentCompId,
                                        currUploadEntryHasCompId,
                                        currUploadEntryCompId,
                                        currUploadEntryRevisionToUpload,
                                        modifyTime,
                                        createTime,
                                        fileSize,
                                        std::string("") /* TODO: contentHash */,
                                        fileHashValue,
                                        my_device_id(),
                                        hasAccessUrl,
                                        accessUrl,
                                        verbose_http_log,
                                        fileMetadataResp);
            if (clearHttpHandleAndCheckForPauseStop(httpHandle)) { return SYNC_AGENT_ERR_STOPPING; }
            if (!isTransientError(rv)) { break; }
            LOG_WARN("%p: Transient Error:%d RetryIndex:%d", this, rv, retries);
            if (retries+1 < NUM_IMMEDIATE_TRANSIENT_RETRY) {  // pause between immediate retries
                if(checkForPauseStop(true, QUICK_RETRY_INTERVAL)) {
                    return SYNC_AGENT_ERR_STOPPING;
                }
            }
        }
        // If VCS POST filemetadata succeeds, update LocalDB entry with the new modified time, vcs_comp_id, and vcs_revision.
        if (rv == 0) {
            if ((fileMetadataResp.numOfRevisions == 0) ||
                (fileMetadataResp.revisionList.size()==0))
            {  // 0 revisions?!?!
                LOG_ERROR("%p: fileMetadataResp has 0 revisions (%s, "FMTu64","FMTu64", "FMTu_size_t")",
                          this,
                          fileMetadataResp.name.c_str(),
                          fileMetadataResp.compId,
                          fileMetadataResp.numOfRevisions,
                          fileMetadataResp.revisionList.size());
                return SYNC_AGENT_ERR_FAIL;
            } else if (fileMetadataResp.numOfRevisions != 1) {
                LOG_ERROR("%p: fileMetadataResp multiple revisions (%s, "FMTu64","FMTu64", "FMTu_size_t"), arbitrarily using 1st one",
                          this,
                          fileMetadataResp.name.c_str(),
                          fileMetadataResp.compId,
                          fileMetadataResp.numOfRevisions,
                          fileMetadataResp.revisionList.size());
            }

            SCRow_syncHistoryTree uploadedEntry;
            uploadedEntry.parent_path = currUploadEntry.parent_path;
            uploadedEntry.name = currUploadEntry.name;
            uploadedEntry.is_dir = false;
            uploadedEntry.comp_id_exists = true;
            uploadedEntry.comp_id = fileMetadataResp.compId;
            uploadedEntry.revision_exists = true;
            uploadedEntry.revision = fileMetadataResp.revisionList[0].revision;
            uploadedEntry.local_mtime_exists = true;
            uploadedEntry.local_mtime = VPLTime_ToMicrosec(fileMetadataResp.lastChanged);
            uploadedEntry.last_seen_in_version_exists = false;
            uploadedEntry.version_scanned_exists = false;
            uploadedEntry.is_on_acs = true;
            uploadedEntry.hash_value = fileHashValue;
            uploadedEntry.file_size_exists = !fileHashValue.empty();
            uploadedEntry.file_size = fileSize;
            rv = localDb.syncHistoryTree_add(uploadedEntry);
            if (rv != 0) {
                LOG_CRITICAL("%p: syncHistoryTree_add:%d", this, rv);
                HANDLE_DB_FAILURE();
            }
            LOG_INFO("%p: ACTION UP upload done:(%s,%s) (compId:"FMTu64
                     ", rev:"FMTu64", lastChangedNano:"FMTu64
                     ", parentFolderCompId:"FMTu64")",
                     this, uploadedEntry.parent_path.c_str(),
                     uploadedEntry.name.c_str(),
                     uploadedEntry.comp_id,
                     uploadedEntry.revision,
                     uploadedEntry.local_mtime,
                     parentCompId);
            reportFileUploaded(false, getRelPath(uploadedEntry),
                    uploadedEntry.is_dir, !currUploadEntryHasCompId);
        }
        return rv;
    }

    int deleteLocalFile(const SCRow_syncHistoryTree& toDelete)
    {
        int rc;
        bool entryRemoved = false;
        SCRelPath scRelPath = getRelPath(toDelete);
        AbsPath toStat = getAbsPath(scRelPath);
        VPLTime_t fileModifyTime;

        if (toDelete.is_dir) {
            LOG_WARN("%p: Cannot Delete Local file (%s). It is a dir in DB. Conflict?",
                     this, toStat.c_str());
            return -1;
        }
        ASSERT(toDelete.comp_id_exists);

        VPLFS_stat_t statBuf;
        rc = VPLFS_Stat(toStat.c_str(), &statBuf);
        if (rc == VPL_ERR_NOENT) {
            // already missing
            entryRemoved = true;
            goto remove_entry;
        } else if (rc != 0) {
            // Do not delete when anything doesn't work out.
            // TODO: Bug 12976: Possible correctness issue when this case is hit?
            //   I can't convince myself that it's safe to ignore failed download deletions here.
            //   What stops the afflicted client from accidentally reuploading the file during
            //   the upload scan?
            LOG_ERROR("%p: VPLFS_Stat:%d, %s", this, rc, toStat.c_str());
            return rc;
        }

        if(!toDelete.revision_exists || !toDelete.local_mtime_exists) {
            // Bug 10646 Comment 11:  This case can be plausibly hit when
            // 1) Scan_1 happens
            // 2) download does not happen (File is removed from vcs, hard restart, network, etc)
            // 3) File is removed from VCS and Scan_2 happens.
            LOG_WARN("%p: ACTION DOWN noDelete Pre-existing file found -- never downloaded, skipping:"
                     "(%s) compId("FMTu64"),mtime(%d,"FMTu64"),rev(%d,"FMTu64")",
                     this,
                     toStat.c_str(),
                     toDelete.comp_id,
                     toDelete.local_mtime_exists?1:0, toDelete.local_mtime,
                     toDelete.revision_exists?1:0, toDelete.revision);

            // TODO: Download conflict, should server win or local win?
            // For now, default to preserve data, upload scan should pick this up.
            entryRemoved = true;
            goto remove_entry;
        }

        fileModifyTime = fsTimeToNormLocalTime(toStat, statBuf.vpl_mtime);
        if(statBuf.type == VPLFS_TYPE_FILE &&
           toDelete.local_mtime == fileModifyTime)
        {
            LOG_INFO("%p: ACTION DOWN delete file %s mtime("FMTu64","FMTu64"),compId("FMTu64"),rev("FMTu64")",
                    this, toStat.c_str(),
                    toDelete.local_mtime, fileModifyTime,
                    toDelete.comp_id, toDelete.revision);
            rc = VPLFile_Delete(toStat.c_str());
            if(rc == VPL_ERR_NOENT) {
                entryRemoved = true;
            }else if(rc != 0) {
                LOG_WARN("%p: VPLFile_Delete, skipping:%d, %s",
                        this, rc, toStat.c_str());
                // TODO: Bug 12976: Possible correctness issue when this case is hit?
                //   I can't convince myself that it's safe to ignore failed download deletions here.
                //   What stops the afflicted client from accidentally reuploading the file during
                //   the upload scan?
                return rc;
            }else{
                entryRemoved = true;
            }
        }else{
            // Should not delete.
            LOG_INFO("%p: ACTION DOWN noDelete. File changed, will not delete %s: isDir:%d, type:%d, "
                     "mtime("FMTu64","FMTu64"),compId("FMTu64"),rev("FMTu64")",
                     this,
                     toStat.c_str(),
                     toDelete.is_dir,
                     statBuf.type,
                     toDelete.local_mtime,
                     fileModifyTime,
                     toDelete.comp_id,
                     toDelete.revision);
            return -1;
        }

      remove_entry:
        if(entryRemoved) {
            rc = localDb.syncHistoryTree_remove(toDelete.parent_path, toDelete.name);
            if(rc != 0) {
                LOG_CRITICAL("%p: syncHistoryTree_remove(%s,%s):%d",
                          this,
                          toDelete.parent_path.c_str(),
                          toDelete.name.c_str(),
                          rc);
                HANDLE_DB_FAILURE();
                return rc;
            }
            // We "downloaded" a deletion; we need to report this.
            reportFileDownloaded(true, scRelPath, toDelete.is_dir, false);
        }
        return 0;
    }

    /// Removes any entries in \a directories that are an ancestor of \a path.
    void removeAllParentPaths(const SCRelPath& path,
                              std::vector<SCRow_syncHistoryTree>& directories)
    {
        string pathStr = path.str();
        std::vector<SCRow_syncHistoryTree>::iterator dirIt = directories.begin();

        while(dirIt != directories.end())
        {
            SCRow_syncHistoryTree& currDir = *dirIt;
            std::string dirPath = getRelPath(currDir).str();
            dirPath.append("/");
            if(pathStr.find(dirPath) == 0) {
                // match found, removing the directory from being removed
                // (the removal would result in an error anyways)
                dirIt = directories.erase(dirIt);
            }else{
                ++dirIt;
            }
        }
    }

    void deleteLocalDir(const SCRow_syncHistoryTree& directory)
    {
        // Recursively delete all known entities from the given directory.
        int rc;
        ASSERT(directory.is_dir);
        ASSERT(directory.name.size() > 0); // We should never try to delete the sync config root.
        std::vector<SCRow_syncHistoryTree> traversedDirsStack;
        {
            std::deque<SCRow_syncHistoryTree> dirsToTraverseQ;
            dirsToTraverseQ.push_back(directory);
            while(!dirsToTraverseQ.empty())
            {
                SCRow_syncHistoryTree toTraverse = dirsToTraverseQ.front();
                dirsToTraverseQ.pop_front();

                std::vector<SCRow_syncHistoryTree> dirEntries_out;
                SCRelPath currDirPath = getRelPath(toTraverse);
                rc = localDb.syncHistoryTree_getChildren(currDirPath.str(), dirEntries_out);
                if ((rc != 0) && (rc != SYNC_AGENT_DB_ERR_ROW_NOT_FOUND)) {
                    LOG_CRITICAL("%p: syncHistoryTree_get(%s) failed: %d",
                            this, currDirPath.c_str(), rc);
                    HANDLE_DB_FAILURE();
                    continue;
                }
                bool deleteError = false;
                for(std::vector<SCRow_syncHistoryTree>::iterator it = dirEntries_out.begin();
                    it != dirEntries_out.end();
                    ++it)
                {
                    SCRow_syncHistoryTree& currDirEntry = *it;
                    if(isRoot(currDirEntry)){ continue; } // Skip root exception case

                    if(currDirEntry.is_dir) {
                        dirsToTraverseQ.push_back(currDirEntry);
                    }else{
                        // No need to delete files in depth first order.  Just delete now.
                        rc = deleteLocalFile(currDirEntry);
                        if(rc != 0) {
                            LOG_WARN("%p: Cannot delete (%s,%s):%d.  No need to "
                                     "delete parent directories.",
                                     this,
                                     currDirEntry.parent_path.c_str(),
                                     currDirEntry.name.c_str(),
                                     rc);
                            deleteError = true;
                            SCRelPath path = getRelPath(currDirEntry);
                            // No need to attempt to delete ancestor directories with
                            // elements still present here.
                            removeAllParentPaths(path, traversedDirsStack);
                        }
                    }
                }
                if(!deleteError) {
                    traversedDirsStack.push_back(toTraverse);
                } // else { // No need to attempt to delete parent path }
            }  // while(!dirsToTraverseQ.empty())
        }

        // Delete the directories;
        while(!traversedDirsStack.empty())
        {
            SCRow_syncHistoryTree dirToDelete = traversedDirsStack.back();
            traversedDirsStack.pop_back();
            ASSERT(dirToDelete.is_dir);

            SCRelPath dirToRmPath = getRelPath(dirToDelete);
            AbsPath dirToRmAbsPath = getAbsPath(dirToRmPath);
            LOG_INFO("%p: ACTION DOWN Delete dir %s", this, dirToRmAbsPath.c_str());
            rc = VPLFS_Rmdir(dirToRmAbsPath.c_str());
            if(rc == VPL_ERR_NOENT) {
                // Already gone, still need to remove entry from syncHistory.
            }else if(rc != 0) {
                LOG_WARN("%p: Could not remove %s:%d, not removing parents",
                        this, dirToRmPath.c_str(), rc);
                // No need to attempt to delete ancestor directories with
                // elements still present here.
                removeAllParentPaths(dirToRmPath, traversedDirsStack);
                // Do not remove from the syncHistoryTree for this entry,
                // Skip to next entry.
                continue;
            }
            rc = localDb.syncHistoryTree_remove(dirToDelete.parent_path,
                                                dirToDelete.name);
            if(rc != 0) {
                LOG_CRITICAL("%p: Should never happen: syncHistoryTree_remove(%s,%s):%d",
                        this,
                        dirToDelete.parent_path.c_str(),
                        dirToDelete.name.c_str(),
                        rc);
                HANDLE_DB_FAILURE();
            }
            // We will report "delete dir" Downloaded here for
            //  each type of syncConfig
            reportFileDownloaded(true, dirToRmPath, true, false);
        }  // while(!traversedDirsStack.empty())
    }

    // Locally creates a directory and updates the local DB.
    void localMakeDir(const std::string& parentPath,
                      const VcsFolder& folder,
                      u64 currentDirVersion,
                      bool& skipDir)
    {
        // Create dir record in LocalDB (vcs_comp_id = compId).
        SCRow_syncHistoryTree newEntry;
        newEntry.parent_path = parentPath;
        newEntry.name = folder.name;
        newEntry.is_dir = true;
        newEntry.comp_id_exists = true;
        newEntry.comp_id = folder.compId;
        newEntry.last_seen_in_version_exists = true;
        newEntry.last_seen_in_version = currentDirVersion;
        int rc = localDb.syncHistoryTree_add(newEntry);
        if (rc < 0) {
            LOG_CRITICAL("%p: syncHistoryTree_add(%s, %s) failed: %d",
                    this, parentPath.c_str(), folder.name.c_str(), rc);
            HANDLE_DB_FAILURE();
            // Retrying probably won't help, so not setting errorOccurred.
            goto skip_dir;
        }

        // Create local dir if it doesn't exist.
        // (Optional, but should improve responsiveness to user.  Also, if we
        // don't do this here, we will need to add the directory to the download
        // change log, or empty directories will not be created.)
        // We also rely on this behavior to determine if a directory was actually
        // deleted by the local user vs. never downloaded (search this file
        // for "NOTE-130326").
        {
            AbsPath currEntryAbsPath = getAbsPath(getRelPath(newEntry));
            LOG_INFO("%p: ACTION DOWN mkdir %s compId("FMTu64")",
                     this, currEntryAbsPath.c_str(), newEntry.comp_id);
            rc = Util_CreateDir(currEntryAbsPath.c_str());
            if (rc < 0) {
                LOG_WARN("%p: Util_CreateDir(%s) failed: %d",
                        this, currEntryAbsPath.c_str(), rc);
                // Generally safe to ignore this error; we will try again after
                // downloading the first file or subdirectory.
                // Possible minor issue: if the directory is empty on VCS, then
                // it will not be created.
            } else {
                // We will report "create dir" Downloaded here for
                //  each type of syncConfig
                reportFileDownloaded(false, getRelPath(newEntry), true, true);
            }
        }
        return;
     skip_dir:
        skipDir = true;
    }

    void addFileSyncHistoryTree(const std::string& parentPath,
                                const VcsFile& file,
                                u64 currentDirVersion,
                                bool& skipDir)
    {
        SCRow_syncHistoryTree newEntry;
        newEntry.parent_path = parentPath;
        newEntry.name = file.name;
        newEntry.is_dir = false;
        newEntry.comp_id_exists = true;
        newEntry.comp_id = file.compId;
        // newEntry.revision is not populated until download (or upload) actually occurs.
        // newEntry.local_mtime is not populated until download (or upload) actually occurs.
        // newEntry.hash_value is not populated until download (or upload) actually occurs.
        // newEntry.file_size is not populated until download (or upload) actually occurs.
        newEntry.hash_value = file.hashValue;
        newEntry.file_size = file.latestRevision.size;
        newEntry.last_seen_in_version_exists = true;
        newEntry.last_seen_in_version = currentDirVersion;
        newEntry.is_on_acs = !file.latestRevision.noAcs;
        int rc = localDb.syncHistoryTree_add(newEntry);
        if (rc < 0) {
            LOG_CRITICAL("%p: syncHistoryTree_add(%s, %s) failed: %d",
                    this, parentPath.c_str(), file.name.c_str(), rc);
            HANDLE_DB_FAILURE();
            // Retrying probably won't help, so not setting errorOccurred.
            goto skip_dir;
        }
        return;
     skip_dir:
        skipDir = true;
    }

    /// Populates the DownloadChangeLog by querying VCS and deletes local files.
    void ScanRemoteChangesAndApplyDeletes()
    {
        int rc;
        SCRow_needDownloadScan currScanDir;
        // While LocalDB.need_download_scan is not empty:
        while ((rc = localDb.needDownloadScan_get(currScanDir)) == 0) {
            // Let currDir = first element in LocalDB.need_download_scan.
            // Client finds changes since last successful sync by calling "VCS GET dir" for currDir.
            SCRelPath currScanDirRelPath = getRelPath(currScanDir);
            DatasetRelPath currScanDirDatasetRelPath = getDatasetRelPath(currScanDirRelPath);

            // VCS returns pages of fileList and currentDirVersion. (Repeat if multiple pages of results.)
            u64 filesSeen = 0;
            u64 startingDirVersion = -1; // -1 indicates uninitialized.
            bool errorOccurred = false;
            VcsGetDirResponse getDirResp;
            do
            {
                for (u32 retries=0; retries < NUM_IMMEDIATE_TRANSIENT_RETRY; ++retries)
                {
                    VPLHttp2 httpHandle;
                    if (setHttpHandleAndCheckForStop(httpHandle)) { return; }
                    rc = vcs_read_folder_paged(vcs_session, httpHandle, dataset,
                                               currScanDirDatasetRelPath.str(),
                                               currScanDir.comp_id,
                                               (filesSeen + 1), // VCS starts at 1 instead of 0 for some reason
                                               VCS_GET_DIR_PAGE_SIZE,
                                               verbose_http_log,
                                               getDirResp);
                    if (clearHttpHandleAndCheckForPauseStop(httpHandle)) { return; }
                    if (!isTransientError(rc)) { break; }
                    LOG_WARN("%p: Transient Error:%d RetryIndex:%d", this, rc, retries);
                    if (retries+1 < NUM_IMMEDIATE_TRANSIENT_RETRY) {  // pause between immediate retries
                        if (checkForPauseStop(true, QUICK_RETRY_INTERVAL)) {
                            return;
                        }
                    }
                }
                if (rc < 0) {
                    if (isTransientError(rc)) {
                        LOG_WARN("%p: Transient error: vcs_read_folder(%s) failed: %d",
                                this, currScanDirDatasetRelPath.c_str(), rc);
                        // Try the VCS scan again later.
                        errorOccurred = true;
                        goto skip_dir;
                    } else if (rc == VCS_ERR_PROVIDED_PATH_DOESNT_MATCH_PROVIDED_COMPID ||
                               rc == VCS_ERR_PATH_DOESNT_POINT_TO_KNOWN_COMPONENT ||
                               rc == VCS_ERR_LATEST_REVISION_NOT_FOUND)
                    {
                        // This doesn't imply that there's a conflict.
                        // VCS_ERR_PROVIDED_PATH_DOESNT_MATCH_PROVIDED_COMPID can happen if another
                        //   client deletes the folder.
                        LOG_INFO("%p: Expected failure for vcs_read_folder(%s): %d",
                                this, currScanDirDatasetRelPath.c_str(), rc);
                        // Should be safe to skip this.
                        // If we got into this state, we expect a notification to scan the parent
                        // directory again soon.
                    } else {
                        LOG_ERROR("%p: Unexpected error: vcs_read_folder(%s) failed: %d",
                                this, currScanDirDatasetRelPath.c_str(), rc);
                        // Try the VCS scan again later.
                        errorOccurred = true;
                        goto skip_dir;
                    }
                }
                if (startingDirVersion == -1) {
                    startingDirVersion = getDirResp.currentDirVersion;
                    LOG_INFO("vcs_read_folder(%s), currentDirVersion("FMTu64")", currScanDirDatasetRelPath.c_str(), getDirResp.currentDirVersion);
                }

                // Each fileList element has the following relevant fields: type (file|dir), compId,
                //     latestRevision.revision. latestRevision.hash, latestRevision.lastChangedNanoSecs.

                // For each file dirEntry in fileList:
                for (vector<VcsFile>::iterator it = getDirResp.files.begin();
                     it != getDirResp.files.end();
                     ++it)
                {
                    // Look it up in localDB.
                    SCRow_syncHistoryTree currDbEntry;
                    rc = localDb.syncHistoryTree_get(currScanDirRelPath.str(), it->name, currDbEntry);
                    // If dirEntry doesn't exist in LocalDB:
                    if (rc == SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) { // dirEntry not present in LocalDB.
                        // Create file/dir record in LocalDB (vcs_comp_id = compId, vcs_revision = -1 since it doesn't exist on the local FS).
                        bool skipDir = false;
                        addFileSyncHistoryTree(currScanDirRelPath.str(),
                                               *it,
                                               getDirResp.currentDirVersion,
                                               /*OUT*/ skipDir);
                        if (skipDir) {
                            // Retrying probably won't help, so not setting errorOccurred.
                            goto skip_dir;
                        }
                        // Add {dirEntry, compId, revision, client_reported_modify_time} to DownloadChangeLog.
                        LOG_INFO("%p: Decision DownloadFile (%s,%s),compId("FMTu64"),rev("FMTu64"),mtime("FMTu64"),isOnAcs(%d)",
                                 this, currScanDirRelPath.c_str(), it->name.c_str(),
                                 it->compId, it->latestRevision.revision,
                                 vcsTimeToNormLocalTime(it->lastChanged),
                                 !it->latestRevision.noAcs);
                        rc = localDb_downloadChangeLog_add_and_setSyncStatus(
                                currScanDirRelPath.str(), it->name, false,
                                DOWNLOAD_ACTION_GET_FILE, it->compId,
                                it->latestRevision.revision,
                                vcsTimeToNormLocalTime(it->lastChanged),
                                !it->latestRevision.noAcs,
                                it->hashValue,
                                !it->hashValue.empty(),
                                it->latestRevision.size);
                        if (rc < 0) {
                            LOG_CRITICAL("%p: downloadChangeLog_add(%s, %s) failed: %d",
                                    this, currScanDirRelPath.c_str(), it->name.c_str(), rc);
                            HANDLE_DB_FAILURE();
                            // Retrying probably won't help, so not setting errorOccurred.
                            goto skip_dir;
                        }
                    } else if (rc < 0) {
                        LOG_CRITICAL("%p: syncHistoryTree_get(%s, %s) failed: %d",
                                  this, currScanDirRelPath.c_str(), it->name.c_str(), rc);
                        HANDLE_DB_FAILURE();
                        // Retrying probably won't help, so not setting errorOccurred.
                        goto skip_dir;
                    } else { // dirEntry present in LocalDB; currDbEntry is valid.
                        // Update LocalDB[dirEntry].last_seen_in_version = currentDirVersion
                        LOG_INFO("%p: syncHistoryTree_updateLastSeenInVersion(%s, %s)", this, currScanDirRelPath.c_str(), it->name.c_str());
                        rc = localDb.syncHistoryTree_updateLastSeenInVersion(currScanDirRelPath.str(), it->name,
                                true, getDirResp.currentDirVersion);
                        if (rc < 0) {
                            LOG_CRITICAL("%p: syncHistoryTree_updateLastSeenInVersion(%s, %s) failed: %d",
                                      this, currScanDirRelPath.c_str(), it->name.c_str(), rc);
                            HANDLE_DB_FAILURE();
                            // Retrying probably won't help, so not setting errorOccurred.
                            goto skip_dir;
                        }
                        // If LocalDB[dirEntry].vcs_comp_id != compId || LocalDB[dirEntry].vcs_revision != revision,
                        if ((currDbEntry.comp_id != it->compId) || (currDbEntry.revision != it->latestRevision.revision)) {
                            if (currDbEntry.is_dir) {
                                // Handling for file->dir.  Non-conflict case.  See Note_03_28_2014 in this file.
                                LOG_INFO("%p: File to replace local directory (%s,%s), Removing known directory elements.",
                                         this, currScanDirRelPath.c_str(), it->name.c_str());
                                deleteLocalDir(currDbEntry);

                                bool skipDir = false;
                                addFileSyncHistoryTree(currScanDirRelPath.str(),
                                                       *it,
                                                       getDirResp.currentDirVersion,
                                                       /*OUT*/ skipDir);
                                if (skipDir) {
                                    // Retrying probably won't help, so not setting errorOccurred.
                                    goto skip_dir;
                                }
                            }
                            // Add {dirEntry, compId, revision, client_reported_modify_time} to DownloadChangeLog.
                            LOG_INFO("%p: Decision DownloadFile (%s,%s),compId("FMTu64","FMTu64
                                     "),rev("FMTu64","FMTu64"),mtime("FMTu64"),isOnAcs(%d), hashValue(%s), fileSize("FMTu64")",
                                     this, currScanDirRelPath.c_str(), it->name.c_str(),
                                     currDbEntry.comp_id, it->compId,
                                     currDbEntry.revision, it->latestRevision.revision,
                                     vcsTimeToNormLocalTime(it->lastChanged),
                                     !it->latestRevision.noAcs, it->hashValue.c_str(), it->latestRevision.size);
                            rc = localDb_downloadChangeLog_add_and_setSyncStatus(
                                    currScanDirRelPath.str(), it->name, false,
                                    DOWNLOAD_ACTION_GET_FILE, it->compId,
                                    it->latestRevision.revision,
                                    vcsTimeToNormLocalTime(it->lastChanged),
                                    !it->latestRevision.noAcs,
                                    it->hashValue, !it->hashValue.empty(), it->latestRevision.size);
                            if (rc < 0) {
                                LOG_CRITICAL("%p: downloadChangeLog_add(%s, %s) failed: %d",
                                        this, currScanDirRelPath.c_str(), it->name.c_str(), rc);
                                HANDLE_DB_FAILURE();
                                // Retrying probably won't help, so not setting errorOccurred.
                                goto skip_dir;
                            }
                        }
                    }
                } // for each file dirEntry in fileList.

                // For each folder dirEntry in fileList:
                for (vector<VcsFolder>::iterator it = getDirResp.dirs.begin();
                     it != getDirResp.dirs.end();
                     ++it)
                {
                    bool addToNeedDownloadScan = false;
                    SCRelPath currEntryDirPath = currScanDirRelPath.getChild(it->name);
                    // Look it up in localDB.
                    SCRow_syncHistoryTree currDbEntry;
                    rc = localDb.syncHistoryTree_get(currScanDirRelPath.str(), it->name, currDbEntry);
                    // If dirEntry doesn't exist in LocalDB:
                    if (rc == SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) { // dirEntry not present in LocalDB.
                        SetStatus(SYNC_CONFIG_STATUS_SYNCING);

                        bool skipDir = false;
                        localMakeDir(currScanDirRelPath.str(),
                                     *it,
                                     getDirResp.currentDirVersion,
                                     /*OUT*/ skipDir);
                        if (skipDir) {
                            goto skip_dir;
                        }

                        // Directory was never scanned before, add it to the queue:
                        addToNeedDownloadScan = true;
                    } else if (rc < 0) {
                        LOG_CRITICAL("%p: syncHistoryTree_get(%s, %s) failed: %d",
                                this, currScanDirRelPath.c_str(), it->name.c_str(), rc);
                        HANDLE_DB_FAILURE();
                        // Retrying probably won't help, so not setting errorOccurred.
                        goto skip_dir;
                    } else { // dirEntry present in LocalDB; currDbEntry is valid.
                        VPLFS_stat_t statBuf;
                        SCRelPath relPath = getRelPath(currDbEntry);
                        AbsPath dirEntryPath = getAbsPath(relPath);
                        bool dirEntryPathIsDir = false;
                        bool dirEntryPathIsFile = false;
                        // TODO: Bug 18016: This is technically the wrong approach and can lead to
                        //   false conflicts.  We need to compare VCS with the localDb.
                        //   Directly comparing VCS and the filesystem is wrong and prone
                        //   to race conditions.
                        int statResult = VPLFS_Stat(dirEntryPath.c_str(), &statBuf);
                        if (statResult==0) {
                            if (statBuf.type == VPLFS_TYPE_DIR) {
                                dirEntryPathIsDir = true;
                            } else if (statBuf.type == VPLFS_TYPE_FILE) {
                                dirEntryPathIsFile = true;
                            }
                        }

                        // Just update LocalDB[dirEntry].last_seen_in_version = currentDirVersion
                        rc = localDb.syncHistoryTree_updateLastSeenInVersion(currScanDirRelPath.str(), it->name,
                                true, getDirResp.currentDirVersion);
                        if (rc < 0) {
                            LOG_CRITICAL("%p: syncHistoryTree_updateLastSeenInVersion(%s, %s) failed: %d",
                                    this, currScanDirRelPath.c_str(), it->name.c_str(), rc);
                            HANDLE_DB_FAILURE();
                            // Retrying probably won't help, so not setting errorOccurred.
                            goto skip_dir;
                        }

                        // If LocalDB[dirEntry].vcs_comp_id != compId:
                        if (currDbEntry.comp_id != it->compId || !dirEntryPathIsDir)
                        {   // There's a difference between VCS dir and localFS
                            // Determine what actions to take to make the local FS match the VCS directory.
                            if (statResult == VPL_ERR_NOENT) {
                                // Dir entry is missing on FileSystem.
                                // Recently deleted which will trigger another scan and allow uploadScan to handle.
                                // No need to continuing scanning the VCS directory (addToNeedDownloadScan = false).

                                // Need to decide whether this is a conflict or not (Has VCS tree since changed?)
                                if (it->version <= currDbEntry.version_scanned)
                                {   // We know of this directory and it has not changed. This is not a conflict,
                                    // let upload scan handle the local change (deletion of directory)
                                    LOG_INFO("%p: Local entry(%s) modified, not conflict", this, dirEntryPath.c_str());
                                } else
                                {   // Could possibly be a conflict.  Directory version is only supposed to be a hint
                                    // to scan a directory, not to be used to determine conflict actions.

                                    // TODO: Walk through this directory tree on VCS Infra and compare with localDB.
                                    //    If the results are different, this is a conflict, and needs to be handled now.
                                    //    Otherwise, not a conflict, let upload scan handle the local change

                                    // For now, assume the expedient thing and just treat this as a conflict.
                                    LOG_INFO("%p: Local entry(%s) Possible conflict with remote dir. Blindly assume to be conflict.",
                                             this, dirEntryPath.c_str());
                                    bool unused;
                                    AbsPath unusedPath("");
                                    SetStatus(SYNC_CONFIG_STATUS_SYNCING);
                                    // TODO: this is confusing; we are not performing CopyBack here!
                                    conflictResolutionDuringCopyBack(relPath, false, unusedPath, unused);
                                }
                            } else if (currDbEntry.is_dir)
                            {
                                if (dirEntryPathIsDir) {
                                    // Update the localDB:
                                    rc = localDb.syncHistoryTree_updateCompId(currScanDirRelPath.str(), it->name,
                                            true, it->compId, false, 0);
                                    if (rc < 0) {
                                        LOG_CRITICAL("%p: syncHistoryTree_updateCompId(%s) failed: %d",
                                                this, dirEntryPath.c_str(), rc);
                                        HANDLE_DB_FAILURE();
                                        // Retrying probably won't help, so not setting errorOccurred.
                                        goto skip_dir;
                                    }
                                } else if (dirEntryPathIsFile)
                                {   // Definite conflict, client change and server change, server wins

                                    // TODO: Bug 18016: This is actually NOT a definite conflict.  There are two cases here.
                                    //  1) Case 1 is the current implementation below.  Upload fails to
                                    //     create a file on VCS because the VCS directory has changed.  Thus,
                                    //     Upload side skips the upload and wants the Download side to take
                                    //     care of the issue by renaming the file out of the way.
                                    //  2) In the case where this client does the following actions:
                                    //      a) Create xyz/ (folder)
                                    //      b) Delete xyz/ and immediately add file named xyz
                                    //     This could potentially not be a conflict case, where we want to do
                                    //     nothing here and allow the Upload side to update VCS with the delete.
                                    //     However, in order to confirm this is indeed the case,
                                    //     we need to traverse the VCS directory and see if all the entries
                                    //     match the localDb, not a trivial task for how rare this case occurs.
                                    //  Currently, Case 2) is not handled, and is always assumed to be Case 1),
                                    //  and the effect is that a false conflict is seen.
                                    LOG_INFO("%p: Local entry(%s) Conflict with remote dir.",
                                             this, dirEntryPath.c_str());
                                    bool unused;
                                    AbsPath unusedPath("");
                                    SetStatus(SYNC_CONFIG_STATUS_SYNCING);
                                    // TODO: this is confusing; we are not performing CopyBack here!
                                    conflictResolutionDuringCopyBack(relPath, false, unusedPath, unused);
                                } else {
                                    LOG_ERROR("%p: Case not handled(%s):statResult:%d, fileType:%d",
                                              this, dirEntryPath.c_str(), statResult, (int)statBuf.type);
                                }
                            } else {  // !currDbEntry.is_dir, localDB entry is file.
                                if (dirEntryPathIsDir)
                                {   // Only localDbRecord is off.
                                    bool skipDir = false;
                                    localMakeDir(currScanDirRelPath.str(),
                                                 *it,
                                                 getDirResp.currentDirVersion,
                                                 /*OUT*/ skipDir);
                                    if (skipDir) {
                                        goto skip_dir;
                                    }
                                    // Directory was never scanned before, add it to the queue:
                                    addToNeedDownloadScan = true;
                                } else if (dirEntryPathIsFile)
                                {   // Handling for dir->file.  Non-conflict case.
                                    // Conflict case handled at around Note_03_28_2014 in this file.
                                    LOG_INFO("%p: Directory to replace local file (%s), Removing known file.",
                                             this, dirEntryPath.c_str());
                                    rc = deleteLocalFile(currDbEntry);  // File removed if it is known.
                                    if (rc != 0) {
                                        LOG_INFO("%p: Local file(%s) conflicts with remote dir",
                                                 this, dirEntryPath.c_str());
                                        bool unused;
                                        AbsPath unusedPath("");
                                        // TODO: this is confusing; we are not performing CopyBack here!
                                        conflictResolutionDuringCopyBack(relPath, false, unusedPath, unused);
                                    }
                                    SetStatus(SYNC_CONFIG_STATUS_SYNCING);

                                    bool skipDir = false;
                                    localMakeDir(currScanDirRelPath.str(),
                                                 *it,
                                                 getDirResp.currentDirVersion,
                                                 /*OUT*/ skipDir);
                                    if (skipDir) {
                                        goto skip_dir;
                                    }
                                    // Directory was never scanned before, add it to the queue:
                                    addToNeedDownloadScan = true;
                                } else {
                                    LOG_ERROR("%p: Case not handled(%s):statResult:%d, fileType:%d",
                                              this, dirEntryPath.c_str(), statResult, (int)statBuf.type);
                                }
                            }
                        }
                        // LocalDB[dirEntry].version_scanned < currentDirVersion),
                        if (currDbEntry.version_scanned < it->version) {
                            // Directory has been updated since we last scanned it, add it to the queue:
                            addToNeedDownloadScan = true;
                            LOG_DEBUG("%p: Previous version_scanned="FMTu64", now="FMTu64"; adding %s to needDownloadScan",
                                    this, currDbEntry.version_scanned, it->version, currEntryDirPath.c_str());
                        }
                    }

                    if (addToNeedDownloadScan) {
                        rc = localDb.needDownloadScan_add(currEntryDirPath.str(), it->compId);
                        if (rc < 0) {
                            LOG_CRITICAL("%p: needDownloadScan_add(%s) failed: %d",
                                    this, currEntryDirPath.c_str(), rc);
                            HANDLE_DB_FAILURE();
                            // Retrying probably won't help, so not setting errorOccurred.
                            goto skip_dir;
                        }
                    }
                } // for each folder dirEntry in fileList.

                // Get ready to process the next page of vcs_read_folder results.
                u64 entriesInCurrRequest = getDirResp.files.size() + getDirResp.dirs.size();
                filesSeen += entriesInCurrRequest;
                if(entriesInCurrRequest == 0 && filesSeen < getDirResp.numOfFiles) {
                    LOG_ERROR("%p: No entries in getDir(%s) response:"FMTu64"/"FMTu64,
                             this, currScanDirDatasetRelPath.c_str(),
                             filesSeen, getDirResp.numOfFiles);
                    goto skip_dir;
                }
            } while (filesSeen < getDirResp.numOfFiles);

            // If currentDirVersion stayed the same for all pages,
            if (getDirResp.currentDirVersion == startingDirVersion) {
                // Perform deletion phase.
                {
                    //  For each dirEntry where LocalDB.rel_path == currDir and LocalDB.last_seen_in_version < currentDirVersion.
                    //      Traverse the local metadata from the to-be-deleted-node depth first
                    //      For each entryToDelete in traversal:
                    //          (optimization) (required for correctness if uploads are allowed to happen asynchronously)
                    //                          Need to skip the file if it was just uploaded by a different thread.
                    //          If entryToDelete is file,
                    //              Lock file (best effort), if local_content matches metadata (no local update), then delete, else skip.
                    //          If entryToDelete is directory
                    //              If directory is empty, then delete, else skip.
                    //          Remove entryToDelete from LocalDB.
                    // TODO: Bug 12976: Currently, we don't even try locking the file currently (in deleteLocalFile)
                    int rc;
                    vector<SCRow_syncHistoryTree> children;
                    rc = localDb.syncHistoryTree_getChildren(
                                        currScanDirRelPath.str(), children);
                    if (rc != 0 && rc != SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) {
                        LOG_CRITICAL("%p: syncHistoryTree_getChildren:%d, %s",
                                  this, rc, currScanDirRelPath.c_str());
                        HANDLE_DB_FAILURE();
                    }
                    for (vector<SCRow_syncHistoryTree>::iterator childrenIt = children.begin();
                        childrenIt != children.end(); ++childrenIt)
                    {
                        SCRow_syncHistoryTree &child = *childrenIt;
                        if(isRoot(child)){ continue; } // Skip root exception case

                        if(child.last_seen_in_version_exists &&
                           child.last_seen_in_version >= getDirResp.currentDirVersion) {
                            continue;
                        }
                        // We need to delete the local file/dir.
                        SetStatus(SYNC_CONFIG_STATUS_SYNCING);
                        if (child.is_dir) {
                            deleteLocalDir(child);
                        } else {
                            deleteLocalFile(child);
                        }
                    }
                }

                // Assign version_scanned = currentDirVersion for currDir.
                rc = localDb.syncHistoryTree_updateVersionScanned(currScanDirRelPath.getParent().str(),
                        currScanDirRelPath.getName(),
                        true, getDirResp.currentDirVersion);
                if (rc < 0) {
                    LOG_CRITICAL("%p: syncHistoryTree_updateVersionScanned(%s) failed: %d",
                            this, currScanDirRelPath.c_str(), rc);
                    HANDLE_DB_FAILURE();
                    // Retrying probably won't help, so not setting errorOccurred.
                    goto skip_dir;
                }
            }
 skip_dir:
            if(errorOccurred) {
                setErrorNeedsRemoteScan();
                return;
            } else {
                // Remove currDir from LocalDB."need_download_scan".
                rc = localDb.needDownloadScan_remove(currScanDir.row_id);
                if (rc < 0) {
                    LOG_CRITICAL("%p: needDownloadScan_remove("FMTu64") failed: %d",
                            this, currScanDir.row_id, rc);
                    HANDLE_DB_FAILURE();
                    // The entry wasn't removed, so probably don't want to call needDownloadScan_get() again.
                    return;
                }
            }
        } // while ((rc = localDb.needDownloadScan_get(currNeedDownloadScan)) == 0)
        if (rc != SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) {
            LOG_CRITICAL("%p: needDownloadScan_get() failed: %d", this, rc);
            HANDLE_DB_FAILURE();
        }
    }

    enum DownloadChangeLogErrorMode {
        DL_CHANGE_LOG_ERROR_MODE_NONE,
        DL_CHANGE_LOG_ERROR_MODE_DOWNLOAD,
        DL_CHANGE_LOG_ERROR_MODE_COPYBACK
    };

    int getNextDownloadChangeLog(DownloadChangeLogErrorMode errorMode,
                                 u64 errorModeMaxRowId,
                                 SCRow_downloadChangeLog& entry_out)
    {
        int rc = 0;
        switch(errorMode)
        {
        case DL_CHANGE_LOG_ERROR_MODE_NONE:
            rc = localDb.downloadChangeLog_get(entry_out);
        break;
        case DL_CHANGE_LOG_ERROR_MODE_DOWNLOAD:
            rc = localDb.downloadChangeLog_getErrDownload(errorModeMaxRowId,
                                                          entry_out);
        break;
        case DL_CHANGE_LOG_ERROR_MODE_COPYBACK:
            rc = localDb.downloadChangeLog_getErrCopyback(errorModeMaxRowId,
                                                          entry_out);
        break;
        default:
            FAILED_ASSERT("Unknown enum:%d", (int)errorMode);
            break;
        }
        return rc;
    }

    /// Applies the local changes to the server.
    void ApplyDownloadChangeLog(DownloadChangeLogErrorMode errorMode)
    {
        int rc;
        SCRow_downloadChangeLog currDownloadEntry;
        u64 errorModeMaxRowId = 0;
        ASSERT(!VPLMutex_LockedSelf(&mutex));

        if(errorMode != DL_CHANGE_LOG_ERROR_MODE_NONE) {
            // Both error modes should not happen at the same time.
            rc = localDb.downloadChangeLog_getMaxRowId(errorModeMaxRowId);
            if(rc != 0) {
                LOG_CRITICAL("%p: Should never happen:downloadChangeLog_getMaxRowId,%d",
                        this, rc);
                HANDLE_DB_FAILURE();
                return;
            }
        }

        // For each {dirEntry, compId, revision, client_reported_modify_time} in DownloadChangeLog:
        while ((rc = getNextDownloadChangeLog(errorMode,
                                              errorModeMaxRowId,
                                              currDownloadEntry)) == 0)
        {
            bool downloadPerformed = false;
            bool skip_ErrIncDownload = false;
            bool skip_ErrIncCopyback = false;
            std::string accessUrlSaved;

            if (!isValidDirEntry(currDownloadEntry.name)) {
                FAILED_ASSERT("Bad entry added to download change log: \"%s\"",
                              currDownloadEntry.name.c_str());
                goto skip;
            }
            if (checkForPauseStop()) {
                return;
            }
            {
                SCRelPath currRelPath = getRelPath(currDownloadEntry);
                DatasetRelPath currDatasetRelPath = getDatasetRelPath(currRelPath);

                SCRow_syncHistoryTree currDbEntry;
                // Get the record from LocalDB.
                {
                    // Invariant: If the entry is in the download change log, it must be in
                    //     the syncHistoryTree.
                    rc = syncHistoryTree_getEntry(currRelPath, currDbEntry);
                    if (rc < 0) {
                        LOG_CRITICAL("%p: syncHistoryTree_getEntry(%s) failed: %d",
                                this, currRelPath.c_str(), rc);
                        HANDLE_DB_FAILURE();
                        goto skip;
                    } else {
                        // dirEntry present in LocalDB; currDbEntry is valid.
                    }
                }

                // (optional optimization 3) Perform predownload conflict check: (if policy 3, we can skip the download if there is an outgoing change).
                // (optional optimization 4, to avoid downloading again) If dirEntry is file as reported by VCS and !LocalDB[dirEntry].is_directory and LocalDB[dirEntry].hash == file hash reported by VCS,
                // - (optional optimization 4) Files are the same, set local info the same as the VCS info. Set LocalDB.vcs_comp_id = compId, Set LocalDB.vcs_revision = revision
                // - (optional optimization 4) Proceed to next entry

                // check file size between DownloadChangeLog table and local file,
                // if size is the same, then compare hash value, then only download file if hash is different
                // Stat the file to get its modified time.
                VPLFS_stat_t hashedStatBuf;
                AbsPath destAbsPath = getAbsPath(currRelPath);
                VPLTime_t fileModifyTime = 0; // dummy init to satisfy compiler
                if (sync_policy.how_to_compare == SyncPolicy::FILE_COMPARE_POLICY_USE_HASH && 
                        VPLFS_Stat(destAbsPath.c_str(), &hashedStatBuf) == 0) {
                    VPLFS_file_size_t hashedFileSize = hashedStatBuf.size;
                    if (hashedFileSize >= 0 && hashedStatBuf.type == VPLFS_TYPE_FILE) {
                        int hashRet = compareHash(currDownloadEntry.file_size,
                             currDownloadEntry.hash_value, hashedFileSize, destAbsPath);
                        if (hashRet == 0) {
                            currDownloadEntry.download_succeeded = true;
                            // We don' need to update modify time to the file if hash is the same.

                            // Mark download done.  Download will never be done again.
                            rc = localDb.downloadChangeLog_setDownloadSuccess(currDownloadEntry.row_id);
                            if (rc != 0) {
                                LOG_CRITICAL("%p: downloadChangeLog_setDownloadSuccess:%d", this, rc);
                                HANDLE_DB_FAILURE();
                            }
                            // Update the LocalDB with the downloaded file's metadata
                            // (timestamp, compId, revision, hash_value, file_size).
                            fileModifyTime = fsTimeToNormLocalTime(destAbsPath, hashedStatBuf.vpl_mtime);
                            rc = localDb.syncHistoryTree_updateEntryInfo(
                                    currDbEntry.parent_path,
                                    currDbEntry.name,
                                    true, fileModifyTime,
                                    true, currDownloadEntry.comp_id,
                                    true, currDownloadEntry.revision,
                                    currDownloadEntry.hash_value,
                                    currDownloadEntry.file_size_exists,
                                    currDownloadEntry.file_size);
                            if (rc < 0) {
                                LOG_CRITICAL("%p: syncHistoryTree_updateEntryInfo(%s) failed: %d",
                                        this, currRelPath.c_str(), rc);
                                HANDLE_DB_FAILURE();
                                goto skip;
                            }

                            reportFileDownloaded(false, currRelPath,
                                    currDownloadEntry.is_dir, false);
                            LOG_INFO("Done update DB info of file%s,"
                                    " skip download this file due to hash is the same",
                                 currRelPath.c_str());

                            goto skip;
                        }
                    } else {
                        LOG_INFO("Cannot get file size or not a file, don't do hash compare.");
                    }
                } // endif (sync_policy.how_to_compare == SyncPolicy::FILE_COMPARE_POLICY_USE_HASH)

                // Intermediate temporary path between infra and final dest
                AbsPath tempFilePath = getTempDownloadFilePath(currDownloadEntry.comp_id,
                                                               currDownloadEntry.revision);
                if (!currDownloadEntry.download_succeeded)
                {
                    DatasetRelPath currDatasetRelPath = getDatasetRelPath(currRelPath);
                    LOG_INFO("%p: ACTION DOWN download begin:%s "
                             "compId("FMTu64"),rev("FMTu64"),mtime("FMTu64"),server_dir(%s)",
                             this,
                             currRelPath.c_str(),
                             currDownloadEntry.comp_id,
                             currDownloadEntry.revision,
                             currDownloadEntry.client_reported_mtime,
                             server_dir.c_str());
                    if(dataset_access_info.archiveAccess != NULL &&
                       is_archive_storage_device_available) // It should be okay to access
                                                            // is_archive_storage_device_available
                                                            // without holding the mutex.

                    {   // If download has not succeeded, first attempt will be archive storage.

                        // TODO_NOT_IMPLEMENTED: This entire If block may not be needed and probably can be removed.
                        //   We don't need archive access if we're doing URL based file access.
                        //   This diff changes the other existing block (acs access) to be VCS URL based.

                        // TODO: Bug 16319: For Two-Way, we don't want to try downloading from our own device.
                        //   It's not a big deal though, since the storage node part quickly returns
                        //   HTTP status 400, so we'll move on to ACS quickly.
                        int rc;
                        VCSArchiveAccess* vcsArchiveAccess = dataset_access_info.archiveAccess;
                        LOG_INFO("%p:ACTION DOWN Archive Storage Attempt(%s)", this, currDatasetRelPath.c_str());
                        for (u32 retries=0; retries < NUM_IMMEDIATE_TRANSIENT_RETRY; ++retries)
                        {
                            VCSArchiveOperation* vcsArchiveOperation = vcsArchiveAccess->createOperation(/*OUT*/ rc);
                            if(vcsArchiveOperation == NULL) {
                                LOG_WARN("archive createOperation("FMTu64","FMTu64",%s): %d. Continuing.",
                                         currDownloadEntry.comp_id,
                                         currDownloadEntry.revision,
                                         currDatasetRelPath.c_str(),
                                         rc);
                            } else {
                                registerForAsyncCancel(vcsArchiveOperation);
                                rc = vcsArchiveOperation->GetFile(
                                        currDownloadEntry.comp_id,
                                        currDownloadEntry.revision,
                                        currDatasetRelPath.str(),
                                        tempFilePath.str());
                                unregisterForAsyncCancel(vcsArchiveOperation);
                                vcsArchiveAccess->destroyOperation(vcsArchiveOperation);
                            }
                            if (!isTransientError(rc)) { break; }
                            LOG_WARN("%p: Archive Transient Error:%d RetryIndex:%d", this, rc, retries);
                            if (retries+1 < NUM_IMMEDIATE_TRANSIENT_RETRY) {  // pause between immediate retries
                                if (checkForPauseStop(true, QUICK_RETRY_INTERVAL)) {
                                    return;
                                }
                            }
                        }
                        if (isTransientError(rc) ||
                            rc == CCD_ERROR_ARCHIVE_DEVICE_OFFLINE)
                        {   // offline error not exactly transient because we don't want to retry 3 times,
                            // but still want to retry later
                            LOG_WARN("%p: Archive Storage GetFile("FMTu64","FMTu64",%s):%d",
                                      this, currDownloadEntry.comp_id,
                                      currDownloadEntry.revision,
                                      currDatasetRelPath.c_str(),
                                      rc);
                            // Bug 16762: TwoWaySync could utilize is_on_acs field.  However,
                            //  until that change is made, this assignment is not actually needed.
                            //  Leaving it here for consistency though.
                            skip_ErrIncDownload = true;
                        } else if (rc != 0) {
                            // This is based off of SyncConfigOneWayDown.  In SyncConfigOneWayDown,
                            // files originate from the archiveStorage device, so if the file does not
                            // exist on the device, then the change log can be dropped.
                            //
                            // In SyncConfigTwoWay, the file may originate from another client,
                            // and the change log MUST NOT be dropped because the archiveStorage
                            // device could simply need more time to download the file.

                            // if (!task->currDownloadEntry.is_on_acs) {
                            //     LOG_ERROR("%p: PermanentError! Dropping vcsArchiveOperation"
                            //               "("FMTu64","FMTu64",%s):%d",
                            //               this, task->currDownloadEntry.comp_id,
                            //               task->currDownloadEntry.revision,
                            //               currDatasetRelPath.c_str(),
                            //               rc);
                            //     goto skip;
                            // }
                        } else {
                            LOG_INFO("%p: Archive Storage GetFile OK ("FMTu64","FMTu64",%s)",
                                    this, currDownloadEntry.comp_id,
                                    currDownloadEntry.revision,
                                    currDatasetRelPath.c_str());
                            currDownloadEntry.download_succeeded = true;
                            downloadPerformed = true;  // Mark in DB that download will never be done again.
                        }
                    }  // Download from archive device

                    if (!currDownloadEntry.download_succeeded)  // Difference from one-way sync:
                                                                // Do not check the is_on_acs field;
                                                                // Virtual Sync for TwoWaySync always
                                                                // has valid accessinfo but doesn't
                                                                // set is_on_acs.
                    {
                        LOG_INFO("%p: ACTION DOWN download attempt(%s) compId("FMTu64"),rev("FMTu64"),mtime("FMTu64"),server_dir(%s)",
                                 this,
                                 currRelPath.c_str(), currDownloadEntry.comp_id, currDownloadEntry.revision,
                                 currDownloadEntry.client_reported_mtime, server_dir.c_str());
                        // VCS GET accessinfo(path, method=GET, compId, revision) - returns accessUrl.
                        VcsAccessInfo accessInfoResp;
                        // Not needed, but keeping here for consistency with OneWay:
                        {
                            // Since we will try ACS now, reset this if it was already set (due to a failed
                            // attempt to download from the Archive Storage Device).
                            skip_ErrIncDownload = false;
                        }
                        LOG_INFO("%p: ACTION DOWN download attempt(%s)", this, currDatasetRelPath.c_str());
                        // TODO: (optional optimization) To avoid downloading an out-of-date revision, either add
                        // a flag to VCS GET accessinfo to return an error if revision is not latest, or add
                        // something like VCS GET accessinfoAndMetadata(path, method=GET, compId), which returns
                        // accessUrl, revision, hash, client_reported_modify_time (for the latest revision).
                        for (u32 retries=0; retries < NUM_IMMEDIATE_TRANSIENT_RETRY; ++retries)
                        {
                            VPLHttp2 httpHandle;
                            if (setHttpHandleAndCheckForStop(httpHandle)) { return; }
                            rc = vcs_access_info_for_file_get(vcs_session, httpHandle, dataset,
                                                              currDatasetRelPath.str(),
                                                              currDownloadEntry.comp_id,
                                                              currDownloadEntry.revision,
                                                              verbose_http_log,
                                                              accessInfoResp);
                            if (clearHttpHandleAndCheckForPauseStop(httpHandle)) { return; }
                            if (!isTransientError(rc)) { break; }
                            LOG_WARN("%p: Transient Error:%d RetryIndex:%d", this, rc, retries);
                            if (retries+1 < NUM_IMMEDIATE_TRANSIENT_RETRY) {  // pause between immediate retries
                                if (checkForPauseStop(true, QUICK_RETRY_INTERVAL)) {
                                    return;
                                }
                            }
                        }
                        if (rc < 0) {
                            if(rc == VCS_ERR_REVISION_NOT_FOUND ||
                               rc == VCS_ERR_COMPONENT_NOT_FOUND ||
                               rc == VCS_ERR_PROVIDED_PATH_DOESNT_MATCH_PROVIDED_COMPID)  // Bug 16833: VCS
                                                // GET accessinfo (for download) returns 5316
                                                // (VCS_PROVIDED_PATH_DOESNT_MATCH_PROVIDED_COMPID)
                                                // for deleted file.
                            {
                                // Something changed on VCS; we expect a notification to do another
                                // remote scan soon.
                                LOG_WARN("%p: Attempt to download %s,"FMTu64","FMTu64" but disappeared:%d. Skipping.",
                                         this,
                                         currDatasetRelPath.c_str(),
                                         currDownloadEntry.comp_id,
                                         currDownloadEntry.revision,
                                         rc);
                                goto skip;
                            } else if (isTransientError(rc)) {
                                LOG_WARN("%p: vcs_access_info_for_file_get(%s, "FMTu64", "FMTu64") returned %d",
                                        this,
                                        currDatasetRelPath.c_str(), currDownloadEntry.comp_id,
                                        currDownloadEntry.revision, rc);
                                // Transient error; retry later.
                                skip_ErrIncDownload = true;
                                goto skip;
                            } else {
                                // Non-retryable; need to do another server scan.
                                // TODO: Bug 12976: Except this won't actually do another server scan because
                                //   "version scanned" will already be updated.  If there actually
                                //   was a change to the parent directory, we should already have a
                                //   dataset_content_change notification to make a new scan request anyway.
                                //   Should we remove this call to setErrorNeedsRemoteScan()?
                                setErrorNeedsRemoteScan();
                                LOG_ERROR("%p: vcs_access_info_for_file_get(%s, "FMTu64", "FMTu64") returned %d",
                                        this,
                                        currDatasetRelPath.c_str(), currDownloadEntry.comp_id,
                                        currDownloadEntry.revision, rc);
                                goto skip;
                            }
                        }

                        // Download file from accessUrl to the temp file.
                        VCSFileUrlAccess* vcsFileUrlAccess = dataset_access_info.fileUrlAccess;
                        for (u32 retries=0; retries < NUM_IMMEDIATE_TRANSIENT_RETRY; ++retries)
                        {
                            VCSFileUrlOperation* vcsFileUrlOperation =
                                    vcsFileUrlAccess->createOperation(accessInfoResp,
                                                                      verbose_http_log,
                                                                      /*OUT*/ rc);
                            if(vcsFileUrlOperation == NULL) {
                                LOG_WARN("vcsFileUrlOperation("FMTu64","FMTu64",%s): %d. Continuing.",
                                         currDownloadEntry.comp_id,
                                         currDownloadEntry.revision,
                                         currDatasetRelPath.c_str(),
                                         rc);
                            } else {
                                registerForAsyncCancel(vcsFileUrlOperation);
                                rc = vcsFileUrlOperation->GetFile(tempFilePath.str());
                                unregisterForAsyncCancel(vcsFileUrlOperation);
                                vcsFileUrlAccess->destroyOperation(vcsFileUrlOperation);
                            }

                            if (!isTransientError(rc)) { break; }
                            LOG_WARN("%p: GetFile Transient Error:%d RetryIndex:%d", this, rc, retries);
                            if (retries+1 < NUM_IMMEDIATE_TRANSIENT_RETRY) {  // pause between immediate retries
                                if (checkForPauseStop(true, QUICK_RETRY_INTERVAL)) {
                                    return;
                                }
                            }
                        }
                        if (rc != 0) {
                            // We consider all errors from ACS/storage to be transient; we need to check with VCS
                            // GET accessinfo again later to determine if it is retryable or not.
                            LOG_WARN("%p: GetFile(%s, "FMTu64", "FMTu64")->\"%s\" returned %d",
                                    this,
                                    currDatasetRelPath.c_str(), currDownloadEntry.comp_id,
                                    currDownloadEntry.revision, tempFilePath.c_str(), rc);
                            // Retry the download again later.
                            skip_ErrIncDownload = true;
                            goto skip;
                        }

                        // Mark in DB that download will never be done again.
                        downloadPerformed = true;
                        // Save the url for subsequent Syncbox archive storage cleanup purpose.
                        // This cannot be cleaned up until copy back is done and vcs_post_archive_url is called.
                        accessUrlSaved = accessInfoResp.accessUrl;

                        // Only relevant if hashes are available:
                        // (optional safeguard) Compute hash of the downloaded file, complain if
                        // that hash doesn't match the hash reported by VCS. Not sure if there is any
                        // action that is safe to perform in this case.
                        // Only relevant if hashes are available:
                        // (optional optimization 5, to avoid hashing every file just downloaded):
                        // Add (or increment numTimesToIgnore) {dirEntry path, numTimesToIgnore} in
                        // in-memory map "fileMonitorToIgnore" (only safe if VPL inotify *does not*
                        // de-dupe multiple changes for the same entry).

                    } // download from ACS/storage to temp file

                    if (downloadPerformed) {
                        // Mark download done.  Download will never be done again.
                        rc = localDb.downloadChangeLog_setDownloadSuccess(currDownloadEntry.row_id);
                        if (rc != 0) {
                            LOG_CRITICAL("%p: downloadChangeLog_setDownloadSuccess:%d", this, rc);
                            HANDLE_DB_FAILURE();
                        }
                    }

                    if (skip_ErrIncDownload)
                    {   // If archiveStorage fails, and attempt to download from ACS is skipped, (possible after Bug 16762 fix)
                        goto skip;
                    }

                }  // "download" to temp

                { // "Copyback" (rename the temp file to the correct final location).
                    if(errorMode == DL_CHANGE_LOG_ERROR_MODE_COPYBACK) {
                        LOG_INFO("%p: ACTION DOWN copyback begin:%s compId("FMTu64"),rev("FMTu64"),mtime("FMTu64")",
                                 this,
                                 currRelPath.c_str(),
                                 currDownloadEntry.comp_id,
                                 currDownloadEntry.revision,
                                 currDownloadEntry.client_reported_mtime);
                    } else if (!downloadPerformed) {
                        // Bug 16762: TwoWaySync could utilize is_on_acs field.  However,
                        //  until that change is made, this block should be unreachable:

                        // Download never happened or did not succeed.  (archive device offline, and file not on acs)
                        LOG_WARN("%p: No download availability:%s compId("FMTu64"),rev("FMTu64")",
                                 this, currRelPath.c_str(),
                                 currDownloadEntry.comp_id,
                                 currDownloadEntry.revision);
                        skip_ErrIncDownload = true;
                        goto skip;
                    }
                    // TODO: Lock local file
                    // Get modified time
                    // TODO: compute hash
                    {   // Checking local destination file.  Making sure it exists.
                        bool fileExistsOnFs;
                        // Only valid if fileExistsOnFs is true.
                        // Only valid if fileExistsOnFs is true.
                        VPLFS_file_size_t fileSize = 0; // dummy init to satisfy compiler
                        {
                            // Stat the file to get its modified time.
                            VPLFS_stat_t statBuf;
                            rc = VPLFS_Stat(destAbsPath.c_str(), &statBuf);
                            if (rc == 0) {
                                fileExistsOnFs = true;
                                fileModifyTime = fsTimeToNormLocalTime(destAbsPath, statBuf.vpl_mtime);
                                fileSize = statBuf.size;
                            } else if (rc == VPL_ERR_NOENT) {
                                // This is an expected case.  (It can happen if we are downloading
                                // a remote file for the first time or if the user deleted the local file.)
                                fileExistsOnFs = false;
                            } else {
                                // Unable to stat the file.
                                // TODO: Bug 12976: Is this safe enough, or should we check for only certain error codes?
                                //   If we're unable to stat the file, it's unlikely that we'll be
                                //   able to overwrite it later anyway.
                                //   But if it's possible, this could potentially cause us to overwrite a local modification
                                //   with a remote change, without detecting a conflict!
                                fileExistsOnFs = false;
                                LOG_WARN("%p: VPLFS_Stat(%s):%d",
                                        this, destAbsPath.c_str(), rc);
                            }
                        }

                        bool doRename;

                        // TODO: (optional case, to make "download first" have the same result as
                        // "upload first" when recovering from "local metadata lost") If local file
                        // hash matches file hash reported by VCS, then
                        // update the LocalDB with the hash, compId, revision, and local timestamp.
                        // Discard downloaded file.

                        // Else if local file mtime doesn't match LocalDB mtime (or hash doesn't match LocalDB hash) (See CaseE),
                        if (currDbEntry.local_mtime_exists) { // Local file previously existed and not a directory.
                            if (fileExistsOnFs) { // Local file exists now.
                                if (fileModifyTime != currDbEntry.local_mtime) {
                                    // Local file/dir was modified; conflict!
                                    LOG_INFO("%p: Local file/dir was modified (%s); conflict with remote file!",
                                            this, destAbsPath.c_str());
                                    // Perform "Conflict Resolution (during "copy back")" subroutine.
                                    conflictResolutionDuringCopyBack(getRelPath(currDownloadEntry), true, tempFilePath, doRename);
                                } else {
                                    // Local file was not changed; safe to do the rename.
                                    doRename = true;
                                }
                            } else { // Local file doesn't exist now.
                                // File was removed.  We can auto-resolve this conflict in favor of
                                // the modification.
                                // NOTE: This is a "safe choice", but this could potentially be a
                                //     configurable policy decision.
                                LOG_INFO("%p: Local file was removed (%s); auto-resolving conflict in favor of remote file.",
                                        this, destAbsPath.c_str());
                                doRename = true;
                            }
                        } else {
                            if (fileExistsOnFs) { // Local file exists now.
                                // Local file/dir was created; conflict!  For a directory, known elements should have already been
                                // removed, leaving only conflict elements.  See Note_03_28_2014 in this file.
                                LOG_INFO("%p: Local file/dir was created (%s); conflict with remote file!",
                                        this, destAbsPath.c_str());
                                // Perform "Conflict Resolution (during "copy back")" subroutine.
                                conflictResolutionDuringCopyBack(getRelPath(currDownloadEntry), true, tempFilePath, doRename);
                            } else { // Local file doesn't exist now.
                                // LocalFS was not changed; safe to do the rename.
                                doRename = true;
                            }
                        }

                        if (doRename) {
                            VPLTime_t normLocalTimeToSet = currDownloadEntry.client_reported_mtime;
                            VPLTime_t normLocalTimeGet = VPLTIME_INVALID;
                            VPLTime_t latestTimeToSet = getLatestNormLocalTimeToSet(tempFilePath);
                            if (latestTimeToSet < normLocalTimeToSet || normLocalTimeToSet == 0 ) {
                                // normLocalTimeToSet should never be 0 and the time should always be set in normal
                                // sync config operation; however, for the one-time migration, lastChangedNanoSec
                                // is never populated so normLocalTimeToSet is actually 0.
                                normLocalTimeToSet = latestTimeToSet;
                            }

#if 1 // VPL_PLAT_HAS_FILE_SET_TIME
                            // Set modified time of the temp file to client_reported_modify_time,
                            // but ensure that mtime of temp download is at least one FS-timestamp-precision
                            // unit less than current time.
                            {
                                VPLTime_t vplFileModTime = vcsTimeToVPLFileSetTime(tempFilePath, normLocalTimeToSet);
                                rc = VPLFile_SetTime(tempFilePath.c_str(), vplFileModTime);
                                if (rc < 0) {
                                    LOG_WARN("%p: VPLFile_SetTime(%s) failed: %d",
                                            this, tempFilePath.c_str(), rc);
                                    skip_ErrIncCopyback = true;
                                    goto skip;
                                }
                                LOG_INFO("%p: TimeSet(%s):"FMTu64,
                                        this, tempFilePath.c_str(), normLocalTimeToSet);
                            }
#else
                            // VPLFile_SetTime is not supported, (see bug 10105).
                            // We can work around this, but it does limit our optimizations.
#endif

                            {
                                // Due to the timestamp granularity of the filesystem, we might not
                                // get exactly what we asked for.
                                // Stat the file to find out the actual modified time.
                                VPLFS_stat_t statBuf;
                                rc = VPLFS_Stat(tempFilePath.c_str(), &statBuf);
                                if (rc != 0) {
                                    LOG_ERROR("%p: VPLFile_Stat(%s) failed:%d",
                                            this, tempFilePath.c_str(), rc);
                                    skip_ErrIncCopyback = true;
                                    goto skip;
                                }
                                normLocalTimeGet = fsTimeToNormLocalTime(tempFilePath, statBuf.vpl_mtime);
                                LOG_INFO("%p: TimeStat(%s):"FMTu64,
                                        this, tempFilePath.c_str(), statBuf.vpl_mtime);
                            }
                            // Ensure parent directory exists for the target.
                            rc = Util_CreateParentDir(destAbsPath.c_str());
                            if (rc < 0) {
                                LOG_ERROR("%p: Util_CreateParentDir(%s) failed: %d",
                                         this, destAbsPath.c_str(), rc);
                                // Copyback hopeless...
                                skip_ErrIncCopyback = true;
                                goto skip;
                            }
                            // Atomically rename the temp download to the local file.
                            rc = VPLFile_Rename(tempFilePath.c_str(), destAbsPath.c_str());
                            if (rc < 0) {
                                LOG_ERROR("%p: VPLFile_Rename(%s, %s) failed: %d",
                                          this, tempFilePath.c_str(),
                                          destAbsPath.c_str(), rc);
                                skip_ErrIncCopyback = true;
                                goto skip;
                            }

                            if (type == SYNC_TYPE_TWO_WAY_HOST_ARCHIVE_STORAGE) {
                                std::string decodedUrl;
                                rc = VPLHttp_DecodeUri(accessUrlSaved, decodedUrl); 
                                if (rc) { 
                                    LOG_CRITICAL("Unexpected error in decoding accessUrlSaved %s", accessUrlSaved.c_str());
                                } else if (decodedUrl.find(dataset_access_info.localStagingArea.urlPrefix) != std::string::npos) {
                                    // If the local device is a host archive storage and the file synced down matches 
                                    // the expected urlPrefix that associates with this archive storage, 
                                    // after retrieving file from staging area and renamed to target folder, 
                                    // calls VCS post url to update url
                                
                                    LOG_DEBUG("Calls vcs_post_archive_url for %s", currDatasetRelPath.c_str());
                                    for (u32 retries=0; retries < NUM_IMMEDIATE_TRANSIENT_RETRY; ++retries)
                                    {
                                        VPLHttp2 httpHandle;
                                        if (setHttpHandleAndCheckForStop(httpHandle)) { return; }
                                        rc = vcs_post_archive_url(vcs_session,
                                                                  httpHandle,   
                                                                  dataset,
                                                                  currDatasetRelPath.str(),
                                                                  currDownloadEntry.comp_id,
                                                                  currDownloadEntry.revision,
                                                                  verbose_http_log);
                                        if (clearHttpHandleAndCheckForPauseStop(httpHandle)) { return; }
                                        if (!isTransientError(rc)) { break; }
                                        LOG_WARN("%p: Transient Error:%d RetryIndex:%d", this, rc, retries);
                                        if (retries+1 < NUM_IMMEDIATE_TRANSIENT_RETRY) {  // pause between immediate retries
                                            if (checkForPauseStop(true, QUICK_RETRY_INTERVAL)) {
                                                return;
                                            }
                                        }
                                    }

                                    if (rc < 0) {
                                        LOG_ERROR("%p: vcs_post_archive_url(%s) failed: %d. Treat it as transient error, should retry",
                                                  this, currDatasetRelPath.c_str(), rc);
                                        skip_ErrIncDownload = true; // Force retry download process. Make sure get the latest info from VCS again
                                        goto skip;
                                    }

                                    // Record staging area file to be cleaned up. 
                                    VPLTime_t cleanupTime = VPLTime_GetTime() + VPLTime_FromSec(ASD_CLEANUP_DELAY_IN_SECS);
                                    LOG_INFO("Schedule cleanup %s @ "FMTu64, decodedUrl.c_str(), (u64)cleanupTime);
                                    size_t pos = decodedUrl.find_last_of("/");       
                                    if (pos != std::string::npos) {    
                                        std::string filename = decodedUrl.substr(pos+1);
                                        rc = localDb.asdStagingAreaCleanUp_add(decodedUrl, filename, cleanupTime);
                                        if (rc < 0) {
                                            LOG_CRITICAL("%p: asdStagingAreaCleanUp_add(%s) failed: %d",
                                                    this, decodedUrl.c_str(), rc);
                                            HANDLE_DB_FAILURE();
                                            goto skip;
                                        }
                                    }
                                }
                            }

                            // Update the LocalDB with the downloaded file's metadata
                            // (timestamp, compId, revision, hash_value, file_size).
                            {
                                rc = localDb.syncHistoryTree_updateEntryInfo(
                                        currDbEntry.parent_path,
                                        currDbEntry.name,
                                        true, normLocalTimeGet,
                                        true, currDownloadEntry.comp_id,
                                        true, currDownloadEntry.revision,
                                        currDownloadEntry.hash_value,
                                        currDownloadEntry.file_size_exists,
                                        currDownloadEntry.file_size);
                                if (rc < 0) {
                                    LOG_CRITICAL("%p: syncHistoryTree_updateEntryInfo(%s) failed: %d",
                                            this, currRelPath.c_str(), rc);
                                    HANDLE_DB_FAILURE();
                                    goto skip;
                                }
                            }

                            reportFileDownloaded(false, currRelPath, currDownloadEntry.is_dir, !fileExistsOnFs);
                            LOG_INFO("%p: ACTION DOWN download done:%s -> %s,mtime("FMTu64")",
                                     this, currRelPath.c_str(), destAbsPath.c_str(),
                                     normLocalTimeToSet);
                        } // if (doRename)
                    }
                } // "copyback"
            } // download and copyback steps
          skip:
            if (skip_ErrIncDownload) {
                if (currDownloadEntry.download_err_count > sync_policy.max_error_retry_count) {
                    LOG_ERROR("%p: Abandoning error:%s,%s,"FMTu64","FMTu64
                              " -- Attempted "FMTu64" times",
                              this,
                              currDownloadEntry.parent_path.c_str(),
                              currDownloadEntry.name.c_str(),
                              currDownloadEntry.comp_id,
                              currDownloadEntry.revision,
                              currDownloadEntry.download_err_count);
                    rc = downloadChangeLog_remove_and_updateRemaining(currDownloadEntry.row_id);
                    if (rc < 0) {
                        LOG_ERROR("%p: downloadChangeLog_remove_and_updateRemaining("FMTu64") failed: %d",
                                this, currDownloadEntry.row_id, rc);
                        return;
                    }
                } else {
                    rc = localDb.downloadChangeLog_incErrDownload(currDownloadEntry.row_id);
                    if (rc < 0) {
                        LOG_CRITICAL("%p: downloadChangeLog_incErrDownload("FMTu64") failed: %d",
                                this, currDownloadEntry.row_id, rc);
                        HANDLE_DB_FAILURE();
                        return;
                    }
                    MutexAutoLock lock(&mutex);
                    setErrorTimeout(sync_policy.error_retry_interval);
                }
            } else if (skip_ErrIncCopyback) {
                if (currDownloadEntry.copyback_err_count > sync_policy.max_error_retry_count) {
                    LOG_ERROR("%p: Abandoning error:%s,%s,"FMTu64","FMTu64
                              " -- Attempted "FMTu64" times",
                              this,
                              currDownloadEntry.parent_path.c_str(),
                              currDownloadEntry.name.c_str(),
                              currDownloadEntry.comp_id,
                              currDownloadEntry.revision,
                              currDownloadEntry.copyback_err_count);
                    rc = downloadChangeLog_remove_and_updateRemaining(currDownloadEntry.row_id);
                    if (rc < 0) {
                        LOG_ERROR("%p: downloadChangeLog_remove_and_updateRemaining("FMTu64") failed: %d",
                                this, currDownloadEntry.row_id, rc);
                        return;
                    }
                } else {
                    rc = localDb.downloadChangeLog_incErrCopyback(currDownloadEntry.row_id);
                    if (rc < 0) {
                        LOG_CRITICAL("%p: downloadChangeLog_incErrCopyback("FMTu64") failed: %d",
                                this, currDownloadEntry.row_id, rc);
                        HANDLE_DB_FAILURE();
                        return;
                    }
                    MutexAutoLock lock(&mutex);
                    setErrorTimeout(sync_policy.error_retry_interval);
                }
            }else{
                rc = downloadChangeLog_remove_and_updateRemaining(currDownloadEntry.row_id);
                if (rc < 0) {
                    LOG_ERROR("%p: downloadChangeLog_remove_and_updateRemaining("FMTu64") failed: %d",
                            this, currDownloadEntry.row_id, rc);
                    return;
                }
            }
        } // while ((rc = localDb.downloadChangeLog_get(currDownloadEntry)) == 0)
        if (rc != SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) {
            LOG_CRITICAL("%p: downloadChangeLog_get() failed: %d", this, rc);
            HANDLE_DB_FAILURE();
        }
    }

    void reportFileUploaded(bool isDeletion, const SCRelPath& relPath, bool isDir,
            bool isNewFile)
    {
        if (event_cb != NULL) {
            AbsPath absPath = getAbsPath(relPath);
            SyncConfigEventDetailType detailType =
                convertDetailType(isDeletion, isDir, isNewFile);;
            u64 event_time = VPLTime_GetTime();
            SyncConfigEventEntryUploaded event(*this, callback_context,
                    user_id, dataset.id, relPath.str(), absPath.str(),
                    detailType, event_time);
            event_cb(event);
        }
    }

    void reportFileConflict(const std::string& abs_orig_path,
         const std::string& abs_conflict_rename_path)
    {
        if (event_cb != NULL) {
            SyncConfigEventDetailType detailType =
                SYNC_CONFIG_EVENT_DETAIL_CONFLICT_FILE_CREATED;
            u64 event_time = VPLTime_GetTime();
            SyncConfigEventEntryDownloaded event(*this, callback_context,
                    user_id, dataset.id, false, std::string(""), abs_conflict_rename_path,
                    detailType, event_time, abs_orig_path);
            event_cb(event);
        }
    }

    void conflictResolutionDuringCopyBack(const SCRelPath& currRelPath,
                                          bool compareContents,
                                          const AbsPath& copyBackTempAbsPath,
                                          bool& proceedWithRenamingTempDownload)
    {
        switch (sync_policy.how_to_determine_winner) {
        case SyncPolicy::SYNC_CONFLICT_POLICY_FIRST_TO_SERVER_WINS:
        {
            if (sync_policy.what_to_do_with_loser ==
                SyncPolicy::SYNC_CONFLICT_POLICY_PROPAGATE_LOSER)
            {
                // Do byte-by-byte comparison, if files are different propagate
                //   loser.
                // OPTIMIZATION: With hashes, the extra download, or even
                //   previous upload, doesn't even have to be done.
                AbsPath destAbsPath = getAbsPath(currRelPath);
                bool contentsEqual = false;
                if(compareContents) {
                    contentsEqual = isFileContentsEqual(destAbsPath, copyBackTempAbsPath);
                }
                if (!contentsEqual) {
                    std::string toRename = getConflictRenamePath(destAbsPath, dataset_access_info.deviceName);
                    LOG_INFO("%p: ACTION RESOLVE CONFLICT propagate loser file/dir (%s)->(%s)",
                             this, destAbsPath.c_str(), toRename.c_str());
                    int rc = VPLFile_Rename(destAbsPath.c_str(), toRename.c_str());
                    if (rc != 0) {
                        LOG_ERROR("%p: VPLFile_Rename(%s->%s):%d",
                                  this, destAbsPath.c_str(), toRename.c_str(), rc);
                    } else {
                        proceedWithRenamingTempDownload = true;
                        reportFileConflict(destAbsPath.str(), toRename);
                    }
                } else {
                    LOG_INFO("%p: ACTION RESOLVE CONFLICT file identical, continue with rename (%s)",
                             this, destAbsPath.c_str());
                    // Continuing with rename for simplicity:  Rename/updateSyncDb is part of the same step.
                    proceedWithRenamingTempDownload = true;
                }

            } else if (sync_policy.what_to_do_with_loser ==
                       SyncPolicy::SYNC_CONFLICT_POLICY_DELETE_LOSER)
            {
                // Delete the conflicting local file or directory.
                AbsPath destAbsPath = getAbsPath(currRelPath);
                LOG_INFO("%p: ACTION RESOLVE CONFLICT delete file/dir \"%s\"", this, destAbsPath.c_str());
                int rc = Util_rmRecursive(destAbsPath.str(), getTempDeleteDir().str());
                if ((rc < 0) && (rc != VPL_ERR_NOENT)) {
                    LOG_WARN("%p: Util_rmRecursive(%s,%s) failed:%d", this,
                             destAbsPath.c_str(), getTempDeleteDir().c_str(), rc);
                    // We will try to continue anyway.
                }
                proceedWithRenamingTempDownload = true;
            } else {
                LOG_ERROR("%p: Unsupported sync_policy.what_to_do_with_loser: %d",
                        this, (int)sync_policy.what_to_do_with_loser);
            }
        }
        break;
        case SyncPolicy::SYNC_CONFLICT_POLICY_LATEST_MODIFY_TIME_WINS:
        default:
            LOG_ERROR("%p: Unsupported sync_policy.how_to_determine_winner: %d",
                    this, (int)sync_policy.how_to_determine_winner);
            break;
        }
    }

    // Signature of this method should match vcsPostFileMetadataAfterUpload except
    // for missing compId and revision, because it will basically call the
    // vcsPostFileMetadataAfterUpload function to create a new file.
    // auto-resolve to preserve data in case of conflict.
    int conflictResolutionVcsFileDeletedDuringUpload(
                              const std::string& currUploadEntryDatasetRelPath,
                              u64 parentCompId,
                              VPLTime_t modifyTime,
                              VPLTime_t createTime,
                              u64 fileSize,
                              const std::string& accessUrl,
                              const SCRow_uploadChangeLog& currUploadEntry,
                              const std::string& fileHashValue)
    {
        // Auto-resolve the conflict to preserve data.
        // If the file was deleted on VCS while upload was occurring, simply
        // create a new file.
        int rc;
        LOG_INFO("%p: ACTION RESOLVE CONFLICT fileChanged/fileDeleted AutoResolve to preserve data:\"%s\"",
                this, getAbsPath(getRelPath(currUploadEntry)).c_str());
        rc = vcsPostFileMetadataAfterUpload(currUploadEntryDatasetRelPath,
                                            parentCompId,
                                            false,  // AutoResolve conflict to create new file.
                                            0,
                                            1,      // AutoResolve conflict to create new file.
                                            modifyTime,
                                            createTime,
                                            fileSize,
                                            true,
                                            accessUrl,
                                            currUploadEntry,
                                            fileHashValue);
        return rc;
    }

    void deferredUploadLoop() { }

    bool isStagingFileScheduledForDelete(const std::string filename)
    {
        int rc;
        SCRow_asdStagingAreaCleanUp entry;
        rc = localDb.asdStagingAreaCleanUp_getRow(filename, entry);
        return (rc == 0);
    }

    bool isStagingAreaMetadataFile(const std::string filename) const
    {
        size_t pos = filename.find_last_of("."); 
        if (pos == std::string::npos) {
            return false;
        }
        return (filename.substr(pos+1) == "meta");
    }

    // Check if the file is orphaned. VCS calls needs to be made to compare with the information
    // recorded in the metadata file of the staging file. We do not do retry here if there is
    // error in accessing VCS. This is not critical sync job. Will just retry it in the next
    // opportunity
    bool isStagingFileOrphaned(const std::string filePath)
    {
        // This can potentially take a long time, so we must not be holding the mutex.
        // Holding the mutex here can cause the ANS callback to block, leading
        // to ANS disconnection.  See https://bugs.ctbg.acer.com/show_bug.cgi?id=17932.
        ASSERT(!VPLMutex_LockedSelf(&mutex));

        std::string metafilePath = filePath + ASD_CLEANUP_META_FILE_EXT;
        ProtobufFileReader reader;
        bool hasCompId = false;
        u64 componentId;
        u64 revisionId;
        std::string relPath;
        int rc; 

        rc = reader.open(metafilePath.c_str(), true);
        if (rc < 0) {
            LOG_ERROR("[%p]: Failed to open metadata file %s for read", this, metafilePath.c_str());
            return false;
        }

        google::protobuf::io::CodedInputStream tempStream(reader.getInputStream());
        rc = readStagingMetadataFile(tempStream, hasCompId, componentId, revisionId, relPath);
        if (rc < 0) {
            LOG_ERROR("[%p]: Failed to read metadata file %s", this, metafilePath.c_str());
            return false;
        }

        if (!hasCompId) {
            // Handle the case where the staging file corresponds to newly created file
            VPLHttp2 httpHandle;
            if (setHttpHandleAndCheckForStop(httpHandle)) { return false; }
            rc = vcs_get_comp_id(vcs_session,
                                 httpHandle, 
                                 dataset,
                                 relPath,
                                 verbose_http_log,
                                 componentId);
            if (clearHttpHandleAndCheckForPauseStop(httpHandle)) { return false; }
            if (rc < 0) {
                LOG_WARN("[%p]: Failed to GET compid %s, metaFile(%s)", this, relPath.c_str(), metafilePath.c_str());
                return false;
            }

            revisionId = 1;     // revision = 1 for new files 
        }

        {
            VcsFileMetadataResponse filemetadataResp;
            VPLHttp2 httpHandle;
            if (setHttpHandleAndCheckForStop(httpHandle)) { return false; }
            rc = vcs_get_file_metadata(vcs_session,
                                       httpHandle, 
                                       dataset,
                                       relPath,
                                       componentId,
                                       verbose_http_log,
                                       filemetadataResp);
            if (clearHttpHandleAndCheckForPauseStop(httpHandle)) { return false; }
            if (rc < 0) {
                LOG_ERROR("[%p]: Failed to GET filemetadata %s, metaFile(%s)", this, relPath.c_str(), metafilePath.c_str());
                return false;
            }

            // Get the latest revision
            u64 latestRevision = 0;
            for (std::vector<VcsFileRevision>::const_iterator it = filemetadataResp.revisionList.begin();
                 it != filemetadataResp.revisionList.end(); ++it)
            {
                if (it->revision > latestRevision)
                    latestRevision = it->revision;
            }

            if (latestRevision > revisionId) {
                // Revision has changed. The staging file is orphaned
                // If this staging file's revision is larger than vcs,
                //  it is not orphaned. this file may still be transferring
                //  and haven't post it meta data to vcs yet.
                LOG_DEBUG("[%p]: Staging file revision "FMTu64" orphaned. Latest revision "FMTu64, this, revisionId, latestRevision);
                return true;
            }
        }

        {
            VPLHttp2 httpHandle;
            VcsAccessInfo accessInfoResp;

            if (setHttpHandleAndCheckForStop(httpHandle)) { return false; }
            rc = vcs_access_info_for_file_get(vcs_session, 
                                              httpHandle, 
                                              dataset,
                                              relPath,
                                              componentId,
                                              revisionId,
                                              verbose_http_log,
                                              accessInfoResp);
            if (clearHttpHandleAndCheckForPauseStop(httpHandle)) { return false; }
            if (rc < 0) {
                LOG_ERROR("[%p]: Failed to GET accessinfo %s", this, relPath.c_str());
                return false;
            }

            std::string decodedUrl;
            rc = VPLHttp_DecodeUri(accessInfoResp.accessUrl, decodedUrl); 
            if (rc) { 
                LOG_ERROR("[%p]: Unexpected error in decoding accessUrlSaved %s", this, accessInfoResp.accessUrl.c_str());
                return false;
            } 

            size_t pos;
            pos = decodedUrl.find(dataset_access_info.localStagingArea.urlPrefix); 
            if (pos != std::string::npos) {
                // The url corresponds to this archive storage 
                size_t pos1, pos2;
                pos1 = decodedUrl.find_last_of("/");
                pos2 = filePath.find_last_of("/");
                if (pos1 != std::string::npos && pos2 != std::string::npos) {
                    if (decodedUrl.substr(pos1+1) != filePath.substr(pos2+1)) {
                        // Another staging file replaced this one
                        LOG_DEBUG("[%p]: Staging file (access_url %s) orphaned. Latest access_url (%s)", 
                                  this, filePath.c_str(), decodedUrl.c_str());  
                        return true;    
                    }
                }
            } else {
                // This is no longer the archive storage for the file
                LOG_DEBUG("[%p]: Staging file (access_url %s) orphaned. Latest access_url (%s)", 
                          this, filePath.c_str(), decodedUrl.c_str());  
                return true;            
            }
        }

        return false;
    }

    int archiveStorageStagingAreaCleanup(VPLTime_t& cleanupTimeout_out)
    {
        // This can potentially take a long time, so we must not be holding the mutex.
        // Holding the mutex here can cause the ANS callback to block, leading
        // to ANS disconnection.  See https://bugs.ctbg.acer.com/show_bug.cgi?id=17932.
        ASSERT(!VPLMutex_LockedSelf(&mutex));
        // If this is a syncbox archive storage, go through the list of staging area 
        // file ready for cleanup. For files that are not ready yet, setup the timeout
        SCRow_asdStagingAreaCleanUp cleanupEntry;
        int rc = 0;
        while ((rc = localDb.asdStagingAreaCleanUp_get(cleanupEntry)) == 0) 
        {
            if (cleanupEntry.cleanup_time <= VPLTime_GetTime()) {
                size_t pos = cleanupEntry.access_url.find_last_of("/");
                std::string filename = cleanupEntry.access_url.substr(pos+1);
                std::string fileToClean = dataset_access_info.localStagingArea.absPath;
                fileToClean.append("/");
                fileToClean.append(filename);
                std::string metafileToClean = fileToClean + ASD_CLEANUP_META_FILE_EXT;

                LOG_INFO("Removing staging area file %s", fileToClean.c_str());
                rc = VPLFile_Delete(fileToClean.c_str());
                if (rc == 0 || rc == VPL_ERR_NOENT) {
                    rc = VPLFile_Delete(metafileToClean.c_str());
                    if ((rc != 0) && (rc != VPL_ERR_NOENT)) {
                        LOG_ERROR("[%p]: Fail to delete metafile %s", this, metafileToClean.c_str());
                    } else {
                        rc = 0; 
                    }
                } else {
                    LOG_ERROR("[%p]: Fail to delete file %s", this, fileToClean.c_str());
                }

                if (rc != 0) { 
                    VPLTime_t cleanupTime = VPLTime_GetTime() + VPLTime_FromSec(ASD_CLEANUP_DELAY_IN_SECS);
                    LOG_ERROR("Reschedule file %s for deletion @ "FMTu64, fileToClean.c_str(), (u64)cleanupTime);
                    // Add it back to the end of the list and schedule to be cleaned later
                    int dberr = localDb.asdStagingAreaCleanUp_add(cleanupEntry.access_url, filename, cleanupTime);
                    if (dberr < 0) {
                        LOG_CRITICAL("[%p]: asdStagingAreaCleanUp_add(%s) failed: %d", this, cleanupEntry.access_url.c_str(), dberr);
                        HANDLE_DB_FAILURE();
                        break;
                    }
                }

                rc = localDb.asdStagingAreaCleanUp_remove(cleanupEntry.row_id);
                if (rc < 0) {
                    LOG_CRITICAL("[%p]: asdStagingAreaCleanUp_remove("FMTu64") failed: %d", this, cleanupEntry.row_id, rc);
                    HANDLE_DB_FAILURE();
                    break;
                }
            } else {
                // Not the right time yet. Defer
                cleanupTimeout_out = VPLTime_DiffClamp(cleanupEntry.cleanup_time, VPLTime_GetTime());
                LOG_DEBUG("[%p]: Now "FMTu64". Next cleanup time @ "FMTu64, this, VPLTime_GetTime(), (u64)cleanupEntry.cleanup_time);
                break;
            }
        }

        // Orphaned file cleanup step
        {
            VPLFS_dir_t dirStream;
            int rc = VPLFS_Opendir(dataset_access_info.localStagingArea.absPath.c_str(), &dirStream);
            if (rc == 0) {
                VPLFS_dirent_t dirEntry;
                while ((rc = VPLFS_Readdir(&dirStream, &dirEntry)) == VPL_OK) {
                    if (dirEntry.type == VPLFS_TYPE_FILE) {
                        std::string filePath = dataset_access_info.localStagingArea.absPath;
                        filePath.append("/");
                        filePath.append(dirEntry.filename);

                        VPLFS_stat_t statBuf;
                        rc = VPLFS_Stat(filePath.c_str(), &statBuf);
                        if (rc != 0) 
                            continue;

                        if (isStagingAreaMetadataFile(dirEntry.filename))
                            continue;

                        if (!isStagingFileScheduledForDelete(dirEntry.filename)) // Ignore file already scheduled for deletion 
                        { 
                            if (isStagingFileOrphaned(filePath)) {
                                std::string metafileToClean = filePath + ASD_CLEANUP_META_FILE_EXT;
                                LOG_INFO("[%p]:Removing orphaned staging area file %s", this, filePath.c_str());
                                rc = VPLFile_Delete(filePath.c_str());
                                if (rc == 0 || rc == VPL_ERR_NOENT) {
                                    rc = VPLFile_Delete(metafileToClean.c_str());
                                    if ((rc != 0) && (rc != VPL_ERR_NOENT)) {
                                        LOG_ERROR("[%p]: Fail to delete metafile %s", this, metafileToClean.c_str());
                                    } else {
                                        rc = 0; 
                                    }
                                } else {
                                    LOG_ERROR("[%p]: Fail to delete file %s", this, filePath.c_str());
                                }

                                if (rc != 0) { 
                                    // Schedule the file for later cleanup if file deletion somehow fails
                                    VPLTime_t cleanupTime = VPLTime_GetTime() + VPLTime_FromSec(ASD_CLEANUP_DELAY_IN_SECS);
                                    std::string cleanupUrl = dataset_access_info.localStagingArea.urlPrefix + dirEntry.filename;
                                    LOG_ERROR("[%p]: Reschedule file %s for deletion @ "FMTu64, this, filePath.c_str(), (u64)cleanupTime);
                                    int dberr = localDb.asdStagingAreaCleanUp_add(cleanupUrl, dirEntry.filename, cleanupTime);
                                    if (dberr < 0) {
                                        LOG_CRITICAL("[%p]: asdStagingAreaCleanUp_add(%s) failed: %d", this, cleanupUrl.c_str(), dberr);
                                        HANDLE_DB_FAILURE();
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            ON_BLOCK_EXIT(VPLFS_Closedir, &dirStream);
        }

        return rc;
    }

    void workerLoop()
    {
        u64 rootCompId = 0;
        MutexAutoLock lock(&mutex);

        // Loop.
        while (1) {
            int enqueueDlScanRes = -1;
            if (checkForPauseStop()) {
                return;
            }

            if (isTimeToRetryErrors()) {
                LOG_INFO("%p: Retrying errors", this);

                // Clear this before starting, so we will try again if there was an error
                // but the archive storage device came online while applying the changelogs.
                clearForceRetry();

                // Do errors
                lock.UnlockNow();

                ApplyUploadChangeLog(true);
                ApplyDownloadChangeLog(DL_CHANGE_LOG_ERROR_MODE_DOWNLOAD);
                ApplyDownloadChangeLog(DL_CHANGE_LOG_ERROR_MODE_COPYBACK);
                // Do unfinished changeLogs
                // TODO: Is this really a good idea?
                ApplyUploadChangeLog(false);
                ApplyDownloadChangeLog(DL_CHANGE_LOG_ERROR_MODE_NONE);

                lock.Relock(&mutex);
            } else {
                // Without this here, once force_error_retry_timestamp is set, we would do an
                // immediate error retry next time we finish a worker loop iteration with an error.
                // We only want that to happen if the archive storage device actually came online
                // during that iteration.
                clearForceRetry();
            }

            // TODO: temporarily always doing a full scan
            if (incrementalLocalScanPaths.size() > 0) {
                full_local_scan_requested = true;
            }

            if (full_local_scan_requested || uploadScanError) {
                full_local_scan_requested = false;
                uploadScanError = false;
                incrementalLocalScanPaths.clear();
                lock.UnlockNow();

                if(rootCompId == 0) {
                    // Creating root directory on VCS here.  Most likely place
                    // to discover network not connected.
                    int rc;
                    // Empty uploadChangeLog represents root directory
                    SCRow_uploadChangeLog root;
                    rc = getParentCompId(root, rootCompId);
                    if (isTransientError(rc)) {
                        MutexAutoLock scopelock(&mutex);
                        setErrorTimeout(sync_policy.error_retry_interval);
                        uploadScanError = true;
                    } else if (rc != 0) {
                        LOG_ERROR("Cannot perform upload sync -- root cannot be created:%d", rc);
                        setErrorNeedsRemoteScan();
                    } else {
                        LOG_INFO("Root created on VCS:%s,"FMTu64,
                                 server_dir.c_str(), rootCompId);
                    }
                }
                if (rootCompId != 0) {
                    PerformFullLocalScan();
                    if (checkForPauseStop()) {
                        return;
                    }
                }
            } else {
                // TODO: remove item from incrementalLocalScanSet;
                // TODO: update UploadChangeLog;
                lock.UnlockNow();
            }

            ASSERT(!VPLMutex_LockedSelf(&mutex));
            if(rootCompId != 0) {
                ApplyUploadChangeLog(false);
                if (checkForPauseStop()) {
                    return;
                }
            }
            lock.Relock(&mutex);

            // If true, we are certain that the dataset version has increased.
            bool vcsScanNeeded = isRemoteScanPending();
            // If we are uncertain whether the dataset version increased or not, use the inexpensive
            // VCS call to check the dataset version now.
            if ((dataset_version_check_requested || dataset_version_check_error) && !vcsScanNeeded) {
                dataset_version_check_requested = false;
                dataset_version_check_error = false;
                lock.UnlockNow();
                // Updates vcsScanNeeded to true if we can benefit from a server scan.
                int rc = checkDatasetVersionChanged(/*out*/ vcsScanNeeded);
                if (rc == SYNC_AGENT_ERR_STOPPING) {
                    return;
                }
                lock.Relock(&mutex);
            }
            if (vcsScanNeeded) {
                dataset_version_check_requested = false;
                dataset_version_check_error = false;
                setDownloadScanRequested(false);
                setDownloadScanError(false);
                lock.UnlockNow();
                {
                    enqueueDlScanRes = enqueueRootToNeedDownloadScan();
                    if (enqueueDlScanRes == 0) {
                        ScanRemoteChangesAndApplyDeletes();
                        if (checkForPauseStop()) {
                            return;
                        }
                    } else if (enqueueDlScanRes == SYNC_AGENT_ERR_STOPPING) {
                        return;
                    } else if (isTransientError(enqueueDlScanRes)) {
                        setErrorNeedsRemoteScan();
                    } else if (enqueueDlScanRes == VCS_ERR_COMPONENT_NOT_FOUND ||
                               enqueueDlScanRes == VCS_ERR_PATH_DOESNT_POINT_TO_KNOWN_COMPONENT)
                    { // TODO: Bug 12976: Is VCS_ERR_COMPONENT_NOT_FOUND even possible from vcs_get_comp_id?
                        LOG_WARN("%p: No SyncConfig root(%s):%d -- nothing to download",
                                this, server_dir.c_str(), enqueueDlScanRes);
                    } else {
                        // TODO: Bug 12976: What is the correct behavior in this case?  For example,
                        //   perhaps the dataset was suspended, shouldn't we try again later?
                        LOG_ERROR("%p: Unexpected error getting compId for sync config root:%d",
                                this, enqueueDlScanRes);
                    }
                }
            } else {
                lock.UnlockNow();
            }

            ASSERT(!VPLMutex_LockedSelf(&mutex));
            // TODO: Bug 12976: do we really need to special case skipping this?  Won't the
            //   changelog be empty if we couldn't enqueue the root?
            if(enqueueDlScanRes==0) {
                ApplyDownloadChangeLog(DL_CHANGE_LOG_ERROR_MODE_NONE);
                if (checkForPauseStop()) {
                    return;
                }
            }

            lock.Relock(&mutex);

            // If this scan/apply pass cleared out all of the errors, reset
            // next_error_processing_timestamp so that setStatus() will do the right thing.
            clearErrorRetryIfNoErrorsRemain();

            if (hasScanToDo()) {
                setStatus(SYNC_CONFIG_STATUS_SCANNING);
            } else {
                setStatus(SYNC_CONFIG_STATUS_DONE);
            }

            VPLTime_t asdCleanupTimeout = VPL_TIMEOUT_NONE; 
            if (type == SYNC_TYPE_TWO_WAY_HOST_ARCHIVE_STORAGE) {
                lock.UnlockNow();
                archiveStorageStagingAreaCleanup(/*out*/ asdCleanupTimeout);
                lock.Relock(&mutex);
            }

            // Wait for work to do.
            while (!checkForWorkToDo()) {  // This will also check error timers
                ASSERT(VPLMutex_LockedSelf(&mutex));
                if (checkForPauseStop()) {
                    return;
                }

                VPLTime_t errTimeout = getTimeoutUntilNextRetryErrors();
                VPLTime_t timeout;
                if (asdCleanupTimeout == VPL_TIMEOUT_NONE) {
                    timeout = errTimeout;
                } else if (errTimeout == VPL_TIMEOUT_NONE) {
                    timeout = asdCleanupTimeout;
                } else {
                    timeout = MIN(asdCleanupTimeout, errTimeout);
                }
                asdCleanupTimeout = VPL_TIMEOUT_NONE; 

                LOG_DEBUG("timeout ("FMTu64") next_error_processing_timestamp("FMTu64")", 
                          timeout, next_error_processing_timestamp);

                int rc = VPLCond_TimedWait(&work_to_do_cond_var, &mutex, timeout);
                if ((rc < 0) && (rc != VPL_ERR_TIMEOUT)) {
                    LOG_WARN("%p: VPLCond_TimedWait failed: %d", this, rc);
                }

                // When there is no work to do, still perform staging area cleanup
                if (type == SYNC_TYPE_TWO_WAY_HOST_ARCHIVE_STORAGE) {
                    lock.UnlockNow();
                    archiveStorageStagingAreaCleanup(asdCleanupTimeout);
                    lock.Relock(&mutex);
                }

                if (checkForPauseStop()) {
                    return;
                }
            }
        } // while (1)
    }
};
