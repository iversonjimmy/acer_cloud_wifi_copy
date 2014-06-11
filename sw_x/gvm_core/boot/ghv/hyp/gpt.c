#include "hyp.h"
#include "kvtophys.h"
#include "pt.h"
#include "vmx.h"
#include "gpt.h"
#include "valloc.h"
#include "prot.h"

/* Guest page tables */

static gpte_t
gpte_get(void *page_table, un vaddr, un level, bool guest64, bool pae)
{
	gpte_t pte;
	un index = 0;
	assert((level >= 1) && (level <= 4));

	switch (level) {
	case 4:
		assert(guest64);
		index = bits((vaddr), 47, 39);
		break;
	case 3:
		assert(guest64);
		index = bits((vaddr), 38, 30);
		break;
	case 2:
		index = ((pae || guest64) ?
			 bits((vaddr), 29, 21) :
			 bits((vaddr), 31, 22));
		break;
	case 1:
		index = ((pae || guest64) ?
			 bits((vaddr), 20, 12) :
			 bits((vaddr), 21, 12));
		break;
	}

	if (guest64 || pae) {
		pte = ((u64 *)page_table)[index];
	} else {
		pte = ((u32 *)page_table)[index];
	}
	if (bit_test(pte, PTE_PRESENT)) {
		return pte;
	}

	return 0;
}

static void *
gpte_to_kv(u32 guest_id, gpte_t pte,
	   bool isPDE, bool guest64, bool pae, bool write_access)
{
	paddr_t paddr = gpte_to_phys(pte, isPDE, guest64, pae);

	if (write_access && !guest_phys_page_writeable(guest_id, paddr)) {
		return NULL;
	} else if (!guest_phys_page_readable(guest_id, paddr)) {
		return NULL;
	}

	if (paddr <= bp->bp_max_low_page) {
		return (void *)phystokv(paddr);
	} else if (isPDE && bit_test(pte, PDE_PAGESIZE)) {
		/* NRG */
		kprintf("gpte_to_kv>map_page(paddr, MPF_LARGE) needed here\n");
		return NULL;
	}
	return (void *)map_page(paddr, 0);
}

/*
 * Returns 0 for a PTE (normal page), 1 for a PDE (large page),
 * or negative for an error.
 */
ret_t
__gvaddr_to_gpte(un vaddr, bool guest64, bool pae, gpte_t *ptep,
		 u32 guest_id, un cr3)
{
	addr_t root = PTE_TO_PAGE(cr3);
	/* NRG test for needed map_page() */
	if (!guest_phys_page_readable(guest_id, root)) {
		return -EDOM;
	}
	void *page_table = (void *)phystokv(root);
	gpte_t pte = 0;

	if (guest64) {
		pte = gpte_get(page_table, vaddr, 4, guest64, pae);
		if (!bit_test(pte, PTE_PRESENT)) {
			return -EFAULT;
		}

		page_table = gpte_to_kv(guest_id, pte,
					true, guest64, pae, false);
		if (!page_table) {
			return -EDOM;
		}
	}

	if (guest64) {
		pte = gpte_get(page_table, vaddr, 3, guest64, pae);
		unmap_page((addr_t)page_table, UMPF_LOCAL_FLUSH);
		if (!bit_test(pte, PTE_PRESENT)) {
			return -EFAULT;
		}

		page_table = gpte_to_kv(guest_id, pte,
					true, guest64, pae, false);
		if (!page_table) {
			return -EDOM;
		}
	} else if (pae) {
		int index = bits(vaddr, 31, 30);
		switch (index) {
		case 0:
			pte = vmread64(VMCE_GUEST_PDPTE0);
			break;
		case 1:
			pte = vmread64(VMCE_GUEST_PDPTE1);
			break;
		case 2:
			pte = vmread64(VMCE_GUEST_PDPTE2);
			break;
		case 3:
			pte = vmread64(VMCE_GUEST_PDPTE3);
			break;
		}
		if (!bit_test(pte, PTE_PRESENT)) {
			return -EFAULT;
		}

		page_table = gpte_to_kv(guest_id, pte,
					true, guest64, pae, false);
		if (!page_table) {
			return -EDOM;
		}
	}

	pte = gpte_get(page_table, vaddr, 2, guest64, pae);
	unmap_page((addr_t)page_table, UMPF_LOCAL_FLUSH);
	if (!bit_test(pte, PTE_PRESENT)) {
		return -EFAULT;
	}
	if (bit_test(pte, PDE_PAGESIZE)) {
		*ptep = pte;
		return 1;
	}

	page_table = gpte_to_kv(guest_id, pte, true, guest64, pae, false);
	if (!page_table) {
		return -EDOM;
	}

	*ptep = gpte_get(page_table, vaddr, 1, guest64, pae);
	unmap_page((addr_t)page_table, UMPF_LOCAL_FLUSH);

	return 0;
}

ret_t
gvaddr_to_gpte(vm_t *v, un vaddr, gpte_t *ptep)
{
	return __gvaddr_to_gpte(vaddr, v->v_guest_64, v->v_guest_pae, ptep,
				GUEST_ID, v->v_guest_cr3);
}

/* Guest 0 is the host */
ret_t
hvaddr_to_pte(un vaddr, gpte_t *ptep)
{
	return __gvaddr_to_gpte(vaddr, HOST_64, HOST_PAE, ptep, 0, host_cr3);
}

ret_t
guest_copy(void *dst, addr_t gvaddr, size_t len, bool from)
{
	vm_t *v = &cpu_vm[cpuno()];
	bool guest64 = v->v_guest_64;
	bool pae = v->v_guest_pae;
	gpte_t pte;

	while (len > 0) {
		ret_t large = gvaddr_to_gpte(v, gvaddr, &pte);
		if (large < 0) {
			kprintf("guest_copy>gvaddr_to_gpte() %ld\n",
				large);
			return large;
		}

		u8 page_shift = guest_page_shift(large, guest64, pae);
		// kprintf("guest_copy>page_shift %d\n", page_shift);
		un page_offset = bits(gvaddr, page_shift - 1, 0);
		// kprintf("guest_copy>page_offset 0x%lx\n", page_offset);
		un page_size = (1UL << page_shift);
		// kprintf("guest_copy>page_size 0x%lx\n", page_size);

		void *src_page = gpte_to_kv(GUEST_ID, pte,
					    large, guest64, pae, !from);
		if (!src_page) {
			kprintf("guest_copy>gpte_to_kv() failed\n");
			return -EDOM;
		}
		void *src = src_page + page_offset;

		un page_len = page_size - page_offset;

		un n = min(len, page_len);

#ifdef NOTDEF
		kprintf("copying %ld bytes %s 0x%p %s 0x%p\n", n,
			from ? "from" : "to", src,
			from ? "to" : "from", dst);
#endif
		if (from) {
			memcpy(dst, src, n); 
		} else {
			memcpy(src, dst, n); 
		}
		unmap_page((addr_t)src_page, UMPF_LOCAL_FLUSH);

		dst += n;
		gvaddr += n;
		len -= n;
	}

	return 0;
}

static size_t
get_line(char *buf, size_t len, char *line_buf, size_t line_len)
{
	un i;
	for (i = 0; i < len; i++) {
		if (buf[i] == '\n') {
			i++;
			break;
		}
	}

	line_len = min(line_len, i);
	memcpy(line_buf, buf, line_len);
	return line_len;
}

ret_t
gpt_test(addr_t gvaddr, size_t len)
{
#define LINE_LEN 128
	char line_buf[LINE_LEN];
	kprintf("gpt_test>gvaddr 0x%lx len 0x%lx\n", gvaddr, len);

	char *buffer = alloc(len + 1);
	if (!buffer) {
		kprintf("gpt_test>too long 0x%lx\n", len);
		return -EINVAL;
	}

	ret_t ret = copy_from_guest(buffer, gvaddr, len);
	if (ret < 0) {
		kprintf("gpt_test>copy_from_guest() failed %ld\n", ret);
		return ret;
	}

	buffer[len] = '\0'; 
	char *next = buffer;
	size_t n;

	while ((n = get_line(next, len, line_buf, LINE_LEN)) != 0) {
		if (n == LINE_LEN) {
			n--;
		}
		line_buf[n] = '\0';
		kprintf("%s", line_buf);
		next += n;
		len -= n;
	}

	free(buffer);

	return 0;
}

static char *indent[] = {
	"    ",
	"   ",
	"  ",
	" ",
	""
};

inline static addr_t
voffset(u32 level, u32 index, bool g64, bool pae)
{
	u32 bits_per_level = (g64 || pae) ? 9 : 10;
	u32 index_shift = (level - 1) * bits_per_level + VM_PAGE_SHIFT;
	addr_t offset = (addr_t)index << index_shift;
	if (g64) {
		offset = (addr_t)(((s64)(offset << 16)) >> 16);
	}
	return offset;
}

void
dump_gpt(void)
{
	kprintf("dump_gpt>ENTRY\n");
	bool g64 = guest_64();
	bool pae = guest_PAE();
	addr_t vlo = 0;
	// addr_t vhi = ~0;
	// addr_t vhi = 0xffff880000000000;
	addr_t vhi = bp->bp_virt_base;
	gpte_t pae_l3[4];

	if (pae && !g64) {
		kprintf("dump_gpt>PAE\n");
		pae_l3[0] = vmread64(VMCE_GUEST_PDPTE0);
		pae_l3[1] = vmread64(VMCE_GUEST_PDPTE1);
		pae_l3[2] = vmread64(VMCE_GUEST_PDPTE2);
		pae_l3[3] = vmread64(VMCE_GUEST_PDPTE3);

		dump_gdirectory(3, 0, pae_l3, g64, pae, vlo, vhi);
		kprintf("dump_gpt>EXIT PAE\n");
		return;
	}

	un guest_cr3 = vmread(VMCE_GUEST_CR3);
	addr_t root = PTE_TO_PAGE(guest_cr3);
	void *page_table = (void *)phystokv(root);

	dump_gdirectory((g64 ? 4 : 2), 0, page_table, g64, pae, vlo, vhi);

	kprintf("dump_gpt>EXIT\n");
}

void
dump_gdirectory(u32 level, addr_t vbase, void *page_table,
		bool g64, bool pae,
		addr_t vlo, addr_t vhi)
{
	un nptes = (pae || g64) ? 512 : 1024;
	bool zero_elide = false;
	un count = 0;

	if (!g64 && pae && (level == 3)) {
		nptes = 4;
	}

	for (u32 index = 0; index < nptes; index++) {
		gpte_t pte;
		if (pae || g64) {
			pte = ((u64 *)page_table)[index];
		} else {
			pte = ((u32 *)page_table)[index];
		}
		if (zero_elide && !bit_test(pte, PTE_PRESENT)) {
			continue;
		}
		addr_t vaddr = vbase + voffset(level, index, g64, pae);
		addr_t evaddr = vaddr + voffset(level, 1, g64, pae) - 1;
		if (vaddr >= vhi) {
			break;
		} else if (evaddr < vlo) {
			continue;
		}
		if (level > 1) {
			kprintf("%s%03d 0x%lx 0x%llx\n",
				indent[level],
				index,
				vaddr,
				pte);
			if (bit_test(pte, PTE_PRESENT) &&
			    !bit_test(pte, PDE_PAGESIZE)) {
				void *np = gpte_to_kv(GUEST_ID, pte, true,
						      g64, pae, false);
				if (!np) {
					kprintf("gpte_to_kv() failed\n");
					continue;
				}
				dump_gdirectory(level - 1, vaddr, np,
						g64, pae, vlo, vhi); 
				unmap_page((addr_t)np, UMPF_LOCAL_FLUSH);
			}
		} else {
			if (++count < 10) {
				kprintf("%s%03d 0x%lx 0x%llx\n",
					indent[level],
					index,
					vaddr,
					pte);
				// dump_pte(level, index, vaddr, pte, g64, pae);
			} else if (count == 10) {
				kprintf("%s...\n", indent[level]);
			}
		}
		zero_elide = !bit_test(pte, PTE_PRESENT);
	}
}
