#include "km_types.h"
#include "bits.h"
#include "x86.h"
#include "string.h"
#include "printf.h"
#include "assert.h"
#include "dt.h"
#include "prot.h"
#include "vm.h"
#include "ept.h"
#include "bitmap.h"
#include "pagepool.h"
#include "malloc.h"
#include "linuxbootdefs.h"
#include "x86emu/x86emu.h"
#include "x86emu/decode.h"
#include "x86emu/debug.h"
#include "realmode.h"
#include "e820.h"
#include "ghv_guest.h"
#include "gpt.h"
#include "fastswitch.h"
#include "acpi-api.h"
#include "setup/gvmdefs.h"
#include "lib/console.h"
#include "apic.h"
#include "build_features.h"
#include "ghvlog.h"
#include "ghv_load.h"
#include "gvm.h"

guest_state_t guest_state[NR_CPUS] = { };
void *ghv_io_bitmaps[2];

static E820Entry guest_e820map[E820MAX];
static u8 guest_e820nr;
static paddr_t guest_max_phys_addr;

realmode_t realmode_guest;
static lock_t realmode_guest_lock;

#define IO_BITMAPS_LEN			(2*VM_PAGE_SIZE)

#define CS_ATTR 0xC09B
#define CS64_ATTR 0x209B
#define DS_ATTR 0xC0F3
#define SS_ATTR 0xC093
#define LDT_ATTR 0x0082
#define TSS_ATTR 0x008B

void io_bitmap_clear(u16 port)
{
    int index = port >> 15;

    bitmap_clear(ghv_io_bitmaps[index], port & 0x7FFF);
}

void io_bitmap_set(u16 port)
{
    int index = port >> 15;

    bitmap_set(ghv_io_bitmaps[index], port & 0x7FFF);
}

static void init_realmode_guest(realmode_t *rm)
{
    int rv;

    realmode_set_default_callbacks(rm);

    bitmap_init(rm->rm_shadow_bitmap, REALMODE_PAGES, BITMAP_ALL_ZEROES);

    rv = realmode_create_mem_shadow(rm);
    if (rv < 0) {
	WARN("Panic: cannot create memory shadow for real-mode emulation.\n");
	while (1);
    } 
}

void init_guest_state(guest_state_t *g)
{
    g->cr0 = bit(CR0_PE);
    g->cr3 = 0;
    g->cr4 = 0;
    g->ia32_efer = 0;
    g->regs.guest_rflags = 2;
    g->cs.sel   = 0;
    g->cs.attr  = CS_ATTR;
    g->cs.base  = 0;
    g->cs.limit = ~0;
    g->ss.sel   = 0;
    g->ss.attr  = SS_ATTR;
    g->ss.base  = 0;
    g->ss.limit = ~0;
    g->ds.sel   = 0;
    g->ds.attr  = DS_ATTR;
    g->ds.base  = 0;
    g->ds.limit = ~0;
    g->gs = g->fs = g->ds;
    g->ldt.sel   = 0;
    g->ldt.attr  = 0;
    g->ldt.base  = 0;
    g->ldt.limit = 0;
    g->tss.sel   = 0;
    g->tss.attr  = TSS_ATTR;
    g->tss.base  = 0;
    g->tss.limit = 0;
    g->idt.base  = 0;
    g->idt.limit = 0;
    g->gdt.base  = 0;
    g->gdt.limit = 0;
    g->long_mode = 0;
    g->drop_int10 = 0;
}

void ghv_guest_init_state(void)
{
    int i;

    ghv_io_bitmaps[0] = (void *)page_alloc();
    ghv_io_bitmaps[1] = (void *)page_alloc();

    for (i = 0; i < NR_CPUS; i++) {
	init_guest_state(&guest_state[i]);
    }

    lock_init(&realmode_guest_lock);
    init_realmode_guest(&realmode_guest);
}

void ghv_guest_prepare_longjmp(registers_t *regs)
{
    dtr_t gdtr, idtr;
    u16 ldtr, tr;
    guest_state_t *const g = &guest_state[0];

    memcpy(&g->regs, regs, sizeof g->regs);

    get_GDTR(&gdtr);
    get_IDTR(&idtr);
    ldtr = get_LDT_sel();
    tr = get_TR();

    g->cr0 = get_CR0();
    g->cr3 = get_CR3();
    g->cr4 = get_CR4();
    g->ia32_efer = get_MSR(MSR_IA32_EFER);

    g->gdt.base  = gdtr.dt_base;
    g->gdt.limit = gdtr.dt_limit;

    g->idt.base  = idtr.dt_base;
    g->idt.limit = idtr.dt_limit;

    g->cs.sel   = regs->guest_cs;
    g->cs.attr  = CS64_ATTR;
    g->cs.base  = 0;
    g->cs.limit = 0;

    g->ds.sel = get_DS();
    g->es.sel = get_ES();
    g->fs.sel = get_FS();
    g->gs.sel = get_GS();

    g->fs.base = get_MSR(MSR_IA32_FS_BASE);
    g->gs.base = get_MSR(MSR_IA32_GS_BASE);

    g->ldt.sel   = ldtr;
    g->ldt.attr  = get_seg_attr(ldtr,  gdtr.dt_base, 0, true);
    g->ldt.base  = get_seg_base(ldtr,  gdtr.dt_base, 0, true);
    g->ldt.limit = get_seg_limit(ldtr, gdtr.dt_base, 0, true);

    g->tss.sel   = tr;
    g->tss.attr  = get_seg_attr(tr,  gdtr.dt_base, 0, true);
    g->tss.base  = get_seg_base(tr,  gdtr.dt_base, 0, true);
    g->tss.limit = get_seg_limit(tr, gdtr.dt_base, 0, true);

    g->regs.rax = 1;

    g->active = true;
    g->long_mode = true;
}

static void
protect_mem_e820(const E820Entry *e, un perm, un type)
{
    ret_t ret;

    ret = protect_memory_phys_addr(e->addr, e->size, perm, type);
    if(ret) {
        kprintf("%s: failed %lx\n", __FUNCTION__, ret);
    }
}

static bool
fix_e820_entry(E820Entry *e)
{
    u64 old_size = e->size;

    // acpi regions should not be written by guest;
    // keep all other non-RAM regions intact;

    if(e->type == E820_RESERVED) {
	protect_mem_e820(e, EPT_PERM_ALL, VMPF_SOFT);
	return false;
    } else if(e->type == E820_ACPI) {
	protect_mem_e820(e, EPT_PERM_R, VMPF_SOFT);
	return false;
    } else if(e->type == E820_NVS) {
	protect_mem_e820(e, EPT_PERM_RW, VMPF_SOFT);
	return false;
    } else if(e->type != E820_RAM) {
	return false;
    } else if (e->addr >= guest_max_phys_addr) {
	e->type = E820_RESERVED;
	return false;
    } else if (e->addr + e->size >= guest_max_phys_addr) {
	E820Entry *n = e + 1;

	/* Split into two entries */
	e->size = guest_max_phys_addr - e->addr;
	protect_mem_e820(e, EPT_PERM_RW, VMPF_SOFT|VMPF_GUEST);

	n->addr = e->addr + e->size;
	n->size = old_size - e->size;
	n->type = E820_RESERVED;
	return true;
    } else {
	u64 old_end = e->addr + old_size;

        e->addr = TRUNC_PAGE(e->addr + VM_PAGE_SIZE - 1);
        e->size = TRUNC_PAGE(old_end) - e->addr;
	protect_mem_e820(e, EPT_PERM_RW, VMPF_SOFT|VMPF_GUEST);
	return false;
    }
}

void ghv_guest_update_memsize(paddr_t size)
{
    E820Entry *e = guest_e820map;
    int i;

    guest_max_phys_addr = size;

    for (i = 0; i < e820_nr(); i++, e++) {
	e820_get(i, e);
	if (fix_e820_entry(e)) {
	    e++;
	}
    }
    ret_t ret = protect_memory_phys_addr(get_apic_base_phys(), VM_PAGE_SIZE,
					 EPT_PERM_RW, VMPF_SOFT);
    if(ret) {
        kprintf("%s: failed %lx\n", __FUNCTION__, ret);
    }

    guest_e820nr = e - guest_e820map;
}

static inline void 
transfer_realmode_seg_state(guest_seg_t *seg, u16 sel, u16 attr)
{
    seg->sel   = 0;
    seg->attr  = attr & 0x9F;
    seg->limit = 0xffff;
    seg->base  = ((u32)sel) << 4;
}

static void transfer_state_from_realmode(guest_state_t *g, const realmode_t *rm)
{
#define r (&rm->rm_x86env.x86)

    g->regs.guest_rsp    = r->R_SP;
    g->regs.guest_rip    = r->R_IP;
    g->regs.guest_rflags = r->R_EFLG | 2;

    g->regs.rax = r->R_EAX;
    g->regs.rcx = r->R_ECX;
    g->regs.rdx = r->R_EDX;
    g->regs.rbx = r->R_EBX;
    g->regs.rsp = r->R_ESP;
    g->regs.rbp = r->R_EBP;
    g->regs.rsi = r->R_ESI;
    g->regs.rdi = r->R_EDI;

    transfer_realmode_seg_state(&g->cs, r->R_CS, CS_ATTR);
    transfer_realmode_seg_state(&g->ss, r->R_SS, DS_ATTR);
    transfer_realmode_seg_state(&g->ds, r->R_DS, DS_ATTR);
    transfer_realmode_seg_state(&g->es, r->R_ES, DS_ATTR);
    transfer_realmode_seg_state(&g->fs, r->R_FS, DS_ATTR);
    transfer_realmode_seg_state(&g->gs, r->R_GS, DS_ATTR);

    g->drop_int10 = rm->rm_drop_int10;
#undef r
}

static inline u16
transfer_protmode_seg_state(const guest_seg_t *seg)
{
    if ((seg->base & 0xf)) {
	WARN("Segment base is not 16-byte aligned... This is not implemented.\n");
    }
    return seg->base >> 4;
}

static void transfer_state_to_realmode(const guest_state_t *g, realmode_t *rm)
{
#define r (&rm->rm_x86env.x86)

    r->R_SP   = g->regs.guest_rsp;
    r->R_IP   = g->regs.guest_rip;
    r->R_EFLG = g->regs.guest_rflags;

    r->R_EAX = g->regs.rax;
    r->R_ECX = g->regs.rcx;
    r->R_EDX = g->regs.rdx;
    r->R_EBX = g->regs.rbx;
    // skip ESP
    r->R_EBP = g->regs.rbp;
    r->R_ESI = g->regs.rsi;
    r->R_EDI = g->regs.rdi;

    r->R_CS = transfer_protmode_seg_state(&g->cs);
    r->R_SS = transfer_protmode_seg_state(&g->ss);
    r->R_DS = transfer_protmode_seg_state(&g->ds);
    r->R_ES = transfer_protmode_seg_state(&g->es);
    r->R_FS = transfer_protmode_seg_state(&g->fs);
    r->R_GS = transfer_protmode_seg_state(&g->gs);

    rm->rm_drop_int10 = g->drop_int10;
#undef r
}

static bool do_trace;

#define trace(...) if (do_trace) printf(__VA_ARGS__)

#define env  (&rm->rm_x86env)

static inline int handle_fpu(realmode_t *rm, u8 op1)
{
    u8 op2 = fetch_byte_imm();
    u8 mod = op2 >> 6;
    u8 rh = (op2 >> 3) & 7;
    u8 rl  = op2 & 7;

    if (op1 == 0xdb && op2 == 0xe3) {
	DECODE_PRINTF("FINIT\n");
	asm volatile("fninit");
    } else if ((op1 & 3) == 1 && mod != 3 && rh == 7) { 
	u32 offset = 0;
	u16 word;

	if ((op1 & 4) == 0) {
	    DECODE_PRINTF("FSTCW\t");
	    asm volatile("fnstcw %0" : "=m"(word));
	} else {
	    DECODE_PRINTF("FSTSW\t");
	    asm volatile("fnstsw %0" : "=m"(word));
	}

	offset = decode_rm_address(mod, rl);
	DECODE_PRINTF("\n");

	store_data_word(offset, word);
    } else {
	return 0;
    }

    return 1;
}

static int ill1_handler(void *r, u8 op1)
{
    realmode_t *const rm = r;

    switch (op1) {
    case 0xd8: case 0xd9: case 0xda: case 0xdb:
    case 0xdc: case 0xdd: case 0xde: case 0xdf:
	return handle_fpu(rm, op1);
    default:
	return 0;
    }
}

static inline void handle_group_7(realmode_t *rm, u8 op2)
{
    guest_state_t *const g = rm->private;

    int mod, rh, rl;

    FETCH_DECODE_MODRM(mod, rh, rl);

    if (mod == 3) {
	u16 *reg;

	switch (rh) {
	case 4:
	    DECODE_PRINTF("SMSW\t");
	    reg = DECODE_RM_WORD_REGISTER(rl);
	    DECODE_PRINTF("\n");
	    *reg = g->cr0;
	    break;
	case 6:
	    DECODE_PRINTF("LMSW\t");
	    reg = DECODE_RM_WORD_REGISTER(rl);
	    DECODE_PRINTF("\n");
	    g->cr0 &= ~0xf;
	    g->cr0 |= bits(*reg, 3, 0);
	    HALT_SYS();
	    break;
	default:
	    printf("ILLEGAL opcode extention for 0f 01 (mod = 11): %d\n", rh);
	    HALT_SYS();
	}
    } else {
	u32 offset;

	switch (rh) {
	case 0:
	    DECODE_PRINTF("SGDT\t");
	    offset = decode_rm_address(mod, rl);
	    DECODE_PRINTF("\n");
	    store_data_word(offset,   g->gdt.limit);
	    store_data_long(offset+2, g->gdt.base);
	    break;
	case 1:
	    DECODE_PRINTF("SIDT\t");
	    offset = decode_rm_address(mod, rl);
	    DECODE_PRINTF("\n");
	    store_data_word(offset,   g->idt.limit);
	    store_data_long(offset+2, g->idt.base);
	    break;
	case 2:
	    DECODE_PRINTF("LGDT\t");
	    offset = decode_rm_address(mod, rl);
	    DECODE_PRINTF("\n");
	    g->gdt.limit = fetch_data_word(offset);
	    g->gdt.base  = fetch_data_long(offset+2);
	    if (!(env->x86.mode & SYSMODE_PREFIX_DATA)) {
		g->gdt.base &= 0xFFFFFF;
	    }
	    break;
	case 3:
	    DECODE_PRINTF("LIDT\t");
	    offset = decode_rm_address(mod, rl);
	    DECODE_PRINTF("\n");
	    g->idt.limit = fetch_data_word(offset);
	    g->idt.base  = fetch_data_long(offset+2);
	    if (!(env->x86.mode & SYSMODE_PREFIX_DATA)) {
		g->idt.base &= 0xFFFFFF;
	    }
	    break;
	case 4:
	    DECODE_PRINTF("SMSW\t");
	    offset = decode_rm_address(mod, rl);
	    DECODE_PRINTF("\n");
	    store_data_word(offset, g->cr0);
	    break;
	case 6:
	    DECODE_PRINTF("LMSW\t");
	    offset = decode_rm_address(mod, rl);
	    DECODE_PRINTF("\n");
	    g->cr0 &= ~0xf;
	    g->cr0 |= bits(fetch_data_word(offset), 3, 0);
	    HALT_SYS();
	    break;
	default:
	    printf("ILLEGAL opcode extension for 0f 01 (mod != 11): %d\n", rh);
	    HALT_SYS();
	}
    }
}

static inline un *decode_cr(realmode_t *rm, int cr)
{
    guest_state_t *const g = rm->private;

    static un __bad_cr;

    switch (cr) {
    case 0:
	return &g->cr0;
    case 3:
	return &g->cr3;
    case 4:
	return &g->cr4;
    default:
	HALT_SYS();
	return &__bad_cr;
    }
}

static inline void handle_cr(realmode_t *rm, u8 op2)
{
    int mod, rl, rh;
    u32 *reg;
    un *cr;

    DECODE_PRINTF("MOV\t");

    FETCH_DECODE_MODRM(mod, rh, rl);

    if (mod != 3) {
	printf("BAD CR ACCESS\n");
	return;
    }

    cr = decode_cr(rm, rh);

    if ((op2 & 2) == 0) { /* CR read */
	reg = decode_rm_long_register(rl);
	DECODE_PRINTF2(",CR%d\n", rh);
	*reg = *cr;
    } else {
	DECODE_PRINTF2("CR%d,", rh);
	reg = decode_rm_long_register(rl);
	DECODE_PRINTF("\n");
	*cr = *reg;

	if (rh == 0) {
	    /* HALT on CR0.PE enabling */
	    if (bit_test(*cr, CR0_PE)) {
		HALT_SYS();
	    } else {
		trace("New CR0: %08lx\n", *cr);
	    }
	}
    }
}

static inline void handle_wrmsr(realmode_t *rm)
{
    DECODE_PRINTF("WRMSR\n");

    trace("wrmsr %08x, val %08x%08x\n", env->x86.R_ECX,
	  env->x86.R_EDX, env->x86.R_EAX);
}

static inline void handle_rdmsr(realmode_t *rm)
{
    u64 val;

    DECODE_PRINTF("RDMSR\n");

    val = get_MSR(env->x86.R_ECX);

    trace("rdmsr %08x, val %016llx\n", env->x86.R_ECX, val);

    env->x86.R_EDX = val >> 32;
    env->x86.R_EAX = val;
}

static inline void handle_rdtsc(realmode_t *rm)
{
    u64 tsc;

    DECODE_PRINTF("RDTSC\n");

    tsc = rdtsc();

    env->x86.R_EDX = tsc >> 32;
    env->x86.R_EAX = tsc;
}

static inline void handle_cpuid(realmode_t *rm)
{
    DECODE_PRINTF("CPUID\n");
    cpuid(env->x86.R_EAX, env->x86.R_EBX, env->x86.R_ECX, env->x86.R_EDX);
}

static int ill2_handler(void *r, u8 op2)
{
    realmode_t *const rm = r;

    switch (op2) {
    case 0x01:
	handle_group_7(rm, op2);
	return 1;
    case 0x09:
	DECODE_PRINTF("WBINVD\n");
	return 1;
    case 0x20: case 0x22:
	handle_cr(rm, op2);
	return 1;
    case 0x30:
	handle_wrmsr(rm);
	return 1;
    case 0x31:
	handle_rdtsc(rm);
	return 1;
    case 0x32:
	handle_rdmsr(rm);
	return 1;
    case 0xa2:
	handle_cpuid(rm);
	return 1;
    default:
	return 0;
    }
}

static int int_handler(void *r, int intnum)
{
    realmode_t *const rm = r;
    switch (intnum & 0xff) {
    case 0x10:
        if( !rm->rm_drop_int10)
            break;
        trace("Int 10 dropped, %08x %08x %08x %08x\n",
            env->x86.R_EAX, env->x86.R_EBX, env->x86.R_ECX, env->x86.R_EDX);
        env->x86.R_EAX = 0;
        SET_FLAG(F_CF);
        return(1);
    case 0x12:
	trace("Int 12, contiguous conventional memory size\n");
	env->x86.R_AX = guest_e820map[0].size / _K;
	CLEAR_FLAG(F_CF);
	return 1;
    case 0x15:
	if (env->x86.R_AX == 0xe820) {
	    u32 offset = ((u32)env->x86.R_ES << 4) + (env->x86.R_DI);
	    u32 *src;

	    trace("Int 15 E820, system memory map\n");

	    if (env->x86.R_EDX != 0x534D4150 ||
		env->x86.R_EBX >= guest_e820nr ||
		env->x86.R_ECX < sizeof(E820Entry)) {
		SET_FLAG(F_CF);
		return 1;
	    }

	    env->x86.R_EAX = 0x534D4150;
	    CLEAR_FLAG(F_CF);

	    if (env->x86.R_ECX > sizeof(E820Entry)) {
		env->x86.R_ECX = sizeof(E820Entry);
	    }

	    src = (u32 *)&guest_e820map[env->x86.R_EBX];
	    env->memfuncs.wrl(rm, offset,    *src++);
	    env->memfuncs.wrl(rm, offset+4,  *src++);
	    env->memfuncs.wrl(rm, offset+8,  *src++);
	    env->memfuncs.wrl(rm, offset+12, *src++);
	    env->memfuncs.wrl(rm, offset+16, *src++);

	    if (++env->x86.R_EBX == guest_e820nr) {
		env->x86.R_EBX = 0;
	    }
	    return 1;
	} else if (env->x86.R_AX == 0xe801) {
	    trace("Int 15 E801, contiguous extended memory size\n");
	    env->x86.R_CX = env->x86.R_AX = 15 * _K;
	    env->x86.R_DX = env->x86.R_BX = (guest_max_phys_addr-16*_M) / (64*_K);
	    CLEAR_FLAG(F_CF);
	    return 1;
	} else if (env->x86.R_AH == 0x88) {
	    trace("Int 15 88, contiguous extended memory size\n");
	    env->x86.R_AH = 0x80;
	    SET_FLAG(F_CF); // should not use;
	    return 1;
	} else if (env->x86.R_AX == 0x2401) {
	    trace("Int 15 0x2401, A20 enable\n");
	    rm->rm_a20enable = true;
	    CLEAR_FLAG(F_CF);
	    env->x86.R_AH = 0;
	    return 1;
	}
	break;
    case 0x16:
	SET_FLAG(F_ZF);
	if (env->x86.R_AH == 1) {
	    static bool key_pressed = true;
	    if (key_pressed) {
		CLEAR_FLAG(F_ZF);
		env->x86.R_AH = 0x5a; // scancode for enter key
		env->x86.R_AL = '\r'; // ASCII code for enter key
	    }
	    key_pressed = !key_pressed; // Toggle key pressed state
	}
	return 1;
	break;
    case 0x18:
	trace("Int 18, BOOT FAILURE\n");
	while (1);
	break;
    case 0x1a:
	if (env->x86.R_AH == 0) {
	    trace("Int 1A, read clock\n");
	    u64 tsc_pit_ratio = (u64)cpuinfo.cpu_tsc_freq_kHz * 1000 / PIT_FREQ_HZ;
	    u64 pit_clock_count = rdtsc() / tsc_pit_ratio;

	    /* Return pit_clock_count[47:16] in CX:DX */
	    env->x86.R_DX = pit_clock_count >> 16;
	    env->x86.R_CX = pit_clock_count >> 32;

	    trace("Int 1A, ticks since midnight %d\n",
		  ((u32)env->x86.R_CX << 16) | env->x86.R_DX);

	    return 1;
#undef PIT_FREQ_HZ	    

	}
	break;
    }

    trace("Int %02x, %08x %08x %08x %08x\n", intnum & 0xff,
	  env->x86.R_EAX, env->x86.R_EBX, env->x86.R_ECX, env->x86.R_EDX);

    return 0;
}

static void __exec_realmode_guest(guest_state_t *g)
{
    realmode_t *const rm = &realmode_guest;
    int flags;

    bit_clear(g->cr0, CR0_PE);

    spinlock(&realmode_guest_lock, flags);

    env->illfuncs.ill_op  = ill1_handler;
    env->illfuncs.ill_op2 = ill2_handler;
    env->intrfunc = int_handler;

    transfer_state_to_realmode(g, rm);

    rm->private = g;

    while (!bit_test(g->cr0, CR0_PE)) {
	trace("Realmode emulation %s at %04x:%04x, regs: \n"
	      "DS %04x ES %04x FS %04x GS %04x SS %04X\n"
	      "EAX %08x ECX %08x EDX %08x EBX %08X\n"
	      "ESP %08x EBP %08x ESI %08x EDI %08X\n",
	      "started",
	      env->x86.R_CS, env->x86.R_IP,
	      env->x86.R_DS,  env->x86.R_ES,  env->x86.R_FS,  env->x86.R_GS, env->x86.R_SS,
	      env->x86.R_EAX, env->x86.R_ECX, env->x86.R_EDX, env->x86.R_EBX,
	      env->x86.R_ESP, env->x86.R_EBP, env->x86.R_ESI, env->x86.R_EDI);
	realmode_exec(rm);
	trace("Realmode emulation %s at %04x:%04x, regs: \n"
	      "DS %04x ES %04x FS %04x GS %04x SS %04X\n"
	      "EAX %08x ECX %08x EDX %08x EBX %08X\n"
	      "ESP %08x EBP %08x ESI %08x EDI %08X\n",
	      "halted",
	      env->x86.R_CS, env->x86.R_IP,
	      env->x86.R_DS,  env->x86.R_ES,  env->x86.R_FS,  env->x86.R_GS, env->x86.R_SS,
	      env->x86.R_EAX, env->x86.R_ECX, env->x86.R_EDX, env->x86.R_EBX,
	      env->x86.R_ESP, env->x86.R_EBP, env->x86.R_ESI, env->x86.R_EDI);
	if ((env->x86.R_CS == 0) && (env->x86.R_IP == 0))
            break;
    }

    transfer_state_from_realmode(g, rm);

    spinunlock(&realmode_guest_lock, flags);

    trace("Entered protected mode.\n");
}

#define PART_TABLE_START 0x1BE

static void __load_active_bootsec(void)
{
    realmode_t *const rm = &realmode_guest;
    int flags;
    int i;

    spinlock(&realmode_guest_lock, flags);

    env->illfuncs.ill_op  = NULL;
    env->illfuncs.ill_op2 = NULL;
    env->intrfunc = NULL;

    env->x86.R_CS = 0;
    env->x86.R_IP = 0x600;

    env->x86.R_AH = 0;		// resset
    env->x86.R_DL = 0x80;	// C:
    realmode_int(rm, 0x13);

    env->x86.R_AH = 0x2;	// read
    env->x86.R_AL = 1;		// num sectors
    env->x86.R_DL = 0x80;	// C:
    env->x86.R_DH = 0;		// head
    env->x86.R_CL = 1;		// sector
    env->x86.R_CH = 0;		// cylinder
    env->x86.R_ES = 0;		// Destination: ES:BX
    env->x86.R_BX = 0x7c00;	// 0000:7c00
    realmode_int(rm, 0x13);

#ifdef NOTDEF
    printf("MBR:\n");
    for (i = 0; i < 512; i++) {
	printf("%02x ", env->memfuncs.rdb(rm, 0x7c00 + i));
	if ((i & 0xf) == 0xf) {
	    printf("\n");
	}
    }
#endif

    for (i = 0; i < 4; i++) {
	u32 part_off = 0x7c00 + PART_TABLE_START + i*16;
	u8 status = env->memfuncs.rdb(rm, part_off);

	if (status == 0x80) {
	    env->x86.R_AH = 0x2;
	    env->x86.R_AL = 1;
	    env->x86.R_DL = 0x80;
	    env->x86.R_DH = env->memfuncs.rdb(rm, part_off + 1);
	    env->x86.R_CL = env->memfuncs.rdb(rm, part_off + 2);
	    env->x86.R_CH = env->memfuncs.rdb(rm, part_off + 3);
	    env->x86.R_ES = 0;		// Destination: ES:BX
	    env->x86.R_BX = 0x7c00;	// 0000:7c00

	    realmode_int(rm, 0x13);
	    break;
	}
    }

    if (i == 4) {
	WARN("Active partition not found!\n");
    }

    spinunlock(&realmode_guest_lock, flags);
}

#undef env

void ghv_guest_boot_mbr(void)
{
    guest_state_t *const g = &guest_state[0];

    __load_active_bootsec();

    init_guest_state(g);

    g->cs.base = 0;
    g->regs.guest_rip = 0x7c00;
    g->regs.rdx = 0x80; // BIOS boot drive: C:

    __exec_realmode_guest(g);

    g->active = true;
}

void ghv_guest_boot_linux(bool drop_int10)
{
    guest_state_t *const g = &guest_state[0];
    u32 base = LINUX_KERN16_START;

    init_guest_state(g);
    g->drop_int10 = drop_int10;

    g->cs.base = base + 0x200;
    g->regs.guest_rip = 0;
    g->ss.base = base;
    g->regs.guest_rsp = 0;
    g->ds.base = base;
    g->es.base = base;

    __exec_realmode_guest(g);

    g->active = true;
}

void ghv_guest_acpi_resume(u32 vector)
{
    guest_state_t *const g = &guest_state[0];

    init_guest_state(g);

    g->cs.base = vector;
    g->regs.guest_rip = 0;
    g->ss.base = vector;
    g->regs.guest_rsp = 0;
    g->ds.base = vector;
    g->es.base = vector;

    __exec_realmode_guest(g);

    g->active = true;
}

void ghv_guest_resume_in_realmode(guest_state_t *g)
{
    __exec_realmode_guest(g);
}

// print guest message;

void
ghv_guest_printk(registers_t *regs)
{
    addr_t gva;
    un len;
    int sz;
    char *p, buf[1024];

    gva = regs->rbx & 0xffffffffUL; //XXX validate;
    len = regs->rcx & 0xffffffffUL;
    if( !gva || !len) {
        return;
    }

    // the linux console driver tries to output full lines,
    // except when dumping accumulated messages from the log buffer;
    // try to detect this based on lenght of request;

    if(len < sizeof(buf)) {
        copy_from_guest(buf, gva, len);
        p = buf + len;
        if(p[-1] != '\n') {
            *p++ = '\n';
        }
        *p = 0;
        kprintf("=%s", buf);
        return;
    }

    // catch-up with accumulated messages;

    kprintf("=...\n");
    while(len) {
        sz = sizeof(buf) - 1;
        if(len < sz) {
            sz = len;
        }
        copy_from_guest(buf, gva, sz);
        buf[sz] = 0;
        console_write(buf, sz);

        gva += sz;
        len -= sz;
    }
    kprintf("=...\n");
}


int debug_dbg(un offset, un size, un gvaddr)
{
    void *buf = malloc(size);
    extern void dbg_read(un offset, un size, void *out);

    if (!buf) return -1;

    dbg_read(offset, size, buf);
    copy_to_guest(gvaddr, buf, size);
    free(buf);

    return 0;
}

// handle ghv hypercalls;
// return 0 for invalid calls, 1 for valid;
bool ghv_handle_hypercall(registers_t *regs, bool usermode)
{
    switch (regs->rax) {
    case 200: case 201: case 202: case 203: case 204: case 206:
        if (usermode) {
            return(0);
        }
    case 205:
        fastswitch_handle_hypercall(regs);
        break;
    case 207:
// Note: Unclear to me if we want to kill this hypercall altogether
// for the production release and return an error to GVM. Would require mods
// to GVM to not make this hypercall...
#if defined(GVM_FEAT_GHV_DBG)
	ghv_guest_printk(regs);
#endif // GVM_FEAT_GHV_DBG
	break;
    case 208:
	regs->rax = ghv_upload(regs->rbx, regs->rcx, (u32)regs->rdx);
	break;
    case 209:
	regs->rax = debug_dbg(regs->rbx, regs->rcx, regs->rdx);
	break;
    default:
	return gvm_vmcall(regs, usermode);
    }
    return(1);
}

bool ghv_handle_io_exit(u16 port, u8 iotype, un *val)
{
    int v;

    v = acpi_virt_io(port, iotype, val);
    if(v >= 0)
        return(v);
    return(fastswitch_trigger(1));
}
