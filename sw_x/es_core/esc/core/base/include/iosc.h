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
#ifndef __IOSC_H__
#define __IOSC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <esc_iosctypes.h>

#if !defined(ASSEMBLER)

IOSCError IOSC_Initialize(void);
IOSCError IOSC_CreateObject(u32 * handle, IOSCObjectType type,
			    IOSCObjectSubType subtype);
IOSCError IOSC_DeleteObject(u32 handle);
IOSCError IOSC_ImportSecretKey(IOSCSecretKeyHandle importedHandle,
			       IOSCSecretKeyHandle verifyHandle,
			       IOSCSecretKeyHandle decryptHandle,
			       IOSCSecretKeySecurity flag, u8 * signbuffer,
			       u8 * ivData, u8 * keybuffer);
IOSCError IOSC_ExportSecretKey(IOSCSecretKeyHandle exportedHandle,
			       IOSCSecretKeyHandle signHandle,
			       IOSCSecretKeyHandle encryptHandle,
			       IOSCSecretKeySecurity flag, u8 * signbuffer,
			       u8 * ivData, u8 * keybuffer);
IOSCError IOSC_ImportPublicKey(u8 * publicKeyData, u8 * exponent,
			       IOSCPublicKeyHandle publicKeyHandle);
IOSCError IOSC_ExportPublicKey(u8 * publicKeyData, u8 * exponent,
			       IOSCPublicKeyHandle publicKeyHandle);
IOSCError IOSC_ComputeSharedKey(IOSCSecretKeyHandle privateHandle,
				IOSCPublicKeyHandle publicHandle,
				IOSCSecretKeyHandle sharedHandle);
IOSCError IOSC_SetData(IOSCDataHandle dataHandle, u32 value);
IOSCError IOSC_GetData(IOSCDataHandle dataHandle, u32 * value);
IOSCError IOSC_GetKeySize(u32 * keySize, IOSCKeyHandle handle);
IOSCError IOSC_GetHashSize(u32 algorithm, u32 *digestSize, 
                u32 *contextSize, u32 *alignment);
IOSCError IOSC_GetSignatureSize(u32 * signSize, IOSCKeyHandle handle);
IOSCError IOSC_GenerateHash(u8 * context, u8 * inputData, u32 inputSize,
			    u32 chainingFlag, u8 * hashData);
IOSCError IOSC_Encrypt(IOSCSecretKeyHandle encryptHandle, u8 * ivData,
		       u8 * inputData, u32 inputSize, u8 * outputData);
IOSCError IOSC_Decrypt(IOSCSecretKeyHandle decryptHandle, u8 * ivData,
		       u8 * inputData, u32 inputSize, u8 * outputData);
IOSCError IOSC_PadMsg(u8* clearTextData, u32 clearTextDataSize,
                      IOSCPadMsgType padMsgType,
                      u32 keyLen,
                      u8* paddedData_out, u32* paddedDataSize_in_out);
IOSCError IOSC_PubKeyEncrypt(u8* paddedData, u32 paddedDataSize,
                             u8* modulus, u32 modulusSize,
                             u8* exponent, u32 exponentSize,
                             IOSCPubKeyEncryptType pubKeyEncryptType,
                             u8* encryptedData_out, u32* encryptedDataSize_in_out);
IOSCError IOSC_VerifyPublicKeySign(u8 * inputData, u32 inputSize,
				   IOSCPublicKeyHandle publicHandle,
				   u8 * signData);
IOSCError IOSC_GenerateBlockMAC(u8 * context, u8 * inputData, u32 inputSize,
				u8 * customData, u32 customDataSize,
				IOSCSecretKeyHandle signerHandle,
				u32 chainingFlag, u8 * signData);
IOSCError IOSC_ImportCertificate(u8 * certData,
				 IOSCPublicKeyHandle signerHandle,
				 IOSCPublicKeyHandle publicKeyHandle);
IOSCError IOSC_GetDeviceCertificate(IOSCEccSignedCert * certificate);
IOSCError IOSC_SetOwnership(u32 handle, u32 users);
IOSCError IOSC_GetOwnership(u32 handle, u32 * users);
IOSCError IOSC_GenerateRand(u8 * randBytes, u32 numBytes);
IOSCError IOSC_GenerateKey(IOSCKeyHandle handle);
IOSCError IOSC_GeneratePublicKeySign(u8 * inputData, u32 inputSize,
				     IOSCSecretKeyHandle signerHandle,
				     u8 * signData);
IOSCError IOSC_GenerateCertificate(IOSCSecretKeyHandle privateHandle,
				   IOSCCertName certname,
				   IOSCEccSignedCert * certificate);

#endif /* !ASSEMBLER */

#ifdef __cplusplus
}
#endif //__cplusplus

#endif /* __IOSC_H__ */
