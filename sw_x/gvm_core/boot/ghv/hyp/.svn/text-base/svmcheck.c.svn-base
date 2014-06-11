#include "hyp.h"
#include "svm.h"

#define vmcb_check(x) \
({ \
	if (!(x)) { \
		kprintf("check failed [%d]: %s\n", \
			__LINE__, #x); \
	 } \
})

inline static bool
check_phys_addr(u64 addr, u8 alignment)
{
	if (alignment && (bits(addr, (alignment - 1), 0) != 0)) {
		return false;
	}
	if (cpu_xfeature_supported(CPU_XFEATURE_INTEL64)) {
		return (bits(addr, 63, cpu_phys_addr_width()) == 0);
	} else {
		return (bits(addr, 63, 32) == 0);
	}
}

void
dump_memory(void *addr, size_t len)
{

	for (un i = 0; i < len; i += 4 * sizeof(u64)) {
		u64 *p = (u64 *)(addr + i);
		kprintf("0x%03lx 0x%016llx 0x%016llx 0x%016llx 0x%016llx\n",
			i, p[0], p[1], p[2], p[3]);
	}
}

void
check_vmcb(vm_t *v)
{
#ifdef NOTDEF
	kprintf("vmcb control:\n");
	dump_memory(v->v_guest_vmcs, 0x100);
	kprintf("vmcb guest:\n");
	dump_memory(v->v_guest_vmcs + 0x400, 0x300);
#endif

	vmcb_guest_state_t *guest = svm_guest_state(v);
	vmcb_control_t *control = svm_control(v);

	u64 efer = guest->vmcb_guest_efer;
	u64 cr0 = guest->vmcb_guest_cr0;
	u64 cr4 = guest->vmcb_guest_cr4;

	vmcb_check(bit_test(efer, EFER_SVME));

	vmcb_check(bit_test(cr0, CR0_CD) || !bit_test(cr0, CR0_NW));
	vmcb_check(bits(cr0, 63, 32) == 0);
	vmcb_check(bits(guest->vmcb_guest_cr3, 63, 52) == 0);
	vmcb_check(bits(cr4, 63, 11) == 0);

	vmcb_check(bits(guest->vmcb_guest_dr6, 63, 32) == 0);
	vmcb_check(bits(guest->vmcb_guest_dr7, 63, 32) == 0);

	vmcb_check(bits(efer, 63, 15) == 0);

	vmcb_check(cpu_xfeature_supported(CPU_XFEATURE_INTEL64) ||
		   (!bit_test(efer, EFER_LMA) && !bit_test(efer, EFER_LME)));

	vmcb_check(!bit_test(efer, EFER_LME) || !bit_test(cr0, CR0_PG) || 
		   bit_test(cr4, CR4_PAE));
	vmcb_check(!bit_test(efer, EFER_LME) || !bit_test(cr0, CR0_PG) || 
		   bit_test(cr0, CR0_PE));

	vmcb_seg_desc_t *cs = &guest->vmcb_guest_cs;
	vmcb_check(bit_test(efer, EFER_LME) || bit_test(cr0, CR0_PG) || 
		   bit_test(cr4, CR4_PAE) || cs->seg_l || cs->seg_d);

	u32 ic1 = control->vmcb_intercept_controls1;
	u32 ic2 = control->vmcb_intercept_controls2;
	vmcb_check(bit_test(ic2, VIC2_VMRUN));

	if (bit_test(ic1, VIC1_IOIO)) {
		u64 addr = control->vmcb_iopm_base_pa;
		if (!check_phys_addr(addr, 12) ||
		    !check_phys_addr(addr + (3 * VM_PAGE_SIZE - 1), 0)) {
			kprintf("vmcb_iopm_base_pa 0x%llx check failed\n",
				addr);

		}
	}
	if (bit_test(ic1, VIC1_MSR)) {
		u64 addr = control->vmcb_msrpm_base_pa;
		if (!check_phys_addr(addr, 12) ||
		    !check_phys_addr(addr + (2 * VM_PAGE_SIZE - 1), 0)) {
			kprintf("vmcb_msrpm_base_pa 0x%llx check failed\n",
				addr);
		}
	}

	/* NRG check for illegal event injection */

	vmcb_check(control->vmcb_guest_asid != 0);
}
