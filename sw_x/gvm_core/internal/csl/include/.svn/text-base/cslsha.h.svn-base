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

#ifndef _CSLSHA_H_
#define _CSLSHA_H_

#include <csltypes.h>
#include "aes_api.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cslShaContext
{
    u32 intermediateHash[CSL_SHA1_DIGESTSIZE/4];
    u32 lengthLo;
    u32 lengthHi;
    u32 messageBlockIndex;
    u8 messageBlock[CSL_SHA1_BLOCKSIZE];
} CSL_ShaContext;

typedef struct cslHmacContext
{
    u8 keyExt[CSL_SHA1_BLOCKSIZE];
    CSL_ShaContext context;
} CSL_HmacContext;


typedef struct cslAesContext
{
    unsigned char iv[CSL_AES_BLOCKSIZE_BYTES];
    unsigned char key[CSL_AES_BLOCKSIZE_BYTES];
    AesKeyInstance keyInst;
    AesCipherInstance cipherInst;
} CSL_AesContext;

int CSL_ResetSha(CSL_ShaContext *context);
int CSL_InputSha(CSL_ShaContext *context, const void *inputData, u32 inputSize);
int CSL_ResultSha(CSL_ShaContext *context, u8 cslShaHash[CSL_SHA1_DIGESTSIZE]);

int CSL_ResetHmac(CSL_HmacContext *context, const CSLOSHMACKey key);
int CSL_ResetHmacEx(CSL_HmacContext *context, const u8 *key, u32 keySize);
int CSL_InputHmac(CSL_HmacContext *context, const void *inputData, u32 inputSize);
int CSL_ResultHmac(CSL_HmacContext *context, u8 cslHmacHash[CSL_SHA1_DIGESTSIZE]);

int CSL_ResetEncryptAes(CSL_AesContext *context, CSLOSAesKey key, 
                 CSLOSAesIv iv);
int CSL_ResetDecryptAes(CSL_AesContext *context, CSLOSAesKey key, 
                 CSLOSAesIv iv);
int CSL_EncryptAes(CSL_AesContext *context, u8 *inputData, u32 size, 
                   u8 *outputData);
int CSL_DecryptAes(CSL_AesContext *context, u8 *inputData, u32 size, 
                   u8 *outputData);

#ifdef COMPILE_HMAC_SHA1_TEST_CODE
void CSL_test_hmac_sha1();
#endif // COMPILE_HMAC_SHA1_TEST_CODE

#ifdef __cplusplus
}
#endif

#endif // include guard
