#include "hyp.h"
#include "pt.h"
#include "kvtophys.h"
#include "valloc.h"

static LOCK_INIT_STATIC(vaddr_lock);
static bitmap_t free_map[bitmap_word_size(FREE_MAP_NBITS)];
static bitmap_t used_map[bitmap_word_size(FREE_MAP_NBITS)];

addr_t base_vaddr;

void
fin_valloc(void)
{
}

ret_t
init_valloc(void)
{
	bitmap_init(free_map, FREE_MAP_NBITS, BITMAP_ALL_ONES);
	base_vaddr = bp->bp_valloc_base;
	return 0;
}

static ret_t
__map_page_vaddr(paddr_t paddr, un mp_flags, addr_t vaddr)
{
	un pte_flags = PTE_FLAGS;
	if (mp_flags & MPF_IO) {
		pte_flags |= (bit(PTE_GLOBAL) | bit(PTE_WRITETHRU) |
			      bit(PTE_NOCACHE));
	}
	pte_t pte = mkpte(paddr, pte_flags);

	ret_t ret = pt_install_pte(pt_root, vaddr, pte, false);
	if (ret) {
		kprintf("__map_page_vaddr>pt_install_pte() error %ld\n", ret);
	}

	return ret;
}

addr_t
map_pages(paddr_t paddr, un npages, un mp_flags)
{
	if ((npages > FREE_MAP_NBITS) || (npages == 0)) {
		kprintf("map_pages>npages=%ld invalid (limit %d)\n",
			npages, FREE_MAP_NBITS);
		return 0;
	}
	if (mp_flags & ~MPF_MASK) {
		kprintf("map_pages>invalid mp_flags 0x%lx\n",
			mp_flags & ~MPF_MASK);
		return 0;
	}
	if (mp_flags & MPF_LARGE) {
		kprintf("map_pages>MPF_LARGE unimplemented\n");
		return 0;
	}

	un flags;
	spinlock(&vaddr_lock, flags);
	bitmap_copy_inverted(used_map, free_map, FREE_MAP_NBITS);
	sn n;
	sn used = FREE_MAP_NBITS;
	do {
		n = bitmap_next(free_map, used);
		if (n < (npages - 1)) {
			spinunlock(&vaddr_lock, flags);
			kprintf("map_pages>can't find %ld free pages\n",
				npages);
			return 0;
		}
		used = bitmap_next(used_map, n);
	} while (n - used < npages);
	for (un i = n; i > (n - npages); i--) {
		bitmap_clear(free_map, i);
	}
	spinunlock(&vaddr_lock, flags);

	/* Start from the lowest bit */
	n = n - npages + 1;
	addr_t start_vaddr = base_vaddr + (n << VM_PAGE_SHIFT);
	addr_t vaddr = start_vaddr;
	for (un i = 0; i < npages; i++) {
		ret_t ret = __map_page_vaddr(paddr, mp_flags, vaddr);
		if (ret) {
			if (ret == -ENOMEM) {
				unmap_pages(start_vaddr, npages, UMPF_NO_FLUSH);
			}
			return 0;
		}
		vaddr += VM_PAGE_SIZE;
		paddr += VM_PAGE_SIZE;
	}

	return start_vaddr;
}

void
unmap_pages(addr_t vaddr, un npages, un ump_flags)
{
	if ((npages > FREE_MAP_NBITS) || (npages == 0)) {
		kprintf("unmap_pages>npages=%ld invalid (limit %d)\n",
			npages, FREE_MAP_NBITS);
		return;
	}
	if (ump_flags & ~UMPF_MASK) {
		kprintf("unmap_page>invalid ump_flags 0x%lx\n",
			ump_flags & ~UMPF_MASK);
		return;
	}
	if (!is_mapped_addr(vaddr)) {
		kprintf("unmap_pages>invalid vaddr\n");
		return;
	}

	bool noflush = ump_flags & UMPF_NO_FLUSH;
	bool localflush = ump_flags & UMPF_LOCAL_FLUSH;

	sn n = (vaddr - base_vaddr) >> VM_PAGE_SHIFT;
	if (n + npages > FREE_MAP_NBITS) {
		kprintf("unmap_pages>invalid page range\n");
		return;
	}

	for (un i = n; i < npages + n; i++) {
		addr_t vaddr = base_vaddr + (i << VM_PAGE_SHIFT);
		pt_clear_pte(pt_root, vaddr, false);

		if (localflush) {
			tlb_flush_local(vaddr);
		} else if (!noflush) {
			tlb_flush(vaddr);
		}
	}

	un flags;
	spinlock(&vaddr_lock, flags);
	for (un i = n; i < npages + n; i++) {
		bitmap_set(free_map, i);
	}
	spinunlock(&vaddr_lock, flags);
}

addr_t
map_page(paddr_t paddr, un mp_flags)
{
	if (mp_flags & ~MPF_MASK) {
		kprintf("map_page>invalid mp_flags 0x%lx\n",
			mp_flags & ~MPF_MASK);
		return 0;
	}
	if (mp_flags & MPF_LARGE) {
		kprintf("map_page>MPF_LARGE unimplemented\n");
		return 0;
	}

	un flags;
	spinlock(&vaddr_lock, flags);
	sn n = bitmap_first(free_map, FREE_MAP_NBITS);
	if (n < 0) {
		spinunlock(&vaddr_lock, flags);
		return 0;
	}
	bitmap_clear(free_map, n);
	spinunlock(&vaddr_lock, flags);

	addr_t vaddr = base_vaddr + (n << VM_PAGE_SHIFT); 

	ret_t ret = __map_page_vaddr(paddr, mp_flags, vaddr);
	if (ret) {
		if (ret == -ENOMEM) {
			spinlock(&vaddr_lock, flags);
			bitmap_set(free_map, n);
			spinunlock(&vaddr_lock, flags);
		}
		return 0;
	}

	return vaddr;
}

void
unmap_page(addr_t vaddr, un ump_flags)
{
	if (ump_flags & ~UMPF_MASK) {
		kprintf("unmap_page>invalid ump_flags 0x%lx\n",
			ump_flags & ~UMPF_MASK);
		return;
	}
	if (!is_mapped_addr(vaddr)) {
		return;
	}

	sn n = (vaddr - base_vaddr) >> VM_PAGE_SHIFT;

	assert(!bitmap_test(free_map, n));

	ret_t ret = pt_clear_pte(pt_root, vaddr, false);
	assert(ret == 0);

	un flags;
	spinlock(&vaddr_lock, flags);
	bitmap_set(free_map, n);
	spinunlock(&vaddr_lock, flags);

	bool noflush = ump_flags & UMPF_NO_FLUSH;
	if (noflush) {
		return;
	}

	bool localflush = ump_flags & UMPF_LOCAL_FLUSH;
	if (localflush) {
		tlb_flush_local(vaddr);
	} else {
		tlb_flush(vaddr);
	}
}

void
valloc_test(void)
{
	kprintf("valloc_test>ENTER\n");

	addr_t p1 = page_alloc();
	addr_t p2 = page_alloc();
	addr_t p3 = page_alloc();

	addr_t v1 = map_page(0x1000, 0);
	kprintf("valloc_test>v1 = 0x%lx\n", v1);

	addr_t v2 = map_page(0x2000, 0);
	kprintf("valloc_test>v2 = 0x%lx\n", v2);
	addr_t v3 = map_page(0x3000, 0);
	kprintf("valloc_test>v3 = 0x%lx\n", v3);

	dump_pt(base_vaddr, end_vaddr);

	kprintf("valloc_test>unmapping\n");

	unmap_page(v3, 0);
	unmap_page(v1, 0);
	unmap_page(v2, 0);

	dump_pt(base_vaddr, end_vaddr);

	v2 = map_page(0x2000, 0);
	v3 = map_page(0x3000, 0);
	v1 = map_page(0x1000, 0);
	kprintf("valloc_test>v1 = 0x%lx\n", v1);
	kprintf("valloc_test>v2 = 0x%lx\n", v2);
	kprintf("valloc_test>v3 = 0x%lx\n", v3);

	dump_pt(base_vaddr, end_vaddr);

	unmap_page(v3, 0);
	unmap_page(v1, 0);
	unmap_page(v2, 0);

	dump_pt(base_vaddr, end_vaddr);

	v1 = map_page(kvtophys(p1), 0);

	un *n = (un *)v1;
	*n = 0x12345678;

	kprintf("p1 0x%lx = 0x%lx\n", p1, *(un *)p1);
	kprintf("v1 0x%lx = 0x%lx\n", v1, *(un *)v1);

	unmap_page(v1, 0);
	v2 = map_page(kvtophys(p2), 0);

	kprintf("p2 0x%lx = 0x%lx\n", p2, *(un *)p2);
	kprintf("v2 0x%lx = 0x%lx\n", v2, *(un *)v2);

	unmap_page(v2, 0);

	addr_t v4, v5;

	v1 = map_pages(0x10000, 3, 0);
	v2 = map_pages(0x20000, 4, 0);
	v3 = map_pages(0x30000, 5, 0);
	unmap_pages(v2, 2, 0);
	v4 = map_pages(0x40000, 3, 0);
	v5 = map_pages(0x50000, 2, 0);

	unmap_pages(v1, 3, 0);
	unmap_pages(v2, 4, 0);
	unmap_pages(v3, 5, 0);
	unmap_pages(v4, 3, 0);
	unmap_pages(v5, 2, 0);

	page_free(p1);
	page_free(p2);
	page_free(p3);

	kprintf("valloc_test>EXIT\n");
}
