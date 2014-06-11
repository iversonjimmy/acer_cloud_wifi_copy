/*
 * iGware wrapper to underlying SHA256 implementations.
 * The default is to use LibTomCrypt's SHA256.
 * If USE_KERNEL_SHA256 is defined, Linux kernel's SHA256 will be used.
 */

#include "sha256.h"
#include <km_types.h>

#if defined(__KERNEL__) && defined(USE_KERNEL_SHA256)
#include <linux/vmalloc.h>
#include <linux/scatterlist.h>
#include <linux/err.h>
#endif

int
SHA256Reset(SHA256Context* ctx)
{
#if defined(__KERNEL__) && defined(USE_KERNEL_SHA256)
    int rc;
    ctx->tfm = crypto_alloc_hash("sha256", 0, CRYPTO_ALG_ASYNC);
    if (IS_ERR(ctx->tfm))
        return PTR_ERR(ctx->tfm);
    ctx->flags = 0;
    rc = crypto_hash_init(ctx);
    if (rc)
        crypto_free_hash(ctx->tfm);
    return rc;
#else
    return sha256_init(ctx);
#endif
}

int
SHA256Input(SHA256Context* ctx, const u8 *buf, int len)
{
#if defined(__KERNEL__) && defined(USE_KERNEL_SHA256)
    int rc;
    struct scatterlist *sglist, *sg;
    unsigned int off = offset_in_page(buf);
    unsigned int nr = (len + off + PAGE_SIZE - 1) / PAGE_SIZE;
    unsigned int mem_size = sizeof(struct scatterlist) * nr;
    int remain = len;
    int i, cur_len;

    if (mem_size > PAGE_SIZE)
        sglist = vmalloc(mem_size);
    else
        sglist = kmalloc(mem_size, GFP_KERNEL);
    sg_init_table(sglist, nr);
    for_each_sg(sglist, sg, nr, i) {
        cur_len = remain;
        if (cur_len > PAGE_SIZE - off)
            cur_len = PAGE_SIZE - off;
        sg_set_buf(sg, buf, cur_len);
        buf += cur_len;
        remain -= cur_len;
        off = 0;
    }

    rc = crypto_hash_update(ctx, sglist, len);
    if (rc)
        crypto_free_hash(ctx->tfm);
    if (mem_size > PAGE_SIZE)
        vfree(sglist);
    else
        kfree(sglist);

    return rc;
#else
    return sha256_process(ctx, buf, len);
#endif
}

int
SHA256Result(SHA256Context* ctx, u8 digest[SHA256_DIGESTSIZE])
{
#if defined(__KERNEL__) && defined(USE_KERNEL_SHA256)
    int rc;
    rc = crypto_hash_final(ctx, digest);
    crypto_free_hash(ctx->tfm);
    return rc;
#else
    return sha256_done(ctx, digest);
#endif
}
