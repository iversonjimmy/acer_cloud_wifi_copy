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
#include "cslsha.h"

#include "csl_impl.h"

#include <csl.h>

/* Set up key and IV and expand key */
int
CSL_ResetEncryptAes(CSL_AesContext *context, CSLOSAesKey key, CSLOSAesIv iv)
{
        
    if(aesMakeKey(&(context->keyInst), AES_DIR_ENCRYPT, CSL_AES_KEYSIZE_BYTES*8, key) != AES_TRUE){
        return CSL_AES_ERROR;
    }
    if(aesCipherInit(&(context->cipherInst), AES_MODE_CBC, iv) != AES_TRUE){
        return CSL_AES_ERROR;
    }
    
    return CSL_OK;
}
int
CSL_ResetDecryptAes(CSL_AesContext *context, CSLOSAesKey key, CSLOSAesIv iv)
{
        
    if(aesMakeKey(&(context->keyInst), AES_DIR_DECRYPT, CSL_AES_KEYSIZE_BYTES*8, key) != AES_TRUE){
        return CSL_AES_ERROR;
    }
    if(aesCipherInit(&(context->cipherInst), AES_MODE_CBC, iv) != AES_TRUE){
        return CSL_AES_ERROR;
    }
    
    return CSL_OK;
}


int
CSL_EncryptAes(CSL_AesContext *context, u8 *inputData, u32 inputSize, u8 *outputData)
{
    if(aesBlockEncrypt(&(context->cipherInst), &(context->keyInst), inputData, inputSize*8, outputData) != inputSize*8){
        return CSL_AES_ERROR;
    }
    return CSL_OK;
}


int
CSL_DecryptAes(CSL_AesContext *context, u8 *inputData, u32 inputSize, u8 *outputData)
{
    if(aesBlockDecrypt(&(context->cipherInst), &(context->keyInst), inputData, inputSize*8, outputData) != inputSize*8){
        return CSL_AES_ERROR;
    }
    return CSL_OK;
}
