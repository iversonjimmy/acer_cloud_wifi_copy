#ifndef __ASSERT_H__
#define __ASSERT_H__

#define always_assert(x) \
({ \
	if (!(x)) { \
		kprintf("assert failed: file %s line %d: %s\n", \
			__FILE__, __LINE__, #x); \
	} \
 })

#define assert(x)	always_assert(x)

#define static_assert(x) { typedef u8 __static_assert[(x) ? 1 : -1]; }

#endif
