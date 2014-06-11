/*
 *               Copyright (C) 2005, BroadOn Communications Corp.
 *
 *  These coded instructions, statements, and computer programs contain
 *  unpublished  proprietary information of BroadOn Communications Corp.,
 *  and  are protected by Federal copyright law. They may not be disclosed
 *  to  third  parties or copied or duplicated in any form, in whole or in
 *  part, without the prior written consent of BroadOn Communications Corp.
 */

/* 
 * Crypto Services Library Types
 */
#ifndef __CSLTYPES_H__
#define __CSLTYPES_H__

#include <vplu_types.h>

#define CSL_SHA1_BLOCKSIZE		64
#define CSL_SHA1_DIGESTSIZE		20

#define CSL_SHA256_BLOCKSIZE            64
#define CSL_SHA256_DIGESTSIZE           32

#define CSL_AES_BLOCKSIZE_BYTES         16
#define CSL_AES_KEYSIZE_BYTES		16
#define CSL_AES_IVSIZE_BYTES 		16
#define CSL_KEY_STRING_LEN              30

/* 
 * Data types for arguments (Octet Stream or byte stream format)
 */
typedef u8 CSLOSEccPrivateRand[CSL_KEY_STRING_LEN];
typedef u8 CSLOSEccPrivateKey[CSL_KEY_STRING_LEN];
typedef u8 CSLOSEccSharedKey[CSL_KEY_STRING_LEN];
typedef u8 CSLOSEccPublicKey[CSL_KEY_STRING_LEN*2];
typedef u8 CSLOSEccSigRand[CSL_KEY_STRING_LEN];
typedef u8 CSLOSEccSig[CSL_KEY_STRING_LEN*2];
typedef u8 CSLOSEccExpPublicKey[CSL_KEY_STRING_LEN*2*16];

typedef u8 CSLOSRsaPublicKey2048[256];
typedef u8 CSLOSRsaPublicKey4096[512];
typedef u8 CSLOSRsaExponent[4];
typedef u8 CSLOSRsaSig2048[256];
typedef u8 CSLOSRsaSig4096[512];

typedef u8 CSLOSRsaPublicKey[256];
typedef u8 CSLOSRsaSig[256];
typedef u8 CSLOSRsaMsg[256];
typedef u8 CSLOSRsaSecretExp[256];

typedef u8 *CSLOSDigest;
typedef u8 CSLOSSha1Hash[CSL_SHA1_DIGESTSIZE];
typedef u8 CSLOSSha256Hash[CSL_SHA256_DIGESTSIZE];
typedef u8 CSLOSAesKey[CSL_AES_KEYSIZE_BYTES];
typedef u8 CSLOSAesIv[CSL_AES_IVSIZE_BYTES];
typedef u8 CSLOSHMACKey[CSL_SHA1_DIGESTSIZE];

#endif /* __CSLTYPES_H__ */
