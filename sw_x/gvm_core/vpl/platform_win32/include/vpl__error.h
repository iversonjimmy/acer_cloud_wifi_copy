//
//  Copyright (C) 2005-2008, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#ifndef __VPL__ERROR_H__
#define __VPL__ERROR_H__

#include <vpl_plat.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Translate an error code from system errno to VPL_ERR_*.
int VPLError_XlatErrno(DWORD errCode);

int VPLError_XlatWinErrno(DWORD errCode);

int VPLError_XlatSockErrno(DWORD errCode);

static inline
int VPLError_GetLastWinError()
{
    return VPLError_XlatWinErrno(GetLastError());
}

/// @note On success, caller must call #LocalFree() on *msgBuf_out when finished with it.
/// <code>
/// char* msgBuf;
/// if (VPLError_GetWinErrMsg(errCode, &msgBuf)) {
///     [use msgBuf..]
///     LocalFree(msgBuf);
/// }
/// </code>
VPL_BOOL VPLError_GetWinErrMsg(DWORD errCode, char** msgBuf_out);

void VPLError_ReportUnknownErr(DWORD errCode, const char* mappingTo);

#ifdef VPL_PLAT_IS_WINRT
int VPLError_XlatHResult(HRESULT hr);
int VPLError_XlatWinRTSockErrno(DWORD err_code);
DWORD WIN32_FROM_HRESULT(HRESULT hr);
#endif

#ifdef __cplusplus
}
#endif

#endif // include guard
