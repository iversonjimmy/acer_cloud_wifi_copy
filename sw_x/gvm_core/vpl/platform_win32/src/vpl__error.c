//
//  Copyright (C) 2005-2008, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#include "vpl_error.h"
#include "vplu_debug.h"
#include "vplu.h"
#include <errno.h>

int VPLError_XlatErrno(DWORD err_code) {
    int rv;
    switch(err_code) {
    case 0:
        rv = VPL_OK;
        break;
    case EACCES:
        rv = VPL_ERR_ACCESS;
        break;
    case EAGAIN:
        rv = VPL_ERR_AGAIN;
        break;
    case EBADF:
        rv = VPL_ERR_BADF;
        break;
    case EBUSY:
        rv = VPL_ERR_BUSY;
        break;
    case EEXIST:
        rv = VPL_ERR_EXIST;
        break;
    case EFAULT:
        rv = VPL_ERR_FAULT;
        break;
    case EINTR:
        rv = VPL_ERR_INTR;
        break;
    case EINVAL:
        rv = VPL_ERR_INVALID;
        break;
    case EIO:
        rv = VPL_ERR_IO;
        break;
    case EMFILE:
        rv = VPL_ERR_MAX;
        break;
    case ENAMETOOLONG:
        rv = VPL_ERR_NAMETOOLONG;
        break;
    case ENFILE:
        rv = VPL_ERR_MAX;
        break;
    case ENODEV:
        rv = VPL_ERR_NODEV;
        break;
    case ENOENT:
        rv = VPL_ERR_NOENT;
        break;
    case ENOMEM:
        rv = VPL_ERR_NOMEM;
        break;
    case ENOSPC:
        rv = VPL_ERR_NOSPC;
        break;
    case ENOSYS:
        rv = VPL_ERR_NOSYS;
        break;
    case ENOTDIR:
        rv = VPL_ERR_NOTDIR;
        break;
    case ENOTEMPTY:
        rv = VPL_ERR_NOTEMPTY;
        break;
    case EPERM:
        rv = VPL_ERR_PERM;
        break;
    case EPIPE:
        rv = VPL_ERR_PIPE;
        break;
    case EROFS:
        rv = VPL_ERR_ROFS;
        break;
    case EDEADLK:
        rv = VPL_ERR_DEADLK;
        break;
    case EXDEV:
        rv = VPL_ERR_CROSS_DEVICE_LINK;
        break;
    default:
        // Generic error for the unmapped.
        rv = VPL_ERR_FAIL;
        VPLError_ReportUnknownErr(err_code, "VPL_ERR_FAIL");
        break;
    }

    return rv;
}

int VPLError_XlatWinErrno(DWORD err_code) {
    int rv;

    switch(err_code) {
      case ERROR_TOO_MANY_POSTS:
        rv = VPL_ERR_MAX;
        break;
      case ERROR_ALREADY_EXISTS:
        rv = VPL_ERR_EXIST;
        break;
      case ERROR_NOT_ENOUGH_MEMORY:
        rv = VPL_ERR_NOMEM;
        break;
      case ERROR_PATH_NOT_FOUND:
        rv = VPL_ERR_NOENT;
        break;
      case ERROR_FILE_NOT_FOUND:
        rv = VPL_ERR_NOENT;
        break;
      // MSDN claims SleepConditionVariableCS will produce
      // WAIT_TIMEOUT if no signal is received in time.  This
      // is false, at least on Windows 7: we get ERROR_TIMEOUT.
      case ERROR_TIMEOUT:
        rv = VPL_ERR_TIMEOUT;
        break;
      case ERROR_ACCESS_DENIED:
        rv = VPL_ERR_ACCESS;
        break;
      case ERROR_DIR_NOT_EMPTY:
        rv = VPL_ERR_NOTEMPTY;
        break;
      default:
        {
            rv = VPL_ERR_FAIL;
            VPLError_ReportUnknownErr(err_code, "VPL_ERR_FAIL");
            break;
        }
    }

    return rv;
}

int VPLError_XlatSockErrno(DWORD err_code) {
    int rv;

    switch(err_code) {
    case WSAEACCES:
        rv = VPL_ERR_ACCESS;
        break;
    case WSAEADDRINUSE:
        rv = VPL_ERR_ADDRINUSE;
        break;
    case WSAEADDRNOTAVAIL:
        rv = VPL_ERR_ADDRNOTAVAIL;
        break;
    case WSAEAFNOSUPPORT:
        rv = VPL_ERR_INVALID;
        break;
    case WSAEBADF:
        rv = VPL_ERR_BADF;
        break;
    case WSAECONNABORTED:
        rv = VPL_ERR_CONNABORT;
        break;
    case WSAECONNRESET:
        rv = VPL_ERR_CONNRESET;
        break;
    case WSAECONNREFUSED:
        rv =  VPL_ERR_CONNREFUSED;
        break;
    case WSAEDESTADDRREQ:
        rv = VPL_ERR_DESTADDRREQ;
        break;
    case WSAEFAULT:
        rv = VPL_ERR_FAULT;
        break;
    case WSAEHOSTUNREACH:
        rv = VPL_ERR_UNREACH;
        break;
    case WSAEINPROGRESS:
        rv = VPL_ERR_BUSY;
        break;
    case WSAEINTR:
        rv = VPL_ERR_INTR;
        break;
    case WSAEINVAL:
        rv = VPL_ERR_INVALID;
        break;
    case WSAEISCONN:
        rv = VPL_ERR_ISCONN;
        break;
    case WSAELOOP:
        rv = VPL_ERR_LOOP;
        break;
    case WSAEMFILE:
        rv = VPL_ERR_MAX;
        break;
    case WSAEMSGSIZE:
        rv = VPL_ERR_MSGSIZE;
        break;
    case WSAENAMETOOLONG:
        rv = VPL_ERR_NAMETOOLONG;
        break;
    case WSAENETDOWN:
        rv = VPL_ERR_NETDOWN;
        break;
    case WSAENETRESET:
        rv = VPL_ERR_NETRESET;
        break;
    case WSAENETUNREACH:
        rv = VPL_ERR_UNREACH;
        break;
    case WSAENOBUFS:
        rv = VPL_ERR_NOBUFS;
        break;
    case WSAENOPROTOOPT:
        rv = VPL_ERR_INVALID;
        break;
    case WSAENOTCONN:
        rv = VPL_ERR_NOTCONN;
        break;
    case WSAENOTSOCK:
        rv = VPL_ERR_NOTSOCK;
        break;
    case WSAEOPNOTSUPP:
        rv = VPL_ERR_OPNOTSUPPORTED;
        break;
    case WSAESHUTDOWN:
        rv = VPL_ERR_PIPE;
        break;
    case WSAETIMEDOUT:	
        rv = VPL_ERR_TIMEOUT;
        break;
    case WSAEWOULDBLOCK:
        rv = VPL_ERR_AGAIN;
        break;
    case WSANOTINITIALISED:
        rv = VPL_ERR_FAIL;
        break;
    default:
        // Generic error for the unmapped.
        rv = VPL_ERR_FAIL;
        VPLError_ReportUnknownErr(err_code, "VPL_ERR_FAIL");
        break;
    }

    return rv;
}

#ifndef VPL_PLAT_IS_WINRT
VPL_BOOL VPLError_GetWinErrMsg(DWORD errCode, char** msgBuf_out)
{
    DWORD res = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        errCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)msgBuf_out,
        0, NULL );
    if (res < 2) {
        return VPL_FALSE;
    } else {
        if ((*msgBuf_out)[res - 1] == '\n') {
            (*msgBuf_out)[res - 1] = '\0';
        }
        if ((*msgBuf_out)[res - 2] == '\r') {
            (*msgBuf_out)[res - 2] = '\0';
        }
        return VPL_TRUE;
    }
}
#endif

void VPLError_ReportUnknownErr(DWORD errCode, const char* mappingTo)
{
#ifdef VPL_PLAT_IS_WINRT
    VPL_REPORT_WARN("Converting unknown Win error code "FMT_DWORD" to %s.",
        errCode, mappingTo);
#else
    char* lpMsgBuf;
    if (VPLError_GetWinErrMsg(errCode, &lpMsgBuf)) {
        VPL_REPORT_WARN("Converting unknown Win error code "FMT_DWORD" (%s) to %s.",
                errCode, lpMsgBuf, mappingTo);
        LocalFree(lpMsgBuf);
    } else {
        VPL_REPORT_WARN("Converting unknown Win error code "FMT_DWORD" (%s) to %s.",
                errCode, "[VPLError_GetWinErrMsg failed]", mappingTo);
    }
#endif
}

#ifdef VPL_PLAT_IS_WINRT
DWORD
WIN32_FROM_HRESULT(HRESULT hr)
{
    DWORD dwRt;

    if ((hr & 0xFFFF0000) == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, 0)) {
        // Could have come from many values, but we choose this one
        dwRt = HRESULT_CODE(hr);
    }
    else if (hr == S_OK) {
        dwRt = HRESULT_CODE(hr);
    }
    else {
        // otherwise, we got an impossible value
        dwRt = 0xFFFFFFFF;
    }

    return dwRt;
}

int VPLError_XlatHResult(HRESULT hr)
{
    int rv = VPL_ERR_FAIL;
    DWORD win32ErrCode = WIN32_FROM_HRESULT(hr);
 
    if(0xFFFFFFFF != win32ErrCode){
        rv = VPLError_XlatSockErrno(win32ErrCode);
        if (rv != VPL_ERR_FAIL) {
            return rv;
        }
        rv = VPLError_XlatWinErrno(win32ErrCode);
        if (rv != VPL_ERR_FAIL) {
            return rv;
        }
        rv = VPLError_XlatErrno(win32ErrCode);
    } else {
        VPL_REPORT_WARN("HResult to Win32 Error Code Unmapped. HResult = 0x%X", hr);
    }
    return rv;
}

using namespace Windows::Networking::Sockets;

#define   WINRT_SOCKET_ERR_MAP(ERR_STATUS, VPL_ERR_CODE) {\
    case SocketErrorStatus::##ERR_STATUS: \
       rv = VPL_ERR_CODE; \
       sprintf(err_str, #ERR_STATUS); \
    break;\
}

//reference: http://msdn.microsoft.com/en-us/library/windows/apps/windows.networking.sockets.socketerrorstatus

int VPLError_XlatWinRTSockErrno(DWORD err_code) {
    int rv;
    char err_str[50];
    SocketErrorStatus _err_code = static_cast<SocketErrorStatus>(err_code);
    switch(_err_code) {
        WINRT_SOCKET_ERR_MAP( Unknown,                     VPL_ERR_FAIL);
        WINRT_SOCKET_ERR_MAP( OperationAborted,            VPL_ERR_FAIL);
        WINRT_SOCKET_ERR_MAP( HttpInvalidServerResponse,   VPL_ERR_FAIL);
        WINRT_SOCKET_ERR_MAP( ConnectionTimedOut,          VPL_ERR_TIMEOUT);
        WINRT_SOCKET_ERR_MAP( AddressFamilyNotSupported ,  VPL_ERR_INVALID);
        WINRT_SOCKET_ERR_MAP( SocketTypeNotSupported    ,  VPL_ERR_INVALID);
        WINRT_SOCKET_ERR_MAP( HostNotFound,                VPL_ERR_UNREACH);
        WINRT_SOCKET_ERR_MAP( NoDataRecordOfRequestedType, VPL_ERR_INVALID);
        WINRT_SOCKET_ERR_MAP( NonAuthoritativeHostNotFound,VPL_ERR_FAIL);
        WINRT_SOCKET_ERR_MAP( ClassTypeNotFound,           VPL_ERR_INVALID);
        WINRT_SOCKET_ERR_MAP( AddressAlreadyInUse,         VPL_ERR_ADDRINUSE);
        WINRT_SOCKET_ERR_MAP( CannotAssignRequestedAddress,VPL_ERR_ADDRNOTAVAIL);
        WINRT_SOCKET_ERR_MAP( ConnectionRefused,           VPL_ERR_CONNREFUSED);
        WINRT_SOCKET_ERR_MAP( NetworkIsUnreachable,        VPL_ERR_UNREACH);
        WINRT_SOCKET_ERR_MAP( UnreachableHost,             VPL_ERR_UNREACH);
        WINRT_SOCKET_ERR_MAP( NetworkIsDown,               VPL_ERR_NETDOWN);
        WINRT_SOCKET_ERR_MAP( NetworkDroppedConnectionOnReset, VPL_ERR_NETRESET);
        WINRT_SOCKET_ERR_MAP( SoftwareCausedConnectionAbort,   VPL_ERR_CONNABORT);
        WINRT_SOCKET_ERR_MAP( ConnectionResetByPeer,       VPL_ERR_CONNRESET);
        WINRT_SOCKET_ERR_MAP( HostIsDown,                  VPL_ERR_UNREACH);
        WINRT_SOCKET_ERR_MAP( NoAddressesFound,            VPL_ERR_PIPE);
        WINRT_SOCKET_ERR_MAP( TooManyOpenFiles,            VPL_ERR_MAX);
        WINRT_SOCKET_ERR_MAP( MessageTooLong,              VPL_ERR_MSGSIZE);
        WINRT_SOCKET_ERR_MAP( CertificateExpired,          VPL_ERR_FAIL);
        WINRT_SOCKET_ERR_MAP( CertificateUntrustedRoot,    VPL_ERR_FAIL);
        WINRT_SOCKET_ERR_MAP( CertificateCommonNameIsIncorrect, VPL_ERR_FAIL);
        WINRT_SOCKET_ERR_MAP( CertificateWrongUsage,       VPL_ERR_FAIL);
        WINRT_SOCKET_ERR_MAP( CertificateRevoked,          VPL_ERR_FAIL);
        WINRT_SOCKET_ERR_MAP( CertificateNoRevocationCheck,VPL_ERR_FAIL);
        WINRT_SOCKET_ERR_MAP( CertificateRevocationServerOffline, VPL_ERR_FAIL);
        WINRT_SOCKET_ERR_MAP( CertificateIsInvalid,        VPL_ERR_FAIL);
        default:
            // Generic error for the unmapped.
            rv = VPL_ERR_FAIL;
            sprintf(err_str, "Unknown Error Code");
            //VPLError_ReportUnknownErr(err_code, "VPL_ERR_FAIL");
        break;
    }

    if (SocketErrorStatus::Unknown <= _err_code && _err_code <= SocketErrorStatus::CertificateIsInvalid) {
        VPL_REPORT_WARN("WinRT Socket error (%s) is mapped to %d", err_str, rv);
    } else {
        VPL_REPORT_WARN("Unknown error code "FMT_DWORD" (%s) is mapped to %d", err_code, err_str, rv);
    }
    return rv;
}
#endif

