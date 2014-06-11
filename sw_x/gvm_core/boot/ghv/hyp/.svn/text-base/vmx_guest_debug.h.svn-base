#ifndef _VMX_GUEST_DEBUG_H_
#define _VMX_GUEST_DEBUG_H_

static inline void
vmx_dump_guest_segments(const char *who)
{
	kprintf("%s>Guest segments:\n", who);
	kprintf("%s>GDT %016lx %04lx\n", who,
		vmread(VMCE_GUEST_GDTR_BASE),
		vmread(VMCE_GUEST_GDTR_LIMIT));

#ifdef __x86_64__
	if (guest_64()) {
#define print_seg(seg)							\
		kprintf("%s>%s: %04lx (attr %08lx)\n",			\
			who, #seg,					\
			vmread(VMCE_GUEST_##seg##_SEL),			\
			vmread(VMCE_GUEST_##seg##_ACCESS_RIGHTS))
		print_seg(CS);
		print_seg(SS);
		print_seg(ES);
		print_seg(DS);
	
#undef 	print_seg
#define print_seg(seg)							\
		kprintf("%s>%s: %04lx (base %016lx, limit %016lx, attr %08lx)\n", \
			who, #seg,					\
			vmread(VMCE_GUEST_##seg##_SEL),			\
			vmread(VMCE_GUEST_##seg##_BASE),		\
			vmread(VMCE_GUEST_##seg##_LIMIT),		\
			vmread(VMCE_GUEST_##seg##_ACCESS_RIGHTS))

		print_seg(FS);
		print_seg(GS);
		print_seg(TR);
		print_seg(LDTR);
#undef 	print_seg
		return;
	}
#endif

#define print_seg(seg)							\
	kprintf("%s>%s: %04lx (base %08lx, limit %08lx, attr %08lx)\n", \
		who, #seg,						\
		vmread(VMCE_GUEST_##seg##_SEL),				\
		vmread(VMCE_GUEST_##seg##_BASE),			\
		vmread(VMCE_GUEST_##seg##_LIMIT),			\
		vmread(VMCE_GUEST_##seg##_ACCESS_RIGHTS))
	
	print_seg(CS);
	print_seg(SS);
	print_seg(ES);
	print_seg(DS);
	print_seg(FS);
	print_seg(GS);
	print_seg(TR);
	print_seg(LDTR);
#undef print_seg
}

static inline void
vmx_dump_guest_registers(const char *who, const un *r)
{
#ifdef __x86_64__
	if (guest_64()) {
		kprintf("%s>Guest registers dump:\n"
			"%016lx %016lx %016lx %016lx "
			"%016lx %016lx %016lx %016lx\n"
			"%016lx %016lx %016lx %016lx "
			"%016lx %016lx %016lx %016lx\n",
			who,
			r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7],
			r[8], r[9], r[10], r[11], r[12], r[13], r[14], r[15]);
		return;
	}
#endif    
	kprintf("%s>Guest registers dump:\n"
		"%08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
		who,
		r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7]);
}

static inline void
__vmx_dump_guest_instruction(const char *who, un guest_linear_rip)
{
	u8 guest_instr[16];

	if (copy_from_guest(guest_instr, guest_linear_rip, sizeof guest_instr) < 0) {
		kprintf("%s>Could not fetch guest instruction.\n", who);
	} else {
		u8 *const p = guest_instr;
		kprintf("%s>Guest instruction: "
			"%02x %02x %02x %02x %02x %02x %02x %02x "
			"%02x %02x %02x %02x %02x %02x %02x %02x\n",
			who,
			p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
			p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
	}
}

static inline void
vmx_dump_guest_instruction(const char *who)
{
	un guest_rip = vmread(VMCE_GUEST_RIP);
	u32 guest_cs_base = vmread(VMCE_GUEST_CS_BASE);
	u16 guest_cs_sel  = vmread(VMCE_GUEST_CS_SEL);
	un guest_linear_rip = guest_cs_base + guest_rip;

	kprintf("%s>Guest RIP: %04x:%016lx (%016lx)\n", who,
		guest_cs_sel, guest_rip, guest_linear_rip);

	__vmx_dump_guest_instruction(who, guest_linear_rip);
}

static inline void
vmx_guest_backtrace(const char *who, u32 ebp)
{
	un guest_ss_base = vmread(VMCE_GUEST_SS_BASE);
	un guest_cs_base = vmread(VMCE_GUEST_CS_BASE);

	kprintf("%s>Guest backtrace start...\n", who);

	while (1) {
		struct {
			u32 old_ebp;
			u32 return_rip;
		} guest_stack_frame;

		if (copy_from_guest(&guest_stack_frame, ebp + guest_ss_base,
				    sizeof guest_stack_frame) < 0) {
			break;
		}

		kprintf("%s>%08x\n", __func__, guest_stack_frame.return_rip);
		__vmx_dump_guest_instruction(__func__, guest_cs_base + guest_stack_frame.return_rip);

		if (guest_stack_frame.old_ebp <= ebp) {
			break;
		}

		ebp = guest_stack_frame.old_ebp;
	}

	kprintf("%s>Guest backtrace end...\n", who);
}

#endif
