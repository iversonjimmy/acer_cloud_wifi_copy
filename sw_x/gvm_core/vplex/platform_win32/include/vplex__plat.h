//
//  Copyright (C) 2005-2010, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished proprietary information of BroadOn Communications Corp.
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#ifndef __VPLEX__PLAT_H__
#define __VPLEX__PLAT_H__

#if !defined(__cplusplus) && !defined(_MSC_VER)
#include <stdbool.h>    // ISO C99 boolean type
#endif

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif


#define VPL_PLAT_HAS_CALENDAR_TIME  1

// TODO
// Using basename(__FILE__) is bad because we don't want __FILE__,
// which may contain directory information outside of the source
// tree, exposed in rodata.
// We should probably run GCC from the directory of the source file.
// Alternatively, there's a patch at http://gcc.gnu.org/bugzilla/show_bug.cgi?id=42579
#define _FILE_NO_DIRS_  __FILE__

#define VPL_PLAT_USE_PRINTF_MUTEX  1
#define VPL_PLAT_SYSLOG_IMPL  1
#define VPL_SYSLOG_ENABLED defined
#define USE_SYSLOG  1

#define VPL_DEFAULT_USER_NAMESPACE  "acer"

//============================================================================

/// Use these to explicitly document that you intend for upper bits to be discarded.
/// For WIN32, this suppresses the Visual C run-time "smaller type" conversion check
/// and has no impact on the generated release-mode machine code.
///@{

/// Discard any bits beyond the first 8 least significant bits.
#define TRUNCATE_TO_U8(value)  (u8)(0xFF & (value))

/// Discard any bits beyond the first 16 least significant bits.
#define TRUNCATE_TO_U16(value)  (u16)(0xFFFF & (value))

/// Discard any bits beyond the first 32 least significant bits.
#define TRUNCATE_TO_U32(value)  (u32)(0xFFFFFFFF & (value))

/// Discard any bits beyond the first 8 least significant bits.
#define TRUNCATE_TO_S8(value)  (s8)(0xFF & (value))

/// Discard any bits beyond the first 16 least significant bits.
#define TRUNCATE_TO_S16(value)  (s16)(0xFFFF & (value))

/// Discard any bits beyond the first 32 least significant bits.
#define TRUNCATE_TO_S32(value)  (s32)(0xFFFFFFFF & (value))

///@}

#ifdef __cplusplus
}
#endif

#endif // include guard
