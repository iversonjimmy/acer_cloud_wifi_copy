//
//  Copyright (C) 2005-2008, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#include "vplex_assert.h"
#include "vplex_private.h"
#include "vplex_serialization.h"
#include "vplex_safe_conversion.h"
#include "vplex_strings.h"
#include "vpl_conv.h"

void VPL_CheckSerialization(void)
{
    // Check VPLConv_ntoh configuration
    u8 buf[sizeof(u32)];
    (IGNORE_RESULT)VPL_PackU32(buf, 0xfe000000);
    ASSERTMSG((buf[0] == 0xfe), "Incorrect endianness configuration");
}

// U8
void* _VPL_packUnchecked_u8(void* dst, u8 src)
{
    memcpy(dst, &src, sizeof(u8));
    return SIZE_T_TO_PTR(PTR_TO_SIZE_T(dst) + sizeU8);
}
const void* _VPL_unpackUnchecked_u8(const void* src, u8* dst)
{
    memcpy(dst, src, sizeof(u8));
    return SIZE_T_TO_PTR(PTR_TO_SIZE_T(src) + sizeU8);
}

// S8
void* _VPL_packUnchecked_s8(void* dst, s8 src)
{
    memcpy(dst, &src, sizeof(s8));
    return SIZE_T_TO_PTR(PTR_TO_SIZE_T(dst) + sizeS8);
}
const void* _VPL_unpackUnchecked_s8(const void* src, s8* dst)
{
    memcpy(dst, src, sizeof(s8));
    return SIZE_T_TO_PTR(PTR_TO_SIZE_T(src) + sizeS8);
}

// U16
void* _VPL_packUnchecked_u16(void* dst, u16 src)
{
    u16 tmp = VPLConv_hton_u16(src);
    memcpy(dst, &tmp, sizeof(u16));
    return SIZE_T_TO_PTR(PTR_TO_SIZE_T(dst) + sizeU16);
}
const void* _VPL_unpackUnchecked_u16(const void* src, u16* dst)
{
    u16 tmp;
    memcpy(&tmp, src, sizeof(u16));
    *dst = VPLConv_ntoh_u16(tmp);
    return SIZE_T_TO_PTR(PTR_TO_SIZE_T(src) + sizeU16);
}

// S16
void* _VPL_packUnchecked_s16(void* dst, s16 src)
{
    s16 tmp = VPLConv_hton_s16(src);
    memcpy(dst, &tmp, sizeof(s16));
    return SIZE_T_TO_PTR(PTR_TO_SIZE_T(dst) + sizeS16);
}
const void* _VPL_unpackUnchecked_s16(const void* src, s16* dst)
{
    s16 tmp;
    memcpy(&tmp, src, sizeof(s16));
    *dst = VPLConv_ntoh_s16(tmp);
    return SIZE_T_TO_PTR(PTR_TO_SIZE_T(src) + sizeS16);
}

// U32
void* _VPL_packUnchecked_u32(void* dst, u32 src)
{
    u32 tmp = VPLConv_hton_u32(src);
    memcpy(dst, &tmp, sizeof(u32));
    return SIZE_T_TO_PTR(PTR_TO_SIZE_T(dst) + sizeU32);
}
const void* _VPL_unpackUnchecked_u32(const void* src, u32* dst)
{
    u32 tmp;
    memcpy(&tmp, src, sizeof(u32));
    *dst = VPLConv_ntoh_u32(tmp);
    return SIZE_T_TO_PTR(PTR_TO_SIZE_T(src) + sizeU32);
}

// S32
void* _VPL_packUnchecked_s32(void* dst, s32 src)
{
    s32 tmp = VPLConv_hton_s32(src);
    memcpy(dst, &tmp, sizeof(s32));
    return SIZE_T_TO_PTR(PTR_TO_SIZE_T(dst) + sizeS32);
}
const void* _VPL_unpackUnchecked_s32(const void* src, s32* dst)
{
    s32 tmp;
    memcpy(&tmp, src, sizeof(s32));
    *dst = VPLConv_ntoh_s32(tmp);
    return SIZE_T_TO_PTR(PTR_TO_SIZE_T(src) + sizeS32);
}

// U64
void* _VPL_packUnchecked_u64(void* dst, u64 src)
{
    u64 tmp = VPLConv_hton_u64(src);
    memcpy(dst, &tmp, sizeof(u64));
    return SIZE_T_TO_PTR(PTR_TO_SIZE_T(dst) + sizeU64);
}
const void* _VPL_unpackUnchecked_u64(const void* src, u64* dst)
{
    u64 tmp;
    memcpy(&tmp, src, sizeof(u64));
    *dst = VPLConv_ntoh_u64(tmp);
    return SIZE_T_TO_PTR(PTR_TO_SIZE_T(src) + sizeU64);
}

// S64
void* _VPL_packUnchecked_s64(void* dst, s64 src)
{
    s64 tmp = VPLConv_hton_s64(src);
    memcpy(dst, &tmp, sizeof(s64));
    return SIZE_T_TO_PTR(PTR_TO_SIZE_T(dst) + sizeS64);
}
const void* _VPL_unpackUnchecked_s64(const void* src, s64* dst)
{
    s64 tmp;
    memcpy(&tmp, src, sizeof(s64));
    *dst = VPLConv_ntoh_s64(tmp);
    return SIZE_T_TO_PTR(PTR_TO_SIZE_T(src) + sizeS64);
}

// Bytes
void* VPL_PackBytes(void* dst, const void* src, u32 size)
{
    memcpy(dst, src, size);
    return SIZE_T_TO_PTR(PTR_TO_SIZE_T(dst) + size);
}
const void* VPL_UnpackBytes(const void* src, void* dst, u32 size)
{
    memcpy(dst, src, size);
    return SIZE_T_TO_PTR(PTR_TO_SIZE_T(src) + size);
}

void* VPL_PackUTF8(void* dst, const utf8* src, u32 maxStrLen)
{
    if(maxStrLen > 0)
    {
        size_t const bytesForDst = _VPL_sizeBufferForUTF8(src, maxStrLen);
        VPLString_SafeStrncpy((char*)dst, bytesForDst, (const char*)src);
        return SIZE_T_TO_PTR(PTR_TO_SIZE_T(dst) + bytesForDst);
    }
    return dst;
}
const void* VPL_UnpackUTF8(const void* src, void* dst, u32 maxStrLen)
{
    if(maxStrLen > 0)
    {
        size_t const bytesForDst = _VPL_sizeBufferForUTF8((const utf8*)src, maxStrLen);
        VPLString_SafeStrncpy((char*)dst, bytesForDst, (const char*)src);
        return SIZE_T_TO_PTR(PTR_TO_SIZE_T(src) + bytesForDst);
    }
    return src;
}

// String (variable size)
void* VPL_PackString(void* dst, const char* src, u32 maxStrLen)
{
    if(maxStrLen > 0)
    {
        size_t const bytesForDst = _VPL_sizeBufferForString(src, maxStrLen);
        VPLString_SafeStrncpy((char*)dst, bytesForDst, src);
        return SIZE_T_TO_PTR(PTR_TO_SIZE_T(dst) + bytesForDst);
    }
    return dst;
}
const void* VPL_UnpackString(const void* src, void* dst, u32 maxStrLen)
{
    if(maxStrLen > 0)
    {
        size_t const bytesForDst = _VPL_sizeBufferForString((const char*)src, maxStrLen);
        VPLString_SafeStrncpy((char*)dst, bytesForDst, (const char*)src);
        return SIZE_T_TO_PTR(PTR_TO_SIZE_T(src) + bytesForDst);
    }
    return src;
}

// Base64
static char base64_enc[64];
static char base64_pad = '=';
static s8 base64_dec[256];
static s8 base64_isInit = 0;

static void VPL_InitBase64(void)
{
    int i;
    for (i = 0; i < 26; i++) {
        base64_enc[i] = 'A' + i;
    }
    for (i = 0; i < 26; i++) {
        base64_enc[i+26] = 'a' + i;
    }
    for (i = 0; i < 10; i++) {
        base64_enc[i+52] = '0' + i;
    }
    base64_enc[62] = '+';
    base64_enc[63] = '/';
    for (i = 0; i < 256; i++) {
        base64_dec[i] = -1;
    }
    for (i = 0; i < 64; i++) {
        base64_dec[(unsigned)base64_enc[i]] = i;
    }
    // Always decode the URL-safe substitutions.
    base64_dec[(unsigned)'-'] = 62;
    base64_dec[(unsigned)'_'] = 63;
    base64_isInit = 1;
}

static char makeUrlSafe(char in)
{
    if (in == '+') {
        return '-';
    } else if (in == '/') {
        return '_';
    } else {
        return in;
    }
}

void VPL_EncodeBase64(const void* src, size_t srcLen, char* dst, size_t* dstLen,
        VPL_BOOL addNewlines, VPL_BOOL urlSafe)
{
    size_t count = 0;
    size_t origDstLen = *dstLen;

    if (!base64_isInit) {
        VPL_InitBase64();
    }
    if (addNewlines) {
        *dstLen = VPL_BASE64_ENCODED_BUF_LEN(srcLen);
    } else {
        *dstLen = VPL_BASE64_ENCODED_SINGLE_LINE_BUF_LEN(srcLen);
    }
    if (origDstLen < *dstLen) {
        VPL_LIB_LOG_WARN(VPL_SG_MISC,
                "For srcLen="FMTu_size_t", need "FMTu_size_t" bytes, but only got "FMTu_size_t,
                srcLen, *dstLen, origDstLen);
        return;
    }

    while (count < srcLen) {
        u32 v24 = ((u32)(((u8*)src)[count])) << 16;
        if (count + 1 < srcLen) {
            v24 |= ((u32)(((u8*)src)[count + 1])) << 8;
        }
        if (count + 2 < srcLen) {
            v24 |= (u32)(((u8*)src)[count + 2]);
        }
        dst[0] = base64_enc[(v24 >> 18) & 0x3f];
        dst[1] = base64_enc[(v24 >> 12) & 0x3f];
        dst[2] = (count + 1 < srcLen) ? base64_enc[(v24 >> 6) & 0x3f] : base64_pad;
        dst[3] = (count + 2 < srcLen) ? base64_enc[(v24) & 0x3f] : base64_pad;
        if (urlSafe) {
            dst[0] = makeUrlSafe(dst[0]);
            dst[1] = makeUrlSafe(dst[1]);
            dst[2] = makeUrlSafe(dst[2]);
            dst[3] = makeUrlSafe(dst[3]);
        }
        count += 3;
        dst += 4;
        if (addNewlines && ((count % (76 / 4 * 3)) == 0)) {
            dst[0] = '\n';
            dst++;
        }
    }
    *dst = '\0';
}

void VPL_DecodeBase64(const char* src, size_t srcLen, void* dst, size_t* dstLen)
{
    u32 v24 = 0;
    size_t count = 0;
    u8 shift = 0;
    u8 numPaddingChars = 0;
    size_t dstCapacity = *dstLen;
    u32 dstSize = 0;

    if (!base64_isInit)
        VPL_InitBase64();

    for (count = 0; count < srcLen; count++) {
        s8 currCharVal = base64_dec[(unsigned)src[count]];
        if (currCharVal != (s8)-1) {
            v24 = (v24 << 6) | (currCharVal & 0x3f);
            shift++;
        }
        if (count == srcLen - 1) {
            // At the last byte; assume padding (if needed) to
            // get us to 4 encoded characters.
            if (shift != 0) {
                numPaddingChars = (4 - shift);
                // Only 0, 1, or 2 characters may be padding.
                // 3 is never allowed.
                if (numPaddingChars == 3) {
                    VPL_LIB_LOG_WARN(VPL_SG_MISC, "Invalid base64 encoding (srcLen="FMTu_size_t"): \"%.*s\"",
                            srcLen, (int)srcLen, src);
                }
                v24 <<= (6 * numPaddingChars);
                shift = 4; // Same as "shift += numPaddingChars"
            }
        }
        // When we've read 4 characters from src, we can emit 3 bytes to dst.
        if (shift == 4) {
            
            if (dstSize < dstCapacity) {
                ((u8*)dst)[dstSize] = (v24 >> 16) & 0xff;
            }
            dstSize++;
            
            if (dstSize < dstCapacity) {
                ((u8*)dst)[dstSize] = (v24 >> 8) & 0xff;
            }
            dstSize++;
            
            if (dstSize < dstCapacity) {
                ((u8*)dst)[dstSize] = v24 & 0xff;
            }
            dstSize++;

            v24 = 0;
            shift = 0;
        }
    }
    
    // Subtract off any bytes that are due to padding.
    // Since the only valid values for numPaddingChars (the number of encoded padding characters) are
    // 0, 1, and 2, the number of excess decoded bytes is always the same as numPaddingChars.
    dstSize -= numPaddingChars;
    
    *dstLen = dstSize;
}
