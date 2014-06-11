#include "aes.h"

#include "csl_impl.h"

#include "aes_api.h"

#define BLOCK_SIZE 128
#define FILE_SIZE 512

#define AES_KEYLEN     128
#define AES_BLOCKLEN   128

typedef struct {
    AesKeyInstance keyI;
    AesCipherInstance cipher;
} aes_big_locals;

/*
  returns >=0 on success, -1 on error
  ***********************************
  use this only for HW
  for software encryption and decryption, the 
  key expansion is included
  if key is in ascii isAscii = 1, else 0
  ***********************************

*/
int aes_HwKeyExpand(u8 *key,u8 *expkey)
{
    int err = 0;
#ifndef __KERNEL__
    aes_big_locals _big_locals;
#endif
    aes_big_locals *big_locals;
#ifdef __KERNEL__
    big_locals = kmalloc(sizeof(*big_locals), GFP_KERNEL);
    if (!big_locals)
        return -1;
#else
    big_locals = &_big_locals;
#endif

    if(aesMakeKey(&big_locals->keyI, AES_DIR_DECRYPT, AES_KEYLEN, key) != AES_TRUE){
        err = -1;
        goto end;
    }

    /* scan for creating expanded key in form used by pi */
    memcpy(expkey, big_locals->keyI.rk, 44*4);

 end:
#ifdef __KERNEL__
    kfree(big_locals);
#endif

    return err;
}

/*************************************
 * Use this only for software (Key expansion included)
 * isAscii = 1 means initVector and key are in ascii
 *************************************
 */
int aes_SwEncrypt(const u8 *key,const u8 *initVector,const u8 *dataIn,u32 bytes,u8 *dataOut)
{
    int error;
    int err = 0;
#ifndef __KERNEL__
    aes_big_locals _big_locals;
#endif
    aes_big_locals *big_locals;
#ifdef __KERNEL__
    big_locals = kmalloc(sizeof(*big_locals), GFP_KERNEL);
    if (!big_locals)
        return -1;
#else
    big_locals = &_big_locals;
#endif

    /* make key */
    if(aesMakeKey(&big_locals->keyI, AES_DIR_ENCRYPT, AES_KEYLEN, key) != AES_TRUE){
        err = -1;
        goto end;
    }

    /* initialise parameters */
    error = aesCipherInit(&big_locals->cipher, AES_MODE_CBC, initVector);
    if (error != AES_TRUE) {
        /*
        fprintf(stderr,"cipherInit error %d \n", error);
        */
        err = -1;
        goto end;
    }

    /*encrypt */
    if(aesBlockEncrypt(&big_locals->cipher, &big_locals->keyI, dataIn, bytes*8, dataOut)
       != bytes*8){
/*
        fprintf(stderr, "cipher encryption error \n");
*/
        err = -1;
        goto end;
    }

 end:
#ifdef __KERNEL__
    kfree(big_locals);
#endif

    return err;
}


/*************************************
 * Use this only for software (Key expansion included)
 * isAscii = 1 means key and IV are in ascii
 *************************************
 */

int aes_SwDecrypt(const u8 *key,const u8 *initVector,const u8 *dataIn,u32 bytes,u8 *dataOut)
{
    int error;
    int err = 0;
#ifndef __KERNEL__
    aes_big_locals _big_locals;
#endif
    aes_big_locals *big_locals;
#ifdef __KERNEL__
    big_locals = kmalloc(sizeof(*big_locals), GFP_KERNEL);
    if (!big_locals)
        return -1;
#else
    big_locals = &_big_locals;
#endif

    /* make key */
    if(aesMakeKey(&big_locals->keyI, AES_DIR_DECRYPT, AES_KEYLEN, key) != AES_TRUE){
        err = -1;
        goto end;
    }

    /* initialise parameters */
    error = aesCipherInit(&big_locals->cipher, AES_MODE_CBC, initVector);
    if (error != AES_TRUE) {
/*
        fprintf(stderr,"cipherInit error %d \n", error);
*/
        err = -1;
        goto end;
    }

    /*encrypt */
    if(aesBlockDecrypt(&big_locals->cipher, &big_locals->keyI, dataIn, bytes*8, dataOut) != bytes*8){
/*
        fprintf(stderr, "cipher encryption error \n");
*/
        err = -1;
        goto end;
    }

 end:
#ifdef __KERNEL__
    kfree(big_locals);
#endif

    return err;
}
