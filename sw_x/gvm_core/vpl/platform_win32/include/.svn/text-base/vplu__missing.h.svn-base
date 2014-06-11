/*
 *               Copyright (C) 2009, BroadOn Communications Corp.
 *
 *  These coded instructions, statements, and computer programs contain
 *  unpublished  proprietary information of BroadOn Communications Corp.,
 *  and  are protected by Federal copyright law. They may not be disclosed
 *  to  third  parties or copied or duplicated in any form, in whole or in
 *  part, without the prior written consent of BroadOn Communications Corp.
 *
 */
#ifndef __VPLU__MISSING_H__
#define __VPLU__MISSING_H__

/// @file
/// Platform-private definitions, please do not include this header directly.

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
//-------------------------------------
// Things in mingw that MSVC is missing:
//-------------------------------------
#define snprintf(dest, size, ...)  VPL_snprintf(dest, size, __VA_ARGS__)

typedef long ssize_t;
#ifndef __cplusplus
 typedef int bool;
# define true 1
# define false 0
#endif

#define va_copy(a, b)  (a) = (b)

#define STDIN_FILENO    0
#define STDOUT_FILENO   1
#define STDERR_FILENO   2
//-------------------------------------
#endif

#ifndef _MSC_VER
//-------------------------------------
// Things in MSVC that mingw is missing:
//-------------------------------------
size_t strnlen(const char *s, size_t maxlen);
#define VPL_PLAT_NEEDS_STRNLEN  defined

//-------------------------------------
#endif

#ifdef __cplusplus
}
#endif

#endif // include guard
