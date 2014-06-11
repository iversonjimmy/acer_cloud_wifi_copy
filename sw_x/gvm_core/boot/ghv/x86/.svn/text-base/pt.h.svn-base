#ifndef __PT_H__
#define __PT_H__

#ifdef __x86_64__

typedef un pte_t;
#define PTEWIDTH "16"
#define FMTPTE   "l"

#define HOST_64		true
#define HOST_PAE	false

#define PT_ROOT_LEVEL 4

#define PTE_TO_PAGE(pte)	(bits(pte, 51, 12) << VM_PAGE_SHIFT)
#define PDE_TO_PAGE(pte)	(bit_test(pte, PDE_PAGESIZE) ? \
				 (bits(pte, 51, 21) << VM_LARGE_PAGE_SHIFT) : \
				 (bits(pte, 51, 12) << VM_PAGE_SHIFT))

#define l4_index(vaddr)		bits((vaddr), 47, 39)
#define l3_index(vaddr)		bits((vaddr), 38, 30)
#define l2_index(vaddr)		bits((vaddr), 29, 21)
#define l1_index(vaddr)		bits((vaddr), 20, 12)

#elif defined(HYP_PAE)

typedef u64 pte_t;
#define PTEWIDTH "16"
#define FMTPTE   "ll"

#define HOST_64		false
#define HOST_PAE	true

#define PT_ROOT_LEVEL 3

#define PTE_TO_PAGE(pte)	(bits64(pte, 51, 12) << VM_PAGE_SHIFT)
#define PDE_TO_PAGE(pte)	(bit_test(pte, PDE_PAGESIZE) ? \
				 (bits64(pte, 51, 21) << VM_LARGE_PAGE_SHIFT) :\
				 (bits64(pte, 51, 12) << VM_PAGE_SHIFT))

#define l3_index(vaddr)		bits((vaddr), 31, 30)
#define l2_index(vaddr)		bits((vaddr), 29, 21)
#define l1_index(vaddr)		bits((vaddr), 20, 12)

#else

typedef un pte_t;
#define PTEWIDTH "8"
#define FMTPTE   "l"

#define HOST_64		false
#define HOST_PAE	false

#define PT_ROOT_LEVEL 2

#define PTE_TO_PAGE(pte)	(bits(pte, 31, 12) << VM_PAGE_SHIFT)
#define PDE_TO_PAGE(pte)	(bit_test(pte, PDE_PAGESIZE) ? \
				 (bits(pte, 31, 22) << VM_LARGE_PAGE_SHIFT) : \
				 (bits(pte, 31, 12) << VM_PAGE_SHIFT))

#define l2_index(vaddr)		bits((vaddr), 31, 22)
#define l1_index(vaddr)		bits((vaddr), 21, 12)

#endif

#define PTEFMT "0x%0" PTEWIDTH FMTPTE "x"

#define PTE_PRESENT	0
#define PTE_WRITABLE	1
#define PTE_USER	2
#define PTE_WRITETHRU	3
#define PTE_NOCACHE	4
#define PTE_ACCESSED	5
#define PTE_DIRTY	6
#define PTE_PAT		7
#define PTE_GLOBAL 	8

#define PDE_PAGESIZE	7
#define PDE_PAT		12

#define PTES_PER_PAGE (VM_PAGE_SIZE / sizeof(pte_t))
#define PTES_PER_PAE_L3	4

#define PAE_L3_FLAGS	(bit(PTE_PRESENT))
#define PTE_FLAGS	(bit(PTE_PRESENT) | bit(PTE_WRITABLE) | \
			 bit(PTE_ACCESSED) | bit(PTE_DIRTY))
#define PDE_FLAGS	(PTE_FLAGS | bit(PTE_USER))
#define PDE_FLAGS_LARGE_PAGE (PTE_FLAGS | bit(PDE_PAGESIZE)) 

#define pde_to_kv(pte)		((pte_t *)phystokv(PDE_TO_PAGE(pte)))

extern pte_t *pt_root;
extern un host_cr3;
extern un shadow_ident_cr3;

inline static pte_t
mkpte(paddr_t paddr, un flags)
{
	paddr_t pfn_mask = PTE_TO_PAGE(~((paddr_t)0));

	return (paddr & pfn_mask) | flags;
}

inline static pte_t
mkpde(un paddr, un flags)
{
	paddr_t pfn_mask;

	if (bit_test(flags, PDE_PAGESIZE)) {
		pfn_mask = PDE_TO_PAGE(~((paddr_t)0));
	} else {
		pfn_mask = PTE_TO_PAGE(~((paddr_t)0));
	}

	return (paddr & pfn_mask) | flags;
}

inline static un
mkcr3(un paddr, un flags)
{
	return TRUNC_PAGE(paddr) | flags;
}

inline static void
tlb_flush_local(addr_t vaddr)
{
	invlpg(vaddr);
}

inline static void
tlb_flush_all_local(void)
{
	un cr4 = get_CR4();
	set_CR4(cr4 ^ bit(CR4_PGE));
	set_CR4(cr4);
}

extern void tlb_flush(addr_t);
extern void tlb_flush_all(void);
extern ret_t pt_construct(void);
extern void pt_destroy(void);
extern ret_t pt_install_pte(pte_t *, un, pte_t, bool);
extern ret_t pt_clear_pte(pte_t *, un, bool);

extern void dump_pt(addr_t, addr_t);
extern void dump_pagetables(un, addr_t, addr_t);

#endif
