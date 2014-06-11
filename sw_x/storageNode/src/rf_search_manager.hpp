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

#ifndef RF_SEARCH_MANAGER_HPP_01_15_2014_
#define RF_SEARCH_MANAGER_HPP_01_15_2014_

#include "rf_search.hpp"
#include "vpl_time.h"
#include "vplu_common.h"

class RemoteFileSearchManager
{
public:
    virtual ~RemoteFileSearchManager() {}

    virtual int RemoteFileSearch_Add(const std::string& searchFilenamePattern,
                                     const std::string& searchScopeDirectory,
                                     bool disableIndex,
                                     bool recursive,
                                     u64& searchQueueId_out) = 0;

    /// Step 1/3 of "Removing a RemoteFileSearch".  DO NOT CONTINUE to step 2/3
    /// if this function returns error.
    virtual int RemoteFileSearch_RequestClose(u64 searchQueueId) = 0;

    /// Step 2/3 of "Removing a RemoteFileSearch".  DO NOT CONTINUE to step 3/3
    /// if this function returns error.
    virtual int RemoteFileSearch_Join(u64 searchQueueId) = 0;

    /// Step 3/3 of "Removing a RemoteFileSearch"
    virtual int RemoteFileSearch_Destroy(u64 searchQueueId) = 0;

    virtual int RemoteFileSearch_GetResults(
            u64 searchQueueId,
            u64 startIndex,
            u64 maxNumReturn,
            std::vector<RemoteFileSearchResult>& results_out,
            bool& searchOngoing_out) = 0;

    // This call blocks until all remote file searches and helper threads are
    // stopped.
    virtual int StopManager() = 0;

protected:
    RemoteFileSearchManager() {}
private:
    VPL_DISABLE_COPY_AND_ASSIGN(RemoteFileSearchManager);
};

/// Create a #RemoteFileSearchManager object.
/// @return The newly created #RemoteFileSearchManager object, or NULL if there was an error (check
///     \a err_code__out to find out the error code).
/// @note You must eventually call #DestroyRemoteFileSearchManager() to avoid leaking resources.
RemoteFileSearchManager* CreateRemoteFileSearchManager(u64 maxGeneratedResults,
                                                       VPLTime_t idleSearchTimeout,
                                                       int& err_code__out);

/// Destroy an object previously created by #CreateRemoteFileSearchManager(), releasing its resources.
void DestroyRemoteFileSearchManager(RemoteFileSearchManager* remoteFileManager);

#endif /* RF_SEARCH_MANAGER_HPP_01_15_2014_ */
