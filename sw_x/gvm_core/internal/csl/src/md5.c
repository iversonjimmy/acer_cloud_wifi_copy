/*
 * iGware wrapper to underlying MD5 implementations.
 * The default is to use LibTomCrypt's MD5.
 */

#include "md5.h"
//#include <km_types.h>

int
MD5Reset(MD5Context* ctx)
{
    return md5_init(ctx);
}

int
MD5Input(MD5Context* ctx, const void *buf, int len)
{
    return md5_process(ctx, (const u8*)buf, len);
}

int
MD5Result(MD5Context* ctx, u8 digest[MD5_DIGESTSIZE])
{
    return md5_done(ctx, digest);
}
