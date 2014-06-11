#ifndef __VAPIC_H__
#define __VAPIC_H__

extern void vapic_init(vm_t *);
extern void vmx_enable_apic_virtualization(vm_t *);
extern void vmx_disable_apic_virtualization(void);
extern bool vm_exit_apic(vm_t *, registers_t *);
extern bool vm_exit_mtf(vm_t *, registers_t *);

extern void svm_enable_apic_virtualization(vm_t *);
extern void svm_disable_apic_virtualization(void);

/* i8259A PIC registers */
#define PIC_MASTER_CMD		0x20
#define PIC_MASTER_IMR		0x21
#define PIC_SLAVE_CMD		0xa0
#define PIC_SLAVE_IMR		0xa1

#define PIC_CASCADE_IR		2
#define MASTER_ICW4_DEFAULT	0x01
#define SLAVE_ICW4_DEFAULT	0x01
#define PIC_ICW4_AEOI		2

extern bool vm_exit_pic_intercept(vm_t *, u8 iotype, u16 port, un val);

#endif
