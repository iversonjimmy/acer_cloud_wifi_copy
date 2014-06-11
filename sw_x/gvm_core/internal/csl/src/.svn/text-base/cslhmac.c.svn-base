/*
 *               Copyright (C) 2005, BroadOn Communications Corp.
 *
 *  These coded instructions, statements, and computer programs contain
 *  unpublished  proprietary information of BroadOn Communications Corp.,
 *  and  are protected by Federal copyright law. They may not be disclosed
 *  to  third  parties or copied or duplicated in any form, in whole or in
 *  part, without the prior written consent of BroadOn Communications Corp.
 *
 */
#include "cslsha.h"

#include "csl_impl.h"

#include <csl.h>

#define IPAD 0x36
#define OPAD 0x5c
    

/* H(K xor opad, H(K xor ipad, text)) */      
int
CSL_ResetHmac(CSL_HmacContext *context, const CSLOSHMACKey key)
{
    return CSL_ResetHmacEx(context, key, sizeof(CSLOSHMACKey));
}

int
CSL_ResetHmacEx(CSL_HmacContext *context, const u8 *key, u32 keySize)
{
    u8 kxorpad[CSL_SHA1_BLOCKSIZE];
    u8 kext[CSL_SHA1_BLOCKSIZE];
    int i;
    u8 keybuf[CSL_SHA1_DIGESTSIZE];

    if (keySize > CSL_SHA1_BLOCKSIZE) {
        CSL_ShaContext c;
        CSL_ResetSha(&c);
        CSL_InputSha(&c, key, keySize);
        CSL_ResultSha(&c, keybuf);
        key = keybuf;
        keySize = CSL_SHA1_DIGESTSIZE;
    }

    memcpy(kext, key, keySize);
    memset(kext + keySize, 0x0, 
           sizeof(kext) - keySize);
    memcpy(context->keyExt, kext, CSL_SHA1_BLOCKSIZE);
    for (i = 0; i < CSL_SHA1_BLOCKSIZE; i++) {
        kxorpad[i] = kext[i] ^ IPAD;
    }
    CSL_ResetSha(&(context->context));      
    CSL_InputSha(&(context->context), (SHA1_BYTE *) kxorpad, CSL_SHA1_BLOCKSIZE);
    return CSL_OK;
}

int
CSL_InputHmac(CSL_HmacContext *context, const void *inputData, u32 inputSize)
{
    CSL_InputSha(&(context->context), inputData, inputSize);
    return CSL_OK;
}


int
CSL_ResultHmac(CSL_HmacContext *context, CSLOSSha1Hash hmac)
{
    u8 kxorpad[CSL_SHA1_BLOCKSIZE];
    u8 hashData[CSL_SHA1_DIGESTSIZE];
    int i;
    CSL_ResultSha(&(context->context), hashData);
    /* second hash */
    for (i = 0; i < CSL_SHA1_BLOCKSIZE; i++) {
        kxorpad[i] = context->keyExt[i] ^ OPAD;
    }
    CSL_ResetSha(&(context->context));
    CSL_InputSha(&(context->context), kxorpad, CSL_SHA1_BLOCKSIZE);
    CSL_InputSha(&(context->context), hashData, CSL_SHA1_DIGESTSIZE);
    CSL_ResultSha(&(context->context), hmac);
    return CSL_OK;
}


#ifdef COMPILE_HMAC_SHA1_TEST_CODE

static void test_case(const char *text,
                      const char *key, u32 keySize,
                      const char *expectedResult, u32 resultSize)
{
    CSL_HmacContext c;
    CSLOSSha1Hash hmac;

    CSL_ResetHmacEx(&c, (const u8*)key, keySize);
    CSL_InputHmac(&c, text, strlen(text));
    CSL_ResultHmac(&c, hmac);
    printf("%s\n", memcmp(hmac, expectedResult, resultSize) == 0 ? "OK" : "FAIL");

    if (keySize == CSL_SHA1_DIGESTSIZE) {
        CSL_ResetHmac(&c, (const u8*)key);
        CSL_InputHmac(&c, text, strlen(text));
        CSL_ResultHmac(&c, hmac);
        printf("%s\n", memcmp(hmac, expectedResult, resultSize) == 0 ? "OK" : "FAIL");
    }
}

void CSL_test_hmac_sha1()
{
    /* test vectors from http://csrc.nist.gov/groups/ST/toolkit/documents/Examples/HMAC_SHA1.pdf */

    test_case("Sample message for keylen=blocklen",
              "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2A\x2B\x2C\x2D\x2E\x2F\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3A\x3B\x3C\x3D\x3E\x3F", 64,
              "\x5F\xD5\x96\xEE\x78\xD5\x55\x3C\x8F\xF4\xE7\x2D\x26\x6D\xFD\x19\x23\x66\xDA\x29", 20);
    test_case("Sample message for keylen<blocklen",
              "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13", 20,
              "\x4C\x99\xFF\x0C\xB1\xB3\x1B\xD3\x3F\x84\x31\xDB\xAF\x4D\x17\xFC\xD3\x56\xA8\x07", 20);
    test_case("Sample message for keylen=blocklen",
              "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2A\x2B\x2C\x2D\x2E\x2F\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3A\x3B\x3C\x3D\x3E\x3F\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4A\x4B\x4C\x4D\x4E\x4F\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5A\x5B\x5C\x5D\x5E\x5F\x60\x61\x62\x63", 100,
              "\x2D\x51\xB2\xF7\x75\x0E\x41\x05\x84\x66\x2E\x38\xF1\x33\x43\x5F\x4C\x4F\xD4\x2A", 20);
    test_case("Sample message for keylen<blocklen, with truncated tag",
              "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2A\x2B\x2C\x2D\x2E\x2F\x30", 49,
              "\xFE\x35\x29\x56\x5C\xD8\xE2\x8C\x5F\xA7\x9E\xAC", 12);
}

#endif // COMPILE_HMAC_SHA1_TEST_CODE
