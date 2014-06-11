//
//  Copyright (C) 2005-2008, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#ifndef __VPL_SAFE_SERIALIZATION_H__
#define __VPL_SAFE_SERIALIZATION_H__

//============================================================================
/// @file
/// Safe serialization/deserialization functions for the basic shared types.
///
/// This header provides serialization buffer wrappers and methods which
/// perform automatic bounds checking.  A wrapper is initialized by calling
/// one of the init methods.
///
/// This wrapped buffer can then be passed to serialization/deserialization
/// methods without worrying about bounds checking.  If the buffer is overrun,
/// a flag is set.  An overrun buffer can still be passed to methods, which
/// will just return immediately.  If a buffer is marked as overrun after a
/// method returns, the contents of the destination are undefined.
///
/// Sample usage:
///   \code
///   VPLOutputBuffer out;
///   VPLInitOutputBuffer(&out, byteArray, byteArraySize);
///   VPLPackU8(&out, u8Value);
///   VPLPackString(&out, stringValue, sourceBufferLength);
///   if (VPLHasOverrunOut(&out)) {
///       // handle overrun
///   } else {
///       u32 serializedLength = VPLOutputBufferSize(&out);
///       // serialized contents are in 'byteArray'
///   }
///   \endcode
//============================================================================

#include "vplex_strings.h"
#include "vplex_mem_utils.h"
#include "vplex_wstring.h"
#include "vplex_serialization.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VPL_SERIALIZATION_OVERRUN 0xffffffff

struct VPLInputBuffer {
    const void * buf;
    u32 size;
    // Set to VPL_SERIALIZATION_OVERRUN on attempt to read past end
    u32 pos;
};
typedef struct VPLInputBuffer VPLInputBuffer;

struct VPLOutputBuffer {
    void * buf;
    u32 size;
    // Set to VPL_SERIALIZATION_OVERRUN on attempt to write past end
    u32 pos;
};
typedef struct VPLOutputBuffer VPLOutputBuffer;

void VPLInitInputBuffer(VPLInputBuffer * inputBuffer, const void * rawIn, u32 length);
void VPLInitOutputBuffer(VPLOutputBuffer * outputBuffer, void * rawOut, u32 length);
s32 VPLHasOverrunIn(VPLInputBuffer * inputBuffer);
s32 VPLHasOverrunOut(VPLOutputBuffer * outputBuffer);
u32 VPLOutputBufferSize(VPLOutputBuffer * outputBuffer);
u32 VPLInputBufferRemaining(VPLInputBuffer * inputBuffer);
u32 VPLOutputBufferRemaining(VPLOutputBuffer * outputBuffer);

/// Make sure we don't mess up if typedefs are changed later.
#define _VPL_PackCheckedPrimitive(destBuffer, srcValue, type) \
    ( \
    ASSERT_EQUAL(sizeof(srcValue), sizeof(type), FMTuSizeT), \
    _VPL_PackUnchecked_##type(destBuffer, srcValue) \
    )

#define _VPL_UnpackCheckedPrimitive(srcBuffer, destValue, type) \
    ( \
    ASSERT_EQUAL(sizeof(*(destValue)), sizeof(type), FMTuSizeT), \
    _VPL_UnpackUnchecked_##type(srcBuffer, destValue) \
    )

#define VPLPackU8(destBuffer, srcValue)   _VPL_PackCheckedPrimitive(destBuffer, srcValue, u8)
#define VPLPackU16(destBuffer, srcValue)  _VPL_PackCheckedPrimitive(destBuffer, srcValue, u16)
#define VPLPackU32(destBuffer, srcValue)  _VPL_PackCheckedPrimitive(destBuffer, srcValue, u32)
#define VPLPackU64(destBuffer, srcValue)  _VPL_PackCheckedPrimitive(destBuffer, srcValue, u64)

#define VPLPackS8(destBuffer, srcValue)   _VPL_PackCheckedPrimitive(destBuffer, srcValue, s8)
#define VPLPackS16(destBuffer, srcValue)  _VPL_PackCheckedPrimitive(destBuffer, srcValue, s16)
#define VPLPackS32(destBuffer, srcValue)  _VPL_PackCheckedPrimitive(destBuffer, srcValue, s32)
#define VPLPackS64(destBuffer, srcValue)  _VPL_PackCheckedPrimitive(destBuffer, srcValue, s64)

void _VPL_PackUnchecked_u8( VPLOutputBuffer * destBuffer,  u8 srcValue);
void _VPL_PackUnchecked_u16(VPLOutputBuffer * destBuffer, u16 srcValue);
void _VPL_PackUnchecked_u32(VPLOutputBuffer * destBuffer, u32 srcValue);
void _VPL_PackUnchecked_u64(VPLOutputBuffer * destBuffer, u64 srcValue);

void _VPL_PackUnchecked_s8( VPLOutputBuffer * destBuffer,  s8 srcValue);
void _VPL_PackUnchecked_s16(VPLOutputBuffer * destBuffer, s16 srcValue);
void _VPL_PackUnchecked_s32(VPLOutputBuffer * destBuffer, s32 srcValue);
void _VPL_PackUnchecked_s64(VPLOutputBuffer * destBuffer, s64 srcValue);

#define VPLUnpackU8(srcBuffer, destValue)   _VPL_UnpackCheckedPrimitive(srcBuffer, destValue, u8)
#define VPLUnpackU16(srcBuffer, destValue)  _VPL_UnpackCheckedPrimitive(srcBuffer, destValue, u16)
#define VPLUnpackU32(srcBuffer, destValue)  _VPL_UnpackCheckedPrimitive(srcBuffer, destValue, u32)
#define VPLUnpackU64(srcBuffer, destValue)  _VPL_UnpackCheckedPrimitive(srcBuffer, destValue, u64)

#define VPLUnpackS8(srcBuffer, destValue)   _VPL_UnpackCheckedPrimitive(srcBuffer, destValue, s8)
#define VPLUnpackS16(srcBuffer, destValue)  _VPL_UnpackCheckedPrimitive(srcBuffer, destValue, s16)
#define VPLUnpackS32(srcBuffer, destValue)  _VPL_UnpackCheckedPrimitive(srcBuffer, destValue, s32)
#define VPLUnpackS64(srcBuffer, destValue)  _VPL_UnpackCheckedPrimitive(srcBuffer, destValue, s64)

void _VPL_UnpackUnchecked_u8( VPLInputBuffer * srcBuffer,  u8 * destValue);
void _VPL_UnpackUnchecked_u16(VPLInputBuffer * srcBuffer, u16 * destValue);
void _VPL_UnpackUnchecked_u32(VPLInputBuffer * srcBuffer, u32 * destValue);
void _VPL_UnpackUnchecked_u64(VPLInputBuffer * srcBuffer, u64 * destValue);

void _VPL_UnpackUnchecked_s8( VPLInputBuffer * srcBuffer,  s8 * destValue);
void _VPL_UnpackUnchecked_s16(VPLInputBuffer * srcBuffer, s16 * destValue);
void _VPL_UnpackUnchecked_s32(VPLInputBuffer * srcBuffer, s32 * destValue);
void _VPL_UnpackUnchecked_s64(VPLInputBuffer * srcBuffer, s64 * destValue);

//------------------------------------
// Bytes
//------------------------------------

/**
 * The result is NOT null-terminated.  The caller must specify the exact size
 * of the buffer.
 */
void VPLPackBytes(VPLOutputBuffer * dst, const void * src, u32 size);

/**
 * The result is NOT null-terminated.  The caller must know the size
 * of the buffer first.
 */
void VPLUnpackBytes(VPLInputBuffer * src, void * dst, u32 size);

//------------------------------------
// String (variable size)
//------------------------------------
static inline size_t _VPL_SizeBufferForString(const char * src, size_t maxStrLen)
{
    if (src == NULLPTR) {
        return 1;
    } else {
        return strnlen(src, maxStrLen) + 1;
    }
}

/// This method always null terminates \a dst.  It will read at most
/// \a maxStrLen bytes from \a src.
/// @note \a maxStrLen does not include the null terminator.  \a maxStrLen + 1
/// bytes may be written to \a dst if a terminator is not found in \a src.
void VPLPackString(VPLOutputBuffer * dst, const char * src, u32 maxStrLen);

/// This method always null terminates \a dst.  It will read at most
/// \a maxStrLen + 1 bytes from \a src.
/// @note \a maxStrLen does not include the null terminator, and \a maxStrLen
/// + 1 bytes may be written to \a dst.  However, a null terminator must be
/// present in \a src or it will be considered an overrun error.
void VPLUnpackString(VPLInputBuffer * src, void * dst, u32 maxStrLen);

//------------------------------------

//------------------------------------
// UTF8 String (variable size)
//------------------------------------
// This is just a copy of SizeBufferForString
static inline size_t _VPL_SizeBufferForUTF8(const utf8 * src, size_t maxStrLen)
{
    return _VPL_sizeBufferForString((const char *)src, maxStrLen);
}

/// This method always null terminates \a dst.  It will read at most
/// \a maxStrLen bytes from \a src.
/// @note \a maxStrLen does not include the null terminator.  \a maxStrLen + 1
/// bytes may be written to \a dst if a terminator is not found in \a src.
void VPLPackUTF8(VPLOutputBuffer * dst, const utf8 * src, u32 maxStrLen);

/// This method always null terminates \a dst.  It will read at most
/// \a maxStrLen + 1 bytes from \a src.
/// @note \a maxStrLen does not include the null terminator, and \a maxStrLen
/// + 1 bytes may be written to \a dst.  However, a null terminator must be
/// present in \a src or it will be considered an overrun error.
void VPLUnpackUTF8(VPLInputBuffer * src, void * dst, u32 maxStrLen);

//------------------------------------

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // include guard
