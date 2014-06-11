#if !defined(_WIN32) && !defined(ANDROID)

#include "vplex_wstring.h"
#include <wchar.h>
#include "vpl_conv.h"
#include "vplex_safe_conversion.h"

#include "vplexTest.h"


#define WIDE_TEST_STR L"test string"
#define UTF16_TEST_STR {VPLConv_hton_u16('t'),VPLConv_hton_u16('e'),VPLConv_hton_u16('s'),VPLConv_hton_u16('t'),VPLConv_hton_u16(' '),VPLConv_hton_u16('s'),VPLConv_hton_u16('t'),VPLConv_hton_u16('r'),VPLConv_hton_u16('i'),VPLConv_hton_u16('n'),VPLConv_hton_u16('g'), 0}
#define TEST_STR "test string"
#define MAX_BUFFER_LEN 50

void testVPLWstring(void)
{
    wchar_t A[] = WIDE_TEST_STR;
    u16 B[] = UTF16_TEST_STR;
    utf8 C[] = TEST_STR;
    const size_t maxBufferLen = MAX_BUFFER_LEN;
    wchar_t wcharBuffer[MAX_BUFFER_LEN];
    utf8 utf8Buffer[MAX_BUFFER_LEN];
    u16 utf16Buffer[MAX_BUFFER_LEN];
    size_t outlen = MAX_BUFFER_LEN;
    unsigned int i = 0;

    APP_LOG("Convert wchar_t string of all ascii chars to UTF16\n");
    outlen = maxBufferLen;
    if (vpl_wstring_to_UTF16(A, sizeof(A), utf16Buffer, &outlen)) {
        // since A has ascii chars only, every other byte with UTF16 should be 0.
        for (i = 0; i < outlen; ++i) {
            if ((VPLConv_hton_u16(utf16Buffer[i]) & 0xFF00) != 0) {
                APP_LOG("FAIL: converting wchar_t string of ascii chars to utf16 is incorrect.\n");
                vplTest_incrErrCount();
                break;
            }
        }
    } else {
        APP_LOG("FAIL: vpl_wstring_to_UTF16 returned false.\n");
        vplTest_incrErrCount();
    }

    APP_LOG("Convert UTF16 to wchar_t\n");
    outlen = maxBufferLen;
    if (vpl_UTF16_to_wstring(B, sizeof(B), wcharBuffer, &outlen)) {
        // this should be the same as the original test string
        if (wcscmp(wcharBuffer, WIDE_TEST_STR) != 0) {
            APP_LOG("FAIL: converting UTF16 to wide string result is not equal to original test string.\n");
            vplTest_incrErrCount();
        }
    } else {
        APP_LOG("FAIL: vpl_UTF16to_wstring returned false.\n");
        vplTest_incrErrCount();
    }

    APP_LOG("Convert UTF16 to UTF8\n");
    outlen = maxBufferLen;
    if (vpl_UTF16_to_UTF8(B, sizeof(B) / sizeof(u16), utf8Buffer, &outlen)) {
        // since this should be all ascii characters, the lengths should be the same
        if (sizeof(B) / sizeof(u16) != outlen) {
            APP_LOG("FAIL: convert utf16 string of ascii chars to utf8, but lengths are different.\n");
            APP_LOG("in length: " FMTuSizeT " out length: " FMTuSizeT "\n", sizeof(B) / sizeof(u16), outlen);
            vplTest_incrErrCount();
        } else {
            // and the lower 8 bits of each utf16 character should be equal to the utf8 chars
            for (i = 0; i < outlen; ++i) {
                if (U16_TO_U8(VPLConv_ntoh_u16(utf16Buffer[i])) != utf8Buffer[i]) {
                    APP_LOG("FAIL: converting utf16 string of ascii chars to utf16 is incorrect.\n");
                    vplTest_incrErrCount();
                    break;
                }
            }
        }
    } else {
        APP_LOG("FAIL: vpl_UTF16_to_UTF8 returned false.\n");
        vplTest_incrErrCount();
    }

    APP_LOG("Convert UTF8 to UTF16\n");
    outlen = maxBufferLen;
    if (vpl_UTF8_to_UTF16(C, sizeof(C), utf16Buffer, &outlen)) {
        // since A has ascii chars only, every other byte with UTF16 should be 0.
        for (i = 0; i < outlen; ++i) {
            if ((VPLConv_hton_u16(utf16Buffer[i]) & 0xFF00) != 0) {
                APP_LOG("FAIL: converting wchar_t string of ascii chars to utf16 is incorrect.\n");
                vplTest_incrErrCount();
                break;
            }
        }
    } else {
        APP_LOG("FAIL: vpl_UTF8_to_UTF16 returned false.\n");
        vplTest_incrErrCount();
    }
}

#endif
