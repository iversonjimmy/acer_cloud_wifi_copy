#ifndef __DT_H__
#define __DT_H__

#define ISTACK_LEN		0x4000
#define IDT_LEN			256

#define GDT_LEN			16

#define GDT_INDEX_HYP_CS	1
#define GDT_INDEX_HYP_CS_32	2
#define GDT_INDEX_HYP_DS	3
#define GDT_INDEX_HYP_USER_CS	4
#define GDT_INDEX_HYP_USER_DS	5

#define GDT_INDEX_HYP_TSS	8
#define GDT_INDEX_HYP_LDT	0

#define HYP_CS			(GDT_INDEX_HYP_CS << 3)
#define HYP_CS_32		(GDT_INDEX_HYP_CS_32 << 3)
#define HYP_DS			(GDT_INDEX_HYP_DS << 3)
#define HYP_USER_CS		((GDT_INDEX_HYP_USER_CS << 3) | 3)
#define HYP_USER_DS		((GDT_INDEX_HYP_USER_DS << 3) | 3)
#define HYP_TSS			(GDT_INDEX_HYP_TSS << 3)
#define HYP_LDT			(GDT_INDEX_HYP_LDT << 3)

extern dtr_t hyp_idtr;
extern dtr_t hyp_tss[];
extern dtr_t hyp_gdt[];

inline static void
clear_tss_busy(seg_desc_t *gdt, u16 sel)
{
	u16 index = sel >> 3;
	seg_desc_t *s = &gdt[index];
	assert((s->seg_type == SYS_SEG_TSS_BUSY) ||
	       (s->seg_type == SYS_SEG_TSS_AVAILABLE));
	s->seg_type = SYS_SEG_TSS_AVAILABLE;
}

inline static void
load_hyp_segments(dtr_t *gdtr)
{
	seg_desc_t *gdt = (void *)gdtr->dt_base;

	set_GDTR(gdtr);

	asm volatile("mov %0, %%ss\n"
		     "mov %0, %%es\n"
		     "mov %0, %%ds\n"
		     "mov %0, %%fs\n"
		     "mov %0, %%gs\n" : : "r"(HYP_DS));

#ifdef __x86_64__
	asm volatile("pushq %0\n"
		     "pushq $1f\n"
		     "lretq\n"
		     "1: nop\n" : : "i"(HYP_CS));
#else
	asm volatile("ljmpl %0,$1f\n"
		     "1: nop\n" : : "i"(HYP_CS));
#endif

	set_LDT_sel(HYP_LDT);

	clear_tss_busy(gdt, HYP_TSS);
	set_TR(HYP_TSS);
}

extern ret_t idt_construct(void);
extern void idt_destroy(void);

extern ret_t tss_construct(dtr_t *, addr_t rsp0);
extern void tss_destroy(dtr_t *);
extern ret_t gdt_construct(dtr_t *, dtr_t *);
extern void gdt_destroy(dtr_t *);

#endif
