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
#include "vplex_safe_serialization.h"
#include "vplex_safe_conversion.h"
#include "vplex_strings.h"
#include "vpl_conv.h"

#define CHECK_BUFFER(vplBuffer, required) \
    do { \
        if ((vplBuffer)->pos == VPL_SERIALIZATION_OVERRUN) \
            return; \
        if ((vplBuffer)->size - (vplBuffer)->pos < (required)) { \
            (vplBuffer)->pos = VPL_SERIALIZATION_OVERRUN; \
            return; \
        } \
    } while (0 == 1)

static inline void advanceBufferIn(VPLInputBuffer * buf, u32 len)
{
    buf->pos += len;
    buf->buf = (const void *)((const u8 *)buf->buf + len);
}
static inline void advanceBufferOut(VPLOutputBuffer * buf, u32 len)
{
    buf->pos += len;
    buf->buf = (void *)((u8 *)buf->buf + len);
}

void VPLInitInputBuffer(VPLInputBuffer * inputBuffer, const void * rawIn,
        u32 length) {
    inputBuffer->buf = rawIn;
    inputBuffer->size = length;
    inputBuffer->pos = 0;
}

void VPLInitOutputBuffer(VPLOutputBuffer * outputBuffer, void * rawOut,
        u32 length) {
    outputBuffer->buf = rawOut;
    outputBuffer->size = length;
    outputBuffer->pos = 0;
}

s32 VPLHasOverrunIn(VPLInputBuffer * inputBuffer) {
    return (inputBuffer->pos == VPL_SERIALIZATION_OVERRUN);
}

s32 VPLHasOverrunOut(VPLOutputBuffer * outputBuffer) {
    return (outputBuffer->pos == VPL_SERIALIZATION_OVERRUN);
}

u32 VPLOutputBufferSize(VPLOutputBuffer * outputBuffer)
{
    return outputBuffer->pos;
}

u32 VPLInputBufferRemaining(VPLInputBuffer * inputBuffer)
{
    return (inputBuffer->size - inputBuffer->pos);
}

u32 VPLOutputBufferRemaining(VPLOutputBuffer * outputBuffer)
{
    return (outputBuffer->size - outputBuffer->pos);
}

#define DEFINE_PACKUNCHECKED(type) \
    void _VPL_PackUnchecked_##type(VPLOutputBuffer * dst, type src) \
    { \
        type tmp = VPLConv_hton_##type(src); \
        CHECK_BUFFER(dst, sizeof(type)); \
        memcpy(dst->buf, &tmp, sizeof(type)); \
        advanceBufferOut(dst, sizeof(type)); \
    }

#define DEFINE_UNPACKUNCHECKED(type) \
    void _VPL_UnpackUnchecked_##type(VPLInputBuffer * src, type * dst) \
    { \
        type tmp; \
        CHECK_BUFFER(src, sizeof(type)); \
        memcpy(&tmp, src->buf, sizeof(type)); \
        advanceBufferIn(src, sizeof(type)); \
        *dst = VPLConv_ntoh_##type(tmp); \
    }

DEFINE_PACKUNCHECKED(u8)
DEFINE_UNPACKUNCHECKED(u8)
DEFINE_PACKUNCHECKED(s8)
DEFINE_UNPACKUNCHECKED(s8)
DEFINE_PACKUNCHECKED(u16)
DEFINE_UNPACKUNCHECKED(u16)
DEFINE_PACKUNCHECKED(s16)
DEFINE_UNPACKUNCHECKED(s16)
DEFINE_PACKUNCHECKED(u32)
DEFINE_UNPACKUNCHECKED(u32)
DEFINE_PACKUNCHECKED(s32)
DEFINE_UNPACKUNCHECKED(s32)
DEFINE_PACKUNCHECKED(u64)
DEFINE_UNPACKUNCHECKED(u64)
DEFINE_PACKUNCHECKED(s64)
DEFINE_UNPACKUNCHECKED(s64)

// Bytes
void VPLPackBytes(VPLOutputBuffer * dst, const void * src, u32 size)
{
    CHECK_BUFFER(dst, size);
    memcpy(dst->buf, src, size);
    advanceBufferOut(dst, size);
}
void VPLUnpackBytes(VPLInputBuffer * src, void * dst, u32 size)
{
    CHECK_BUFFER(src, size);
    memcpy(dst, src->buf, size);
    advanceBufferIn(src, size);
}

// UTF8 strings
void VPLPackUTF8(VPLOutputBuffer * dst, const utf8 * src, u32 maxStrLen)
{
    // TODO: are these checks necessary?
    if(maxStrLen > 0)
    {
        size_t const bytesForDst = _VPL_SizeBufferForUTF8(src, maxStrLen);
        CHECK_BUFFER(dst, bytesForDst);
        VPLString_SafeStrncpy((char *)dst->buf, bytesForDst,
                (const char *)src);
        advanceBufferOut(dst, (u32)bytesForDst);
    }
}
void VPLUnpackUTF8(VPLInputBuffer * src, void * dst, u32 maxStrLen)
{
    if(maxStrLen > 0)
    {
        // Truncate maximum length if there is less remaining in the buffer
        u32 bufferRemaining = src->size - src->pos - 1;
        u32 realMaxStrLen =
                (bufferRemaining > maxStrLen ? maxStrLen : bufferRemaining);
        size_t const bytesForDst = _VPL_SizeBufferForUTF8((const utf8*)src->buf, realMaxStrLen);
        CHECK_BUFFER(src, 1);
        // Ensure that a null terminator was present.  This should always be
        // the case for a string serialized with packUTF8.
        if (((u8 *)src->buf)[bytesForDst-1] != 0) {
            src->pos = VPL_SERIALIZATION_OVERRUN;
            return;
        }
        // Copy to dst
        VPLString_SafeStrncpy((char *)dst, bytesForDst, (const char*)src->buf);
        advanceBufferIn(src, (u32)bytesForDst);
    }
}

// String (variable size)
// TODO: these are the same as the UTF8 methods.
void VPLPackString(VPLOutputBuffer * dst, const char * src, u32 maxStrLen)
{
    if(maxStrLen > 0)
    {
        size_t const bytesForDst = _VPL_SizeBufferForString(src, maxStrLen);
        CHECK_BUFFER(dst, bytesForDst);
        VPLString_SafeStrncpy((char *)dst->buf, bytesForDst, src);
        advanceBufferOut(dst, (u32)bytesForDst);
    }
}
void VPLUnpackString(VPLInputBuffer * src, void * dst, u32 maxStrLen)
{
    if(maxStrLen > 0)
    {
        u32 bufferRemaining = src->size - src->pos - 1;
        u32 realMaxStrLen =
                (bufferRemaining > maxStrLen ? maxStrLen : bufferRemaining);
        size_t const bytesForDst = _VPL_SizeBufferForString((const char*)src->buf, realMaxStrLen);
        CHECK_BUFFER(src, 1);
        if (((u8 *)src->buf)[bytesForDst-1] != 0) {
            src->pos = VPL_SERIALIZATION_OVERRUN;
            return;
        }
        VPLString_SafeStrncpy((char *)dst, bytesForDst, (const char*)src->buf);
        advanceBufferIn(src, (u32)bytesForDst);
    }
}
