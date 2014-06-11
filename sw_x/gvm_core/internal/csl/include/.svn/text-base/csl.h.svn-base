/* "$Id: csl.h,v 1.3 2011-05-14 22:52:09 steveo Exp $" */
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
/* 
 *
 * csl.h
 * Crypto Services Library Header
 *
 */

#ifndef __CSL_H__
#define __CSL_H__

#include <km_types.h>
#include <csltypes.h>
#include "sha1.h"
#include "csloaep.h"

/* Errors */

typedef enum { CSL_OK = 0, 
               CSL_OVERFLOW, 
               CSL_DIVIDE_BY_ZERO, 
               CSL_SHA_ERROR,
               CSL_AES_ERROR,
               CSL_BAD_KEY,
               CSL_NULL_POINTER,
               CSL_VERIFY_ERROR,
               CSL_NO_MEMORY
} CSL_error;

#if defined(__cplusplus)
extern "C" {
#endif

/* 
 * Highest Level calls for Elliptic Curve Operations
 */

CSL_error CSL_GenerateEccKeyPair(CSLOSEccPrivateRand rand, 
				 CSLOSEccPrivateKey privateKey,
				 CSLOSEccPublicKey publicKey);
CSL_error CSL_GenerateEccPublicKey(CSLOSEccPrivateKey privateKey,
                                   CSLOSEccPublicKey publicKey);
CSL_error CSL_GenerateEccSharedKey(CSLOSEccPrivateKey privateKey, 
                                   CSLOSEccPublicKey publicKey, 
                                   CSLOSEccSharedKey sharedKey, u32 numBytes);
CSL_error CSL_PrecomputeFour(CSLOSEccPublicKey  publicKey, 
                             CSLOSEccExpPublicKey prePublicKey);
CSL_error CSL_GenerateEccSharedKeyPre(CSLOSEccPrivateKey privateKey, 
                                      CSLOSEccExpPublicKey publicKey, 
                                      CSLOSEccSharedKey sharedKey, 
                                      u32 numBytes);
CSL_error CSL_ComputeEccSig(CSLOSDigest digest, 
                            u32 digestSize,
                            CSLOSEccPrivateKey private_key, 
                            CSLOSEccSig sign,
                            CSLOSEccSigRand random_data);
CSL_error CSL_VerifyEccSig(CSLOSDigest digest, 
                           u32 digestSize,
                           CSLOSEccPublicKey public_key, 
                           CSLOSEccSig sign);

/*
 * highest level calls for RSA functions
 */


CSL_error CSL_VerifyRsaSig2048(CSLOSDigest digest, 
                               u32 digestSize,
                               CSLOSRsaPublicKey2048 certpublickey, 
                               CSLOSRsaSig2048 certsign, 
                               CSLOSRsaExponent certexponent, 
                               u32 expsize);


CSL_error CSL_VerifyRsaSig4096(CSLOSDigest digest, 
                               u32 digestSize,
                               CSLOSRsaPublicKey4096 certpublickey, 
                               CSLOSRsaSig4096 certsign, 
                               CSLOSRsaExponent certexponent, 
                               u32 expsize);

/* this function can be used for any number of bits by allocating 
 * correct number of bits for public key, sign . Use the above two
 * for 2048, 4096
 * etc.
 */

CSL_error CSL_VerifyRsaSig(CSLOSDigest digest, 
                           u32 digestSize,
                           CSLOSRsaPublicKey certpublickey, 
                           CSLOSRsaSig certsign, 
                           CSLOSRsaExponent certexponent, 
                           u32 expsize, 
                           u32 keysize);

/*
 * RSA public key decrypt
 *
 * The input and output data must be keySize
 */
CSL_error CSL_DecryptRsa(u8 *publicKey, 
                         u32 keySize, 
                         u8 *exponent, 
                         u32 expSize, 
                         u8 *inputData, 
                         u8 *outputData);

#ifdef USE_BSAFE
/*
 * For TWL, use RSA BSAFE implementation
 */
extern s32 CSL_VerifyRsaSigData2048(void *data, u32 datalen, 
                     void *certsign,
                     u32 signerHandle);

extern s32 CSL_VerifyRsaSigData4096(void *data, u32 datalen, 
                     void *certsign,
                     u32 signerHandle);
#endif


/*
 * These functions are used by the publishing code in the
 * developer SDK, but not on the device side
 */
void CSL_ComputeRsaSig(CSLOSRsaSig result, 
                       CSLOSRsaMsg paddedmessage, 
                       CSLOSRsaPublicKey certpublickey, 
                       CSLOSRsaSecretExp secretexponent,  
                       u32 keysize);

void CSL_RsaSignData(u8 *hashVal,
                     u32 hashSize,
                     u8 *rsaPubMod, 
                     u8 *rsaPrivExp, // raw data, no DER encoding
                     u32 rsaKeySize, // in bytes
                     u8 *signData);

#if 0

/* the functions below are for testing only */

void CSL_ComputeRsaSigFast(u8 *result, u32 *message, 
                           u32 *certpublickey, u32 *certp, 
                           u32 *certq, u32 *dmp, u32 *dmq, 
                           u32 *qinv,  int num_bits);
void CSL_ComputeHmac(u8 *text, u32 text_length, 
                     u8 *key, u32 key_length, 
                     u8 *hmac);
#endif

#if defined(__cplusplus)
}
#endif

#endif /* __CSL_H__ */
