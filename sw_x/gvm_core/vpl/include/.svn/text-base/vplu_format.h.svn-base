//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __VPLU_FORMAT_H__
#define __VPLU_FORMAT_H__

//============================================================================
/// @file
/// VPL utility (VPLU) definitions for printf/scanf format strings.
//============================================================================

#include "vpl_types.h"

#ifdef __cplusplus
extern "C" {
#endif

//============================================================================
/// @name printf formatting
//@{

#ifndef PRIs8
# define PRIs8  PRIi8
#endif
#ifndef PRIs16
# define PRIs16  PRIi16
#endif
#ifndef PRIs32
# define PRIs32  PRIi32
#endif
#ifndef PRIs64
# define PRIs64  PRIi64
#endif

/// Format string for printing a signed 8-bit integer (s8).
#define FMTs8   "%"PRIs8
/// Format string for printing a signed 16-bit integer (s16).
#define FMTs16  "%"PRIs16
/// Format string for printing a signed 32-bit integer (s32).
#define FMTs32  "%"PRIs32
/// Format string for printing a signed 64-bit integer (s64).
#define FMTs64  "%"PRIs64

/// Format string for printing an unsigned 8-bit integer (u8).
#define FMTu8   "%"PRIu8
/// Format string for printing an unsigned 16-bit integer (u16).
#define FMTu16  "%"PRIu16
/// Format string for printing an unsigned 32-bit integer (u32).
#define FMTu32  "%"PRIu32
/// Format string for printing an unsigned 64-bit integer (u64).
#define FMTu64  "%"PRIu64

/// Format string for printing an 8-bit integer as hexadecimal.
#define FMTx8   "%"PRIx8
/// Format string for printing a 16-bit integer as hexadecimal.
#define FMTx16  "%"PRIx16
/// Format string for printing a 32-bit integer as hexadecimal.
#define FMTx32  "%"PRIx32
/// Format string for printing a 64-bit integer as hexadecimal.
#define FMTx64  "%"PRIx64

/// Format string for printing an 8-bit integer as 2 hexadecimal digits.
#define FMTxx8   "%02"PRIx8
/// Format string for printing a 16-bit integer as 4 hexadecimal digits.
#define FMTxx16  "%04"PRIx16
/// Format string for printing a 32-bit integer as 8 hexadecimal digits.
#define FMTxx32  "%08"PRIx32
/// Format string for printing a 64-bit integer as 16 hexadecimal digits.
#define FMTxx64  "%016"PRIx64

/// Format string for printing an 8-bit integer as 2 upper-case hexadecimal digits.
#define FMTX08   "%02"PRIX8
/// Format string for printing a 16-bit integer as 4 upper-case hexadecimal digits.
#define FMTX016  "%04"PRIX16
/// Format string for printing a 32-bit integer as 8 upper-case hexadecimal digits.
#define FMTX032  "%08"PRIX32
/// Format string for printing a 64-bit integer as 16 upper-case hexadecimal digits.
#define FMTX064  "%016"PRIX64

/// Format string for printing an 8-bit integer as 2 hexadecimal digits, prefixed by "0x".
#define FMT0x8   "0x"FMTxx8
/// Format string for printing a 16-bit integer as 4 hexadecimal digits, prefixed by "0x".
#define FMT0x16  "0x"FMTxx16
/// Format string for printing a 32-bit integer as 8 hexadecimal digits, prefixed by "0x".
#define FMT0x32  "0x"FMTxx32
/// Format string for printing a 64-bit integer as 16 hexadecimal digits, prefixed by "0x".
#define FMT0x64  "0x"FMTxx64

/// Format string for printing a pointer address prefixed by "0x".
#define FMT0xPTR  VPL_PLAT_0xPTR_PREFIX"%p"

/// Format string for printing a pointer.
/// Whether or not you get the "0x" depends on the platform; if you want to make
/// sure it's printed, use #FMT0xPTR instead.
#define FMT_PTR  "%p"

/// Format string for printing an off_t as an unsigned integer.
#define FMTu_off_t   "%"VPL_PRIu_off_t
/// Format string for printing an off_t as hexadecimal.
#define FMTx_off_t   "%"VPL_PRIx_off_t

/// Format string for printing a size_t as an unsigned integer.
#define FMTu_size_t "%"VPL_PRIu_size_t
/// Format string for printing a size_t as hexadecimal.
#define FMTx_size_t "%"VPL_PRIx_size_t

/// Format string for printing an ssize_t as a signed integer.
#define FMTd_ssize_t "%"VPL_PRId_ssize_t
/// Format string for printing an ssize_t as hexadecimal.
#define FMTx_ssize_t "%"VPL_PRIx_ssize_t

/// Format string for printing a VPL_BOOL.
#define FMT_VPL_BOOL  "BOOL(%d)"

//-------------------
// Default formats.

#define FMT_off_t  FMTu_off_t
#define FMT_size_t  FMTu_size_t
#define FMT_ssize_t  FMTd_ssize_t

//-------------------

#ifdef WIN32
# define FMT_DWORD  "%lu"
# define FMT_HRESULT  "%#x"
#endif

/// @deprecated
#define PRIuSizeT  VPL_PLAT_SIZET_FMT_PREFIX"u"
/// @deprecated
#define FMTuSizeT  FMTu_size_t

//@}

#ifdef  __cplusplus
}
#endif

#endif // include guard
