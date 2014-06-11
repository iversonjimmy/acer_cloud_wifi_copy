#include "hyp.h"
#include "apic.h"
#include "interrupt.h"

irq_vector_t irq_handlers[NR_VECTORS];

void
irq_register(u8 vector, irq_handler_t handler)
{
	if (handler != NULL) {
		atomic_cx((un *)&irq_handlers[vector].handler,
			  NULL, (un)handler);
	}
}

bool
interrupt(u8 vector)
{
	irq_handler_t handler = irq_handlers[vector].handler;
	if (handler) {
		handler(vector);
		apic_ack_irq();
		return true;
	}

	return false;
}
