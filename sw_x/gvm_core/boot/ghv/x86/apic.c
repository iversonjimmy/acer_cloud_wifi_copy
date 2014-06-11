#include "hyp.h"
#include "valloc.h"
#include "apic.h"

un apic_base;

ret_t
map_apic(void)
{
	if (bp->bp_flags & BPF_NO_APIC) {
		return 0;
	}
	if (apic_base) {
		kprintf("apic_base = 0x%lx\n", apic_base);
		return 0;
	}

	u64 apic_base_msr = get_MSR(MSR_IA32_APIC_BASE);
	u64 apic_base_phys = apic_base_msr & ~0xFFF;

	kprintf("apic_base = 0x%llx\n", apic_base_msr);
	kprintf("apic_base phys addr = 0x%llx\n", apic_base_phys);
	kprintf("apic_base op mode = %ld\n", bits(apic_base_msr, 11, 10));
	if (cpu_feature_supported(CPU_FEATURE_X2APIC)) {
		kprintf("CPU_FEATURE_X2APIC\n");
		u64 xapicid = get_MSR(MSR_IA32_EXT_XAPICID);
		kprintf("xapicid = 0x%llx\n", xapicid);
		if (bits(apic_base_msr, 11, 10) == 3) {
			kprintf("x2APIC mode support not implemented\n");
			return -1;
		}	
	}

	assert(bits(apic_base_msr, 11, 10) == 2); /* xAPIC mode */

	apic_base = map_page(apic_base_phys, MPF_IO);
	if (!apic_base) {
		kprintf("apic_map>map_io_registers() failed\n");
		return -ENOMEM;
	}

	kprintf("apic_base = 0x%lx\n", apic_base);

	kprintf("apic_ver = 0x%x\n", APIC_READ(APIC_VER));	
	u32 apic_id = APIC_READ(APIC_ID);
	kprintf("apic_id = 0x%x\n", apic_id);	
	u32 apic_ldr = APIC_READ(APIC_LDR);
	kprintf("apic_ldr = 0x%x\n", apic_ldr);	
	u32 apic_dfr = APIC_READ(APIC_DFR);
	kprintf("apic_dfr = 0x%x\n", apic_dfr);	

	return 0;
}

void
unmap_apic(void)
{
	if (!apic_base) {
		return;
	}

	unmap_page(apic_base, UMPF_NO_FLUSH);

	apic_base = 0;
}

ret_t
apic_dump(void)
{
	kprintf("apic_id = 0x%x\n", cpu_apic_id());
	if (cpuid_stem(0, NULL) >= 0xb) {
		kprintf("x2apic_id = 0x%x\n", cpu_x2apic_id());
	}
	kprintf("cpu_id = 0x%x\n", cpu_id());

	u32 apic_id = APIC_READ(APIC_ID);
	kprintf("apic_id = 0x%x\n", apic_id);	
	u32 apic_ldr = APIC_READ(APIC_LDR);
	kprintf("apic_ldr = 0x%x\n", apic_ldr);	
	u32 apic_dfr = APIC_READ(APIC_DFR);
	kprintf("apic_dfr = 0x%x\n", apic_dfr);	

	return 0;
}

/*
 * Argument points to a 256bit bitmap
 */
void
apic_get_ISR(bitmap_t *b)
{
	if (!apic_base) {
		bitmap_init(b, 256, BITMAP_ALL_ZEROES);
		return;
	}

	u32 *res = (u32 *)b;

	res[0] = APIC_READ(APIC_ISR);
	res[1] = APIC_READ(APIC_ISR1);
	res[2] = APIC_READ(APIC_ISR2);
	res[3] = APIC_READ(APIC_ISR3);
	res[4] = APIC_READ(APIC_ISR4);
	res[5] = APIC_READ(APIC_ISR5);
	res[6] = APIC_READ(APIC_ISR6);
	res[7] = APIC_READ(APIC_ISR7);
}

/*
 * Argument points to a 256bit bitmap
 */
void
apic_get_IRR(bitmap_t *b)
{	
	if (!apic_base) {
		bitmap_init(b, 256, BITMAP_ALL_ZEROES);
		return;
	}

	u32 *res = (u32 *)b;
	
	res[0] = APIC_READ(APIC_IRR);
	res[1] = APIC_READ(APIC_IRR1);
	res[2] = APIC_READ(APIC_IRR2);
	res[3] = APIC_READ(APIC_IRR3);
	res[4] = APIC_READ(APIC_IRR4);
	res[5] = APIC_READ(APIC_IRR5);
	res[6] = APIC_READ(APIC_IRR6);
	res[7] = APIC_READ(APIC_IRR7);
}

void
apic_send_IPI(u8 dest, u8 vector)
{
	if (!apic_base) {
		return;
	}
	u32 icr1 = (u32)dest << APIC_ICR1_DEST_SHIFT;
	u32 icr = ((u32)vector |
		   APIC_ICR_(DLV, FIXED) |
		   APIC_ICR_(MODE, PHYS) |
		   APIC_ICR_(LVL, ASSERT) |
		   APIC_ICR_(DEST, EXPLICIT));

	apic_wait_for_idle(apic_base);

	APIC_WRITE(APIC_ICR1, icr1);
	APIC_WRITE(APIC_ICR, icr);
}

void
__apic_send_self(un apic_base, u8 vector)
{
	if (!apic_base) {
		return;
	}
	u32 icr = ((u32)vector |
		   APIC_ICR_(DLV, FIXED) |
		   APIC_ICR_(MODE, PHYS) |
		   APIC_ICR_(LVL, ASSERT) |
		   APIC_ICR_(DEST, SELF));

	apic_wait_for_idle(apic_base);

	APIC_WRITE(APIC_ICR, icr);
}

void
apic_send_self(u8 vector)
{
	__apic_send_self(apic_base, vector);
}

void
apic_broadcast_INIT(void)
{
	if (!apic_base) {
		return;
	}
	u8 vector = 0;
	u32 icr = ((u32)vector |
		   APIC_ICR_(DLV, INIT) |
		   APIC_ICR_(MODE, PHYS) |
		   APIC_ICR_(LVL, ASSERT) |
		   APIC_ICR_(DEST, OTHERS));

	apic_wait_for_idle(apic_base);

	APIC_WRITE(APIC_ICR, icr);
}

void
apic_broadcast_SIPI(u8 vector)
{
	if (!apic_base) {
		return;
	}
	u32 icr = ((u32)vector |
		   APIC_ICR_(DLV, SIPI) |
		   APIC_ICR_(MODE, PHYS) |
		   APIC_ICR_(LVL, ASSERT) |
		   APIC_ICR_(DEST, OTHERS));

	apic_wait_for_idle(apic_base);

	APIC_WRITE(APIC_ICR, icr);
}

void
apic_send_INIT(u8 dest)
{
	if (!apic_base) {
		return;
	}
	u8 vector = 0;
	u32 icr1 = (u32)dest << APIC_ICR1_DEST_SHIFT;
	u32 icr = ((u32)vector |
		   APIC_ICR_(DLV, INIT) |
		   APIC_ICR_(MODE, PHYS) |
		   APIC_ICR_(LVL, ASSERT) |
		   APIC_ICR_(DEST, EXPLICIT));

	apic_wait_for_idle(apic_base);

	APIC_WRITE(APIC_ICR1, icr1);
	APIC_WRITE(APIC_ICR, icr);
}

void
apic_send_SIPI(u8 dest, u8 vector)
{
	if (!apic_base) {
		return;
	}
	u32 icr1 = (u32)dest << APIC_ICR1_DEST_SHIFT;
	u32 icr = ((u32)vector |
		   APIC_ICR_(DLV, SIPI) |
		   APIC_ICR_(MODE, PHYS) |
		   APIC_ICR_(LVL, ASSERT) |
		   APIC_ICR_(DEST, EXPLICIT));

	apic_wait_for_idle(apic_base);

	APIC_WRITE(APIC_ICR1, icr1);
	APIC_WRITE(APIC_ICR, icr);
}

void
apic_startup_cpu(u8 dest, u8 vector)
{
	apic_send_INIT(dest);
	mdelay(10);
	apic_send_SIPI(dest, vector);
	udelay(200);
	apic_send_SIPI(dest, vector);
	udelay(200);
}


void
apic_startup_all_cpus(u8 vector)
{
	apic_broadcast_INIT();
	mdelay(10);
	apic_broadcast_SIPI(vector);
	udelay(200);
	apic_broadcast_SIPI(vector);
	udelay(200);
}

u32
apic_get_TPR(void)
{
	if (!apic_base) {
		return 0;
	}

	return bits(APIC_READ(APIC_TPR), 7, 0);
}
