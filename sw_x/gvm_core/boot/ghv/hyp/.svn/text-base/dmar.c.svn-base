#include <linux/acpi.h>
#include <asm/io.h>

#include "bootparam.h"

static void
parse_scope_path(void *ptr, void *endp)
{
	while (ptr < endp) {
		struct acpi_dmar_pci_path *path =
			(struct acpi_dmar_pci_path *)ptr;
		printk("  dev %d\n", path->dev); 
		printk("  fn %d\n", path->fn); 

		ptr += sizeof(struct acpi_dmar_pci_path);
	}
}

static void
parse_device_scope(void *ptr, void *endp)
{
	while (ptr < endp) {
		struct acpi_dmar_device_scope *scope =
			(struct acpi_dmar_device_scope *)ptr;

		switch (scope->entry_type) {
		case ACPI_DMAR_SCOPE_TYPE_ENDPOINT:
			printk("ACPI_DMAR_SCOPE_TYPE_ENDPOINT\n");
			break;
		case ACPI_DMAR_SCOPE_TYPE_BRIDGE:
			printk("ACPI_DMAR_SCOPE_TYPE_BRIDGE\n");
			break;
		case ACPI_DMAR_SCOPE_TYPE_IOAPIC:
			printk("ACPI_DMAR_SCOPE_TYPE_IOAPIC\n");
			break;
		case ACPI_DMAR_SCOPE_TYPE_HPET:
			printk("ACPI_DMAR_SCOPE_TYPE_HPET\n");
			break;
		default:
			printk("parse_dmar_hardware_unit>unexpected type %d\n",
			       scope->entry_type);
			break;
		}

		printk("enumeration_id %d\n", scope->enumeration_id);
		printk("bus %d\n", scope->bus);

		parse_scope_path(ptr + sizeof(struct acpi_dmar_device_scope),
				 ptr + scope->length);

		ptr += scope->length;
	}
}

static void
parse_dmar_hardware_unit(struct acpi_dmar_hardware_unit *dhu)
{
	printk("parse_dmar_hardware_unit>flags = 0x%x\n", dhu->flags);
	printk("parse_dmar_hardware_unit>segment = 0x%x\n", dhu->segment);
	printk("parse_dmar_hardware_unit>address = 0x%llx\n", dhu->address);

	if (dhu->flags & ACPI_DMAR_INCLUDE_ALL) {
		printk("parse_dmar_hardware_unit>default unit\n");
	}
	if (osbp->niommus < NIOMMUS) {
		osbp->iommu_base[osbp->niommus++] = dhu->address;
	} else {
		printk("parse_dmar_hardware_unit>WARNING too many iommus\n");
	}

	void *ptr = (void *)dhu + sizeof(struct acpi_dmar_hardware_unit);
	void *endp = (void *)dhu + dhu->header.length;

	parse_device_scope(ptr, endp);
}

static void
parse_dmar_reserved_memory(struct acpi_dmar_reserved_memory *rmrr)
{
	printk("parse_dmar_reserved_memory>segment = 0x%x\n", rmrr->segment);
	printk("parse_dmar_reserved_memory>base_address = 0x%llx\n",
	       rmrr->base_address);
	printk("parse_dmar_reserved_memory>end_address = 0x%llx\n",
	       rmrr->end_address);

	void *ptr = (void *)rmrr + sizeof(struct acpi_dmar_reserved_memory);
	void *endp = (void *)rmrr + rmrr->header.length;

	parse_device_scope(ptr, endp);
}

static void
parse_dmar_atsr(struct acpi_dmar_atsr *atsr)
{
	printk("parse_dmar_atsr>flags = 0x%x\n", atsr->flags);
	printk("parse_dmar_atsr>segment = 0x%x\n", atsr->segment);

	void *ptr = (void *)atsr + sizeof(struct acpi_dmar_atsr);
	void *endp = (void *)atsr + atsr->header.length;

	parse_device_scope(ptr, endp);
}

int
dmar_init(void)
{
	printk("dmar_init>ENTRY\n");

	struct acpi_table_dmar *dmar_tbl = NULL;

	acpi_status status;
	status = acpi_get_table(ACPI_SIG_DMAR, 0,
				(struct acpi_table_header **)&dmar_tbl);
	if (ACPI_SUCCESS(status) && !dmar_tbl) {
		status = AE_NOT_FOUND;
	}
	if (!ACPI_SUCCESS(status)) {
		printk("dmar_init>DMA Remapping table not found\n");
		return 0;
	}

	printk("dmar_init>dmar_tbl 0x%p\n", dmar_tbl);
	printk("dmar_init>dmar_tbl length %d\n", dmar_tbl->header.length);
	printk("dmar_init>dmar_tbl width %d\n", dmar_tbl->width);
	printk("dmar_init>dmar_tbl flags 0x%x\n", dmar_tbl->flags);

	void *ptr = (void *)dmar_tbl + sizeof(struct acpi_table_dmar);
	printk("dmar_init>ptr 0x%p\n", ptr);
	void *endp = (void *)dmar_tbl + dmar_tbl->header.length;
	printk("dmar_init>endp 0x%p\n", endp);

	while (ptr < endp) {
		struct acpi_dmar_header *hdr = (struct acpi_dmar_header *)ptr;

		switch (hdr->type) {
		case ACPI_DMAR_TYPE_HARDWARE_UNIT:
			printk("ACPI_DMAR_TYPE_HARDWARE_UNIT\n");
			parse_dmar_hardware_unit(ptr);
			break;
		case ACPI_DMAR_TYPE_RESERVED_MEMORY:
			printk("ACPI_DMAR_TYPE_RESERVED_MEMORY\n");
			parse_dmar_reserved_memory(ptr);
			break;
		case ACPI_DMAR_TYPE_ATSR:
			printk("ACPI_DMAR_TYPE_ATSR\n");
			parse_dmar_atsr(ptr);
			break;
		default:
			printk("dmar_init>unexpected type %d\n", hdr->type);
			break;
		}

		ptr += hdr->length;
	}

	printk("dmar_init>EXIT\n");

	return 0;
}

void
dmar_fin(void)
{
}
