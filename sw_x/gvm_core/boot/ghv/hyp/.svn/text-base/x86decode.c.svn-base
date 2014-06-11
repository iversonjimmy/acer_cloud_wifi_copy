#include "hyp.h"
#include "x86decode.h"

#define PREFIX_LOCK   0xF0
#define PREFIX_REPN   0xF2
#define PREFIX_REPZ   0xF3
#define PREFIX_SEG_CS 0x2E
#define PREFIX_SEG_DS 0x3E
#define PREFIX_SEG_ES 0x26
#define PREFIX_SEG_SS 0x36
#define PREFIX_SEG_FS 0x64
#define PREFIX_SEG_GS 0x65
#define PREFIX_DATA   0x66
#define PREFIX_ADDR   0x67

#define PREFIX_REX    0x40

#define OPOFF_STOR8 0
#define OPOFF_STOR  1
#define OPOFF_LOAD8 2
#define OPOFF_LOAD  3

static s32
decode_imm(const u8 *instr, u8 size)
{
    switch(size) {
    case 8:
	return *(s8 *)instr;
    case 16:
	return *(s16 *)instr;
    case 32:
	return *(s32 *)instr;
    default:
	return 0;
    }
}


static u8
get_reg(u8 reg, int rex, int rexR, u8 byte)
{
    if (rexR) {
	return reg + REG_8;
    }

    if (!rex && byte && (reg & 4)) {
	/* When operand size is 8, sp, bp, si, di become ah, ch, dh, bh */
	return reg + REG_AH - REG_SP;
    }

    return reg;
}

static int
decode_sib(const u8 *instr, u8 mod, x86_instrn_info_t *info)
{
    u8 sib = *instr;
    u8 scale = 1 << (sib >> 6);
    u8 index = (sib >> 3) & 7;
    u8 base  = sib & 7;
   
    /* REX.X is always decoded */
    index = get_reg(index, info->prefix.rex, info->prefix.rexX, 0);
    info->oper_b.mem.scale = scale;

    /* RSP is never index, but R12 can be */
    if (index == REG_SP) {
	info->oper_b.mem.index = REG_NONE;
    } else {
	info->oper_b.mem.index = index;
    }

    /* RBP/R13 as base must come with disp, otherwise it means 0+disp32 */
    if (mod == 0 && base == REG_BP) {
	info->oper_b.mem.base = REG_NONE;
	info->oper_b.mem.disp = decode_imm(instr + 1, 32);
	return 5;
    } else {
	info->oper_b.mem.base = get_reg(base, info->prefix.rex,
					info->prefix.rexB, 0);
    }

    return 1;
}

static int
decode_modrm(const u8 *instr, x86_instrn_info_t *info, bool is64bit)
{
    u8 modrm = *instr;
    u8 mod = modrm >> 6;
    u8 reg = (modrm >> 3) & 7;
    u8 rm  = modrm & 7;
    int len = 1;
    u8 immn = 32;

    info->type_a = 0;
    info->type_b = mod != 3;
    info->oper_a.reg = get_reg(reg, info->prefix.rex, info->prefix.rexR,
			       info->data_size == 8);
    info->oper_b.reg = get_reg(rm, info->prefix.rex, info->prefix.rexB,
			       mod == 3 && info->data_size == 8);

    if (mod == 3) { /* register, not mem, done. */
	return len;
    }

    if (unlikely(info->addr_size == 16)) {
	immn = 16;
	if ((rm & 4) == 0) {
	    info->oper_b.mem.base  = (rm & 2) ? REG_BP : REG_BX;
	    info->oper_b.mem.index = (rm & 1) ? REG_DI : REG_SI;
	    info->oper_b.mem.scale = 1;
	} else {
	    switch (rm & 3) {
	    case 0:
		info->oper_b.mem.base = REG_SI;	break;
	    case 1:
		info->oper_b.mem.base = REG_DI;	break;
	    case 2:
		info->oper_b.mem.base = REG_BP;	break;
	    case 3:
		info->oper_b.mem.base = REG_BX; break;
	    }
	    info->oper_b.mem.index = REG_NONE;
	    info->oper_b.mem.scale = 0;
	}
    } else {
	/* sp is never base, escape to SIB byte */
	if (rm == REG_SP) {
	    len += decode_sib(instr + len, mod, info);
	} else {
	    info->oper_b.mem.scale = 0;
	    info->oper_b.mem.index = REG_NONE;
	}
    }

    switch (mod) {
    case 0: /* no displacement */
	/* RBP/R13 must have disp, otherwise it means zero/rip based */
	if ((info->oper_b.mem.base & 0x7) == REG_BP) {
	    /*
	     * 64-bit mode, this is RIP-relative mode, const offset must be
	     * encoded with the SIB byte
	     */
	    info->oper_b.mem.base = is64bit ? REG_RIP : REG_NONE;
	    info->oper_b.mem.disp = decode_imm(instr + len, immn);
	    len += immn/8;
	}
	break;
    case 1: /* disp8 follows */
	info->oper_b.mem.disp = decode_imm(instr + len, 8);
	len++;
	break;
    case 2: /* disp16/32 follows */
	info->oper_b.mem.disp = decode_imm(instr + len, immn);
	len += immn/8;
	break;
    }
    
    return len;
}

int
decode_x86_instrn(const u8 *instr, x86_instrn_info_t *info, bool is64bit)
{
    int i = 0;
    u8 opcode;

    memzero(info, sizeof *info);

    info->seg = 0xff;

    do {
	opcode = instr[i++];

	switch (opcode) {
	case PREFIX_LOCK:   info->prefix.lock = 1; break;
	case PREFIX_REPN:   info->prefix.repn = 1; break;
	case PREFIX_SEG_CS: info->seg = SEG_CS;    break;
	case PREFIX_SEG_SS: info->seg = SEG_SS;    break;
	case PREFIX_SEG_DS: info->seg = SEG_DS;    break;
	case PREFIX_SEG_ES: info->seg = SEG_ES;    break;
	case PREFIX_SEG_FS: info->seg = SEG_FS;    break;
	case PREFIX_SEG_GS: info->seg = SEG_GS;    break;
	case PREFIX_DATA:   info->prefix.data = 1; break;
	case PREFIX_ADDR:   info->prefix.addr = 1; break;
	default:
	    if (is64bit && (opcode & 0xF0) == PREFIX_REX) {
		info->prefix.rex  = 1;
		info->prefix.rexB = (opcode >> 0) & 1;
		info->prefix.rexX = (opcode >> 1) & 1;
		info->prefix.rexR = (opcode >> 2) & 1;
		info->prefix.rexW = (opcode >> 3) & 1;
	    } else {
		goto prefix_done;
	    }
	}

    } while (1);

 prefix_done:
    info->data_size = info->prefix.data ? 16 : 32;
    info->addr_size = info->prefix.addr ? 16 : 32;

    if (is64bit) {
	info->addr_size *= 2;
	if (info->prefix.rexW) {
	    info->data_size = 64;
	}
    }

    /* ALU r, rm / ALU rm, r */
    if (likely((opcode & 0xFC) == 0x88) || likely((opcode & 0xFC) == 0xa0) ||
	likely((opcode >> 3) <= ALU_CMP && (opcode & 4) == 0)) {

	if ((opcode & 0xFC) == 0x88 || (opcode & 0xFC) == 0xa0) {
	    info->op = ALU_MOV;
	} else {
	    info->op = opcode >> 3;
	}

	info->dir = (opcode >> 1) & 1;

	if ((opcode & 1) == 0) {
	    info->data_size = 8;
	}

	if ((opcode & 0xFC) == 0xa0) {
	    info->dir ^= 1;
	    info->oper_a.reg = REG_AX;
	    info->type_a = 0;
	    info->oper_b.mem.base = decode_imm(instr + i, info->data_size);
	    info->type_b = 1;
	    return i + info->data_size / 8;
	} else {
	    return i + decode_modrm(instr + i, info, is64bit);
	}
    }

    /* "Unary" ALU instructions with/without imm */
    if (likely((opcode & 0xFE) == 0xC6) || /* mov imm,  r/m */
	likely((opcode & 0xFE) == 0xF6) || /* alu [imm,] r/m */
	likely((opcode & 0xFC) == 0x80) || /* alu imm,  r/m */
	likely((opcode & 0xFC) == 0x84) || /* test/xchg, r, r/m */
	likely((opcode & 0xFC) == 0xD0) || /* sh  1/cl, r/m */
	likely((opcode & 0xFE) == 0xC0) || /* sh  imm,  r/m */
	false) {
	
	u8 imm_size = 0;
	u8 op_ext;

	i += decode_modrm(instr + i, info, is64bit);

	op_ext = info->oper_a.reg;
	info->oper_a.imm = 0;

	if ((opcode & 1) == 0) {
	    info->data_size = 8;
	}

	imm_size = info->data_size > 32 ? 32 : info->data_size;

	switch(opcode & 0xFE) {
	case 0xC6:
	    if (op_ext) {
		return -1;
	    }
	    info->op = ALU_MOV;
	    break;
	case 0xF6:
	    info->op = ALU_TEST + op_ext;
	    if (op_ext) {
		imm_size = 0;
	    }
	    break;
	case 0x80:
	case_0x80:
	    info->op = ALU_ADD + op_ext;
	    break;
	case 0x82:
	    if (opcode == 0x83) {
		imm_size = 8;
	    }
	    if (opcode == 0x82 && is64bit) {
		return -1;
	    }
	    goto case_0x80;
	case 0x84:
	case 0x86:
	    imm_size = 0;
	    info->op = ALU_TEST + ((opcode >> 1) & 1);
	    break;
	case 0xC0:
	    imm_size = 8;
	    goto case_sh;
	case 0xD0:
	    info->type_a = 1;
	    info->oper_a.imm = 1;
	    goto case_d0_d2;
	case 0xD2:
	    info->type_a = 0;
	    info->oper_a.reg = REG_CX;
	case_d0_d2:
	    imm_size = 0;
	case_sh:
	    info->op = ALU_ROL + op_ext;
	    break;
	default:
	    return 0;
	}
	
	if (imm_size) {
	    info->type_a = 1;
	    info->oper_a.imm = decode_imm(instr + i, imm_size);
	}

	return i + imm_size/8;
    }

    return -1;
}


#ifdef _TEST_

#include <stdio.h>

asm (
".data\n"
".global test_32_begin\n"
".global test_32_end\n"
"test_32_begin:\n"
"add %eax, %eax\n"
"or  %ecx, (%ecx)\n"
"adc %edx, 8(%edx)\n"
"sbb %ebx, 0x100(%ebx)\n"
"and %ebp, 0x100(%ebx,%esi,1)\n"
"sub %esp, -0x100(%ebx,%edi,2)\n"
"xor %esi, (%esp)\n"
"cmp %edi, (%ebp)\n"
"mov %ecx, 0xbabecafe\n"
"movb %ah, %ds:(%eax)\n"
"movb %ch, %es:(%ecx)\n"
"addb $1, (%eax)\n"
"addl $2, (%eax)\n"
"addw %ax, (%eax)\n"
"add (%eax), %eax\n"
"test (%eax), %eax\n"
"test %al, (%eax)\n"
"xchg %ecx, (%eax)\n"
"xchg %cl, (%eax)\n"
"testb $1, (%eax)\n"
"testw $0x1000, (%eax)\n"
"testl $0x10000, (%eax)\n"
"testl $0x2, (%bx)\n"
"testl $0x3, (%bp)\n"
"testl $0x4, (%si)\n"
"testl $0x4, (%di)\n"
"test %ah, 0x1984(%bx,%si)\n"
"test %bh, 0x1984(%bx,%di)\n"
"test %ch, 0x1984(%bp,%si)\n"
"test %dh, 0x1984(%bp,%di)\n"
"addr16 testb %al, 0x1234\n"
"test_32_end:\n"

".code64\n"
".global test_64_begin\n"
".global test_64_end\n"
"test_64_begin:\n"
"add %eax, %eax\n"
"add %rax, %r8\n"
"add %r8, %rax\n"
"add %eax, %r8d\n"
"add %ax, %r8w\n"
"add %al, %r8b\n"
"add %dh, %cl\n"
"add %sil, %cl\n"
"addl $5, 0(%rip)\n"
"addl $6, 0x1000\n"
"addw $5, (%r13)\n"
"addq %rax, (%r12)\n"
"addq %rax, (%r8,%r12)\n"
"addq %rax, (%eax)\n"
"addq %rax, (%ebx)\n"
"testw $1, (%rax)\n"
"testb $1, (%rax)\n"
"testl $1, (%rax)\n"
"testq $1, (%rax)\n"
"addb $1, (%rax)\n"
"addw $1, (%rax)\n"
"addl $1, (%rax)\n"
"addq $1, (%rax)\n"
"addw $0x1000, (%rax)\n"
"addl $0x1000, (%rax)\n"
"addq $0x1000, (%rax)\n"
"addl $0x10000, (%rax)\n"
"addq $0x10000, (%rax)\n"
"movb $1, 0x12345678\n"
"movw $1, 0x12345678\n"
"movl $1, 0x12345678\n" 
"movq $1, 0x12345678\n" 
"movw $0x1000, 0x12345678\n"
"movl $0x1000, 0x12345678\n"
"movq $0x1000, 0x12345678\n"
"movl $0x10000, 0x12345678\n"
"movq $0x10000, 0x12345678\n"
"shlb $1, (%rax)\n"
"shlw $3, (%rax)\n"
"shll %cl, (%rax)\n"
"shlq %cl, (%rax)\n"
"test_64_end:\n"
".code32\n"
);

extern const u8 test_32_begin[];
extern const u8 test_32_end[];
extern const u8 test_64_begin[];
extern const u8 test_64_end[];

#endif

static const char *reg_names[] = {
    "ax", "cx", "dx", "bx",
    "sp", "bp", "si", "di",
    "8",  "9",  "10", "11",
    "12", "13", "14", "15",
    "ah", "ch", "dh", "bh",
    "rip",
    [REG_NONE] = "eiz",
};

static const char *op_names[] = {
    "add", "or", "adc", "sbb", "and", "sub", "xor", "cmp",
    "test", "xchg", "not", "neg", "mul", "imul", "div", "idiv",
    "rol", "ror", "rcl", "rcr", "shl", "shr", "sal", "sar",
    "mov",
};

static char
insn_suffix(int size)
{
    switch(size) {
    case 8:
	return 'b';
    case 16:
	return 'w';
    case 32:
	return 'l';
    case 64:
	return 'q';
    }
    return 0;
}

static int
snprintf_reg_name(char *buf, size_t len, u8 reg, int size)
{
    if (size == 32) {
	if (reg < 8) {
	    return snprintf(buf, len, "%%e%s", reg_names[reg]);
	} else if (reg < 16) {
	    return snprintf(buf, len, "%%r%sd", reg_names[reg]);
	}
    }
    if (size == 16) {
	if (reg >= 8 && reg < 16) {
	    return snprintf(buf, len, "%%r%sw", reg_names[reg]);
	}
    }
    if (size == 64 && reg < 16) {
	return snprintf(buf, len, "%%r%s", reg_names[reg]);
    }
    if (size == 8) {
	if (reg < 4) {
	    return snprintf(buf, len, "%%%cl", reg_names[reg][0]);
	} else if (reg < 8) {
	    return snprintf(buf, len, "%%%sl", reg_names[reg]);
	} else if (reg < 16) {
	    return snprintf(buf, len, "%%r%sb", reg_names[reg]);
	}
    }
    return snprintf(buf, len, "%%%s", reg_names[reg]);
}

static const char *
filler(int len)
{
    static const char filler[] = "\t\t\t\t\t";

    return filler + (len / 8);
}

void
print_instr(x86_instrn_info_t *info, bool is64)
{
    char op_a[32];
    char op_b[32];
    int len = 0;

    if (info->type_a) {
	len += snprintf(op_a, sizeof op_a, "$0x%x", info->oper_a.imm);
    } else {
	len += snprintf_reg_name(op_a, sizeof op_a,
				 info->oper_a.reg, info->data_size);
    }

    len += 2;

    if (info->type_b) {
	char b[8];

	snprintf_reg_name(b, sizeof b,
			  info->oper_b.mem.base, info->addr_size);

	if (info->oper_b.mem.scale) {
	    char i[8];

	    snprintf_reg_name(i, sizeof i,
			      info->oper_b.mem.index, info->addr_size);
	    len += snprintf(op_b, sizeof op_b, "0x%x(%s,%s,%d)",
			    info->oper_b.mem.disp,
			    b, i, info->oper_b.mem.scale);
	} else if (info->oper_b.mem.base != REG_NONE) {
	    len += snprintf(op_b, sizeof op_b, "0x%x(%s)",
			    info->oper_b.mem.disp, b);
	} else {
	    len += snprintf(op_b, sizeof op_b, "0x%x",
			    info->oper_b.mem.disp);
	}
    } else {
	len += snprintf_reg_name(op_b, sizeof op_b,
				 info->oper_b.reg, info->data_size);
    }

    kprintf("%s%c\t%s, %s%s\n",
	    op_names[info->op], insn_suffix(info->data_size),
	    info->dir ? op_b : op_a,
	    info->dir ? op_a : op_b,
	    filler(len));
}

#ifdef _TEST_

int main()
{
    const u8 *instr = test_32_begin;

    printf("test_32\n");

    while(instr < test_32_end) {
	int r = print_instr(instr, 0);

	if (r == -1) {
	    fprintf(stderr, "Error at %d\n", instr - test_32_begin);
	    return 1;
	}

	instr += r;
    }

    printf("test_64\n");

    instr = test_64_begin;

    while(instr < test_64_end) {
	int r = print_instr(instr, 1);

	if (r == -1) {
	    fprintf(stderr, "Error at %d\n", instr - test_64_begin);
	    return 1;
	}

	instr += r;
    }


    return 0;
}

#endif
