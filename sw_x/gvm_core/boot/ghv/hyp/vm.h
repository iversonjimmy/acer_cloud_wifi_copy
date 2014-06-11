#ifndef __VM_H__
#define __VM_H__

/* Common to both VMX and SVM */

#include "bitmap.h"
#include "list.h"
#include "guest.h"
#include "setjmp.h"

#define VSTACK_LEN			0x8000

#define NINTVECS	256

typedef struct {
	bitmap_t	va_apic[bitmap_word_size(NINTVECS)]; /* APIC sourced */
	bitmap_t	va_irr[bitmap_word_size(NINTVECS)];
	bitmap_t	va_isr[bitmap_word_size(NINTVECS)];
	bitmap_t	va_eoi[bitmap_word_size(NINTVECS)];
	u32		va_icr1;
	u32		va_task_pri;
} vapic_t;

typedef struct {
	un	v_cpuno;
	void	*v_vmxon_region;
	void	*v_host_vmcb;
	void	*v_guest_vmcs;
	void	*v_stack;
	void	*v_msr_bitmaps;
	void	*v_virtual_apic;
	u64	v_nexits;
	un	v_mtf_exits;
	addr_t	v_modified_rip;
	un	v_guest_cr3;
	un	v_cr3_read_shadow;
#ifdef __x86_64__
	u64	v_ia32_efer_shadow;
#endif
	bool	v_trace_exits;
	bool	v_guest_paging;
	bool	v_in_vmx;
	bool	v_in_smm;
	bool	v_in_vmx_root;
	bool	v_cr3_reload_needed;
	bool	v_guest_64;
	bool	v_guest_pae;
	list_t	v_notes_list;
	lock_t	v_notes_lock;
	vapic_t	v_vapic;
	u64	v_cache_timing_thres;
} vm_t;

extern vm_t cpu_vm[];
extern un active_cpumask;
extern void *hyp_istacks[];
extern addr_t dma_test_page;

#endif
