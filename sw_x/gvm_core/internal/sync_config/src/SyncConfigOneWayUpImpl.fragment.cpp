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

/// Design choice: We are very conservative about when to do a VCS scan.
/// We only scan when the localDB is first created or when we get an
/// error that implies that we need to rescan VCS.
///
/// This prevents the case when there is some misconfiguration and two
/// clients both attempt to upload to the same directory.
/// For example, assume that we actively monitored the VCS directory:
/// - Assume Client A has file X.
/// - Assume Client B doesn't have file X.
/// 1. Client A uploads file X.
/// 2. Client B scans VCS and learns of file X.
/// 3. Client B doesn't have file X, so it deletes it.
/// 4. Client A scan VCS and sees that file X is missing.
/// 5. Client A has file X, so... (goto step 1)
///
/// The downside of this choice is that if something is unexpectedly removed or added
/// in VCS, SyncConfigOneWayUpImpl probably won't find out for a long time, if ever
/// (since it only takes action in response to local changes).

class SyncConfigOneWayUpImpl : public SyncConfigImpl
{
private:
    SyncConfigOneWayUpImpl(
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
            void* callback_context)
    :  SyncConfigImpl(user_id, dataset, type,
                      sync_policy, local_dir, server_dir,
                      dataset_access_info,
                      thread_pool, make_dedicated_thread,
                      event_cb, callback_context)
    {}
    // Search this file for "const static" for any internal constants.

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

    virtual void setDownloadScanRequested(bool value)
    {
        ASSERT(VPLMutex_LockedSelf(&mutex));
        download_scan_requested = value;
    }

    virtual void setDownloadScanError(bool value)
    {
        ASSERT(VPLMutex_LockedSelf(&mutex));
        downloadScanError = value;
    }

    virtual bool isRemoteScanPending()
    {
        // Not currently supported for OneWay; always return false.
        return false;
    }

    virtual int updateUploadsRemaining()
    {
        // Not currently supported for OneWay; always return 0.
        return 0;
    }
    virtual int updateDownloadsRemaining()
    {
        // Not currently supported for OneWay; always return 0.
        return 0;
    }

    virtual int GetSyncStateForPath(const std::string& abs_path_ts,
                                    SyncConfigStateType_t& state__out,
                                    u64& dataset_id__out,
                                    bool& is_sync_folder_root)
    {
        return CCD_ERROR_NOT_IMPLEMENTED;
    }

    void addFileDeleteToUploadChangeLog(const SCRow_syncHistoryTree& fileToDelete)
    {
        ASSERT(!fileToDelete.is_dir);
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
                                    std::string(""), false, 0);
    }

    void setErrorNeedsRemoteScan(VPLTime_t minimumTimeout)
    {
        MutexAutoLock lock(&mutex);
        initial_scan_requested_OneWay = true;
        setErrorTimeout(minimumTimeout);
    }

    void setErrorNeedsRemoteScan()
    {
        setErrorNeedsRemoteScan(sync_policy.error_retry_interval);
    }

    /// Perform a scan of VCS for the specified #SCRow_needDownloadScan.
    /// This just updates the localDB syncHistoryTree so that we can make the right
    /// decisions during the local scan.
    /// We will also set the status to SYNCING if we see any components that we don't already know about.
    /// @return true if the VCS scan was successful.
    bool initialServerScan(const SCRow_needDownloadScan& currScanDir)
    {
        int rc = 0;
        SCRelPath currScanDirRelPath = getRelPath(currScanDir);
        DatasetRelPath currScanDirDatasetRelPath = getDatasetRelPath(currScanDirRelPath);

        // VCS returns pages of fileList and currentDirVersion. (Repeat if multiple pages of results.)
        u64 filesSeen = 0;
        u64 startingDirVersion = -1; // -1 indicates uninitialized.
        VcsGetDirResponse getDirResp;
        do
        {
            for (u32 retries=0; retries < NUM_IMMEDIATE_TRANSIENT_RETRY; ++retries)
            {
                VPLHttp2 httpHandle;
                if (setHttpHandleAndCheckForStop(httpHandle)) { return false; }
                rc = vcs_read_folder_paged(vcs_session, httpHandle, dataset,
                                           currScanDirDatasetRelPath.str(),
                                           currScanDir.comp_id,
                                           (filesSeen + 1), // VCS starts at 1 instead of 0 for some reason
                                           VCS_GET_DIR_PAGE_SIZE,
                                           verbose_http_log,
                                           getDirResp);
                if (clearHttpHandleAndCheckForPauseStop(httpHandle)) { return false; }
                if (!isTransientError(rc)) { break; }
                LOG_WARN("%p: Transient Error:%d RetryIndex:%d", this, rc, retries);
                if (retries+1 < NUM_IMMEDIATE_TRANSIENT_RETRY) {  // pause between immediate retries
                    if (checkForPauseStop(true, QUICK_RETRY_INTERVAL)) {
                        return false;
                    }
                }
            }
            if (rc < 0) {
                LOG_WARN("%p: vcs_read_folder(%s) failed: %d", this, currScanDirDatasetRelPath.c_str(), rc);
                // We intentionally do not request a new initial scan in this case.
                // For a VCS hierarchy with lots of subdirectories, it might take a very long time
                // to get a continuous sequence of successes for all GET dir calls, particularly
                // if the user has an unreliable network connection or turns off their machine frequently.
                // We can tolerate the failure, because if we successfully scanned this directory
                // in the past and it hasn't changed, we will already have the correct VCS state in
                // the localDB.
                // If there really was an important change on VCS that we missed, we expect to find out
                // when we try to upload the files within this directory (POST filemetadata should
                // return VCS_UPLOADREVISION_NOT_HWM_PLUS_1, which will cause us to queue up another
                // initial scan.

#if 0 // TODO: bug 10255: needs to be tested better.
                // Special case; we won't recover without this.
                if (currScanDirRelPath.isSCRoot() && (rc == VCS_ERR_PROVIDED_PATH_DOESNT_MATCH_PROVIDED_COMPID)) {
                    // SyncConfig root dir has been removed (or recreated).  Must fix our local sync history tree to recover.
                    LOG_WARN("%p: Removing compId for sync config root (%s)", this, currScanDirDatasetRelPath.c_str());
                    localDb.syncHistoryTree_updateCompId("", "", false, 0, false, 0);
                }
#endif
                return false;
            }
            if (startingDirVersion == -1) {
                startingDirVersion = getDirResp.currentDirVersion;
            }

            // Each fileList element has the following relevant fields: type (file|dir), compId,
            //     latestRevision.revision. latestRevision.hash.

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
                    // In case we are recovering from a migration or lost local DB, check
                    // the local FS to see if we already have the proper file:
                    SCRelPath currEntryDirPath = currScanDirRelPath.getChild(it->name);
                    AbsPath currEntryAbsPath = getAbsPath(currEntryDirPath);
                    VPLTime_t localFsMtime = 0; // dummy init for linux build
                    if (checkIfLocalFileMatches(currEntryAbsPath, *it, localFsMtime)) {
                        LOG_INFO("%p: Skipping upload for \"%s\". local file(time:"FMTu64") matches VCS.",
                                this, currEntryDirPath.c_str(), localFsMtime);
                        // Create file/dir record in LocalDB.
                        SCRow_syncHistoryTree newEntry;
                        newEntry.parent_path = currScanDirRelPath.str();
                        newEntry.name = it->name;
                        newEntry.is_dir = false;
                        newEntry.comp_id_exists = true;
                        newEntry.comp_id = it->compId;
                        newEntry.revision_exists = true;
                        newEntry.revision = it->latestRevision.revision;
                        newEntry.local_mtime_exists = true;
                        newEntry.local_mtime = fsTimeToNormLocalTime(currEntryAbsPath, localFsMtime);
                        newEntry.last_seen_in_version_exists = true;
                        newEntry.last_seen_in_version = getDirResp.currentDirVersion;
                        BEGIN_TRANSACTION();
                        rc = localDb.syncHistoryTree_add(newEntry);
                        if (rc < 0) {
                            LOG_CRITICAL("%p: syncHistoryTree_add(%s) failed: %d",
                                    this, currEntryDirPath.c_str(), rc);
                            HANDLE_DB_FAILURE();
                            return false;
                        }
                        CHECK_END_TRANSACTION(false);
                        // Proceed to the next VcsFile.
                        continue;
                    }
                    // Create file/dir record in LocalDB (vcs_comp_id = compId, vcs_revision = -1 since it doesn't exist on the local FS).
                    SCRow_syncHistoryTree newEntry;
                    newEntry.parent_path = currScanDirRelPath.str();
                    newEntry.name = it->name;
                    newEntry.is_dir = false;
                    newEntry.comp_id_exists = true;
                    newEntry.comp_id = it->compId;
                    newEntry.revision_exists = true;
                    newEntry.revision = it->latestRevision.revision;
                    // newEntry.local_mtime is not populated until upload actually occurs.
                    newEntry.last_seen_in_version_exists = true;
                    newEntry.last_seen_in_version = getDirResp.currentDirVersion;
                    BEGIN_TRANSACTION();
                    rc = localDb.syncHistoryTree_add(newEntry);
                    if (rc < 0) {
                        LOG_CRITICAL("%p: syncHistoryTree_add(%s, %s) failed: %d",
                                this, currScanDirRelPath.c_str(), it->name.c_str(), rc);
                        HANDLE_DB_FAILURE();
                        return false;
                    }
                    CHECK_END_TRANSACTION(false);
                    // Our localDB doesn't match VCS; there will be something to sync.
                    SetStatus(SYNC_CONFIG_STATUS_SYNCING);
                } else if (rc < 0) {
                    LOG_CRITICAL("%p: syncHistoryTree_get(%s, %s) failed: %d",
                              this, currScanDirRelPath.c_str(), it->name.c_str(), rc);
                    HANDLE_DB_FAILURE();
                    return false;
                } else { // dirEntry present in LocalDB; currDbEntry is valid.
                    // Update LocalDB[dirEntry].last_seen_in_version = currentDirVersion
                    BEGIN_TRANSACTION();
                    rc = localDb.syncHistoryTree_updateLastSeenInVersion(currScanDirRelPath.str(), it->name,
                            true, getDirResp.currentDirVersion);
                    if (rc < 0) {
                        LOG_CRITICAL("%p: syncHistoryTree_updateLastSeenInVersion(%s, %s) failed: %d",
                                  this, currScanDirRelPath.c_str(), it->name.c_str(), rc);
                        HANDLE_DB_FAILURE();
                        return false;
                    }
                    CHECK_END_TRANSACTION(false);
                    // If LocalDB[dirEntry].vcs_comp_id != compId || LocalDB[dirEntry].vcs_revision != revision,
                    if ((currDbEntry.comp_id != it->compId) || (currDbEntry.revision != it->latestRevision.revision)) {
                        SCRow_syncHistoryTree newEntry;
                        newEntry.parent_path = currScanDirRelPath.str();
                        newEntry.name = it->name;
                        newEntry.is_dir = false;
                        newEntry.comp_id_exists = true;
                        newEntry.comp_id = it->compId;
                        newEntry.revision_exists = true;
                        newEntry.revision = it->latestRevision.revision;
                        // newEntry.local_mtime is not populated until upload actually occurs.
                        newEntry.last_seen_in_version_exists = true;
                        newEntry.last_seen_in_version = getDirResp.currentDirVersion;
                        BEGIN_TRANSACTION();
                        rc = localDb.syncHistoryTree_add(newEntry);
                        if (rc < 0) {
                            LOG_CRITICAL("%p: syncHistoryTree_add(%s, %s) failed: %d",
                                      this, currScanDirRelPath.c_str(), it->name.c_str(), rc);
                            HANDLE_DB_FAILURE();
                            return false;
                        }
                        CHECK_END_TRANSACTION(false);
                    }
                }
            } // For each folder dirEntry in fileList.
            for (vector<VcsFolder>::iterator it = getDirResp.dirs.begin();
                 it != getDirResp.dirs.end();
                 ++it)
            {
                SCRelPath currEntryDirPath = currScanDirRelPath.getChild(it->name);
                // Look it up in localDB.
                SCRow_syncHistoryTree currDbEntry;
                rc = localDb.syncHistoryTree_get(currScanDirRelPath.str(), it->name, currDbEntry);
                // If dirEntry doesn't exist in LocalDB:
                if (rc == SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) { // dirEntry not present in LocalDB.
                    // Create dir record in LocalDB (vcs_comp_id = compId).
                    SCRow_syncHistoryTree newEntry;
                    newEntry.parent_path = currScanDirRelPath.str();
                    newEntry.name = it->name;
                    newEntry.is_dir = true;
                    newEntry.comp_id_exists = true;
                    newEntry.comp_id = it->compId;
                    newEntry.last_seen_in_version_exists = true;
                    newEntry.last_seen_in_version = getDirResp.currentDirVersion;
                    BEGIN_TRANSACTION();
                    rc = localDb.syncHistoryTree_add(newEntry);
                    if (rc < 0) {
                        LOG_CRITICAL("%p: syncHistoryTree_add(%s, %s) failed: %d",
                                this, currScanDirRelPath.c_str(), it->name.c_str(), rc);
                        HANDLE_DB_FAILURE();
                        return false;
                    }
                    CHECK_END_TRANSACTION(false);
                    SetStatus(SYNC_CONFIG_STATUS_SYNCING);

                } else if (rc < 0) {
                    LOG_CRITICAL("%p: syncHistoryTree_get(%s, %s) failed: %d",
                            this, currScanDirRelPath.c_str(), it->name.c_str(), rc);
                    HANDLE_DB_FAILURE();
                    return false;
                } else { // dirEntry present in LocalDB; currDbEntry is valid.
                    // Just update LocalDB[dirEntry].last_seen_in_version = currentDirVersion
                    BEGIN_TRANSACTION();
                    rc = localDb.syncHistoryTree_updateLastSeenInVersion(currScanDirRelPath.str(), it->name,
                            true, getDirResp.currentDirVersion);
                    if (rc < 0) {
                        LOG_CRITICAL("%p: syncHistoryTree_updateLastSeenInVersion(%s, %s) failed: %d",
                                this, currScanDirRelPath.c_str(), it->name.c_str(), rc);
                        HANDLE_DB_FAILURE();
                        return false;
                    }
                    CHECK_END_TRANSACTION(false);
                    // If LocalDB[dirEntry].vcs_comp_id != compId:
                    if (currDbEntry.comp_id != it->compId) {
                        // Update the localDB:
                        BEGIN_TRANSACTION();
                        rc = localDb.syncHistoryTree_updateCompId(currScanDirRelPath.str(), it->name,
                                true, it->compId, false, 0);
                        if (rc < 0) {
                            LOG_CRITICAL("%p: syncHistoryTree_updateCompId(%s) failed: %d",
                                     this, currScanDirRelPath.c_str(), rc);
                            HANDLE_DB_FAILURE();
                            return false;
                        }
                        CHECK_END_TRANSACTION(false);
                    }

                    // TODO: Bug 11475: Ideally, we shouldn't discard this information.
#if 0
                    // LocalDB[dirEntry].version_scanned < currentDirVersion),
                    if (currDbEntry.version_scanned < it->version) {
                        // Directory has been updated since we last scanned it, add it to the queue:
                        LOG_DEBUG("%p: Previous version_scanned="FMTu64", now="FMTu64"; adding %s to needDownloadScan",
                                this, currDbEntry.version_scanned, it->version, currEntryDirPath.c_str());
                    }
#endif
                }
            } // For each directory dirEntry in fileList.

            // Get ready to process the next page of vcs_read_folder results.
            u64 entriesInCurrRequest = getDirResp.files.size() + getDirResp.dirs.size();
            filesSeen += entriesInCurrRequest;
            if(entriesInCurrRequest == 0 && filesSeen < getDirResp.numOfFiles) {
                LOG_ERROR("%p: No entries in getDir(%s) response:"FMTu64"/"FMTu64,
                          this, currScanDirRelPath.c_str(),
                          filesSeen, getDirResp.numOfFiles);
                break;
            }
        } while (filesSeen < getDirResp.numOfFiles);

        // Check if currentDirVersion stayed the same for all pages,
        if (getDirResp.currentDirVersion != startingDirVersion) {
            // Note: OneWayUp Sync is not designed to recover from multiple producers!
            LOG_ERROR("Directory version changed during scan ("FMTu64"->"FMTu64") for %s",
                    startingDirVersion, getDirResp.currentDirVersion, currScanDirDatasetRelPath.c_str());
        } else {
            // Perform deletion phase.
            {
                //  For each dirEntry where LocalDB.rel_path == currDir and LocalDB.last_seen_in_version < currentDirVersion.
                //      Traverse the local metadata from the to-be-deleted-node depth first
                //      For each entryToDelete in traversal:
                //          Remove entryToDelete from LocalDB.
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
                    if (child.is_dir) {
                        recursiveRemoveDirFromLocalDb(child);
                    } else {
                        removeFileFromLocalDb(child);
                    }
                }
            }
        }

        return true;
    }

    // Logic is based on TwoSyncConfigTwoWayImpl::deleteLocalFile.
    void removeFileFromLocalDb(const SCRow_syncHistoryTree& toDelete)
    {
        ASSERT(!toDelete.is_dir);
        int rc = localDb.syncHistoryTree_remove(toDelete.parent_path, toDelete.name);
        if(rc != 0) {
            LOG_CRITICAL("%p: syncHistoryTree_remove(%s,%s) failed: %d",
                      this,
                      toDelete.parent_path.c_str(),
                      toDelete.name.c_str(),
                      rc);
            HANDLE_DB_FAILURE();
        }
    }

    // Logic is based on TwoSyncConfigTwoWayImpl::deleteLocalDir.
    void recursiveRemoveDirFromLocalDb(const SCRow_syncHistoryTree& directory)
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
                        removeFileFromLocalDb(currDirEntry);
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
            rc = localDb.syncHistoryTree_remove(dirToDelete.parent_path,
                                                dirToDelete.name);
            if(rc != 0) {
                LOG_CRITICAL("%p: syncHistoryTree_remove(%s,%s) failed: %d",
                        this,
                        dirToDelete.parent_path.c_str(),
                        dirToDelete.name.c_str(),
                        rc);
                HANDLE_DB_FAILURE();
            }
        }  // while(!traversedDirsStack.empty())
    }

    /// Get the SyncConfig root componentId from the LocalDB syncHistoryTree (or if the
    /// componentId is not yet in the LocalDB, we will attempt to retrieve it from VCS and
    /// store it in the LocalDB syncHistoryTree for future use).
    /// Caller must handle expected error cases: #VCS_ERR_COMPONENT_NOT_FOUND
    /// and #VCS_ERR_PATH_DOESNT_POINT_TO_KNOWN_COMPONENT.
    // TODO: bug 11587: Is VCS_ERR_COMPONENT_NOT_FOUND even possible?
    int GetRootCompId()
    {
        int rc;

        SCRelPath scrRoot = SCRelPath("");
        SCRow_syncHistoryTree entry_out;
        rc = localDb.syncHistoryTree_get("", scrRoot.str(), entry_out);
        if (rc == 0 && entry_out.comp_id_exists) {
            // We already have it; done.
        } else {
            LOG_INFO("%p: localDb get root failed:%d, may be first traverse, getting compId",
                     this, rc);
            u64 compId;
            DatasetRelPath drpRoot = getDatasetRelPath(scrRoot);
            for (u32 retries=0; retries < NUM_IMMEDIATE_TRANSIENT_RETRY; ++retries)
            {
                VPLHttp2 httpHandle;
                if (setHttpHandleAndCheckForStop(httpHandle)) { return SYNC_AGENT_ERR_STOPPING; }
                rc = vcs_get_comp_id(vcs_session,
                                     httpHandle,
                                     dataset,
                                     drpRoot.str(),
                                     verbose_http_log,
                                     compId);
                if (clearHttpHandleAndCheckForPauseStop(httpHandle)) { return SYNC_AGENT_ERR_STOPPING; }
                if (!isTransientError(rc)) { break; }
                LOG_WARN("%p: Transient Error:%d RetryIndex:%d", this, rc, retries);
                if (retries+1 < NUM_IMMEDIATE_TRANSIENT_RETRY) {  // pause between immediate retries
                    if (checkForPauseStop(true, QUICK_RETRY_INTERVAL)) {
                        return SYNC_AGENT_ERR_STOPPING;
                    }
                }
            }
            if (rc != 0) {
                // Caller will handle the error.
                return rc;
            }

            rc = setRootCompId(compId);
            if (rc != 0) {
                LOG_ERROR("%p: Setting root compId failed:%d", this, rc);
                // HANDLE_DB_FAILURE already called inside setRootCompId().
                return rc;
            }
        }

        return 0;
    }

    /// Populates the UploadChangeLog by detecting adds/updates/deletes from the local filesystem.
    void PerformFullLocalScan(bool initialScan)
    {
        int fsOpCount = 0; // We will check for pause/stop every FILESYS_OPERATIONS_PER_STOP_CHECK operations.
        int rc = localDb.uploadChangeLog_clear();
        if (rc < 0) {
            LOG_CRITICAL("%p: uploadChangeLog_clear() failed: %d", this, rc);
            HANDLE_DB_FAILURE();
            return;
        }

        if (initialScan) {
            LOG_INFO("%p: Server scan requested", this);
        }
        deque<SCRelPath> relDirQ;
        // Add SyncConfig root to DirectoryQ
        relDirQ.push_back(SCRelPath(""));
        while (!relDirQ.empty()) {
            BEGIN_TRANSACTION();

            if (fsOpCount++ >= FILESYS_OPERATIONS_PER_STOP_CHECK) {
                if (checkForPauseStop()) {
                    goto end_function;
                }
                fsOpCount = 0;
            }
            SCRelPath currRelDir = relDirQ.front();
            relDirQ.pop_front();
            AbsPath currAbsLocalDir = getAbsPath(currRelDir);
            LOG_DEBUG("%p: Scanning "FMTu64":%s (%s). InitialScan:%d",
                      this, dataset.id, currRelDir.c_str(), currAbsLocalDir.c_str(), initialScan);
            set<string> pathExists; // There should be no "/" characters in here.
            if (initialScan) {
                rc = 0;
                if(currRelDir.str()=="") {
                    rc = GetRootCompId();
                    // TODO: bug 11587: Is VCS_ERR_COMPONENT_NOT_FOUND even possible?
                    if ((rc == VCS_ERR_COMPONENT_NOT_FOUND) ||
                        (rc == VCS_ERR_PATH_DOESNT_POINT_TO_KNOWN_COMPONENT))
                    {
                        // This is acceptable.  We will just create it when we upload our first
                        // file or create our first directory.
                        LOG_INFO("%p: No root comp id: %d. Initial scan unneeded.", this, rc);
                        initialScan = false;
                    } else if (rc != 0) {
                        LOG_WARN("%p: GetRootCompId (vcs_get_comp_id) failed: %d. Must retry later.", this, rc);
                        setErrorNeedsRemoteScan();
                        // TODO: Bug 11314: remove this comment during code review:
                        // Note: updateStatusDueToScanRequest() should have already been set at the beginning of the scan.
                        goto end_function;
                    }
                }
            }
            if (initialScan) {
                SCRow_syncHistoryTree currDbEntry;
                SCRow_needDownloadScan currDlScanDir;
                SCRelPath currRelParentPath = currRelDir.getParent();
                std::string currRelName = currRelDir.getName();

                rc = localDb.syncHistoryTree_get(currRelParentPath.str(),
                                                 currRelName,
                                                 currDbEntry);
                if (rc == SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) {
                    // If currRelDir existed on VCS when we scanned the parent directory,
                    // currRelDir should already be in the syncHistoryTree at this point.
                    LOG_INFO("%p: New local directory found:%s", this, currRelDir.c_str());
                } else if (rc != 0) {
                    LOG_CRITICAL("%p: syncHistoryTree_get(%s,%s):%d",
                              this, currRelParentPath.c_str(), currRelName.c_str(), rc);
                    HANDLE_DB_FAILURE();
                    return;
                } else {
                    if (!currDbEntry.comp_id_exists) {
                        LOG_ERROR("%p: comp_id_exists for scan(%s,%s)",
                                  this, currRelParentPath.c_str(), currRelName.c_str());
                        FAILED_ASSERT("CompId should've been previously found");
                    } else {
                        if(currDbEntry.is_dir) {
                            currDlScanDir.dir_path = currRelDir.str();
                            currDlScanDir.comp_id = currDbEntry.comp_id;
                            // Whether initialServerScan succeeds or not, we should do the local scan
                            // for currDlScanDir.
                            // This allows us to eventually reach steady-state, even if the user
                            // has an unreliable network connection.
                            initialServerScan(currDlScanDir);
                        }
                    }
                }
            }

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
                    // TODO: Bug 11588: Instead of dropping, we could set uploadScanError and setErrorTimeout (to try
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
                if (!isValidDirEntry(dirEntry.filename)) {
                    continue;
                }
                // Only increase the count, but don't call clearHttpHandleAndCheckForPauseStop,
                // since we still have dirStream open.
                fsOpCount++;

                // Add to "pathExists" set.
                pathExists.insert(dirEntry.filename);

                SCRelPath currEntryRelPath = currRelDir.getChild(dirEntry.filename);
                bool currEntryIsDir = (dirEntry.type == VPLFS_TYPE_DIR);
                if (currEntryIsDir) {
                    relDirQ.push_back(currEntryRelPath);
                }
                SCRow_syncHistoryTree currDbEntry;
                rc = localDb.syncHistoryTree_get(currRelDir.str(), dirEntry.filename, currDbEntry);
                if (rc == SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) { // dirEntry not present in LocalDB.
                    LOG_INFO("%p: Decision UploadCreate (%s,%s), isDir(%d)",
                             this, currRelDir.c_str(), dirEntry.filename, currEntryIsDir);
                    BEGIN_TRANSACTION();
                    localDb_uploadChangeLog_add_and_setSyncStatus(
                            currRelDir.str(), dirEntry.filename, currEntryIsDir,
                            UPLOAD_ACTION_CREATE,
                            false, 0, false, 0, std::string(""), false, 0);
                    CHECK_END_TRANSACTION(false);
                } else if (rc < 0) {
                    LOG_CRITICAL("%p: syncHistoryTree_get(%s, %s) failed: %d",
                              this, currRelDir.c_str(), dirEntry.filename, rc);
                    HANDLE_DB_FAILURE();
                    return;
                } else { // dirEntry present in LocalDB; currDbEntry is valid.
                    // Compare the previous record of our local FS with what actually exists now.
                    if (currEntryIsDir != currDbEntry.is_dir) {
                        // Local FS entry was changed from file to dir (or vice-versa).
                        LOG_INFO("%p: Decision UploadDelete/UploadCreate file changed to dir (%s,%s),%d,%d",
                                 this, currRelDir.c_str(), dirEntry.filename,
                                 currDbEntry.is_dir, currDbEntry.comp_id_exists);
                        BEGIN_TRANSACTION();
                        if (currDbEntry.is_dir) {
                            localDb_uploadChangeLog_add_and_setSyncStatus(
                                    currRelDir.str(), dirEntry.filename, currDbEntry.is_dir,
                                    UPLOAD_ACTION_DELETE,
                                    currDbEntry.comp_id_exists, currDbEntry.comp_id,
                                    false, 0, std::string(""), false, 0);
                        } else {
                            // TODO: Bug 13548, Still need to handle the case where a file is known but 
                            // the revision not yet populated (not downloaded).
                            ASSERT(currDbEntry.revision_exists);
                            localDb_uploadChangeLog_add_and_setSyncStatus(
                                    currRelDir.str(), dirEntry.filename, currDbEntry.is_dir,
                                    UPLOAD_ACTION_DELETE,
                                    currDbEntry.comp_id_exists, currDbEntry.comp_id,
                                    currDbEntry.revision_exists, currDbEntry.revision,
                                    std::string(""), false, 0);
                        }
                        localDb_uploadChangeLog_add_and_setSyncStatus(
                                currRelDir.str(), dirEntry.filename, currEntryIsDir,
                                UPLOAD_ACTION_CREATE, false, 0, false, 0, std::string(""), false, 0);
                        CHECK_END_TRANSACTION(false);
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
                                // TODO: Bug 11588: Instead of dropping, we could set uploadScanError and setErrorTimeout (to try
                                //   again automatically), but that seems more dangerous since the directory may
                                //   never become readable, and we'd keep doing a scan every 15 minutes.
                                //   If we had some way to track the specific directories
                                //   causing problems and backoff the retry time, it might be better.
                            }
                            continue;
                        }
                        VPLTime_t localModifiedTime = fsTimeToNormLocalTime(
                                currEntryAbsPath, statBuf.vpl_mtime);
                        if (localModifiedTime != currDbEntry.local_mtime) {
                            LOG_INFO("%p: Decision UploadUpdate (%s,%s), isDir(%d), mtime("FMTu64","FMTu64")",
                                     this, currRelDir.c_str(), dirEntry.filename, currEntryIsDir,
                                     currDbEntry.local_mtime, localModifiedTime);
                            BEGIN_TRANSACTION();
                            localDb_uploadChangeLog_add_and_setSyncStatus(
                                    currRelDir.str(), dirEntry.filename, currEntryIsDir,
                                    UPLOAD_ACTION_UPDATE,
                                    false, 0, false, 0, std::string(""), false, 0);
                            CHECK_END_TRANSACTION(false);
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
                            LOG_INFO("%p: Decision UploadUpdate (%s,%s), isDir(%d)",
                                     this, currRelDir.c_str(), dirEntry.filename, currEntryIsDir);
                            BEGIN_TRANSACTION();
                            localDb_uploadChangeLog_add_and_setSyncStatus(
                                    currRelDir.str(), dirEntry.filename, currEntryIsDir,
                                    UPLOAD_ACTION_UPDATE, false, 0, false, 0,
                                    std::string(""), false, 0);
                            CHECK_END_TRANSACTION(false);
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
            for (std::vector<SCRow_syncHistoryTree>::iterator it = previousEntries.begin();
                 it != previousEntries.end();
                 ++it)
            {
                SCRow_syncHistoryTree& currToDelete = *it;
                if (isRoot(currToDelete)) { continue; } // Skip root exception case

                if (contains(pathExists, currToDelete.name)) {
                    LOG_DEBUG("%p: %s still exists.", this, currToDelete.name.c_str());
                } else {
                    LOG_DEBUG("%p: %s no longer exists.", this, currToDelete.name.c_str());
                    BEGIN_TRANSACTION();
                    if (currToDelete.is_dir) {
                        addDirDeleteToUploadChangeLog(currToDelete);
                    } else {
                        addFileDeleteToUploadChangeLog(currToDelete);
                    }
                    CHECK_END_TRANSACTION(false);
                }
            }
        } // while (!relDirQ.empty())
 end_function:
        CHECK_END_TRANSACTION(true);
        return;
    }

    void addDirDeleteToUploadChangeLog(const SCRow_syncHistoryTree& dirToDelete)
    {
        LOG_INFO("%p: Decision UploadDelete dir (%s,%s),compId(%d,"FMTu64")",
                 this,
                 dirToDelete.parent_path.c_str(),
                 dirToDelete.name.c_str(),
                 dirToDelete.comp_id_exists,
                 dirToDelete.comp_id);
        localDb_uploadChangeLog_add_and_setSyncStatus(
                dirToDelete.parent_path,
                dirToDelete.name,
                dirToDelete.is_dir,
                UPLOAD_ACTION_DELETE,
                dirToDelete.comp_id_exists,
                dirToDelete.comp_id,
                false, 0, std::string(""), false, 0);
    }

    int recursiveDirDeleteFromDb(const SCRow_syncHistoryTree& directory)
    {
        // Recursively add all children files and directories of entry to
        // UploadChangeLog as "remove X" (add them depth-first, to ensure that
        // child files are removed before their parent directory).
        int rc;
        int rv = 0;
        // Need to keep track of the visited directories so that we can delete them last.
        // Each directory should appear after all of its parent directories.
        std::vector<SCRow_syncHistoryTree> traversedDirsStack;
        {
            std::deque<SCRow_syncHistoryTree> dirsToTraverseQ;
            dirsToTraverseQ.push_back(directory);
            while (!dirsToTraverseQ.empty())
            {
                SCRow_syncHistoryTree toTraverse = dirsToTraverseQ.front();
                dirsToTraverseQ.pop_front();

                std::vector<SCRow_syncHistoryTree> dirEntries_out;
                SCRelPath currDirPath = getRelPath(toTraverse);
                rc = localDb.syncHistoryTree_getChildren(currDirPath.str(), dirEntries_out);
                if ((rc != 0) && (rc != SYNC_AGENT_DB_ERR_ROW_NOT_FOUND)) {
                    LOG_CRITICAL("%p: syncHistoryTree_get(%s) failed: %d", this, currDirPath.c_str(), rc);
                    HANDLE_DB_FAILURE();
                    return rc;
                }
                for (std::vector<SCRow_syncHistoryTree>::iterator it = dirEntries_out.begin();
                     it != dirEntries_out.end();
                     ++it)
                {
                    SCRow_syncHistoryTree& currDirEntry = *it;
                    if (isRoot(currDirEntry)) { continue; } // Skip root exception case

                    if (currDirEntry.is_dir) {
                        dirsToTraverseQ.push_back(currDirEntry);
                    } else {
                        // No need to delete files in depth first order.  Just delete now.
                        rc = localDb.syncHistoryTree_remove(currDirEntry.parent_path,
                                                            currDirEntry.name);
                        if (rc != 0) {
                            LOG_CRITICAL("%p: syncHistoryTree_remove:(%s,%s)",
                                      this,
                                      currDirEntry.parent_path.c_str(),
                                      currDirEntry.name.c_str());
                            if (rv != 0) { rv = rc; }
                        }
                    }
                }
                traversedDirsStack.push_back(toTraverse);
            }  // while (!dirsToTraverseQ.empty())
        }

        // Delete the directories;
        while (!traversedDirsStack.empty())
        {
            // Pull from the back since we want to delete the deeper directories first.
            SCRow_syncHistoryTree dirToDelete = traversedDirsStack.back();
            traversedDirsStack.pop_back();
            ASSERT(dirToDelete.is_dir);

            rc = localDb.syncHistoryTree_remove(dirToDelete.parent_path,
                                                dirToDelete.name);
            if (rc != 0) {
                LOG_CRITICAL("%p: syncHistoryTree_remove:(%s,%s)",
                          this,
                          dirToDelete.parent_path.c_str(),
                          dirToDelete.name.c_str());
                if (rv != 0) { rv = rc; }
            }
        }  // while (!traversedDirsStack.empty())

        return rv;
    }

    int callVcsDeleteDirRecursive(const VcsFolder& folder)
    {
        int rc;
        LOG_ALWAYS("%p: ACTION UP   delete dir (recursive) begin:%s",
                this, folder.name.c_str());
        for (u32 retries=0; ; ++retries)
        {
            if (((retries+1) % NUM_IMMEDIATE_TRANSIENT_RETRY) == 0) {
                LOG_WARN("%p: Transient errors during folder delete.  Pausing for "FMTu64,
                        this, sync_policy.error_retry_interval);
                MutexAutoLock lock(&mutex);
                setErrorTimeout(sync_policy.error_retry_interval);
                if (checkForPauseStop(true, sync_policy.error_retry_interval)) {
                    return SYNC_AGENT_ERR_STOPPING;
                }
                LOG_INFO("%p: Time to retry transient error.", this);
            }
            VPLHttp2 httpHandle;
            if (setHttpHandleAndCheckForStop(httpHandle)) { return SYNC_AGENT_ERR_STOPPING; }
            rc = vcs_delete_dir(vcs_session,
                                httpHandle,
                                dataset,
                                folder.name,
                                folder.compId,
                                true,     // Recursive
                                false, 0,  // No dataset version
                                verbose_http_log);
            if (clearHttpHandleAndCheckForPauseStop(httpHandle)) { return SYNC_AGENT_ERR_STOPPING; }
            if (!isTransientError(rc)) { break; }
            LOG_WARN("%p: Transient Error:%d RetryIndex:%d", this, rc, retries);
            // Pause between immediate retries.
            if(checkForPauseStop(true, QUICK_RETRY_INTERVAL)) {
                return SYNC_AGENT_ERR_STOPPING;
            }
        }
        ASSERT(!isTransientError(rc)); // This is the only condition that should exit the loop.
        if (rc != 0) {
            // This is a non-retryable error.  We should scan VCS again later.
            setErrorNeedsRemoteScan();
            LOG_ERROR("%p: vcs_delete_dir failed:%s, %d", this, folder.name.c_str(), rc);
        } else {
            LOG_ALWAYS("%p: ACTION UP   delete dir (recursive) done:%s",
                       this, folder.name.c_str());
        }
        return rc;
    }

    int vcsRmFolderRecursive(const SCRelPath& folderArg,
                             u64 compId)
    {
        int rv = 0;
        int rc = 0;

        VcsFolder rootFolder;
        rootFolder.name = getDatasetRelPath(folderArg).str();
        rootFolder.compId = compId;
        rc = callVcsDeleteDirRecursive(rootFolder);
        if (rc < 0) {
            if (rc == SYNC_AGENT_ERR_STOPPING) { return rc; }
            if (rv == 0) { rv = rc; }
        }

        if (rv == 0) {  // Remove from database if removal from infra succeeds.
            SCRow_syncHistoryTree directoryToRemove;
            directoryToRemove.parent_path = folderArg.getParent().str();
            directoryToRemove.name = folderArg.getName();
            directoryToRemove.is_dir = true;

            rv = recursiveDirDeleteFromDb(directoryToRemove);
            if (rv != 0) {
                LOG_ERROR("%p: vcsRmFolderRecursive:(%s,%s)",
                          this,
                          directoryToRemove.parent_path.c_str(),
                          directoryToRemove.name.c_str());
            }
        }

        return rv;
    }

    int getNextUploadChangeLog(bool isErrorMode,
                               u64 afterRowId,
                               u64 errorModeMaxRowId,
                               SyncConfigDb_RowFilter rowFilter,
                               SCRow_uploadChangeLog& entry_out)
    {
        int rc = 0;
        if (isErrorMode) {
            rc = localDb.uploadChangeLog_getErrAfterRowId(afterRowId,
                                                          errorModeMaxRowId,
                                                          rowFilter,
                                                          /*OUT*/entry_out);
        } else {
            rc = localDb.uploadChangeLog_getAfterRowId(afterRowId,
                                                       rowFilter,
                                                       /*OUT*/entry_out);
        }
        return rc;
    }

    bool hasNextConcurrentTaskAfterRowId(bool isErrorMode,
                                         u64 afterRowId,
                                         u64 errorModeMaxRowId) //Only relevant when isErrorMode = true
    {
        SCRow_uploadChangeLog changeLog;
        int rc;
        rc = getNextUploadChangeLog(isErrorMode,
                                    afterRowId,
                                    errorModeMaxRowId,
                                    SCDB_ROW_FILTER_ALLOW_CREATE_AND_UPDATE,
                                    /*out*/ changeLog);
        if(rc != 0) {
            return false;
        }
        if(changeLog.upload_action == UPLOAD_ACTION_CREATE ||
           changeLog.upload_action == UPLOAD_ACTION_UPDATE)
        {
            return true;
        }
        return false;
    }

    struct SyncUpPostFileMetadataEntry
    {
        SCRow_uploadChangeLog currUploadEntry;
        u64 parentCompId;
        bool currUploadEntryHasCompId;
        u64 currUploadEntryCompId;
        u64 currUploadEntryRevisionToUpload;
        VPLTime_t fileModifyTime;
        VPLTime_t fileCreateTime;
        u64 fileSize;
        bool virtualUpload;
        std::string accessUrl;

        SyncUpPostFileMetadataEntry()
        :   parentCompId(0),
            currUploadEntryHasCompId(false),
            currUploadEntryCompId(0),
            currUploadEntryRevisionToUpload(0),
            fileModifyTime(VPLTIME_INVALID),
            fileCreateTime(VPLTIME_INVALID),
            fileSize(0),
            virtualUpload(false)
        {}
    };

    struct UploadFileResult {
        SyncUpPostFileMetadataEntry postFileMetadataEntry; // Identified by currUploadEntry.rowId (unchanged from UploadFileTask)

        bool skip;          // True when the task should be dropped.  It will be abandoned completely, although it may be added again later by a subsequent scan.
        bool skip_ErrInc;   // True when the task should be retried.

        /// Set this in a thread pool worker thread to tell the SyncConfig's primary worker thread
        /// that it should stop processing the changelog.  (The primary worker thread should still
        /// wait for any other tasks that it has already dispatched to finish though.)
        bool endFunction;
        bool clearUploadChangeLog;

        UploadFileResult()
        :  skip(false),
           skip_ErrInc(false),
           endFunction(false),
           clearUploadChangeLog(false)
        {}
    };

    struct UploadFileTask {
        SCRow_uploadChangeLog currUploadEntry;  // Identified by rowId
        SyncConfigOneWayUpImpl* thisPtr;

        u64 parentCompId;
        VPLTime_t fileModifyTime;
        VPLTime_t fileCreateTime;
        VPLFS_file_size_t fileSize;

        bool currUploadEntryHasCompId;
        u64 currUploadEntryCompId;
        u64 currUploadEntryRevisionToUpload;
        u64 logOnlyCurrUploadMtime;

        VPLMutex_t* mutex;  // Used to protect enqueuedUlTasks and enqueuedUlResults
        std::vector<UploadFileTask*>* enqueuedUlTasks;
        std::vector<UploadFileResult*>* enqueuedUlResults;

        UploadFileTask()
        :  thisPtr(NULL),
           parentCompId(0),
           fileModifyTime(0),
           fileCreateTime(0),
           fileSize(0),
           currUploadEntryHasCompId(false),
           currUploadEntryCompId(0),
           currUploadEntryRevisionToUpload(0),
           logOnlyCurrUploadMtime(0),
           mutex(NULL),
           enqueuedUlTasks(NULL),
           enqueuedUlResults(NULL)
        {}
    };

    virtual int LookupComponentByPath(const std::string& sync_config_relative_path,
                                      u64& component_id__out,
                                      u64& revision__out,
                                      bool& is_on_acs__out)
    {
        return CCD_ERROR_NOT_IMPLEMENTED;
    }

    static void getAccessInfoAndUpload(SyncConfigOneWayUpImpl* thisPtr,
                                       const AbsPath& currEntryAbsPath,
                                       u64 fileModifyTime,
                                       u64 fileSize,
                                       void* taskPtr_infoOnly,
                                       VcsAccessInfo& accessInfoResp_out,
                                       bool& errorInc_out,
                                       bool& skip_out,
                                       bool& endFunction_out)
    {
        int rc;
        accessInfoResp_out.clear();
        errorInc_out = false;
        skip_out = false;
        endFunction_out = false;

        // Since a lot of time may have passed since we scanned this file, make sure
        // that it still exists before calling VCS GET accessinfo.
        {
            VPLFS_stat_t statBuf;
            rc = VPLFS_Stat(currEntryAbsPath.c_str(), &statBuf);
            if (rc != 0) {
                // Dropping the file until we do another local scan.
                if (rc == VPL_ERR_NOENT) {
                    // The file was probably deleted.
                    LOG_INFO("%p: task(%p): File (%s) no longer exists",
                             thisPtr, taskPtr_infoOnly, currEntryAbsPath.c_str());
                } else {
                    LOG_WARN("%p: task(%p): VPLFS_Stat(%s) failed: %d",
                             thisPtr, taskPtr_infoOnly, currEntryAbsPath.c_str(), rc);
                    // TODO: Bug 11588: Instead of dropping, we could set uploadScanError and setErrorTimeout (to try
                    //   again automatically), but that seems more dangerous since the directory may
                    //   never become readable, and we'd keep doing a scan every 15 minutes.
                    //   If we had some way to track the specific entities
                    //   causing problems and backoff the retry time, it might be better.
                }
                goto skip;
            }
        }

        // Get the 3rd party storage URL (accessUrl) from VCS GET accessinfo (pass method=PUT).
        for (u32 retries=0; retries < NUM_IMMEDIATE_TRANSIENT_RETRY; ++retries)
        {
            VPLHttp2 httpHandle;
            if (thisPtr->setHttpHandleAndCheckForStop(httpHandle)) { goto end_function; }
            rc = vcs_access_info_for_file_put(thisPtr->vcs_session,
                                              httpHandle,
                                              thisPtr->dataset,
                                              thisPtr->verbose_http_log,
                                              accessInfoResp_out);
            if (thisPtr->clearHttpHandleAndCheckForStopWithinTask(httpHandle)) { goto end_function; }
            if (!isTransientError(rc)) { break; }
            LOG_WARN("%p: task(%p): Transient Error:%d RetryIndex:%d", thisPtr, taskPtr_infoOnly, rc, retries);
            if (retries+1 < NUM_IMMEDIATE_TRANSIENT_RETRY) {  // pause between immediate retries
                if(thisPtr->checkForPauseStopWithinTask(true, QUICK_RETRY_INTERVAL)) {
                    goto end_function;
                }
            }
        }
        if (rc < 0) {
            if (isTransientError(rc)) {
                // Transient error; retry later.
                errorInc_out = true;
                LOG_WARN("%p: task(%p): vcs_access_info_for_file_put failed: %d", thisPtr, taskPtr_infoOnly, rc);
            } else {
                // Non-retryable; need to do another server scan.
                thisPtr->setErrorNeedsRemoteScan();
                LOG_ERROR("%p: task(%p): vcs_access_info_for_file_put failed: %d", thisPtr, taskPtr_infoOnly, rc);
            }
            goto skip;
        }
        // Upload file to 3rd party storage.
        for (u32 retries=0; retries < NUM_IMMEDIATE_TRANSIENT_RETRY; ++retries)
        {
            VPLHttp2 httpHandle;
            if (thisPtr->setHttpHandleAndCheckForStop(httpHandle)) { goto end_function; }
            rc = vcs_s3_putFileHelper(accessInfoResp_out,
                                      httpHandle,
                                      NULL,
                                      NULL,
                                      currEntryAbsPath.str(),
                                      thisPtr->verbose_http_log);
            if (thisPtr->clearHttpHandleAndCheckForStopWithinTask(httpHandle)) { goto end_function; }
            if (!isTransientError(rc)) { break; }
            LOG_WARN("%p: task(%p): Transient Error:%d RetryIndex:%d", thisPtr, taskPtr_infoOnly, rc, retries);
            if (retries+1 < NUM_IMMEDIATE_TRANSIENT_RETRY) {  // pause between immediate retries
                if(thisPtr->checkForPauseStopWithinTask(true, QUICK_RETRY_INTERVAL)) {
                    goto end_function;
                }
            }
        }
        if (rc != 0) {
            LOG_WARN("%p: task(%p): vcs_s3_putFileHelper returned %d", thisPtr, taskPtr_infoOnly, rc);

            if (rc == VPL_ERR_NOENT) {
                // VPL_ERR_NOENT is expected if the file-to-upload was removed from the local filesystem.
                // We can safely drop the upload, since the scanner should be triggered again by the change.
            } else {
                // We consider all errors from ACS to be transient; we need to check with VCS
                // GET accessinfo again later to determine if it is retryable or not.
                errorInc_out = true;
            }
            goto skip;
        }
        {
            // TODO: Compute rolling hash while uploading (required for correctness in case file changes).
            //       If rolling hash does not match previous local file hash, abort (proceed to next entry in ChangeLog).
            //       For now, simply compare last modified time.
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
                    LOG_INFO("%p: task(%p): File (%s) no longer exists",
                             thisPtr, taskPtr_infoOnly, currEntryAbsPath.c_str());
                } else {
                    LOG_WARN("%p: task(%p): VPLFS_Stat(%s) failed: %d",
                             thisPtr, taskPtr_infoOnly, currEntryAbsPath.c_str(), rc);
                    // TODO: Bug 11588: Instead of dropping, we could set uploadScanError and setErrorTimeout (to try
                    //   again automatically), but that seems more dangerous since the directory may
                    //   never become readable, and we'd keep doing a scan every 15 minutes.
                    //   If we had some way to track the specific entities
                    //   causing problems and backoff the retry time, it might be better.
                }
                goto skip;
            }
            modifyTime = fsTimeToNormLocalTime(currEntryAbsPath, statBuf.vpl_mtime);
            if (modifyTime != fileModifyTime ||
               statBuf.size != fileSize)
            {
                LOG_INFO("%p: task(%p): %s file changed during upload. Upload safely abandoned. "
                         "("FMTu64","FMTu64")->("FMTu64","FMTu64")",
                         thisPtr, taskPtr_infoOnly, currEntryAbsPath.c_str(),
                         fileModifyTime, fileSize,
                         modifyTime, (u64)statBuf.size);
                goto skip;
            }
        }
        return;

     end_function:
        endFunction_out = true;
        return;
     skip:
        skip_out = true;
        return;
    }

    static void PerformUploadTask(void* ctx)
    {
        UploadFileTask* task = (UploadFileTask*) ctx;
        UploadFileResult* result = new UploadFileResult();
        SyncConfigOneWayUpImpl* thisPtr = task->thisPtr;
        LOG_DEBUG("%p: task(%p): Created UploadFileResult:%p", thisPtr, task, result);

        AbsPath currEntryAbsPath = thisPtr->getAbsPath(getRelPath(task->currUploadEntry));
        LOG_INFO("%p: task(%p->%p): ACTION UP post_filemetadata begin:%s rowId("FMTu64"),mtimes("FMTu64","FMTu64"),"
                 "ctime("FMTu64"),compId("FMTu64"),revUp("FMTu64")",
                 thisPtr, task, result,
                 currEntryAbsPath.c_str(),
                 task->currUploadEntry.row_id,
                 task->logOnlyCurrUploadMtime,
                 task->fileModifyTime,
                 task->fileCreateTime,
                 task->currUploadEntryHasCompId?task->currUploadEntryCompId:0,
                 task->currUploadEntryRevisionToUpload);
        // Compute local file hash.
        // (optional optimization 1) Call VCS GET filemetadata.  If VCS hash == local file hash,
        //     (optional optimization 1) Update LocalDB with new timestamp, hash, set vcs_comp_id = compId, and set vcs_revision = revision (for case when LocalDB was lost).
        //     (optional optimization 1) Proceed to next entry in ChangeLog.
        // (optional optimization 2) If (ChangeLog.vcs_comp_id != compId) || (ChangeLog.vcs_revision != revision),
        //     (optional optimization 2) Do conflict resolution.
        //     (optional optimization 2) Proceed to next entry in ChangeLog.
        VcsAccessInfo accessInfoResp;
        bool virtualUpload = thisPtr->virtualUpload();
        if (!virtualUpload)
        {
            getAccessInfoAndUpload(thisPtr,
                                   currEntryAbsPath,
                                   task->fileModifyTime,
                                   task->fileSize,
                                   task, // info only
                                   /*OUT*/ accessInfoResp,
                                   /*OUT*/ result->skip_ErrInc,
                                   /*OUT*/ result->skip,
                                   /*OUT*/ result->endFunction);
            if (result->skip_ErrInc || result->skip || result->endFunction) {
                goto done;
            }
        }

        {
            SyncUpPostFileMetadataEntry& pfmReq = result->postFileMetadataEntry;
            pfmReq.currUploadEntry = task->currUploadEntry;
            pfmReq.parentCompId = task->parentCompId;
            pfmReq.currUploadEntryHasCompId = task->currUploadEntryHasCompId;
            pfmReq.currUploadEntryCompId = task->currUploadEntryCompId;
            pfmReq.currUploadEntryRevisionToUpload = task->currUploadEntryRevisionToUpload;
            pfmReq.fileModifyTime = task->fileModifyTime;
            pfmReq.fileCreateTime = task->fileCreateTime;
            pfmReq.fileSize = task->fileSize;
            pfmReq.virtualUpload = virtualUpload;
            pfmReq.accessUrl = accessInfoResp.accessUrl;
        }

     done:
        {
            LOG_INFO("%p: task(%p) --> result(%p), row("FMTu64")",
                     thisPtr, task, result, task->currUploadEntry.row_id);

            // Completed -- move from TaskList to ResultList.
            // Error or not, this must always be done.
            MutexAutoLock lock(task->mutex);
            task->enqueuedUlResults->push_back(result);
            removeEnqueuedUlTask(task->currUploadEntry.row_id,
                               *(task->enqueuedUlTasks));
        }
        return;
    }

    void ApplyUploadDeletions(bool isDirectory,
                              bool isErrorMode)
    {
        int rc;
        SCRow_uploadChangeLog currUploadEntry;
        // afterRowId supports going on to the next request before the last
        // request is complete.  Required for parallel operations.  Only
        // set when a "Task" is spawned.
        u64 afterRowId = 0;
        u64 errorModeMaxRowId = 0;  // Only valid when isErrorMode
        SyncConfigDb_RowFilter rowFilter = SCDB_ROW_FILTER_ALLOW_FILE_DELETE;
        if (isDirectory) {
            rowFilter = SCDB_ROW_FILTER_ALLOW_DIRECTORY_DELETE;
        }

        ASSERT(!VPLMutex_LockedSelf(&mutex));

        if (isErrorMode) {
            rc = localDb.uploadChangeLog_getMaxRowId(errorModeMaxRowId);
            if (rc != 0) {
                LOG_CRITICAL("%p: Should never happen:uploadChangeLog_getMaxRowId,%d", this, rc);
                HANDLE_DB_FAILURE();
                return;
            }
        }
        // For each entry in UploadChangeLog:
        int nextChangeRc;
        while ((nextChangeRc = getNextUploadChangeLog(isErrorMode,
                                                      afterRowId,
                                                      errorModeMaxRowId,
                                                      rowFilter,
                                                      /*out*/ currUploadEntry)) == 0)
        {
            bool skip_ErrInc = false;  // When true, appends to error.
            DatasetRelPath currEntryDatasetRelPath = getDatasetRelPath(currUploadEntry);

            // If changelog entry is dir; call VCS DELETE dir (compId).
            if (currUploadEntry.is_dir) {
                ASSERT(currUploadEntry.comp_id_exists);
                LOG_INFO("%p: ACTION UP recursive delete begin (dir):%s", this, currEntryDatasetRelPath.c_str());
                SCRelPath currEntryRelPath = getRelPath(currUploadEntry);
                BEGIN_TRANSACTION();
                // Removes the directory (and everything beneath it) from both VCS and
                // the LocalDB syncHistoryTree.
                rc = vcsRmFolderRecursive(currEntryRelPath, currUploadEntry.comp_id);
                CHECK_END_TRANSACTION_BYTES(0, false);
                if (checkForPauseStopWithinTask(false, 0)) {
                    goto end_function;
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
                    if (setHttpHandleAndCheckForStop(httpHandle)) { goto end_function; }
                    rc = vcs_delete_file(vcs_session,
                                         httpHandle,
                                         dataset,
                                         currEntryDatasetRelPath.str(),
                                         currUploadEntry.comp_id,
                                         currUploadEntry.revision,
                                         verbose_http_log);
                    if (clearHttpHandleAndCheckForStopWithinTask(httpHandle)) { goto end_function; }
                    if (!isTransientError(rc)) { break; }
                    LOG_WARN("%p: Transient Error:%d RetryIndex:%d", this, rc, retries);
                    if (retries+1 < NUM_IMMEDIATE_TRANSIENT_RETRY) {  // pause between immediate retries
                        if(checkForPauseStopWithinTask(true, QUICK_RETRY_INTERVAL)) {
                            goto end_function;
                        }
                    }
                }
                if (rc == 0) {
                    // Remove entry from ChangeLog and remove entry from LocalDB.
                    BEGIN_TRANSACTION();
                    rc = localDb.syncHistoryTree_remove(currUploadEntry.parent_path,
                                                        currUploadEntry.name);
                    if(rc != 0) {
                        LOG_CRITICAL("%p: syncHistoryTree_remove(%s,%s), %d",
                                  this,
                                  currUploadEntry.parent_path.c_str(),
                                  currUploadEntry.name.c_str(), rc);
                        HANDLE_DB_FAILURE();
                        goto end_function;
                    }
                    CHECK_END_TRANSACTION_BYTES(0, false);
                }
            }
            // If a non-retryable error occurred within vcsRmFolderRecursive, it should have
            // already called setErrorNeedsRemoteScan().
            //
            // Expected errors from DELETE file:
            // 5316 - VCS_PROVIDED_PATH_DOESNT_MATCH_PROVIDED_COMPID: The <path> in the requestURI and the <compId> in the request do not match
            // - This case is ambiguous.  It means either:
            //   1. We asked to delete a file that doesn't exist (which is what we want anyway), or
            //   2. We asked to delete a file that currently exists, but we passed the wrong compId;
            //      (we must rescan VCS so that we can actually remove the file).
            // 5325 - VCS_REVISION_NOT_FOUND: The <revision> in the request parameter is not found
            // - Implies that another client modified the file;
            //   (we must rescan VCS so that we can actually remove the file).
            if (rc == 0)
            { // It succeeded.
                // Note: difference from Two Way Sync:
                //   The component should already be removed from the LocalDB at this point.
                LOG_INFO("%p: ACTION UP delete done:%s", this, currEntryDatasetRelPath.c_str());
            } else if (isTransientError(rc)) {
                // Transient error; retry this delete action later.
                LOG_WARN("%p: Transient err during UPLOAD_ACTION_DELETE:%d, %s",
                        this, rc, currEntryDatasetRelPath.c_str());
                skip_ErrInc = true;
                goto skip;
            } else {
                // Non-retryable; need to do another server scan.
                setErrorNeedsRemoteScan();
                LOG_ERROR("%p: Master state out of sync with infra, component"
                          " not deleted: compId("FMTu64"),err:%s",
                          this, currUploadEntry.comp_id, currEntryDatasetRelPath.c_str());
                goto skip;
            }
         skip:
            {
                bool handleResultEndFunction = false;
                handleChangeLogResult(skip_ErrInc,
                                      currUploadEntry,
                                      /*OUT*/    handleResultEndFunction);
                if(handleResultEndFunction) {
                    LOG_ERROR("%p: Handle result end function", this);
                    goto end_function;
                }
            }
        }
     end_function:
        CHECK_END_TRANSACTION_BYTES(0, true);
        return;
    }

    void ApplyUploadCreateOrUpdate(bool isErrorMode)
    {
        int rc;
        SCRow_uploadChangeLog currUploadEntry;
        // afterRowId supports going on to the next request before the last
        // request is complete.  Required for parallel operations.  Only
        // set when a "Task" is spawned.
        u64 afterRowId = 0;
        u64 errorModeMaxRowId = 0;  // Only valid when isErrorMode
        std::vector<UploadFileTask*> enqueuedUlTasks;
        std::vector<UploadFileResult*> enqueuedUlResults;
        std::vector<SyncUpPostFileMetadataEntry> postFileMetadataBatch;
        VPLTime_t lastBatchProcessed = VPLTime_GetTimeStamp();
        ASSERT(!VPLMutex_LockedSelf(&mutex));

        if (isErrorMode) {
            rc = localDb.uploadChangeLog_getMaxRowId(errorModeMaxRowId);
            if (rc != 0) {
                LOG_CRITICAL("%p: Should never happen:uploadChangeLog_getMaxRowId,%d", this, rc);
                HANDLE_DB_FAILURE();
                return;
            }
        }
        // For each entry in UploadChangeLog:
        int nextChangeRc;
        while ((nextChangeRc = getNextUploadChangeLog(isErrorMode,
                                                      afterRowId,
                                                      errorModeMaxRowId,
                                                      SCDB_ROW_FILTER_ALLOW_CREATE_AND_UPDATE,
                                                      /*out*/ currUploadEntry)) == 0)
        {
            bool taskEnqueueAttempt = false;  // true when upload task was spawned
                                              // or attempted to be spawned.
            bool skip_ErrInc = false;  // When true, appends to error.
            if (!isValidDirEntry(currUploadEntry.name)) {
                FAILED_ASSERT("Bad entry added to upload change log: \"%s\"", currUploadEntry.name.c_str());
                goto skip;
            }
            if (!useUploadThreadPool())
            {   // Not using thread pool, allowed to just block
                if (checkForPauseStop()) {
                    goto end_function;
                }
            }
            // Case Add Directory:
            if ((currUploadEntry.upload_action == UPLOAD_ACTION_CREATE) && currUploadEntry.is_dir) {
                // Call VCS POST dir.
                u64 parentCompId;
                BEGIN_TRANSACTION();
                rc = getParentCompId(currUploadEntry, /*OUT*/ parentCompId);
                CHECK_END_TRANSACTION_BYTES(0, false);
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
                    if (setHttpHandleAndCheckForStop(httpHandle)) { goto end_function; }
                    rc = vcs_make_dir(vcs_session, httpHandle, dataset,
                            currUploadEntryPath.str(),
                            parentCompId,
                            DEFAULT_MYSQL_DATE, // infoLastChanged
                            DEFAULT_MYSQL_DATE, // infoCreateDate
                            my_device_id(),
                            verbose_http_log,
                            vcsMakeDirOut);
                    if (clearHttpHandleAndCheckForStopWithinTask(httpHandle)) { goto end_function; }
                    if (!isTransientError(rc)) { break; }
                    LOG_WARN("%p: Transient Error:%d RetryIndex:%d", this, rc, retries);
                    if (retries+1 < NUM_IMMEDIATE_TRANSIENT_RETRY) {  // pause between immediate retries
                        if(checkForPauseStopWithinTask(true, QUICK_RETRY_INTERVAL)) {
                            goto end_function;
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
                    BEGIN_TRANSACTION();
                    rc = localDb.syncHistoryTree_add(newEntry);
                    if (rc < 0) {
                        LOG_CRITICAL("%p: syncHistoryTree_add(%s) failed: %d", this, currUploadEntryPath.c_str(), rc);
                        HANDLE_DB_FAILURE();
                        goto skip;  // skip, retry doesn't have much hope to recover.
                    }
                    CHECK_END_TRANSACTION_BYTES(0, false);
                } else if (isTransientError(rc)) {
                    LOG_WARN("%p: vcs_post_dir transient error: %d", this, rc);
                    // Transient error; retry later.
                    skip_ErrInc = true;
                    goto skip;
                } else {
                    // Non-retryable; need to do another server scan.
                    if (rc == VCS_PROVIDED_FOLDER_PATH_DOESNT_MATCH_PROVIDED_PARENTCOMPID) {
                        // This can happen due to a slow migration (see
                        // http://wiki.ctbg.acer.com/wiki/index.php/Expected_VCS_error_cases).
                        LOG_WARN("%p: Master state out of sync with infra: %d,parentCompId("FMTu64"),mkdir:%s",
                                  this, rc, parentCompId, currUploadEntryPath.c_str());
                        setErrorNeedsRemoteScan(VCS_RESCAN_DELAY);
                    } else {
                        // Unexpected:
                        LOG_ERROR("%p: Master state out of sync with infra: %d,parentCompId("FMTu64"),mkdir:%s",
                                  this, rc, parentCompId, currUploadEntryPath.c_str());
                        setErrorNeedsRemoteScan();
                    }
                    goto skip;
                }
            }
            // Case Add File or Update File:
            else if (!currUploadEntry.is_dir &&
                     ( (currUploadEntry.upload_action == UPLOAD_ACTION_CREATE) ||
                       (currUploadEntry.upload_action == UPLOAD_ACTION_UPDATE)))
            {
                // Get the current compId, revision, and hash for the file from the localDB.
                bool currUploadEntryHasCompId = false;
                u64 currUploadEntryCompId = -1;
                u64 currUploadEntryRevisionToUpload = 1;
                u64 logOnlyCurrUploadMtime = 0;
                SCRelPath currUploadEntryRelPath = getRelPath(currUploadEntry);
                {
                    SCRow_syncHistoryTree currHistoryEntry;
                    rc = syncHistoryTree_getEntry(currUploadEntryRelPath, /*OUT*/currHistoryEntry);
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
                        goto end_function;
                    }
                }

                AbsPath currEntryAbsPath = getAbsPath(getRelPath(currUploadEntry));
                VPLTime_t fileModifyTime;
                VPLTime_t fileCreateTime;
                VPLFS_file_size_t fileSize;
                // FAT32 correctness case: If timestamp on FS is less than 2 seconds old, skip (proceed to next entry in ChangeLog). (It is safe to skip, because we expect to get a file monitor notification to check this file again in 2 seconds.)
                // (NOTE: If the local FS timestamp matches the localDB timestamp, it is *not* actually safe to skip this file (See CaseF, workaround #2)).
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
                        goto skip;
                    }
                    fileCreateTime = fsTimeToNormLocalTime(currEntryAbsPath, statBuf.vpl_ctime);
                    fileModifyTime = fsTimeToNormLocalTime(currEntryAbsPath, statBuf.vpl_mtime);
                    fileSize = statBuf.size;
                }
                u64 parentCompId;
                {
                    BEGIN_TRANSACTION();
                    rc = getParentCompId(currUploadEntry, parentCompId);
                    if (rc == SYNC_AGENT_ERR_STOPPING) {
                        goto end_function;
                    }
                    CHECK_END_TRANSACTION_BYTES(0, false);
                    if (isTransientError(rc)) {
                        // Transient error; retry this upload later.
                        LOG_WARN("%p: getParentCompId, (%s,%s): %d",
                                 this,
                                 currUploadEntry.parent_path.c_str(),
                                 currUploadEntry.name.c_str(), rc);
                        skip_ErrInc = true;
                        goto skip;
                    } else if (rc != 0) {
                        // Non-retryable; need to do another server scan.
                        setErrorNeedsRemoteScan();
                        LOG_ERROR("%p: getParentCompId, (%s,%s): %d",
                                  this,
                                  currUploadEntry.parent_path.c_str(),
                                  currUploadEntry.name.c_str(), rc);
                        // Since getParentCompId failed, we're in bad shape.
                        // Abandon all pending uploads to avoid uploading files that we can't commit.
                        rc = localDb.uploadChangeLog_clear();
                        if (rc < 0) {
                            LOG_CRITICAL("%p: uploadChangeLog_clear() failed: %d", this, rc);
                            HANDLE_DB_FAILURE();
                        }
                        goto end_function;
                    }
                }

                UploadFileTask* taskCtx = new UploadFileTask();
                taskCtx->currUploadEntry = currUploadEntry;
                taskCtx->thisPtr = this;

                taskCtx->parentCompId = parentCompId;
                taskCtx->fileModifyTime = fileModifyTime;
                taskCtx->fileCreateTime = fileCreateTime;
                taskCtx->fileSize = fileSize;
                taskCtx->currUploadEntryHasCompId = currUploadEntryHasCompId;
                taskCtx->currUploadEntryCompId = currUploadEntryCompId;
                taskCtx->currUploadEntryRevisionToUpload = currUploadEntryRevisionToUpload;
                taskCtx->logOnlyCurrUploadMtime = logOnlyCurrUploadMtime;

                taskCtx->mutex = &mutex;
                taskCtx->enqueuedUlTasks = &enqueuedUlTasks;
                taskCtx->enqueuedUlResults = &enqueuedUlResults;

                taskEnqueueAttempt = true;
                {   // Enqueue Task.
                    MutexAutoLock lock(&mutex);
                    enqueuedUlTasks.push_back(taskCtx);
                    if(useUploadThreadPool()) {
                        // Normal one way upload with thread pool.
                        if(hasDedicatedThread) {
                            rc = threadPool->AddTaskDedicatedThread(&threadPoolNotifier,
                                                                    PerformUploadTask,
                                                                    taskCtx,
                                                                    (u64)this);
                        } else {
                            rc = threadPool->AddTask(PerformUploadTask,
                                                     taskCtx,
                                                     (u64)this);
                        }
                    } else {
                        // No threadPool!  Do the work on this thread.
                        PerformUploadTask(taskCtx);
                        rc = 0;
                    }

                    if(rc == 0) {
                        afterRowId = currUploadEntry.row_id;
                        LOG_INFO("%p: task(%p): Created task uploadEntryRow("FMTu64")",
                                 this, taskCtx, afterRowId);
                    } else {
                        // Task enqueue attempt failed, but that's OK.  ThreadPool
                        // can be normally completely occupied.
                        enqueuedUlTasks.pop_back();
                        delete taskCtx;
                    }
                }
            } else {
                FAILED_ASSERT("Unexpected upload action:"FMTu64,
                              currUploadEntry.upload_action);
            }
          skip:
            if(!taskEnqueueAttempt) {
                bool handleResultEndFunction = false;
                handleChangeLogResult(skip_ErrInc,
                                      currUploadEntry,
                                      /*OUT*/    handleResultEndFunction);
                if(handleResultEndFunction) {
                    LOG_ERROR("%p: Handle result end function", this);
                    goto end_function;
                }
            } // else getNextUploadChangeLog could stay on the current element.  If we
              // successfully launched the current element as a task, we will rely on afterRowId to avoid
              // checking it again.  If we were unable to launch it (because the thread pool
              // was full), we will get the same element from getNextUploadChangeLog again
              // for the next iteration.

            /////////////////////////////////////////////////////////
            /////////////// Process any task results ////////////////
            // http://wiki.ctbg.acer.com/wiki/index.php/User_talk:Rlee/CCD_Sync_One_Way_Speedup
            {
                bool hasConcurrentTaskAfterRowId = hasNextConcurrentTaskAfterRowId(
                                                        isErrorMode,
                                                        afterRowId,
                                                        errorModeMaxRowId);
                bool threadPoolShutdown = false;
                bool findFreeThread = hasFreeThread(/*OUT*/threadPoolShutdown);
                MutexAutoLock lock(&mutex);

                do
                {

                    while(enqueuedUlResults.size() == 0  &&  // No results we can immediately process
                          !threadPoolShutdown &&
                          !stop_thread &&
                          useUploadThreadPool() &&
                          (
                            // Has further work that needs to be done, but need
                            // to wait for capacity or permission to run (user paused)
                            (hasConcurrentTaskAfterRowId &&
                             !findFreeThread) ||

                            // Paused (and no results to process)
                            !allowed_to_run ||

                            // Has outstanding tasks that need to complete, but
                            // need to wait for completion.
                            (enqueuedUlTasks.size() > 0 &&
                             !hasConcurrentTaskAfterRowId)
                          )
                         )
                    {
                        LOG_DEBUG("%p: About to wait: res:"FMTu_size_t", shut:%d, "
                                  "ptr:%p, has:%d, free:%d, task:"FMTu_size_t,
                                  this, enqueuedUlResults.size(), threadPoolShutdown,
                                  threadPool, hasConcurrentTaskAfterRowId,
                                 findFreeThread, enqueuedUlTasks.size());

                        if (!allowed_to_run && enqueuedUlTasks.size() == 0)
                        {   // Notify for blocking interface that we are about to pause
                            worker_loop_paused_or_stopped = true;
                                            // If the pause request was blocking, signal the waiting thread now.
                            if (worker_loop_wait_for_pause_sem != NULL) {
                                int rc = VPLSem_Post(worker_loop_wait_for_pause_sem);
                                if (rc != 0) {
                                    LOG_ERROR("%p: VPLSem_Post(%p):%d",
                                              this, worker_loop_wait_for_pause_sem, rc);
                                }
                            }
                        }

                        lock.UnlockNow();

                        ASSERT(!VPLMutex_LockedSelf(&mutex));  // still may be locked (recursive)
                        const static VPLTime_t BATCH_COMMIT_MAX_TIME = VPLTime_FromSec(30);
                        rc = VPLSem_TimedWait(&threadPoolNotifier, BATCH_COMMIT_MAX_TIME);
                        if (rc != 0 && rc != VPL_ERR_TIMEOUT) {
                            LOG_WARN("VPLSem_Wait failed: %d", rc);
                        }
                        LOG_DEBUG("%p: Done waiting", this);

                        lock.Relock(&mutex);

                        if(rc == VPL_ERR_TIMEOUT &&
                           VPLTime_DiffClamp(VPLTime_GetTimeStamp(),
                                             lastBatchProcessed) >= BATCH_COMMIT_MAX_TIME)
                        {
                            bool endFunction = false;
                            LOG_INFO("Calling processPostFileMetadata after "FMTu64"ms. LastBatchProcessed("FMTu64"ms)",
                                     VPLTime_ToMillisec(BATCH_COMMIT_MAX_TIME),
                                     VPLTime_ToMillisec(lastBatchProcessed));
                            processPostFileMetadataBatch(postFileMetadataBatch,
                                                         lastBatchProcessed,
                                                         endFunction);
                            if (endFunction) {
                                goto end_function;
                            }
                            continue;
                        }

                        hasConcurrentTaskAfterRowId = hasNextConcurrentTaskAfterRowId(
                                                            isErrorMode,
                                                            afterRowId,
                                                            errorModeMaxRowId);
                        findFreeThread = hasFreeThread(/*OUT*/threadPoolShutdown);
                    }

                    // Handle results whether threadpool is NULL or not
                    while(enqueuedUlResults.size() > 0)
                    {
                        bool handleResultEndFunction = false;
                        UploadFileResult* result = enqueuedUlResults.back();
                        lock.UnlockNow();

                        handleUlTaskResult(result,
                                           postFileMetadataBatch,
                                           lastBatchProcessed,
                                           /*OUT*/    handleResultEndFunction);
                        LOG_INFO("%p: Processed result(%p), rowId:"FMTu64",afterRowId:"FMTu64,
                                 this, result, result->postFileMetadataEntry.currUploadEntry.row_id, afterRowId);
                        lock.Relock(&mutex);
                        bool endFunction = result->endFunction || handleResultEndFunction;
                        removeEnqueuedUlResult(result->postFileMetadataEntry.currUploadEntry.row_id,
                                               enqueuedUlResults);
                        if(endFunction) {
                            // One of the tasks resulted in an end function.
                            goto end_function;
                        }
                    }
                    // If Pause is enabled and using threadpool, keep in this loop to Pause
                    // Otherwise checkForPauseStop will be called at the beginning
                    //  of the next round.
                } while(!allowed_to_run && useUploadThreadPool() && !stop_thread);

                if (allowed_to_run) {
                    worker_loop_paused_or_stopped = false;
                }

                if (stop_thread) {
                    goto end_function;
                }
            }  // End Scope of MutexAutoLock lock(&mutex);
            /////////////// Process any task results ////////////////
            /////////////////////////////////////////////////////////

        } // while ((nextChangeRc = getNextUploadChangeLog(...)) == 0)
        if (nextChangeRc != SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) {
            LOG_CRITICAL("%p: getNextUploadChangeLog() failed: %d", this, nextChangeRc);
            HANDLE_DB_FAILURE();
        }
 end_function:
        {
            MutexAutoLock lock(&mutex);
            while(enqueuedUlTasks.size() > 0 || enqueuedUlResults.size() > 0)
            {
                if(enqueuedUlResults.size()==0) {
                    LOG_INFO("%p: Waiting for phase ("FMTu_size_t" tasks need completion, "FMTu_size_t" to process)",
                             this, enqueuedUlTasks.size(), enqueuedUlResults.size());
                    lock.UnlockNow();
                    ASSERT(!VPLMutex_LockedSelf(&mutex));  // still may be locked (recursive)
                    VPLSem_Wait(&threadPoolNotifier);
                    lock.Relock(&mutex);
                }
                while(enqueuedUlResults.size() > 0)
                {
                    bool unused_endFunction; // already in end_function section.
                    UploadFileResult* result = enqueuedUlResults.back();
                    lock.UnlockNow();

                    handleUlTaskResult(result,
                                       postFileMetadataBatch,
                                       lastBatchProcessed,
                                       /*OUT*/ unused_endFunction);
                    LOG_INFO("%p: Processed result(%p), rowId:"FMTu64", endNow:%d",
                             this, result,
                             result->postFileMetadataEntry.currUploadEntry.row_id,
                             unused_endFunction);

                    lock.Relock(&mutex);
                    removeEnqueuedUlResult(result->postFileMetadataEntry.currUploadEntry.row_id,
                                           enqueuedUlResults);
                }
            }
        }
        if (!checkForPauseStop()) {
            bool unused_endFunction = false;
            processPostFileMetadataBatch(postFileMetadataBatch,
                                         lastBatchProcessed,
                                         unused_endFunction);
        }
        CHECK_END_TRANSACTION_BYTES(0, true);
    }

    /// Applies the local changes to the server.
    void ApplyUploadChangeLog(bool isErrorMode)
    {
        ApplyUploadCreateOrUpdate(isErrorMode);

        if (checkForPauseStop()) { return; }
        // Apply change log deletions that are files
        ApplyUploadDeletions(false,  // isDirectory
                             isErrorMode);

        if (checkForPauseStop()) { return; }
        // Apply change log deletions that are directories
        ApplyUploadDeletions(true,  // isDirectory
                             isErrorMode);
    }

    void removeEnqueuedUlResult(u64 rowId,
                                std::vector<UploadFileResult*>& enqueued_results)
    {
        ASSERT(VPLMutex_LockedSelf(&mutex));
        // Remove the finished result, go backwards since we last examined the last element
        for(int index = enqueued_results.size()-1;
            index >= 0;
            --index)
        {
            UploadFileResult* toDelete = enqueued_results[index];
            if(toDelete->postFileMetadataEntry.currUploadEntry.row_id == rowId)
            {   // Found it
                enqueued_results.erase(enqueued_results.begin()+index);
                LOG_DEBUG("%p: result(%p): Delete UploadFileResult", this, toDelete);
                delete toDelete;
                break;
            }
        }
    }

    static void removeEnqueuedUlTask(u64 rowId,
                                     std::vector<UploadFileTask*>& enqueued_tasks)
    {
        for(std::vector<UploadFileTask*>::iterator taskIter = enqueued_tasks.begin();
            taskIter != enqueued_tasks.end(); ++taskIter)
        {
            if((*taskIter)->currUploadEntry.row_id == rowId) {
                UploadFileTask* toDelete = *taskIter;
                enqueued_tasks.erase(taskIter);
                delete toDelete;
                break;
            }
        }
    }

    void handleUlTaskResult(UploadFileResult* result,
                            std::vector<SyncUpPostFileMetadataEntry>& postFileMetadataBatch,
                            VPLTime_t& lastBatchProcessed,
                            bool& endFunction)
    {
        endFunction = false;

        if (result == NULL) {
            return;
        }

        if (result->skip_ErrInc || result->skip) {
            handleChangeLogResult(result->skip_ErrInc,
                                  result->postFileMetadataEntry.currUploadEntry,
                                  /*OUT*/ endFunction);
            return;
        }

        if (result->endFunction || endFunction) {
            return;
        }

        postFileMetadataBatch.push_back(result->postFileMetadataEntry);

        const static size_t POST_FILE_METADATA_MAX_BATCH_SIZE = 25;
        if (postFileMetadataBatch.size() >= POST_FILE_METADATA_MAX_BATCH_SIZE)
        {
            processPostFileMetadataBatch(postFileMetadataBatch,
                                         lastBatchProcessed,
                                         endFunction);
        }
    }

    void SyncUpPostFileMetadata_To_VcsBatchFileMetadataRequests(
            const std::vector<SyncUpPostFileMetadataEntry>& postFileMetadataBatch,
            std::vector<VcsBatchFileMetadataRequest>& pfmBatchRequests_out)
    {
        pfmBatchRequests_out.clear();

        for(std::vector<SyncUpPostFileMetadataEntry>::const_iterator pfmIter =
                postFileMetadataBatch.begin();
            pfmIter != postFileMetadataBatch.end(); ++pfmIter)
        {
            VcsBatchFileMetadataRequest request;
            request.folderCompId = pfmIter->parentCompId;
            request.folderPath = std::string("/")+getDatasetRelPath(
                    getRelPath(pfmIter->currUploadEntry).getParent()).str();
            request.fileName = pfmIter->currUploadEntry.name;
            request.size = pfmIter->fileSize;
            request.updateDevice = my_device_id();
            request.hasAccessUrl = !pfmIter->virtualUpload;
            request.accessUrl = pfmIter->accessUrl;
            request.lastChanged = pfmIter->fileModifyTime;
            request.createDate = pfmIter->fileCreateTime;
            request.hasContentHash = false;
            request.uploadRevision = pfmIter->currUploadEntryRevisionToUpload;
            request.hasBaseRevision = false;
            request.hasCompId = pfmIter->currUploadEntryHasCompId;
            request.compId = pfmIter->currUploadEntryCompId;

            pfmBatchRequests_out.push_back(request);
        }
    }

    void processPfmBatchAsTransientError(
            const std::vector<SyncUpPostFileMetadataEntry>& postFileMetadataBatch,
            bool& endFunction_out)
    {
        endFunction_out = false;
        for(std::vector<SyncUpPostFileMetadataEntry>::const_iterator pfmIter =
                postFileMetadataBatch.begin();
            pfmIter != postFileMetadataBatch.end(); ++pfmIter)
        {
            bool tempEndFunction = false;
            const static bool errorInc = true;
            handleChangeLogResult(errorInc,
                                  pfmIter->currUploadEntry,
                                  /*OUT*/ tempEndFunction);
            endFunction_out |= tempEndFunction;
        }
    }

    void processPfmBatchAsPermanentError(
            const std::vector<SyncUpPostFileMetadataEntry>& postFileMetadataBatch,
            bool& endFunction_out)
    {
        endFunction_out = false;
        for(std::vector<SyncUpPostFileMetadataEntry>::const_iterator pfmIter =
                postFileMetadataBatch.begin();
            pfmIter != postFileMetadataBatch.end(); ++pfmIter)
        {
            bool tempEndFunction = false;
            const static bool errorInc = false;  // remove from table.  (success looks similar to permanent error)
            LOG_ERROR("%p: ACTION UP Batch Drop, Abandoning(%s,%s),"FMTu64",%d,"FMTu64",row_id:"FMTu64
                      " -- Attempted "FMTu64" times",
                      this,
                      pfmIter->currUploadEntry.parent_path.c_str(),
                      pfmIter->currUploadEntry.name.c_str(),
                      pfmIter->currUploadEntry.comp_id,
                      pfmIter->currUploadEntry.revision_exists,
                      pfmIter->currUploadEntry.revision,
                      pfmIter->currUploadEntry.row_id,
                      pfmIter->currUploadEntry.upload_err_count);
            handleChangeLogResult(errorInc,
                                  pfmIter->currUploadEntry,
                                  /*OUT*/ tempEndFunction);
            endFunction_out |= tempEndFunction;
        }
    }

    void processPostFileMetadataBatch(
            std::vector<SyncUpPostFileMetadataEntry>& postFileMetadataBatch,
            VPLTime_t& lastBatchProcessed,
            bool& endFunction)
    {
        int rc;
        endFunction = false;

        VcsBatchFileMetadataResponse batchPFMResponse;

        {
            std::vector<VcsBatchFileMetadataRequest> batchPFMRequests;
            SyncUpPostFileMetadata_To_VcsBatchFileMetadataRequests(
                                                    postFileMetadataBatch,
                                                    /*OUT*/ batchPFMRequests);

            for (u32 retries=0; retries < NUM_IMMEDIATE_TRANSIENT_RETRY; ++retries)
            {
                VPLHttp2 httpHandle;
                if (setHttpHandleAndCheckForStop(httpHandle)) { endFunction = true; return; }
                rc = vcs_batch_post_file_metadata(vcs_session,
                                                  httpHandle,
                                                  dataset,
                                                  verbose_http_log,
                                                  batchPFMRequests,
                                                  /*OUT*/ batchPFMResponse);
                if (clearHttpHandleAndCheckForPauseStop(httpHandle)) { return; }
                if (!isTransientError(rc)) { break; }
                LOG_WARN("%p: Transient Error:%d RetryIndex:%d", this, rc, retries);
                if (retries+1 < NUM_IMMEDIATE_TRANSIENT_RETRY) {  // pause between immediate retries
                    if(checkForPauseStop(true, QUICK_RETRY_INTERVAL)) {
                        endFunction = true;
                        return;
                    }
                }
            }
        }

        if (isTransientError(rc)) {
            LOG_WARN("%p: vcs_batch_post_file_metadata(%d), batchSize("FMTu64")",
                     this, rc, (u64)postFileMetadataBatch.size());
            bool tempEndFunction;
            processPfmBatchAsTransientError(postFileMetadataBatch, /*OUT*/ tempEndFunction);
            endFunction |= tempEndFunction;
            goto all_processed;
        } else if (rc != 0) {
            // Permanent Error
            LOG_ERROR("%p: Permanent error vcs_batch_post_file_metadata(%d), batchSize("FMTu64")",
                      this, rc, (u64)postFileMetadataBatch.size());
            bool tempEndFunction;
            processPfmBatchAsPermanentError(postFileMetadataBatch, /*OUT*/ tempEndFunction);
            endFunction |= tempEndFunction;
            goto all_processed;
        }

        ASSERT(rc==0);

        { // Verify number of entries
            u64 entriesReceived = 0;
            for(std::vector<VcsBatchFileMetadataResponse_Folder>::const_iterator
                                folderIter = batchPFMResponse.folders.begin();
                            folderIter != batchPFMResponse.folders.end();
                            ++folderIter)
            {
                entriesReceived += folderIter->files.size();
            }

            if (batchPFMResponse.entriesReceived != entriesReceived ||
                postFileMetadataBatch.size() != entriesReceived)
            {
                LOG_ERROR("%p:Response has inaccurate entries received, ("
                          FMTu64","FMTu64"). Expected:"FMTu64,
                          this, entriesReceived,
                          batchPFMResponse.entriesReceived,
                          (u64)postFileMetadataBatch.size());
                bool tempEndFunction;
                processPfmBatchAsTransientError(postFileMetadataBatch, /*OUT*/ tempEndFunction);
                endFunction |= tempEndFunction;
                goto all_processed;
            }
        } // Done verifying number of entries

        {
            u32 entryNum = 0;  // Tracks the index of SyncUpPostFileMetadataEntry
            bool clearUploadChangeLog = false;
            for(std::vector<VcsBatchFileMetadataResponse_Folder>::const_iterator
                    folderIter = batchPFMResponse.folders.begin();
                folderIter != batchPFMResponse.folders.end();
                ++folderIter)
            {
                for(std::vector<VcsBatchFileMetadataResponse_File>::const_iterator
                        fileIter = folderIter->files.begin();
                    fileIter != folderIter->files.end();
                    ++fileIter)
                {
                    DatasetRelPath currUploadEntryDatasetRelPath =
                        getDatasetRelPath(postFileMetadataBatch[entryNum].currUploadEntry);
                    AbsPath currEntryAbsPath = getAbsPath(getRelPath(
                            postFileMetadataBatch[entryNum].currUploadEntry));

                    bool skip_errInc = false;

                    if (fileIter->errCode == 0) {
                        SCRow_syncHistoryTree uploadedDbEntry;

                        if (fileIter->revisionList.size()!=1 ||
                            fileIter->numOfRevisions!=1)
                        {
                            LOG_ERROR("%p: Server Error numRevision not 1, "
                                      "Retry later:%s - compId:"FMTu64
                                      ", numRevision:"FMTu64", revisionListSize:"FMTu64", entryNum:%d",
                                      this, fileIter->name.c_str(),
                                      fileIter->compId,
                                      fileIter->numOfRevisions,
                                      (u64)fileIter->revisionList.size(), entryNum);
                            skip_errInc = true;
                            goto skip;
                        }

                        uploadedDbEntry.parent_path =
                                postFileMetadataBatch[entryNum].currUploadEntry.parent_path;
                        uploadedDbEntry.name =
                                postFileMetadataBatch[entryNum].currUploadEntry.name;
                        uploadedDbEntry.is_dir = false;
                        uploadedDbEntry.comp_id_exists = true;
                        uploadedDbEntry.comp_id = fileIter->compId;
                        uploadedDbEntry.revision_exists = true;
                        uploadedDbEntry.revision = fileIter->revisionList[0].revision;
                        uploadedDbEntry.local_mtime_exists = true;
                        uploadedDbEntry.local_mtime = VPLTime_ToMicrosec(fileIter->lastChanged);
                        uploadedDbEntry.last_seen_in_version_exists = false;
                        uploadedDbEntry.version_scanned_exists = false;

                        LOG_INFO("%p: ACTION UP post_filemetadata done:%s (compId:"FMTu64
                                 ", lastChangedNano:"FMTu64", createDateNano:"FMTu64
                                 ", parentFolderCompId:"FMTu64")",
                                 this, currEntryAbsPath.c_str(),
                                 fileIter->compId,
                                 fileIter->lastChanged,
                                 fileIter->createDate,
                                 folderIter->folderCompId);
                        BEGIN_TRANSACTION();
                        rc = localDb.syncHistoryTree_add(uploadedDbEntry);
                        if (rc != 0) {
                            LOG_CRITICAL("%p: syncHistoryTree_add:%d", this, rc);
                            HANDLE_DB_FAILURE();
                            endFunction = true;
                        }
                        CHECK_END_TRANSACTION_BYTES(fileIter->revisionList[0].size, false);

                        if (virtualUpload()) {
                            BEGIN_TRANSACTION();
                            rc = localDb.deferredUploadJobSet_add(uploadedDbEntry.parent_path,
                                                                  uploadedDbEntry.name,
                                                                  folderIter->folderCompId,
                                                                  uploadedDbEntry.comp_id_exists,
                                                                  uploadedDbEntry.comp_id,
                                                                  uploadedDbEntry.revision_exists,
                                                                  uploadedDbEntry.revision,
                                                                  uploadedDbEntry.local_mtime);
                            if (rc != 0) {
                                LOG_CRITICAL("%p: deferredUploadJobSet_add:%d",
                                             this, rc);
                                HANDLE_DB_FAILURE();
                                endFunction = true;
                            }
                            // Force-commit the transactions.  We want the deferred upload thread
                            // to see the uploads that need to be done when it is signaled.
                            CHECK_END_TRANSACTION_BYTES(0, true);

                            MutexAutoLock lock(&mutex);
                            deferredUploadThreadPerformUpload = true;
                            VPLCond_Broadcast(&work_to_do_cond_var);
                        }
                    } else if (isTransientError(fileIter->errCode)) {
                        // Transient error; retry this upload later.
                        LOG_WARN("%p: vcs_post_file_metadata(%d) parentCompId("FMTu64"),"
                                 "postFileMetadata(%s),compId("FMTu64")",
                                 this, fileIter->errCode,
                                 folderIter->folderCompId,
                                 currUploadEntryDatasetRelPath.c_str(),
                                 fileIter->compId);
                        skip_errInc = true;
                        goto skip;
                    } else
                    {   // Non-retryable permanent error; need to do another server scan.
                        if (fileIter->errCode == VCS_ERR_UPLOADREVISION_NOT_HWM_PLUS_1)
                        { // Expected; see http://wiki.ctbg.acer.com/wiki/index.php/Expected_VCS_error_cases
                            // Bug 11559: There is no need to pause, but this is an emergency
                            // patch, so we'll play it safe by sleeping 1 minute, in
                            // case there's some larger spin.
                            setErrorNeedsRemoteScan(VCS_RESCAN_DELAY);
                            LOG_INFO("%p: Handling UploadRevision problem through re-scan. Clearing uploadChangeLog."
                                     " vcs_post_file_metadata(%d): parentCompId("FMTu64"),postFileMetadata(%s),compId("FMTu64"),rev(%d,"FMTu64")",
                                     this, fileIter->errCode,
                                     folderIter->folderCompId,
                                     currUploadEntryDatasetRelPath.c_str(),
                                     fileIter->compId,
                                     (fileIter->revisionList.empty()?0:1),
                                     (fileIter->revisionList.empty()?0:fileIter->revisionList[0].revision));
                        } else {
                            setErrorNeedsRemoteScan();
                            LOG_ERROR("%p: vcs_post_file_metadata(%d): parentCompId("FMTu64"),postFileMetadata(%s),compId("FMTu64"). Clearing uploadChangeLog.",
                                      this, fileIter->errCode,
                                      folderIter->folderCompId,
                                      currUploadEntryDatasetRelPath.c_str(),
                                      fileIter->compId);
                        }
                        // Abandon all pending uploads to avoid uploading files that we can't commit.
                        clearUploadChangeLog = true;
                        endFunction = true;
                        goto skip;
                    }
 skip:
                    {
                        bool endFunctionChangeLog = false;
                        handleChangeLogResult(skip_errInc,
                                              postFileMetadataBatch[entryNum].currUploadEntry,
                                              /*OUT*/ endFunctionChangeLog);
                        endFunction |= endFunctionChangeLog;
                    }

                    entryNum++;
                }
            }
            if(clearUploadChangeLog) {
                BEGIN_TRANSACTION();
                rc = localDb.uploadChangeLog_clear();
                if (rc < 0) {
                    LOG_CRITICAL("%p: uploadChangeLog_clear() failed: %d", this, rc);
                    HANDLE_DB_FAILURE();
                    endFunction = true;
                }
                CHECK_END_TRANSACTION_BYTES(0, false);
            }
        }
 all_processed:
        postFileMetadataBatch.clear();
        lastBatchProcessed = VPLTime_GetTimeStamp();
    }

    void handleChangeLogResult(bool skip_ErrInc,
                               const SCRow_uploadChangeLog& currUploadEntry,
                               /*OUT*/    bool& endFunction)
    {
        int rc;
        endFunction = false;
        if (skip_ErrInc) {
            if (currUploadEntry.upload_err_count > ERROR_COUNT_LIMIT) {
                LOG_ERROR("%p: ACTION UP Abandoning error:%s,%s,"FMTu64",%d,"FMTu64",row_id:"FMTu64
                          " -- Attempted "FMTu64" times",
                          this,
                          currUploadEntry.parent_path.c_str(),
                          currUploadEntry.name.c_str(),
                          currUploadEntry.comp_id,
                          currUploadEntry.revision_exists,
                          currUploadEntry.revision,
                          currUploadEntry.row_id,
                          currUploadEntry.upload_err_count);
                BEGIN_TRANSACTION();
                rc = localDb.uploadChangeLog_remove(currUploadEntry.row_id);
                if (rc == SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) {
                    LOG_WARN("%p: already removed, uploadChangeLog_remove("FMTu64"). Continuing.",
                             this, currUploadEntry.row_id);
                    return;
                } else if (rc != 0) {
                    LOG_CRITICAL("%p: uploadChangeLog_remove("FMTu64") failed: %d",
                            this, currUploadEntry.row_id, rc);
                    HANDLE_DB_FAILURE();
                    endFunction = true;
                    return;
                }
                CHECK_END_TRANSACTION_BYTES(0, false);
            } else {
                BEGIN_TRANSACTION();
                rc = localDb.uploadChangeLog_incErr(currUploadEntry.row_id);
                if (rc == SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) {
                    LOG_WARN("%p: already removed, uploadChangeLog_incErr("FMTu64"). Continuing.",
                             this, currUploadEntry.row_id);
                    return;
                } else if (rc != 0) {
                    LOG_CRITICAL("%p: uploadChangeLog_incErr("FMTu64") failed: %d",
                            this, currUploadEntry.row_id, rc);
                    HANDLE_DB_FAILURE();
                    endFunction = true;
                    return;
                }
                CHECK_END_TRANSACTION_BYTES(0, false);
                MutexAutoLock lock(&mutex);
                setErrorTimeout(sync_policy.error_retry_interval);
            }
        } else {  // uploadChangeLog entry succeeded or is being dropped.
            BEGIN_TRANSACTION();
            rc = localDb.uploadChangeLog_remove(currUploadEntry.row_id);
            if (rc == SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) {
                LOG_WARN("%p: already removed, uploadChangeLog_remove("FMTu64"). Continuing.",
                         this, currUploadEntry.row_id);
                return;
            }else if (rc != 0) {
                LOG_CRITICAL("%p: uploadChangeLog_remove("FMTu64") failed: %d",
                        this, currUploadEntry.row_id, rc);
                HANDLE_DB_FAILURE();
                endFunction = true;
                return;
            }
            CHECK_END_TRANSACTION_BYTES(0, false);
        }
    }

    /// Caller is responsible for handling error.
    ///   SYNC_AGENT_ERR_STOPPING
    static int vcsPostFileMetadataAfterAcsUpload(SyncConfigOneWayUpImpl* thisPtr,
                                                 const std::string& currUploadEntryDatasetRelPath,
                                                 u64 parentCompId,
                                                 bool currUploadEntryHasCompId,
                                                 u64 currUploadEntryCompId,
                                                 u64 currUploadEntryRevisionToUpload,
                                                 VPLTime_t modifyTime,
                                                 VPLTime_t createTime,
                                                 u64 fileSize,
                                                 bool accessUrlExists,
                                                 const std::string& accessUrl,
                                                 const SCRow_uploadChangeLog& currUploadEntry,
                                                 bool& updateDb,
                                                 SCRow_syncHistoryTree& uploadedDbEntry)
    {
        int rv = 0;
        updateDb = false;
        uploadedDbEntry.clear();

        VcsFileMetadataResponse fileMetadataResp;
        for (u32 retries=0; retries < NUM_IMMEDIATE_TRANSIENT_RETRY; ++retries)
        {
            VPLHttp2 httpHandle;
            if (thisPtr->setHttpHandleAndCheckForStop(httpHandle)) { return SYNC_AGENT_ERR_STOPPING; }
            rv = vcs_post_file_metadata(thisPtr->vcs_session,
                                        httpHandle,
                                        thisPtr->dataset,
                                        currUploadEntryDatasetRelPath,
                                        parentCompId,
                                        currUploadEntryHasCompId,
                                        currUploadEntryCompId,
                                        currUploadEntryRevisionToUpload,
                                        modifyTime,
                                        createTime,
                                        fileSize,
                                        std::string("") /* TODO: contentHash */,
                                        std::string("") /* Now only using for 2way plus FILE_COMPARE_POLICY_USE_HASH */,
                                        thisPtr->my_device_id(),
                                        accessUrlExists,
                                        accessUrl,
                                        thisPtr->verbose_http_log,
                                        fileMetadataResp);
            if (thisPtr->clearHttpHandleAndCheckForStopWithinTask(httpHandle)) { return SYNC_AGENT_ERR_STOPPING; }
            if (!isTransientError(rv)) { break; }
            LOG_WARN("%p: Transient Error:%d RetryIndex:%d", thisPtr, rv, retries);
            if (retries+1 < NUM_IMMEDIATE_TRANSIENT_RETRY) {  // pause between immediate retries
                if(thisPtr->checkForPauseStopWithinTask(true, QUICK_RETRY_INTERVAL)) {
                    return SYNC_AGENT_ERR_STOPPING;
                }
            }
        }

        if (rv != 0) {
            return rv;
        }
        ASSERT(rv == 0);
        // If VCS POST filemetadata succeeds, update LocalDB entry with the new modified time, vcs_comp_id, and vcs_revision.
        if ((fileMetadataResp.numOfRevisions == 0) ||
            (fileMetadataResp.revisionList.size()==0))
        {  // 0 revisions?!?!
            LOG_ERROR("%p: fileMetadataResp has 0 revisions (%s, "FMTu64","FMTu64", "FMTu_size_t")",
                      thisPtr,
                      fileMetadataResp.name.c_str(),
                      fileMetadataResp.compId,
                      fileMetadataResp.numOfRevisions,
                      fileMetadataResp.revisionList.size());
            return SYNC_AGENT_ERR_FAIL;
        } else if (fileMetadataResp.numOfRevisions != 1) {
            LOG_ERROR("%p: fileMetadataResp multiple revisions (%s, "FMTu64","FMTu64", "FMTu_size_t"), arbitrarily using 1st one",
                      thisPtr,
                      fileMetadataResp.name.c_str(),
                      fileMetadataResp.compId,
                      fileMetadataResp.numOfRevisions,
                      fileMetadataResp.revisionList.size());
        }

        updateDb = true;
        uploadedDbEntry.parent_path = currUploadEntry.parent_path;
        uploadedDbEntry.name = currUploadEntry.name;
        uploadedDbEntry.is_dir = false;
        uploadedDbEntry.comp_id_exists = true;
        uploadedDbEntry.comp_id = fileMetadataResp.compId;
        uploadedDbEntry.revision_exists = true;
        uploadedDbEntry.revision = fileMetadataResp.revisionList[0].revision;
        uploadedDbEntry.local_mtime_exists = true;
        uploadedDbEntry.local_mtime = VPLTime_ToMicrosec(fileMetadataResp.lastChanged);
        uploadedDbEntry.last_seen_in_version_exists = false;
        uploadedDbEntry.version_scanned_exists = false;
        return 0;
    }

    //=========================================================================
    //========================== DEFERRED UPLOADS =============================

    struct DeferredUploadFileResult {
        SCRow_deferredUploadJobSet deferredUploadEntry;  // Identified by rowId (unchanged from UploadFileTask)

        bool skip_ErrInc;
        bool endFunction;

        bool removeDeferredJobSet;
        VPLFS_file_size_t fileSize;

        DeferredUploadFileResult()
        :  skip_ErrInc(false),
           endFunction(false),
           removeDeferredJobSet(false),
           fileSize(0)
        {}
    };

    struct DeferredUploadFileTask {
        SCRow_deferredUploadJobSet deferredUploadEntry;  // Identified by rowId
        SyncConfigOneWayUpImpl* thisPtr;

        VPLTime_t fileModifyTime;
        VPLTime_t fileCreateTime;
        VPLFS_file_size_t fileSize;

        bool currUploadEntryHasCompId;
        u64 currUploadEntryCompId;
        u64 currUploadEntryRevisionToUpload;
        u64 logOnlyCurrUploadMtime;

        VPLMutex_t* mutex;  // Used to protect enqueuedDeferredUlTasks and enqueuedDeferredUlResults
        std::vector<DeferredUploadFileTask*>* enqueuedDeferredUlTasks;
        std::vector<DeferredUploadFileResult*>* enqueuedDeferredUlResults;

        DeferredUploadFileTask()
        :  thisPtr(NULL),
           fileModifyTime(0),
           fileCreateTime(0),
           fileSize(0),
           currUploadEntryHasCompId(false),
           currUploadEntryCompId(0),
           currUploadEntryRevisionToUpload(0),
           logOnlyCurrUploadMtime(0),
           mutex(NULL),
           enqueuedDeferredUlTasks(NULL),
           enqueuedDeferredUlResults(NULL)
        {}
    };

    int getNextDeferredUploadJobSet(bool isErrorMode,
                                    u64 afterRowId,
                                    u64 errorModeMaxRowId,
                                    SCRow_deferredUploadJobSet& entry_out)
    {
        int rc = 0;
        if (isErrorMode) {
            rc = deferredUploadDb.deferredUploadJobSet_getErrAfterRowId(
                    afterRowId,
                    errorModeMaxRowId,
                    /*OUT*/entry_out);
        } else {
            rc = deferredUploadDb.deferredUploadJobSet_getAfterRowId(
                    afterRowId,
                    /*OUT*/entry_out);
        }
        return rc;
    }

    // Return true if there is an entry.
    bool hasNextConcurrentDeferredUploadJobSetAfterRowId(
            bool isErrorMode,
            u64 afterRowId,
            u64 errorModeMaxRowId) //Only relevant when isErrorMode = true
    {
        SCRow_deferredUploadJobSet uploadJob_unused;
        int rc;
        rc = getNextDeferredUploadJobSet(isErrorMode,
                                         afterRowId,
                                         errorModeMaxRowId,
                                         /*out*/ uploadJob_unused);
        if(rc != 0) {
            return false;  // entry does not exist
        }
        // All deferred upload jobs currently can be concurrent.
        return true;  // entry exists!
    }

    static void PerformDeferredUploadTask(void* ctx)
    {
        int rc;
        DeferredUploadFileTask* task = (DeferredUploadFileTask*) ctx;
        // result will be freed in removeEnqueuedDeferredUlResult(), called by deferredUploadLoop().
        // For this to happen, result must always be added to task->enqueuedDeferredUlResults
        // before leaving this function.
        DeferredUploadFileResult* result = new DeferredUploadFileResult();
        SyncConfigOneWayUpImpl* thisPtr = task->thisPtr;
        LOG_DEBUG("%p: task(%p): Created UploadFileResult:%p", thisPtr, task, result);

        AbsPath currEntryAbsPath = thisPtr->getAbsPath(getRelPath(task->deferredUploadEntry));
        LOG_INFO("%p: task(%p->%p): ACTION UP deferred_upload begin:%s rowId("FMTu64"),mtimes("FMTu64","FMTu64"),"
                 "ctime("FMTu64"),compId("FMTu64"),revUp("FMTu64")",
                 thisPtr, task, result,
                 currEntryAbsPath.c_str(),
                 task->deferredUploadEntry.row_id,
                 task->logOnlyCurrUploadMtime,
                 task->fileModifyTime,
                 task->fileCreateTime,
                 task->currUploadEntryHasCompId?task->currUploadEntryCompId:0,
                 task->currUploadEntryRevisionToUpload);
        // Compute local file hash.
        // (optional optimization 1) Call VCS GET filemetadata.  If VCS hash == local file hash,
        //     (optional optimization 1) Update LocalDB with new timestamp, hash, set vcs_comp_id = compId, and set vcs_revision = revision (for case when LocalDB was lost).
        //     (optional optimization 1) Proceed to next entry in ChangeLog.
        // (optional optimization 2) If (ChangeLog.vcs_comp_id != compId) || (ChangeLog.vcs_revision != revision),
        //     (optional optimization 2) Do conflict resolution.
        //     (optional optimization 2) Proceed to next entry in ChangeLog.

        // Get the 3rd party storage URL (accessUrl) from VCS GET accessinfo (pass method=PUT).
        VcsAccessInfo accessInfoResp;
        {
            bool skip;
            bool endFunction;
            getAccessInfoAndUpload(thisPtr,
                                   currEntryAbsPath,
                                   task->fileModifyTime,
                                   task->fileSize,
                                   task, // info only
                                   /*OUT*/ accessInfoResp,
                                   /*OUT*/result->skip_ErrInc,
                                   /*OUT*/skip,
                                   /*OUT*/endFunction);
            if (skip) {
                goto skip;
            }
            if (endFunction) {
                goto end_function;
            }
        }
        // Update VCS with the uploaded file data. Call VCS POST url.
        {
            DatasetRelPath currUploadEntryDatasetRelPath = thisPtr->getDatasetRelPath(task->deferredUploadEntry);

            for (u32 retries=0; retries < NUM_IMMEDIATE_TRANSIENT_RETRY; ++retries)
            {
                VPLHttp2 httpHandle;
                if (thisPtr->setHttpHandleAndCheckForStop(httpHandle)) { goto end_function; }
                rc = vcs_post_acs_access_url_virtual(thisPtr->vcs_session,
                                                     httpHandle,
                                                     thisPtr->dataset,
                                                     currUploadEntryDatasetRelPath.str(),
                                                     task->currUploadEntryCompId,
                                                     task->currUploadEntryRevisionToUpload,
                                                     accessInfoResp.accessUrl,
                                                     thisPtr->verbose_http_log);
                if (thisPtr->clearHttpHandleAndCheckForStopWithinTask(httpHandle)) { goto end_function; }
                if (!isTransientError(rc)) { break; }
                LOG_WARN("%p: task(%p): Transient Error:%d RetryIndex:%d", thisPtr, task, rc, retries);
                if (retries+1 < NUM_IMMEDIATE_TRANSIENT_RETRY) {  // pause between immediate retries
                    if(thisPtr->checkForPauseStopWithinTask(true, QUICK_RETRY_INTERVAL)) {
                        goto end_function;
                    }
                }
            }
            if (rc == 0) {
                LOG_INFO("%p: task(%p): ACTION UP deferred_upload done:%s",
                         thisPtr, task, currEntryAbsPath.c_str());
            } else {
                if (isTransientError(rc)) {
                    // Transient error; retry later.
                    result->skip_ErrInc = true;
                    LOG_WARN("%p: task(%p): POST url (%s) failed: %d", thisPtr, task, currUploadEntryDatasetRelPath.c_str(), rc);
                } else if (rc == VCS_ERR_REVISION_IS_DELETED) {
                    // Bug 16456: This is an expected case.  It happens when the revision passed to POST url
                    //   has already been replaced by a newer revision on VCS.
                    LOG_INFO("%p: task(%p): POST url (%s) failed; skip obsolete revision.", thisPtr, task, currUploadEntryDatasetRelPath.c_str());
                } else {
                    // Non-retryable; need to do another server scan.
                    thisPtr->setErrorNeedsRemoteScan();
                    LOG_ERROR("%p: task(%p): POST url (%s) failed: %d", thisPtr, task, currUploadEntryDatasetRelPath.c_str(), rc);
                }
                goto skip;
            }
        }
        goto done;

     end_function:
        result->endFunction = true;
        goto done;
     skip:
     done:
        {
            result->deferredUploadEntry = task->deferredUploadEntry;
            result->fileSize = task->fileSize;
            LOG_INFO("%p: task(%p) --> result(%p)", thisPtr, task, result);

            // Completed -- move from TaskList to ResultList.
            // Error or not, this must always be done.
            MutexAutoLock lock(task->mutex);
            task->enqueuedDeferredUlResults->push_back(result);
            thisPtr->removeEnqueuedDeferredUlTask(task->deferredUploadEntry.row_id,
                                                  *(task->enqueuedDeferredUlTasks));
        }
        return;
    }

    void removeEnqueuedDeferredUlResult(u64 rowId,
                                        std::vector<DeferredUploadFileResult*>& enqueued_results)
    {
        ASSERT(VPLMutex_LockedSelf(&mutex));
        // Remove the finished result, go backwards since we last examined the last element
        for(int index = enqueued_results.size()-1;
            index >= 0;
            --index)
        {
            DeferredUploadFileResult* toDelete = enqueued_results[index];
            if(toDelete->deferredUploadEntry.row_id == rowId)
            {   // Found it
                enqueued_results.erase(enqueued_results.begin()+index);
                LOG_DEBUG("%p: result(%p): Delete DeferredUploadFileResult", this, toDelete);
                delete toDelete;
                break;
            }
        }
    }

    static void removeEnqueuedDeferredUlTask(u64 rowId,
                                             std::vector<DeferredUploadFileTask*>& enqueued_tasks)
    {
        for(std::vector<DeferredUploadFileTask*>::iterator taskIter = enqueued_tasks.begin();
            taskIter != enqueued_tasks.end(); ++taskIter)
        {
            if((*taskIter)->deferredUploadEntry.row_id == rowId) {
                DeferredUploadFileTask* toDelete = *taskIter;
                enqueued_tasks.erase(taskIter);
                delete toDelete;
                break;
            }
        }
    }

    void handleDeferredUlTaskResult(DeferredUploadFileResult* result,
                                    bool& endFunction_out,
                                    VPLTime_t& errorTimeout_in_out)
    {
        endFunction_out = false;
        // In contrast to handleUlTaskResult(), there's no need to update SyncHistoryTree
        // here.  We will let infra update syncHistory tree, since datasetVersion
        // will be incremented (because the noAcs field changed for this component).
        //if(result->updateSyncHistoryTree) {
        //    BEGIN_TRANSACTION();
        //    rc = localDb.syncHistoryTree_add(result->syncHistoryEntry);
        //    if (rc != 0) {
        //        LOG_CRITICAL("%p: result(%p): syncHistoryTree_add:%d", this, result, rc);
        //        HANDLE_DB_FAILURE();
        //        endFunction = true;
        //    }
        //    CHECK_END_TRANSACTION_BYTES(result->fileSize, false);
        //}
        bool endFunctionChangeLog = false;
        handleJobSetResult(result->skip_ErrInc,
                           result->deferredUploadEntry,
                           /*OUT*/ endFunctionChangeLog,
                           /*IN,OUT*/ errorTimeout_in_out);
        endFunction_out |= endFunctionChangeLog;
    }

    void computeDeferredUploadErrorTimeout(VPLTime_t waitDuration,
                                           VPLTime_t& errorTimeout_in_out)
    {
        VPLTime_t setErrorTime = VPLTime_GetTimeStamp() + waitDuration;
        if(errorTimeout_in_out == VPLTIME_INVALID ||
           errorTimeout_in_out < setErrorTime)
        {  // Wait at least setErrorTime, because if there's a lot of work (that
           // takes sync_policy.error_retry_interval), we don't want the CPU to continuously spin.
            errorTimeout_in_out = setErrorTime;
        }
    }

    VPLTime_t getDeferredUploadTimeoutUntilNextRetryErrors(VPLTime_t errorTimeout)
    {
        if (errorTimeout == VPLTIME_INVALID) {
            return VPL_TIMEOUT_NONE;
        } else {
            return VPLTime_DiffClamp(errorTimeout, VPLTime_GetTimeStamp());
        }
    }

    void handleJobSetResult(bool skip_ErrInc,
                            const SCRow_deferredUploadJobSet& currUploadEntry,
                            bool& endFunction_out,
                            VPLTime_t& errorTimeout_in_out)
    {
        int rc;
        endFunction_out = false;
        if (skip_ErrInc) {
            if (currUploadEntry.error_count > ERROR_COUNT_LIMIT) {
                LOG_ERROR("%p: Abandoning error:%s,%s,"FMTu64",%d,"FMTu64
                          " -- Attempted "FMTu64" times, reason:(%d,"FMTs64")",
                          this,
                          currUploadEntry.parent_path.c_str(),
                          currUploadEntry.name.c_str(),
                          currUploadEntry.last_synced_parent_comp_id,
                          currUploadEntry.last_synced_revision_id_exists,
                          currUploadEntry.last_synced_revision_id,
                          currUploadEntry.error_count,
                          currUploadEntry.error_reason_exists,
                          currUploadEntry.error_reason);
                rc = deferredUploadDb.deferredUploadJobSet_remove(currUploadEntry.row_id);
                if (rc == SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) {
                    // ignore,
                    LOG_INFO("%p: deferred upload changed(%s,%s,"FMTu64"):"
                             "compId("FMTu64"),revisionId(%d,"FMTu64")",
                             this,
                             currUploadEntry.parent_path.c_str(),
                             currUploadEntry.name.c_str(),
                             currUploadEntry.row_id,
                             currUploadEntry.last_synced_parent_comp_id,
                             currUploadEntry.last_synced_revision_id_exists,
                             currUploadEntry.last_synced_revision_id);
                    goto end;
                }else if (rc != 0) {
                    LOG_CRITICAL("%p: deferredUploadJobSet_remove("FMTu64") failed: %d",
                                 this, currUploadEntry.row_id, rc);
                    HANDLE_DB_FAILURE();
                    endFunction_out = true;
                    goto end;
                }
            } else {
                const u64 ABSENT_ERROR_RETRY_TIME = 0;
                const u64 ABSENT_ERROR_REASON = 0;
                rc = deferredUploadDb.deferredUploadJobSet_incErr(currUploadEntry.row_id,
                                                                  ABSENT_ERROR_RETRY_TIME,
                                                                  ABSENT_ERROR_REASON);
                if (rc == SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) {
                    // ignore,
                    LOG_INFO("%p: deferred upload changed(%s,%s,"FMTu64"):"
                             "compId("FMTu64"),revisionId(%d,"FMTu64")",
                             this,
                             currUploadEntry.parent_path.c_str(),
                             currUploadEntry.name.c_str(),
                             currUploadEntry.row_id,
                             currUploadEntry.last_synced_parent_comp_id,
                             currUploadEntry.last_synced_revision_id_exists,
                             currUploadEntry.last_synced_revision_id);
                    goto end;
                } else if (rc != 0) {
                    LOG_CRITICAL("%p: uploadChangeLog_incErr("FMTu64") failed: %d",
                            this, currUploadEntry.row_id, rc);
                    HANDLE_DB_FAILURE();
                    endFunction_out = true;
                    goto end;
                }
                MutexAutoLock lock(&mutex);
                computeDeferredUploadErrorTimeout(sync_policy.error_retry_interval, /*IN,OUT*/ errorTimeout_in_out);
            }
        } else {  // deferredUploadJob succeeded.
            rc = deferredUploadDb.deferredUploadJobSet_remove(currUploadEntry.row_id);
            if (rc == SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) {
                // ignore,
                LOG_INFO("%p: deferred upload changed(%s,%s,"FMTu64"):"
                        "compId("FMTu64"),revisionId(%d,"FMTu64")",
                        this,
                        currUploadEntry.parent_path.c_str(),
                        currUploadEntry.name.c_str(),
                        currUploadEntry.row_id,
                        currUploadEntry.last_synced_parent_comp_id,
                        currUploadEntry.last_synced_revision_id_exists,
                        currUploadEntry.last_synced_revision_id);
                goto end;
            } else if (rc < 0) {
                LOG_CRITICAL("%p: uploadChangeLog_remove("FMTu64") failed: %d",
                        this, currUploadEntry.row_id, rc);
                HANDLE_DB_FAILURE();
                endFunction_out = true;
                goto end;
            }
        }
 end:
        MutexAutoLock lock(&mutex);
        calcDeferredUploadLoopStatus(deferredUploadDb);
    }

    void deferredUploadLoop()
    {
        VPLTime_t nextErrorRetryTime = VPLTIME_INVALID;
        {   // On first startup, update status of what work should be done.
            MutexAutoLock lock(&mutex);
            calcDeferredUploadLoopStatus(deferredUploadDb);

            if (deferUploadLoopStatusHasError &&
                (nextErrorRetryTime == VPLTIME_INVALID ||
                 nextErrorRetryTime <= VPLTime_GetTimeStamp()))
            {
                nextErrorRetryTime = VPLTime_GetTimeStamp();
                deferredUploadThreadPerformError = true;
            }
        }

        while(!stop_thread) {
            int rc;
            int nextJobRc = 0;
            SCRow_deferredUploadJobSet currDeferredUlEntry;
            // afterRowId supports going on to the next request before the last
            // request is complete.  Required for parallel operations.  Only
            // set when a "Task" is spawned.
            u64 afterRowId = 0;
            u64 errorModeMaxRowId = 0;  // Only valid when isErrorMode
            std::vector<DeferredUploadFileTask*> enqueuedDeferredUlTasks;
            std::vector<DeferredUploadFileResult*> enqueuedDeferredUlResults;
            bool isErrorMode = false;
            ASSERT(!VPLMutex_LockedSelf(&mutex));
            {
                MutexAutoLock lock(&mutex);
                if (deferredUploadThreadPerformUpload) {
                    deferredUploadThreadPerformUpload = false;
                } else if (deferredUploadThreadPerformError) {
                    isErrorMode = true;
                    deferredUploadThreadPerformError = false;
                    nextErrorRetryTime = VPLTIME_INVALID;
                } else {
                    goto end_function;
                }
            }

            if (isErrorMode) {
                rc = deferredUploadDb.deferredUploadJobSet_getMaxRowId(errorModeMaxRowId);
                if (rc != 0) {
                    LOG_CRITICAL("%p: Should never happen:deferredUploadJobSet_getMaxRowId,%d", this, rc);
                    HANDLE_DB_FAILURE();
                    return;
                }
            }
            // For each entry in DeferredUploadChangeLog:
            while ((nextJobRc = getNextDeferredUploadJobSet(isErrorMode,
                                                            afterRowId,
                                                            errorModeMaxRowId,
                                                            /*out*/ currDeferredUlEntry)) == 0)
            {
                bool taskEnqueueAttempt = false;  // true when deferred upload task
                                                  // was spawned or attempted to
                                                  // be spawned.
                bool skip_ErrInc = false;  // When true, appends to error.
                if (!isValidDirEntry(currDeferredUlEntry.name)) {
                    FAILED_ASSERT("Bad entry added to deferred upload job set: \"%s\"",
                                  currDeferredUlEntry.name.c_str());
                    goto skip;
                }
                if(!useDeferredUploadThreadPool())
                {   // Not using thread pool, allowed to just block
                    if (checkForPauseStopWithinDeferredUploadLoop()) {
                        goto end_function;
                    }
                }
                // Case Add File or Update File:
                {
                    // Get the current compId, revision, and hash for the file from the localDB.
                    u64 logOnlyCurrUploadMtime = 0;

                    AbsPath currEntryAbsPath = getAbsPath(getRelPath(currDeferredUlEntry));
                    VPLTime_t fileModifyTime;
                    VPLTime_t fileCreateTime;
                    VPLFS_file_size_t fileSize;
                    // FAT32 correctness case: If timestamp on FS is less than
                    // 2 seconds old, skip (proceed to next entry in ChangeLog).
                    // (It is safe to skip, because we expect to get a file monitor
                    // notification to check this file again in 2 seconds.)
                    // (NOTE: If the local FS timestamp matches the localDB
                    // timestamp, it is *not* actually safe to skip this file
                    // (See CaseF, workaround #2)).
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
                            goto skip;
                        }
                        fileCreateTime = fsTimeToNormLocalTime(currEntryAbsPath, statBuf.vpl_ctime);
                        fileModifyTime = fsTimeToNormLocalTime(currEntryAbsPath, statBuf.vpl_mtime);
                        fileSize = statBuf.size;
                    }

                    DeferredUploadFileTask* taskCtx = new DeferredUploadFileTask();
                    taskCtx->deferredUploadEntry = currDeferredUlEntry;
                    taskCtx->thisPtr = this;

                    taskCtx->fileModifyTime = fileModifyTime;
                    taskCtx->fileCreateTime = fileCreateTime;
                    taskCtx->fileSize = fileSize;
                    taskCtx->currUploadEntryHasCompId = currDeferredUlEntry.last_synced_comp_id_exists;
                    taskCtx->currUploadEntryCompId = currDeferredUlEntry.last_synced_comp_id;
                    if (currDeferredUlEntry.last_synced_revision_id_exists) {
                        taskCtx->currUploadEntryRevisionToUpload = currDeferredUlEntry.last_synced_revision_id;
                    } else {
                        taskCtx->currUploadEntryRevisionToUpload = 1;
                    }
                    taskCtx->logOnlyCurrUploadMtime = logOnlyCurrUploadMtime;

                    taskCtx->mutex = &mutex;
                    taskCtx->enqueuedDeferredUlTasks = &enqueuedDeferredUlTasks;
                    taskCtx->enqueuedDeferredUlResults = &enqueuedDeferredUlResults;

                    taskEnqueueAttempt = true;
                    {   // Enqueue Task.
                        MutexAutoLock lock(&mutex);
                        calcDeferredUploadLoopStatus(deferredUploadDb);
                        enqueuedDeferredUlTasks.push_back(taskCtx);
                        // Since threadPoolNotifier can only reliably wake one thread, only the
                        // deferred upload thread or the primary worker thread can block on
                        // threadPoolNotifier.
                        if(useDeferredUploadThreadPool()) {
                            if(hasDedicatedThread) {
                                rc = threadPool->AddTaskDedicatedThread(&threadPoolNotifier,
                                                                        PerformDeferredUploadTask,
                                                                        taskCtx,
                                                                        (u64)this);
                            } else {
                                rc = threadPool->AddTask(PerformDeferredUploadTask,
                                                         taskCtx,
                                                         (u64)this);
                            }
                        } else {
                            // No threadPool!  Do the work on this thread.
                            PerformDeferredUploadTask(taskCtx);
                            rc = 0;
                        }

                        if(rc == 0) {
                            afterRowId = currDeferredUlEntry.row_id;
                            LOG_INFO("%p: task(%p): Created task uploadEntryRow("FMTu64")",
                                     this, taskCtx, afterRowId);
                        } else {
                            // Task enqueue attempt failed, but that's OK.  ThreadPool
                            // can be normally completely occupied.
                            enqueuedDeferredUlTasks.pop_back();
                            delete taskCtx;
                        }
                    }
                }
              skip:
                if(!taskEnqueueAttempt) {
                    bool handleResultEndFunction = false;
                    handleJobSetResult(skip_ErrInc,
                                       currDeferredUlEntry,
                                       /*OUT*/    handleResultEndFunction,
                                       /*IN,OUT*/ nextErrorRetryTime);
                    if(handleResultEndFunction) {
                        LOG_ERROR("%p: Handle result end function", this);
                        goto end_function;
                    }
                }

                /////////////////////////////////////////////////////////
                /////////////// Process any task results ////////////////
                // http://wiki.ctbg.acer.com/wiki/index.php/User_talk:Rlee/CCD_Sync_One_Way_Speedup
                {
                    bool hasConcurrentJobAfterRowId = hasNextConcurrentDeferredUploadJobSetAfterRowId(
                                                            isErrorMode,
                                                            afterRowId,
                                                            errorModeMaxRowId);
                    bool threadPoolShutdown = false;
                    bool findFreeThread = hasFreeThread(/*OUT*/threadPoolShutdown);
                    MutexAutoLock lock(&mutex);

                    do
                    {

                        while(enqueuedDeferredUlResults.size() == 0  &&  // No results we can immediately process
                              !threadPoolShutdown &&
                              !stop_thread &&
                              useDeferredUploadThreadPool() &&
                              (
                                // Has further work that needs to be done, but need
                                // to wait for capacity
                                (hasConcurrentJobAfterRowId &&
                                 !findFreeThread) ||

                                // Paused (and no results to process)
                                !allowed_to_run ||

                                // Has outstanding tasks that need to complete, but
                                // need to wait for completion.
                                (enqueuedDeferredUlTasks.size() > 0 &&
                                 !hasConcurrentJobAfterRowId)
                              )
                             )
                        {
                            LOG_DEBUG("%p: About to wait: res:"FMTu_size_t", shut:%d, "
                                      "ptr:%p, has:%d, free:%d, task:"FMTu_size_t,
                                      this, enqueuedDeferredUlResults.size(),
                                      threadPoolShutdown, threadPool,
                                      hasConcurrentJobAfterRowId,
                                      findFreeThread, enqueuedDeferredUlTasks.size());

                            if (!allowed_to_run && enqueuedDeferredUlTasks.size()==0) {
                                deferred_upload_loop_paused_or_stopped = true;
                                // If the pause request was blocking, signal the waiting thread now.
                                if (deferred_upload_loop_wait_for_pause_sem != NULL) {
                                    int rc = VPLSem_Post(deferred_upload_loop_wait_for_pause_sem);
                                    if (rc != 0) {
                                        LOG_ERROR("%p: VPLSem_Post(%p):%d",
                                                  this, deferred_upload_loop_wait_for_pause_sem, rc);
                                    }
                                }
                            }

                            lock.UnlockNow();

                            ASSERT(!VPLMutex_LockedSelf(&mutex));  // still may be locked (recursive)
                            rc = VPLSem_Wait(&threadPoolNotifier);
                            LOG_DEBUG("%p: Done waiting", this);

                            lock.Relock(&mutex);
                            hasConcurrentJobAfterRowId = hasNextConcurrentDeferredUploadJobSetAfterRowId(
                                                                isErrorMode,
                                                                afterRowId,
                                                                errorModeMaxRowId);
                            findFreeThread = hasFreeThread(/*OUT*/threadPoolShutdown);
                        }

                        // Handle results whether threadpool is NULL or not
                        while(enqueuedDeferredUlResults.size() > 0)
                        {
                            bool handleResultEndFunction = false;
                            DeferredUploadFileResult* result = enqueuedDeferredUlResults.back();
                            lock.UnlockNow();

                            handleDeferredUlTaskResult(result,
                                                       /*OUT*/    handleResultEndFunction,
                                                       /*IN,OUT*/ nextErrorRetryTime);
                            LOG_INFO("%p: Processed result(%p), rowId:"FMTu64",afterRowId:"FMTu64,
                                     this, result, result->deferredUploadEntry.row_id, afterRowId);
                            lock.Relock(&mutex);
                            bool endFunction = result->endFunction || handleResultEndFunction;
                            removeEnqueuedDeferredUlResult(
                                    result->deferredUploadEntry.row_id,
                                    enqueuedDeferredUlResults);
                            if(endFunction) {
                                // One of the tasks resulted in an end function.
                                goto end_function;
                            }
                        }
                        // If Pause is enabled and using threadpool, keep in this loop.
                        // Otherwise checkForPauseStop will be called at the beginning
                        //  of the next round.
                    }  while(!allowed_to_run && useDeferredUploadThreadPool() && !stop_thread);

                    if (allowed_to_run) {
                        worker_loop_paused_or_stopped = false;
                    }

                    if (stop_thread) {
                        goto end_function;
                    }
                }  // End Scope of MutexAutoLock lock(&mutex);
                /////////////// Process any task results ////////////////
                /////////////////////////////////////////////////////////
            } // while ((nextJobRc = uploadChangeLog_get(currEntry)) == 0)
            if (nextJobRc != SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) {
                LOG_CRITICAL("%p: uploadChangeLog_get() failed: %d", this, nextJobRc);
                HANDLE_DB_FAILURE();
            }
     end_function:
            {
                MutexAutoLock lock(&mutex);
                while(enqueuedDeferredUlTasks.size() > 0 || enqueuedDeferredUlResults.size() > 0)
                {
                    if(enqueuedDeferredUlResults.size()==0) {
                        LOG_INFO("%p: Waiting for phase ("FMTu_size_t" tasks need completion, "FMTu_size_t" to process)",
                                 this, enqueuedDeferredUlTasks.size(), enqueuedDeferredUlResults.size());
                        lock.UnlockNow();
                        ASSERT(!VPLMutex_LockedSelf(&mutex));  // still may be locked (recursive)
                        VPLSem_Wait(&threadPoolNotifier);
                        lock.Relock(&mutex);
                    }
                    while(enqueuedDeferredUlResults.size() > 0)
                    {
                        bool unused_endFunction; // already in end_function section.
                        DeferredUploadFileResult* result = enqueuedDeferredUlResults.back();
                        lock.UnlockNow();

                        handleDeferredUlTaskResult(result,
                                                   /*OUT*/    unused_endFunction,
                                                   /*IN,OUT*/ nextErrorRetryTime);
                        LOG_INFO("%p: Processed result(%p), rowId:"FMTu64", endNow:%d",
                                 this, result,
                                 result->deferredUploadEntry.row_id, unused_endFunction);

                        lock.Relock(&mutex);
                        removeEnqueuedDeferredUlResult(result->deferredUploadEntry.row_id,
                                                        enqueuedDeferredUlResults);

                    }
                }
            }

            MutexAutoLock lock(&mutex);

            if (type == SYNC_TYPE_ONE_WAY_UPLOAD &&
                enqueuedDeferredUlTasks.size() == 0 &&
                enqueuedDeferredUlResults.size() == 0)
            {   // For MIGRATE_TO_NORMAL, checking if complete
                calcDeferredUploadLoopStatus(deferredUploadDb);
                if (!deferUploadLoopStatusHasWorkToDo &&
                    !deferUploadLoopStatusHasError)
                {   // Migration complete, clear migrate from.
                    rc = deferredUploadDb.admin_set_sync_type(type, false, 0);
                    if (rc != 0) {
                        LOG_ERROR("%p:admin_set_sync_type(%d):%d", this, (int)type, rc);
                    }

                    // DeferredUpload resources not needed after migration.
                    // Go ahead and free deferredUpload resources early.
                    ASSERT(VPLMutex_LockedSelf(&mutex));
                    if (deferredUploadThreadInit) {
                        // Clean up deferred thread, not needed in normal OneWayUp
                        rc = deferredUploadDb.closeDb();
                        if (rc != 0) {
                            LOG_ERROR("%p: close deferredUploadDb failed:%d", this, rc);
                        } else {
                            deferredUploadDbInit = false;
                        }

                        // Clean up deferred thread, not needed in normal OneWayUp.
                        rc = VPLDetachableThread_Detach(&deferredUploadThread);
                        if(rc != 0) {
                            LOG_ERROR("%p: VPLDetachableThread_Detach:%d.  Exiting anyways.",
                                      this, rc);
                        }
                        LOG_INFO("%p:migration to normal upload done. "
                                 "Exit deferred upload thread.", this);
                        deferredUploadThreadInit = false;
                        deferred_upload_loop_paused_or_stopped = true;
                        return;
                    }
                }
            }

            // Wait for work to do.
            while ( !(deferredUploadThreadPerformUpload ||
                      deferredUploadThreadPerformError))
            {   // This will also check error timers
                ASSERT(VPLMutex_LockedSelf(&mutex));
                if (checkForPauseStopWithinDeferredUploadLoop()) {
                    return;
                }

                VPLTime_t timeout = getDeferredUploadTimeoutUntilNextRetryErrors(nextErrorRetryTime);
                int rc = VPLCond_TimedWait(&work_to_do_cond_var, &mutex, timeout);
                if ((rc < 0) && (rc != VPL_ERR_TIMEOUT)) {
                    LOG_WARN("%p: VPLCond_TimedWait failed: %d", this, rc);
                }
                if (checkForPauseStopWithinDeferredUploadLoop()) {
                    return;
                }

                // Call to calcDeferredUploadLoopStatus not strictly necessary,
                // but is here to refresh the variable deferUploadLoopStatusHasError
                // from the DB.  Currently the only way to insert an error in the
                // DB is from this thread, so the state should match.  This is only a
                // safety net.
                calcDeferredUploadLoopStatus(deferredUploadDb);  // refreshes deferUploadLoopStatusHasError

                if (deferUploadLoopStatusHasError &&
                    (nextErrorRetryTime == VPLTIME_INVALID ||
                     nextErrorRetryTime <= VPLTime_GetTimeStamp()))
                {
                    // Wait a minimum of sync_policy.error_retry_interval for next retry
                    // If retrying the errors cause more errors, then the
                    // nextErrorRetryTime should be set appropriately.
                    computeDeferredUploadErrorTimeout(sync_policy.error_retry_interval, /*IN,OUT*/ nextErrorRetryTime);
                    deferredUploadThreadPerformError = true;
                }
            }
        }
        return;
    }

    //========================== DEFERRED UPLOADS =============================
    //=========================================================================

    void handleSyncTypeUpMigration()
    {
        int rc;
        SCRow_admin adminRow;
        rc = localDb.admin_get(adminRow);
        if (rc == SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) {
            LOG_CRITICAL("%p:admin row does not exist", this);
            return;
        } else if (rc != 0) {
            LOG_ERROR("%p:admin_get:%d", this, rc);
            return;
        }

        // Upload migration cases:
        // #) migrateFrom -> currType
        // 1) Normal -> pure virtual    // nothing needs to be done.
        // 2) Normal -> hybrid          // nothing needs to be done.
        // 3) pure virtual -> normal    // deferred uploads need to be complete.
        // 4) pure virtual -> hybrid    // nothing needs to be done.
        // 5) hybrid -> pure virtual    // nothing needs to be done.
        // 6) hybrid -> normal          // deferred uploads need to be complete.
        // .
        // Note that when migrating to normal upload, it is possible that there's nothing in
        // the deferred upload queue.  But for simplicity, we will launch the deferred upload
        // thread anyway.  The deferred upload thread will detect when the queue is empty, then
        // clear the migrate_from_sync_type field in the DB and shut itself down.
        if (adminRow.migrate_from_sync_type_exists)
        {
            if (type == SYNC_TYPE_ONE_WAY_UPLOAD_PURE_VIRTUAL_SYNC ||     // 1), 5)
                (type == SYNC_TYPE_ONE_WAY_UPLOAD_HYBRID_VIRTUAL_SYNC))    // 2), 4)
            {   // Handle cases where nothing needs to be done, just remove migrate_from field
                rc = localDb.admin_set_sync_type(type,
                                                 false, 0);
                if (rc != 0) {
                    LOG_ERROR("%p:admin_set_sync_type(%d):%d", this, (int)type, rc);
                }
            }
            // else see comment label MIGRATE_TO_NORMAL.
        }
    }

    void workerLoop()
    {
        handleSyncTypeUpMigration();

        u64 rootCompId = 0;
        MutexAutoLock lock(&mutex);

        // Loop.
        while (1) {
            if (checkForPauseStop()) {
                return;
            }
            if (isTimeToRetryErrors()) {
                LOG_INFO("%p: Retrying errors", this);
                // Do errors
                lock.UnlockNow();
                ApplyUploadChangeLog(true);
                lock.Relock(&mutex);
            }

            // TODO: temporarily always doing a full local scan
            if (incrementalLocalScanPaths.size() > 0) {
                full_local_scan_requested = true;
            }

            if (full_local_scan_requested || uploadScanError || initial_scan_requested_OneWay) {
                full_local_scan_requested = false;
                uploadScanError = false;
                incrementalLocalScanPaths.clear();
                bool initialScan = initial_scan_requested_OneWay;
                initial_scan_requested_OneWay = false;
                updateStatusDueToScanRequest();
                lock.UnlockNow();

                // TODO: Big problem when rootCompId changes on VCS
                //       https://bugs.ctbg.acer.com/show_bug.cgi?id=10255
                if (rootCompId == 0) {
                    // Creating root directory on VCS here.  Most likely place
                    // to discover network not connected.
                    int rc;
                    // Empty uploadChangeLog represents root directory
                    SCRow_uploadChangeLog root;
                    rc = getParentCompId(root, /*out*/ rootCompId);
                    if (isTransientError(rc)) {
                        MutexAutoLock scopelock(&mutex);
                        setErrorTimeout(sync_policy.error_retry_interval);
                        uploadScanError = true;
                        if (initialScan) {
                            initial_scan_requested_OneWay = true;
                        }
                    } else if(rc != 0) {
                        LOG_ERROR("%p: Cannot perform upload sync; root cannot be created:%d",
                                this, rc);
                        setErrorNeedsRemoteScan();
                    } else {
                        LOG_INFO("%p: Root created on VCS:%s,"FMTu64,
                                 this, server_dir.c_str(), rootCompId);
                    }
                }
                if (rootCompId != 0) {
                    PerformFullLocalScan(initialScan);
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
            if (rootCompId != 0) {
                ApplyUploadChangeLog(false);
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

            // Wait for work to do.
            while (!checkForWorkToDo()) {    // This will also check error timers
                ASSERT(VPLMutex_LockedSelf(&mutex));
                if (checkForPauseStop()) {
                    return;
                }
                VPLTime_t timeout = getTimeoutUntilNextRetryErrors();
                int rc = VPLCond_TimedWait(&work_to_do_cond_var, &mutex, timeout);
                if ((rc < 0) && (rc != VPL_ERR_TIMEOUT)) {
                    LOG_WARN("%p: VPLCond_TimedWait failed: %d", this, rc);
                }
                if (checkForPauseStop()) {
                    return;
                }
            }
        } // while (1)
    }
};

