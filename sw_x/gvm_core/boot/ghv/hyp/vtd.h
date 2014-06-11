#ifndef __VTD_H__
#define __VTD_H__

#include "list.h"
#include "guest.h"

#define VTD_VER			0x000	/* Version reg */
#define VTD_CAP 		0x008	/* Capability reg */
#define VTD_XCAP 		0x010	/* Extended capability reg */
#define VTD_GC	 		0x018	/* Global command reg */
#define VTD_GS 			0x01c	/* Global status reg */
#define VTD_RTA 		0x020	/* Root-entry table address reg */
#define VTD_CC	 		0x028	/* Context command reg */
#define VTD_FS	 		0x034	/* Fault status reg */
#define VTD_FEC 		0x038	/* Fault event control reg */
#define VTD_FED 		0x03c	/* Fault event data reg */
#define VTD_FEA 		0x040	/* Fault event address reg */
#define		VTD_FEA_DEST_SHIFT	12
#define VTD_FEA1 		0x044	/* Fault event upper address reg */
#define VTD_AFL 		0x058	/* Advanced fault log reg */
#define VTD_PME 		0x064	/* Protected memory enable reg */
#define VTD_PLMB 		0x068	/* Protected low memory base reg */
#define VTD_PLML 		0x06c	/* Protected low memory limit reg */
#define VTD_PHMB 		0x070	/* Protected high memory base reg */
#define VTD_PHML 		0x078	/* Protected high memory limit reg */
#define VTD_IQH 		0x080	/* Invalidation queue head reg */
#define VTD_IQT 		0x088	/* Invalidation queue tail reg */
#define VTD_IQA 		0x090	/* Invalidation queue address reg */
#define VTD_ICS 		0x09c	/* Invalidation completion status reg */
#define VTD_ICEC 		0x0a0	/* Inval. compl. event control reg */
#define VTD_ICED 		0x0a4	/* Inval. compl. event data reg */
#define VTD_ICEA 		0x0a8	/* Inval. compl. event address */
#define VTD_ICEA1 		0x0ac	/* Inval. compl. event upper address */
#define VTD_IRTA 		0x0b8	/* Interrupt remapping table address */

#define VTD_CAP_DRD		55	/* DMA read draining required */
#define VTD_CAP_DWD		54	/* DMA write draining required */
#define VTD_CAP_MAMV_HI		53
#define VTD_CAP_MAMV		48	/* Max. address value mask */
#define VTD_CAP_NFR_HI		47
#define VTD_CAP_NFR		40	/* No. of fault recording regs. */
#define VTD_CAP_PSI		39	/* Page selective invalidation */
#define VTD_CAP_SPS_HI		37
#define VTD_CAP_SPS		34	/* Super page support */
#define VTD_CAP_FRO_HI		33
#define VTD_CAP_FRO		24	/* Fault-recording register offset */
#define VTD_CAP_ISOCH		23	/* Isochrony */
#define VTD_CAP_ZLR		22	/* Zero length read */
#define VTD_CAP_MGAW_HI		21
#define VTD_CAP_MGAW		16	/* Maximum guest address width */
#define VTD_CAP_SAGAW_HI	12
#define VTD_CAP_SAGAW		8	/* Supptd. adjstd. guest addr. widths */
#define		VTD_AGAW_30	0	/* 2-level page table */
#define		VTD_AGAW_39	1	/* 3-level page table */
#define		VTD_AGAW_48	2	/* 4-level page table */
#define		VTD_AGAW_57	3	/* 5-level page table */
#define		VTD_AGAW_64	4	/* 6-level page table */
#define VTD_CAP_CM		7	/* Caching mode */
#define VTD_CAP_PHMR		6	/* Protected high memory region */
#define VTD_CAP_PLMR		5	/* Protected low memory region */
#define VTD_CAP_RWBF		4	/* Requires write buffer flushing */
#define VTD_CAP_AFL		3	/* Advanced fault logging */
#define VTD_CAP_ND_HI		2
#define VTD_CAP_ND		0	/* Number of domains supported */

#define VTD_XCAP_MHMV_HI	23
#define VTD_XCAP_MHMV		20	/* Maximun handle mask value */
#define VTD_XCAP_IRO_HI		17
#define VTD_XCAP_IRO		8	/* IOTLB register offset */
#define VTD_XCAP_SC		7	/* Snoop control */
#define VTD_XCAP_PT		6	/* Pass through */
#define VTD_XCAP_CH		5	/* Caching hints */
#define VTD_XCAP_EIM		4	/* Extended interrupt mode */
#define VTD_XCAP_IR		3	/* Interrupt remapping support */
#define VTD_XCAP_DI		2	/* Device IOTLB support */
#define VTD_XCAP_QI		1	/* Queued invalidation support */
#define VTD_XCAP_C		0	/* Coherency */

#define VTD_GC_TE		31	/* Translation enable */
#define VTD_GC_SRTP		30	/* Set root table pointer */
#define VTD_GC_SFL		29	/* Set fault log */
#define VTD_GC_EAFL		28	/* Enable advanced fault logging */
#define VTD_GC_WBF		27	/* Write buffer flush */
#define VTD_GC_QIE		26	/* Queued invalidate enable */
#define VTD_GC_IRE		25	/* Interrupt remapping enable */
#define VTD_GC_SIRTP		24	/* Set interrupt remap table pointer */
#define VTD_GC_CFI		23	/* Compatibility format interrupt */

#define VTD_GS_TES		31	/* Translation enable status */
#define VTD_GS_RTPS		30	/* Root table pointer status */
#define VTD_GS_FLS		29	/* Fault log status */
#define VTD_GS_AFLS		28	/* Advanced fault logging status */
#define VTD_GS_WBFS		27	/* Write buffer flush status */
#define VTD_GS_QIES		26	/* Queued invalidation enable status */
#define VTD_GS_IRES		25	/* Intr. remapping enable status */
#define VTD_GS_IRTPS		24	/* Intr. remapping table ptr status */
#define VTD_GS_CFIS		23	/* Compat. format interrupt status */

#define VTD_TABLE_LEN		256

typedef struct {
	u64	lo;
	u64	reserved;
} root_entry_t;

#define VTD_PRESENT	0x1

inline static u64
mkrte_lo(u64 paddr)
{
	u64 pfn_mask = ~((1ULL << VM_PAGE_SHIFT) - 1ULL);

	return ((paddr & pfn_mask) | VTD_PRESENT);
}

typedef struct {
	u64	lo;
	u64	hi;
} context_entry_t;

inline static u64
mkcxe_lo(u64 paddr)
{
	u64 pfn_mask = ~((1ULL << VM_PAGE_SHIFT) - 1ULL);

	return ((paddr & pfn_mask) | VTD_PRESENT);
}

#define VTD_DOMAIN_ID	((u64)GUEST_ID)

#define CXE_DID_SHIFT	8

#define CXE_AW_30	0x0
#define CXE_AW_39	0x1
#define CXE_AW_48	0x2
#define CXE_AW_57	0x3
#define CXE_AW_64	0x4

inline static u64
mkcxe_hi(u16 domain_id, u8 address_width)
{
	return ((u64)domain_id << CXE_DID_SHIFT) | address_width;
}

typedef struct {
	u64	lo;
	u64	hi;
} frr_t;

typedef struct {
	list_t		iom_list;
	lock_t		iom_lock;
	un		iom_base;
	un		iom_iotlb_base;
	frr_t		*iom_frr;
	root_entry_t	*iom_root;
	context_entry_t	*iom_context;
	u64		iom_cap;
	u64		iom_xcap;
	u64		iom_old_fault;
	u64		iom_old_address;
	u64		iom_last_print;
	u32		iom_gc;
	u16		iom_nfr;
} iommu_t;

#define VTD_CC_ICC	63
#define VTD_CC_CIRG	61
#define		VTD_CC_CIRG_GLOBAL	(1ULL << VTD_CC_CIRG)
#define		VTD_CC_CIRG_DOMAIN	(2ULL << VTD_CC_CIRG)
#define		VTD_CC_CIRG_DEVICE	(3ULL << VTD_CC_CIRG)
#define VTD_CC_CAIG	59

#define VTD_TLBA		0x00	/* IOTLB invalidate address reg. */
#define VTD_TLB			0x08	/* IOTLB invalidate control reg. */

#define VTD_TLBA_IH		6	/* Invalidation hint */

#define VTD_TLB_IVT		63	/* Invalidate IOTLB */
#define VTD_TLB_IRG_HI		61
#define VTD_TLB_IRG		60	/* Invalidation request granularity */
#define		VTD_TLB_IRG_GLOBAL	(1ULL << VTD_TLB_IRG)
#define		VTD_TLB_IRG_DOMAIN	(2ULL << VTD_TLB_IRG)
#define		VTD_TLB_IRG_PAGE	(3ULL << VTD_TLB_IRG)
#define VTD_TLB_AIG_HI		58
#define VTD_TLB_AIG		57	/* Actual invalidation granularity */
#define VTD_TLB_DR		49	/* Drain reads */
#define VTD_TLB_DW		48	/* Drain writes */
#define VTD_TLB_DID_HI		47
#define VTD_TLB_DID		32	/* Domain ID */

#define VTD_FS_FRI_HI		15
#define VTD_FS_FRI		8	/* Fault record index */
#define VTD_FS_ITE		6	/* Invalidation time-out error */
#define VTD_FS_ICE		5	/* Invalidation completion error */
#define VTD_FS_IQE		4	/* Invalidation queue error */
#define VTD_FS_APF		3	/* Advanced pending fault */
#define VTD_FS_AFO		2	/* Advanced fault overflow */
#define VTD_FS_PPF		1	/* Primary pending fault */
#define VTD_FS_PFO		0	/* Primary fault overflow */

#define VTD_FRR1_F	(127-64)	/* Fault logged */
#define VTD_FRR1_T	(126-64)	/* Type 0=Write 1=Read/AtomicOp */
#define VTD_FRR1_AT_HI	(125-64)
#define VTD_FRR1_AT	(124-64)	/* Address type */
#define VTD_FRR1_FR_HI	(103-64)
#define VTD_FRR1_FR	(96-64)		/* Fault reason */
#define VTD_FRR1_SID_HI	(79-64)
#define VTD_FRR1_SID	(64-64)		/* Source ID */
#define VTD_FRR_FI_HI	63
#define VTD_FRR_FI	12		/* Fault info */

extern bool iommu_flush_tables;

inline static void
iommu_flush_cache(addr_t addr, size_t len)
{
	if (iommu_flush_tables) {
		flush_cache(addr, len);
	}
}

extern ret_t vtd_construct(void);
extern void vtd_destroy(void);
extern void vtd_invalidate_page(paddr_t);
extern void vtd_invalidate(void);
extern void vtd_enable_fault_interrupts(void);
extern void vtd_enable(void);

#endif
