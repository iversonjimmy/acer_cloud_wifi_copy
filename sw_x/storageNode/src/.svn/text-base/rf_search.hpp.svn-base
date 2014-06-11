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

#ifndef RF_SEARCH_INTERFACE_HPP_01_07_2014_
#define RF_SEARCH_INTERFACE_HPP_01_07_2014_

#include "vpl_time.h"
#include "vplu_common.h"
#include "vplu_types.h"
#include <string>
#include <vector>

// RFS for RemoteFileSearch
struct RFS_ShortcutDetails
{
    std::string path;
    std::string displayName;
    std::string type;
    std::string args;
};

struct RemoteFileSearchResult {
    std::string path;
    std::string displayName;
    bool isDir;
    VPLTime_t lastChanged;
    u64 size;
    bool isReadOnly;
    bool isHidden;
    bool isSystem;
    bool isArchive;
    bool isAllowed;

    bool isShortcut;  // If true, isDir == false.
    RFS_ShortcutDetails shortcut;  // Shortcut details, only valid when isShortcut==true

    RemoteFileSearchResult()
    :   isDir(false),
        lastChanged(VPLTIME_INVALID),
        size(0),
        isReadOnly(false),
        isHidden(false),
        isSystem(false),
        isArchive(false),
        isAllowed(false),
        isShortcut(false)
    {}
};

class RemoteFileSearch {
 public:
    virtual ~RemoteFileSearch() {};

    // Returns the identifier that gets assigned on construction of the
    // implementation of this interface.
    virtual u64 GetSearchQueueId() = 0;

    // Gets the immediate results of the ongoing search.  This API will return immediately
    // startIndex - Index starting from 0 of where to return results.  Going backwards
    //              or skipping entries will result in an error.  This allows remote file
    //              to clean up results after they are no longer needed.
    // maxNumReturn - The maximum number of results to return starting from startIndex.
    // results_out - search results that are ready immediately
    // searchOngoing - When true, further results may appear if this API is called
    //                 at a later time.  When false, there will be no more results.
    virtual int GetResults(u64 startIndex,
                           u64 maxNumReturn,
                           std::vector<RemoteFileSearchResult>& results_out,
                           bool& searchOngoing_out) = 0;

    /// Stop the worker thread at the next reasonable opportunity
    virtual int RequestClose() = 0;

    /// Wait for the worker thread to stop after #RemoteFileSearch::RequestClose().
    virtual int Join() = 0;

 protected:
    RemoteFileSearch() {};
 private:
    VPL_DISABLE_COPY_AND_ASSIGN(RemoteFileSearch);
};

RemoteFileSearch* CreateRemoteFileSearch(
                        u64 assignedSearchQueueId,
                        const std::string& searchFilenamePattern,
                        const std::string& searchScopeDirectory,
                        u64 maxGeneratedResults,
                        bool disableIndex,
                        bool recursive,
                        int& errCode_out);

/// Destroy an object previously created by #CreateRemoteFileSearch(), releasing its resources.
void DestroyRemoteFileSearch(RemoteFileSearch* remoteFileSearch);

#endif /* RF_SEARCH_INTERFACE_HPP_01_07_2014_ */
