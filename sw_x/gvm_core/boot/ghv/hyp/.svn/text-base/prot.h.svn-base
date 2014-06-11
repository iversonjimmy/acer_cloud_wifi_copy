#ifndef __PROT_H__
#define __PROT_H__

extern ret_t vm_protect_page(paddr_t, un, un);
#define VMPF_FLUSH	0x0001
#define VMPF_HYP	0x0002	/* Mark page as belonging to hypervisor */
#define VMPF_SOFT	0x0004	/* Also set soft permissions */
#define VMPF_W_UNTIL_X	0x0008	/* Allow Write until first (allowed) eXecute */
#define VMPF_GUEST	0x0010	/* Mark page as belonging to guest */
#define VMPF_ALL	(VMPF_FLUSH | VMPF_HYP | VMPF_SOFT | VMPF_W_UNTIL_X | \
			 VMPF_GUEST)

extern void vm_flush(void);
extern void vm_soft_perms_sync(void);

extern ret_t protect_memory_vaddr(addr_t, size_t, un, un);
extern ret_t protect_memory_guest_vaddr(addr_t, size_t, un, un);
extern ret_t protect_memory_phys_addr(paddr_t, size_t, un, un);
extern void protect_hypervisor_memory(void);
extern bool vm_check_protection(paddr_t, size_t, un);

extern bool guest_phys_page_readable(u32, paddr_t);
extern bool guest_phys_page_writeable(u32, paddr_t);

extern bool enable_nx;
extern un nx_anomaly_count;

inline static void
vm_enable_nx(void)
{
	nx_anomaly_count = 0;
	enable_nx = true;
}

inline static void
vm_disable_nx(void)
{
	enable_nx = false;
}

inline static bool
vm_nx_is_enabled(void)
{
	return enable_nx;
}

#endif
