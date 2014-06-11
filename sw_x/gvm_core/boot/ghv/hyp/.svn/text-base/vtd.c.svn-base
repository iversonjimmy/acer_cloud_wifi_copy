#include "hyp.h"
#include "kvtophys.h"
#include "ept.h"
#include "interrupt.h"
#include "valloc.h"
#include "vtd.h"

#define vtd_cap(x) (bit_test(cap, x) ? \
		    kprintf(#x "\n") : \
		    kprintf("not " #x "\n"))
#define vtd_xcap(x) (bit_test(xcap, x) ? \
		     kprintf(#x "\n") : \
		     kprintf("not " #x "\n"))

static void
vtd_dump_cap(u64 cap)
{
	vtd_cap(VTD_CAP_DRD);
	vtd_cap(VTD_CAP_DWD);
	if (bit_test(cap, VTD_CAP_PSI)) {
		kprintf("VTD_CAP_MAMV = %ld\n", bits(cap, VTD_CAP_MAMV_HI,
						    VTD_CAP_MAMV));
	}
	kprintf("VTD_CAP_NFR = %ld\n", bits(cap, VTD_CAP_NFR_HI, VTD_CAP_NFR));
	vtd_cap(VTD_CAP_PSI);
	kprintf("VTD_CAP_SPS = %ld\n", bits(cap, VTD_CAP_SPS_HI, VTD_CAP_SPS));
	kprintf("VTD_CAP_FRO = 0x%lx\n", bits(cap, VTD_CAP_FRO_HI, VTD_CAP_FRO));
	vtd_cap(VTD_CAP_ISOCH);
	vtd_cap(VTD_CAP_ZLR);
	kprintf("VTD_CAP_MGAW = %ld\n", bits(cap, VTD_CAP_MGAW_HI,
					    VTD_CAP_MGAW));
	kprintf("VTD_CAP_SAGAW = 0x%lx\n", bits(cap, VTD_CAP_SAGAW_HI,
					       VTD_CAP_SAGAW));
	vtd_cap(VTD_CAP_CM);
	vtd_cap(VTD_CAP_PHMR);
	vtd_cap(VTD_CAP_PLMR);
	vtd_cap(VTD_CAP_RWBF);
	vtd_cap(VTD_CAP_AFL);
	kprintf("VTD_CAP_ND = 0x%lx\n", bits(cap, VTD_CAP_ND_HI, VTD_CAP_ND));
}

static void
vtd_dump_xcap(u64 xcap)
{
	if (bit_test(xcap, VTD_XCAP_IR)) {
		kprintf("VTD_XCAP_MHMV = %ld\n", bits(xcap, 23, 20));
	}
	kprintf("VTD_XCAP_IRO = 0x%lx\n", bits(xcap, 17, 8));
	vtd_xcap(VTD_XCAP_SC);
	vtd_xcap(VTD_XCAP_PT);
	vtd_xcap(VTD_XCAP_CH);
	if (bit_test(xcap, VTD_XCAP_IR)) {
		vtd_xcap(VTD_XCAP_EIM);
	}
	vtd_xcap(VTD_XCAP_IR);
	vtd_xcap(VTD_XCAP_DI);
	vtd_xcap(VTD_XCAP_QI);
	vtd_xcap(VTD_XCAP_C);
}

static void
vtd_context_cache_invalidate(iommu_t *iommu)
{
	u64 *cc = (u64 *)(iommu->iom_base + VTD_CC);
	u64 cmd = bit(VTD_CC_ICC) | VTD_CC_CIRG_GLOBAL; 

	un flags;
	spinlock(&iommu->iom_lock, flags);
	volatile_write(cc, cmd);

	u64 res;
	while (res = volatile_read(cc), bit_test(res, VTD_CC_ICC)) {
		pause();
	}
	spinunlock(&iommu->iom_lock, flags);
}

static void
vtd_iotlb_invalidate(iommu_t *iommu)
{
	u64 *tlb = (u64 *)(iommu->iom_iotlb_base + VTD_TLB);
	u64 cmd = bit(VTD_TLB_IVT) | VTD_TLB_IRG_GLOBAL; 
	if (bit_test(iommu->iom_cap, VTD_CAP_DRD)) {
		cmd |= VTD_TLB_DR;
	}
	if (bit_test(iommu->iom_cap, VTD_CAP_DWD)) {
		cmd |= VTD_TLB_DW;
	}

	un flags;
	spinlock(&iommu->iom_lock, flags);
	volatile_write(tlb, cmd);

	u64 res;
	while (res = volatile_read(tlb), bit_test(res, VTD_TLB_IVT)) {
		pause();
	}
	spinunlock(&iommu->iom_lock, flags);
}

static void
vtd_iotlb_invalidate_page(iommu_t *iommu, paddr_t addr)
{
	u64 *tlb = (u64 *)(iommu->iom_iotlb_base + VTD_TLB);
	u64 *tlb_addr = (u64 *)(iommu->iom_iotlb_base + VTD_TLBA);
	u64 invaddr = TRUNC_PAGE(addr) | bit(VTD_TLBA_IH);
	u64 cmd = (bit(VTD_TLB_IVT) | VTD_TLB_IRG_PAGE |
		   (VTD_DOMAIN_ID << VTD_TLB_DID)); 
	if (bit_test(iommu->iom_cap, VTD_CAP_DRD)) {
		cmd |= VTD_TLB_DR;
	}
	if (bit_test(iommu->iom_cap, VTD_CAP_DWD)) {
		cmd |= VTD_TLB_DW;
	}

	un flags;
	spinlock(&iommu->iom_lock, flags);
	volatile_write(tlb_addr, invaddr);
	memory();
	volatile_write(tlb, cmd);

	u64 res;
	while (res = volatile_read(tlb), bit_test(res, VTD_TLB_IVT)) {
		pause();
	}
	spinunlock(&iommu->iom_lock, flags);
}

static void
vtd_set_root_table_pointer(iommu_t *iommu)
{
	u64 paddr = (u64)kvtophys(iommu->iom_root);
	u64 *rta = (u64 *)(iommu->iom_base + VTD_RTA);

	un flags;
	spinlock(&iommu->iom_lock, flags);
	volatile_write(rta, paddr);
	memory();

	u32 *gc = (u32 *)(iommu->iom_base + VTD_GC);
	u32 cmd = iommu->iom_gc | bit(VTD_GC_SRTP);
	volatile_write(gc, cmd);
	memory();

	u32 *gs = (u32 *)(iommu->iom_base + VTD_GS);
	u32 res;
	while (res = volatile_read(gs), !bit_test(res, VTD_GS_RTPS)) {
		pause();
	}
	spinunlock(&iommu->iom_lock, flags);
}

static void
vtd_translation_enable(iommu_t *iommu)
{
	u32 *gc = (u32 *)(iommu->iom_base + VTD_GC);
	u32 cmd = iommu->iom_gc | bit(VTD_GC_TE);

	un flags;
	spinlock(&iommu->iom_lock, flags);
	volatile_write(gc, cmd);
	memory();

	iommu->iom_gc = cmd;

	u32 *gs = (u32 *)(iommu->iom_base + VTD_GS);
	u32 res;
	while (res = volatile_read(gs), !bit_test(res, VTD_GS_TES)) {
		pause();
	}
	spinunlock(&iommu->iom_lock, flags);
}

static void
vtd_translation_disable(iommu_t *iommu)
{
	u32 *gs = (u32 *)(iommu->iom_base + VTD_GS);

	un flags;
	spinlock(&iommu->iom_lock, flags);
	u32 res = volatile_read(gs);
	if (!bit_test(res, VTD_GS_TES)) {
		spinunlock(&iommu->iom_lock, flags);
		return;
	}

	kprintf("vtd_translation_disable>Disabling translation\n");

	memory();

	u32 *gc = (u32 *)(iommu->iom_base + VTD_GC);
	u32 cmd = bit_clear(iommu->iom_gc, VTD_GC_TE);

	volatile_write(gc, cmd);
	memory();

	while (res = volatile_read(gs), bit_test(res, VTD_GS_TES)) {
		pause();
	}
	spinunlock(&iommu->iom_lock, flags);
}

static void
vtd_write_buffer_flush(iommu_t *iommu)
{
	if (!bit_test(iommu->iom_cap, VTD_CAP_RWBF)) {
		return;
	}
	u32 *gc = (u32 *)(iommu->iom_base + VTD_GC);
	u32 cmd = iommu->iom_gc | bit(VTD_GC_WBF);

	un flags;
	spinlock(&iommu->iom_lock, flags);
	volatile_write(gc, cmd);
	memory();

	u32 *gs = (u32 *)(iommu->iom_base + VTD_GS);
	u32 res;
	while (res = volatile_read(gs), bit_test(res, VTD_GS_WBFS)) {
		pause();
	}
	spinunlock(&iommu->iom_lock, flags);
}

static char *fault_reason[] = {
	"Reserved",
	"Root entry not Present",
	"Context entry not Present",
	"Context entry invalid",
	"Address out of range",
	"Write protected",
	"Read protected",
	"Page table address error",
	"Root table address error",
	"Context table address error",
	"Reserved fields in Root entry",
	"Reserved fields in Context entry",
	"Reserved fields in page table entry",
	"Translation type blocked in Context entry",
};
#define FAULT_TABLE_LEN (sizeof(fault_reason)/sizeof(char *))

static void
vtd_dump_fault(iommu_t *iommu)
{
	un flags;
	spinlock(&iommu->iom_lock, flags);
	u32 fs = volatile_read((u32 *)(iommu->iom_base + VTD_FS));

	bool fault_pending = bit_test(fs, VTD_FS_PPF);
	if (!fault_pending) {
		spinunlock(&iommu->iom_lock, flags);
		return;
	}

	u16 index = bits(fs, VTD_FS_FRI_HI, VTD_FS_FRI);

	u64 fault;
	while (fault = volatile_read(&iommu->iom_frr[index].hi),
				     bit_test(fault, VTD_FRR1_F)) {
		u64 address = volatile_read(&iommu->iom_frr[index].lo);

		if (fault != iommu->iom_old_fault ||
		    address != iommu->iom_old_address ||
		    rdtsc() > (iommu->iom_last_print + TSC_TIMEOUT)) {
			u16 reason = bits(fault, VTD_FRR1_FR_HI, VTD_FRR1_FR);
			u16 sid = bits(fault, VTD_FRR1_SID_HI, VTD_FRR1_SID);

			kprintf("VT-D Fault %d:%s %s 0x%llx SID 0x%x\n",
				reason, 
				reason < FAULT_TABLE_LEN ? fault_reason[reason] :
				"unknown",
				bit_test(fault, VTD_FRR1_T) ? "R" : "W",
				address, sid);
			iommu->iom_last_print = rdtsc();
		}

		iommu->iom_old_fault = fault;
		iommu->iom_old_address = address;

		/* Clear it */
		volatile_write(&iommu->iom_frr[index].hi, fault);

		if (++index >= iommu->iom_nfr) {
			index = 0;
		}
	}

	/* Clear any status bits */
	volatile_write((u32 *)(iommu->iom_base + VTD_FS), fs);
	spinunlock(&iommu->iom_lock, flags);
}

static void
vtd_init_fault_interrupt(iommu_t *iommu, u8 vector)
{
	kprintf("vtd_init_fault_interrupt>ENTER vector = %d\n", vector);
	u32 dest = 0;

	un flags;
	spinlock(&iommu->iom_lock, flags);
	volatile_write((u32 *)(iommu->iom_base + VTD_FED), vector);
	volatile_write((u32 *)(iommu->iom_base + VTD_FEA),
		       0xFEE00000|(dest << VTD_FEA_DEST_SHIFT));
	volatile_write((u32 *)(iommu->iom_base + VTD_FEA1), 0);
	memory();
	volatile_write((u32 *)(iommu->iom_base + VTD_FEC), 0);
	spinunlock(&iommu->iom_lock, flags);
}

static void
vtd_free_iommu(iommu_t *iom)
{
	assert(iom);
	page_free((un)iom->iom_root);
	page_free((un)iom->iom_context);
	unmap_page(iom->iom_base, 0);
	free(iom);
}

static bool all_iommus_cache_coherent = true;

iommu_t *
vtd_alloc_iommu(un iommu_base)
{
	iommu_t *iom = alloc(sizeof *iom);
	if (!iom) {
		return iom;
	}

	list_init(&iom->iom_list);
	lock_init(&iom->iom_lock);

	iom->iom_base = iommu_base;

	u64 cap = volatile_read((u64 *)(iommu_base + VTD_CAP));
	u64 xcap = volatile_read((u64 *)(iommu_base + VTD_XCAP));

	iom->iom_cap = cap;
	iom->iom_xcap = xcap;

	u32 version = volatile_read((u32 *)(iommu_base + VTD_VER));
	kprintf("vtd_alloc_iommu>version = 0x%x\n", version);
	kprintf("vtd_alloc_iommu>cap = 0x%llx\n", iom->iom_cap);
	vtd_dump_cap(iom->iom_cap);

	kprintf("vtd_alloc_iommu>xcap = 0x%llx\n", iom->iom_xcap);
	vtd_dump_xcap(iom->iom_xcap);

	iom->iom_nfr = bits(cap, VTD_CAP_NFR_HI, VTD_CAP_NFR) + 1;
	kprintf("vtd_alloc_iommu>number of fault registers = %d\n",
		iom->iom_nfr);

	un frr_offset = bits(cap, VTD_CAP_FRO_HI, VTD_CAP_FRO);
	kprintf("vtd_alloc_iommu>frr_offset = 0x%lx\n", frr_offset);

	iom->iom_frr = (frr_t *)(iommu_base + (frr_offset << 4));
	kprintf("vtd_alloc_iommu>frr = 0x%p\n", iom->iom_frr);

	un tlb_offset = bits(xcap, VTD_XCAP_IRO_HI, VTD_XCAP_IRO);
	kprintf("vtd_alloc_iommu>tlb_offset = 0x%lx\n", tlb_offset);

	iom->iom_iotlb_base = iommu_base + (tlb_offset << 4);
	kprintf("vtd_alloc_iommu>iotlb_base = 0x%lx\n", iom->iom_iotlb_base);

	assert(iom->iom_root == NULL);
	assert(iom->iom_context == NULL);

	iom->iom_root = (root_entry_t *)page_alloc();
	if (iom->iom_root == NULL) {
		goto fail;
	}

	iom->iom_context = (context_entry_t *)page_alloc();
	if (iom->iom_context == NULL) {
		goto fail;
	}

	u8 sagaw = bits(cap, VTD_CAP_SAGAW_HI, VTD_CAP_SAGAW);
	epte_t *page_table_root;
	u8 addr_width;
	if (bit_test(sagaw, VTD_AGAW_48)) {
		page_table_root = ept_root;
		addr_width = CXE_AW_48;
		kprintf("vtd_alloc_iommu>AGAW 48, page_table_root 0x%p\n",
			page_table_root);
	} else if (bit_test(sagaw, VTD_AGAW_39)) {
		page_table_root = epte_to_kv(ept_root[0]);
		addr_width = CXE_AW_39;
		kprintf("vtd_alloc_iommu>AGAW 39, page_table_root 0x%p\n",
			page_table_root);
	} else {
		kprintf("vtd_alloc_iommu>unsupported AGAW, SAGAW = 0x%x\n",
			sagaw);
		goto fail;
	}

	for (un i = 0; i < VTD_TABLE_LEN; i++) {
		iom->iom_root[i].lo = mkrte_lo(kvtophys(iom->iom_context));

	}
	iommu_flush_cache((un)iom->iom_root, VM_PAGE_SIZE);

	for (un i = 0; i < VTD_TABLE_LEN; i++) {
		iom->iom_context[i].hi = mkcxe_hi(VTD_DOMAIN_ID, addr_width);
		iom->iom_context[i].lo = mkcxe_lo(kvtophys(page_table_root));
	}
	iommu_flush_cache((un)iom->iom_context, VM_PAGE_SIZE);

	all_iommus_cache_coherent = (all_iommus_cache_coherent &&
				     bit_test(xcap, VTD_XCAP_C));

	return iom;

fail:
	vtd_free_iommu(iom);
	return NULL;
}

LIST_INIT_STATIC(iommus);
bool iommu_flush_tables = true;

static void
vtd_dump_fault_all(void)
{
	if (list_is_empty(&iommus)) {
		kprintf("vtd_dump_fault_all>iommus list is empty \n");
		return;
	}

	FOR_ALL(iommus, elem) {
		iommu_t *iommu = super(elem, iommu_t, iom_list);
		vtd_dump_fault(iommu);
	}
}

ret_t
vtd_construct(void)
{
	kprintf("vtd_construct>ENTRY\n");

	if (bp->niommus == 0) {
		kprintf("vtd_construct>IOMMUs not found\n");
		return /* -ENXIO */ 0;
	}

	if (bp->bp_flags & BPF_NO_VTD) {
		kprintf("vtd_construct>BPF_NO_VTD\n");
		return 0;
	}

	for (un i = 0; i < bp->niommus; i++) {
		un iommu_base = map_page(bp->iommu_base[i], MPF_IO);
		iommu_t *iom = vtd_alloc_iommu(iommu_base);
		if (!iom) {
			unmap_page(iommu_base, 0);
			return -ENOMEM;
		}

		list_add_tail(&iommus, &iom->iom_list);
	}

	iommu_flush_tables = !all_iommus_cache_coherent;

	kprintf("vtd_construct>iommu_flush_tables is %s\n",
		iommu_flush_tables ? "true" : "false");

	kprintf("vtd_construct>EXIT\n");
	return 0;
}

void
vtd_enable()
{
	FOR_ALL(iommus, elem) {
		iommu_t *iommu = super(elem, iommu_t, iom_list);

		kprintf("vtd_enable>vtd_write_buffer_flush\n");
		vtd_write_buffer_flush(iommu);

		kprintf("vtd_enable>vtd_set_root_table_pointer\n");
		vtd_set_root_table_pointer(iommu);

		kprintf("vtd_enable>vtd_context_cache_invalidate\n");
		vtd_context_cache_invalidate(iommu);
		kprintf("vtd_enable>vtd_iotlb_invalidate\n");
		vtd_iotlb_invalidate(iommu);

		kprintf("vtd_enable>vtd_translation_enable\n");
		vtd_translation_enable(iommu);

		memory();

		vtd_dump_fault(iommu);
	}

	vtd_enable_fault_interrupts();
}

void
vtd_destroy(void)
{
	kprintf("vtd_destroy>ENTRY\n");
	if (list_is_empty(&iommus)) {
		return;
	}

	FOR_ALL(iommus, elem) {
		iommu_t *iommu = super(elem, iommu_t, iom_list);
		vtd_translation_disable(iommu);
	}

	list_t *elem;
	while ((elem = list_remove_head(&iommus)) != NULL) {
		iommu_t *iommu = super(elem, iommu_t, iom_list);
		vtd_free_iommu(iommu);
	}
}

void
vtd_invalidate_page(paddr_t paddr)
{
	FOR_ALL(iommus, elem) {
		iommu_t *iommu = super(elem, iommu_t, iom_list);
		vtd_iotlb_invalidate_page(iommu, paddr);
	}
}

void
vtd_invalidate(void)
{
	FOR_ALL(iommus, elem) {
		iommu_t *iommu = super(elem, iommu_t, iom_list);
		vtd_iotlb_invalidate(iommu);
	}
}

void
vtd_fault_interrupt(u8 vector)
{
	vtd_dump_fault_all();
}

void
vtd_enable_fault_interrupts(void)
{
	u8 vector = bp->intvec_vtd_fault;
	if (!vector) {
		kprintf("vtd_enable_fault_interrupts>vector not set\n");
		return;
	}

	irq_register(vector, vtd_fault_interrupt);

	FOR_ALL(iommus, elem) {
		iommu_t *iommu = super(elem, iommu_t, iom_list);
		vtd_init_fault_interrupt(iommu, vector);
	}
}
