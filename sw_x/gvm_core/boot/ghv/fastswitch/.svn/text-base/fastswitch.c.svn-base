#include "km_types.h"
#include "bits.h"
#include "x86.h"
#include "kvtophys.h"
#include "kprintf.h"
#include "assert.h"
#include "pt.h"
#include "dt.h"
#include "string.h"
#include "printf.h"
#include "bitmap.h"
#include "apic.h"
#include "pagepool.h"
#include "malloc.h"
#include "valloc.h"
#include "bootparam.h"
#include "vmx.h"
#include "setup/gvmdefs.h"
#include "linuxbootdefs.h"
#include "e820.h"
#include "gpt.h"
#include "console.h"

#include "pci.h"
#include "acpi-api.h"
#include "fastswitch.h"
#include "ghv_guest.h"
#include "ghv_mem.h"
#include "console.h"
#include "smp.h"
#include "ghv_load.h"
#include "gvm.h"

typedef u32 pfn_t;
static pfn_t *saved_pfn_map;
static u32 saved_pfn_nr = 0;

#define	SWITCH_STATE_BAD 0
#define	SWITCH_STATE_OS1 1
#define	SWITCH_STATE_OS2 2

u32 switch_state = SWITCH_STATE_OS1;

#define	SWITCH_FLAG_SERIAL_PORT    0x0000ffff // pci serial port;
#define	SWITCH_FLAG_OS1_SLEEP      0x00010000 // os1->os2 via sleep;
#define	SWITCH_FLAG_OS2_SLEEP      0x00020000 // os2->os1 via sleep;
#define	SWITCH_FLAG_WAKEUP_RTC     0x00040000 // use rtc for wakeup;
#define SWITCH_FLAG_DIRECT	   0x00080000 // Direct switch, bypass ACPI
#define	SWITCH_FLAG_OS2_DROP_INT10 0x00100000
#define SWITCH_FLAG_DEBUG_EHCI     0x10000000
#define SWITCH_FLAG_DEBUG_1394     0x20000000
#define SWITCH_FLAG_DEBUG_SERIAL   0x40000000 // Onboard serial
#define SWITCH_FLAG_DEBUG_EARLY    0x80000000

static u32 switch_flags = 0
    | SWITCH_FLAG_OS1_SLEEP
    | SWITCH_FLAG_OS2_SLEEP
    | SWITCH_FLAG_WAKEUP_RTC
    | SWITCH_FLAG_OS2_DROP_INT10;

extern void init_guest_state(guest_state_t *);
extern void vmx_on(void);
extern void vmx_guest_init(void);
extern void __attribute__((noreturn)) exit(int r);

extern void vmx_guest_init(void);

static inline addr_t pfn_to_addr(pfn_t pfn)
{
    return phystokv((paddr_t)pfn << VM_PAGE_SHIFT);
}

// memory corruption check code;

#ifdef	MEMCHK

#define MEMCHK_START (1*1024*1024ULL)      // after BIOS space;
#define MEMCHK_MAX   (8*1024*1024*1024ULL) // max of 8GB physical memory;

// need a bitmap of pages not to check;
// for the address range of interest;

bitmap_t crc_not[bitmap_word_size(MEMCHK_MAX/VM_PAGE_SIZE)];
bitmap_t crc_mem[bitmap_word_size(MEMCHK_MAX/VM_PAGE_SIZE)];

// compute a crc32 over each page, allocated in crc_h;
// also compute vertical crc32 over each word, in crc_v[], state in crc_s[];

static u32 *crc_h;
static u32 crc_v[VM_PAGE_SIZE / sizeof(un)];
static u32 crc_s[VM_PAGE_SIZE / sizeof(un)];

// compute crc over a block;
// must be a multiple of un size;

static u32
crc_horz(addr_t va, u32 size)
{
    un *s = (un *)va;
    un crc;

    size /= sizeof(un);
    crc = va | (va >> 32);
    while(size--) {
        asm volatile("crc32q (%1),%0" : "+r"(crc) : "r"(s) : "cc");
        s++;
    }
    return((u32)crc);
}

// compute vertical crc over page's un words;

static void
crc_vert(addr_t va)
{
    un *s = (un *)va;
    u32 *c = crc_s;
    un size = VM_PAGE_SIZE / sizeof(un);
    un crc;

    while(size--) {
        crc = *c;
        asm volatile("crc32q (%1),%0" : "+r"(crc) : "r"(s) : "cc");
        *c++ = crc;
        s++;
    }
}

// initialize vertical crc;

static void
crc_vinit(void)
{
    u32 *c = crc_s;
    u32 n;

    for(n = 0; n < (VM_PAGE_SIZE / sizeof(un)); n++) {
        *c++ = n | (~n << 16);
    }
}

// compute or check crc;

static void
crc_x(int check)
{
    u32 pfn, crc, n;
    u32 nunmapped = 0;
    u32 bad = 0;
    addr_t va;
    extern bool is_mapped(addr_t);

    kprintf(" ## crc %s\n", check? "check" : "compute");
    if( !crc_h)
        crc_h = malloc(MEMCHK_MAX/VM_PAGE_SIZE * sizeof(u32));
    assert(crc_h != 0);

    // do horizontal crc32;
    // update vertical crc32 state;

    crc_vinit();
    for(pfn = 0; pfn < MEMCHK_MAX/VM_PAGE_SIZE; pfn++) {
        if( !bitmap_test(crc_mem, pfn))
            continue;                    // not memory region;
        if(bitmap_test(crc_not, pfn))
            continue;                    // do not check excluded pfns;

        va = ((addr_t)pfn) << VM_PAGE_SHIFT;
        if(is_mapped(va)) {
            crc = crc_horz(va, VM_PAGE_SIZE);
            crc_vert(va);
        } else {
            crc = 0;
            nunmapped++;
        }
        if(check) {
            if(crc != crc_h[pfn]) {
                bad++;
                kprintf(" ## crc_h %x000\n", pfn);
            }
        } else {
            crc_h[pfn] = crc;
        }
    }

    // save vertical crc on compute;
    // compare vertical crc on check;

    if(check) {
        for(n = 0; n < VM_PAGE_SIZE / sizeof(un); n++) {
            if(crc_v[n] != crc_s[n])
                kprintf(" ## crc_v %x\n", n);
        }
    } else {
        memcpy(crc_v, crc_s, sizeof(crc_s));
    }
    n = bitmap_count(crc_not, MEMCHK_MAX/VM_PAGE_SIZE);
    kprintf(" ## crc %s: bad=%d no_check=%d unmapped=%d\n",
        check? "check" : "bad", bad, n, nunmapped);
}

// API;

// add a pfn to list of pfns not to check;

void
crc_no_check(u32 pfn)
{
    assert(pfn < (MEMCHK_MAX >> VM_PAGE_SHIFT));
    bitmap_set(crc_not, pfn);
}

// limit crc check to e820 memory regions;

static void
crc_e820(void)
{
    E820Entry e;
    int i;
    u64 pfn, last;

    // do not check BIOS space;

    for(pfn = 0; pfn < MEMCHK_START/VM_PAGE_SIZE; pfn++) {
        bitmap_set(crc_not, pfn);
    }

    // check only memory regions;

    bitmap_init(crc_mem, MEMCHK_MAX/VM_PAGE_SIZE, 0);
    for(i = 0; i < e820_nr(); i++) {
        e820_get(i, &e);
        if(e.type != E820_RAM)
            continue;
        kprintf(" ## crc mem %llx..%llx\n", e.addr, e.addr + e.size);
        pfn = TRUNC_PAGE(e.addr + VM_PAGE_SIZE - 1) >> VM_PAGE_SHIFT;
        last = TRUNC_PAGE(e.addr + e.size + VM_PAGE_SIZE - 1) >> VM_PAGE_SHIFT;
        assert(last < MEMCHK_MAX/VM_PAGE_SIZE);
        while(pfn <= last) {
            bitmap_set(crc_mem, pfn++);
        }
    }
}

#else // MEMCHCK

static void crc_x(int check) { }
void crc_no_check(u32 pfn) { }
void crc_e820(void) { }

#endif // MEMCHCK

static void probe_pfns(void)
{
    size_t len;
    pfn_t *dbg_pfns;
    int i;

    saved_pfn_map = ghv_blob_retrieve(GHV_BLOB_FASTSWITCH_PFNS, &len);
    saved_pfn_nr = len/sizeof(pfn_t);

    if ((saved_pfn_nr << VM_PAGE_SHIFT) >= 0xC0000000) {
	saved_pfn_nr = 0xC0000000 >> VM_PAGE_SHIFT;
    }

    printf("Probing pfns\n");
    for (i = 0; i < saved_pfn_nr; i++) {
        crc_no_check(saved_pfn_map[i]);
    }

    dbg_pfns = ghv_blob_retrieve(GHV_BLOB_DBG_PFNS, &len);
    if (dbg_pfns) {
	dbg_init(dbg_pfns, len / sizeof(pfn_t));
	free(dbg_pfns);
    }
}

static void fastswitch_late_init(void)
{
    static int initialized = 0;

    if (!initialized) {
	pci_init();
	pci_scan_all();
	probe_pfns();

	initialized = 1;
    }
}

static void
memsave(addr_t p, addr_t q, size_t count)
{
    un *pl = (void *)p;
    un *ql = (void *)q;

    count /= sizeof(un);

    while (count-- > 0) {
        *pl++ = *ql++;
    }
}

static void
memswap(addr_t p, addr_t q, size_t count)
{
    un *pl = (void *)p;
    un *ql = (void *)q;
    
    count /= sizeof(un);
    
    while (count-- > 0) {
	un a, b;
	
	a = *pl;
	b = *ql;
	
	*pl = b;
	*ql = a;
	
	pl++;
	ql++;
    }
}

static void mem_page(pfn_t pfn, void (*mem)(addr_t, addr_t, size_t))
{
    addr_t src = pfn_to_addr(pfn);
    addr_t dst = pfn_to_addr(saved_pfn_map[pfn]);

    if (src == dst) {
        if(pfn < 0x100) {
            WARN("PANIC! Bios mem page %d\n", pfn);
            while(1);
        }
	return;
    }

    if (dst < saved_pfn_nr * VM_PAGE_SIZE) {
	WARN("PANIC! overlapping memory region detected.\n"
	       "Refusing to swap %lx <-> %lx\n", src, dst);
	while (1);
    }

    mem(dst, src, VM_PAGE_SIZE);
}

static void memswap_all(u32 state)
{
    pfn_t i;
    E820Entry e = { .addr = 0, .size = 0xA0000 };
    pfn_t lower_pfn_limit;
    void (*memfunc)(addr_t p, addr_t q, size_t count);
    char c;

    // compute crc before memswap os1->os2;

    if(state == SWITCH_STATE_OS1) {
        crc_x(0);
    }

    printf("swapping %d pages, state=%d\n", saved_pfn_nr, state);

    e820_get(0, &e);
    lower_pfn_limit = (e.addr + e.size) / VM_PAGE_SIZE;

    printf("bios mem\n-");
    for (i = 1; i < 0x100; i++) {
        if((i >= 1) && (i < lower_pfn_limit)) {
            memfunc = memswap;
            c = '.';
        } else if(state == SWITCH_STATE_OS1) {
            memfunc = memsave;
            c = '*';
        } else if(state == SWITCH_STATE_OS2) {
            memfunc = 0;
            c = '?';
        } else {
            memfunc = 0;
            c = '-';
        }

        if(memfunc) {
            mem_page(i, memfunc);
        }

        printf("%c", c);
        if((i & 63) == 63) {
	    printf("\n");
        }
    }

    printf("pfns mem\n");
    for (i = 0x100; i < saved_pfn_nr; i++) {
	if ((i & 0x3fff) == 0) {
	    printf("\n");
	}
	mem_page(i, memswap);
	if ((i & 0xff) == 0) {
	    printf(".");
	}
    }

    printf("\n");

    // check crc after memswap os2->os1;

    if(state == SWITCH_STATE_OS2) {
        crc_x(1);
    }
    wbinvd();
}

#define HPET_BASE_ADDRESS	0xfed00000
#define HPET_CFG		0x010
#define HPET_CFG_ENABLE		0x001
#define HPET_CFG_LEGACY		0x002

/* i8253A PIT registers */
#define PIT_MODE		0x43
#define PIT_CH0			0x40
#define PIT_CH2			0x42

static void fastswitch_reset_devices(void)
{
    /* Disable HPET emulation of legacy timer */
    addr_t hpet_base = map_page(HPET_BASE_ADDRESS, MPF_IO);
    volatile_write((u32 *)(hpet_base + HPET_CFG), 0);
    unmap_page(hpet_base, UMPF_LOCAL_FLUSH);

    /* Enable the PIT */
    outb(PIT_MODE, 0x34);
    outb(PIT_CH0, PIT_INTERVAL & 0xff);
    outb(PIT_CH0, PIT_INTERVAL >> 8);    
}

// reset all application processors (APs);
// We used to always stop all APs, which is a requirement for directly
// switching to new guest without going through power-off. However, some
// systems hard-reset when an INIT is sent to any AP (probably because some
// SMM code interferes. This pretty much forces us to always go through
// the power-off path to guarantee that all cores are reset.
// define FASTSWITCH_STOP_APS to force all APs into INIT

static void
stop_aps(void)
{
#ifdef FASTSWITCH_STOP_APS
    apic_broadcast_INIT();
    mdelay(10);
#endif // FASTSWITCH_STOP_APS
}

// fastswitch panic;

static void __attribute__((noreturn))
fastswitch_panic(void)
{
    WARN("fastswitch panic 0x%x\n", switch_state);
    switch_state = SWITCH_STATE_BAD;
    while(1); //XXX should fault to reset the machine
}

// resume handler for sleep path;
// power-off s3 os1->os2;

static void
resume_os2(void)
{
    bool drop_int10;

    if (!(switch_flags & SWITCH_FLAG_DIRECT)) {
	cmos_save_state();  // must be called before acpi_save_state();
	acpi_save_state();
	acpi_shutdown();
    }

    e820_init();
    crc_e820();

    ghv_guest_update_memsize(saved_pfn_nr * VM_PAGE_SIZE);

    // smp_init cannot be called in the context of os1;
    // bp global flags need to be valid before smp_init();

    memswap_all(switch_state);
    switch_state = SWITCH_STATE_OS2;

    if (!gvm_init(saved_pfn_nr)) {
	fastswitch_exit();
    }

    bp->bp_flags &= ~(BPF_NO_VAPIC | BPF_NO_EXTINT | BPF_NO_VTD | BPF_NO_EPT);
    smp_init();
    drop_int10 = ((switch_flags & SWITCH_FLAG_OS2_DROP_INT10) != 0);
    ghv_guest_boot_linux(drop_int10);
    vmx_guest_init();
}

// resume os1;

static void
resume_os1(void)
{
    u32 vector;
    u64 t_c;

    memswap_all(switch_state);
    cmos_restore_state();
    acpi_restore_state();
    vector = acpi_get_waking_vector();
    acpi_shutdown();

    t_c = cache_timing(10);

    printf("Cache timing of %d reads: %lld\n", 10, t_c);

    switch_state = SWITCH_STATE_OS1;
    bp->bp_flags |= (BPF_NO_VAPIC | BPF_NO_EXTINT | BPF_NO_VTD | BPF_NO_EPT);
    ghv_guest_acpi_resume(vector);
    cpu_vm[0].v_cache_timing_thres = t_c * 3;
    vmx_guest_init();
}

// resume handler to call after ghv resume;

static void (*resume_hdl)(void);
static volatile addr_t ghv_wakeup_trampoline;
static void *saved_trampoline_area;

// resume ghv;

void
resume_ghv(int cpuno)
{
    guest_state_t *gs;

    // try console init for builtin ports;
    if (switch_flags & SWITCH_FLAG_DEBUG_SERIAL) {
        console_init(CONSOLE_ID_SERIAL, -1);
    }
    kprintf("ghv resume.\n");

    // take acpi out of sleep;

    acpi_hw_wakeup();

    // must rescan pci;
    // we cannot do this before acpi has returned the system to S0;
    // debug devices will be reinitialized;

    pci_rescan();

    // restore the trampoline code that got us here

    trampoline_deinit(ghv_wakeup_trampoline, saved_trampoline_area);
    ghv_wakeup_trampoline = 0;
    saved_trampoline_area = NULL;

    // restore cmos to turn off periodic interrupts;

    cmos_tmp_state(0);

    // re-initialize vmx globals;

    active_cpumask = 0;
    for(cpuno = 0; cpuno < NR_CPUS; cpuno++) {
        cpu_vm[cpuno].v_in_vmx = false;
        gs = guest_state + cpuno;
        init_guest_state(gs);
        gs->active = 0;
    }

    // set cpu state;

    set_CR3(host_cr3);
    load_hyp_segments(&hyp_gdt[0]);
    set_IDTR(&hyp_idtr);

    // put BP under vmx;

    vmx_on();
    if(bp->ret[0]) {
        WARN("vmxon failed with ret %d!\n", bp->ret[0]);
    } else if(resume_hdl) {
        resume_hdl();
    }
    fastswitch_panic();
}

// prepare and sleep ghv;
// setup for possible automatic wakeup;

void
sleep_ghv(void)
{
    kprintf("ghv sleep.\n");

    // use same trampoline code for BP wakeup;
    // need to sleep/wakeup the gvm side, as trampoline memory is needed;

    ghv_wakeup_trampoline = trampoline_init(&hyp_gdt[0], resume_ghv,
					    &saved_trampoline_area);
    acpi_set_waking_vector(ghv_wakeup_trampoline);

    // optionally, use rtc to wake us up;

    if(switch_flags & SWITCH_FLAG_WAKEUP_RTC) {
        acpi_force_wakeup();
    }

    // turn off vmx on core 0;

    vmclear(kvtophys(cpu_vm[0].v_guest_vmcs));
    vmxoff();

    // stop all pci debug devices;
    // make hardware go through sleep;

    pci_debug_stop();
    acpi_hw_sleep();
}

// shot down os2;
// args are printk buffer of guest;

static void
shoot_os(int byguest)
{
    int n = cpuno();
    int to;

    // can only fastswitch back if in os2;

    kprintf("%s byguest=%d state=%d\n", __FUNCTION__, byguest, switch_state);
    if(switch_state != SWITCH_STATE_OS2 || (switch_flags & SWITCH_FLAG_DIRECT)) {
        return;
    }

    // only send INITs to all other cores one time;
    // multiple INITs would reset cores, after vmxoff();

    if(byguest) {
        apic_broadcast_INIT();
        mdelay(10);
    }

    asm volatile("lock andl %1, %0\n"  :
        "=m"(smp_call_in_mask) :
        "r"(~(1 << n)) : "cc");

    // wait for all cores to exit;
    // timeout for cores that the guest did not claim;

    for(to = 500/10; --to > 0;) {
        if(smp_call_in_mask == 0) {
            break;
        }
#ifdef DEBUG_CORE_EXIT
        kprintf("active cores 0x%x\n", smp_call_in_mask);
#endif // DEBUG_CORE_EXIT
        mdelay(10);
    }
    kprintf("active cores 0x%x\n", smp_call_in_mask);

    // turn vmx mode off;

    if(n != 0) {
         vmclear(kvtophys(cpu_vm[n].v_guest_vmcs));
         vmxoff();
    }

    // APs will loop until hard-reset;

    while(n != 0) {
        pause();
    }

    // vmx off on all APs, and running on #0;
    // trigger fastswitch from #0;

    stop_aps();

    kprintf("shooting os2\n");
    fastswitch_trigger(0);
}

// check of we can switch back to windows;
// this is called when the hv exits, due to guest crashes;
// if this function returns, then try to vmxoff and apic reset;
// only returns when we cannot switch back;

void
fastswitch_exit(void)
{
    shoot_os(0);
}

// longjmp back to guest;

static int
longjmp_to_guest(registers_t *regs, un ret)
{
    un cr4;

    if (switch_state != SWITCH_STATE_OS1) {
	return -1;
    }

    extern void vmx_copy_guest_state_to_host(void);
    extern void iret_to_guest(registers_t *regs);

    kprintf("%s>Mapping GHV into guest provided page tables...\n", __func__);
    ghv_map_to_guest(vmread(VMCE_GUEST_CR3));

    kprintf("%s>Copying guest state to host...\n", __func__);
    vmx_copy_guest_state_to_host();

    kprintf("%s>vmxoff...!\n", __func__);
    cr4 = vmread(VMCE_GUEST_CR4);
    vmxoff();
    bit_clear(cr4, CR4_VMXE);
    set_CR4(cr4);
    cr4 = get_CR4();
    assert(!bit_test(cr4, CR4_VMXE));

    kprintf("%s>Goodbye!\n", __func__);
    regs->rax = ret;
    iret_to_guest(regs);

    return 0;
}

// handle switch flags;

static un
handle_flags(un flags)
{
    un ret;

    ret = switch_flags;
    switch_flags = flags;

    // this may crash the system if port is not a serial ctrl;
    // only for debug, so this is ok;

    if (flags & SWITCH_FLAG_SERIAL_PORT) {
        pci_sio_set(flags & SWITCH_FLAG_SERIAL_PORT);
    }

    if (flags & SWITCH_FLAG_DIRECT) {
	kprintf("Direct switch!\n");
	resume_os2();
    }

    return(ret);
}

// handle fastswitch hypercalls;

void
fastswitch_handle_hypercall(registers_t *regs)
{
    switch (regs->rax) {
    case 200:
        regs->rax = saved_pfn_nr;
        break;
    case 202:
        regs->rax = longjmp_to_guest((registers_t *)regs->rbx, regs->rcx);
        break;
    case 203:
        acpi_map_fadt(regs->rbx, regs->rcx);
        if(regs->rcx) {
            pci_debug_stop();
        }
        break;
    case 204:
	fastswitch_late_init();
        regs->rax = handle_flags(regs->rbx);
        break;
    case 205:
        shoot_os(1);
        break;
    case 206:
        regs->rax = pci_scan(regs->rbx, regs->rcx);
        break;
    }
}

// do the fastswitch;
// need to know if we came from a trap or not;

bool
fastswitch_trigger(int trap)
{
    u32 flags, got_tmp;

    if (!saved_pfn_nr || !saved_pfn_map) {
	return true;
    }

    // switch phase which is still abortable;
    // need to stop kprintf to gvm buffer;
    // turn off consoles that are not builtin;

    kprintf("switch flags=0x%x\n", switch_flags);
    console_exit(CONSOLE_ID_ALL, 1);

    // we are running on BP, APs could be doing anything,
    // meaning they could crash any way while memswap()ing;
    // we need to reset the APs by sending INITs;

    stop_aps();

    // acpi_init() destroys state in fadt/facs;
    // save current acpi state into tmp;
    // os2 shootdown may not have a tmp state saved;
    // unarm acpi traping;

    cmos_tmp_state(1);
    got_tmp = acpi_tmp_state(1);
    acpi_unarm();

    // os1->os2 requires a tmp state;

    if((switch_state == SWITCH_STATE_OS1) && !got_tmp) {
        return(true);
    }

    // must have valid acpi to switch;
    // if none, let the os go to sleep;
    // leaves acpica active, because sleep_ghv needs it;

    if( !acpi_fastswitch(trap)) {
        kprintf("ACPI not ready!\n");
        return(true);
    }

    // rescan pci for debug;
    // we cannot do this before acpi has returned the system to S0;
    // debug devices will be reinitialized;

    if(switch_flags & SWITCH_FLAG_DEBUG_EARLY) {
        pci_rescan();
    }

    // if we want to go through the S3 sleep path,
    // then assign ghv resume handlers and go into S3;

    flags = 0;
    switch(switch_state) {

    case SWITCH_STATE_OS1:
        flags = SWITCH_FLAG_OS1_SLEEP;
        resume_hdl = resume_os2;
        break;

    case SWITCH_STATE_OS2:
        flags = SWITCH_FLAG_OS2_SLEEP;
        resume_hdl = resume_os1;
        break;
    }
    if(switch_flags & flags) {
        sleep_ghv();
        fastswitch_panic();
    }

    // committed to direct switching;

    fastswitch_reset_devices();
    mdelay(100);

    switch(switch_state) {

    case SWITCH_STATE_OS1:
        resume_os2();
        break;

    case SWITCH_STATE_OS2:
        resume_os1();
        break;
    }
    fastswitch_panic();
}
