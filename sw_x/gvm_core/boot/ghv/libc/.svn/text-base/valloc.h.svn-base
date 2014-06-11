#ifndef __VALLOC_H__
#define __VALLOC_H__

#define FREE_MAP_NBITS 1024

extern addr_t base_vaddr;

#define end_vaddr (base_vaddr + (FREE_MAP_NBITS * VM_PAGE_SIZE))

inline static bool
is_mapped_addr(addr_t vaddr)
{
	return ((vaddr >= base_vaddr) && (vaddr < end_vaddr));
}

#define MPF_LARGE		0x01
#define MPF_IO			0x02
#define MPF_MASK		(MPF_LARGE|MPF_IO)

#define UMPF_NO_FLUSH		0x01
#define UMPF_LOCAL_FLUSH	0x02
#define UMPF_MASK		(UMPF_NO_FLUSH|UMPF_LOCAL_FLUSH)

extern ret_t init_valloc(void);
extern void fin_valloc(void);
extern addr_t map_page(paddr_t __paddr, un __mp_flags);
extern void unmap_page(addr_t __vaddr, un __ump_flags);
extern addr_t map_pages(paddr_t __paddr, un __npages, un __mp_flags);
extern void unmap_pages(addr_t __vaddr, un __npages, un __ump_flags);
#endif
