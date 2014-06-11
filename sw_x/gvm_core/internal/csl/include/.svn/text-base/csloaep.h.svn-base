#ifndef __CSLOAEP_H__
#define __CSLOAEP_H__

#include <km_types.h>

#if defined(__cplusplus)
extern "C" {
#endif


typedef struct {
    u8* db;
    u32 dbLen;
} PadOaepContext;

int initPadOaepContext(PadOaepContext* padOaepContext,
                       u32 keyLen,
                       u8* dbBuf, u32 dbBufLen);

int pad_oaep(PadOaepContext* padOaepContext,
             u32 keyLen,
             u8* seed, u32 seedLen,
             u8* message, u32 messageLen,
             u8* paddedMsg_out, u32* paddedMsg_in_out);

int unpad_oaep(PadOaepContext* padOaepContext,
               u32 keyLen,
               u8* paddedMsg, u32 paddedMsgLen,
               u8* msg_out, u32* msgLen_in_out);


#if defined(__cplusplus)
}
#endif

#endif /* __CSLOAEP_H__ */
