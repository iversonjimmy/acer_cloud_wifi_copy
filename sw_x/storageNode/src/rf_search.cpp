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
#include "rf_search.hpp"
#include "vplex_assert.h"
#include "vplu_format.h"
#include "vplu_mutex_autolock.hpp"
#include "vpl_fs.h"
#include "vpl_th.h"
#include "gvm_errors.h"

#include <algorithm>
#include <deque>

#include "log.h"

class RemoteFileSearchImpl : public RemoteFileSearch
{
protected:
    VPL_DISABLE_COPY_AND_ASSIGN(RemoteFileSearchImpl);

    // Id assigned in the construction of this object.
    const u64 searchQueueId;

    // Search pattern for the file.
    const std::string searchFilenamePattern;

    // Directory to start the search.
    const std::string searchScopeDirectory;

    bool closeRequested;

    // Expected index of the next request
    u64 expectedIndex;

    // Start index of the previous request
    u64 previousIndex;

    VPLMutex_t mutex;

public:
    RemoteFileSearchImpl(
            u64 searchQueueId,
            const std::string& searchFilenamePattern,
            const std::string& searchScopeDirectory)
    :   searchQueueId(searchQueueId),
        searchFilenamePattern(searchFilenamePattern),
        searchScopeDirectory(searchScopeDirectory),
        closeRequested(false),
        expectedIndex(0),
        previousIndex(0)
    {
        VPL_SET_UNINITIALIZED(&mutex);
    }

    int init()
    {
        int rv = 0;
        rv = VPLMutex_Init(&mutex);
        if (rv != 0) {
            LOG_ERROR("%p: VPLMutex_Init failed: %d", this, rv);
            return rv;
        }

        rv = subclassInit();
        if (rv != 0) {
            LOG_ERROR("%p: specificInit failed: %d", this, rv);
            return rv;
        }

        return rv;

    }

    virtual ~RemoteFileSearchImpl()
    {
        if (VPL_IS_INITIALIZED(&mutex)) {
            VPLMutex_Destroy(&mutex);
        }
    }

protected:
    virtual int subclassInit() = 0;

    virtual u64 GetSearchQueueId()
    {
        return searchQueueId;
    }

    static void setRemoteFileSearchResult(const std::string& path,
                                          const std::string& displayName,
                                          const VPLFS_stat_t& statBuf,
                                          bool isShortcut,
                                          RFS_ShortcutDetails shortcut,
                                          RemoteFileSearchResult& result_out)
    {
        result_out.path = path;
        result_out.displayName = displayName;
        result_out.isDir = (statBuf.type == VPLFS_TYPE_DIR);
        result_out.lastChanged = statBuf.vpl_mtime;
        result_out.size = static_cast<u64>(statBuf.size);
        result_out.isReadOnly = (statBuf.isReadOnly != VPL_FALSE);
        result_out.isHidden = (statBuf.isHidden != VPL_FALSE);
        result_out.isSystem = (statBuf.isSystem != VPL_FALSE);
        result_out.isArchive = (statBuf.isArchive != VPL_FALSE);
        // Everything allowed. This field requested to be reserved for future use.
        result_out.isAllowed = true;

        result_out.isShortcut = isShortcut;
        result_out.shortcut = shortcut;
    }

    static std::string cleanupWindowsPath(const std::string& winPath)
    {
        std::string intermediate;
        intermediate = winPath;
        // Replace all '\\' with '/'
        std::replace(intermediate.begin(), intermediate.end(), '/', '\\');

        // Remove double slashes
        std::string toReturn;
        char previous = '\0';
        for(int i=0; i < intermediate.size(); ++i) {
            if(previous == '\\' && intermediate[i] == '\\') {
                continue;
            }
            toReturn.push_back(intermediate[i]);
            previous = intermediate[i];
        }
        return toReturn;
    }
};

#include "win32/rf_search_indexed.fragment.cpp"
#include "win32/rf_search_unindexed.fragment.cpp"

RemoteFileSearch* CreateRemoteFileSearch(
        u64 assignedSearchQueueId,
        const std::string& searchFilenamePattern,
        const std::string& searchScopeDirectory,
        u64 maxGeneratedResults,
        bool disableIndex,
        bool recursive,
        int& errCode_out)
{
    errCode_out = 0;
    RemoteFileSearchImpl* toReturn;

    if (RemoteFileSearchIndexedWin32::IndexExists(searchScopeDirectory) &&
        !disableIndex)
    {
        toReturn = new (std::nothrow) RemoteFileSearchIndexedWin32(
                                                    assignedSearchQueueId,
                                                    searchFilenamePattern,
                                                    searchScopeDirectory);
        LOG_INFO("Created RemoteFileIndexSearch(%s, %s, "FMTu64")",
                 searchScopeDirectory.c_str(), searchFilenamePattern.c_str(),
                 assignedSearchQueueId);
    } else {
        toReturn = new (std::nothrow) RemoteFileSearchUnindexedWin32(
                                                    assignedSearchQueueId,
                                                    searchFilenamePattern,
                                                    searchScopeDirectory,
                                                    recursive,
                                                    maxGeneratedResults);
        LOG_INFO("Created RemoteFileUnindexSearch(%s, %s, "FMTu64")",
                 searchScopeDirectory.c_str(), searchFilenamePattern.c_str(),
                 assignedSearchQueueId);
    }


    if (toReturn == NULL) {
        return toReturn;
    }

    int rc = toReturn->init();
    if (rc != 0) {
        LOG_ERROR("SearchCreation(%s, %s, "FMTu64"):%d",
                  searchScopeDirectory.c_str(), searchFilenamePattern.c_str(),
                  assignedSearchQueueId, rc);
        delete toReturn;
        toReturn = NULL;
    }

    return toReturn;
}

void DestroyRemoteFileSearch(RemoteFileSearch* remoteFileSearch)
{
    delete remoteFileSearch;
}
