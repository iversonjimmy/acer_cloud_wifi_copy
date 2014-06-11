#include "hyp.h"
#include "vmx.h"
#include "apic.h"
#include "interrupt.h"
#include "notify.h"

un notify_intvec = 0;

static void
notify_irq(u8 vector)
{
	u32 n = cpuno();
	vm_t *v = &cpu_vm[n];

	nested_spinlock(&v->v_notes_lock);

	list_t *elem;
	while ((elem = list_remove_head(&v->v_notes_list)) != NULL) {
		nested_spinunlock(&v->v_notes_lock);

		note_t *note = super(elem, note_t, nb_list[n]);
		if (note->nb_func) {
			note->nb_func(v, note->nb_arg0, note->nb_arg1);
		}
		fence();
		atomic_inc(&note->nb_completion_count);

		nested_spinlock(&v->v_notes_lock);
	}
	nested_spinunlock(&v->v_notes_lock);
}

void
notify_init(void)
{
	irq_register(bp->intvec_notify, notify_irq); 

	notify_intvec = bp->intvec_notify;
}

void
notify_all(nb_func_t func, nb_arg_t arg0, nb_arg_t arg1)
{
	sn this = cpuno();

	assert(irq_is_disabled());

	if (unlikely(notify_intvec == 0)) {
		func(&cpu_vm[this], arg0, arg1);
		return;
	}

	note_t note;
	un count = 0;

	note.nb_func = func;
	note.nb_arg0 = arg0;
	note.nb_arg1 = arg1;
	note.nb_completion_count = 0;
	/* No need to list_init(&note.nb_list[i]) because adding an element
	 * to a list will overwrite the element's prev and next ptrs.
	 */

	un notify_mask = 0;
	for_each_active_cpu(i) {
		if (i == this) {
			continue;
		}
		vm_t *v = &cpu_vm[i];
		nested_spinlock(&v->v_notes_lock);
		list_add_tail(&v->v_notes_list, &note.nb_list[i]);
		nested_spinunlock(&v->v_notes_lock);
		bit_set(notify_mask, i);

		apic_send_IPI(i, notify_intvec);
		count++;
	}

	func(&cpu_vm[this], arg0, arg1);

	fence();
	u64 timeout = rdtsc() + TSC_TIMEOUT;
	while (volatile_read(&note.nb_completion_count) < count) {
		/* Poll for incoming notifications */
		notify_irq(0);
		if (rdtsc() > timeout) {
			kprintf("notify_all>TIMEOUT %u %ld\n",
				volatile_read(&note.nb_completion_count),
				count /* ,
				note.nb_ack_map */);
			for_each_cpu(i, notify_mask) {
				vm_t *v = &cpu_vm[i];
				nested_spinlock(&v->v_notes_lock);
				/* If the element was already removed, this
				 * is a no-op.
				 */
				list_remove(&note.nb_list[i]);
				nested_spinunlock(&v->v_notes_lock);
			}
			return;
		}
	}
	fence();
}
