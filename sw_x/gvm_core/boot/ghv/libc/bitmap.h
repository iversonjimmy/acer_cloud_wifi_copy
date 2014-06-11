#ifndef __BITMAP_H__
#define __BITMAP_H__

typedef un bitmap_t;

#define	BITMAP_WORD_SHIFT		(4 + sizeof(un)/sizeof(u32))
#define	BITMAP_WORD_OFFSET		((1UL << BITMAP_WORD_SHIFT) - 1)

#define	BITMAP_ALL_ZEROES		0UL
#define	BITMAP_ALL_ONES			~0UL

#define	bitmap_word_index(i)		((i) >> BITMAP_WORD_SHIFT)
#define	bitmap_word_at(bitmap, i)	((bitmap)[bitmap_word_index(i)])
#define	bitmap_word_bit(i)		(1UL << ((i) & BITMAP_WORD_OFFSET))
#define bitmap_word_size(i)		(((i) + BITMAP_WORD_OFFSET) >> \
					 BITMAP_WORD_SHIFT)

inline static void
bitmap_init(bitmap_t *bitmap, un nbits, un value)
{
	memset(bitmap, value, sizeof(bitmap_t[bitmap_word_size(nbits)]));
}

inline static bool
bitmap_test(bitmap_t *bitmap, un i)
{
	return ((bitmap_word_at(bitmap, i) & bitmap_word_bit(i)) != 0);
} 

inline static void
bitmap_set(bitmap_t *bitmap, un i)
{
	bitmap_word_at(bitmap, i) |= bitmap_word_bit(i);
}

inline static void
bitmap_clear(bitmap_t *bitmap, un i)
{
	bitmap_word_at(bitmap, i) &= ~(bitmap_word_bit(i));
}

inline static sn
bitmap_first(bitmap_t *bitmap, const un nbits)
{
	assert((nbits & BITMAP_WORD_OFFSET) == 0);

	for (sn i = bitmap_word_size(nbits) - 1; i >= 0; i--) {
		if (bitmap[i] == 0) {
			continue;
		}
		return (bit_first(bitmap[i]) + (i << BITMAP_WORD_SHIFT)); 
	}
	return -1;
}

/*
 * Returns the next highest bit set after prev,
 * or -1 if no other bits are set.
 */
inline static sn
bitmap_next(bitmap_t *bitmap, const un prev)
{
	bitmap_t mask = (bitmap_word_bit(prev) - 1UL);
	for (sn i = bitmap_word_size(prev + 1) - 1; i >= 0; i--) {
		if ((mask & bitmap[i]) == 0) {
			mask = ~0UL;
			continue;
		}
		return (bit_first((mask & bitmap[i])) +
			(i << BITMAP_WORD_SHIFT)); 
	}
	return -1;
}

inline static sn
bitmap_last(bitmap_t *bitmap, const un nbits)
{
	assert((nbits & BITMAP_WORD_OFFSET) == 0);

	for (sn i = 0; i < bitmap_word_size(nbits); i++) {
		if (bitmap[i] == 0) {
			continue;
		}
		return (bit_last(bitmap[i]) + (i << BITMAP_WORD_SHIFT)); 
	}
	return -1;
}

inline static un
bitmap_count(bitmap_t *bitmap, const un nbits)
{
	assert((nbits & BITMAP_WORD_OFFSET) == 0);

	un count = 0;
	for (un i = 0; i < bitmap_word_size(nbits); i++) {
		count += bit_count(bitmap[i]);
	}
	return count;
}

inline static void
bitmap_invert(bitmap_t *bitmap, const un nbits)
{
	for (un i = 0; i < bitmap_word_size(nbits); i++) {
		bitmap[i] ^= ~0;
	}
}

inline static void
bitmap_copy_inverted(bitmap_t *dest, bitmap_t *src, const un nbits)
{
	for (un i = 0; i < bitmap_word_size(nbits); i++) {
		dest[i] = src[i] ^ ~0;
	}
}

inline static void
bitmap_dump(bitmap_t *bitmap, const un nbits)
{
	for (un i = 0; i < bitmap_word_size(nbits); i++) {
		kprintf("%02lu 0x%08lx\n", i, bitmap[i]);
	}
}

#endif
