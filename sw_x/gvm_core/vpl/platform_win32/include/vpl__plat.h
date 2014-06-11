/*
*                Copyright (C) 2005, BroadOn Communications Corp.
*
*   These coded instructions, statements, and computer programs contain
*   unpublished  proprietary information of BroadOn Communications Corp.,
*   and  are protected by Federal copyright law. They may not be disclosed
*   to  third  parties or copied or duplicated in any form, in whole or in
*   part, without the prior written consent of BroadOn Communications Corp.
*
*/

#ifndef __VPL__PLAT_H__
#define __VPL__PLAT_H__

#ifndef WINVER
#  error "Expected WINVER to be defined at this point"
#endif

#if defined(_WIN32_WINNT) && (_WIN32_WINNT != WINVER)
#  error "Did not expect WINVER and _WIN32_WINNT to be different"
#endif

#include <windows.h>
#include <winsock2.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdarg.h>

#include <vplu_common.h>


#ifdef VPL_PLAT_IS_WINRT
#define _VPL__SharedAppFolder L"AcerCloud"
#endif

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------
// Win32-specific functions:
//--------------------------------

/// Convert UTF8 string to UTF16 string.
/// The output buffer will be allocated by the function.  The caller must free the output buffer after use.
/// @param[in] utf8 UTF8 string. Must be NUL terminated.
/// @param[out] wstring Corresponding UTF16 string. Will be NUL terminated.
/// @note To avoid memory leak, wstring must be freed by calling free() after use.
int _VPL__utf8_to_wstring(const char* utf8, wchar_t** wstring);

/// Convert UTF16 string to UTF8 string.
/// The output buffer must be supplied by the caller.
/// @param wstringLen Can be -1 if \a wstring is null-terminated.  Otherwise it should be the number
///     of "characters" (which I assume means 16-bit code units) in wstring.
int _VPL__wstring_to_utf8(const wchar_t* wstring, int wstringLen, char* utf8, size_t utf8BufLen);

/// Convert UTF16 string to UTF8 string.
/// The output buffer will be allocated by the function.  The caller must free the output buffer after use.
/// @param[in] wstring UTF16 string. Must be NUL terminated.
/// @param[out] utf8 Corresponding UTF8 string. Will be NUL terminated.
/// @note To avoid memory leak, utf8 must be freed by calling free() after use.
int _VPL__wstring_to_utf8_alloc(const wchar_t* wstring, char** utf8);

/// Set the user's SID.
/// This is only expected to be called when the process is running as the local system user.
int _VPL__SetUserSid(const char *sidStr);
/// Get the user's SID.
PSID _VPL__GetUserSid();
/// Get the user's SID string
/// User should free sidStr by VPL_ReleaseOSUserId
int _VPL__GetUserSidStr(char** sidStr);

/// Invoke shell change notify to trigger the shell extensions.
/// When sync state changes for a file or syncbox root folder,
/// this function should be called so that the shell extensions
/// will be invoked to display the correct overlay icon.
/// @param[in] path Absolute path to the file/folder that was changed. Must be UTF8 and NUL terminated.
void _VPL__ShellChangeNotify(const char* path);

//--------------------------------

#ifdef __cplusplus
} 
#endif

#ifdef __cplusplus
class ComInitGuard
{
public:
    ComInitGuard() : needToUninit(false) {}

    ~ComInitGuard();

    /// Pass COINIT_MULTITHREADED or similar.
    HRESULT init(DWORD dwCoInit);

private:
    VPL_DISABLE_COPY_AND_ASSIGN(ComInitGuard);
    bool needToUninit;
};
#endif


#endif // include guard
