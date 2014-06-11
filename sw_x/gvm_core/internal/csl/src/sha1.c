#include "sha1.h"

#if !defined(SDK_TWL)
/* do not use these definitions for TWL */


/* NIST Secure Hash Algorithm */
/* heavily modified by Uwe Hollerbach uh@alumni.caltech edu */
/* from Peter C. Gutmann's implementation as found in */
/* Applied Cryptography by Bruce Schneier */

/* NIST's proposed modification to SHA of 7/11/94 may be */
/* activated by defining USE_MODIFIED_SHA */
#define USE_MODIFIED_SHA
/* #define UNROLL_LOOPS */
#if defined(__i386__) || defined(HOST_IS_LITTLE_ENDIAN)
#define SHA1_LITTLE_ENDIAN
#endif

#include "csl_impl.h"

/* SHA f()-functions */

#define f1(x,y,z)	((x & y) | (~x & z))
#define f2(x,y,z)	(x ^ y ^ z)
#define f3(x,y,z)	((x & y) | (x & z) | (y & z))
#define f4(x,y,z)	(x ^ y ^ z)

/* SHA constants */

#define CONST1		0x5a827999L
#define CONST2		0x6ed9eba1L
#define CONST3		0x8f1bbcdcL
#define CONST4		0xca62c1d6L

/* 32-bit rotate */

#define ROT32(x,n)	((x << n) | (x >> (32 - n)))

#define FUNC(n,i)						\
    temp = ROT32(A,5) + f##n(B,C,D) + E + big_locals->W[i] + CONST##n;	\
    E = D; D = C; C = ROT32(B,30); B = A; A = temp

/* do SHA transformation */

typedef struct {
    SHA1_LONG W[80];
} SHA1Transform_big_locals;

static void SHA1Transform(SHA1Context *ctx)
{
    int i;
    SHA1_LONG temp, A, B, C, D, E;
#ifndef __KERNEL__
    SHA1Transform_big_locals _big_locals;
#endif
    SHA1Transform_big_locals *big_locals;
#ifdef __KERNEL__
    big_locals = kmalloc(sizeof(SHA1Transform_big_locals), GFP_KERNEL);
    if (big_locals == NULL)
        return;  // currently has no way to tell caller of error
#else
    big_locals = &_big_locals;
#endif

    for (i = 0; i < 16; ++i) {
	big_locals->W[i] = ctx->data[i];
    }
    for (i = 16; i < 80; ++i) {
	big_locals->W[i] = big_locals->W[i-3] ^ big_locals->W[i-8] ^ big_locals->W[i-14] ^ big_locals->W[i-16];
#ifdef USE_MODIFIED_SHA
	big_locals->W[i] = ROT32(big_locals->W[i], 1);
#endif /* USE_MODIFIED_SHA */
    }
    A = ctx->digest[0];
    B = ctx->digest[1];
    C = ctx->digest[2];
    D = ctx->digest[3];
    E = ctx->digest[4];
#ifdef UNROLL_LOOPS
    FUNC(1, 0);  FUNC(1, 1);  FUNC(1, 2);  FUNC(1, 3);  FUNC(1, 4);
    FUNC(1, 5);  FUNC(1, 6);  FUNC(1, 7);  FUNC(1, 8);  FUNC(1, 9);
    FUNC(1,10);  FUNC(1,11);  FUNC(1,12);  FUNC(1,13);  FUNC(1,14);
    FUNC(1,15);  FUNC(1,16);  FUNC(1,17);  FUNC(1,18);  FUNC(1,19);

    FUNC(2,20);  FUNC(2,21);  FUNC(2,22);  FUNC(2,23);  FUNC(2,24);
    FUNC(2,25);  FUNC(2,26);  FUNC(2,27);  FUNC(2,28);  FUNC(2,29);
    FUNC(2,30);  FUNC(2,31);  FUNC(2,32);  FUNC(2,33);  FUNC(2,34);
    FUNC(2,35);  FUNC(2,36);  FUNC(2,37);  FUNC(2,38);  FUNC(2,39);

    FUNC(3,40);  FUNC(3,41);  FUNC(3,42);  FUNC(3,43);  FUNC(3,44);
    FUNC(3,45);  FUNC(3,46);  FUNC(3,47);  FUNC(3,48);  FUNC(3,49);
    FUNC(3,50);  FUNC(3,51);  FUNC(3,52);  FUNC(3,53);  FUNC(3,54);
    FUNC(3,55);  FUNC(3,56);  FUNC(3,57);  FUNC(3,58);  FUNC(3,59);

    FUNC(4,60);  FUNC(4,61);  FUNC(4,62);  FUNC(4,63);  FUNC(4,64);
    FUNC(4,65);  FUNC(4,66);  FUNC(4,67);  FUNC(4,68);  FUNC(4,69);
    FUNC(4,70);  FUNC(4,71);  FUNC(4,72);  FUNC(4,73);  FUNC(4,74);
    FUNC(4,75);  FUNC(4,76);  FUNC(4,77);  FUNC(4,78);  FUNC(4,79);
#else /* !UNROLL_LOOPS */
    for (i = 0; i < 20; ++i) {
	FUNC(1,i);
    }
    for (i = 20; i < 40; ++i) {
	FUNC(2,i);
    }
    for (i = 40; i < 60; ++i) {
	FUNC(3,i);
    }
    for (i = 60; i < 80; ++i) {
	FUNC(4,i);
    }
#endif /* !UNROLL_LOOPS */
    ctx->digest[0] += A;
    ctx->digest[1] += B;
    ctx->digest[2] += C;
    ctx->digest[3] += D;
    ctx->digest[4] += E;

#ifdef __KERNEL__
    kfree(big_locals);
#endif
}

#ifdef SHA1_LITTLE_ENDIAN

/* change endianness of data */

static void byte_reverse(SHA1_LONG *buffer, int count)
{
    int i;
    SHA1_BYTE ct[4], *cp;

    count /= sizeof(SHA1_LONG);
    cp = (SHA1_BYTE *) buffer;
    for (i = 0; i < count; ++i) {
	ct[0] = cp[0];
	ct[1] = cp[1];
	ct[2] = cp[2];
	ct[3] = cp[3];
	cp[0] = ct[3];
	cp[1] = ct[2];
	cp[2] = ct[1];
	cp[3] = ct[0];
	cp += sizeof(SHA1_LONG);
    }
}

#endif /* SHA1_LITTLE_ENDIAN */

/* initialize the SHA digest */

int SHA1Reset(SHA1Context *ctx)
{
    ctx->digest[0] = 0x67452301L;
    ctx->digest[1] = 0xefcdab89L;
    ctx->digest[2] = 0x98badcfeL;
    ctx->digest[3] = 0x10325476L;
    ctx->digest[4] = 0xc3d2e1f0L;
    ctx->count_lo = 0L;
    ctx->count_hi = 0L;
    ctx->no_padding = 0L;
    return 0;
}


int SHA1SetNoPadding(SHA1Context *ctx)
{
    ctx->no_padding = 1;
    return 0;
}

/* update the SHA digest */

int SHA1Input(SHA1Context *ctx, const SHA1_BYTE *buffer, int count)
{
    int res = (ctx->count_lo>>3) & (SHA1_BLOCKSIZE-1);
    if ((ctx->count_lo + ((SHA1_LONG) count << 3)) < ctx->count_lo) {
	++ctx->count_hi;
    }
    ctx->count_lo += (SHA1_LONG) count << 3;
    ctx->count_hi += (SHA1_LONG) count >> 29;
    if (res && res+count >= SHA1_BLOCKSIZE) {
	memcpy((SHA1_BYTE*)ctx->data+res, buffer, SHA1_BLOCKSIZE-res);
	count += res;
	buffer -= res;
	res = 0;
	goto process;
    }
    while (count >= SHA1_BLOCKSIZE) {
	memcpy(ctx->data, buffer, SHA1_BLOCKSIZE);
process:
#ifdef SHA1_LITTLE_ENDIAN
	byte_reverse(ctx->data, SHA1_BLOCKSIZE);
#endif /* SHA1_LITTLE_ENDIAN */
	SHA1Transform(ctx);
	buffer += SHA1_BLOCKSIZE;
	count -= SHA1_BLOCKSIZE;
    }
    memcpy((SHA1_BYTE*)ctx->data+res, buffer, count);
    return 0;
}

/* finish computing the SHA digest */

int SHA1Result(SHA1Context *ctx, SHA1_BYTE digest[SHA1_DIGESTSIZE])
{
    int count;
    SHA1_LONG lo_bit_count, hi_bit_count;

    if (ctx->no_padding == 0) {
        lo_bit_count = ctx->count_lo;
        hi_bit_count = ctx->count_hi;
        count = (int) ((lo_bit_count >> 3) & 0x3f);
        ((SHA1_BYTE *) ctx->data)[count++] = 0x80;
        if (count > 56) {
            memset((SHA1_BYTE *) ctx->data + count, 0, 64 - count);
#ifdef SHA1_LITTLE_ENDIAN
            byte_reverse(ctx->data, SHA1_BLOCKSIZE);
#endif /* SHA1_LITTLE_ENDIAN */
            SHA1Transform(ctx);
            memset(&ctx->data, 0, 56);
        } else {
            memset((SHA1_BYTE *) ctx->data + count, 0, 56 - count);
        }
#ifdef SHA1_LITTLE_ENDIAN
        byte_reverse(ctx->data, SHA1_BLOCKSIZE);
#endif /* SHA1_LITTLE_ENDIAN */
        ctx->data[14] = hi_bit_count;
        ctx->data[15] = lo_bit_count;

        SHA1Transform(ctx);
    }

    memcpy(digest, ctx->digest, sizeof ctx->digest);
#ifdef SHA1_LITTLE_ENDIAN
    byte_reverse((SHA1_LONG *)digest, SHA1_DIGESTSIZE);
#endif /* SHA1_LITTLE_ENDIAN */
    return 0;
}

#ifdef TEST
int main(int argc, char* argv[])
{
    SHA1Context sha;
    SHA1_BYTE digest[SHA1_DIGESTSIZE];
    int i;
    unsigned char x[63];

    for (i = 0; i < sizeof x; i++)
	x[i] = 'a';
    SHA1Reset(&sha);
#if 1
    for (i = 0; i < 1000000/sizeof x; i++)
	SHA1Input(&sha, x, sizeof x);
    if (1000000 % sizeof x)
	SHA1Input(&sha, x, 1000000 % sizeof x);
#else
    //SHA1Input(&sha, "abc", 3);
    {
    char *p = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    while (*p) {
	SHA1Input(&sha, p, 4); p += 4;
    }
    }
#endif
    SHA1Result(&sha, digest);
    for (i = 0; i < 5; i++) printf("%08x\n", sha.digest[i]);
    for (i = 0; i < SHA1_DIGESTSIZE; i++)
	printf("%02x", digest[i]);
    printf("\n");
}
#endif /* TEST */

#endif /* SDK_TWL */
