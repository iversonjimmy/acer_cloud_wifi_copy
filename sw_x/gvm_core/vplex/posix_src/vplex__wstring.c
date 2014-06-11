#include <vplex_wstring.h>
#include <vplex_safe_conversion.h>

#include <errno.h>
#include <arpa/inet.h>
#if defined __GLIBC__ && !defined __UCLIBC__
#include <iconv.h>
#endif
/// This file is the platform implementation of the VPL 
/// platform-independent API for wide strings, for Unix/Linux
/// systems.  All sane Unix/Linux systems define the host wchar_t
/// to be a uint32_t, encoded in UTF-32 (UCS-4).
/// This implementation of the API uses the GNU iconv
/// library.  That library is distributed under the LGPL, so it is
/// safe to use in BroadOn-internal code. We need to review use of
/// iconv(3) if we ever release the server code outside of BroadOn.

static bool utf16CharToUtf32Char(u16 const* utf16Char, size_t* utf16len, wchar_t* utf32Char)
{
    // determine if the utf16 char is 16bits or 32bits
    if (ntohs(*utf16Char) > 0xD800 && ntohs(*utf16Char) <= 0xDFFF) {
        if ((*utf16len) < 2) {
            return false;
        }
        *utf16len = 2;
        *utf32Char = INT_TO_U32(ntohs(utf16Char[0]) & 0x27FF);
        *utf32Char = (*utf32Char << 10) | (utf16Char[1] & 0x23FF);
    } else {
        if ((*utf16len) < 1) {
            return false;
        }
        *utf16len = 1;
        *utf32Char = INT_TO_U32(ntohs(utf16Char[0]));
    }
    
    return true;
}

static bool utf32CharToUtf16Char(wchar_t utf32Char, u16* utf16Char, size_t* utf16len)
{
    // is our UTF16 encoding 16bits or 32bits?
    if (utf32Char > 0xD800 && utf32Char <= 0xDFFF) {
        // this is not a valid code point
        return false;
    } else if (utf32Char <= 0xFFFF) {
        if (*utf16len < 1) {
            return false;
        }
        *utf16len = 1;
        *utf16Char = ntohs((u16)utf32Char);
    } else {
        if (*utf16len < 2) {
            return false;
        }
        *utf16len = 2;
        utf16Char[0] = ntohs((u16)(0xD800 | ((utf32Char - 0x10000) >> 10)));
        utf16Char[1] = ntohs((u16)(0xDC00 | (0x03FF & (utf32Char - 0x10000))));
    }

    return true;
}

// On Linux, transcode from UTF16(BE) to UTF-32/UCS-4.
 bool vpl_UTF16_to_wstring(u16 const * in, size_t inlen, wchar_t *out, size_t *outlen)
{
    u16 const* src = in;
    wchar_t* dst = out;
    size_t origOutlen = *outlen;
    *outlen = 0;
    inlen = inlen / sizeof(u16);

    while ((size_t)(src - in) < inlen && *outlen < origOutlen) {
        size_t inLeft = inlen - (src - in);
        if (utf16CharToUtf32Char(src, &inLeft, dst)) {
            src += inLeft;
            dst ++;
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

bool vpl_wstring_to_UTF16(const wchar_t * in, size_t inlen, u16 *out, size_t *outlen)
{
    wchar_t const* src = in;
    u16* dst = out;
    size_t origOutlen = *outlen;
    *outlen = 0;
    inlen = inlen / sizeof(wchar_t);

    while ((size_t)(src - in) < inlen && *outlen < origOutlen) {
        size_t outLeft = origOutlen - (dst - out);
        if (utf32CharToUtf16Char(*src, dst, &outLeft)) {
            src ++;
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
