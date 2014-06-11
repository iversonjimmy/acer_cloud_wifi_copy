#include "compiler.h"
#include "km_types.h"
#include "bits.h"
#include "printf.h"
#include "assert.h"
#include "x86.h"
#include "pt.h"
#include "valloc.h"
#include "pagepool.h"
#include "setup/gvmdefs.h"
#include "ghv_mem.h"

extern u8 _begin[];

extern u8 _kern_text[];
extern u8 _kern_etext[];
extern u8 _kern_data[];
extern u8 _kern_edata[];
extern u8 _kern_bss[];
extern u8 _kern_ebss[];
extern u8 _kern_stack_begin[];
extern u8 _kern_stack_end[];

extern u8 _user_text[];
extern u8 _user_etext[];
extern u8 _user_data[];
extern u8 _user_edata[];
extern u8 _user_bss[];
extern u8 _user_ebss[];
extern u8 _user_stack_begin[];
extern u8 _user_stack_end[];

#define PTE_BASE   (((un)0xFFFF << 48) | ((un)PT_MAGIC_INDEX << 39))
#define PDE_BASE   (PTE_BASE   | ((un)PT_MAGIC_INDEX << 30))
#define PDPTE_BASE (PDE_BASE   | ((un)PT_MAGIC_INDEX << 21))
#define PML4E_BASE (PDPTE_BASE | ((un)PT_MAGIC_INDEX << 12))

static inline
pte_t *__get_pte(addr_t va, int level)
{
    pte_t *base;
    un index;

    switch (level) {
    case 1:
	base  = (void *)PTE_BASE;
	index = bits(va, 47, 12);
	break;
    case 2:
	base  = (void *)PDE_BASE;
	index = bits(va, 47, 21);
	break;
    case 3:
	base  = (void *)PDPTE_BASE;
	index = bits(va, 47, 30);
	break;
    case 4:
	base  = (void *)PML4E_BASE;
	index = bits(va, 47, 39);
	break;
    default:
	return NULL;
    }

    return &base[index];
}

#ifdef PT_PHYS_DUMP
void
phys_dump(paddr_t pa)
{
    addr_t va;
    pte_t *l4, *l3, *l2, *l1;
    un i4, i3, i2, i1;
    pte_t v, mask;
    pte_t mask1 = (~0ULL) << 12;
    pte_t mask2 = (~0ULL) << 21;
    pte_t mask3 = (~0ULL) << 30;
    pte_t mask4 = (~0ULL) << 39;

    mask = mask1;
    pa &= mask;
    v = get_CR3();
    kprintf("pt: cr3=%lx pa=%lx\n", v, pa);
    l4 = (void *)PML4E_BASE;
    if((v & mask) == pa)
        printf("l4: %p %lx\n", l4, v);
    for(i4 = 0; i4 < 512; i4++, l4++) {
        v = *l4;
        if (!bit_test(v, PTE_PRESENT))
            continue;
	mask = bit_test(v, PDE_PAGESIZE)? mask4 : mask1;
        va = i4 << 39;
        if((v & mask) == (pa & mask))
            printf("l3-%03lx %012lx %lx\n", i4, va, v);
        l3 = __get_pte(va, 3);
        if(i4 == PT_MAGIC_INDEX)
            continue;
        if(bit_test(v, PDE_PAGESIZE))
            continue;
        for(i3 = 0; i3 < 512; i3++, l3++) {
            v = *l3;
            if (!bit_test(v, PTE_PRESENT))
                continue;
	    mask = bit_test(v, PDE_PAGESIZE)? mask3 : mask1;
            va = (va & 0xff8000000000ULL) | (i3 << 30);
            if((v & mask) == (pa & mask))
                printf("l2-%03lx %012lx %lx\n", i3, va, v);
            if(bit_test(v, PDE_PAGESIZE))
                continue;
            l2 = __get_pte(va, 2);
            for(i2 = 0; i2 < 512; i2++, l2++) {
                v = *l2;
                if (!bit_test(v, PTE_PRESENT))
                    continue;
	        mask = bit_test(v, PDE_PAGESIZE)? mask2 : mask1;
                va = (va & 0xffffc0000000ULL) | (i2 << 21);
                if((v & mask) == (pa & mask))
                    printf("l1-%03lx %012lx %lx\n", i2, va, v);
                if(bit_test(v, PDE_PAGESIZE))
                    continue;
                l1 = __get_pte(va, 1);
                for(i1 = 0; i1 < 512; i1++, l1++) {
                    v = *l1;
                    if (!bit_test(v, PTE_PRESENT))
                        continue;
                    mask = mask1;
                    va = (va & 0xffffffe00000ULL) | (i1 << 12);
                    if((v & mask) == (pa & mask))
                        printf("l0-%03lx %012lx %lx\n", i1, va, v);
                }
            }
        }
    }
}
#endif // PT_PHYS_DUMP

bool
is_mapped(addr_t va)
{
    pte_t *l3 = __get_pte(va, 3);
    pte_t *pde = __get_pte(va, 2);
    pte_t *pte = __get_pte(va, 1);

    if( !bit_test(*l3, PTE_PRESENT))
        return(0);
    if( !bit_test(*pde, PTE_PRESENT))
        return(0);
    if(bit_test(*pde, PDE_PAGESIZE))
        return(1);
    if(bit_test(*pte, PTE_PRESENT))
        return(1);
    return(0);
}

paddr_t
virt_to_phys(addr_t va)
{
    pte_t *pte = __get_pte(va, 4);

    if (!bit_test(*pte, PTE_PRESENT)) {
	return NOMAP;
    }

    pte = __get_pte(va, 3);
    if (!bit_test(*pte, PTE_PRESENT)) {
	return NOMAP;
    }

    pte = __get_pte(va, 2);
    if (!bit_test(*pte, PTE_PRESENT)) {
	return NOMAP;
    }
    if (bit_test(*pte, PDE_PAGESIZE)) {
	return TRUNC_LARGE_PAGE(*pte) | (va & VM_LARGE_PAGE_OFFSET);
    }

    pte = __get_pte(va, 1);
    if (!bit_test(*pte, PTE_PRESENT)) {
	return NOMAP;
    }   
    return TRUNC_PAGE(*pte) | (va & VM_PAGE_OFFSET);	 
}

static inline void
set_pte_range(pte_t *pt, void *base, void *start, void *end, un pte_flags)
{
    int i = ((addr_t)start - (addr_t)base) >> VM_PAGE_SHIFT;
    int j = ((addr_t)end - (addr_t)base) >> VM_PAGE_SHIFT;

    for (; i < j; i++) {
	pt[i] = TRUNC_PAGE(pt[i]) | pte_flags;
    }
}

static void
remap_ghv(void)
{
#if 0
    static pte_t ghv_pt[PTES_PER_PAGE] __attribute__((aligned(VM_PAGE_SIZE)));
    paddr_t phys = virt_to_phys((addr_t)_begin);
    addr_t va;
    int i;

    for (i = 0; i < PTES_PER_PAGE; i++) {
	ghv_pt[i] = phys;
	phys += VM_PAGE_SIZE;
    }

    un ro_pte_flags = bit(PTE_GLOBAL) | bit(PTE_USER) | bit(PTE_PRESENT);
    un rw_pte_flags = ro_pte_flags | bit(PTE_WRITABLE);

    set_pte_range(ghv_pt, _begin, _kern_text, _kern_etext, ro_pte_flags);
    set_pte_range(ghv_pt, _begin, _kern_data, _kern_edata, ro_pte_flags);
    set_pte_range(ghv_pt, _begin, _kern_bss,  _kern_ebss,  ro_pte_flags);
    set_pte_range(ghv_pt, _begin, _kern_stack_begin, _kern_stack_end, ro_pte_flags);

    set_pte_range(ghv_pt, _begin, _user_text, _user_etext, ro_pte_flags);
    set_pte_range(ghv_pt, _begin, _user_data, _user_edata, rw_pte_flags);
    set_pte_range(ghv_pt, _begin, _user_bss,  _user_ebss,  rw_pte_flags);
    set_pte_range(ghv_pt, _begin, _user_stack_begin, _user_stack_end, rw_pte_flags);

    volatile pte_t *ghv_pde = __get_pte((addr_t)_begin, 2);
    assert(bit_test(*ghv_pde, PDE_PAGESIZE));

    *ghv_pde = virt_to_phys((addr_t)ghv_pt) | rw_pte_flags;

    for (va = (addr_t)_begin; va < (addr_t)_first_free_page; va += VM_PAGE_SIZE) {
	invlpg(va);
    }
#endif
}

static void pml4_fixup(addr_t pml4)
{
    pte_t *p = (pte_t *)pml4;

    p[PT_MAGIC_INDEX] = pml4 | PTE_FLAGS;
}

void ghv_pagetable_fixup(un cr3)
{
    pml4_fixup((addr_t)TRUNC_PAGE(cr3));
    remap_ghv();
}

void ghv_map(addr_t va, paddr_t pa)
{
    pte_t *pte;

    pte = __get_pte(va, 3);
    if (!bit_test(*pte, PTE_PRESENT)) {
        addr_t pt = page_alloc();
	*pte = pt | PDE_FLAGS;
    } else {
        assert(!bit_test(*pte, PDE_PAGESIZE));
    }

    pte = __get_pte(va, 2);
    if (!bit_test(*pte, PTE_PRESENT)) {
	addr_t pt = page_alloc();
	*pte = pt | PDE_FLAGS;
    } else {
	assert(!bit_test(*pte, PDE_PAGESIZE));
    }

    pte = __get_pte(va, 1);
    assert(!bit_test(*pte, PTE_PRESENT));
    *pte = pa | PTE_FLAGS;
}

paddr_t ghv_unmap(addr_t va)
{
    pte_t *pde = __get_pte(va, 2);
    pte_t *pte = __get_pte(va, 1);
    paddr_t page;

    assert(bit_test(*pde, PTE_PRESENT));
    assert(!bit_test(*pde, PDE_PAGESIZE));
    assert(bit_test(*pte, PTE_PRESENT));

    page = bits(*pte, 47, 12) << 12;
    *pte = 0;

    invlpg(va);
    return page;
}

#define SPACE_PER_STACK ((_kern_stack_end - _kern_stack_begin) / NR_CPUS / 2)

void *alloc_stack(int cpuno, u32 size, bool interrupt)
{
    extern u8 _kern_stack_begin[];
    extern u8 _kern_stack_end[];
    int stackno = (interrupt ? NR_CPUS : 0) + cpuno;
    u8 *stack;
    u32 pages = (size + VM_PAGE_OFFSET) >> VM_PAGE_SHIFT;

    size = pages << VM_PAGE_SHIFT;

    assert(cpuno < NR_CPUS);
    assert(size <= SPACE_PER_STACK - VM_PAGE_SIZE); /* Need 1 guard page */

    /* allocate from end */
    stack = _kern_stack_begin + ((stackno + 1) * SPACE_PER_STACK);
    while (pages) {
	addr_t page;
	pages--;
	stack -= VM_PAGE_SIZE;

	page = page_alloc();
	assert(page);

	ghv_map((addr_t)stack, page);
    }

    printf("%s(%d, %x, %d) = %p\n", __func__, cpuno, size, interrupt, stack);

    return stack;
}

void free_stack(void *stack)
{
    extern u8 _kern_stack_begin[];
    extern u8 _kern_stack_end[];
    int stackno;
    u8 *stackend;
    u8 *s = stack;

    assert(s > _kern_stack_begin && s < _kern_stack_end);

    stackno = (s - _kern_stack_begin) / SPACE_PER_STACK;
    stackend = _kern_stack_begin + ((stackno + 1) * SPACE_PER_STACK);

    for (; s < stackend; s += VM_PAGE_SIZE) {
	addr_t page = ghv_unmap((addr_t)s);
	page_free(page);
    }
}

void ghv_map_to_guest(un guest_cr3)
{
    pte_t *guest_pml4 = (void *)map_page(guest_cr3, 0);
    
    guest_pml4[0] = *__get_pte(0, 4);

    if (bit_test(guest_pml4[511], PTE_PRESENT)) {
	pte_t *guest_pdpt = (void *)map_page(guest_pml4[511], 0);
	guest_pdpt[511] = *__get_pte((addr_t)_begin, 3);
	unmap_page((addr_t)guest_pdpt, UMPF_LOCAL_FLUSH);
    } else {
	guest_pml4[511] = *__get_pte((addr_t)_begin, 4);
    }

    unmap_page((addr_t)guest_pml4, UMPF_LOCAL_FLUSH);
}
