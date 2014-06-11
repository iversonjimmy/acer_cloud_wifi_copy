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
#include "aes.h"
#include "sha1.h"
#include "integer_math.h"
#include "conversions.h"
#include <iosctypes.h>

#define KDF1

/* native implementations */
#ifdef HWCRYPTO
extern IOSCError __iosGenerateHash(IOSCHashContext context, u8 * inputData, u32 inputSize, u32 chainingFlag, IOSCHash hashData, s32, void *);
static u8 alignedKeyStr[OCTET_STRING_LEN]__attribute__((aligned(64)));

#endif

  
static void post_process_key(field_2n *sharedkey, CSLOSEccSharedKey sharedkeystr){ 
#ifdef HWCRYPTO
    IOSCHashContext sha;
    IOSCHash hash;
    FE2OSP(alignedKeyStr, sharedkey);
    __iosGenerateHash(sha, 0, 0, IOSC_HASH_FIRST, hash, -1, 0);
    __iosGenerateHash(sha, alignedKeyStr, OCTET_STRING_LEN, IOSC_HASH_LAST, hash, -1, 0);    
    memcpy((unsigned char *)sharedkeystr, hash, sizeof(IOSCHash));
#else
    u8 inputkeystr[OCTET_STRING_LEN];
    SHA1Context shactx;
    /* convert to octet string */
    FE2OSP(inputkeystr, sharedkey);
    SHA1Reset(&shactx);
    SHA1Input(&shactx, inputkeystr, OCTET_STRING_LEN);
    SHA1Result(&shactx, (unsigned char *)sharedkeystr);
#endif
}


/* 
 * Public key generation call
 */

extern curve named_curve;
extern point named_point;

CSL_error 
CSL_GenerateEccKeyPair(CSLOSEccPrivateRand rand, 
		       CSLOSEccPrivateKey privateKey,
		       CSLOSEccPublicKey publicKey){
  field_2n pvtkey;
  OS2FEP(rand, &pvtkey);
  memset(privateKey, 0x0, sizeof(CSLOSEccPrivateKey));
  FE2OSP(privateKey, &pvtkey);

  return CSL_GenerateEccPublicKey(privateKey, publicKey);
}
  
CSL_error
CSL_GenerateEccPublicKey(CSLOSEccPrivateKey privateKey,
                         CSLOSEccPublicKey publicKey){
    field_2n pvtkey;
    point publickey;
    CSL_error error = CSL_OK;
    /* this is needed because the sizes are extended to word size multiples*/
    memset(publicKey, 0x0, sizeof(CSLOSEccPublicKey));
    poly_elliptic_init_233_bit();

    OS2FEP(privateKey, &pvtkey);
    error = alg_generate_public_key(&named_point, &named_curve, &pvtkey, &publickey);
    EC2OSP(&publickey, publicKey, OCTET_STRING_LEN*2);
    return error;
}
  

/* 
 * Call to get shared key from public and private keys
 * it returns numbytes bytes so it can be used to get AES or HMAC key.
 */

CSL_error
CSL_GenerateEccSharedKey(CSLOSEccPrivateKey privateKey, 
                         CSLOSEccPublicKey publicKey, 
                         CSLOSEccSharedKey sharedKey, u32 numBytes){
    
    field_2n pvtkey, sharedkey;
    point publickey;
    CSLOSEccSharedKey sharedKeyCopy;
    CSL_error error = CSL_OK;
    
    if(numBytes > sizeof(CSLOSEccSharedKey)){
        return CSL_OVERFLOW;
    }
    poly_elliptic_init_233_bit();
    
    OS2FEP(privateKey, &pvtkey);
    OS2ECP(publicKey, OCTET_STRING_LEN*2, &publickey);
    error = alg_generate_shared_key(&named_point, &named_curve, &publickey, &pvtkey, &sharedkey);
    /* collect the result into output */
#ifdef KDF1
    post_process_key(&sharedkey, sharedKeyCopy);
#else
    FE2OSP(sharedKeyCopy, &sharedkey);
#endif
    /* convert to octet string of desired length */
    memcpy(sharedKey, sharedKeyCopy, numBytes);
    

    return error;
}


/*
 * This call is used for precomputing the public key in expanded form
 * before using the shared key version with precomputing
 */
CSL_error
CSL_PrecomputeFour(CSLOSEccPublicKey publicKey, CSLOSEccExpPublicKey prePublicKey){
    point publickey;
    int j;
#ifdef __KERNEL__
    point *precomputedpublickey = kmalloc(sizeof(point) * 16, GFP_KERNEL);
    if (!precomputedpublickey)
        return CSL_NO_MEMORY;
#else
    point precomputedpublickey[16];
#endif

    OS2ECP(publicKey, OCTET_STRING_LEN*2, &publickey);
    alg_do_precompute_four(&publickey, &(precomputedpublickey[0]), &named_curve);

    for(j = 0; j < 16; j++){
        EC2OSP(&(precomputedpublickey[j]), (prePublicKey + ((OCTET_STRING_LEN*2)*j)), OCTET_STRING_LEN*2);
    }

#ifdef __KERNEL__
    kfree(precomputedpublickey);
#endif
    
    return CSL_OK;
}


/* 
 * Call to get shared key from private key and public key using an expanded 
 * public key
 */

CSL_error
CSL_GenerateEccSharedKeyPre(CSLOSEccPrivateKey privateKey, 
                            CSLOSEccExpPublicKey publicKey, 
                            CSLOSEccSharedKey sharedKey, 
                            u32 numBytes){
    field_2n pvtkey, sharedkey;
    CSLOSEccSharedKey sharedKeyCopy;
    CSL_error error = CSL_OK;
    int j;
#ifdef __KERNEL__
    point *precomputedpublickey = kmalloc(sizeof(point) * 16, GFP_KERNEL);
    if (!precomputedpublickey)
        return CSL_NO_MEMORY;
#else
    point precomputedpublickey[16];
#endif
    
    poly_elliptic_init_233_bit();
    OS2FEP(privateKey, &pvtkey);    
    /* convert public key to point variable */
    for(j=0; j< 16; j++){
        OS2ECP((publicKey + (2*OCTET_STRING_LEN*j)), (2*OCTET_STRING_LEN),  &(precomputedpublickey[j]));
    }
    error = alg_generate_shared_key_pre(&named_point, &named_curve, &precomputedpublickey[0], &pvtkey, &sharedkey);
    /* collect the result into output */
#ifdef KDF1
    post_process_key(&sharedkey, sharedKeyCopy);
#else
    FE2OSP(sharedKeyCopy, &sharedkey);
#endif
    /* convert to octet string of desired length */
    memcpy(sharedKey, sharedKeyCopy, numBytes);

#ifdef __KERNEL__
    kfree(precomputedpublickey);
#endif

    return error;
}    



/* 
 * Call to get ECDSA signature
 */
typedef struct {
    ec_parameter base;
    ec_signature signature;
    field_2n pvtkey;
    field_2n random_data_field;
} CSL_ComputeEccSig_big_locals;
                
CSL_error
CSL_ComputeEccSig(CSLOSDigest digest, 
                  u32 digestSize,
                  CSLOSEccPrivateKey private_key, 
                  CSLOSEccSig sign,
                  CSLOSEccSigRand random_data)
{
    CSL_error ret;
    CSL_error reterr = CSL_OK;

    CSL_ComputeEccSig_big_locals *big_locals;
#ifdef __KERNEL__
    big_locals = kmalloc(sizeof(*big_locals), GFP_KERNEL);
    if (!big_locals)
        return CSL_NO_MEMORY;
#else
    CSL_ComputeEccSig_big_locals _big_locals;
    big_locals = &_big_locals;
#endif

    alg_init_233_bit_ECDSA(&big_locals->base, NUM_BITS);
    OS2FEP(private_key, &big_locals->pvtkey);
    OS2FEP(random_data, &big_locals->random_data_field);
    ret = alg_poly_ECDSA_signature((char *)digest, digestSize, &big_locals->base, &big_locals->pvtkey, &big_locals->signature, &big_locals->random_data_field);
    if (ret == CSL_OK) {
        EC2OSP((point *)&big_locals->signature, sign, OCTET_STRING_LEN*2);
        reterr = CSL_OK;
    }
    else
        reterr = CSL_DIVIDE_BY_ZERO;

#ifdef __KERNEL__
    kfree(big_locals);
#endif

    return reterr;
}


/*
 * Call to verify ECDSA signature: returns CSL_TRUE or CSL_VERIFY_ERROR
 *
 */
typedef struct {
    ec_parameter base;
    ec_signature signature;
    point public_key;
} CSL_VerifyEccSig_big_locals;

CSL_error CSL_VerifyEccSig( CSLOSDigest digest, 
                            u32 digestSize,
                            CSLOSEccPublicKey publicKey, 
                            CSLOSEccSig sign){
    CSL_error reterr = CSL_OK;
    field_boolean res;
    CSL_VerifyEccSig_big_locals *big_locals;
#ifdef __KERNEL__
    big_locals = kmalloc(sizeof(*big_locals), GFP_KERNEL);
    if (!big_locals)
        return CSL_NO_MEMORY;
#else
    CSL_VerifyEccSig_big_locals _big_locals;
    big_locals = &_big_locals;
#endif
    
    alg_init_233_bit_ECDSA(&big_locals->base, NUM_BITS);
    OS2ECP(publicKey, OCTET_STRING_LEN*2, &big_locals->public_key);
    OS2ECP(sign, OCTET_STRING_LEN*2, (point *)&big_locals->signature);
    
    alg_poly_ECDSA_verify((char *)digest, digestSize, &big_locals->base, &big_locals->public_key, &big_locals->signature, &res);
    reterr = res == CSL_TRUE ? CSL_OK : CSL_VERIFY_ERROR;

#ifdef __KERNEL__
    kfree(big_locals);
#endif
    return reterr;
}
    
