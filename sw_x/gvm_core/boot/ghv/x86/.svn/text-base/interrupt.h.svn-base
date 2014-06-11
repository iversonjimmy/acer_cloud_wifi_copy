#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

#define NR_VECTORS	256

typedef void (*irq_handler_t)(u8);

typedef struct {
	irq_handler_t	handler;
} irq_vector_t;

extern bool interrupt(u8 vector);
extern void irq_register(u8 vector, irq_handler_t handler);

#endif
