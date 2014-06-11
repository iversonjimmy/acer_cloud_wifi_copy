#ifndef __EPT_H__
#define __EPT_H__

extern bool __cpu_supports_ept;

extern u32 epte_user_exec_except_cnt;
extern u32 epte_kern_exec_except_cnt;
extern u32 epte_user_write_except_cnt;
extern u32 epte_kern_write_except_cnt;

inline static bool
cpu_supports_ept(void)
{
	return __cpu_supports_ept;
}

extern bool __cpu_supports_vpid;
inline static bool
cpu_supports_vpid(void)
{
	return __cpu_supports_vpid;
}

typedef u64 epte_t;
#define EPT_R			0
#define EPT_W			1
#define EPT_X			2
#define EPT_IGNORE_PAT		6
#define EPT_2MB_PAGE		7
#define EPT_GUEST		57
#define EPT_W_UNTIL_X		58
#define EPT_R_SOFT		59
#define EPT_W_SOFT		60
#define EPT_X_SOFT		61
#define EPT_HYP			62
#define EPT_VALID		63

/* Memory type field */
#define EPT_MT_SHIFT		3
#define EPT_MT_WIDTH		3
/* Memory type field values - identical to MTRR types */
#define EPT_MT_UC		MT_UC
#define EPT_MT_WC		MT_WC
#define EPT_MT_WT		MT_WT
#define EPT_MT_WP		MT_WP
#define EPT_MT_WB		MT_WB

#define EPT_PERM_NONE		0ULL
#define EPT_PERM_R		(1ULL << EPT_R)
#define EPT_PERM_RW		((1ULL << EPT_R)|(1ULL << EPT_W))
#define EPT_PERM_RX		((1ULL << EPT_R)|(1ULL << EPT_X))
#define EPT_PERM_ALL		(EPT_PERM_RW|(1ULL << EPT_X))

/* Hardware conditional permission */
#define EPT_PERM_X		(1ULL << EPT_X)

/* Illegal permissions */
#define EPT_PERM_W		(1ULL << EPT_W)
#define EPT_PERM_WX		((1ULL << EPT_W)|(1ULL << EPT_X))

#define EPT_SOFT_ALL		(EPT_PERM_ALL << EPT_R_SOFT)

inline static u64
mkeptptr(un paddr)
{
	return (u64)(paddr | 6 | ((4 - 1) << 3));
}

inline static epte_t
mkepte(u64 paddr, un perms)
{
	u64 pfn_mask = ~((1ULL << VM_PAGE_SHIFT) - 1ULL);

	return (paddr & pfn_mask) | perms | (1ULL << EPT_VALID);
}

#define epte_to_paddr(epte)	((epte) & 0x000ffffffffff000ULL) 
#define epte_to_kv(epte)	((epte_t *)phystokv(epte_to_paddr(epte)))

#define EPT_XQ_ACCESS_READ		0
#define EPT_XQ_ACCESS_WRITE		1
#define EPT_XQ_ACCESS_EXECUTE		2
#define EPT_XQ_PERM_READ		3
#define EPT_XQ_PERM_WRITE		4
#define EPT_XQ_PERM_EXECUTE		5
#define EPT_XQ_GUEST_LADDR_VALID	7
#define EPT_XQ_NOT_PT_ACCESS		8

extern epte_t *ept_root;

#define el4_index(gpaddr)		bits((gpaddr), 47, 39);
#define el3_index(gpaddr)		bits((gpaddr), 38, 30);
#define el2_index(gpaddr)		bits((gpaddr), 29, 21);
#define el1_index(gpaddr)		bits((gpaddr), 20, 12);

inline static un
invept(un type, u64 eptptr)
{
	struct {
		u64 eptp;
		u64 rsvd;
	} memop = {eptptr, 0};
	u8 err;
	asm volatile("invept %1, %2\n\t"
		     "setbe %0" :
		     "=q" (err) :
		     "m" (memop), "r" (type) :
		     "cc", "memory");
	return err;
}
#define INVEPT_TYPE_SINGLE	1
#define INVEPT_TYPE_GLOBAL	2

inline static un
invvpid(un type, u16 vpid, un gpaddr)
{
	struct {
		u64 vpid;
		u64 gpaddr;
	} memop = {vpid, gpaddr};
	u8 err;
	asm volatile("invvpid %1, %2\n\t"
		     "setbe %0" :
		     "=q" (err) :
		     "m" (memop), "r" (type) :
		     "cc", "memory");
	return err;
}
#define INVVPID_TYPE_ADDRESS			0
#define INVVPID_TYPE_SINGLE			1
#define INVVPID_TYPE_ALL			2
#define INVVPID_TYPE_SINGLE_RETAIN_GLOBALS	3

extern bool vm_exit_ept(registers_t *);
extern void ept_destroy(void);
extern void ept_invalidate_global(void);
extern void ept_invalidate_addr(paddr_t);
extern ret_t ept_construct(void);
extern ret_t ept_protect_page(paddr_t, un, un);
#define EPTF_FLUSH	VMPF_FLUSH
#define EPTF_HYP	VMPF_HYP
#define EPTF_SOFT	VMPF_SOFT
#define EPTF_W_UNTIL_X	VMPF_W_UNTIL_X
#define EPTF_GUEST	VMPF_GUEST
#define EPTF_ALL	(EPTF_FLUSH | EPTF_HYP | EPTF_SOFT | EPTF_W_UNTIL_X | \
			 EPTF_GUEST)
extern ret_t gpaddr_to_epte(paddr_t, epte_t *);

#define ept_invalidate()	ept_invalidate_addr(0)

extern void ept_soft_perms_sync(void);

#endif
