#ifndef __VMX_H__
#define __VMX_H__

#include "vmcs.h"
#include "vm.h"

/* Guest activity states 20.4.2 */
#define VMX_ACTIVITY_STATE_ACTIVE	0
#define VMX_ACTIVITY_STATE_HALT		1
#define VMX_ACTIVITY_STATE_SHUTDOWN	2
#define VMX_ACTIVITY_STATE_WAIT_SIPI	3

/* VM-Exec Pin-Based controls 20.6.1 */
#define VMEXEC_PIN_EXT_INTR_EXIT			0
#define VMEXEC_PIN_NMI_EXIT				3
#define VMEXEC_PIN_VIRTUAL_NMIS				5
#define VMEXEC_PIN_PREEMPTION_TIMER			6

/* VM-Exec CPU Primary controls 20.6.2 */
#define VMEXEC_CPU1_INTR_WINDOW_EXIT			2
#define VMEXEC_CPU1_USE_TSC_OFFSET			3
#define VMEXEC_CPU1_HLT_EXIT				7
#define VMEXEC_CPU1_INVLPG_EXIT				9
#define VMEXEC_CPU1_MWAIT_EXIT				10
#define VMEXEC_CPU1_RDPMC_EXIT				11
#define VMEXEC_CPU1_RDTSC_EXIT				12
#define VMEXEC_CPU1_CR3_LOAD_EXIT			15
#define VMEXEC_CPU1_CR3_STORE_EXIT			16
#define VMEXEC_CPU1_CR8_LOAD_EXIT			19
#define VMEXEC_CPU1_CR8_STORE_EXIT			20
#define VMEXEC_CPU1_USE_TPR_SHADOW			21
#define VMEXEC_CPU1_NMI_WINDOW_EXIT			22
#define VMEXEC_CPU1_MOV_DR_EXIT				23
#define VMEXEC_CPU1_IO_EXIT				24
#define VMEXEC_CPU1_USE_IO_BITMAPS			25
#define VMEXEC_CPU1_MONITOR_TRAP_FLAG			27
#define VMEXEC_CPU1_USE_MSR_BITMAPS			28
#define VMEXEC_CPU1_MONITOR_EXIT			29
#define VMEXEC_CPU1_PAUSE_EXIT				30
#define VMEXEC_CPU1_ACTIVATE_SECONDARY_CTRLS		31

/* VM-Exec CPU Secondary controls 20.6.2 */
#define VMEXEC_CPU2_VIRTUALIZE_APIC			0
#define VMEXEC_CPU2_ENABLE_EPT				1
#define VMEXEC_CPU2_DESCRIPTOR_TABLE_EXIT		2
#define VMEXEC_CPU2_ENABLE_RDTSCP			3
#define VMEXEC_CPU2_VIRTUALIZE_x2APIC			4
#define VMEXEC_CPU2_ENABLE_VPID				5
#define VMEXEC_CPU2_WBINVD_EXIT				6
#define VMEXEC_CPU2_UNRESTRICTED_GUEST			7
#define VMEXEC_CPU2_PAUSE_LOOP_EXIT			10

/* VM-Exit controls 20.7.1 */
#define VMEXIT_CTL_SAVE_DEBUG_CTLS			2
#define VMEXIT_CTL_HOST_64				9
#define VMEXIT_CTL_LOAD_MSR_IA32_PERF_GLOBAL_CTRL	12
#define VMEXIT_CTL_ACK_INTERRUPT			15
#define VMEXIT_CTL_SAVE_IA32_PAT			18
#define VMEXIT_CTL_LOAD_IA32_PAT			19
#define VMEXIT_CTL_SAVE_IA32_EFER			20
#define VMEXIT_CTL_LOAD_IA32_EFER			21
#define VMEXIT_CTL_SAVE_PREEMPTION_TIMER		22

/* VM-Entry controls 20.8.1 */
#define VMENTRY_CTL_LOAD_DEBUG_CTLS			2
#define VMENTRY_CTL_GUEST_64				9
#define VMENTRY_CTL_SMM					10
#define VMENTRY_CTL_NOT_DUAL_MONITOR			11
#define VMENTRY_CTL_LOAD_MSR_IA32_PERF_GLOBAL_CTRL	13
#define VMENTRY_CTL_LOAD_IA32_PAT			14
#define VMENTRY_CTL_LOAD_IA32_EFER			15

/* VM-Exit Basic exit reasons */
#define EXIT_REASON_EXCEPTION				0
#define EXIT_REASON_INTERRUPT				1
#define EXIT_REASON_TRIPLE_FAULT			2
#define EXIT_REASON_INIT_SIGNAL				3
#define EXIT_REASON_SIPI				4
#define EXIT_REASON_IO_SMI				5
#define EXIT_REASON_SMI					6
#define EXIT_REASON_INTERRUPT_WINDOW			7
#define EXIT_REASON_NMI_WINDOW				8
#define EXIT_REASON_TASK_SWITCH				9
#define EXIT_REASON_CPUID				10
#define EXIT_REASON_GETSEC				11
#define EXIT_REASON_HLT					12
#define EXIT_REASON_INVD				13
#define EXIT_REASON_INVLPG				14
#define EXIT_REASON_RDPMC				15
#define EXIT_REASON_RDTSC				16
#define EXIT_REASON_RSM					17
#define EXIT_REASON_VMCALL				18
#define EXIT_REASON_VMCLEAR				19
#define EXIT_REASON_VMLAUNCH				20
#define EXIT_REASON_VMPTRLD				21
#define EXIT_REASON_VMPTRST				22
#define EXIT_REASON_VMREAD				23
#define EXIT_REASON_VMRESUME				24
#define EXIT_REASON_VMWRITE				25
#define EXIT_REASON_VMXOFF				26
#define EXIT_REASON_VMXON				27
#define EXIT_REASON_CR_ACCESS				28
#define EXIT_REASON_MOV_DR				29
#define EXIT_REASON_IO					30
#define EXIT_REASON_RDMSR				31
#define EXIT_REASON_WRMSR				32
#define EXIT_REASON_VMENTRY_FAIL_GUEST_STATE		33
#define EXIT_REASON_VMENTRY_FAIL_MSR_LOADING		34
#define EXIT_REASON_MWAIT				36
#define EXIT_REASON_MONITOR_TRAP_FLAG			37
#define EXIT_REASON_MONITOR				39
#define EXIT_REASON_PAUSE				40
#define EXIT_REASON_VMENTRY_FAIL_MACHINE_CHECK		41
#define EXIT_REASON_TPR_BELOW_THRESHOLD			43
#define EXIT_REASON_APIC_ACCESS				44
#define EXIT_REASON_GDTR_IDTR_ACCESS			46
#define EXIT_REASON_LDTR_TR_ACCESS			47
#define EXIT_REASON_EPT_VIOLATION			48
#define EXIT_REASON_EPT_MISCONFIG			49
#define EXIT_REASON_INVEPT				50
#define EXIT_REASON_RDTSCP				51
#define EXIT_REASON_VMX_PREEMPTION_TIMER_EXPIRED	52
#define EXIT_REASON_INVVPID				53
#define EXIT_REASON_WBINVD				54
#define EXIT_REASON_XSETBV				55

#define LAST_EXIT_REASON				EXIT_REASON_XSETBV

#define CR0_TRAP_BITS (bit(CR0_PG) | bit(CR0_CD) | bit(CR0_NW) | bit(CR0_PE))
#define CR4_TRAP_BITS (bit(CR4_VMXE))

#define IA32_EFER_GUEST_BITS (bit(EFER_SCE) | bit(EFER_NXE))

#define VMCS_LEN			VM_PAGE_SIZE
#define MSR_BITMAPS_LEN			VM_PAGE_SIZE

#define IO_OUTB  0
#define IO_OUTW  1
#define IO_OUTL  3
#define IO_INB   8
#define IO_INW   9
#define IO_INL  11

inline static un
vmxon(u64 addr)
{
	u8 err;
	asm volatile("vmxon %1\n\t"
		     "setbe %0" :
		     "=q" (err) :
		     "m" (addr) :
		     "cc", "memory");
	return err;
}

inline static un
vmxoff(void)
{
	u8 err;
	asm volatile("vmxoff\n\t"
		     "setbe %0" :
		     "=q" (err) : :
		     "cc");
	return err;
}

inline static un
vmclear(u64 addr)
{
	u8 err;
	asm volatile("vmclear %1\n\t"
		     "setbe %0" :
		     "=q" (err) :
		     "m" (addr) :
		     "cc", "memory");
	return err;
}

inline static un
vmptrld(u64 addr)
{
	u8 err;
	asm volatile("vmptrld %1\n\t"
		     "setbe %0" :
		     "=q" (err) :
		     "m" (addr) :
		     "cc");
	return err;
}

inline static un
__vmread(un field)
{
	un res;

	asm volatile("vmread %1, %0" :
		     "=r" (res) :
		     "r" (field) :
		     "cc");
	return res;
}

inline static void
__vmwrite(un field, un value)
{
	u8 fail_valid;
	u8 fail_invalid;
	asm volatile("vmwrite %2, %3\n\t"
		     "setz %0\n\t"
		     "setc %1" :
		     "=q" (fail_valid), "=q" (fail_invalid) :
		     "r" (value), "r" (field) :
		     "cc");
	if (fail_invalid) {
		kprintf("vmwrite(0x%lx, 0x%lx) VMfailInvalid\n",
			field, value);
	} else if (fail_valid) {
		kprintf("vmwrite(0x%lx, 0x%lx) error %ld\n",
			field, value, __vmread(VMCE_VM_INSTRUCTION_ERROR));
	}
}

#define vmread(f) \
({ \
	static_assert(!field_width_is_64(f)); \
	__vmread(f); \
})

#define vmwrite(f, v) \
({ \
	static_assert(!field_width_is_64(f)); \
	__vmwrite((f), (v)); \
})

#ifdef __x86_64__

inline static u64
__vmread64(un field)
{
    return __vmread(field);
}

inline static void
__vmwrite64(un field, u64 val)
{
    return __vmwrite(field, val);
}

#else

inline static u64
__vmread64(un field)
{
	un res;
	un reshi;

	asm volatile("vmread %1, %0" :
		     "=r" (res) :
		     "r" (field) :
		     "cc");
	asm volatile("vmread %1, %0" :
		     "=r" (reshi) :
		     "r" (field|_VMCE_ACCESS_HIGH) :
		     "cc");
	return ((u64)reshi << 32) | res;
}

inline static void
__vmwrite64(un field, u64 value)
{
	u8 fail_valid;
	u8 fail_invalid;
	un valuelo = bits(value, 31, 0);
	un valuehi = bits(value, 63, 32);
	asm volatile("vmwrite %2, %3\n\t"
		     "setz %0\n\t"
		     "setc %1" :
		     "=q" (fail_valid), "=q" (fail_invalid) :
		     "r" (valuelo), "r" (field) :
		     "cc");
	if (fail_invalid) {
		kprintf("vmwrite64(0x%lx, 0x%lx) VMfailInvalid\n",
			field, valuelo);
	} else if (fail_valid) {
		kprintf("vmwrite64(0x%lx, 0x%lx) error %ld\n",
			field, valuelo, vmread(VMCE_VM_INSTRUCTION_ERROR));
	}
	asm volatile("vmwrite %2, %3\n\t"
		     "setz %0\n\t"
		     "setc %1" :
		     "=q" (fail_valid), "=q" (fail_invalid) :
		     "r" (valuehi), "r" (field|_VMCE_ACCESS_HIGH) :
		     "cc");
	if (fail_invalid) {
		kprintf("vmwrite64(0x%lx, 0x%lx) VMfailInvalid\n",
			field|_VMCE_ACCESS_HIGH, valuehi);
	} else if (fail_valid) {
		kprintf("vmwrite64(0x%lx, 0x%lx) error %ld\n",
			field|_VMCE_ACCESS_HIGH, valuehi,
			vmread(VMCE_VM_INSTRUCTION_ERROR));
	}
}
#endif

#define vmread64(f) \
({ \
	static_assert(field_width_is_64(f)); \
	__vmread64(f);	      \
})

#define vmwrite64(f, v) \
({ \
	static_assert(field_width_is_64(f)); \
	__vmwrite64((f), (v)); \
})

extern void vmlaunch_with_regs(const registers_t *regs, bool launch)
    __attribute__((noreturn));


inline static void
skip_emulated_instr(void)
{
	un rip = vmread(VMCE_GUEST_RIP);
	rip += vmread(VMCE_VMEXIT_INSTRUCTION_LENGTH);
	vmwrite(VMCE_GUEST_RIP, rip);
}

inline static u32
vmx_chk_rqd_bits(char *name, u32 bits, u32 fixed0, u32 fixed1)
{
	if ((bits | fixed0) != bits) {
		u32 missing = (bits | fixed0) ^ bits;
		if (name) {
			kprintf("%s required bits 0x%x missing\n", name,
				missing);
		}
		bits ^= missing;
	}
	if ((bits & fixed1) != bits) {
		u32 disallowed = (bits & fixed1) ^ bits;
		if (name) {
			kprintf("%s disallowed bits 0x%x present\n", name,
				disallowed);
		}
		bits ^= disallowed;
	}	

	return bits;
}

#define vmx_add_rqd_bits(b, f0, f1)	vmx_chk_rqd_bits(0, (b), (f0), (f1))

/* VMENTRY_INTERRUPT_INFO_FIELD bits */
#define II_DELIVER_ERRCODE		11
#define II_VALID			31

#define iitype(ii)	bits(ii, 10, 8)

/* Interruption types */
#define IT_EXTERNAL_INTERRUPT		0
/* Reserved				1 */
#define IT_NMI				2
#define IT_HARDWARE_EXCEPTION		3
#define IT_SOFTWARE_INTERRUPT		4
#define IT_PRIV_SOFTWARE_EXCEPTION	5
#define IT_SOFTWARE_EXCEPTION		6
#define IT_OTHER_EVENT			7

/* VMCE_GUEST_INTERRUPTIBILITY_STATE bits */
#define IS_STI				0
#define IS_MOV_SS			1
#define IS_SMI				2
#define IS_NMI				3

/* VMCE_GUEST_PENDING_DEBUG_EXCEPTIONS bits */
#define PD_B0				0
#define PD_B1				1
#define PD_B2				2
#define PD_B3				3
#define PD_BP				12
#define PD_BS				14

inline static bool
guest_usermode(void)
{
	u16 cs = vmread(VMCE_GUEST_CS_SEL);
	return (bits(cs, 1, 0) == 3);
}

inline static bool
guest_interrupts_are_blocked(void)
{
	u32 gis = vmread(VMCE_GUEST_INTERRUPTIBILITY_STATE);
	return (!bit_test(vmread(VMCE_GUEST_RFLAGS), FLAGS_IF) ||
		bit_test(gis, IS_STI) || bit_test(gis, IS_MOV_SS));
}

inline static bool
guest_64(void)
{
	un entry_ctl = vmread(VMCE_VMENTRY_CONTROLS);
	return bit_test(entry_ctl, VMENTRY_CTL_GUEST_64);
}

inline static bool
guest_PAE(void)
{
	return bit_test(vmread(VMCE_GUEST_CR4), CR4_PAE);
}

extern void guest_set_interrupt_pending(u8);

extern void vm_entry_inject(u32, u8, u32);

inline static void
vm_entry_inject_interrupt(u8 intvec)
{
	vm_entry_inject(IT_EXTERNAL_INTERRUPT, intvec, 0);
}

inline static void
vm_entry_inject_exception(u8 vector, u32 errcode)
{
	vm_entry_inject(IT_HARDWARE_EXCEPTION, vector, errcode);
}

inline static void
vm_entry_inject_mtf(void)
{
	vm_entry_inject(IT_OTHER_EVENT, 0, 0);
}

inline static bool
ept_enabled(void)
{
	u32 procbased_ctls2 = vmread(VMCE_SECONDARY_CPU_BASED_CONTROLS);
	return bit_test(procbased_ctls2, VMEXEC_CPU2_ENABLE_EPT);
}

extern void check_vmcs(vm_t *);
extern void dump_vmcs(void);
extern void dump_vmx_control_support(void);

extern bool vmx_exec_cpu1_supported(un);
extern void vmx_set_exec_cpu1(un);
extern void vmx_clear_exec_cpu1(un);

extern bool vmx_exec_cpu2_supported(un);
extern void vmx_set_exec_cpu2(un);
extern void vmx_clear_exec_cpu2(un);

#endif
