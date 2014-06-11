#include "hyp.h"
#include "vmx.h"
#include "vapic.h"
#include "apic.h"
#include "x86decode.h"
#include "gpt.h"

void
svm_enable_apic_virtualization(vm_t *v)
{
	kprintf("svm_enable_apic_virtualization>UNIMPLEMENTED\n");
}

void
svm_disable_apic_virtualization(void)
{
	kprintf("svm_disable_apic_virtualization>UNIMPLEMENTED\n");
}

void
vmx_enable_apic_virtualization(vm_t *v)
{
	kprintf("vmx_enable_apic_virtualization>ENTER\n");
	u64 msr_ia32_apic_base = get_MSR(MSR_IA32_APIC_BASE);
	u64 apic_base_phys = (bits64(msr_ia32_apic_base,
				     cpuinfo.cpu_phys_addr_width,
				     12) << VM_PAGE_SHIFT);
	vmwrite64(VMCE_APIC_ACCESS_ADDR, apic_base_phys);
	vmx_set_exec_cpu2(VMEXEC_CPU2_VIRTUALIZE_APIC);

	assert(bit_test(msr_ia32_apic_base, AP_ENABLE));
	assert(apic_base);

	vapic_init(v);

	v->v_mtf_exits = 0;
}

void
vmx_disable_apic_virtualization(void)
{
	vmx_clear_exec_cpu2(VMEXEC_CPU2_VIRTUALIZE_APIC);
}

#define ACC_R	0
#define ACC_W	1

void
vapic_init(vm_t *v)
{
	memzero(&v->v_vapic, sizeof v->v_vapic);
	v->v_vapic.va_icr1 = APIC_READ(APIC_ICR1);
}

static void
vapic_intercept(u32 icr, u32 icr1)
{
	bool level_deassert = (bits(icr, 15, 14) == 0x2);
	u8 delivery_mode = bits(icr, 10, 8);
	u8 vector = bits(icr, 7, 0);
	u8 dest = bits(icr1, 31, 24);

	char *type = NULL;

	switch (delivery_mode) {
	case APIC_ICR_DLV_SMI:
		type = "SMI";
		break;
	case APIC_ICR_DLV_NMI:
		type = "NMI";
		break;
	case APIC_ICR_DLV_INIT:
		if (level_deassert) {
			type = "INIT Level de-assert";
		} else {
			type = "INIT";
		}
		break;
	case APIC_ICR_DLV_SIPI:
		type = "SIPI";
		break;
	}

	bool logical = bit_test(icr, APIC_ICR_MODE_SHIFT);

	if (type) {
		kprintf("vapic_intercept>sending %s to %s dest %d "
			"vector 0x%x\n",
			type,
			logical ? "logical" : "physical",
			dest, vector);
	}
}

bool
vm_exit_apic(vm_t *v, registers_t *regs)
{
	vapic_t *va = &v->v_vapic;

	un xq = vmread(VMCE_EXIT_QUALIFICATION);

	u8 access_type = bits(xq, 15, 12);
	un offset = bits(xq, 11, 0);
	addr_t guest_rip = vmread(VMCE_GUEST_RIP);
	addr_t guest_linear_rip = vmread(VMCE_GUEST_CS_BASE) + guest_rip;

	bool guest64 = guest_64();
#define MAX_CODE_LEN 32
	u8 code[MAX_CODE_LEN];
	ret_t ret = copy_from_guest(code, guest_linear_rip, MAX_CODE_LEN);
	if (ret < 0) {
		kprintf("vm_exit_apic>copy_from_guest() failed %ld\n", ret);
		goto err_out;
	}
	x86_instrn_info_t info;
	sn len = decode_x86_instrn(code, &info, guest64);
	if (len <= 0) {
		kprintf("vm_exit_apic>decode_x86_instrn() failed "
			"%02x %02x %02x %02x\n",
			code[0], code[1], code[2], code[3]);
		goto err_out;
	}

	if (access_type == ACC_W) {
		if ((info.data_size != 32) || (info.dir != 0) ||
		    (info.op != ALU_MOV)) {
			kprintf("vm_exit_apic>write access to 0x%lx, "
				"unexpected instruction\n", offset);
			goto dump;
		}
	} else {
		if ((info.data_size != 32) || (info.dir != 1) ||
		    (info.op != ALU_MOV)) {
			kprintf("vm_exit_apic>read access to 0x%lx, "
				"unexpected instruction\n", offset);
			goto dump;
		}
	}

	switch (offset) {
	/* Read only */
	case APIC_VER:
	case APIC_APR:
	case APIC_PPR:
	case APIC_TMR:  case APIC_TMR1: case APIC_TMR2: case APIC_TMR3:
	case APIC_TMR4: case APIC_TMR5: case APIC_TMR6: case APIC_TMR7:
	case APIC_TMR_CCR:
		if (access_type == ACC_R) {
			u8 dst_reg = info.oper_a.reg;
			regs->reg[dst_reg] = APIC_READ(offset);
		}
		break;
	case APIC_ISR:  case APIC_ISR1: case APIC_ISR2: case APIC_ISR3:
	case APIC_ISR4: case APIC_ISR5: case APIC_ISR6: case APIC_ISR7:
		if (access_type == ACC_R) {
			u8 i = ((offset - APIC_ISR) >> 4);
			u32 *isr = (u32 *)(va->va_isr);
			u8 dst_reg = info.oper_a.reg;
			regs->reg[dst_reg] = isr[i];
		}
		break;
	case APIC_IRR:  case APIC_IRR1: case APIC_IRR2: case APIC_IRR3:
	case APIC_IRR4: case APIC_IRR5: case APIC_IRR6: case APIC_IRR7:
		if (access_type == ACC_R) {
			u8 i = ((offset - APIC_IRR) >> 4);
			u32 *irr = (u32 *)(va->va_irr);
			u32 *apic = (u32 *)(va->va_apic);
			u8 dst_reg = info.oper_a.reg;
			regs->reg[dst_reg] = irr[i] & apic[i];
		}
		break;
	/* Write only */
	case APIC_EOI:
		if (access_type == ACC_W) {
			sn s = bitmap_first(va->va_isr, NINTVECS);
			if (s < 0) {
				/* This shouldn't happen,
				 * but in case it ever does: */
				apic_ack_irq();
				break;
			}
			bitmap_clear(va->va_isr, s);
			bitmap_set(va->va_eoi, s);
			sn r = bitmap_first(va->va_irr, NINTVECS);
			if (s < r) {
				break;
			}
			sn e = NINTVECS;
			while ((e = bitmap_next(va->va_eoi, e)) >= 0) {
				apic_ack_irq();
				bitmap_clear(va->va_eoi, e);
			}
		}
		break;
	/* Read/Write*/
	case APIC_ID:
	case APIC_TPR:
	case APIC_LDR:
	case APIC_DFR:
	case APIC_SIV:
	case APIC_ESR:
	case APIC_LVT_CMCI:
	case APIC_LVT_TIMER:
	case APIC_LVT_THERM:
	case APIC_LVT_PERF:
	case APIC_LVT_LINT0:
	case APIC_LVT_LINT1:
	case APIC_LVT_ERR:
	case APIC_TMR_ICR:
	case APIC_TMR_DIV:
		if (access_type == ACC_W) {
			u8 src_reg = info.oper_a.reg;
			u32 src_data = (info.type_a ? (u32)info.oper_a.imm :
					(u32)(regs->reg[src_reg]));
			APIC_WRITE(offset, src_data);
			if (offset == APIC_TPR) {
				va->va_task_pri = bits(src_data, 7, 4);
			}
		} else {
			u8 dst_reg = info.oper_a.reg;
			regs->reg[dst_reg] = APIC_READ(offset);
		}
		break;
	/* Interrupt control */
	case APIC_ICR:
		if (access_type == ACC_W) {
			u8 src_reg = info.oper_a.reg;
			u32 src_data = (info.type_a ? (u32)info.oper_a.imm :
					(u32)(regs->reg[src_reg]));
			vapic_intercept(src_data, va->va_icr1);
			apic_wait_for_idle(apic_base);
			APIC_WRITE(APIC_ICR1, va->va_icr1);
			APIC_WRITE(APIC_ICR, src_data);
		} else {
			u8 dst_reg = info.oper_a.reg;
			regs->reg[dst_reg] = (APIC_READ(offset) &
					      ~APIC_ICR_(STATUS, BUSY));
		}
		break;
	case APIC_ICR1:
		if (access_type == ACC_W) {
			u8 src_reg = info.oper_a.reg;
			u32 src_data = (info.type_a ? (u32)info.oper_a.imm :
					(u32)(regs->reg[src_reg]));
			va->va_icr1 = src_data;
		} else {
			u8 dst_reg = info.oper_a.reg;
			regs->reg[dst_reg] = va->va_icr1;
		}
		break;
	default:
dump:
		kprintf("APIC 0x%lx %s 0x%lx rax=0x%lx rdx=0x%lx len=%ld\n",
			guest_linear_rip,
			access_type == ACC_R ? "R" : "W",
			offset,
			regs->rax,
			regs->rdx,
			len);

		print_instr(&info, false);
		goto execute;
		break;
	}

	/* Emulated the instruction, so skip it */
	vmwrite(VMCE_GUEST_RIP, guest_rip + len);
	return true;

execute:
	vmx_set_exec_cpu1(VMEXEC_CPU1_MONITOR_TRAP_FLAG);
err_out:
	vmx_disable_apic_virtualization();
	return true;
}

bool
vm_exit_mtf(vm_t *v, registers_t *regs)
{
	vmx_clear_exec_cpu1(VMEXEC_CPU1_MONITOR_TRAP_FLAG);

	if (v->v_mtf_exits++ < 20) {
		vmx_enable_apic_virtualization(v);
	} else {
		kprintf("vm_exit_mtf>apic virtualization disabled\n");
	}

	return true;
}

bool
vm_exit_pic_intercept(vm_t *v, u8 iotype, u16 port, un val)
{
	vapic_t *va = &v->v_vapic;

	/* Only care about OUTB for now */
	if (iotype != IO_OUTB) {
		return true;
	}

	/*
	 * As long as the OS is writing to the PIC, we can assume that the
	 * APIC isn't used. So let's just clear the virtual ISR on every write.
	 */
	bitmap_init(va->va_isr, NINTVECS, BITMAP_ALL_ZEROES);

	return true;
}
