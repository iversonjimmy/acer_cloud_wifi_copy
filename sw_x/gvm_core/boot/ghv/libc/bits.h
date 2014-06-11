#ifndef __BITS_H__
#define __BITS_H__

#define bit(bitno)		(1ULL << (bitno))
#define bit_clear(word, bitno)	((word) &= ~bit(bitno))
#define bit_set(word, bitno)	((word) |= bit(bitno))
#define bit_test(word, bitno)	(((word) & bit(bitno)) != 0)

inline static long
bit_first(unsigned long x)
{
	if (x == 0) {
		return -1;
	}

	long res;
	asm volatile("bsr %1, %0" : "=r" (res) : "r" (x) : "cc");
	return res;
}

inline static long
bit_last(unsigned long x)
{
	if (x == 0) {
		return -1;
	}

	long res;
	asm volatile("bsf %1, %0" : "=r" (res) : "r" (x) : "cc");
	return res;
}

inline static long
bit_next(unsigned long x, u8 previous_bit)
{
	unsigned long mask = ((1UL << previous_bit) - 1UL);
	return bit_first(x & mask);
}

inline static unsigned long
bitcount_mit(unsigned long n)
{
	/* MIT HACKMEM bitcount algorithm */
	register unsigned long c;

#ifdef __x86_64__
	c = n - ((n >> 1) & 0x7777777777777777UL)
	      - ((n >> 2) & 0x3333333333333333UL)
	      - ((n >> 3) & 0x1111111111111111UL);

	return ((c + (c >> 4)) & 0x0F0F0F0F0F0F0F0FUL) % 255;
#else	
	c = n - ((n >> 1) & 033333333333)
	      - ((n >> 2) & 011111111111);

	return ((c + (c >> 3)) & 030707070707) % 63;
#endif
}

inline static unsigned long
__bit_count(unsigned long x)
{
	return bitcount_mit(x);
}

inline static unsigned long
extract(u64 x, u8 shift, u8 width)
{
	unsigned long mask = ((1UL << width) - 1);

	return ((x >> shift) & mask);
}

#define bits(x, hibit, lobit)	extract((u64)(x), \
				       	(lobit), ((hibit) + 1 - (lobit)))

inline static u64
extract64(u64 x, u8 shift, u8 width)
{
	u64 mask = ((1ULL << width) - 1ULL);

	return ((x >> shift) & mask);
}

#define bits64(x, hibit, lobit)	extract64((u64)(x), \
					  (lobit), ((hibit) + 1 - (lobit)))

#define for_each_cpu(i, cpumask) for (long i = bit_first(cpumask); i >= 0; \
				      i = bit_next(cpumask, i))

#endif
