#ifndef _GHV_GUEST_H_
#define _GHV_GUEST_H_

#include "x86.h"

typedef struct {
    u16 sel;
    u16 attr;
    u32 limit;
    un  base;
} guest_seg_t;

typedef struct {
    u16 limit;
    un  base;
} guest_dt_t;

typedef struct {
    un cr0;
    un cr3;
    un cr4;
    u64 ia32_efer;
    guest_seg_t cs;
    guest_seg_t ss;
    guest_seg_t ds;
    guest_seg_t es;
    guest_seg_t fs;
    guest_seg_t gs;
    guest_seg_t ldt;
    guest_seg_t tss;
    guest_dt_t idt;
    guest_dt_t gdt;
    registers_t regs;
    bool long_mode;
    bool active;
    bool drop_int10;
} guest_state_t;

extern guest_state_t guest_state[NR_CPUS];
extern void *ghv_io_bitmaps[2];

extern void io_bitmap_clear(u16 port);
extern void io_bitmap_set(u16 port);
extern void ghv_guest_init_state(void);
extern void ghv_guest_update_memsize(paddr_t size);
extern void ghv_guest_resume_in_realmode(guest_state_t *g);
extern void ghv_guest_boot_mbr(void);
extern void ghv_guest_boot_linux(bool drop_int10);
extern void ghv_guest_acpi_resume(u32 vector);
extern void ghv_guest_prepare_longjmp(registers_t *regs);
extern bool ghv_handle_hypercall(registers_t *regs, bool usermode);
extern bool ghv_handle_io_exit(u16 port, u8 iotype, un *val);

#endif
