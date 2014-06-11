#ifndef __X86DECODE_H__
#define __X86DECODE_H__

#include "km_types.h"

typedef enum {
    ALU_ADD = 0, ALU_OR, ALU_ADC, ALU_SBB, ALU_AND, ALU_SUB, ALU_XOR, ALU_CMP,
    ALU_TEST, ALU_XCHG, ALU_NOT, ALU_NEG, ALU_MUL, ALU_IMUL, ALU_DIV, ALU_IDIV,
    ALU_ROL, ALU_ROR, ALU_RCL, ALU_RCR, ALU_SHL, ALU_SHR, ALU_SAL, ALU_SAR,
    ALU_MOV,
} x86_aluop_t;

typedef enum {
    SEG_CS, SEG_SS, SEG_DS, SEG_ES, SEG_FS, SEG_GS,
} x86_seg_t;

typedef enum {
    REG_AX, REG_CX, REG_DX, REG_BX, REG_SP, REG_BP, REG_SI, REG_DI,
    REG_8,  REG_9,  REG_10, REG_11, REG_12, REG_13, REG_14, REG_15,
    REG_AH, REG_CH, REG_DH, REG_BH, REG_RIP,
    REG_NONE = 0xFF,
} x86_reg_t;

typedef struct {
    struct {
	u16 lock:1;
	u16 repn:1;
	u16 repz:1;
	u16 rex:1;
	u16 rexB:1;
	u16 rexX:1;
	u16 rexR:1;
	u16 rexW:1;
	u16 data:1;
	u16 addr:1;
    } prefix;

    u8 seg;   /* x86_seg_t */
    u8 op;    /* x86_aluop_t */

    u8 type_a : 1; /* 0 = reg, 1 = imm */
    u8 type_b : 1; /* 0 = reg, 1 = mem */
    u8 dir    : 1; /* 0 = store (a->b), 1 = b->a (load) */

    u8 addr_size; /* 16, 32, 64 */
    u8 data_size; /* 16, 32, 64 */

    union {
	u8 reg;   /* x86_reg_t */
	s32 imm;
    } oper_a;
    union {
	u8 reg;
	struct {
	    u8 base;   /* x86_reg_t */
	    u8 index;  /* x86_reg_t */
	    u8 scale;  /* 0, 1, 2, 4, 8 */
	    s32 disp;
	} mem;
    } oper_b;
} x86_instrn_info_t;

/*
 * Returns length of instruction, or -1 on failure.
 */
int decode_x86_instrn(const u8 *instr, x86_instrn_info_t *info, bool is64bit);
void print_instr(x86_instrn_info_t *info, bool is64);

#endif
