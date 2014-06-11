#include "hyp.h"
#include "vmx.h"

cpuinfo_t cpuinfo;

void
init_cpuinfo(void)
{
	u32 a, b, c, d;
	a = 1; c = 0;
	cpuid(a, b, c, d);

	cpuinfo.cpu_features = (((u64)c << 32) | d);
	cpuinfo.cpu_clflush_line_size = bits(b, 15, 8) << 3;

	cpuinfo.cpu_type = bits(a, 13, 12);
	cpuinfo.cpu_family = bits(a, 11, 8);
	cpuinfo.cpu_model = bits(a, 7, 4);
	cpuinfo.cpu_stepping = bits(a, 3, 0);

	if (cpuinfo.cpu_family == 0xf) {
		cpuinfo.cpu_family += bits(a, 27, 20);
	}
	if (cpuinfo.cpu_family >= 0x6) {
		cpuinfo.cpu_model += (bits(a, 19, 16) << 4);
	}

	kprintf("cpu type 0x%02x family 0x%02x model 0x%02x stepping 0x%02x\n",
		cpuinfo.cpu_type, cpuinfo.cpu_family,
		cpuinfo.cpu_model, cpuinfo.cpu_stepping);

	cpuinfo.cpu_xfeatures = __cpu_xfeatures();
	kprintf("cpu_xfeature_supported(CPU_XFEATURE_INTEL64) = %d\n",
		cpu_xfeature_supported(CPU_XFEATURE_INTEL64));
	kprintf("cpu_xfeature_supported(CPU_XFEATURE_NXE) = %d\n",
		cpu_xfeature_supported(CPU_XFEATURE_NXE));

	cpuinfo.cpu_phys_addr_width = __cpu_phys_addr_width();
	cpuinfo.cpu_virt_addr_width = __cpu_virt_addr_width();

	cpuinfo.cpu_tsc_freq_kHz = get_TSC_frequency();
	cpuinfo.cpu_tsc_freq_MHz = cpuinfo.cpu_tsc_freq_kHz / 1000;

	un ms = 1;
	for (un i = 0; i <= 3; i++) {
		kprintf("%ldms is %lld cycles\n", ms, ms_to_cycles(ms));
		kprintf("%ldus is %lld cycles\n", ms, us_to_cycles(ms));
		ms = ms * 10;
	}

	if (cpu_feature_supported(CPU_FEATURE_VMX)) {
		bool true_msrs = bit_test(get_MSR(MSR_IA32_VMX_BASIC), 55);

		if (true_msrs) {
			cpuinfo.msr_pinbased_ctls = 
				MSR_IA32_VMX_TRUE_PINBASED_CTLS;
			cpuinfo.msr_procbased_ctls =
				MSR_IA32_VMX_TRUE_PROCBASED_CTLS;
			cpuinfo.msr_exit_ctls =
				MSR_IA32_VMX_TRUE_EXIT_CTLS;
			cpuinfo.msr_entry_ctls =
				MSR_IA32_VMX_TRUE_ENTRY_CTLS;
		} else {
			cpuinfo.msr_pinbased_ctls =
				MSR_IA32_VMX_PINBASED_CTLS;
			cpuinfo.msr_procbased_ctls =
				MSR_IA32_VMX_PROCBASED_CTLS;
			cpuinfo.msr_exit_ctls =
				MSR_IA32_VMX_EXIT_CTLS;
			cpuinfo.msr_entry_ctls =
				MSR_IA32_VMX_ENTRY_CTLS;
		}

		if (vmx_exec_cpu1_supported(
					VMEXEC_CPU1_ACTIVATE_SECONDARY_CTRLS)) {
			cpuinfo.msr_procbased_ctls2 =
				MSR_IA32_VMX_PROCBASED_CTLS2;
		}
	}
}

void
mdelay(un ms)
{
	u64 start = rdtsc();
	u64 finish = start + ms_to_cycles(ms);

	while (rdtsc() < finish) {
		pause();
	}
}

void
udelay(un us)
{
	u64 start = rdtsc();
	u64 finish = start + us_to_cycles(us);

	while (rdtsc() < finish) {
		pause();
	}
}

void
flush_cache(addr_t addr, size_t len)
{
	fence();

	addr_t end = addr + len;
	un clsize = clflush_line_size();

	for (addr_t a = addr; a < end; a += clsize) {
		clflush((void *)a);
	}

	fence();
}

addr_t
get_seg_base(u16 selector, addr_t gdtbase, u16 ldt_sel, bool target64)
{
	if (selector == 0) {
		return 0;
	}
	un offset = selector & ~0x7UL;

	un table = gdtbase;

	if (selector & 4) {
		assert(ldt_sel);
		table = get_seg_base(ldt_sel, gdtbase, 0, target64);
	}

	seg_desc_t *s = (seg_desc_t *)(table + offset);

	return seg_desc_base(s, target64);
}

addr_t
get_seg_base_self(u16 selector)
{
	dtr_t gdtr;
	get_GDTR(&gdtr);

	return get_seg_base(selector, gdtr.dt_base, get_LDT_sel(), true);
}

u32
get_seg_limit(u16 selector, addr_t gdtbase, u16 ldt_sel, bool target64)
{
	if (selector == 0) {
		return 0;
	}
	un offset = selector & ~0x7UL;

	un table = gdtbase;

	if (selector & 4) {
		assert(ldt_sel);
		table = get_seg_base(ldt_sel, gdtbase, 0, target64);
	}

	seg_desc_t *s = (seg_desc_t *)(table + offset);

	return seg_desc_limit(s);
}

u32
get_seg_limit_self(u16 selector)
{
	dtr_t gdtr;
	get_GDTR(&gdtr);

	return get_seg_limit(selector, gdtr.dt_base, get_LDT_sel(), true);
}

u32
get_seg_attr(u16 selector, addr_t gdtbase, u16 ldt_sel, bool target64)
{
	if (selector == 0) {
		return SEG_UNUSABLE;
	}
	un offset = selector & ~0x7UL;

	un table = gdtbase;

	if (selector & 4) {
		assert(ldt_sel);
		table = get_seg_base(ldt_sel, gdtbase, 0, target64);
	}

	seg_desc_t *s = (seg_desc_t *)(table + offset);

	return s->seg_attr & ~ATTR_RESERVED;
}

u32
get_seg_attr_self(u16 selector)
{
	dtr_t gdtr;
	get_GDTR(&gdtr);

	return get_seg_attr(selector, gdtr.dt_base, get_LDT_sel(), true);
}

un
get_TSC_frequency(void)
{
	un freq = 2000000; /* kHz */

	if (cpu_is_nehalem()) {
		/* Intel Microarchitecture (Nehalem) */

		u64 platform_info = get_MSR(MSR_PLATFORM_INFO);

		freq = bits64(platform_info, 15, 8) * 133333; /*kHz */

		kprintf("get_TSC_frequency>freq %ld kHz\n", freq);

		return freq;
	} else if (cpu_is_core2()) {
		u64 platform_id = get_MSR(MSR_IA32_PLATFORM_ID);
		u64 perf_status = get_MSR(MSR_IA32_PERF_STATUS);

		bool xe = bit_test(perf_status, 31);

		un max_reslvd_bus_ratio = (xe ? bits64(perf_status, 44, 40) :
					   bits64(platform_id, 12, 8));

		u64 fsb_freq = get_MSR(MSR_FSB_FREQ);

		static const un scalable_bus_freq[8] = {
			266667,
			133333,
			200000,
			166667,
			333333,
			100000,
			400000,
		};

		freq = (scalable_bus_freq[bits(fsb_freq, 2, 0)] *
			max_reslvd_bus_ratio);
		kprintf("get_TSC_frequency>freq %ld kHz\n", freq);
	} else if (cpu_is_amd_10h()) {
		freq = 2800000; /* kHz */
		kprintf("get_TSC_frequency>AMD Family 10h CPU, "
			"guessing freq %ld kHz\n", freq);
	} else {
		kprintf("get_TSC_frequency>unknown CPU model, "
			"guessing freq %ld kHz\n", freq);
	}
	return freq;
}

u8
mtrr_type(un paddr)
{
	u64 def_type = get_MSR(MSR_IA32_DEF_TYPE);

	if (!bit_test(def_type, MT_E)) {
		return MT_UC;
	}

	u64 mtrr;
	u8 subrange;

	if ((paddr < 0x100000) && bit_test(def_type, MT_FE)) {
		if (paddr < 0x80000) {
			mtrr = get_MSR(MSR_IA32_MTRR_FIX64K_0000);
			subrange = extract(paddr, 16, 3);
		} else if (paddr < 0xa0000) {
			mtrr = get_MSR(MSR_IA32_MTRR_FIX16K_80000);
			subrange = extract(paddr, 14, 3);
		} else if (paddr < 0xc0000) {
			mtrr = get_MSR(MSR_IA32_MTRR_FIX16K_A0000);
			subrange = extract(paddr, 14, 3);
		} else if (paddr < 0xc8000) {
			mtrr = get_MSR(MSR_IA32_MTRR_FIX4K_C0000);
			subrange = extract(paddr, 12, 3);
		} else if (paddr < 0xd0000) {
			mtrr = get_MSR(MSR_IA32_MTRR_FIX4K_C8000);
			subrange = extract(paddr, 12, 3);
		} else if (paddr < 0xd8000) {
			mtrr = get_MSR(MSR_IA32_MTRR_FIX4K_D0000);
			subrange = extract(paddr, 12, 3);
		} else if (paddr < 0xe0000) {
			mtrr = get_MSR(MSR_IA32_MTRR_FIX4K_D8000);
			subrange = extract(paddr, 12, 3);
		} else if (paddr < 0xe8000) {
			mtrr = get_MSR(MSR_IA32_MTRR_FIX4K_E0000);
			subrange = extract(paddr, 12, 3);
		} else if (paddr < 0xf0000) {
			mtrr = get_MSR(MSR_IA32_MTRR_FIX4K_E8000);
			subrange = extract(paddr, 12, 3);
		} else if (paddr < 0xf8000) {
			mtrr = get_MSR(MSR_IA32_MTRR_FIX4K_F0000);
			subrange = extract(paddr, 12, 3);
		} else {
			mtrr = get_MSR(MSR_IA32_MTRR_FIX4K_F8000);
			subrange = extract(paddr, 12, 3);
		}
		u8 shift = (subrange << 3);
		return extract(mtrr, shift, 8); 
	}	

	u8 n = cpu_phys_addr_width() - 1;

	u8 type = 0xff;

	for (u8 i = 0; i < 16; i += 2) {
		u64 phys_base = get_MSR(MSR_IA32_MTRR_PHYS_BASE0 + i);
		u64 phys_mask = get_MSR(MSR_IA32_MTRR_PHYS_MASK0 + i);

		if (!bit_test(phys_mask, MTPM_VALID)) {
		       continue;
		}	       

		un base = bits(phys_base, n, 12);
		un mask = bits(phys_mask, n, 12);

		if ((bits(paddr, n, 12) & mask) == (base & mask)) {
			u8 new_type = bits(phys_base, 7, 0);
			if (new_type == MT_UC) {
				return new_type;
			} else if (new_type == type) {
				continue;
			} else if ((type == MT_WB) && (new_type == MT_WT)) {
				type = MT_WT;
			} else if ((type == MT_WT) && (new_type == MT_WB)) {
				type = MT_WT;
			} else if (type == 0xff) {
			       type = new_type;
			} else {
				kprintf("mtrr_type>undefined type overlap %d "
					"and %d for paddr 0x%lx\n",
					type, new_type, paddr);
			}
		}
	}

	if (type != 0xff) {
		return type;
	}

	return bits(def_type, 7, 0);
}

u64 __cache_timing(register int n)
{
	static volatile un x;
	register un dummy;
	u64 start;
	dummy = x;
	start = rdtsc();
	while (n--) {
		dummy = x;
	}
	return rdtsc() - start;
}


#define DUMP
#ifdef DUMP

static bool
dump_seg_desc(seg_desc_t *s, bool target64)
{
	bool dbl = false;

	if (s->seg_type == 0) {
		kprintf("type=0\n");
		return dbl;
	}
	addr_t base = (((u32)(s->seg_base2) << 24) |
		       ((u32)(s->seg_base1) << 16) |
		       s->seg_base0);
	u32 limit = (((u32)(s->seg_limit1) << 16) | s->seg_limit0);
#ifdef __x86_64__
	if ((s->seg_s == 0) && target64) {
		base |= ((u64)(((seg_desc64_t *)s)->seg_base3) << 32);
		dbl = true;
	}
#endif
	kprintf("base 0x%lx limit 0x%x type=%d s=%d dpl=%d p=%d"
		" avl=%d l=%d d=%d g=%d\n",
		base,
		limit,
		s->seg_type, s->seg_s, s->seg_dpl, s->seg_p,
		s->seg_avl, s->seg_l, s->seg_d, s->seg_g);
	return dbl;
}

void
dump_gdt(dtr_t *gdtr, bool target64)
{
	un gdtbase = gdtr->dt_base;
	un gdtmax = (gdtr->dt_limit + 1)/sizeof(seg_desc_t);
	seg_desc_t *s = (seg_desc_t *)gdtbase;

	for (int i = 0; i < gdtmax ; i++) {
		kprintf("selector %02x\n", i << 3);
		if (dump_seg_desc(&s[i], target64)) {
			i++;
		}
	}
}

static void
dump_segment(char *name, un selector, un gdtbase, bool target64)
{
	kprintf("%s: 0x%lx\n", name, selector);
	un index = selector >> 3;
	un offset = selector & ~0x7UL;

	kprintf("index = 0x%lx\n", index);
	kprintf("offset = 0x%lx\n", offset);

	dump_seg_desc((seg_desc_t *)(gdtbase + offset), target64);
}

static bool
dump_intr_desc(int i, intr_desc_t *d)
{
	bool dbl = false;

	un offset = ((un)(d->id_offset1) << 16) | d->id_offset0;
#ifdef __x86_64__
	offset |= ((un)(d->id_offset2) << 32);
#define ISTFMT " ist=%d "
#else
#define ISTFMT " "
#endif

	kprintf("%3d sel 0x%x offset 0x%lx" ISTFMT "type=%d dpl=%d p=%d\n",
		i,
		d->id_sel,
		offset,
#ifdef __x86_64__
		d->id_ist,
#endif
		d->id_type, d->id_dpl, d->id_p);
	return dbl;
}

void
dump_idt(dtr_t *idtr)
{
	un idtbase = idtr->dt_base;
	un idtmax = (idtr->dt_limit + 1)/sizeof(intr_desc_t);
	intr_desc_t *idesc = (intr_desc_t *)idtbase;

	for (int i = 0; i < idtmax ; i++) {
		if (dump_intr_desc(i, &idesc[i])) {
			i++;
		}
	}
}

#define TSS_DUMP(x) kprintf("tss_" #x "\t0x%x\n", tss->tss_ ## x);

void
dump_tss32(tss32_t *tss)
{
	dtr_t gdtr;
	get_GDTR(&gdtr);
	un gdtbase = gdtr.dt_base;

	kprintf("tss @ 0x%lx\n", (addr_t)tss);
	TSS_DUMP(prev_task_link);
	TSS_DUMP(rsp0);
	TSS_DUMP(ss0);
	dump_segment("SS0", tss->tss_ss0, gdtbase, false);
	TSS_DUMP(rsp1);
	TSS_DUMP(ss1);
	dump_segment("SS1", tss->tss_ss1, gdtbase, false);
	TSS_DUMP(rsp2);
	TSS_DUMP(ss2);
	dump_segment("SS2", tss->tss_ss2, gdtbase, false);
	TSS_DUMP(cr3);
	TSS_DUMP(eip);
	TSS_DUMP(eflags);
	TSS_DUMP(eax);
	TSS_DUMP(ecx);
	TSS_DUMP(edx);
	TSS_DUMP(ebx);
	TSS_DUMP(esp);
	TSS_DUMP(ebp);
	TSS_DUMP(esi);
	TSS_DUMP(edi);
	TSS_DUMP(es);
	TSS_DUMP(cs);
	TSS_DUMP(ss);
	TSS_DUMP(ds);
	TSS_DUMP(fs);
	TSS_DUMP(gs);
	TSS_DUMP(ldtr);
	TSS_DUMP(debug_trap);
	TSS_DUMP(iomap_base);
}

#undef TSS_DUMP
#define TSS_DUMP(x) kprintf("tss_" #x "\t0x%lx\n", (un)(tss->tss_ ## x));

void
dump_tss64(tss64_t *tss)
{
	kprintf("tss @ 0x%lx\n", (addr_t)tss);
	TSS_DUMP(rsp0);
	TSS_DUMP(rsp1);
	TSS_DUMP(rsp2);
	TSS_DUMP(ist1);
	TSS_DUMP(ist2);
	TSS_DUMP(ist3);
	TSS_DUMP(ist4);
	TSS_DUMP(ist5);
	TSS_DUMP(ist6);
	TSS_DUMP(ist7);
	TSS_DUMP(iomap_base);
}

void
dump_MTRRs(void)
{
	u64 mtrrcap = get_MSR(MSR_IA32_MTRRCAP);
	u64 def_type = get_MSR(MSR_IA32_DEF_TYPE);
	u64 cr_pat = get_MSR(MSR_IA32_CR_PAT);

	kprintf("dump_MTTRs>MSR_IA32_MTRRCAP 0x%llx\n", mtrrcap);
	kprintf("dump_MTTRs>MSR_IA32_DEF_TYPE 0x%llx\n", def_type);
	kprintf("dump_MTTRs>MSR_IA32_CR_PAT 0x%llx\n", cr_pat);

	kprintf("dump_MTTRs>MSR_IA32_MTRR_FIX64K_0000 0x%llx\n",
		get_MSR(MSR_IA32_MTRR_FIX64K_0000));
	kprintf("dump_MTTRs>MSR_IA32_MTRR_FIX16K_80000 0x%llx\n",
		get_MSR(MSR_IA32_MTRR_FIX16K_80000));
	kprintf("dump_MTTRs>MSR_IA32_MTRR_FIX16K_A0000 0x%llx\n",
		get_MSR(MSR_IA32_MTRR_FIX16K_A0000));
	kprintf("dump_MTTRs>MSR_IA32_MTRR_FIX4K_C0000 0x%llx\n",
		get_MSR(MSR_IA32_MTRR_FIX4K_C0000));
	kprintf("dump_MTTRs>MSR_IA32_MTRR_FIX4K_C8000 0x%llx\n",
		get_MSR(MSR_IA32_MTRR_FIX4K_C8000));
	kprintf("dump_MTTRs>MSR_IA32_MTRR_FIX4K_D0000 0x%llx\n",
		get_MSR(MSR_IA32_MTRR_FIX4K_D0000));
	kprintf("dump_MTTRs>MSR_IA32_MTRR_FIX4K_D8000 0x%llx\n",
		get_MSR(MSR_IA32_MTRR_FIX4K_D8000));
	kprintf("dump_MTTRs>MSR_IA32_MTRR_FIX4K_E0000 0x%llx\n",
		get_MSR(MSR_IA32_MTRR_FIX4K_E0000));
	kprintf("dump_MTTRs>MSR_IA32_MTRR_FIX4K_E8000 0x%llx\n",
		get_MSR(MSR_IA32_MTRR_FIX4K_E8000));
	kprintf("dump_MTTRs>MSR_IA32_MTRR_FIX4K_F0000 0x%llx\n",
		get_MSR(MSR_IA32_MTRR_FIX4K_F0000));
	kprintf("dump_MTTRs>MSR_IA32_MTRR_FIX4K_F8000 0x%llx\n",
		get_MSR(MSR_IA32_MTRR_FIX4K_F8000));

	kprintf("dump_MTTRs>MSR_IA32_MTRR_PHYS_BASE0 0x%09llx\n",
		get_MSR(MSR_IA32_MTRR_PHYS_BASE0));
	kprintf("dump_MTTRs>MSR_IA32_MTRR_PHYS_MASK0 0x%09llx\n",
		get_MSR(MSR_IA32_MTRR_PHYS_MASK0));
	kprintf("dump_MTTRs>MSR_IA32_MTRR_PHYS_BASE1 0x%09llx\n",
		get_MSR(MSR_IA32_MTRR_PHYS_BASE1));
	kprintf("dump_MTTRs>MSR_IA32_MTRR_PHYS_MASK1 0x%09llx\n",
		get_MSR(MSR_IA32_MTRR_PHYS_MASK1));
	kprintf("dump_MTTRs>MSR_IA32_MTRR_PHYS_BASE2 0x%09llx\n",
		get_MSR(MSR_IA32_MTRR_PHYS_BASE2));
	kprintf("dump_MTTRs>MSR_IA32_MTRR_PHYS_MASK2 0x%09llx\n",
		get_MSR(MSR_IA32_MTRR_PHYS_MASK2));
	kprintf("dump_MTTRs>MSR_IA32_MTRR_PHYS_BASE3 0x%09llx\n",
		get_MSR(MSR_IA32_MTRR_PHYS_BASE3));
	kprintf("dump_MTTRs>MSR_IA32_MTRR_PHYS_MASK3 0x%09llx\n",
		get_MSR(MSR_IA32_MTRR_PHYS_MASK3));
	kprintf("dump_MTTRs>MSR_IA32_MTRR_PHYS_BASE4 0x%09llx\n",
		get_MSR(MSR_IA32_MTRR_PHYS_BASE4));
	kprintf("dump_MTTRs>MSR_IA32_MTRR_PHYS_MASK4 0x%09llx\n",
		get_MSR(MSR_IA32_MTRR_PHYS_MASK4));
	kprintf("dump_MTTRs>MSR_IA32_MTRR_PHYS_BASE5 0x%09llx\n",
		get_MSR(MSR_IA32_MTRR_PHYS_BASE5));
	kprintf("dump_MTTRs>MSR_IA32_MTRR_PHYS_MASK5 0x%09llx\n",
		get_MSR(MSR_IA32_MTRR_PHYS_MASK5));
	kprintf("dump_MTTRs>MSR_IA32_MTRR_PHYS_BASE6 0x%09llx\n",
		get_MSR(MSR_IA32_MTRR_PHYS_BASE6));
	kprintf("dump_MTTRs>MSR_IA32_MTRR_PHYS_MASK6 0x%09llx\n",
		get_MSR(MSR_IA32_MTRR_PHYS_MASK6));
	kprintf("dump_MTTRs>MSR_IA32_MTRR_PHYS_BASE7 0x%09llx\n",
		get_MSR(MSR_IA32_MTRR_PHYS_BASE7));
	kprintf("dump_MTTRs>MSR_IA32_MTRR_PHYS_MASK7 0x%09llx\n",
		get_MSR(MSR_IA32_MTRR_PHYS_MASK7));

#ifdef NOTDEF
	for (un paddr = 0; paddr < 0x104000; paddr += 0x1000) {
		kprintf("0x%06lx %d\n", paddr, mtrr_type(paddr));
	}
#endif
}

void
dump_x86(void)
{
#ifdef __x86_64__
	const bool host64 = true;
#else
	const bool host64 = false;
#endif
	int a = 0;
	int c = 0;
	int b, d;
	cpuid(a, b, c, d);
	kprintf("0x%x 0x%x 0x%x 0x%x\n", a, b, c, d);

	char buf[16];
	u32 ret = cpuid_stem(0, buf);
	kprintf("max value for basic CPUID info = %d, %s\n", ret, buf);

	kprintf("VMX CPU support%s found\n",
		cpu_feature_supported(CPU_FEATURE_VMX) ? "" : " NOT");
	kprintf("SMX CPU support%s found\n",
		cpu_feature_supported(CPU_FEATURE_SMX) ? "" : " NOT");

	dtr_t idtr;
	get_IDTR(&idtr);
	kprintf("idtr base 0x%lx limit 0x%x\n", idtr.dt_base, idtr.dt_limit);

	dump_idt(&idtr);

	dtr_t gdtr;
	get_GDTR(&gdtr);
	kprintf("gdtr base 0x%lx limit 0x%x\n", gdtr.dt_base, gdtr.dt_limit);

	dump_gdt(&gdtr, host64);

	un gdtbase = gdtr.dt_base;

	dump_segment("CS", get_CS(), gdtbase, host64);
	dump_segment("SS", get_SS(), gdtbase, host64);
	dump_segment("DS", get_DS(), gdtbase, host64);
	dump_segment("ES", get_ES(), gdtbase, host64);
	dump_segment("FS", get_FS(), gdtbase, host64);
	dump_segment("GS", get_GS(), gdtbase, host64);
	dump_segment("TR", get_TR(), gdtbase, host64);

	kprintf("LDT_sel 0x%lx\n", get_LDT_sel());

#ifdef __x86_64__
	kprintf("MSR_IA32_FS_BASE 0x%llx\n", get_MSR(MSR_IA32_FS_BASE));
	kprintf("MSR_IA32_GS_BASE 0x%llx\n", get_MSR(MSR_IA32_GS_BASE));
	kprintf("MSR_IA32_KERNEL_GS_BASE 0x%llx\n",
		get_MSR(MSR_IA32_KERNEL_GS_BASE));

	dump_tss64((tss64_t *)get_seg_base_self(get_TR()));
#endif

	// dump_MTRRs();
}

#endif
