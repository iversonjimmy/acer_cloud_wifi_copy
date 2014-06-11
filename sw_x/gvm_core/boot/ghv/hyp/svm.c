#include "hyp.h"
#include "kvtophys.h"
#include "pt.h"
#include "dt.h"
#include "valloc.h"
#include "ept.h"
#include "apic.h"
#include "vtd.h"
#include "interrupt.h"
#include "svm.h"
#include "vmcb.h"
#include "notify.h"
#include "page.h"
#include "vapic.h"
#include "gpt.h"
#include "prot.h"

#include "ghv_guest.h"
#include "ghv_mem.h"

extern void exit(int) __attribute__((noreturn));

static void svm_load_guest_state(vm_t *v, const guest_state_t *gs);

extern bool ghv_injected;


static ret_t svm_init_devices(void);

static void *
alloc_vmcb(void)
{
	static_assert(VMCB_LEN == VM_PAGE_SIZE);
	void *vmcb = (void *)page_alloc();

	if (!vmcb) {
		kprintf("alloc_vmcb>kalloc failed\n");
		return NULL;
	}

	if (bits((un)vmcb, 11, 0) != 0) {
		kprintf("alloc_vmcb>vmcb is not 4KB aligned\n");
		return NULL;
	}

	return vmcb;
}

static sn
svm_alloc(vm_t *v)
{
	v->v_vmxon_region = alloc_vmcb();
	if (!v->v_vmxon_region) {
		kprintf("svm_alloc>alloc_vmcb() returned NULL\n");
		return -ENOMEM;
	}
	v->v_host_vmcb = alloc_vmcb();
	if (!v->v_host_vmcb) {
		return -ENOMEM;
	}
	v->v_guest_vmcs = alloc_vmcb();
	if (!v->v_guest_vmcs) {
		return -ENOMEM;
	}
	v->v_stack = alloc_stack(v->v_cpuno, VSTACK_LEN, false);
	if (!v->v_stack) {
		return -ENOMEM;
	}
#ifdef NOTDEF
	static_assert(MSR_BITMAPS_LEN == VM_PAGE_SIZE);
	v->v_msr_bitmaps = (void *)page_alloc();
	if (!v->v_msr_bitmaps) {
		return -ENOMEM;
	}
#endif

	list_init(&v->v_notes_list);
	lock_init(&v->v_notes_lock);

	return 0;
}

static void
svm_free(vm_t *v)
{
	page_free((addr_t)v->v_vmxon_region);	v->v_vmxon_region = NULL;
	page_free((addr_t)v->v_host_vmcb);	v->v_host_vmcb = NULL;
	page_free((addr_t)v->v_guest_vmcs);	v->v_guest_vmcs = NULL;
	free_stack(v->v_stack);			v->v_stack = NULL;
	page_free((addr_t)v->v_msr_bitmaps);	v->v_msr_bitmaps = NULL;
}

void
svm_free_all(void)
{
	for_each_os_cpu(i) {
		svm_free(&cpu_vm[i]);
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
svm_alloc_all(void)
{
	ret_t ret;

	extern void svm_on(void);
	extern void svm_off(void);
	extern void svm_guest_init(void);
	extern void svm_free_all(void);

	bp->bp_vm_on = svm_on;
	bp->bp_vm_off = svm_off;
	bp->bp_vm_guest_init = svm_guest_init;
	bp->bp_vm_free_all = svm_free_all;

	kprintf("svm_alloc_all>first kprintf()\n");

	if (!cpu_xfeature_supported(CPU_XFEATURE_SVM)) {
		kprintf("svm_alloc_all>SVM CPU support NOT found\n");
		ret = -ENXIO;
		goto out;
	} else if (bp->bp_flags & BPF_TEST_HARDWARE) {
		kprintf("SVM CPU support found\n");
	}

	u64 vm_cr = get_MSR(MSR_VM_CR);
	bool svmdis = bit_test(vm_cr, 4);
	bool lock = bit_test(vm_cr, 3);
	bool lock_support = svm_lock_support();
	kprintf("svm_alloc_all>svm_lock_support() returned %d\n", lock_support);
	if (svmdis) {
		if (!lock_support) {
			kprintf("svm_alloc_all>MSR_VM_CR not set up"
				" correctly by BIOS\n");
			ret = -ENXIO;
			goto out;
		} else {
			kprintf("svm_alloc_all>MSR_VM_CR clearing SVMDIS"
				" and setting LOCK\n");
			bit_clear(vm_cr, 4);
			bit_set(vm_cr, 3);
			set_MSR(MSR_VM_CR, vm_cr);
		}
	} else if (!lock) {
		kprintf("svm_alloc_all>MSR_VM_CR setting LOCK\n");
		bit_set(vm_cr, 3);
		set_MSR(MSR_VM_CR, vm_cr);
	}	

	svmdis = bit_test(vm_cr, 4);
	if (svmdis) {
		kprintf("svm_alloc_all>MSR_VM_CR failed to clear SVMDIS\n");
		ret = -ENXIO;
		goto out;
	}

	if (bp->bp_flags & BPF_TEST_HARDWARE) {
		kprintf("MSR_VM_CR set up correctly\n");
	}

	if (bp->bp_flags & BPF_TEST_HARDWARE) {
		/* If BPF_TEST_HARDWARE, bail out here */
		ret = -EINVAL;
		goto out;
	}


	ret = idt_construct();
	if (ret < 0) {
		goto out;
	}

	/* Standalone hypervisor should have called pt_construct() a long time
	   ago, and should be using the new pagetables already */

	ret = init_valloc();
	if (ret < 0) {
		goto out;
	}

#ifdef NOTDEF
	ret = ept_construct();
	if (ret < 0) {
		goto out;
	}
#endif

	dma_test_page = page_alloc();
	if (!dma_test_page) {
		ret = -ENOMEM;
		goto out;
	}

	for (un i = 1; i < 64; i++) {
		((u32 *)dma_test_page)[i] = 0xcafebabe;
	}

	for_each_os_cpu(i) {
		kprintf("svm_alloc_all>allocating for CPU %ld\n", i);

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
		ret = svm_alloc(&cpu_vm[i]);
		if (ret < 0) {
			goto out;
		}
	}

	ret = svm_init_devices();

out:
	if (ret < 0) {
		svm_free_all();
	}

	return ret;
}

#ifdef NOTDEF

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
#endif

void
svm_on(void)
{
	assert(irq_is_disabled());

	kprintf("svm_on>ENTRY\n");

	un n = cpuno();
	vm_t *v = &cpu_vm[n];
	ret_t ret = 0;
	bp->ret[n] = 0;

	if (!cpu_xfeature_supported(CPU_XFEATURE_SVM)) {
		kprintf("svm_on>SVM CPU support NOT found\n");
		ret = -ENXIO;
		goto out;
	}


	u64 efer = get_MSR(MSR_IA32_EFER);
	if (bit_test(efer, EFER_SVME)) {
		kprintf("svm_on>already in SVM operation\n");
		ret = -EEXIST;
		goto out;
	}

	bit_set(efer, EFER_SVME);
	set_MSR(MSR_IA32_EFER, efer);

	efer = get_MSR(MSR_IA32_EFER);
	if (!bit_test(efer, EFER_SVME)) {
		kprintf("svm_on>set EFER_SVME failed\n");
		ret = -ENXIO;
	} else {
		kprintf("svm_on>set EFER_SVME succeeded!\n");
		set_MSR(MSR_VM_HSAVE_PA, kvtophys(v->v_vmxon_region));
		ept_invalidate_global();
	}

out:
	bp->ret[n] = ret;
}

inline static void
__svm_off(vm_t *v)
{
	/* It's not safe to call kprintf() from here. */
	u64 efer = get_MSR(MSR_IA32_EFER);
	if (bit_test(efer, EFER_SVME)) {
		ept_invalidate_global();
		bit_clear(efer, EFER_SVME);
		set_MSR(MSR_IA32_EFER, efer);
	}
}

void
svm_off(void)
{
	assert(irq_is_disabled());

	vm_t *v = &cpu_vm[cpuno()];

	__svm_off(v);
}

static inline void
svm_enable_external_interrupt_exiting(vm_t *v)
{
	vmcb_control_t *control = svm_control(v);

	bit_set(control->vmcb_intercept_controls1, VIC1_INTR);
}

static inline void
svm_disable_external_interrupt_exiting(vm_t *v)
{
	vmcb_control_t *control = svm_control(v);

	bit_clear(control->vmcb_intercept_controls1, VIC1_INTR);
}

static void
svm_entry_inject_vintr(vm_t *v, u8 intvec)
{
	vmcb_control_t *control = svm_control(v);

	control->vmcb_v_intr_vector = intvec;
	control->vmcb_v_intr_prio = interrupt_pri(intvec);
	control->vmcb_v_irq = 1;
}

inline static sn
guest_vintr_injection_pending(vm_t *v)
{
	vmcb_control_t *control = svm_control(v);
	if (control->vmcb_v_irq) {
		return control->vmcb_v_intr_vector;
	}
	return -1;
}

void
svm_entry_inject(vm_t *v, u32 interruption_type, u8 vector, u32 errcode)
{
	vmcb_control_t *control = svm_control(v);
	u64 eventinj = vector;
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
	default:
		kprintf("vm_entry_inject>unknown interruption_type %d\n",
			interruption_type);
		return;
	}

	eventinj |= (interruption_type << 8);
	if (deliver_error_code) {
		bit_set(eventinj, II_DELIVER_ERRCODE);
		eventinj |= ((u64)errcode << 32);
	}

	bit_set(eventinj, II_VALID);

	control->vmcb_eventinj = eventinj;
}

inline static bool
guest_injection_pending(vm_t *v)
{
	vmcb_control_t *control = svm_control(v);
	return bit_test(control->vmcb_eventinj, II_VALID);
}

static void
guest_set_interrupt_pending(vm_t *v, u8 intvec)
{
	vapic_t *va = &v->v_vapic;

	assert(!bitmap_test(va->va_irr, intvec));
	bitmap_set(va->va_irr, intvec);
}

static bool
deliver_pending_interrupt(void)
{
	vm_t *v = &cpu_vm[cpuno()];
	vapic_t *va = &v->v_vapic;
	vmcb_control_t *control = svm_control(v);

	sn r = bitmap_first(va->va_irr, NINTVECS);
	if (r < 0) {
		/* No more interrupts pending */
		bit_clear(control->vmcb_intercept_controls1, VIC1_IRET);
		return false;
	}

	sn s = guest_vintr_injection_pending(v);
	if (interrupt_pri(r) <= interrupt_pri(s)) {
		kprintf("deliver_pending_interrupt>s=%ld pri=%d\n",
			s, interrupt_pri(s));
		bit_set(control->vmcb_intercept_controls1, VIC1_IRET);
		return true;
	} else if (s >= 0) {
		bitmap_set(va->va_irr, s);
	}

	if (r >= VEC_FIRST_USER) {
		svm_entry_inject_vintr(v, r);
		bit_set(control->vmcb_intercept_controls1, VIC1_IRET);
	}
	bitmap_clear(va->va_irr, r);

	return true;
}

static bool
vm_exit_vintr(vm_t *v, registers_t *regs)
{
	sn s = guest_vintr_injection_pending(v);
	kprintf("vm_exit_vintr>guest_vintr_injection_pending() = %ld\n", s);

	return true;
}

static bool
vm_exit_iret(vm_t *v, registers_t *regs)
{
	vmcb_guest_state_t *guest = svm_guest_state(v);

	assert(regs->rsp == guest->vmcb_guest_rsp);
	assert(v->v_guest_64);

	if (v->v_guest_64) {
		struct {
			u64 rip;
			u64 cs;
			u64 rflags;
			u64 rsp;
			u64 ss;
		} iret;

		ret_t ret = copy_from_guest(&iret, regs->rsp, sizeof iret);
		if (ret < 0) {
			kprintf("vm_exit_iret>copy stack ret %ld\n",
				ret);
			return false;
		}

		if ((iret.cs != guest->vmcb_guest_cs.seg_sel)) {
			seg_desc_t desc;
			addr_t desc_addr = guest->vmcb_guest_gdtr.seg_base;
			desc_addr += (iret.cs & ~7);

			ret = copy_from_guest(&desc, desc_addr, sizeof desc);
			if (ret < 0) {
				kprintf("vm_exit_iret>copy CS desc ret %ld\n",
					ret);
				return false;
			}

			vmcb_seg_desc_t *vdesc = &guest->vmcb_guest_cs;
			vdesc->seg_sel = iret.cs;
			vdesc->seg_base = seg_desc_base(&desc, v->v_guest_64);
			vdesc->seg_limit = seg_desc_limit(&desc);
			vdesc->seg_attr = vmx_to_svm_attr(desc.seg_attr &
							  ~ATTR_RESERVED);
			guest->vmcb_guest_cpl = RPL(vdesc->seg_sel);
		}

		if ((iret.ss != guest->vmcb_guest_ss.seg_sel)) {
			vmcb_seg_desc_t *vdesc = &guest->vmcb_guest_ss;

			vdesc->seg_sel = iret.ss;
			if (vdesc->seg_sel == 0) {
				vdesc->seg_base = 0;
				vdesc->seg_limit = 0;
				vdesc->seg_attr = 0;
			} else {
				seg_desc_t desc;
				addr_t desc_addr =
					guest->vmcb_guest_gdtr.seg_base;
				desc_addr += (iret.ss & ~7);

				ret = copy_from_guest(&desc, desc_addr,
						      sizeof desc);
				if (ret < 0) {
					kprintf("vm_exit_iret>copy SS desc "
						"ret %ld\n", ret);
					return false;
				}

				vdesc->seg_base = seg_desc_base(&desc,
								v->v_guest_64);
				vdesc->seg_limit = seg_desc_limit(&desc);
				vdesc->seg_attr =
					vmx_to_svm_attr(desc.seg_attr &
							~ATTR_RESERVED);
			}
		}

		guest->vmcb_guest_rip = iret.rip;
		guest->vmcb_guest_rflags = iret.rflags;
		guest->vmcb_guest_rsp = regs->rsp = iret.rsp;
	}

	return true;
}

static bool
vm_exit_hardware_interrupt(vm_t *v, registers_t *regs)
{
	stgi();
	irq_enable();
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	asm volatile("nop");
	irq_disable();
	clgi();
	asm volatile("nop");
	asm volatile("nop");

#ifdef NOTDEF
	vmcb_control_t *control = svm_control(v);

	static int count = 0;

	if (count++ >= 1000) {
		kprintf("vm_exit_hardware_interrupt>Turning off\n");
		bit_clear(control->vmcb_intercept_controls1, VIC1_INTR);
		count = 0;
	}
#endif

	return true;
}

void
svm_handle_interrupt(vm_t *v, exc_frame_t *regs)
{
	assert(irq_is_disabled());
	clgi();

	un vector = regs->vector;

	if (interrupt(vector)) {
		return;
	}

	guest_set_interrupt_pending(v, vector);
}


static ret_t
svm_init_devices(void)
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

#ifdef NOTDEF
	ret = vtd_construct();
	if (ret < 0) {
		goto out;
	}
#endif


	notify_init();
#ifdef NOTDEF
	vtd_enable();

	protect_hypervisor_memory();
#endif

out:
	spinunlock(&lock, flags);
	return ret;
}

static inline void
svm_fin_devices(void)
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


static void
svm_load_guest_state(vm_t *v, const guest_state_t *gs)
{
	kprintf("svm_load_guest_state>UNIMPLEMENTED\n");
}


static bool
vm_exit_vmmcall(vm_t *v, registers_t *regs)
{
	vmcb_guest_state_t *guest = svm_guest_state(v);
	if( !ghv_handle_hypercall(regs, guest_usermode(guest))) {
		vm_entry_inject_exception(v, VEC_UD, 0);
		return true;
	}
	skip_emulated_instr(v);

	return true;
}

#define p(x)	kprintf("vmexit>" #x " 0x%lx\n", (un)x);

static void
vmexit_dump(vm_t *v, registers_t *regs)
{
	vmcb_guest_state_t *guest = svm_guest_state(v);
	vmcb_control_t *control = svm_control(v);
	s64 exitcode = control->vmcb_exitcode;
	bool entry_failure  = (exitcode == -1);
	kprintf("vmexit>basic_exit reason %lld%s\n", exitcode,
		entry_failure ? " entry failure" : "");

	u64 exitinfo1 = control->vmcb_exitinfo1;
	u64 exitinfo2 = control->vmcb_exitinfo2;
	u64 exitintinfo = control->vmcb_exitintinfo;
	u64 nrip = control->vmcb_nrip;
	u32 clean_bits = control->vmcb_clean_bits;
	u8 guest_instr_count = control->vmcb_guest_instr_count;

	u64 guest_rip = guest->vmcb_guest_rip;

	u8 v_tpr = control->vmcb_v_tpr;
	bool v_irq = control->vmcb_v_irq; 
	u8 v_intr_prio = control->vmcb_v_intr_prio; 
	bool v_ign_tpr = control->vmcb_v_ign_tpr; 
	bool v_intr_masking = control->vmcb_v_intr_masking; 
	u8 v_intr_vector = control->vmcb_v_intr_vector; 

	p(exitcode);
	p(exitinfo1);
	p(exitinfo2);
	p(exitintinfo);
	p(guest_rip);
	p(nrip);
	p(guest_instr_count);
	p(clean_bits);
	p(v_tpr);
	p(v_irq);
	p(v_intr_prio);
	p(v_ign_tpr);
	p(v_intr_masking);
	p(v_intr_vector);

#ifdef __x86_64__
#define rfmt ":0x%016lx"
#else
#define rfmt ":0x%08lx"
#endif
	kprintf("vmexit>"R"sp"rfmt"\n", regs->rsp);
	kprintf("vmexit>"R"ax"rfmt" "R"cx"rfmt" "R"dx"rfmt"\n",
		regs->rax, regs->rcx, regs->rdx);
	kprintf("vmexit>"R"bx"rfmt" "R"bp"rfmt" "R"si"rfmt"\n",
		regs->rbx, regs->rbp, regs->rsi);
#ifndef __x86_64__
	kprintf("vmexit>"R"di"rfmt"\n",
		regs->rdi);
#else
	kprintf("vmexit>rdi"rfmt" r8 "rfmt" r9 "rfmt"\n",
		regs->rdi, regs->r8, regs->r9);
	kprintf("vmexit>r10"rfmt" r11"rfmt" r12"rfmt"\n",
		regs->r10, regs->r11, regs->r12);
	kprintf("vmexit>r13"rfmt" r14"rfmt" r15"rfmt"\n",
		regs->r13, regs->r14, regs->r15);
#endif
	kprintf("nexits %lld\n", v->v_nexits);
	if (entry_failure) {
		check_vmcb(v);
	}
}

void
svm_copy_guest_state_to_host(vm_t *v)
{
	vmcb_guest_state_t *guest = svm_guest_state(v);

	/* CR3 needs to be set first, because GDT comes out of virtual space */
	set_CR3(guest->vmcb_guest_cr3);

	dtr_t dtr;
	dtr.dt_base = guest->vmcb_guest_gdtr.seg_base;
	dtr.dt_limit = guest->vmcb_guest_gdtr.seg_limit;
	set_GDTR(&dtr);

	dtr.dt_base = guest->vmcb_guest_idtr.seg_base;
	dtr.dt_limit = guest->vmcb_guest_idtr.seg_limit;
	set_IDTR(&dtr);

	set_DS(guest->vmcb_guest_ds.seg_sel);
	set_ES(guest->vmcb_guest_es.seg_sel);
	/*
	 * vmload restores FS, GS, TR, LDTR (including hidden state),
	 * KernelGSBase, STAR, LSTAR, CSTAR, SFMASK,
	 * SYSENTER_CS, SYSENTER_ESP, SYSENTER_EIP
	 */
	vmload(kvtophys(v->v_guest_vmcs));
}

static void
svm_set_host_state(vm_t *v, registers_t *regs)
{
	kprintf("svm_set_host_state>ENTRY\n");
	dtr_t *dtr;
	un n = cpuno();

	/* CR3 needs to be set first, because GDT comes out of virtual space */
	set_CR3(host_cr3);
	kprintf("svm_set_host_state>after cr3\n");

	dtr = &hyp_gdt[n];
	set_GDTR(dtr);
	kprintf("svm_set_host_state>after GDT\n");

	u16 sel = HYP_TSS;
	clear_tss_busy((seg_desc_t *)dtr->dt_base, sel);
	set_TR(sel);
	kprintf("svm_set_host_state>after TR\n");

	set_IDTR(&hyp_idtr);
	kprintf("svm_set_host_state>after IDT\n");

	set_DS(HYP_DS);
	set_ES(HYP_DS);
	set_FS(HYP_DS);
	set_GS(HYP_DS);
	kprintf("svm_set_host_state>after DS etc.\n");

	vmsave(kvtophys(v->v_host_vmcb));
	kprintf("svm_set_host_state>after vmsave()\n");

	un host_rip = (un)&vmrun;
	un host_rsp = (un)regs;

	vmload(kvtophys(v->v_guest_vmcs));

	push(HYP_DS);
	push(host_rsp);
	pushf();
	push(HYP_CS);
	push(host_rip);
	iret();
}

static void
dump_stack_mem(void *addr, size_t len)
{

	for (un i = 0; i < len; i += 4 * sizeof(u64)) {
		u64 *p = (u64 *)(addr + i);
		kprintf("%16p 0x%016llx 0x%016llx 0x%016llx 0x%016llx\n",
			p, p[0], p[1], p[2], p[3]);
	}
}

void
test_host_env(registers_t *regs)
{
	vm_t *v = &cpu_vm[cpuno()];

	kprintf("test_host_env>ENTRY\n");
	kprintf("test_host_env>RSP 0x%lx\n", get_RSP());
	kprintf("test_host_env>regs 0x%lx\n", (un)regs);
	un rsp = get_RSP();
	un stack_end = (un)(v->v_stack) + VSTACK_LEN;
	dump_stack_mem((void *)rsp, stack_end - rsp);
}


static void
svm_exit(vm_t *v, registers_t *regs)
{
	exit(-1);
}

typedef struct {
	un st_nexits;
	un st_total_time;
} vm_stats_t;

static vm_stats_t vmstats[LAST_EXITCODE + 1];

static void
dump_vmstats(void)
{
	for (un i = 0; i <= LAST_EXITCODE; i++) {
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
 *  Phys addr of next guest VMCB to run (must be non-zero)
 *  0 if return to VMX-root mode is needed (debug-only)
 */
sn
vmexit(registers_t *regs)
{
	irq_disable();

	u64 tsc_start = rdtsc();
	bool ignore_time = false;
	vm_t *v = &cpu_vm[cpuno()];

	vmsave(kvtophys(v->v_guest_vmcs));
	vmload(kvtophys(v->v_host_vmcb));

	vmcb_control_t *control = svm_control(v);
	vmcb_guest_state_t *guest = svm_guest_state(v);

	regs->rsp = guest->vmcb_guest_rsp;
	regs->rax = guest->vmcb_guest_rax;

	s64 exitcode = control->vmcb_exitcode;
	bool entry_failure = (exitcode == -1);
	bool handled = false;


	// vmexit_dump(v, regs);

	if (entry_failure) {
		goto exit_hypervisor;
	}

	v->v_guest_64 = bit_test(guest->vmcb_guest_efer, EFER_LMA);
	v->v_guest_pae = bit_test(guest->vmcb_guest_cr4, CR4_PAE);
	v->v_guest_cr3 = guest->vmcb_guest_cr3;


	if (exitcode <= LAST_EXITCODE) {
		atomic_inc(&(vmstats[exitcode].st_nexits));
	}

	if (v->v_trace_exits) {
		kprintf("vmexit>nexits %lld\n", v->v_nexits);
		kprintf("vmexit>reason %lld\n", exitcode);
		kprintf("vmexit>guest rip %llx\n", guest->vmcb_guest_rip);
	}

	switch (exitcode) {
		/* Unexpected exits: disabled by VM-exec controls, or SMM */
	case VMEXIT_CR0_READ:
	case VMEXIT_CR1_READ:
	case VMEXIT_CR2_READ:
	case VMEXIT_CR3_READ:
	case VMEXIT_CR4_READ:
	case VMEXIT_CR5_READ:
	case VMEXIT_CR6_READ:
	case VMEXIT_CR7_READ:
	case VMEXIT_CR8_READ:
	case VMEXIT_CR9_READ:
	case VMEXIT_CR10_READ:
	case VMEXIT_CR11_READ:
	case VMEXIT_CR12_READ:
	case VMEXIT_CR13_READ:
	case VMEXIT_CR14_READ:
	case VMEXIT_CR15_READ:
	case VMEXIT_CR0_WRITE:
	case VMEXIT_CR1_WRITE:
	case VMEXIT_CR2_WRITE:
	case VMEXIT_CR3_WRITE:
	case VMEXIT_CR4_WRITE:
	case VMEXIT_CR5_WRITE:
	case VMEXIT_CR6_WRITE:
	case VMEXIT_CR7_WRITE:
	case VMEXIT_CR8_WRITE:
	case VMEXIT_CR9_WRITE:
	case VMEXIT_CR10_WRITE:
	case VMEXIT_CR11_WRITE:
	case VMEXIT_CR12_WRITE:
	case VMEXIT_CR13_WRITE:
	case VMEXIT_CR14_WRITE:
	case VMEXIT_CR15_WRITE:
	case VMEXIT_DR0_READ:
	case VMEXIT_DR1_READ:
	case VMEXIT_DR2_READ:
	case VMEXIT_DR3_READ:
	case VMEXIT_DR4_READ:
	case VMEXIT_DR5_READ:
	case VMEXIT_DR6_READ:
	case VMEXIT_DR7_READ:
	case VMEXIT_DR8_READ:
	case VMEXIT_DR9_READ:
	case VMEXIT_DR10_READ:
	case VMEXIT_DR11_READ:
	case VMEXIT_DR12_READ:
	case VMEXIT_DR13_READ:
	case VMEXIT_DR14_READ:
	case VMEXIT_DR15_READ:
	case VMEXIT_DR0_WRITE:
	case VMEXIT_DR1_WRITE:
	case VMEXIT_DR2_WRITE:
	case VMEXIT_DR3_WRITE:
	case VMEXIT_DR4_WRITE:
	case VMEXIT_DR5_WRITE:
	case VMEXIT_DR6_WRITE:
	case VMEXIT_DR7_WRITE:
	case VMEXIT_DR8_WRITE:
	case VMEXIT_DR9_WRITE:
	case VMEXIT_DR10_WRITE:
	case VMEXIT_DR11_WRITE:
	case VMEXIT_DR12_WRITE:
	case VMEXIT_DR13_WRITE:
	case VMEXIT_DR14_WRITE:
	case VMEXIT_DR15_WRITE:
	case VMEXIT_EXCP0:
	case VMEXIT_EXCP1:
	case VMEXIT_EXCP2:
	case VMEXIT_EXCP3:
	case VMEXIT_EXCP4:
	case VMEXIT_EXCP5:
	case VMEXIT_EXCP6:
	case VMEXIT_EXCP7:
	case VMEXIT_EXCP8:
	case VMEXIT_EXCP9:
	case VMEXIT_EXCP10:
	case VMEXIT_EXCP11:
	case VMEXIT_EXCP12:
	case VMEXIT_EXCP13:
	case VMEXIT_EXCP14:
	case VMEXIT_EXCP15:
	case VMEXIT_EXCP16:
	case VMEXIT_EXCP17:
	case VMEXIT_EXCP18:
	case VMEXIT_EXCP19:
	case VMEXIT_EXCP20:
	case VMEXIT_EXCP21:
	case VMEXIT_EXCP22:
	case VMEXIT_EXCP23:
	case VMEXIT_EXCP24:
	case VMEXIT_EXCP25:
	case VMEXIT_EXCP26:
	case VMEXIT_EXCP27:
	case VMEXIT_EXCP28:
	case VMEXIT_EXCP29:
	case VMEXIT_EXCP30:
	case VMEXIT_EXCP31:
	case VMEXIT_NMI:
	case VMEXIT_SMI:
	case VMEXIT_INIT:
	case VMEXIT_CR0_SEL_WRITE:
	case VMEXIT_IDTR_READ:
	case VMEXIT_GDTR_READ:
	case VMEXIT_LDTR_READ:
	case VMEXIT_TR_READ:
	case VMEXIT_IDTR_WRITE:
	case VMEXIT_GDTR_WRITE:
	case VMEXIT_LDTR_WRITE:
	case VMEXIT_TR_WRITE:
	case VMEXIT_RDTSC:
	case VMEXIT_RDPMC:
	case VMEXIT_PUSHF:
	case VMEXIT_POPF:
	case VMEXIT_CPUID:
	case VMEXIT_RSM:
	case VMEXIT_SWINT:
	case VMEXIT_INVD:
	case VMEXIT_PAUSE:
	case VMEXIT_HLT:
	case VMEXIT_INVLPG:
	case VMEXIT_INVLPGA:
	case VMEXIT_IOIO:
	case VMEXIT_MSR:
	case VMEXIT_TASK_SWITCH:
	case VMEXIT_FERR_FREEZE:
	case VMEXIT_SHUTDOWN:
	case VMEXIT_VMRUN:
	case VMEXIT_VMLOAD:
	case VMEXIT_VMSAVE:
	case VMEXIT_STGI:
	case VMEXIT_CLGI:
	case VMEXIT_SKINIT:
	case VMEXIT_RDTSCP:
	case VMEXIT_ICEBP:
	case VMEXIT_WBINVD:
	case VMEXIT_MONITOR:
	case VMEXIT_MWAIT:
	case VMEXIT_MWAIT_CONDITIONAL:
	case VMEXIT_NPF:
		kprintf("vmexit>unexpected exit %lld\n", exitcode);
		break;

	case VMEXIT_INTR:
		handled = vm_exit_hardware_interrupt(v, regs);
		break;

	case VMEXIT_IRET:
		handled = vm_exit_iret(v, regs);
		break;

	case VMEXIT_VINTR:
		handled = vm_exit_vintr(v, regs);
		break;

	case VMEXIT_VMMCALL:
		if (regs->rax == 902) {
			ignore_time = true;
		}
		handled = vm_exit_vmmcall(v, regs);
		break;
	default:
		kprintf("vmexit>unknown exit\n");
		break;
	}


	if (handled) {
		bool success = true;
		/* Resume guest to deliver pending interrupt */
		bool interrupts_pending = deliver_pending_interrupt();

		if (!ignore_time) {
			u64 time = rdtsc() - tsc_start;
			atomic_add(&(vmstats[exitcode].st_total_time),
					   time);		
		}
		(void)interrupts_pending;
		if (success) {
			guest->vmcb_guest_rsp = regs->rsp;
			guest->vmcb_guest_rax = regs->rax;
			check_vmcb(v);
			vmload(kvtophys(v->v_guest_vmcs));
			return kvtophys(v->v_guest_vmcs);
		}
	}


exit_hypervisor:
	kprintf("vmexit>exit_hypervisor\n");
	dump_vmstats();
	if (!handled) {
		vmexit_dump(v, regs);
		bp->ret[v->v_cpuno] = -1;
	}

	svm_exit(v, regs);

	irq_disable();
	stgi();

	/* It's not safe to call kprintf() from here. */

	__svm_off(v);

	return 0;
}

void
svm_guest_init(void)
{
	assert(irq_is_disabled());

	un n = cpuno();
	vm_t *v = &cpu_vm[n];
	bp->ret[n] = 0;

	vmcb_control_t *control = svm_control(v);

	v->v_nexits = 0;

	/* Guest state */


    	const guest_state_t *const gs = &guest_state[n];
	svm_load_guest_state(v, gs);


	u32 vic2 = (bit(VIC2_VMRUN) |
		    bit(VIC2_VMMCALL) |
		    bit(VIC2_VMLOAD) |
		    bit(VIC2_VMSAVE) |
		    bit(VIC2_STGI) |
		    bit(VIC2_CLGI));
	control->vmcb_intercept_controls2 = vic2;

	control->vmcb_guest_asid = GUEST_ID;

	if (!(bp->bp_flags & BPF_NO_EXTINT)) {
		svm_enable_external_interrupt_exiting(v);
	}


#ifdef NOTDEF
	dump_vmcb();
#endif
	check_vmcb(v);

	clgi();
	kprintf("svm_guest_init>host stack 0x%lx\n",
		(un)(v->v_stack) + VSTACK_LEN);
	kprintf("svm_guest_init>before vmrun\n");
	kprintf("svm_guest_init>RSP 0x%lx\n", get_RSP());

	/* Initialize the host stack with a register frame */
	registers_t *regs = (registers_t *)((un)(v->v_stack) + VSTACK_LEN) - 1;
	kprintf("svm_guest_init>host stack register frame %p\n", regs);
	kprintf("svm_guest_init>VMCB 0x%lx\n", kvtophys(v->v_guest_vmcs));
	asm volatile("mov     %%rbx, 3*8(%0)\n\t"
		     "mov     %%rsp, 4*8(%0)\n\t"
		     "mov     %%rbp, 5*8(%0)\n\t"
		     "mov     %%r12, 12*8(%0)\n\t"
		     "mov     %%r13, 13*8(%0)\n\t"
		     "mov     %%r14, 14*8(%0)\n\t"
		     "mov     %%r15, 15*8(%0)" : :
		     "r" (regs) :
		     "memory");
	regs->rax = kvtophys(v->v_guest_vmcs);
	svm_set_host_state(v, regs);

	asm volatile(".global svm_initial_guest_entry\n"
		     "svm_initial_guest_entry:");
	kprintf("svm_guest_init>svm_initial_guest_entry\n");
	// dump_x86();
	return;
	/*
	 * Now in guest mode.  Don't put anything here.
	 * Risk of deadlock (spinning with interrupts disabled).
	 */
}
