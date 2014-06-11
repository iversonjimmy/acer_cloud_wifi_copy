#include "hyp.h"
#include "vmx.h"
#include "pt.h"
#include "vmxcheck.h"
#include "valloc.h"

static void
dump_bits(char *title, bitname_t names[], u32 controls, bool reserved)
{
	sn n;

	kprintf("%s\n", title);

	while ((n = bit_last(controls)) >= 0) {
		u8 i = 0;
		while (names[i].n < n) {
			i++;
		}
		if (names[i].n == n) {
			kprintf("\t%3ld %s\n", n,  names[i].name);
		} else if (reserved) {
			kprintf("\t%3ld %s\n", n, "reserved");
		}
		bit_clear(controls, n);
	}
}

#define dump_controls(x) \
({ \
	u32 __controls = vmread(x); \
	dump_bits(#x, x ## _BITS, __controls, true); \
})

#define dump_supported_controls(x, msr) \
({ \
 	u64 __msr = msr ? get_MSR(msr) : 0; \
	u32 __controls = bits(__msr, 63, 32); \
	dump_bits(#x " supported", x ## _BITS, __controls, false); \
})

#define dump_unsupported_controls(x, msr) \
({ \
 	u64 __msr = msr ? get_MSR(msr) : 0; \
	u32 __controls = ~bits(__msr, 31, 0) ^ bits(__msr, 63, 32); \
	u32 __common = __controls & bits(__msr, 63, 32); \
	__controls ^= __common; \
	dump_bits(#x " unsupported", x ## _BITS, __controls, false); \
})

void
dump_vmx_control_support(void)
{
	u32 msr = cpuinfo.msr_pinbased_ctls;
	dump_supported_controls(VMCE_PIN_BASED_CONTROLS, msr);
	dump_unsupported_controls(VMCE_PIN_BASED_CONTROLS, msr);

	msr = cpuinfo.msr_procbased_ctls;
	dump_supported_controls(VMCE_PRIMARY_CPU_BASED_CONTROLS, msr);
	dump_unsupported_controls(VMCE_PRIMARY_CPU_BASED_CONTROLS, msr);

	msr = cpuinfo.msr_procbased_ctls2;
	dump_supported_controls(VMCE_SECONDARY_CPU_BASED_CONTROLS, msr);
	dump_unsupported_controls(VMCE_SECONDARY_CPU_BASED_CONTROLS, msr);

	msr = cpuinfo.msr_exit_ctls;
	dump_supported_controls(VMCE_VMEXIT_CONTROLS, msr);
	dump_unsupported_controls(VMCE_VMEXIT_CONTROLS, msr);

	msr = cpuinfo.msr_entry_ctls;
	dump_supported_controls(VMCE_VMENTRY_CONTROLS, msr);
	dump_unsupported_controls(VMCE_VMENTRY_CONTROLS, msr);
}

void
dump_vmcs(void)
{
	dump_controls(VMCE_PIN_BASED_CONTROLS);
	dump_controls(VMCE_PRIMARY_CPU_BASED_CONTROLS);
	dump_controls(VMCE_SECONDARY_CPU_BASED_CONTROLS);
	dump_controls(VMCE_VMEXIT_CONTROLS);
	dump_controls(VMCE_VMENTRY_CONTROLS);
}

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
check_vmcs(vm_t *v)
{
	kprintf("checking vmcs...\n");

	/* VM Execution Control Fields 23.2.1.1 */

	un vmexec_pin = vmread(VMCE_PIN_BASED_CONTROLS);
	u32 control = vmexec_pin;
	u64 msr = get_MSR(cpuinfo.msr_pinbased_ctls);
	vmx_chk_rqd_bits("VMCE_PIN_BASED_CONTROLS", control,
			 bits(msr, 31, 0),
			 bits(msr, 63, 32));

	un vmexec_primary = vmread(VMCE_PRIMARY_CPU_BASED_CONTROLS);
	un vmexec_secondary = 0;

	control = vmexec_primary;
	msr = get_MSR(cpuinfo.msr_procbased_ctls);
	vmx_chk_rqd_bits("VMCE_PRIMARY_CPU_BASED_CONTROLS", control,
			 bits(msr, 31, 0),
			 bits(msr, 63, 32));

	if (bit_test(control, VMEXEC_CPU1_ACTIVATE_SECONDARY_CTRLS)) {
		vmexec_secondary = vmread(VMCE_SECONDARY_CPU_BASED_CONTROLS);
		control = vmexec_secondary;
		msr = get_MSR(cpuinfo.msr_procbased_ctls2);
		vmx_chk_rqd_bits("VMCE_SECONDARY_CPU_BASED_CONTROLS", control,
				 bits(msr, 31, 0),
				 bits(msr, 63, 32));
	}

	msr = get_MSR(MSR_IA32_VMX_MISC);
	control = vmread(VMCE_CR3_TARGET_COUNT);
	if (control > bits(msr, 24, 16)) {
		kprintf("VMCE_CR3_TARGET_COUNT %d check failed\n", control);
	}

	if (bit_test(vmexec_primary, VMEXEC_CPU1_USE_IO_BITMAPS)) {
		u64 addr = vmread64(VMCE_ADDR_IO_BITMAP_A);
		if (!check_phys_addr(addr, 12)) {
			kprintf("VMCE_ADDR_IO_BITMAP_A 0x%llx check failed\n",
				addr);
		}
		addr = vmread64(VMCE_ADDR_IO_BITMAP_B);
		if (!check_phys_addr(addr, 12)) {
			kprintf("VMCE_ADDR_IO_BITMAP_B 0x%llx check failed\n",
				addr);
		}
	}

	if (bit_test(vmexec_primary, VMEXEC_CPU1_USE_MSR_BITMAPS)) {
		u64 addr = vmread64(VMCE_ADDR_MSR_BITMAPS);
		if (!check_phys_addr(addr, 12)) {
			kprintf("VMCE_ADDR_MSR_BITMAPS 0x%llx check failed\n",
				addr);
		}
	}

	if (bit_test(vmexec_primary, VMEXEC_CPU1_USE_TPR_SHADOW)) {
		u64 addr = vmread64(VMCE_VIRTUAL_APIC_ADDR);
		if (!check_phys_addr(addr, 12)) {
			kprintf("VMCE_VIRTUAL_APIC_ADDR 0x%llx check failed\n",
				addr);
		}
		un reg = vmread(VMCE_TPR_THRESHOLD);
		if (bits(reg, 31, 4) != 0) {
			kprintf("VMCE_TPR_THRESHOLD 0x%lx check failed\n",
				reg);
		}	
		if (!bit_test(vmexec_secondary, VMEXEC_CPU2_VIRTUALIZE_APIC)) {
			if (bits(reg, 3, 0) >
			    bits(((u8 *)(v->v_virtual_apic))[0x80], 7,4)) {
				kprintf("VMCE_TPR_THRESHOLD/apic 0x%lx "
					"check failed\n", reg);
			}
		}
	}

	vmcs_check(bit_test(vmexec_pin, VMEXEC_PIN_NMI_EXIT) == 0 ?
		   bit_test(vmexec_pin, VMEXEC_PIN_VIRTUAL_NMIS) == 0 :
		   true);

	vmcs_check(bit_test(vmexec_pin, VMEXEC_PIN_VIRTUAL_NMIS) == 0 ?
		   bit_test(vmexec_primary, VMEXEC_CPU1_NMI_WINDOW_EXIT) == 0 :
		   true);


	if (bit_test(vmexec_secondary, VMEXEC_CPU2_VIRTUALIZE_APIC)) {
		u64 addr = vmread64(VMCE_VIRTUAL_APIC_ADDR);
		if (!check_phys_addr(addr, 12)) {
			kprintf("VMCE_VIRTUAL_APIC_ADDR 0x%llx check failed\n",
				addr);
		}
	}

	vmcs_check(bit_test(vmexec_secondary, VMEXEC_CPU2_VIRTUALIZE_x2APIC) ?
		   (bit_test(vmexec_primary, VMEXEC_CPU1_USE_TPR_SHADOW) &&
		    bit_test(vmexec_secondary, VMEXEC_CPU2_VIRTUALIZE_APIC)) :
		   true);

	vmcs_check(bit_test(vmexec_secondary, VMEXEC_CPU2_ENABLE_VPID) ?
		   vmread(VMCE_VPID) != 0 :
		   true);

	if (bit_test(vmexec_secondary, VMEXEC_CPU2_ENABLE_EPT)) {
		u64 eptptr = vmread64(VMCE_EPT_PTR);
		u64 msr = get_MSR(MSR_IA32_VMX_EPT_VPID_CAP);
		u8 ept_mem_type = bits(eptptr, 2, 0);
		/* ept_mem_type must be one of the supported types */
		vmcs_check((1 << ept_mem_type) && bits(msr, 15, 8)); 
		vmcs_check(bits(eptptr, 5, 3) == 3);
		vmcs_check(bits(eptptr, 11, 6) == 0);
		vmcs_check(bits(eptptr, 63, cpu_phys_addr_width()) == 0);
	}

	bool ug = bit_test(vmexec_secondary, VMEXEC_CPU2_UNRESTRICTED_GUEST);

	vmcs_check(ug ?  bit_test(vmexec_secondary, VMEXEC_CPU2_ENABLE_EPT) :
		   true);

	/* VM-Exit Control Fields 23.2.1.2 */

	un vmexit_controls = vmread(VMCE_VMEXIT_CONTROLS);

	control = vmexit_controls;
	msr = get_MSR(cpuinfo.msr_exit_ctls);
	vmx_chk_rqd_bits("VMCE_VMEXIT_CONTROLS", control,
			 bits(msr, 31, 0),
			 bits(msr, 63, 32));

	vmcs_check(bit_test(vmexec_pin, VMEXEC_PIN_PREEMPTION_TIMER) == 0 ?
		   bit_test(vmexit_controls,
			    VMEXIT_CTL_SAVE_PREEMPTION_TIMER) == 0 :
		   true);

	if (vmread(VMCE_VMEXIT_MSR_STORE_COUNT) != 0) {
		u64 addr = vmread64(VMCE_VMEXIT_MSR_STORE_ADDR);
		if (!check_phys_addr(addr, 4)) {
			kprintf("VMCE_VMEXIT_MSR_STORE_ADDR 0x%llx "
				"check failed\n", addr);
		}
		u64 last_byte = (addr +
				 (vmread(VMCE_VMEXIT_MSR_STORE_COUNT) * 16)
				 - 1);
		if (!check_phys_addr(last_byte, 0)) {
			kprintf("VMCE_VMEXIT_MSR_STORE_ADDR(end) 0x%llx "
				"check failed\n", last_byte);
		}
	}

	/* VM-Entry Control Fields 23.2.1.3 */

	un vmentry_controls = vmread(VMCE_VMENTRY_CONTROLS);

	control = vmentry_controls;
	msr = get_MSR(cpuinfo.msr_entry_ctls);
	vmx_chk_rqd_bits("VMCE_VMENTRY_CONTROLS", control,
			 bits(msr, 31, 0),
			 bits(msr, 63, 32));

	u32 intr_info = vmread(VMCE_VMENTRY_INTERRUPT_INFO_FIELD);
	if (bit_test(intr_info, II_VALID)) {
		u8 intr_type = iitype(intr_info);
		u8 vec = bits(intr_info, 7, 0);
		vmcs_check(intr_type != 1);
		vmcs_check(!bit_test(vmexec_primary,
				     VMEXEC_CPU1_MONITOR_TRAP_FLAG) ?
			   intr_type != IT_OTHER_EVENT :
			   true);
		vmcs_check(intr_type == IT_NMI ? vec == VEC_NMI : true); 
		vmcs_check(intr_type == IT_HARDWARE_EXCEPTION ? vec <= 31 :
			   true); 
		vmcs_check(intr_type == IT_OTHER_EVENT ? vec == 0 :
			   true); 

		if (bit_test(intr_info, II_DELIVER_ERRCODE)) {
			vmcs_check(bit_test(vmread(VMCE_GUEST_CR0), CR0_PE));
			vmcs_check(intr_type == IT_HARDWARE_EXCEPTION);
			vmcs_check((vec == VEC_DF) || (vec == VEC_TS) ||
				   (vec == VEC_NP) || (vec == VEC_SS) ||
				   (vec == VEC_GP) || (vec == VEC_PF) ||
				   (vec == VEC_AC));

			u32 error_code =
				vmread(VMCE_VMENTRY_EXCEPTION_ERROR_CODE);

			vmcs_check(bits(error_code, 31, 15) == 0);
		}

		vmcs_check(bits(intr_info, 30, 12) == 0);

		u32 instr_len = vmread(VMCE_VMENTRY_INSTRUCTION_LENGTH);

		vmcs_check(((intr_type == IT_SOFTWARE_INTERRUPT) ||
			    (intr_type == IT_SOFTWARE_EXCEPTION) ||
			    (intr_type == IT_PRIV_SOFTWARE_EXCEPTION)) ?
			   ((instr_len >=1) && (instr_len <=15)) :
			   true);
	}

	if (vmread(VMCE_VMENTRY_MSR_LOAD_COUNT) != 0) {
		u64 addr = vmread64(VMCE_VMENTRY_MSR_LOAD_ADDR);
		if (!check_phys_addr(addr, 4)) {
			kprintf("VMCE_VMENTRY_MSR_LOAD_ADDR 0x%llx "
				"check failed\n", addr);
		}
		u64 last_byte = (addr +
				 (vmread(VMCE_VMENTRY_MSR_LOAD_COUNT) * 16)
				 - 1);
		if (!check_phys_addr(last_byte, 0)) {
			kprintf("VMCE_VMENTRY_MSR_LOAD_ADDR(end) 0x%llx "
				"check failed\n", last_byte);
		}
	}

	vmcs_check(!v->v_in_smm ?
		   !bit_test(vmentry_controls, VMENTRY_CTL_SMM) &&
		   !bit_test(vmentry_controls, VMENTRY_CTL_NOT_DUAL_MONITOR) :
		   true);
	vmcs_check(!(bit_test(vmentry_controls, VMENTRY_CTL_SMM) &&
		     bit_test(vmentry_controls, VMENTRY_CTL_NOT_DUAL_MONITOR)));

	/* Host Control regs and MSRs 23.2.2 */

	un addr;

	un cr0 = vmread(VMCE_HOST_CR0);
	un cr0_fixed0 = get_MSR(MSR_IA32_VMX_CR0_FIXED0);
	un cr0_fixed1 = get_MSR(MSR_IA32_VMX_CR0_FIXED1);
	vmx_chk_rqd_bits("VMCE_HOST_CR0", cr0, cr0_fixed0, cr0_fixed1);

	un cr4 = vmread(VMCE_HOST_CR4);
	un cr4_fixed0 = get_MSR(MSR_IA32_VMX_CR4_FIXED0);
	un cr4_fixed1 = get_MSR(MSR_IA32_VMX_CR4_FIXED1);
	vmx_chk_rqd_bits("VMCE_HOST_CR4", cr4, cr4_fixed0, cr4_fixed1);

#ifdef __x86_64__
	un cr3 = vmread(VMCE_HOST_CR3);
	if (bits(cr3, 63, cpu_phys_addr_width()) != 0) {
		kprintf("VMCE_HOST_CR3 0x%lx check failed\n", cr3);
	}

	addr = vmread(VMCE_HOST_IA32_SYSENTER_ESP);
	if (!is_canonical_addr(addr)) {
		kprintf("VMCE_HOST_IA32_SYSENTER_ESP 0x%lx "
			"check failed\n", addr);
	}

	addr = vmread(VMCE_HOST_IA32_SYSENTER_EIP);
	if (!is_canonical_addr(addr)) {
		kprintf("VMCE_HOST_IA32_SYSENTER_EIP 0x%lx "
			"check failed\n", addr);
	}
#endif

	if (bit_test(vmexit_controls,
		     VMEXIT_CTL_LOAD_MSR_IA32_PERF_GLOBAL_CTRL)) {
		u64 reg = vmread64(VMCE_HOST_IA32_PERF_GLOBAL_CTRL);
		if ((bits(reg, 63, 35) != 0) || bits(reg, 31, 2) != 0) {
			kprintf("VMCE_HOST_IA32_PERF_GLOBAL_CTRL 0x%llx "
				"check failed\n", reg);
		}
	}

	if (bit_test(vmexit_controls, VMEXIT_CTL_LOAD_IA32_PAT)) {
		u64 reg = vmread64(VMCE_HOST_IA32_PAT);
		for (u8 i = 0; i < 64; i += 8) {
			u8 byte = bits(reg, i + 7, i);
			if ((byte > 7) || ((byte > 1) && (byte < 4))) {
				kprintf("VMCE_HOST_IA32_PAT 0x%llx "
					"check failed\n", reg);
			}
		}
	}

	if (bit_test(vmexit_controls, VMEXIT_CTL_LOAD_IA32_EFER)) {
		u64 reg = vmread64(VMCE_HOST_IA32_EFER);
		if ((bits(reg, 63, 12) != 0) || (bits(reg, 9, 9) != 0) ||
		    (bits(reg, 7, 1) != 0)) {
			kprintf("VMEXIT_CTL_LOAD_IA32_EFER (rsvd) 0x%llx "
				"check failed\n", reg);
		}
		un host64 = bit_test(vmexit_controls, VMEXIT_CTL_HOST_64);
		if ((bit_test(reg, EFER_LME) != host64) ||
		    (bit_test(reg, EFER_LMA) != host64)) {
			kprintf("VMEXIT_CTL_LOAD_IA32_EFER (host64) 0x%llx "
				"check failed\n", reg);
		}
	}

	/* Host Segment and Descriptor Table regs 23.2.3 */

#define sel_check(field) \
({ \
	u16 _sel = vmread(field); \
	if (bits(_sel, 2, 0) != 0) { \
		kprintf("%s 0x%x bits(2,0) must be 0\n", #field, _sel); \
	} \
 	if (field == VMCE_HOST_CS_SEL || field == VMCE_HOST_TR_SEL || \
	    (field == VMCE_HOST_SS_SEL && !bit_test(vmexit_controls, \
						    VMEXIT_CTL_HOST_64))) { \
		if (_sel == 0) { \
			kprintf("%s 0x%x cannot be 0\n", #field, _sel); \
		} \
 	} \
})

	sel_check(VMCE_HOST_CS_SEL);
	sel_check(VMCE_HOST_SS_SEL);
	sel_check(VMCE_HOST_DS_SEL);
	sel_check(VMCE_HOST_ES_SEL);
	sel_check(VMCE_HOST_FS_SEL);
	sel_check(VMCE_HOST_GS_SEL);
	sel_check(VMCE_HOST_TR_SEL);

#ifdef __x86_64__
#define base_address_check(field) \
({ \
	un _addr = vmread(field); \
	if (!is_canonical_addr(_addr)) { \
		kprintf("%s 0x%lx must be canonical\n", #field, _addr); \
	} \
})
	base_address_check(VMCE_HOST_FS_BASE);
	base_address_check(VMCE_HOST_GS_BASE);
	base_address_check(VMCE_HOST_GDTR_BASE);
	base_address_check(VMCE_HOST_IDTR_BASE);
	base_address_check(VMCE_HOST_TR_BASE);
#endif

	/* Address space size 23.2.4 */

	u64 efer = get_MSR(MSR_IA32_EFER);
	if (!bit_test(efer, EFER_LMA)) {
		if (bit_test(vmentry_controls, VMENTRY_CTL_GUEST_64)) {
			kprintf("VMENTRY_CTL_GUEST_64 must not be set\n");
		}
		if (bit_test(vmexit_controls, VMEXIT_CTL_HOST_64)) {
			kprintf("VMEXIT_CTL_HOST_64 must not be set\n");
		}
	} else {
		if (!bit_test(vmexit_controls, VMEXIT_CTL_HOST_64)) {
			kprintf("VMEXIT_CTL_HOST_64 must be set\n");
		}
	}

	addr = vmread(VMCE_HOST_RIP);
	if (!bit_test(vmexit_controls, VMEXIT_CTL_HOST_64)) {
		if (bit_test(vmentry_controls, VMENTRY_CTL_GUEST_64)) {
			kprintf("VMENTRY_CTL_GUEST_64 must not be set\n");
		}
		if (bits(addr, 63, 32) != 0) {
			kprintf("VMCE_HOST_RIP 0x%lx check failed\n", addr);
		}
	} else {
		un cr4 = vmread(VMCE_HOST_CR4);
		if (!bit_test(cr4, CR4_PAE)) {
			kprintf("VMCE_HOST_CR4.PAE must be set\n");
		}
		if (!is_canonical_addr(addr)) {
			kprintf("VMCE_HOST_RIP 0x%lx not canonical\n", addr);
		}
	}

	/* Guest Control Regs, Debug Regs and MSRs 23.3.1.1 */

	if (ug) {
		cr0_fixed0 &= ~(bit(CR0_PE)|bit(CR0_PG));
	}

	cr0 = vmread(VMCE_GUEST_CR0);
	vmx_chk_rqd_bits("VMCE_GUEST_CR0", cr0, cr0_fixed0, cr0_fixed1);

	vmcs_check(bit_test(cr0, CR0_PG) ? bit_test(cr0, CR0_PE) : true);

	cr4 = vmread(VMCE_GUEST_CR4);
	vmx_chk_rqd_bits("VMCE_GUEST_CR4", cr4, cr4_fixed0, cr4_fixed1);

	u64 debugctl = vmread64(VMCE_GUEST_IA32_DEBUGCTL);
	if (bit_test(vmentry_controls, VMENTRY_CTL_LOAD_DEBUG_CTLS)) {
		if ((bits(debugctl, 5, 2) != 0) ||
		    (bits(debugctl, 63, 15) != 0)) {
			kprintf("VMCE_GUEST_IA32_DEBUGCTL 0x%llx "
				"reserved bits check failed\n", debugctl);
		}
#ifdef __x86_64__
		un dr7 = vmread(VMCE_GUEST_DR7);
		vmcs_check(bits(dr7, 63, 32) == 0);
#endif
	}

#ifdef __x86_64__
	vmcs_check(bit_test(vmentry_controls, VMENTRY_CTL_GUEST_64) ?
		   bit_test(cr0, CR0_PG) && bit_test(cr4, CR4_PAE) :
		   true);

	cr3 = vmread(VMCE_GUEST_CR3);
	if (bits(cr3, 63, cpu_phys_addr_width()) != 0) {
		kprintf("VMCE_GUEST_CR3 0x%lx check failed\n", cr3);
	}

	addr = vmread(VMCE_GUEST_IA32_SYSENTER_ESP);
	if (!is_canonical_addr(addr)) {
		kprintf("VMCE_GUEST_IA32_SYSENTER_ESP 0x%lx "
			"check failed\n", addr);
	}

	addr = vmread(VMCE_GUEST_IA32_SYSENTER_EIP);
	if (!is_canonical_addr(addr)) {
		kprintf("VMCE_GUEST_IA32_SYSENTER_EIP 0x%lx "
			"check failed\n", addr);
	}
#endif

	if (bit_test(vmentry_controls,
		     VMENTRY_CTL_LOAD_MSR_IA32_PERF_GLOBAL_CTRL)) {
		u64 reg = vmread64(VMCE_GUEST_IA32_PERF_GLOBAL_CTRL);
		if ((bits(reg, 63, 35) != 0) || (bits(reg, 31, 2) != 0)) {
			kprintf("VMCE_GUEST_IA32_PERF_GLOBAL_CTRL 0x%llx "
				"reserved bits check failed\n", reg);
		}
	}

	if (bit_test(vmentry_controls, VMENTRY_CTL_LOAD_IA32_PAT)) {
		u64 reg = vmread64(VMCE_GUEST_IA32_PAT);
		for (u8 i = 0; i < 64; i += 8) {
			u8 byte = bits(reg, i + 7, i);
			if ((byte > 7) || ((byte > 1) && (byte < 4))) {
				kprintf("VMCE_GUEST_IA32_PAT 0x%llx "
					"check failed\n", reg);
			}
		}
	}

	bool guest64 = bit_test(vmentry_controls, VMENTRY_CTL_GUEST_64);
	if (bit_test(vmentry_controls, VMENTRY_CTL_LOAD_IA32_EFER)) {
		u64 reg = vmread64(VMCE_GUEST_IA32_EFER);
		if ((bits(reg, 63, 12) != 0) || (bits(reg, 9, 9) != 0) ||
		    (bits(reg, 7, 1) != 0)) {
			kprintf("VMENTRY_CTL_LOAD_IA32_EFER (rsvd) 0x%llx "
				"check failed\n", reg);
		}
		if ((bit_test(reg, EFER_LME) != guest64) ||
		    (bit_test(cr0, CR0_PG) &&
		     (bit_test(reg, EFER_LMA) != guest64))) {
			kprintf("VMENTRY_CTL_LOAD_IA32_EFER (guest64) 0x%llx "
				"check failed\n", reg);
		}
	}

	/* Guest Control Segment Regs 23.3.1.2 */

#define VMREAD_SEG(x) \
	u16 x ##_sel = vmread(VMCE_GUEST_ ## x ## _SEL); \
	u32 x ##_limit = vmread(VMCE_GUEST_ ## x ## _LIMIT); \
	u32 x ##_attr = vmread(VMCE_GUEST_ ## x ## _ACCESS_RIGHTS); \
	un  x ##_base = vmread(VMCE_GUEST_ ## x ## _BASE); \
	(void)x ##_sel; \
	(void)x ##_limit; \
	(void)x ##_attr; \
	(void)x ##_base;

#define ASSIGN_SEG(x) \
	sel = x ##_sel; \
	limit =  x ##_limit; \
	attr = x ##_attr; \
	base = x ##_base; \
	name = #x;

	bool v86 = bit_test(vmread(VMCE_GUEST_RFLAGS), FLAGS_VM);

#define usable(attr)	(!bit_test(attr, 16))
#define seg_usable(x)	usable(x ## _attr)
#define rpl(sel)	(bits(sel, 1, 0))
#define seg_rpl(x)	rpl(x ## _sel)
#define seg_ti(x)	(bit_test(x ## _sel, 2))

	VMREAD_SEG(CS);
	VMREAD_SEG(SS);
	VMREAD_SEG(DS);
	VMREAD_SEG(ES);
	VMREAD_SEG(FS);
	VMREAD_SEG(GS);
	VMREAD_SEG(TR);
	VMREAD_SEG(LDTR);

	vmcs_check(!seg_ti(TR));
	vmcs_check(seg_usable(LDTR) ? !seg_ti(LDTR) : true);
	vmcs_check((!v86 && !ug) ? seg_rpl(SS) == seg_rpl(CS) : true); 

#ifdef __x86_64__
	vmcs_check(is_canonical_addr(TR_base));
	vmcs_check(is_canonical_addr(FS_base));
	vmcs_check(is_canonical_addr(GS_base));
	vmcs_check(seg_usable(LDTR) ? is_canonical_addr(LDTR_base) : true);
	vmcs_check(bits(CS_base, 63, 32) == 0);
	vmcs_check(seg_usable(SS) ? bits(SS_base, 63, 32) == 0 : true);
	vmcs_check(seg_usable(DS) ? bits(DS_base, 63, 32) == 0 : true);
	vmcs_check(seg_usable(ES) ? bits(ES_base, 63, 32) == 0 : true);
#endif

	if (v86) {
		vmcs_check(CS_base == (CS_sel << 4));
		vmcs_check(SS_base == (SS_sel << 4));
		vmcs_check(DS_base == (DS_sel << 4));
		vmcs_check(ES_base == (ES_sel << 4));
		vmcs_check(FS_base == (FS_sel << 4));
		vmcs_check(GS_base == (GS_sel << 4));

		vmcs_check(CS_limit == 0xffff);
		vmcs_check(SS_limit == 0xffff);
		vmcs_check(DS_limit == 0xffff);
		vmcs_check(ES_limit == 0xffff);
		vmcs_check(FS_limit == 0xffff);
		vmcs_check(GS_limit == 0xffff);

		vmcs_check(CS_attr == 0xf3);
		vmcs_check(SS_attr == 0xf3);
		vmcs_check(DS_attr == 0xf3);
		vmcs_check(ES_attr == 0xf3);
		vmcs_check(FS_attr == 0xf3);
		vmcs_check(GS_attr == 0xf3);
	} else {
		u16 sel = 0;
		u32 limit = 0;
		u32 attr = 0;
		un base = 0;
		char *name = "";
		for (u8 i = 0; i < 6; i++) {
			switch (i) {
			case 0:
				ASSIGN_SEG(CS);
				break;
			case 1:
				ASSIGN_SEG(SS);
				break;
			case 2:
				ASSIGN_SEG(DS);
				break;
			case 3:
				ASSIGN_SEG(ES);
				break;
			case 4:
				ASSIGN_SEG(FS);
				break;
			case 5:
				ASSIGN_SEG(GS);
				break;
			}
			u8 type = bits(attr, 3, 0);
			u8 s = bits(attr, 4, 4);
			u8 dpl = bits(attr, 6, 5);
			u8 p = bits(attr, 7, 7);
			u8 db = bits(attr, 14, 14);
			u8 g = bits(attr, 15, 15);
			switch (i) {
			case 0:
				/* Type */
				seg_check(name, (type == 9) || (type == 11) ||
					  (type == 13) || (type == 15) ||
					  (ug ? (type == 3) : false));
				/* S */
				seg_check(name, s == 1);
				/* DPL */
				seg_check(name, type == 3 ? dpl == 0 : true);
				seg_check(name, type == 9 || type == 11 ?
					  dpl == bits(SS_attr, 6, 5) : true);
				seg_check(name, type == 13 || type == 15 ?
					  dpl <= bits(SS_attr, 6, 5) : true);
				/* P */
				seg_check(name, p == 1);
				/* reserved 11:8 */
				seg_check(name, bits(attr, 11, 8) == 0);
				/* db */
				seg_check(name, guest64 && bit_test(attr, 13) ?
					 db == 0 : true); 
				/* g */
				seg_check(name, bits(limit, 11, 0) != 0xfff ?
					  g == 0 : true);
				seg_check(name, bits(limit, 31, 20) != 0 ?
					  g == 1 : true);
				/* reserved 31:17 */
				seg_check(name, bits(attr, 31, 17) == 0);
				break;
			case 1:
				/* Type */
				seg_check(name, seg_usable(SS) ?
					  (type == 3) || (type == 7) : true);
				/* DPL */
				seg_check(name, !ug ? dpl == rpl(sel) : true);
				seg_check(name, (bits(CS_attr, 3, 0) == 3) ||
					  !bit_test(cr0, CR0_PE) ?
					  dpl == 0 : true);
				goto rest_default;
				break;
			default:
				/* Type */
				seg_check(name, usable(attr) ?
					  bit_test(type, 0) : true);
				seg_check(name, usable(attr) &&
						     bit_test(type, 3) ?
						     bit_test(type, 1) : true);
				/* DPL */
				seg_check(name, !ug && usable(attr) &&
					  (type <= 11) ?
					  dpl >= rpl(sel) : true); 
rest_default:
				/* S */
				seg_check(name, usable(attr) ? s == 1 : true);
				/* P */
				seg_check(name, usable(attr) ? p == 1 : true);
				/* reserved 11:8 */
				seg_check(name, usable(attr) ?
					  bits(attr, 11, 8) == 0 : true);
				/* g */
				seg_check(name, usable(attr) &&
					  bits(limit, 11, 0) != 0xfff ?
					  g == 0 : true);
				seg_check(name, usable(attr) &&
					  bits(limit, 31, 20) != 0 ?
					  g == 1 : true);
				/* reserved 31:17 */
				seg_check(name, usable(attr) ?
					  bits(attr, 31, 17) == 0 : true);
				break;
			}
		}
	}

	u16 sel;
	u32 limit;
	u32 attr;
	un base;
	char *name;

	ASSIGN_SEG(TR);
	u8 type = bits(attr, 3, 0);
	u8 s = bits(attr, 4, 4);
	u8 p = bits(attr, 7, 7);
	u8 g = bits(attr, 15, 15);
	/* Type */
	seg_check(name, guest64 ? type == 11 : type == 3 || type == 11);
	/* S */
	seg_check(name, s == 0);
	/* P */
	seg_check(name, p == 1);
	/* reserved 11:8 */
	seg_check(name, bits(attr, 11, 8) == 0);
	/* g */
	seg_check(name, bits(limit, 11, 0) != 0xfff ? g == 0 : true);
	seg_check(name, bits(limit, 31, 20) != 0 ? g == 1 : true);
	/* usable */
	seg_check(name, usable(attr));
	/* reserved 31:17 */
	seg_check(name, bits(attr, 31, 17) == 0);

	ASSIGN_SEG(LDTR);
	type = bits(attr, 3, 0);
	s = bits(attr, 4, 4);
	p = bits(attr, 7, 7);
	g = bits(attr, 15, 15);
	/* Type */
	seg_check(name, usable(attr) ? type == 2 : true);
	/* S */
	seg_check(name, usable(attr) ? s == 0 : true);
	/* P */
	seg_check(name, usable(attr) ? p == 1 : true);
	/* reserved 11:8 */
	seg_check(name, usable(attr) ? bits(attr, 11, 8) == 0 : true);
	/* g */
	seg_check(name, usable(attr) && bits(limit, 11, 0) != 0xfff ?
		  g == 0 : true);
	seg_check(name, usable(attr) && bits(limit, 31, 20) != 0 ?
		  g == 1 : true);
	/* reserved 31:17 */
	seg_check(name, usable(attr) ? bits(attr, 31, 17) == 0 : true);

	/* Guest Descriptor Table Regs 23.3.1.3 */

	limit = vmread(VMCE_GUEST_GDTR_LIMIT);

#ifdef __x86_64__
	base_address_check(VMCE_GUEST_GDTR_BASE);
	base_address_check(VMCE_GUEST_IDTR_BASE);
#endif
	vmcs_check(bits(vmread(VMCE_GUEST_GDTR_LIMIT), 31, 16) == 0);
	vmcs_check(bits(vmread(VMCE_GUEST_IDTR_LIMIT), 31, 16) == 0);

	/* Guest RIP and RFLAGS 23.3.1.4 */

#ifdef __x86_64__
	un guest_rip = vmread(VMCE_GUEST_RIP);
	vmcs_check(!guest64 || !bit_test(CS_attr, 13) ?
		   bits(guest_rip, 63, 32) == 0 :
		   true);
	vmcs_check(guest64 && bit_test(CS_attr, 13) ?
		   is_canonical_addr(guest_rip) :
		   true);

#endif
	un guest_rflags = vmread(VMCE_GUEST_RFLAGS);
	un rflags_mask = ~((1UL << 22) - 1);
	rflags_mask |= bit(15)|bit(5)|bit(3)|bit(1);
	vmcs_check((guest_rflags & rflags_mask) == bit(1));

	vmcs_check(guest64 || !bit_test(cr0, CR0_PE) ?
		   !bit_test(guest_rflags, FLAGS_VM) :
		   true);
	vmcs_check(bit_test(intr_info, II_VALID) &&
		   (iitype(intr_info) == IT_EXTERNAL_INTERRUPT) ?
		   bit_test(guest_rflags, FLAGS_IF) :
		   true);

	/* Guest non-register state 23.3.1.5 */

	u32 activity_state = vmread(VMCE_GUEST_ACTIVITY_STATE);
	u32 interruptibility_state = vmread(VMCE_GUEST_INTERRUPTIBILITY_STATE);
	msr = get_MSR(MSR_IA32_VMX_MISC);

	vmcs_check((activity_state <= 3) &&
		   (activity_state > 0 ? bit_test(msr, activity_state + 5) :
		    true));
	vmcs_check(bits(SS_attr, 6, 5) != 0 ?
		   activity_state != VMX_ACTIVITY_STATE_HALT :
		   true);
	vmcs_check(bit_test(interruptibility_state, IS_STI) ||
		   bit_test(interruptibility_state, IS_MOV_SS) ?
		   activity_state == VMX_ACTIVITY_STATE_ACTIVE :
		   true);

	if (bit_test(intr_info, II_VALID)) {
		u8 intr_type = iitype(intr_info);
		u8 vec = bits(intr_info, 7, 0);
		switch (activity_state) {
		case VMX_ACTIVITY_STATE_HALT:
			if ((intr_type == IT_EXTERNAL_INTERRUPT) ||
			    (intr_type == IT_NMI)) {
				break;
			}
			if ((intr_type == IT_HARDWARE_EXCEPTION) &&
			    ((vec == VEC_DB) || (vec == VEC_MC))) {
				break;
			}
			if ((intr_type == IT_OTHER_EVENT) && (vec == 0)) {
				break;
			}
			kprintf("check failed: invalid interrupt (type %d "
				"vector %d) delivery attempt in HALT state\n",
				intr_type, vec);
			break;
		case VMX_ACTIVITY_STATE_SHUTDOWN:
			if (intr_type == IT_NMI) {
				break;
			}
			if ((intr_type == IT_HARDWARE_EXCEPTION) &&
			    (vec == VEC_MC)) {
				break;
			}
			kprintf("check failed: invalid interrupt (type %d "
				"vector %d) delivery attempt in SHUTDOWN "
				"state\n",
				intr_type, vec);
			break;
		case VMX_ACTIVITY_STATE_WAIT_SIPI:
			kprintf("check failed: invalid interrupt (type %d "
				"vector %d) delivery attempt in WAIT_SIPI "
				"state\n",
				intr_type, vec);
			break;
		}
	}

	vmcs_check(bit_test(vmentry_controls, VMENTRY_CTL_SMM) ?
		   activity_state != VMX_ACTIVITY_STATE_WAIT_SIPI :
		   true);

	vmcs_check(bits(interruptibility_state, 31, 4) == 0);
	vmcs_check(!(bit_test(interruptibility_state, IS_STI) &&
		     bit_test(interruptibility_state, IS_MOV_SS)));
	vmcs_check(!bit_test(guest_rflags, FLAGS_IF) ?
		   !bit_test(interruptibility_state, IS_STI) :
		   true);
	vmcs_check(bit_test(intr_info, II_VALID) &&
		   iitype(intr_info) == IT_EXTERNAL_INTERRUPT ?
		   !bit_test(interruptibility_state, IS_STI) &&
		   !bit_test(interruptibility_state, IS_MOV_SS) :
		   true);
	vmcs_check(bit_test(intr_info, II_VALID) &&
		   iitype(intr_info) == IT_NMI ?
		   !bit_test(interruptibility_state, IS_MOV_SS) :
		   true);
	vmcs_check(!v->v_in_smm ? !bit_test(interruptibility_state, IS_SMI):
		   true);
	vmcs_check(bit_test(vmentry_controls, VMENTRY_CTL_SMM) ?
		   bit_test(interruptibility_state, IS_SMI):
		   true);
	/* Some processors may require this */
	vmcs_check(bit_test(intr_info, II_VALID) &&
		   iitype(intr_info) == IT_NMI ?
		   !bit_test(interruptibility_state, IS_STI) :
		   true);
	vmcs_check(bit_test(vmexec_pin, VMEXEC_PIN_VIRTUAL_NMIS) &&
		   bit_test(intr_info, II_VALID) &&
		   iitype(intr_info) == IT_NMI ?
		   !bit_test(interruptibility_state, IS_NMI) :
		   true);

	un pending_debug = vmread(VMCE_GUEST_PENDING_DEBUG_EXCEPTIONS);
	un pending_debug_mask = ~((1UL << 15) - 1);
	pending_debug_mask |= bit(13) | 0xff0;
	vmcs_check((pending_debug & pending_debug_mask) == 0);

	if (bit_test(interruptibility_state, IS_STI) ||
	    bit_test(interruptibility_state, IS_MOV_SS) ||
	    (activity_state == VMX_ACTIVITY_STATE_HALT)) {
		vmcs_check(bit_test(guest_rflags, FLAGS_TF) &&
			   !bit_test(debugctl, DB_BTF) ?
			   bit_test(pending_debug, PD_BS) :
			   true);
		vmcs_check(!bit_test(guest_rflags, FLAGS_TF) ||
			   bit_test(debugctl, DB_BTF) ?
			   !bit_test(pending_debug, PD_BS) :
			   true);
	}

	u64 linkptr = vmread64(VMCE_VMCS_LINK_PTR);
	/* For now, keep it unused */
	vmcs_check(linkptr == -1ULL);
	if (linkptr != -1ULL) {
		check_phys_addr(linkptr, 12);
		/* More checks go here if this field is ever needed. */
	}

	/* Guest Page Directory Pointer Table Entries 23.3.1.6 */

#define pdpte_check(pdpte) \
({ \
	if (bit_test(pdpte, PTE_PRESENT)) { \
		vmcs_check(bits(pdpte, 2, 1) == 0); \
		vmcs_check(bits(pdpte, 8, 5) == 0); \
		paddr_t pdpte ## _addr = bits64(pdpte, 63, 12) << VM_PAGE_SHIFT; \
		vmcs_check(check_phys_addr(pdpte ## _addr, 0)); \
	} \
})

	if (bit_test(cr0, CR0_PG) && bit_test(cr4, CR4_PAE) && !guest64) {
		u64 pdpte0;
		u64 pdpte1;
		u64 pdpte2;
		u64 pdpte3;

		if (bit_test(vmexec_secondary, VMEXEC_CPU2_ENABLE_EPT)) {
			pdpte0 = vmread64(VMCE_GUEST_PDPTE0);
			pdpte1 = vmread64(VMCE_GUEST_PDPTE1);
			pdpte2 = vmread64(VMCE_GUEST_PDPTE2);
			pdpte3 = vmread64(VMCE_GUEST_PDPTE3);
		} else {
			un cr3 = vmread(VMCE_GUEST_CR3);
			u32 guest_pdpt_page = bits(cr3, 31, 12) << 12;
			u32 pdpt_off  = bits(cr3, 11,  5) << 5;
			addr_t pdpt_page;
			u64 *pdpt;

			pdpt_page = map_page(guest_pdpt_page, 0);
			pdpt = (u64 *)(pdpt_page + pdpt_off);

			pdpte0 = pdpt[0];
			pdpte1 = pdpt[1];
			pdpte2 = pdpt[2];
			pdpte3 = pdpt[3];

			unmap_page(pdpt_page, UMPF_LOCAL_FLUSH);
		}

		pdpte_check(pdpte0);
		pdpte_check(pdpte1);
		pdpte_check(pdpte2);
		pdpte_check(pdpte3);
	}

	kprintf("checking vmcs... done\n");
}
