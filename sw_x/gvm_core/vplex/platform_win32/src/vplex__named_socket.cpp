//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

//============================================================================
/// @file
/// 
//============================================================================

#ifdef UNICODE
# undef UNICODE
#endif
#ifdef _UNICODE
# undef _UNICODE
#endif

#include "vplex_named_socket.h"
#include "vplex_private.h"
#include <io.h>
#include <fcntl.h>
#include <winsock2.h>
#ifdef _MSC_VER
#  include <sddl.h>
#else
// The mingw version has wrong declaration (input string is not const).
extern "C" {
BOOL WINAPI ConvertStringSidToSidA(LPCSTR StringSid, PSID *Sid);
}
#endif

#include <scopeguard.hpp>

//--------------------------------------------
// TODO: refactor; this is not specific to sockets

/// Log the error code and error message from GetLastError().
#define VPL_LOG_LAST_WIN_ERROR(level_, subgroup_, operation_fmt_, ...)  \
    vplLogLastWinError(level_, subgroup_, __FILE__, __LINE__, __func__, \
            "Error from "operation_fmt_":", ##__VA_ARGS__)

/// You should generally use #LOG_LAST_WIN_ERROR instead of calling this directly.
static
DWORD vplLogLastWinError(VPLTrace_level level, int subgroup, const char* file, int line, const char* function,
        const char* format, ...)
{
    DWORD error = GetLastError();
    char* lpMsgBuf;
    VPL_BOOL gotWinErrMsg = VPLError_GetWinErrMsg(error, &lpMsgBuf);
    {
        va_list ap;
        va_start(ap, format);
        VPLTrace_VTraceEx(level, file, line, function, true, VPLTRACE_GRP_VPL, subgroup, format, ap);
        va_end(ap);
    }
    VPLTrace_TraceEx(level, file, line, function, true, VPLTRACE_GRP_VPL, subgroup,
            "... error "FMT_DWORD": %s", error,
            (gotWinErrMsg ? lpMsgBuf : "[VPLError_GetWinErrMsg failed]"));
    if (gotWinErrMsg) {
        LocalFree(lpMsgBuf);
    }
    return error;
}
//--------------------------------------------

// 256 characters on Windows
#define MAX_PIPE_NAME_LEN  256

#define DEFAULT_BUFFER_LEN  4096

#define DEFAULT_TIMEOUT_MILLISEC  10000

#define SOCK_LOG_LAST_ERRNO(operation_fmt_, ...) \
    VPL_LIB_LOG_ERR(VPL_SG_SOCKET, operation_fmt_" failed, %d: %s", ##__VA_ARGS__, errno, strerror(errno));

#define SOCK_LOG_LAST_WIN_ERROR(fmt_, ...)  VPL_LOG_LAST_WIN_ERROR(TRACE_ERROR, VPL_SG_SOCKET, fmt_, ##__VA_ARGS__)

#define PIPE_NAME_FORMAT_STR  "\\\\.\\pipe\\%s"

#ifndef _MSC_VER
#define PIPE_REJECT_REMOTE_CLIENTS 8
#endif

s32
VPLNamedSocket_OpenAndActivate(VPLNamedSocket_t* socket, const char* uniqueName, const char* clientOsUserId)
{
    if ((socket == NULL) || (uniqueName == NULL)) {
        return VPL_ERR_INVALID;
    }
    socket->sfd = -1;
    char pipeName[MAX_PIPE_NAME_LEN];
    snprintf(pipeName, sizeof(pipeName), PIPE_NAME_FORMAT_STR, uniqueName);

    SECURITY_ATTRIBUTES sa;
    sa.lpSecurityDescriptor = (PSECURITY_DESCRIPTOR)malloc(SECURITY_DESCRIPTOR_MIN_LENGTH);
    if (sa.lpSecurityDescriptor == NULL) {
        return VPL_ERR_NOMEM;
    }
    ON_BLOCK_EXIT(free, sa.lpSecurityDescriptor);
    InitializeSecurityDescriptor((PSECURITY_DESCRIPTOR)sa.lpSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
    // We will grant access to the specified client SID.
    PSID pSid1;
    if (!ConvertStringSidToSidA(clientOsUserId, &pSid1)) {
        SOCK_LOG_LAST_WIN_ERROR("ConvertStringSidToSidA");
        return VPL_ERR_SOCKET;
    }
    ON_BLOCK_EXIT(LocalFree, pSid1);
    // We must also grant access to our own SID (which is probably SYSTEM), otherwise we won't be able to
    // actually service the named pipe later.
    char* myOsUserId;
    int rv = VPL_GetOSUserId(&myOsUserId);
    if (rv != 0) {
        return rv;
    }
    ON_BLOCK_EXIT(VPL_ReleaseOSUserId, myOsUserId);
    PSID pSid2;
    if (!ConvertStringSidToSidA(myOsUserId, &pSid2)) {
        SOCK_LOG_LAST_WIN_ERROR("ConvertStringSidToSidA");
        return VPL_ERR_SOCKET;
    }
    // ACE: access control entry
#define NUM_OF_ACES 2
    DWORD dwAclSize = sizeof(ACL) + (NUM_OF_ACES * sizeof(ACCESS_ALLOWED_ACE)) +
            GetLengthSid(pSid1) + GetLengthSid(pSid2);
    dwAclSize = (dwAclSize + sizeof(DWORD) - 1) & ~(sizeof(DWORD) - 1);
    PACL pAcl = (PACL)malloc(dwAclSize);
    if (pAcl == NULL) {
        return VPL_ERR_NOMEM;
    }
    ON_BLOCK_EXIT(free, pAcl);
    if (!InitializeAcl(pAcl, dwAclSize, ACL_REVISION)) {
        SOCK_LOG_LAST_WIN_ERROR("InitializeAcl");
        return VPL_ERR_SOCKET;
    }
    // ACE #1 (make sure to update NUM_OF_ACES and dwAclSize if adding/removing).
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_READ | GENERIC_WRITE, pSid1)) {
        SOCK_LOG_LAST_WIN_ERROR("AddAccessAllowedAce");
        return VPL_ERR_SOCKET;
    }
    // ACE #2 (make sure to update NUM_OF_ACES and dwAclSize if adding/removing).
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_READ | GENERIC_WRITE, pSid2)) {
        SOCK_LOG_LAST_WIN_ERROR("AddAccessAllowedAce");
        return VPL_ERR_SOCKET;
    }
    SetSecurityDescriptorDacl((PSECURITY_DESCRIPTOR)sa.lpSecurityDescriptor, TRUE, pAcl, FALSE);
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE pipeHandle = CreateNamedPipeA(pipeName, // LPCTSTR lpName,
            PIPE_ACCESS_DUPLEX, // DWORD dwOpenMode,
            // TODO: FILE_FLAG_FIRST_PIPE_INSTANCE seems to result in ERROR_INVALID_PARAMETER.
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE |
                PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS, // DWORD dwPipeMode,
            PIPE_UNLIMITED_INSTANCES, // DWORD nMaxInstances,
            DEFAULT_BUFFER_LEN, // DWORD nOutBufferSize,
            DEFAULT_BUFFER_LEN, // DWORD nInBufferSize,
            DEFAULT_TIMEOUT_MILLISEC, // DWORD nDefaultTimeOut,
            &sa // LPSECURITY_ATTRIBUTES lpSecurityAttributes
            );
    if (pipeHandle == INVALID_HANDLE_VALUE) {
        SOCK_LOG_LAST_WIN_ERROR("CreateNamedPipeA");
        return VPL_ERR_SOCKET;
    }
    socket->sfd = _open_osfhandle((intptr_t)pipeHandle, 0);
    if (socket->sfd < 0) {
        SOCK_LOG_LAST_WIN_ERROR("_open_osfhandle");
        return VPL_ERR_SOCKET;
    }
    return VPL_OK;
}

s32
VPLNamedSocket_Reactivate(VPLNamedSocket_t* socket, const char* uniqueName, const char* clientOsUserId)
{
    return VPLNamedSocket_OpenAndActivate(socket, uniqueName, clientOsUserId);
}

s32
VPLNamedSocket_Accept(VPLNamedSocket_t* listeningSocket,
        VPLNamedSocket_t* connectedSocket_out)
{
    if ((listeningSocket == NULL) || (connectedSocket_out == NULL)) {
        return VPL_ERR_INVALID;
    }
    HANDLE h = (HANDLE)_get_osfhandle(listeningSocket->sfd);
    if (h == INVALID_HANDLE_VALUE) {
        return VPL_ERR_SOCKET;
    }
    // Note that this will block (as required by #VPLNamedSocket_Accept's contract) because we
    // called CreateNamedPipe with PIPE_WAIT.
    //
    // From MSDN:
    // "If a client connects before the function is called, the function
    // returns zero and GetLastError returns ERROR_PIPE_CONNECTED. This
    // can happen if a client connects in the interval between the call
    // to CreateNamedPipe and the call to ConnectNamedPipe. In this
    // situation, there is a good connection between client and server,
    // even though the function returns zero."
    BOOL success = ConnectNamedPipe(h, NULL) ?
            TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
    if (!success) {
        SOCK_LOG_LAST_WIN_ERROR("ConnectNamedPipe");
        VPLNamedSocket_Close(listeningSocket);
        return VPL_ERR_SOCKET;
    }
    connectedSocket_out->sfd = listeningSocket->sfd;
    listeningSocket->sfd = -1;
    return VPL_OK;
}

s32
VPLNamedSocket_Close(VPLNamedSocket_t* socket)
{
    if (socket == NULL) {
        return VPL_ERR_INVALID;
    }
    int rc = _close(socket->sfd);
    if (rc < 0) {
        SOCK_LOG_LAST_ERRNO("_close");
        return VPL_ERR_SOCKET;
    }
    socket->sfd = -1;
    return VPL_OK;
}

s32
VPLNamedSocketClient_Close(VPLNamedSocketClient_t* socket)
{
    if (socket == NULL) {
        return VPL_ERR_INVALID;
    }
    int rc = _close(socket->cfd);
    if (rc < 0) {
        SOCK_LOG_LAST_ERRNO("_close");
        return VPL_ERR_SOCKET;
    }
    socket->cfd = -1;
    return VPL_OK;
}

s32
VPLNamedSocketClient_Open(const char* uniqueName, VPLNamedSocketClient_t* connectedSocket_out)
{
    int rv;

    char pipeName[MAX_PIPE_NAME_LEN];
    snprintf(pipeName, sizeof(pipeName), PIPE_NAME_FORMAT_STR, uniqueName);

    HANDLE hPipe;

    static const int MAX_TRIES = 25;
    int notFoundCount = 0;
    int busyCount = 0;

    while (1) {
        hPipe = CreateFile(
           pipeName,       // pipe name
           GENERIC_READ |  // read and write access
           GENERIC_WRITE,
           0,              // no sharing
           NULL,           // default security attributes
           OPEN_EXISTING,  // opens existing pipe
           0,              // default attributes
           NULL);          // no template file

        if (hPipe != INVALID_HANDLE_VALUE) {
            // It's valid!
            rv = VPL_OK;
            break;
        }
        // Transient error cases:
        // If the CreateNamedPipe function was not successfully called on the server prior to this
        //   operation, a pipe will not exist and CreateFile will fail with ERROR_FILE_NOT_FOUND.
        // If there is at least one active pipe instance but there are no available listener pipes
        //   on the server, which means all pipe instances are currently connected, CreateFile
        //   fails with ERROR_PIPE_BUSY.
        if (GetLastError() == ERROR_PIPE_BUSY) {
            busyCount++;
        } else if (GetLastError() == ERROR_FILE_NOT_FOUND) {
            notFoundCount++;
            if (notFoundCount >= MAX_TRIES) {
                VPL_LIB_LOG_WARN(VPL_SG_SOCKET, "Named pipe \"%s\" is unavailable; is the pipe server running?", uniqueName);
                rv = VPL_ERR_NAMED_SOCKET_NOT_EXIST;
                goto out;
            }
        } else {
            SOCK_LOG_LAST_WIN_ERROR("CreateFile for named pipe \"%s\"", uniqueName);
            rv = VPL_ERR_SOCKET;
            goto out;
        }

        if (notFoundCount + busyCount >= MAX_TRIES) {
            VPL_LIB_LOG_WARN(VPL_SG_SOCKET, "Named pipe \"%s\" was too busy (%d, %d).",
                    uniqueName, busyCount, notFoundCount);
            rv = VPL_ERR_SOCKET;
            goto out;
        }
        // Try again after sleeping.
        VPLThread_Sleep(VPLTIME_FROM_MILLISEC(40));
    }
    connectedSocket_out->cfd = _open_osfhandle((intptr_t)hPipe, 0);
    if (connectedSocket_out->cfd < 0) {
        SOCK_LOG_LAST_WIN_ERROR("_open_osfhandle");
        rv = VPL_ERR_SOCKET;
        goto out;
    }
out:
    return rv;
}

int
VPLNamedSocket_GetFd(const VPLNamedSocket_t* socket)
{
    return socket->sfd;
}

int
VPLNamedSocketClient_GetFd(const VPLNamedSocketClient_t* socket)
{
    return socket->cfd;
}
