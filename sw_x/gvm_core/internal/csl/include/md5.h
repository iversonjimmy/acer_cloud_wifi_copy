#ifndef __MD5_H__
#define __MD5_H__

#include <km_types.h>

#ifdef __cplusplus
extern "C" {
#endif

//#define MD5_BLOCKSIZE    64
#define MD5_DIGESTSIZE   16

#include "tomcrypt.h"

typedef hash_state MD5Context;

extern int MD5Reset(MD5Context* ctx);
extern int MD5Input(MD5Context* ctx, const void *buf, int len);
extern int MD5Result(MD5Context* ctx, u8 digest[MD5_DIGESTSIZE]);

#ifdef __cplusplus
}
#endif

#endif /* __MD5_H__ */
