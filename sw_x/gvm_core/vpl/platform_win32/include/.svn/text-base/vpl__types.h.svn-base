//
//  Copyright (C) 2005-2008, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#ifndef __VPL__TYPES_H__
#define __VPL__TYPES_H__

/// @file
/// Platform-private definitions, please do not include this header directly.

#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
#  define VPL_PLAT_IS_WINRT 1
#else
#  define VPL_PLAT_IS_WIN_DESKTOP_MODE 1
#endif

// Windows Server 2003 (0x0502) with SP1 (0x05020100)
// 0x0502 is needed for getaddrinfo
// 0x0600 is needed for PINIT_ONCE and WSAPoll.
// 0x6002 is needed for Win32 subset APIs on WinRT platform, e.g.: CoCreateInstanceFromApp 
#ifdef _WIN32_WINNT
# if _WIN32_WINNT < 0x0600
#   include "vplu.h"
#   pragma message("_WIN32_WINNT is "VPL_STRING(_WIN32_WINNT))
#   error "Specified _WIN32_WINNT version is too low for VPL."
# endif
#elif defined(VPL_PLAT_IS_WINRT)
# define _WIN32_WINNT  0x0602
#else
# define _WIN32_WINNT  0x0600
#endif

#ifdef _MSC_VER
# include <sdkddkver.h>
#else
# ifndef WINVER
#   define WINVER  _WIN32_WINNT
# endif
#endif

#ifndef WINVER
#  error "Expected WINVER to be defined at this point"
#endif

#if defined(_WIN32_WINNT) && (_WIN32_WINNT != WINVER)
#  error "Did not expect WINVER and _WIN32_WINNT to be different"
#endif

// Looks like we need 0x0700 to get:
// - aligned malloc
// - Unicode support for vpl__fs.cpp
#ifndef __MSVCRT_VERSION__
#  define __MSVCRT_VERSION__  0x0700
#endif

#define WIN32_LEAN_AND_MEAN

// Put C99 wide-char functions (e.g., strtoull()) into scope, in addition
// to wprintf() and friends enabled when _XOPEN_SOURCE is 500.
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 500 
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef VPL_NO_C99_TYPES

/// Get the ISO C99 types.
#ifdef __cplusplus
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#endif // __cplusplus

#ifdef _MSC_VER

# define __func__  __FUNCTION__

// MSVC does not support the C99 inline keyword when compiling as C;
// instead it supports the MS specific "__inline" keyword.
// However, it does support the C++ inline keyword when compiling as C++.
# ifndef __cplusplus
#   define inline __inline
# endif

# if (_MSC_VER >= 1700)
#  include <stdint.h> // Wow, they finally provide this C99 header (but still not inttypes.h)
# else
   typedef __int8  int8_t;
   typedef __int16 int16_t;
   typedef __int32 int32_t;
   typedef __int64 int64_t;
   typedef unsigned __int8  uint8_t;
   typedef unsigned __int16 uint16_t;
   typedef unsigned __int32 uint32_t;
   typedef unsigned __int64 uint64_t;
#  define INT8_MIN (-128)
#  define INT16_MIN (-32768)
#  define INT32_MIN (-2147483647 - 1)
#  define INT64_MIN  (-9223372036854775807LL - 1)
#  define INT8_MAX 127
#  define INT16_MAX 32767
#  define INT32_MAX 2147483647
#  define INT64_MAX 9223372036854775807LL
#  define UINT8_MAX  (0xff)
#  define UINT16_MAX (0xffff)
#  define UINT32_MAX (0xffffffff)
#  define UINT64_MAX (0xffffffffffffffffLL)

#  define INT8_C(val)  val##i8
#  define INT16_C(val) val##i16
#  define INT32_C(val) val##i32
#  define INT64_C(val) val##i64
#  define UINT8_C(val)  val##ui8
#  define UINT16_C(val) val##ui16
#  define UINT32_C(val) val##ui32
#  define UINT64_C(val) val##ui64
# endif

# define PRIi8 "i"
# define PRIi16 "i"
# define PRIi32 "i"
# define PRIi64 "I64i"
# define PRId8 "d"
# define PRId16 "d"
# define PRId32 "d"
# define PRId64 "I64d"
# define PRIu8 "u"
# define PRIu16 "u"
# define PRIu32 "u"
# define PRIu64 "I64u"
# define PRIx8 "x"
# define PRIx16 "x"
# define PRIx32 "x"
# define PRIx64 "I64x"
# define PRIX8 "X"
# define PRIX16 "X"
# define PRIX32 "X"
# define PRIX64 "I64X"
# define strtoull  _strtoui64
# define SCNu64 "I64u"
#else
# include <stdint.h>     // ISO C99
# include <inttypes.h>   // ISO C99
#endif

#define VPL_C99_TYPES_DEFINED  TRUE

#endif // #ifndef VPL_NO_C99_TYPES

#if defined(__x86_64__)
//#  define VPL_PRI_PREFIX_off_t "?"
//#  define VPL_SCN_PREFIX_off_t "?"
#  define VPL_PRI_PREFIX_size_t "l"
#  define VPL_SCN_PREFIX_size_t "l"
//#  define VPL_PRI_PREFIX_ssize_t "?"
//#  define VPL_SCN_PREFIX_ssize_t "?"
#  define VPL_PLAT_INT_IS_64BIT  1
#  define VPL_CPP_ENABLE_EXCEPTIONS  1
#else
#  define VPL_PLAT_IS_X86  1
//% TODO: Figure out how to make off_t 64-bits for Win32 build, then switch these first 2 to "I64"
#  define VPL_PRI_PREFIX_off_t "l"
#  define VPL_SCN_PREFIX_off_t "l"
#  define VPL_PRI_PREFIX_size_t ""
#  define VPL_SCN_PREFIX_size_t ""
#  define VPL_PRI_PREFIX_ssize_t "l"
#  define VPL_SCN_PREFIX_ssize_t "l"
#  define VPL_PLAT_INT_IS_64BIT  0
#  define VPL_CPP_ENABLE_EXCEPTIONS  1
#endif

/// This platform automatically includes "0x" when using printf "%p"
#define VPL_PLAT_0xPTR_PREFIX

/// @deprecated
#define VPL_PLAT_SIZET_FMT_PREFIX  VPL_PRI_PREFIX_size_t

#ifdef  __cplusplus
}
#endif

#endif // include guard
