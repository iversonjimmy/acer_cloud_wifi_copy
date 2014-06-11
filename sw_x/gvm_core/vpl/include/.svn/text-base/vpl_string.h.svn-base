//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __VPL_STRING_H__
#define __VPL_STRING_H__

//============================================================================
/// @file
/// Virtual Platform Layer definition of basic string functions.
//============================================================================

#include "vpl_types.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Suggested minimum size for a buffer passed to #VPL_strerror_errno().
#define VPL_STRERROR_ERRNO_SUGGESTED_BUFLEN  128

/// Returns a pointer to a string containing the error message for the specified errno.
/// This may be either a pointer to a string that the function stores in \a buf, or a pointer
/// to some (immutable) static string (in which case \a buf is unused).
/// If the function stores a string in \a buf, then at most \a buflen bytes are stored (the string
/// may be truncated if \a buflen is too small) and the string always includes a terminating
/// null byte.
/// You can use #VPL_STRERROR_ERRNO_SUGGESTED_BUFLEN when allocating \a buf.
/// @note This function is intended to mimic the GNU-specific version of strerror_r().
/// @note You should not use strerror_r() directly, since it has a different signature on different
///     platforms (and doesn't even exist for WIN32).
/// @param errnum Should always come from errno (this function doesn't know about #VPLError_t).
const char* VPL_strerror_errno(int errnum, char* buf, size_t buflen);

/**
 Apparently, there are conflicting implementations of standard functions on different platforms.
 Use these to indicate that the C99 semantics are desired.
 C99 semantics:
   \c snprintf() and \c vsnprintf() will write at most size-1 of the characters printed
   into the output string (str[size-1] then gets the terminating '\\0').
   If the return value is greater than or equal to the size argument, the string
   was too short and some of the printed characters were discarded.  If size is
   zero, nothing is written and dest may be a NULL pointer.
 @{
 */

// Microsoft doesn't support snprintf (part of C99).
// They provide _snprintf (note leading underscore), but that does not always append a terminating null.
// Note: For Win32, #VPL_snprintf() returns 0 and calls #VPL_ReportMsg() if the underlying _vscprintf()
//     function fails, which is known to occur due to unexpected wide char encoding (see bug 8475).
#ifdef _MSC_VER
int VPL_snprintf(char *str, size_t size, const char *format, ...);
#else
#  define VPL_snprintf(dest, size, ...)  snprintf(dest, size, __VA_ARGS__)
#endif

// Microsoft complains that we should use _strdup instead of strdup.
#ifdef _MSC_VER
static inline
char* VPL_strdup(const char *cstr)
{
    return _strdup(cstr);
}
#else
static inline
char* VPL_strdup(const char *cstr)
{
    return strdup(cstr);
}
#endif

// Microsoft's implementation of vsnprintf is not compliant with C99.  (When truncation
// occurs, the return value is -1 instead of the number of characters that would have
// been written.)  Use #VPL_vsnprintf() anywhere you care about the return value.
// Note: For Win32, #VPL_vsnprintf() returns 0 and calls #VPL_ReportMsg() if the underlying _vscprintf()
//     function fails, which is known to occur due to unexpected wide char encoding (see bug 8475).
#ifdef _MSC_VER
int VPL_vsnprintf(char *str, size_t size, const char *format, va_list ap);
#else
static inline
int VPL_vsnprintf(char *str, size_t size, const char *format, va_list ap)
{
    return vsnprintf(str, size, format, ap);
}
#endif

/** @} */

#ifdef  __cplusplus
}
#endif

#endif // include guard
