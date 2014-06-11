#include "km_types.h"
#include "bits.h"
#include "x86.h"
#include "assert.h"
#include "string.h"
#include "printf.h"
#include "bitmap.h"
#include "pagepool.h"
#include "malloc.h"
#include "valloc.h"
#include "x86emu/x86emu.h"
#include "realmode.h"

static u8 mem_read_byte(void *e, u32 addr)
{
    realmode_t *rm = e;

    if (!rm->rm_a20enable) {
	addr &= 0xfffff;
    }

    u32 page = addr >> VM_PAGE_SHIFT;
    u32 off  = addr & VM_PAGE_OFFSET;

    return *(u8 *)(rm->rm_pages[page] + off);
}

static u16 mem_read_word(void *e, u32 addr)
{
    realmode_t *rm = e;

    if (!rm->rm_a20enable) {
	addr &= 0xfffff;
    }

    if (addr % sizeof(u16)) {
	u16 hi, lo;

	lo = mem_read_byte(e, addr);
	hi = mem_read_byte(e, addr + 1);
	return (hi << 8) | lo;
    } else {
	u32 page = addr >> VM_PAGE_SHIFT;
	u32 off  = addr & VM_PAGE_OFFSET;

	return *(u16 *)(rm->rm_pages[page] + off);
    }
}

static u32 mem_read_long(void *e, u32 addr)
{
    realmode_t *rm = e;

    if (!rm->rm_a20enable) {
	addr &= 0xfffff;
    }

    if (addr % sizeof(u32)) {
	u32 hi, lo;

	lo = mem_read_word(e, addr);
	hi = mem_read_word(e, addr+2);
	return (hi << 16) | lo;
    } else {
	u32 page = addr >> VM_PAGE_SHIFT;
	u32 off  = addr & VM_PAGE_OFFSET;

	return *(u32 *)(rm->rm_pages[page] + off);
    }
}

static void mem_write_byte(void *e, u32 addr, u8 val)
{
    realmode_t *rm = e;

    if (!rm->rm_a20enable) {
	addr &= 0xfffff;
    }

    u32 page = addr >> VM_PAGE_SHIFT;
    u32 off  = addr & VM_PAGE_OFFSET;

    *(u8 *)(rm->rm_pages[page] + off) = val;
}

static void mem_write_word(void *e, u32 addr, u16 val)
{
    realmode_t *rm = e;

    if (!rm->rm_a20enable) {
	addr &= 0xfffff;
    }

    if (addr % sizeof(u16)) {
	mem_write_byte(e, addr,   val);
	mem_write_byte(e, addr+1, val >> 8);
    } else {
	u32 page = addr >> VM_PAGE_SHIFT;
	u32 off  = addr & VM_PAGE_OFFSET;
	
	*(u16 *)(rm->rm_pages[page] + off) = val;
    }
}

static void mem_write_long(void *e, u32 addr, u32 val)
{
    realmode_t *rm = e;

    if (!rm->rm_a20enable) {
	addr &= 0xfffff;
    }

    if (addr % sizeof(u32)) {
	mem_write_word(e, addr,   val);
	mem_write_word(e, addr+2, val >> 16);
    } else {
	u32 page = addr >> VM_PAGE_SHIFT;
	u32 off  = addr & VM_PAGE_OFFSET;

	*(u32 *)(rm->rm_pages[page] + off) = val;
    }
}

static u8 pio_inb(void *e, u16 port)
{
    return inb(port);
}

static u16 pio_inw(void *e, u16 port)
{
    return inw(port);
}

static u32 pio_inl(void *e, u16 port)
{
    return inl(port);
}

static void pio_outb(void *e, u16 port, u8 val)
{
    return outb(port, val);
}

static void pio_outw(void *e, u16 port, u16 val)
{
    return outw(port, val);
}

static void pio_outl(void *e, u16 port, u32 val)
{
    return outl(port, val);
}

void realmode_set_default_callbacks(realmode_t *rm)
{
    X86EMU_sysEnv *const env = &rm->rm_x86env;
    memzero(&rm->rm_x86env, sizeof rm->rm_x86env);
    
    env->memfuncs.rdb = mem_read_byte;
    env->memfuncs.rdw = mem_read_word;
    env->memfuncs.rdl = mem_read_long;
    env->memfuncs.wrb = mem_write_byte;
    env->memfuncs.wrw = mem_write_word;
    env->memfuncs.wrl = mem_write_long;

    env->piofuncs.inb  = pio_inb;
    env->piofuncs.inw  = pio_inw;
    env->piofuncs.inl  = pio_inl;
    env->piofuncs.outb = pio_outb;
    env->piofuncs.outw = pio_outw;
    env->piofuncs.outl = pio_outl;

    env->illfuncs.ill_op  = NULL;
    env->illfuncs.ill_op2 = NULL;
    env->intrfunc = NULL;
}

void realmode_exec(realmode_t *rm)
{
    X86EMU_exec(&rm->rm_x86env);
}

void realmode_int(realmode_t *rm, u8 intnum)
{
    u8 saved_instr[3];
    X86EMU_sysEnv *const env = &rm->rm_x86env;
    u32 eip = ((u32)env->x86.R_CS << 4) + env->x86.R_IP;
    u16 saved_cs = env->x86.R_CS;
    u16 saved_ip = env->x86.R_IP;

    saved_instr[0] = env->memfuncs.rdb(rm, eip+0);
    saved_instr[1] = env->memfuncs.rdb(rm, eip+1);
    saved_instr[2] = env->memfuncs.rdb(rm, eip+2);

    env->memfuncs.wrb(rm, eip+0, 0xcd);
    env->memfuncs.wrb(rm, eip+1, intnum);
    env->memfuncs.wrb(rm, eip+2, 0xf4);

    X86EMU_exec(&rm->rm_x86env);

    env->memfuncs.wrb(rm, eip+0, saved_instr[0]);
    env->memfuncs.wrb(rm, eip+1, saved_instr[1]);
    env->memfuncs.wrb(rm, eip+2, saved_instr[2]);

    env->x86.R_CS = saved_cs;
    env->x86.R_IP = saved_ip;
}

static void __release_mem_shadow(realmode_t *rm, int maxpage)
{
    int i;

    for (i = maxpage - 1; i >= 0; i--) {
	if (bitmap_test(rm->rm_shadow_bitmap, i)) {
	    page_free(rm->rm_pages[i]);
	} else {
	    unmap_page(rm->rm_pages[i], UMPF_LOCAL_FLUSH);
	}
    }
}

int realmode_create_mem_shadow(realmode_t *rm)
{
    int i;
    for (i = 0; i < REALMODE_PAGES; i++) {
	addr_t bios_page = NULL;

	bios_page = map_page(i << VM_PAGE_SHIFT, MPF_IO);
	if (bios_page == NULL) {
	    __release_mem_shadow(rm, i);
	    return -1;
	}

	if (bitmap_test(rm->rm_shadow_bitmap, i)) {
	    rm->rm_pages[i] = page_alloc();
	    memcpy((void *)rm->rm_pages[i], (void *)bios_page, VM_PAGE_SIZE);
	    unmap_page(bios_page, UMPF_LOCAL_FLUSH);
	} else {
	    rm->rm_pages[i] = bios_page;
	}
    }

    return 0;
}

void realmode_release_mem_shadow(realmode_t *rm)
{
    __release_mem_shadow(rm, REALMODE_PAGES);
}
