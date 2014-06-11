#ifndef __TYPES_H__
#define __TYPES_H__

#include <stdbool.h>

#define NULL 0

typedef unsigned char		u8;
typedef unsigned short		u16;
typedef unsigned int		u32;
typedef unsigned long		un;
typedef unsigned long long	u64;

typedef signed char		s8;
typedef short			s16;
typedef int			s32;
typedef long			sn;
typedef long long		s64;

typedef u16			le16;
typedef u16			be16;
typedef u32			le32;
typedef u32			be32;

#ifndef USERSPACE
typedef un size_t;
#endif

typedef sn  ret_t;
typedef un  addr_t;

#if defined(__x86_64__)
typedef un paddr_t;
#define FMTPADDR "l"
#else
typedef u64 paddr_t;
#define FMTPADDR "ll"
#endif

#if defined(__x86_64__)
#define VWIDTH "16"
#else
#define VWIDTH "8"
#endif

#define PWIDTH   "10"

#define VFMT   "0x%0" VWIDTH "lx"
#define PFMT   "0x%0" PWIDTH FMTPADDR "x"

#define offsetof(type, field)	((sn)&(((type *)0)->field))
#define super(ptr, type, field)	((type *)((void *)(ptr) - \
					  offsetof(type, field)))

#define min(x, y) ({ \
	typeof(x) _x = (x); \
	typeof(y) _y = (y); \
	(void)(&_x == &_y); \
	_x < _y ? _x : _y; \
})

#endif
