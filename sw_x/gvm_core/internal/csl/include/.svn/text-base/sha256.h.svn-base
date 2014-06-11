#ifndef __SHA256_H__
#define __SHA256_H__

#include <km_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SHA256_BLOCKSIZE    64
#define SHA256_DIGESTSIZE   32

#if defined(__KERNEL__) && defined(USE_KERNEL_SHA256)

#include <linux/crypto.h>

typedef struct hash_desc SHA256Context;

#else

#include "tomcrypt.h"

typedef hash_state SHA256Context;

#endif

extern int SHA256Reset(SHA256Context* ctx);
extern int SHA256Input(SHA256Context* ctx, const u8 *buf, int len);
extern int SHA256Result(SHA256Context* ctx, u8 digest[SHA256_DIGESTSIZE]);

#ifdef __cplusplus
}
#endif

#endif /* __SHA256_H__ */
