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

#define ALIGN(x) (((x+(sizeof(int)-1))/sizeof(int))*sizeof(int))

static VPLLazyInitMutex_t g_monitorDirMutex = VPLLAZYINITMUTEX_INIT;

# define MONITOR_DIR_THREAD_STACK_SIZE  (128 * 1024)

static u32 g_isInitCount = 0;

static VPLThread_t g_vpl_monitor_dir_thread;
static VPLSem_t g_semaphoreCommand;
static VPLSem_t g_semaphoreReturn;

struct VPLFS_MonitorHandle_t
{
    HANDLE hDir;
    OVERLAPPED overlapped;
    int internalEvents;
    bool has_rename_old_name;
    char rename_old_name[MAX_PATH];
    bool stop;
    VPLFS_MonitorCallback cb;
    VPLFS_MonitorEvent* cbReturnBuf;
    DWORD nBufferLength;
    char* pBuffer;
};

enum MonitorDirCommand {
    MONITOR_COMMAND_NONE,
    MONITOR_COMMAND_START,
    MONITOR_COMMAND_STOP,
    MONITOR_COMMAND_DESTROY,
};

/// The VPLFileMonitor API handler thread (vpl_monitor_dir_handler) waits on g_semaphoreCommand,
/// and each VPLFS_Monitor*() API blocks until the handler thread posts to g_semaphoreReturn.
/// The VPLFS_Monitor*() calls are also protected by g_monitorDirMutex, so only
/// one API call can be influencing the vpl_monitor_dir_handler thread at a time.
/// So, it is safe to access the following variables:
/// 1) while holding g_monitorDirMutex and before posting g_semaphoreCommand, or
/// 2) while holding g_monitorDirMutex and after waiting on g_semaphoreReturn, or
/// 3) within vpl_monitor_dir_handler, after waiting on g_semaphoreCommand and before posting g_semaphoreReturn.
//@{
static int g_returnErrorCode;
static MonitorDirCommand g_monitorCommand;
static std::vector<VPLFS_MonitorHandle_t*> g_monitorHandles;
static VPLFS_MonitorHandle_t* g_monitorHandle;
//@}

static int beginMonitor(VPLFS_MonitorHandle_t* handle);

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

VOID CALLBACK ChDirCompletionRoutine(DWORD dwErrorCode,
                                     DWORD dwNumberOfBytesTransfered,
                                     LPOVERLAPPED lpOverlapped)
{
    //VPL_REPORT_INFO("ChDirCompletionRoutine");
    int rc;
    DWORD dwOffset = 0;
    FILE_NOTIFY_INFORMATION* pInfo;
    VPLFS_MonitorHandle_t* monHandle = (VPLFS_MonitorHandle_t*)(lpOverlapped->hEvent);

    if(dwErrorCode==ERROR_OPERATION_ABORTED || g_isInitCount==0 || monHandle->stop) {
        // Will be handled below.
    }else if(dwErrorCode != ERROR_SUCCESS) {
        VPL_REPORT_WARN("Unexpected error:"FMT_DWORD, dwErrorCode);
    }else { // (dwErrorCode == ERROR_SUCCESS), btw, ERROR_SUCCESS == 0.
        DWORD nextEntryOffset = 1;
        VPLFS_MonitorEvent* cbBuffer = (VPLFS_MonitorEvent*)
                (monHandle->pBuffer+monHandle->nBufferLength);
        u32 previousEntry = 0;
        void* eventBuffer = cbBuffer;
        int eventBufferSize = 0;

        if(dwNumberOfBytesTransfered == 0)
        {  // This means the buffer overflowed
            monHandle->cb(monHandle,
                          eventBuffer,
                          eventBufferSize,
                          VPLFS_MonitorCB_OVERFLOW);
        }else {

            do {
                // Get a pointer to the first change record...
                pInfo = (FILE_NOTIFY_INFORMATION*) &(monHandle->pBuffer[dwOffset]);
                nextEntryOffset = pInfo->NextEntryOffset;
                char filename[4 * MAX_PATH];  // a UTF16 char may require at most 4 bytes when encoded in UTF8

                // NOTE: pInfo->FileName is not null-terminated!
                // NOTE: pInfo->FileNameLength is in bytes!

                int temp_rc = _VPL__wstring_to_utf8(pInfo->FileName,
                        pInfo->FileNameLength / sizeof(WCHAR),
                        filename, ARRAY_SIZE_IN_BYTES(filename));
                if(temp_rc != 0) {
                    VPL_REPORT_WARN("%s failed: %d", "_VPL__wstring_to_utf8", temp_rc);
                    // TODO: skip the rest of this loop?
                }

                if(!isBeginWithPeriod(filename))
                {   // If the file is not considered hidden (begins with a period.
                    //VPL_REPORT_INFO("Filename:%s, FilenameLength:"FMT_DWORD", action:"FMT_DWORD", nextEntry:"FMT_DWORD,
                    //                filename, pInfo->FileNameLength, pInfo->Action, pInfo->NextEntryOffset);

                    switch(pInfo->Action) {
                    case FILE_ACTION_ADDED:
                        {
                            //VPL_REPORT_INFO("FILE_ACTION_ADDED: %s", filename);
                            cbBuffer->action = VPLFS_MonitorEvent_FILE_ADDED;
                            monHandle->has_rename_old_name = false;
                            setCommon(cbBuffer, filename);
                            previousEntry = cbBuffer->nextEntryOffsetBytes;
                            eventBufferSize += cbBuffer->nextEntryOffsetBytes;
                            cbBuffer = (VPLFS_MonitorEvent*)(((char*)(cbBuffer))+cbBuffer->nextEntryOffsetBytes);
                        }break;
                    case FILE_ACTION_REMOVED:
                        {
                            //VPL_REPORT_INFO("FILE_ACTION_REMOVED: %s", filename);
                            cbBuffer->action = VPLFS_MonitorEvent_FILE_REMOVED;
                            monHandle->has_rename_old_name = false;
                            setCommon(cbBuffer, filename);
                            previousEntry = cbBuffer->nextEntryOffsetBytes;
                            eventBufferSize += cbBuffer->nextEntryOffsetBytes;
                            cbBuffer = (VPLFS_MonitorEvent*)(((char*)(cbBuffer))+cbBuffer->nextEntryOffsetBytes);
                        }break;
                    case FILE_ACTION_MODIFIED:
                        {
                            //VPL_REPORT_INFO("FILE_ACTION_MODIFIED: %s", filename);
                            cbBuffer->action = VPLFS_MonitorEvent_FILE_MODIFIED;
                            monHandle->has_rename_old_name = false;
                            setCommon(cbBuffer, filename);
                            previousEntry = cbBuffer->nextEntryOffsetBytes;
                            eventBufferSize += cbBuffer->nextEntryOffsetBytes;
                            cbBuffer = (VPLFS_MonitorEvent*)(((char*)(cbBuffer))+cbBuffer->nextEntryOffsetBytes);
                        }break;
                    case FILE_ACTION_RENAMED_OLD_NAME:
                        {
                            //VPL_REPORT_INFO("FILE_ACTION_RENAMED_OLD_NAME: %s", filename);
                            monHandle->has_rename_old_name = true;
                            strncpy(monHandle->rename_old_name, filename, MAX_PATH);
                        }break;
                    case FILE_ACTION_RENAMED_NEW_NAME:
                        {
                            //VPL_REPORT_INFO("FILE_ACTION_RENAMED_NEW_NAME: %s", filename);
                            if(monHandle->has_rename_old_name) {
                                cbBuffer->action = VPLFS_MonitorEvent_FILE_RENAMED;
                                setCommon(cbBuffer, monHandle->rename_old_name);
                                cbBuffer->moveTo = ((char*)cbBuffer) + cbBuffer->nextEntryOffsetBytes;
                                std::string moveToName(filename);
                                replaceBackslashWithForwardSlash(moveToName);
                                strncpy((char*)cbBuffer->moveTo, moveToName.c_str(), MAX_PATH);
                                int nameLen = moveToName.size()+1;  // +1 for '\0'
                                cbBuffer->filename = (char*)(cbBuffer+1);
                                cbBuffer->nextEntryOffsetBytes += ALIGN(nameLen);
                                previousEntry = cbBuffer->nextEntryOffsetBytes;
                                eventBufferSize += cbBuffer->nextEntryOffsetBytes;
                                cbBuffer = (VPLFS_MonitorEvent*)(((char*)(cbBuffer))+cbBuffer->nextEntryOffsetBytes);
                                monHandle->has_rename_old_name = false;
                            }else{
                                VPL_REPORT_WARN("RENAMED_NEW_NAME without RENAME_OLD_NAME:%s", filename);
                            }
                        }break;
                    default:
                        VPL_REPORT_WARN("Unhandled action:"FMT_DWORD", %s", pInfo->Action, filename);
                        break;
                    }
                }

                // More than one change may happen at the same time. Load the next change and continue...
                dwOffset += pInfo->NextEntryOffset;
            }while((pInfo->NextEntryOffset != 0) &&
                   (dwOffset<dwNumberOfBytesTransfered) &&
                   (monHandle->stop != true));

            cbBuffer = (VPLFS_MonitorEvent*)(((char*)cbBuffer)- previousEntry);
            cbBuffer->nextEntryOffsetBytes = 0;

            if(eventBufferSize>0) {
                monHandle->cb(monHandle,
                              eventBuffer,
                              eventBufferSize,
                              VPLFS_MonitorCB_OK);
            }
        }
    }

    if(g_isInitCount==0 || monHandle->stop) {
        VPL_REPORT_INFO("Cleaned up file monitor");
        CloseHandle(monHandle->hDir);
        free(monHandle);
    } else {
        rc = beginMonitor(monHandle);
        if(rc != 0) {
            VPL_REPORT_WARN("beginMonitor failed:%d", rc);
        }
    }
}

static int beginMonitor(VPLFS_MonitorHandle_t* handle)
{
    // VPL_REPORT_INFO("beginMonitor called");
    int rv = 0;
    handle->overlapped.Internal = 0;
    handle->overlapped.InternalHigh = 0;
    handle->overlapped.Offset = 0;
    handle->overlapped.OffsetHigh = 0;
    handle->overlapped.hEvent = handle;  // hEvent not used by system, used as ctx ptr.
#ifdef VPL_PLAT_IS_WINRT
    // TODO: using WinRT APIs to implement file monitor
    rv = VPL_ERR_NOOP;
#else
    if(!ReadDirectoryChangesW(handle->hDir,
                              handle->pBuffer, //<--FILE_NOTIFY_INFORMATION records are put into this buffer
                              handle->nBufferLength,
                              TRUE,
                              FILE_NOTIFY_CHANGE_FILE_NAME|
                                  FILE_NOTIFY_CHANGE_DIR_NAME|
                                  FILE_NOTIFY_CHANGE_SIZE|
                                  FILE_NOTIFY_CHANGE_LAST_WRITE|
                                  FILE_NOTIFY_CHANGE_CREATION,
                              NULL, // Only defined for synchronous calls.
                              &handle->overlapped,
                              &ChDirCompletionRoutine) )
    {
       rv = VPLError_GetLastWinError();
    }
#endif
    return rv;
}


static void* vpl_monitor_dir_handler(void* unused)
{
    int rc;
    while(true) {

        rc = VPLSem_Wait(&g_semaphoreCommand);
        if(rc != VPL_OK) {
            VPL_REPORT_WARN("Semaphore wait:%d", rc);
        }
        g_returnErrorCode = 0;
        VPL_REPORT_INFO("Command Received:%d", g_monitorCommand);

        switch(g_monitorCommand) {
        case MONITOR_COMMAND_NONE:
            break;
        case MONITOR_COMMAND_START:
            rc = beginMonitor(g_monitorHandle);
            if(rc != 0) {
                VPL_REPORT_WARN("Cannot begin monitor:%d", rc);
                g_returnErrorCode = rc;
            }else {
                g_monitorHandles.push_back(g_monitorHandle);
            }

            g_monitorCommand = MONITOR_COMMAND_NONE;
            rc = VPLSem_Post(&g_semaphoreReturn);
            if(rc != VPL_OK){
                VPL_REPORT_WARN("Semaphore Return post:%d", rc);
                g_returnErrorCode = rc;
            }
            break;
        case MONITOR_COMMAND_STOP:
            g_monitorHandle->stop = true;
#ifdef VPL_PLAT_IS_WINRT
            // TODO: properly stop dir monitoring
            rc = VPL_ERR_NOOP;
#else
            if (CancelIo(g_monitorHandle->hDir) == 0) {
                rc = VPLError_GetLastWinError();
                VPL_REPORT_WARN("CancelIo:%d", rc);
            }
#endif
            g_monitorHandles.erase(std::remove(g_monitorHandles.begin(),
                                               g_monitorHandles.end(),
                                               g_monitorHandle),
                                   g_monitorHandles.end());

            g_monitorCommand = MONITOR_COMMAND_NONE;
            rc = VPLSem_Post(&g_semaphoreReturn);
            if(rc != VPL_OK){
                VPL_REPORT_WARN("Semaphore Return post:%d", rc);
                g_returnErrorCode = rc;
            }
            break;
        case MONITOR_COMMAND_DESTROY:
            {
                std::vector<VPLFS_MonitorHandle_t*>::iterator iter =
                        g_monitorHandles.begin();
                for(;iter!=g_monitorHandles.end();++iter) {
                    (*iter)->stop = true;
#ifdef VPL_PLAT_IS_WINRT
                    // TODO: properly stop dir monitoring
                    rc = VPL_ERR_NOOP;
#else
                    if (CancelIo((*iter)->hDir) == 0) {
                        rc = VPLError_GetLastWinError();
                        VPL_REPORT_WARN("CancelIo:%d", rc);
                    }
#endif
                }
                g_monitorHandles.clear();
            }
            g_monitorCommand = MONITOR_COMMAND_NONE;
            goto exit;
        default:
            VPL_REPORT_WARN("Unrecognized command: %d", rc);
            break;
        }
    }
 exit:
    return NULL;
}

/// Initializes VPLFS_Monitor APIs.
int VPLFS_MonitorInit()
{
    int rv = 0;
    VPLThread_attr_t thread_attr;

    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&g_monitorDirMutex));
    if(g_isInitCount!=0) {
        g_isInitCount++;
        VPL_REPORT_WARN("Already init, refcount:%d", g_isInitCount);
        goto done;
    }

    rv = VPLSem_Init(&g_semaphoreCommand, 1, 0);
    if (rv != VPL_OK) {
        goto fail_semaphoreCommand;
    }

    rv = VPLSem_Init(&g_semaphoreReturn, 1, 0);
    if (rv != VPL_OK) {
        goto fail_semaphoreReturn;
    }

    rv = VPLThread_AttrInit(&thread_attr);
    if(rv != VPL_OK) {
        VPL_REPORT_WARN("Failed to initialize thread attributes.");
        goto fail_attr_init;
    }
    VPLThread_AttrSetStackSize(&thread_attr, MONITOR_DIR_THREAD_STACK_SIZE);
    VPL_REPORT_INFO("Starting vpl_monitor_dir thread with stack size %d.",
                    MONITOR_DIR_THREAD_STACK_SIZE);

    // We need to spawn another thread because CancelIo() needs to be called from the same thread
    // that called ReadDirectoryChangesW().  CancelIoEx() doesn't have this limitation, but it
    // isn't supported before Windows Vista.
    rv = VPLThread_Create(&g_vpl_monitor_dir_thread,
                          vpl_monitor_dir_handler, NULL, &thread_attr, "vpl_monitor_dir");
    if(rv != VPL_OK) {
        VPL_REPORT_WARN("Failed to init vpl_monitor_dir thread: %s(%d).",
                        strerror(rv), rv);
        goto fail_thread;
    }
    g_isInitCount++;
    VPLThread_AttrDestroy(&thread_attr);
    goto done;
 fail_thread:
    VPLThread_AttrDestroy(&thread_attr);
 fail_attr_init:
    VPLSem_Destroy(&g_semaphoreReturn);
 fail_semaphoreReturn:
    VPLSem_Destroy(&g_semaphoreCommand);
 fail_semaphoreCommand:
 done:
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&g_monitorDirMutex));
    return rv;
}
/// Cleans up initialization of VPLFS_Monitor
int VPLFS_MonitorDestroy()
{
    int rc;
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&g_monitorDirMutex));
    g_isInitCount--;
    if(g_isInitCount != 0) {
        VPL_REPORT_WARN("Still init, refcount:%d", g_isInitCount);
        goto still_init;
    }
    g_monitorCommand = MONITOR_COMMAND_DESTROY;
    rc = VPLSem_Post(&g_semaphoreCommand);
    if(rc != VPL_OK) {
        VPL_REPORT_WARN("semaphore:%d", rc);
    }
    VPLThread_Join(&g_vpl_monitor_dir_thread, NULL);
    VPLSem_Destroy(&g_semaphoreReturn);
    VPLSem_Destroy(&g_semaphoreCommand);
 still_init:
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
    VPL_REPORT_INFO("monitorDir called");
    *handle_out = NULL;
    int rv;
    int rc;
    DWORD dwRecSize = sizeof(FILE_NOTIFY_INFORMATION) + MAX_PATH;
    DWORD dwCount = num_events_internal;
    int bufferLength = dwRecSize*dwCount;
    int returnBuf = num_events_internal*ALIGN(sizeof(VPLFS_MonitorEvent)+MAX_PATH);
    VPLFS_MonitorHandle_t* handle;
    WCHAR* wDirectory = NULL;

    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&g_monitorDirMutex));
    if(g_isInitCount==0) {
        rv = VPL_ERR_NOT_INIT;
        goto error_not_init;
    }
    handle = (VPLFS_MonitorHandle_t*)
            malloc(sizeof(VPLFS_MonitorHandle_t) + bufferLength + returnBuf);
    if (handle == NULL) {
        rv = VPL_ERR_NOMEM;
        goto error_not_init;
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
        goto error;
    }
#ifdef VPL_PLAT_IS_WINRT
    CREATEFILE2_EXTENDED_PARAMETERS cf2ex = {0};
    cf2ex.dwSize = sizeof(CREATEFILE2_EXTENDED_PARAMETERS);
    cf2ex.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
    cf2ex.dwFileFlags = FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED;
    handle->hDir = CreateFile2(wDirectory, 
                               FILE_LIST_DIRECTORY,
                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                               OPEN_EXISTING,
                               &cf2ex);
#else
    handle->hDir = CreateFileW(wDirectory,
                              FILE_LIST_DIRECTORY,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              NULL,
                              OPEN_EXISTING,
                              FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
                              NULL);
#endif
    if(handle->hDir == INVALID_HANDLE_VALUE) {
        rv = VPLError_GetLastWinError();
        VPL_REPORT_WARN("CreateFile: %d, %s", rv, directory);
        goto error2;
    }
    g_monitorCommand = MONITOR_COMMAND_START;
    g_monitorHandle = handle;
    rc = VPLSem_Post(&g_semaphoreCommand);
    if(rc != VPL_OK) {
        VPL_REPORT_WARN("semaphore:%d", rc);
    }

    rc = VPLSem_Wait(&g_semaphoreReturn);
    if(rc != VPL_OK) {
        VPL_REPORT_WARN("semaphore:%d", rc);
    }
    rv = g_returnErrorCode;
    *handle_out = (VPLFS_MonitorHandle*)handle;
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&g_monitorDirMutex));
    return rv;
 error2:
    free(wDirectory);
 error:
    free(handle);
 error_not_init:
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&g_monitorDirMutex));
    return rv;
}

int VPLFS_MonitorDirStop(VPLFS_MonitorHandle handle)
{
    // call cancelIO to free.
    // don't free here!!!!  free(handle);
    int rc;
    int rv;
    VPL_REPORT_INFO("monitorStop called");
    VPLFS_MonitorHandle_t* monitorHandle = (VPLFS_MonitorHandle_t*)handle;
    VPLMutex_Lock(VPLLazyInitMutex_GetMutex(&g_monitorDirMutex));
    if(g_isInitCount==0) {
        rv = VPL_ERR_NOT_INIT;
        goto error_not_init;
    }

    g_monitorCommand = MONITOR_COMMAND_STOP;
    g_monitorHandle = monitorHandle;
    rc = VPLSem_Post(&g_semaphoreCommand);
    if(rc != VPL_OK) {
        VPL_REPORT_WARN("semaphore:%d", rc);
    }

    rc = VPLSem_Wait(&g_semaphoreReturn);
    if(rc != VPL_OK) {
        VPL_REPORT_WARN("semaphore:%d", rc);
    }
    rv = g_returnErrorCode;
 error_not_init:
    VPLMutex_Unlock(VPLLazyInitMutex_GetMutex(&g_monitorDirMutex));
    return rv;
}
