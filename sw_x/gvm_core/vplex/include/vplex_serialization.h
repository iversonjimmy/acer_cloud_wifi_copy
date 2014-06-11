//
//  Copyright (C) 2005-2008, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#ifndef __VPLEX_SERIALIZATION_H__
#define __VPLEX_SERIALIZATION_H__

//============================================================================
/// @file
/// Serialization/deserialization functions for the basic shared types.
//============================================================================

#include "vplex_strings.h"
#include "vplex_mem_utils.h"
#include "vplex_wstring.h"

#ifdef __cplusplus
extern "C" {
#endif

void VPL_CheckSerialization(void);

//------------------------------------
// Integer types
#ifdef VPL_INTERNAL_DISABLE
//! TODO: Signed and unsigned work exactly the same; would it be better to only
//!       have one function for each size?
#endif
//------------------------------------
#ifndef sizeU8
#define sizeU8 (sizeof(u8))
#define sizeS8 (sizeof(s8))

#define sizeU16 (sizeof(u16))
#define sizeS16 (sizeof(s16))

#define sizeU32 (sizeof(u32))
#define sizeS32 (sizeof(s32))

#define sizeU64 (sizeof(u64))
#define sizeS64 (sizeof(s64))
#endif

/// Make sure we don't mess up if typedefs are changed later.
#define _VPL_packCheckedPrimitive(destBuffer, srcValue, type) \
    ( \
    ASSERT_EQUAL(sizeof(srcValue), sizeof(type), FMTuSizeT), \
    _VPL_packUnchecked_##type(destBuffer, srcValue) \
    )

#define _VPL_unpackCheckedPrimitive(srcBuffer, destValue, type) \
    ( \
    ASSERT_EQUAL(sizeof(*(destValue)), sizeof(type), FMTuSizeT), \
    _VPL_unpackUnchecked_##type(srcBuffer, destValue) \
    )

#define VPL_PackU8(destBuffer, srcValue)   _VPL_packCheckedPrimitive(destBuffer, srcValue, u8)
#define VPL_PackU16(destBuffer, srcValue)  _VPL_packCheckedPrimitive(destBuffer, srcValue, u16)
#define VPL_PackU32(destBuffer, srcValue)  _VPL_packCheckedPrimitive(destBuffer, srcValue, u32)
#define VPL_PackU64(destBuffer, srcValue)  _VPL_packCheckedPrimitive(destBuffer, srcValue, u64)

#define VPL_PackS8(destBuffer, srcValue)   _VPL_packCheckedPrimitive(destBuffer, srcValue, s8)
#define VPL_PackS16(destBuffer, srcValue)  _VPL_packCheckedPrimitive(destBuffer, srcValue, s16)
#define VPL_PackS32(destBuffer, srcValue)  _VPL_packCheckedPrimitive(destBuffer, srcValue, s32)
#define VPL_PackS64(destBuffer, srcValue)  _VPL_packCheckedPrimitive(destBuffer, srcValue, s64)

void* _VPL_packUnchecked_u8( void* destBuffer,  u8 srcValue);
void* _VPL_packUnchecked_u16(void* destBuffer, u16 srcValue);
void* _VPL_packUnchecked_u32(void* destBuffer, u32 srcValue);
void* _VPL_packUnchecked_u64(void* destBuffer, u64 srcValue);

void* _VPL_packUnchecked_s8( void* destBuffer,  s8 srcValue);
void* _VPL_packUnchecked_s16(void* destBuffer, s16 srcValue);
void* _VPL_packUnchecked_s32(void* destBuffer, s32 srcValue);
void* _VPL_packUnchecked_s64(void* destBuffer, s64 srcValue);

#define VPL_UnpackU8(srcBuffer, destValue)   _VPL_unpackCheckedPrimitive(srcBuffer, destValue, u8)
#define VPL_UnpackU16(srcBuffer, destValue)  _VPL_unpackCheckedPrimitive(srcBuffer, destValue, u16)
#define VPL_UnpackU32(srcBuffer, destValue)  _VPL_unpackCheckedPrimitive(srcBuffer, destValue, u32)
#define VPL_UnpackU64(srcBuffer, destValue)  _VPL_unpackCheckedPrimitive(srcBuffer, destValue, u64)

#define VPL_UnpackS8(srcBuffer, destValue)   _VPL_unpackCheckedPrimitive(srcBuffer, destValue, s8)
#define VPL_UnpackS16(srcBuffer, destValue)  _VPL_unpackCheckedPrimitive(srcBuffer, destValue, s16)
#define VPL_UnpackS32(srcBuffer, destValue)  _VPL_unpackCheckedPrimitive(srcBuffer, destValue, s32)
#define VPL_UnpackS64(srcBuffer, destValue)  _VPL_unpackCheckedPrimitive(srcBuffer, destValue, s64)

const void* _VPL_unpackUnchecked_u8( const void* srcBuffer,  u8* destValue);
const void* _VPL_unpackUnchecked_u16(const void* srcBuffer, u16* destValue);
const void* _VPL_unpackUnchecked_u32(const void* srcBuffer, u32* destValue);
const void* _VPL_unpackUnchecked_u64(const void* srcBuffer, u64* destValue);

const void* _VPL_unpackUnchecked_s8( const void* srcBuffer,  s8* destValue);
const void* _VPL_unpackUnchecked_s16(const void* srcBuffer, s16* destValue);
const void* _VPL_unpackUnchecked_s32(const void* srcBuffer, s32* destValue);
const void* _VPL_unpackUnchecked_s64(const void* srcBuffer, s64* destValue);

//------------------------------------
// Enum
//------------------------------------
static inline void* VPL_PackEnumU8(void* dst, int src)
{
    return VPL_PackU8(dst, INT_TO_U8(src));
}
static inline const void* vpl_private_unpackU8ToInt(const void* src, void* dst_int_ptr)
{
    u8 temp;
    const void* new_src;
    new_src = VPL_UnpackU8(src, &temp);
    (*(int*)dst_int_ptr) = (int)temp;
    return new_src;
}
#define VPL_UnpackEnumU8(src, dst) \
    ( \
    ASSERT_EQUAL(sizeof(*dst), sizeof(int), FMTuSizeT) , \
    vpl_private_unpackU8ToInt(src, dst) \
    )

//------------------------------------
// Bytes
//------------------------------------
#define sizeBytes(size) (size)

/**
 * The result is NOT null-terminated.  The caller must specify the exact size
 * of the buffer.
 */
void* VPL_PackBytes(void* dst, const void* src, u32 size);

/**
 * The result is NOT null-terminated.  The caller must know the size
 * of the buffer first.
 */
const void* VPL_UnpackBytes(const void* src, void* dst, u32 size);

//------------------------------------
// String (variable size)
//------------------------------------
static inline size_t _VPL_sizeBufferForString(const char* src, size_t maxStrLen)
{
    if (src == NULLPTR) {
        return 1;
    }
    else {
        return strnlen(src, maxStrLen) + 1;
    }
}

/// @note \a dst MUST be able to store at least (\a maxStrLen + 1) chars
void* VPL_PackString(void* dst, const char* src, u32 maxStrLen);

/// @note \a dst MUST be able to store at least (\a maxStrLen + 1) chars.
const void* VPL_UnpackString(const void* src, void* dst, u32 maxStrLen);

//------------------------------------
// UTF8 String (variable size)
//------------------------------------
// This is just a copy of SizeBufferForString
static inline size_t _VPL_sizeBufferForUTF8(const utf8* src, size_t maxStrLen)
{
    return _VPL_sizeBufferForString((const char*)src, maxStrLen);
}

/// @note \a dst MUST be able to store at least (\a maxStrLen + 1) chars
void* VPL_PackUTF8(void* dst, const utf8* src, u32 maxStrLen);

/// @note \a dst MUST be able to store at least (\a maxStrLen + 1) chars.
const void* VPL_UnpackUTF8(const void* src, void* dst, u32 maxStrLen);

//------------------------------------
// Base64 encoding
//------------------------------------

/// Destination buffer size required for #VPL_EncodeBase64() with newlines enabled.
/// - For every 3 bytes of input (rounded up), we produce 4 characters of encoded output.
/// - Spec says newline required every 76 characters.
/// - Add a byte for the null-terminator.
#define VPL_BASE64_ENCODED_BUF_LEN(srcLen)  ((((((srcLen) + 2) / 3) * 4) * 77 / 76) + 1)

/// Destination buffer size required for #VPL_EncodeBase64() with newlines disabled.
/// - For every 3 bytes of input (rounded up), we produce 4 characters of encoded output.
/// - Add a byte for the null-terminator.
#define VPL_BASE64_ENCODED_SINGLE_LINE_BUF_LEN(srcLen)  (((((srcLen) + 2) / 3) * 4) + 1)

/// Destination buffer size required for #VPL_DecodeBase64(); in other words, the maximum
/// decoded length for a base64-encoded string of length @a encodedLen.
/// The actual length used may be less since the encoding can contain ignored whitespace as well
/// as padding.
/// - For every 4 bytes of encoded input, we produce at most 3 characters of output.
/// - Note that #VPL_DecodeBase64() does not add a null-terminator, so this calculation does not
///   include it either.
#define VPL_BASE64_DECODED_MAX_BUF_LEN(encodedLen)  ((((encodedLen) / 4) * 3))

/// Converts \a src to a null-terminated base64 string (rfc2045) stored in
/// \a dst.  \a dstLen initially contains the allocated size of \a dst and is
/// set to the length of the output including null terminator.  If \a dst
/// would overflow, \a dstLen is set to what would otherwise be the length of
/// the output.
/// @param urlSafe If true, replaces '/' with '_' and '+' with '-', since those characters
///     may cause problems when used in a URL.
void VPL_EncodeBase64(const void* src, size_t srcLen, char* dst, size_t* dstLen,
        VPL_BOOL addNewlines, VPL_BOOL urlSafe);

/// Decodes a base64 string and stores the result in \a dst.  \a dstLen
/// initially contains the allocated size of \a dst and is set to the length
/// of the output.  If \a dst would overflow, \a dstLen is set to what would
/// otherwise be the length of the output.  In any case, \a dst will not be null-terminated.
/// Ignores any whitespace in \a src.
/// Automatically handles URL-safe substitutions ('_' and '-') in \a src.
void VPL_DecodeBase64(const char* src, size_t srcLen, void* dst, size_t* dstLen);

//------------------------------------

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // include guard
