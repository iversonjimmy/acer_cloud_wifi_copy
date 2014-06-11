/*
 *               Copyright (C) 2009, BroadOn Communications Corp.
 *
 *  These coded instructions, statements, and computer programs contain
 *  unpublished  proprietary information of BroadOn Communications Corp.,
 *  and  are protected by Federal copyright law. They may not be disclosed
 *  to  third  parties or copied or duplicated in any form, in whole or in
 *  part, without the prior written consent of BroadOn Communications Corp.
 *
 */

/*
 * Library that implements all the functionality for cryptographic kernel 
 * functions
 */
#include "core_glue.h"

#include <esc_iosctypes.h>
#include <iosccert.h>
#include <csl.h>
#include <sha1.h>
#include <aes.h>
#include "bsl.h"
#include "bsl_defs.h"
#include "keystore.h"
#include "crypto_impl.h"
#include "integer_math.h"

#if defined(GHV)
// Need to allow modificatin of the common handle in order to support
// use-once tickets.
#define CHECK_DEFAULT_HANDLES(handle) \
    if( ((handle <= BSL_MAX_DEFAULT_HANDLES) && \
	 (handle != IOSC_COMMON_ENC_HANDLE)) \
        || IOSC_IS_COMMON_KEY(handle) \
        || (handle == IOSC_ROOT_KEY_HANDLE)){ \
        return IOSC_ERROR_ACCESS; \
    }
#else // !GHV
#define CHECK_DEFAULT_HANDLES(handle) \
    if((handle <= BSL_MAX_DEFAULT_HANDLES) \
        || IOSC_IS_COMMON_KEY(handle) \
        || (handle == IOSC_ROOT_KEY_HANDLE)){ \
        return IOSC_ERROR_ACCESS; \
    }
#endif // !defined(GHV)

#define SET_PROTECTION(handle, prot) \
    error = BSL_SetProtection(handle, prot); \
    if(error != IOSC_ERROR_OK){ \
        return error; \
    }

#define GET_PROTECTION(handle, prot) \
    error = BSL_GetProtection(handle, prot); \
    if(error != IOSC_ERROR_OK){ \
        return error; \
    }

#define CHECK_DEFAULT_RO_DATA_HANDLES(handle) \
    if(handle == IOSC_DEV_ID_HANDLE){ \
        return IOSC_ERROR_ACCESS; \
    }

#define MAKE_TYPE(type, subType) (((type) << BSL_TYPE_SHIFT) | (subType)) 

#define ALIGNUP(x, alignment) (((x) + (alignment) - 1) & ~((alignment) - 1))
#define ALIGNED(x, alignment) (((x) & ((alignment) - 1)) == 0)

#ifdef IOS
// NTOHL() & HTONL() has been defined in <sys/_endian.h> on iOS.
static u32 __NTOHL(u32 x)
#else
static u32 NTOHL(u32 x)
#endif
{
#ifdef HOST_IS_LITTLE_ENDIAN
    return (x << 24) | ((x << 8) & 0xFF0000) | ((x >> 8) & 0xFF00) | ((x >> 24) & 0xFF);
#else
    return x;
#endif
}

#ifdef IOS
#define __HTONL(x) __NTOHL(x)
#else
#define HTONL(x) NTOHL(x)
#endif


static IOSCError
getSizeFromType(u8 keytype, u8 keysubtype, u32 *keySize)
{
    switch (MAKE_TYPE(keytype, keysubtype)) {
    case MAKE_TYPE(IOSC_SECRETKEY_TYPE, IOSC_ENC_SUBTYPE):
	 *keySize = sizeof(CSLOSAesKey);
	break;
    case MAKE_TYPE(IOSC_SECRETKEY_TYPE, IOSC_MAC_SUBTYPE):
	*keySize = sizeof(CSLOSHMACKey);
	break;
    case MAKE_TYPE(IOSC_SECRETKEY_TYPE, IOSC_ECC233_SUBTYPE):
	*keySize = sizeof(CSLOSEccPrivateKey);
	break;

    case MAKE_TYPE(IOSC_PUBLICKEY_TYPE, IOSC_RSA2048_SUBTYPE):
	*keySize = sizeof(CSLOSRsaPublicKey2048);
	break;
    case MAKE_TYPE(IOSC_PUBLICKEY_TYPE, IOSC_RSA4096_SUBTYPE):
	*keySize = sizeof(CSLOSRsaPublicKey4096);
	break;
    case MAKE_TYPE(IOSC_PUBLICKEY_TYPE, IOSC_ECC233_SUBTYPE):
	*keySize = sizeof(CSLOSEccPublicKey);
	break;

    case MAKE_TYPE(IOSC_KEYPAIR_TYPE, IOSC_ECC233_SUBTYPE):
	*keySize = sizeof(CSLOSEccPrivateKey) + sizeof(CSLOSEccPublicKey);
	break;

    case MAKE_TYPE(IOSC_DATA_TYPE, IOSC_VERSION_SUBTYPE):
    case MAKE_TYPE(IOSC_DATA_TYPE, IOSC_CONSTANT_SUBTYPE):
	*keySize = 0;
	break;

    default:
        return IOSC_ERROR_INVALID;
    }    
    return IOSC_ERROR_OK;
}


static u32 
getExpSize(CSLOSRsaExponent exponent, u32 maxSize)
{
    int trailingZeros = 0;
    int i;
    /* find trailing zeros */
    for(i = maxSize -1; i >= 0; i--){
        if(exponent[i] != 0x0){
            break;
        }
        trailingZeros++;
    }
    return maxSize - trailingZeros;
}
 

static void
getKeyTypeSubtype(IOSCKeyHandle handle, u8 *keytype, u8 *keysubtype)
{
    u8 type;
    /* special case root key handle */
    if (handle == IOSC_ROOT_KEY_HANDLE) {
        *keytype = IOSC_PUBLICKEY_TYPE;
        *keysubtype = IOSC_RSA4096_SUBTYPE;
    } else if (IOSC_IS_COMMON_KEY(handle)) {
        *keytype = IOSC_SECRETKEY_TYPE;
        *keysubtype = IOSC_ENC_SUBTYPE;
    } else {
        keyGetType(handle, &type);
        *keytype = ((type & BSL_TYPE_MASK)>>BSL_TYPE_SHIFT);
        *keysubtype = (type & BSL_SUBTYPE_MASK);
    }
    return;
}


IOSCError
BSL_Initialize()
{
    /*
     * Call the platform specific crypto initialization
     */
    return IOSCryptoInitialize();
}


IOSCError
BSL_KeyStoreInit()
{
    keyStoreInit();

    return IOSC_ERROR_OK;
}


IOSCError
BSL_InstallObject(u32 handle,
		  IOSCObjectType type,
		  IOSCObjectSubType subtype,
		  u8 *key, u32 keySize,
		  u8 *misc)
{
    u32 keySizeFromType = 0;
    IOSCError error;

    // Can only install objects into default handles
    if (handle > BSL_MAX_DEFAULT_HANDLES) {
        return IOSC_ERROR_INVALID;
    }

    error = getSizeFromType((u8 )type, (u8 )subtype, &keySizeFromType);
    if (error != IOSC_ERROR_OK) {
        return error;
    }

    if (keySizeFromType != keySize) {
        return IOSC_ERROR_INVALID_SIZE;
    }

    error = keyInstall(handle, keySize);
    if (error != IOSC_ERROR_OK) {
        return error;
    }

    error = keySetType(handle, MAKE_TYPE(type, subtype));
    if (error != IOSC_ERROR_OK) goto del;

    if (keySize > 0) {
        error = keyInsertData(handle, key, keySize);
        if (error != IOSC_ERROR_OK) goto del;
    }
    
    if (misc) {
        error = keyInsertMiscData(handle, misc);
        if (error != IOSC_ERROR_OK) goto del;
    }

    // Installed objects are by default not exportable
    BSL_SetProtection(handle, BSL_PROT_NO_EXPORT);

    return IOSC_ERROR_OK;

 del:
    keyDelete(handle);
    return error;
}

IOSCError 
BSL_CreateObject (u32 *handle, 
		  IOSCObjectType type, 
		  IOSCObjectSubType subtype)
{
    u32 keySize = 0;
    IOSCError error;

    error = getSizeFromType((u8 )type, (u8 )subtype, &keySize);
    if (error != IOSC_ERROR_OK) {
        return IOSC_ERROR_FAIL_ALLOC;
    }
    
    error = keyAlloc(handle, keySize);
    if (error != IOSC_ERROR_OK) {
        return IOSC_ERROR_FAIL_ALLOC;
    }
    error = keySetType(*handle, MAKE_TYPE(type, subtype));
    if (error != IOSC_ERROR_OK) {
        keyFree(*handle);
        return error;
    }
    return IOSC_ERROR_OK;
}

IOSCError 
BSL_DeleteObject(u32 handle)
{
    IOSCError error;
    CHECK_DEFAULT_HANDLES(handle);

    error = keyFree(handle);
    if (error != IOSC_ERROR_OK) {
        return IOSC_ERROR_FAIL_ALLOC;
    }
    return IOSC_ERROR_OK;
}

IOSCError 
BSL_ImportSecretKey (IOSCSecretKeyHandle importedHandle, 
                     IOSCSecretKeyHandle verifyHandle, 
                     IOSCSecretKeyHandle decryptHandle, 
                     IOSCSecretKeySecurity flag,
                     u8 *signBuffer, 
                     u8 *ivData,
                     u8 *keyBuffer)
{
    IOSCError error = IOSC_ERROR_OK;
    u8 keyType, keySubType;
    u32 keySize = 0x0;
    IOSCSha1Hash hmac;
    IOSCHashContext context;
    u32 importedProt;
    u32 decryptProt;

    /*
     * Don't allow overwriting default keys
     */
    CHECK_DEFAULT_HANDLES(importedHandle); 

    /*
     * Set secret key protection attribute
     *
     * Assign protection bits for this key. it cannot be NO_EXPORT from 
     * this call. It has to be either ENC_EXPORT or PTXT_EXPORT.
     * It is ENC_EXPORT if flag is ENC and decryptHandle is
     * ENC_EXPORT or NO_EXPORT
     * In all other cases its PTXT_EXPORT
     */
    importedProt = BSL_PROT_PTXT_EXPORT; /* default */
    if((flag == IOSC_NOSIGN_ENC) || (flag == IOSC_SIGN_ENC)){
        GET_PROTECTION(decryptHandle, &decryptProt);
        if((decryptProt == BSL_PROT_NO_EXPORT) || (decryptProt == BSL_PROT_ENC_EXPORT)){
            importedProt = BSL_PROT_ENC_EXPORT;
        }
    }
    SET_PROTECTION(importedHandle, importedProt);

    /* check that types match */
    getKeyTypeSubtype(importedHandle, &keyType, &keySubType);
    if (keyType != IOSC_SECRETKEY_TYPE) {
        return IOSC_ERROR_INVALID_OBJTYPE;
    }
    
    /* check sign and decrypt if needed */
    error = BSL_GetKeySize(&keySize, importedHandle);
    if (error != IOSC_ERROR_OK) {
        return IOSC_ERROR_INVALID_OBJTYPE;
    }
    
    if ((flag == IOSC_SIGN_ENC) || (flag == IOSC_SIGN_NOENC)) {
        getKeyTypeSubtype(verifyHandle, &keyType, &keySubType);
        if (keyType != IOSC_PUBLICKEY_TYPE) {
            return IOSC_ERROR_INVALID_OBJTYPE;
        }   
    
        if (flag == IOSC_SIGN_ENC) { /* adjust size if necessary */
            keySize = ALIGNUP(keySize, CSL_AES_BLOCKSIZE_BYTES);
        }

        error = BSL_GenerateBlockMAC(context, 0, 0, 0, 0, verifyHandle, IOSC_MAC_FIRST, signBuffer);
        error = BSL_GenerateBlockMAC(context, keyBuffer, keySize, 0, 0, verifyHandle, IOSC_MAC_LAST, hmac);

        if(memcmp(hmac, signBuffer, sizeof(CSLOSSha1Hash)) != 0){
            return IOSC_ERROR_FAIL_CHECKVALUE;
        }
    }

    if ((flag == IOSC_NOSIGN_ENC) || (flag == IOSC_SIGN_ENC)) {
        u8 *secretCopy;

        getKeyTypeSubtype(decryptHandle, &keyType, &keySubType);
        if ((keyType != IOSC_SECRETKEY_TYPE)||(keySubType != IOSC_ENC_SUBTYPE)) {
            return IOSC_ERROR_INVALID_OBJTYPE;
        }

        keySize = ALIGNUP(keySize, CSL_AES_BLOCKSIZE_BYTES);
        secretCopy = (u8 *) IOSCryptoAllocAligned(keySize, IOSC_AES_ADDR_ALIGN);
        if (secretCopy == NULL) {
            error = IOSC_ERROR_FAIL_INTERNAL;
            goto out;
        }

        error = BSL_Decrypt(decryptHandle, ivData, keyBuffer, keySize, secretCopy);
        if (error == IOSC_ERROR_OK) {
            error = keyInsertData(importedHandle, secretCopy, keySize);
        }

    out:
        IOSCryptoFreeAligned(secretCopy, IOSC_AES_ADDR_ALIGN);

        if (error != 0) {
            return IOSC_ERROR_FAIL_INTERNAL;
        }
    
    } else {

        error =  keyInsertData(importedHandle, keyBuffer, keySize);
        if (error != 0) {
            return IOSC_ERROR_FAIL_INTERNAL;
        }
    }

    return error;
}

/*
 * TODO Check if key can be exported from this handle 
 */

IOSCError 
BSL_ExportSecretKey (IOSCSecretKeyHandle exportedHandle,  
                     IOSCSecretKeyHandle signHandle, 
                     IOSCSecretKeyHandle encryptHandle, 
                     IOSCSecretKeySecurity flag,
                     u8 *signBuffer, 
                     u8 *ivData,
                     u8 *keyBuffer)
{
    IOSCError error = IOSC_ERROR_OK;
    /* check if this handle can be exported */
    u8 keyType = 0; 
    u8 keySubType = 0;
    u32 keySize = 0;
    u32 keySizeExp;
    u8 *secretCopy = NULL;
    IOSCHashContext context;
    u32 exportedProt;
    u32 encryptProt;

    /* If it is BSL_PROT_NO_EXPORT, return */
    GET_PROTECTION(exportedHandle, &exportedProt);

    if(exportedProt == BSL_PROT_NO_EXPORT){
        error = IOSC_ERROR_ACCESS;
        goto out;
    }
    /* If it is BSL_PROT_ENC_EXPORT, check that the flag is right */
    if(exportedProt == BSL_PROT_ENC_EXPORT){
        if((flag != IOSC_NOSIGN_ENC) && (flag != IOSC_SIGN_ENC)){
            error = IOSC_ERROR_ACCESS;
            goto out;
        }
        /* If the encrypthandle is NO_EXPORT or ENC_EXPORT, permit;
         * else, dont
         */
        GET_PROTECTION(encryptHandle, &encryptProt);
        if((encryptProt != BSL_PROT_NO_EXPORT) && (encryptProt != BSL_PROT_ENC_EXPORT)){
            error = IOSC_ERROR_ACCESS;
            goto out;
        }
    }
    getKeyTypeSubtype(exportedHandle, &keyType, &keySubType);
    if ((keyType != IOSC_SECRETKEY_TYPE) && (keyType != IOSC_KEYPAIR_TYPE)) {
        error = IOSC_ERROR_INVALID_OBJTYPE;
        goto out;
    }
    
    error = BSL_GetKeySize(&keySize, exportedHandle);
    if(error != IOSC_ERROR_OK){
        error = IOSC_ERROR_INVALID_OBJTYPE;
        goto out;
    }

    keySizeExp = ALIGNUP(keySize, CSL_AES_BLOCKSIZE_BYTES);
    secretCopy = (u8 *) IOSCryptoAllocAligned(keySizeExp, IOSC_AES_ADDR_ALIGN);

    if (secretCopy == NULL) {
        error = IOSC_ERROR_FAIL_INTERNAL;
        goto out;
    }

    error = keyGetData(exportedHandle, secretCopy, keySize);
    if (error != IOSC_ERROR_OK) {
        error = IOSC_ERROR_FAIL_INTERNAL;
        goto out;
    }
    
    memset(secretCopy + keySize, secretCopy[0], keySizeExp - keySize);

    if ((flag == IOSC_NOSIGN_ENC) || (flag == IOSC_SIGN_ENC)) {
        getKeyTypeSubtype(encryptHandle, &keyType, &keySubType);
        if ((keyType != IOSC_SECRETKEY_TYPE)||(keySubType != IOSC_ENC_SUBTYPE)) {
            error = IOSC_ERROR_INVALID_OBJTYPE;
            goto out;
        }

	error = BSL_Encrypt(encryptHandle, ivData, secretCopy, keySizeExp, secretCopy);
        if(error != IOSC_ERROR_OK) {
            goto out;
        }

        keySize = keySizeExp;
    }

    if ((flag == IOSC_SIGN_ENC) || (flag == IOSC_SIGN_NOENC)) {
        getKeyTypeSubtype(signHandle, &keyType, &keySubType);
        if ((keyType != IOSC_SECRETKEY_TYPE)||(keySubType != IOSC_MAC_SUBTYPE)) {
            error = IOSC_ERROR_INVALID_OBJTYPE;
            goto out;
        }

        error = BSL_GenerateBlockMAC(context, 0, 0, 0, 0, signHandle, IOSC_MAC_FIRST, signBuffer);
        error = BSL_GenerateBlockMAC(context, secretCopy, keySizeExp, 0, 0, signHandle, IOSC_MAC_LAST, signBuffer);
    }
    memcpy(keyBuffer, secretCopy, keySize);
    
 out:
    if (secretCopy) IOSCryptoFreeAligned(secretCopy, IOSC_AES_ADDR_ALIGN);
    return error;
}

/*
 * TODO add signer id, validate ECC public key.
 */

IOSCError 
BSL_ImportPublicKey (u8 *publicKeyData, 
                     u8 *publicKeyExponent,
                     IOSCPublicKeyHandle publicKeyHandle)
{
    IOSCError error = IOSC_ERROR_OK;
    u8 keyType = 0; 
    u8 keySubType = 0;
    u32 keySize = 0;

    /* any call that write secret key material should check if its default*/
    CHECK_DEFAULT_HANDLES(publicKeyHandle); 

    getKeyTypeSubtype(publicKeyHandle, &keyType, &keySubType);
    
    if (keyType != IOSC_PUBLICKEY_TYPE) {
        return IOSC_ERROR_INVALID_OBJTYPE;
    }
    error = BSL_GetKeySize(&keySize, publicKeyHandle);
    if(error != IOSC_ERROR_OK){
        return IOSC_ERROR_INVALID_OBJTYPE;
    }
    error =  keyInsertData(publicKeyHandle, publicKeyData, keySize);
    if (error != IOSC_ERROR_OK) {
        return IOSC_ERROR_FAIL_INTERNAL;
    }
    if((keySubType == IOSC_RSA2048_SUBTYPE) ||
       (keySubType == IOSC_RSA4096_SUBTYPE)){

        error = keyInsertMiscData(publicKeyHandle, publicKeyExponent);
        if (error != IOSC_ERROR_OK) {
            return IOSC_ERROR_FAIL_INTERNAL;
        }
    }
    
    return error;
}

IOSCError 
BSL_ExportPublicKey (u8 *publicKeyData, 
                     u8 *publicKeyExponent,
                     IOSCPublicKeyHandle publicKeyHandle)
{
    IOSCError error = IOSC_ERROR_OK;
    u8 keyType = 0; 
    u8 keySubType = 0;
    u32 keySize = 0;
    u8 privatepublic[CSL_KEY_STRING_LEN*3]; 
    getKeyTypeSubtype(publicKeyHandle, &keyType, &keySubType);
    
    if ((keyType != IOSC_PUBLICKEY_TYPE) &&
       (keyType != IOSC_KEYPAIR_TYPE)) {
        return IOSC_ERROR_INVALID_OBJTYPE;
    }
    error = BSL_GetKeySize(&keySize, publicKeyHandle);
    if (error != IOSC_ERROR_OK) {
        return IOSC_ERROR_INVALID_OBJTYPE;
    }
    /* handle root key differently */
    if (publicKeyHandle == IOSC_ROOT_KEY_HANDLE) {
        memcpy(publicKeyData, IOSCryptoGetRootKey(), keySize);
        memcpy(publicKeyData, IOSCryptoGetRootKeyExp(), sizeof(CSLOSRsaExponent));
        return IOSC_ERROR_OK;
    }
    
    if (keyType == IOSC_PUBLICKEY_TYPE) {
        error =  keyGetData(publicKeyHandle, publicKeyData, keySize);
        if (error != IOSC_ERROR_OK) {
            return IOSC_ERROR_FAIL_INTERNAL;
        }
        if((keySubType == IOSC_RSA2048_SUBTYPE) ||
           (keySubType == IOSC_RSA4096_SUBTYPE)) {
            error = keyGetMiscData(publicKeyHandle, publicKeyExponent);
        if (error != IOSC_ERROR_OK) {
            return IOSC_ERROR_FAIL_INTERNAL;
        }
        }
    } else {
        /* extract public key of key pair data */
        error =  keyGetData(publicKeyHandle, privatepublic, keySize);
        memcpy(publicKeyData, privatepublic + sizeof(CSLOSEccPrivateKey), sizeof(CSLOSEccPublicKey));
        if (error != IOSC_ERROR_OK) {
            return IOSC_ERROR_FAIL_INTERNAL;
        }
    }
        
    return error;
}

static IOSCError 
generateKey (IOSCKeyHandle handle)
{
    IOSCError error = IOSC_ERROR_OK;
    /* Get size from type: MAX for now */
    u8 randoms[CSL_KEY_STRING_LEN], privatepublic[CSL_KEY_STRING_LEN*3]; 
    
    /* One private, one public key */
    CSLOSEccPublicKey publickey;
    CSLOSEccPrivateKey privatekey;
    u8 keyType = 0; 
    u8 keySubType = 0;
    u32 keySize;
    int i;

    /* Any call that write secret key material should check if its default*/
    CHECK_DEFAULT_HANDLES(handle); 
    /* This key can be exported encrypted, or used to encrypt another key 
     * that is exported encrypted 
     */
    SET_PROTECTION(handle, BSL_PROT_ENC_EXPORT);

    /* Determine size first 
     */
    getKeyTypeSubtype(handle, &keyType, &keySubType);
    if ((keyType != IOSC_SECRETKEY_TYPE) &&
       (keyType != IOSC_KEYPAIR_TYPE)) {
        return IOSC_ERROR_INVALID_OBJTYPE;
    }
    error = BSL_GetKeySize(&keySize, handle);
    if(error != IOSC_ERROR_OK){
        return IOSC_ERROR_INVALID_OBJTYPE;
    }

    error = BSL_GenerateRand(randoms, sizeof randoms);

    if(error  != IOSC_ERROR_OK){
        return IOSC_ERROR_FAIL_INTERNAL;
    }
    
    switch (keyType) {
    case IOSC_SECRETKEY_TYPE:
        switch (keySubType) {
        case IOSC_ENC_SUBTYPE:
            error =  keyInsertData(handle, randoms, keySize);
            break;
        case IOSC_MAC_SUBTYPE:
            /* set to 160 bits */
            for (i= sizeof(CSLOSHMACKey); i < sizeof(randoms); i++) {
                randoms[i] = 0x0;
            }
            error =  keyInsertData(handle, randoms, keySize);
            break;
        case IOSC_ECC233_SUBTYPE:
            /* set to OS format : 0x01 ff ff ... (233 bits will be used) */
            error =  keyInsertData(handle, randoms, keySize);
            break;
        default:
            return IOSC_ERROR_INVALID_FORMAT;
        }
        if (error != IOSC_ERROR_OK) {
            return IOSC_ERROR_FAIL_INTERNAL;
        }
        break;
    case IOSC_KEYPAIR_TYPE:
        switch (keySubType) {
        case IOSC_ECC233_SUBTYPE:            
            CSL_GenerateEccKeyPair(randoms, privatekey, publickey);
            memcpy(privatepublic, privatekey, sizeof(CSLOSEccPrivateKey));
            memcpy(privatepublic + sizeof(CSLOSEccPrivateKey), publickey, sizeof(CSLOSEccPublicKey));
            error =  keyInsertData(handle, privatepublic, keySize);
            break;
        default:
            return IOSC_ERROR_INVALID_FORMAT;
        }
        if (error != IOSC_ERROR_OK) {
            return IOSC_ERROR_FAIL_INTERNAL;
        }
        break;
    default:
        return IOSC_ERROR_INVALID_FORMAT;
    }    
    return error;
}


IOSCError 
BSL_GenerateKey (IOSCKeyHandle handle)
{
    return generateKey(handle);
}


IOSCError 
BSL_ComputeSharedKey (IOSCSecretKeyHandle privateHandle, 
                     IOSCPublicKeyHandle publicHandle, 
                     IOSCSecretKeyHandle sharedHandle)
{
    IOSCError error;
    CSL_error cslError;
    CSLOSEccPublicKey publickey;
    CSLOSEccPrivateKey privatekey;
    CSLOSEccSharedKey sharedkey;
    u8 privatepublic[CSL_KEY_STRING_LEN*3];
    u8 keyType = 0; 
    u8 keySubType = 0;
    u32 keySize = 0;
    u32 returnSize = 0;
    
    u32 sharedProt;
    /* Any call that write secret key material should check if its default*/
    CHECK_DEFAULT_HANDLES(sharedHandle); 

    /* Set up protection bits: it inherits the bits of the input secret */
    GET_PROTECTION(privateHandle, &sharedProt);
    SET_PROTECTION(sharedHandle, sharedProt);

    getKeyTypeSubtype(privateHandle, &keyType, &keySubType);
    if((keyType != IOSC_SECRETKEY_TYPE) && (keyType != IOSC_KEYPAIR_TYPE)){
        return IOSC_ERROR_INVALID_OBJTYPE;
    }
    /* check its ECC233 type */
    if(keySubType != IOSC_ECC233_SUBTYPE){
        return IOSC_ERROR_INVALID_OBJTYPE;
    }
    getKeyTypeSubtype(publicHandle, &keyType, &keySubType);
    if((keyType != IOSC_PUBLICKEY_TYPE) && (keyType != IOSC_KEYPAIR_TYPE)){
        return IOSC_ERROR_INVALID_OBJTYPE;
    }
    /* check its ECC233 type */
    if(keySubType != IOSC_ECC233_SUBTYPE){
        return IOSC_ERROR_INVALID_OBJTYPE;
    }

    getKeyTypeSubtype(sharedHandle, &keyType, &keySubType);
    if((keyType != IOSC_SECRETKEY_TYPE) || 
       ((keySubType != IOSC_ENC_SUBTYPE) 
           &&(keySubType != IOSC_MAC_SUBTYPE))){
        return IOSC_ERROR_INVALID_OBJTYPE;
    }

    error = keyGetData(privateHandle, privatekey, sizeof(privatekey));
    if (error != IOSC_ERROR_OK) {
        return IOSC_ERROR_FAIL_INTERNAL;
    } 

    /* extract public key */
    error = BSL_GetKeySize(&keySize, publicHandle);
    if (error != IOSC_ERROR_OK){
        return IOSC_ERROR_FAIL_INTERNAL;
    }

    getKeyTypeSubtype(publicHandle, &keyType, &keySubType);
    if (keyType == IOSC_PUBLICKEY_TYPE) {
        error = keyGetData(publicHandle, publickey, keySize);
        if (error != IOSC_ERROR_OK) {
            return IOSC_ERROR_FAIL_INTERNAL;
        }
    } else if (keyType == IOSC_KEYPAIR_TYPE) {
        error = keyGetData(publicHandle, privatepublic, keySize);
        memcpy(publickey, privatepublic + sizeof(CSLOSEccPrivateKey), sizeof(CSLOSEccPublicKey));
        if (error != IOSC_ERROR_OK) {
            return IOSC_ERROR_FAIL_INTERNAL;
        }
    } else {
        return IOSC_ERROR_INVALID_FORMAT;
    }
    error = BSL_GetKeySize(&returnSize, sharedHandle);
    if (error != IOSC_ERROR_OK) {
        return IOSC_ERROR_FAIL_INTERNAL;
    }
    cslError = CSL_GenerateEccSharedKey(privatekey, publickey, sharedkey, returnSize);
    if (cslError != CSL_OK) {
        return IOSC_ERROR_FAIL_INTERNAL;
    }
    error = BSL_GetKeySize(&keySize, sharedHandle);
    if (error != IOSC_ERROR_OK) {
        return IOSC_ERROR_FAIL_INTERNAL;
    }
    error =  keyInsertData(sharedHandle, sharedkey, keySize);
    if (error != CSL_OK) {
        return IOSC_ERROR_FAIL_INTERNAL;
    }
    
    return error;
}


IOSCError 
BSL_GetData (IOSCDataHandle dataHandle, 
            u32 *value)
{
    IOSCError error = IOSC_ERROR_OK;
    u32 dataBuf;
    u8 *data = (u8 *)&dataBuf;
    u8 keyType = 0; 
    u8 keySubType = 0;
    
    getKeyTypeSubtype(dataHandle, &keyType, &keySubType);
    if (keyType != IOSC_DATA_TYPE) {
        return IOSC_ERROR_INVALID_OBJTYPE;
    }
    error =  keyGetMiscData(dataHandle, data);
    *value =  *((u32 *)data);

    if (error != IOSC_ERROR_OK) {
        return IOSC_ERROR_FAIL_INTERNAL;
    }
    return error;
}


IOSCError 
BSL_SetData (IOSCDataHandle dataHandle, 
            u32 value)
{
    IOSCError error = IOSC_ERROR_OK;
    u8 keyType = 0; 
    u8 keySubType = 0;
    u32 dataBuf;
    u8 *data = (u8 *)&dataBuf;

    CHECK_DEFAULT_RO_DATA_HANDLES(dataHandle);
    getKeyTypeSubtype(dataHandle, &keyType, &keySubType);
    if (keyType != IOSC_DATA_TYPE) {
        return IOSC_ERROR_INVALID_OBJTYPE;
    } 

    if (keySubType == IOSC_VERSION_SUBTYPE) {
        /* check that its greater */
        error =  keyGetMiscData(dataHandle, data);
        
        if (value < (u32) (*((u32*)data))) {
            return IOSC_ERROR_INVALID_VERSION;
        }

        /* increment copy in NVM */
	while (IOSCryptoGetVersion(dataHandle) < value){
	    error = IOSCryptoIncVersion(dataHandle);
	    if (error != IOSC_ERROR_OK) {
		return IOSC_ERROR_FAIL_INTERNAL;
	    }
	}
    }
    
    memcpy(data, &value, sizeof(u32));
    error =  keyInsertMiscData(dataHandle, data);
    if (error != IOSC_ERROR_OK) {
        return IOSC_ERROR_FAIL_INTERNAL;
    }
    
    return error;
}


IOSCError 
BSL_GetKeySize(u32 *keySize, 
               IOSCKeyHandle handle)
{
    u8 keyType, keySubType;
    IOSCError error;
    /* handle root key differently */
    if (handle == IOSC_ROOT_KEY_HANDLE) {
        *keySize = sizeof(CSLOSRsaPublicKey4096);
        return IOSC_ERROR_OK;
    }
    getKeyTypeSubtype(handle, &keyType, &keySubType);
    error = getSizeFromType(keyType, keySubType, keySize);
    
    if (error != IOSC_ERROR_OK) {
        return IOSC_ERROR_FAIL_INTERNAL;
    }
    return IOSC_ERROR_OK;
}


IOSCError 
BSL_GetSignatureSize(u32 *signSize, 
                    IOSCKeyHandle handle)
{
    IOSCError error = IOSC_ERROR_OK;
    u8 keyType, keySubType;
    if (handle == IOSC_ROOT_KEY_HANDLE) {
        *signSize = sizeof(CSLOSRsaSig4096); 
        return IOSC_ERROR_OK;
    }   
    getKeyTypeSubtype(handle, &keyType, &keySubType);

    switch (keyType) {
    case IOSC_SECRETKEY_TYPE:
        switch (keySubType) {
        case IOSC_MAC_SUBTYPE:
            *signSize = sizeof(CSLOSSha1Hash);
            break;
        case IOSC_ECC233_SUBTYPE:
            *signSize = sizeof(CSLOSEccSig);
            break;
        default:
            return IOSC_ERROR_INVALID;
        }
        break;
    case IOSC_PUBLICKEY_TYPE:
        switch (keySubType) {
        case IOSC_ECC233_SUBTYPE:
            *signSize = sizeof(CSLOSEccPublicKey);
            break;
        case IOSC_RSA2048_SUBTYPE:
            *signSize = sizeof(CSLOSRsaPublicKey2048);
            break;
        case IOSC_RSA4096_SUBTYPE:
            *signSize = sizeof(CSLOSRsaPublicKey4096);
            break;    
        default:
            return IOSC_ERROR_INVALID;
        }
        break;
    case IOSC_KEYPAIR_TYPE:
        switch (keySubType) {
        case IOSC_ECC233_SUBTYPE:
            *signSize = sizeof(CSLOSEccPublicKey);
            break;
        default:
            return IOSC_ERROR_INVALID;
        }
        break;
    default:
        return IOSC_ERROR_INVALID;
    }        
    return error;
}


IOSCError
BSL_GenerateRand (u8 *randBytes, 
		  u32 numBytes)
{
    return IOSCryptoGenerateRand(randBytes, numBytes);
}


/*
 * Encryption Functions: make sure input is multiple of 16 bytes  
 */

IOSCError 
BSL_Encrypt (IOSCSecretKeyHandle encryptHandle, 
	     u8 * ivData, 
	     u8 * inputData, 
	     u32 inputSize, 
	     u8 * outputData)
{
    IOSCError error = IOSC_ERROR_OK;
    u8 *key = NULL;
    u8 keyType, keySubType;
    u32 keySize;

    if (!ALIGNED(inputSize, CSL_AES_BLOCKSIZE_BYTES)) {
        error = IOSC_ERROR_INVALID_SIZE;
        goto out;
    }

    /* Check that key type is correct */
    getKeyTypeSubtype(encryptHandle, &keyType, &keySubType);
    if (keyType != IOSC_SECRETKEY_TYPE || keySubType != IOSC_ENC_SUBTYPE) {
        error = IOSC_ERROR_INVALID_OBJTYPE;
        goto out;
    }

    if (IOSC_IS_COMMON_KEY(encryptHandle)) {
        error = IOSCryptoEncryptAESCBCHwKey(
                    IOSC_COMMON_HANDLE_TO_KEYID(encryptHandle),
                    ivData, inputData, inputSize, outputData);
        goto out;
    }

    error = BSL_GetKeySize(&keySize, encryptHandle);
    if (error != IOSC_ERROR_OK) {
        error = IOSC_ERROR_FAIL_INTERNAL;
        goto out;
    }

    key = (u8 *) IOSCryptoAllocAligned(keySize, IOSC_AES_ADDR_ALIGN);
    if (key == NULL) {
        error = IOSC_ERROR_FAIL_INTERNAL;
        goto out;
    }

    error = keyGetData(encryptHandle, key, keySize);
    if (error != IOSC_ERROR_OK) {
        error = IOSC_ERROR_FAIL_INTERNAL;
        goto out;
    }

    error = IOSCryptoEncryptAESCBC(key, ivData, inputData, inputSize, outputData);

 out:
    if (key) { IOSCryptoFreeAligned(key, IOSC_AES_ADDR_ALIGN); }
    return error;
}


IOSCError 
BSL_Decrypt (IOSCSecretKeyHandle decryptHandle, 
	     u8 * ivData, 
	     u8 * inputData, 
	     u32 inputSize, 
	     u8 * outputData)
{
    IOSCError error = IOSC_ERROR_OK;
    u8 keyType, keySubType;
    u32 keySize;
    u8 *key = NULL;

    if (!ALIGNED(inputSize, CSL_AES_BLOCKSIZE_BYTES)) {
        error = IOSC_ERROR_INVALID_SIZE;
        goto out;
    }

    /* Check that key type is correct */
    getKeyTypeSubtype(decryptHandle, &keyType, &keySubType);
    if (keyType != IOSC_SECRETKEY_TYPE || keySubType != IOSC_ENC_SUBTYPE) {
        error = IOSC_ERROR_INVALID_OBJTYPE;
        goto out;
    }

    if (IOSC_IS_COMMON_KEY(decryptHandle)) {
        error = IOSCryptoDecryptAESCBCHwKey(
                    IOSC_COMMON_HANDLE_TO_KEYID(decryptHandle),
                    ivData, inputData, inputSize, outputData);
        goto out;
    }

    error = BSL_GetKeySize(&keySize, decryptHandle);
    if (error != IOSC_ERROR_OK) {
        error = IOSC_ERROR_FAIL_INTERNAL;
        goto out;
    }

    key = (u8 *) IOSCryptoAllocAligned(keySize, IOSC_AES_ADDR_ALIGN);
    if (key == NULL) {
        error = IOSC_ERROR_FAIL_INTERNAL;
        goto out;
    }

    error = keyGetData(decryptHandle, key, keySize);
    if (error != IOSC_ERROR_OK) {
        error = IOSC_ERROR_FAIL_INTERNAL;
        goto out;
    }

    error = IOSCryptoDecryptAESCBC(key, ivData, inputData, inputSize, outputData);

 out:
    if (key) { IOSCryptoFreeAligned(key, IOSC_AES_ADDR_ALIGN); }
    return error;
}

typedef struct {
    u8 context[MAX_RSA_MODULUS_LEN-SHA1_DIGESTSIZE-3];
    u8 randomSeed[20];
} BSL_PadMsg_big_locals;

IOSCError
BSL_PadMsg(u8* clearTextData, u32 clearTextDataSize,
           IOSCPadMsgType padMsgType,
           u32 keyLen,
           u8* paddedData_out, u32* paddedDataSize_in_out)
{
    IOSCError error = IOSC_ERROR_OK;
    PadOaepContext padOaepContext;
    u32 randomSeedLen = 20;
    int internalError;

    BSL_PadMsg_big_locals *big_locals;
#ifdef __KERNEL__
    big_locals = kmalloc(sizeof(*big_locals), GFP_KERNEL);
    if (!big_locals)
        return IOSC_ERROR_FAIL_ALLOC;
#else
    BSL_PadMsg_big_locals _big_locals;
    big_locals = &_big_locals;
#endif

    if(padMsgType != IOSC_PADMSG_OAEP){
        error = IOSC_ERROR_INVALID_FORMAT;
        goto error_label;
    }
    internalError = initPadOaepContext(&padOaepContext,
                                       keyLen,
                                       big_locals->context, MAX_RSA_MODULUS_LEN-SHA1_DIGESTSIZE-3);
    if( internalError !=0 || keyLen > MAX_RSA_MODULUS_LEN) {
        error = IOSC_ERROR_INVALID_SIZE;
        goto error_label;
    }
    /// get random seed
    internalError = BSL_GenerateRand(big_locals->randomSeed, randomSeedLen);
    if(internalError !=0 ){
        error = IOSC_ERROR_FAIL_INTERNAL;
        goto error_label;
    }

    //IOSCryptoComputeSHA1
    internalError = pad_oaep(&padOaepContext,
                             keyLen,
                             big_locals->randomSeed, randomSeedLen,
                             clearTextData, clearTextDataSize,
                             paddedData_out, paddedDataSize_in_out);
    if(internalError < 0 && internalError> -5) {
        error = IOSC_ERROR_INVALID_SIZE;
    } else if(internalError == -5) {
        error = IOSC_ERROR_INVALID;
    }
    goto end;

error_label:
    *paddedDataSize_in_out = 0;
 end:
#ifdef __KERNEL__
    kfree(big_locals);
#endif
    return error;
}

typedef struct {
    u8 internalExp[MAX_RSA_MODULUS_LEN];
} BSL_PubKeyEncrypt_big_locals;

IOSCError
BSL_PubKeyEncrypt(u8* paddedData, u32 paddedDataSize,
                  u8* modulus, u32 modulusSize,
                  u8* exponent, u32 exponentSize,
                  IOSCPubKeyEncryptType pubKeyEncryptType,
                  u8* encryptedData_out, u32* encryptedDataSize_in_out)
{
    IOSCError error = IOSC_ERROR_OK;
    int isRsa = 0;
    u32 keySize = 0;

    BSL_PubKeyEncrypt_big_locals *big_locals;
#ifdef __KERNEL__
    big_locals = kmalloc(sizeof(*big_locals), GFP_KERNEL);
    if (!big_locals)
        return IOSC_ERROR_FAIL_ALLOC;
#else
    BSL_PubKeyEncrypt_big_locals _big_locals;
    big_locals = &_big_locals;
#endif

    switch(pubKeyEncryptType){
    case IOSC_PUBKEY_ENCRYPT_RSA4096:
        isRsa = 1;
        keySize = 4096/8;
        break;
    case IOSC_PUBKEY_ENCRYPT_RSA2048:
        isRsa = 1;
        keySize = 2048/8;
        break;
    case IOSC_PUBKEY_ENCRYPT_RSA1024:
        isRsa = 1;
        keySize = 1024/8;
        break;
    case IOSC_PUBKEY_ENCRYPT_ECC512:
        isRsa = 0;
        error = IOSC_ERROR_INVALID_FORMAT;  // Currently unsupported
        goto error_label;
    case IOSC_PUBKEY_ENCRYPT_ECC233:
        isRsa = 0;
        error = IOSC_ERROR_INVALID_FORMAT;  // Currently unsupported
        goto error_label;
    default:
        error = IOSC_ERROR_INVALID_FORMAT;
        goto error_label;
    }
    if(isRsa) {
        if(paddedDataSize != keySize ||
           modulusSize != keySize ||
           exponentSize > keySize ||
           *encryptedDataSize_in_out < keySize ||
           keySize > MAX_RSA_MODULUS_LEN) {
            error = IOSC_ERROR_INVALID_SIZE;
            goto error_label;
        }
        memset(big_locals->internalExp, 0, keySize-exponentSize);
        memcpy(big_locals->internalExp+keySize-exponentSize, exponent, exponentSize);
        CSL_ComputeRsaSig(encryptedData_out, paddedData, modulus, big_locals->internalExp, keySize);
        *encryptedDataSize_in_out=keySize;
    }
    goto end;

 error_label:
    *encryptedDataSize_in_out = 0;
 end:
#ifdef __KERNEL__
    kfree(big_locals);
#endif
    return error;
}

typedef struct {
    CSLOSEccPrivateKey key;
    u8 randoms[IOSC_MAX_RAND_BYTES], privatepublic[CSL_KEY_STRING_LEN*3];
} generatePublicKeySign_big_locals;

static IOSCError 
generatePublicKeySign (u8 * inputData, 
                       u32 inputSize, 
                       IOSCSecretKeyHandle signerHandle, 
                       u8 * signData)
{
    IOSCError error = IOSC_ERROR_OK;
    IOSCError reterr = IOSC_ERROR_OK;
    u8 keyType, keySubType;
    u32 keySize = 0;

    generatePublicKeySign_big_locals *big_locals;
#ifdef __KERNEL__
    big_locals = kmalloc(sizeof(*big_locals), GFP_KERNEL);
    if (!big_locals)
        return IOSC_ERROR_FAIL_ALLOC;
#else
    generatePublicKeySign_big_locals _big_locals;
    big_locals = &_big_locals;
#endif
        
    getKeyTypeSubtype(signerHandle, &keyType, &keySubType);

    if ((keyType != IOSC_SECRETKEY_TYPE) &&
       (keyType != IOSC_KEYPAIR_TYPE)) {
        reterr = IOSC_ERROR_INVALID_OBJTYPE;
        goto end;
    }
    if (keySubType != IOSC_ECC233_SUBTYPE) {
        reterr = IOSC_ERROR_INVALID_OBJTYPE;
        goto end;
    }
    /* compute sig */
    if (inputSize != sizeof(CSLOSSha1Hash) && 
        inputSize != sizeof(CSLOSSha256Hash)) {
        reterr = IOSC_ERROR_INVALID_SIZE;
        goto end;
    }
    error = BSL_GetKeySize(&keySize, signerHandle);
    if(error != IOSC_ERROR_OK){
        reterr = IOSC_ERROR_INVALID_OBJTYPE;
        goto end;
    }
    /* get the key in both cases */
    if (keyType == IOSC_SECRETKEY_TYPE) {
        error = keyGetData(signerHandle, big_locals->key, keySize);
    } else {
        /* key pair type */
        error = keyGetData(signerHandle, big_locals->privatepublic, keySize);
        memcpy(big_locals->key, big_locals->privatepublic, sizeof(CSLOSEccPrivateKey));
    }

    error = BSL_GenerateRand(big_locals->randoms, sizeof(big_locals->randoms));
    
    if (error != IOSC_ERROR_OK) {
        reterr = IOSC_ERROR_FAIL_INTERNAL;
        goto end;
    }
    
    CSL_ComputeEccSig(inputData, inputSize, big_locals->key, signData, big_locals->randoms);

    reterr = error;

 end:
#ifdef __KERNEL__
    kfree(big_locals);
#endif
    return reterr;
}

IOSCError 
BSL_GeneratePublicKeySign (u8 * inputData, 
                           u32 inputSize, 
                           IOSCSecretKeyHandle signerHandle, 
                           u8 * signData)
{
    return generatePublicKeySign(inputData, inputSize, signerHandle, signData);
}

//-----------------------------
// TODO: temp debug function
static void dump_mem(void* buf, size_t size)
{
    int i;
    for (i = 0; i < size; i++) {
        printf("%02x ", (((unsigned char*)buf)[i]));
    }
}
//-----------------------------

typedef struct {
    CSLOSEccPublicKey eccpublickey;
    CSLOSRsaPublicKey4096 rsapublickey;
    u8 privatepublic[CSL_KEY_STRING_LEN*3]; /* one private, one public key */
} BSL_VerifyPublicKeySign_big_locals;

IOSCError 
BSL_VerifyPublicKeySign (u8 * inputData, 
                         u32 inputSize, 
                         IOSCPublicKeyHandle publicHandle, 
                         u8 * signData) 
{
    IOSCError error = IOSC_ERROR_OK;
    IOSCError reterr = IOSC_ERROR_OK;
    u8 keyType, keySubType;
    u32 keySize;
    CSL_error cslerror;
    CSLOSRsaExponent exponent;

    BSL_VerifyPublicKeySign_big_locals *big_locals;
#ifdef __KERNEL__
    big_locals = kmalloc(sizeof(*big_locals), GFP_KERNEL);
    if (!big_locals)
        return IOSC_ERROR_FAIL_ALLOC;
#else
    BSL_VerifyPublicKeySign_big_locals _big_locals;
    big_locals = &_big_locals;
#endif

    getKeyTypeSubtype(publicHandle, &keyType, &keySubType);
    /* compute sig */
    if (inputSize != sizeof(CSLOSSha1Hash) && 
        inputSize != sizeof(CSLOSSha256Hash)) {
        reterr = IOSC_ERROR_INVALID_SIZE;
        goto end;
    }
    if ((keyType != IOSC_PUBLICKEY_TYPE) &&
       (keyType != IOSC_KEYPAIR_TYPE)) {
        reterr = IOSC_ERROR_INVALID_OBJTYPE;
        goto end;
    }
    error = BSL_GetKeySize(&keySize, publicHandle);
    if (error != IOSC_ERROR_OK) {
        reterr = IOSC_ERROR_INVALID_OBJTYPE;
        goto end;
    }

    if (keyType == IOSC_PUBLICKEY_TYPE) {
        if (keySubType == IOSC_ECC233_SUBTYPE) {
            error = keyGetData(publicHandle, big_locals->eccpublickey, keySize);
            cslerror = CSL_VerifyEccSig(inputData, inputSize, big_locals->eccpublickey, signData);
            if (cslerror != CSL_OK) {
                reterr = IOSC_ERROR_FAIL_CHECKVALUE;
                goto end;
            }
        } else if (keySubType == IOSC_RSA2048_SUBTYPE) {
            error = keyGetData(publicHandle, big_locals->rsapublickey, keySize);
            if (error != IOSC_ERROR_OK) {
                reterr = IOSC_ERROR_FAIL_INTERNAL;
                goto end;
            }
            error = keyGetMiscData(publicHandle, exponent);
            if (error != IOSC_ERROR_OK) {
                reterr = IOSC_ERROR_FAIL_INTERNAL;
                goto end;
            }
            error = CSL_VerifyRsaSig(inputData, inputSize, big_locals->rsapublickey, signData,
                    exponent, getExpSize(exponent, sizeof(CSLOSRsaExponent)),
                    sizeof(CSLOSRsaPublicKey2048));
            if (error != CSL_OK) {
                //-----------------------------
                // TODO: temp debug logging
                printf("Fail at "__FILE__": %d\n\n", __LINE__);
                
                printf("digest: ");
                dump_mem(inputData, inputSize);
                printf("\n\n");
                
                printf("rsa public key: ");
                dump_mem(big_locals->rsapublickey, sizeof(big_locals->rsapublickey));
                printf("\n\n");
                
                printf("cert sig: ");
                dump_mem(signData, sizeof(CSLOSRsaPublicKey2048));
                printf("\n\n");
                
                printf("exponent: ");
                dump_mem(exponent, sizeof(exponent));
                printf("\n\n");
                //-----------------------------
                return IOSC_ERROR_FAIL_CHECKVALUE;
            }
        } else if (keySubType == IOSC_RSA4096_SUBTYPE) {
            u8 *key;
            u8 *exp;

            if (publicHandle == IOSC_ROOT_KEY_HANDLE) {
                key = IOSCryptoGetRootKey();
                exp = IOSCryptoGetRootKeyExp();
            } else {
                error = keyGetData(publicHandle, big_locals->rsapublickey, keySize);
                error = keyGetMiscData(publicHandle, exponent);
                if (error != IOSC_ERROR_OK) {
                    reterr = IOSC_ERROR_FAIL_INTERNAL;
                    goto end;
                }

                key = big_locals->rsapublickey;
                exp = exponent;
            }

            error = CSL_VerifyRsaSig(inputData, inputSize, key, signData, 
                        exp, getExpSize(exp, sizeof(CSLOSRsaExponent)),
                        sizeof(CSLOSRsaPublicKey4096));
            if (error != CSL_OK) {
                reterr = IOSC_ERROR_FAIL_CHECKVALUE;
                goto end;
            }
        } else {
            reterr = IOSC_ERROR_INVALID_OBJTYPE;
            goto end;
        }
    }
    else if (keyType == IOSC_KEYPAIR_TYPE) {
        if (keySubType == IOSC_ECC233_SUBTYPE) {
            error = keyGetData(publicHandle, big_locals->privatepublic, keySize);
            memcpy(big_locals->eccpublickey , big_locals->privatepublic + sizeof(CSLOSEccPrivateKey), sizeof(CSLOSEccPublicKey));
            cslerror = CSL_VerifyEccSig(inputData, inputSize, big_locals->eccpublickey, signData);
            if (cslerror != CSL_OK) {
                reterr = IOSC_ERROR_FAIL_CHECKVALUE;
                goto end;
            }
        }
    } else {
        reterr = IOSC_ERROR_INVALID_OBJTYPE;
        goto end;
    }    

    reterr = error;

 end:
#ifdef __KERNEL__
    kfree(big_locals);
#endif

    return reterr;
}


/*
 * Signature Functions 
 */
IOSCError
BSL_GenerateHash (IOSCHashContext context, 
                  u8* inputData, 
                  u32 inputSize, 
                  u32 chainingFlag, 
                  IOSCHash hashData)
{
    int hashType = chainingFlag & IOSC_HASH_TYPE_MASK;
    chainingFlag &= IOSC_HASH_OP_MASK;

    switch (hashType) {
    case IOSC_HASH_SHA1:
        return IOSCryptoComputeSHA1(context, inputData, inputSize, chainingFlag, hashData);

    case IOSC_HASH_SHA256:
        return IOSCryptoComputeSHA256(context, inputData, inputSize, chainingFlag, hashData);

    default:
        return IOSC_ERROR_INVALID;
    }

}


#define IPAD 0x36
#define OPAD 0x5c

IOSCError 
BSL_GenerateBlockMAC (u8 *context,
		      u8 *inputData, 
		      u32 inputSize, 
		      u8 *customData, 
		      u32 customDataSize, 
		      IOSCSecretKeyHandle signerHandle, 
		      u32 chainingFlag,
		      u8 *hashData)
{
    IOSCError error = IOSC_ERROR_OK;
    u8 keyType, keySubType;
    u32 keySize;
    u8 firstHash[CSL_SHA1_DIGESTSIZE];
    u8 *kxorpad = NULL;
    int i;

    if (chainingFlag == IOSC_HASH_MIDDLE) {
	return BSL_GenerateHash(context, inputData, inputSize, IOSC_HASH_MIDDLE, hashData);
    }

    kxorpad = (u8 *) IOSCryptoAllocAligned(CSL_SHA1_BLOCKSIZE, IOSC_SHA1_ADDR_ALIGN);

    if (kxorpad == NULL) {
	error = IOSC_ERROR_FAIL_INTERNAL;
	goto out;
    }

    // Need to do key expansion for FIRST and LAST
    getKeyTypeSubtype(signerHandle, &keyType, &keySubType);

    if ((keyType != IOSC_SECRETKEY_TYPE) ||
       (keySubType != IOSC_MAC_SUBTYPE)) {
        error = IOSC_ERROR_INVALID_OBJTYPE;
	goto out;
    }

    error = BSL_GetKeySize(&keySize, signerHandle);

    if (error || keySize != sizeof(IOSCHmacKey)) {
	error = IOSC_ERROR_INVALID_OBJTYPE;
	goto out;
    }

    error = keyGetData(signerHandle, kxorpad, keySize);
    memset(kxorpad + keySize, 0x0, CSL_SHA1_BLOCKSIZE - keySize);

    if (chainingFlag == IOSC_HASH_FIRST) {
    	error = BSL_GenerateHash(context, 0, 0, IOSC_HASH_FIRST, firstHash);
	if (error != IOSC_ERROR_OK) {
	    goto out;
	}
	
	for (i = 0; i < CSL_SHA1_BLOCKSIZE; i++) {
	    kxorpad[i] ^= IPAD;
	}
	
	error = BSL_GenerateHash(context, kxorpad, CSL_SHA1_BLOCKSIZE, IOSC_HASH_MIDDLE, firstHash);
	if (error != IOSC_ERROR_OK) {
	    goto out;
	}
	
	/* prepend custom data */
	if (customDataSize != 0) {
	    error = BSL_GenerateHash(context, customData, customDataSize, IOSC_HASH_MIDDLE, firstHash);
	    if (error != IOSC_ERROR_OK) {
		goto out;
	    }
	}

	/* XXX can input be supplied for IOSC_HASH_BEGIN */
	if (inputSize > 0) {
	    error = BSL_GenerateHash(context, inputData, inputSize, IOSC_HASH_MIDDLE, firstHash);
	}
    } else {
	SHA1Context sha1;

	error = BSL_GenerateHash(context, inputData, inputSize, IOSC_HASH_LAST, firstHash);
	
	if (error != IOSC_ERROR_OK) {
	    goto out;
	}

	// Second hash, do in software.
	SHA1Reset(&sha1);
	
	for (i = 0; i < CSL_SHA1_BLOCKSIZE; i++) {
	    kxorpad[i] ^= OPAD;
	}

	SHA1Input(&sha1, kxorpad, CSL_SHA1_BLOCKSIZE);
	SHA1Input(&sha1, firstHash, sizeof firstHash);
	SHA1Result(&sha1, hashData);
    }

 out:
    if (kxorpad) IOSCryptoFreeAligned(kxorpad, IOSC_SHA1_ADDR_ALIGN);
    return error;
}


/*
 * Convert from PUBKEY types to the corresponding ObjectSubTypes
 */
static inline IOSCObjectSubType
__ioscPubKeyToObjectSubType(u32 pubKeyType)
{
    if (pubKeyType == IOSC_PUBKEY_RSA4096) return IOSC_RSA4096_SUBTYPE;
    else if (pubKeyType == IOSC_PUBKEY_RSA2048) return IOSC_RSA2048_SUBTYPE;
    else if (pubKeyType == IOSC_PUBKEY_ECC) return IOSC_ECC233_SUBTYPE;
    else return (IOSCObjectSubType) -1;
}


/*
 * Certificate functions
 */

IOSCError 
BSL_ImportCertificate (IOSCGenericCert certData, 
                       IOSCPublicKeyHandle signerHandle, 
                       IOSCPublicKeyHandle publicKeyHandle)
{
    IOSCError error = IOSC_ERROR_OK;
    u8 keyType = 0; 
    u8 keySubType = 0;

    u8 *hashData;
    u32 hashLen;
    u8 *signature;
    u8 *pubKeyData;
    u8 *pubKeyExp;

    int hashType = IOSC_HASH_SHA1;  /* default hash type */
    int digestLen = sizeof(IOSCSha1Hash);
    IOSCHashContext context;
    IOSCSha256Hash hash;
    IOSCCertSigType sigType;
    IOSCCertPubKeyType pubKeyType;
#ifdef IOS
    sigType = (IOSCCertSigType) (__NTOHL(*(IOSCCertSigType *)certData));
#else
    sigType = (IOSCCertSigType) NTOHL(*(IOSCCertSigType *)certData); 
#endif
    getKeyTypeSubtype(signerHandle, &keyType, &keySubType);
    if (keyType != IOSC_PUBLICKEY_TYPE) {
        return IOSC_ERROR_INVALID_OBJTYPE;
    }

    getKeyTypeSubtype(publicKeyHandle, &keyType, &keySubType);
    if (keyType != IOSC_PUBLICKEY_TYPE) {
        return IOSC_ERROR_INVALID_OBJTYPE;
    }

#ifdef IOS

#define CERT_PUBKEY_TYPE(sigtype)				\
    (__NTOHL(((IOSCCertHeader *)((u8 *)certData + sizeof(sigtype)))->pubKeyType))
    
#else    

#define CERT_PUBKEY_TYPE(sigtype)				\
    ((IOSCCertPubKeyType)                       \
    NTOHL(((IOSCCertHeader *)((u8 *)certData + sizeof(sigtype)))->pubKeyType))

#endif

#define CERT_HASH_OFF(certtype)   ((size_t)((certtype *)0)->sig.issuer)
#define CERT_HASH_LEN(certtype)   (sizeof(certtype) - CERT_HASH_OFF(certtype))
#define CERT_HASH_START(certtype) (certData + CERT_HASH_OFF(certtype))

#define CERT_SIGNATURE(certtype) ((certtype *)certData)->sig.sig
#define CERT_PUBKEY(certtype) ((certtype *)certData)->pubKey
#define CERT_EXPONENT(certtype) ((certtype *)certData)->exponent;

    switch (sigType) {
    case IOSC_SIG_RSA4096_H256:
        hashType = IOSC_HASH_SHA256;
        digestLen = sizeof(IOSCSha256Hash);
        /* Fall through */
    case IOSC_SIG_RSA4096:
        pubKeyType = CERT_PUBKEY_TYPE(IOSCSigRsa4096);
        switch(pubKeyType) {
        case IOSC_PUBKEY_RSA2048:
            hashData   = CERT_HASH_START(IOSCRsa4096RsaCert);
            hashLen    = CERT_HASH_LEN  (IOSCRsa4096RsaCert);
            signature  = CERT_SIGNATURE (IOSCRsa4096RsaCert);
            pubKeyData = CERT_PUBKEY    (IOSCRsa4096RsaCert);
            pubKeyExp  = CERT_EXPONENT  (IOSCRsa4096RsaCert);
            break;
        default:
            return IOSC_ERROR_INVALID_FORMAT;
        }
        break;
    case IOSC_SIG_RSA2048_H256:
        hashType = IOSC_HASH_SHA256;
        digestLen = sizeof(IOSCSha256Hash);
        /* Fall through */
    case IOSC_SIG_RSA2048:
        pubKeyType = CERT_PUBKEY_TYPE(IOSCSigRsa2048);
        switch (pubKeyType) {
        case IOSC_PUBKEY_RSA2048:
            hashData   = CERT_HASH_START(IOSCRsa2048RsaCert);
            hashLen    = CERT_HASH_LEN  (IOSCRsa2048RsaCert);
            signature  = CERT_SIGNATURE (IOSCRsa2048RsaCert);
            pubKeyData = CERT_PUBKEY    (IOSCRsa2048RsaCert);
            pubKeyExp  = CERT_EXPONENT  (IOSCRsa2048RsaCert);
            break;
        case IOSC_PUBKEY_ECC:
            hashData   = CERT_HASH_START(IOSCRsa2048EccCert);
            hashLen    = CERT_HASH_LEN  (IOSCRsa2048EccCert);
            signature  = CERT_SIGNATURE (IOSCRsa2048EccCert);
            pubKeyData = CERT_PUBKEY    (IOSCRsa2048EccCert);
            pubKeyExp  = NULL;
            break;
        default:
            return IOSC_ERROR_INVALID_FORMAT;
        }
        break;
    case IOSC_SIG_ECC_H256:
        hashType = IOSC_HASH_SHA256;
        digestLen = sizeof(IOSCSha256Hash);
        /* Fall through */
    case IOSC_SIG_ECC:
        pubKeyType = CERT_PUBKEY_TYPE(IOSCSigEcc);
        switch (pubKeyType) {
        case IOSC_PUBKEY_ECC:
            hashData   = CERT_HASH_START(IOSCEccEccCert);
            hashLen    = CERT_HASH_LEN  (IOSCEccEccCert);
            signature  = CERT_SIGNATURE (IOSCEccEccCert);
            pubKeyData = CERT_PUBKEY    (IOSCEccEccCert);
            pubKeyExp  = NULL;
            break;
        default:
            return IOSC_ERROR_INVALID_FORMAT;
        }
        break;
    default:
        return IOSC_ERROR_INVALID_FORMAT;
    }

#undef CERT_PUBKEY_TYPE
#undef CERT_HASH_OFF
#undef CERT_HASH_LEN
#undef CERT_HASH_START
#undef CERT_SIGNATURE
#undef CERT_PUBKEY
#undef CERT_EXPONENT

    error = BSL_GenerateHash(context, 0, 0, IOSC_HASH_FIRST|hashType, hash);
    if (error != IOSC_ERROR_OK) {
        goto out;
    }

    error = BSL_GenerateHash(context, hashData, hashLen, IOSC_HASH_LAST|hashType, hash);
    if (error != IOSC_ERROR_OK) {
        goto out;
    }

    error = BSL_VerifyPublicKeySign(hash, digestLen, signerHandle, signature);	
    if (error != IOSC_ERROR_OK) {
        goto out;
    }

    /*
     * Check that the supplied key handle is the appropriate type based on the
     * certificate itself.  Unfortunately the enums are not the same, so we need
     * conversion to make this work.
     */
    if (keySubType != __ioscPubKeyToObjectSubType(pubKeyType)) {
        return IOSC_ERROR_INVALID_OBJTYPE;
    }

    error = BSL_ImportPublicKey(pubKeyData, pubKeyExp, publicKeyHandle);
 out:
    return error;
}

typedef struct {
    CSLOSEccPrivateKey inputkey, privatekey;
    u8 randoms[IOSC_MAX_RAND_BYTES];
    IOSCHashContext context;
    IOSCSha256Hash hash;
} BSL_GenerateCertificate_big_locals;

IOSCError 
BSL_GenerateCertificate(IOSCSecretKeyHandle privateHandle, 
                        IOSCCertName certname,
                        IOSCEccSignedCert *certificate)
{
    IOSCError error = IOSC_ERROR_OK;
    IOSCError reterr = IOSC_ERROR_OK;
    u8 keyType = 0; 
    u8 keySubType = 0;
    u32 keySize;
    IOSCEccEccCert *cert = (IOSCEccEccCert *)certificate;
    int i, j;

    BSL_GenerateCertificate_big_locals *big_locals;
#ifdef __KERNEL__
    big_locals = kmalloc(sizeof(*big_locals), GFP_KERNEL);
    if (!big_locals)
        return IOSC_ERROR_FAIL_ALLOC;
#else
    BSL_GenerateCertificate_big_locals _big_locals;
    big_locals = &_big_locals;
#endif

    getKeyTypeSubtype(privateHandle, &keyType, &keySubType);
    if ((keyType != IOSC_SECRETKEY_TYPE)&&(keyType != IOSC_KEYPAIR_TYPE)) {
        reterr = IOSC_ERROR_INVALID_OBJTYPE;
        goto end;
    }

    error = keyGetData(privateHandle, big_locals->inputkey, sizeof(CSLOSEccPrivateKey));
    if (error != IOSC_ERROR_OK) {
        reterr = IOSC_ERROR_FAIL_INTERNAL;
        goto end;
    }
    
    /* Use device cert as template */
    IOSCryptoGetDeviceCert(*certificate);
    
    /* Change issuer and name */
    i = 0;
    while(i < sizeof cert->sig.issuer && cert->sig.issuer[i] != '\0') {
        i++;
    }

    if (i < sizeof cert->sig.issuer) {
        cert->sig.issuer[i++] = '-';
    }

    j = 0;
    while(i < sizeof cert->sig.issuer && cert->head.name.deviceId[j] != '\0') {
        cert->sig.issuer[i++] = cert->head.name.deviceId[j++];
    }
    
    strncpy((char *)cert->head.name.deviceId, (char *)certname, sizeof cert->head.name.deviceId);
    cert->head.date = 0;

    /* New style signature using SHA-256 */
#ifdef IOS
    cert->sig.sigType = (IOSCCertSigType) __HTONL(IOSC_SIG_ECC_H256);
#else
    cert->sig.sigType = (IOSCCertSigType) HTONL(IOSC_SIG_ECC_H256);
#endif
    CSL_GenerateEccPublicKey(big_locals->inputkey, cert->pubKey);
    
    error = BSL_GenerateRand(big_locals->randoms, sizeof(big_locals->randoms));
    
    if (error != IOSC_ERROR_OK) {
        reterr = IOSC_ERROR_FAIL_INTERNAL;
        goto end;
    }

    error = BSL_GetKeySize(&keySize, IOSC_DEV_SIGNING_KEY_HANDLE);
    if (error != IOSC_ERROR_OK) {
        reterr = IOSC_ERROR_FAIL_INTERNAL;
        goto end;
    }

    if (keySize != sizeof(big_locals->privatekey)) {
        reterr = IOSC_ERROR_FAIL_INTERNAL;
        goto end;
    }

    error = keyGetData(IOSC_DEV_SIGNING_KEY_HANDLE, big_locals->privatekey, keySize);
    if (error != IOSC_ERROR_OK) {
        reterr = error;
        goto end;
    }

    error = BSL_GenerateHash(big_locals->context, NULL, 0, IOSC_SHA256_INIT, NULL);
    if (error != IOSC_ERROR_OK) {
        reterr = error;
        goto end;
    }

    error = BSL_GenerateHash(big_locals->context, cert->sig.issuer,
			     (u8 *)(cert + 1) - (u8 *)cert->sig.issuer,
			     IOSC_SHA256_FINAL, big_locals->hash);

    if (error != IOSC_ERROR_OK) {
        reterr = error;
        goto end;
    }

    CSL_ComputeEccSig(big_locals->hash, sizeof(big_locals->hash), big_locals->privatekey, cert->sig.sig, big_locals->randoms);

    reterr = error;

 end:
#ifdef __KERNEL__
    kfree(big_locals);
#endif
 
    return reterr;
}

IOSCError 
BSL_GetDeviceCertificate (IOSCEccSignedCert *certificate)
{
    return IOSCryptoGetDeviceCert(*certificate);
}

IOSCError
BSL_SetProtection (IOSCSecretKeyHandle privateHandle, 
                   u32 prot)
{
    u8 keyType = 0; 
    u8 keySubType = 0;
    getKeyTypeSubtype(privateHandle, &keyType, &keySubType);
    if  ((keyType != IOSC_SECRETKEY_TYPE) && (keyType != IOSC_KEYPAIR_TYPE)) {
        return IOSC_ERROR_INVALID_OBJTYPE;
    }
    return keyInsertProt(privateHandle, prot);
}


/* wrapper to set owner */
IOSCError
BSL_SetOwnership (u32 handle, 
                  u32 ownership)
{
    return keyInsertOwnership(handle, ownership);
}


/* wrapper to get prot */
IOSCError
BSL_GetProtection (IOSCSecretKeyHandle privateHandle, 
                   u32 *prot)
{
    u8 keyType = 0; 
    u8 keySubType = 0;
    getKeyTypeSubtype(privateHandle, &keyType, &keySubType);
    if ((keyType != IOSC_SECRETKEY_TYPE) && (keyType != IOSC_KEYPAIR_TYPE)) {
        return IOSC_ERROR_INVALID_OBJTYPE;
    }
    if (IOSC_IS_COMMON_KEY(privateHandle)) {
        *prot = BSL_PROT_NO_EXPORT;
        return IOSC_ERROR_OK;
    }
    return keyGetProt(privateHandle, prot);
}


/* wrapper to get owner */
IOSCError
BSL_GetOwnership (u32 handle, 
                  u32 *ownership)
{
    if (IOSC_IS_COMMON_KEY(handle)) {
        *ownership = 0;
    }
    return keyGetOwnership(handle, ownership);
}


/* Get characteristics of hash functions */
IOSCError
BSL_GetHashSize (u32 algorithm, 
                 u32 *hashSize,
                 u32 *contextSize,
                 u32 *alignment)
{
    IOSCError error = IOSC_ERROR_OK;

    switch (algorithm) {

    case IOSC_HASH_SHA1:
        *hashSize = sizeof(IOSCHash);
        *contextSize = IOSC_HASH_CONTEXT_SIZE;
        *alignment = IOSC_SHA1_ADDR_ALIGN;
        break;

    case IOSC_HASH_SHA256:
        *hashSize = sizeof(IOSCHash256);
        *contextSize = IOSC_HASH_CONTEXT_SIZE;
        *alignment = IOSC_SHA256_ADDR_ALIGN;
        break;

    default:
        error = IOSC_ERROR_INVALID;
        break;
    }

    return error;
}
