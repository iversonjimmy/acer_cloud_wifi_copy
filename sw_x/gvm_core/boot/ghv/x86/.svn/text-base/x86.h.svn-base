#ifndef __X86_H__
#define __X86_H__

#define VM_PAGE_SHIFT	12
#define VM_PAGE_SIZE	(1UL << VM_PAGE_SHIFT)
#define VM_PAGE_OFFSET	(VM_PAGE_SIZE - 1UL)

#define TRUNC_PAGE(x)	(((x) & VM_PAGE_OFFSET) ^ (x))
#define ROUND_PAGE(x)	TRUNC_PAGE((x) + VM_PAGE_OFFSET)

#define VM_4M_PAGE_SHIFT  22
#define VM_4M_PAGE_SIZE   (1UL << VM_4M_PAGE_SHIFT)
#define VM_4M_PAGE_OFFSET (VM_4M_PAGE_SIZE - 1UL)

#define TRUNC_4M_PAGE(x) (((x) & VM_4M_PAGE_OFFSET) ^ (x))
#define ROUND_4M_PAGE(x) TRUNC_4M_PAGE((x) + VM_4M_PAGE_OFFSET)

#define VM_2M_PAGE_SHIFT  21
#define VM_2M_PAGE_SIZE   (1UL << VM_2M_PAGE_SHIFT)
#define VM_2M_PAGE_OFFSET (VM_2M_PAGE_SIZE - 1UL)

#define TRUNC_2M_PAGE(x) (((x) & VM_2M_PAGE_OFFSET) ^ (x))
#define ROUND_2M_PAGE(x) TRUNC_2M_PAGE((x) + VM_2M_PAGE_OFFSET)

#if defined(__x86_64__) || defined(HYP_PAE)
#define VM_LARGE_PAGE_SHIFT VM_2M_PAGE_SHIFT
#else
#define VM_LARGE_PAGE_SHIFT VM_4M_PAGE_SHIFT
#endif
#define VM_LARGE_PAGE_SIZE	(1UL << VM_LARGE_PAGE_SHIFT)
#define VM_LARGE_PAGE_OFFSET	(VM_LARGE_PAGE_SIZE - 1UL)

#define TRUNC_LARGE_PAGE(x)	(((x) & VM_LARGE_PAGE_OFFSET) ^ (x))
#define ROUND_LARGE_PAGE(x)	TRUNC_LARGE_PAGE((x) + VM_LARGE_PAGE_OFFSET)

#define memory()		asm volatile("":::"memory")

inline static void
clflush(volatile void *ptr)
{
	asm volatile("clflush %0" : "+m" (*(volatile char *)ptr));
}

inline static void
invlpg(addr_t addr)
{
	asm volatile("invlpg (%0)" : : "r" (addr) : "memory");
}

#define volatile_read(ptr)	(*((volatile typeof(*(ptr)) *)(ptr)))
#define volatile_write(ptr, n)	(*((volatile typeof(*(ptr)) *)(ptr)) = (n))

inline static s8
atomic_cx1(s8 *atom, s8 old, s8 new)
{
	s8 prev;
	asm volatile("lock; cmpxchgb %1, %2" :
		     "=a" (prev) :
		     "r" (new), "m" (*atom), "0" (old) :
		     "memory");
	return prev;
}

inline static s16
atomic_cx2(s16 *atom, s16 old, s16 new)
{
	s16 prev;
	asm volatile("lock; cmpxchgw %1, %2" :
		     "=a" (prev) :
		     "r" (new), "m" (*atom), "0" (old) :
		     "memory");
	return prev;
}

inline static s32
atomic_cx4(s32 *atom, s32 old, s32 new)
{
	s32 prev;
	asm volatile("lock; cmpxchgl %1, %2" :
		     "=a" (prev) :
		     "r" (new), "m" (*atom), "0" (old) :
		     "memory");
	return prev;
}

inline static s64
atomic_cx8(s64 *atom, s64 old, s64 new)
{
#ifdef __x86_64__
	s64 prev;
	asm volatile("lock; cmpxchgq %1, %2" :
		     "=a" (prev) :
		     "r" (new), "m" (*atom), "0" (old) :
		     "memory");
	return prev;
#else
	u32 prevlo, prevhi;
	asm volatile("lock; cmpxchg8b %2" :
		     "=d" (prevhi), "=a" (prevlo) :
		     "m" (*atom),
		     "0" (bits(old, 63, 32)), "1" (bits(old, 31, 0)),
		     "c" (bits(new, 63, 32)), "b" (bits(new, 31, 0)) :
		     "memory");
	return (s64)(((u64)prevhi << 32) | (u64)prevlo);
#endif
}

#define atomic_cx(ptr, old, new) \
({ \
	const un _size = sizeof(*(ptr)); \
	typeof(*(ptr)) _prev; \
	switch (_size) { \
	case 1: \
		_prev = atomic_cx1((s8 *)(ptr), (old), (new)); \
		break; \
	case 2: \
		_prev = atomic_cx2((s16 *)(ptr), (old), (new)); \
		break; \
	case 4: \
		_prev = atomic_cx4((s32 *)(ptr), (old), (new)); \
		break; \
	case 8: \
		_prev = atomic_cx8((s64 *)(ptr), (old), (new)); \
		break; \
	} \
	_prev; \
})

#define atomic_or(ptr, value) \
({ \
	typeof(*(ptr)) __old; \
	typeof(*(ptr)) __new; \
	typeof(*(ptr)) __res = *(ptr); \
	do { \
		__old = __res; \
		__new = __old | (value); \
		__res = atomic_cx((ptr), __old, __new); \
	} while (__res != __old); \
	__old; \
})

#define atomic_add(ptr, value) \
({ \
	typeof(*(ptr)) __old; \
	typeof(*(ptr)) __new; \
	typeof(*(ptr)) __res = *(ptr); \
	do { \
		__old = __res; \
		__new = __old + (value); \
		__res = atomic_cx((ptr), __old, __new); \
	} while (__res != __old); \
	__old; \
})

#define atomic_inc(ptr) atomic_add(ptr, 1)

#define atomic_bit_clear(ptr, bitno) \
({ \
	typeof(*(ptr)) __old; \
	typeof(*(ptr)) __new; \
	typeof(*(ptr)) __res = *(ptr); \
	do { \
		__new = __old = __res; \
		bit_clear(__new, (bitno)); \
		__res = atomic_cx((ptr), __old, __new); \
	} while (__res != __old); \
	__old; \
})

#define fence()	asm volatile("mfence" : : : "memory")
#define pause()	asm volatile("pause");

// cache ops;

#define wbinvd() asm volatile("wbinvd" : : : "memory")

// cpu id;

#ifndef __pic__
#define cpuid(eax, ebx, ecx, edx) \
{ \
	asm volatile("cpuid" : \
		     "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx) : \
		     "0" (eax), "2" (ecx)); \
}
#else
#define cpuid(eax, ebx, ecx, edx) \
{ \
	asm volatile("xchg %%ebx, %1\n"				      \
		     "cpuid\n"					      \
		     "xchg %%ebx, %1\n" :			      \
		     "=a" (eax), "=r" (ebx), "=c" (ecx), "=d" (edx) : \
		     "0" (eax), "2" (ecx)); \
}
#endif

inline static u32
cpuid_stem(u32 a, char *buf)
{
	u32 b, c, d;

	c = 0;
	cpuid(a, b, c, d);

	if (buf) {
		*(u32 *)buf = b;
		*(u32 *)(buf + 4) = d;
		*(u32 *)(buf + 8) = c;
		buf[12] = '\0';
	}

	return a;
}

inline static u32
cpu_apic_id(void)
{
	u32 a, b, c, d;

	a = 1; c = 0;
	cpuid(a, b, c, d);

	return b >> 24;
}

inline static u32
cpu_x2apic_id(void)
{
	u32 a, b, c, d;

	a = 0xb; c = 0;
	cpuid(a, b, c, d);

	return b != 0 ? d : ~0;
}

inline static u32
cpu_id(void)
{
	if (cpuid_stem(0, NULL) >= 0xb) {
		u32 id = cpu_x2apic_id();
		if (id != ~0U) {
			return id;
		}
	}

	return cpu_apic_id();
}
#define cpuno()	cpu_apic_id()

// locks;

typedef struct {
	un lock;
#ifdef LOCK_DEBUG
	s32 cpu;
	char *name;
#endif // LOCK_DEBUG
} lock_t;

#ifdef LOCK_DEBUG
#define LOCK_INIT(name)		{ 0, -1, #name }
#else // LOCK_DEBUG
#define LOCK_INIT(name)		{ 0 }
#endif // LOCK_DEBUG
#define LOCK_INIT_STATIC(name)	lock_t name = LOCK_INIT(name)

#ifdef LOCK_DEBUG
static void
lock_panic(u32 cpu, lock_t *lock)
{
    int size;
    char buf[80];
    extern int snprintf(char *str, size_t size, const char *format, ...)\
        __attribute__ ((format (printf, 3, 4)));
    extern int serial_write(const char *buf, size_t count);

    char *lock_name = lock->name;
    if (!lock_name) {
	    lock_name = "";
    }
    size = snprintf(buf, sizeof(buf), "#[%d] lock %p %s#\n", cpu, lock,
		    lock_name);
    serial_write(buf, size);
    while(1) {
        pause();
    }
}
#endif // LOCK_DEBUG

inline static void
__lock_init(lock_t *lock, char *name)
{
	lock->lock = 0;
#ifdef LOCK_DEBUG
	lock->cpu = -1;
	lock->name = name;
#endif // LOCK_DEBUG
	fence();
}

#define lock_init(l)	__lock_init((l), #l)

inline static void
nested_spinlock(lock_t *lock)
{
#ifdef LOCK_DEBUG
	u32 cpu = cpuno();
#endif // LOCK_DEBUG

	while (atomic_cx(&lock->lock, 0, 1) != 0) {
#ifdef LOCK_DEBUG
		if(cpu == volatile_read(&lock->cpu)) {
			lock_panic(cpu, lock);
		}
#endif // LOCK_DEBUG
		while (volatile_read(&lock->lock) != 0) {
			pause();
		}
	}
#ifdef LOCK_DEBUG
	lock->cpu = cpu;
#endif // LOCK_DEBUG
	fence();
}

inline static void
nested_spinunlock(lock_t *lock)
{
#ifdef LOCK_DEBUG
	volatile_write(&lock->cpu, -1);
#endif
	fence();
	volatile_write(&lock->lock, 0);
}

#define spinlock(l, flags) do { \
	flags = irq_disable(); \
	nested_spinlock(l); \
} while (0)

#define spinunlock(l, flags) do { \
	nested_spinunlock(l); \
	irq_restore(flags); \
} while (0)


/* Running in Linux */
#define for_each_os_cpu(i)	for_each_cpu(i, bp->os_cpumask)
extern un active_cpumask; /* Initialized in hypervisor */
#define for_each_active_cpu(i)	for_each_cpu(i, active_cpumask)


inline static u64
__cpu_features(void)
{
	u32 a, b, c, d;

	a = 1; c = 0;
	cpuid(a, b, c, d);

	return (((u64)c << 32) | d);
}

#define CPU_FEATURE_SSE3	0	+ 32
#define CPU_FEATURE_MONITOR	3	+ 32
#define CPU_FEATURE_DSCPL	4	+ 32
#define CPU_FEATURE_VMX		5	+ 32
#define CPU_FEATURE_SMX		6	+ 32
#define CPU_FEATURE_X2APIC	21	+ 32
#define CPU_FEATURE_POPCNT	23	+ 32
#define CPU_FEATURE_XSAVE	26	+ 32
#define CPU_FEATURE_HYP		31	+ 32

inline static u64
__cpu_xfeatures(void)
{
	u32 a, b, c, d;

	a = 0x80000001; c = 0;
	cpuid(a, b, c, d);

	return (((u64)c << 32) | d);
}

#define CPU_XFEATURE_NXE	20
#define CPU_XFEATURE_INTEL64	29

#define CPU_XFEATURE_SVM	2	+ 32

inline static u8
__cpu_phys_addr_width(void)
{
	u32 a, b, c, d;

	a = 0x80000008; c = 0;
	cpuid(a, b, c, d);

	return bits(a, 7, 0);
}

inline static u8
__cpu_virt_addr_width(void)
{
	u32 a, b, c, d;

	a = 0x80000008; c = 0;
	cpuid(a, b, c, d);

	return bits(a, 15, 8);
}

inline static u16
__clflush_line_size(void)
{
	u32 a, b, c, d;

	a = 1; c = 0;
	cpuid(a, b, c, d);

	return bits(b, 15, 8) << 3;
}

typedef struct {
	u64	cpu_features;
	u64	cpu_xfeatures;
	u32	cpu_tsc_freq_kHz;
	u32	cpu_tsc_freq_MHz;
	u32	msr_pinbased_ctls;
	u32	msr_procbased_ctls;
	u32	msr_exit_ctls;
	u32	msr_entry_ctls;
	u32	msr_procbased_ctls2;
	u16	cpu_clflush_line_size;
	u8	cpu_phys_addr_width;
	u8	cpu_virt_addr_width;
	u8	cpu_type;
	u8	cpu_family;
	u8	cpu_model;
	u8	cpu_stepping;
} __attribute__((aligned(256))) cpuinfo_t;

extern cpuinfo_t cpuinfo;
extern void init_cpuinfo(void);

inline static bool
cpu_is_nehalem(void)
{
	return (cpuinfo.cpu_family == 0x06 &&
		(cpuinfo.cpu_model == 0x1a ||
		 cpuinfo.cpu_model == 0x1e ||
		 cpuinfo.cpu_model == 0x1f ||
		 cpuinfo.cpu_model == 0x25 ||
		 cpuinfo.cpu_model == 0x2e));
}

inline static bool
cpu_is_core2(void)
{
	return (cpuinfo.cpu_family == 0x06 &&
		(cpuinfo.cpu_model == 0x0f ||
		 cpuinfo.cpu_model == 0x17));
}

inline static bool
cpu_is_Intel(void)
{
	return (cpu_is_nehalem() || cpu_is_core2());
}

inline static bool
cpu_is_amd_10h(void)
{
	return (cpuinfo.cpu_family == 0x10);
}


inline static bool
cpu_is_AMD(void)
{
	return (cpu_is_amd_10h());
}

inline static u64
cpu_features(void)
{
	return cpuinfo.cpu_features;
}

inline static bool
cpu_feature_supported(u8 feature)
{
	return bit_test(cpu_features(), feature);
}

inline static bool
__cpu_feature_supported(u8 feature)
{
	return bit_test(__cpu_features(), feature);
}

inline static u64
cpu_xfeatures(void)
{
	return cpuinfo.cpu_xfeatures;
}

inline static bool
cpu_xfeature_supported(u8 xfeature)
{
	return bit_test(cpu_xfeatures(), xfeature);
}

inline static bool
__cpu_xfeature_supported(u8 xfeature)
{
	return bit_test(__cpu_xfeatures(), xfeature);
}

inline static u8
cpu_phys_addr_width(void)
{
	return cpuinfo.cpu_phys_addr_width;
}

inline static u8
cpu_virt_addr_width(void)
{
	return cpuinfo.cpu_virt_addr_width;
}

inline static u16
clflush_line_size(void)
{
	return cpuinfo.cpu_clflush_line_size;
}

#ifdef __x86_64__
inline static bool
is_canonical_addr(un addr)
{
	u8 msb = cpu_virt_addr_width() - 1;
	u8 width = 63 - msb + 1;
	un mask = ((1UL << width) - 1);
	un hi = bits(addr, 63, msb);

	return ((hi == 0) || (hi == mask));
}
#else
#define is_canonical_addr(addr)	true
#endif

inline static u64
get_MSR(u32 msr)
{
	un high, low;

	asm volatile("rdmsr" :
		     "=a" (low), "=d" (high) :
		     "c" (msr));
	return (((u64)high << 32) | low);
}

inline static void
set_MSR(u32 msr, u64 value)
{
	u32 high = bits(value, 63, 32);
	u32 low  = bits(value, 31, 0); 

	asm volatile("wrmsr" : : "a" (low), "d" (high), "c" (msr));
}

#define MSR_IA32_PLATFORM_ID		0x17
#define MSR_IA32_APIC_BASE		0x1b
#define AP_BSP				8
#define AP_ENABLE			11

#define MSR_IA32_FEATURE_CONTROL	0x3a

#define MSR_FSB_FREQ			0xcd
#define MSR_PLATFORM_INFO		0xce

#define MSR_IA32_MPERF			0xe7
#define MSR_IA32_APERF			0xe8

#define MSR_IA32_MTRRCAP		0xfe

#define MSR_IA32_SYSENTER_CS		0x174
#define MSR_IA32_SYSENTER_ESP		0x175
#define MSR_IA32_SYSENTER_EIP		0x176

#define MSR_IA32_MCG_STATUS		0x17a

#define MSR_IA32_PERF_STATUS		0x198
#define MSR_IA32_PERF_CTL		0x199

#define MSR_IA32_DEBUGCTL		0x1d9
#define DB_LBR				0
#define DB_BTF				1
#define DB_TR				6
#define DB_BTS				7
#define DB_BTINT			8
#define DB_BTS_OFF_OS			9
#define DB_BTS_OFF_USR			10
#define DB_FREEZE_LBRS_ON_PMI		11
#define DB_FREEZE_PERFMON_ON_PMI	12
#define DB_ENABLE_UNCORE_PMI		13
#define DB_FREEZE_WHILE_SMM		14

#define MSR_LASTBRANCHFROMIP		0x1db
#define MSR_LASTBRANCHTOIP		0x1dc
#define MSR_LASTINTFROMIP		0x1dd
#define MSR_LASTINTTOIP			0x1de

#define MSR_IA32_MTRR_PHYS_BASE0	0x200
#define MSR_IA32_MTRR_PHYS_MASK0	0x201
#define MSR_IA32_MTRR_PHYS_BASE1	0x202
#define MSR_IA32_MTRR_PHYS_MASK1	0x203
#define MSR_IA32_MTRR_PHYS_BASE2	0x204
#define MSR_IA32_MTRR_PHYS_MASK2	0x205
#define MSR_IA32_MTRR_PHYS_BASE3	0x206
#define MSR_IA32_MTRR_PHYS_MASK3	0x207
#define MSR_IA32_MTRR_PHYS_BASE4	0x208
#define MSR_IA32_MTRR_PHYS_MASK4	0x209
#define MSR_IA32_MTRR_PHYS_BASE5	0x20a
#define MSR_IA32_MTRR_PHYS_MASK5	0x20b
#define MSR_IA32_MTRR_PHYS_BASE6	0x20c
#define MSR_IA32_MTRR_PHYS_MASK6	0x20d
#define MSR_IA32_MTRR_PHYS_BASE7	0x20e
#define MSR_IA32_MTRR_PHYS_MASK7	0x20f

#define MTPM_VALID			11

#define MSR_IA32_MTRR_FIX64K_0000	0x250
#define MSR_IA32_MTRR_FIX16K_80000	0x258
#define MSR_IA32_MTRR_FIX16K_A0000	0x259
#define MSR_IA32_MTRR_FIX4K_C0000	0x268
#define MSR_IA32_MTRR_FIX4K_C8000	0x269
#define MSR_IA32_MTRR_FIX4K_D0000	0x26a
#define MSR_IA32_MTRR_FIX4K_D8000	0x26b
#define MSR_IA32_MTRR_FIX4K_E0000	0x26c
#define MSR_IA32_MTRR_FIX4K_E8000	0x26d
#define MSR_IA32_MTRR_FIX4K_F0000	0x26e
#define MSR_IA32_MTRR_FIX4K_F8000	0x26f

/* Memory caching types */
#define MT_UC	0
#define MT_WC	1
#define MT_WT	4
#define MT_WP	5
#define MT_WB	6

#define MSR_IA32_CR_PAT			0x277

#define MSR_IA32_DEF_TYPE		0x2ff
#define MT_E				11
#define MT_FE				10

#define MSR_IA32_PERF_GLOBAL_CTRL	0x38f

#define MSR_IA32_VMX_BASIC		0x480
#define MSR_IA32_VMX_PINBASED_CTLS	0x481
#define MSR_IA32_VMX_PROCBASED_CTLS	0x482
#define MSR_IA32_VMX_EXIT_CTLS		0x483
#define MSR_IA32_VMX_ENTRY_CTLS		0x484
#define MSR_IA32_VMX_MISC		0x485
#define MSR_IA32_VMX_CR0_FIXED0		0x486
#define MSR_IA32_VMX_CR0_FIXED1		0x487
#define MSR_IA32_VMX_CR4_FIXED0		0x488
#define MSR_IA32_VMX_CR4_FIXED1		0x489
#define MSR_IA32_VMX_VMCS_ENUM		0x48a
#define MSR_IA32_VMX_PROCBASED_CTLS2	0x48b
#define MSR_IA32_VMX_EPT_VPID_CAP	0x48c
#define MSR_IA32_VMX_TRUE_PINBASED_CTLS	0x48d
#define MSR_IA32_VMX_TRUE_PROCBASED_CTLS 0x48e
#define MSR_IA32_VMX_TRUE_EXIT_CTLS	0x48f
#define MSR_IA32_VMX_TRUE_ENTRY_CTLS	0x490

#define MSR_IA32_EXT_XAPICID		0x802

#define MSR_IA32_EFER			0xc0000080
#define EFER_SCE /* syscall enable */	0
#define EFER_LME /* x64 enable */	8
#define EFER_LMA /* x64 active */	10
#define EFER_NXE /* no-exec enable */	11
#define EFER_SVME			12 /* secure virtual machine enable */
#define EFER_LMSLE			13 /* x64 segment limit enable */
#define EFER_FFXSR			14 /* fast fxsave/fxrstore */

#define MSR_STAR			0xc0000081
#define MSR_LSTAR			0xc0000082
#define MSR_CSTAR			0xc0000083
#define MSR_SFMASK			0xc0000084

#define MSR_IA32_FS_BASE		0xc0000100
#define MSR_IA32_GS_BASE		0xc0000101
#define MSR_IA32_KERNEL_GS_BASE		0xc0000102

#define MSR_VM_CR			0xc0010114
#define MSR_VM_HSAVE_PA			0xc0010117

inline static un
get_DR6(void)
{
	un res;
	asm volatile("mov %%dr6, %0" :
		     "=r" (res) : : "cc");
	return res;
}

inline static un
get_DR7(void)
{
	un res;
	asm volatile("mov %%dr7, %0" :
		     "=r" (res) : : "cc");
	return res;
}

inline static un
get_CR0(void)
{
	un res;
	asm volatile("mov %%cr0, %0" :
		     "=r" (res) : : "cc");
	return res;
}

inline static void
set_CR0(un val)
{
	asm volatile("mov %0, %%cr0" : :
		     "r" (val) : "cc");
}

#define CR0_PG		31
#define CR0_CD		30
#define CR0_NW		29
#define CR0_AM		18
#define CR0_WP		16
#define CR0_NE		5
#define CR0_ET		4
#define CR0_TS		3
#define CR0_EM		2
#define CR0_MP		1
#define CR0_PE		0

inline static un
get_CR2(void)
{
	un res;
	asm volatile("mov %%cr2, %0" :
		     "=r" (res) : : "cc");
	return res;
}

inline static void
set_CR2(un val)
{
	asm volatile("mov %0, %%cr2" : :
		     "r" (val) : "cc");
}

inline static un
get_CR3(void)
{
	un res;
	asm volatile("mov %%cr3, %0" :
		     "=r" (res) : : "cc");
	return res;
}

inline static void
set_CR3(un val)
{
	asm volatile("mov %0, %%cr3" : :
		     "r" (val) : "cc");
}

inline static un
get_CR4(void)
{
	un res;
	asm volatile("mov %%cr4, %0" :
		     "=r" (res) : : "cc");
	return res;
}

inline static void
set_CR4(un val)
{
	asm volatile("mov %0, %%cr4" : :
		     "r" (val) : "cc");
}

#define CR4_VME		0
#define CR4_PVI		1
#define CR4_TSD		2
#define CR4_DE		3
#define CR4_PSE		4
#define CR4_PAE		5
#define CR4_MCE		6
#define CR4_PGE		7
#define CR4_PCE		8
#define CR4_OSFXSR	9
#define CR4_OSXMMEXCPT	10
#define CR4_VMXE	13
#define CR4_SMXE	14
#define CR4_OSXSAVE	18

inline static un
get_CS(void)
{
	un res;
	asm volatile("mov %%CS, %0" : "=r" (res));
	return res;
}

inline static un
get_DS(void)
{
	un res;
	asm volatile("mov %%DS, %0" : "=r" (res));
	return res;
}

inline static void
set_DS(u16 sel)
{
	asm volatile("mov %0, %%DS" : : "r" (sel));
}

inline static un
get_ES(void)
{
	un res;
	asm volatile("mov %%ES, %0" : "=r" (res));
	return res;
}

inline static void
set_ES(u16 sel)
{
	asm volatile("mov %0, %%ES" : : "r" (sel));
}

inline static un
get_FS(void)
{
	un res;
	asm volatile("mov %%FS, %0" : "=r" (res));
	return res;
}

inline static void
set_FS(u16 sel)
{
	asm volatile("mov %0, %%FS" : : "r" (sel));
}

inline static un
get_GS(void)
{
	un res;
	asm volatile("mov %%GS, %0" : "=r" (res));
	return res;
}

inline static void
set_GS(u16 sel)
{
	asm volatile("mov %0, %%GS" : : "r" (sel));
}

inline static un
get_SS(void)
{
	un res;
	asm volatile("mov %%SS, %0" : "=r" (res));
	return res;
}

inline static un
get_TR(void)
{
	un res;
	asm volatile("str %0" : "=r" (res));
	return res;
}

inline static void
set_TR(u16 sel)
{
	asm volatile("ltr %0" : : "r"(sel));
}

inline static un
get_LDT_sel(void)
{
	un res;
	asm volatile("sldt %0" : "=r" (res));
	return res;
}

inline static void
set_LDT_sel(u16 sel)
{
	asm volatile("lldt %0" : : "r"(sel));
}

#define RPL(sel)	bits(sel, 1, 0)
#define CPL()		RPL(get_CS())

typedef struct {
	u16	dt_limit;
	un	dt_base;
} __attribute__ ((packed)) dtr_t;

inline static void
get_GDTR(dtr_t *g)
{
	asm volatile("sgdt %0" :
		     "=m" (*g) : :
		     "memory");
}

inline static void
set_GDTR(dtr_t *g)
{
	asm volatile("lgdt %0" : :
		     "m" (*g));
}

inline static void
get_IDTR(dtr_t *g)
{
	asm volatile("sidt %0" :
		     "=m" (*g) : :
		     "memory");
}

inline static void
set_IDTR(dtr_t *g)
{
	asm volatile("lidt %0" : :
		     "m" (*g));
}

typedef struct {
	u16	seg_limit0;
	u16	seg_base0;
	u8	seg_base1;
	union {
		struct {
			u8 seg_type:4, seg_s:1, seg_dpl:2, seg_p:1;
			u8 seg_limit1:4, seg_avl:1, seg_l:1, seg_d:1, seg_g:1;
		};
		u16	seg_attr;
	};
	u8	seg_base2;
} __attribute__ ((packed)) seg_desc_t;

typedef struct {
	u16	seg_limit0;
	u16	seg_base0;
	u8	seg_base1;
	u16	seg_attr;
	u8	seg_base2;
	u32	seg_base3;
	u32	seg_reserved;
} __attribute__ ((packed)) seg_desc64_t;

#define SEG_UNUSABLE	(1 << 16)
#define ATTR_RESERVED	0xf00

#define SEG_DATA_R		0
#define SEG_DATA_RA		1
#define SEG_DATA_RW		2
#define SEG_DATA_RWA		3
#define SEG_DATA_R_XDOWN	4
#define SEG_DATA_RA_XDOWN	5
#define SEG_DATA_RW_XDOWN	6
#define SEG_DATA_RWA_XDOWN	7
#define SEG_CODE_X		8
#define SEG_CODE_XA		9
#define SEG_CODE_XR		10
#define SEG_CODE_XRA		11
#define SEG_CODE_X_CONFORM	12
#define SEG_CODE_XA_CONFORM	13
#define SEG_CODE_XR_CONFORM	14
#define SEG_CODE_XRA_CONFORM	15

#define SYS_SEG_TSS16_AVAILABLE	1
#define SYS_SEG_LDT		2
#define SYS_SEG_TSS16_BUSY	3
#define SYS_SEG_CALL_GATE16	4
#define SYS_SEG_TASK_GATE	5
#define SYS_SEG_INTR_GATE16	6
#define SYS_SEG_TRAP_GATE16	7
#define SYS_SEG_TSS_AVAILABLE	9
#define SYS_SEG_TSS_BUSY	11
#define SYS_SEG_CALL_GATE	12
#define SYS_SEG_INTR_GATE	14
#define SYS_SEG_TRAP_GATE	15

static inline addr_t
seg_desc_base(const seg_desc_t *s, bool target64)
{
	addr_t base = (((u32)(s->seg_base2) << 24) |
		       ((u32)(s->seg_base1) << 16) |
		       s->seg_base0);
#ifdef __x86_64__
	if ((s->seg_s == 0) && target64) {
		base |= ((u64)(((seg_desc64_t *)s)->seg_base3) << 32);
	}
#endif
	return base;
}

static inline u32
seg_desc_limit(const seg_desc_t *s)
{
	u32 limit = (((u32)(s->seg_limit1) << 16) | s->seg_limit0);
	if (s->seg_g) {
		limit = ((limit + 1) << VM_PAGE_SHIFT) - 1;
	}
	return limit;
}

extern addr_t get_seg_base(u16, addr_t, u16, bool);
extern u32 get_seg_limit(u16, addr_t, u16, bool);
extern u32 get_seg_attr(u16, addr_t, u16, bool);

extern addr_t get_seg_base_self(u16);
extern u32 get_seg_limit_self(u16);
extern u32 get_seg_attr_self(u16);

typedef struct {
	u16	id_offset0;
	u16	id_sel;
	union {
		u16	id_attr;
		struct {
#ifdef __x86_64__
			u8 id_ist:3, id_reserved0:5;
#else
			u8 id_reserved;
#endif
			u8 id_type:4, id_zero:1, id_dpl:2, id_p:1;
		};
	};
	u16	id_offset1;
#ifdef __x86_64__
	u32	id_offset2;
	u32	id_reserved1;
#endif
} __attribute__ ((packed)) intr_desc_t;

typedef struct {
	u32 tss_prev_task_link;
	u32 tss_rsp0;
	u32 tss_ss0;
	u32 tss_rsp1;
	u32 tss_ss1;
	u32 tss_rsp2;
	u32 tss_ss2;
	u32 tss_cr3;
	u32 tss_eip;
	u32 tss_eflags;
	u32 tss_eax;
	u32 tss_ecx;
	u32 tss_edx;
	u32 tss_ebx;
	u32 tss_esp;
	u32 tss_ebp;
	u32 tss_esi;
	u32 tss_edi;
	u32 tss_es;
	u32 tss_cs;
	u32 tss_ss;
	u32 tss_ds;
	u32 tss_fs;
	u32 tss_gs;
	u32 tss_ldtr;
	u16 tss_debug_trap;
	u16 tss_iomap_base;
} __attribute__ ((packed)) tss32_t;

typedef struct {
	u32	tss_reserved0;
	u64	tss_rsp0;
	u64	tss_rsp1;
	u64	tss_rsp2;
	u64	tss_reserved1;
	u64	tss_ist1;
	u64	tss_ist2;
	u64	tss_ist3;
	u64	tss_ist4;
	u64	tss_ist5;
	u64	tss_ist6;
	u64	tss_ist7;
	u64	tss_reserved2;
	u16	tss_reserved3;
	u16	tss_iomap_base;
} __attribute__ ((packed)) tss64_t;

#ifdef __x86_64__
typedef tss64_t tss_t;
#else
typedef tss32_t tss_t;
#endif

#define FLAGS_CF		0
#define FLAGS_PF		2
#define FLAGS_AF		4
#define FLAGS_ZF		6
#define FLAGS_SF		7
#define FLAGS_TF		8
#define FLAGS_IF		9
#define FLAGS_DF		10
#define FLAGS_OF		11
#define FLAGS_IOPL_SHIFT	12
#define FLAGS_IOPL_WIDTH		2
#define FLAGS_NT		14
#define FLAGS_RF		16
#define FLAGS_VM		17
#define FLAGS_AC		18
#define FLAGS_VIF		19
#define FLAGS_VIP		20
#define FLAGS_ID		21

inline static un
get_FLAGS(void)
{
	un flags;
	asm volatile("pushf; pop %0" :
		     "=g" (flags));
	return flags;
}

#ifdef __x86_64__
#define R "r"
#else
#define R "e"
#endif

inline static un __attribute__((always_inline))
get_RSP(void)
{
	un rsp;
	asm volatile("mov %%"R"sp, %0" :
		     "=r" (rsp));
	return rsp;
}

inline static bool
irq_is_disabled(void)
{
	un flags;
	asm volatile("pushf; pop %0" :
		     "=g" (flags));
		     
	return ((flags & bit(FLAGS_IF)) == 0);
}

inline static un
irq_disable(void)
{
	un flags;
	asm volatile("pushf; pop %0; cli" :
		     "=g" (flags) : :
		     "memory");
	return flags;
}

inline static void
irq_restore(un flags)
{
	asm volatile("push %0; popf" : :
		     "g" (flags) :
		     "memory", "cc");
}

inline static void
irq_enable(void)
{
	asm volatile("sti" : : : "memory");
}

typedef struct {
	union {
#ifdef __x86_64__
		un reg[16];
#else
		un reg[8];
#endif
		struct {
			un	rax;	/* 0 */
			un	rcx;	/* 1 */
			un	rdx;	/* 2 */
			un	rbx;	/* 3 */
			un	rsp;	/* 4 */
			un	rbp;	/* 5 */
			un	rsi;	/* 6 */
			un	rdi;	/* 7 */
#ifdef __x86_64__
			un	r8;
			un	r9;
			un	r10;
			un	r11;
			un	r12;
			un	r13;
			un	r14;
			un	r15;
#endif
		};
	};
#ifndef __x86_64__
	/* See the comment in restore_guest_to_vmx_root_mode() */
	un	guest_iret_rsp;
#endif
	un	guest_rip;
	un	guest_cs;
	un	guest_rflags;
	un	guest_rsp;
	un	guest_ss;
} registers_t;

typedef struct {
	union {
#ifdef __x86_64__
		un reg[16];
#else
		un reg[8];
#endif
		struct {
			un	rax;
			un	rcx;
			un	rdx;
			un	rbx;
			un	rsp;
			un	rbp;
			un	rsi;
			un	rdi;
#ifdef __x86_64__
			un	r8;
			un	r9;
			un	r10;
			un	r11;
			un	r12;
			un	r13;
			un	r14;
			un	r15;
#endif
		};
	};
	un	vector;
	un	errcode;
	un	rip;
	un	cs;
	un	rflags;
	un	user_rsp;
	un	user_ss;
} exc_frame_t;

#define VEC_DE		0	/* Divide by zero error */
#define VEC_DB		1	/* Debug */
#define VEC_NMI		2
#define VEC_BP		3
#define VEC_OF		4
#define VEC_BR		5
#define VEC_UD		6 	/* Invalid opcode */
#define VEC_NM		7
#define VEC_DF		8

#define VEC_TS		10
#define VEC_NP		11
#define VEC_SS		12
#define VEC_GP		13
#define VEC_PF		14

#define VEC_MF		16
#define VEC_AC		17
#define VEC_MC		18
#define VEC_XF		19

#define VEC_FIRST_USER  32	/* First USER defined vector */

inline static u64
rdtsc(void)
{
	u32 eax, edx;

	asm volatile("rdtsc" : "=a" (eax), "=d" (edx));
	return ((u64)edx << 32) | eax;
}

inline static u64
ms_to_cycles(un ms)
{
	return (u64)ms * cpuinfo.cpu_tsc_freq_kHz;
}

inline static un
cycles_to_ms(u64 cycles)
{
	return (cycles / cpuinfo.cpu_tsc_freq_kHz);
}

inline static u64
us_to_cycles(un us)
{
	return (u64)us * cpuinfo.cpu_tsc_freq_MHz;
}

inline static un
cycles_to_us(u64 cycles)
{
	return (cycles / cpuinfo.cpu_tsc_freq_MHz);
}

extern void mdelay(un);
extern void udelay(un);

#define TSC_TIMEOUT	ms_to_cycles(500)

extern un get_TSC_frequency(void);
extern u8 mtrr_type(un);

extern void flush_cache(addr_t, size_t);

#define _out(type, suffix)						\
    static inline void							\
    out##suffix(u16 port, type val)					\
    {									\
	asm volatile("out" #suffix " %0, %1" : : "a"(val), "d"(port));	\
    }

#define _in(type, suffix)						\
    static inline type							\
    in##suffix(u16 port)						\
    {									\
	type val;							\
	asm volatile("in" #suffix " %1, %0" : "=a"(val) : "d"(port));	\
	return val;							\
    }

_out(u8,  b)
_out(u16, w)
_out(u32, l)
_in(u8,  b)
_in(u16, w)
_in(u32, l)


#define _movs(type, suffix)						\
    static inline void							\
    movs##suffix(type *dest, const type *src, un count)			\
    {									\
	un S, D, c;							\
	asm volatile("cld\n"						\
		     "rep movs" #suffix "\n"				\
		     : "=c"(c), "=S"(S), "=D"(D)			\
		     : "c"(count), "S"(src), "D"(dest)			\
		     : "cc", "memory");					\
    }

#define _stos(type, suffix)						\
    static inline void							\
    stos##suffix(type *dest, type val, un count)			\
    {									\
	un D, c;							\
	asm volatile("cld\n"						\
		     "rep stos" #suffix	"\n"				\
		     : "=c"(c), "=D"(D)					\
		     : "c"(count), "D"(dest), "a"(val)			\
		     : "cc", "memory");					\
    }

_movs(u8,  b)
_movs(u16, w)
_movs(u32, l)

_stos(u8,  b)
_stos(u16, w)
_stos(u32, l)

#ifdef __x86_64__
_movs(u64, q)
_stos(u64, q)
#endif

inline static unsigned long
bit_count(unsigned long x)
{
	unsigned long res;
	if (cpu_feature_supported(CPU_FEATURE_POPCNT)) {
		asm volatile("popcnt %1, %0" : "=r" (res) : "r" (x) : "cc");
	} else {
		res = __bit_count(x);
	}
	return res;
}

#define xsetbv(ecx, eax, edx) \
{ \
	asm volatile("xsetbv" : : \
		     "a" (eax), "c" (ecx), "d" (edx)); \
}


extern u64 __cache_timing(int n);

static inline u64 cache_timing(int n)
{
	__cache_timing(1); // call function once to bring instr into i-cache
	return __cache_timing(n);
}

inline static void __attribute__((always_inline))
push(un x)
{
	asm volatile("push %0" : :
		     "r" (x));
}

inline static void __attribute__((always_inline))
pushf(void)
{
	asm volatile("pushf");
}

inline static void __attribute__((always_inline))
iret(void)
{
#ifdef __x86_64__
	asm volatile("iretq");
#else
	asm volatile("iret");
#endif
}

#endif
