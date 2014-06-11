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
 * Highest Level Crypto Services Library API
 */

#include "csl.h"

#include "csl_impl.h"

#include "algorithms.h"
#include "elliptic_math.h"
#include "algorithms.h"
#include "integer_math.h"
#include "conversions.h"

/* Platform specific implementations */
#include <crypto_impl.h>


/*
 * RSA public key decrypt
 *
 * Called by Verify primitives
 */
typedef struct {
    bigint_digit bign[MAX_BIGINT_DIGITS];
    bigint_digit bigm[MAX_BIGINT_DIGITS];
    bigint_digit bige[MAX_BIGINT_DIGITS];
    bigint_digit bigc[MAX_BIGINT_DIGITS];
} CSL_DecryptRsa_big_locals;

CSL_error 
CSL_DecryptRsa(u8 *publicKey, 
               u32 keySize, 
               u8 *exponent, 
               u32 expSize, 
               u8 *inputData, 
               u8 *outputData)
{
    u32 eDigits, nDigits;
    int outlen;
    CSL_DecryptRsa_big_locals *big_locals;
#ifdef __KERNEL__
    big_locals = kmalloc(sizeof(*big_locals), GFP_KERNEL);
    if (!big_locals)
        return CSL_NO_MEMORY;
#else
#ifdef LINUX
    // For linux user-space versions, put locals on the stack for thread safety
    CSL_DecryptRsa_big_locals _big_locals;
#elif defined(_MSC_VER)
    CSL_DecryptRsa_big_locals _big_locals;
#else
    // For embedded platforms, declare large arrays static to avoid 
    // stack overflow
    static CSL_DecryptRsa_big_locals _big_locals;
#endif
    big_locals = &_big_locals;
#endif

    nDigits = keySize/BIGINT_DIGIT_BYTES;
    eDigits = 1;

    OS2IP(big_locals->bign, nDigits, publicKey, keySize);
    OS2IP(big_locals->bigm, nDigits, inputData, keySize);
    OS2IP(big_locals->bige, eDigits, exponent, expSize);
    bigint_mod_exp(big_locals->bigc, big_locals->bigm, big_locals->bige, eDigits, big_locals->bign, nDigits);
    
    // XXX? outlen = ((keySize * 8) + 7)/8;
    outlen = keySize;
    I2OSP(outputData, outlen, big_locals->bigc, nDigits); 

#ifdef __KERNEL__
    kfree(big_locals);
#endif
    return CSL_OK;
}


/* 
 * Call to verify RSA signature, 
 * returns CSL_OK or CSL_VERIFY_ERROR : only supports exponents 3, 17, 65537.
 */
CSL_error
CSL_VerifyRsaSig(CSLOSDigest digest, u32 digestSize,
    CSLOSRsaPublicKey certpublickey, CSLOSRsaSig certsign,
    CSLOSRsaExponent certexponent, u32 expsize, u32 keysize)
{
    u32 n;
    u8 *p, match;
    int padlen;
    u8 *digSig;
    int digSigLen;
    /*
     * Note that the leading 0 byte on each of these digest signatures is
     * actually a 0 pad byte specified by EMSA-PKCS1-v1_5
     */
    static u8 sha1_sig[16] = { 0x00,
        0x30, 0x21, 0x30, 0x09, 0x06, 0x05, 0x2b, 0x0e,
        0x03, 0x02, 0x1a, 0x05, 0x00, 0x04, 0x14
    };
    static u8 sha256_sig[20] = { 0x00,
        0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86,
        0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01, 0x05,
        0x00, 0x04, 0x20
    };
#if defined(__KERNEL__)
    // For linux kernel, put the large array in the heap
    u8 *result = kmalloc(MAX_BIGINT_DIGITS * BIGINT_DIGIT_BYTES, GFP_KERNEL);
    if (!result)
        return CSL_NO_MEMORY;
#elif defined(LINUX)
    // For linux user space, put the large array on the stack for thread safety
    u8 result[MAX_BIGINT_DIGITS * BIGINT_DIGIT_BYTES];
#elif defined(_MSC_VER)
    // For VS compiler, put the large array on the stack for thread safety
    u8 result[MAX_BIGINT_DIGITS * BIGINT_DIGIT_BYTES];
#else
    // For embedded platforms, declare large arrays static to avoid 
    // stack overflow
    static u8 result[SIZE_RSA_ALIGN(MAX_BIGINT_DIGITS * BIGINT_DIGIT_BYTES)]
                     ATTR_RSA_ALIGN;
#endif

    IOSCryptoDecryptRSA(certpublickey, keysize, certexponent, expsize, certsign, result);

    /*
     * The padding is different depending on the digest algorithm.
     * The code currently supports only SHA1 and SHA-256.
     */
    if (digestSize == sizeof(CSLOSSha1Hash)) {
        digSigLen = sizeof sha1_sig;
        digSig = sha1_sig;
    } else if (digestSize == sizeof(CSLOSSha256Hash)) {
        digSigLen = sizeof sha256_sig;
        digSig = sha256_sig;
    } else {
        match = 0;
        goto out;
    }
    /* Number of 0xff pad bytes */
    padlen = keysize - digestSize - digSigLen - 2;

    p = result;
    match = (p[0] == 0x00) & (p[1] == 0x01);
    p += 2;
    for (n = 0; n < padlen; n++) {
        match &= (*p++ == 0xff);
    }

    /* followed by ASN1 description of digest */
    for (n = 0; n < digSigLen; n++) {
        match &= (*p++ == *digSig++);
    }
#if defined(RVL_DEVEL)
    /* 
     * This ifdef is to support existing tools for developers that incorrectly
     * generate the padding for the RSA signature. Therefore, to remain 
     * backward compatible, we can only check the 20-byte hash and not the 
     * entire signature which should be padded using PKCS#1 v1.5 type 2 
     * padding (officially EMSA-PKCS1-v1_5).
     * For production images, we are using the correct padding (via HSM).
     * See bug #2855, #2876.
     */
    match = 1;
#endif
    /* compare message digest */
    for (n = 0; n < digestSize; n++) {
        match &= (*p++ == *digest++);
    }
out:
#if defined(__KERNEL__)
    kfree(result);
#endif
    return (match ? CSL_OK : CSL_VERIFY_ERROR);
}

CSL_error
CSL_VerifyRsaSig2048(CSLOSDigest digest, 
                     u32 digestSize,
                     CSLOSRsaPublicKey2048 certpublickey, 
                     CSLOSRsaSig2048 certsign, 
                     CSLOSRsaExponent certexponent, 
                     u32 expsize)
{
    return CSL_VerifyRsaSig(digest, digestSize, certpublickey, certsign, 
                            certexponent, expsize, 256);
}

CSL_error
CSL_VerifyRsaSig4096(CSLOSDigest digest, 
                     u32 digestSize,
                     CSLOSRsaPublicKey4096 certpublickey, 
                     CSLOSRsaSig4096 certsign, 
                     CSLOSRsaExponent certexponent, 
                     u32 expsize)
{
    return CSL_VerifyRsaSig(digest, digestSize, certpublickey, certsign, 
                            certexponent, expsize, 512);
}

#if 0
#include "sha1.h"

/*
 * HMAC needs key material of length 20 bytes to support HMAC-SHA1-80
 * and the result is truncated outside
 */

void
CSL_ComputeHmac(u8 *text, u32 text_length, 
		u8 *key, u32 key_length, u8 *hmac)
{
	SHA1Context sha;
	u8 k_ipad[65]; /* inner padding */
	u8 k_opad[65]; /* outer padding */

	u8 tk[16];
	
	int i;
	
	/* if key is longer than 64 bytes set it to key = SHA1(key)
	*/
	if(key_length > 64){
		SHA1Reset(&sha);
    		SHA1Input(&sha, key, key_length);
    		SHA1Result(&sha, tk);
		key = tk;
		key_length = 20;
	}
	/* the HMAC = SHA1(K xor opad, SHA1(K xor ipad, text))
	*/
	memset(k_ipad, 0, sizeof(k_ipad));
	memset(k_opad, 0, sizeof(k_opad));
	memcpy(k_ipad, key, key_length);
	memcpy(k_opad, key, key_length);

	/* xor key with ipad and opad values */
	for(i=0; i < 64; i++){
		k_ipad[i] ^= 0x36;
		k_opad[i] ^= 0x5c;
	}
	
	/* perform inner SHA1 */
	SHA1Reset(&sha);
	SHA1Input(&sha, k_ipad, 64);
	SHA1Input(&sha, text, text_length);
	SHA1Result(&sha, hmac);
	
	/* perform outer SHA1 */
	SHA1Reset(&sha);
	SHA1Input(&sha, k_opad, 64);
	SHA1Input(&sha, hmac, 16);
	SHA1Result(&sha, hmac);

	/* truncate outside */

}
#endif
