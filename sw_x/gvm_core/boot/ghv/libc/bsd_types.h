#ifndef __BSD_TYPES_H__
#define __BSD_TYPES_H__

#ifdef NO_INTMAX_64
typedef un	uintmax_t;
typedef sn	intmax_t;
#else
typedef u64	uintmax_t;
typedef s64	intmax_t;
#endif

typedef u8	u_char;
typedef u16	u_short;
typedef u32	u_int;
typedef un	u_long;
typedef s64	quad_t;
typedef u64	u_quad_t;
typedef un	uintptr_t;
typedef sn	ptrdiff_t;
typedef sn	ssize_t;

#define CHAR_BIT	8	/* Number of bits in a char */

/* Define the order of 32-bit words in 64-bit words. */
#define _QUAD_HIGHWORD	1
#define _QUAD_LOWWORD	0

#endif
