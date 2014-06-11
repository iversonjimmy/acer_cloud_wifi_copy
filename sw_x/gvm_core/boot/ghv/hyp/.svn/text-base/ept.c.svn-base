#include "hyp.h"
#include "kvtophys.h"
#include "vmx.h"
#include "ept.h"
#include "vtd.h"
#include "notify.h"
#include "apic.h"
#include "prot.h"

bool __cpu_supports_ept;
bool __cpu_supports_vpid;

u32 epte_user_exec_except_cnt;
u32 epte_kern_exec_except_cnt;
u32 epte_user_write_except_cnt;
u32 epte_kern_write_except_cnt;

static epte_t
epte_get(epte_t *page_table, un index, bool alloc_missing)
{
	epte_t epte = page_table[index];
	if (bit_test(epte, EPT_VALID) || !alloc_missing) {
		return epte;
	}

	epte_t *new_table = (epte_t *)page_alloc();
	if (!new_table) {
		return 0;
	}

	epte_t new_epte = mkepte(kvtophys(new_table), EPT_PERM_ALL);

	if (atomic_cx(&page_table[index], epte, new_epte) != epte) {
		page_free((un)new_table);
		epte = page_table[index];
	} else {
		epte = new_epte;
		iommu_flush_cache((un)new_table, VM_PAGE_SIZE);
		iommu_flush_cache((un)&page_table[index], sizeof(epte_t));
	}	

	return epte;
}

static epte_t *
epte_ptr_get(paddr_t gpaddr, bool alloc_missing_tables)
{
	epte_t *page_table = ept_root;
	un index = el4_index(gpaddr);
	epte_t epte = epte_get(page_table, index, alloc_missing_tables);
	if (epte == 0) {
		return NULL;
	}

	page_table = epte_to_kv(epte);
	index = el3_index(gpaddr);
	epte = epte_get(page_table, index, alloc_missing_tables);
	if (epte == 0) {
		return NULL;
	}

	page_table = epte_to_kv(epte);
	index = el2_index(gpaddr);
	epte = epte_get(page_table, index, alloc_missing_tables);
	if (epte == 0) {
		return NULL;
	}

	page_table = epte_to_kv(epte);
	index = el1_index(gpaddr);

	return &page_table[index];
}

void
ept_invalidate_global(void)
{
	if (cpu_supports_ept()) {
		invept(INVEPT_TYPE_GLOBAL, 0);
	}
}

static void
#ifdef HYP_PAE
__ept_invalidate_addr(vm_t *v, u32 gpaddr0, u32 gpaddr1)
{
	paddr_t gpaddr = ((paddr_t)gpaddr1 << 32) | gpaddr0;
#else
__ept_invalidate_addr(vm_t *v, paddr_t gpaddr)
{
#endif
	/* There is currently no option to invept to invalidate
	 * a particular page, so gpaddr is ignored */
	u64 eptp = vmread64(VMCE_EPT_PTR);
	un err = invept(INVEPT_TYPE_SINGLE, eptp);
	if (err) {
		kprintf("__ept_invalidate_addr>ERROR eptp 0x%llx\n", eptp);
		return;
	}
	if (TRUNC_PAGE(gpaddr) == kvtophys(dma_test_page)) {
	  kprintf("__ept_invalidate_addr>succeeded gpaddr ="PFMT"\n",
		  gpaddr);
	}
}

void
ept_invalidate_addr(paddr_t gpaddr)
{
	/* See Vol3B 24.3.3 */
	if (TRUNC_PAGE(gpaddr) == kvtophys(dma_test_page)) {
		kprintf("ept_invalidate_addr>gpaddr = "PFMT"\n",
			gpaddr);
	}

#ifdef HYP_PAE
	notify_all((nb_func_t)__ept_invalidate_addr,
		   (nb_arg_t)bits(gpaddr, 31, 0),
		   (nb_arg_t)bits(gpaddr, 63, 32));
#else
	notify_all((nb_func_t)__ept_invalidate_addr,
		   (nb_arg_t)gpaddr, (nb_arg_t)0);
#endif
}

ret_t
ept_protect_page(paddr_t gpaddr, un prot, un flags)
{
	if (!cpu_supports_ept()) {
		return -ENXIO;
	}
	if (prot & ~EPT_PERM_ALL) {
		return -EINVAL;
	}

	if ((prot == EPT_PERM_W) || (prot == EPT_PERM_WX)) {
		return -EINVAL;
	}

	if (prot == EPT_PERM_X) {
		u64 msr = get_MSR(MSR_IA32_VMX_EPT_VPID_CAP);
		if (!bit_test(msr, 0)) {
			return -EINVAL;
		}
	}
	if (flags & ~EPTF_ALL) {
		return -EINVAL;
	}

	epte_t *ptr = epte_ptr_get(gpaddr, true);
	if (!ptr) {
		return -ENOMEM;
	}

	epte_t old_epte = *ptr;
	epte_t new_epte;
	if (!bit_test(old_epte, EPT_VALID)) {
		u8 mt = mtrr_type(gpaddr);
		new_epte = mkepte(gpaddr, prot|(mt << EPT_MT_SHIFT));  
	} else {
		new_epte = (old_epte & ~((epte_t)EPT_PERM_ALL)) | prot;
	}
	if (flags & EPTF_SOFT) {
		new_epte = ((new_epte & ~((epte_t)EPT_SOFT_ALL)) |
			    ((epte_t)prot << EPT_R_SOFT));
	}
	if (flags & EPTF_W_UNTIL_X) {
		bit_set(new_epte, EPT_W_UNTIL_X);
	} else {
		bit_clear(new_epte, EPT_W_UNTIL_X);
	}
	if (flags & EPTF_HYP) {
		bit_set(new_epte, EPT_HYP);
		bit_clear(new_epte, EPT_GUEST);
	}
	if (flags & EPTF_GUEST) {
		if (bit_test(new_epte, EPT_HYP)) {
			kprintf("ept_protect_page>attempt to set GUEST flag "
				"on HYP page\n");
			return -EINVAL;
		}
		bit_set(new_epte, EPT_GUEST);
	}
	epte_t result = atomic_cx(ptr, old_epte, new_epte);
	if (result == new_epte) {
#ifdef NOTDEF
		kprintf("ept_protect_page>result == new_epte\n");
#endif
		/* Someone else beat us to it.  Nothing to do. */
		return 0;
	}
	if (result != old_epte) {
		kprintf("ept_protect_page>result != old_epte\n");
		kprintf("ept_protect_page>old %08lx%08lx new %08lx%08lx "
			"res %08lx%08lx\n",
			bits(old_epte, 63, 32), bits(old_epte, 31, 0),
			bits(new_epte, 63, 32), bits(new_epte, 31, 0),
			bits(result, 63, 32), bits(result, 31, 0));
		/* Someone else did something different. */
		return -EAGAIN;
	}
	iommu_flush_cache((un)ptr, sizeof(epte_t));
	if (flags & EPTF_FLUSH) {
		ept_invalidate_addr(gpaddr);
	}
	return 0;
}

bool
vm_exit_ept_nx(registers_t *regs, u64 gpaddr, epte_t *p, un xq)
{
	epte_t epte = *p;
	char *modestr = guest_usermode() ? "usermode" : "kernelmode";

	if (bit_test(xq, EPT_XQ_ACCESS_EXECUTE) &&
	    !bit_test(xq, EPT_XQ_PERM_EXECUTE)) {
		un prot = epte & EPT_PERM_ALL;
		if (bit_test(epte, EPT_X_SOFT)) {
			bit_set(prot, EPT_X);
		} else if (bit_test(epte, EPT_W_UNTIL_X)) {
			bit_set(prot, EPT_X);
			bit_clear(prot, EPT_W);
		} else {
			un n;
			if (((n = atomic_inc(&nx_anomaly_count)) < 10) ||
			    (n % 100 == 0)) {
				kprintf("vm_exit_ept_%s>exec attempt %ld "
					"but no exec permission\n", modestr, n);
				kprintf("vm_exit_ept_%s>epte = 0x%llx\n",
					modestr, epte);
			}
#ifndef NOTDEF
			vm_entry_inject_exception(VEC_GP, 0);
#else
			if (n > 1000) {
				vm_disable_nx();
			}
			bit_set(prot, EPT_X);
#endif
			/* collect stats */
			if ( guest_usermode() ) {
				atomic_inc(&epte_user_exec_except_cnt);
			}
			else {
				atomic_inc(&epte_kern_exec_except_cnt);
			}
		}
		if (!bit_test(epte, EPT_W_SOFT)) {
			/* Not yet.  Need a hook to set W again when no longer
			 * executable.  Catching write fault won't work if the
			 * first write is a DMA write */
			bit_clear(prot, EPT_W);
		}
		ret_t ret = vm_protect_page(gpaddr, prot, VMPF_SOFT|VMPF_FLUSH);
		if (ret) {
			kprintf("vm_exit_ept_%s>vm_protect_page"
				"(0x%llx, 0x%lx) returned %ld\n", modestr,
				gpaddr, prot, ret);
			return false;
		}
	} else if (bit_test(xq, EPT_XQ_ACCESS_WRITE) &&
	    !bit_test(xq, EPT_XQ_PERM_WRITE)) {
		un prot = epte & EPT_PERM_ALL;
		un vmp_flags = VMPF_FLUSH;
		if (bit_test(epte, EPT_W_SOFT)) {
			bit_set(prot, EPT_W);
		} else {
#ifdef NOTDEF
			static un count = 0;
			un n;
			if (((n = atomic_inc(&count)) < 10) || (n % 100 == 0)) {
				kprintf("vm_exit_ept_nx>write attempt %ld "
					"but no write permission\n", n);
				kprintf("vm_exit_ept_nx>epte = 0x%llx\n",
					epte);
			}
#endif
			prot = EPT_PERM_RW;
			vmp_flags |= VMPF_SOFT;

			/* collect stats */
			if ( guest_usermode() ) {
				atomic_inc(&epte_user_write_except_cnt);
			}
			else {
				atomic_inc(&epte_kern_write_except_cnt);
			}
		}
		ret_t ret = vm_protect_page(gpaddr, prot, vmp_flags);
		if (ret) {
			kprintf("vm_exit_ept_%s>vm_protect_page"
				"(0x%llx, 0x%lx) returned %ld\n", modestr,
				gpaddr, prot, ret);
			return false;
		}
	}

	return true;
}

bool
vm_exit_ept(registers_t *regs)
{
	u64 gpaddr = vmread64(VMCE_GUEST_PHYS_ADDR); 
	un xq = vmread(VMCE_EXIT_QUALIFICATION);
	un gladdr = vmread(VMCE_GUEST_LINEAR_ADDR);

	bool dump = false;

	if (TRUNC_PAGE(gpaddr) == kvtophys(dma_test_page)) {
		dump = true;
	}

	epte_t *p = epte_ptr_get(gpaddr, false);
	if ((p == NULL) || (!bit_test(*p, EPT_VALID))) {
		u8 mt = mtrr_type(gpaddr);
		if (mt != MT_UC) {
			kprintf("vm_exit_ept>attempted access "
				"to unmapped, non IO page 0x%llx, MT %d\n",
				gpaddr, mt);
			goto protection_violation;
		}

		/* This is a MMIO page that hasn't yet
		 * been set up.
		 */
		epte_t epte = mkepte(gpaddr, EPT_PERM_RW|(mt << EPT_MT_SHIFT));

		if (p == NULL) {
			p = epte_ptr_get(gpaddr, true);
		}
		if (p == NULL) {
			kprintf("vm_exit_ept>page_table alloc failed\n");
			vmx_clear_exec_cpu2(VMEXEC_CPU2_ENABLE_EPT);
			return false;
		}
		epte_t old_epte = *p;
		epte_t result = atomic_cx(p, old_epte, epte);
		if (result == old_epte && result != epte) {
			/* Update succeeded, so flush needed */
			iommu_flush_cache((un)p, sizeof(epte_t));
		}
		return true;
	}

	epte_t old_epte = *p;
	assert(bit_test(old_epte, EPT_VALID));

	if (bit_test(old_epte, EPT_HYP)) {

		kprintf("vm_exit_ept>attempted access "
			"to hyp page 0x%llx\n", gpaddr);
		goto protection_violation;
	}

	if (!bit_test(old_epte, EPT_GUEST)) {
		kprintf("vm_exit_ept>attempted access "
			"to non-guest page 0x%llx\n", gpaddr);
		goto protection_violation;
	}

	if (vm_nx_is_enabled()) {
		return vm_exit_ept_nx(regs, gpaddr, p, xq);
	}

	if (bit_test(xq, EPT_XQ_ACCESS_EXECUTE) &&
	    !bit_test(xq, EPT_XQ_PERM_EXECUTE)) {
		epte_t epte = *p;
		un prot = epte & EPT_PERM_ALL;
		bit_set(prot, EPT_X);
		if (vm_nx_is_enabled()) {
			/* Not yet.  Need a hook to set W again when no longer
			 * executable.  Catching write fault won't work if the
			 * first write is a DMA write */
			bit_clear(prot, EPT_W);
		}
		ret_t ret = vm_protect_page(gpaddr, prot, VMPF_FLUSH);
		if (ret) {
			kprintf("vm_exit_ept>vm_protect_page(0x%llx, 0x%lx) "
				"returned %ld\n",
				gpaddr, prot, ret);
			return false;
		}
		return true;
	} else if (bit_test(xq, EPT_XQ_ACCESS_WRITE) &&
	    !bit_test(xq, EPT_XQ_PERM_WRITE)) {
#ifdef NOTDEF
		epte_t epte = *p;
		static un count = 0;
		un n;
		if (((n = atomic_inc(&count)) < 5) || (n % 100 == 0)) {
			kprintf("vm_exit_ept>write attempt %ld "
				"but no write permission\n", n);
			kprintf("vm_exit_ept>epte = 0x%llx\n", epte);
			dump = true;
		}
#endif
		ret_t ret = vm_protect_page(gpaddr, EPT_PERM_RW, VMPF_FLUSH);
		if (ret) {
			kprintf("vm_exit_ept>vm_protect_page(0x%llx, 0x%lx) "
				"returned %ld\n",
				gpaddr, (un)EPT_PERM_RW, ret);
			return false;
		}
		return true;
	}

protection_violation:
#ifdef NOTDEF
	vmx_clear_exec_cpu2(VMEXEC_CPU2_ENABLE_EPT);
#else
	vm_entry_inject_exception(VEC_GP, 0);
#endif
	dump = true;

	if (dump) {
		kprintf("vm_exit_ept>access type %s%s%s\n",
			bit_test(xq, EPT_XQ_ACCESS_READ) ? "R" : "",
			bit_test(xq, EPT_XQ_ACCESS_WRITE) ? "W" : "",
			bit_test(xq, EPT_XQ_ACCESS_EXECUTE) ? "X" : "");
		kprintf("vm_exit_ept>permission  %s%s%s\n",
			bit_test(xq, EPT_XQ_PERM_READ) ? "R" : "",
			bit_test(xq, EPT_XQ_PERM_WRITE) ? "W" : "",
			bit_test(xq, EPT_XQ_PERM_EXECUTE) ? "X" : "");

		if (bit_test(xq, EPT_XQ_GUEST_LADDR_VALID)) {
			kprintf("vm_exit_ept>guest linear address 0x%lx\n",
				gladdr);
			if (bit_test(xq, EPT_XQ_NOT_PT_ACCESS)) {
				kprintf("vm_exit_ept>"
					"access to guest physical address\n");

			} else {
				kprintf("vm_exit_ept>access to page table\n");
			}
		}
		kprintf("vm_exit_ept>guest physical address 0x%llx\n", gpaddr);
	}

	return true;
}

#if 0
// This is sample code
void
ept_table_walk(epte_t *page_table, un level)
{
	if (level > 1) {
		kprintf("ept_table_walk>level=%ld "
			"page_table=0x%lx/0x%lx\n",
			level, (un)page_table, kvtophys(page_table));
	}
	epte_t *start = page_table;
	epte_t *finish = start + 512;
	for (epte_t *p = start; p < finish; p++) {
		if (!bit_test(*p, EPT_VALID)) {
			continue;
		}
		if ((level > 1) && !bit_test(*p, EPT_2MB_PAGE)) {
			epte_t *table = epte_to_kv(*p);
			ept_table_walk(table, level - 1);
		} else {
			// function(*p);
		}
	}
}
#endif // 0

static void
_ept_soft_perms_sync_walk(epte_t *page_table, paddr_t gpaddr, un level,
			  u32 *good_cnt, u32 *bad_cnt)
{
	un vmp_flags = VMPF_FLUSH | VMPF_SOFT;
	paddr_t ngpaddr;
	ret_t ret;

	epte_t *start = page_table;
	epte_t *finish = start + 512;
	for (epte_t *p = start; p < finish; p++) {
		if (!bit_test(*p, EPT_VALID)) {
			continue;
		}
		if ((level > 1) && !bit_test(*p, EPT_2MB_PAGE)) {
			epte_t *table = epte_to_kv(*p);
			ngpaddr = (gpaddr << 9) | ((p - start) << 12);
			_ept_soft_perms_sync_walk(table, ngpaddr, level - 1,
						  good_cnt, bad_cnt);
			continue;
		} 

		if ( bit_test(*p, EPT_X_SOFT) ) {
			*good_cnt += 1;
			continue;
		}
		if ( !bit_test(*p, EPT_X) ) {
			continue;
		}
		*bad_cnt += 1;
		ngpaddr = (gpaddr << 9) | ((p - start) << 12);

		// We currently don't support a read-only page. It's
		// either RX, RW or RWX. So, if it's not set to X we
		// make it RW.
		ret = vm_protect_page(ngpaddr, EPT_PERM_RW, vmp_flags);
		if ( ret ) {
			kprintf("_ept_soft_perms_sync_walk:"
				"vm_protect_page returned %ld\n", ret);
		}
	}
}

// This function is called when memory protection is turned on to
// remove any execute permissions from pages that weren't explicity
// set to be executable. i.e., make the EPT permissions match the soft
// permissions.
void
ept_soft_perms_sync(void)
{
	u32 good_cnt;
	u32 bad_cnt;

	if (ept_root == NULL) {
		return;
	}

	good_cnt = bad_cnt = 0;

	_ept_soft_perms_sync_walk(ept_root, 0, 4, &good_cnt, &bad_cnt);

	kprintf("ept_soft_perms_sync: good %d bad %d\n", good_cnt, bad_cnt);
}

void
ept_table_free(epte_t *page_table, un level)
{
	if (level > 1) {
		kprintf("ept_table_free>level=%ld page_table=0x%lx/0x%lx\n",
			level, (un)page_table, kvtophys(page_table));
		epte_t *start = page_table;
		epte_t *finish = start + 512;
		for (epte_t *p = start; p < finish; p++) {
			if (bit_test(*p, EPT_VALID) &&
			    !bit_test(*p, EPT_2MB_PAGE)) {
				epte_t *table = epte_to_kv(*p);
				ept_table_free(table, level - 1);
				*p = 0;
			}
		}
	}

	page_free((un)page_table);
}

void
ept_destroy(void)
{
	if (ept_root == NULL) {
		return;
	}

	ept_table_free(ept_root, 4);
	ept_root = NULL;
}

static ret_t
ept_construct_page(un gpaddr)
{
	u8 mt = mtrr_type(gpaddr);
	epte_t epte = mkepte(gpaddr, EPT_PERM_RW|(mt << EPT_MT_SHIFT));
	epte_t *p = epte_ptr_get(gpaddr, true);

	if (p == NULL) {
		kprintf("ept_construct_page>page_table alloc failed\n");
		return -ENOMEM;
	}

	epte_t old_epte = *p;
	if (!bit_test(old_epte, EPT_VALID)) {
		atomic_cx(p, old_epte, epte); 
		iommu_flush_cache((un)p, sizeof(epte_t));
	}

	return 0;
}

ret_t
ept_construct(void)
{
	kprintf("ept_construct>ENTER\n");

	__cpu_supports_ept = vmx_exec_cpu2_supported(VMEXEC_CPU2_ENABLE_EPT);
	__cpu_supports_vpid = vmx_exec_cpu2_supported(VMEXEC_CPU2_ENABLE_VPID);
	if (!cpu_supports_ept()) {
		kprintf("ept_construct>EPT is not supported by this CPU\n");
		if (bp->bp_flags & BPF_NO_EPT) {
			return 0;
		}
		return -ENXIO;
	}

	u64 msr = get_MSR(MSR_IA32_VMX_EPT_VPID_CAP);
	if (bit_test(msr, 0)) {
		kprintf("ept_construct>Execute-only translation is supported\n");
	}
	if (bit_test(msr, 32)) {
		kprintf("ept_construct>INVVPID is supported\n");
	}
	if (bit_test(msr, 40)) {
		kprintf("ept_construct>ADDRESS INVVPID is supported\n");
	}
	if (bit_test(msr, 41)) {
		kprintf("ept_construct>SINGLE INVVPID is supported\n");
	}
	if (bit_test(msr, 42)) {
		kprintf("ept_construct>ALL INVVPID is supported\n");
	}
	if (bit_test(msr, 43)) {
		kprintf("ept_construct>SINGLE notGLOBALS INVVPID is supported\n");
	}

	assert(ept_root == NULL);

	ept_root = (epte_t *)page_alloc();
	if (ept_root == NULL) {
		return -ENOMEM;
	}

	un max_pfn = bp->num_physpages;
	for (un pfn = 0; pfn < max_pfn; pfn++) {
		if ((pfn & 0xffff) == 0) {
			kprintf("ept_construct>pfn = 0x%lx\n", pfn);
		}
		un gpaddr = pfn << VM_PAGE_SHIFT;
		ret_t ret = ept_construct_page(gpaddr);
		if (ret) {
			return ret;
		}
	}

	kprintf("ept_construct>calling ept_construct_page(0x%lx)\n",
		get_apic_base_phys());
	ret_t ret = ept_construct_page(get_apic_base_phys());
	if (ret) {
		return ret;
	}

	kprintf("ept_construct>EXIT\n");
	return 0;
}

ret_t
gpaddr_to_epte(paddr_t gpaddr, epte_t *eptep)
{
	epte_t *p = epte_ptr_get(gpaddr, false);
	if (!p) {
		return -EDOM;
	}
	*eptep = *p;

	return 0;
}
