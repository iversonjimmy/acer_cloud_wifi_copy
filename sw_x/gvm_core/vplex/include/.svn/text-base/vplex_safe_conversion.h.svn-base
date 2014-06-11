//
//  Copyright (C) 2005-2008, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#ifndef __VPL_SAFE_CONVERSION_H__
#define __VPL_SAFE_CONVERSION_H__

//============================================================================
/// @file
/// Functions and macros that panic during debug builds if the value is being changed by the
/// conversion. These also make it explicit which type is being converted from.
//============================================================================

#include "vplex_plat.h"
#include "vplex_assert.h"

#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

#define safeConversionAssertFailed(file, line, ...) \
    VPLAssert_Failed(file, NULL, line, __VA_ARGS__)

//------------------------------------------------------------------------

static inline u32 U8_TO_U32(u8 value)
{
    // trivially safe
    return (u32)value;
}

//------------------------------------------------------------------------

static inline u64 U8_TO_U64(u8 value)
{
    // trivially safe
    return (u64)value;
}

//------------------------------------------------------------------------

static inline s32 S16_TO_S32(s16 value)
{
    // trivially safe
    return (s32)value;
}

//------------------------------------------------------------------------

#define U16_TO_U8(value)  vpl_priv_u16_to_u8(_FILE_NO_DIRS_, __LINE__, value)
static inline u8 vpl_priv_u16_to_u8(char const* file, int line, u16 value)
{
#ifdef _DEBUG
    if(value > UINT8_MAX)
    {
        safeConversionAssertFailed(file, line, "<"FMTu16"> too big for u8", value);
    }
#else
    UNUSED(file);
    UNUSED(line);
#endif
    return (u8)value;
}

static inline s32 U16_TO_S32(u16 value)
{
    // trivially safe
    return (s32)value;
}

static inline u32 U16_TO_U32(u16 value)
{
    // trivially safe
    return (u32)value;
}

static inline u64 U16_TO_U64(u16 value)
{
    // trivially safe
    return (u64)value;
}

//------------------------------------------------------------------------

#define S32_TO_U8(value)  vpl_priv_s32_to_u8(_FILE_NO_DIRS_, __LINE__, value)
static inline u8 vpl_priv_s32_to_u8(char const* file, int line, s32 value)
{
#ifdef _DEBUG
    if(value < 0)
    {
        safeConversionAssertFailed(file, line, "<"FMTs32"> being converted to unsigned", value);
    }
    if(value > UINT8_MAX)
    {
        safeConversionAssertFailed(file, line, "<"FMTs32"> too big for u8", value);
    }
#else
    UNUSED(file);
    UNUSED(line);
#endif
    return (u8)value;
}

#define S32_TO_S16(value) vpl_priv_s32_to_s16(_FILE_NO_DIRS_, __LINE__, value)
static inline s16 vpl_priv_s32_to_s16(char const* file, int line, s32 value)
{
#ifdef _DEBUG
    if(value > INT16_MAX) {
        safeConversionAssertFailed(file, line, "<"FMTs32"> too big for s16", value);
    }
    if(value < INT16_MIN) {
        safeConversionAssertFailed(file, line, "<"FMTs32"> too negative for s16", value);
    }
#else
    UNUSED(file);
    UNUSED(line);
#endif
    return (s16)value;
}

#define S32_TO_U16(value) vpl_priv_s32_to_u16(_FILE_NO_DIRS_, __LINE__, value)
static inline u16 vpl_priv_s32_to_u16(char const* file, int line, s32 value)
{
#ifdef _DEBUG
    if (value < 0) {
        safeConversionAssertFailed(file, line, "<"FMTs32"> being converted to unsigned", value);
    }
    if (value > UINT16_MAX) {
        safeConversionAssertFailed(file, line, "<"FMTs32"> too big for u16", value);
    }
#else
    UNUSED(file);
    UNUSED(line);
#endif
    return (u16)value;
}

#define S32_TO_U32(value)  vpl_priv_s32_to_u32(_FILE_NO_DIRS_, __LINE__, value)
static inline u32 vpl_priv_s32_to_u32(char const* file, int line, s32 value)
{
#ifdef _DEBUG
    if (value < 0) {
        safeConversionAssertFailed(file, line, "<"FMTs32"> being converted to unsigned", value);
    }
#else
    UNUSED(file);
    UNUSED(line);
#endif
    return (u32)value;
}

#define S32_TO_INT(value) vpl_priv_s32_to_int(_FILE_NO_DIRS_, __LINE__, value)
static inline int vpl_priv_s32_to_int(char const* file, int line, s32 value)
{
#ifdef _DEBUG
    if(value > INT_MAX) {
        safeConversionAssertFailed(file, line, "<"FMTs32"> too big for int", value);
    }
    if(value < INT_MIN) {
        safeConversionAssertFailed(file, line, "<"FMTs32"> too negative for int", value);
    }
#else
    UNUSED(file);
    UNUSED(line);
#endif
    return (int)value;
}

#define S32_TO_SIZE_T(value)  vpl_priv_s32_to_size_t(_FILE_NO_DIRS_, __LINE__, value)
static inline size_t vpl_priv_s32_to_size_t(char const* file, int line, s32 value)
{
#ifdef _DEBUG
    if (value < 0) {
        safeConversionAssertFailed(file, line, "<"FMTs32"> being converted to unsigned", value);
    }
#else
    UNUSED(file);
    UNUSED(line);
#endif
    return (size_t)value;
}

//------------------------------------------------------------------------

#define U32_TO_U8(value)  vpl_priv_u32_to_u8(_FILE_NO_DIRS_, __LINE__, value)
static inline u8 vpl_priv_u32_to_u8(char const* file, int line, u32 value)
{
#ifdef _DEBUG
    if(value > UINT8_MAX)
    {
        safeConversionAssertFailed(file, line, "<"FMTu32"> too big for u8", value);
    }
#else
    UNUSED(file);
    UNUSED(line);
#endif
    return (u8)value;
}

#define U32_TO_S16(value)  vpl_priv_u32_to_s16(_FILE_NO_DIRS_, __LINE__, value)
static inline s16 vpl_priv_u32_to_s16(char const* file, int line, u32 value)
{
#ifdef _DEBUG
    if(value > INT16_MAX)
    {
        safeConversionAssertFailed(file, line, "<"FMTu32"> too big for s16", value);
    }
#else
    UNUSED(file);
    UNUSED(line);
#endif
    return (s16)value;
}

#define U32_TO_U16(value)  vpl_priv_u32_to_u16(_FILE_NO_DIRS_, __LINE__, value)
static inline u16 vpl_priv_u32_to_u16(char const* file, int line, u32 value)
{
#ifdef _DEBUG
    if(value > UINT16_MAX)
    {
        safeConversionAssertFailed(file, line, "<"FMTu32"> too big for u16", value);
    }
#else
    UNUSED(file);
    UNUSED(line);
#endif
    return (u16)value;
}

#define U32_TO_S32(value)  vpl_priv_u32_to_s32(_FILE_NO_DIRS_, __LINE__, value)
static inline s32 vpl_priv_u32_to_s32(char const* file, int line, u32 value)
{
#ifdef _DEBUG
    if((s64)value > INT32_MAX)
    {
        safeConversionAssertFailed(file, line, "<"FMTu32"> too big for s32", value);
    }
#else
    UNUSED(file);
    UNUSED(line);
#endif
    return (s32)value;
}

static inline s64 U32_TO_S64(u32 value)
{
    // trivially safe
    return (s64)value;
}

static inline u64 U32_TO_U64(u32 value)
{
    // trivially safe
    return (u64)value;
}

#define U32_TO_INT(value)  vpl_priv_u32_to_int(_FILE_NO_DIRS_, __LINE__, value)
static inline int vpl_priv_u32_to_int(char const* file, int line, u32 value)
{
#ifdef _DEBUG
    if((s64)value > INT_MAX)
    {
        safeConversionAssertFailed(file, line, "<"FMTu32"> too big for int", value);
    }
#else
    UNUSED(file);
    UNUSED(line);
#endif
    return (int)value;
}

#define U32_TO_SIZE_T(value)  vpl_priv_u32_to_size_t(_FILE_NO_DIRS_, __LINE__, value)
static inline size_t vpl_priv_u32_to_size_t(char const* file, int line, u32 value)
{
#ifdef _DEBUG
    u32 u32Size = sizeof(u32);
    u32 sizeSizeT = sizeof(size_t);
    if(u32Size > sizeSizeT) {
        safeConversionAssertFailed(file, line, "u32 size <"FMTu32"> is larger than sizeof(size_t) <"FMTu32">", u32Size, sizeSizeT);
    }
#else
    UNUSED(file);
    UNUSED(line);
#endif
    return (size_t)value;
}

//------------------------------------------------------------------------

#define S64_TO_S32(value)  vpl_priv_s64_to_s32(_FILE_NO_DIRS_, __LINE__, value)
static inline s32 vpl_priv_s64_to_s32(char const* file, int line, s64 value)
{
#ifdef _DEBUG
    if(value > INT32_MAX) {
        safeConversionAssertFailed(file, line, "<"FMTs64"> too big for s32", value);
    }
    if(value < INT32_MIN) {
        safeConversionAssertFailed(file, line, "<"FMTs64"> too negative for s32", value);
    }
#else
    UNUSED(file);
    UNUSED(line);
#endif
    return (s32)value;
}

//------------------------------------------------------------------------

#define S64_TO_U64(value)  vpl_priv_s64_to_u64(_FILE_NO_DIRS_, __LINE__, value)
static inline u64 vpl_priv_s64_to_u64(char const* file, int line, s64 value)
{
#ifdef _DEBUG
    if (value < 0) {
        safeConversionAssertFailed(file, line, "<"FMTs64"> being converted to unsigned", value);
    }
#else
    UNUSED(file);
    UNUSED(line);
#endif
    return (u64)value;
}

//------------------------------------------------------------------------

#define U64_TO_U16(value)  _u64_to_u16(_FILE_NO_DIRS_, __LINE__, value)
static inline u16 _u64_to_u16(char const* file, int line, u64 value)
{
#ifdef _DEBUG
    if(value > UINT16_MAX) {
        safeConversionAssertFailed(file, line, "<"FMTu64"> too big for u16", value);
    }
#else
    UNUSED(file);
    UNUSED(line);
#endif
    return (u16)value;
}

#define U64_TO_S32(value)  _u64_to_s32(_FILE_NO_DIRS_, __LINE__, value)
static inline s32 _u64_to_s32(char const* file, int line, u64 value)
{
#ifdef _DEBUG
    if(value > INT32_MAX) {
        safeConversionAssertFailed(file, line, "<"FMTu64"> too big for s32", value);
    }
#else
    UNUSED(file);
    UNUSED(line);
#endif
    return (s32)value;
}

#define U64_TO_U32(value)  _u64_to_u32(_FILE_NO_DIRS_, __LINE__, value)
static inline u32 _u64_to_u32(char const* file, int line, u64 value)
{
#ifdef _DEBUG
    if(value > UINT32_MAX) {
        safeConversionAssertFailed(file, line, "<"FMTu64"> too big for u32", value);
    }
#else
    UNUSED(file);
    UNUSED(line);
#endif
    return (u32)value;
}

#define U64_TO_S64(value)  vpl_priv_u64_to_s64(_FILE_NO_DIRS_, __LINE__, value)
static inline s64 vpl_priv_u64_to_s64(char const* file, int line, u64 value)
{
#ifdef _DEBUG
    if(value > (u64)INT64_MAX) {
        safeConversionAssertFailed(file, line, "<"FMTu64"> too big for s64", value);
    }
#else
    UNUSED(file);
    UNUSED(line);
#endif
    return (s64)value;
}

#define U64_TO_INT(value)  vpl_priv_u64_to_int(_FILE_NO_DIRS_, __LINE__, value)
static inline int vpl_priv_u64_to_int(char const* file, int line, u64 value)
{
#ifdef _DEBUG
    if(value > INT_MAX)
    {
        safeConversionAssertFailed(file, line, "<"FMTu64"> too big for int", value);
    }
#else
    UNUSED(file);
    UNUSED(line);
#endif
    return (int)value;
}

//------------------------------------------------------------------------

#define INT_TO_U8(value)  int_to_u8(_FILE_NO_DIRS_, __LINE__, value)
static inline u8 int_to_u8(char const* file, int line, int value)
{
#ifdef _DEBUG
    if(value < 0)
    {
        safeConversionAssertFailed(file, line, "<"FMTint"> being converted to unsigned", value);
    }
    if(value > UINT8_MAX)
    {
        safeConversionAssertFailed(file, line, "<"FMTint"> too big for u8", value);
    }
#else
    UNUSED(file);
    UNUSED(line);
#endif
    return (u8)value;
}

#define INT_TO_S16(value)  vpl_priv_int_to_s16(_FILE_NO_DIRS_, __LINE__, value)
static inline s16 vpl_priv_int_to_s16(char const* file, int line, int value)
{
#ifdef _DEBUG
    if(value > INT16_MAX) {
        safeConversionAssertFailed(file, line, "<"FMTint"> too big for s16", value);
    }
    if(value < INT16_MIN) {
        safeConversionAssertFailed(file, line, "<"FMTint"> too negative for s16", value);
    }
#else
    UNUSED(file);
    UNUSED(line);
#endif
    return (s16)value;
}

#define INT_TO_U16(value)  vpl_priv_int_to_u16(_FILE_NO_DIRS_, __LINE__, value)
static inline u16 vpl_priv_int_to_u16(char const* file, int line, int value)
{
#ifdef _DEBUG
    if(value < 0) {
        safeConversionAssertFailed(file, line, "<"FMTint"> being converted to unsigned", value);
    }
    if(value > UINT16_MAX) {
        safeConversionAssertFailed(file, line, "<"FMTint"> too big for u16", value);
    }
#else
    UNUSED(file);
    UNUSED(line);
#endif
    return (u16)value;
}

#define INT_TO_U32(value)  vpl_priv_int_to_u32(_FILE_NO_DIRS_, __LINE__, value)
static inline u32 vpl_priv_int_to_u32(char const* file, int line, int value)
{
#ifdef _DEBUG
    if(value < 0) {
        safeConversionAssertFailed(file, line, "<"FMTint"> being converted to unsigned", value);
    }
#  if VPL_PLAT_INT_IS_64BIT
    if(value > UINT32_MAX) {
        safeConversionAssertFailed(file, line, "<"FMTint"> too big for u32", value);
    }
#  endif
#else
    UNUSED(file);
    UNUSED(line);
#endif
    return (u32)value;
}

#define INT_TO_SIZE_T(value)  vpl_priv_int_to_size_t(_FILE_NO_DIRS_, __LINE__, value)
static inline size_t vpl_priv_int_to_size_t(char const* file, int line, int value)
{
#ifdef _DEBUG
    if(value < 0) {
        safeConversionAssertFailed(file, line, "<"FMTint"> being converted to unsigned", value);
    }
#else
    UNUSED(file);
    UNUSED(line);
#endif
    return (size_t)value;
}

//------------------------------------------------------------------------

/// We're intentionally switching between pointers and integers in the following functions.
#ifdef  __MWERKS__
  #ifndef __CDT_PARSER__
    #pragma warn_any_ptr_int_conv off
  #endif
#endif

// size_t should always be large enough to store any pointer, but we might as well check to avoid surprises.
#define PTR_TO_SIZE_T(value)  vpl_priv_ptr_to_size_t(_FILE_NO_DIRS_, __LINE__, value)
static inline size_t vpl_priv_ptr_to_size_t(char const* file, int line, void const* value)
{
#ifdef _DEBUG
    u32 ptrSize = sizeof(value);
    u32 sizeSize = sizeof(size_t);
    if(ptrSize > sizeSize)
    {
        safeConversionAssertFailed(file, line, "ptr size <"FMTu32"> is larger than sizeof(size_t) <"FMTu32">", ptrSize, sizeSize);
    }
#else
    UNUSED(file);
    UNUSED(line);
#endif
    return (size_t)value;
}

#define SIZE_T_TO_PTR(value)  vpl_priv_size_t_to_ptr(_FILE_NO_DIRS_, __LINE__, value)
static inline void* vpl_priv_size_t_to_ptr(char const* file, int line, size_t value)
{
#ifdef _DEBUG
    u32 ptrSize = sizeof(void*);
    u32 sizeSize = sizeof(value);
    if(ptrSize < sizeSize)
    {
        safeConversionAssertFailed(file, line, "ptr size <"FMTu32"> is less than sizeof(size_t) <"FMTu32">", ptrSize, sizeSize);
    }
#else
    UNUSED(file);
    UNUSED(line);
#endif
    return (void*)value;
}

/// Restore the original setting.
#ifdef  __MWERKS__
  #ifndef __CDT_PARSER__
    #pragma warn_any_ptr_int_conv reset
  #endif
#endif

//------------------------------------------------------------------------

#define SIZE_T_TO_U8(value)  vpl_priv_size_t_to_u8(_FILE_NO_DIRS_, __LINE__, value)
static inline u8 vpl_priv_size_t_to_u8(char const* file, int line, size_t value)
{
#ifdef _DEBUG
    if (value > UINT8_MAX) {
        safeConversionAssertFailed(file, line, "<"FMTuSizeT"> too big for u8", value);
    }
#else
    UNUSED(file);
    UNUSED(line);
#endif
    return (u8)value;
}

#define SIZE_T_TO_U16(value)  vpl_priv_size_t_to_u16(_FILE_NO_DIRS_, __LINE__, value)
static inline u16 vpl_priv_size_t_to_u16(char const* file, int line, size_t value)
{
#ifdef _DEBUG
    if (value > UINT16_MAX) {
        safeConversionAssertFailed(file, line, "<"FMTuSizeT"> too big for u16", value);
    }
#else
    UNUSED(file);
    UNUSED(line);
#endif
    return (u16)value;
}

#define SIZE_T_TO_U32(value)  vpl_priv_size_t_to_u32(_FILE_NO_DIRS_, __LINE__, value)
static inline u32 vpl_priv_size_t_to_u32(char const* file, int line, size_t value)
{
#ifdef _DEBUG
    if (value > UINT32_MAX) {
        safeConversionAssertFailed(file, line, "<"FMTuSizeT"> too big for u32", value);
    }
#else
    UNUSED(file);
    UNUSED(line);
#endif
    return (u32)value;
}

static inline u8* U8_PTR(void* ptr)
{
    return (u8*)ptr;
}

static inline u8 const* U8_CONST_PTR(void const* ptr)
{
    return (u8 const*)ptr;
}

#ifdef  __cplusplus
}
#endif

#endif // include guard
