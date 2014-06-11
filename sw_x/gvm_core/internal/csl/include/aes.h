#ifndef __AES_H__
#define __AES_H__

#include <km_types.h>

#if defined(__cplusplus)
extern "C" {
#endif

/*
  aes key expansion for use in setting PI_AES_EKEY for hardware decryption.
  
  arguments
    key: input, 128 bit key
    expkey: output, 44*4 byte expanded key
  returns >=0 on success, -1 on error
*/
int aes_HwKeyExpand(u8 *key,u8 *expkey);

/*
  aes encryption given the 128 bit key (NOT the expanded key).

  arguments
    key: input, 128 bit key
    initVector: input, 128 bit initialization vector
    dataIn: input, unencrypted data
    bytes: input, number of bytes to be encrypted, must be multiple of 16.
    dataOut: output, encrypted data

  returns >=0 on success, -1 on error
*/
int aes_SwEncrypt(const u8 *key,const u8 *initVector,const u8 *dataIn,u32 bytes,u8 *dataOut);

/*
  aes decryption given the 128 bit key (NOT the expanded key).

  arguments
    key: input, 128 bit key
    initVector: input, 128 bit initialization vector
    dataIn: input, encrypted data
    bytes: input, number of bytes to be decrypted, must be multiple of 16.
    dataOut: output, decrypted data

  returns >=0 on success, -1 on error
*/
int aes_SwDecrypt(const u8 *key,const u8 *initVector,const u8 *dataIn,u32 bytes,u8 *dataOut);

#if defined(__cplusplus)
}
#endif

#endif	/* __AES_H__ */
