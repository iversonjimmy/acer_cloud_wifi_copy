
#ifndef __GHV_ENDIAN_H__
#define __GHV_ENDIAN_H__

static inline u16
bswap16(u16 d16)
{
	u8 *d8p;

	d8p = (u8 *)&d16;

	return ((u16)d8p[0] << 8)  | (u16)d8p[1];
}

static inline u32
bswap32(u32 d32)
{
	u8 *d8p;

	d8p = (u8 *)&d32;

	return    ((u32)d8p[0] << 24) | ((u32)d8p[1] << 16)
		| ((u32)d8p[2] << 8)  | (u32)d8p[3];
}

static inline u64
bswap64(u64 d64)
{
	u8 *d8p;

	d8p = (u8 *)&d64;

	return    ((u64)d8p[0] << 56) | ((u64)d8p[1] << 48) 
		| ((u64)d8p[2] << 40) | ((u64)d8p[3] << 32)
		| ((u64)d8p[4] << 24) | ((u64)d8p[5] << 16)
		| ((u64)d8p[6] << 8)  | (u64)d8p[7];

}

#ifdef WIN32
  // blindly assuming little endian
#  define cpu_to_be16(x)		bswap16(x)
#  define cpu_to_be32(x)		bswap32(x)
#  define cpu_to_be64(x)		bswap64(x)
#elif !defined(_KERNEL_MODULE)
  // cpu_to_* macros are already defined in linux/byteorder/generic.h for kernel module builds
#  if __BYTE_ORDER == __LITTLE_ENDIAN
#    define cpu_to_be16(x)		bswap16(x)
#    define cpu_to_be32(x)		bswap32(x)
#    define cpu_to_be64(x)		bswap64(x)
#  else
#    define cpu_to_be16(x)		(x)
#    define cpu_to_be32(x)		(x)
#    define cpu_to_be64(x)		(x)
#  endif // BIG_ENDIAN
#endif

#endif // __GHV_ENDIAN_H__
