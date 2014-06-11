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

#include <windows.h>
#include <SearchAPI.h>
#include <atldbcli.h>

#include "scopeguard.hpp"

// Included for static functions
#include "HttpSvc_Sn_Handler_rf.hpp"
#include "fs_dataset.hpp"

#include "log.h"

#pragma comment (lib, "SearchSDK.lib")

class RemoteFileSearchIndexedWin32 : public RemoteFileSearchImpl
{
private:
    VPL_DISABLE_COPY_AND_ASSIGN(RemoteFileSearchIndexedWin32);
    // Direct binding to C++ class. This sample uses hardcoded parameters for SQL
    // string creation and binds to CMyAccessor class layout.
    class CMyAccessor
    {
    public:
        WCHAR _szItemPathDisplay[2048];
        WCHAR _szItemUrl[2048];
        __int64 _size;
        BEGIN_COLUMN_MAP(CMyAccessor)
            COLUMN_ENTRY(1, _szItemPathDisplay)
            COLUMN_ENTRY(2, _szItemUrl)
            COLUMN_ENTRY(3, _size)
        END_COLUMN_MAP()
    };

    // State to create an accessor into the index.
    bool init_cDataSource;
    CDataSource cDataSource;
    bool init_cSession;
    CSession cSession;
    bool init_cCommand;
    CCommand<CAccessor<CMyAccessor>, CRowset> cCommand;
    HRESULT getDbEntryErrCode;


    // Entries returned in the previous command.
    std::deque<RemoteFileSearchResult> returnedResults;

    RemoteFileSearchIndexedWin32(
                u64 searchQueueId,
                const std::string& searchFilenamePattern,
                const std::string& searchScopeDirectory)
    :   init_cDataSource(false),
        init_cSession(false),
        init_cCommand(false),
        getDbEntryErrCode(S_OK),
        RemoteFileSearchImpl(searchQueueId,
                             searchFilenamePattern,
                             searchScopeDirectory)
    {}

    // Allow the factory function to call the constructor
    friend RemoteFileSearch* CreateRemoteFileSearch(
            u64 assignedSearchQueueId,
            const std::string& searchFilenamePattern,
            const std::string& searchScopeDirectory,
            u64 maxGeneratedResults,
            bool disableIndex,
            bool recursive,
            int& errCode_out);

    virtual ~RemoteFileSearchIndexedWin32()
    {
        if (init_cCommand) {
            cCommand.Close();
            cCommand.ReleaseCommand();
            init_cCommand = false;
        }

        if (init_cSession) {
            cSession.Close();
            init_cSession = false;
        }

        if (init_cDataSource) {
            cDataSource.Close();
            init_cDataSource = false;
        }
    }

protected:
    virtual int subclassInit()
    {
        int rv = 0;

        // Set up the query for ISearchQueryHelper
        std::string queryString;

        queryString.append("\"");
        queryString.append(searchScopeDirectory);
        queryString.append("\"");

        queryString.append(" filename:\"");
        queryString.append(searchFilenamePattern);
        queryString.append("\"");

        // TODO: Clean up co-initialize gracefully, but it's not so simple:
        // http://msdn.microsoft.com/en-us/library/windows/desktop/ms695279(v=vs.85).aspx
        // This needs to be called once per thread.  Also it needs to be uninitialized once per thread.
        HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);
        if (hr != S_OK && hr != S_FALSE) {
            LOG_ERROR("CoInitializeEx(%s):%d", queryString.c_str(), hr);
            ReportHRESULTError(L"CoInitializeEx", hr);
            return hr;
        }

        wchar_t* utf16_queryString;
        rv = _VPL__utf8_to_wstring(queryString.c_str(), &utf16_queryString);
        if (rv != VPL_OK) {
            LOG_ERROR("_VPL__utf8_to_wstring(%s):%d", queryString.c_str(), rv);
            return rv;
        }
        ON_BLOCK_EXIT(free, utf16_queryString);

        PWSTR pszSQL;
        // This AQS will match all files named desktop.ini and return System.ItemPathDisplay and System.Size
        // which will be mapped to _szItemUrl and _size respectively
        hr = GetSQLStringFromParams(1033,  // unused
                                    L"",
                                    1033,  // unused
                                    -1,
                                    L"System.ItemPathDisplay, System.ItemUrl, System.Size",
                                    L"",
                                    SEARCH_ADVANCED_QUERY_SYNTAX,
                                    SEARCH_TERM_NO_EXPANSION,
                                    L"",
                                    utf16_queryString,
                                    &pszSQL);
        if (FAILED(hr)) {
            LOG_ERROR("GetSQLStringFromParams(%s):%d", queryString.c_str(), hr);
            ReportHRESULTError(L"GetSQLStringFromParams", hr);
            return hr;
        }
        ON_BLOCK_EXIT(CoTaskMemFree, pszSQL);

        char* utf8_pszSQL = NULL;
        int rc = _VPL__wstring_to_utf8_alloc(pszSQL, &utf8_pszSQL);
        if (rc != 0) {
            LOG_ERROR("_VPL__wstring_to_utf8_alloc:%d", rc);
            return rc;
        }
        ON_BLOCK_EXIT(free, utf8_pszSQL);

        LOG_INFO("%p: Query for searchQueueId("FMTu64",%s,%s):%s",
                 this, searchQueueId, searchScopeDirectory.c_str(),
                 searchFilenamePattern.c_str(), utf8_pszSQL);

        // InitializationString specified at:
        //    http://msdn.microsoft.com/en-us/library/windows/desktop/ff684395(v=vs.85).aspx
        hr = cDataSource.OpenFromInitializationString(L"provider=Search.CollatorDSO.1;EXTENDED?PROPERTIES=\"Application=Windows\"");
        if (FAILED(hr)) {
            LOG_ERROR("%p:OpenFromInitializationString:%d", this, hr);
            ReportHRESULTError(L"OpenFromInitializationString", hr);
            return hr;
        }
        init_cDataSource = true;

        hr = cSession.Open(cDataSource);
        if (FAILED(hr)) {
            LOG_ERROR("%p:cSession:%d", this, hr);
            ReportHRESULTError(L"cSession.Open", hr);
            return hr;
        }
        init_cSession = true;

        // cCommand is derived from CMyAccessor which has binding information in column map
        // This allows ATL to put data directly into appropriate class members.
        hr = cCommand.Open(cSession, pszSQL);
        if (FAILED(hr)) {
            LOG_ERROR("%p:cCommand:%d", this, hr);
            ReportHRESULTError(L"cSession.Open", hr);
            return hr;
        }
        init_cCommand = true;

        return 0;
    }

public:
    // Returns true if an index exists for the given path.
    // path is an absolute path windows path (with backslashes).
    // Example: "C:\\Users\\example\\one"
    static bool IndexExists(const std::string& windowsPath)
    {
        // * Should already be windows-style path.
        // * Aliases should already be replaced in the absolute path.
        bool pathIsIndexed = false;

        int rc = IsPathCrawledByIndexer(windowsPath,
                                        /*OUT*/ pathIsIndexed);
        if (rc != 0) {
            LOG_ERROR("IsPathCrawledByIndexer(%s):%d", windowsPath.c_str(), rc);
        }
        return pathIsIndexed;
    }

private:
    static int ReportHRESULTError(PCWSTR pszOpName, HRESULT hr)
    {
        int iErr = 0;

        if (FAILED(hr))
        {
            if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
            {
                iErr = HRESULT_CODE(hr);
                void *pMsgBuf;

                int rc = ::FormatMessageW(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL,
                    iErr,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    (PWSTR) &pMsgBuf,
                    0, NULL );
                if(rc == 0) {
                    LOG_ERROR("FormatMessageW(%d, %d)", hr, iErr);
                    return -1;
                }
                ON_BLOCK_EXIT(LocalFree, pMsgBuf);

                char* utf8_pszOpName = NULL;
                rc = _VPL__wstring_to_utf8_alloc(pszOpName, &utf8_pszOpName);
                if (rc != 0) {
                    LOG_ERROR("_VPL__wstring_to_utf8_alloc:%d", rc);
                }
                char* utf8_pMsgBuf = NULL;
                rc = _VPL__wstring_to_utf8_alloc((PWSTR)pMsgBuf, &utf8_pMsgBuf);
                if (rc != 0) {
                    LOG_ERROR("_VPL__wstring_to_utf8_alloc:%d", rc);
                }

                // Success at logging error!
                LOG_ERROR("Error:%s failed with error(%d, %s)",
                          utf8_pszOpName==NULL?"":utf8_pszOpName,
                          iErr,
                          utf8_pMsgBuf==NULL?"":utf8_pMsgBuf);
                if (utf8_pszOpName != NULL) { free(utf8_pszOpName); }
                if (utf8_pMsgBuf != NULL) {free(utf8_pMsgBuf); }
            }
            else
            {
                char* utf8_pszOpName = NULL;
                int rc = _VPL__wstring_to_utf8_alloc(pszOpName, &utf8_pszOpName);
                if (rc != 0) {
                    LOG_ERROR("_VPL__wstring_to_utf8_alloc:%d", rc);
                    iErr = rc;
                } else {
                    // Success at logging error!
                    LOG_ERROR("Error:%s failed with error "FMTx32, utf8_pszOpName, hr);
                    free(utf8_pszOpName);
                }
            }
        }

        return iErr;
    }

    static std::string stripFileColonStrFromUrl(const std::string& itemUrl)
    {
        std::string fileColon("file:");
        if (itemUrl.find(fileColon)==0) {
            return itemUrl.substr(fileColon.size()); // Get from end of "file:" to end.
        }
        return itemUrl;
    }

    //=================================================================
    //======================== Index Exists ===========================
    // Modeled after CrawlScopeCommandLine (csmcmd)
    //    http://msdn.microsoft.com/en-us/library/windows/desktop/dd940334(v=vs.85).aspx

    static HRESULT CreateCatalogManager(ISearchCatalogManager **ppSearchCatalogManager)
    {
        *ppSearchCatalogManager = NULL;

        ISearchManager *pSearchManager;
        HRESULT hr = CoCreateInstance(CLSID_CSearchManager,
                                      NULL,
                                      CLSCTX_SERVER,
                                      IID_PPV_ARGS(&pSearchManager));
        if (SUCCEEDED(hr))
        {
            hr = pSearchManager->GetCatalog(L"SystemIndex", ppSearchCatalogManager);
            pSearchManager->Release();
        }
        return hr;
    }

    static HRESULT CreateCrawlScopeManager(ISearchCrawlScopeManager **ppSearchCrawlScopeManager)
    {
        *ppSearchCrawlScopeManager = NULL;

        ISearchCatalogManager *pCatalogManager;
        HRESULT hr = CreateCatalogManager(&pCatalogManager);
        if (SUCCEEDED(hr))
        {
            // Crawl scope manager for that catalog
            hr = pCatalogManager->GetCrawlScopeManager(ppSearchCrawlScopeManager);
            pCatalogManager->Release();
        }
        return hr;
    }

    static int IsPathCrawledByIndexer(const std::string& windowsPath,
                                      bool& pathIsIndexed_out)
    {
        pathIsIndexed_out = false;
        wchar_t* u16_windowsPath = NULL;
        int rc = _VPL__utf8_to_wstring(windowsPath.c_str(), &u16_windowsPath);
        if(rc != 0) {
            LOG_ERROR("_VPL__utf8_to_wstring(%s):%d", windowsPath.c_str(), rc);
            return rc;
        }

        // Crawl scope manager for that catalog
        ISearchCrawlScopeManager *pSearchCrawlScopeManager;
        HRESULT hr = CreateCrawlScopeManager(&pSearchCrawlScopeManager);
        if (SUCCEEDED(hr))
        {
            BOOL answer = FALSE;
            hr = pSearchCrawlScopeManager->IncludedInCrawlScope(u16_windowsPath, &answer);
            if (SUCCEEDED(hr)) {
                if (answer == TRUE) {
                    pathIsIndexed_out = true;
                }
            } else {
                LOG_ERROR("IncludedInCrawlScope(%s):%d", windowsPath.c_str(), hr);
            }

            pSearchCrawlScopeManager->Release();
        } else {
            LOG_ERROR("CreateCrawlScopeManager:%d", hr);
        }

        free(u16_windowsPath);

        return ReportHRESULTError(L"IsPathCrawledByIndexer", hr);
    }
    //======================== Index Exists ===========================
    //=================================================================

    //=======================================================================
    //============================ Perform Query ============================
    // Modeled after WSOleDB example program
    //   http://msdn.microsoft.com/en-us/library/windows/desktop/dd940342(v=vs.85).aspx

    // This helper function creates SQL string using query helper out of parameters
    HRESULT GetSQLStringFromParams(LCID lcidContentLocaleParam,
                                   PCWSTR pszContentPropertiesParam,
                                   LCID lcidKeywordLocaleParam,
                                   LONG nMaxResultsParam,
                                   PCWSTR pszSelectColumnsParam,
                                   PCWSTR pszSortingParam,
                                   SEARCH_QUERY_SYNTAX sqsSyntaxParam,
                                   SEARCH_TERM_EXPANSION steTermExpansionParam,
                                   PCWSTR pszWhereRestrictionsParam,
                                   PCWSTR pszExprParam,
                                   PWSTR *ppszSQL)
    {
        ISearchQueryHelper *pQueryHelper;

        // Create an instance of the search manager
        ISearchManager *pSearchManager;
        HRESULT hr = CoCreateInstance(__uuidof(CSearchManager),
                                      NULL,
                                      CLSCTX_LOCAL_SERVER,
                                      IID_PPV_ARGS(&pSearchManager));
        if (SUCCEEDED(hr))
        {
            // Get the catalog manager from the search manager
            ISearchCatalogManager *pSearchCatalogManager;
            hr = pSearchManager->GetCatalog(L"SystemIndex", &pSearchCatalogManager);
            if (SUCCEEDED(hr))
            {
                // Get the query helper from the catalog manager
                hr = pSearchCatalogManager->GetQueryHelper(&pQueryHelper);
                if (SUCCEEDED(hr))
                {
#ifdef CHANGE_FROM_DEFAULT_LOCALE
                    // No need to set the content locale, just use the existing default.
                    hr = pQueryHelper->put_QueryContentLocale(lcidContentLocaleParam);
                    if (SUCCEEDED(hr))
                    {
#endif
                        hr = pQueryHelper->put_QueryContentProperties(pszContentPropertiesParam);
#ifdef CHANGE_FROM_DEFAULT_LOCALE
                    }
                    // No need to set the keyword locale, just use the existing default.
                    if (SUCCEEDED(hr))
                    {
                        hr = pQueryHelper->put_QueryKeywordLocale(lcidKeywordLocaleParam);
                    }
#endif
                    if (SUCCEEDED(hr))
                    {
                        hr = pQueryHelper->put_QueryMaxResults(nMaxResultsParam);
                    }
                    if (SUCCEEDED(hr))
                    {
                        hr = pQueryHelper->put_QuerySelectColumns(pszSelectColumnsParam);
                    }
                    if (SUCCEEDED(hr))
                    {
                        hr = pQueryHelper->put_QuerySorting(pszSortingParam);
                    }
                    if (SUCCEEDED(hr))
                    {
                        hr = pQueryHelper->put_QuerySyntax(sqsSyntaxParam);
                    }
                    if (SUCCEEDED(hr))
                    {
                        hr = pQueryHelper->put_QueryTermExpansion(steTermExpansionParam);
                    }
                    if (SUCCEEDED(hr))
                    {
                        hr = pQueryHelper->put_QueryWhereRestrictions(pszWhereRestrictionsParam);
                    }
                    if (SUCCEEDED(hr))
                    {
                        hr = pQueryHelper->GenerateSQLFromUserQuery(pszExprParam, ppszSQL);
                    }
                    pQueryHelper->Release();
                }
                pSearchCatalogManager->Release();
            }
            pSearchManager->Release();
        }
        return hr;
    }

protected:
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

        // Initialize the command (just at the beginning of each search)
        if (startIndex == 0 && expectedIndex == 0 && previousIndex == 0) {
            getDbEntryErrCode = cCommand.MoveFirst();
        }

        if (startIndex == expectedIndex)
        {   // Clean up entries from the last request now that we have a
            // new current request.
            u64 numItemsToCleanUp = expectedIndex-previousIndex;
            for(;numItemsToCleanUp>0 && returnedResults.size()>0;numItemsToCleanUp--) {
                returnedResults.pop_front();
            }
            ASSERT(numItemsToCleanUp == 0);
        }
        // From here, any elements in the returnedResults are valid to return

        // Get results from previously returned results, if any.
        int numReturned = 0;
        while (numReturned < maxNumReturn &&
               numReturned < returnedResults.size())
        {
            results_out.push_back(returnedResults[numReturned]);
            numReturned++;
        }

        // Get more results from the index db.
        for (;S_OK == getDbEntryErrCode && numReturned<maxNumReturn;
             getDbEntryErrCode = cCommand.MoveNext())
        {
            char* utf8_itemUrl;
            int rc = _VPL__wstring_to_utf8_alloc(cCommand._szItemUrl, &utf8_itemUrl);
            if (rc != 0) {
                LOG_ERROR("%p:_VPL__wstring_to_utf8_alloc:%d, Skipping", this, rc);
                continue;
            }
            ON_BLOCK_EXIT(free, utf8_itemUrl);

            char* utf8_itemPathDisplay;
            rc = _VPL__wstring_to_utf8_alloc(cCommand._szItemPathDisplay,
                                             &utf8_itemPathDisplay);
            if (rc != 0) {
                LOG_ERROR("%p:_VPL__wstring_to_utf8_alloc for (%s):%d, Not critical, continuing",
                          this, utf8_itemUrl, rc);
            }
            ON_BLOCK_EXIT(free, utf8_itemPathDisplay);

            std::string itemUrl =  stripFileColonStrFromUrl(std::string(utf8_itemUrl));

            // Check if there's permission to return result.
            rc = fs_dataset::checkAccessRightHelper(itemUrl.c_str(),
                                                    VPLFILE_CHECK_PERMISSION_READ);
            if (rc != VPL_OK && rc != VPL_ERR_ACCESS) {
                LOG_ERROR("checkAccessRightHelper(%s):%d", itemUrl.c_str(), rc);
                continue;
            }
            VPLFS_stat_t statBuf;
            rc = VPLFS_Stat(itemUrl.c_str(), /*OUT*/ &statBuf);
            if (rc != VPL_OK) {
                LOG_ERROR("VPLFS_Stat(%s):%d", itemUrl.c_str(), rc);
                continue;
            }

            std::string remoteFileAbsPath;
            HttpSvc::Sn::Handler_rf_Helper::winPathToRemoteFilePath(
                                                itemUrl,
                                                /*OUT*/ remoteFileAbsPath);
            std::string displayName(utf8_itemPathDisplay);

            bool isShortcut = false;
            std::string targetPath;
            RFS_ShortcutDetails shortcut;
            if (HttpSvc::Sn::Handler_rf_Helper::isShortcut(itemUrl,
                                                           statBuf.type))
            {
                rc = _VPLFS__GetShortcutDetail(itemUrl,
                                               /*OUT*/ targetPath,
                                               /*OUT*/ shortcut.type,
                                               /*OUT*/ shortcut.args);
                if (rc != VPL_OK) {
                    // shortcut cannot be parsed correctly, ignore it
                    LOG_WARN("can't parse shortcut(%s):%d. Continuing.",
                             itemUrl, rc);
                } else {
                    isShortcut = true;
                    std::string winTargetPath = cleanupWindowsPath(targetPath);
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

            results_out.push_back(result);

            returnedResults.push_back(result);
            numReturned++;
        }

        if (getDbEntryErrCode == S_OK) {
            searchOngoing_out = true;
        }

        // Update tracking;
        if (startIndex == expectedIndex) {
            previousIndex = expectedIndex;
        }
        expectedIndex = previousIndex + numReturned;

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

        return 0;
    }

    virtual int Join()
    {
        // No thread to join here.
        return 0;
    }
};
