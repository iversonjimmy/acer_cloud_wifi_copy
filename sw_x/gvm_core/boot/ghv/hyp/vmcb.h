#ifndef __VMCB_H__
#define __VMCB_H__

typedef struct {
	u16			vmcb_intercept_cr_reads;			
	u16			vmcb_intercept_cr_writes;			
	u16			vmcb_intercept_dr_reads;			
	u16			vmcb_intercept_dr_writes;			
	u32			vmcb_intercept_exceptions;
	u32			vmcb_intercept_controls1;
#define VIC1_INTR	0
#define VIC1_NMI	1
#define VIC1_SMI	2
#define VIC1_INIT	3
#define VIC1_VINTR	4
#define VIC1_CR0	5
#define VIC1_RD_IDTR	6
#define VIC1_RD_GDTR	7
#define VIC1_RD_LDTR	8
#define VIC1_RD_TR	9
#define VIC1_WR_IDTR	10
#define VIC1_WR_GDTR	11
#define VIC1_WR_LDTR	12
#define VIC1_WR_TR	13
#define VIC1_RDTSC	14
#define VIC1_RDPMC	15
#define VIC1_PUSHF	16
#define VIC1_POPF	17
#define VIC1_CPUID	18
#define VIC1_RSM	19
#define VIC1_IRET	20
#define VIC1_INTn	21
#define VIC1_INVD	22
#define VIC1_PAUSE	23
#define VIC1_HLT	24
#define VIC1_INVLPG	25
#define VIC1_INVLPGA	26
#define VIC1_IOIO	27
#define VIC1_MSR	28
#define VIC1_TASKSW	29
#define VIC1_FERR_FRZ	30
#define VIC1_SHUTDOWN	31
	u32			vmcb_intercept_controls2;
#define VIC2_VMRUN	0
#define VIC2_VMMCALL	1
#define VIC2_VMLOAD	2
#define VIC2_VMSAVE	3
#define VIC2_STGI	4
#define VIC2_CLGI	5
#define VIC2_SKINIT	6
#define VIC2_RDTSCP	7
#define VIC2_ICEBP	8
#define VIC2_WBINVD	9
#define VIC2_MONITOR	10
#define VIC2_MWAIT	11
#define VIC2_MWAIT_COND	12
	u8			vmcb_reserved0[0x3e - 0x14];
	u16			vmcb_pause_filter_count;
	u64			vmcb_iopm_base_pa;
	u64			vmcb_msrpm_base_pa;
	u64			vmcb_tsc_offset;
	u32			vmcb_guest_asid;
	u8			vmcb_tlb_control;
	u8			vmcb_reserved1[3];
	u64			vmcb_v_tpr:8,
				vmcb_v_irq:1,
				vmcb_reserved2:7,
				vmcb_v_intr_prio:4,
				vmcb_v_ign_tpr:1,
				vmcb_reserved3:3,
				vmcb_v_intr_masking:1,
				vmcb_reserved4:7,
				vmcb_v_intr_vector:8,
				vmcb_reserved5:24;
	u64			vmcb_interrupt_shadow:1,
				vmcb_reserved6:63;
	u64			vmcb_exitcode;
	u64			vmcb_exitinfo1;
	u64			vmcb_exitinfo2;
	u64			vmcb_exitintinfo;
	u64			vmcb_np_enable:1,
				vmcb_reserved7:63;
	u8			vmcb_reserved8[0xa8 - 0x98];
	u64			vmcb_eventinj;
	u64			vmcb_n_cr3;
	u64			vmcb_v_lbr_enable:1,
				vmcb_reserved9:63;
	u32			vmcb_clean_bits;
	u32			vmcb_reserved10;
	u64			vmcb_nrip;
	u8			vmcb_guest_instr_count;
	u8			vmcb_guest_instr[15];
} __attribute__ ((packed)) vmcb_control_t;

typedef struct {
	u16	seg_sel;
	union {
		struct {
			u8 seg_type:4, seg_s:1, seg_dpl:2, seg_p:1;
			u8 seg_avl:1, seg_l:1, seg_d:1, seg_g:1;
		};
		u16	seg_attr;
	};
	u32	seg_limit;
	u64	seg_base;
} __attribute__ ((packed)) vmcb_seg_desc_t;

typedef struct {
	vmcb_seg_desc_t		vmcb_guest_es;
	vmcb_seg_desc_t		vmcb_guest_cs;
	vmcb_seg_desc_t		vmcb_guest_ss;
	vmcb_seg_desc_t		vmcb_guest_ds;
	vmcb_seg_desc_t		vmcb_guest_fs;
	vmcb_seg_desc_t		vmcb_guest_gs;
	vmcb_seg_desc_t		vmcb_guest_gdtr;
	vmcb_seg_desc_t		vmcb_guest_ldtr;
	vmcb_seg_desc_t		vmcb_guest_idtr;
	vmcb_seg_desc_t		vmcb_guest_tr;
	u8			vmcb_reserved0[0x2b];
	u8			vmcb_guest_cpl;
	u32			vmcb_reserved1;
	u64			vmcb_guest_efer;
	u8			vmcb_reserved2[0x70];
	u64			vmcb_guest_cr4;
	u64			vmcb_guest_cr3;
	u64			vmcb_guest_cr0;
	u64			vmcb_guest_dr7;
	u64			vmcb_guest_dr6;
	u64			vmcb_guest_rflags;
	u64			vmcb_guest_rip;
	u8			vmcb_reserved3[0x58];
	u64			vmcb_guest_rsp;
	u8			vmcb_reserved4[0x18];
	u64			vmcb_guest_rax;
	u64			vmcb_guest_star;
	u64			vmcb_guest_lstar;
	u64			vmcb_guest_cstar;
	u64			vmcb_guest_sfmask;
	u64			vmcb_guest_kernelgsbase;
	u64			vmcb_guest_sysenter_cs;
	u64			vmcb_guest_sysenter_esp;
	u64			vmcb_guest_sysenter_eip;
	u64			vmcb_guest_cr2;
	u8			vmcb_reserved5[0x20];
	u64			vmcb_guest_pat;
	u64			vmcb_guest_dbgctl;
	u64			vmcb_guest_br_from;
	u64			vmcb_guest_br_to;
	u64			vmcb_guest_lastexcpfrom;
	u64			vmcb_guest_lastexcpto;
} __attribute__ ((packed)) vmcb_guest_state_t;

#endif
