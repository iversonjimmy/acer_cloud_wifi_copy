#include "hyp.h"
#include "vmx.h"
#include "pt.h"
#include "dt.h"

void
test_gp(void)
{
	un temp;
	set_FS(0);
	asm volatile("mov %%fs:0, %0" : "=r" (temp));
}

un
test_divzero(un div)
{
	return 10 / div;
}

void
test_exception(void)
{
	kprintf("test_exception>ENTER\n");

	extern void test_exception_label(void);
	un n = cpuno();
	vm_t *v = &cpu_vm[n];
	v->v_modified_rip = (addr_t)&test_exception_label;

	un *ptr = 0;

	un res = volatile_read(ptr);
	kprintf("test_exception>res = %ld\n", res);

	asm volatile(".global test_exception_label\n\t"
		     "test_exception_label:");

	kprintf("test_exception>resumed after label\n");

	v->v_modified_rip = 0;
}

static void
dump_exception(un vector, exc_frame_t *regs)
{
#ifdef __x86_64__
#define rfmt ":0x%016lx"
#else
#define rfmt ":0x%08lx"
#endif
	kprintf("exception %ld errcode"rfmt"\n", vector, regs->errcode);
	addr_t vaddr = get_CR2();
	un cr3 = get_CR3();
	kprintf("cr2"rfmt"\n", vaddr);
	kprintf("cr3"rfmt"\n", cr3);
	if (vector ==  VEC_PF) {
		dump_pagetables(cr3, vaddr, vaddr);
	}
	kprintf(R"ip"rfmt"\n", regs->rip);
	kprintf(R"sp"rfmt"\n", regs->rsp);
	kprintf(R"ax"rfmt" "R"cx"rfmt" "R"dx"rfmt"\n",
		regs->rax, regs->rcx, regs->rdx);
	kprintf(R"bx"rfmt" "R"bp"rfmt" "R"si"rfmt"\n",
		regs->rbx, regs->rbp, regs->rsi);
#ifndef __x86_64__
	kprintf(R"di"rfmt"\n",
		regs->rdi);
#else
	kprintf("rdi"rfmt" r8 "rfmt" r9 "rfmt"\n",
		regs->rdi, regs->r8, regs->r9);
	kprintf("r10"rfmt" r11"rfmt" r12"rfmt"\n",
		regs->r10, regs->r11, regs->r12);
	kprintf("r13"rfmt" r14"rfmt" r15"rfmt"\n",
		regs->r13, regs->r14, regs->r15);
#endif
}

ret_t
exception(exc_frame_t *regs)
{
	un vector = regs->vector;
	regs->rsp = (un)regs + sizeof(*regs);
	un n = cpuno();
	vm_t *v = &cpu_vm[n];

	if (vector >= VEC_FIRST_USER) {
		if (cpu_is_AMD()) {
			extern void svm_handle_interrupt(vm_t *, exc_frame_t *);
			svm_handle_interrupt(v, regs);
			return 0;
		}
	}

	extern void handle_usermode_exception(exc_frame_t *regs);
	if (regs->cs == HYP_USER_CS) {
		handle_usermode_exception(regs);
		return 0;
	}

#ifdef NOTDEF
	if ((vector == VEC_PF) && v->v_cr3_reload_needed) {
		set_CR3(get_CR3());
		v->v_cr3_reload_needed = false;
		return 0;
	}
#endif

	if (v->v_modified_rip) {
		regs->rip = v->v_modified_rip;
		v->v_modified_rip = 0;
		return 0;
	}

	dump_exception(vector, regs);

	extern void exit(int);
	exit(vector);
	while (1);
}
