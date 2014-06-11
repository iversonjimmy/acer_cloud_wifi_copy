// acpi-api.h
// acpi api definitions;

#ifndef __ACPI_API_H__
#define __ACPI_API_H__

// functions;

extern void cmos_tmp_state(int save);
extern void cmos_save_state(void);
extern void cmos_restore_state(void);

extern void acpi_map_fadt(u32 phys_facs, u32 phys_fadt);
extern void acpi_unarm(void);
extern bool acpi_fastswitch(int trap);
extern void acpi_shutdown(void);

extern u32 acpi_get_waking_vector(void);
extern void acpi_set_waking_vector(u32 vector);
extern int acpi_tmp_state(int save);
extern void acpi_save_state(void);
extern void acpi_restore_state(void);
extern int acpi_virt_io(u16 port, u8 iotype, un *val);
extern void acpi_force_wakeup(void);
extern void acpi_hw_power_off(void);
extern void acpi_hw_sleep(void);
extern void acpi_hw_wakeup(void);

#endif // __ACPI_API_H__
