#include "compiler.h"
#include "km_types.h"
#include "bits.h"
#include "x86.h"
#include "kprintf.h"
#include "assert.h"
#include "pt.h"
#include "dt.h"
#include "bootparam.h"
#include "printf.h"
#include "string.h"
#include "vmx.h"
#include "bitmap.h"
#include "apic.h"
#include "smp.h"
#include "malloc.h"
#include "setup/gvmdefs.h"

extern void vmx_on(void);
extern void vmx_off(void);
extern void vmx_guest_init(void);

void exit(int r) __attribute__((noreturn));

static void
make_ident_pd_32(u32 *pagetable)
{
    int i;

    for (i = 0; i < 1024; i++) {
        pagetable[i] = (i << VM_4M_PAGE_SHIFT) | PDE_FLAGS_LARGE_PAGE;
    }
}

static void
trampoline_copy_from_template(u8 *tramp)
{
    extern u8 trampoline_template_start[];
    extern u8 trampoline_template_end[];
    size_t size = trampoline_template_end - trampoline_template_start;

    assert(size <= VM_PAGE_SIZE);
    memcpy(tramp, trampoline_template_start, size);
}

static void
trampoline_set_gdt(u8 *tramp, dtr_t *gdt)
{
    extern u32 trampoline_offset_gdt;
    memcpy(tramp + trampoline_offset_gdt, (void *)gdt->dt_base, 0x40);
}

static void
trampoline_set_cr3(u8 *tramp, un cr3_32, un cr3_64)
{
    extern u32 trampoline_offset_cr3_32;
    extern u32 trampoline_offset_cr3_64;

    *(u32 *)(tramp + trampoline_offset_cr3_32) = cr3_32;
    *(u32 *)(tramp + trampoline_offset_cr3_64) = cr3_64;
}

static void
trampoline_set_rsp(u8 *tramp)
{
    extern u32 trampoline_offset_per_cpu_rsp;
    addr_t *per_cpu_rsp = (addr_t *)(tramp + trampoline_offset_per_cpu_rsp);
    int i;

    for (i = 0; i < NR_CPUS; i++) {
	per_cpu_rsp[i] = (addr_t)cpu_vm[i].v_stack + VSTACK_LEN;	       
    }
}

static void
trampoline_set_ia32_efer(u8 *tramp)
{
    extern u32 trampoline_offset_ia32_efer;
    u32 efer = 0;

    if(cpu_xfeature_supported(CPU_XFEATURE_NXE)) {
        efer = bit(EFER_NXE);
    }
    *(u32 *)(tramp + trampoline_offset_ia32_efer) = efer;
}

static void
trampoline_set_entry(u8 *tramp, void (*entry)(int))
{
    extern u32 trampoline_offset_entry;
    *(void **)(tramp + trampoline_offset_entry) = entry;
}

static void
trampoline_fix_offsets(u8 *tramp, u32 base)
{
    extern u32 trampoline_relocation_offsets[];
    u32 *off = trampoline_relocation_offsets;

    while (*off) {
	*(u32 *)(tramp + *off++) += base;
    }
}

#define TRAMP_SIZE (VM_PAGE_SIZE * 3)

addr_t
trampoline_init(dtr_t *gdt, void (*entry)(int), void **ptr_saved_trampoline_area)
{
    const addr_t addr = LINUX_KERN16_START + LINUX_HEAP_START;
    void *tramp = (void *)addr; /* XXX, map in */
    void *pd   = (void *)(addr + VM_PAGE_SIZE);
    void *pml4 = (void *)(addr + VM_PAGE_SIZE*2);

    *ptr_saved_trampoline_area = malloc(TRAMP_SIZE);
    memcpy(*ptr_saved_trampoline_area, tramp, TRAMP_SIZE);
   
    make_ident_pd_32(pd);
    memcpy(pml4, (void *)TRUNC_PAGE(host_cr3), VM_PAGE_SIZE);

    trampoline_copy_from_template(tramp);
    trampoline_set_gdt(tramp, gdt);
    trampoline_set_cr3(tramp, (un)pd, (un)pml4);
    trampoline_set_rsp(tramp);
    trampoline_set_ia32_efer(tramp);
    trampoline_set_entry(tramp, entry);
    trampoline_fix_offsets(tramp, addr);

    wbinvd();

    return addr;
}

void
trampoline_deinit(addr_t tramp, void *saved_trampoline_area)
{
    memcpy((void *)tramp, saved_trampoline_area, TRAMP_SIZE);
    free(saved_trampoline_area);
}

static void __attribute__((noreturn, noinline))
smp_vmx_init(int n)
{
    WARN("Processor %d online!\n", n);
    vmx_on();
    if(bp->ret[n]) {
        WARN("vmxon failed with ret %d!\n", bp->ret[n]);
        goto out;
    }
    
    vmx_guest_init();
    WARN("vmx_guest_init failed!\n");
    
 out:
    exit(-1);

}

volatile u32 smp_call_in_mask = 1;
volatile u32 smp_call_out_mask = 0;

void
smp_entry(int n)
{
    asm volatile("lock orl %1, %0\n"  :
		 "=m"(smp_call_in_mask) :
		 "r"(1 << n) : "cc");

    while (!(smp_call_out_mask & (1 << n))) {
	continue;
    }
		 
    set_CR3(host_cr3);
    load_hyp_segments(&hyp_gdt[n]);
    set_IDTR(&hyp_idtr);
  
    smp_vmx_init(n);
}

void
smp_init(void)
{
    addr_t tramp;
    void *saved_tramp;

    tramp = trampoline_init(&hyp_gdt[0], smp_entry, &saved_tramp);

    smp_call_in_mask = 1;

    apic_startup_all_cpus(tramp >> 12);

    smp_call_out_mask = ~0;

    mdelay(100);

    /* XXX TODO: make sure all APs call in */
    printf("%s>call_in_mask: %x\n", __func__, smp_call_in_mask);

    trampoline_deinit(tramp, saved_tramp);
}

