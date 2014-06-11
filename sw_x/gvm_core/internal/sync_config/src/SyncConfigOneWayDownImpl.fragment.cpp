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

class SyncConfigOneWayDownImpl : public SyncConfigImpl
{
private:
    SyncConfigOneWayDownImpl(
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

    void setErrorNeedsRemoteScan()
    {
        MutexAutoLock lock(&mutex);
        downloadScanError = true;
        setErrorTimeout(sync_policy.error_retry_interval);
    }

    int deleteLocalEntry(const SCRow_syncHistoryTree& toDelete)
    {
        int rc;
        bool entryRemoved = false;
        SCRelPath scRelPath = getRelPath(toDelete);
        AbsPath toStat = getAbsPath(scRelPath);
        VPLTime_t fileModifyTime = 0;

        ASSERT(toDelete.comp_id_exists);

        VPLFS_stat_t statBuf;
        if (allowLocalFSUpdate()) {
            rc = VPLFS_Stat(toStat.c_str(), &statBuf);
            if(rc == VPL_ERR_NOENT) {
                // already missing
                entryRemoved = true;
                LOG_INFO("%p: ACTION DOWN Delete %s (already gone from local FS), mtime("FMTu64"),compId("FMTu64"),rev("FMTu64")",
                        this, toStat.c_str(),
                        toDelete.local_mtime,
                        toDelete.comp_id,
                        toDelete.revision_exists?toDelete.revision:0);
                goto remove_entry;
            }else if(rc != 0) {
                // Do not delete when anything doesn't work out.
                // Drop the deletion task for now.  We will retry next time we have a reason to scan
                // the parent directory again on VCS (which might never happen).
                // TODO: Bug 11588: Instead of dropping, we might want to add this to the error
                //   queue.  If there really is a permanent error, it would cause us to retry
                //   every 15 minutes.  However, I don't think it would result in any server calls
                //   in this case.
                LOG_ERROR("%p: ACTION DOWN Delete %s VPLFS_Stat failed: %d, mtime("FMTu64"),compId("FMTu64"),rev("FMTu64")",
                        this, toStat.c_str(), rc,
                        toDelete.local_mtime,
                        toDelete.comp_id,
                        toDelete.revision_exists?toDelete.revision:0);
                return rc;
            }
            fileModifyTime = fsTimeToNormLocalTime(toStat, statBuf.vpl_mtime);
        }

        if (!allowLocalFSUpdate()) {
            entryRemoved = true;
        } else if (statBuf.type == VPLFS_TYPE_FILE)
        {
            if(!toDelete.revision_exists || !toDelete.local_mtime_exists) {
                // Bug 10646 Comment 11:  This case can be plausibly hit when
                // 1) Scan_1 happens
                // 2) download does not happen (File removed from vcs, hard restart, etc)
                // 3) File is removed from VCS and Scan_2 happens.
                // Technically, instead of queueing up a DOWNLOAD_ACTION_DELETE, we could have
                // immediately removed the entry from the syncHistoryTree, but it seems more
                // robust to have the final check here.
                LOG_INFO("%p: Pre-existing file found -- never downloaded:"
                         "(%s) compId("FMTu64"),mtime(%d,"FMTu64"),rev(%d,"FMTu64")",
                         this,
                         toStat.c_str(),
                         toDelete.comp_id,
                         toDelete.local_mtime_exists?1:0, toDelete.local_mtime,
                         toDelete.revision_exists?1:0, toDelete.revision);
            }
            LOG_INFO("%p: ACTION DOWN Delete File %s mtime("FMTu64","FMTu64"),compId("FMTu64"),rev("FMTu64")",
                    this, toStat.c_str(),
                    toDelete.local_mtime, fileModifyTime,
                    toDelete.comp_id,
                    toDelete.revision_exists?toDelete.revision:0);
            rc = VPLFile_Delete(toStat.c_str());
            if(rc == VPL_ERR_NOENT) {
                entryRemoved = true;
            }else if(rc == 0) {
                entryRemoved = true;
            }else{
                LOG_WARN("%p: VPLFile_Delete, skipping:%d, %s",
                        this, rc, toStat.c_str());
                // Drop the deletion task for now.  We will retry next time we have a reason to scan
                // the parent directory again on VCS (which might never happen).
                // TODO: Bug 11588: Instead of dropping, we might want to add this to the error
                //   queue.  If there really is a permanent error, it would cause us to retry
                //   every 15 minutes.  However, I don't think it would result in any server calls
                //   in this case.
                return rc;
            }
        }else if (statBuf.type == VPLFS_TYPE_DIR){
            LOG_INFO("%p: ACTION DOWN Delete dir %s", this, toStat.c_str());
            rc = Util_rmRecursive(toStat.str(), getTempDeleteDir().str());
            if(rc != 0) {
                LOG_ERROR("%p: Util_rmRecursive (%s,%s):%d", this,
                          toStat.c_str(), getTempDeleteDir().c_str(), rc);
                // Drop the deletion task for now.  We will retry next time we have a reason to scan
                // the parent directory again on VCS (which might never happen).
                // TODO: Bug 11588: Instead of dropping, we might want to add this to the error
                //   queue.  If there really is a permanent error, it would cause us to retry
                //   every 15 minutes.  However, I don't think it would result in any server calls
                //   in this case.
                return rc;
            }else{
                entryRemoved = true;
            }
        }

      remove_entry:
        if(entryRemoved) {
            rc = localDb.syncHistoryTree_remove(toDelete.parent_path, toDelete.name);
            if(rc != 0) {
                LOG_CRITICAL("%p: syncHistoryTree_remove (%s,%s):%d",
                        this,
                        toDelete.parent_path.c_str(),
                        toDelete.name.c_str(),
                        rc);
                HANDLE_DB_FAILURE();
                return rc;
            }
            if (allowLocalFSUpdate() && statBuf.type == VPLFS_TYPE_FILE) {
                reportFileDownloaded(true, scRelPath, false, false);
            }
        }
        return 0;
    }

    /// Removes any entries in \a directories that are an ancestor of \a path.
    void removeAllParentPaths(const SCRelPath& path,
                              std::vector<SCRow_syncHistoryTree>& directories)
    {
        string pathStr = path.str();
        std::vector<SCRow_syncHistoryTree>::iterator dirIt = directories.begin();

        while (dirIt != directories.end())
        {
            SCRow_syncHistoryTree& currDir = *dirIt;
            std::string dirPath = getRelPath(currDir).str();
            dirPath.append("/");
            if (pathStr.find(dirPath) == 0) {
                // match found, removing the directory from being removed
                // (the removal would result in an error anyways)
                dirIt = directories.erase(dirIt);
            } else {
                ++dirIt;
            }
        }
    }

    /// Recursively delete all known entities from the given directory.
    void deleteLocalDir(const SCRow_syncHistoryTree& directory)
    {
        int rc;
        ASSERT(directory.is_dir);
        ASSERT(directory.name.size() > 0); // We should never try to delete the sync config root.
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
                        rc = deleteLocalEntry(currDirEntry);
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

                        // Bug 16923: Remove file currDirEntry from the downloadChangeLog if needed.
                        // currDirEntry should only be in the change log if there is a pending
                        // download error for it.
                        {
                            int rc = localDb.downloadChangeLog_remove(currDirEntry.parent_path, currDirEntry.name);
                            if (rc == 0) {
                                LOG_INFO("%p: Removed obsolete downloadChangeLog(%s,%s)",
                                         this, currDirEntry.parent_path.c_str(), currDirEntry.name.c_str());
                            } else if (rc == SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) {
                                // Expected, this is the normal case.
                            } else {
                                LOG_ERROR("%p:downloadChangeLog_remove(%s,%s):%d",
                                          this, currDirEntry.parent_path.c_str(),
                                          currDirEntry.name.c_str(), rc);
                                // This is not expected, but we can continue.
                            }
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

            // ONE_WAY_DOWN, we can delete this directory whether it's a file
            // or directory
            rc = deleteLocalEntry(dirToDelete);
            if(rc != 0) {
                LOG_WARN("%p: Could not remove %s:%d, not removing parents",
                        this, dirToRmPath.c_str(), rc);
                // No need to attempt to delete ancestor directories with
                // elements still present here.
                removeAllParentPaths(dirToRmPath, traversedDirsStack);
                // Do not remove from the syncHistoryTree for this entry,
                // Skip to next entry.
                continue;
            }
        }  // while(!traversedDirsStack.empty())
    }

    void vcsFileToSyncHistoryTreeEntry(const VcsFile& vcsFile,
                                       const SCRelPath& currDirectory,
                                       u64 currentDirVersion,
                                       VPLTime_t localFsMtime,
                                       SCRow_syncHistoryTree& syncHistoryEntry_out)
    {
        AbsPath currEntryAbsPath = getAbsPath(currDirectory);

        syncHistoryEntry_out.parent_path = currDirectory.str();
        syncHistoryEntry_out.name = vcsFile.name;
        syncHistoryEntry_out.is_dir = false;
        syncHistoryEntry_out.comp_id_exists = true;
        syncHistoryEntry_out.comp_id = vcsFile.compId;
        syncHistoryEntry_out.revision_exists = true;
        syncHistoryEntry_out.revision = vcsFile.latestRevision.revision;
        syncHistoryEntry_out.local_mtime_exists = true;
        syncHistoryEntry_out.local_mtime = fsTimeToNormLocalTime(currEntryAbsPath, localFsMtime);
        syncHistoryEntry_out.last_seen_in_version_exists = true;
        syncHistoryEntry_out.last_seen_in_version = currentDirVersion;
        syncHistoryEntry_out.is_on_acs = !vcsFile.latestRevision.noAcs;
    }

    /// Populates the DownloadChangeLog by querying VCS.
    void ScanRemoteChanges()
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
                    if (setHttpHandleAndCheckForStop(httpHandle)) { goto end_function; }
                    rc = vcs_read_folder_paged(vcs_session,
                                               httpHandle,
                                               dataset,
                                               currScanDirDatasetRelPath.str(),
                                               currScanDir.comp_id,
                                               (filesSeen + 1), // VCS starts at 1 instead of 0 for some reason
                                               VCS_GET_DIR_PAGE_SIZE,
                                               verbose_http_log,
                                               getDirResp);
                    if (clearHttpHandleAndCheckForPauseStop(httpHandle)) { goto end_function; }
                    if (!isTransientError(rc)) { break; }
                    LOG_WARN("%p: Transient Error:%d RetryIndex:%d", this, rc, retries);
                    if (retries+1 < NUM_IMMEDIATE_TRANSIENT_RETRY) {  // pause between immediate retries
                        if (checkForPauseStop(true, QUICK_RETRY_INTERVAL)) {
                            goto end_function;
                        }
                    }
                }
                if (rc < 0) {
                    if (isTransientError(rc)) {
                        LOG_WARN("%p: Transient error: vcs_read_folder(%s) failed: %d",
                                this, currScanDirDatasetRelPath.c_str(), rc);
                        // Try the VCS scan again later.
                        // We will leave this directory in needDownloadScan, so we will be sure to
                        // try it again next time.
                        errorOccurred = true;
                        goto skip_dir;
                    } else if (rc == VCS_ERR_PROVIDED_PATH_DOESNT_MATCH_PROVIDED_COMPID ||
                               rc == VCS_ERR_PATH_DOESNT_POINT_TO_KNOWN_COMPONENT ||
                               rc == VCS_ERR_LATEST_REVISION_NOT_FOUND)
                    {
                        LOG_WARN("%p: Problem reading vcs path(%d):\"%s\", will delete all entries within",
                                 this, rc, currScanDirDatasetRelPath.c_str());
                        // Code will ultimately perform deletion (towards the end of this function).

                        // TODO: Bug 12976: Alternatively, it might be safer to skip this directory.
                        // If we got into this state, we expect a notification to scan the parent
                        // directory again soon.

#if 0 // TODO: bug 10255: needs to be tested better.
                        // Special case; we won't recover without this.
                        if (currScanDirRelPath.isSCRoot() && (rc == VCS_ERR_PROVIDED_PATH_DOESNT_MATCH_PROVIDED_COMPID)) {
                            // SyncConfig root dir has been removed (or recreated).  Must fix our local sync history tree to recover.
                            LOG_WARN("%p: Removing compId for sync config root (%s)", this, currScanDirDatasetRelPath.c_str());
                            localDb.syncHistoryTree_updateCompId("", "", false, 0, false, 0);
                        }
#endif
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
                        // In case we are recovering from a migration or lost local DB, check
                        // the local FS to see if we already have the proper file:
                        SCRelPath currEntryDirPath = currScanDirRelPath.getChild(it->name);
                        AbsPath currEntryAbsPath = getAbsPath(currEntryDirPath);
                        VPLTime_t localFsMtime = 0; // dummy init
                        if (checkIfLocalFileMatches(currEntryAbsPath, *it, /*OUT*/ localFsMtime)) {
                            LOG_INFO("%p: Skipping download for \"%s\". local file(time:"FMTu64") matches VCS.",
                                     this, currEntryDirPath.c_str(), localFsMtime);
                            // Create file/dir record in LocalDB.
                            SCRow_syncHistoryTree newEntry;
                            vcsFileToSyncHistoryTreeEntry(*it,
                                                          currScanDirRelPath,
                                                          getDirResp.currentDirVersion,
                                                          localFsMtime,
                                                          /*OUT*/ newEntry);
                            BEGIN_TRANSACTION();
                            rc = localDb.syncHistoryTree_add(newEntry);
                            if (rc < 0) {
                                LOG_CRITICAL("%p: syncHistoryTree_add(%s) failed: %d",
                                        this, currEntryDirPath.c_str(), rc);
                                HANDLE_DB_FAILURE();
                                goto skip_dir;
                            }
                            CHECK_END_TRANSACTION(false);
                            continue;
                        }
                        // Create file/dir record in LocalDB.
                        // vcs_comp_id = compId, vcs_revision = -1 since it doesn't exist on the local FS.
                        SCRow_syncHistoryTree newEntry;
                        newEntry.parent_path = currScanDirRelPath.str();
                        newEntry.name = it->name;
                        newEntry.is_dir = false;
                        newEntry.comp_id_exists = true;
                        newEntry.comp_id = it->compId;
                        // newEntry.revision is not populated until download actually occurs.
                        // newEntry.local_mtime is not populated until download actually occurs.
                        newEntry.is_on_acs = !it->latestRevision.noAcs;
                        newEntry.last_seen_in_version_exists = true;
                        newEntry.last_seen_in_version = getDirResp.currentDirVersion;
                        // Note that this transaction is required for correctness; otherwise,
                        // if the entry is added to syncHistoryTree and then we crash before adding
                        // it to downloadChangeLog, the download task would be lost.
                        BEGIN_TRANSACTION();
                        rc = localDb.syncHistoryTree_add(newEntry);
                        if (rc < 0) {
                            LOG_CRITICAL("%p: syncHistoryTree_add(%s, %s) failed: %d",
                                    this, currScanDirRelPath.c_str(), it->name.c_str(), rc);
                            HANDLE_DB_FAILURE();
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
                                std::string(""), false, 0);
                        if (rc < 0) {
                            LOG_CRITICAL("%p: downloadChangeLog_add(%s, %s) failed: %d",
                                    this, currScanDirRelPath.c_str(), it->name.c_str(), rc);
                            HANDLE_DB_FAILURE();
                            // Retrying probably won't help, so not setting errorOccurred.
                            goto skip_dir;
                        }
                        CHECK_END_TRANSACTION(false);
                    } else if (rc < 0) {
                        LOG_CRITICAL("%p: syncHistoryTree_get(%s, %s) failed: %d",
                                  this, currScanDirRelPath.c_str(), it->name.c_str(), rc);
                        HANDLE_DB_FAILURE();
                        // Retrying probably won't help, so not setting errorOccurred.
                        goto skip_dir;
                    } else { // dirEntry present in LocalDB; currDbEntry is valid.
                        // Update LocalDB[dirEntry].last_seen_in_version = currentDirVersion
                        BEGIN_TRANSACTION();
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
                            // Add {dirEntry, compId, revision, client_reported_modify_time} to DownloadChangeLog.
                            LOG_INFO("%p: Decision DownloadFile (%s,%s),compId("FMTu64","FMTu64
                                     "),rev("FMTu64","FMTu64"),mtime("FMTu64"),isOnAcs(%d)",
                                     this, currScanDirRelPath.c_str(), it->name.c_str(),
                                     currDbEntry.comp_id, it->compId,
                                     currDbEntry.revision, it->latestRevision.revision,
                                     vcsTimeToNormLocalTime(it->lastChanged),
                                     !it->latestRevision.noAcs);
                            rc = localDb_downloadChangeLog_add_and_setSyncStatus(
                                    currScanDirRelPath.str(), it->name, false,
                                    DOWNLOAD_ACTION_GET_FILE, it->compId,
                                    it->latestRevision.revision,
                                    vcsTimeToNormLocalTime(it->lastChanged),
                                    !it->latestRevision.noAcs,
                                    std::string(""), false, 0);
                            if (rc < 0) {
                                LOG_CRITICAL("%p: downloadChangeLog_add(%s, %s) failed: %d",
                                        this, currScanDirRelPath.c_str(), it->name.c_str(), rc);
                                HANDLE_DB_FAILURE();
                                // Retrying probably won't help, so not setting errorOccurred.
                                goto skip_dir;
                            }
                        } else if (currDbEntry.is_on_acs != (!it->latestRevision.noAcs))
                        {   // is_on_acs field is not up-to-date.
                            ASSERT(currDbEntry.comp_id == it->compId);
                            ASSERT(currDbEntry.revision == it->latestRevision.revision);
                            // Note: This case only happens if download has already
                            // been successful; thus, there should be no entry in
                            // the download_change_log.  If the download has NOT
                            // been successful, revision will not match, and the logic
                            // would have entered in the block above "Decision DownloadFile"

                            SCRelPath currEntryDirPath = currScanDirRelPath.getChild(it->name);
                            LOG_INFO("%p: Updating is_on_acs for \"%s\" (%d->%d)",
                                     this, currEntryDirPath.c_str(),
                                     currDbEntry.is_on_acs,
                                     !it->latestRevision.noAcs);

                            // Create file/dir record in LocalDB.
                            SCRow_syncHistoryTree newEntry;
                            vcsFileToSyncHistoryTreeEntry(*it,
                                                          currScanDirRelPath,
                                                          getDirResp.currentDirVersion,
                                                          currDbEntry.local_mtime,
                                                          /*OUT*/ newEntry);
                            rc = localDb.syncHistoryTree_add(newEntry);
                            if (rc < 0) {
                                LOG_CRITICAL("%p: syncHistoryTree_add(%s) failed: %d",
                                        this, currEntryDirPath.c_str(), rc);
                                HANDLE_DB_FAILURE();
                                goto skip_dir;
                            }

                            // No need to update download_change_log, there should
                            // be no entry there.
                        } // else { Scanned file already up-to-date }
                        CHECK_END_TRANSACTION(false);
                    }
                } // For each folder dirEntry in fileList.
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
                            // Retrying probably won't help, so not setting errorOccurred.
                            goto skip_dir;
                        }
                        CHECK_END_TRANSACTION(false);

                        SetStatus(SYNC_CONFIG_STATUS_SYNCING);

                        // Create local dir if it doesn't exist.
                        // (Optional, but should improve responsiveness to user.  Also, if we
                        // don't do this here, we will need to add the directory to the download
                        // change log, or empty directories will not be created.)
                        AbsPath currEntryAbsPath = getAbsPath(getRelPath(newEntry));
                        {
                            LOG_INFO("%p: ACTION DOWN mkdir %s compId("FMTu64")",
                                     this, currEntryAbsPath.c_str(), newEntry.comp_id);
                            if (allowLocalFSUpdate()) {
                                rc = Util_CreateDir(currEntryAbsPath.c_str());
                                if (rc < 0) {
                                    LOG_WARN("%p: Util_CreateDir(%s) failed: %d",
                                            this, currEntryAbsPath.c_str(), rc);
                                    // Generally safe to ignore this error; we will try again after
                                    // downloading the first file or subdirectory.
                                    // Possible minor issue: if the directory is empty on VCS, then
                                    // it will not be created.
                                }
                            }
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
                        // Just update LocalDB[dirEntry].last_seen_in_version = currentDirVersion
                        BEGIN_TRANSACTION();
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
                        if (currDbEntry.comp_id != it->compId) {
                            // Update the localDB:
                            rc = localDb.syncHistoryTree_updateCompId(currScanDirRelPath.str(), it->name,
                                    true, it->compId, false, 0);
                            if (rc < 0) {
                                LOG_CRITICAL("%p: syncHistoryTree_updateCompId(%s) failed: %d",
                                          this, currScanDirRelPath.c_str(), rc);
                                HANDLE_DB_FAILURE();
                                // Retrying probably won't help, so not setting errorOccurred.
                                goto skip_dir;
                            }
                        }
                        CHECK_END_TRANSACTION(false);
                        // LocalDB[dirEntry].version_scanned < currentDirVersion),
                        if (currDbEntry.version_scanned < it->version) {
                            // Directory has been updated since we last scanned it, add it to the queue:
                            addToNeedDownloadScan = true;
                            LOG_DEBUG("%p: Previous version_scanned="FMTu64", now="FMTu64"; adding %s to needDownloadScan",
                                    this, currDbEntry.version_scanned, it->version, currEntryDirPath.c_str());
                        }
                    }

                    if (addToNeedDownloadScan) {
                        BEGIN_TRANSACTION();
                        rc = localDb.needDownloadScan_add(currEntryDirPath.str(), it->compId);
                        if (rc < 0) {
                            LOG_CRITICAL("needDownloadScan_add(%s) failed: %d",
                                    currEntryDirPath.c_str(), rc);
                            HANDLE_DB_FAILURE();
                            // Retrying probably won't help, so not setting errorOccurred.
                            goto skip_dir;
                        }
                        CHECK_END_TRANSACTION(false);
                    }
                } // For each directory dirEntry in fileList.

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
                    //      Add it to the download changelog (as a deletion).
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
                        BEGIN_TRANSACTION();
                        // We need to delete the local file/dir.
                        LOG_INFO("%p: Decision DownloadDelete (%s,%s),compId("FMTu64"),rev("FMTu64"),mtime("FMTu64"),isOnAcs(%d)",
                                this, child.parent_path.c_str(), child.name.c_str(),
                                child.comp_id, child.revision, child.local_mtime, child.is_on_acs);
                        rc = localDb_downloadChangeLog_add_and_setSyncStatus(
                                child.parent_path, child.name, child.is_dir,
                                DOWNLOAD_ACTION_DELETE, child.comp_id,
                                child.revision, child.local_mtime, child.is_on_acs,
                                std::string(""), false, 0);
                        if (rc < 0) {
                            LOG_CRITICAL("%p: downloadChangeLog_add(%s, %s) failed: %d",
                                    this, currScanDirRelPath.c_str(), child.name.c_str(), rc);
                            HANDLE_DB_FAILURE();
                            continue;
                        }
                        CHECK_END_TRANSACTION(false);
                    }
                }

                // If this is the first time we ever scanned this directory on VCS, also
                // do a local scan and remove any local files/directories that do not exist in VCS.
                {
                    SCRow_syncHistoryTree currDirScanned;
                    rc = syncHistoryTree_getEntry(currScanDirRelPath,
                                                  /*OUT*/ currDirScanned);
                    if (rc != 0) {
                        // The entry should not be missing
                        // (rc == SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) because we
                        // added it previous into the DB to get to this point.
                        // The impact of this error is that files that should
                        // be deleted locally are not, no need for special handling.
                        LOG_ERROR("%p: syncHistoryTree_getEntry(%s) failed: %d",
                                  this, currScanDirRelPath.c_str(), rc);
                        HANDLE_DB_FAILURE();
                    }
                    else if (currDirScanned.version_scanned_exists) {
                        // "Initial local scan" has already been performed for this directory.
                    }
                    else if (allowLocalFSUpdate())
                    {   // We perform this "initial local scan" whenever a scanned
                        // directory does not have its version_scanned set.
                        VPLFS_dir_t dirStream;
                        AbsPath currAbsLocalDir = getAbsPath(currScanDirRelPath);
                        int rc = VPLFS_Opendir(currAbsLocalDir.c_str(), &dirStream);
                        if (rc < 0) {
                            LOG_ERROR("%p: VPLFS_Opendir(%s) failed: %d",
                                    this, currAbsLocalDir.c_str(), rc);
                            // TODO: Bug 11608 Comment 2: We won't currently ever retry.
                            goto skip_dir;
                        }
                        ON_BLOCK_EXIT(VPLFS_Closedir, &dirStream);

                        // For each dirEntry in currDir:
                        VPLFS_dirent_t dirEntry;
                        while ((rc = VPLFS_Readdir(&dirStream, &dirEntry)) == VPL_OK) {
                            if(!isValidDirEntry(dirEntry.filename)) {
                                continue;
                            }

                            SCRow_syncHistoryTree entry_out;
                            rc = localDb.syncHistoryTree_get(currScanDirRelPath.str(),
                                                             dirEntry.filename,
                                                             entry_out);
                            if(rc == 0 &&
                               ((entry_out.is_dir && (dirEntry.type==VPLFS_TYPE_DIR)) ||
                                (!entry_out.is_dir && (dirEntry.type==VPLFS_TYPE_FILE))))
                            {
                                // Exists as expected,
                                continue;
                            }

                            string toDelete = currAbsLocalDir.str()+
                                              string("/")+
                                              string(dirEntry.filename);
                            LOG_INFO("%p: ACTION DOWN Delete %s", this, toDelete.c_str());
                            rc = Util_rmRecursive(toDelete, getTempDeleteDir().str());
                            if(rc != 0) {
                                LOG_WARN("%p: Util_rmRecursive(%s,%s):%d", this,
                                         toDelete.c_str(),
                                         getTempDeleteDir().c_str(), rc);
                                // TODO: Bug 11608 Comment 2: We won't currently ever retry.
                            }
                        }
                    }
                    // TODO: Bug 11608 Comment 2: Assume the "initial local scan"
                    // never fails (that local delete is always possible).  Of course,
                    // this may not be the case, and if it needs to be handled,
                    // we should introduce another boolean in the localDb to
                    // hold that state.
                }

                // Assign version_scanned = currentDirVersion for currDir.
                // TODO: Bug 11608 Comment 2: Currently, once this happens, we will never do another
                // "initial local scan" for this path, even if it failed.
                // Not setting version_scanned could result in repeated calls
                // to VCS GetDir, an undesirable behavior.
                BEGIN_TRANSACTION();
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
                CHECK_END_TRANSACTION(false);
            }
 skip_dir:
            if(errorOccurred) {
                // It is important to leave this directory in needDownloadScan, so we will be sure
                // to try it again next time.  (Its parent directories should already have their
                // version_scanned fields updated, so having the directory in needDownloadScan is
                // the only thing that guarantees us to scan this directory again later.)
                setErrorNeedsRemoteScan();
                goto end_function;
            } else {
                BEGIN_TRANSACTION();
                // Remove currDir from LocalDB."need_download_scan".
                rc = localDb.needDownloadScan_remove(currScanDir.row_id);
                if (rc < 0) {
                    LOG_CRITICAL("%p: needDownloadScan_remove("FMTu64") failed: %d",
                            this, currScanDir.row_id, rc);
                    HANDLE_DB_FAILURE();
                    // The entry wasn't removed, so we probably don't want to call needDownloadScan_get() again.
                    goto end_function;
                }
                CHECK_END_TRANSACTION(false);
            }
        } // while ((rc = localDb.needDownloadScan_get(currNeedDownloadScan)) == 0)
        if (rc != SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) {
            LOG_CRITICAL("%p: needDownloadScan_get() failed: %d", this, rc);
            HANDLE_DB_FAILURE();
        }
 end_function:
        CHECK_END_TRANSACTION(true);
    }

    enum DownloadChangeLogErrorMode {
        DL_CHANGE_LOG_ERROR_MODE_NONE,
        DL_CHANGE_LOG_ERROR_MODE_DOWNLOAD,
        DL_CHANGE_LOG_ERROR_MODE_COPYBACK
    };

    int getNextDownloadChangeLog(DownloadChangeLogErrorMode errorMode,
                                 u64 afterRowId,
                                 u64 errorModeMaxRowId,
                                 SCRow_downloadChangeLog& entry_out)
    {
        int rc = 0;
        switch(errorMode)
        {
        case DL_CHANGE_LOG_ERROR_MODE_NONE:
            rc = localDb.downloadChangeLog_getAfterRowId(afterRowId, entry_out);
        break;
        case DL_CHANGE_LOG_ERROR_MODE_DOWNLOAD:
            rc = localDb.downloadChangeLog_getErrDownloadAfterRowId(
                                                          afterRowId,
                                                          errorModeMaxRowId,
                                                          entry_out);
        break;
        case DL_CHANGE_LOG_ERROR_MODE_COPYBACK:
            rc = localDb.downloadChangeLog_getErrCopybackAfterRowId(
                                                          afterRowId,
                                                          errorModeMaxRowId,
                                                          entry_out);
        break;
        default:
            FAILED_ASSERT("Unknown enum:%d", (int)errorMode);
            break;
        }
        return rc;
    }

    struct DownloadFileResult
    {
        SCRow_downloadChangeLog currDownloadEntry;  // Identified by row_id (unchanged from DownloadFileTask)

        bool setDownloadSuccess;

        bool skip_ErrIncDownload;
        bool skip_ErrIncCopyback;

        /// Set this in a thread pool worker thread to tell the SyncConfig's primary worker thread
        /// that it should stop processing the changelog.  (The primary worker thread should still
        /// wait for any other tasks that it has already dispatched to finish though.)
        bool endFunction;

        bool updateSyncHistoryTree;
        SCRow_syncHistoryTree syncHistoryEntry;
        VPLTime_t normLocalTimeGet;
        VPLFS_file_size_t fileSize;

        VPLTime_t infoNormLocalTimeThatWasSet;

        DownloadFileResult()
        :  setDownloadSuccess(false),
           skip_ErrIncDownload(false),
           skip_ErrIncCopyback(false),
           endFunction(false),
           updateSyncHistoryTree(false),
           normLocalTimeGet(VPLTIME_INVALID),
           fileSize(0),
           infoNormLocalTimeThatWasSet(VPLTIME_INVALID)
        {}
    };

    struct DownloadFileTask
    {
        SCRow_downloadChangeLog currDownloadEntry;  // Identified by row_id
        SyncConfigOneWayDownImpl* thisPtr;
        DownloadChangeLogErrorMode errorMode;
        VCSArchiveAccess* vcsArchiveAccess;

        VPLMutex_t* mutex;  // Used to protect enqueuedDlTasks and enqueuedDlResults
        std::vector<DownloadFileTask*>* enqueuedDlTasks;
        std::vector<DownloadFileResult*>* enqueuedDlResults;

        DownloadFileTask()
        :  thisPtr(NULL),
           errorMode(DL_CHANGE_LOG_ERROR_MODE_NONE),
           vcsArchiveAccess(NULL),
           mutex(NULL),
           enqueuedDlTasks(NULL),
           enqueuedDlResults(NULL)
        {}
    };

    bool hasNextConcurrentTaskAfterRowId(DownloadChangeLogErrorMode errorMode,
                                         u64 afterRowId,
                                         u64 errorModeMaxRowId) //Only relevant when errorMode != DL_CHANGE_LOG_ERROR_MODE_NONE
    {
        SCRow_downloadChangeLog changeLog;
        int rc = 0;
        switch(errorMode)
        {
        case DL_CHANGE_LOG_ERROR_MODE_NONE:
            rc = localDb.downloadChangeLog_getAfterRowId(afterRowId, changeLog);
        break;
        case DL_CHANGE_LOG_ERROR_MODE_DOWNLOAD:
            rc = localDb.downloadChangeLog_getErrDownloadAfterRowId(
                                                          afterRowId,
                                                          errorModeMaxRowId,
                                                          changeLog);
        break;
        case DL_CHANGE_LOG_ERROR_MODE_COPYBACK:
            rc = localDb.downloadChangeLog_getErrCopybackAfterRowId(
                                                          afterRowId,
                                                          errorModeMaxRowId,
                                                          changeLog);
        break;
        default:
            FAILED_ASSERT("Unknown enum:%d", (int)errorMode);
            rc = -1;
            break;
        }

        if(rc != 0) {
            return false;
        }

        if(changeLog.download_action == DOWNLOAD_ACTION_GET_FILE)
        {
            return true;
        }

        return false;
    }

    static void PerformDownloadTask(void* ctx)
    {
        // (optional optimization 4, to avoid downloading again) If dirEntry is
        //      file as reported by VCS and !LocalDB[dirEntry].is_directory and
        //      LocalDB[dirEntry].hash == file hash reported by VCS,
        // - (optional optimization 4) Files are the same, set local info the
        //      same as the VCS info.
        //      Set LocalDB.vcs_comp_id = compId, Set LocalDB.vcs_revision = revision
        // - (optional optimization 4) Proceed to next entry
        int rc;
        DownloadFileTask* task = (DownloadFileTask*) ctx;
        DownloadFileResult* result = new DownloadFileResult();
        SyncConfigOneWayDownImpl* thisPtr = task->thisPtr;
        SCRelPath currRelPath = getRelPath(task->currDownloadEntry);
        LOG_DEBUG("%p: task(%p): Created DownloadFileResult:%p", thisPtr, task, result);

        // Intermediate temporary path between infra and final dest
        AbsPath tempFilePath = thisPtr->getTempDownloadFilePath(task->currDownloadEntry.comp_id,
                                                                task->currDownloadEntry.revision);
        if (!thisPtr->allowLocalFSUpdate()) {
            // Update the LocalDB with the downloaded file's metadata (timestamp, compId, revision).
            result->updateSyncHistoryTree = true;
            result->normLocalTimeGet = task->currDownloadEntry.client_reported_mtime;
            result->currDownloadEntry = task->currDownloadEntry;
            result->fileSize = 0;
            result->infoNormLocalTimeThatWasSet = task->currDownloadEntry.client_reported_mtime;
            goto done;
        }

        // Download the file from VCS/Storage to a temp file.
        if (!task->currDownloadEntry.download_succeeded)
        {
            DatasetRelPath currDatasetRelPath = thisPtr->getDatasetRelPath(currRelPath);
            LOG_INFO("%p: task(%p->%p): ACTION DOWN download begin:%s "
                     "compId("FMTu64"),rev("FMTu64"),mtime("FMTu64"),server_dir(%s)",
                     thisPtr, task, result,
                     currRelPath.c_str(),
                     task->currDownloadEntry.comp_id,
                     task->currDownloadEntry.revision,
                     task->currDownloadEntry.client_reported_mtime,
                     thisPtr->server_dir.c_str());
            if (task->vcsArchiveAccess != NULL &&
                thisPtr->is_archive_storage_device_available) // It should be okay to access
                                                              // is_archive_storage_device_available
                                                              // without holding the mutex.
            {
                int rc;
                LOG_INFO("%p:ACTION DOWN Archive Storage Attempt(%s)", thisPtr, currDatasetRelPath.c_str());
                for (u32 retries=0; retries < NUM_IMMEDIATE_TRANSIENT_RETRY; ++retries)
                {
                    VCSArchiveOperation* vcsArchiveOperation = task->vcsArchiveAccess->createOperation(/*OUT*/ rc);
                    if(vcsArchiveOperation == NULL) {
                        LOG_WARN("archive createOperation("FMTu64","FMTu64",%s): %d. Continuing.",
                                 task->currDownloadEntry.comp_id,
                                 task->currDownloadEntry.revision,
                                 currDatasetRelPath.c_str(),
                                 rc);
                    } else {
                        thisPtr->registerForAsyncCancel(vcsArchiveOperation);
                        rc = vcsArchiveOperation->GetFile(task->currDownloadEntry.comp_id,
                                                     task->currDownloadEntry.revision,
                                                     currDatasetRelPath.str(),
                                                     tempFilePath.str());
                        thisPtr->unregisterForAsyncCancel(vcsArchiveOperation);
                        task->vcsArchiveAccess->destroyOperation(vcsArchiveOperation);
                    }
                    if (!isTransientError(rc)) { break; }
                    LOG_WARN("%p: task(%p): Transient Error:%d RetryIndex:%d", thisPtr, task, rc, retries);
                    if (retries+1 < NUM_IMMEDIATE_TRANSIENT_RETRY) {  // pause between immediate retries
                        if (thisPtr->checkForPauseStopWithinTask(true, QUICK_RETRY_INTERVAL)) {
                            goto end_function;
                        }
                    }
                }
                if (isTransientError(rc) ||
                    rc == CCD_ERROR_ARCHIVE_DEVICE_OFFLINE)
                {   // offline error not exactly transient because we don't want to retry 3 times,
                    // but still want to retry later
                    LOG_WARN("%p: vcsArchiveOperation("FMTu64","FMTu64",%s):%d",
                              thisPtr, task->currDownloadEntry.comp_id,
                              task->currDownloadEntry.revision,
                              currDatasetRelPath.c_str(),
                              rc);
                    result->skip_ErrIncDownload = true;
                } else if (rc != 0) {
                    if (!task->currDownloadEntry.is_on_acs) {
                        LOG_WARN("%p: Permanent error for vcsArchiveOperation"
                                  "("FMTu64","FMTu64",%s):%d and not on ACS; dropping download.",
                                  thisPtr, task->currDownloadEntry.comp_id,
                                  task->currDownloadEntry.revision,
                                  currDatasetRelPath.c_str(),
                                  rc);
                        goto skip;
                    } else {
                        LOG_WARN("%p: Permanent error for vcsArchiveOperation"
                                "("FMTu64","FMTu64",%s):%d; try download from ACS.",
                                thisPtr, task->currDownloadEntry.comp_id,
                                task->currDownloadEntry.revision,
                                currDatasetRelPath.c_str(),
                                rc);
                    }
                } else {
                    LOG_INFO("%p: Archive Storage GetFile OK ("FMTu64","FMTu64",%s)",
                            thisPtr, task->currDownloadEntry.comp_id,
                            task->currDownloadEntry.revision,
                            currDatasetRelPath.c_str());
                    task->currDownloadEntry.download_succeeded = true;
                    result->setDownloadSuccess = true;  // Mark in DB that download will never be done again.
                }
            }

            if (!task->currDownloadEntry.download_succeeded &&
                task->currDownloadEntry.is_on_acs)
            {   // Try to download via ACS if archive storage download has not
                // done so already.
                // VCS GET accessinfo(path, method=GET, compId, revision) - returns accessUrl.
                VcsAccessInfo accessInfoResp;
                // Since we will try ACS now, reset this if it was already set (due to a failed
                // attempt to download from the Archive Storage Device).
                result->skip_ErrIncDownload = false;
                LOG_INFO("%p:ACTION DOWN ACS Download Attempt(%s) compId("FMTu64"),rev("FMTu64"),mtime("FMTu64"),server_dir(%s)",
                         thisPtr, currDatasetRelPath.c_str(), task->currDownloadEntry.comp_id,
                         task->currDownloadEntry.revision, task->currDownloadEntry.client_reported_mtime,
                         thisPtr->server_dir.c_str());
                // TODO: (optional optimization) To avoid downloading an out-of-date revision, either add
                // a flag to VCS GET accessinfo to return an error if revision is not latest, or add
                // something like VCS GET accessinfoAndMetadata(path, method=GET, compId), which returns
                // accessUrl, revision, hash, client_reported_modify_time (for the latest revision).
                for (u32 retries=0; retries < NUM_IMMEDIATE_TRANSIENT_RETRY; ++retries)
                {
                    VPLHttp2 httpHandle;
                    if (thisPtr->setHttpHandleAndCheckForStop(httpHandle)) { goto end_function; }
                    rc = vcs_access_info_for_file_get(thisPtr->vcs_session,
                                                      httpHandle,
                                                      thisPtr->dataset,
                                                      currDatasetRelPath.str(),
                                                      task->currDownloadEntry.comp_id,
                                                      task->currDownloadEntry.revision,
                                                      thisPtr->verbose_http_log,
                                                      accessInfoResp);
                    if (thisPtr->clearHttpHandleAndCheckForStopWithinTask(httpHandle)) { goto end_function; }
                    if (!isTransientError(rc)) { break; }
                    LOG_WARN("%p: task(%p): Transient Error:%d RetryIndex:%d", thisPtr, task, rc, retries);
                    if (retries+1 < NUM_IMMEDIATE_TRANSIENT_RETRY) {  // pause between immediate retries
                        if (thisPtr->checkForPauseStopWithinTask(true, QUICK_RETRY_INTERVAL)) {
                            goto end_function;
                        }
                    }
                }
                if (rc < 0) {
                    if(rc == VCS_ERR_REVISION_NOT_FOUND ||
                       rc == VCS_ERR_COMPONENT_NOT_FOUND ||
                       rc == VCS_ERR_PROVIDED_PATH_DOESNT_MATCH_PROVIDED_COMPID)
                    {
                        // Something changed on VCS; we expect a notification to do another
                        // remote scan soon.
                        LOG_WARN("%p: task(%p): Attempt to download %s,"FMTu64","FMTu64" but disappeared:%d. Skipping.",
                                 thisPtr, task,
                                 currDatasetRelPath.c_str(),
                                 task->currDownloadEntry.comp_id,
                                 task->currDownloadEntry.revision,
                                 rc);
                        // Drop the download.
                        goto skip;
                    } else if (isTransientError(rc)) {
                        LOG_WARN("%p: task(%p): vcs_access_info_for_file_get(%s, "FMTu64", "FMTu64") returned %d",
                                thisPtr, task,
                                currDatasetRelPath.c_str(),
                                task->currDownloadEntry.comp_id,
                                task->currDownloadEntry.revision,
                                rc);
                        // Transient error; retry later.
                        result->skip_ErrIncDownload = true;
                        goto skip;
                    } else {
                        // Non-retryable; need to do another server scan.
                        // TODO: Bug 12976: Except this won't actually do another server scan because
                        //   "version scanned" will already be updated.  If there actually
                        //   was a change to the parent directory, we should already have a
                        //   dataset_content_change notification to make a new scan request anyway.
                        //   Should we remove this call to setErrorNeedsRemoteScan()?
                        thisPtr->setErrorNeedsRemoteScan();
                        LOG_ERROR("%p: task(%p): vcs_access_info_for_file_get(%s, "FMTu64", "FMTu64") returned %d",
                                  thisPtr, task,
                                  currDatasetRelPath.c_str(),
                                  task->currDownloadEntry.comp_id,
                                  task->currDownloadEntry.revision, rc);
                        goto skip;
                    }
                }

                // Download file from accessUrl to the temp file.
                for (u32 retries=0; retries < NUM_IMMEDIATE_TRANSIENT_RETRY; ++retries)
                {
                    VPLHttp2 httpHandle;
                    if (thisPtr->setHttpHandleAndCheckForStop(httpHandle)) { goto end_function; }
                    rc = vcs_s3_getFileHelper(accessInfoResp,
                                              httpHandle,
                                              NULL,
                                              NULL,
                                              tempFilePath.str(),
                                              thisPtr->verbose_http_log);
                    if (thisPtr->clearHttpHandleAndCheckForStopWithinTask(httpHandle)) { goto end_function; }
                    // We consider all errors from ACS to be transient.
                    // In particular, this can return HTTP status 404 for a few seconds
                    // after the file has been created, because some storage providers do
                    // not have a read-after-write consistency guarantee.
                    if (rc == 0) { break; }
                    LOG_WARN("%p: task(%p): Transient Error:%d RetryIndex:%d", thisPtr, task, rc, retries);
                    if (retries+1 < NUM_IMMEDIATE_TRANSIENT_RETRY) {  // pause between immediate retries
                        if (thisPtr->checkForPauseStopWithinTask(true, QUICK_RETRY_INTERVAL)) {
                            goto end_function;
                        }
                    }
                }
                if (rc != 0) {
                    // We consider all errors from ACS to be transient; we need to check with VCS
                    // GET accessinfo again later to determine if it is retryable or not.
                    LOG_WARN("%p: vcs_s3_getFileHelper(%s, "FMTu64", "FMTu64")->\"%s\" returned %d",
                             thisPtr,
                            currDatasetRelPath.c_str(),
                            task->currDownloadEntry.comp_id,
                            task->currDownloadEntry.revision,
                            tempFilePath.c_str(), rc);
                    // Retry the download again later.
                    result->skip_ErrIncDownload = true;
                    goto skip;
                }

                // Mark in DB that download will never be done again.
                result->setDownloadSuccess = true;

                // Only relevant if hashes are available:
                // (optional safeguard) Compute hash of the downloaded file, complain if
                // that hash doesn't match the hash reported by VCS. Not sure if there is any
                // action that is safe to perform in this case.
                // Only relevant if hashes are available:
                // (optional optimization 5, to avoid hashing every file just downloaded):
                // Add (or increment numTimesToIgnore) {dirEntry path, numTimesToIgnore} in
                // in-memory map "fileMonitorToIgnore" (only safe if VPL inotify *does not*
                // de-dupe multiple changes for the same entry).

            } // download from VCS/Storage to temp file

            if(result->skip_ErrIncDownload) {
                goto skip;
            }
        }  // "download" to temp

        { // "Copyback" (rename the temp file to the correct final location).
            if(task->errorMode == DL_CHANGE_LOG_ERROR_MODE_COPYBACK) {
                LOG_INFO("%p: task(%p->%p): ACTION DOWN copyback begin:%s compId("FMTu64"),rev("FMTu64"),mtime("FMTu64")",
                         thisPtr, task, result,
                         currRelPath.c_str(),
                         task->currDownloadEntry.comp_id,
                         task->currDownloadEntry.revision,
                         task->currDownloadEntry.client_reported_mtime);
            } else if (!result->setDownloadSuccess) {
                // Download never happened or did not succeed.  (archive device offline, and file not on acs)
                LOG_WARN("%p: No download availability:%s compId("FMTu64"),rev("FMTu64")",
                         thisPtr, currRelPath.c_str(),
                         task->currDownloadEntry.comp_id,
                         task->currDownloadEntry.revision);
                result->skip_ErrIncDownload = true;
                goto skip;
            }
            AbsPath destAbsPath = thisPtr->getAbsPath(currRelPath);
            // TODO: Lock local file
            // Get modified time
            // TODO: compute hash
            {
                ////////////////////////////////////////////////////////////
                //////// DIVERGE FROM TWO-WAY, no need to check dest ///////
                // Make sure that the parent is a directory.
                {
                    VPLFS_stat_t statBuf;
                    AbsPath destAbsParent = thisPtr->getAbsPath(currRelPath.getParent());
                    rc = VPLFS_Stat(destAbsParent.c_str(), &statBuf);
                    if (rc != 0) {
                        // VPL_ERR_NOENT is expected.
                        if (rc != VPL_ERR_NOENT) {
                            // Unexpected; try and continue anyway.
                            LOG_ERROR("%p: task(%p): VPLFS_Stat(%s):%d",
                                      thisPtr, task, destAbsPath.c_str(), rc);
                        }
                        // Ensure parent directory exists.
                        rc = Util_CreateParentDir(destAbsPath.c_str());
                        if (rc < 0) {
                            LOG_ERROR("%p: task(%p): Util_CreateParentDir(%s) failed: %d",
                                     thisPtr, task, destAbsPath.c_str(), rc);
                            // Copyback hopeless...
                            result->skip_ErrIncCopyback = true;
                            goto skip;
                        }
                    } else if (statBuf.type != VPLFS_TYPE_DIR)
                    {
                        ASSERT(rc == 0);
                        rc = Util_rmFileOpenHandleSafe(destAbsParent.str(),
                                                       thisPtr->getTempDeleteDir().str());
                        if(rc != 0) {
                            LOG_WARN("%p: task(%p): Util_rmFileOpenHandleSafe(%s,%s):%d",
                                     thisPtr, task, destAbsParent.c_str(),
                                     thisPtr->getTempDeleteDir().c_str(), rc);
                            // We'll try to continue.  If this is really a problem,
                            // Util_CreateParentDir() won't be able to succeed and
                            // we'll stop there.
                        }
                        // Ensure parent directory exists.
                        rc = Util_CreateParentDir(destAbsPath.c_str());
                        if (rc < 0) {
                            LOG_ERROR("%p: task(%p): Util_CreateParentDir(%s) failed: %d",
                                     thisPtr, task, destAbsPath.c_str(), rc);
                            // Copyback hopeless...
                            result->skip_ErrIncCopyback = true;
                            goto skip;
                        }
                    }  // else, the directory already exists.
                }
                //////// DIVERGE FROM TWO-WAY, no need to check dest ///////
                ////////////////////////////////////////////////////////////

                {
                    VPLTime_t normLocalTimeToSet = task->currDownloadEntry.client_reported_mtime;
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
                            LOG_ERROR("%p: task(%p): VPLFile_SetTime(%s) failed: %d",
                                      thisPtr, task, tempFilePath.c_str(), rc);
                            result->skip_ErrIncCopyback = true;
                            goto skip;
                        }
                        result->infoNormLocalTimeThatWasSet = normLocalTimeToSet;  // printed out on download complete
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
                            LOG_ERROR("%p: task(%p): VPLFile_Stat(%s) failed:%d",
                                    thisPtr, task, tempFilePath.c_str(), rc);
                            result->skip_ErrIncCopyback = true;
                            goto skip;
                        }
                        result->normLocalTimeGet = fsTimeToNormLocalTime(tempFilePath, statBuf.vpl_mtime);
                        result->fileSize = static_cast<u64>(statBuf.size);
                        LOG_INFO("%p: task(%p), TimeStat(%s):"FMTu64", TimeSet:"FMTu64", TimeGet:"FMTu64", Size:"FMTu64,
                                thisPtr, task,
                                tempFilePath.c_str(),
                                statBuf.vpl_mtime,
                                result->normLocalTimeGet,
                                normLocalTimeToSet,
                                (u64)result->fileSize);
                    }
                    // Atomically rename the temp download to the local file.
                    rc = VPLFile_Rename(tempFilePath.c_str(), destAbsPath.c_str());
                    if (rc < 0) {
                        LOG_ERROR("%p: task(%p): VPLFile_Rename(%s, %s) failed: %d",
                                  thisPtr, task, tempFilePath.c_str(),
                                  destAbsPath.c_str(), rc);
                        result->skip_ErrIncCopyback = true;
                        goto skip;
                    }

                    // Update the LocalDB with the downloaded file's metadata (timestamp, compId, revision).
                    result->updateSyncHistoryTree = true;
                } // if (doRename)
            }
        } // "copyback"
        goto done;

     end_function:
        result->endFunction = true;
        goto done;
     skip:
     done:
        {
            result->currDownloadEntry = task->currDownloadEntry;
            LOG_INFO("%p: end task(%p)->result(%p): download_ok(%d),copyback_ok(%d)", thisPtr, task, result,
                    task->currDownloadEntry.download_succeeded, result->updateSyncHistoryTree);

            // Completed -- move from TaskList to ResultList.
            // Error or not, this must always be done.
            MutexAutoLock lock(task->mutex);
            task->enqueuedDlResults->push_back(result);
            removeEnqueuedDlTask(task->currDownloadEntry.row_id,
                                 *(task->enqueuedDlTasks));
        }
        return;
    }

    /// Applies the local changes to the server.
    void ApplyDownloadChangeLog(DownloadChangeLogErrorMode errorMode)
    {
        int rc;
        SCRow_downloadChangeLog currDownloadEntry;

        // afterRowId supports going on to the next request before the last
        // request is complete.  Required for parallel operations.  Only
        // set when a "Task" is spawned.
        u64 afterRowId = 0;
        u64 errorModeMaxRowId = 0;  // Only valid when in errorMode
        std::vector<DownloadFileTask*> enqueuedDlTasks;
        std::vector<DownloadFileResult*> enqueuedDlResults;
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
        int nextChangeRc;
        while ((nextChangeRc = getNextDownloadChangeLog(errorMode,
                                                        afterRowId,
                                                        errorModeMaxRowId,
                                                        /*out*/ currDownloadEntry)) == 0)
        {
            bool taskEnqueueAttempt = false;  // true when download task was spawned or
                                              // attempted to be spawned
            bool skip_ErrIncDownload = false;
            bool skip_ErrIncCopyback = false;
            if (!isValidDirEntry(currDownloadEntry.name)) {
                FAILED_ASSERT("Bad entry added to download change log: \"%s\"",
                              currDownloadEntry.name.c_str());
                goto skip;
            }
            // When using a thread pool, this thread (the primary worker thread) is not
            // allowed to block, since we want it to process finished worker threads ASAP.
            if (!useDownloadThreadPool())
            {   // Not using thread pool, allowed to just block.
                if (checkForPauseStop()) {
                    goto end_function;
                }
            }
            {
                SCRelPath currRelPath = getRelPath(currDownloadEntry);

                SCRow_syncHistoryTree currDbEntry;

                // Get the record from LocalDB.
                {
                    // Invariant: If the entry is in the download change log, it must be in
                    //     the syncHistoryTree.
                    rc = syncHistoryTree_getEntry(currRelPath, currDbEntry);
                    if(rc == SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) {
                        LOG_CRITICAL("%p: syncHistoryTree_getEntry(%s) missing: %d",
                                this, currRelPath.c_str(), rc);
                        goto skip;
                    }else if (rc != 0) {
                        LOG_CRITICAL("%p: syncHistoryTree_getEntry(%s) failed: %d",
                                this, currRelPath.c_str(), rc);
                        HANDLE_DB_FAILURE();
                        goto skip;
                    } else {
                        // dirEntry present in LocalDB; currDbEntry is valid.
                    }
                }

                switch (currDownloadEntry.download_action) {
                case DOWNLOAD_ACTION_DELETE:
                {
                    // Traverse the local metadata from the to-be-deleted-node depth first
                    // For each entryToDelete in traversal:
                    //     (optimization) (required for correctness if uploads are allowed
                    //                     to happen asynchronously) Need to skip the file if
                    //                     it was just uploaded by a different thread.
                    //     If entryToDelete is file,
                    //         Lock file (best effort), if local_content matches metadata
                    //         (no local update), then delete, else skip.
                    //     If entryToDelete is directory
                    //         If directory is empty, then delete, else skip.
                    //     Remove entryToDelete from LocalDB.
                    
                    // This check doesn't really matter for one-way sync, but it does matter for two-way sync.
                    if ((currDownloadEntry.is_dir != currDbEntry.is_dir) ||
                        (currDownloadEntry.comp_id != currDbEntry.comp_id) ||
                        (currDownloadEntry.revision != currDbEntry.revision) ||
                        (currDownloadEntry.client_reported_mtime != currDbEntry.local_mtime))
                    {
                        LOG_WARN("%p: Changelog no longer matches syncHistory: "
                                "is_dir(%d,%d), comp_id("FMTu64","FMTu64"), "
                                "revision("FMTu64","FMTu64"), mtime("FMTu64","FMTu64")",
                                this,
                                currDownloadEntry.is_dir, currDbEntry.is_dir,
                                currDownloadEntry.comp_id, currDbEntry.comp_id,
                                currDownloadEntry.revision, currDbEntry.revision,
                                currDownloadEntry.client_reported_mtime, currDbEntry.local_mtime);
                    }
                    BEGIN_TRANSACTION();
                    if (currDbEntry.is_dir) {
                        deleteLocalDir(currDbEntry);
                    } else {
                        deleteLocalEntry(currDbEntry);
                    }
                    CHECK_END_TRANSACTION_BYTES(0, false);
                    break;  // switch (currDownloadEntry.download_action)
                }
                case DOWNLOAD_ACTION_GET_FILE:
                {
                    DownloadFileTask* taskCtx = new DownloadFileTask();
                    taskCtx->currDownloadEntry = currDownloadEntry;
                    taskCtx->thisPtr = this;

                    taskCtx->errorMode = errorMode;
                    taskCtx->vcsArchiveAccess = dataset_access_info.archiveAccess;

                    taskCtx->mutex = &mutex;
                    taskCtx->enqueuedDlTasks = &enqueuedDlTasks;
                    taskCtx->enqueuedDlResults = &enqueuedDlResults;

                    taskEnqueueAttempt = true;
                    {   // Enqueue Task.
                        MutexAutoLock lock(&mutex);
                        enqueuedDlTasks.push_back(taskCtx);
                        if(useDownloadThreadPool()) {
                            if(hasDedicatedThread) {
                                rc = threadPool->AddTaskDedicatedThread(&threadPoolNotifier,
                                                                        PerformDownloadTask,
                                                                        taskCtx,
                                                                        (u64)this);
                            } else {
                                rc = threadPool->AddTask(PerformDownloadTask,
                                                         taskCtx,
                                                         (u64)this);
                            }
                        } else {
                            // No threadPool!  Do the work on this thread.
                            PerformDownloadTask(taskCtx);
                            rc = 0;
                        }

                        if(rc == 0) {
                            afterRowId = currDownloadEntry.row_id;
                            LOG_INFO("%p: task(%p): Created task downloadEntryRow("FMTu64")",
                                     this, taskCtx, afterRowId);
                        } else {
                            // Task enqueue attempt failed, but that's ok.  ThreadPool
                            // can be normally completely occupied.
                            enqueuedDlTasks.pop_back();
                            delete taskCtx;
                        }
                    }
                    break;  // switch (currDownloadEntry.download_action)
                } // case DOWNLOAD_ACTION_GET_FILE
                default:
                    FAILED_ASSERT("Unexpected: %d", (int)currDownloadEntry.download_action);
                    break;  // switch (currDownloadEntry.download_action)
                } // switch (currDownloadEntry.download_action)
            }
          skip:
            if(!taskEnqueueAttempt) {
                bool handleResultEndFunction = false;
                handleChangeLogResult(skip_ErrIncDownload,
                                      skip_ErrIncCopyback,
                                      currDownloadEntry,
                                      NULL,
                                      /*OUT*/    handleResultEndFunction);
                if(handleResultEndFunction) {
                    LOG_ERROR("%p: Handle result end function", this);
                    goto end_function;
                }
            } // else getNextDownloadChangeLog could stay on the current element.  If we
              // successfully launched the current element as a task, we will rely on afterRowId to avoid
              // checking it again.  If we were unable to launch it (because the thread pool
              // was full), we will get the same element from getNextDownloadChangeLog again
              // for the next iteration.

            /////////////////////////////////////////////////////////
            /////////////// Process any task results ////////////////
            // http://wiki.ctbg.acer.com/wiki/index.php/User_talk:Rlee/CCD_Sync_One_Way_Speedup
            {
                bool hasConcurrentTaskAfterRowId = hasNextConcurrentTaskAfterRowId(
                                                            errorMode,
                                                            afterRowId,
                                                            errorModeMaxRowId);
                bool threadPoolShutdown = false;
                bool findFreeThread = hasFreeThread(/*OUT*/threadPoolShutdown);
                MutexAutoLock lock(&mutex);

                do
                {
                    while(enqueuedDlResults.size() == 0  &&  // No results we can immediately process
                          !threadPoolShutdown &&
                          !stop_thread &&
                          useDownloadThreadPool() &&
                          (
                            // Has further work that needs to be done, but need
                            // to wait for capacity.
                            (hasConcurrentTaskAfterRowId &&
                             !findFreeThread) ||

                             // Paused (and no results to process).
                             !allowed_to_run ||

                            // Has outstanding tasks that need to complete, but
                            // need to wait for completion.
                            (enqueuedDlTasks.size() > 0 &&
                             !hasConcurrentTaskAfterRowId)
                          )
                         )
                    {
                        LOG_DEBUG("%p: About to wait: res:"FMTu_size_t", shut:%d, "
                                  "ptr:%p, has:%d, free:%d, task:"FMTu_size_t
                                  ", threadNotifier:%p",
                                  this, enqueuedDlResults.size(),
                                  threadPoolShutdown, threadPool, hasConcurrentTaskAfterRowId,
                                  findFreeThread, enqueuedDlTasks.size(), &threadPoolNotifier);

                        if (!allowed_to_run && enqueuedDlTasks.size()==0)
                        {   // Notify for blocking interface that we are about to pause.
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
                        rc = VPLSem_Wait(&threadPoolNotifier);
                        if (rc != 0) {
                            LOG_WARN("VPLSem_Wait failed: %d", rc);
                        }
                        LOG_DEBUG("%p: Done waiting", this);

                        lock.Relock(&mutex);
                        hasConcurrentTaskAfterRowId = hasNextConcurrentTaskAfterRowId(
                                                                errorMode,
                                                                afterRowId,
                                                                errorModeMaxRowId);
                        findFreeThread = hasFreeThread(/*OUT*/threadPoolShutdown);
                    }

                    // Handle results whether threadpool is NULL or not
                    while(enqueuedDlResults.size() > 0)
                    {
                        bool handleResultEndFunction = false;
                        DownloadFileResult* result = enqueuedDlResults.back();
                        lock.UnlockNow();

                        handleDlTaskResult(result,
                                           /*OUT*/    handleResultEndFunction);
                        LOG_INFO("%p: Processed result(%p), rowId:"FMTu64,
                                 this, result, result->currDownloadEntry.row_id);
                        lock.Relock(&mutex);
                        bool endFunction = result->endFunction || handleResultEndFunction;
                        removeEnqueuedDlResult(result->currDownloadEntry.row_id,
                                               enqueuedDlResults);
                        if(endFunction) {
                            // One of the tasks resulted in an end function.
                            goto end_function;
                        }
                    }
                    // If Pause is enabled and using threadpool, keep in this loop.
                    // Otherwise checkForPauseStop will be called at the beginning
                    //  of the next round.
                } while(!allowed_to_run && useDownloadThreadPool() && !stop_thread);

                if (allowed_to_run) {
                    worker_loop_paused_or_stopped = false;
                }

                if (stop_thread) {
                    goto end_function;
                }
            }  // End Scope of MutexAutoLock lock(&mutex);
            /////////////// Process any task results ////////////////
            /////////////////////////////////////////////////////////
        } // End while ((nextChangeRc = getNextDownloadChangeLog(...)) == 0)
        if (nextChangeRc != SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) {
            LOG_CRITICAL("%p: getNextDownloadChangeLog() failed: %d", this, nextChangeRc);
            HANDLE_DB_FAILURE();
        }
 end_function:
        {
            MutexAutoLock lock(&mutex);
            while(enqueuedDlTasks.size() > 0 || enqueuedDlResults.size() > 0)
            {
                if(enqueuedDlResults.size()==0) {
                    LOG_INFO("%p: Waiting for phase ("FMTu_size_t" tasks need completion, "FMTu_size_t" to process)",
                             this, enqueuedDlTasks.size(), enqueuedDlResults.size());
                    lock.UnlockNow();
                    ASSERT(!VPLMutex_LockedSelf(&mutex));  // still may be locked (recursive)
                    VPLSem_Wait(&threadPoolNotifier);
                    lock.Relock(&mutex);
                }
                while(enqueuedDlResults.size() > 0)
                {
                    bool unUsed_endFunction; // already in end_function section.
                    DownloadFileResult* result = enqueuedDlResults.back();
                    lock.UnlockNow();

                    handleDlTaskResult(result,
                                       /*OUT*/    unUsed_endFunction);
                    LOG_INFO("%p: Processed result(%p), rowId:"FMTu64", endNow:%d",
                             this, result,
                             result->currDownloadEntry.row_id, unUsed_endFunction);

                    lock.Relock(&mutex);
                    removeEnqueuedDlResult(result->currDownloadEntry.row_id,
                                           enqueuedDlResults);
                }
            }
        }
        CHECK_END_TRANSACTION_BYTES(0, true);
    }

    void removeEnqueuedDlResult(u64 rowId,
                                std::vector<DownloadFileResult*>& enqueued_results)
    {
        ASSERT(VPLMutex_LockedSelf(&mutex));
        // Remove the finished result, go backwards since we last examined the last element
        for(int index = enqueued_results.size()-1;
            index >= 0;
            --index)
        {
            DownloadFileResult* toDelete = enqueued_results[index];
            if(toDelete->currDownloadEntry.row_id == rowId)
            {   // Found it
                enqueued_results.erase(enqueued_results.begin()+index);
                LOG_DEBUG("%p: result(%p): Delete DownloadFileResult", this, toDelete);
                delete toDelete;
                break;
            }
        }
    }

    static void removeEnqueuedDlTask(u64 rowId,
                                     std::vector<DownloadFileTask*>& enqueued_tasks)
    {
        for(std::vector<DownloadFileTask*>::iterator taskIter = enqueued_tasks.begin();
            taskIter != enqueued_tasks.end(); ++taskIter)
        {
            if((*taskIter)->currDownloadEntry.row_id == rowId) {
                DownloadFileTask* toDelete = *taskIter;
                enqueued_tasks.erase(taskIter);
                delete toDelete;
                break;
            }
        }
    }

    void handleDlTaskResult(DownloadFileResult* result,
                            bool& endFunction)
    {
        int rc;
        endFunction = false;

        if(result->setDownloadSuccess)
        {   // Mark download done.  Download will never be done again.
            BEGIN_TRANSACTION();
            rc = localDb.downloadChangeLog_setDownloadSuccess(result->currDownloadEntry.row_id);
            if (rc != 0) {
                LOG_CRITICAL("%p: downloadChangeLog_setDownloadSuccess:%d", this, rc);
                HANDLE_DB_FAILURE();
                endFunction = true;
                return;
            }
            CHECK_END_TRANSACTION_BYTES(0, false);
        }

        if(result->updateSyncHistoryTree)
        {
            SCRelPath currRelPath = getRelPath(result->currDownloadEntry);
            BEGIN_TRANSACTION();
            rc = localDb.syncHistoryTree_updateEntryInfo(
                    currRelPath.getParent().str(),
                    currRelPath.getName(),
                    true, result->normLocalTimeGet,
                    true, result->currDownloadEntry.comp_id,
                    true, result->currDownloadEntry.revision,
                    std::string(""), false, 0);
            if (rc < 0) {
                LOG_CRITICAL("%p: syncHistoryTree_updateEntryInfo(%s) failed: %d",
                             this, currRelPath.c_str(), rc);
                HANDLE_DB_FAILURE();
                endFunction = true;
                return;
            }
            CHECK_END_TRANSACTION_BYTES((u64)result->fileSize, false);

            reportFileDownloaded(false, currRelPath, false, false);
            AbsPath destAbsPath = getAbsPath(currRelPath);
            LOG_INFO("%p: ACTION DOWN %s done:%s -> %s,mtime(requested:"FMTu64", actual:"FMTu64")",
                     this,
                     allowLocalFSUpdate() ? "download" : "(virtual)",
                     currRelPath.c_str(), destAbsPath.c_str(),
                     result->infoNormLocalTimeThatWasSet,
                     result->normLocalTimeGet);
        }
        bool endFunctionChangeLog = false;
        handleChangeLogResult(result->skip_ErrIncDownload,
                              result->skip_ErrIncCopyback,
                              result->currDownloadEntry,
                              result,
                              /*OUT*/    endFunctionChangeLog);
        endFunction |= endFunctionChangeLog;
    }

    void handleChangeLogResult(bool skip_ErrIncDownload,
                               bool skip_ErrIncCopyback,
                               const SCRow_downloadChangeLog& currDownloadEntry,
                               DownloadFileResult* dbgInfoResult,
                               /*OUT*/    bool& endFunction)
    {
        int rc;
        endFunction = false;
        if (skip_ErrIncDownload) {
            if (currDownloadEntry.download_err_count > ERROR_COUNT_LIMIT) {
                LOG_ERROR("%p: Abandoning error:%s,%s,"FMTu64","FMTu64
                          " -- Attempted "FMTu64" times",
                          this,
                          currDownloadEntry.parent_path.c_str(),
                          currDownloadEntry.name.c_str(),
                          currDownloadEntry.comp_id,
                          currDownloadEntry.revision,
                          currDownloadEntry.download_err_count);
                BEGIN_TRANSACTION();
                rc = localDb.downloadChangeLog_remove(currDownloadEntry.row_id);
                if (rc == SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) {
                    LOG_WARN("%p: already removed, downloadChangeLog_remove("FMTu64"). Continuing.",
                             this, currDownloadEntry.row_id);
                    return;
                } else if (rc != 0) {
                    LOG_CRITICAL("%p: downloadChangeLog_remove("FMTu64") failed: %d",
                            this, currDownloadEntry.row_id, rc);
                    HANDLE_DB_FAILURE();
                    endFunction = true;
                    return;
                }
                CHECK_END_TRANSACTION_BYTES(0, false);
            } else {
                BEGIN_TRANSACTION();
                rc = localDb.downloadChangeLog_incErrDownload(currDownloadEntry.row_id);
                if (rc == SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) {
                    LOG_WARN("%p: already removed, downloadChangeLog_incErrDownload("FMTu64"). Continuing.",
                             this, currDownloadEntry.row_id);
                    return;
                } else if (rc != 0) {
                    LOG_CRITICAL("%p: downloadChangeLog_incErrDownload("FMTu64") failed: %d",
                            this, currDownloadEntry.row_id, rc);
                    HANDLE_DB_FAILURE();
                    endFunction = true;
                    return;
                }
                CHECK_END_TRANSACTION_BYTES(0, false);
                MutexAutoLock lock(&mutex);
                setErrorTimeout(sync_policy.error_retry_interval);
            }
        } else if (skip_ErrIncCopyback) {
            if (currDownloadEntry.copyback_err_count > ERROR_COUNT_LIMIT) {
                LOG_ERROR("%p: Abandoning error:%s,%s,"FMTu64","FMTu64
                          " -- Attempted "FMTu64" times",
                          this,
                          currDownloadEntry.parent_path.c_str(),
                          currDownloadEntry.name.c_str(),
                          currDownloadEntry.comp_id,
                          currDownloadEntry.revision,
                          currDownloadEntry.copyback_err_count);
                BEGIN_TRANSACTION();
                rc = localDb.downloadChangeLog_remove(currDownloadEntry.row_id);
                if (rc == SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) {
                    LOG_WARN("%p: already removed, downloadChangeLog_remove("FMTu64"). Continuing.",
                             this, currDownloadEntry.row_id);
                    return;
                } else if (rc != 0) {
                    LOG_CRITICAL("%p: downloadChangeLog_remove("FMTu64") failed: %d",
                            this, currDownloadEntry.row_id, rc);
                    HANDLE_DB_FAILURE();
                    endFunction = true;
                    return;
                }
                CHECK_END_TRANSACTION_BYTES(0, false);
            } else {
                BEGIN_TRANSACTION();
                rc = localDb.downloadChangeLog_incErrCopyback(currDownloadEntry.row_id);
                if (rc == SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) {
                    LOG_WARN("%p: already removed, downloadChangeLog_incErrCopyback("FMTu64"). Continuing.",
                             this, currDownloadEntry.row_id);
                    return;
                } else if (rc != 0) {
                    LOG_CRITICAL("%p: downloadChangeLog_incErrCopyback("FMTu64") failed: %d",
                            this, currDownloadEntry.row_id, rc);
                    HANDLE_DB_FAILURE();
                    endFunction = true;
                    return;
                }
                CHECK_END_TRANSACTION_BYTES(0, false);
                MutexAutoLock lock(&mutex);
                setErrorTimeout(sync_policy.error_retry_interval);
            }
        }else{
            BEGIN_TRANSACTION();
            rc = localDb.downloadChangeLog_remove(currDownloadEntry.row_id);
            if (rc == SYNC_AGENT_DB_ERR_ROW_NOT_FOUND) {
                LOG_WARN("%p: already removed, downloadChangeLog_remove("FMTu64"). Continuing.",
                         this, currDownloadEntry.row_id);
                return;
            } else if (rc != 0) {
                LOG_CRITICAL("%p: downloadChangeLog_remove("FMTu64") failed: %d",
                        this, currDownloadEntry.row_id, rc);
                HANDLE_DB_FAILURE();
                endFunction = true;
                return;
            }
            CHECK_END_TRANSACTION_BYTES(0, false);
        }
    }

    void deferredUploadLoop() { }

    void migrateFrom_OneWayDownload_to_OneWayDownloadPureVirtualSync()
    {
        // Simple transition to delete all local content, but leaving the DB.
        int rc;
        VPLFS_dir_t dirStream;
        bool errorsExist = false;
        ASSERT(type == SYNC_TYPE_ONE_WAY_DOWNLOAD_PURE_VIRTUAL_SYNC);

        // Locally delete all files other than ".sync_temp" from the
        // syncConfig root directory
        rc = VPLFS_Opendir(local_dir.c_str(), &dirStream);
        if (rc != 0) {
            if (rc == VPL_ERR_NOENT) {
                LOG_INFO("%p: Directory (%s) no longer exists", this, local_dir.c_str());
                // Success, no directory means no migration work to do.
            } else {
                LOG_WARN("%p: VPLFS_Opendir(%s) failed: %d", this, local_dir.c_str(), rc);
                errorsExist = true;
            }
            goto exit;
        }
        {   // Opendir was successful.
            ON_BLOCK_EXIT(VPLFS_Closedir, &dirStream);

            // For each dirEntry in currDir:
            VPLFS_dirent_t dirEntry;
            while ((rc = VPLFS_Readdir(&dirStream, &dirEntry)) == VPL_OK) {
                // Ignore special directories and filenames that contain unsupported characters.
                if (!isValidDirEntry(dirEntry.filename)) {
                    continue;
                }

                ASSERT(std::string(dirEntry.filename)!=std::string(SYNC_TEMP_DIR));
                AbsPath toRemove = local_dir.appendRelPath(SCRelPath(std::string(dirEntry.filename)));
                rc = Util_rmRecursive(toRemove.str(),
                                      getTempDeleteDir().str());
                if (rc != 0) {
                    LOG_ERROR("%p:Util_rmRecursive(%s):%d", this, toRemove.c_str(), rc);
                    errorsExist = true;
                }
                if (checkForPauseStop()) {
                    return;
                }
            } // for each dirEntry
        }

     exit:
        if (!errorsExist) {  // Remove the migrate_from field with successful migration.
            rc = localDb.admin_set_sync_type(type,
                                             false, 0);
            if (rc != 0) {
                LOG_ERROR("%p:admin_set_sync_type(%d):%d", this, (int)type, rc);
            }
        }
        // Note: Ignore errors, we do not want errors in migration to hinder the actual syncConfig.
    }

    void migrateFrom_OneWayDownloadPureVirtualSync_to_OneWayDownload()
    {
        int rc;
        bool errorsExist = false;

        ASSERT(type == SYNC_TYPE_ONE_WAY_DOWNLOAD);

        // Need to add everything that is in the syncHistoryTree to the downloadChangeLog.
        {
            std::deque<SCRelPath> dirsToTraverseQ;
            dirsToTraverseQ.push_back(SCRelPath(""));
            while (!dirsToTraverseQ.empty())
            {
                SCRelPath toTraverse = dirsToTraverseQ.front();
                dirsToTraverseQ.pop_front();

                std::vector<SCRow_syncHistoryTree> dirEntries_out;

                rc = localDb.syncHistoryTree_getChildren(toTraverse.str(), dirEntries_out);
                if ((rc != 0) && (rc != SYNC_AGENT_DB_ERR_ROW_NOT_FOUND)) {
                    LOG_CRITICAL("%p: syncHistoryTree_get(%s) failed: %d",
                            this, toTraverse.c_str(), rc);
                    HANDLE_DB_FAILURE();
                    errorsExist = true;
                    goto end;
                }

                for(std::vector<SCRow_syncHistoryTree>::iterator it = dirEntries_out.begin();
                    it != dirEntries_out.end();
                    ++it)
                {
                    SCRow_syncHistoryTree& currDirEntry = *it;
                    if(isRoot(currDirEntry)){ continue; } // Skip root exception case

                    if(currDirEntry.is_dir) {
                        dirsToTraverseQ.push_back(getRelPath(currDirEntry));
                    }else{
                        // Set up the entry to be downloaded, and mark as NOT downloaded.
                        AbsPath dest = getAbsPath(getRelPath(currDirEntry));

                        if (currDirEntry.local_mtime_exists)
                        {
                            VPLFS_stat_t statBuf;
                            rc = VPLFS_Stat(dest.c_str(), &statBuf);
                            if(rc == 0 &&
                               statBuf.vpl_mtime == currDirEntry.local_mtime)
                            {  // This means the file already exists,
                                LOG_INFO("%p:Already exist(%s), no need to download",
                                         this, dest.c_str());
                                continue;
                            }
                        }

                        BEGIN_TRANSACTION();
                        rc = localDb_downloadChangeLog_add_and_setSyncStatus(
                                currDirEntry.parent_path,
                                currDirEntry.name,
                                false,
                                DOWNLOAD_ACTION_GET_FILE,
                                currDirEntry.comp_id,
                                currDirEntry.revision,
                                currDirEntry.local_mtime,
                                currDirEntry.is_on_acs,
                                std::string(""), false, 0);
                        if (rc != 0) {
                            LOG_ERROR("%p: downloadEntry(%s,%s,compId:"FMTu64
                                      ",revisionId:"FMTu64"):%d",
                                      this,
                                      currDirEntry.parent_path.c_str(),
                                      currDirEntry.name.c_str(),
                                      currDirEntry.comp_id,
                                      currDirEntry.revision, rc);
                            errorsExist = true;
                            continue;
                        }
                        CHECK_END_TRANSACTION(false);

                        LOG_INFO("%p: Decision syncTypeMigrate downloadEntry"
                                 "(%s,%s,compId:"FMTu64",revisionId:"FMTu64"):%d",
                                  this,
                                  currDirEntry.parent_path.c_str(),
                                  currDirEntry.name.c_str(),
                                  currDirEntry.comp_id,
                                  currDirEntry.revision,
                                  rc);
                    }
                    if (checkForPauseStop()) {
                        goto end;
                    }
                }
            }  // while(!dirsToTraverseQ.empty())
        }
        if (!errorsExist) {  // Remove the migrate_from field with successful migration.
            BEGIN_TRANSACTION();
            rc = localDb.admin_set_sync_type(type,
                                             false, 0);
            if (rc != 0) {
                LOG_ERROR("%p:admin_set_sync_type(%d):%d", this, (int)type, rc);
            }
            CHECK_END_TRANSACTION(false);
        }
        // Note: Ignore errors, we do not want errors in migration to hinder the actual syncConfig.

     end:
        CHECK_END_TRANSACTION(true);

    }

    void handleSyncTypeDownMigration()
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

        if (adminRow.migrate_from_sync_type_exists) {
            if (adminRow.migrate_from_sync_type == SYNC_TYPE_ONE_WAY_DOWNLOAD &&
                type == SYNC_TYPE_ONE_WAY_DOWNLOAD_PURE_VIRTUAL_SYNC)
            {
                migrateFrom_OneWayDownload_to_OneWayDownloadPureVirtualSync();
            } else if (adminRow.migrate_from_sync_type ==
                            SYNC_TYPE_ONE_WAY_DOWNLOAD_PURE_VIRTUAL_SYNC &&
                       type == SYNC_TYPE_ONE_WAY_DOWNLOAD)
            {
                migrateFrom_OneWayDownloadPureVirtualSync_to_OneWayDownload();
            } else {
                LOG_ERROR("%p:Migrate syncType %d -> %d not supported",
                          this, (int)adminRow.migrate_from_sync_type,
                          (int)adminRow.sync_type);
            }
        }
    }

    void workerLoop()
    {
        handleSyncTypeDownMigration();

        MutexAutoLock lock(&mutex);

        // Loop.
        while (1) {
            // If 0, then the sync config root was enqueued during this iteration of the loop.
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
                ApplyDownloadChangeLog(DL_CHANGE_LOG_ERROR_MODE_DOWNLOAD);
                ApplyDownloadChangeLog(DL_CHANGE_LOG_ERROR_MODE_COPYBACK);
                lock.Relock(&mutex);
            } else {
                // Without this here, once force_error_retry_timestamp is set, we would do an
                // immediate error retry next time we finish a worker loop iteration with an error.
                // We only want that to happen if the archive storage device actually came online
                // during that iteration.
                clearForceRetry();
            }

            // If true, we are certain that the dataset version has increased.
            bool vcsScanNeeded = download_scan_requested || downloadScanError;
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
                download_scan_requested = false;
                downloadScanError = false;
                lock.UnlockNow();

                {
                    enqueueDlScanRes = enqueueRootToNeedDownloadScan();
                    if (enqueueDlScanRes == 0) {
                        ScanRemoteChanges();
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
                        // TODO: Bug 12976: Confirm specific error code is comp_id is missing and
                        //     if initialScan, deleting everything in local tree.
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

            // Perform any deletions and downloads.
            {
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

            while (!checkForWorkToDo()) {   // This will also check error timers
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

