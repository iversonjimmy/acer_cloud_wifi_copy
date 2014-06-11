/*
 * Common to both VMX and SVM
 */
#include "hyp.h"
#include "vm.h"
#include "ept.h"

void *hyp_istacks[MAX_CPUS];
un active_cpumask = 0;
vm_t cpu_vm[MAX_CPUS];

epte_t *ept_root;

bootparam_t *bp;

addr_t dma_test_page = 0;

extern ret_t vmx_alloc_all(void);
extern ret_t svm_alloc_all(void);

ret_t
vm_alloc_all(bootparam_t *boot_parameters)
{
	bp = boot_parameters;

	kprintf("vm_alloc_all>first kprintf()\n");

	init_cpuinfo();

#ifdef NOTDEF
	dump_x86();
#endif

#ifndef __x86_64__
	un cr4 = get_CR4();
	if (bit_test(cr4, CR4_PAE)) {
#ifdef HYP_PAE
		kprintf("vm_alloc_all>32bit PAE mode detected\n");
#else
		kprintf("vm_alloc_all>PAE mode detected, not supported\n");
		return -EINVAL;
#endif
	} else {
		kprintf("vm_alloc_all>32bit mode detected (no PAE)\n");
	}
#endif

	if (cpu_is_Intel()) {
		return vmx_alloc_all();
	} else if (cpu_is_AMD()) {
		return svm_alloc_all();
	}

	kprintf("vm_alloc_all>unsupported CPU type\n");
	return -ENXIO;
}
