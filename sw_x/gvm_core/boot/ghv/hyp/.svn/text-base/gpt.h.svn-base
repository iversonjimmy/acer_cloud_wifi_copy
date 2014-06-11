#ifndef __GPT_H__
#define __GPT_H__

#include "pt.h"

typedef u64 gpte_t;

inline static u8
guest_page_shift(bool large, bool guest64, bool pae)
{
	return (large ?
		((guest64 || pae) ? VM_2M_PAGE_SHIFT : VM_4M_PAGE_SHIFT) :
		VM_PAGE_SHIFT);
}

inline static paddr_t
gpte_to_phys(gpte_t pte, bool isPDE, bool guest64, bool pae)
{
	paddr_t paddr;
	if (guest64 || pae) {
		if (isPDE && bit_test(pte, PDE_PAGESIZE)) {
			paddr = (bits64(pte, 51, 21) << VM_2M_PAGE_SHIFT);
		} else {
			paddr = (bits64(pte, 51, 12) << VM_PAGE_SHIFT);
		}
	} else {
		if (isPDE && bit_test(pte, PDE_PAGESIZE)) {
			paddr = (bits64(pte, 31, 22) << VM_4M_PAGE_SHIFT);
		} else {
			paddr = (bits64(pte, 31, 12) << VM_PAGE_SHIFT);
		}
	}

	return paddr;
}

extern ret_t __gvaddr_to_gpte(un, bool, bool, gpte_t *, u32, un);
extern ret_t gvaddr_to_gpte(vm_t *, un, gpte_t *);
extern ret_t hvaddr_to_pte(un, gpte_t *);
extern ret_t guest_copy(void *, addr_t, size_t, bool);

inline static ret_t
copy_from_guest(void *dst, addr_t gvaddr, size_t len)
{
	return guest_copy(dst, gvaddr, len, true);
}

inline static ret_t
copy_to_guest(addr_t gvaddr, void *src, size_t len)
{
	return guest_copy(src, gvaddr, len, false);
}

extern void dump_gdirectory(u32, addr_t, void *, bool, bool, addr_t, addr_t);
extern void dump_gpt(void);

#endif
