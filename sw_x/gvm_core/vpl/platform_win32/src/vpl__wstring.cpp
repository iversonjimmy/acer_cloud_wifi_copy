#include "vpl_plat.h"
#include "vplu.h"

int _VPL__utf8_to_wstring(const char *utf8, wchar_t** wstring)
{
    int nwchars = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8, -1, NULL, 0);
    if (nwchars == 0) {
        return VPLError_XlatWinErrno(GetLastError());
    }

    wchar_t* buf = (wchar_t*) malloc(nwchars * sizeof(wchar_t));
    if (buf == NULL) {
        return VPL_ERR_NOMEM;
    }

    nwchars = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8, -1, buf, nwchars);
    if (nwchars == 0) {
        free(buf);
        return VPLError_GetLastWinError();
    }

    *wstring = buf;
    return VPL_OK;
}

// WC_ERR_INVALID_CHARS is missing in mingw, so define it here
#ifndef _MSC_VER
#  define WC_ERR_INVALID_CHARS 0x00000080
#endif

int _VPL__wstring_to_utf8(const wchar_t* wstring, int wstringLen, char *utf8, size_t utf8BufLen)
{
    if (utf8BufLen == 0) {
        return VPL_ERR_INVALID;
    }
    int nbytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wstring, wstringLen, utf8, (int)utf8BufLen, NULL, NULL);
    if (nbytes == 0) {
        return VPLError_GetLastWinError();
    }
    if (wstringLen == -1) {
        // utf8 will already be null-terminated.
        // I don't think this check is actually needed (it should return 0 and set ERROR_INSUFFICIENT_BUFFER),
        // but until we make sure, this check doesn't hurt.
        if (nbytes > (int)utf8BufLen) {
            return VPL_ERR_NAMETOOLONG;
        }
    } else {
        // utf8 may or may not be null-terminated at this point.
        if (utf8[nbytes - 1] != '\0') {
            // It's not null-terminated; we better have at least one more byte.
            if (nbytes >= (int)utf8BufLen) {
                return VPL_ERR_NAMETOOLONG;
            } else {
                utf8[nbytes] = '\0';
            }
        }
    }
    return VPL_OK;
}

int _VPL__wstring_to_utf8_alloc(const wchar_t* wstring, char **utf8)
{
    if (wstring == NULL || utf8 == NULL)
        return VPL_ERR_INVALID;

    // determine the size of the utf8 buffer needed
    int nBytesNeeded = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wstring, -1, NULL, 0, NULL, NULL);
    if (nBytesNeeded == 0) {
        return VPLError_GetLastWinError();
    }

    char *buf = (char*)malloc(nBytesNeeded);
    if (buf == NULL) {
        return VPL_ERR_NOMEM;
    }

    // do the actual code conversion
    int nBytesWritten = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wstring, -1, buf, nBytesNeeded, NULL, NULL);
    if (nBytesWritten == 0) {
        return VPLError_GetLastWinError();
    }
    else if (nBytesWritten != nBytesNeeded) {
        return VPL_ERR_FAIL;
    }
    // ELSE conversion succeeded and utf8 string is in buf[]

    *utf8 = buf;
    return VPL_OK;
}
