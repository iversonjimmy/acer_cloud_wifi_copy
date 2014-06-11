#include "km_types.h"
#include "bits.h"
#include "x86.h"

#ifdef C_MEMCPY

inline static void __attribute__ ((always_inline))
memcpy8(u64 *dst, const u64 *src, size_t n)
{
	while (n) {
		*dst++ = *src++;
		n -= sizeof(u64);
	}
}

inline static void __attribute__ ((always_inline))
memcpy4(u32 *dst, const u32 *src, size_t n)
{
	while (n) {
		*dst++ = *src++;
		n -= sizeof(u32);
	}
}

inline static void __attribute__ ((always_inline))
memcpy2(u16 *dst, const u16 *src, size_t n)
{
	while (n) {
		*dst++ = *src++;
		n -= sizeof(u16);
	}
}

inline static void __attribute__ ((always_inline))
memcpy1(u8 *dst, const u8 *src, size_t n)
{
	while (n) {
		*dst++ = *src++;
		n -= sizeof(u8);
	}
}

void *
memcpy(void *dst, const void *src, size_t n)
{
	un asrc = (un)src;
	un adst = (un)dst;
	size_t len;

	/* 8 byte chunks */
	if (n > 7) {
		len = n & ~0x7;
		memcpy8((u64 *)adst, (u64 *)asrc, len);
		asrc += len;
		adst += len;
		n    -= len;
	}

	/* 4 byte chunks */
	if (n > 3) {
		len = n & ~0x3;
		memcpy4((u32 *)adst, (u32 *)asrc, len);
		adst += len;
		asrc += len;
		n    -= len;
	}

	/* 2 byte chunks */
	if (n > 1) {
		len = n & ~0x1;
		memcpy2((u16 *)adst, (u16 *)asrc, len);
		adst += len;
		asrc += len;
		n    -= len;
	}

	if (n > 0) {
		memcpy1((u8 *)adst, (u8 *)asrc, n);
	}

	return dst;
}

#else

void *
memcpy(void *dst, const void *src, size_t n)
{
	un asrc = (un)src;
	un adst = (un)dst;

#define copy_block(type,func)					\
	if (n >= sizeof(type) &&				\
	    asrc % sizeof(type) == 0 &&				\
	    adst % sizeof(type) == 0) {				\
		size_t count = n   / sizeof(type);		\
		size_t len = count * sizeof(type);		\
		func((type *)adst, (type *)asrc, count);	\
		adst += len;					\
		asrc += len;					\
		n    -= len;					\
	}

#ifdef __x86_64__
	copy_block(u64, movsq);	/* 8 byte chunks */
#endif
	copy_block(u32, movsl);	/* 4 byte chunks */
	copy_block(u16, movsw);	/* 2 byte chunks */
	copy_block(u8,  movsb);	/* rest */

#undef copy_block

	return dst;
}

#endif
