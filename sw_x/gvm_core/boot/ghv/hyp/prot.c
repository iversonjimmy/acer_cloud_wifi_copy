#include "hyp.h"
#include "ept.h"
#include "vmx.h"
#include "gpt.h"
#include "vtd.h"
#include "prot.h"

bool enable_nx = false;
un nx_anomaly_count = 0;

void
vm_soft_perms_sync(void)
{
	ept_soft_perms_sync();
}

ret_t
vm_protect_page(paddr_t gpaddr, un prot, un flags)
{
	if (flags & ~VMPF_ALL) {
		return -EINVAL;
	}
	ret_t ret = ept_protect_page(gpaddr, prot,
				     flags & (EPTF_ALL));
	if (ret) {
		return ret;
	}
	if (flags & VMPF_FLUSH) {
		vtd_invalidate_page(gpaddr);
	}

	return ret;
}

void
vm_flush(void)
{
	ept_invalidate();
	vtd_invalidate();
}

ret_t
protect_memory_guest_vaddr(addr_t gvaddr, size_t len, un prot, un vmp_flags)
{
	kprintf("protect_memory_guest_vaddr>gvaddr 0x%lx len 0x%lx\n",
		gvaddr, len);
	if (gvaddr != TRUNC_PAGE(gvaddr)) {
		return -EINVAL;
	}
	len = ROUND_PAGE(len);

	vm_t *v = &cpu_vm[cpuno()];
	bool guest64 = v->v_guest_64;
	bool pae = v->v_guest_pae;
	gpte_t pte;

	addr_t end_addr = gvaddr + len;

	for (addr_t addr = gvaddr; addr < end_addr; addr += VM_PAGE_SIZE) {
		ret_t large = gvaddr_to_gpte(v, addr, &pte);
		if (large < 0) {
			kprintf("protect_memory_guest_vaddr>gvaddr_to_gpte() "
				"returned %ld\n", large);
			return large;
		}
		u8 page_shift = guest_page_shift(large, guest64, pae);
		un page_offset = bits(gvaddr, page_shift - 1, 0);

		paddr_t paddr = gpte_to_phys(pte, large, guest64, pae);

		ret_t ret = vm_protect_page(paddr + page_offset, prot,
					    vmp_flags);
		if (ret) {
			kprintf("protect_memory_guest_vaddr>failed %ld\n", ret);
			return ret;
		}
	}

	return 0;
}

ret_t
protect_memory_vaddr(addr_t vaddr, size_t len, un prot, un vmp_flags)
{
	kprintf("protect_memory_vaddr>vaddr 0x%lx len 0x%lx\n",
		vaddr, len);
	if (vaddr != TRUNC_PAGE(vaddr)) {
		return -EINVAL;
	}
	len = ROUND_PAGE(len);

	gpte_t pte;

	addr_t end_addr = vaddr + len;

	for (addr_t addr = vaddr; addr < end_addr; addr += VM_PAGE_SIZE) {
		ret_t large = hvaddr_to_pte(addr, &pte);
		if (large < 0) {
			kprintf("protect_memory_vaddr>vaddr_to_pte() "
				"returned %ld\n", large);
			return large;
		}
		u8 page_shift = guest_page_shift(large, HOST_64, HOST_PAE);
		un page_offset = bits(vaddr, page_shift - 1, 0);

		paddr_t paddr = gpte_to_phys(pte, large, HOST_64, HOST_PAE);

		ret_t ret = vm_protect_page(paddr + page_offset, prot,
					    vmp_flags);
		if (ret) {
			kprintf("protect_memory_vaddr>failed %ld\n", ret);
			return ret;
		}
	}

	return 0;
}

ret_t
protect_memory_phys_addr(paddr_t paddr, size_t len, un prot, un vmp_flags)
{
	kprintf("protect_memory_phys_addr>paddr " PFMT " len 0x%lx\n",
		paddr, len);
	if (paddr != TRUNC_PAGE(paddr)) {
		return -EINVAL;
	}
	len = ROUND_PAGE(len);

	paddr_t end_addr = paddr + len;

	for (paddr_t addr = paddr; addr < end_addr; addr += VM_PAGE_SIZE) {
		ret_t ret = vm_protect_page(addr, prot, vmp_flags);
		if (ret) {
			kprintf("protect_memory_phys_addr>failed %ld\n", ret);
			return ret;
		}
	}

	return 0;
}

void
protect_hypervisor_memory(void)
{
	/* Protect the dynamic free page list */
	for (un i = 0; i < bp->bp_nfreepages; i++) {
		protect_memory_phys_addr(bp->bp_freepages[i],
					 VM_LARGE_PAGE_SIZE,
					 EPT_PERM_NONE,
					 VMPF_HYP);
	}

	/* Protect the static text/data/bss */
#define STATIC_PROT	EPT_PERM_NONE
	if (bp->bp_hyp_addr == 0) {
		kprintf("protect_hypervisor_memory>bp->bp_hyp_addr == 0\n");
		return;
	}

	if (bp->bp_flags & BPF_HYP_ADDR_GVIRTUAL) {
		protect_memory_guest_vaddr(bp->bp_hyp_addr,
					   bp->bp_hyp_len,
					   STATIC_PROT,
					   VMPF_HYP);
	} else if (bp->bp_flags & BPF_HYP_ADDR_VIRTUAL) {
		protect_memory_vaddr(bp->bp_hyp_addr,
				     bp->bp_hyp_len,
				     STATIC_PROT,
				     VMPF_HYP);
	} else {
		protect_memory_phys_addr(bp->bp_hyp_addr,
					 bp->bp_hyp_len,
					 STATIC_PROT,
					 VMPF_HYP);
	}

	vm_flush();
}

bool
vm_check_protection(paddr_t paddr, size_t len, un prot)
{
	// Don't bother if ept is not enabled
	if (cpu_is_AMD() || !ept_enabled()) {
		return true;
	}

	size_t offset = (paddr & VM_PAGE_OFFSET);
	paddr = TRUNC_PAGE(paddr);
	len = ROUND_PAGE(len + offset);
	prot = (prot & EPT_PERM_ALL);

	paddr_t end_addr = paddr + len;

	for (paddr_t addr = paddr; addr < end_addr; addr += VM_PAGE_SIZE) {
		epte_t epte;
		ret_t ret = gpaddr_to_epte(addr, &epte); 
		if (ret) {
			kprintf("vm_check_protection>paddr " PFMT " len 0x%lx ret %ld FALSE\n",
				paddr, len, ret);
			return false;
		}
		if (!(epte & prot)) {
			return false;
		}
	}

	return true;
}

bool
guest_phys_page_readable(u32 guest_id, paddr_t paddr)
{
	if (guest_id == 0) {
		return true;
	}

	return vm_check_protection(paddr, VM_PAGE_SIZE, EPT_PERM_R);
}

bool
guest_phys_page_writeable(u32 guest_id, paddr_t paddr)
{
	if (guest_id == 0) {
		return true;
	}

	return vm_check_protection(paddr, VM_PAGE_SIZE, EPT_PERM_RW);
}
