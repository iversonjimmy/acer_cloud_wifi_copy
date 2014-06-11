#include "hyp.h"
#include "vmcb.h"

#define OP(name)	kprintf(#name " offset 0x%lx\n", \
			       	offsetof(vmcb_guest_state_t, name))

void
vmcb_dump_offsets(void)
{
	OP(vmcb_guest_es);
	OP(vmcb_guest_cs);
	OP(vmcb_guest_ss);
	OP(vmcb_guest_ds);
	OP(vmcb_guest_fs);
	OP(vmcb_guest_gs);
	OP(vmcb_guest_gdtr);
	OP(vmcb_guest_ldtr);
	OP(vmcb_guest_idtr);
	OP(vmcb_guest_tr);
	OP(vmcb_guest_cpl);
	OP(vmcb_guest_efer);
	OP(vmcb_guest_cr4);
	OP(vmcb_guest_cr3);
	OP(vmcb_guest_cr0);
	OP(vmcb_guest_dr7);
	OP(vmcb_guest_dr6);
	OP(vmcb_guest_rflags);
	OP(vmcb_guest_rip);
	OP(vmcb_guest_rsp);
	OP(vmcb_guest_rax);
	OP(vmcb_guest_star);
	OP(vmcb_guest_lstar);
	OP(vmcb_guest_cstar);
	OP(vmcb_guest_sfmask);
	OP(vmcb_guest_kernelgsbase);
	OP(vmcb_guest_sysenter_cs);
	OP(vmcb_guest_sysenter_esp);
	OP(vmcb_guest_sysenter_eip);
	OP(vmcb_guest_cr2);
	OP(vmcb_guest_pat);
	OP(vmcb_guest_dbgctl);
	OP(vmcb_guest_br_from);
	OP(vmcb_guest_br_to);
	OP(vmcb_guest_lastexcpfrom);
	OP(vmcb_guest_lastexcpto);

#undef OP
#define OP(name)	kprintf(#name " offset 0x%lx\n", \
			       	offsetof(vmcb_control_t, name))

	OP(vmcb_intercept_cr_reads);
	OP(vmcb_intercept_cr_writes);
	OP(vmcb_intercept_dr_reads);
	OP(vmcb_intercept_dr_writes);
	OP(vmcb_intercept_exceptions);
	OP(vmcb_intercept_controls1);
	OP(vmcb_intercept_controls2);
	OP(vmcb_pause_filter_count);
	OP(vmcb_iopm_base_pa);
	OP(vmcb_msrpm_base_pa);
	OP(vmcb_tsc_offset);

	OP(vmcb_exitcode);
	OP(vmcb_exitinfo1);
	OP(vmcb_exitinfo2);
	OP(vmcb_exitintinfo);
	OP(vmcb_eventinj);
	OP(vmcb_n_cr3);
	OP(vmcb_clean_bits);
	OP(vmcb_nrip);
	OP(vmcb_guest_instr_count);
	OP(vmcb_guest_instr);
}
