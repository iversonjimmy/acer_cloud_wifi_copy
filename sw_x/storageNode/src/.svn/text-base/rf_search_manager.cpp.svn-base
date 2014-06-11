//
//  Copyright 2014 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER CLOUD
//  TECHNOLOGY INC.
//
#include "rf_search_manager.hpp"
#include "fs_dataset.hpp"
#include "gvm_errors.h"
#include "gvm_thread_utils.h"
#include "vpl_th.h"
#include "vpl_time.h"
#include "vplex_assert.h"
#include "vplu_mutex_autolock.hpp"
#include <map>
#include <string>
#include "log.h"

struct RemoteFileSearchInstance
{
    RemoteFileSearch* remoteFileSearch;
    // Timestamp of the last queue access.  The instance will
    // be cleaned up after a set time has passed.
    VPLTime_t lastRemoteFileSearchAccess;
    // Request closed has been called on the remoteFileSearch.
    bool closed;
    bool joined;

    RemoteFileSearchInstance()
    :   remoteFileSearch(NULL),
        lastRemoteFileSearchAccess(VPLTIME_INVALID),
        closed(false),
        joined(false)
    {}
};

class RemoteFileSearchManagerImpl:public RemoteFileSearchManager
{
protected:
    VPL_DISABLE_COPY_AND_ASSIGN(RemoteFileSearchManagerImpl);

    /// Protects the following fields
    VPLMutex_t mutex;
    u64 lastSearchQueueId;
    std::vector<RemoteFileSearchInstance*> remoteFileSearchInstances;
    const u64 maxGeneratedResults;

    const int maxNumRfSearchInstances;    // Maximum number of ongoing searches.
    const VPLTime_t idleSearchTimeout;
    VPLCond_t checkTimeoutsCondVar;
    VPLDetachableThreadHandle_t checkTimeoutsThread;

    // State needed to convert RemoteFile path to win32 path.
    std::map<std::string, std::string> prefixRewriteRules;
    VPLTime_t lastPrefixRewriteRulesRefresh;
    const VPLTime_t refreshPrefixRulesThreshold;

    bool managerStopped;

    // Set to 0 for no timeouts, otherwise, remoteFileSearches will be cleaned
    // up after timeout time has elapsed without activity.
    RemoteFileSearchManagerImpl(u64 maxGeneratedResults,
                                VPLTime_t idleSearchTimeout)
    :   lastSearchQueueId(0),
        maxGeneratedResults(maxGeneratedResults),
        maxNumRfSearchInstances(15),
        idleSearchTimeout(idleSearchTimeout),
        lastPrefixRewriteRulesRefresh(0),
        refreshPrefixRulesThreshold(VPLTime_FromMinutes(10)),
        managerStopped(true)
    {
        VPL_SET_UNINITIALIZED(&mutex);
        VPL_SET_UNINITIALIZED(&checkTimeoutsCondVar);
    }

    friend RemoteFileSearchManager* CreateRemoteFileSearchManager(
            u64 maxGeneratedResults,
            VPLTime_t idleSearchTimeout,
            int& err_code__out);

    int init()
    {
        int rv;
        lastSearchQueueId = VPLTime_GetTime() * 10000;

        rv = VPLMutex_Init(&mutex);
        if (rv != 0) {
            LOG_ERROR("VPLMutex_Init:%d", rv);
            return rv;
        }

        rv = VPLCond_Init(&checkTimeoutsCondVar);
        if (rv != 0) {
            LOG_ERROR("VPLCond_Init:%d", rv);
            return rv;
        }
        managerStopped = false;

        if (idleSearchTimeout > 0)
        {   // Start a thread to automatically clean up after a set amount of
            // time for each instance
            rv = Util_SpawnThread(CheckIdleSearchTimeout_ThreadFn,
                                  this,
                                  UTIL_DEFAULT_THREAD_STACK_SIZE,
                                  VPL_TRUE,
                                  &checkTimeoutsThread);
        }

        return rv;
    }

    ~RemoteFileSearchManagerImpl()
    {
        if (!managerStopped) {
            LOG_ERROR("ManagerStop() not called.");
        }

        VPLCond_Destroy(&checkTimeoutsCondVar);
        VPLMutex_Destroy(&mutex);
    }

    virtual int RemoteFileSearch_Add(const std::string& searchFilenamePattern,
                                     const std::string& searchScopeDirectory,
                                     bool disableIndex,
                                     bool recursive,
                                     u64& searchQueueId_out)
    {
        int errCode;
        searchQueueId_out = 0;
        u64 nextSearchQueueId;
        std::string searchScopeDirectoryAbsPath;


        MutexAutoLock lock(&mutex);
        if (managerStopped) {
            LOG_ERROR("Manager already stopped. Programmer error.");
            return RF_SEARCH_ERR_INTERNAL;
        }

        if (remoteFileSearchInstances.size() >= maxNumRfSearchInstances) {
            LOG_ERROR("Max remoteFileSearchInstances: %d.  Search dropped(%s, %s)",
                      remoteFileSearchInstances.size(),
                      searchFilenamePattern.c_str(), searchScopeDirectory.c_str());
            return RF_SEARCH_ERR_MAX_ONGOING_SEARCHES;
        }

        // State needed to convert RemoteFile path to win32 path.
        if ((VPLTime_GetTimeStamp() >=
              (lastPrefixRewriteRulesRefresh + refreshPrefixRulesThreshold)) ||
            (lastPrefixRewriteRulesRefresh == 0))
        {
            std::set<std::string> winValidVirtualFolders;
            std::map<std::string, _VPLFS__LibInfo> winLibFolders;
            fs_dataset::refreshPrefixRewriteRulesHelper(/*unused*/ winValidVirtualFolders,
                                                        prefixRewriteRules,
                                                        /*unused*/ winLibFolders);
            lastPrefixRewriteRulesRefresh = VPLTime_GetTimeStamp();
        }
        errCode = fs_dataset::rfPathToAbsPath(searchScopeDirectory,
                                              prefixRewriteRules,
                                              searchScopeDirectoryAbsPath);
        if (errCode != 0) {
            LOG_ERROR("getWindowsAbsPath(%s):%d",
                      searchScopeDirectory.c_str(), errCode);
            return RF_SEARCH_ERR_PATH_DOES_NOT_EXIST;
        }

        // Cleanup path returned by rfPathToAbsPath.  The check for whether an
        // index exists is fairly sensitive, so the path must be perfect.
        // The rfPathToAbsPath function can return two '/' in a row, dedupe these.
        const std::string doubleForwardSlash("//");
        for(int strIndex = searchScopeDirectoryAbsPath.find(doubleForwardSlash);
            strIndex!=std::string::npos;
            strIndex = searchScopeDirectoryAbsPath.find(doubleForwardSlash, strIndex))
        {
            searchScopeDirectoryAbsPath.replace(strIndex, doubleForwardSlash.size(), "\\");
        }

        // It's a window's path, except need to replace '/' with '\\'
        std::replace(searchScopeDirectoryAbsPath.begin(),
                     searchScopeDirectoryAbsPath.end(), '/', '\\');

        nextSearchQueueId = ++lastSearchQueueId;
        RemoteFileSearch* remoteFileSearch = CreateRemoteFileSearch(
                nextSearchQueueId,
                searchFilenamePattern,
                searchScopeDirectoryAbsPath,
                maxGeneratedResults,
                disableIndex,
                recursive,
                /*OUT*/ errCode);
        if (remoteFileSearch == NULL) {
            LOG_ERROR("CreateRemoteFileSearch:%d", errCode);
            return RF_SEARCH_ERR_INTERNAL;
        }

        {
            RemoteFileSearchInstance* rfsi = new RemoteFileSearchInstance;
            rfsi->remoteFileSearch = remoteFileSearch;
            rfsi->lastRemoteFileSearchAccess = VPLTime_GetTimeStamp();

            remoteFileSearchInstances.push_back(rfsi);
        }
        searchQueueId_out = remoteFileSearch->GetSearchQueueId();

        if (idleSearchTimeout > 0) {
            ASSERT(VPLMutex_LockedSelf(&mutex));
            VPLCond_Signal(&checkTimeoutsCondVar);
        }
        return 0;
    }

    /// Step 1/3 of "Removing a RemoteFileSearch".  DO NOT CONTINUE to step 2/3
    /// if this function returns error.
    virtual int RemoteFileSearch_RequestClose(u64 searchQueueId)
    {
        MutexAutoLock lock(&mutex);
        if (managerStopped) {
            LOG_ERROR("Manager already stopped. Programmer error.");
            return RF_SEARCH_ERR_INTERNAL;
        }
        for (std::vector<RemoteFileSearchInstance*>::iterator iter=remoteFileSearchInstances.begin();
             iter != remoteFileSearchInstances.end(); ++iter)
        {
            RemoteFileSearchInstance* instance = (*iter);
            RemoteFileSearch* rfs = instance->remoteFileSearch;
            if (searchQueueId==rfs->GetSearchQueueId())
            {
                if (!instance->closed) {
                    int rc = rfs->RequestClose();
                    if (rc != 0) {
                        LOG_ERROR("searchQueueId("FMTu64"), RequestClose:%d",
                                  rfs->GetSearchQueueId(), rc);
                    } else {
                        instance->closed = true;
                    }
                    return rc;
                } else {
                    // Not necessarily programmer error, could be expected race.
                    LOG_INFO("searchQueueId("FMTu64") already closed.", searchQueueId);
                    break;
                }
            }
        }
        return RF_SEARCH_ERR_SEARCH_QUEUE_ID_INVALID;
    }

    /// Step 2/3 of "Removing a RemoteFileSearch".  DO NOT CONTINUE to step 3/3
    /// if this function returns error.
    virtual int RemoteFileSearch_Join(u64 searchQueueId)
    {
        RemoteFileSearch* toJoin = NULL;
        {
            MutexAutoLock lock(&mutex);
            if (managerStopped) {
                LOG_ERROR("Manager already stopped. Programmer error.");
                return RF_SEARCH_ERR_INTERNAL;
            }
            for (std::vector<RemoteFileSearchInstance*>::iterator iter=remoteFileSearchInstances.begin();
                 iter != remoteFileSearchInstances.end(); ++iter)
            {
                RemoteFileSearchInstance* instance = (*iter);
                RemoteFileSearch* rfs = instance->remoteFileSearch;
                if (searchQueueId==rfs->GetSearchQueueId())
                {
                    if (!instance->joined) {
                        if (!instance->closed) {
                            LOG_ERROR("searchQueueId("FMTu64") RequestClose never succeeded", searchQueueId);
                            break;
                        }
                        instance->joined = true;
                        toJoin = rfs;
                        break;
                    } else {
                        LOG_ERROR("searchQueueId("FMTu64") already joined", searchQueueId);
                        return RF_SEARCH_ERR_INTERNAL;
                    }
                }
            }
        }
        int rv = RF_SEARCH_ERR_SEARCH_QUEUE_ID_INVALID;
        if (toJoin) {
            rv = toJoin->Join();
            if (rv != 0) {
                LOG_ERROR("Join for searchQueueId("FMTu64"):%d", searchQueueId, rv);
            }
        }
        return rv;
    }

    /// Step 3/3 of "Removing a RemoteFileSearch"
    virtual int RemoteFileSearch_Destroy(u64 searchQueueId)
    {
        MutexAutoLock lock(&mutex);
        if (managerStopped) {
            LOG_ERROR("Manager already stopped. Programmer error.");
            return RF_SEARCH_ERR_INTERNAL;
        }
        for (std::vector<RemoteFileSearchInstance*>::iterator iter=remoteFileSearchInstances.begin();
             iter != remoteFileSearchInstances.end(); ++iter)
        {
            RemoteFileSearchInstance* instance = (*iter);
            RemoteFileSearch* rfs = instance->remoteFileSearch;
            if (searchQueueId==rfs->GetSearchQueueId() &&
                instance->closed &&
                instance->joined)
            {
                if (instance->closed && instance->joined) {
                    DestroyRemoteFileSearch(rfs);
                    remoteFileSearchInstances.erase(iter);
                    delete instance;
                    return 0;
                } else {
                    LOG_ERROR("searchQueueId("FMTu64") not closed and joined (%d,%d)",
                              searchQueueId, instance->closed?1:0, instance->joined?1:0);
                    return RF_SEARCH_ERR_INTERNAL;
                }
            }
        }
        return RF_SEARCH_ERR_SEARCH_QUEUE_ID_INVALID;
    }

    virtual int RemoteFileSearch_GetResults(
            u64 searchQueueId,
            u64 startIndex,
            u64 maxNumReturn,
            std::vector<RemoteFileSearchResult>& results_out,
            bool& searchOngoing_out)
    {
        results_out.clear();
        searchOngoing_out = false;

        MutexAutoLock lock(&mutex);
        if (managerStopped) {
            LOG_ERROR("Manager stopped");
            return RF_SEARCH_ERR_INTERNAL;
        }

        RemoteFileSearchInstance* rfsi = getRemoteFileInstance(searchQueueId);
        if (rfsi == NULL) {
            LOG_ERROR("searchQueueId("FMTu64") no longer available.", searchQueueId);
            return RF_SEARCH_ERR_SEARCH_QUEUE_ID_INVALID;
        }

        rfsi->lastRemoteFileSearchAccess = VPLTime_GetTimeStamp();
        int rv = rfsi->remoteFileSearch->GetResults(startIndex,
                                                    maxNumReturn,
                                                    /*OUT*/ results_out,
                                                    /*OUT*/ searchOngoing_out);
        if (rv != 0) {
            LOG_ERROR("searchQueueId("FMTu64") GetResults:%d", rv);
        }

        return rv;
    }

    static VPLTHREAD_FN_DECL CheckIdleSearchTimeout_ThreadFn(void* arg)
    {
        RemoteFileSearchManagerImpl* self = (RemoteFileSearchManagerImpl*) arg;
        LOG_INFO("%p: Start idleSearchTimeout thread.", self);
        self->checkIdleSearchTimeoutLoop();
        LOG_INFO("%p: Ended idleSearchTimeout thread.", self);
        return VPLTHREAD_RETURN_VALUE;
    }

    void checkIdleSearchTimeoutLoop()
    {
        MutexAutoLock lock(&mutex);
        while (1)
        {
            VPLTime_t minTimeToExpiration = VPL_TIMEOUT_NONE;  // (max-int u64)
            bool traversedAllEntries = false;

            while (!traversedAllEntries) {
                // Reset for-loop state: When lock is released within for-loop below,
                // the code breaks from the for-loop to restart the work under lock.
                minTimeToExpiration = VPL_TIMEOUT_NONE;   // (max-int u64)
                ssize_t numInstances = remoteFileSearchInstances.size();
                ssize_t instancesTraversed = 0;

                for (std::vector<RemoteFileSearchInstance*>::iterator iter =
                        remoteFileSearchInstances.begin();
                     iter != remoteFileSearchInstances.end(); ++iter)
                {
                    ++instancesTraversed;
                    VPLTime_t currentTime = VPLTime_GetTimeStamp();
                    RemoteFileSearchInstance* instance = (*iter);
                    VPLTime_t timeToTimeout = VPLTime_DiffClamp(currentTime,
                                              instance->lastRemoteFileSearchAccess);
                    if (timeToTimeout >= idleSearchTimeout)
                    {   // Automatically remove the search instance.
                        RemoteFileSearch* rfs = instance->remoteFileSearch;
                        if (instance->closed) {
                            // Ignore, this is already being closed
                            continue;
                        }
                        int rc = rfs->RequestClose();
                        if (rc != 0) {
                            LOG_ERROR("searchQueueId("FMTu64"), auto RequestClose:%d. Continuing.",
                                      rfs->GetSearchQueueId(), rc);
                            continue;
                        }
                        instance->closed = true;

                        // Committed to remove,
                        LOG_INFO("searchQueueId("FMTu64") timed out. Cleaning up.",
                                 rfs->GetSearchQueueId());
                        remoteFileSearchInstances.erase(iter);
                        delete instance;

                        lock.UnlockNow();

                        rc = rfs->Join();
                        if (rc != 0) {
                            LOG_ERROR("searchQueueId("FMTu64") Join:%d",
                                      rfs->GetSearchQueueId(), rc);
                        }
                        DestroyRemoteFileSearch(rfs);

                        lock.Relock(&mutex);
                        // We've removed a RemoteFileSearchInstance.  Renew for-loop
                        // state under lock because the data structure may have
                        // changed. This is n^2 behavior, but the number of
                        // concurrent searches should be small.
                        break;  // Will go to LABEL_RENEW_FOR_LOOP_STATE
                    } else {
                        if (timeToTimeout < minTimeToExpiration) {
                            minTimeToExpiration = timeToTimeout;
                        }
                    }
                }
                // LABEL_RENEW_FOR_LOOP_STATE

                if (numInstances == instancesTraversed)
                {   // Jump out of loop since all entries were traversed.
                    //  Will go to LABEL_ALL_RF_SEARCH_INSTANCES_TRAVERSED
                    traversedAllEntries = true;
                }
            }
            // LABEL_ALL_RF_SEARCH_INSTANCES_TRAVERSED

            ASSERT(VPLMutex_LockedSelf(&mutex));

            if (managerStopped) {
                break; // Goes to LABEL_END_LOOP
            }

            int rc = VPLCond_TimedWait(&checkTimeoutsCondVar, &mutex, minTimeToExpiration);
            if (rc != 0 && rc != VPL_ERR_TIMEOUT) {
                LOG_ERROR("%p: VPLCond_TimedWait failed: %d", this, rc);
            }

            if (managerStopped) {
                break; // Goes to LABEL_END_LOOP
            }
        }
        // LABLE_END_LOOP
    }

    virtual int StopManager()
    {
        int rv = 0;
        {
            MutexAutoLock lock(&mutex);
            if (managerStopped) {
                LOG_ERROR("Manager already stopped. Programmer error.");
                return RF_SEARCH_ERR_INTERNAL;
            }
            managerStopped = true;
            if (idleSearchTimeout > 0) {
                VPLCond_Signal(&checkTimeoutsCondVar);
            }
        }

        if (idleSearchTimeout > 0) {
            LOG_INFO("%p: Join checkTimeoutsThread", this);
            rv = VPLDetachableThread_Join(&checkTimeoutsThread);
            LOG_INFO("%p: Join checkTimeoutsThread Complete:%d", this, rv);
        }

        // ASSUMPTION_A: The assumption here is that there will be no other accesses
        // from here on.  Locking is not necessary, but still locking for
        // consistency and protecting data structures against programmer error.
        {
            // Request close
            for (std::vector<RemoteFileSearchInstance*>::iterator iter=remoteFileSearchInstances.begin();
                 iter != remoteFileSearchInstances.end(); ++iter)
            {
                RemoteFileSearchInstance* instance = (*iter);
                RemoteFileSearch* rfs = instance->remoteFileSearch;
                {
                    if (!instance->closed) {
                        int rc = rfs->RequestClose();
                        if (rc != 0) {
                            LOG_ERROR("searchQueueId("FMTu64"), RequestClose:%d. Continuing.",
                                      rfs->GetSearchQueueId(), rc);
                            rv = RF_SEARCH_ERR_INTERNAL;
                            continue;
                        }
                        instance->closed = true;
                    }
                }
            }
        }

        {
            // Join all searches
            std::vector<RemoteFileSearchInstance*> rfsiCopy;
            {
                MutexAutoLock lock(&mutex);
                rfsiCopy = remoteFileSearchInstances;
            }

            // See ASSUMPTION_A above.
            for (std::vector<RemoteFileSearchInstance*>::iterator iter=rfsiCopy.begin();
                 iter != rfsiCopy.end(); ++iter)
            {
                RemoteFileSearchInstance* instance = (*iter);
                RemoteFileSearch* rfs = instance->remoteFileSearch;
                {
                    if (!instance->joined) {
                        if (!instance->closed) {
                            LOG_ERROR("searchQueueId("FMTu64") RequestClose never succeeded",
                                      rfs->GetSearchQueueId());

                            continue;
                        }
                        instance->joined = true;
                        int rc = rfs->Join();
                        if (rc != 0) {
                            LOG_ERROR("searchQueueId("FMTu64") Join:%d",
                                      rfs->GetSearchQueueId(), rc);
                            rv = RF_SEARCH_ERR_INTERNAL;
                            continue;
                        }
                    }
                }
            }
            {
                MutexAutoLock lock(&mutex);
                remoteFileSearchInstances = rfsiCopy;
            }
        }

        {
            MutexAutoLock lock(&mutex);
            // Request close
            for (std::vector<RemoteFileSearchInstance*>::iterator iter=remoteFileSearchInstances.begin();
                 iter != remoteFileSearchInstances.end(); ++iter)
            {
                RemoteFileSearchInstance* instance = (*iter);
                RemoteFileSearch* rfs = instance->remoteFileSearch;
                if (instance->closed && instance->joined) {
                    DestroyRemoteFileSearch(rfs);
                } else {
                    LOG_ERROR("searchQueueId("FMTu64") not closed and joined (%d,%d). Leaking",
                              rfs->GetSearchQueueId(), instance->closed?1:0, instance->joined?1:0);
                    rv = RF_SEARCH_ERR_INTERNAL;
                }
                delete instance;
            }
            remoteFileSearchInstances.clear();
        }
        return rv;
    }

private:
    RemoteFileSearchInstance* getRemoteFileInstance(u64 searchQueueId)
    {
        ASSERT(VPLMutex_LockedSelf(&mutex));
        for (std::vector<RemoteFileSearchInstance*>::iterator iter=remoteFileSearchInstances.begin();
             iter != remoteFileSearchInstances.end(); ++iter)
        {
            RemoteFileSearchInstance* instance = (*iter);
            RemoteFileSearch* rfs = instance->remoteFileSearch;
            if (searchQueueId==rfs->GetSearchQueueId() &&
                !instance->closed &&
                !instance->joined)
            {
                return instance;
            }
        }
        return NULL;
    }
};


RemoteFileSearchManager* CreateRemoteFileSearchManager(u64 maxGeneratedResults,
                                                       VPLTime_t idleSearchTimeout,
                                                       int& err_code__out)
{
    RemoteFileSearchManagerImpl* result = new (std::nothrow) RemoteFileSearchManagerImpl(
            maxGeneratedResults,
            idleSearchTimeout);
    if (result==NULL) {
        LOG_ERROR("Out of memory.");
        err_code__out = RF_SEARCH_ERR_INTERNAL;
        return NULL;
    }

    err_code__out = result->init();
    if (err_code__out != 0) {
        delete result;
        result = NULL;
    }

    return result;
}

/// Destroy an object previously created by #CreateRemoteFileSearchManager(), releasing its resources.
void DestroyRemoteFileSearchManager(RemoteFileSearchManager* remoteFileManager)
{
    delete remoteFileManager;
}
