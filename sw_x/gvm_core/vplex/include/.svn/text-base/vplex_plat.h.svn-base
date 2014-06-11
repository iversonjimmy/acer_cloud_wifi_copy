//
//  Copyright (C) 2005-2010, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#ifndef __VPLEX_PLAT_H__
#define __VPLEX_PLAT_H__

#include "vplu.h"
#include "vplu_types.h"
#include "vplex__plat.h"
#include "vplex_error.h"

#ifdef  __cplusplus
extern "C" {
#endif

#define VPL_AsBOOL(exp)  ((exp) ? VPL_TRUE : VPL_FALSE)

typedef u8 utf8;

typedef u16 utf16be;

/// Application ID.
//% Also used in user data queries.
typedef u64 VPL_AppId_t;
#define FMT_VPL_AppId_t "%016"PRIx64

// TODO: we should probably pick one terminology (title vs app) and be consistent
#define VPL_TITLE_ID_NONE  0
#define VPL_APP_ID_NONE    0

typedef u32 VPL_ContentId_t;
#define FMT_VPL_ContentId_t "%08"PRIx32

//-----------------------------------

#ifndef NULLPTR
#   ifdef __cplusplus
#       define NULLPTR  0
#   else
#       define NULLPTR  ((void *)0)
#   endif
#endif

#ifndef NULL
#   define NULL  NULLPTR
#endif

//-----------------------------------

/// Answers "are @a a and @a b equal after applying @a mask to each?"
#define EQUAL_USE_BITMASK(a, b, mask) \
    ((((a) ^ (b)) & (mask)) == 0)

/// Test if the specified value is a printable ASCII character.
#define VPL_isprint(x)  (((x) >= 0x20) && ((x) <= 0x7E))

static inline VPL_BOOL VPL_IsSafeForPrintf(char x)
{
    return VPL_isprint(x) || (x == '\t') || (x == '\r') || (x == '\n');
}

//-----------------------------------

/// @name Intentional truncation
/// Use these to explicitly document that you intend for upper bits to be discarded,
/// possibly changing the numeric value of the data.
//% For most platforms, a simple cast will do. In fact, for GCC, do no more
//% than cast, since doing more can trigger compiler bugs resulting in
//% unexpected behavior related to sign-extension in arithmetic operations.
//% Define platform-specific versions for platforms that have specific issues
//% with throwing away the upper bits (our Win32 debug builds will assert if
//% a simple cast discards any bits not set to 0).
//@{

/// Discard any bits beyond the first 8 least significant bits.
#ifndef TRUNCATE_TO_U8
#define TRUNCATE_TO_U8(value)  (u8)(value)
#endif

/// Discard any bits beyond the first 16 least significant bits.
#ifndef TRUNCATE_TO_U16
#define TRUNCATE_TO_U16(value)  (u16)(value)
#endif

/// Discard any bits beyond the first 32 least significant bits.
#ifndef TRUNCATE_TO_U32
#define TRUNCATE_TO_U32(value)  (u32)(value)
#endif

/// Discard any bits beyond the first 8 least significant bits.
#ifndef TRUNCATE_TO_S8
#define TRUNCATE_TO_S8(value)  (s8)(value)
#endif

/// Discard any bits beyond the first 16 least significant bits.
#ifndef TRUNCATE_TO_S16
#define TRUNCATE_TO_S16(value)  (s16)(value)
#endif

/// Discard any bits beyond the first 32 least significant bits.
#ifndef TRUNCATE_TO_S32
#define TRUNCATE_TO_S32(value)  (s32)(value)
#endif

//@}

//-----------------------------------
/// @name Typedefs for minimum bit-width integers
/// These are used to indicate that it is safe to use more bits than specified if
/// performance could be improved by doing so. These are used to succinctly
/// document programmer intent.
//@{

typedef s8 s8fast;
typedef u8 u8fast;
typedef s16 s16fast;
typedef u16 u16fast;
typedef s32 s32fast;
typedef u32 u32fast;
typedef s64 s64fast;
typedef u64 u64fast;

#define PRIs8fast   PRIs8
#define PRIs16fast  PRIs16
#define PRIs32fast  PRIs32
#define PRIs64fast  PRIs64

#define PRIu8fast   PRIu8
#define PRIu16fast  PRIu16
#define PRIu32fast  PRIu32
#define PRIu64fast  PRIu64

//@}

//-----------------------------------
/// @name Typedefs for precise bit-width integers
/// These are used to indicate that the program logic depends on the number of
/// bits to be exact. These are used to succinctly document programmer intent.
//@{

typedef s8 s8precise;
typedef u8 u8precise;
typedef s16 s16precise;
typedef u16 u16precise;
typedef s32 s32precise;
typedef u32 u32precise;
typedef s64 s64precise;
typedef u64 u64precise;

//@}

//-----------------------------------

/// Use to explicitly acknowledge that this really is a raw \c int being printed. Often,
/// it is better to replace the platform dependent \c int with a platform independent
/// fixed bit-width integer type.
#define PRIint  "d"
#define FMTint  "%"PRIint

#define PRIdouble  "G"
#define FMTdouble  "%"PRIdouble

#define FMTstr  "%s"

#define PRIenum  "d"
#define FMTenum  "%"PRIenum

//-----------------------------------
/// @name strto*() functions for exact-width integer types
/// These behave much like #strtoul(), #strtoull(), etc. with one exception;
/// these automatically set #errno to 0 before attempting the conversion.
/// You can still check #errno after each call to ensure that the parsing was
/// successful.
/// If you don't care about the error code, you may also use #VPLConv_strToU64().
//@{

u32 VPL_strToU32(const char* str, char** endptr, int base);

u64 VPL_strToU64(const char* str, char** endptr, int base);

//@}
//-----------------------------------

/// Look up localhost name.
/// @param[out] name Pointer to buffer to store the name.
/// @param[in] len Size of buffer.
/// @return #VPL_OK on success
/// @return #VPL_ERR_FAULT @a name or @a len gave an invalid address
/// @return #VPL_ERR_INVALID @a name is NULL or invalid
/// @return #VPL_ERR_PERM Insufficient system privileges to complete this call
int VPL_GetLocalHostname(char* name, size_t len);

#ifdef  __cplusplus
}
#endif

#endif // include guard
