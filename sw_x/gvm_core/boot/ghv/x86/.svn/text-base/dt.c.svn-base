#include "hyp.h"
#include "dt.h"

dtr_t hyp_idtr;

dtr_t hyp_tss[MAX_CPUS];
dtr_t hyp_gdt[MAX_CPUS];

static void
__set_gdt(seg_desc_t *gdt, u16 sel, addr_t addr, u32 limit,
	  bool system, u8 type, u8 dpl, bool target64)
{
	un index = sel >> 3;

	seg_desc_t *s = &gdt[index];

	bool code = !system && bit_test(type, 3);

	s->seg_s = (system ? 0 : 1);
	s->seg_type = type;
	s->seg_dpl = dpl;
	s->seg_p = 1;
	if (code && target64) {
		s->seg_l = 1;
		s->seg_d = 0;
	} else if (system) {
		s->seg_l = 0;
		s->seg_d = 0;
	} else {
		s->seg_l = 0;
		s->seg_d = 1;
	}
	s->seg_avl = 0;

	s->seg_base0 = bits(addr, 15, 0);
	s->seg_base1 = bits(addr, 23, 16);
	s->seg_base2 = bits(addr, 31, 24);
#ifdef __x86_64__
	if (system && target64) {
		seg_desc64_t *s64 = (seg_desc64_t *)s;
		s64->seg_base3 = bits(addr, 63, 32);
	}
#endif

	if (limit >= (1U << 20)) {
		s->seg_g = 1;
		limit = (limit >> VM_PAGE_SHIFT);
	}
	s->seg_limit0 = bits(limit, 15, 0);
	s->seg_limit1 = bits(limit, 19, 16);
}

static void
set_gdt_code(seg_desc_t *gdt, u16 sel, u8 dpl, bool target64)
{
	__set_gdt(gdt, sel, 0, ~0, false, SEG_CODE_XRA, dpl, target64);
}

static void
set_gdt_data(seg_desc_t *gdt, u16 sel, u8 dpl)
{
	__set_gdt(gdt, sel, 0, ~0, false, SEG_DATA_RWA, dpl, false);
}

static void
set_gdt_tss(seg_desc_t *gdt, u16 sel, dtr_t *tss, bool target64)
{
	__set_gdt(gdt, sel, tss->dt_base, tss->dt_limit,
		  true, SYS_SEG_TSS_BUSY, 0, target64);
}

static void
set_idt(intr_desc_t *idt, un vector, addr_t addr)
{
	idt[vector].id_sel = HYP_CS;
	idt[vector].id_type = SYS_SEG_INTR_GATE;
	idt[vector].id_dpl = 0; 
	idt[vector].id_p = 1; 
#ifdef __x86_64__
	idt[vector].id_ist = 0;
	idt[vector].id_offset2 = bits(addr, 63, 32);
#endif
	idt[vector].id_offset1 = bits(addr, 31, 16);
	idt[vector].id_offset0 = bits(addr, 15, 0);
}

ret_t
gdt_construct(dtr_t *d, dtr_t *tss)
{
	size_t len = sizeof(seg_desc_t[GDT_LEN]);
	seg_desc_t *gdt = (seg_desc_t *)alloc(len);
	if (!gdt) {
		return -ENOMEM;
	}

	d->dt_base = (addr_t)gdt;
	d->dt_limit = len - 1;

#ifdef __x86_64__
	const bool host64 = true;
#else
	const bool host64 = false;
#endif

	set_gdt_code(gdt, HYP_CS,      0, host64);
	set_gdt_code(gdt, HYP_CS_32,   0, false);
	set_gdt_code(gdt, HYP_USER_CS, 3, host64);

	set_gdt_data(gdt, HYP_DS,      0);
	set_gdt_data(gdt, HYP_USER_DS, 3);

	set_gdt_tss(gdt, HYP_TSS, tss, host64);

	return 0;
}

void
gdt_destroy(dtr_t *d)
{
	seg_desc_t *gdt = (seg_desc_t *)(d->dt_base);
	if (!gdt) {
		return;
	}

	free(gdt);

	d->dt_base = 0;
	d->dt_limit = 0;
}

ret_t
idt_construct(void)
{
	size_t len = sizeof(intr_desc_t[IDT_LEN]);
	intr_desc_t *idt = (intr_desc_t *)alloc(len);
	if (!idt) {
		return -ENOMEM;
	}

	hyp_idtr.dt_base = (addr_t)idt;
	hyp_idtr.dt_limit = len - 1;

#define SET_IDT(v, f) \
	extern void f(void); \
	set_idt(idt, v, (addr_t)&f)

	SET_IDT(VEC_DE, div_zero); 
	SET_IDT(VEC_DB, debug); 
	SET_IDT(VEC_NMI, nmi); 
	SET_IDT(VEC_BP, breakpoint); 
	SET_IDT(VEC_OF, overflow); 
	SET_IDT(VEC_BR, bounds); 
	SET_IDT(VEC_UD, invalid_opcode); 
	SET_IDT(VEC_NM, no_device); 
	SET_IDT(VEC_DF, double_fault); 
	SET_IDT(VEC_TS, invalid_tss); 
	SET_IDT(VEC_NP, seg_not_present); 
	SET_IDT(VEC_SS, stack_fault); 
	SET_IDT(VEC_GP, gen_protect);
	SET_IDT(VEC_PF, page_fault); 
	SET_IDT(VEC_MF, fp_error); 
	SET_IDT(VEC_AC, align_check); 
	SET_IDT(VEC_MC, machine_check); 
	SET_IDT(VEC_XF, simd_error); 

	SET_IDT(9, exception9); 
	SET_IDT(15, exception15); 

	extern addr_t intvec_start;
	extern un intvec_len;

	addr_t addr = intvec_start;
	for (un vec = 20; vec < 256; vec++) {
		set_idt(idt, vec, addr);
		addr += intvec_len;
	}	

	return 0;
}

void
idt_destroy(void)
{
	void *p = (void *)hyp_idtr.dt_base;
	if (p) {
		free(p);
		hyp_idtr.dt_base = 0;
		hyp_idtr.dt_limit = 0;
	}
}

ret_t
tss_construct(dtr_t *d, addr_t rsp0)
{
	size_t len = sizeof(tss_t);
	tss_t *t = alloc(len);
	if (!t) {
		return -ENOMEM;
	}
	d->dt_base = (addr_t)t;
	d->dt_limit = len - 1;

	t->tss_iomap_base = len; /* No iomap */

	t->tss_rsp0 = rsp0;
	if (!t->tss_rsp0) {
		goto err;
	}
#ifndef __x86_64__
	t->tss_ss0 = HYP_DS;
#endif

	return 0;
err:
	tss_destroy(d);
	return NULL;
}

void
tss_destroy(dtr_t *d)
{
	tss_t *t = (tss_t *)(d->dt_base);
	if (!t) {
		return;
	}

	t->tss_rsp0 = NULL;

	free(t);

	d->dt_base = 0;
	d->dt_limit = 0;
}
