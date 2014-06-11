//
//  Copyright (C) 2005-2008, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#ifndef __VPL_SAFE_FLOAT_CONVERSION_H__
#define __VPL_SAFE_FLOAT_CONVERSION_H__

//============================================================================
/// @file
///
//============================================================================

#include <vplex_safe_conversion.h>

static inline float S16_TO_FLOAT(s16 value)
{
    // trivially safe
    return (float)value;
}

static inline float U16_TO_FLOAT(u16 value)
{
    // trivially safe
    return (float)value;
}

static inline float S32_TO_FLOAT(s32 value)
{
    // trivially safe
    return (float)value;
}

static inline double U32_TO_DOUBLE(u32 value)
{
    // trivially safe
    return (double)value;
}

static inline double U64_TO_DOUBLE(u64 value)
{
    // trivially safe
    return (double)value;
}

#define FLOAT_TO_S16(value)  vpl_priv_float_to_s16(_FILE_NO_DIRS_, __LINE__, value)
static inline s16 vpl_priv_float_to_s16(char const* file, int line, float value)
{
#ifdef _DEBUG
    if (value > INT16_MAX) {
        safeConversionAssertFailed(file, line, "<%f> too big for s16", value);
    }
    if (value < INT16_MIN) {
        safeConversionAssertFailed(file, line, "<%f> too negative for s16", value);
    }
#else
    UNUSED(file);
    UNUSED(line);
#endif
    return (s16)value;
}

#define FLOAT_TO_U16(value)  vpl_priv_float_to_u16(_FILE_NO_DIRS_, __LINE__, value)
static inline u16 vpl_priv_float_to_u16(char const* file, int line, float value)
{
#ifdef _DEBUG
    if (value < 0) {
        safeConversionAssertFailed(file, line, "<%f> being converted to unsigned", value);
    }
    if (value > UINT16_MAX) {
        safeConversionAssertFailed(file, line, "<%f> too big for u16", value);
    }
#else
    UNUSED(file);
    UNUSED(line);
#endif
    return (u16)value;
}

#define FLOAT_TO_S32(value)  vpl_priv_float_to_s32(_FILE_NO_DIRS_, __LINE__, value)
static inline s32 vpl_priv_float_to_s32(char const* file, int line, float value)
{
#ifdef _DEBUG
    if (value > INT32_MAX) {
        safeConversionAssertFailed(file, line, "<%f> too big for s32", value);
    }
    if (value < INT32_MIN) {
        safeConversionAssertFailed(file, line, "<%f> too negative for s32", value);
    }
#else
    UNUSED(file);
    UNUSED(line);
#endif
    return (s32)value;
}

#endif // include guard
