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

#include <iostypes.h>
#include <iosc.h>
#include "bsl.h"

IOSCError 
IOSC_Initialize()
{   
    return BSL_Initialize();
}

IOSCError 
IOSC_CreateObject(u32 *handle, 
                  IOSCObjectType type, 
                  IOSCObjectSubType subtype)
{
    return BSL_CreateObject(handle, type, subtype);
}

IOSCError 
IOSC_DeleteObject(u32 handle)
{
    return BSL_DeleteObject(handle);
}

IOSCError 
IOSC_ImportSecretKey(IOSCSecretKeyHandle importedHandle, 
                     IOSCSecretKeyHandle verifyHandle, 
                     IOSCSecretKeyHandle decryptHandle, 
                     IOSCSecretKeySecurity flag,
                     u8 *signbuffer, 
                     u8 *ivData,
                     u8 *keybuffer)
{
    return BSL_ImportSecretKey(importedHandle, verifyHandle, 
                     decryptHandle, flag, signbuffer, ivData, keybuffer);
}

IOSCError 
IOSC_ExportSecretKey(IOSCSecretKeyHandle exportedHandle,  
                     IOSCSecretKeyHandle signHandle, 
                     IOSCSecretKeyHandle encryptHandle, 
                     IOSCSecretKeySecurity flag,
                     u8 *signbuffer, 
                     u8 *ivData,
                     u8 *keybuffer)
{
    return BSL_ExportSecretKey(exportedHandle,  signHandle, 
                     encryptHandle, flag, signbuffer, ivData, keybuffer);
}

IOSCError 
IOSC_ImportPublicKey(u8 *publicKeyData, 
                     u8 *exponent,
                     IOSCPublicKeyHandle publicKeyHandle)
{
    return BSL_ImportPublicKey(publicKeyData, exponent, publicKeyHandle);
}

IOSCError 
IOSC_ExportPublicKey(u8 *publicKeyData, 
                     u8 *exponent, 
                     IOSCPublicKeyHandle publicKeyHandle)
{
    return BSL_ExportPublicKey(publicKeyData, exponent, publicKeyHandle);
}

IOSCError 
IOSC_GenerateKey(IOSCKeyHandle handle)
{
    return BSL_GenerateKey(handle);
}

IOSCError 
IOSC_ComputeSharedKey(IOSCSecretKeyHandle privateHandle, 
                      IOSCPublicKeyHandle publicHandle, 
                      IOSCSecretKeyHandle sharedHandle)
{
    return BSL_ComputeSharedKey(privateHandle, publicHandle, sharedHandle);
}

IOSCError 
IOSC_GetData(IOSCDataHandle dataHandle, 
             u32 *value)
{
    return BSL_GetData(dataHandle, value);
}

IOSCError 
IOSC_SetData(IOSCDataHandle dataHandle, 
             u32 value)
{
    return BSL_SetData(dataHandle, value);
}

IOSCError 
IOSC_GetKeySize(u32 *keySize, 
                IOSCKeyHandle handle)
{
    return BSL_GetKeySize(keySize, handle);
}

IOSCError 
IOSC_GetSignatureSize(u32 *signSize, 
                      IOSCKeyHandle handle)
{
    return BSL_GetSignatureSize(signSize, handle);
}

IOSCError 
IOSC_GenerateRand(u8 *randBytes, 
                  u32 numBytes)
{
    return BSL_GenerateRand(randBytes, numBytes);
}

IOSCError 
IOSC_GenerateHash(IOSCHashContext context, 
          u8 *inputData, 
          u32 inputSize, 
          u32 chainingFlag, 
          IOSCHash hashData)
{
    return BSL_GenerateHash(context, inputData, inputSize, chainingFlag, hashData);
}

IOSCError 
IOSC_Encrypt(IOSCSecretKeyHandle encryptHandle, 
             u8 *ivData, 
             u8 *inputData, 
             u32 inputSize, 
             u8 *outputData)
{
    return BSL_Encrypt(encryptHandle, ivData, inputData, inputSize, outputData);
}

IOSCError 
IOSC_Decrypt(IOSCSecretKeyHandle decryptHandle, 
             u8 *ivData, 
             u8 *inputData, 
             u32 inputSize, 
             u8 *outputData)
{
    return BSL_Decrypt(decryptHandle, ivData, inputData, inputSize, outputData);
}

IOSError
IOSC_PadMsg(u8* clearTextData, u32 clearTextDataSize,
            IOSCPadMsgType padMsgType,
            u32 keyLen,
            u8* paddedData_out, u32* paddedDataSize_in_out)
{
    return BSL_PadMsg(clearTextData, clearTextDataSize,
                      padMsgType,
                      keyLen,
                      paddedData_out, paddedDataSize_in_out);
}

IOSError
IOSC_PubKeyEncrypt(u8* paddedData, u32 paddedDataSize,
                   u8* modulus, u32 modulusSize,
                   u8* exponent, u32 exponentSize,
                   IOSCPubKeyEncryptType pubKeyEncryptType,
                   u8* encryptedData_out, u32* encryptedDataSize_in_out)
{
    return BSL_PubKeyEncrypt(paddedData, paddedDataSize, 
                             modulus, modulusSize,
                             exponent, exponentSize,
                             pubKeyEncryptType,
                             encryptedData_out, encryptedDataSize_in_out);
}

IOSCError 
IOSC_GeneratePublicKeySign(u8 *inputData, 
                           u32 inputSize, 
                           IOSCSecretKeyHandle signerHandle, 
                           u8 *signData)
{
    return BSL_GeneratePublicKeySign(inputData, inputSize, signerHandle, signData);
}

IOSCError 
IOSC_VerifyPublicKeySign(u8 *inputData, 
                         u32 inputSize, 
                         IOSCPublicKeyHandle publicHandle, 
                         u8 *signData)
{
    return BSL_VerifyPublicKeySign(inputData, inputSize, publicHandle, signData);
}

IOSCError 
IOSC_GenerateBlockMAC(u8 *context,
                      u8 *inputData, 
                      u32 inputSize, 
                      u8 *customData, 
                      u32 customDataSize, 
                      IOSCSecretKeyHandle signerHandle, 
                      u32 chainingFlag,
                      u8 *signData)
{
    return  BSL_GenerateBlockMAC(context, inputData, inputSize, customData, customDataSize, signerHandle, chainingFlag, signData);
}

IOSCError 
IOSC_ImportCertificate(IOSCGenericCert certData, 
                       IOSCPublicKeyHandle signerHandle, 
                       IOSCPublicKeyHandle publicKeyHandle)
{
    return BSL_ImportCertificate(certData, signerHandle, publicKeyHandle);
}

IOSCError 
IOSC_GenerateCertificate(IOSCSecretKeyHandle privateHandle, 
                         IOSCCertName certname,
                         IOSCEccSignedCert *certificate)
{
    return BSL_GenerateCertificate(privateHandle, certname, certificate);
}

IOSCError 
IOSC_GetDeviceCertificate(IOSCEccSignedCert *certificate)
{
    return BSL_GetDeviceCertificate(certificate);
}

IOSCError 
IOSC_GetHashSize(u32 algorithm,
                 u32 *digestSize,
                 u32 *contextSize,
                 u32 *alignment)
{
    return BSL_GetHashSize(algorithm, digestSize, contextSize, alignment);
}
