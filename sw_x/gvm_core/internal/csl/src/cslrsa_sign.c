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
 * RSA Sign Routine
 *
 */
#include "csl.h"

#include "csl_impl.h"

#include "algorithms.h"
#include "integer_math.h"
#include "conversions.h"

/* Platform specific implementations */
#include <crypto_impl.h>

/* 
 * RSA private key encrypt using the straightforward but less
 * efficient algorithm:
 *
 *   c = (m**d) mod n
 *
 * where
 *   c = ciphertext
 *   m = cleartext message
 *   d = private exponent
 *   n = public modulus
 *
 * In the common usage, the cleartext message is a digest (SHA1 or SHA256)
 * padded to keysize as specified by EMSA-PKCS1-v1_5.
 */
void 
CSL_ComputeRsaSig(CSLOSRsaSig result, CSLOSRsaMsg paddedmessage, CSLOSRsaPublicKey certpublickey, CSLOSRsaSecretExp secretexponent, u32 keysize)
{
    u32 dDigits, nDigits;
#ifdef __KERNEL__
    bigint_digit *_big = kmalloc(4 * sizeof(bigint_digit) * MAX_BIGINT_DIGITS, GFP_KERNEL);
    bigint_digit *bign = _big;
    bigint_digit *bigm = bign + MAX_BIGINT_DIGITS;
    bigint_digit *bigd = bigm + MAX_BIGINT_DIGITS; /* secret exp */
    bigint_digit *bigc = bigd + MAX_BIGINT_DIGITS;
#else
    bigint_digit bign[MAX_BIGINT_DIGITS];
    bigint_digit bigm[MAX_BIGINT_DIGITS];
    bigint_digit bigd[MAX_BIGINT_DIGITS]; /* secret exp */
    bigint_digit bigc[MAX_BIGINT_DIGITS];
#endif
    int outlen;
    
    nDigits = keysize/BIGINT_DIGIT_BYTES;
    dDigits = keysize/BIGINT_DIGIT_BYTES;
            
    OS2IP (bign, nDigits, certpublickey, keysize);
    OS2IP (bigm, nDigits, paddedmessage, keysize);
    OS2IP (bigd, dDigits, secretexponent, keysize);
                
    bigint_mod_exp(bigc, bigm, bigd, dDigits, bign, nDigits);
    
    outlen = ((keysize * 8) + 7)/8;
        
    I2OSP(result, outlen, bigc, nDigits); 

#ifdef __KERNEL__
    kfree(_big);
#endif
}


/*
 * Generate RSA signature
 *
 * Input is unpadded message digest.  Add padding and call RSA Encrypt to
 * generate the signature.
 */
void
CSL_RsaSignData(u8 *hashVal, u32 hashSize, u8 *rsaPubMod, u8 *rsaPrivExp,
    u32 rsaKeySize, u8 *signData)
{
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
    int digSigLen;
    u8 *digSig;
#ifdef __KERNEL__
    u8 *msgBuf = kmalloc(sizeof(CSLOSRsaPublicKey4096), GFP_KERNEL); // Worst case size
#else
    u8 msgBuf[sizeof(CSLOSRsaPublicKey4096)];   // Worst case size
#endif

    if (rsaKeySize > sizeof(msgBuf)) {
        // What now?
        goto end;
    }

    /*
     * Construct padded message to sign
     */
    memset(msgBuf, 0xff, rsaKeySize);
    msgBuf[0] = 0;
    msgBuf[1] = 1;
    memcpy(&msgBuf[rsaKeySize - hashSize], hashVal, hashSize);

    if (hashSize == 20) {
        digSig = sha1_sig;
        digSigLen = sizeof(sha1_sig);
    } if (hashSize == 32) {
        digSig = sha256_sig;
        digSigLen = sizeof(sha256_sig);
    } else {
        // What now? printf("Bogus digest length %d\n", hashSize);
        goto end;
    }
    memcpy(&msgBuf[rsaKeySize - hashSize - digSigLen], digSig, digSigLen);

    /*
     * Now call RSA encrypt function
     */
    CSL_ComputeRsaSig(signData, msgBuf, rsaPubMod, rsaPrivExp, rsaKeySize);

 end:
#ifdef __KERNEL__
    kfree(msgBuf);
#else
    ;  // dummy statement needed for the label
#endif
}


#if 0
/*
 * RSA Sign using Garners algorithm and Chinese Remainder theorem. 
 * See the derivation on page 613 in handbook of applied cryptography
 * use factors dp and dq of secret exponent d and p and q of public 
 * exponent n, and qinv
 */
void
CSL_ComputeRsaSigFast(u8 *result, u32 *message, u32 *certpublickey, u32 *certp, u32 *certq, u32 *dmp, u32 *dmq, u32 *qinv, int num_bits)
{
    u32 pDigits, qDigits, cDigits, nDigits;
    bigint_digit bigp[MAX_BIGINT_DIGITS];
    bigint_digit bigq[MAX_BIGINT_DIGITS];
    bigint_digit bigc[MAX_BIGINT_DIGITS];
    bigint_digit bigdmp[MAX_BIGINT_DIGITS];
    bigint_digit bigdmq[MAX_BIGINT_DIGITS]; 
    bigint_digit bigqinv[MAX_BIGINT_DIGITS];
    bigint_digit bign[MAX_BIGINT_DIGITS];
    bigint_digit cP[MAX_BIGINT_DIGITS];
    bigint_digit cQ[MAX_BIGINT_DIGITS];
    bigint_digit mP[MAX_BIGINT_DIGITS];
    bigint_digit mQ[MAX_BIGINT_DIGITS];
    bigint_digit temp[MAX_BIGINT_DIGITS];

    int outlen;
    int i;
    int num_words = num_bits/BIGINT_DIGIT_BITS;
    
    bigint_zero(bigp, MAX_BIGINT_DIGITS);
    bigint_zero(bigq, MAX_BIGINT_DIGITS);
    bigint_zero(bigdmp, MAX_BIGINT_DIGITS);
    bigint_zero(bigdmq, MAX_BIGINT_DIGITS);
    bigint_zero(bigqinv, MAX_BIGINT_DIGITS);
    bigint_zero(bigc, MAX_BIGINT_DIGITS);
    
    for (i = 0; i < num_words/2; i++) {
        bigp[num_words/2 - 1 - i] = certp[i];
        bigq[num_words/2 - 1 - i] = certq[i];
        bigdmp[num_words/2 - 1 - i] = dmp[i];
        bigdmq[num_words/2 - 1 - i] = dmq[i];
        bigqinv[num_words/2 - 1 - i] = qinv[i];
    }
    for (i = 0; i < num_words; i++){
        bigc[num_words - 1 - i] = message[i];
        bign[num_words - 1 - i] = certpublickey[i];
    }
    cDigits = bigint_digits(bigc, MAX_BIGINT_DIGITS);
    pDigits = bigint_digits(bigp, MAX_BIGINT_DIGITS);
    qDigits = bigint_digits(bigq, MAX_BIGINT_DIGITS);
    nDigits = bigint_digits(bign, MAX_BIGINT_DIGITS);
    
    /*
     * compute cP and cQ 
     */
    bigint_mod(cP, bigc, cDigits, bigp, pDigits);
    bigint_mod(cQ, bigc, cDigits, bigq, qDigits);

    /*
     * Compute mP = cP^dP mod p  and  mQ = cQ^dQ mod q. 
     */
    bigint_mod_exp(mP, cP, bigdmp, pDigits, bigp, pDigits);
    bigint_zero(mQ, nDigits);
    bigint_mod_exp(mQ, cQ, bigdmq, pDigits, bigq, pDigits);

    /*
     * do CRT 
     * m = ((((mP - mQ) mod p)*qinv) mod p) *q + mQ
     */
    if (bigint_cmp(mP, mQ, pDigits) >= 0) {
        bigint_sub(temp, mP, mQ, pDigits);
    } else {
        bigint_sub(temp, mQ, mP, pDigits);
        bigint_sub(temp, bigp, temp, pDigits);
    }
    
    bigint_mod_mult(temp, temp, bigqinv, bigp, pDigits);
    bigint_mult(temp, temp, bigq, pDigits);
    bigint_add(temp, temp, mQ, nDigits);
      
    outlen = (num_bits + 7)/8;
    I2OSP(result, outlen, temp, nDigits); 
}
#endif
