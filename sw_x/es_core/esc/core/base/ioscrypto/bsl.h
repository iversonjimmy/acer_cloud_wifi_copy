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

#ifndef __BSL_H__
#define __BSL_H__

/*
 * API definition for functions that implement the cryptographic security kernel
 */

#include <km_types.h>
#include <esc_iosctypes.h>

/* 
 * Not exposed
 */

#define IOSC_TYPE_SHIFT 4
#define IOSC_TYPE_MASK 0xf0
#define IOSC_SUBTYPE_MASK 0x0f
#define IOSC_MAXSECRET_BYTES 32

/*
 * Not exposed
 */
IOSCError BSL_KeyStoreInit(void);

/*
 * External Initialization
 */
IOSCError BSL_Initialize(void);

/*
 * Object functions
 */

IOSCError BSL_InstallObject(u32 handle,
			    IOSCObjectType type, 
			    IOSCObjectSubType subtype,
			    u8 *key, u32 keySize,
			    u8 *misc);

IOSCError BSL_CreateObject (u32 *handle, 
			    IOSCObjectType type, 
			    IOSCObjectSubType subtype);

IOSCError BSL_DeleteObject(u32 handle);


/*
 * Key functions
 */

IOSCError BSL_GenerateKey (IOSCKeyHandle handle);

IOSCError BSL_ExpandKey (IOSCSecretKeyHandle handle, 
                        u8 * expandedData, 
                        u32 sizeBytes, u8 * seed);

IOSCError BSL_ImportSecretKey (IOSCSecretKeyHandle importedHandle, 
                              IOSCSecretKeyHandle verifyHandle, 
                              IOSCSecretKeyHandle decryptHandle, 
                              IOSCSecretKeySecurity flag,
                              u8 *signbuffer, 
			      u8 *ivData,
                              u8 *keybuffer);

IOSCError BSL_ExportSecretKey (IOSCSecretKeyHandle exportedHandle,  
                              IOSCSecretKeyHandle signHandle, 
                              IOSCSecretKeyHandle encryptHandle,
                              IOSCSecretKeySecurity flag,
                              u8 *signbuffer,
			      u8 *ivData,
                              u8 *keybuffer);

IOSCError BSL_ImportPublicKey (u8 * publicKeyData, 
			      u8 * exponent,
                              IOSCPublicKeyHandle publicKeyHandle);

IOSCError BSL_ExportPublicKey (u8 * publicKeyData, 
			      u8 * exponent, 
                              IOSCPublicKeyHandle publickeyHandle);

IOSCError BSL_ComputeSharedKey (IOSCSecretKeyHandle privateHandle, 
                               IOSCPublicKeyHandle publicHandle, 
                               IOSCSecretKeyHandle sharedHandle);

/*
 * Signature Functions 
 */

IOSCError BSL_GenerateHash (IOSCHashContext context, 
			    u8 * inputData, 
			    u32 inputSize, 
			    u32 chainingFlag, 
			    IOSCHash hashData);

IOSCError BSL_GenerateBlockMAC (u8 * context,
				u8 * inputData, 
				u32 inputSize, 
				u8 *customData, 
				u32 customDataSize, 
				IOSCSecretKeyHandle signerHandle, 
				u32 chainingFlag,
				u8 * signData);

IOSCError BSL_GeneratePublicKeySign (u8 * inputData, 
                                    u32 inputSize, 
                                    IOSCSecretKeyHandle signerHandle, 
                                    u8 * signData);

IOSCError BSL_VerifyPublicKeySign (u8 * inputData, 
                                  u32 inputSize, 
                                  IOSCPublicKeyHandle signerHandle, 
                                  u8 * signData);

/*
 * Encryption Functions 
 */

IOSCError BSL_Encrypt (IOSCSecretKeyHandle encryptHandle, 
		       u8 * ivData, 
		       u8 * inputData, 
		       u32 inputSize, 
		       u8 * outputData);

IOSCError BSL_Decrypt (IOSCSecretKeyHandle decryptHandle, 
		       u8 * ivData, 
		       u8 * inputData, 
		       u32 inputSize, 
		       u8 * outputData);

IOSCError BSL_PadMsg(u8* clearTextData, u32 clearTextDataSize,
                     IOSCPadMsgType padMsgType,
                     u32 keyLen,
                     u8* paddedData_out, u32* paddedDataSize_in_out);

IOSCError BSL_PubKeyEncrypt(u8* paddedData, u32 paddedDataSize,
                            u8* modulus, u32 modulusSize,
                            u8* exponent, u32 exponentSize,
                            IOSCPubKeyEncryptType pubKeyEncryptType,
                            u8* encryptedData_out, u32* encryptedDataSize_in_out);

/*
 * Certificate functions
 */

IOSCError BSL_ImportCertificate (IOSCGenericCert certData, 
				 IOSCPublicKeyHandle signerHandle, 
				 IOSCPublicKeyHandle publicKeyHandle);

IOSCError BSL_GenerateCertificate (IOSCSecretKeyHandle privateHandle, 
				   IOSCCertName certname,
				   IOSCEccSignedCert *certificate);

IOSCError BSL_GetDeviceCertificate (IOSCEccSignedCert *certificate);


/*
 * Utility functions
 */

IOSCError BSL_GetData (IOSCDataHandle dataHandle, 
                      u32 *value);

IOSCError BSL_SetData (IOSCDataHandle dataHandle, 
                      u32 value);

IOSCError BSL_GetKeySize(u32 *keySize, 
                        IOSCKeyHandle handle);

IOSCError BSL_GetSignatureSize(u32 *signatureSize, 
                              IOSCSecretKeyHandle handle);

IOSCError BSL_GenerateRand (u8 *randBytes, 
                           u32 numBytes);

IOSCError BSL_SetProtection (IOSCSecretKeyHandle privateHandle, 
                             u32 prot);

IOSCError BSL_SetOwnership (u32 handle, 
                            u32 ownership);

IOSCError BSL_GetProtection (IOSCSecretKeyHandle privateHandle, 
                             u32 *prot);

IOSCError BSL_GetOwnership (u32 handle, 
                            u32 *ownership);

IOSCError BSL_GetHashSize (u32 algorithm, 
                           u32 *hashSize,
                           u32 *contextSize,
                           u32 *alignment);

#endif /* __BSL_H__ */
