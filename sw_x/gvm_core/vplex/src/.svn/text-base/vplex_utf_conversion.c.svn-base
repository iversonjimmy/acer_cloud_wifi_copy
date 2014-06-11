#include "vplex_wstring.h"
#include "vplex_safe_conversion.h"
#include "vpl_conv.h"

#define UTF8_1BYTE_RANGE_MAX  0x7F
#define UTF8_2BYTE_RANGE_MAX  0x7FF
#define UTF8_3BYTE_RANGE_MAX  0xFFFF
#define UTF8_4BYTE_RANGE_MAX  0x10FFFF

// TODO: fix read/write beyond buffer cases
/**
 * Convert one character from a UTF-16BE buffer to a UTF-8 buffer.
 * @param utf16CharBuf Buffer containing a UTF-16BE character (could be either 16 or 32 bits).
 * @param utf16BufConsumed_in_out [in] Maximum number of bytes to read from \a utf16CharBuf.
 *     [out] Actual number of bytes consumed from \a utf16CharBuf.
 * @param utf16Buf_out On success, the next character will be output in UTF-16
 *     format at this location.
 * @param utf16BufLen_in_out [in] Maximum number of bytes to write to \a utf16Buf_out.
 *     [out] Actual number of bytes written to \a utf16Buf_out.
 * @return true for success, false if there was a problem.
 */
static bool vpl_utf16CharToUtf8Char(
        u16 const* utf16CharBuf,
        size_t* utf16len,
        utf8* utf8Char,
        size_t* utf8len)
{
    u32 codePoint = 0;

    // determine if the utf16 char is 16bits or 32bits
    if (VPLConv_ntoh_u16(*utf16CharBuf) > 0xD800 && VPLConv_ntoh_u16(*utf16CharBuf) <= 0xDFFF) {
        *utf16len = 2;
        codePoint = INT_TO_U32(VPLConv_ntoh_u16(utf16CharBuf[0]) & 0x27FF);
        codePoint = (codePoint << 10) | (utf16CharBuf[1] & 0x23FF);
    } else {
        *utf16len = 1;
        codePoint = INT_TO_U32(VPLConv_ntoh_u16(utf16CharBuf[0]));
    }

    if (codePoint <= UTF8_1BYTE_RANGE_MAX) {
        // 1 byte
        if (*utf8len < 1) {
            return false;
        }
        utf8Char[0] = U32_TO_U8(codePoint);
        *utf8len = 1;
    }
    else if (codePoint <= UTF8_2BYTE_RANGE_MAX) {
        // 2 bytes
        if (*utf8len < 2) {
            return false;
        }
        utf8Char[1] = (u8) (0x80 | ( codePoint        & 0x3F));  // low  6 bits
        utf8Char[0] = (u8) (0xC0 | ((codePoint >>  6) & 0x1F));  // next 5 bits
        *utf8len = 2;
    }
    else if (codePoint <= UTF8_3BYTE_RANGE_MAX) {
        // 3 bytes
        if (*utf8len < 3) {
            return false;
        }
        utf8Char[2] = (u8) (0x80 | ( codePoint        & 0x3F));  // low  6 bits
        utf8Char[1] = (u8) (0x80 | ((codePoint >>  6) & 0x3F));  // next 6 bits
        utf8Char[0] = (u8) (0xE0 | ((codePoint >> 12) & 0x0F));  // next 4 bits
        *utf8len = 3;
    }
    else if (codePoint <= UTF8_4BYTE_RANGE_MAX) {
        // 4 bytes
        if (*utf8len < 4) {
            return false;
        }
        utf8Char[3] = (u8) (0x80 | ( codePoint        & 0x3F));  // low  6 bits
        utf8Char[2] = (u8) (0x80 | ((codePoint >>  6) & 0x3F));  // next 6 bits
        utf8Char[1] = (u8) (0x80 | ((codePoint >> 12) & 0x3F));  // next 6 bits
        utf8Char[0] = (u8) (0xF0 | ((codePoint >> 18) & 0x07));  // next 3 bits
        *utf8len = 4;
    }
    else {
        return false;
    }
    return true;
}

VPLUtf8ByteType VPLString_ClassifyUtf8Byte(utf8 byte)
{
    if (byte == 0x00) {
        return VPLUtf8_NULL_TERM;
    }
    else if (byte <= 0x7F) {
        return VPLUtf8_ASCII;
    }
    else if (byte <= 0xBF) {
        return VPLUtf8_BYTE_2_3_OR_4;
    }
    else if (byte <= 0xC1) {
        return VPLUtf8_INVALID;
    }
    else if (byte <= 0xDF) {
        return VPLUtf8_START_OF_2_BYTE_CHAR;
    }
    else if (byte <= 0xEF) {
        return VPLUtf8_START_OF_3_BYTE_CHAR;
    }
    else if (byte <= 0xF4) {
        return VPLUtf8_START_OF_4_BYTE_CHAR;
    }
    else {
        return VPLUtf8_INVALID;
    }
}

/**
 * Convert one character from a UTF-8 buffer to a UTF-16BE buffer.
 * @param utf8CharBuf Buffer containing a [potentially multibyte] UTF-8 character.
 * @param utf8BufConsumed_in_out [in] Maximum number of bytes to read from \a utf8CharBuf.
 *     [out] Actual number of bytes consumed from \a utf8CharBuf.
 * @param utf16Buf_out On success, the next character will be output in UTF-16
 *     format at this location.
 * @param utf16BufLen_in_out [in] Maximum number of bytes to write to \a utf16Buf_out.
 *     [out] Actual number of bytes written to \a utf16Buf_out.
 * @return true for success, false if there was a problem.
 */
static bool vpl_utf8CharToUtf16Char(
        utf8 const* utf8CharBuf,
        size_t* utf8BufConsumed_in_out,
        u16* utf16Buf_out,
        size_t* utf16BufConsumed_in_out)
{
    u32 codePoint = 0;
    size_t utf8BufLen_in = *utf8BufConsumed_in_out;
    size_t utf16BufLen_in = *utf16BufConsumed_in_out;
    VPLUtf8ByteType type = VPLString_ClassifyUtf8Byte(utf8CharBuf[0]);
    *utf16BufConsumed_in_out = 0;
    
    // Determine how many bytes the utf8 char is.
    switch(type) {
        case VPLUtf8_NULL_TERM:
        case VPLUtf8_ASCII:
            *utf8BufConsumed_in_out = 1;
            break;
        case VPLUtf8_START_OF_2_BYTE_CHAR:
            *utf8BufConsumed_in_out = 2;
            break;
        case VPLUtf8_START_OF_3_BYTE_CHAR:
            *utf8BufConsumed_in_out = 3;
            break;
        case VPLUtf8_START_OF_4_BYTE_CHAR:
            *utf8BufConsumed_in_out = 4;
            break;
    case VPLUtf8_BYTE_2_3_OR_4: // not accepted
    case VPLUtf8_INVALID: // A placeholder, not a legimitate type.
        default:
            // Invalid value for the first byte in a sequence.
            // Consume the byte and return false.
            *utf8BufConsumed_in_out = 1;
            return false;
    }
    
    // Avoid reading past the end of the buffer.
    if (utf8BufLen_in < *utf8BufConsumed_in_out) {
        return false;
    }
    
    switch(*utf8BufConsumed_in_out) {
        case 1:
            codePoint = utf8CharBuf[0];
            break;
        case 2:
            codePoint = INT_TO_U32(utf8CharBuf[0] & 0x1F);
            codePoint = (codePoint << 6) | (utf8CharBuf[1] & 0x3F);
            break;
        case 3:
            codePoint = INT_TO_U32(utf8CharBuf[0] & 0x0F);
            codePoint = (codePoint << 6) | (utf8CharBuf[1] & 0x3F);
            codePoint = (codePoint << 6) | (utf8CharBuf[2] & 0x3F);
            break;
        case 4:
            codePoint = INT_TO_U32(utf8CharBuf[0] & 0x07);
            codePoint = (codePoint << 6) | (utf8CharBuf[1] & 0x3F);
            codePoint = (codePoint << 6) | (utf8CharBuf[2] & 0x3F);
            codePoint = (codePoint << 6) | (utf8CharBuf[3] & 0x3F);
            break;
        default:
            return false;
    }

    // Convert to UTF-16
    if (codePoint > 0xD800 && codePoint <= 0xDFFF) {
        // This is not a valid code point.
        return false;
    } else if (codePoint <= 0xFFFF) {
        // 16-bit
        if (utf16BufLen_in < 1) {
            return false;
        }
        *utf16BufConsumed_in_out = 1;
        utf16Buf_out[0] = VPLConv_ntoh_u16(U32_TO_U16(codePoint));
    } else {
        // 32-bit
        if (utf16BufLen_in < 2) {
            return false;
        }
        *utf16BufConsumed_in_out = 2;
        utf16Buf_out[0] = VPLConv_ntoh_u16(U32_TO_U16(0xD800 | ((codePoint - 0x10000) >> 10)));
        utf16Buf_out[1] = VPLConv_ntoh_u16(U32_TO_U16(0xDC00 | (0x03FF & (codePoint - 0x10000))));
    }
    return true;
}

// Convert a UTF16BE string to UTF8.
bool vpl_UTF16_to_UTF8(u16 const* in, size_t inlen, utf8 *out, size_t *outlen)
{
    u16 const* src = in;
    utf8* dst = out;
    size_t origOutlen = *outlen;

    while ((size_t)(src - in) < inlen && *outlen > 0) {
        size_t inLeft = inlen - (src - in);
        size_t outLeft = origOutlen - (dst - out);
        if (vpl_utf16CharToUtf8Char(src, &inLeft, dst, &outLeft)) {
            if (*src == 0) {
                *outlen = *outlen + 1;
                return true;
            }
            src += inLeft;
            dst += outLeft;
            *outlen = (size_t)(dst - out);
        } else {
            return false;
        }
    }

    if ((size_t)(src - in) == inlen) {
        return true;
    }

    return false;
}

// Convert a UTF8 string to UTF16BE
bool vpl_UTF8_to_UTF16(utf8 const * in, size_t inlen, u16 *out, size_t *outlen_in_out)
{
    utf8 const* src = in;
    u16* dst = out;
    size_t origOutlen = *outlen_in_out;

    while ((size_t)(src - in) < inlen && *outlen_in_out > 0) {
        
        // Compute the remaining number of bytes in the buffers to specify
        // the maximum number of bytes to consume for the next character.
        size_t srcConsumed = inlen - (src - in);
        size_t dstConsumed = origOutlen - (dst - out);
        
        if (vpl_utf8CharToUtf16Char(src, &srcConsumed, dst, &dstConsumed)) {
            if (*src == 0) {
                *outlen_in_out = *outlen_in_out + 1;
                return true;
            }
            src += srcConsumed;
            dst += dstConsumed;
            *outlen_in_out = (size_t)(dst - out);
        } else {
            // Error occurred!
            return false;
        }
    }

    if ((size_t)(src - in) == inlen) {
        return true;
    }

    return false;
}

void vplUTF16Copy(u16* dest, size_t* destlen, u16 const* src)
{
    size_t i = 0;
    while (i < *destlen && *src != 0) {
        *dest = *src;
        dest++;
        src++;
        i++;
    }

    if (i < *destlen) {
        *dest = 0;
        *destlen = i;
    }
}
