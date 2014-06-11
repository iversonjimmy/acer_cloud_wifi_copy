//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __VPL_CONV_H__
#define __VPL_CONV_H__

//============================================================================
/// @file
/// Virtual Platform Layer API for standard host/network byte order conversions.
//============================================================================

#include "vpl_plat.h"

// Platform specific definitions.
#include "vpl__conv.h"

/// Reverse the byte order for a 2 byte variable.
#define VPLCONV_BYTE_SWAP16(x)  ((((x) << 8) & 0xff00) | (((x) >> 8) & 0xff))

/// Reverse the byte order for a 4 byte variable.
#define VPLCONV_BYTE_SWAP32(x)  ((((x) << 24) & 0xff000000) | (((x) << 8) & 0xff0000) | (((x) >> 8) & 0xff00) | (((x) >> 24) & 0xff))

/// Reverse the byte order for an 8 byte variable.
#define VPLCONV_BYTE_SWAP64(x) \
    ((((x) << 56) & 0xff00000000000000ll) | \
     (((x) << 40) & 0x00ff000000000000ll) | \
     (((x) << 24) & 0x0000ff0000000000ll) | \
     (((x) << 8)  & 0x000000ff00000000ll) | \
     (((x) >> 8)  & 0x00000000ff000000ll) | \
     (((x) >> 24) & 0x0000000000ff0000ll) | \
     (((x) >> 40) & 0x000000000000ff00ll) | \
     (((x) >> 56) & 0x00000000000000ffll))

///
/// @name "ntoh" functions
/// Convert from network byte order to host byte order.
//@{

/// Convert \a x from host byte order to network byte order and preserve the type.
#if VPL_HOST_IS_LITTLE_ENDIAN
static inline int64_t VPLConv_ntoh_s64(int64_t value) { return (int64_t)VPLCONV_BYTE_SWAP64((uint64_t)value); }
static inline int32_t VPLConv_ntoh_s32(int32_t value) { return (int32_t)VPLCONV_BYTE_SWAP32((uint32_t)value); }
static inline int16_t VPLConv_ntoh_s16(int16_t value) { return (int16_t)VPLCONV_BYTE_SWAP16((uint16_t)value); }
static inline int8_t  VPLConv_ntoh_s8 (int8_t value)  { return value; }

static inline uint64_t VPLConv_ntoh_u64(uint64_t value) { return (uint64_t)VPLCONV_BYTE_SWAP64(value); }
static inline uint32_t VPLConv_ntoh_u32(uint32_t value) { return (uint32_t)VPLCONV_BYTE_SWAP32(value); }
static inline uint16_t VPLConv_ntoh_u16(uint16_t value) { return (uint16_t)VPLCONV_BYTE_SWAP16(value); }
static inline uint8_t  VPLConv_ntoh_u8 (uint8_t value)  { return value; }
#else
static inline int64_t VPLConv_ntoh_s64(int64_t value) { return value; }
static inline int32_t VPLConv_ntoh_s32(int32_t value) { return value; }
static inline int16_t VPLConv_ntoh_s16(int16_t value) { return value; }
static inline int8_t  VPLConv_ntoh_s8 (int8_t value)  { return value; }

static inline uint64_t VPLConv_ntoh_u64(uint64_t value) { return value; }
static inline uint32_t VPLConv_ntoh_u32(uint32_t value) { return value; }
static inline uint16_t VPLConv_ntoh_u16(uint16_t value) { return value; }
static inline uint8_t  VPLConv_ntoh_u8 (uint8_t value)  { return value; }
#endif
//@}

///
/// @name "hton" functions
/// Convert from host byte order to network byte order.
//@{

/// Convert \a x from host byte order to network byte order and preserve the type.
static inline int64_t VPLConv_hton_s64(int64_t value) { return VPLConv_ntoh_s64(value); }
static inline int32_t VPLConv_hton_s32(int32_t value) { return VPLConv_ntoh_s32(value); }
static inline int16_t VPLConv_hton_s16(int16_t value) { return VPLConv_ntoh_s16(value); }
static inline int8_t  VPLConv_hton_s8 (int8_t  value) { return VPLConv_ntoh_s8(value); }

static inline uint64_t VPLConv_hton_u64(uint64_t value) { return VPLConv_ntoh_u64(value); }
static inline uint32_t VPLConv_hton_u32(uint32_t value) { return VPLConv_ntoh_u32(value); }
static inline uint16_t VPLConv_hton_u16(uint16_t value) { return VPLConv_ntoh_u16(value); }
static inline uint8_t  VPLConv_hton_u8 (uint8_t  value) { return VPLConv_ntoh_u8(value); }
//@}

#ifdef _MSC_VER
/// Only supports input strings in the range INT64_MIN (-2^63) to INT64_MAX (2^63 - 1), inclusive.
static inline int64_t VPLConv_strToS64(const char *nptr, char **endptr, int base) { return _strtoi64(nptr, endptr, base); }
/// Designed to handle input strings in the range INT64_MIN (-2^63) to UINT64_MAX (2^64 - 1), inclusive.
/// Negative values are mapped to their corresponding s64 bit representation.
static inline uint64_t VPLConv_strToU64(const char *nptr, char **endptr, int base) { return _strtoui64(nptr, endptr, base); }
#else
/// Only supports input strings in the range INT64_MIN (-2^63) to INT64_MAX (2^63 - 1), inclusive.
static inline int64_t VPLConv_strToS64(const char *nptr, char **endptr, int base) { return strtoll(nptr, endptr, base); }
/// Designed to handle input strings in the range INT64_MIN (-2^63) to UINT64_MAX (2^64 - 1), inclusive.
/// Negative values are mapped to their corresponding s64 bit representation.
static inline uint64_t VPLConv_strToU64(const char *nptr, char **endptr, int base) { return strtoull(nptr, endptr, base); }
#endif

#endif  // include guard
