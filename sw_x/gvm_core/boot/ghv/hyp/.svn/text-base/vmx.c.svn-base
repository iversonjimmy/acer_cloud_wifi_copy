#include "hyp.h"
#include "kvtophys.h"
#include "pt.h"
#include "dt.h"
#include "valloc.h"
#include "ept.h"
#include "apic.h"
#include "vtd.h"
#include "interrupt.h"
#include "vmx.h"
#include "notify.h"
#include "page.h"
#include "vapic.h"
#include "gpt.h"
#include "prot.h"
#include "vmx_guest_debug.h"

//Define NOHT to disallow hyperthreads to be started.
//define NOHT

#include "ghv_guest.h"
#include "ghv_mem.h"

extern void exit(int) __attribute__((noreturn));

static void vmx_load_guest_state(vm_t *v, const guest_state_t *gs);
static void vmx_load_guest_state_and_regs(vm_t *v, registers_t *regs, const guest_state_t *gs);
static void vmx_store_guest_state(const vm_t *v, const registers_t *regs, guest_state_t *gs);


static ret_t vmx_init_devices(void);

static void *
alloc_vmcs(void)
{
	static_assert(VMCS_LEN == VM_PAGE_SIZE);
	void *vmcs = (void *)page_alloc();

	if (!vmcs) {
		kprintf("alloc_vmcs>kalloc failed\n");
		return NULL;
	}

	if (bits((un)vmcs, 11, 0) != 0) {
		kprintf("alloc_vmcs>vmcs is not 4KB aligned\n");
		return NULL;
	}

	u64 basic_vmx_info = get_MSR(MSR_IA32_VMX_BASIC);
	u32 VMCS_rev_id = bits(basic_vmx_info, 31, 0);
	*(u32 *)vmcs = VMCS_rev_id;

	return vmcs;
}

static sn
vmx_alloc(vm_t *v)
{
	v->v_vmxon_region = alloc_vmcs();
	if (!v->v_vmxon_region) {
		kprintf("vmx_alloc>alloc_vmcs() returned NULL\n");
		return -ENOMEM;
	}
	v->v_guest_vmcs = alloc_vmcs();
	if (!v->v_guest_vmcs) {
		return -ENOMEM;
	}
	v->v_stack = alloc_stack(v->v_cpuno, VSTACK_LEN, false);
	if (!v->v_stack) {
		return -ENOMEM;
	}
	static_assert(MSR_BITMAPS_LEN == VM_PAGE_SIZE);
	v->v_msr_bitmaps = (void *)page_alloc();
	if (!v->v_msr_bitmaps) {
		return -ENOMEM;
	}

	list_init(&v->v_notes_list);
	lock_init(&v->v_notes_lock);

	return 0;
}

static void
vmx_free(vm_t *v)
{
	page_free((addr_t)v->v_vmxon_region);	v->v_vmxon_region = NULL;
	page_free((addr_t)v->v_guest_vmcs);	v->v_guest_vmcs = NULL;
	free_stack(v->v_stack);			v->v_stack = NULL;
	page_free((addr_t)v->v_msr_bitmaps);	v->v_msr_bitmaps = NULL;
}

void
vmx_free_all(void)
{
	for_each_os_cpu(i) {
		vmx_free(&cpu_vm[i]);
		gdt_destroy(&hyp_gdt[i]);
		tss_destroy(&hyp_tss[i]);
		free_stack(hyp_istacks[i]);
	}

	page_free(dma_test_page);
	dma_test_page = 0;
	ept_destroy();

	pt_destroy();
	idt_destroy();
}

extern void dump_x86(void);
ret_t
vmx_alloc_all(void)
{
	ret_t ret;

	extern void vmx_on(void);
	extern void vmx_off(void);
	extern void vmx_guest_init(void);
	extern void vmx_free_all(void);

	bp->bp_vm_on = vmx_on;
	bp->bp_vm_off = vmx_off;
	bp->bp_vm_guest_init = vmx_guest_init;
	bp->bp_vm_free_all = vmx_free_all;

	kprintf("vmx_alloc_all>first kprintf()\n");

	if (!cpu_feature_supported(CPU_FEATURE_VMX)) {
		kprintf("vmx_alloc_all>VMX CPU support NOT found\n");
		ret = -ENXIO;
		goto out;
	} else if (bp->bp_flags & BPF_TEST_HARDWARE) {
		kprintf("VMX CPU support found\n");
	}

	u64 feature_control = get_MSR(MSR_IA32_FEATURE_CONTROL);
	bool locked = bits(feature_control, 0, 0);
	bool vmx_smm = bits(feature_control, 1, 1);
	bool vmx_ok = bits(feature_control, 2, 2);
    kprintf("IA32_FEATURE_CONTROL locked=%d smx=%d vmx=%d\n",
		locked, vmx_smm, vmx_ok);
	if (!locked || !vmx_ok) {
		kprintf("vmx_alloc_all>MSR_IA32_FEATURE_CONTROL not set up"
			" correctly by BIOS\n");
		ret = -ENXIO;
		goto out;
	} else if (bp->bp_flags & BPF_TEST_HARDWARE) {
		kprintf("MSR_IA32_FEATURE_CONTROL set up"
			" correctly by BIOS\n");
	}

	if (bp->bp_flags & BPF_TEST_HARDWARE) {
		u64 basic_vmx_info = get_MSR(MSR_IA32_VMX_BASIC);
		kprintf("MSR_IA32_VMX_BASIC: 0x%llx\n", basic_vmx_info);
		kprintf("VMCS rev id 0x%lx\n", bits(basic_vmx_info, 31, 0));

		kprintf("VMCS size %ld\n", bits(basic_vmx_info, 44, 32));
		kprintf("VMCS width %ld\n", bits(basic_vmx_info, 48, 48));
		kprintf("VMCS dualmon %ld\n", bits(basic_vmx_info, 49, 49));
		kprintf("VMCS memtype 0x%lx\n", bits(basic_vmx_info, 53, 50));
		kprintf("VMCS INS/OUTS %ld\n", bits(basic_vmx_info, 54, 54));
		kprintf("Use TRUE MSRs %ld\n", bits(basic_vmx_info, 55, 55));

		u64 misc = get_MSR(MSR_IA32_VMX_MISC);
		kprintf("MSR_IA32_VMX_MISC: 0x%llx\n", misc);
		kprintf("support for HLT state %ld\n", bits(misc, 6, 6));
		kprintf("support for shutdown state %ld\n", bits(misc, 7, 7));
		kprintf("support for wait-SIPI state %ld\n", bits(misc, 8, 8));
		kprintf("number of CR3 target values %ld\n", bits(misc, 24, 16));
		kprintf("max MSR list len %ld\n",
			512 * (bits(misc, 27, 25) + 1));
		kprintf("MSEG rev ID %ld\n", bits(misc, 63, 32));

		dump_vmx_control_support();

		/* If BPF_TEST_HARDWARE, bail out here */
		ret = -EINVAL;
		goto out;
	}


	if (!bit_test(get_MSR(MSR_IA32_VMX_MISC), 8)) {
		kprintf("vmx_alloc_all>wait-SIPI state support not found\n");
		ret = -ENXIO;
		goto out;
	}

	ret = idt_construct();
	if (ret < 0) {
		goto out;
	}

	/* Standalone hypervisor should have called pt_construct() a long time
	   ago, and should be using the new pagetablse already */

	ret = init_valloc();
	if (ret < 0) {
		goto out;
	}

	if (bp->bp_flags & BPF_NO_VMX) {
		return ret;
	}

	ret = ept_construct();
	if (ret < 0) {
		goto out;
	}

	dma_test_page = page_alloc();
	if (!dma_test_page) {
		ret = -ENOMEM;
		goto out;
	}

	for (un i = 1; i < 64; i++) {
		((u32 *)dma_test_page)[i] = 0xcafebabe;
	}

	for_each_os_cpu(i) {
		kprintf("vmx_alloc_all>allocating for CPU %ld\n", i);

		hyp_istacks[i] = alloc_stack(i, ISTACK_LEN, true);

		ret = tss_construct(&hyp_tss[i],
				    (addr_t)hyp_istacks[i] + ISTACK_LEN);
		if (ret) {
			goto out;
		}

		ret = gdt_construct(&hyp_gdt[i], &hyp_tss[i]);
		if (ret) {
			goto out;
		}

		cpu_vm[i].v_cpuno = i;
		ret = vmx_alloc(&cpu_vm[i]);
		if (ret < 0) {
			goto out;
		}
	}

	ret = vmx_init_devices();

out:
	if (ret < 0) {
		vmx_free_all();
	}

	return ret;
}

#define HIGHMSR		0xc0000000
#define MSRLIMIT	0x00001fff

inline static void
msr_read_bitmaps_init(vm_t *v, un value)
{
	bitmap_t *bitmap = (bitmap_t *)(v->v_msr_bitmaps);
	bitmap_init(bitmap, 2048 << 3, value);
}

inline static void
msr_write_bitmaps_init(vm_t *v, un value)
{
	bitmap_t *bitmap = (bitmap_t *)(v->v_msr_bitmaps + 2048);
	bitmap_init(bitmap, 2048 << 3, value);
}

inline static void
msr_read_bitmap_set(vm_t *v, un msr)
{
	un i;
	un offset;

	if (msr <= MSRLIMIT) {
		offset = 0;
		i = msr;
	} else if ((msr >= HIGHMSR) && (msr <= (HIGHMSR|MSRLIMIT))) {
		offset = 1024;
		i = msr - HIGHMSR; 
	} else {
		kprintf("msr_read_bitmap_set>msr 0x%lx is out of range\n",
			msr);
		return;
	}
	bitmap_t *bitmap = (bitmap_t *)(v->v_msr_bitmaps + offset);
	bitmap_set(bitmap, i);
}

inline static void
msr_read_bitmap_clear(vm_t *v, un msr)
{
	un i;
	un offset;

	if (msr <= MSRLIMIT) {
		offset = 0;
		i = msr;
	} else if ((msr >= HIGHMSR) && (msr <= (HIGHMSR|MSRLIMIT))) {
		offset = 1024;
		i = msr - HIGHMSR; 
	} else {
		kprintf("msr_read_bitmap_clear>msr 0x%lx is out of range\n",
			msr);
		return;
	}
	bitmap_t *bitmap = (bitmap_t *)(v->v_msr_bitmaps + offset);
	bitmap_clear(bitmap, i);
}

static void
msr_write_bitmap_set(vm_t *v, un msr)
{
	un i;
	un offset;

	if (msr <= MSRLIMIT) {
		offset = 2048;
		i = msr;
	} else if ((msr >= HIGHMSR) && (msr <= (HIGHMSR|MSRLIMIT))) {
		offset = 3072;
		i = msr - HIGHMSR; 
	} else {
		kprintf("msr_write_bitmap_set>msr 0x%lx is out of range\n",
			msr);
		return;
	}
	bitmap_t *bitmap = (bitmap_t *)(v->v_msr_bitmaps + offset);
	bitmap_set(bitmap, i);
}

inline static void
msr_write_bitmap_clear(vm_t *v, un msr)
{
	un i;
	un offset;

	if (msr <= MSRLIMIT) {
		offset = 2048;
		i = msr;
	} else if ((msr >= HIGHMSR) && (msr <= (HIGHMSR|MSRLIMIT))) {
		offset = 3072;
		i = msr - HIGHMSR; 
	} else {
		kprintf("msr_write_bitmap_set>msr 0x%lx is out of range\n",
			msr);
		return;
	}
	bitmap_t *bitmap = (bitmap_t *)(v->v_msr_bitmaps + offset);
	bitmap_clear(bitmap, i);
}

inline static void
msr_bitmaps_clear(vm_t *v, un msr)
{
	msr_read_bitmap_clear(v, msr);
	msr_write_bitmap_clear(v, msr);
}


void
vmx_on(void)
{
	assert(irq_is_disabled());

	kprintf("vmx_on>ENTRY\n");

	un n = cpuno();
	vm_t *v = &cpu_vm[n];
	ret_t ret = 0;
	bp->ret[n] = 0;

	if (v->v_in_vmx) {
		kprintf("vmx_on>already in VMX operation\n");
		ret = -1;
		goto out;
	}

	if (!cpu_feature_supported(CPU_FEATURE_VMX)) {
		kprintf("vmx_on>VMX CPU support NOT found\n");
		ret = -1;
		goto out;
	}

	u64 feature_control = get_MSR(MSR_IA32_FEATURE_CONTROL);
	bool locked = bits(feature_control, 0, 0);
	bool vmx_smm = bits(feature_control, 1, 1);
	bool vmx_ok = bits(feature_control, 2, 2);
    kprintf("IA32_FEATURE_CONTROL locked=%d smx=%d vmx=%d\n",
		locked, vmx_smm, vmx_ok);
	if (!locked || !vmx_ok) {
		kprintf("vmx_on>MSR_IA32_FEATURE_CONTROL not set up correctly"
			" by BIOS\n");
		ret = -ENXIO;
		goto out;
	}

	un cr0_fixed0 = get_MSR(MSR_IA32_VMX_CR0_FIXED0);
	un cr0_fixed1 = get_MSR(MSR_IA32_VMX_CR0_FIXED1);
	un cr0 = get_CR0();
	set_CR0(vmx_chk_rqd_bits("cr0", cr0,  cr0_fixed0, cr0_fixed1));

	un cr4_fixed0 = get_MSR(MSR_IA32_VMX_CR4_FIXED0);
	un cr4_fixed1 = get_MSR(MSR_IA32_VMX_CR4_FIXED1);

	un cr4 = get_CR4();
	cr4 |= bit(CR4_VMXE);
	set_CR4(cr4);
	cr4 = get_CR4();
	set_CR4(vmx_chk_rqd_bits("cr4", cr4, cr4_fixed0, cr4_fixed1));

	un err = vmxon(kvtophys(v->v_vmxon_region));
	if (err) {
		kprintf("vmx_on>vmxon failed\n");
		ret = -1;
	} else {
		kprintf("vmx_on>vmxon succeeded!\n");
		v->v_in_vmx = true;
		ept_invalidate_global();
	}

out:
	bp->ret[n] = ret;
}

inline static void
__vmx_off(vm_t *v)
{
	/* It's not safe to call kprintf() from here. */
	if (v->v_in_vmx) {
		ept_invalidate_global();
		un err = vmxoff();
		if (err == 0) {
			v->v_in_vmx = false;
		}
	}
}

void
vmx_off(void)
{
	assert(irq_is_disabled());

	vm_t *v = &cpu_vm[cpuno()];

	__vmx_off(v);
}

bool
vmx_exec_cpu1_supported(un control)
{
	control = bit(control);
	u64 rqd = get_MSR(cpuinfo.msr_procbased_ctls);
	un res = vmx_add_rqd_bits(control,
				  bits(rqd, 31, 0),
				  bits(rqd, 63, 32));
	return (res & control);
}

void
vmx_set_exec_cpu1(un control)
{
	un vmexec_primary = vmread(VMCE_PRIMARY_CPU_BASED_CONTROLS);
	bit_set(vmexec_primary, control);
	vmwrite(VMCE_PRIMARY_CPU_BASED_CONTROLS, vmexec_primary);
}

void
vmx_clear_exec_cpu1(un control)
{
	un vmexec_primary = vmread(VMCE_PRIMARY_CPU_BASED_CONTROLS);
	bit_clear(vmexec_primary, control);
	vmwrite(VMCE_PRIMARY_CPU_BASED_CONTROLS, vmexec_primary);
}

bool
vmx_exec_cpu2_supported(un control)
{
	u32 msr = cpuinfo.msr_procbased_ctls2;
	if (msr == 0) {
		return false;
	}
	control = bit(control);
	u64 rqd = get_MSR(msr);
	un res = vmx_add_rqd_bits(control,
				  bits(rqd, 31, 0),
				  bits(rqd, 63, 32));
	return (res & control);
}

void
vmx_set_exec_cpu2(un control)
{
	un vmexec_primary = vmread(VMCE_PRIMARY_CPU_BASED_CONTROLS);
	bool secondary_active = bit_test(vmexec_primary,
					 VMEXEC_CPU1_ACTIVATE_SECONDARY_CTRLS);
	un vmexec_secondary = vmread(VMCE_SECONDARY_CPU_BASED_CONTROLS);
	bit_set(vmexec_secondary, control);
	vmwrite(VMCE_SECONDARY_CPU_BASED_CONTROLS, vmexec_secondary);
	if (!secondary_active) {
		bit_set(vmexec_primary, VMEXEC_CPU1_ACTIVATE_SECONDARY_CTRLS);
		vmwrite(VMCE_PRIMARY_CPU_BASED_CONTROLS, vmexec_primary);
	}
}

void
vmx_clear_exec_cpu2(un control)
{
	un vmexec_secondary = vmread(VMCE_SECONDARY_CPU_BASED_CONTROLS);
	bit_clear(vmexec_secondary, control);
	vmwrite(VMCE_SECONDARY_CPU_BASED_CONTROLS, vmexec_secondary);
}

static void
vmx_set_pin_ctl(un control)
{
	un vmexec_pin = vmread(VMCE_PIN_BASED_CONTROLS);
	bit_set(vmexec_pin, control);
	vmwrite(VMCE_PIN_BASED_CONTROLS, vmexec_pin);
}

static void
vmx_clear_pin_ctl(un control)
{
	un vmexec_pin = vmread(VMCE_PIN_BASED_CONTROLS);
	bit_clear(vmexec_pin, control);
	vmwrite(VMCE_PIN_BASED_CONTROLS, vmexec_pin);
}

static void
vmx_set_exit_ctl(un control)
{
	un vmexit_ctl = vmread(VMCE_VMEXIT_CONTROLS);
	bit_set(vmexit_ctl, control);
	vmwrite(VMCE_VMEXIT_CONTROLS, vmexit_ctl);
}

static void
vmx_clear_exit_ctl(un control)
{
	un vmexit_ctl = vmread(VMCE_VMEXIT_CONTROLS);
	bit_clear(vmexit_ctl, control);
	vmwrite(VMCE_VMEXIT_CONTROLS, vmexit_ctl);
}

static void
vmx_enable_external_interrupt_exiting(void)
{
	vmx_set_pin_ctl(VMEXEC_PIN_EXT_INTR_EXIT);
	vmx_set_exit_ctl(VMEXIT_CTL_ACK_INTERRUPT);
}

static inline void
vmx_disable_external_interrupt_exiting(void)
{
	vmx_clear_pin_ctl(VMEXEC_PIN_EXT_INTR_EXIT);
	vmx_clear_exit_ctl(VMEXIT_CTL_ACK_INTERRUPT);
}

static inline un
vmx_get_shadow(un r, un guest_host_mask, un read_shadow)
{
	r &= ~guest_host_mask; // clear host controled bits
	r |= read_shadow & guest_host_mask; // set shadowed bits
	return r;
}

static void
vmx_update_guest_cr3(vm_t *v, bool guest64, un cr3)
{
	v->v_cr3_read_shadow = cr3;
	vmwrite(VMCE_GUEST_CR3, cr3);

	if (guest64) {
	    return;
	}

	un cr4 = vmread(VMCE_GUEST_CR4);

	/* 32-bit PAE mode requires the processor to cache all 4 PDPTEs */
	if (bit_test(cr4, CR4_PAE)) {
		u32 guest_pdpt_page = bits(cr3, 31, 12) << 12;
		u32 pdpt_off  = bits(cr3, 11,  5) << 5;
		addr_t pdpt_page;
		u64 *pdpt;
		if (!guest_phys_page_readable(GUEST_ID, guest_pdpt_page)) {
			vm_entry_inject_exception(VEC_GP, 0);
			return;
		}
		pdpt_page = map_page(guest_pdpt_page, 0);
		pdpt = (u64 *)(pdpt_page + pdpt_off);
		vmwrite64(VMCE_GUEST_PDPTE0, pdpt[0]);
		vmwrite64(VMCE_GUEST_PDPTE1, pdpt[1]);
		vmwrite64(VMCE_GUEST_PDPTE2, pdpt[2]);
		vmwrite64(VMCE_GUEST_PDPTE3, pdpt[3]);
		unmap_page(pdpt_page, UMPF_LOCAL_FLUSH);
	}
}

static void
vmx_update_guest_cr0(un cr0)
{
	un cr0_fixed0 = get_MSR(MSR_IA32_VMX_CR0_FIXED0);
	un cr0_fixed1 = get_MSR(MSR_IA32_VMX_CR0_FIXED1);
	un guest_cr0 = vmx_add_rqd_bits(cr0, cr0_fixed0, cr0_fixed1);

#if 0
	/* Figure out what the right thing to do here is */
	if (bit_test(cr0, CR0_NW) || bit_test(cr0, cr0_CD)) {
		bit_clear(guest_cr0, CR0_NW);
		bit_clear(guest_cr0, CR0_CD);	    
	}
#endif

	kprintf("%s>%lx %lx\n", __func__, cr0, guest_cr0);
	
	vmwrite(VMCE_GUEST_CR0, guest_cr0);
	vmwrite(VMCE_CR0_READ_SHADOW, cr0);
}

static void
vmx_update_guest_cr4(un cr4)
{
	un cr4_fixed0 = get_MSR(MSR_IA32_VMX_CR4_FIXED0);
	un cr4_fixed1 = get_MSR(MSR_IA32_VMX_CR4_FIXED1);
	un guest_cr4 = vmx_add_rqd_bits(cr4, cr4_fixed0, cr4_fixed1);

	/* Don't let the guest think it can change CR4.VMXE */
	bit_clear(cr4, CR4_VMXE);

	vmwrite(VMCE_GUEST_CR4, guest_cr4);
	vmwrite(VMCE_CR4_READ_SHADOW, cr4);
}

static void
vmx_update_guest_paging_state(vm_t *v)
{
	un cr0_read_shadow = vmread(VMCE_CR0_READ_SHADOW);
	un cr4_read_shadow = vmread(VMCE_CR4_READ_SHADOW);
	
	if (!bit_test(cr0_read_shadow, CR0_PG)) {
		/* Guest not paging, emulate that with ident page tables.
		 * In addition, hide CR4.PGE and CR4.PSE bits from guest. */
		un guest_cr0 = vmread(VMCE_GUEST_CR0);
		un guest_cr4 = vmread(VMCE_GUEST_CR4);
		un cr4_trap_bits = vmread(VMCE_CR4_GUEST_HOST_MASK);

		/* Disabling paging, save CR3/CR4 read shadows */
		if (v->v_guest_paging) {
			un cr4 = vmx_get_shadow(guest_cr4,
						cr4_trap_bits,
						cr4_read_shadow);
			vmwrite(VMCE_CR4_READ_SHADOW, cr4);

			v->v_cr3_read_shadow = vmread(VMCE_GUEST_CR3);
		}

		bit_set(guest_cr0, CR0_PG);
		vmwrite(VMCE_GUEST_CR0, guest_cr0);

		bit_set(guest_cr4, CR4_PGE);
		bit_set(guest_cr4, CR4_PSE);
		bit_clear(guest_cr4, CR4_PAE);
		vmwrite(VMCE_GUEST_CR4, guest_cr4);

		vmwrite(VMCE_GUEST_CR3, shadow_ident_cr3);
		protect_memory_phys_addr(shadow_ident_cr3, VM_PAGE_SIZE,
					 EPT_PERM_R, VMPF_HYP);

		bit_set(cr4_trap_bits, CR4_PGE);
		bit_set(cr4_trap_bits, CR4_PSE);
		bit_set(cr4_trap_bits, CR4_PAE);
		vmwrite(VMCE_CR4_GUEST_HOST_MASK, cr4_trap_bits);

		vmx_set_exec_cpu1(VMEXEC_CPU1_CR3_LOAD_EXIT);
		vmx_set_exec_cpu1(VMEXEC_CPU1_CR3_STORE_EXIT);

#ifdef __x86_64__
		un entry_ctl = vmread(VMCE_VMENTRY_CONTROLS);
		bit_clear(entry_ctl, VMENTRY_CTL_GUEST_64);
		vmwrite(VMCE_VMENTRY_CONTROLS, entry_ctl);

		bit_clear(v->v_ia32_efer_shadow, EFER_LMA);
#endif

		v->v_guest_paging = false;
	} else if (!v->v_guest_paging) {
		bool guest64 = false;

		/* Guest paging for the first time, rsync CR4 shadow,
		   and stop hiding CR4.PGE and CR4.PSE.
		   Also reload CR3, for PAE. */
		un cr4_trap_bits = vmread(VMCE_CR4_GUEST_HOST_MASK);
		un cr4 = vmx_get_shadow(vmread(VMCE_GUEST_CR4),
					cr4_trap_bits,
					cr4_read_shadow);
		vmx_update_guest_cr4(cr4);

#ifdef __x86_64__
		if (bit_test(cr4, CR4_PAE) &&
		    bit_test(v->v_ia32_efer_shadow, EFER_LME)) {
			un entry_ctl = vmread(VMCE_VMENTRY_CONTROLS);
			assert(!bit_test(entry_ctl, VMENTRY_CTL_GUEST_64));
			bit_set(entry_ctl, VMENTRY_CTL_GUEST_64);
			vmwrite(VMCE_VMENTRY_CONTROLS, entry_ctl);

			bit_set(v->v_ia32_efer_shadow, EFER_LMA);

			guest64 = true;
		}
#endif

		bit_clear(cr4_trap_bits, CR4_PGE);
		bit_clear(cr4_trap_bits, CR4_PSE);
		bit_clear(cr4_trap_bits, CR4_PAE);
		vmwrite(VMCE_CR4_GUEST_HOST_MASK, cr4_trap_bits);

		vmx_update_guest_cr3(v, guest64, v->v_cr3_read_shadow);

		vmx_clear_exec_cpu1(VMEXEC_CPU1_CR3_LOAD_EXIT);
		vmx_clear_exec_cpu1(VMEXEC_CPU1_CR3_STORE_EXIT);

		v->v_guest_paging = true;
#ifdef NOTDEF
		kprintf("vmx_update_guest_paging_state>first time paging: "
			"CR0 %lx CR4 %lx(%lx) CR3 %lx "
			"PDPTEs %llx %llx %llx %llx\n",
			vmread(VMCE_GUEST_CR0),	vmread(VMCE_GUEST_CR4), cr4,
			vmread(VMCE_GUEST_CR3),
			vmread64(VMCE_GUEST_PDPTE0), vmread64(VMCE_GUEST_PDPTE1),
			vmread64(VMCE_GUEST_PDPTE2), vmread64(VMCE_GUEST_PDPTE3)
			);
#endif
	}
}

void
vm_entry_inject(u32 interruption_type, u8 vector, u32 errcode)
{
	u32 vmentry_interrupt_info = vector;
	bool deliver_error_code;

	switch (interruption_type) {
	case IT_EXTERNAL_INTERRUPT:
		deliver_error_code = false;
		assert(vector >= VEC_FIRST_USER);
		break;
	case IT_HARDWARE_EXCEPTION:		
		switch (vector) {
		case VEC_DF:
		case VEC_TS:
		case VEC_NP:
		case VEC_SS:
		case VEC_GP:
		case VEC_PF:
		case VEC_AC:
			deliver_error_code = true;
			break;
		case VEC_DE:
		case VEC_DB:
		case VEC_BP:
		case VEC_OF:
		case VEC_BR:
		case VEC_UD:
		case VEC_NM:
		case VEC_MF:
		case VEC_MC:
		case VEC_XF:
			deliver_error_code = false;
			break;
		default:
			kprintf("vm_entry_inject>unimplemented vector %d\n",
				vector);
			return;
		}
		break;
	case IT_OTHER_EVENT:
		deliver_error_code = false;
		if (vector != 0) {
			kprintf("vm_entry_inject>IT_OTHER_EVENT with non-zero"
			       " vector	%d\n", vector);
			return;
		}
		break;
	default:
		kprintf("vm_entry_inject>unknown interruption_type %d\n",
			interruption_type);
		return;
	}

	vmentry_interrupt_info |= (interruption_type << 8);
	if (deliver_error_code) {
		bit_set(vmentry_interrupt_info, II_DELIVER_ERRCODE);
		vmwrite(VMCE_VMENTRY_EXCEPTION_ERROR_CODE, errcode);
	}

	bit_set(vmentry_interrupt_info, II_VALID);

	vmwrite(VMCE_VMENTRY_INTERRUPT_INFO_FIELD, vmentry_interrupt_info);
}

inline static bool
guest_injection_pending(void)
{
	return bit_test(vmread(VMCE_VMENTRY_INTERRUPT_INFO_FIELD), II_VALID);
}

void
guest_set_interrupt_pending(u8 intvec)
{
	un cpu = cpuno();
	vm_t *v = &cpu_vm[cpu];
	vapic_t *va = &v->v_vapic;

	if (apic_ISR_test(intvec)) {
		bitmap_set(va->va_apic, intvec);
	} else {
		bitmap_clear(va->va_apic, intvec);
	}

	bitmap_set(va->va_irr, intvec);
}

bool
deliver_pending_interrupt(void)
{
	vm_t *v = &cpu_vm[cpuno()];
	vapic_t *va = &v->v_vapic;

	sn r = bitmap_first(va->va_irr, NINTVECS);
	if (r < 0) {
		/* No more interrupts pending */
		vmx_clear_exec_cpu1(VMEXEC_CPU1_INTR_WINDOW_EXIT);
		return false;
	}
	bool apic_interrupt = bitmap_test(va->va_apic, r);

	sn s = bitmap_first(va->va_isr, NINTVECS);
	if (apic_interrupt && ((interrupt_pri(r) <= interrupt_pri(s)) ||
			       (interrupt_pri(r) <= va->va_task_pri))) {
		/* wait for EOI */
		vmx_clear_exec_cpu1(VMEXEC_CPU1_INTR_WINDOW_EXIT);
		return true;
	}

	if (guest_injection_pending() || guest_interrupts_are_blocked()) {
		vmx_set_exec_cpu1(VMEXEC_CPU1_INTR_WINDOW_EXIT);
		return true;
	}

	if (r >= VEC_FIRST_USER) {
	    vm_entry_inject_interrupt(r);
	}
	bitmap_clear(va->va_irr, r);
	if (apic_interrupt) {
		bitmap_set(va->va_isr, r);
	}

	vmx_clear_exec_cpu1(VMEXEC_CPU1_INTR_WINDOW_EXIT);

	return true;
}

bool
vm_exit_rdmsr(vm_t *v, registers_t *regs)
{
	un msr = regs->rcx;

	if (((msr > MSRLIMIT) && (msr < HIGHMSR)) ||
	    (msr > (HIGHMSR|MSRLIMIT))) {
		vm_entry_inject_exception(VEC_GP, 0);
		return true;
	}

	un result;

	switch (msr) {
	case MSR_IA32_FS_BASE:
		result = vmread(VMCE_GUEST_FS_BASE);
		break;

	case MSR_IA32_GS_BASE:
		result = vmread(VMCE_GUEST_GS_BASE);
		break;
#ifdef __x86_64__
	case MSR_IA32_EFER:
		result = v->v_ia32_efer_shadow;
		break;
#endif
	default:
		kprintf("vm_exit_rdmsr>MSR 0x%lx\n", regs->rcx);
		return false;

	}

	regs->rax = bits(result, 31, 0);
	regs->rdx = bits(result, 63, 32);

	skip_emulated_instr();

	return true;
}

bool
vm_exit_wrmsr(vm_t *v, registers_t *regs)
{
	un msr = regs->rcx;

	if (((msr > MSRLIMIT) && (msr < HIGHMSR)) ||
	    (msr > (HIGHMSR|MSRLIMIT))) {
		vm_entry_inject_exception(VEC_GP, 0);
		return true;
	}

	u64 arg = ((u64)(regs->rdx) << 32) | regs->rax;

	switch (msr) {
	case MSR_IA32_FS_BASE:
		vmwrite(VMCE_GUEST_FS_BASE, arg);
		break;
	case MSR_IA32_GS_BASE:
		vmwrite(VMCE_GUEST_GS_BASE, arg);
		kprintf("vm_exit_wrmsr>gsbase := 0x%llx\n", arg);
		break;
	case MSR_IA32_APIC_BASE:
		kprintf("vm_exit_wrmsr>set_MSR(MSR_IA32_APIC_BASE, 0x%llx\n",
			arg);
		set_MSR(MSR_IA32_APIC_BASE, arg);
		break;
#ifdef __x86_64__
	case MSR_IA32_EFER: {
		u64 ia32_efer = get_MSR(MSR_IA32_EFER);

		// do not allow guest to turn on NXE if hw does not support it;
		if( !cpu_xfeature_supported(CPU_XFEATURE_NXE) && (arg & bit(EFER_NXE))) {
			vm_entry_inject_exception(VEC_GP, 0);
			return true;
		}

		ia32_efer &= ~IA32_EFER_GUEST_BITS;
		ia32_efer |= arg & IA32_EFER_GUEST_BITS;

		/* Not allowed to change LME bit while paging is on. */
		if (v->v_guest_paging &&
		    bit_test(arg ^ v->v_ia32_efer_shadow, EFER_LME)) {
			vm_entry_inject_exception(VEC_GP, 0);
			return true;
		}

		set_MSR(MSR_IA32_EFER, ia32_efer);
		v->v_ia32_efer_shadow = arg;

		break;
	}
#endif
	default:
		kprintf("vm_exit_wrmsr>MSR 0x%lx\n", regs->rcx);
		return false;

	}

	skip_emulated_instr();

	return true;
}

inline static void
squash_feature(un feature, registers_t *regs)
{
	if (feature >= 32) {
		feature -= 32;
		bit_clear(regs->rcx, feature);
	} else {
		bit_clear(regs->rdx, feature);
	}
}

inline static void
advertise_feature(un feature, registers_t *regs)
{
	if (feature >= 32) {
		feature -= 32;
		bit_set(regs->rcx, feature);
	} else {
		bit_set(regs->rdx, feature);
	}
}

bool
vm_exit_cpuid(registers_t *regs)
{
	un rax = regs->rax;
	char *id = "BroadOn     ";

	cpuid(regs->rax, regs->rbx, regs->rcx, regs->rdx);
	switch (rax) {
	case 0x1:
		squash_feature(CPU_FEATURE_VMX, regs);
		squash_feature(CPU_FEATURE_SMX, regs);
		advertise_feature(CPU_FEATURE_HYP, regs);
		break;
	case 0x40000000:
		regs->rax = 0x40000000;
		memcpy((u32 *)(&regs->rbx), id + 0, 4);
		memcpy((u32 *)(&regs->rdx), id + 4, 4);
		memcpy((u32 *)(&regs->rcx), id + 8, 4);
		break;
	}

	skip_emulated_instr();

	return true;
}

bool
vm_exit_xsetbv(registers_t *regs)
{
	if (!cpu_feature_supported(CPU_FEATURE_XSAVE)) {
		vm_entry_inject_exception(VEC_UD, 0);
		return true;
	}

	un rcx = bits(regs->rcx, 31, 0);
	un rdx = bits(regs->rdx, 31, 0);
	un rax = bits(regs->rax, 31, 0);

	if (guest_usermode() ||
	    (rcx != 0) ||
	    (rdx != 0) ||
	    (bits(rax, 31, 2) != 0) ||
	    (bit_test(rax, 0) != 1)) {
		vm_entry_inject_exception(VEC_GP, 0);
		return true;
	}

	xsetbv(rcx, rax, rdx);

	skip_emulated_instr();

	return true;
}

#define MOV_TO		0
#define MOV_FROM	1

#define MOV_TO_CR3	((MOV_TO << 4) | 3)
#define MOV_FROM_CR3	((MOV_FROM << 4) | 3)
#define MOV_TO_CR0	((MOV_TO << 4) | 0)
#define MOV_TO_CR4	((MOV_TO << 4) | 4)
#define CLTS		(2 << 4)
#define LMSW		(3 << 4)


/* Returns 'true' if access was handled, otherwise 'false' */
bool
vm_exit_cr_access(vm_t *v, registers_t *regs)
{
	un exit_qualification = vmread(VMCE_EXIT_QUALIFICATION);
	u8 access = bits(exit_qualification, 5, 0);
	u8 n = bits(exit_qualification, 11, 8);
	un cr0 = bit(CR0_PE); // We want to detect when CR0.PE is turned off

	switch (access) {
	case MOV_TO_CR3:
#ifdef NOTDEF
		kprintf("vm_exit_cr_access>MOV_TO_CR3 from reg %d\n", n);
		kprintf("vm_exit_cr_access>reg %d value 0x%lx\n", n,
			regs->reg[n]);
#endif
		vmx_update_guest_cr3(v, guest_64(), regs->reg[n]);
		break;
	case MOV_FROM_CR3:
		regs->reg[n] = v->v_guest_paging ? vmread(VMCE_GUEST_CR3) : v->v_cr3_read_shadow;
#ifdef NOTDEF
		kprintf("vm_exit_cr_access>MOV_FROM_CR3 to reg %d\n", n);
		kprintf("vm_exit_cr_access>reg %d value 0x%lx\n", n,
			regs->reg[n]);
#endif
		break;
	case MOV_TO_CR4:
#ifdef NOTDEF
		kprintf("vm_exit_cr_access>MOV_TO_CR4 from reg %d value 0x%lx\n", n,
			regs->reg[n]);
#endif
		vmx_update_guest_cr4(regs->reg[n]);
		break;
	case MOV_TO_CR0:
#ifdef NOTDEF
		kprintf("vm_exit_cr_access>MOV_TO_CR0 from reg %d value 0x%lx\n", n,
			regs->reg[n]);
#endif
		cr0 = regs->reg[n];
		vmx_update_guest_cr0(cr0);
		break;
	case CLTS:
	case LMSW:
		/*
		 * We don't have the full CR0 from these two instructions,
		 * so we need to construct what the guest thinks the full cr0
		 * value currently is.
		 */
		cr0 = vmx_get_shadow(vmread(VMCE_GUEST_CR0),
				     vmread(VMCE_CR0_GUEST_HOST_MASK),
				     vmread(VMCE_CR0_READ_SHADOW));

		if (access == CLTS) {
			bit_clear(cr0, CR0_TS);
			kprintf("vm_exit_cr_access>CLTS, new CR0 value 0x%lx", cr0);

		} else {
			cr0 &= ~0xf;
			cr0 |= bits(exit_qualification, 19, 16);
			kprintf("vm_exit_cr_access>LMSW, new CR0 value 0x%lx", cr0);

		}
		vmx_update_guest_cr0(cr0);
		break;
	default:
		kprintf("vm_exit_cr_access>unknown CR access\n");
		kprintf("vm_exit_cr_access>exit qualification 0x%lx\n",
			exit_qualification);
		return false;
	}

	skip_emulated_instr();

	if (!bit_test(cr0, CR0_PE)) {
		guest_state_t *const gs = &guest_state[v->v_cpuno];
		vmx_store_guest_state(v, regs, gs);
#ifdef NOTDEF
		kprintf("vm_exit_cr_access>Guest requested realmode emulation: "
			"CS:IP %04lx:%04lx\n",
			(vmread(VMCE_GUEST_CS_BASE) >> 4) & 0xffff,
			vmread(VMCE_GUEST_RIP) & 0xffff
			);
#endif
		ghv_guest_resume_in_realmode(gs);
		vmx_load_guest_state_and_regs(v, regs, gs);
	}
	vmx_update_guest_paging_state(v);

	return true;
}

bool
vm_exit_interrupt_window(registers_t *regs)
{
	/* Handled at the end of every vm_exit() */
	return true;
}

bool
vm_exit_hardware_interrupt(registers_t *regs)
{
	u32 interrupt_info = vmread(VMCE_VMEXIT_INTERRUPT_INFO);
	if (!bit_test(interrupt_info, II_VALID)) {
		kprintf("vm_exit_hardware_interrupt>not II_VALID\n");
		return false;
	}
	u8 vector = bits(interrupt_info, 7, 0);
	bool handled = interrupt(vector);
	if (!handled) {
		/* Pass it up to the guest */
		guest_set_interrupt_pending(vector);
	}

	return true;
}


static ret_t
vmx_init_devices(void)
{
	static LOCK_INIT_STATIC(lock);
	static bool initialized = false;
	un flags;
	ret_t ret = 0;

	spinlock(&lock, flags);
	if (initialized) {
		goto out;
	}
	initialized = true;

	ret = map_apic();
	if (ret < 0) {
		goto out;
	}

	ret = vtd_construct();
	if (ret < 0) {
		goto out;
	}


	notify_init();
	vtd_enable();

	protect_hypervisor_memory();

out:
	spinunlock(&lock, flags);
	return ret;
}

static inline void
vmx_fin_devices(void)
{
	vtd_destroy();
	set_kp_intvec(0, 0);
	unmap_apic();
}

extern void pt_test(void);
extern void valloc_test(void);
extern void test_exception(void);
extern void test_gp(void);
extern ret_t gpt_test(addr_t, size_t);
extern un test_divzero(un);

static void dump_vmstats(void);

#ifdef PARAVIRT_SMP
static inline void
vmx_record_guest_ap_state(int apic_id, un name, un val)
{
	guest_state_t *const gs = &guest_state[apic_id];

	kprintf("SIPI hook: %x %lx %lx\n", apic_id, name, val);

	switch (name) {
	case VMCE_GUEST_CR0:
		gs->cr0 = val;
		break;
	case VMCE_GUEST_CR3:
		gs->cr3 = val;
		break;
	case VMCE_GUEST_CR4:
		gs->cr4 = val;
		break;

	case VMCE_GUEST_RSP:
		gs->regs.guest_rsp = val;
		break;
	case VMCE_GUEST_RIP:
		gs->regs.guest_rip = val;
		break;
	case VMCE_GUEST_RFLAGS:
		gs->regs.guest_rflags = val;
		break;

	case VMCE_GUEST_CS_SEL:
		gs->cs.sel = val;
		break;
	case VMCE_GUEST_SS_SEL:
		gs->ss.sel = val;
		break;
	case VMCE_GUEST_DS_SEL:
		gs->ds.sel = val;
		break;
	case VMCE_GUEST_ES_SEL:
		gs->es.sel = val;
		break;
	case VMCE_GUEST_FS_SEL:
		gs->fs.sel = val;
		break;
	case VMCE_GUEST_GS_SEL:
		gs->gs.sel = val;
		break;
	case VMCE_GUEST_TR_SEL:
		gs->tss.sel = val;
		break;
	case VMCE_GUEST_LDTR_SEL:
		gs->ldt.sel = val;
		break;

	case VMCE_GUEST_CS_BASE:
		gs->cs.base = val;
		break;
	case VMCE_GUEST_SS_BASE:
		gs->ss.base = val;
		break;
	case VMCE_GUEST_DS_BASE:
		gs->ds.base = val;
		break;
	case VMCE_GUEST_ES_BASE:
		gs->es.base = val;
		break;
	case VMCE_GUEST_FS_BASE:
		gs->fs.base = val;
		break;
	case VMCE_GUEST_GS_BASE:
		gs->gs.base = val;
		break;
	case VMCE_GUEST_TR_BASE:
		gs->tss.base = val;
		break;
	case VMCE_GUEST_LDTR_BASE:
		gs->ldt.base = val;
		break;

	case VMCE_GUEST_CS_LIMIT:
		gs->cs.limit = val;
		break;
	case VMCE_GUEST_SS_LIMIT:
		gs->ss.limit = val;
		break;
	case VMCE_GUEST_DS_LIMIT:
		gs->ds.limit = val;
		break;
	case VMCE_GUEST_ES_LIMIT:
		gs->es.limit = val;
		break;
	case VMCE_GUEST_FS_LIMIT:
		gs->fs.limit = val;
		break;
	case VMCE_GUEST_GS_LIMIT:
		gs->gs.limit = val;
		break;
	case VMCE_GUEST_TR_LIMIT:
		gs->tss.limit = val;
		break;
	case VMCE_GUEST_LDTR_LIMIT:
		gs->ldt.limit = val;
		break;

	case VMCE_GUEST_CS_ACCESS_RIGHTS:
		gs->cs.attr = val;
		break;
	case VMCE_GUEST_SS_ACCESS_RIGHTS:
		gs->ss.attr = val;
		break;
	case VMCE_GUEST_DS_ACCESS_RIGHTS:
		gs->ds.attr = val;
		break;
	case VMCE_GUEST_ES_ACCESS_RIGHTS:
		gs->es.attr = val;
		break;
	case VMCE_GUEST_FS_ACCESS_RIGHTS:
		gs->fs.attr = val;
		break;
	case VMCE_GUEST_GS_ACCESS_RIGHTS:
		gs->gs.attr = val;
		break;
	case VMCE_GUEST_TR_ACCESS_RIGHTS:
		gs->tss.attr = val;
		break;
	case VMCE_GUEST_LDTR_ACCESS_RIGHTS:
		gs->ldt.attr = val;
		break;

	case VMCE_GUEST_GDTR_BASE:
		gs->gdt.base = val;
		break;
	case VMCE_GUEST_IDTR_BASE:
		gs->idt.base = val;
		break;
	case VMCE_GUEST_GDTR_LIMIT:
		gs->gdt.limit = val;
		break;
	case VMCE_GUEST_IDTR_LIMIT:
		gs->idt.limit = val;

	default:
		break;
	}
}
#endif /* PARAVIRT_SMP */

static void
vmx_load_guest_state(vm_t *v, const guest_state_t *gs)
{
	vmx_update_guest_cr0(gs->cr0);
	vmx_update_guest_cr4(gs->cr4);
	vmx_update_guest_cr3(v, gs->long_mode, gs->cr3);
	v->v_ia32_efer_shadow = gs->ia32_efer;

	vmwrite(VMCE_GUEST_CS_SEL, gs->cs.sel);
	vmwrite(VMCE_GUEST_SS_SEL, gs->ss.sel);
	vmwrite(VMCE_GUEST_DS_SEL, gs->ds.sel);
	vmwrite(VMCE_GUEST_ES_SEL, gs->es.sel);
	vmwrite(VMCE_GUEST_FS_SEL, gs->fs.sel);
	vmwrite(VMCE_GUEST_GS_SEL, gs->gs.sel);
	vmwrite(VMCE_GUEST_TR_SEL,   gs->tss.sel);
	vmwrite(VMCE_GUEST_LDTR_SEL, gs->ldt.sel);

	vmwrite(VMCE_GUEST_CS_BASE, gs->cs.base);
	vmwrite(VMCE_GUEST_SS_BASE, gs->ss.base);
	vmwrite(VMCE_GUEST_DS_BASE, gs->ds.base);
	vmwrite(VMCE_GUEST_ES_BASE, gs->es.base);
	vmwrite(VMCE_GUEST_FS_BASE, gs->fs.base);
	vmwrite(VMCE_GUEST_GS_BASE, gs->gs.base);
	vmwrite(VMCE_GUEST_TR_BASE,   gs->tss.base);
	vmwrite(VMCE_GUEST_LDTR_BASE, gs->ldt.base);

	vmwrite(VMCE_GUEST_CS_LIMIT, gs->cs.limit);
	vmwrite(VMCE_GUEST_SS_LIMIT, gs->ss.limit);
	vmwrite(VMCE_GUEST_DS_LIMIT, gs->ds.limit);
	vmwrite(VMCE_GUEST_ES_LIMIT, gs->es.limit);
	vmwrite(VMCE_GUEST_FS_LIMIT, gs->fs.limit);
	vmwrite(VMCE_GUEST_GS_LIMIT, gs->gs.limit);
	vmwrite(VMCE_GUEST_TR_LIMIT,   gs->tss.limit);
	vmwrite(VMCE_GUEST_LDTR_LIMIT, gs->ldt.limit);

#define gs_seg_access_rights(seg) (gs->seg.attr ? gs->seg.attr : (1 << 16))
	vmwrite(VMCE_GUEST_CS_ACCESS_RIGHTS, gs_seg_access_rights(cs));
	vmwrite(VMCE_GUEST_SS_ACCESS_RIGHTS, gs_seg_access_rights(ss));
	vmwrite(VMCE_GUEST_DS_ACCESS_RIGHTS, gs_seg_access_rights(ds));
	vmwrite(VMCE_GUEST_ES_ACCESS_RIGHTS, gs_seg_access_rights(es));
	vmwrite(VMCE_GUEST_FS_ACCESS_RIGHTS, gs_seg_access_rights(fs));
	vmwrite(VMCE_GUEST_GS_ACCESS_RIGHTS, gs_seg_access_rights(gs));
	vmwrite(VMCE_GUEST_TR_ACCESS_RIGHTS,   gs_seg_access_rights(tss));
	vmwrite(VMCE_GUEST_LDTR_ACCESS_RIGHTS, gs_seg_access_rights(ldt));
#undef gs_seg_access_rights

	vmwrite(VMCE_GUEST_GDTR_BASE, gs->gdt.base);
	vmwrite(VMCE_GUEST_IDTR_BASE, gs->idt.base);

	vmwrite(VMCE_GUEST_GDTR_LIMIT, gs->gdt.limit);
	vmwrite(VMCE_GUEST_IDTR_LIMIT, gs->idt.limit);

	vmwrite(VMCE_GUEST_RIP,    gs->regs.guest_rip);
	vmwrite(VMCE_GUEST_RSP,    gs->regs.guest_rsp);
	vmwrite(VMCE_GUEST_RFLAGS, gs->regs.guest_rflags);

	vmwrite64(VMCE_ADDR_IO_BITMAP_A, kvtophys(ghv_io_bitmaps[0]));
	vmwrite64(VMCE_ADDR_IO_BITMAP_B, kvtophys(ghv_io_bitmaps[1]));

	vmwrite(VMCE_GUEST_ACTIVITY_STATE, gs->active ?
		VMX_ACTIVITY_STATE_ACTIVE : VMX_ACTIVITY_STATE_WAIT_SIPI);
}

static void
vmx_load_guest_state_and_regs(vm_t *v, registers_t *regs, const guest_state_t *gs)
{
	vmx_load_guest_state(v, gs);
	*regs = gs->regs;
}

static void
vmx_store_guest_state(const vm_t *v, const registers_t *regs, guest_state_t *gs)
{
	gs->regs = *regs;

	gs->cr0 = vmx_get_shadow(vmread(VMCE_GUEST_CR0),
				 vmread(VMCE_CR0_GUEST_HOST_MASK),
				 vmread(VMCE_CR0_READ_SHADOW));
	gs->cr4 = vmx_get_shadow(vmread(VMCE_GUEST_CR4),
				 vmread(VMCE_CR4_GUEST_HOST_MASK),
				 vmread(VMCE_CR4_READ_SHADOW));
	gs->cr3 = v->v_guest_paging ? vmread(VMCE_GUEST_CR3) : v->v_cr3_read_shadow;
	gs->ia32_efer = v->v_ia32_efer_shadow;

	gs->cs.sel  = vmread(VMCE_GUEST_CS_SEL);
	gs->ss.sel  = vmread(VMCE_GUEST_SS_SEL);
	gs->ds.sel  = vmread(VMCE_GUEST_DS_SEL);
	gs->es.sel  = vmread(VMCE_GUEST_ES_SEL);
	gs->fs.sel  = vmread(VMCE_GUEST_FS_SEL);
	gs->gs.sel  = vmread(VMCE_GUEST_GS_SEL);
	gs->tss.sel = vmread(VMCE_GUEST_TR_SEL);
	gs->ldt.sel = vmread(VMCE_GUEST_LDTR_SEL);

	gs->cs.base  = vmread(VMCE_GUEST_CS_BASE);
	gs->ss.base  = vmread(VMCE_GUEST_SS_BASE);
	gs->ds.base  = vmread(VMCE_GUEST_DS_BASE);
	gs->es.base  = vmread(VMCE_GUEST_ES_BASE);
	gs->fs.base  = vmread(VMCE_GUEST_FS_BASE);
	gs->gs.base  = vmread(VMCE_GUEST_GS_BASE);
	gs->tss.base = vmread(VMCE_GUEST_TR_BASE);
	gs->ldt.base = vmread(VMCE_GUEST_LDTR_BASE);

	gs->cs.limit  = vmread(VMCE_GUEST_CS_LIMIT);
	gs->ss.limit  = vmread(VMCE_GUEST_SS_LIMIT);
	gs->ds.limit  = vmread(VMCE_GUEST_DS_LIMIT);
	gs->es.limit  = vmread(VMCE_GUEST_ES_LIMIT);
	gs->fs.limit  = vmread(VMCE_GUEST_FS_LIMIT);
	gs->gs.limit  = vmread(VMCE_GUEST_GS_LIMIT);
	gs->tss.limit = vmread(VMCE_GUEST_TR_LIMIT);
	gs->ldt.limit = vmread(VMCE_GUEST_LDTR_LIMIT);

#define set_attr_if_usable(attr, access) do {			\
	    un __access = (access);				\
	    (attr) = (__access & (1 << 16)) ? 0 : __access;	\
	} while (0)

	set_attr_if_usable(gs->cs.attr,  vmread(VMCE_GUEST_CS_ACCESS_RIGHTS));
	set_attr_if_usable(gs->ss.attr,  vmread(VMCE_GUEST_SS_ACCESS_RIGHTS));
	set_attr_if_usable(gs->ds.attr,  vmread(VMCE_GUEST_DS_ACCESS_RIGHTS));
	set_attr_if_usable(gs->es.attr,  vmread(VMCE_GUEST_ES_ACCESS_RIGHTS));
	set_attr_if_usable(gs->fs.attr,  vmread(VMCE_GUEST_FS_ACCESS_RIGHTS));
	set_attr_if_usable(gs->gs.attr,  vmread(VMCE_GUEST_GS_ACCESS_RIGHTS));
	set_attr_if_usable(gs->tss.attr, vmread(VMCE_GUEST_TR_ACCESS_RIGHTS));
	set_attr_if_usable(gs->ldt.attr, vmread(VMCE_GUEST_LDTR_ACCESS_RIGHTS));

#undef set_attr_if_usable
	
	gs->gdt.base = vmread(VMCE_GUEST_GDTR_BASE);
	gs->idt.base = vmread(VMCE_GUEST_IDTR_BASE);

	gs->gdt.limit = vmread(VMCE_GUEST_GDTR_LIMIT);
	gs->idt.limit = vmread(VMCE_GUEST_IDTR_LIMIT);

	gs->regs.guest_rip    = vmread(VMCE_GUEST_RIP);
	gs->regs.guest_rsp    = vmread(VMCE_GUEST_RSP);
	gs->regs.guest_rflags = vmread(VMCE_GUEST_RFLAGS);

	gs->active = vmread(VMCE_GUEST_ACTIVITY_STATE) == VMX_ACTIVITY_STATE_ACTIVE;
}

static bool
vm_exit_sipi(vm_t *v, registers_t *regs)
{
	guest_state_t *const gs = &guest_state[v->v_cpuno];
	un vec = bits(vmread(VMCE_EXIT_QUALIFICATION), 7, 0);

#ifdef PARAVIRT_SMP
	un *tramp = (void *)(vec << VM_PAGE_SHIFT);
	*tramp = 0xA5A5A5A5; /* ACK the SIPI */
#else
	gs->active = true;
	gs->cs.base = vec << VM_PAGE_SHIFT;
	gs->regs.guest_rip = 0;
	ghv_guest_resume_in_realmode(gs);
#endif
	vmx_load_guest_state_and_regs(v, regs, gs);
	vmx_update_guest_paging_state(v);

	kprintf("vm_exit>SIPI Post-SIPI state: "
		"RIP %lx RSP %lx CR0 %lx CR4 %lx CR3 %lx "
		"PDPTEs %llx %llx %llx %llx\n",
		vmread(VMCE_GUEST_RIP), vmread(VMCE_GUEST_RSP),
		vmread(VMCE_GUEST_CR0),	vmread(VMCE_GUEST_CR4),
		vmread(VMCE_GUEST_CR3),
		vmread64(VMCE_GUEST_PDPTE0), vmread64(VMCE_GUEST_PDPTE1),
		vmread64(VMCE_GUEST_PDPTE2), vmread64(VMCE_GUEST_PDPTE3)
		);

	return true;
}


static inline void
vmx_store_tss32(vm_t *v, registers_t *regs, tss32_t *tss)
{
	tss->tss_cr3 = v->v_guest_paging ? vmread(VMCE_GUEST_CR3) : v->v_cr3_read_shadow;
	tss->tss_eip    = vmread(VMCE_GUEST_RIP);
	tss->tss_eflags = vmread(VMCE_GUEST_RFLAGS);

	tss->tss_eax = regs->rax;
	tss->tss_ecx = regs->rcx;
	tss->tss_edx = regs->rdx;
	tss->tss_ebx = regs->rbx;
	tss->tss_esp = vmread(VMCE_GUEST_RSP);
	tss->tss_ebp = regs->rbp;
	tss->tss_esi = regs->rsi;
	tss->tss_edi = regs->rdi;

	tss->tss_es = vmread(VMCE_GUEST_ES_SEL) & 0xffff;
	tss->tss_cs = vmread(VMCE_GUEST_CS_SEL) & 0xffff;
	tss->tss_ss = vmread(VMCE_GUEST_SS_SEL) & 0xffff;
	tss->tss_ds = vmread(VMCE_GUEST_DS_SEL) & 0xffff;
	tss->tss_fs = vmread(VMCE_GUEST_FS_SEL) & 0xffff;
	tss->tss_gs = vmread(VMCE_GUEST_GS_SEL) & 0xffff;
	tss->tss_ldtr = vmread(VMCE_GUEST_LDTR_SEL) & 0xffff;
}


static inline bool
vmx_load_tss32(vm_t *v, registers_t *regs, tss32_t *tss, u8 *vec, un *error)
{
	assert(!guest_64());
	un gdt_base  = vmread(VMCE_GUEST_GDTR_BASE);
	un gdt_limit = vmread(VMCE_GUEST_GDTR_LIMIT);
	seg_desc_t desc;

	vmx_update_guest_cr3(v, false, tss->tss_cr3);
	vmx_update_guest_paging_state(v);

	regs->rax = tss->tss_eax;
	regs->rcx = tss->tss_ecx;
	regs->rdx = tss->tss_edx;
	regs->rbx = tss->tss_ebx;
	// skip rsp
	regs->rbp = tss->tss_ebp;
	regs->rsi = tss->tss_esi;
	regs->rdi = tss->tss_edi;

#define load_guest_seg_desc(seg, sel) do {				\
		if (((sel) & ~7) > (gdt_limit & ~7)) {			\
			*vec = VEC_GP;					\
			*error = sel;					\
		}							\
		addr_t desc_addr = (gdt_base + (sel)) & ~7;		\
		if (copy_from_guest(&desc, desc_addr, sizeof desc) < 0) { \
			*vec = VEC_PF;					\
			*error = 0;					\
			return false;					\
		}							\
		vmwrite(VMCE_GUEST_##seg##_SEL,  (sel));		\
		vmwrite(VMCE_GUEST_##seg##_BASE,  seg_desc_base(&desc, 0)); \
		vmwrite(VMCE_GUEST_##seg##_LIMIT, seg_desc_limit(&desc)); \
		vmwrite(VMCE_GUEST_##seg##_ACCESS_RIGHTS, (sel) ? desc.seg_attr & 0xf0ff : 1<<16); \
	} while (0)

	load_guest_seg_desc(ES,   tss->tss_es);
	load_guest_seg_desc(CS,   tss->tss_cs);
	load_guest_seg_desc(SS,   tss->tss_ss);
	load_guest_seg_desc(DS,   tss->tss_ds);
	load_guest_seg_desc(FS,   tss->tss_fs);
	load_guest_seg_desc(GS,   tss->tss_gs);
	load_guest_seg_desc(LDTR, tss->tss_ldtr);

#undef 	load_guest_seg_desc

	vmwrite(VMCE_GUEST_RIP,    tss->tss_eip);
	vmwrite(VMCE_GUEST_RFLAGS, tss->tss_eflags | 2);
	vmwrite(VMCE_GUEST_RSP, tss->tss_esp);

	return true;
}

enum {
    TASK_SWITCH_CALL = 0,
    TASK_SWITCH_IRET,
    TASK_SWITCH_JMP,
    TASK_SWITCH_IDT,
};

static bool vm_exit_task_switch(vm_t *v, registers_t *regs)
{
	un exit_qualification = vmread(VMCE_EXIT_QUALIFICATION);
	u16 new_tr = bits(exit_qualification, 15, 0);
	u8 reason = bits(exit_qualification, 31, 30);
	un old_tss_base  = vmread(VMCE_GUEST_TR_BASE);
	u16 old_tr = vmread(VMCE_GUEST_TR_SEL);
	un new_tss_base;
	un new_tss_limit;
	un gdt_base  = vmread(VMCE_GUEST_GDTR_BASE);
	un gdt_limit = vmread(VMCE_GUEST_GDTR_LIMIT);
	u8 cpl = bits(vmread(VMCE_GUEST_CS_SEL), 1, 0);
	u8 rpl = bits(new_tr, 1, 0);
	seg_desc_t old_tss_desc;
	seg_desc_t new_tss_desc;
	tss32_t old_tss, new_tss;

	static_assert(sizeof(tss32_t) == 104);
	
	bool guest64 = guest_64();
	if (guest64) {
		kprintf("vm_exit_task_switch>Task switch should NOT occur in 64-bit!\n");
		return false;
	}

	kprintf("vm_exit_task_switch>Task Switch %x to %x, reason %d\n",
		old_tr, new_tr, reason);

#ifdef NOTDEF
	kprintf("vm_exit_task_switch>Pre-TASK SWITCH state:\n"
		"RIP %lx RSP %lx CR0 %lx CR4 %lx CR3 %lx\n"
		"PDPTEs %llx %llx %llx %llx\n"
		"CS: sel %04lx, base %08lx, limit %08lx\n"
		"DS: sel %04lx, base %08lx, limit %08lx\n",
		vmread(VMCE_GUEST_RIP), vmread(VMCE_GUEST_RSP),
		vmread(VMCE_GUEST_CR0),	vmread(VMCE_GUEST_CR4),
		vmread(VMCE_GUEST_CR3),
		vmread64(VMCE_GUEST_PDPTE0), vmread64(VMCE_GUEST_PDPTE1),
		vmread64(VMCE_GUEST_PDPTE2), vmread64(VMCE_GUEST_PDPTE3),
		vmread(VMCE_GUEST_CS_SEL), vmread(VMCE_GUEST_CS_BASE),
		vmread(VMCE_GUEST_CS_LIMIT),
		vmread(VMCE_GUEST_DS_SEL), vmread(VMCE_GUEST_DS_BASE),
		vmread(VMCE_GUEST_DS_LIMIT)
		);
#endif

	/* Intel Vol-3A 7.3: Task switching. */

	/* 1. Obtains the TSS segment selector for the new task */
	if ((new_tr & 4)) {
		kprintf("vm_exit_task_switch>new TR is from LDT, not implemented\n");
		return false;
	}

	if ((new_tr & ~7) > (gdt_limit & ~7)) {
		kprintf("vm_exit_task_switch>new TR above gdt_limit, injecting #GP\n");
		vm_entry_inject_exception(VEC_GP, old_tr);
		return true;
	}
	if (copy_from_guest(&new_tss_desc, (gdt_base + new_tr) & ~7, sizeof new_tss_desc) < 0 ||
	    copy_from_guest(&old_tss_desc, (gdt_base + old_tr) & ~7, sizeof old_tss_desc) < 0) {
		kprintf("vm_exit_task_switch>could not read guest GDT, injecting #PF\n");
		vm_entry_inject_exception(VEC_PF, 0);
		return true;
	}

	/* 2. Check that the old task is allowed to switch to the new task. */
	if (reason == TASK_SWITCH_JMP || reason == TASK_SWITCH_CALL) {
		/* For CALL, JMP, CPL and RPL must be less than DPL */
		if (cpl > new_tss_desc.seg_dpl || rpl > new_tss_desc.seg_dpl) {
			kprintf("vm_exit_task_switch>access violation, injecting #GP\n");
			vm_entry_inject_exception(VEC_GP, 0);
			return true;
		}
	}

	/* 3. Check that the TSS descriptor is marked valid, and limit >= 0x67h */
	/* 4. Check that tehe new task is avaliable (CALL, JMP, INT), or busy (IRET) */
	new_tss_base  = seg_desc_base(&new_tss_desc, guest64);
	new_tss_limit = seg_desc_limit(&new_tss_desc);
	if (!new_tss_desc.seg_p || new_tss_desc.seg_s ||
	    new_tss_limit < sizeof(new_tss) - 1 ||
	    (reason == TASK_SWITCH_IRET && new_tss_desc.seg_type != 11) ||
	    (reason != TASK_SWITCH_IRET && new_tss_desc.seg_type !=  9)) {
		kprintf("vm_exit_task_switch>Bad task segment descriptor, injecting #TS\n");
		vm_entry_inject_exception(VEC_TS, new_tr);
	}

	/* 5. Check that the old TSS, new TSS are paged into system memory */
	if (copy_from_guest(&new_tss, new_tss_base, sizeof new_tss) < 0 ||
	    copy_from_guest(&old_tss, old_tss_base, sizeof old_tss) < 0) {
		kprintf("vm_exit_task_switch>could not read guest TSS, injecting #PF\n");
		vm_entry_inject_exception(VEC_PF, 0);
		return true;
	}

	/* 6. If task switch was initiated with a JMP, or IRET, clear B flag of the old descriptor */
	if (reason == TASK_SWITCH_JMP || reason == TASK_SWITCH_IRET) {
		bit_clear(old_tss_desc.seg_type, 1); // clear the busy flag of old descriptor
		copy_to_guest((gdt_base + old_tr) & ~7,
			      &old_tss_desc, sizeof old_tss_desc);
	}

	/* 7. If task switch was initiated with an IRET, clear NT in EFLAGS */
	if (reason == TASK_SWITCH_IRET) {
		un rflags = vmread(VMCE_GUEST_RFLAGS);
		bit_clear(rflags, FLAGS_NT);
		vmwrite(VMCE_GUEST_RFLAGS, rflags);
	}

	/* 8. Save the state of the old task in the old TSS */
	vmx_store_tss32(v, regs, &old_tss);
	copy_to_guest(old_tss_base, &old_tss, sizeof old_tss);
	if (new_tss_base == old_tss_base) {
		new_tss = old_tss;
	}

	/* 9. If task switch was initiated with CALL or INT, set NT flag of EFLAGS loaded from the new TSS */
	/* Well, let's do this after EFLAGS is loaded in step 12. */

	/* 10. If task switch was intiated with CALL, JMP, or INT, set B flag in new TSS descriptor */
	if (reason != TASK_SWITCH_IRET) {
		bit_set(new_tss_desc.seg_type, 1); // set the busy flag on new descriptor
		copy_to_guest((gdt_base + new_tr) & ~7,
			      &new_tss_desc, sizeof new_tss_desc);
	}

	/* 11. Load the TR with segment selector and descriptor of new TSS */
	vmwrite(VMCE_GUEST_TR_SEL,   new_tr);
	vmwrite(VMCE_GUEST_TR_BASE,  new_tss_base);
	vmwrite(VMCE_GUEST_TR_LIMIT, new_tss_limit);
	vmwrite(VMCE_GUEST_TR_ACCESS_RIGHTS, new_tss_desc.seg_attr & 0xf0ff);

	/* 12. Load state from new TSS */
	/* 13. Load segments associated with new TSS */
	u8 vec;
	un error;
	if (!vmx_load_tss32(v, regs, &new_tss, &vec, &error)) {
		// Due to not being able to read TSS
		kprintf("vm_exit_task_switch>error loading from TSS, injecting #%d\n", vec);
		vm_entry_inject_exception(vec, error);
		return true;
	}
	
	/* Step 9 */
	if (reason == TASK_SWITCH_CALL || reason == TASK_SWITCH_IDT) {
		new_tss.tss_prev_task_link = old_tr;
		copy_to_guest(new_tss_base, &new_tss, sizeof new_tss);
		un rflags = vmread(VMCE_GUEST_RFLAGS);
		bit_set(rflags, FLAGS_NT);
		vmwrite(VMCE_GUEST_RFLAGS, rflags);
	}

	un cr0 = vmread(VMCE_GUEST_CR0);
	bit_set(cr0, CR0_TS);
	vmwrite(VMCE_GUEST_CR0, cr0);

#ifdef NOTDEF
	kprintf("vm_exit_task_switch>Post-TASK SWITCH state:\n"
		"RIP %lx RSP %lx CR0 %lx CR4 %lx CR3 %lx\n"
		"PDPTEs %llx %llx %llx %llx\n"
		"CS: sel %04lx, base %08lx, limit %08lx\n"
		"DS: sel %04lx, base %08lx, limit %08lx\n",
		vmread(VMCE_GUEST_RIP), vmread(VMCE_GUEST_RSP),
		vmread(VMCE_GUEST_CR0),	vmread(VMCE_GUEST_CR4),
		vmread(VMCE_GUEST_CR3),
		vmread64(VMCE_GUEST_PDPTE0), vmread64(VMCE_GUEST_PDPTE1),
		vmread64(VMCE_GUEST_PDPTE2), vmread64(VMCE_GUEST_PDPTE3),
		vmread(VMCE_GUEST_CS_SEL), vmread(VMCE_GUEST_CS_BASE),
		vmread(VMCE_GUEST_CS_LIMIT),
		vmread(VMCE_GUEST_DS_SEL), vmread(VMCE_GUEST_DS_BASE),
		vmread(VMCE_GUEST_DS_LIMIT)
		);
#endif
	return true;
}

static bool
vm_exit_exception(vm_t *v, registers_t *regs)
{
	un exit_qualification = vmread(VMCE_EXIT_QUALIFICATION);
	u32 interrupt_info = vmread(VMCE_VMEXIT_INTERRUPT_INFO);
	u32 errcode = vmread(VMCE_VMEXIT_INTERRUPT_ERROR_CODE);
	u8 vec = bits(interrupt_info, 7, 0);
	bool has_errcode = bit_test(interrupt_info, 11);

	kprintf("vm_exit_exception>Exception information:\n"
		"vector %d, has_error_code %d, error_code %08x "
		"exit_qualification: %lx\n",
		vec, has_errcode, errcode, exit_qualification);

	vmx_dump_guest_instruction(__func__);
	vmx_dump_guest_registers(__func__, regs->reg);

	if (vec == VEC_PF) {
		set_CR2(exit_qualification);
	}

	vm_entry_inject_exception(vec, errcode);

	return true;
}


static bool
vm_exit_io(vm_t *v, registers_t *regs)
{
	un exit_qualification = vmread(VMCE_EXIT_QUALIFICATION);

	u8 iotype = bits(exit_qualification, 3, 0);
	bool str = bit_test(exit_qualification, 4);
	bool rep = bit_test(exit_qualification, 5);
	u16 port = bits(exit_qualification, 31, 16);	
	bool do_io = false;

	skip_emulated_instr();

	if (str || rep) {
		kprintf("vm_exit_io>port i/o string operation not supported.\n");
		return false;
	}

	switch (port) {
	case PIC_MASTER_CMD:
	case PIC_MASTER_IMR:
	case PIC_SLAVE_CMD:
	case PIC_SLAVE_IMR:
		do_io = vm_exit_pic_intercept(v, iotype, port, regs->rax);
		break;
	default:
		do_io = ghv_handle_io_exit(port, iotype, &regs->rax);
		break;
	}

	if (do_io) {
		switch (iotype) {
		case IO_OUTB:
			outb(port, regs->rax);
			break;
		case IO_OUTW:
			outw(port, regs->rax);
			break;
		case IO_OUTL:
			outl(port, regs->rax);
			break;
		case IO_INB:
			regs->rax &= ~0xffUL;
			regs->rax |= inb(port);
			break;
		case IO_INW:
			regs->rax &= ~0xffffUL;
			regs->rax |= inw(port);
			break;
		case IO_INL:
			regs->rax = inl(port);
			break;
		default:
			kprintf("vm_exit_io>Unknow IO type %x\n", iotype);
			return false;
		}
	} else {
		switch (iotype) {
		case IO_OUTB:
		case IO_OUTW:
		case IO_OUTL:
			break;
		case IO_INB:
			regs->rax |= 0xff;
			break;
		case IO_INW:
			regs->rax |= 0xffff;
			break;
		case IO_INL:
			regs->rax = ~0U;
			break;
		default:
			kprintf("vm_exit_io>Unknow IO type %x\n", iotype);
			return false;
		}
	}

	return true;
}

static bool
vm_exit_vmcall(vm_t *v, registers_t *regs)
{
	if( !ghv_handle_hypercall(regs, guest_usermode())) {
		vm_entry_inject_exception(VEC_UD, 0);
		return true;
	}
	skip_emulated_instr();

	return true;
}

extern void vm_exit_handler(void);

#define p(x)	kprintf("vm_exit>" #x " 0x%lx\n", (un)x);

void
vm_exit_dump(vm_t *v, registers_t *regs)
{
	u32 exit_reason = vmread(VMCE_EXIT_REASON);
	u32 basic_exit_reason = bits(exit_reason, 15, 0);
	bool entry_failure  = bits(exit_reason, 31, 31);
	kprintf("vm_exit>basic_exit reason %d%s\n", basic_exit_reason,
		entry_failure ? " entry failure" : "");

	un exit_qualification = vmread(VMCE_EXIT_QUALIFICATION);

	u32 interrupt_info = vmread(VMCE_VMEXIT_INTERRUPT_INFO);
	u32 interrupt_errcode = vmread(VMCE_VMEXIT_INTERRUPT_ERROR_CODE);

	u32 instr_len = vmread(VMCE_VMEXIT_INSTRUCTION_LENGTH);
	u32 instr_info = vmread(VMCE_VMEXIT_INSTRUCTION_INFO);
	un guest_rip = vmread(VMCE_GUEST_RIP);
	un guest_linear_addr = vmread(VMCE_GUEST_LINEAR_ADDR);

	u32 idt_vectoring_info = vmread(VMCE_IDT_VECTORING_INFO_FIELD);
	u32 idt_vectoring_errcode = vmread(VMCE_IDT_VECTORING_ERROR_CODE);

	p(exit_qualification);
	p(interrupt_info);
	p(interrupt_errcode);
	p(guest_rip);
	p(instr_len);
	p(instr_info);
	p(guest_linear_addr);
	p(idt_vectoring_info);
	p(idt_vectoring_errcode);

#ifdef __x86_64__
#define rfmt ":0x%016lx"
#else
#define rfmt ":0x%08lx"
#endif
	kprintf("vm_exit>"R"sp"rfmt"\n", regs->rsp);
	kprintf("vm_exit>"R"ax"rfmt" "R"cx"rfmt" "R"dx"rfmt"\n",
		regs->rax, regs->rcx, regs->rdx);
	kprintf("vm_exit>"R"bx"rfmt" "R"bp"rfmt" "R"si"rfmt"\n",
		regs->rbx, regs->rbp, regs->rsi);
#ifndef __x86_64__
	kprintf("vm_exit>"R"di"rfmt"\n",
		regs->rdi);
#else
	kprintf("vm_exit>rdi"rfmt" r8 "rfmt" r9 "rfmt"\n",
		regs->rdi, regs->r8, regs->r9);
	kprintf("vm_exit>r10"rfmt" r11"rfmt" r12"rfmt"\n",
		regs->r10, regs->r11, regs->r12);
	kprintf("vm_exit>r13"rfmt" r14"rfmt" r15"rfmt"\n",
		regs->r13, regs->r14, regs->r15);
#endif
	kprintf("nexits %lld\n", v->v_nexits);
	if (entry_failure) {
		check_vmcs(v);
	}
}

void
vmx_copy_guest_state_to_host(void)
{
	dtr_t dtr;
	u16 sel;

	/* CR3 needs to be set first, because GDT comes out of virtual space */
	set_CR3(vmread(VMCE_GUEST_CR3));

	dtr.dt_base = vmread(VMCE_GUEST_GDTR_BASE);
	dtr.dt_limit = vmread(VMCE_GUEST_GDTR_LIMIT);
	set_GDTR(&dtr);

	sel = vmread(VMCE_GUEST_TR_SEL);
	clear_tss_busy((seg_desc_t *)dtr.dt_base, sel);
	set_TR(sel);

	sel = vmread(VMCE_GUEST_LDTR_SEL);
	set_LDT_sel(sel);

	dtr.dt_base = vmread(VMCE_GUEST_IDTR_BASE);
	dtr.dt_limit = vmread(VMCE_GUEST_IDTR_LIMIT);
	set_IDTR(&dtr);

	set_DS(vmread(VMCE_GUEST_DS_SEL));
	set_ES(vmread(VMCE_GUEST_ES_SEL));
	set_FS(vmread(VMCE_GUEST_FS_SEL));
	set_GS(vmread(VMCE_GUEST_GS_SEL));

#ifdef __x86_64__
	set_MSR(MSR_IA32_FS_BASE, vmread(VMCE_GUEST_FS_BASE));
	set_MSR(MSR_IA32_GS_BASE, vmread(VMCE_GUEST_GS_BASE));
#endif
}



static void
vmx_exit(vm_t *v, registers_t *regs)
{
	exit(-1);
}

typedef struct {
	un st_nexits;
	un st_total_time;
} vm_stats_t;

vm_stats_t vmstats[LAST_EXIT_REASON + 1];

static inline void
dump_vmstats(void)
{
	for (un i = 0; i <= LAST_EXIT_REASON; i++) {
		if (vmstats[i].st_nexits > 0) {
			u64 total_us = cycles_to_us(vmstats[i].st_total_time);
			un avg_us = (total_us / vmstats[i].st_nexits);
			kprintf("%ld\t%ld\t%ldus\n", i,
				vmstats[i].st_nexits,
				avg_us);
		}
	}
}

/* Returns
 *  0  if VMRESUME is needed
 *  1  if VMLAUNCH is needed
 * -1  if return to VMX-root mode is needed (debug-only)
 */
sn
vm_exit(registers_t *regs)
{
	u64 tsc_start = rdtsc();
	bool ignore_time = false;
	vm_t *v = &cpu_vm[cpuno()];

	if (unlikely(v->v_cache_timing_thres)) {
		u64 cycles = cache_timing(10);
		while (cycles > v->v_cache_timing_thres) {
			kprintf("bad cache timing: %lld, "
				"cache appears to be off...\n", cycles);
			cycles = cache_timing(10);
		}
	}

	u32 exit_reason = vmread(VMCE_EXIT_REASON);
	u32 basic_exit_reason = bits(exit_reason, 15, 0);
	bool entry_failure  = bits(exit_reason, 31, 31);
	bool handled = false;

	if (unlikely(exit_reason == ~0U)) {
	    int tries = 0;
	    while (exit_reason == ~0U) {
		exit_reason = vmread(VMCE_EXIT_REASON);
		basic_exit_reason = bits(exit_reason, 15, 0);
		entry_failure  = bits(exit_reason, 31, 31);
		tries++;
	    }
	    kprintf("exit reason %d was bad for %d attempts\n",
		    exit_reason, tries);

	}

	if (entry_failure) {
		goto exit_hypervisor;
	}

	regs->rsp = vmread(VMCE_GUEST_RSP);

	v->v_guest_64 = guest_64();
	v->v_guest_pae = guest_PAE();
	v->v_guest_cr3 = vmread(VMCE_GUEST_CR3);


	if (basic_exit_reason <= LAST_EXIT_REASON) {
		atomic_inc(&(vmstats[basic_exit_reason].st_nexits));
	}

	if (v->v_trace_exits) {
		kprintf("vm_exit>nexits %lld\n", v->v_nexits);
		kprintf("vm_exit>reason %d\n", basic_exit_reason);
		kprintf("vm_exit>guest rip %lx\n", vmread(VMCE_GUEST_RIP));
	}


	switch (basic_exit_reason) {
		/* Unexpected exits: disabled by VM-exec controls, or SMM */
	case EXIT_REASON_IO_SMI:	/* SMM */
	case EXIT_REASON_SMI:		/* SMM */
	case EXIT_REASON_NMI_WINDOW:
	case EXIT_REASON_HLT:
	case EXIT_REASON_INVLPG:
	case EXIT_REASON_RDPMC:
	case EXIT_REASON_RDTSC:
	case EXIT_REASON_RSM:		/* SMM */
	case EXIT_REASON_MOV_DR:
	case EXIT_REASON_MWAIT:
	case EXIT_REASON_MONITOR:
	case EXIT_REASON_PAUSE:
	case EXIT_REASON_TPR_BELOW_THRESHOLD:
	case EXIT_REASON_GDTR_IDTR_ACCESS:
	case EXIT_REASON_LDTR_TR_ACCESS:
	case EXIT_REASON_RDTSCP:
	case EXIT_REASON_WBINVD:
		kprintf("vm_exit>unexpected exit %d\n", basic_exit_reason);
		break;

		/* Fatal software or hardware errors */
	case EXIT_REASON_TRIPLE_FAULT:
	case EXIT_REASON_VMENTRY_FAIL_MACHINE_CHECK:
	case EXIT_REASON_EPT_MISCONFIG:
		kprintf("vm_exit>fatal error exit %d\n", basic_exit_reason);
		break;

		/* Entry failure */
	case EXIT_REASON_VMENTRY_FAIL_GUEST_STATE:
		kprintf("vm_exit>vmentry failed due to guest state\n");
		check_vmcs(v);
		break;
	case EXIT_REASON_VMENTRY_FAIL_MSR_LOADING:
		kprintf("vm_exit>vmentry failed due to MSR loading\n");
		break;

		/* Misbehaving guest trying to use VMX features*/
	case EXIT_REASON_GETSEC:
	case EXIT_REASON_VMCLEAR:
	case EXIT_REASON_VMLAUNCH:
	case EXIT_REASON_VMPTRLD:
	case EXIT_REASON_VMPTRST:
	case EXIT_REASON_VMREAD:
	case EXIT_REASON_VMRESUME:
	case EXIT_REASON_VMWRITE:
	case EXIT_REASON_VMXOFF:
	case EXIT_REASON_VMXON:
	case EXIT_REASON_INVEPT:
	case EXIT_REASON_INVVPID:
		vm_entry_inject_exception(VEC_UD, 0);
		handled = true;
		break;

		/* Implemented exits */
	case EXIT_REASON_EXCEPTION:
		handled = vm_exit_exception(v, regs);
		break;
	case EXIT_REASON_INTERRUPT:
		handled = vm_exit_hardware_interrupt(regs);
		break;
	case EXIT_REASON_INIT_SIGNAL:
		kprintf("vm_exit>INIT signal\n");
		exit(-1);
		break;
	case EXIT_REASON_SIPI:
		kprintf("vm_exit>SIPI\n");
#ifdef NOHT
		if ((v->v_cpuno & 1)) {
			return 0;
		}
#endif
		handled = vm_exit_sipi(v, regs);
		break;
	case EXIT_REASON_INTERRUPT_WINDOW:
		handled = vm_exit_interrupt_window(regs);
		break;
	case EXIT_REASON_TASK_SWITCH:
		handled = vm_exit_task_switch(v, regs);
		break;
	case EXIT_REASON_CPUID:
		handled = vm_exit_cpuid(regs);
		break;
	case EXIT_REASON_VMCALL:
		if (regs->rax == 902) {
			ignore_time = true;
		}
		handled = vm_exit_vmcall(v, regs);
		break;
	case EXIT_REASON_CR_ACCESS:
		handled = vm_exit_cr_access(v, regs);
		break;
	case EXIT_REASON_IO:
		handled = vm_exit_io(v, regs);
		break;
	case EXIT_REASON_RDMSR:
		handled = vm_exit_rdmsr(v, regs);
		break;
	case EXIT_REASON_WRMSR:
		handled = vm_exit_wrmsr(v, regs);
		break;
	case EXIT_REASON_MONITOR_TRAP_FLAG:
		handled = vm_exit_mtf(v, regs);
		break;
	case EXIT_REASON_APIC_ACCESS:
		handled = vm_exit_apic(v, regs);
		break;
	case EXIT_REASON_EPT_VIOLATION:
		handled = vm_exit_ept(regs);
		break;
	case EXIT_REASON_VMX_PREEMPTION_TIMER_EXPIRED:
		break;
	case EXIT_REASON_INVD:
		kprintf("vm_exit>guest attempt to execute INVD ignored\n");
		skip_emulated_instr();
		handled = true;
		break;
	case EXIT_REASON_XSETBV:
		handled = vm_exit_xsetbv(regs);
		break;
	default:
		kprintf("vm_exit>unknown exit\n");
		break;
	}


	if (handled) {
		bool success = true;
		/* Resume guest to deliver pending interrupt */
		deliver_pending_interrupt();

		if (!ignore_time) {
			u64 time = rdtsc() - tsc_start;
			atomic_add(&(vmstats[basic_exit_reason].st_total_time),
					   time);		
		}
		if (success) {
			return 0;
		}
	}

exit_hypervisor:
	if (!handled) {
		vm_exit_dump(v, regs);
		bp->ret[v->v_cpuno] = -1;
	}

	vmx_exit(v, regs);

	/* It's not safe to call kprintf() from here. */

	vmclear(kvtophys(v->v_guest_vmcs));

	__vmx_off(v);

	return -1;
}

void
vmx_guest_init(void)
{
	assert(irq_is_disabled());

	un n = cpuno();
	vm_t *v = &cpu_vm[n];
	bp->ret[n] = 0;

	void *guest = v->v_guest_vmcs;
	un err = vmclear(kvtophys(guest));
	if (err) {
		kprintf("vmx_guest_init>vmclear failed\n");
		bp->ret[n] = -1;
		return;
	}

	err = vmptrld(kvtophys(guest));
	if (err) {
		kprintf("vmx_guest_init>vmptrld failed\n");
		bp->ret[n] = -1;
		return;
	}

	v->v_nexits = 0;

	/* Host state */

	vmwrite(VMCE_HOST_CR0, get_CR0());
	vmwrite(VMCE_HOST_CR3, host_cr3);
	vmwrite(VMCE_HOST_CR4, get_CR4());

	vmwrite(VMCE_HOST_CS_SEL, HYP_CS);
	vmwrite(VMCE_HOST_SS_SEL, HYP_DS);
	vmwrite(VMCE_HOST_DS_SEL, HYP_DS);
	vmwrite(VMCE_HOST_ES_SEL, HYP_DS);
	vmwrite(VMCE_HOST_FS_SEL, HYP_DS);
	vmwrite(VMCE_HOST_GS_SEL, HYP_DS);
	vmwrite(VMCE_HOST_TR_SEL, HYP_TSS);

#ifdef NOTDEF
	static LOCK_INIT_STATIC(dump_lock);

	nested_spinlock(&dump_lock);

	un tr = get_TR();
	kprintf("TR sel 0x%lx\n", get_TR());
	kprintf("TSS base 0x%lx limit 0x%x attr 0x%x\n",
		get_seg_base_self(tr),
		get_seg_limit_self(tr),
		get_seg_attr_self(tr));

	extern void dump_tss32(tss32_t *);
	dump_tss32((tss32_t *)get_seg_base_self(tr));

	nested_spinunlock(&dump_lock);
#endif

	vmwrite64(VMCE_HOST_IA32_PERF_GLOBAL_CTRL,
		  get_MSR(MSR_IA32_PERF_GLOBAL_CTRL));
	vmwrite(VMCE_HOST_IA32_SYSENTER_CS, get_MSR(MSR_IA32_SYSENTER_CS));
	vmwrite(VMCE_HOST_IA32_SYSENTER_ESP, get_MSR(MSR_IA32_SYSENTER_ESP));
	vmwrite(VMCE_HOST_IA32_SYSENTER_EIP, get_MSR(MSR_IA32_SYSENTER_EIP));

	vmwrite(VMCE_HOST_FS_BASE, get_MSR(MSR_IA32_FS_BASE));
	vmwrite(VMCE_HOST_GS_BASE, get_MSR(MSR_IA32_GS_BASE));
#ifdef NOTDEF
	set_MSR(MSR_IA32_KERNEL_GS_BASE, get_MSR(MSR_IA32_GS_BASE));
#endif
	vmwrite(VMCE_HOST_GDTR_BASE, hyp_gdt[n].dt_base);
	vmwrite(VMCE_HOST_IDTR_BASE, hyp_idtr.dt_base);
	vmwrite(VMCE_HOST_TR_BASE, hyp_tss[n].dt_base);

	vmwrite(VMCE_HOST_RSP, (un)(v->v_stack) + VSTACK_LEN);
	vmwrite(VMCE_HOST_RIP, (un)&vm_exit_handler);

	/* Guest state */

	u64 misc = get_MSR(MSR_IA32_VMX_MISC);
	/*
	 * 1 VMX timer tick = 2^X TSC ticks
  	 * We want to preempt every every (2^32 - 1) TSC ticks.
	 * This value is reset on every VMENTRY, unless it's saved on exit.
	 */
	u8 preempt_X = bits(misc, 4, 0);
	u32 preempt_timer_val = (1U << (32 - preempt_X)) - 1;
	vmwrite(VMCE_VMX_PREEMPTION_TIMER_VALUE, preempt_timer_val);

    	const guest_state_t *const gs = &guest_state[n];
	vmx_load_guest_state(v, gs);

	vmwrite64(VMCE_GUEST_IA32_DEBUGCTL, 0);
	vmwrite64(VMCE_GUEST_IA32_PERF_GLOBAL_CTRL, 0);
	vmwrite(VMCE_GUEST_IA32_SYSENTER_CS, 0);
	vmwrite(VMCE_GUEST_IA32_SYSENTER_ESP, 0);
	vmwrite(VMCE_GUEST_IA32_SYSENTER_EIP, 0);

	vmwrite(VMCE_GUEST_DR7, 0);

	vmwrite(VMCE_GUEST_INTERRUPTIBILITY_STATE, 0);
	vmwrite(VMCE_GUEST_PENDING_DEBUG_EXCEPTIONS, 0);


	vmwrite64(VMCE_VMCS_LINK_PTR, -1ULL);

	/* Control */

	u32 pinbased_ctls = 0;
	//pinbased_ctls |= bit(VMEXEC_PIN_PREEMPTION_TIMER);
	u64 pinbased_ctls_rqd = get_MSR(cpuinfo.msr_pinbased_ctls);
	pinbased_ctls = vmx_add_rqd_bits(pinbased_ctls,
					 bits(pinbased_ctls_rqd, 31, 0),
					 bits(pinbased_ctls_rqd, 63, 32));
	vmwrite(VMCE_PIN_BASED_CONTROLS, pinbased_ctls);

	u32 procbased_ctls = bit(VMEXEC_CPU1_USE_MSR_BITMAPS);
	procbased_ctls |= bit(VMEXEC_CPU1_USE_IO_BITMAPS);
	u64 procbased_ctls_rqd = get_MSR(cpuinfo.msr_procbased_ctls);
	procbased_ctls = vmx_add_rqd_bits(procbased_ctls,
					  bits(procbased_ctls_rqd, 31, 0),
					  bits(procbased_ctls_rqd, 63, 32));
	vmwrite(VMCE_PRIMARY_CPU_BASED_CONTROLS, procbased_ctls);

	if (vmx_exec_cpu1_supported(VMEXEC_CPU1_ACTIVATE_SECONDARY_CTRLS)) {
		vmx_set_exec_cpu1(VMEXEC_CPU1_ACTIVATE_SECONDARY_CTRLS);
		u32 procbased_ctls2 = 0;
		if (!(bp->bp_flags & BPF_NO_EPT)) {
			bit_set(procbased_ctls2, VMEXEC_CPU2_ENABLE_EPT);
			if (cpu_supports_vpid()) {
				vmwrite(VMCE_VPID, GUEST_ID);
				bit_set(procbased_ctls2,
					VMEXEC_CPU2_ENABLE_VPID);
			}
		}
		u64 procbased_ctls2_rqd = get_MSR(cpuinfo.msr_procbased_ctls2);
		procbased_ctls2 =
			vmx_add_rqd_bits(procbased_ctls2,
					 bits(procbased_ctls2_rqd, 31, 0),
					 bits(procbased_ctls2_rqd, 63, 32));
		vmwrite(VMCE_SECONDARY_CPU_BASED_CONTROLS, procbased_ctls2);
	}

	/* MSR bitmaps */
	msr_read_bitmaps_init(v, BITMAP_ALL_ZEROES);
	msr_write_bitmaps_init(v, BITMAP_ALL_ZEROES);
	msr_write_bitmap_set(v, MSR_IA32_FS_BASE);
	msr_write_bitmap_set(v, MSR_IA32_GS_BASE);
	msr_write_bitmap_set(v, MSR_IA32_APIC_BASE);

#ifdef NOTDEF
	/* NRG how do we deal with KERNEL_GS? */
	msr_write_bitmap_set(v, MSR_IA32_KERNEL_GS_BASE);
#endif

#ifdef __x86_64__
	msr_read_bitmap_set(v,  MSR_IA32_EFER);
	msr_write_bitmap_set(v, MSR_IA32_EFER);
#endif

	vmwrite64(VMCE_ADDR_MSR_BITMAPS, kvtophys(v->v_msr_bitmaps));

	vmwrite(VMCE_EXCEPTION_BITMAP, 0);

	vmwrite(VMCE_PAGE_FAULT_ERROR_CODE_MASK, 0);
	vmwrite(VMCE_PAGE_FAULT_ERROR_CODE_MATCH, 0);

	vmwrite64(VMCE_TSC_OFFSET, 0);

	/* NRG host needs to own the VMXE bit */
	vmwrite(VMCE_CR0_GUEST_HOST_MASK, CR0_TRAP_BITS);
	vmwrite(VMCE_CR4_GUEST_HOST_MASK, CR4_TRAP_BITS);

	vmx_update_guest_paging_state(v);

	vmwrite(VMCE_CR3_TARGET_COUNT, 0);
	vmwrite(VMCE_CR3_TARGET_VALUE_0, 0);
	vmwrite(VMCE_CR3_TARGET_VALUE_1, 0);
	vmwrite(VMCE_CR3_TARGET_VALUE_2, 0);
	vmwrite(VMCE_CR3_TARGET_VALUE_3, 0);

	vmwrite64(VMCE_VIRTUAL_APIC_ADDR, 0);
	vmwrite64(VMCE_APIC_ACCESS_ADDR, 0);
	vmwrite(VMCE_TPR_THRESHOLD, 0);

	vmwrite64(VMCE_EXECUTIVE_VMCS_PTR, 0);

	vmwrite64(VMCE_EPT_PTR, mkeptptr(kvtophys(ept_root)));

	u64 exit_ctls = 0;
#ifdef __x86_64__
	exit_ctls |= bit(VMEXIT_CTL_HOST_64);
#endif
	u64 exit_ctls_rqd = get_MSR(cpuinfo.msr_exit_ctls);
	exit_ctls = vmx_add_rqd_bits(exit_ctls,
				     bits(exit_ctls_rqd, 31, 0),
				     bits(exit_ctls_rqd, 63, 32));
	vmwrite(VMCE_VMEXIT_CONTROLS, exit_ctls);

	u64 entry_ctls = 0;
#ifdef __x86_64__
	if (gs->long_mode)
	entry_ctls |= bit(VMENTRY_CTL_GUEST_64);
#endif
	u64 entry_ctls_rqd = get_MSR(cpuinfo.msr_entry_ctls);
	entry_ctls = vmx_add_rqd_bits(entry_ctls,
				      bits(entry_ctls_rqd, 31, 0),
				      bits(entry_ctls_rqd, 63, 32));
	vmwrite(VMCE_VMENTRY_CONTROLS, entry_ctls);

	vmwrite(VMCE_VMEXIT_MSR_STORE_COUNT, 0);
	vmwrite64(VMCE_VMEXIT_MSR_STORE_ADDR, 0);
	vmwrite(VMCE_VMEXIT_MSR_LOAD_COUNT, 0);
	vmwrite64(VMCE_VMEXIT_MSR_LOAD_ADDR, 0);

	vmwrite(VMCE_VMENTRY_MSR_LOAD_COUNT, 0);
	vmwrite64(VMCE_VMENTRY_MSR_LOAD_ADDR, 0);

	vmwrite(VMCE_VMENTRY_INTERRUPT_INFO_FIELD, 0);
	vmwrite(VMCE_VMENTRY_EXCEPTION_ERROR_CODE, 0);
	vmwrite(VMCE_VMENTRY_INSTRUCTION_LENGTH, 0);

	if (!(bp->bp_flags & BPF_NO_EXTINT)) {
		vmx_enable_external_interrupt_exiting();
	}

	if (!(bp->bp_flags & BPF_NO_VAPIC)) {
		vmx_enable_apic_virtualization(v);
	} else {
		vmx_disable_apic_virtualization();
	}

#ifdef NOTDEF
	dump_vmcs();
#endif
	check_vmcs(v);

	vmlaunch_with_regs(&gs->regs, true);

	kprintf("vmx_guest_init>if you see this, vmlaunch() failed\n");

	asm volatile(".global vmx_initial_guest_entry\n\t"
		     "vmx_initial_guest_entry:");
	return;
	/*
	 * Now in guest mode.  Don't put anything here.
	 * Risk of deadlock (spinning with interrupts disabled).
	 */
}

void
vm_entry_failed(registers_t *regs, un rflags)
{
	vm_t *v = &cpu_vm[cpuno()];

	if (bit_test(rflags, FLAGS_CF)) {
		kprintf("vmlaunch/resume VMfailInvalid\n");
	} else if (bit_test(rflags, FLAGS_ZF)) {
		kprintf("vmlaunch/resume error %ld\n",
			vmread(VMCE_VM_INSTRUCTION_ERROR));
	} else {
		kprintf("vmlaunch/resume fail with neither Z or C flags set\n");
	}
	check_vmcs(v);
	vmx_exit(v, regs);
}
