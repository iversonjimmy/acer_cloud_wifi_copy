#ifndef __SHA1_H__
#define __SHA1_H__

#ifdef __cplusplus
extern "C" {
#endif

#if  !defined(SDK_TWL)
typedef unsigned char SHA1_BYTE;
typedef unsigned int SHA1_LONG;

#define SHA1_BLOCKSIZE		64
#define SHA1_DIGESTSIZE		20

typedef struct {
  SHA1_LONG digest[5];		/* message digest */
  SHA1_LONG count_lo, count_hi;	/* 64-bit bit count */
  SHA1_LONG data[16];			/* SHA data buffer */
  SHA1_LONG no_padding;
} SHA1Context;

extern int SHA1Reset(SHA1Context* ctx);
extern int SHA1SetNoPadding(SHA1Context *ctx);
extern int SHA1Input(SHA1Context* ctx, const SHA1_BYTE *buf, int len);
extern int SHA1Result(SHA1Context* ctx, SHA1_BYTE digest[SHA1_DIGESTSIZE]);
extern int SHA1LastPaddedBlock(SHA1Context* ctx, SHA1_BYTE block[SHA1_BLOCKSIZE]);


#else

/* TWL specific version */

#include <twl/os/common/systemCall.h>

#define SHA1_BYTE void

#define SHA1_BLOCKSIZE	SVC_SHA1_BLOCK_SIZE
#define SHA1_DIGESTSIZE SVC_SHA1_DIGEST_SIZE

#define SHA1Context SVCSHA1Context

#define SHA1Reset SVC_SHA1Init
#define SHA1Input SVC_SHA1Update
#define SHA1Result SVC_SHA1GetHash
    
#endif /* SDK_TWL */
#ifdef __cplusplus
}
#endif

#endif /* __SHA1_H__ */
