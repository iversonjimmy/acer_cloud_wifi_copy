// acpi.c
// ghv acpi interface;

#include "km_types.h"
#include "bits.h"
#include "x86.h"
#include "e820.h"
#include "kprintf.h"
#include "assert.h"
#include "pt.h"
#include "dt.h"
#include "string.h"
#include "bitmap.h"
#include "apic.h"
#include "malloc.h"
#include "valloc.h"
#include "bootparam.h"
#include "console.h"
#include "printf.h"
#include "ghv_guest.h"

#include "acpi.h"
#include "acconfig.h"
#include "actypes.h"
#include "aclocal.h"
#include "acmacros.h"
#include "acobject.h"
#include "acstruct.h"
#include "acglobal.h"
#include "actbl.h"
#include "actables.h"
#include "acnamesp.h"
#include "achware.h"
#include "acutils.h"
#include "acpi-api.h"

#include "pci.h"

typedef struct {
    UINT8 descriptor;
    UINT16 length;
    ACPI_RESOURCE_GENERIC_REGISTER reg;
} __attribute__ ((packed)) ACPI_POWER_REGISTER;

// debug;

#undef DEBUG_ACPI
#undef DEBUG_ACPI_OS

#ifdef DEBUG_ACPI
#define dbg(x) kprintf x
#else // DEBUG_ACPI
#define dbg(x)
#endif // DEBUG_ACPI

#ifdef DEBUG_ACPI_OS
#define dbgos(x) kprintf x
#else // DEBUG_ACPI_OS
#define dbgos(x)
#endif // DEBUG_ACPI_OS

// acpica defines;

#define _COMPONENT ACPI_OS_SERVICES ACPI_MODULE_NAME ("acpigvm")

// rtc saved state;

typedef struct {
    u8 r0a;        // rtc register 0x0a, status A;
    u8 r0b;        // rtc register 0x0b, status B;
    u8 r0d;        // rtc battery status;
} rtc_state_t;

// acpi saved state;

typedef struct {
    rtc_state_t rtc;
    ACPI_TABLE_FACS facs;
    u16 pm1_ena[2];
} acpi_state_t;

// embedded controller;

typedef struct {
    ACPI_HANDLE handle;
    UINT64 cmd;     // cmd/status port;
    UINT64 data;    // data port;
    u8 gpe;
    u8 as_handler;  // addr space handler installed;
    struct {
        u8 cmd;     // cmd;
        u8 w[4];    // write stream, [0]=len;
        u8 r[4];    // read stream, [0]=len;
    } x;
} acpi_ec_t;

// ec status register;

#define ACPI_EC_STS_OBF 0x01
#define ACPI_EC_STS_IBF 0x02
#define ACPI_EC_STS_BURST 0x10
#define ACPI_EC_STS_SCI 0x20

/* EC commands */
enum ec_command {
    ACPI_EC_COMMAND_READ = 0x80,
    ACPI_EC_COMMAND_WRITE = 0x81,
    ACPI_EC_BURST_ENABLE = 0x82,
    ACPI_EC_BURST_DISABLE = 0x83,
    ACPI_EC_COMMAND_QUERY = 0x84,
};

#define ACPI_EC_TO_X 100 // ec transaction timeout;
#define ACPI_EC_DELAY_POLL 10 // delay before polling, usecs;

// acpi globals;

typedef struct {
    u16 state;
    u16 pm1_val;            // sleep state value;
    u16 pm1_ctl[2];         // pm1[ab]_ctl ports;
    u8 pm1_ss[8];           // sleep state types per Sx state;
    u32 fadt_phys;          // fadt phys address from os1;
    u32 fadt_size;          // size of tmp fadt;
    struct {
        u32 count;          // # of processors from acpi;
        u32 mask;           // one bit per cpu;
    } cpu;
    acpi_ec_t ec;           // embedded ctrl;
    paddr_t rsdp;           // phys addr of rsdp;
	ACPI_TABLE_FADT fadt;	// copy of os1 fadt;
} acpi_t;

#define	ACPI_STATE_NOT 0
#define	ACPI_STATE_OK 0x01

static acpi_state_t state_tmp; // tmp state at switch point;
static acpi_state_t state_os1; // state of os1;
static acpi_t acpi_gbl;

// outb to CMOS_ADDR set byte index;
// following in/out to CMOS_DATA read/write that byte;

#define	CMOS_ADDR 0x70
#define	CMOS_DATA 0x71

#define	CMOS_PERIODIC_INTR_500MS 0x0f
#define	CMOS_PERIODIC_INTR_MASK 0x40
#define	CMOS_BATTERY_GOOD 0x80

#define	CMOS_GET(reg, data) outb(CMOS_ADDR, reg), data = inb(CMOS_DATA)
#define	CMOS_SET(reg, data) outb(CMOS_ADDR, reg), outb(CMOS_DATA, data)

// get cmos state;

static void
cmos_get(rtc_state_t *rtc)
{
    CMOS_GET(0x0d, rtc->r0d);
    CMOS_GET(0x0a, rtc->r0a);
    CMOS_GET(0x0b, rtc->r0b);
}

// put cmos state;

static void
cmos_put(rtc_state_t *rtc)
{
    u16 v;

    CMOS_GET(0x0d, v);
    if(v & CMOS_BATTERY_GOOD) {
        CMOS_SET(0x0a, rtc->r0a);
        CMOS_SET(0x0b, rtc->r0b);
    }
    CMOS_GET(0x0c, v);      // read reg 0x0c clears pending interrupt;
}

// save cmos tmp state;

void
cmos_tmp_state(int save)
{
    rtc_state_t *rtc = &state_tmp.rtc;

    if(save) {
        cmos_get(rtc);
    } else {
        cmos_put(rtc);
    }
}

// save cmos state of os1;

void
cmos_save_state(void)
{
    state_os1.rtc = state_tmp.rtc;
}

// restore cmos state of os1;

void
cmos_restore_state(void)
{
    cmos_put(&state_os1.rtc);
}

// dump cmos content;

void
cmos_dump(void)
{
    u32 n;

    printf("cmos:");
    for(n = 0; n < 0x80; n++) {
        if((n % 16) == 0)
            printf("\n%02x:", n);
        outb(0x70, n);
        printf(" %02x", inb(0x71));
    }
    printf("\n");
}

// get firmware waking vector;

u32
acpi_get_waking_vector(void)
{
    return(AcpiGbl_FACS->FirmwareWakingVector);
}

// set firmware waking vector;

void
acpi_set_waking_vector(u32 vector)
{
	AcpiSetFirmwareWakingVector(vector);
}

// remove acpi pm1 io traps;

static void
acpi_pm1_unarm(void)
{
    acpi_t *ap = &acpi_gbl;

    if(ap->pm1_ctl[0]) {
        io_bitmap_clear(ap->pm1_ctl[0]);
    }
    if(ap->pm1_ctl[1]) {
        io_bitmap_clear(ap->pm1_ctl[1]);
    }
}

// init pm1_ctl registers;
// no longer a hypercall, as fadt is passed from os1;

static void
acpi_pm1_ctl(u16 pm1a_port, u16 pm1b_port)
{
    acpi_t *ap = &acpi_gbl;

    acpi_pm1_unarm();
    ap->pm1_ctl[0] = pm1a_port;
    ap->pm1_ctl[1] = pm1b_port;
    if(ap->pm1_ctl[0]) {
        io_bitmap_set(ap->pm1_ctl[0]);
    }
    if(ap->pm1_ctl[1]) {
        io_bitmap_set(ap->pm1_ctl[1]);
    }
}

// hypercall handler;
// cannot use any acpica function;
// set pointer to os1 fadt and map;
// this will be used until the fastswitch trigger,
// after which we reinitialize acpi locally;

void
acpi_map_fadt(u32 phys_facs, u32 phys_fadt)
{
    acpi_t *ap = &acpi_gbl;
    ACPI_TABLE_FADT *fadt;

    // map new table before accesses;
    // fadt=0 unarms io traping;

    if( !phys_fadt) {
        acpi_unarm();
        return;
    }

	// must copy fadt, because windows may point to a copy
	// in its memory, not directly to the e820 acpi region;
	// but memswap could swap that memory away;
	// the copy should be referenced after the fastswitch point,
	// never a new map_page from fadt_phys;

    ap->fadt_phys = phys_fadt;
    ap->fadt_size = sizeof(ACPI_TABLE_FADT);
    fadt = AcpiOsMapMemory(ap->fadt_phys, ap->fadt_size);
    if( !fadt) {
        return;
    }
	memcpy(&ap->fadt, fadt, ap->fadt_size);

    // get all the fadt info needed for fastswitch trigger;
    // make sure we have a valid fadt here;

    if( !ACPI_COMPARE_NAME(fadt->Header.Signature, ACPI_SIG_FADT)) {
        goto done;
    }
    //XXX checksum

    // got a valid fadt;
    // initialize pm1[ab]_ctl ports;

    acpi_pm1_ctl(fadt->Pm1aControlBlock, fadt->Pm1bControlBlock);
    dbg(("acpi: FADT=0x%x, pm1_ctl=%x/%x\n",
        phys_fadt, fadt->Pm1aControlBlock, fadt->Pm1bControlBlock));

	// unmap fadt;
done:
    AcpiOsUnmapMemory(fadt, ap->fadt_size);
}

// copy facs;
// by element as this could be memory mapped io;

static void
acpi_copy_facs(ACPI_TABLE_FACS *d, ACPI_TABLE_FACS *s)
{
    d->Length = s->Length;
    d->HardwareSignature = s->HardwareSignature;
    d->FirmwareWakingVector = s->FirmwareWakingVector;
    d->GlobalLock = s->GlobalLock;
    d->Flags = s->Flags;
    d->XFirmwareWakingVector = s->XFirmwareWakingVector;
    d->Version = s->Version;
    d->OspmFlags = s->OspmFlags;
    memcpy(d->Signature, s->Signature, sizeof(s->Signature));
    memzero(d->Reserved, sizeof(d->Reserved));
    memzero(d->Reserved1, sizeof(d->Reserved1));
}

// save fadt state;

static void
acpi_save_fadt(ACPI_TABLE_FADT *fadt, acpi_state_t *s)
{
    u32 len;
    u16 port;

    // save wake events;

    s->pm1_ena[0] = s->pm1_ena[1] = 0;
    len = fadt->Pm1EventLength / 2;
    if(len == 2) {
        port = fadt->Pm1aEventBlock;
        s->pm1_ena[0] = inw(port + len);
        port = fadt->Pm1bEventBlock;
        s->pm1_ena[1] = inw(port + len);
    }
}

// restore fadt state;

static void
acpi_restore_fadt(ACPI_TABLE_FADT *fadt, acpi_state_t *s)
{
    u32 len;
    u16 port;

    len = fadt->Pm1EventLength / 2;
    if(len == 2) {
        port = fadt->Pm1aEventBlock;
        if(port) {
            outw(port + len, s->pm1_ena[0]);
        }
        port = fadt->Pm1bEventBlock;
        if(port) {
            outw(port + len, s->pm1_ena[1]);
        }
    }
}

// temporarily save/restore acpi state;
// called before hv acpica is initialized,
// so it must do it's own mapping;

int
acpi_tmp_state(int save)
{
    acpi_t *ap = &acpi_gbl;
    acpi_state_t *s = &state_tmp;
    ACPI_TABLE_FADT *fadt = 0;
    ACPI_TABLE_FACS *facs = 0;
    int status = 0;

    if( !ap->fadt_phys) {
        goto done;
    }
	fadt = &ap->fadt;
    facs = AcpiOsMapMemory(fadt->Facs, sizeof(s->facs));
    if( !facs) {
        goto done;
    }
    if(save) {
        acpi_copy_facs(&s->facs, facs);
        acpi_save_fadt(fadt, s);
    } else {
        acpi_copy_facs(facs, &s->facs);
        acpi_restore_fadt(fadt, s);
    }
    status = 1;

    // unmap fadt/facs spaces;

done:
    if(facs) {
        AcpiOsUnmapMemory(facs, sizeof(s->facs));
    }
    return(status);
}

// save os1 acpi state;
// simply save the tmp state;

void
acpi_save_state(void)
{
    memcpy(&state_os1, &state_tmp, sizeof(state_tmp));
}

// restore os1 acpi state;

void
acpi_restore_state(void)
{
    acpi_state_t *s = &state_os1;
    ACPI_TABLE_FADT *fadt = &AcpiGbl_FADT;

    // restore facs;

    if(AcpiGbl_FACS) {
        acpi_copy_facs(AcpiGbl_FACS, &s->facs);
    }

    // restore wake events;

    if(fadt) {
        acpi_restore_fadt(fadt, s);
    }
}

// virtualize acpi registers;

int
acpi_virt_io(u16 port, u8 iotype, un *val)
{
    acpi_t *ap = &acpi_gbl;

    // pm1 regs should only be accessed by boot processor;

    if((port != ap->pm1_ctl[0]) && (port != ap->pm1_ctl[1])) {
        return(true);
    }
    if(cpuno() != 0) {
        return(true);
    }

    // try re-init console;

    dbg(("acpi: pm1 port %x iotype %d val %lx\n", port, iotype, *val));

    if(bit_test(iotype, 3)) {
        return(true);
    }

    // only fastswitch on state S3;
    // XXX implement check for S3;
    // XXX must run methods, but have no acpica yet, argh!

    // some BIOSes/HW are buggy and turn off on first write
    // without SLEEP_ENABLE bit being written;

    if( !bit_test(*val, ACPI_BITPOSITION_SLEEP_ENABLE)) {
        kprintf("acpi: sleep not enabled yet, ignoring.\n");
        return(false);
    }
    ap->pm1_val = *val;
    return(-1);
}

// hard power-off after fastswitch trap in case of failure;

void
acpi_hw_power_off(void)
{
    acpi_t *ap = &acpi_gbl;

    while(1) {
        outw(ap->pm1_ctl[0], ap->pm1_val);
        pause();
    }
}

// write pm1[ab]_ctl registers to sleep;

void
acpi_hw_sleep(void)
{
    ACPI_STATUS status;

    dbg(("%s: S3\n", __FUNCTION__));

    // even though AcpiEnterSleepState() flushes caches,
	// we still need to do it here for fail cases;

    wbinvd();

    status = AcpiEnterSleepStatePrep(ACPI_STATE_S3);
    if(ACPI_FAILURE(status)) {
        kprintf("AcpiEnterSleepStatePrep() failed: %x\n", status);
        return;
    }
    status = AcpiEnterSleepState(ACPI_STATE_S3);
    if(ACPI_FAILURE(status)) {
        kprintf("AcpiEnterSleepState() failed: %x\n", status);
    }
}

// wakeup after ghv sleep/resume;

void
acpi_hw_wakeup(void)
{
    ACPI_STATUS status;

    dbg(("%s: S3\n", __FUNCTION__));

    status = AcpiLeaveSleepState(ACPI_STATE_S3);
    if(ACPI_FAILURE(status)) {
        kprintf("AcpiLeaveSleepState() %x\n", status);
    }
}

// ec driver;

// ec read status;

static inline u8
acpi_ec_read_sts(acpi_ec_t *ec)
{
    u8 x = inb(ec->cmd);
    return(x);
}

// ec read data;

static inline u8
acpi_ec_read_data(acpi_ec_t *ec)
{
    u8 x = inb(ec->data);
    return(x);
}

// write command;

static inline void
acpi_ec_write_cmd(acpi_ec_t *ec, u8 cmd)
{
    outb(ec->cmd, cmd);
}

// write data;

static inline void
acpi_ec_write_data(acpi_ec_t *ec, u8 data)
{
    outb(ec->data, data);
}

// wait on a buffer flag;

static int
acpi_ec_x_spin(acpi_ec_t *ec, u32 to, u8 mask, u8 val)
{
    u8 sts;

    for(; to > 0; to--) {
        sts = acpi_ec_read_sts(ec);
        if((sts & mask) == val) {
            break;
        }
        udelay(10);
    }
    return(to);
}

// start an ec transaction;

static inline void
acpi_ec_x_start(acpi_ec_t *ec)
{
    acpi_ec_write_cmd(ec, ec->x.cmd);
}

// handle ec transaction streams;

static ACPI_STATUS
acpi_ec_x_stream(acpi_ec_t *ec, u32 to)
{
    u8 *s, len;

    // stream write data;

    s = ec->x.w;
    for(len = *s++; len > 0; len--) {
        to = acpi_ec_x_spin(ec, to, ACPI_EC_STS_IBF, 0);
        if( !to) {
            return(AE_TIME);
        }
        acpi_ec_write_data(ec, *s++);
    }

    // stream read data;

    s = ec->x.r;
    for(len = *s++; len > 0; len--) {
        to = acpi_ec_x_spin(ec, to, ACPI_EC_STS_OBF, ACPI_EC_STS_OBF);
        if( !to) {
            return(AE_TIME);
        }
        *s++ = acpi_ec_read_data(ec);
    }

    return(AE_OK);
}

// ec transaction;

static ACPI_STATUS
ec_x(acpi_ec_t *ec)
{
    ACPI_STATUS status;
    u32 to;

    to = ACPI_EC_TO_X * 1000/10;
    to = acpi_ec_x_spin(ec, to, ACPI_EC_STS_IBF, 0);
    if( !to) {
        kprintf("acpi: ec input buffer full\n");
        return(AE_TIME);
    }

    acpi_ec_x_start(ec);
    status = acpi_ec_x_stream(ec, to);
    dbg(("acpi: ec cmd %02x, status %x\n", ec->x.cmd, status));
    return(status);
}

// enable burst;

static ACPI_STATUS
acpi_ec_burst(acpi_ec_t *ec, int enable)
{
    ACPI_STATUS status;
    u8 sts;

    ec->x.w[0] = 0;
    if(enable) {
        ec->x.cmd = ACPI_EC_BURST_ENABLE;
        ec->x.r[0] = 1;
    } else {
        ec->x.cmd = ACPI_EC_BURST_DISABLE;
        ec->x.r[0] = 0;
        sts = acpi_ec_read_sts(ec);
        if( !(sts & ACPI_EC_STS_BURST)) {
            return(AE_OK);
        }
    }
    status = ec_x(ec);
    return(status);
}

// ec read;

static ACPI_STATUS
acpi_ec_read(acpi_ec_t *ec, UINT8 addr, UINT8 *val)
{
    ACPI_STATUS status;

    ec->x.cmd = ACPI_EC_COMMAND_READ;
    ec->x.w[0] = 1;
    ec->x.w[1] = addr;
    ec->x.r[0] = 1;
    status = ec_x(ec);
    *val = ec->x.r[1];
    return(status);
}

// ec write;

static ACPI_STATUS
acpi_ec_write(acpi_ec_t *ec, UINT8 addr, UINT8 *val)
{
    ec->x.cmd = ACPI_EC_COMMAND_WRITE;
    ec->x.w[0] = 2;
    ec->x.w[1] = addr;
    ec->x.w[2] = *val;
    ec->x.r[0] = 0;
    return(ec_x(ec));
}

// ec address space handler;

static ACPI_STATUS
acpi_ec_as_handler(UINT32 func, ACPI_PHYSICAL_ADDRESS addr,
    UINT32 width, UINT64 *val, void *ctx, void *region)
{
    acpi_ec_t *ec = ctx;
    UINT8 *p = (UINT8 *)val;
    UINT32 n;
    ACPI_STATUS status = AE_BAD_PARAMETER;
    ACPI_STATUS (*rw)(acpi_ec_t *ec, u8 addr, UINT8 *val);

    dbg(("%s: func=%d addr=%lx/%d\n", __FUNCTION__, func, addr, width));
    if((addr > 0xff) || !val || !ctx) {
        return(AE_BAD_PARAMETER);
    }
    switch(func) {
    case ACPI_READ:
        rw = acpi_ec_read;
        break;
    case ACPI_WRITE:
        rw = acpi_ec_write;
        break;
    default:
        return(AE_BAD_PARAMETER);
    }
    if(width > 8) {
        status = acpi_ec_burst(ec, 1);
        if(ACPI_FAILURE(status)) {
            return(status);
        }
    }
    for(n = 0; n < width/8; n++, addr++, p++) {
        status = rw(ec, addr, p);
        if(ACPI_FAILURE(status)) {
            break;
        }
    }
    if(width > 8) {
        status = acpi_ec_burst(ec, 0);
    }
    return(status);
}

// parse ec io ports;

static ACPI_STATUS
acpi_ec_ports(ACPI_RESOURCE *res, void *ctx)
{
    acpi_ec_t *ec = ctx;

    if(res->Type != ACPI_RESOURCE_TYPE_IO) {
        return(AE_OK);
    }

    // first addr region is data port;
    // second addr region is cmd port;

    if(ec->data == 0) {
        ec->data = res->Data.Io.Minimum;
    } else if(ec->cmd == 0) {
        ec->cmd = res->Data.Io.Minimum;
    } else {
        return(AE_CTRL_TERMINATE);
    }
    dbg(("%s: data=%lx cmd=%lx\n", __FUNCTION__, ec->data, ec->cmd));
    return(AE_OK);
}

// parse ec device;

static ACPI_STATUS
acpi_ec_device(ACPI_HANDLE handle, UINT32 level, void *ctx, void **ret)
{
    acpi_ec_t *ec = ctx;
    ACPI_STATUS status;

    dbg(("acpi: found ec\n"));
    ec->cmd = ec->data = 0;

    // get io ports;

    status = AcpiWalkResources(handle, METHOD_NAME__CRS, acpi_ec_ports, ctx);
    if(ACPI_FAILURE(status)) {
        return(status);
    }

    // no support for GPEs;
    // no locks needed for fastswitch acpi;

    ec->gpe = 0;

    ec->handle = handle;
    return(AE_CTRL_TERMINATE);
}

// probe ecdt;
// does not deal with very broken EC implementations;

static char *acpi_ec_pnpid = "PNP0C09";

static ACPI_STATUS
acpi_ec_probe(void)
{
    acpi_t *ap = &acpi_gbl;
    ACPI_STATUS status;
    ACPI_TABLE_HEADER *table;
    ACPI_TABLE_ECDT *ecdt;

    // get ecdt;

    memzero(&ap->ec, sizeof(acpi_ec_t));

    status = AcpiGetTable(ACPI_SIG_ECDT, 0, &table);
    if(ACPI_SUCCESS(status) && table) {
        ecdt = (ACPI_TABLE_ECDT *)table;
        ap->ec.cmd = ecdt->Control.Address;
        ap->ec.data = ecdt->Data.Address;
        ap->ec.gpe = ecdt->Gpe;

        ap->ec.handle = ACPI_ROOT_OBJECT;
        AcpiGetHandle(ACPI_ROOT_OBJECT, (ACPI_STRING)ecdt->Id, &ap->ec.handle);
    }

    // some broken machines require EC, but have no ECDT;
    // lookup EC in DSDT;

    status = AcpiGetDevices(acpi_ec_pnpid, acpi_ec_device, &ap->ec, NULL);
    if(ACPI_FAILURE(status)) {
        kprintf("acpi: ec probe: %x\n", status);
        ap->ec.handle = 0;
    }
    if( !ap->ec.handle) {
        return(AE_OK);
    }

    // install ec handlers;
    // we don't handle GPEs, so install only addr space handlers;

    if( !ap->ec.as_handler) {
        status = AcpiInstallAddressSpaceHandler(ap->ec.handle,
            ACPI_ADR_SPACE_EC, &acpi_ec_as_handler, NULL, &ap->ec);
        if(ACPI_FAILURE(status)) {
            return(status);
        }
        ap->ec.as_handler = 1;
    }

    status = AE_OK;
    return(status);
}

// get processor power info via acpi _CST;

static ACPI_STATUS
acpi_processor_pinfo_cst(ACPI_HANDLE dev, char *name)
{
    ACPI_STATUS status;
    ACPI_BUFFER buf;
    ACPI_OBJECT *cst, *el, *obj;
    int i, nps, cx;
    ACPI_POWER_REGISTER *apr;
    char *sid;

    // get power info from _CST;

    buf.Pointer = 0;
    buf.Length = ACPI_ALLOCATE_BUFFER;
    status = AcpiEvaluateObjectTyped(dev, "_CST", NULL, &buf, ACPI_TYPE_PACKAGE);
    if(ACPI_FAILURE(status)) {
        dbg(("acpi: %4.4s _CST not found: %x\n", name, status));
        goto done;
    }

    // check cst package;

    status = AE_BAD_PARAMETER;
    cst = buf.Pointer;
    if((cst == NULL) || (cst->Package.Count < 2)) {
        dbg(("acpi: %4.4s _CST not enough elements\n", name));
        goto done;
    }

    // check number of power states;

    nps = cst->Package.Elements[0].Integer.Value;
    if((nps < 1) || (nps > 8) || (nps != cst->Package.Count - 1)) {
        dbg(("acpi: %4.4s _CST invalid count %d\n", name, nps));
        goto done;
    }

    for(i = 1; i <= nps; i++) {
        el = &(cst->Package.Elements[i]);
        if(el->Type != ACPI_TYPE_PACKAGE)
            continue;
        if(el->Package.Count != 4)
            continue;

        obj = &(el->Package.Elements[0]);
        if(obj->Type != ACPI_TYPE_BUFFER)
            continue;
        apr = (ACPI_POWER_REGISTER *)obj->Buffer.Pointer;

        obj = &(el->Package.Elements[1]);
        if(obj->Type != ACPI_TYPE_INTEGER)
            continue;

        switch(apr->reg.SpaceId) {
        case ACPI_ADR_SPACE_SYSTEM_IO:
            sid = "io";
            break;
        case ACPI_ADR_SPACE_FIXED_HARDWARE:
            sid = "ffh";
            break;
        default:
            sid = "???";
        }
        cx = obj->Integer.Value;
        dbg(("acpi: %4.4s %d power states: C%d %s 0x%lx\n", name, nps,
            cx, sid, apr->reg.Address));
    }

    status = AE_OK;
    
done:
    if(buf.Pointer)
        ACPI_FREE(buf.Pointer);
    return(status);
}

static ACPI_STATUS
acpi_processor(ACPI_HANDLE dev, ACPI_DEVICE_INFO *info)
{
    ACPI_STATUS status;

    status = acpi_processor_pinfo_cst(dev, (char *)&info->Name);


    return(status);
}

// acpi process one type;

static ACPI_STATUS
acpi_process_type(ACPI_HANDLE dev, UINT32 level, void *ctx, void **ret)
{   
    acpi_t *ap = &acpi_gbl;
    ACPI_DEVICE_INFO *info;
    ACPI_STATUS status;
    ACPI_STATUS (*hdl)(ACPI_HANDLE dev, ACPI_DEVICE_INFO *info);

    // get device info;
        
    status = AcpiGetObjectInfo(dev, &info);
    if(ACPI_FAILURE(status)) {
        dbg(("cannot get dev %p\n", dev));
        return(status);
    }   

    switch(info->Type) {
    case ACPI_TYPE_PROCESSOR:
        ap->cpu.count++;
        hdl = acpi_processor;
        break;
    case ACPI_TYPE_POWER:
    case ACPI_TYPE_THERMAL:
        hdl = 0;
        break;
    default:
        goto done;
    }

    dbg(("%*s f%02x v%02x type %x %4.4s HID %s ADR %8.8llx, STA: %x\n", level, "",
        info->Flags, info->Valid, info->Type, (char *)&info->Name,
        info->HardwareId.String, (u64)info->Address, info->CurrentStatus));

    if(hdl) {
        status = hdl(dev, info);
    }

done:
    ACPI_FREE(info);
    return(AE_OK);
}

// acpi process one device;

static ACPI_STATUS
acpi_process_device(ACPI_HANDLE dev, UINT32 level, void *ctx, void **ret)
{
    ACPI_DEVICE_INFO *info;
    ACPI_STATUS status;
    UINT32 mask;
    ACPI_BUFFER buf;
    ACPI_RESOURCE *res;

    // get device info;

    status = AcpiGetObjectInfo(dev, &info);
    if(ACPI_FAILURE(status)) {
        dbg(("cannot get dev %p\n", dev));
        return(status);
    }
    buf.Pointer = 0;
    dbg(("%*s f%02x v%02x %4.4s HID %s ADR %8.8llx, STA: %x\n", level, "",
        info->Flags, info->Valid, (char *)&info->Name,
        info->HardwareId.String, (u64)info->Address, info->CurrentStatus));

    // check if device is enabled and functioning;

    if( !(info->Valid & ACPI_VALID_STA)) {
        goto done;
    }
    mask = ACPI_STA_DEVICE_ENABLED | ACPI_STA_DEVICE_FUNCTIONING;
    if((info->CurrentStatus & mask) == mask) {
        goto done;
    }

    // get current ressource setting via _CRS;

    buf.Length = ACPI_ALLOCATE_BUFFER;
    status = AcpiGetCurrentResources(dev, &buf);
    if(ACPI_FAILURE(status)) {
        dbg(("AcpiGetCurrentResources() failed: %x\n", status));
        goto done;
    }

    // enable resource, if it is an IRQ, ie. a link device;

    res = ACPI_CAST_PTR(ACPI_RESOURCE, buf.Pointer);
    if(res->Type == ACPI_RESOURCE_TYPE_IRQ) {
        status = AcpiSetCurrentResources(dev, &buf);
        dbg(("AcpiSetCurrentResources(): %x\n", status));
    }

done:
    if(buf.Pointer)
        ACPI_FREE(buf.Pointer);

    // remember all root bridges;
    // evaluate _REG and _BBN;
    // assume 0 as default seg and bus;

    if(info->Flags & ACPI_PCI_ROOT_BRIDGE) {
        ACPI_OBJECT obj;
        pci_id_t id = { 0 };

        buf.Pointer = &obj;
        buf.Length = sizeof(ACPI_OBJECT);

        status = AcpiEvaluateObjectTyped(dev, METHOD_NAME__REG, NULL, &buf, ACPI_TYPE_INTEGER);
        if(ACPI_SUCCESS(status) || (status == AE_NOT_FOUND)) {
            if(obj.Type == ACPI_TYPE_INTEGER) {
                id.seg = obj.Integer.Value;
            }
        }

        status = AcpiEvaluateObjectTyped(dev, METHOD_NAME__BBN, NULL, &buf, ACPI_TYPE_INTEGER);
        if(ACPI_SUCCESS(status) || (status == AE_NOT_FOUND)) {
            if(obj.Type == ACPI_TYPE_INTEGER) {
                id.bus = obj.Integer.Value;
            }
        }
        pci_root_add(id);
    }

    ACPI_FREE(info);
    return(AE_OK);
}

// walk the namespace to find all processors;
// this determines max # of cpus supported, not enabled cpus;

static int
acpi_find_processors(acpi_t *ap)
{
    ACPI_STATUS status;
    ACPI_TABLE_HEADER *table;

    ap->cpu.count = 0;
    ap->cpu.mask = 0;

    status = AcpiWalkNamespace(ACPI_TYPE_PROCESSOR, ACPI_ROOT_OBJECT,
                ACPI_UINT32_MAX, acpi_process_type, NULL, NULL, NULL);

    if(ap->cpu.count > NR_CPUS) {
        kprintf("acpi: too many processors %d (max %d)\n", ap->cpu.count, NR_CPUS);
        return(0);
    }

    // determine cpu logical ids and enable flags;
    // get cpu masks from APIC table;
    // update cpu.count based on enable # of processors;

    status = AcpiGetTable(ACPI_SIG_MADT, 0, &table);
    if(ACPI_SUCCESS(status) && table) {
        UINT8 *madt = ACPI_CAST_PTR(UINT8, table);
        UINT8 *madt_end = madt + table->Length;
        ACPI_SUBTABLE_HEADER *sub_table;
        ACPI_MADT_LOCAL_APIC *lapic_entry;
        int ncpu, fail = 0;
 
        madt += sizeof(ACPI_TABLE_MADT);
        for(ncpu = 0; madt < madt_end; madt += sub_table->Length) {
            sub_table = ACPI_CAST_PTR(ACPI_SUBTABLE_HEADER, madt);
            if(sub_table->Type != ACPI_MADT_TYPE_LOCAL_APIC) {
                continue;
            }
            lapic_entry = ACPI_CAST_PTR(ACPI_MADT_LOCAL_APIC, sub_table);
            if( !(lapic_entry->LapicFlags & 1)) {
                continue;
            }
            if(lapic_entry->ProcessorId > ap->cpu.count) {
                kprintf("acpi: apic processor id %d > %d max\n",
                    lapic_entry->ProcessorId, ap->cpu.count);
                fail++;
                continue;
            }
            if(lapic_entry->Id >= NR_CPUS) {
                kprintf("acpi: cpu %d apic id %d > %d max\n",
                    lapic_entry->ProcessorId, lapic_entry->Id, NR_CPUS);
                fail++;
                continue;
            }
            if(ap->cpu.mask & (1 << lapic_entry->Id)) {
                kprintf("acpi: cpu %d apic id %d duplicate\n",
                    lapic_entry->ProcessorId, lapic_entry->Id);
                fail++;
                continue;
            }
            ap->cpu.mask |= (1 << lapic_entry->Id);
            ncpu++;
        }
        if(fail) {
            kprintf("acpi: APIC %d disagrees with DSDT %d\n", ncpu, ap->cpu.count);
            return(0);
        }
        ap->cpu.count = ncpu;
    } else {
        kprintf("acpi: missing APIC table\n");
        return(0);
    }
    kprintf("acpi found %d processors, mask 0x%x\n", ap->cpu.count, ap->cpu.mask);
    return(1);
}

// initialize acpi system;
// cannot be done before fastswitch trigger, as os1 acpi is still
// running, and windows will bsod when hypervisor takes too long
// in intr disabled mode during acpi initialization, as well as
// possible locks in method codes;

static bool
acpi_init(int trap)
{
    acpi_t *ap = &acpi_gbl;
    ACPI_STATUS status;
    UINT32 flags;
    UINT32 ss;
    UINT8 sa, sb;

    // reset debug flags;

    AcpiDbgLevel = ACPI_NORMAL_DEFAULT | ACPI_LV_INFO;
    AcpiDbgLevel |= ACPI_LV_EXEC;
    AcpiDbgLayer = 0xFFFFFFFF;

    AcpiGbl_AllMethodsSerialized = TRUE;
    AcpiGbl_GlobalLockPresent = FALSE;

    // find RSDP, only once;

    if( !ap->rsdp) {
        status = AcpiFindRootPointer(&ap->rsdp);
        if(ACPI_FAILURE(status)) {
            kprintf("acpi: RSDP not found: %x\n", status);
            return(0);
        }
    }
    
    // initialize acpi;

    status = AcpiInitializeSubsystem();
    if(ACPI_FAILURE(status)) {
        kprintf("AcpiInitializeSubsystem() failed: %x\n", status);
        return(0);
    }

    // initialize all tables;
    // overlay tables handed down from os1;

    status = AcpiInitializeTables(NULL, 0, 0);
    if(ACPI_FAILURE(status)) {
        kprintf("AcpiInitializeTables() failed: %x\n", status);
        return(0);
    }

    status = AcpiLoadTables();
    if(ACPI_FAILURE(status)) {
        kprintf("AcpiLoadTables() failed: %x\n", status);
        return(0);
    }

    // should have a valid fadt now;
    // enable acpi, which maps the facs;

    flags = 0;

    status = AcpiEnableSubsystem(flags);
    if(ACPI_FAILURE(status)) {
        kprintf("AcpiEnableSubsystem() failed: %x\n", status);
        return(0);
    }
    if( !AcpiGbl_FACS) {
        kprintf("acpi: no FACS!\n");
        return(0);
    }

    // EC driver must be loaded before EC device is in name space;
    // legal to not have an ECDT, so no return check;

    status = acpi_ec_probe();
    if(ACPI_FAILURE(status) && (status == AE_SUPPORT)) {
        kprintf("acpi: no EC support implemented\n");
        return(0);
    }

    // initialize objects;
    // should not initialize devices, as we need to do a _WAK;

    flags = ACPI_NO_EVENT_INIT;
    if(trap) {
        flags |= ACPI_NO_DEVICE_INIT;
    }
    status = AcpiInitializeObjects(flags);
    if(ACPI_FAILURE(status)) {
        kprintf("AcpiEnableSubsystem() failed: %x\n", status);
        return(0);
    }

    // get sleep type data;
    // must get all to match against pm1_ctl to find sleep state;

    for(ss = ACPI_STATE_S0; ss < ACPI_S_STATE_COUNT; ss++) {
        status = AcpiGetSleepTypeData(ss, &sa, &sb);
        if(ACPI_FAILURE(status)) {
            sa = sb = 0xff;
        }
        ap->pm1_ss[ss] = sa | sb;
		dbg(("sleep state %x: S%x - %x %x\n", status, ss, sa, sb));
    }

    // get current sleep type;
    // leave that sleep state if not S0;

    if(trap) {
        ss = ap->pm1_val;
    } else {
        status = AcpiHwRegisterRead(ACPI_REGISTER_PM1_CONTROL, &ss);
        if(ACPI_FAILURE(status)) {
            kprintf("cannot read pm1_ctl: %x, assuming S0\n", status);
            ss = 0;
        }
    }
	dbg(("%s: trap %d pm1_ctl %04x\n", __FUNCTION__, trap, ss));
    sa = (ss & ACPI_BITMASK_SLEEP_TYPE) >> ACPI_BITPOSITION_SLEEP_TYPE;
    for(ss = ACPI_STATE_S1; ss < ACPI_S_STATE_COUNT; ss++) {
        if(sa == ap->pm1_ss[ss]) {
            dbg(("leaving sleep state S%x\n", ss));
            status = AcpiLeaveSleepState(ss);
            if(ACPI_FAILURE(status)) {
                kprintf("AcpiLeaveSleepState() %x\n", status);
            }
            break;
        }
    }

    // init all devices;

    status = AcpiNsInitializeDevices();
    kprintf("acpi_init_devices: %x\n", status);

    // find all processors;

    acpi_find_processors(ap);

    // walk the namespace for all devices;
    // process each device, remember root bridges;

    status = AcpiWalkNamespace(ACPI_TYPE_DEVICE, ACPI_ROOT_OBJECT,
                ACPI_UINT32_MAX, acpi_process_device, NULL, NULL, NULL);
    kprintf("acpi_process_devices: %x\n", status);

    return(1);
}

// check if acpi is ready for fastswitch;
// this is a safe point to take over acpi;

bool
acpi_fastswitch(int trap)
{
    bool status;

    // take over acpi;
    // if it fails, restore facs, so we can go sleep;

    status = acpi_init(trap);
    if( !status) {
        acpi_shutdown();
        acpi_tmp_state(0);
        cmos_tmp_state(0);
    }
    return(status);
}

// shutdown down acpi;

void
acpi_shutdown(void)
{
    AcpiTerminate();
}

// force immediate wakeup;
// acpi_save_state() and acpi_restore_state() take care of os1 state,
// so it is safe to destroy acpi hardware state here;

void
acpi_force_wakeup(void)
{
    ACPI_TABLE_FADT *fadt = &AcpiGbl_FADT;
    u32 port;
    u16 v;
    int n;

    // basic checks;

    if( !fadt) {
        kprintf("acpi: no fadt for forced wakeup!\n");
        return;
    }
    if(fadt->Pm1EventLength != 4) {
        kprintf("acpi: pm1 length %d not for io\n", fadt->Pm1EventLength);
        return;
    }

    // read reg 0x0d to check if rtc is good to use;
    // try to use it anyway, hoping that a cap on the board
    // has enough charge for the rtc to live for another second;

    CMOS_GET(0x0d, v);
    if( !(v & CMOS_BATTERY_GOOD)) {
        kprintf("CMOS battery bad or not existent!\n");
    }

    // use rtc periodic 500msec interrupt to wake us;
    // os1 settings are already saved;

    CMOS_GET(0x0a, v);
    CMOS_SET(0x0a, v | CMOS_PERIODIC_INTR_500MS);
    CMOS_GET(0x0b, v);
    CMOS_SET(0x0b, v | CMOS_PERIODIC_INTR_MASK);

    // check if interrupt really ticks;

    CMOS_GET(0x0c, v);            // read reg 0x0c clears pending interrupt;
    for(n = 0; n < 6; n++) {
        CMOS_GET(0x0c, v);        // should have pending interrupt;
        if(v & CMOS_PERIODIC_INTR_MASK) {
            break;                // got periodic interrupt;
        }
        mdelay(100);
    }
    if( !(v & CMOS_PERIODIC_INTR_MASK)) {
        kprintf("CMOS periodic intr did not trigger!\n");
    }

    // enable acpi rtc wake event;

    port = fadt->Pm1aEventBlock;
    if(port) {
        port += fadt->Pm1EventLength / 2;
        v = inw(port);
        outw(port, v | (1 << 10));
    }
}

// unarm acpi trapping;

void
acpi_unarm(void)
{
    acpi_pm1_unarm();
}

/*
 * acpica interfaces;
 */

/*
 * os specific initialization/termination;
 * callback from AcpiInitialize();
 */
ACPI_STATUS
AcpiOsInitialize(void)
{
	dbgos((" ## %s\n", __FUNCTION__));
    return(AE_OK);
}

ACPI_STATUS
AcpiOsTerminate(void)
{
	dbgos((" ## %s\n", __FUNCTION__));
    return(AE_OK);
}

ACPI_STATUS
AcpiOsValidateInterface(char *iface)
{
    return(AE_SUPPORT);
}

/*
 * acpica printf;
 * only have console output in ghv/hyp;
 */
void
AcpiOsVprintf(const char *fmt, va_list args)
{
    UINT8 flags;

    flags = AcpiGbl_DbOutputFlags | ACPI_DB_CONSOLE_OUTPUT;
    if(flags & ACPI_DB_REDIRECTABLE_OUTPUT) {
        flags |= ACPI_DB_CONSOLE_OUTPUT;
    }
    if(flags & ACPI_DB_CONSOLE_OUTPUT) {
        kvaprintf(FLAG_PRINT | FLAG_NOCPU, fmt, args);
    }
}

void ACPI_INTERNAL_VAR_XFACE
AcpiOsPrintf(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    AcpiOsVprintf(fmt, args);
    va_end(args);
}

/*
 * acpi map/unmap physical memory;
 */
void *
AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS phys, ACPI_SIZE length)
{
	addr_t ea, va;
    un npages;

    ea = phys + length;
    npages = (ea >> VM_PAGE_SHIFT) - (phys >> VM_PAGE_SHIFT) + 1;
    va = map_pages(TRUNC_PAGE(phys), npages, MPF_IO);
    va |= (phys & VM_PAGE_OFFSET);
    dbgos(("%s: %x/%p phys %llx\n", __FUNCTION__, (u32)length, (void *)va, (u64)phys));
	return((void *)va);
}

void
AcpiOsUnmapMemory(void *virt, ACPI_SIZE length)       
{
    addr_t ea, va;
    un npages;

	dbgos(("%s: %x/%p\n", __FUNCTION__, (u32)length, virt));
    va = (addr_t)virt;
    ea = va + length;
    npages = (ea >> VM_PAGE_SHIFT) - (va >> VM_PAGE_SHIFT) + 1;
    unmap_pages(TRUNC_PAGE(va), npages, UMPF_LOCAL_FLUSH);
}

/*
 * memory allocate/free
 */
void *
AcpiOsAllocate(ACPI_SIZE size)
{
    return(malloc(size));
}

void
AcpiOsFree(void *mem)
{
    free(mem);
}

/*
 * sleep functions;
 */
void
AcpiOsStall(UINT32 usec)
{
	udelay(usec);
}

void
AcpiOsSleep(UINT64 msec)
{
	mdelay(msec);
}

UINT64
AcpiOsGetTimer(void)
{
    UINT64 t100ns;
    extern cpuinfo_t cpuinfo;

    t100ns = rdtsc() / (cpuinfo.cpu_tsc_freq_MHz / 10);
    return(t100ns);
}

/*
 * access to memory;
 */
ACPI_STATUS
AcpiOsReadMemory(ACPI_PHYSICAL_ADDRESS phys, UINT32 *val, UINT32 width)
{
    void *va;
    ACPI_STATUS status;

	dbgos((" ## %s %llx/%d\n", __FUNCTION__, (u64)phys, width));
	va = AcpiOsMapMemory(phys, 4);
    if( !va) {
        return(AE_BAD_PARAMETER);
    }
    status = AE_OK;
    switch(width) {
    case 8:
        *val = *(UINT8 *)va;
        break;
    case 16:
        *val = *(UINT16 *)va;
        break;
    case 32:
        *val = *(UINT32 *)va;
        break;
    default:
    	status = AE_BAD_PARAMETER;
    }
    AcpiOsUnmapMemory(va, 4);
    return(status);
}

ACPI_STATUS
AcpiOsWriteMemory(ACPI_PHYSICAL_ADDRESS phys, UINT32 val, UINT32 width)
{
    void *va;
    ACPI_STATUS status;

	dbgos((" ## %s %llx/%d\n", __FUNCTION__, (u64)phys, width));
	va = AcpiOsMapMemory(phys, 4);
    if( !va) {
        return(AE_BAD_PARAMETER);
    }
    status = AE_OK;
    switch(width) {
    case 8:
        *(UINT8 *)va = val;
        break;
    case 16:
        *(UINT16 *)va = val;
        break;
    case 32:
        *(UINT32 *)va = val;
        break;
    default:
    	status = AE_BAD_PARAMETER;
    }
    AcpiOsUnmapMemory(va, 4);
    return(status);
}

/*
 * access to pci config space;
 */
static inline pci_id_t
acpi_to_pci_id(ACPI_PCI_ID *id)
{
    pci_id_t x;

    x.seg = id->Segment;
    x.bus = id->Bus;
    x.dev = id->Device;
    x.func = id->Function;
    return(x);
}

ACPI_STATUS
AcpiOsReadPciConfiguration(ACPI_PCI_ID *id, UINT32 reg, void *val, UINT32 width)
{
	dbgos((" ## %s %04x:%02x:%02x.%x reg %x/%d\n", __FUNCTION__,
        id->Segment, id->Bus, id->Device, id->Function, reg, width));

    if(pci_raw_read(acpi_to_pci_id(id), reg, width, val)) {
        return(AE_OK);
    }
    return(AE_BAD_PARAMETER);
}

ACPI_STATUS
AcpiOsWritePciConfiguration(ACPI_PCI_ID *id, UINT32 reg, UINT64 val, UINT32 width)
{
	dbgos((" ## %s %04x:%02x:%02x.%x reg %x/%d\n", __FUNCTION__,
        id->Segment, id->Bus, id->Device, id->Function, reg, width));

    if(pci_raw_write(acpi_to_pci_id(id), reg, width, &val)) {
        return(AE_OK);
    }
    return(AE_BAD_PARAMETER);
}

void
AcpiOsDerivePciId(ACPI_HANDLE dev, ACPI_HANDLE region, ACPI_PCI_ID **id)
{
	dbgos((" ## %s\n", __FUNCTION__));
}

/*
 * access to io space;
 */
ACPI_STATUS
AcpiOsReadPort(ACPI_IO_ADDRESS port, UINT32 *val, UINT32 width)
{
    switch(width) {
    case 8:
        *val = inb(port);
        break;
    case 16:
        *val = inw(port);
        break;
    case 32:
        *val = inl(port);
        break;
    default:
        return(AE_BAD_PARAMETER);
    }
	dbgos((" ## %s %x/%d=%x\n", __FUNCTION__, (u32)port, width, *val));
    return(AE_OK);
}

ACPI_STATUS
AcpiOsWritePort(ACPI_IO_ADDRESS port, UINT32 val, UINT32 width)
{
	dbgos((" ## %s %x/%d=%x\n", __FUNCTION__, (u32)port, width, val));
    switch(width) {
    case 8:
        outb(port, val);
        break;
    case 16:
        outw(port, val);
        break;
    case 32:
        outl(port, val);
        break;
    default:
        return(AE_BAD_PARAMETER);
    }
    return(AE_OK);
}

/*
 * semaphore handling;
 */
ACPI_STATUS
AcpiOsCreateSemaphore(UINT32 max, UINT32 initial, ACPI_HANDLE *handle)
{
    void *sem;

	dbgos((" ## %s\n", __FUNCTION__));
    if( !handle) {
        return(AE_BAD_PARAMETER);
    }
    sem = AcpiOsAllocate(sizeof(void *));
    if( !sem) {
        return(AE_NO_MEMORY);
    }
    *handle = sem;
    return(AE_OK);
}

ACPI_STATUS
AcpiOsDeleteSemaphore(ACPI_HANDLE handle)
{
    void *sem = (void *)handle;

	dbgos((" ## %s\n", __FUNCTION__));
    if( !sem) {
        return(AE_BAD_PARAMETER);
    }
    AcpiOsFree(sem);
    return (AE_OK);
}

ACPI_STATUS
AcpiOsWaitSemaphore(ACPI_HANDLE handle, UINT32 units, UINT16 timeout)
{
    void *sem = (void *)handle;

	if( !sem) {
        return(AE_BAD_PARAMETER);
    }
    return(AE_OK);
}

ACPI_STATUS
AcpiOsSignalSemaphore(ACPI_HANDLE handle, UINT32 units)
{
    void *sem = (void *)handle;

	if( !sem) {
        return(AE_BAD_PARAMETER);
    }
    return(AE_OK);
}

/*
 * lock interface;
 */
ACPI_STATUS
AcpiOsCreateLock(ACPI_SPINLOCK *handle)
{
	dbgos((" ## %s\n", __FUNCTION__));
    return(AcpiOsCreateSemaphore(1, 1, handle));
}

void
AcpiOsDeleteLock(ACPI_SPINLOCK handle)
{
	dbgos((" ## %s\n", __FUNCTION__));
    AcpiOsDeleteSemaphore(handle);
}


ACPI_CPU_FLAGS
AcpiOsAcquireLock(ACPI_HANDLE handle)
{
	dbgos((" ## %s\n", __FUNCTION__));
    AcpiOsWaitSemaphore(handle, 1, 0xFFFF);
    return(0);
}

void
AcpiOsReleaseLock(ACPI_SPINLOCK handle, ACPI_CPU_FLAGS flags)
{
	dbgos((" ## %s\n", __FUNCTION__));
    AcpiOsSignalSemaphore(handle, 1);
}

/*
 * sci interrupt handling;
 */
UINT32
AcpiOsInstallInterruptHandler(UINT32 intnum, ACPI_OSD_HANDLER hdl, void *ctx)
{
	dbgos((" ## %s\n", __FUNCTION__));
    return(AE_OK);         
}

ACPI_STATUS
AcpiOsRemoveInterruptHandler(UINT32 intnum, ACPI_OSD_HANDLER hdl)
{
	dbgos((" ## %s\n", __FUNCTION__));
    return(AE_OK);
}


/*
 * thread interface;
 */
ACPI_THREAD_ID
AcpiOsGetThreadId(void)
{
    return(1);
}

ACPI_STATUS
AcpiOsExecute(ACPI_EXECUTE_TYPE type, ACPI_OSD_EXEC_CALLBACK func, void *ctx)
{
    AcpiOsPrintf("%s: type %x, func %x, ctx %x\n", __FUNCTION__, type, func, ctx);
    return(AE_OK);
}

ACPI_STATUS
AcpiOsSignal(UINT32 func, void *info)
{
    AcpiOsPrintf("%s: func %x, info %x\n", __FUNCTION__, func, info);
    switch(func) {
    case ACPI_SIGNAL_FATAL:
        break;
    case ACPI_SIGNAL_BREAKPOINT:
        break;
    default:
        break;
    }
    return(AE_OK);
}

/*
 * root/table interface;
 */
ACPI_PHYSICAL_ADDRESS
AcpiOsGetRootPointer(void)
{
    return(acpi_gbl.rsdp);
}

ACPI_STATUS
AcpiOsTableOverride(ACPI_TABLE_HEADER *table, ACPI_TABLE_HEADER **new)
{
	dbgos((" ## %s %4.4s\n", __FUNCTION__, table->Signature));
    //XXX add windows overwrites;
    *new = 0;
    return(AE_NO_ACPI_TABLES);
}

/*
 * pre-defined value overwrites;
 */
ACPI_STATUS
AcpiOsPredefinedOverride(const ACPI_PREDEFINED_NAMES *init, ACPI_STRING *new)
{   
	dbgos((" ## %s\n", __FUNCTION__));
    if( !init || !new) {
        return (AE_BAD_PARAMETER);
    }
    *new = 0; //XXX no overwrite;
    return(AE_OK);
}

