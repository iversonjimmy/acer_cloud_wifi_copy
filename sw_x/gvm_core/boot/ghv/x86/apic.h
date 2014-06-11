#ifndef __APIC_H__
#define __APIC_H__

/* Intel Sys Prog Guide Vol 3A 9.4 */

#define APIC_ID		0x0020
#define APIC_VER	0x0030
#define APIC_TPR	0x0080
#define APIC_APR	0x0090
#define APIC_PPR	0x00a0
#define APIC_EOI	0x00b0
#define APIC_LDR	0x00d0
#define APIC_DFR	0x00e0
#define APIC_SIV	0x00f0
#define		APIC_SIV_ENABLE_SHIFT	8
#define APIC_ISR	0x0100
#define APIC_ISR1	0x0110
#define APIC_ISR2	0x0120
#define APIC_ISR3	0x0130
#define APIC_ISR4	0x0140
#define APIC_ISR5	0x0150
#define APIC_ISR6	0x0160
#define APIC_ISR7	0x0170
#define APIC_TMR	0x0180
#define APIC_TMR1	0x0190
#define APIC_TMR2	0x01a0
#define APIC_TMR3	0x01b0
#define APIC_TMR4	0x01c0
#define APIC_TMR5	0x01d0
#define APIC_TMR6	0x01e0
#define APIC_TMR7	0x01f0
#define APIC_IRR	0x0200
#define APIC_IRR1	0x0210
#define APIC_IRR2	0x0220
#define APIC_IRR3	0x0230
#define APIC_IRR4	0x0240
#define APIC_IRR5	0x0250
#define APIC_IRR6	0x0260
#define APIC_IRR7	0x0270
#define APIC_ESR	0x0280
#define APIC_LVT_CMCI	0x02f0
#define APIC_ICR	0x0300
#define		APIC_ICR_DLV_SHIFT	8
#define			APIC_ICR_DLV_FIXED	0
#define			APIC_ICR_DLV_LOPRI	1
#define			APIC_ICR_DLV_SMI	2
#define			APIC_ICR_DLV_NMI	4
#define			APIC_ICR_DLV_INIT	5
#define			APIC_ICR_DLV_SIPI	6
#define		APIC_ICR_MODE_SHIFT	11
#define			APIC_ICR_MODE_PHYS	0
#define			APIC_ICR_MODE_LOG	1
#define		APIC_ICR_STATUS_SHIFT	12
#define			APIC_ICR_STATUS_BUSY	1
#define		APIC_ICR_LVL_SHIFT	14
#define			APIC_ICR_LVL_DEASSERT	0
#define			APIC_ICR_LVL_ASSERT	1
#define		APIC_ICR_TRIG_SHIFT	15
#define			APIC_ICR_TRIG_EDGE	0
#define			APIC_ICR_TRIG_LEVEL	1
#define		APIC_ICR_DEST_SHIFT	18
#define			APIC_ICR_DEST_EXPLICIT	0
#define			APIC_ICR_DEST_SELF	1
#define			APIC_ICR_DEST_ALL	2
#define			APIC_ICR_DEST_OTHERS	3
#define APIC_ICR_(a, b)	(APIC_ICR_ ## a ## _ ## b << APIC_ICR_ ## a ## _SHIFT)
#define APIC_ICR1	0x0310
#define		APIC_ICR1_DEST_SHIFT	24
#define APIC_LVT_TIMER	0x0320
#define APIC_LVT_THERM	0x0330
#define APIC_LVT_PERF	0x0340
#define APIC_LVT_LINT0	0x0350
#define APIC_LVT_LINT1	0x0360
#define APIC_LVT_ERR	0x0370
#define APIC_TMR_ICR	0x0380
#define APIC_TMR_CCR	0x0390
#define APIC_TMR_DIV	0x03e0

inline static un
get_apic_base_phys(void)
{
	return get_MSR(MSR_IA32_APIC_BASE) & ~0xFFFUL;
}

extern un apic_base;

#define APIC_READ(off)		volatile_read((u32 *)(apic_base + (off)))
#define APIC_WRITE(off, x)	volatile_write((u32 *)(apic_base + (off)), (x));

inline static void
apic_ack_irq(void)
{
	APIC_WRITE(APIC_EOI, 0);
}

inline static void
apic_wait_for_idle(un apic_base)
{
	u32 icr;
	do {
		icr = APIC_READ(APIC_ICR);
	} while (icr & APIC_ICR_(STATUS, BUSY));
}

inline static s32
interrupt_pri(s32 vec)
{
	return (vec >> 4);
}

inline static bool
apic_IRR_test(u8 vector)
{
	u32 offset = APIC_IRR + (bits((u32)vector, 7, 5) << 4);
	return bit_test(APIC_READ(offset), bits(vector, 4, 0));
}

inline static bool
apic_ISR_test(u8 vector)
{
	u32 offset = APIC_ISR + (bits((u32)vector, 7, 5) << 4);
	return bit_test(APIC_READ(offset), bits(vector, 4, 0));
}

extern void apic_send_IPI(u8 dest, u8 vector);
extern void __apic_send_self(un base, u8 vector);
extern void apic_send_self(u8 vector);
extern void apic_send_SIPI(u8 dest, u8 vector);
extern void apic_send_INIT(u8 dest);
extern void apic_broadcast_SIPI(u8 vector);
extern void apic_broadcast_INIT(void);
extern ret_t apic_dump(void);
extern ret_t map_apic(void);
extern void unmap_apic(void);

extern void apic_startup_cpu(u8 dest, u8 vector);
extern void apic_startup_all_cpus(u8 vector);

extern u32 apic_get_TPR(void);
extern void apic_get_ISR(bitmap_t *);
extern void apic_get_IRR(bitmap_t *);

#endif
