/*
*  Copyright 2010 iGware Inc.
*  All Rights Reserved.
*
*  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
*  TRADE SECRETS OF IGWARE INC.
*  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
*  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
*
*/

#include "vpl_fs.h"
#include "vpl_lazy_init.h"
#include "vpl_th.h"
#include "vplu_debug.h"
#include "vplu_format.h"
#include "vplu_missing.h"

#include <stdlib.h>
#include <tchar.h>
#include <winbase.h>

#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include <thread>

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Storage;
using namespace Windows::Storage::Search;
using namespace Windows::Storage::FileProperties;

#define ALIGN(x) (((x+(sizeof(int)-1))/sizeof(int))*sizeof(int))

static VPLLazyInitMutex_t g_monitorDirMutex = VPLLAZYINITMUTEX_INIT;

#ifdef ANDROID
// On Android, res_queryN() (which is used within getaddrinfo()) declares a 64K buffer on the stack!
#  define MONITOR_DIR_THREAD_STACK_SIZE  (192 * 1024)
#else
#  define MONITOR_DIR_THREAD_STACK_SIZE  (16 * 1024)
#endif

static bool g_isInit = false;

struct VPLFS_MonitorHandle_t
{
    int internalEvents;
    bool has_rename_old_name;
    char rename_old_name[MAX_PATH];
    bool stop;
    VPLFS_MonitorCallback cb;
    VPLFS_MonitorEvent* cbReturnBuf;
    DWORD nBufferLength;
    char* pBuffer;
};

struct _ItemData
{
    StorageItemTypes type;
    std::wstring path;
    long long dateModified;
    unsigned long long size;
};

enum FILE_ACTION {
    ACTION_ADDED = 0,
    ACTION_REMOVED = 1,
    ACTION_MODIFIED = 2,
};

struct _EventData
{
    std::wstring path;
    FILE_ACTION action;
};

//----------------------------------------------------------------------------------
// Internal functions

static void replaceBackslashWithForwardSlash(std::string& myPath_in_out)
{
    size_t backslashIndex = myPath_in_out.find("\\");
    while(backslashIndex != std::string::npos) {
        myPath_in_out.replace(backslashIndex, 1, "/" );
        backslashIndex = myPath_in_out.find("\\", backslashIndex + 1);
    }
}

static void setCommon(VPLFS_MonitorEvent* cbBuffer, const char* filename)
{
    cbBuffer->moveTo = NULL;
    std::string strFilename(filename);
    replaceBackslashWithForwardSlash(strFilename);
    strncpy((char*)(cbBuffer+1), strFilename.c_str(), MAX_PATH);
    int nameLen = strFilename.size()+1;  // +1 for '\0'
    cbBuffer->filename = (char*)(cbBuffer+1);
    cbBuffer->nextEntryOffsetBytes = sizeof(VPLFS_MonitorEvent)+ALIGN(nameLen);
}

static bool isBeginWithPeriod(const char* filename)
{
    std::string strFilename(filename);
    if(strFilename.size()>0 && strFilename[0]=='.') {
        return true;
    }
    if(strFilename.find("\\.") != std::string::npos) {  // subdirectory is hidden
        return true;
    }

    return false;
}

static std::wstring ReplaceWString( const std::wstring& orignStr, const std::wstring& oldStr, const std::wstring& newStr ) 
{ 
    size_t pos = 0; 
    std::wstring tempStr = orignStr; 
    std::wstring::size_type newStrLen = newStr.length(); 
    std::wstring::size_type oldStrLen = oldStr.length(); 
    while(true) 
    { 
        pos = tempStr.find(oldStr, pos); 
        if (pos == std::wstring::npos) break;
        tempStr.replace(pos, oldStrLen, newStr);
        pos += newStrLen;
    }
    return tempStr; 
}

//----------------------------------------------------------------------------------
// _MonitorData

ref class _MonitorData sealed
{
private:
    StorageItemQueryResult^ m_monitorQuery;
    volatile bool m_bIsBrowsing;
    volatile bool m_bHasNewJobWhenBroswing;
    VPLMutex_t m_CacheMutex;

internal:
    VPLFS_MonitorHandle_t* m_monitorHandle;

private:
    std::map<std::wstring,_ItemData> m_Cache;
    std::wstring m_MonitorPath;

public:
    int StartMonitor();

    static void ClearMonitorMap();

internal:
    _MonitorData(const wchar_t* folderPath);
    void SetMonitorHandle(VPLFS_MonitorHandle_t* handle);

private:
    ~_MonitorData();

    void SetStartBrowsingFlag(bool browsing);
    void InsertToCache(IStorageItem^ item);
    void GetDateModifiedAndSize(IStorageItem^ item, long long &dateModified, unsigned long long &size);

    void OnFileChanged(IStorageQueryResultBase^ base, Object^ e);
    void GetAllFiles(StorageFolder^ monitorFolder);
    void DiffWithCache(std::map<std::wstring,_ItemData> currentItems);
    void LaunchFileChangeEvent(std::vector<_EventData> events);

    _ItemData GetItemData(IStorageItem^ item);
    void ClearCache();

};

//----------------------------------------------------------------------------------
// _MonitorData Implementation

std::map<VPLFS_MonitorHandle_t*,_MonitorData^> m_MonitorMap;

_MonitorData::_MonitorData(const wchar_t* folderPath)
    : m_monitorQuery(nullptr),
    m_monitorHandle(NULL),
    m_bIsBrowsing(false),
    m_bHasNewJobWhenBroswing(false),
    m_MonitorPath(L"")
{
    std::wstring tmp(folderPath);
    m_MonitorPath = ReplaceWString(tmp,L"/",L"\\");
    VPLMutex_Init(&m_CacheMutex);
}

_MonitorData::~_MonitorData()
{
    VPLMutex_Unlock(&m_CacheMutex);
    VPLMutex_Destroy(&m_CacheMutex);

    ClearCache();

    if( m_monitorQuery != nullptr )
        delete m_monitorQuery;
}

void _MonitorData::SetMonitorHandle(VPLFS_MonitorHandle_t* handle)
{
    m_monitorHandle = handle;
}

void _MonitorData::SetStartBrowsingFlag(bool browsing)
{
    if(browsing) {
        this->m_bIsBrowsing = true;
        this->m_bHasNewJobWhenBroswing = false;
    }
    else {
        this->m_bIsBrowsing = false;
        //if the is new job (change) when last broswing -> we broswe monitor folder again
        if(m_bHasNewJobWhenBroswing) {
            String^ szDir = ref new String(m_MonitorPath.c_str());
            IAsyncOperation<StorageFolder^>^ getFolderOP = nullptr;
            try {
                getFolderOP = StorageFolder::GetFolderFromPathAsync(szDir);
            }
            catch(Exception^ exception) {
                return;
            }

            getFolderOP->Completed = ref new AsyncOperationCompletedHandler<StorageFolder^>(
                [this] (IAsyncOperation<StorageFolder^>^ op, AsyncStatus status) {
                    try {
                        this->GetAllFiles(op->GetResults());
                    }
                    catch(Exception^ e) {
                        auto msg = e->Message;
                    }
                }
            );
        }
    }
}

int _MonitorData::StartMonitor()
{
    int ret = VPL_OK;
    String^ szDir = ref new String(m_MonitorPath.c_str());
    IAsyncOperation<StorageFolder^>^ getFolderOP = nullptr;
    try{
        getFolderOP = StorageFolder::GetFolderFromPathAsync(szDir);
    }
    catch(Exception^ exception) {
        return VPL_ERR_ACCESS;
    }

    if(getFolderOP->Status == AsyncStatus::Error) {
        return VPL_ERR_ACCESS;
    }

    getFolderOP->Completed = ref new AsyncOperationCompletedHandler<StorageFolder^>(
        [this] (IAsyncOperation<StorageFolder^>^ op, AsyncStatus status) {
            try {
                StorageFolder^ folder = op->GetResults();
                auto queryOptions = ref new QueryOptions(Windows::Storage::Search::CommonFileQuery::OrderByName, nullptr);
                queryOptions->FolderDepth = FolderDepth::Deep;

                m_monitorQuery = folder->CreateItemQueryWithOptions(queryOptions);
                m_monitorQuery->ContentsChanged += ref new TypedEventHandler<IStorageQueryResultBase^, Object^>(this, &_MonitorData::OnFileChanged);

                SetStartBrowsingFlag(true);
                IAsyncOperation<IVectorView<IStorageItem^>^>^ action = m_monitorQuery->GetItemsAsync();
                action->Completed = ref new AsyncOperationCompletedHandler<IVectorView<IStorageItem^>^>(
                    [this] (IAsyncOperation<IVectorView<IStorageItem^>^>^ op, AsyncStatus status) {
                        try {
                            IVectorView<IStorageItem^>^ items = op->GetResults();
                            //get all files/folders properties
                            for(int i=0 ; i<items->Size ; i++) {
                                IStorageItem^ item = items->GetAt(i);
                                if( item->IsOfType(StorageItemTypes::Folder) || item->IsOfType(StorageItemTypes::File) ) {
                                    InsertToCache(item);
                                }
                            }
                            SetStartBrowsingFlag(false);
                        }
                        catch(Exception^ e) {
                            auto msg = e->Message;
                        }
                    }
                );
            }
            catch(Exception^ e) {
                auto msg = e->Message;
            }
        }
    );

    return VPL_OK;
}

void _MonitorData::InsertToCache(IStorageItem^ item)
{
    _ItemData data = GetItemData(item);
    VPLMutex_Lock(&m_CacheMutex);
    m_Cache.insert( std::pair<std::wstring,_ItemData>(data.path,data) );
    VPLMutex_Unlock(&m_CacheMutex);
}

_ItemData _MonitorData::GetItemData(IStorageItem^ item)
{
    _ItemData data;
    //get last write time & file size
    GetDateModifiedAndSize(item, data.dateModified, data.size);    
    //file path
    data.path = item->Path->Data();
    //file type (file/folder)
    if( item->IsOfType(StorageItemTypes::Folder) ) {
        data.type = StorageItemTypes::Folder;
    }
    else if(item->IsOfType(StorageItemTypes::File)) {
        data.type = StorageItemTypes::File;
    }

    return data;
}

void _MonitorData::GetDateModifiedAndSize(IStorageItem^ item, long long &dateModified, unsigned long long &size)
{
    HANDLE completedEvent = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
    if( !completedEvent ) {
        return ;
    }

    IAsyncOperation<BasicProperties^>^ getPropertyAction = item->GetBasicPropertiesAsync();
    getPropertyAction->Completed = ref new AsyncOperationCompletedHandler<BasicProperties^>(
        [this,item,&completedEvent,&dateModified,&size] (IAsyncOperation<BasicProperties^>^ op, AsyncStatus status) {
            try {
                BasicProperties^ prop = op->GetResults();
                dateModified = prop->DateModified.UniversalTime;
                size = prop->Size;
            }
            catch(Exception^ e) {
                auto msg = e->Message;
                dateModified = 0;
                size = 0;
            }
            SetEvent(completedEvent);
        }
    );

    WaitForSingleObjectEx(completedEvent ,INFINITE, TRUE);
    CloseHandle(completedEvent);
}

void _MonitorData::OnFileChanged(Windows::Storage::Search::IStorageQueryResultBase^ base, Platform::Object^ e)
{
    StorageFolder^ folder = base->Folder;
    //if lase handle not finish yet -> store the job and handle it later
    if(m_bIsBrowsing) {
        m_bHasNewJobWhenBroswing = true;
    }
    //no un-finished job -> get all files and diff
    else {
        //retrieve all files/folders in monitor folder
        GetAllFiles(base->Folder);
    }
}

void _MonitorData::GetAllFiles(StorageFolder^ monitorFolder)
{
    std::map<std::wstring,_ItemData>* currentItems = new std::map<std::wstring,_ItemData>();

    auto queryOptions = ref new QueryOptions(Windows::Storage::Search::CommonFileQuery::OrderByName, nullptr);
    queryOptions->FolderDepth = FolderDepth::Deep;

    StorageItemQueryResult^ monitorQuery = monitorFolder->CreateItemQueryWithOptions(queryOptions);
    IAsyncOperation<IVectorView<IStorageItem^>^>^ op = nullptr;
    try{
        op = monitorQuery->GetItemsAsync();
    }
    catch(Exception^ e) {
        return;
    }
    if(op->Status == AsyncStatus::Error) {
        return;
    }

    SetStartBrowsingFlag(true);
    op->Completed = ref new AsyncOperationCompletedHandler<IVectorView<IStorageItem^>^>(
        [this, currentItems] (IAsyncOperation<IVectorView<IStorageItem^>^>^ op, AsyncStatus status) {
            try {
                IVectorView<IStorageItem^>^ items = op->GetResults();
                //get all files/folders properties
                for(int i=0 ; i<items->Size ; i++) {
                    IStorageItem^ item = items->GetAt(i);
                    if( item->IsOfType(StorageItemTypes::Folder) || item->IsOfType(StorageItemTypes::File) ) {
                        _ItemData data = GetItemData(item);
                        currentItems->insert( std::pair<std::wstring,_ItemData>(data.path,data) );
                    }
                }
                //diff with cache data last time
                DiffWithCache(*currentItems);

                currentItems->clear();
                delete currentItems;

                //restore browsing flag (finished)
                SetStartBrowsingFlag(false);
                
            }
            catch(Exception^ e) {
                auto msg = e->Message;
            }
        }
    );
}

void _MonitorData::DiffWithCache(std::map<std::wstring,_ItemData> currentItems)
{
    std::vector<_EventData> events;

    VPLMutex_Lock(&m_CacheMutex);

    for (std::map<std::wstring,_ItemData>::iterator it = currentItems.begin(); it != currentItems.end(); ++it) {
        //try find the item in cache
        // if not found -> the item is new added
        _EventData ed;
        if( m_Cache.size() == 0 || m_Cache.count(it->first) == 0) {
            ed.path = it->first;
            ed.action = ACTION_ADDED;
            events.push_back(ed);
        }
        // if found -> check if any modification
        else {
            _ItemData old = m_Cache.find(it->first)->second;
            //ignore changes of "folder" (same behavior as Win32 API)
            if(old.type == StorageItemTypes::File) {
                if(old.dateModified < it->second.dateModified || old.size != it->second.size) {
                    ed.path = it->first;
                    ed.action = ACTION_MODIFIED;
                    events.push_back(ed);
                }
            }
            m_Cache.erase(it->first);
        }
    }

    std::vector<std::wstring> removedFolders;
    //if data still leave in m_Cache, it means these files are deleted
    if(m_Cache.size() > 0) {
        for (std::map<std::wstring,_ItemData>::iterator it = m_Cache.begin(); it != m_Cache.end(); ++it) {
            _EventData ed;
            if(it->second.type == StorageItemTypes::Folder) { //if folder removed -> store in removedFolders
                removedFolders.push_back(it->first);
                ed.path = it->first;
                ed.action = ACTION_REMOVED;
                events.push_back(ed);
            }
            else if (it->second.type == StorageItemTypes::File) {   //if file removed -> check if the file is in removed folder
                bool isFound = false;                               // if yes, do not put into file change events
                for(int i=0 ; i<removedFolders.size() ; i++) {
                    if( it->first.find(removedFolders.at(i)) >= 0 ) {
                        isFound = true;
                        break;
                    }
                }
                if( !isFound ) {
                    ed.path = it->first;
                    ed.action = ACTION_REMOVED;
                    events.push_back(ed);
                }
            }
        }
    }

    //clear old cache 
    m_Cache.clear();
    m_Cache.insert(currentItems.begin(),currentItems.end());

    VPLMutex_Unlock(&m_CacheMutex);

    //if diff with last cache but no change -> return
    if( events.size() == 0)
        return;

    //launch file change events
    if( events.size() > m_monitorHandle->internalEvents) { //if we get more events then user buffer -> seperate the events
        int numEvents = m_monitorHandle->internalEvents;
        int copyIt = 0;
        std::vector<_EventData> subevents;
        bool bContinue = true;
        while( bContinue ) {
            if( (copyIt+1)*numEvents > events.size() ) {
                subevents.resize( events.size() - (copyIt*numEvents) );
                std::copy( events.begin()+copyIt*numEvents, events.end(), subevents.begin() );
                bContinue = false;
            }
            else {
                subevents.resize(numEvents);
                std::copy( events.begin()+copyIt*numEvents, events.begin()+ (copyIt+1)*numEvents, subevents.begin() );
                if((copyIt+1)*numEvents == events.size()) //all events are launched
                    bContinue = false;
            }

            LaunchFileChangeEvent(subevents);
            copyIt++;
        }
    }
    else {
        LaunchFileChangeEvent(events);
    }
}

void _MonitorData::LaunchFileChangeEvent(std::vector<_EventData> events)
{
    //VPL_REPORT_INFO("ChDirCompletionRoutine");
    int rc;
    DWORD dwOffset = 0;
    DWORD nextEntryOffset = 1;
    VPLFS_MonitorEvent* cbBuffer = (VPLFS_MonitorEvent*)(m_monitorHandle->pBuffer+m_monitorHandle->nBufferLength);
    u32 previousEntry = 0;
    void* eventBuffer = cbBuffer;
    int eventBufferSize = 0;

    for(int i=0 ; i<events.size() ; i++) {
        _EventData ed = events.at(i);
        //vpl file monitor event should only return relative path to original monitor path
        //so we replace to relative here
        std::wstring relativePath = ReplaceWString(ed.path,m_MonitorPath,L"");
        char filename[MAX_PATH];
        int temp_rc = _VPL__wstring_to_utf8(relativePath.c_str(),
            ed.path.length(),
            filename, ARRAY_SIZE_IN_BYTES(filename));
        if(temp_rc != 0) {
            VPL_REPORT_WARN("%s failed: %d", "_VPL__wstring_to_utf8", temp_rc);
            // TODO: skip the rest of this loop?
        }

        if(!isBeginWithPeriod(filename))
        {   // If the file is not considered hidden (begins with a period.
            //VPL_REPORT_INFO("Filename:%s, FilenameLength:"FMT_DWORD", action:"FMT_DWORD", nextEntry:"FMT_DWORD,
            //                filename, pInfo->FileNameLength, pInfo->Action, pInfo->NextEntryOffset);

            switch(ed.action) {
            case ACTION_ADDED:
                {
                    //VPL_REPORT_INFO("FILE_ACTION_ADDED: %s", filename);
                    cbBuffer->action = VPLFS_MonitorEvent_FILE_ADDED;
                    m_monitorHandle->has_rename_old_name = false;
                    setCommon(cbBuffer, filename);
                    previousEntry = cbBuffer->nextEntryOffsetBytes;
                    eventBufferSize += cbBuffer->nextEntryOffsetBytes;
                    cbBuffer = (VPLFS_MonitorEvent*)(((char*)(cbBuffer))+cbBuffer->nextEntryOffsetBytes);
                }break;
            case ACTION_REMOVED:
                {
                    //VPL_REPORT_INFO("FILE_ACTION_REMOVED: %s", filename);
                    cbBuffer->action = VPLFS_MonitorEvent_FILE_REMOVED;
                    m_monitorHandle->has_rename_old_name = false;
                    setCommon(cbBuffer, filename);
                    previousEntry = cbBuffer->nextEntryOffsetBytes;
                    eventBufferSize += cbBuffer->nextEntryOffsetBytes;
                    cbBuffer = (VPLFS_MonitorEvent*)(((char*)(cbBuffer))+cbBuffer->nextEntryOffsetBytes);
                }break;
            case ACTION_MODIFIED:
                {
                    //VPL_REPORT_INFO("FILE_ACTION_MODIFIED: %s", filename);
                    cbBuffer->action = VPLFS_MonitorEvent_FILE_MODIFIED;
                    m_monitorHandle->has_rename_old_name = false;
                    setCommon(cbBuffer, filename);
                    previousEntry = cbBuffer->nextEntryOffsetBytes;
                    eventBufferSize += cbBuffer->nextEntryOffsetBytes;
                    cbBuffer = (VPLFS_MonitorEvent*)(((char*)(cbBuffer))+cbBuffer->nextEntryOffsetBytes);
                }break;
            default:
                VPL_REPORT_WARN("Unhandled action:"FMT_DWORD", %s", ed.action, filename);
                break;
            }
        }


    }

    cbBuffer = (VPLFS_MonitorEvent*)(((char*)cbBuffer)- previousEntry);
    cbBuffer->nextEntryOffsetBytes = 0;

    m_monitorHandle->cb(m_monitorHandle,
        eventBuffer,
        eventBufferSize,
        VPLFS_MonitorCB_OK);
}

void _MonitorData::ClearCache()
{
    // TODO: free anything required
}

void _MonitorData::ClearMonitorMap()
{
    if( m_MonitorMap.size() > 0) {
        for (std::map<VPLFS_MonitorHandle_t*,_MonitorData^>::iterator it = m_MonitorMap.begin(); it != m_MonitorMap.end(); ++it) {
            _MonitorData^ data = it->second;
            if( data != nullptr)
                delete data;
        }
    }
    m_MonitorMap.clear();
}

_MonitorData^ GetMonitorData(VPLFS_MonitorHandle_t* handle)
{
    _MonitorData^ ret = nullptr;
    if( m_MonitorMap.count(handle) > 0) {
        ret = m_MonitorMap.find(handle)->second;
    }
    return ret;
}

bool RemoveMonitorData(VPLFS_MonitorHandle_t* handle)
{
    bool ret = false;
    if (handle == NULL)
        ret = false;
    else {
        if( m_MonitorMap.count(handle) > 0) {
            _MonitorData^ data = m_MonitorMap.find(handle)->second;
            m_MonitorMap.erase(handle);
            if(data != nullptr)
                delete data;
            ret = true;
        }
    }

    return ret;
}

//----------------------------------------------------------------------------------
// VPL Monitor Implementation

/// Initializes VPLFS_Monitor APIs.
int VPLFS_MonitorInit()
{
    int rv = 0;

    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&g_monitorDirMutex));
    if(g_isInit) {
        rv = VPL_ERR_IS_INIT;
        goto done;
    }
    g_isInit = true;
done:
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&g_monitorDirMutex));
    return rv;
}

/// Cleans up initialization of VPLFS_Monitor
int VPLFS_MonitorDestroy()
{
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&g_monitorDirMutex));
    g_isInit = false;
    _MonitorData::ClearMonitorMap();
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&g_monitorDirMutex));
    return 0;
}

/// Begins recursive monitoring of a directory.  Events will be returned in the
/// callback as they occur.
/// @param[in] directory Directory name to monitor.
/// @param[in] num_events_internal Amount of internal buffer to allocate to
///                                store, events. The more buffer, the less
///                                chance of overrun.
/// @param[in] cb Callback function to call when an event is received.  Note:
///               win32 users must call VPL_Yield or VPL_Sleep for events to
///               be received.
/// @param[out] handle_out Handle to identify the monitor.  The handle is returned
///                        with events and used to identify the monitor to stop.
int VPLFS_MonitorDir(const char* directory,
                     int num_events_internal,
                     VPLFS_MonitorCallback cb,
                     VPLFS_MonitorHandle *handle_out)
{
    //Bug 3842: temporary disable file monitor for not used on WinRT.
    //TODO: release VPLFS_MonitorEvent buffer memory
    return VPL_ERR_NOOP;
#if 0
    VPL_REPORT_INFO("monitorDir called");
    *handle_out = NULL;
    int rv;
    DWORD dwRecSize = sizeof(FILE_NOTIFY_INFORMATION) + MAX_PATH;
    DWORD dwCount = num_events_internal;
    int bufferLength = dwRecSize*dwCount;
    int returnBuf = num_events_internal*ALIGN(sizeof(VPLFS_MonitorEvent)+MAX_PATH);
    VPLFS_MonitorHandle_t* handle;
    WCHAR* wDirectory = NULL;

    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&g_monitorDirMutex));
    if(!g_isInit) {
        VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&g_monitorDirMutex));
        return VPL_ERR_NOT_INIT;
    }
    handle = (VPLFS_MonitorHandle_t*)
        malloc(sizeof(VPLFS_MonitorHandle_t) + bufferLength + returnBuf);
    if (handle == NULL) {
        VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&g_monitorDirMutex));
        return VPL_ERR_NOMEM;
    }
    handle->pBuffer = (char*)(handle+1);
    handle->nBufferLength = bufferLength;
    handle->internalEvents = num_events_internal;
    handle->stop = false;
    handle->has_rename_old_name = false;
    handle->cb = cb;
    handle->cbReturnBuf = (VPLFS_MonitorEvent*)(handle->pBuffer + handle->nBufferLength);

    //open the directory to watch....
    rv = _VPL__utf8_to_wstring(directory, &wDirectory);
    if (rv != 0) {
        VPL_REPORT_WARN("_VPL__utf8_to_wstring(%s) failed: %d", directory, rv);
        free(handle);
        VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&g_monitorDirMutex));
        return rv;
    }

    _MonitorData^ data = ref new _MonitorData(wDirectory);
    data->SetMonitorHandle(handle);
    rv = data->StartMonitor();
    if(rv != 0) {
        delete data;
        free(handle);
        free(wDirectory);
        VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&g_monitorDirMutex));
        return rv;
    }

    m_MonitorMap.insert( std::pair<VPLFS_MonitorHandle_t*,_MonitorData^>(handle,data) );

    *handle_out = (VPLFS_MonitorHandle*)handle;
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&g_monitorDirMutex));
    free(wDirectory);
    return rv;
#endif
}

int VPLFS_MonitorDirStop(VPLFS_MonitorHandle handle)
{
    //Bug 3842: temporary disable file monitor for not used on WinRT.
    //TODO: release VPLFS_MonitorEvent buffer memory
    return VPL_ERR_NOOP;
#if 0
    VPL_REPORT_INFO("monitorStop called");
    VPLFS_MonitorHandle_t* monitorHandle = (VPLFS_MonitorHandle_t*)handle;
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&g_monitorDirMutex));
    RemoveMonitorData(monitorHandle);
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&g_monitorDirMutex));
    free(monitorHandle);
    return VPL_OK;
#endif
}
