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

// NOTE: this file is only intended to be #include'd by rf_search.cpp
#include "gvm_thread_utils.h"
#include "gvm_file_utils.hpp"
#include "vpl_fs.h"

// included for static functions
#include "fs_dataset.hpp"
#include "HttpSvc_Sn_Handler_rf.hpp"

class RemoteFileSearchUnindexedWin32 : public RemoteFileSearchImpl
{
private:
    VPL_DISABLE_COPY_AND_ASSIGN(RemoteFileSearchUnindexedWin32);
    // Thread control
    VPLCond_t workToDoCondVar;
    VPLDetachableThreadHandle_t workerSearchThread;
    bool threadEnded;  // True when thread finishes work (not necessarily cleaned up)
    bool threadJoined; // True when thread is cleaned up.

    // Search will generate results as needed, up to maxSearchResults, and pause
    // until results are consumed.
    std::deque<RemoteFileSearchResult> searchResults;
    // Number of search results to buffer before waiting for results to be consumed.
    // Once consumed, further results will be generated up to maxGeneratedResults
    // even if the previous results have not yet been cleaned up.
    const u32 maxGeneratedResults;
    const bool recursive;

    // Same as searchFilenamePattern above except in lower-case.
    std::string lowerCaseSearchFilenamePattern;

    RemoteFileSearchUnindexedWin32(
            u64 searchQueueId,
            const std::string& searchFilenamePattern,
            const std::string& searchScopeDirectory,
            bool recursive,
            u64 maxGeneratedResults)
    :   threadEnded(false),
        threadJoined(false),
        maxGeneratedResults(maxGeneratedResults),
        recursive(recursive),
        RemoteFileSearchImpl(searchQueueId,
                             searchFilenamePattern,
                             searchScopeDirectory)
    {
        VPL_SET_UNINITIALIZED(&workToDoCondVar);
        lowerCaseSearchFilenamePattern = searchFilenamePattern;
        std::transform(lowerCaseSearchFilenamePattern.begin(),
                       lowerCaseSearchFilenamePattern.end(),
                       lowerCaseSearchFilenamePattern.begin(),
                       ::tolower);
    }
    friend RemoteFileSearch* CreateRemoteFileSearch(
                                        u64 assignedSearchQueueId,
                                        const std::string& searchFilenamePattern,
                                        const std::string& searchScopeDirectory,
                                        u64 maxGeneratedResults,
                                        bool disableIndex,
                                        bool recursive,
                                        int& errCode_out);
    ~RemoteFileSearchUnindexedWin32()
    {
        if (VPL_IS_INITIALIZED(&workToDoCondVar)) {
            VPLCond_Destroy(&workToDoCondVar);
        }
    }
protected:

    virtual int subclassInit()
    {
        int rv = 0;
        rv = VPLCond_Init(&workToDoCondVar);
        if (rv != 0) {
            LOG_ERROR("%p: VPLCond_Init failed: %d", this, rv);
            return rv;
        }

        rv = Util_SpawnThread(RemoteFileSearch_ThreadFn, this,
                UTIL_DEFAULT_THREAD_STACK_SIZE, VPL_TRUE, &workerSearchThread);
        if (rv != 0) {
            LOG_WARN("%p: Util_SpawnThread failed: %d", this, rv);
            threadEnded = true;
            threadJoined = true;
            return rv;
        }
        return rv;
    }

    static VPLTHREAD_FN_DECL RemoteFileSearch_ThreadFn(void* arg)
    {
        RemoteFileSearchUnindexedWin32* self = (RemoteFileSearchUnindexedWin32*)arg;
        LOG_INFO(FMTu64": Started worker thread.", self->GetSearchQueueId());
        self->workerSearchLoop();
        LOG_INFO(FMTu64": RemoteFileSearch thread exiting", self->GetSearchQueueId());
        return VPLTHREAD_RETURN_VALUE;
    }

    void workerSearchLoop()
    {
        // Traverse the file system.
        int rc = 0;

        {
            VPLFS_stat_t statBuf;
            rc = VPLFS_Stat(searchScopeDirectory.c_str(), &statBuf);
            if(rc != VPL_OK) {
                LOG_WARN("searchScopeDirectory(%s):%d",
                         searchScopeDirectory.c_str(), rc);
                return;
            }

            if(statBuf.type == VPLFS_TYPE_FILE) {
                std::string child = Util_getChild(searchScopeDirectory);
                std::string lowerCaseChild = child;
                std::transform(lowerCaseChild.begin(), lowerCaseChild.end(),
                               lowerCaseChild.begin(), ::tolower);
                if (doesStringMatchPattern(lowerCaseChild,
                                           lowerCaseSearchFilenamePattern))
                {
                    std::string winAbsPath = cleanupWindowsPath(searchScopeDirectory);
                    std::string remoteFileAbsPath;
                    HttpSvc::Sn::Handler_rf_Helper::winPathToRemoteFilePath(
                                                winAbsPath,
                                                /*OUT*/ remoteFileAbsPath);

                    std::string displayName;
                    rc = _VPLFS__LocalizedPath(winAbsPath, /*OUT*/ displayName);
                    if (rc != 0) {
                        LOG_WARN("_VPLFS__LocalizedPath(%s):%d", remoteFileAbsPath.c_str(), rc);
                    }

                    bool isShortcut = false;
                    std::string targetPath;
                    RFS_ShortcutDetails shortcut;
                    if (HttpSvc::Sn::Handler_rf_Helper::isShortcut(
                                        searchScopeDirectory, statBuf.type))
                    {
                        rc = _VPLFS__GetShortcutDetail(searchScopeDirectory,
                                                       /*OUT*/ targetPath,
                                                       /*OUT*/ shortcut.type,
                                                       /*OUT*/ shortcut.args);
                        if (rc != VPL_OK) {
                            // shortcut cannot be parsed correctly, ignore it
                            LOG_WARN("can't parse shortcut(%s):%d. Continuing.",
                                     searchScopeDirectory.c_str(), rc);
                        } else {
                            std::string winTargetPath = cleanupWindowsPath(targetPath);
                            isShortcut = true;
                            HttpSvc::Sn::Handler_rf_Helper::winPathToRemoteFilePath(
                                                        winTargetPath,
                                                        /*OUT*/ shortcut.path);
                            rc = _VPLFS__LocalizedPath(winTargetPath, /*OUT*/ shortcut.displayName);
                            if (rc != 0) {
                                LOG_WARN("_VPLFS__LocalizedPath(%s):%d", winTargetPath.c_str(), rc);
                            }
                        }
                    }

                    RemoteFileSearchResult result;
                    setRemoteFileSearchResult(remoteFileAbsPath,
                                              displayName,
                                              statBuf,
                                              isShortcut,
                                              shortcut,
                                              /*OUT*/ result);

                    MutexAutoLock lock(&mutex);
                    searchResults.push_back(result);
                }
                return;
            }
        }
        std::vector<std::string> dirPaths;
        dirPaths.push_back(searchScopeDirectory);

        while(!dirPaths.empty()) {
            VPLFS_dir_t dp;
            VPLFS_dirent_t dirp;
            std::string currDir(dirPaths.back());
            dirPaths.pop_back();

            if (closeRequested) {
                break;
            }

            if((rc = VPLFS_Opendir(currDir.c_str(), &dp)) != VPL_OK) {
                LOG_ERROR("VPLFS_Opendir(%s):%d", currDir.c_str(), rc);
                continue;
            }

            while(VPLFS_Readdir(&dp, &dirp) == VPL_OK) {
                std::string dirent(dirp.filename);
                std::string absFile;
                Util_appendToAbsPath(currDir, dirent, absFile);
                VPLFS_stat_t statBuf;

                if(dirent == "." || dirent == "..") {
                    continue;
                }

                if (closeRequested) {
                    break;
                }

                if((rc = VPLFS_Stat(absFile.c_str(), &statBuf)) != VPL_OK) {
                    LOG_ERROR("VPLFS_Stat(%s,%s),type(%d):%d",
                              currDir.c_str(), dirent.c_str(), (int)dirp.type, rc);
                    continue;
                }
                do {
                    std::string lowerCaseDirent = dirent;
                    std::transform(lowerCaseDirent.begin(), lowerCaseDirent.end(),
                                   lowerCaseDirent.begin(), ::tolower);
                    if (doesStringMatchPattern(lowerCaseDirent,
                                               lowerCaseSearchFilenamePattern))
                    {
                        std::string remoteFileAbsPath;
                        std::string winAbsFile = cleanupWindowsPath(absFile);
                        bool isShortcut = false;
                        std::string targetPath;
                        RFS_ShortcutDetails shortcut;

                        // Check if there's permission to return result.
                        rc = fs_dataset::checkAccessRightHelper(winAbsFile,
                                                                VPLFILE_CHECK_PERMISSION_READ);
                        if (rc != VPL_OK && rc != VPL_ERR_ACCESS) {
                            LOG_ERROR("checkAccessRightHelper(%s):%d",
                                      winAbsFile.c_str(), rc);
                            break;  // goto LABEL_SKIP_ENTRY_INSERTION
                        }
                        if (HttpSvc::Sn::Handler_rf_Helper::isShortcut(
                                                winAbsFile, statBuf.type))
                        {
                            rc = _VPLFS__GetShortcutDetail(winAbsFile,
                                                           /*OUT*/ targetPath,
                                                           /*OUT*/ shortcut.type,
                                                           /*OUT*/ shortcut.args);
                            if (rc != VPL_OK) {
                                // shortcut cannot be parsed correctly, ignore it
                                LOG_WARN("can't parse shortcut(%s):%d. Continuing.",
                                         winAbsFile.c_str(), rc);
                            } else {
                                isShortcut = true;
                                std::string winTargetPath = cleanupWindowsPath(targetPath);
                                HttpSvc::Sn::Handler_rf_Helper::winPathToRemoteFilePath(
                                                            winTargetPath,
                                                            /*OUT*/ shortcut.path);
                                rc = _VPLFS__LocalizedPath(winTargetPath, /*OUT*/ shortcut.displayName);
                                if (rc != 0) {
                                    LOG_WARN("_VPLFS__LocalizedPath(%s):%d", shortcut.path.c_str(), rc);
                                }
                            }
                        }

                        HttpSvc::Sn::Handler_rf_Helper::winPathToRemoteFilePath(
                                                    winAbsFile,
                                                    /*OUT*/ remoteFileAbsPath);

                        // TODO: Optimization: It would be more efficient to build
                        //   the display name along with the FSname, calling
                        //   _VPLFS__GetDisplayName rather than _VPLFS__LocalizedPath
                        std::string displayName;
                        rc = _VPLFS__LocalizedPath(winAbsFile, /*OUT*/ displayName);
                        if (rc != 0) {
                            LOG_WARN("_VPLFS__LocalizedPath(%s):%d", winAbsFile.c_str(), rc);
                        }

                        RemoteFileSearchResult result;
                        setRemoteFileSearchResult(remoteFileAbsPath,
                                                  displayName,
                                                  statBuf,
                                                  isShortcut,
                                                  shortcut,
                                                  /*OUT*/ result);

                        MutexAutoLock lock(&mutex);
                        u64 numStored = expectedIndex - previousIndex;
                        while (searchResults.size() - numStored >= maxGeneratedResults &&
                               !closeRequested)
                        {
                            rc = VPLCond_TimedWait(&workToDoCondVar, &mutex, VPL_TIMEOUT_NONE);
                            if ((rc != 0) && (rc != VPL_ERR_TIMEOUT)) {
                                LOG_ERROR("VPLCond_TimedWait:%d", rc);
                            }
                            numStored = expectedIndex - previousIndex;
                        }

                        searchResults.push_back(result);
                    }
                } while(false);  // LABEL_SKIP_ENTRY_INSERTION

                if (statBuf.type == VPLFS_TYPE_DIR) {
                    if (recursive) {
                        dirPaths.push_back(absFile);
                    } else {
                        // No need to traverse into subdirectories.
                    }
                }
            }
            VPLFS_Closedir(&dp);
        }
        threadEnded = true;
    }

    struct PatternStackArgs {
        std::string inputString;
        std::string pattern;
    };
    // Checks whether an arbitrary string matches a pattern with wildcards '*' and '?'
    // '*' - matches 0 or more characters
    // '?' - matches 1 arbitrary characters
    // Returns true if the string matches the pattern, returns false if the string
    // does not match the pattern.  Iterative version of a recursive function.
    static bool doesStringMatchPattern(const std::string& inputString,
                                       const std::string& pattern)
    {
        std::vector<PatternStackArgs> stackArgs;
        PatternStackArgs initial;
        initial.inputString.assign(inputString);
        initial.pattern.assign(pattern);
        stackArgs.push_back(initial);

        while (!stackArgs.empty()) {
            PatternStackArgs myArgs = stackArgs.back();
            stackArgs.pop_back();
            const char* myPattern = myArgs.pattern.c_str();
            const char* myInputString = myArgs.inputString.c_str();

            // If we reach at the end of both strings, we are done
            if (myPattern[0] == '\0' && myInputString[0] == '\0') {
                return true;
            }

            // Optimization: many '*' in a row is equivalent to 1 '*'
            if (myPattern[0] == '*' && myPattern[1] == '*') {
                PatternStackArgs newArgs;
                newArgs.inputString.assign(myInputString);
                newArgs.pattern.assign(myPattern+1);
                stackArgs.push_back(newArgs);
                continue;
            }

            // Optimization: once the pattern is '*', everything matches
            if (myPattern[0] == '*' && myPattern[1] == '\0') {
                return true;
            }

            // Make sure that characters after '*' are present in inputString
            if (myPattern[0] == '*' && myPattern[1] != '\0' && myInputString[0] == '\0') {
                continue;
            }

            // If the first string contains '?', or current characters of both
            // strings match
            if (myPattern[0] == '?' || myPattern[0] == myInputString[0]) {
                PatternStackArgs newArgs;
                newArgs.inputString.assign(myInputString+1);
                newArgs.pattern.assign(myPattern+1);
                stackArgs.push_back(newArgs);
                continue;
            }

            // If there is *, then there are two possibilities
            // a) consider current character of the inputString
            // b) ignore current character of the inputString.
            if (myPattern[0] == '*') {
                {
                    PatternStackArgs newArgs;
                    newArgs.inputString.assign(myInputString+1);
                    newArgs.pattern.assign(myPattern);
                    stackArgs.push_back(newArgs);
                }
                {
                    PatternStackArgs newArgs;
                    newArgs.inputString.assign(myInputString);
                    newArgs.pattern.assign(myPattern+1);
                    stackArgs.push_back(newArgs);
                }
                continue;
            }
        }
        return false;
    }

    virtual int GetResults(u64 startIndex,
                           u64 maxNumReturn,
                           std::vector<RemoteFileSearchResult>& results_out,
                           bool& searchOngoing_out)
    {
        results_out.clear();
        searchOngoing_out = false;

        MutexAutoLock lock(&mutex);
        if (closeRequested) {
            LOG_ERROR("%p:Search("FMTu64") already closed", this, searchQueueId);
            return RF_SEARCH_ERR_SEARCH_QUEUE_ID_INVALID;
        }

        // 1. Verify startIndex is appropriate
        if (startIndex < previousIndex) {
            LOG_ERROR("%p: startIndex went backwards("FMTu64"->"FMTu64", "FMTu64")",
                      this, previousIndex, startIndex, expectedIndex);
            return RF_SEARCH_ERR_START_INDEX_JUMPED_BACKWARDS;
        }
        if (startIndex != previousIndex && startIndex != expectedIndex) {
            LOG_ERROR("%p: startIndex has gaps ("FMTu64" is not "FMTu64" or "FMTu64")",
                      this, startIndex, previousIndex, expectedIndex);
            return RF_SEARCH_ERR_START_INDEX_JUMPED_FORWARDS;
        }

        if (startIndex == expectedIndex)
        {   // Clean up entries from the last request now that we have a
            // new current request.
            u64 numItemsToCleanUp = expectedIndex-previousIndex;
            for(;numItemsToCleanUp>0 && searchResults.size()>0;numItemsToCleanUp--) {
                searchResults.pop_front();
            }
            ASSERT(numItemsToCleanUp == 0);
        }
        // From here, any elements in the searchResults are valid to return
        int numReturned = 0;
        while (numReturned < maxNumReturn &&
               numReturned < searchResults.size())
        {
            results_out.push_back(searchResults[numReturned]);
            numReturned++;
        }
        if (!threadEnded || numReturned < searchResults.size()) {
            searchOngoing_out = true;
        }

        // Update tracking;
        if (startIndex == expectedIndex) {
            previousIndex = expectedIndex;
        }
        expectedIndex = previousIndex + numReturned;

        VPLCond_Signal(&workToDoCondVar);

        return 0;
    }

    virtual int RequestClose()
    {
        MutexAutoLock lock(&mutex);
        if (closeRequested) {
            // This warning could occur in normal operation if the user ended
            // the search after it was automatically requested to close (timed out),
            // but before the search was actually cleaned up.
            LOG_WARN("RequestClose() already called");
            return -1;
        }
        closeRequested = true;

        VPLCond_Signal(&workToDoCondVar);

        return 0;
    }

    virtual int Join()
    {
        int rv = 0;
        if (!threadJoined) {
            LOG_INFO("searchQueueId("FMTu64") Join", searchQueueId);
            int rc = VPLDetachableThread_Join(&workerSearchThread);
            LOG_INFO("searchQueueId("FMTu64") Join complete:%d", searchQueueId, rc);

            threadJoined = true;
        } else {
            LOG_ERROR("Join() already called.  Should never happen.");
        }
        return 0;
    }
};

