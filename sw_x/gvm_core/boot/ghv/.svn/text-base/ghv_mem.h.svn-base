#ifndef __GHV_MEM_H__
#define __GHV_MEM_H__

#define NOMAP ~0UL

void ghv_map(addr_t va, paddr_t pa);
paddr_t ghv_unmap(addr_t va);

void *alloc_stack(int cpuno, u32 size, bool interrupt);
void free_stack(void *stack);

void ghv_pagetable_fixup(un cr3);
paddr_t virt_to_phys(addr_t);

void ghv_map_to_guest(un guest_cr3);

#endif
