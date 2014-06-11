//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
#include "ccdiquery.h"

#if CCD_USE_IPC_SOCKET

#include <sstream>
#include <algorithm>

#include "ccd_core_service.hpp"
#include <ccdi_internal_defs.hpp>

#include "ccdi_rpc-server.pb.h"
#include "FileProtoChannel.h"

#include "base.h"
#include "query.h"

#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
#include <io.h>
#include <Psapi.h>
#endif

using namespace ccd;
using namespace std;

#define ASYNC_HANDLER_THREAD_STACK_SIZE  (32 * 1024)

//----------------------------------------------------------------------------
// CCDI RPCs built with Protocol Buffers
//----------------------------------------------------------------------------

static VPLTHREAD_FN_DECL
CCDIQuery_AsyncHandler2(void* arguments)
{
    LOG_FUNC_ENTRY(LOG_LEVEL_DEBUG);

    CCDIQueryHandler2Args* args = (CCDIQueryHandler2Args*)arguments;

    LOG_INFO("Handling CCDI protobuf RPC");

#ifdef _WIN32
    { // Log the client process.
        HANDLE hSocket = (HANDLE)_get_osfhandle(VPLNamedSocket_GetFd(&(args->connectedSock)));
        if (hSocket == INVALID_HANDLE_VALUE) {
            LOG_ERROR("Cannot get socket handle");
        } else {
            ULONG clientPid = 0;
            if (GetNamedPipeClientProcessId(hSocket, &clientPid) == 0) {
                LOG_ERROR("GetNamedPipeClientProcessId failed: "FMT_DWORD, GetLastError());
            } else {
                HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, clientPid);
                if (hProcess == NULL) {
                    LOG_ERROR("OpenProcess("FMTu64") failed: "FMT_DWORD, clientPid, GetLastError());
                } else {
                    ON_BLOCK_EXIT(CloseHandle, hProcess);
                    wchar_t exe_path[2048] = {};
                    // Be sure to pass the size of the lpFilename buffer in characters (not bytes).
                    if (GetModuleFileNameEx(hProcess, 0, exe_path, ARRAY_ELEMENT_COUNT(exe_path) - 1)) {
                        char *processName;
                        int rv = _VPL__wstring_to_utf8_alloc(exe_path, &processName);
                        if (rv == VPL_OK) {
                            LOG_INFO("ClientProcess: %s (%lu)", processName, clientPid);
                            free(processName);
                        }
                    } else {
                        LOG_ERROR("GetModuleFileNameEx for process "FMTu64" failed: "FMT_DWORD,
                                clientPid, GetLastError());
                    }
                }
            }
        }
    }
#endif //_WIN32

    // Service the RPC:
    {
        VPLTime_t startTimestamp = VPLTime_GetTimeStamp();
        FileProtoChannel channel(VPLNamedSocket_GetFd(&(args->connectedSock)), false);
        bool success;
        CCDIServiceImpl service;
        if (args->requestFrom == CCDI_CONNECTION_SYSTEM) {
            success = service.CCDIService::handleRpc(channel,
                                                     CCDProtobufRpcDebugGenericCallback,
                                                     CCDProtobufRpcDebugRequestCallback,
                                                     CCDProtobufRpcDebugResponseCallback);
        } else {
            LOG_ERROR("Handling protobuf RPC (game) deprecated");
            success = false;
        }
        {
            VPLTime_t elapsed = VPLTime_GetTimeStamp() - startTimestamp;
            LOG_INFO("Done handling protobuf RPC (elapsed="FMT_VPLTime_t"ms) success=%d",
                    VPLTime_ToMillisec(elapsed), success);
        }
    }

    VPLNamedSocket_Close(&(args->connectedSock));
    free(args);
    return VPLTHREAD_RETURN_VALUE;
}

int
CCDIQuery_SyncHandler2(CCDIQueryHandler2Args* args)
{
    LOG_FUNC_ENTRY(LOG_LEVEL_DEBUG);
    int rv = Util_SpawnThread(CCDIQuery_AsyncHandler2, args,
            ASYNC_HANDLER_THREAD_STACK_SIZE, VPL_FALSE, NULL);
    if (rv < 0) {
        LOG_ERROR("Util_SpawnThread returned %d; closing socket", rv);
        VPLNamedSocket_Close(&(args->connectedSock));
        free(args);
    }
    return rv;
}

#endif // CCD_USE_IPC_SOCKET
