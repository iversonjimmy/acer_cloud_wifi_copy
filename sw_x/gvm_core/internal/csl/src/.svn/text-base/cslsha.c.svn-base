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

#define shaCircularShift(bits, word) \
        (((word) << (bits)) | ((word) >> (32 - (bits))))

void shaPadMessage(CSL_ShaContext *);
void shaProcessMessageBlock(CSL_ShaContext *);

int 
CSL_ResetSha(CSL_ShaContext *context)
{
    if(!context){
        return CSL_SHA_ERROR;
    }
    
    context->lengthLo = 0;
    context->lengthHi = 0;
    context->messageBlockIndex = 0;
    context->intermediateHash[0] = 0x67452301;
    context->intermediateHash[1] = 0xefcdab89;
    context->intermediateHash[2] = 0x98badcfe;
    context->intermediateHash[3] = 0x10325476;
    context->intermediateHash[4] = 0xc3d2e1f0;

    return CSL_OK;
}

int 
CSL_ResultSha(CSL_ShaContext *context, u8 cslShaHash[CSL_SHA1_DIGESTSIZE])
{
    int i;
    if(!context || !cslShaHash){
        return CSL_SHA_ERROR;
    }
    
    shaPadMessage(context);
    memset(context->messageBlock, 0x0, CSL_SHA1_BLOCKSIZE);
    context->lengthLo = 0;
    context->lengthHi = 0;
    
    for(i=0; i< CSL_SHA1_DIGESTSIZE; ++i){
        cslShaHash[i] = context->intermediateHash[i>>2] >> 8 * (3-(i & 0x03));
    }
    return CSL_OK;
}

int
CSL_InputSha(CSL_ShaContext *context, const void *inputData, u32 inputSize)
{
    // We want this to be u8 for pointer arithmetic.
    const u8* message = (const u8*)inputData;
    if(!context || !message){
        return CSL_SHA_ERROR;
    }
    if (!inputSize){
        return CSL_OK;
    }
    
    while(inputSize--){
        context->messageBlock[context->messageBlockIndex++] = 
            (*message & 0xFF);
        context->lengthLo += 8;
        if(context->lengthLo == 0){
            context->lengthHi++;
            if(context->lengthHi == 0){
                return CSL_SHA_ERROR;
            }
        }
        if(context->messageBlockIndex == CSL_SHA1_BLOCKSIZE){
            shaProcessMessageBlock(context);
        }
        message++;
    }
    return CSL_OK;
}

void
shaProcessMessageBlock(CSL_ShaContext *context)
{
    const u32 K[] = { 0x5a827999, 0x6ed9eba1, 0x8f1bbcdc, 0xca62c1d6 };
    int t;
    u32 temp, W[80], A, B, C, D, E;
    
    /* Initialize */
    for(t=0; t<16; t++){
        W[t] = context->messageBlock[t*4] << 24;
        W[t] |= context->messageBlock[t*4 + 1] << 16;
        W[t] |= context->messageBlock[t*4 + 2] << 8;
        W[t] |= context->messageBlock[t*4 + 3];
    }

    for(t=16; t < 80; t++){
        W[t] =  shaCircularShift(1, W[t-3]^W[t-8]^W[t-14]^W[t-16]);
    }
    
    /* assign ABCDE: following notation in the rfc 3174 */
    A = context->intermediateHash[0];
    B = context->intermediateHash[1];
    C = context->intermediateHash[2];
    D = context->intermediateHash[3];
    E = context->intermediateHash[4];

    /* apply the function f in the spec*/
    for(t=0; t<20; t++){
        temp = shaCircularShift(5,A) + ((B&C) | ((~B)&D)) + E + W[t] + K[0];
        E = D;
        D = C;
        C = shaCircularShift(30,B);
        B = A;
        A = temp;
    }
    for(t=20; t<40; t++){
        temp = shaCircularShift(5,A) + (B^C^D) + E + W[t] + K[1];
        E = D;
        D = C;
        C = shaCircularShift(30, B);
        B = A;
        A = temp;
    }
    for(t=40; t<60; t++){
        temp = shaCircularShift(5,A) + ((B&C) | (B&D) | (C&D)) + 
            E + W[t] + K[2];
        E = D;
        D = C;
        C = shaCircularShift(30, B);
        B = A;
        A = temp;
    }
    for(t=60; t<80; t++){
        temp = shaCircularShift(5,A) + (B^C^D) + E + W[t] + K[3];
        E = D;
        D = C;
        C = shaCircularShift(30, B);
        B = A;
        A = temp;
    }
    context->intermediateHash[0] += A;
    context->intermediateHash[1] += B;
    context->intermediateHash[2] += C;
    context->intermediateHash[3] += D;
    context->intermediateHash[4] += E;

    context->messageBlockIndex = 0;

}

void
shaPadMessage(CSL_ShaContext *context)
{
    if(context->messageBlockIndex > 55){
        context->messageBlock[context->messageBlockIndex++] = 0x80;
        while(context->messageBlockIndex < 64){
            context->messageBlock[context->messageBlockIndex++] = 0;
        }
        shaProcessMessageBlock(context);
        while(context->messageBlockIndex < 56){
            context->messageBlock[context->messageBlockIndex++] = 0;
        }
    }
    else{
        context->messageBlock[context->messageBlockIndex++] = 0x80;
        while(context->messageBlockIndex < 56){
            context->messageBlock[context->messageBlockIndex++] = 0;
        }
    }
    context->messageBlock[56] = context->lengthHi >> 24;
    context->messageBlock[57] = context->lengthHi >> 16;
    context->messageBlock[58] = context->lengthHi >> 8;
    context->messageBlock[59] = context->lengthHi;
    context->messageBlock[60] = context->lengthLo >> 24;
    context->messageBlock[61] = context->lengthLo >> 16;
    context->messageBlock[62] = context->lengthLo >> 8;
    context->messageBlock[63] = context->lengthLo;
    
    shaProcessMessageBlock(context);
}

    
