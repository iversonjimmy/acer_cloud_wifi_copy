/*
 *               Copyright (C) 2010, BroadOn Communications Corp.
 *
 *  These coded instructions, statements, and computer programs contain
 *  unpublished  proprietary information of BroadOn Communications Corp.,
 *  and  are protected by Federal copyright law. They may not be disclosed
 *  to  third  parties or copied or duplicated in any form, in whole or in
 *  part, without the prior written consent of BroadOn Communications Corp.
 *
 */

/* 
 * Crypto operations provided by platform-specific IOSCrypto layer.
 */

#include <esc_iosctypes.h>

#ifndef __CRYPTO_IMPL_H__
#define __CRYPTO_IMPL_H__

IOSCError IOSCryptoInitialize(void);

IOSCError IOSCryptoComputeSHA1(IOSCHashContext context, u8 *inputData, u32 inputSize, u32 chainingFlag, IOSCHash hashData);

IOSCError IOSCryptoComputeSHA256(IOSCHashContext context, u8 *inputData, u32 inputSize, u32 chainingFlag, IOSCHash256 hashData);

IOSCError IOSCryptoEncryptAESCBC(u8 *aesKey, u8 *ivData, u8 *inputData, u32 inputSize, u8 *outputData);

IOSCError IOSCryptoDecryptAESCBC(u8 *aesKey, u8 *ivData, u8 *inputData, u32 inputSize, u8 *outputData);

IOSCError IOSCryptoEncryptAESCBCHwKey(u32 hwKeyId, u8 *ivData, u8 *inputData, u32 inputSize, u8 *outputData);

IOSCError IOSCryptoDecryptAESCBCHwKey(u32 hwKeyId, u8 *ivData, u8 *inputData, u32 inputSize, u8 *outputData);

IOSCError IOSCryptoDecryptRSA(u8 *publicKey, u32 keySize, u8 *exponent, u32 expSize, u8 *inputData, u8 *outputData);

IOSCError IOSCryptoGenerateRand(u8 *randoms, u32 size);

void *IOSCryptoAllocAligned(u32 size, u32 alignment);
void  IOSCryptoFreeAligned(void *,    u32 alignment);

IOSCError IOSCryptoIncVersion(IOSCDataHandle handle);
s32  IOSCryptoGetVersion(IOSCDataHandle handle);

u8 *IOSCryptoGetRootKey(void);
u8 *IOSCryptoGetRootKeyExp(void);

IOSCError IOSCryptoGetDeviceCert(u8 *cert);

#endif  // __CRYPTO_IMPL_H__
