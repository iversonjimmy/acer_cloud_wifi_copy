/* $Id: esc_iosctypes.h,v 1.1 2011-11-07 14:22:19 alex Exp $ */

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
 * Types to be exposed to caller of IOSC 
 */

#ifndef __IOSCTYPES_H__
#define __IOSCTYPES_H__

#include "km_types.h"

/* Platform specific definitions */
#include "crypto_impl_defs.h"

/*
 * Error codes for crypto API
 */
#define IOSC_ERROR_OK                     0
#define IOSC_ERROR_ACCESS                -2000
#define IOSC_ERROR_EXISTS                -2001
#define IOSC_ERROR_INVALID               -2002
#define IOSC_ERROR_MAX                   -2003
#define IOSC_ERROR_NOEXISTS              -2004

#define IOSC_ERROR_INVALID_OBJTYPE       -2005
#define IOSC_ERROR_INVALID_RNG           -2006
#define IOSC_ERROR_INVALID_FLAG          -2007
#define IOSC_ERROR_INVALID_FORMAT        -2008
#define IOSC_ERROR_INVALID_VERSION       -2009
#define IOSC_ERROR_INVALID_SIGNER        -2010
#define IOSC_ERROR_FAIL_CHECKVALUE       -2011
#define IOSC_ERROR_FAIL_INTERNAL         -2012
#define IOSC_ERROR_FAIL_ALLOC            -2013
#define IOSC_ERROR_INVALID_SIZE          -2014
#define IOSC_ERROR_INVALID_ADDR          -2015
#define IOSC_ERROR_INVALID_ALIGN         -2016

typedef s32 IOSCError;

/*
 * Hash related defines 
 */
#define IOSC_HASH_FIRST   0
#define IOSC_HASH_MIDDLE  1
#define IOSC_HASH_LAST    2
#define IOSC_HASH_RESTART 3

/*
 * Hash types to allow support for more than just SHA-1
 */
#define IOSC_HASH_OP_MASK   0x000000ff
#define IOSC_HASH_TYPE_MASK 0x0000ff00
#define IOSC_HASH_SHA1      0x00000000
#define IOSC_HASH_SHA256    0x00000100

#define IOSC_SHA1_INIT      IOSC_HASH_SHA1|IOSC_HASH_FIRST
#define IOSC_SHA1_UPDATE    IOSC_HASH_SHA1|IOSC_HASH_MIDDLE
#define IOSC_SHA1_FINAL     IOSC_HASH_SHA1|IOSC_HASH_LAST
#define IOSC_SHA1_RESTART   IOSC_HASH_SHA1|IOSC_HASH_RESTART

#define IOSC_SHA256_INIT    IOSC_HASH_SHA256|IOSC_HASH_FIRST
#define IOSC_SHA256_UPDATE  IOSC_HASH_SHA256|IOSC_HASH_MIDDLE
#define IOSC_SHA256_FINAL   IOSC_HASH_SHA256|IOSC_HASH_LAST
#define IOSC_SHA256_RESTART IOSC_HASH_SHA256|IOSC_HASH_RESTART

#define IOSC_MAC_FIRST    0
#define IOSC_MAC_MIDDLE   1
#define IOSC_MAC_LAST     2

#define IOSC_SHA1_DIGEST_SIZE    20
#define IOSC_SHA256_DIGEST_SIZE  32

#define IOSC_AES_BLOCKSIZE_BYTES 16
#define IOSC_AES_KEYSIZE_BYTES   16
#define IOSC_AES_IVSIZE_BYTES    16

typedef enum 
{
    IOSC_SECRETKEY_TYPE = 0,
    IOSC_PUBLICKEY_TYPE,
    IOSC_KEYPAIR_TYPE, 
    IOSC_DATA_TYPE
} IOSCObjectType;

typedef enum 
{
    IOSC_ENC_SUBTYPE = 0,
    IOSC_MAC_SUBTYPE,
    IOSC_RSA2048_SUBTYPE,
    IOSC_RSA4096_SUBTYPE, 
    IOSC_ECC233_SUBTYPE,
    IOSC_CONSTANT_SUBTYPE,
    IOSC_VERSION_SUBTYPE
} IOSCObjectSubType;

typedef enum
{
    IOSC_NOSIGN_NOENC = 0,
    IOSC_NOSIGN_ENC,
    IOSC_SIGN_NOENC,
    IOSC_SIGN_ENC
} IOSCSecretKeySecurity;

/* Handles available by default */
#define IOSC_DEV_SIGNING_KEY_HANDLE 0
#define IOSC_DEV_ID_HANDLE          1
#define IOSC_FS_ENC_HANDLE          2
#define IOSC_FS_MAC_HANDLE          3
#define IOSC_COMMON_ENC_HANDLE      4
#define IOSC_BACKUP_ENC_HANDLE      5
#define IOSC_APP_ENC_HANDLE         6

/* Special handles that do not overlap the normal handle space */
#define IOSC_ROOT_KEY_HANDLE        0xfffffff
#define IOSC_COMMON_KEY_PREFIX      0xccccc00
#define IOSC_COMMON_KEY_IDMASK      0xff

#define IOSC_COMMON_KEYID_TO_HANDLE(id) \
    ((IOSCSecretKeyHandle) \
        (IOSC_COMMON_KEY_PREFIX | (IOSC_COMMON_KEY_IDMASK & (id))))
#define IOSC_COMMON_HANDLE_TO_KEYID(handle) \
    (IOSC_COMMON_KEY_IDMASK & (handle))
#define IOSC_IS_COMMON_KEY(handle) \
    ((IOSC_COMMON_KEY_PREFIX & (handle)) == IOSC_COMMON_KEY_PREFIX)

/* Writeable data */
#define IOSC_BOOTOSVER_HANDLE       7
#define IOSC_CACRLVER_HANDLE        8
#define IOSC_SIGNERCRLVER_HANDLE    9
#define IOSC_FSVER_HANDLE           10

/* New common key - specifically for china but define for everyone */
#define IOSC_COMMON2_ENC_HANDLE     11

/* Max bytes through GenerateRand */
#define IOSC_MAX_RAND_BYTES         128
#define IOSC_MAX_HMAC_BYTES         65536

/* Handles to state or context */

typedef u32 IOSCSecretKeyHandle;
typedef u32 IOSCKeyHandle; 
typedef u32 IOSCDataHandle;
typedef u32 IOSCPublicKeyHandle;
#if defined(__CLOUDNODE__)
typedef u8  IOSCHashContext[IOSC_HASH_CONTEXT_SIZE] __attribute__ ((aligned(8)));
#else
typedef u8  IOSCHashContext[IOSC_HASH_CONTEXT_SIZE];
#endif
typedef u8  IOSCHash[20];
typedef u8  IOSCHash256[32];
typedef u8  IOSCCertName[64];

typedef u8 IOSCSha1Hash[IOSC_SHA1_DIGEST_SIZE];
typedef u8 IOSCSha256Hash[IOSC_SHA256_DIGEST_SIZE];
typedef u8 IOSCAesKey[IOSC_AES_KEYSIZE_BYTES];
typedef u8 IOSCAesIv[IOSC_AES_IVSIZE_BYTES];
typedef u8 IOSCHmacKey[IOSC_SHA1_DIGEST_SIZE];

/* Size constraints for AES and SHA */
#define IOSC_AES_BLOCK_SIZE     16  /* AES works on 16 byte blocks */
#define IOSC_AES_SIZE_ALIGN     IOSC_AES_BLOCK_SIZE     
#define IOSC_SHA1_SIZE_ALIGN    0   /* No constraints for hash input size */
#define IOSC_SHA256_SIZE_ALIGN  0   /* No constraints for hash input size */

/* Not exposing actual cert defns */

typedef u8 IOSCGenericCert[1024];  /* largest cert size */
typedef u8 IOSCEccSignedCert[384]; /* for device certs */

/* Cert types exposed to application */

typedef u32 IOSCCertSigType;

#define IOSC_SIG_RSA4096      0x00010000  /* RSA 4096 bit signature */
#define IOSC_SIG_RSA2048      0x00010001  /* RSA 2048 bit signature */
#define IOSC_SIG_ECC          0x00010002  /* ECC signature 512 bits*/
#define IOSC_SIG_RSA4096_H256 0x00010003  /* RSA 4096 bit sig using SHA-256 */
#define IOSC_SIG_RSA2048_H256 0x00010004  /* RSA 2048 bit sig using SHA-256 */
#define IOSC_SIG_ECC_H256     0x00010005  /* ECC sig 512 bits using SHA-256 */
#define IOSC_SIG_HMAC_SHA1    0x00010006  /* HMAC-SHA1 160 bit signature */

typedef u32 IOSCCertPubKeyType;

#define IOSC_PUBKEY_RSA4096   0     /* RSA 4096 bit key */
#define IOSC_PUBKEY_RSA2048   1     /* RSA 2048 bit key */
#define IOSC_PUBKEY_ECC       2     /* ECC pub key 512 bits*/

/* Encryption and Padding types */

typedef u32 IOSCPubKeyEncryptType;

#define IOSC_PUBKEY_ENCRYPT_RSA4096   0
#define IOSC_PUBKEY_ENCRYPT_RSA2048   1
#define IOSC_PUBKEY_ENCRYPT_RSA1024   2
#define IOSC_PUBKEY_ENCRYPT_ECC512    3
#define IOSC_PUBKEY_ENCRYPT_ECC233    4

typedef u32 IOSCPadMsgType;

#define IOSC_PADMSG_OAEP              0    /* Non-deterministic padding (more secure)*/
#define IOSC_PADMSG_EMSA_PKCS1_V1_5   1    /* Deterministic padding */

/* 
 * Key type definitions
 */

#define IOSC_ECC_KEYSIZE_BYTES    30

typedef u8 IOSCEccPublicKey[IOSC_ECC_KEYSIZE_BYTES*2];
typedef u8 IOSCEccPrivateKey[IOSC_ECC_KEYSIZE_BYTES];
typedef u8 IOSCEccSig[IOSC_ECC_KEYSIZE_BYTES*2];

typedef u8 IOSCRsaPublicKey2048[256];
typedef u8 IOSCRsaPublicKey4096[512];
typedef u8 IOSCRsaExponent[4];
typedef u8 IOSCRsaSig2048[256];
typedef u8 IOSCRsaSig4096[512];

/* padding to do word size alignment */
typedef u8 IOSCEccPrivatePad[2];
typedef u8 IOSCEccPublicPad[4];

#endif 
