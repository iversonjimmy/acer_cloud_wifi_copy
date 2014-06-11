//
//  Copyright (C) 2005-2008, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#ifndef __VPLEX_STRINGS_H__
#define __VPLEX_STRINGS_H__

#include "vplex_safe_conversion.h"
#include "vplex_plat.h"
#include "vpl_string.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Apparently, memcpy(NULL, NULL, 0) does not have well-defined behavior.
/// Use this to indicate that we want to ignore the first two params when the length is 0.
/// (As an optimization, we can remove the check on platforms where we verify that
/// it is safe to pass invalid pointers when n is 0.)
static inline void VPL_Memcpy(void* s1, const void* s2, size_t n)
{
    if (n != 0) {
        (IGNORE_RESULT)memcpy(s1, s2, n);
    }
}

//---------------------------------------------

/// Similar to strncpy, except this ensures that \a dest is always null-terminated (unless
/// \a destBufferSize is 0). If the \a src string (counting its null-terminator) is longer than
/// \a destBufferSize bytes, \a src will be truncated.
/// This also makes no guarantee that any bytes beyond the null-terminator will be zeroed (as
/// strncpy does).
/// @note Only use this when truncation is acceptable.
void VPLString_SafeStrncpy(char* dest,  size_t destBufferSize, char const* src);

/// \a size includes the null-terminator
static inline
bool VPLString_Equal(char const* first, char const* second, size_t size)
{
    return (0 == strncmp(first, second, size));
}

/// \a literalString must be a compile-time string literal
static inline
bool VPLString_EqualLiteral(char const* literalString, char const* variableString)
{
    return (0 == strcmp(literalString, variableString));
}

static inline
VPL_BOOL VPLString_StartsWith(const char* actual, const char* expected)
{
    return (strncmp(actual, expected, strlen(expected)) == 0);
}

/// Like strstr, but reads at most \a n characters from haystack.
char* VPLString_strnstr(const char* haystack, const char* needle, size_t n);

/// Returns the highest index in \a str that is \a charToFind.
/// Returns -1 if the character is not in \a str.
int VPLString_FindLastInstance(char charToFind, char const* str);

/// Allocates (via malloc) a buffer for the exact size required; the
/// caller must free the buffer.
char* VPLString_MallocAndSprintf(char const* formatMsg, ...) ATTRIBUTE_PRINTF(1, 2);

/// Returns the index in string that contains the last slash or backslash, or -1 if
/// the string has neither a slash nor backslash.
int VPLString_GetIndexOfLastSlash(const char* string);

/// Returns pointer to the first character after the last slash or backslash.
/// If there isn't a slash or backslash, returns @a string itself.
static inline
const char* VPLString_GetFilenamePart(const char* string)
{
    if (string == NULL) {
        return "*";
    }
    return string + VPLString_GetIndexOfLastSlash(string) + 1;
}

#ifdef  __cplusplus
}
#endif

#endif // include guard
