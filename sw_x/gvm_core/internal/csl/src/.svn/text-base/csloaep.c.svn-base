#include "csloaep.h"

#include "csl_impl.h"

#include "conversions.h"
#include "sha1.h"
#include <csltypes.h>


typedef struct {
    u8* dataBuf;
    u32 dataBufLen;  // Should be 4 bytes larger than the mgfSeed
} MGF1Context;

int initMGF1Context(MGF1Context* mgfContext,
                    u8* contextBuffer, u32 contextBufferLen, // Should be 4 bytes larger than mgfSeedLen
                    u32 mgfSeedLen);
int maskGenerationFunction1(MGF1Context* mgfContext,
                            u8* mgfSeed, u32 mgfSeedLen,
                            u8* mask_out, u32* maskLen_in_out);

int initMGF1Context(MGF1Context* mgfContext,
                    u8* contextBuffer, u32 contextBufferLen, // Should be 4 bytes larger than mgfSeedLen
                    u32 mgfSeedLen)
{
    if((contextBufferLen < mgfSeedLen + 4) ||
       (mgfSeedLen > mgfSeedLen+4)) // mgfSeedLen wrapped
        {
            mgfContext->dataBufLen = 0;
            return -1;
        }

    mgfContext->dataBuf = contextBuffer;
    mgfContext->dataBufLen = contextBufferLen;
    return 0;
}

//http://tools.ietf.org/html/rfc3447#appendix-B.2
//MGF1 is a Mask Generation Function based on a hash function.
//MGF1 (mgfSeed, maskLen)
//  Options:
//    Hash     hash function (hLen denotes the length in octets of the hash
//             function output)
//  Input:
//    mgfSeed  seed from which mask is generated, an octet string
//    maskLen  intended length in octets of the mask, at most 2^32 hLen
//  Output:
//    mask     mask, an octet string of length maskLen
//  Errors:
//     0 success
//    -1 uninitialized MGF1Context
//    -2 internal error
//   n/a "mask too long"  --u32 maskLen will never be larger than 2^32*hLen
int maskGenerationFunction1(MGF1Context* mgfContext,
                            u8* mgfSeed, u32 mgfSeedLen,
                            u8* mask_out, u32* maskLen_in_out)
{
    if(!mgfContext ||
       mgfContext->dataBufLen < mgfSeedLen + 4) {
        *maskLen_in_out = 0;
        return -1;
    }
    (void)memcpy(mgfContext->dataBuf, mgfSeed, mgfSeedLen);

    //1. If maskLen > 2^32 hLen, output "mask too long" and stop.
    //     Currently u32 maskLen will never be larger than 2^32 hLen.
    //2. Let T be the empty octet string.
    //     mask_out is T.
    //3. For counter from 0 to \ceil (maskLen / hLen) - 1, do the
    //   following:
    {
        u32 ceilValue = ((*maskLen_in_out - 1)/SHA1_DIGESTSIZE) + 1;
        u32 mask_outIndex = 0;
        SHA1_BYTE sha1digest[SHA1_DIGESTSIZE];
        u32 counter;
        for(counter = 0; (counter < ceilValue) && (mask_outIndex < *maskLen_in_out); counter++) 
            // (mask_outIndex < *maskLen_in_out) condition is redundant but harmless
            {
                //   a. Convert counter to an octet string C of length 4 octets (see
                //      Section 4.1):  C = I2OSP (counter, 4) .
                unsigned char C[4];
                I2OSP(C, 4, (bigint_digit*)&counter, 1);

                //   b. Concatenate the hash of the seed mgfSeed and C to the octet
                //      string T:      T = T || Hash(mgfSeed || C) .
                (void)memcpy(mgfContext->dataBuf+mgfSeedLen, C, 4);
                {
                    u32 appendLen;
                    int result;
                    SHA1Context sha1_ctx;
                    result = SHA1Reset(&sha1_ctx);
                    result |= SHA1Input(&sha1_ctx, mgfContext->dataBuf, mgfSeedLen+4);
                    result |= SHA1Result(&sha1_ctx, sha1digest);
                    if(result!=0){return -2;}
                    if(SHA1_DIGESTSIZE > *maskLen_in_out - mask_outIndex) {
                        appendLen = *maskLen_in_out - mask_outIndex;
                    }else{
                        appendLen = SHA1_DIGESTSIZE;
                    }
                    (void)memcpy(mask_out + mask_outIndex, sha1digest, appendLen);
                    mask_outIndex += appendLen;
                }
            }
    }
    //4. Output the leading maskLen octets of T as the octet string mask.
    //     mask_out
    return 0;
}

unsigned char lhash_sha1_empty[SHA1_DIGESTSIZE] = {0xda, 0x39, 0xa3, 0xee, 0x5e, 0x6b, 0x4b, 0x0d,
                                                   0x32, 0x55, 0xbf, 0xef, 0x95, 0x60, 0x18, 0x90,
                                                   0xaf, 0xd8, 0x07, 0x09};

int initPadOaepContext(PadOaepContext* padOaepContext,
                       u32 keyLen,
                       u8* dbBuf, u32 dbBufLen)
{
    u32 dbLen = (keyLen - SHA1_DIGESTSIZE - 1) + 4;  // +4 to reuse buffer for mgf function
    if(dbBufLen < dbLen) {
        padOaepContext->dbLen = 0;
        return -1;
    }
    padOaepContext->db = dbBuf;
    padOaepContext->dbLen = dbBufLen;
    return 0;
}

// OAEP padding
// http://tools.ietf.org/html/rfc3447#section-7.1
//7.1.1 Encryption operation
//   RSAES-OAEP-ENCRYPT ((n, e), M, L)
//     Options:
//     Hash     hash function (hLen denotes the length in octets of the hash
//              function output)
//     MGF      mask generation function
//     Input:
//       (n, e)   recipient's RSA public key (k denotes the length in octets
//                of the RSA modulus n)
//       M        message to be encrypted, an octet string of length mLen,
//                where mLen <= k - 2hLen - 2
//       L        optional label to be associated with the message; the
//                default value for L, if L is not provided, is the empty
//                string
//     Output:
//       C        ciphertext, an octet string of length k
//     Errors:
//        0   success
//       -1   "message too long"
//       -2   seedLen not SHA1_DIGESTSIZE
//       -3   PadOaepContext not initialized
//       -4   paddedMsg_in_out too small (should be equal to keyLen).
//       -5   Internal Error
//      n/a   "label too long"  --label is always null for our purposes 
//     Assumption: RSA public key (n, e) is valid
int pad_oaep(PadOaepContext* padOaepContext,
             u32 keyLen,
             u8* seed, u32 seedLen,
             u8* message, u32 messageLen,
             u8* paddedMsg_out, u32* paddedMsg_in_out)
{
    if(*paddedMsg_in_out < keyLen) {
        *paddedMsg_in_out = 0;
        return -4;
    }
    //1. Length checking:
    //   a. If the length of L is greater than the input limitation for the
    //      hash function (2^61 - 1 octets for SHA-1), output "label too
    //      long" and stop.
    //         Label will always be null for our purposes.  No need to check.
    //   b. If mLen > k - 2hLen - 2, output "message too long" and stop.
    if(messageLen > keyLen-(2*SHA1_DIGESTSIZE)-2) {
        *paddedMsg_in_out = 0;
        return -1;
    }

    //2. EME-OAEP encoding (see Figure 1 below):
    //   a. If the label L is not provided, let L be the empty string. Let
    //      lHash = Hash(L), an octet string of length hLen (see the note
    //      below).  // our Hash function is hardcoded sha1, so lHash is lhash_sha1_empty
    //   b. Generate an octet string PS consisting of k - mLen - 2hLen - 2
    //      zero octets.  The length of PS may be zero.
    {
        u32 dbIndex;
        u32 psLen = keyLen-messageLen-(2*SHA1_DIGESTSIZE)-2;
    
        //   c. Concatenate lHash, PS, a single octet with hexadecimal value
        //      0x01, and the message M to form a data block DB of length k -
        //      hLen - 1 octets as
        //         DB = lHash || PS || 0x01 || M.
        if(padOaepContext->dbLen < keyLen-SHA1_DIGESTSIZE-1){
            *paddedMsg_in_out = 0;
            return -3;
        }
        dbIndex = 0;
        (void)memcpy(padOaepContext->db, lhash_sha1_empty, SHA1_DIGESTSIZE);
        dbIndex += SHA1_DIGESTSIZE;
        (void)memset(padOaepContext->db + dbIndex, 0, psLen);
        dbIndex += psLen;
        padOaepContext->db[dbIndex] = 0x01;
        dbIndex++;
        (void)memcpy(padOaepContext->db + dbIndex, message, messageLen);
        dbIndex += messageLen;
    }

    //   d. Generate a random octet string seed of length hLen.
    if(seedLen != SHA1_DIGESTSIZE){
        *paddedMsg_in_out = 0;
        return -2;
    }

    {
        //   e. Let dbMask = MGF(seed, k - hLen - 1).
        u32 dbStartIndex = 1+SHA1_DIGESTSIZE;  // relative to the paddedMsg_out buffer
        u8* dbMask = paddedMsg_out+dbStartIndex;
        u32 dbMaskLen = keyLen-SHA1_DIGESTSIZE-1;
        {
            unsigned char buffer[SHA1_DIGESTSIZE+4];
            MGF1Context mgfContext;
            if(initMGF1Context(&mgfContext, buffer, SHA1_DIGESTSIZE+4, seedLen) != 0) {
                *paddedMsg_in_out = 0;
                return -5;
            }
            if(maskGenerationFunction1(&mgfContext, seed, seedLen, dbMask, &dbMaskLen) != 0) {
                *paddedMsg_in_out = 0;
                return -5;
            }
        }

        {
            //   f. Let maskedDB = DB \xor dbMask.
            u8* maskedDb = dbMask;
            u32 dbMaskIndex;
            for(dbMaskIndex=0; dbMaskIndex<dbMaskLen; dbMaskIndex++)
                {
                    maskedDb[dbMaskIndex] = padOaepContext->db[dbMaskIndex] ^ dbMask[dbMaskIndex];
                }
        
            //   g. Let seedMask = MGF(maskedDB, hLen).
            if(padOaepContext->dbLen < dbMaskLen+4) {
                *paddedMsg_in_out = 0;
                return -3;
            }
    
            {
                u8* seedMask = paddedMsg_out+1;
                u32 seedMaskLen = seedLen;
                {
                    u8* buffer = padOaepContext->db;
                    MGF1Context mgfContext;
                    if(initMGF1Context(&mgfContext, buffer, dbMaskLen+4, dbMaskLen) != 0){
                        *paddedMsg_in_out = 0;
                        return -4;
                    }
                    if(maskGenerationFunction1(&mgfContext, maskedDb, dbMaskLen, seedMask, &seedMaskLen) != 0){
                        *paddedMsg_in_out = 0;
                        return -4;
                    }
                }
                {
                    //   h. Let maskedSeed = seed \xor seedMask.
                    u8* maskedSeed = seedMask;
                    u32 seedMaskIndex;
                    for(seedMaskIndex=0; seedMaskIndex < seedMaskLen; seedMaskIndex++){
                        maskedSeed[seedMaskIndex] = seed[seedMaskIndex] ^ seedMask[seedMaskIndex];
                    }
                }
            }
        }
    }

    //   i. Concatenate a single octet with hexadecimal value 0x00,
    //      maskedSeed, and maskedDB to form an encoded message EM of
    //      length k octets as
    //         EM = 0x00 || maskedSeed || maskedDB.
    paddedMsg_out[0] = 0;  // maskedSeed and maskedDB already in place.
    return 0;
}

//http://tools.ietf.org/html/rfc3447#section-7.1.2
int unpad_oaep(PadOaepContext* padOaepContext,
               u32 keyLen,
               u8* paddedMsg, u32 paddedMsgLen,
               u8* msg_out, u32* msgLen_in_out)
{
    u32 paddedMsgIndex;
    u32 yLen;
    u8* maskedSeed;
    u32 maskedSeedLen;
    u8* maskedDB;
    u32 maskedDBLen;
    u8* seedMask;
    u32 seedMaskLen;
    u8* dbMask;

    if(*msgLen_in_out < keyLen || keyLen != paddedMsgLen) {
        (void)memset(msg_out, 0, *msgLen_in_out);
        *msgLen_in_out=0;
        return -1;
    }
    //3. EME-OAEP decoding:
    //   a. If the label L is not provided, let L be the empty string. Let
    //      lHash = Hash(L), an octet string of length hLen (see the note
    //      in Section 7.1.1).
    //          L is null for our purposes.
    //   b. Separate the encoded message EM into a single octet Y, an octet
    //      string maskedSeed of length hLen, and an octet string maskedDB
    //      of length k - hLen - 1 as
    //         EM = Y || maskedSeed || maskedDB.
    paddedMsgIndex = 0;
    yLen = 1;
    paddedMsgIndex += yLen;

    maskedSeed = paddedMsg+paddedMsgIndex;
    maskedSeedLen = SHA1_DIGESTSIZE;
    paddedMsgIndex += maskedSeedLen;

    maskedDB = paddedMsg+paddedMsgIndex;
    maskedDBLen = keyLen-SHA1_DIGESTSIZE-1;

    //   c. Let seedMask = MGF(maskedDB, hLen).
    seedMask = msg_out+1;
    seedMaskLen = SHA1_DIGESTSIZE;
    {
        u8* buffer = padOaepContext->db;
        MGF1Context mgfContext;
        if(initMGF1Context(&mgfContext, buffer, maskedDBLen+4, maskedDBLen) != 0){
            (void)memset(msg_out, 0, *msgLen_in_out);
            *msgLen_in_out = 0;
            return -4;
        }
        if(maskGenerationFunction1(&mgfContext, maskedDB, maskedDBLen, seedMask, &seedMaskLen) != 0){
            (void)memset(msg_out, 0, *msgLen_in_out);
            *msgLen_in_out = 0;
            return -4;
        }
    }

    {
        //   d. Let seed = maskedSeed \xor seedMask.
        u8* seed = seedMask;
        u32 seedMaskIndex;
        for(seedMaskIndex=0; seedMaskIndex<seedMaskLen; seedMaskIndex++)
            {
                seed[seedMaskIndex] = maskedSeed[seedMaskIndex] ^ seedMask[seedMaskIndex];
            }
    
        //   e. Let dbMask = MGF(seed, k - hLen - 1).
        dbMask = msg_out+1+SHA1_DIGESTSIZE;
        {
            unsigned char buffer[SHA1_DIGESTSIZE+4];
            MGF1Context mgfContext;
            if(initMGF1Context(&mgfContext, buffer, SHA1_DIGESTSIZE+4, seedMaskLen) != 0) {
                (void)memset(msg_out, 0, *msgLen_in_out);
                *msgLen_in_out = 0;
                return -4;
            }
            if(maskGenerationFunction1(&mgfContext, seed, seedMaskLen, dbMask, &maskedDBLen) != 0) {
                (void)memset(msg_out, 0, *msgLen_in_out);
                *msgLen_in_out = 0;
                return -4;
            }
        }
    }

    //   f. Let DB = maskedDB \xor dbMask.
    {
        u8* db = dbMask;
        u32 dbMaskIndex;
        for(dbMaskIndex=0; dbMaskIndex<maskedDBLen; dbMaskIndex++)
            {
                db[dbMaskIndex] = maskedDB[dbMaskIndex] ^ dbMask[dbMaskIndex];
            }
    
        //   g. Separate DB into an octet string lHash' of length hLen, a
        //      (possibly empty) padding string PS consisting of octets with
        //      hexadecimal value 0x00, and a message M as
        //         DB = lHash' || PS || 0x01 || M.
        //      If there is no octet with hexadecimal value 0x01 to separate PS
        //      from M, if lHash does not equal lHash', or if Y is nonzero,
        //      output "decryption error" and stop.  (See the note below.)
        if( memcmp(lhash_sha1_empty, db, SHA1_DIGESTSIZE) != 0 ) {
            (void)memset(msg_out, 0, *msgLen_in_out);
            *msgLen_in_out = 0;
            return -1;
        }
        {
            u8* ps = db+SHA1_DIGESTSIZE;
            u32 psIndex;
            for(psIndex=0; psIndex < maskedDBLen-SHA1_DIGESTSIZE; psIndex++)
                {
                    if(ps[psIndex] == 0) {
                        continue;
                    }else if(ps[psIndex] == 0x01) {
                        break;
                    }else{
                        (void)memset(msg_out, 0, *msgLen_in_out);
                        *msgLen_in_out = 0;
                        return -1;
                    }
                }
            if(psIndex == maskedDBLen-SHA1_DIGESTSIZE){ // 0x01 not found
                (void)memset(msg_out, 0, *msgLen_in_out);
                *msgLen_in_out = 0;
                return -1;
            }
            {
                u32 messageIndex = SHA1_DIGESTSIZE + psIndex + 1;
                u32 msgLen = maskedDBLen-messageIndex;
                // Move the message to the front of output buffer
                (void)memcpy(msg_out, db+messageIndex, msgLen);
                // Set the remaining buffer to 0
                (void)memset(msg_out+msgLen, 0, *msgLen_in_out-msgLen);
                *msgLen_in_out = msgLen;
            }
        }
    }
    return 0;
}
