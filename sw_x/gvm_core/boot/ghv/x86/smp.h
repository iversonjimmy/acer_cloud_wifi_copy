#ifndef __SMP_H__
#define __SMP_H__

extern addr_t trampoline_init(dtr_t *gdt, void (*entry)(int),
			      void **ptr_saved_trampoline_area);
extern void trampoline_deinit(addr_t trampoline, void *saved_trampoline);
extern void smp_init(void);
extern volatile u32 smp_call_in_mask;

#endif
