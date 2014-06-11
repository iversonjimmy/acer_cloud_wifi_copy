//
//  Copyright (C) 2005-2008, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#ifndef __VPL_MEM_UTILS_H__
#define __VPL_MEM_UTILS_H__

//============================================================================
/// @file
///
//============================================================================

#include "vplex_safe_conversion.h"
#include <stddef.h>

static inline size_t VPLPtr_DifferenceUnsigned(void const* end, void const* start)
{
    return PTR_TO_SIZE_T(end) - PTR_TO_SIZE_T(start);
}

static inline ptrdiff_t VPLPtr_DifferenceSigned(void const* end, void const* start)
{
    return (U8_CONST_PTR(end) - U8_CONST_PTR(start));
}

static inline void* VPLPtr_AddUnsigned(void const* base, size_t offsetBytes)
{
    return ((u8*)base) + offsetBytes;
}

static inline void* VPLPtr_AddSigned(void const* base, ptrdiff_t offsetBytes)
{
    return SIZE_T_TO_PTR(PTR_TO_SIZE_T(base) + offsetBytes);
}

/// xor each byte in \a buf with \a byte.
static inline void VPLBuf_XorBytes(u8 byte, void* buf, size_t len)
{
    u8* buf_as_bytes = (u8*)buf;
    size_t i;
    for (i = 0; i < len; i++) {
        buf_as_bytes[i] ^= byte;
    }
}

#ifdef VPL_PLAT_CACHE_LINE_SIZE
#   define VPL_PLAT_CACHE_LINE_MASK  ((VPL_PLAT_CACHE_LINE_SIZE) - 1)

#   define IS_MULTIPLE_OF_CACHE_LINE_SIZE(size) \
        (((size) & (VPL_PLAT_CACHE_LINE_MASK)) == 0)

#   define VPL_BUFFER_SIZED_FOR_IPC(size)  ROUND_UP_TO_CACHE_LINE_SIZE(size)

#   define VPL_BUFFER_FOR_IPC_ATTRIBUTE  ATTRIBUTE_CACHE_LINE_ALIGNED

    /// The largest number of padding bytes required to align an arbitrary pointer.
#   define VPL_IPC_BUFFER_ALIGN_PADDING  ((VPL_PLAT_CACHE_LINE_SIZE) - 1)

#   define ROUND_UP_TO_CACHE_LINE_SIZE(size)  \
        (((size) + (VPL_PLAT_CACHE_LINE_SIZE) - 1) & (~(VPL_PLAT_CACHE_LINE_MASK)))

#else  // !defined(VPL_PLAT_CACHE_LINE_SIZE)

#   define IS_MULTIPLE_OF_CACHE_LINE_SIZE(size)  (UNUSED(size), VPL_TRUE)

#   define VPL_BUFFER_SIZED_FOR_IPC(size)  (size)

#   define VPL_BUFFER_FOR_IPC_ATTRIBUTE

    /// The largest number of padding bytes required to align an arbitrary pointer.
#   define VPL_IPC_BUFFER_ALIGN_PADDING  0

    //--------------------------------
#   ifdef VPL_INTERNAL
    /// These 2 macros should only be used in demos.  Our internal code
    /// should use:
    /// #VPL_BUFFER_FOR_IPC_ATTRIBUTE instead of #ATTRIBUTE_CACHE_LINE_ALIGNED.
    /// #VPL_BUFFER_SIZED_FOR_IPC instead of #ROUND_UP_TO_CACHE_LINE_SIZE, and
#   endif // VPL_INTERNAL
    //% see note above
#   define ATTRIBUTE_CACHE_LINE_ALIGNED
    //% see note above
#   define ROUND_UP_TO_CACHE_LINE_SIZE(size)  (size)
    //--------------------------------

#endif   // !defined(VPL_PLAT_CACHE_LINE_SIZE)

#define ASSERT_MULTIPLE_OF_CACHE_LINE_SIZE(size)  \
    ASSERTMSG(IS_MULTIPLE_OF_CACHE_LINE_SIZE(size), FMTuSizeT, size)

#define ASSERT_CACHE_LINE_ALIGNED(ptr)  \
    ASSERTMSG(IS_MULTIPLE_OF_CACHE_LINE_SIZE(PTR_TO_SIZE_T(ptr)), FMT_PTR, ptr)

/// Wraps #IS_MULTIPLE_OF_CACHE_LINE_SIZE to avoid "int <-> ptr" compiler warnings.
static inline bool VPLPtr_IsAlignedOnCacheLine(void const* ptr) {
    return IS_MULTIPLE_OF_CACHE_LINE_SIZE(PTR_TO_SIZE_T(ptr));
}

/// Wraps #ROUND_UP_TO_CACHE_LINE_SIZE to avoid "int <-> ptr" compiler warnings.
static inline void* VPLPtr_RoundUpToCacheLine(void* ptr) {
    return SIZE_T_TO_PTR(ROUND_UP_TO_CACHE_LINE_SIZE(PTR_TO_SIZE_T(ptr)));
}

#endif // include guard
