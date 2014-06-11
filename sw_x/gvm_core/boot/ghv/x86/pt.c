#include "hyp.h"
#include "kvtophys.h"
#include "vmx.h"
#include "notify.h"
#include "pt.h"

pte_t *pt_root;
u32 *shadow_ident_pt;
un host_cr3;
un shadow_ident_cr3;

static char *indent[] = {
	"    ",
	"   ",
	"  ",
	" ",
	""
};

static void
dump_pde(un level, u32 index, addr_t vaddr, pte_t p)
{
	kprintf("%s%03d " VFMT " " PTEFMT " " PFMT " %s%s%s%s%s%s%s%s%s%s\n",
		indent[level],
		index,
		vaddr,
		p,
		PDE_TO_PAGE(p),
		(bit_test(p, PDE_PAGESIZE) ?
		 (bit_test(p, PDE_PAT)		? "R" : "") : ""),
		bit_test(p, PTE_GLOBAL)		? "G" : "",
		bit_test(p, PDE_PAGESIZE)	? "Z" : "",
		bit_test(p, PTE_DIRTY)		? "D" : "",
		bit_test(p, PTE_ACCESSED)	? "A" : "",
		bit_test(p, PTE_NOCACHE)	? "N" : "",
		bit_test(p, PTE_WRITETHRU)	? "T" : "",
		bit_test(p, PTE_USER)		? "U" : "",
		bit_test(p, PTE_WRITABLE)	? "W" : "",
		bit_test(p, PTE_PRESENT)	? "P" : "");
}

static void
dump_pte(un level, u32 index, addr_t vaddr, pte_t p)
{
	kprintf("%s%03d " VFMT " " PTEFMT " " PFMT " %s%s%s%s%s%s%s%s%s\n",
		indent[level],
		index,
		vaddr,
		p,
		PTE_TO_PAGE(p),
		bit_test(p, PTE_GLOBAL)		? "G" : "",
		bit_test(p, PTE_PAT)		? "R" : "",
		bit_test(p, PTE_DIRTY)		? "D" : "",
		bit_test(p, PTE_ACCESSED)	? "A" : "",
		bit_test(p, PTE_NOCACHE)	? "N" : "",
		bit_test(p, PTE_WRITETHRU)	? "T" : "",
		bit_test(p, PTE_USER)		? "U" : "",
		bit_test(p, PTE_WRITABLE)	? "W" : "",
		bit_test(p, PTE_PRESENT)	? "P" : "");
}

#ifdef __x86_64__
#define BITS_PER_LEVEL		9
#define VADDR_SIGN_EXTEND(addr)	((addr_t)((s64)((addr) << 16) >> 16))
#elif defined(HYP_PAE)
#define BITS_PER_LEVEL		9
#define VADDR_SIGN_EXTEND(addr)	(addr)
#else
#define BITS_PER_LEVEL		10
#define VADDR_SIGN_EXTEND(addr)	(addr)
#endif
#define INDEX_SHIFT(level)	(((level) - 1) * BITS_PER_LEVEL + VM_PAGE_SHIFT)
#define VOFFSET(level, index)	VADDR_SIGN_EXTEND(((addr_t)(index)) << INDEX_SHIFT(level))

void
dump_directory(un level, addr_t vbase, pte_t *page_table,
	       addr_t vlo, addr_t vhi)
{
	pte_t *begin = page_table;
	pte_t *end = begin + PTES_PER_PAGE;
#ifdef HYP_PAE
	if (level == PT_ROOT_LEVEL) {
		end = begin + PTES_PER_PAE_L3;
	}
#endif
	bool zero_elide = false;
	un count = 0;

	for (pte_t *p = begin; p < end; p++) {
		if (zero_elide && *p == 0) {
			continue;
		}
		un index = p - begin;
		addr_t vaddr = vbase + VOFFSET(level, index);
		addr_t evaddr = vaddr + (VOFFSET(level, 1) - 1);
		if (vaddr >= vhi) {
			break;
		} else if (evaddr < vlo) {
			continue;
		}
		if (level > 1) {
			pte_t pde = *p;
			dump_pde(level, index, vaddr, pde);
			if (bit_test(pde, PTE_PRESENT) &&
			    !bit_test(pde, PDE_PAGESIZE)) {
				addr_t page = PDE_TO_PAGE(pde);
				if (page > bp->bp_max_low_page) {
					kprintf("dump_directory>page high\n");
					continue;
				}
				pte_t *np = (pte_t *)phystokv(page);
				dump_directory(level - 1, vaddr, np, vlo, vhi);
			}	
		} else {
			if (++count < 10) {
				dump_pte(level, index, vaddr, *p);
			} else if (count == 10) {
				kprintf("%s...\n", indent[level]);
			}
		}
		zero_elide = (*p == 0);
	}
}

void
dump_pagetables(un cr3, addr_t begin, addr_t end)
{
	kprintf("0x%lx\n", cr3);
	paddr_t root = PTE_TO_PAGE(cr3);
	pte_t *root_table = (pte_t *)phystokv(root);
	kprintf("root_table %p\n", root_table);

	dump_directory(PT_ROOT_LEVEL, 0, root_table, begin, end);
}

void
dump_pt(addr_t begin, addr_t end)
{
	kprintf("dump_pt>ENTRY 0x%p\n", pt_root);
	if (!pt_root) {
		return;
	}

	dump_directory(PT_ROOT_LEVEL, 0, pt_root, begin, end);

	kprintf("dump_pt>EXIT\n");
}

static pte_t
pte_get(pte_t *page_table, un index, un level, bool alloc_missing)
{
	pte_t pte = page_table[index];
	if (bit_test(pte, PTE_PRESENT) || !alloc_missing) {
		return pte;
	}

	pte_t *new_table = (pte_t *)page_alloc();
	if (!new_table) {
		return 0;
	}

	un flags = PDE_FLAGS;
#ifdef HYP_PAE
	if (level == 3) {
		flags = PAE_L3_FLAGS;
	}
#endif

	pte_t new_pte = mkpde(kvtophys(new_table), flags);

	if (atomic_cx(&page_table[index], pte, new_pte) != pte) {
		page_free((un)new_table);
		pte = page_table[index];
	} else {
		pte = new_pte;
#ifdef HYP_PAE
		if (level == 3) {
			/* Ensure CPU cached copy of PDPTE is reloaded */
			for_each_os_cpu(i) {
				vm_t *v = &cpu_vm[i];
				v->v_cr3_reload_needed = true;
			}
		}
#endif
	}	

	return pte;
}

static pte_t *
pte_ptr_get(pte_t *root, un vaddr, bool large, bool alloc_missing_tables)
{
	pte_t *page_table = root;
	un index;
	pte_t pte;
#if defined(__x86_64__) || defined(HYP_PAE)
#ifdef __x86_64__
	index = l4_index(vaddr);
	pte = pte_get(page_table, index, 4, alloc_missing_tables);
	if (pte == 0) {
		return NULL;
	}

	page_table = pde_to_kv(pte);
#endif
	index = l3_index(vaddr);
	pte = pte_get(page_table, index, 3, alloc_missing_tables);
	if (pte == 0) {
		return NULL;
	}

	page_table = pde_to_kv(pte);
#endif
	index = l2_index(vaddr);
	if (!large) {
		pte = pte_get(page_table, index, 2, alloc_missing_tables);
		if (pte == 0) {
			return NULL;
		}
		if (bit_test(pte, PDE_PAGESIZE)) {
			return &page_table[index];
		}

		page_table = pde_to_kv(pte);
		index = l1_index(vaddr);
	}

	return &page_table[index];
}

void
pt_table_free(pte_t *page_table, un level)
{
	if (level > 1) {
		kprintf("pt_table_free>level=%ld page_table=0x%lx/0x%lx\n",
			level, (un)page_table, kvtophys(page_table));
		pte_t *start = page_table;
		pte_t *finish = start + PTES_PER_PAGE;
#ifdef HYP_PAE
		if (level == PT_ROOT_LEVEL) {
			finish = start + PTES_PER_PAE_L3;
		}
#endif
		for (pte_t *p = start; p < finish; p++) {
			if (bit_test(*p, PTE_PRESENT) &&
			    !bit_test(*p, PDE_PAGESIZE)) {
				pte_t *table = pde_to_kv(*p);
				pt_table_free(table, level - 1);
				*p = 0;
			}
		}
	}

	page_free((un)page_table);
}

void
pt_destroy(void)
{
	if (pt_root) {
		pt_table_free(pt_root, PT_ROOT_LEVEL);
		pt_root = NULL;
	}
	if (shadow_ident_pt) {
		page_free((addr_t)shadow_ident_pt);
		shadow_ident_pt = NULL;
	}
}

ret_t
pt_install_pte(pte_t *root, un vaddr, pte_t pte, bool large)
{
	pte_t *p = pte_ptr_get(root, vaddr, large, true);

	if (p == NULL) {
		kprintf("pt_install_pte>page_table alloc failed\n");
		return -ENOMEM;
	}

	pte_t old_pte = *p;
	if (bit_test(old_pte, PTE_PRESENT) ||
	    (atomic_cx(p, old_pte, pte) != old_pte)) {
		return -EEXIST;
	}

	return 0;
}

ret_t
pt_clear_pte(pte_t *root, un vaddr, bool large)
{
	pte_t *p = pte_ptr_get(root, vaddr, large, false);
	if (p == NULL) {
		kprintf("pt_clear_pte>vaddr not found\n");
		return -EINVAL;
	}

	pte_t old_pte = *p;
	if (!bit_test(old_pte, PTE_PRESENT)) {
		kprintf("pt_clear_pte>vaddr not found\n");
		return -EINVAL;
	}

	atomic_cx(p, old_pte, 0);

	return 0;
}

static inline ret_t
pt_construct_page(pte_t *root, un vaddr, paddr_t paddr, bool large)
{
	pte_t pte = (large ? mkpde(paddr, PDE_FLAGS_LARGE_PAGE) :
		     mkpte(paddr, PTE_FLAGS));

	return pt_install_pte(root, vaddr, pte, large);
}

static ret_t
pt_init_root(void)
{
	assert(pt_root == NULL);

	pt_root = (pte_t *)page_alloc();
	if (pt_root == NULL) {
		return -ENOMEM;
	}
#ifdef HYP_PAE
	for (un i = 0; i < PTES_PER_PAE_L3; i++) {
		if (pte_get(pt_root, i, PT_ROOT_LEVEL, true) == 0) {
			return -ENOMEM;
		}
	}
#endif
	return 0;
}

ret_t
copy_directory(un level, addr_t vbase, pte_t *page_table,
	       addr_t vlo, addr_t vhi)
{
	pte_t *begin = page_table;
	pte_t *end = begin + PTES_PER_PAGE;
#ifdef HYP_PAE
	if (level == PT_ROOT_LEVEL) {
		end = begin + PTES_PER_PAE_L3;
	}
#endif

	ret_t ret = 0;

	for (pte_t *p = begin; p < end; p++) {
		if (!bit_test(*p, PTE_PRESENT)) {
			continue;
		}
		addr_t vaddr = vbase + VOFFSET(level, p - begin);
		addr_t evaddr = vaddr + (VOFFSET(level, 1) - 1);
		if (vaddr >= vhi) {
			break;
		} else if (evaddr < vlo) {
			continue;
		}
		if (level > 1) {
			pte_t pde = *p;
			if (bit_test(pde, PDE_PAGESIZE)) {
				ret = pt_install_pte(pt_root, vaddr, pde, true);
			} else {
				paddr_t page = PDE_TO_PAGE(pde);
				if (page > bp->bp_max_low_page) {
					kprintf("copy_directory>page high\n");
					continue;
				}
				pte_t *np = (pte_t *)phystokv(page);
				ret = copy_directory(level - 1, vaddr, np,
						     vlo, vhi);
			}	
		} else {
			ret = pt_install_pte(pt_root, vaddr, *p, false);
		}
		if (ret) {
			break;
		}
	}

	return ret;
}

static void
pt_compare_pte(addr_t vaddr, pte_t pte)
{
	pte_t *ptr = pte_ptr_get(pt_root, vaddr, false, false);
	if (!ptr) {
		kprintf("vaddr " VFMT " old " PTEFMT " new missing\n",
		       vaddr, pte);
		return;
	}

	pte_t pte1 = *ptr;

	bit_clear(pte, PTE_ACCESSED);
	bit_clear(pte, PTE_DIRTY);
	bit_clear(pte1, PTE_ACCESSED);
	bit_clear(pte1, PTE_DIRTY);

	if (pte != pte1) {
		kprintf("vaddr " VFMT " old " PTEFMT " != new " PTEFMT "\n",
		       vaddr, pte, pte1);
	}
}

void
compare_directory(un level, addr_t vbase, pte_t *page_table,
		  addr_t vlo, addr_t vhi)
{
	pte_t *begin = page_table;
	pte_t *end = begin + PTES_PER_PAGE;
#ifdef HYP_PAE
	if (level == PT_ROOT_LEVEL) {
		end = begin + PTES_PER_PAE_L3;
	}
#endif

	for (pte_t *p = begin; p < end; p++) {
		if (!bit_test(*p, PTE_PRESENT)) {
			continue;
		}
		addr_t vaddr = vbase + VOFFSET(level, p - begin);
		addr_t evaddr = vaddr + (VOFFSET(level, 1) - 1);
		if (vaddr >= vhi) {
			break;
		} else if (evaddr < vlo) {
			continue;
		}
		if (level > 1) {
			pte_t pde = *p;
			if (bit_test(pde, PDE_PAGESIZE)) {
				pt_compare_pte(vaddr, pde);
			} else {
				paddr_t page = PDE_TO_PAGE(pde);
				if (page > bp->bp_max_low_page) {
					kprintf("compare_directory>page high\n");
					continue;
				}
				pte_t *np = (pte_t *)phystokv(page);
				compare_directory(level - 1, vaddr, np,
						  vlo, vhi);
			}	
		} else {
			pt_compare_pte(vaddr, *p);
		}
	}
}

ret_t
copy_pagetables(void)
{
	un cr3 = get_CR3();
	kprintf("current cr3 0x%lx\n", cr3);

	addr_t root = PTE_TO_PAGE(cr3);
	kprintf("phys root 0x%lx\n", root);
	pte_t *root_table = (pte_t *)phystokv(root);
	kprintf("root_table %p\n", root_table);

	assert(pt_root);

	ret_t ret = copy_directory(PT_ROOT_LEVEL, 0, root_table,
				   bp->bp_virt_base, ~0UL);
	kprintf("copy_pagetables>copy_directory ret %ld\n", ret);
	compare_directory(PT_ROOT_LEVEL, 0, root_table,
			  bp->bp_virt_base, ~0UL);
	kprintf("copy_pagetables>compare_directory finished\n");

	kprintf("copy_pagetables>copying lowmem for standalone hv\n");
	ret = copy_directory(PT_ROOT_LEVEL, 0, root_table,
			     KV_OFFSET, KV_OFFSET + bp->bp_max_low_page - 1);
	kprintf("copy_pagetables>copy_directory ret %ld\n", ret);
	compare_directory(PT_ROOT_LEVEL, 0, root_table,
			  KV_OFFSET, KV_OFFSET + bp->bp_max_low_page - 1);
	kprintf("copy_pagetables>compare_directory finished\n");
	return ret;
}

ret_t
pt_construct(void)
{
	kprintf("pt_construct>ENTER\n");

	ret_t ret = pt_init_root();
	if (ret) {
		kprintf("pt_construct>pt_init_root() failed %ld\n", ret);
		return ret;
	}

	ret = copy_pagetables();
	if (ret < 0) {
		kprintf("pt_construct>copy_pagetables() failed %ld\n", ret);
		return ret;
	}


	host_cr3 = mkcr3(kvtophys(pt_root), 0);

	shadow_ident_pt = (void *)page_alloc();
	if (shadow_ident_pt == NULL) {
		return -ENOMEM;
	}

	for (u32 i = 0; i < VM_PAGE_SIZE / sizeof(u32); i++) {
		const u32 flags = PDE_FLAGS_LARGE_PAGE;
		shadow_ident_pt[i] = (i << VM_4M_PAGE_SHIFT) | flags;
	}

	shadow_ident_cr3 = mkcr3(kvtophys(shadow_ident_pt), 0);

	kprintf("pt_construct>EXIT host_cr3 0x%lx shadow_ident_cr3 0x%lx\n",
		host_cr3, shadow_ident_cr3);
	return 0;
}

#ifdef __x86_64__
#define TEST_VADDR 0xffff800000000000UL
#define TEST_LIMIT 0xffff880000100000UL
#endif

void
pt_test(void)
{
}

void
__tlb_flush(vm_t *v, addr_t vaddr)
{
	tlb_flush_local(vaddr);
}

void
tlb_flush(addr_t vaddr)
{
	notify_all((nb_func_t)__tlb_flush, (nb_arg_t)vaddr, (nb_arg_t)0);
}

void
__tlb_flush_all(vm_t *v)
{
	tlb_flush_all_local();
}

void
tlb_flush_all(void)
{
	notify_all((nb_func_t)__tlb_flush_all, (nb_arg_t)0, (nb_arg_t)0);
}
