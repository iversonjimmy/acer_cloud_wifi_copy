/****************************************************************************
*
*						Realmode X86 Emulator Library
*
*            	Copyright (C) 1996-1999 SciTech Software, Inc.
* 				     Copyright (C) David Mosberger-Tang
* 					   Copyright (C) 1999 Egbert Eich
*
*  ========================================================================
*
*  Permission to use, copy, modify, distribute, and sell this software and
*  its documentation for any purpose is hereby granted without fee,
*  provided that the above copyright notice appear in all copies and that
*  both that copyright notice and this permission notice appear in
*  supporting documentation, and that the name of the authors not be used
*  in advertising or publicity pertaining to distribution of the software
*  without specific, written prior permission.  The authors makes no
*  representations about the suitability of this software for any purpose.
*  It is provided "as is" without express or implied warranty.
*
*  THE AUTHORS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
*  INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
*  EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
*  CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
*  USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
*  OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
*  PERFORMANCE OF THIS SOFTWARE.
*
*  ========================================================================
*
* Language:		ANSI C
* Environment:	Any
* Developer:    Kendall Bennett
*
* Description:  This file includes subroutines to implement the decoding
*               and emulation of all the x86 processor instructions.
*
* There are approximately 250 subroutines in here, which correspond
* to the 256 byte-"opcodes" found on the 8086.  The table which
* dispatches this is found in the files optab.[ch].
*
* Each opcode proc has a comment preceeding it which gives it's table
* address.  Several opcodes are missing (undefined) in the table.
*
* Each proc includes information for decoding (DECODE_PRINTF and
* DECODE_PRINTF2), debugging (TRACE_REGS, SINGLE_STEP), and misc
* functions (START_OF_INSTR, END_OF_INSTR).
*
* Many of the procedures are *VERY* similar in coding.  This has
* allowed for a very large amount of code to be generated in a fairly
* short amount of time (i.e. cut, paste, and modify).  The result is
* that much of the code below could have been folded into subroutines
* for a large reduction in size of this file.  The downside would be
* that there would be a penalty in execution speed.  The file could
* also have been *MUCH* larger by inlining certain functions which
* were called.  This could have resulted even faster execution.  The
* prime directive I used to decide whether to inline the code or to
* modularize it, was basically: 1) no unnecessary subroutine calls,
* 2) no routines more than about 200 lines in size, and 3) modularize
* any code that I might not get right the first time.  The fetch_*
* subroutines fall into the latter category.  The The decode_* fall
* into the second category.  The coding of the "switch(mod){ .... }"
* in many of the subroutines below falls into the first category.
* Especially, the coding of {add,and,or,sub,...}_{byte,word}
* subroutines are an especially glaring case of the third guideline.
* Since so much of the code is cloned from other modules (compare
* opcode #00 to opcode #01), making the basic operations subroutine
* calls is especially important; otherwise mistakes in coding an
* "add" would represent a nightmare in maintenance.
*
****************************************************************************/

#include "x86emu/x86emui.h"

/*----------------------------- Implementation ----------------------------*/

/****************************************************************************
PARAMETERS:
op1 - Instruction op code

REMARKS:
Handles illegal opcodes.
****************************************************************************/
static void x86emuOp_illegal_op(X86EMU_sysEnv *env, u8 op1)
{
    START_OF_INSTR();
    if (!env->illfuncs.ill_op || !env->illfuncs.ill_op(env, op1)) {
	DECODE_PRINTF("ILLEGAL X86 OPCODE\n");
	TRACE_REGS();
	printf("%04x:%04x: %02X ILLEGAL X86 OPCODE!\n",
	       env->x86.R_CS, env->x86.R_IP-1,op1);
	HALT_SYS();
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

static const char *const alu_op_names[] = {
    "ADD", "OR", "ADC", "SBB", "AND", "SUB", "XOR", "CMP"
};

static u8 (*const alu_byte_operation[])(X86EMU_sysEnv *env, u8 d, u8 s) =
{
    x86emuPrimOp_add_byte,           /* 00 */
    x86emuPrimOp_or_byte,            /* 01 */
    x86emuPrimOp_adc_byte,           /* 02 */
    x86emuPrimOp_sbb_byte,           /* 03 */
    x86emuPrimOp_and_byte,           /* 04 */
    x86emuPrimOp_sub_byte,           /* 05 */
    x86emuPrimOp_xor_byte,           /* 06 */
    x86emuPrimOp_cmp_byte,           /* 07 */
};

static u16 (*const alu_word_operation[])(X86EMU_sysEnv *env, u16 d, u16 s) =
{
    x86emuPrimOp_add_word,           /*00 */
    x86emuPrimOp_or_word,            /*01 */
    x86emuPrimOp_adc_word,           /*02 */
    x86emuPrimOp_sbb_word,           /*03 */
    x86emuPrimOp_and_word,           /*04 */
    x86emuPrimOp_sub_word,           /*05 */
    x86emuPrimOp_xor_word,           /*06 */
    x86emuPrimOp_cmp_word,           /*07 */
};

static u32 (*const alu_long_operation[])(X86EMU_sysEnv *env, u32 d, u32 s) =
{
    x86emuPrimOp_add_long,           /*00 */
    x86emuPrimOp_or_long,            /*01 */
    x86emuPrimOp_adc_long,           /*02 */
    x86emuPrimOp_sbb_long,           /*03 */
    x86emuPrimOp_and_long,           /*04 */
    x86emuPrimOp_sub_long,           /*05 */
    x86emuPrimOp_xor_long,           /*06 */
    x86emuPrimOp_cmp_long,           /*07 */
};

static inline void x86emuOp_alu_RM_R(X86EMU_sysEnv *env, u8 op1, int opersize)
{
    int mod, rl, rh;
    int alu = (op1 >> 3) & 7;

    START_OF_INSTR();
    DECODE_PRINTF2("%s\t", alu_op_names[alu]);
    FETCH_DECODE_MODRM(mod, rh, rl);
    if (mod == 3) {
	if (opersize == sizeof(u8)) {
	    u8 *srcreg, *destreg, destval;
	    destreg = DECODE_RM_BYTE_REGISTER(rl);
	    DECODE_PRINTF(",");
	    srcreg = DECODE_RM_BYTE_REGISTER(rh);
	    DECODE_PRINTF("\n");
	    TRACE_AND_STEP();
	    destval = alu_byte_operation[alu](env, *destreg, *srcreg);
	    if (alu != 7)
		*destreg = destval;
	} else if (opersize == sizeof(u16)) {
	    u16 *srcreg, *destreg, destval;
	    destreg = DECODE_RM_WORD_REGISTER(rl);
	    DECODE_PRINTF(",");
	    srcreg = DECODE_RM_WORD_REGISTER(rh);
	    DECODE_PRINTF("\n");
	    TRACE_AND_STEP();
	    destval = alu_word_operation[alu](env, *destreg, *srcreg);
	    if (alu != 7)
		*destreg = destval;
	} else {
	    u32 *srcreg, *destreg, destval;
	    destreg = DECODE_RM_LONG_REGISTER(rl);
	    DECODE_PRINTF(",");
	    srcreg = DECODE_RM_LONG_REGISTER(rh);
	    DECODE_PRINTF("\n");
	    TRACE_AND_STEP();
	    destval = alu_long_operation[alu](env, *destreg, *srcreg);
	    if (alu != 7)
		*destreg = destval;
	}
    } else {
	u32 offset;

	offset = x86emu_decode_rm_address(env, mod, rl);
	DECODE_PRINTF(",");

	if (opersize == sizeof(u8)) {
	    u8 *reg, val;
	    reg = DECODE_RM_BYTE_REGISTER(rh);
	    DECODE_PRINTF("\n");
	    TRACE_AND_STEP();	
	    val = fetch_data_byte(offset);
	    val = alu_byte_operation[alu](env, val, *reg);
	    if (alu != 7)
		store_data_byte(offset, val);
	} else if (opersize == sizeof(u16)) {
	    u16 *reg, val;
	    reg = DECODE_RM_WORD_REGISTER(rh);
	    DECODE_PRINTF("\n");
	    TRACE_AND_STEP();
	    val = fetch_data_word(offset);
	    val = alu_word_operation[alu](env, val, *reg);
	    if (alu != 7)
		store_data_word(offset, val);
	} else {
	    u32 *reg, val;
	    reg = DECODE_RM_LONG_REGISTER(rh);
	    DECODE_PRINTF("\n");
	    TRACE_AND_STEP();
	    val = fetch_data_long(offset);
	    val = alu_long_operation[alu](env, val, *reg);
	    if (alu != 7)
		store_data_long(offset, val);
	}
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

static inline void x86emuOp_alu_R_RM(X86EMU_sysEnv *env, u8 op1, int opersize)
{
    int mod, rl, rh;
    int alu = (op1 >> 3) & 7;

    START_OF_INSTR();
    DECODE_PRINTF2("%s\t", alu_op_names[alu]);
    FETCH_DECODE_MODRM(mod, rh, rl);
    if (mod == 3) {
	if (opersize == sizeof(u8)) {
	    u8 *srcreg, *destreg, destval;
	    destreg = DECODE_RM_BYTE_REGISTER(rh);
	    DECODE_PRINTF(",");
	    srcreg = DECODE_RM_BYTE_REGISTER(rl);
	    DECODE_PRINTF("\n");
	    TRACE_AND_STEP();
	    destval = alu_byte_operation[alu](env, *destreg, *srcreg);
	    if (alu != 7)
		*destreg = destval;
	} else if (opersize == sizeof(u16)) {
	    u16 *srcreg, *destreg, destval;
	    destreg = DECODE_RM_WORD_REGISTER(rh);
	    DECODE_PRINTF(",");
	    srcreg = DECODE_RM_WORD_REGISTER(rl);
	    DECODE_PRINTF("\n");
	    TRACE_AND_STEP();
	    destval = alu_word_operation[alu](env, *destreg, *srcreg);
	    if (alu != 7)
		*destreg = destval;
	} else {
	    u32 *srcreg, *destreg, destval;
	    destreg = DECODE_RM_LONG_REGISTER(rh);
	    DECODE_PRINTF(",");
	    srcreg = DECODE_RM_LONG_REGISTER(rl);
	    DECODE_PRINTF("\n");
	    TRACE_AND_STEP();
	    destval = alu_long_operation[alu](env, *destreg, *srcreg);
	    if (alu != 7)
		*destreg = destval;
	}
    } else {
	u32 offset;

	if (opersize == sizeof(u8)) {
	    u8 *reg, val;
	    reg = DECODE_RM_BYTE_REGISTER(rh);
	    DECODE_PRINTF(",");
	    offset = x86emu_decode_rm_address(env, mod, rl);
	    DECODE_PRINTF("\n");
	    TRACE_AND_STEP();	
	    val = fetch_data_byte(offset);
	    val = alu_byte_operation[alu](env, *reg, val);
	    if (alu != 7)
		*reg = val;
	} else if (opersize == sizeof(u16)) {
	    u16 *reg, val;
	    reg = DECODE_RM_WORD_REGISTER(rh);
	    DECODE_PRINTF(",");
	    offset = x86emu_decode_rm_address(env, mod, rl);
	    DECODE_PRINTF("\n");
	    TRACE_AND_STEP();	
	    val = fetch_data_word(offset);
	    val = alu_word_operation[alu](env, *reg, val);
	    if (alu != 7)
		*reg = val;
	} else {
	    u32 *reg, val;
	    reg = DECODE_RM_LONG_REGISTER(rh);
	    DECODE_PRINTF(",");
	    offset = x86emu_decode_rm_address(env, mod, rl);
	    DECODE_PRINTF("\n");
	    TRACE_AND_STEP();	
	    val = fetch_data_long(offset);
	    val = alu_long_operation[alu](env, *reg, val);
	    if (alu != 7)
		*reg = val;
	}
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

static inline void x86emuOp_alu_A_IMM(X86EMU_sysEnv *env, u8 op1, int opersize)
{
    int alu = (op1 >> 3) & 7;

    START_OF_INSTR();
    DECODE_PRINTF2("%s\t", alu_op_names[alu]);
    if (opersize == sizeof(u8)) {
	u8 val;
	DECODE_PRINTF("AL,");
	val = fetch_byte_imm();
	DECODE_PRINTF2("%02x\n", val);
	TRACE_AND_STEP();	
	val = alu_byte_operation[alu](env, env->x86.R_AL, val);
	if (alu != 7)
	    env->x86.R_AL = val;
    } else if (opersize == sizeof(u16)) {
	u16 val;
	DECODE_PRINTF("AX,");
	val = fetch_word_imm();
	DECODE_PRINTF2("%04x\n", val);
	TRACE_AND_STEP();	
	val = alu_word_operation[alu](env, env->x86.R_AX, val);
	if (alu != 7)
	    env->x86.R_AX = val;
    } else {
	u32 val;
	DECODE_PRINTF("EAX,");
	val = fetch_long_imm();
	DECODE_PRINTF2("%08x\n", val);
	TRACE_AND_STEP();	
	val = alu_long_operation[alu](env, env->x86.R_EAX, val);
	if (alu != 7)
	    env->x86.R_EAX = val;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}


/****************************************************************************
REMARKS:
Handles opcode 0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38
****************************************************************************/
static void x86emuOp_alu_byte_RM_R(X86EMU_sysEnv *env, u8 op1)
{
    return x86emuOp_alu_RM_R(env, op1, sizeof(u8));
}

/****************************************************************************
REMARKS:
Handles opcode 0x01, 0x09, 0x11, 0x19, 0x21, 0x29, 0x31, 0x39
****************************************************************************/
static void x86emuOp_alu_word_RM_R(X86EMU_sysEnv *env, u8 op1)
{
    if ((env->x86.mode & SYSMODE_PREFIX_DATA)) {
	return x86emuOp_alu_RM_R(env, op1, sizeof(u32));
    } else {
	return x86emuOp_alu_RM_R(env, op1, sizeof(u16));
    }
}

/****************************************************************************
REMARKS:
Handles opcode 0x02, 0x0A, 0x12, 0x1A, 0x22, 0x2A, 0x32, 0x3A
****************************************************************************/
static void x86emuOp_alu_byte_R_RM(X86EMU_sysEnv *env, u8 op1)
{
    return x86emuOp_alu_R_RM(env, op1, sizeof(u8));
}

/****************************************************************************
REMARKS:
Handles opcode 0x03, 0x0B, 0x13, 0x1B, 0x23, 0x2B, 0x33, 0x3B
****************************************************************************/
static void x86emuOp_alu_word_R_RM(X86EMU_sysEnv *env, u8 op1)
{
    if ((env->x86.mode & SYSMODE_PREFIX_DATA)) {
	return x86emuOp_alu_R_RM(env, op1, sizeof(u32));
    } else {
	return x86emuOp_alu_R_RM(env, op1, sizeof(u16));
    }
}

/****************************************************************************
REMARKS:
Handles opcode 0x04, 0x0C, 0x14, 0x1C, 0x24, 0x2C, 0x34, 0x3C
****************************************************************************/
static void x86emuOp_alu_byte_AL_IMM(X86EMU_sysEnv *env, u8 op1)
{
    return x86emuOp_alu_A_IMM(env, op1, sizeof(u8));
}

/****************************************************************************
REMARKS:
Handles opcode 0x05, 0x0D, 0x15, 0x1D, 0x25, 0x2D, 0x35, 0x3D
****************************************************************************/
static void x86emuOp_alu_word_AX_IMM(X86EMU_sysEnv *env, u8 op1)
{
    if ((env->x86.mode & SYSMODE_PREFIX_DATA)) {
	return x86emuOp_alu_A_IMM(env, op1, sizeof(u32));
    } else {
	return x86emuOp_alu_A_IMM(env, op1, sizeof(u16));
    }
}

/****************************************************************************
REMARKS:
Handles opcode 0x06, 0x0e, 0x16, 0x1e
****************************************************************************/
static void x86emuOp_push_SR(X86EMU_sysEnv *env, u8 op1)
{
    int r = (op1 >> 3) & 3;
    u16 *sreg;

    START_OF_INSTR();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
	DECODE_PRINTF("PUSHD\t");
    } else {
	DECODE_PRINTF("PUSH\t");
    }
    sreg = decode_rm_seg_register(r);
    DECODE_PRINTF("\n");
    TRACE_AND_STEP();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
	push_long(*sreg);
    } else {
	push_word(*sreg);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x07, 0x17, 0x1f
****************************************************************************/
static void x86emuOp_pop_SR(X86EMU_sysEnv *env, u8 op1)
{
    int r = (op1 >> 3) & 3;
    u16 *sreg;

    START_OF_INSTR();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
	DECODE_PRINTF("POPD\t");
    } else {
	DECODE_PRINTF("POP\t");
    }
    sreg = decode_rm_seg_register(r);
    DECODE_PRINTF("\n");
    TRACE_AND_STEP();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
	*sreg = (u16)pop_long();
    } else {
	*sreg = pop_word();
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}


/****************************************************************************
REMARKS:
Handles opcode 0x0f. Escape for two-byte opcode (286 or better)
****************************************************************************/
static void x86emuOp_two_byte(X86EMU_sysEnv *env, u8 op1)
{
    u8 op2 = sys_rdb(((u32)env->x86.R_CS << 4) + (env->x86.R_IP++));
    INC_DECODED_INST_LEN(1);
    (*x86emu_optab2[op2])(env, op2);
}

/****************************************************************************
REMARKS:
Handles opcode 0x26
****************************************************************************/
static void x86emuOp_segovr_ES(X86EMU_sysEnv *env, u8 op1)
{
    START_OF_INSTR();
    DECODE_PRINTF("ES:\n");
    TRACE_AND_STEP();
    env->x86.mode |= SYSMODE_SEGOVR_ES;
    /*
     * note the lack of DECODE_CLEAR_SEGOVR(r) since, here is one of 4
     * opcode subroutines we do not want to do this.
     */
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x27
****************************************************************************/
static void x86emuOp_daa(X86EMU_sysEnv *env, u8 op1)
{
    START_OF_INSTR();
    DECODE_PRINTF("DAA\n");
    TRACE_AND_STEP();
    env->x86.R_AL = daa_byte(env->x86.R_AL);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x2e
****************************************************************************/
static void x86emuOp_segovr_CS(X86EMU_sysEnv *env, u8 op1)
{
    START_OF_INSTR();
    DECODE_PRINTF("CS:\n");
    TRACE_AND_STEP();
    env->x86.mode |= SYSMODE_SEGOVR_CS;
    /* note no DECODE_CLEAR_SEGOVR here. */
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x2f
****************************************************************************/
static void x86emuOp_das(X86EMU_sysEnv *env, u8 op1)
{
    START_OF_INSTR();
    DECODE_PRINTF("DAS\n");
    TRACE_AND_STEP();
    env->x86.R_AL = das_byte(env->x86.R_AL);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x36
****************************************************************************/
static void x86emuOp_segovr_SS(X86EMU_sysEnv *env, u8 op1)
{
    START_OF_INSTR();
    DECODE_PRINTF("SS:\n");
    TRACE_AND_STEP();
    env->x86.mode |= SYSMODE_SEGOVR_SS;
    /* no DECODE_CLEAR_SEGOVR ! */
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x37
****************************************************************************/
static void x86emuOp_aaa(X86EMU_sysEnv *env, u8 op1)
{
    START_OF_INSTR();
    DECODE_PRINTF("AAA\n");
    TRACE_AND_STEP();
    env->x86.R_AX = aaa_word(env->x86.R_AX);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x3e
****************************************************************************/
static void x86emuOp_segovr_DS(X86EMU_sysEnv *env, u8 op1)
{
    START_OF_INSTR();
    DECODE_PRINTF("DS:\n");
    TRACE_AND_STEP();
    env->x86.mode |= SYSMODE_SEGOVR_DS;
    /* NO DECODE_CLEAR_SEGOVR! */
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x3f
****************************************************************************/
static void x86emuOp_aas(X86EMU_sysEnv *env, u8 op1)
{
    START_OF_INSTR();
    DECODE_PRINTF("AAS\n");
    TRACE_AND_STEP();
    env->x86.R_AX = aas_word(env->x86.R_AX);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47
****************************************************************************/
static void x86emuOp_inc_R(X86EMU_sysEnv *env, u8 op1)
{
    int r = op1 & 7;
    START_OF_INSTR();
    DECODE_PRINTF("INC\t");
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
	u32 *reg = DECODE_RM_LONG_REGISTER(r);
	DECODE_PRINTF("\n");
	TRACE_AND_STEP();
	*reg = inc_long(*reg);
    } else {
	u16 *reg = DECODE_RM_WORD_REGISTER(r);
	DECODE_PRINTF("\n");
	TRACE_AND_STEP();
	*reg = inc_word(*reg);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F
****************************************************************************/
static void x86emuOp_dec_R(X86EMU_sysEnv *env, u8 op1)
{
    int r = op1 & 7;
    START_OF_INSTR();
    DECODE_PRINTF("DEC\t");
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
	u32 *reg = DECODE_RM_LONG_REGISTER(r);
	DECODE_PRINTF("\n");
	TRACE_AND_STEP();
	*reg = dec_long(*reg);
    } else {
	u16 *reg = DECODE_RM_WORD_REGISTER(r);
	DECODE_PRINTF("\n");
	TRACE_AND_STEP();
	*reg = dec_word(*reg);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57
****************************************************************************/
static void x86emuOp_push_R(X86EMU_sysEnv *env, u8 op1)
{
    int r = op1 & 7;
    START_OF_INSTR();
    DECODE_PRINTF("PUSH\t");
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
	u32 *reg = DECODE_RM_LONG_REGISTER(r);
	DECODE_PRINTF("\n");
	TRACE_AND_STEP();
	push_long(*reg);
    } else {
	u16 *reg = DECODE_RM_WORD_REGISTER(r);
	DECODE_PRINTF("\n");
	TRACE_AND_STEP();
	push_word(*reg);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F
****************************************************************************/
static void x86emuOp_pop_R(X86EMU_sysEnv *env, u8 op1)
{
    int r = op1 & 7;
    START_OF_INSTR();
    DECODE_PRINTF("POP\t");
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
	u32 *reg = DECODE_RM_LONG_REGISTER(r);
	DECODE_PRINTF("\n");
	TRACE_AND_STEP();
	*reg = pop_long();
    } else {
	u16 *reg = DECODE_RM_WORD_REGISTER(r);
	DECODE_PRINTF("\n");
	TRACE_AND_STEP();
	*reg = pop_word();
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x60
****************************************************************************/
static void x86emuOp_push_all(X86EMU_sysEnv *env, u8 op1)
{
    START_OF_INSTR();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("PUSHAD\n");
    } else {
        DECODE_PRINTF("PUSHA\n");
    }
    TRACE_AND_STEP();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
        u32 old_sp = env->x86.R_ESP;

        push_long(env->x86.R_EAX);
        push_long(env->x86.R_ECX);
        push_long(env->x86.R_EDX);
        push_long(env->x86.R_EBX);
        push_long(old_sp);
        push_long(env->x86.R_EBP);
        push_long(env->x86.R_ESI);
        push_long(env->x86.R_EDI);
    } else {
        u16 old_sp = env->x86.R_SP;

        push_word(env->x86.R_AX);
        push_word(env->x86.R_CX);
        push_word(env->x86.R_DX);
        push_word(env->x86.R_BX);
        push_word(old_sp);
        push_word(env->x86.R_BP);
        push_word(env->x86.R_SI);
        push_word(env->x86.R_DI);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x61
****************************************************************************/
static void x86emuOp_pop_all(X86EMU_sysEnv *env, u8 op1)
{
    START_OF_INSTR();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("POPAD\n");
    } else {
        DECODE_PRINTF("POPA\n");
    }
    TRACE_AND_STEP();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
        env->x86.R_EDI = pop_long();
        env->x86.R_ESI = pop_long();
        env->x86.R_EBP = pop_long();
        env->x86.R_ESP += 4;              /* skip ESP */
        env->x86.R_EBX = pop_long();
        env->x86.R_EDX = pop_long();
        env->x86.R_ECX = pop_long();
        env->x86.R_EAX = pop_long();
    } else {
        env->x86.R_DI = pop_word();
        env->x86.R_SI = pop_word();
        env->x86.R_BP = pop_word();
        env->x86.R_SP += 2;               /* skip SP */
        env->x86.R_BX = pop_word();
        env->x86.R_DX = pop_word();
        env->x86.R_CX = pop_word();
        env->x86.R_AX = pop_word();
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/*opcode 0x62   ILLEGAL OP, calls x86emuOp_illegal_op() */
/*opcode 0x63   ILLEGAL OP, calls x86emuOp_illegal_op() */

/****************************************************************************
REMARKS:
Handles opcode 0x64
****************************************************************************/
static void x86emuOp_segovr_FS(X86EMU_sysEnv *env, u8 op1)
{
    START_OF_INSTR();
    DECODE_PRINTF("FS:\n");
    TRACE_AND_STEP();
    env->x86.mode |= SYSMODE_SEGOVR_FS;
    /*
     * note the lack of DECODE_CLEAR_SEGOVR(r) since, here is one of 4
     * opcode subroutines we do not want to do this.
     */
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x65
****************************************************************************/
static void x86emuOp_segovr_GS(X86EMU_sysEnv *env, u8 op1)
{
    START_OF_INSTR();
    DECODE_PRINTF("GS:\n");
    TRACE_AND_STEP();
    env->x86.mode |= SYSMODE_SEGOVR_GS;
    /*
     * note the lack of DECODE_CLEAR_SEGOVR(r) since, here is one of 4
     * opcode subroutines we do not want to do this.
     */
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x66 - prefix for 32-bit register
****************************************************************************/
static void x86emuOp_prefix_data(X86EMU_sysEnv *env, u8 op1)
{
    START_OF_INSTR();
    DECODE_PRINTF("DATA:\n");
    TRACE_AND_STEP();
    env->x86.mode |= SYSMODE_PREFIX_DATA;
    /* note no DECODE_CLEAR_SEGOVR here. */
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x67 - prefix for 32-bit address
****************************************************************************/
static void x86emuOp_prefix_addr(X86EMU_sysEnv *env, u8 op1)
{
    START_OF_INSTR();
    DECODE_PRINTF("ADDR:\n");
    TRACE_AND_STEP();
    env->x86.mode |= SYSMODE_PREFIX_ADDR;
    /* note no DECODE_CLEAR_SEGOVR here. */
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x68
****************************************************************************/
static void x86emuOp_push_word_IMM(X86EMU_sysEnv *env, u8 op1)
{
    START_OF_INSTR();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
        u32 imm = fetch_long_imm();
	DECODE_PRINTF2("PUSHD\t%08x\n", imm);
	TRACE_AND_STEP();
        push_long(imm);
    } else {
        u16 imm = fetch_word_imm();
	DECODE_PRINTF2("PUSH\t%04x\n", imm);
        push_word(imm);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x69
****************************************************************************/
static void x86emuOp_imul_word_IMM(X86EMU_sysEnv *env, u8 op1)
{
    int mod, rl, rh;
    u32 srcoffset;

    START_OF_INSTR();
    DECODE_PRINTF("IMUL\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    if (mod != 3) {
        if (env->x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;
            u32 srcval;
            u32 res_lo,res_hi;
            s32 imm;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm_address(mod, rl);
            srcval = fetch_data_long(srcoffset);
            imm = fetch_long_imm();
            DECODE_PRINTF2(",%d\n", (s32)imm);
            TRACE_AND_STEP();
            imul_long_direct(&res_lo,&res_hi,(s32)srcval,(s32)imm);
            if (res_hi != 0) {
                SET_FLAG(F_CF);
                SET_FLAG(F_OF);
            } else {
                CLEAR_FLAG(F_CF);
                CLEAR_FLAG(F_OF);
            }
            *destreg = (u32)res_lo;
        } else {
            u16 *destreg;
            u16 srcval;
            u32 res;
            s16 imm;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm_address(mod, rl);
            srcval = fetch_data_word(srcoffset);
            imm = fetch_word_imm();
            DECODE_PRINTF2(",%d\n", (s32)imm);
            TRACE_AND_STEP();
            res = (s16)srcval * (s16)imm;
            if (res > 0xFFFF) {
                SET_FLAG(F_CF);
                SET_FLAG(F_OF);
            } else {
                CLEAR_FLAG(F_CF);
                CLEAR_FLAG(F_OF);
            }
            *destreg = (u16)res;
        }
    } else {                     /* register to register */
        if (env->x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg,*srcreg;
            u32 res_lo,res_hi;
            s32 imm;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_LONG_REGISTER(rl);
            imm = fetch_long_imm();
            DECODE_PRINTF2(",%d\n", (s32)imm);
            TRACE_AND_STEP();
            imul_long_direct(&res_lo,&res_hi,(s32)*srcreg,(s32)imm);
            if (res_hi != 0) {
                SET_FLAG(F_CF);
                SET_FLAG(F_OF);
            } else {
                CLEAR_FLAG(F_CF);
                CLEAR_FLAG(F_OF);
            }
            *destreg = (u32)res_lo;
        } else {
            u16 *destreg,*srcreg;
            u32 res;
            s16 imm;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_WORD_REGISTER(rl);
            imm = fetch_word_imm();
            DECODE_PRINTF2(",%d\n", (s32)imm);
            res = (s16)*srcreg * (s16)imm;
            if (res > 0xFFFF) {
                SET_FLAG(F_CF);
                SET_FLAG(F_OF);
            } else {
                CLEAR_FLAG(F_CF);
                CLEAR_FLAG(F_OF);
            }
            *destreg = (u16)res;
        }
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x6a
****************************************************************************/
static void x86emuOp_push_byte_IMM(X86EMU_sysEnv *env, u8 op1)
{
    START_OF_INSTR();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
	s32 imm = (s8)fetch_byte_imm();
	DECODE_PRINTF2("PUSHD\t%08x\n", imm);
	TRACE_AND_STEP();
	push_long(imm);
    } else {
	s16 imm = (s8)fetch_byte_imm();
	DECODE_PRINTF2("PUSH\t%04x\n", imm);
	TRACE_AND_STEP();
	push_word(imm);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x6b
****************************************************************************/
static void x86emuOp_imul_byte_IMM(X86EMU_sysEnv *env, u8 op1)
{
    int mod, rl, rh;
    u32 srcoffset;
    s8  imm;

    START_OF_INSTR();
    DECODE_PRINTF("IMUL\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    if (mod != 3) {
        if (env->x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;
            u32 srcval;
            u32 res_lo,res_hi;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm_address(mod, rl);
            srcval = fetch_data_long(srcoffset);
            imm = fetch_byte_imm();
            DECODE_PRINTF2(",%d\n", (s32)imm);
            TRACE_AND_STEP();
            imul_long_direct(&res_lo,&res_hi,(s32)srcval,(s32)imm);
            if (res_hi != 0) {
                SET_FLAG(F_CF);
                SET_FLAG(F_OF);
            } else {
                CLEAR_FLAG(F_CF);
                CLEAR_FLAG(F_OF);
            }
            *destreg = (u32)res_lo;
        } else {
            u16 *destreg;
            u16 srcval;
            u32 res;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm_address(mod, rl);
            srcval = fetch_data_word(srcoffset);
            imm = fetch_byte_imm();
            DECODE_PRINTF2(",%d\n", (s32)imm);
            TRACE_AND_STEP();
            res = (s16)srcval * (s16)imm;
            if (res > 0xFFFF) {
                SET_FLAG(F_CF);
                SET_FLAG(F_OF);
            } else {
                CLEAR_FLAG(F_CF);
                CLEAR_FLAG(F_OF);
            }
            *destreg = (u16)res;
        }
    } else {                     /* register to register */
        if (env->x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg,*srcreg;
            u32 res_lo,res_hi;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_LONG_REGISTER(rl);
            imm = fetch_byte_imm();
            DECODE_PRINTF2(",%d\n", (s32)imm);
            TRACE_AND_STEP();
            imul_long_direct(&res_lo,&res_hi,(s32)*srcreg,(s32)imm);
            if (res_hi != 0) {
                SET_FLAG(F_CF);
                SET_FLAG(F_OF);
            } else {
                CLEAR_FLAG(F_CF);
                CLEAR_FLAG(F_OF);
            }
            *destreg = (u32)res_lo;
        } else {
            u16 *destreg,*srcreg;
            u32 res;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_WORD_REGISTER(rl);
            imm = fetch_byte_imm();
            DECODE_PRINTF2(",%d\n", (s32)imm);
            res = (s16)*srcreg * (s16)imm;
            if (res > 0xFFFF) {
                SET_FLAG(F_CF);
                SET_FLAG(F_OF);
            } else {
                CLEAR_FLAG(F_CF);
                CLEAR_FLAG(F_OF);
            }
            *destreg = (u16)res;
        }
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x6c
****************************************************************************/
static void x86emuOp_ins_byte(X86EMU_sysEnv *env, u8 op1)
{
    START_OF_INSTR();
    DECODE_PRINTF("INSB\n");
    ins(1);
    TRACE_AND_STEP();
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x6d
****************************************************************************/
static void x86emuOp_ins_word(X86EMU_sysEnv *env, u8 op1)
{
    START_OF_INSTR();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("INSD\n");
        ins(4);
    } else {
        DECODE_PRINTF("INSW\n");
        ins(2);
    }
    TRACE_AND_STEP();
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x6e
****************************************************************************/
static void x86emuOp_outs_byte(X86EMU_sysEnv *env, u8 op1)
{
    START_OF_INSTR();
    DECODE_PRINTF("OUTSB\n");
    outs(1);
    TRACE_AND_STEP();
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x6f
****************************************************************************/
static void x86emuOp_outs_word(X86EMU_sysEnv *env, u8 op1)
{
    START_OF_INSTR();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("OUTSD\n");
        outs(4);
    } else {
        DECODE_PRINTF("OUTSW\n");
        outs(2);
    }
    TRACE_AND_STEP();
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x70-0x7f
****************************************************************************/
static void x86emuOp_jump_near_cc(X86EMU_sysEnv *env, u8 op1)
{
    s8 offset;
    u16 target;

    START_OF_INSTR();
    DECODE_PRINTF2("J%s\t", x86emu_decode_cond_name(op1));
    offset = (s8)fetch_byte_imm();
    target = (u16)(env->x86.R_IP + (s16)offset);
    DECODE_PRINTF2("%04x\n", target);
    TRACE_AND_STEP();
    if (x86emu_decode_test_cond(env, op1))
        env->x86.R_IP = target;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x80
****************************************************************************/
static void x86emuOp_opc80_byte_RM_IMM(X86EMU_sysEnv *env, u8 op1)
{
    int mod, rl, rh;
    u8 *destreg;
    u32 destoffset;
    u8 imm;
    u8 destval;

    /*
     * Weirdo special case instruction format.  Part of the opcode
     * held below in "RH".  Doubly nested case would result, except
     * that the decoded instruction
     */
    START_OF_INSTR();
    FETCH_DECODE_MODRM(mod, rh, rl);
    DECODE_PRINTF2("%s\t", alu_op_names[rh]);
    /* know operation, decode the mod byte to find the addressing
       mode. */
    if (mod != 3) {
        DECODE_PRINTF("BYTE PTR ");
        destoffset = decode_rm_address(mod, rl);
        DECODE_PRINTF(",");
        destval = fetch_data_byte(destoffset);
        imm = fetch_byte_imm();
        DECODE_PRINTF2("%02x\n", imm);
        TRACE_AND_STEP();
        destval = (*alu_byte_operation[rh]) (env, destval, imm);
        if (rh != 7)
            store_data_byte(destoffset, destval);
    } else {                     /* register to register */
        destreg = DECODE_RM_BYTE_REGISTER(rl);
        DECODE_PRINTF(",");
        imm = fetch_byte_imm();
        DECODE_PRINTF2("%02x\n", imm);
        TRACE_AND_STEP();
        destval = (*alu_byte_operation[rh]) (env, *destreg, imm);
        if (rh != 7)
            *destreg = destval;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}


/****************************************************************************
REMARKS:
Handles opcode 0x81
****************************************************************************/
static void x86emuOp_opc81_word_RM_IMM(X86EMU_sysEnv *env, u8 op1)
{
    int mod, rl, rh;
    u32 destoffset;

    /*
     * Weirdo special case instruction format.  Part of the opcode
     * held below in "RH".  Doubly nested case would result, except
     * that the decoded instruction
     */
    START_OF_INSTR();
    FETCH_DECODE_MODRM(mod, rh, rl);
    DECODE_PRINTF2("%s\t", alu_op_names[rh]);
    /*
     * Know operation, decode the mod byte to find the addressing 
     * mode.
     */
    if (mod != 3) {
        if (env->x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval,imm;

            DECODE_PRINTF("DWORD PTR ");
            destoffset = decode_rm_address(mod, rl);
            DECODE_PRINTF(",");
            destval = fetch_data_long(destoffset);
            imm = fetch_long_imm();
            DECODE_PRINTF2("%08x\n", imm);
            TRACE_AND_STEP();
            destval = (*alu_long_operation[rh]) (env, destval, imm);
            if (rh != 7)
                store_data_long(destoffset, destval);
        } else {
            u16 destval,imm;

            DECODE_PRINTF("WORD PTR ");
            destoffset = decode_rm_address(mod, rl);
            DECODE_PRINTF(",");
            destval = fetch_data_word(destoffset);
            imm = fetch_word_imm();
            DECODE_PRINTF2("%04x\n", imm);
            TRACE_AND_STEP();
            destval = (*alu_word_operation[rh]) (env, destval, imm);
            if (rh != 7)
                store_data_word(destoffset, destval);
        }
    } else {                     /* register to register */
        if (env->x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;
            u32 destval,imm;

            destreg = DECODE_RM_LONG_REGISTER(rl);
            DECODE_PRINTF(",");
            imm = fetch_long_imm();
            DECODE_PRINTF2("%0x\n", imm);
            TRACE_AND_STEP();
            destval = (*alu_long_operation[rh]) (env, *destreg, imm);
            if (rh != 7)
                *destreg = destval;
        } else {
            u16 *destreg;
            u16 destval,imm;

            destreg = DECODE_RM_WORD_REGISTER(rl);
            DECODE_PRINTF(",");
            imm = fetch_word_imm();
            DECODE_PRINTF2("%04x\n", imm);
            TRACE_AND_STEP();
            destval = (*alu_word_operation[rh]) (env, *destreg, imm);
            if (rh != 7)
                *destreg = destval;
        }
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x83
****************************************************************************/
static void x86emuOp_opc83_word_RM_IMM(X86EMU_sysEnv *env, u8 op1)
{
    int mod, rl, rh;
    u32 destoffset;

    /*
     * Weirdo special case instruction format.  Part of the opcode
     * held below in "RH".  Doubly nested case would result, except
     * that the decoded instruction Similar to opcode 81, except that
     * the immediate byte is sign extended to a word length.
     */
    START_OF_INSTR();
    FETCH_DECODE_MODRM(mod, rh, rl);
    /* XXX DECODE_PRINTF may be changed to something more
       general, so that it is important to leave the strings
       in the same format, even though the result is that the 
       above test is done twice. */
    DECODE_PRINTF2("%s\t", alu_op_names[rh]);
    /* know operation, decode the mod byte to find the addressing
       mode. */
    if (mod != 3) {
        if (env->x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;
	    s32 imm;

            DECODE_PRINTF("DWORD PTR ");
            destoffset = decode_rm_address(mod,rl);
            destval = fetch_data_long(destoffset);
            imm = (s8) fetch_byte_imm();
            DECODE_PRINTF2(",%08x\n", imm);
            TRACE_AND_STEP();
            destval = (*alu_long_operation[rh]) (env, destval, imm);
            if (rh != 7)
                store_data_long(destoffset, destval);
        } else {
            u16 destval;
	    s16 imm;

            DECODE_PRINTF("WORD PTR ");
            destoffset = decode_rm_address(mod,rl);
            destval = fetch_data_word(destoffset);
            imm = (s8) fetch_byte_imm();
            DECODE_PRINTF2(",%04x\n", imm);
            TRACE_AND_STEP();
            destval = (*alu_word_operation[rh]) (env, destval, imm);
            if (rh != 7)
                store_data_word(destoffset, destval);
        }
    } else {                     /* register to register */
        if (env->x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;
            u32 destval;
	    s32 imm;

            destreg = DECODE_RM_LONG_REGISTER(rl);
            imm = (s8) fetch_byte_imm();
            DECODE_PRINTF2(",%08x\n", imm);
            TRACE_AND_STEP();
            destval = (*alu_long_operation[rh]) (env, *destreg, imm);
            if (rh != 7)
                *destreg = destval;
        } else {
            u16 *destreg;
            u16 destval;
	    s16 imm;

            destreg = DECODE_RM_WORD_REGISTER(rl);
            imm = (s8) fetch_byte_imm();
            DECODE_PRINTF2(",%04x\n", imm);
            TRACE_AND_STEP();
            destval = (*alu_word_operation[rh]) (env, *destreg, imm);
            if (rh != 7)
                *destreg = destval;
        }
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x84
****************************************************************************/
static void x86emuOp_test_byte_RM_R(X86EMU_sysEnv *env, u8 op1)
{
    int mod, rl, rh;
    u8 *destreg, *srcreg;
    u32 destoffset;
    u8 destval;

    START_OF_INSTR();
    DECODE_PRINTF("TEST\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    if (mod != 3) {
        destoffset = decode_rm_address(mod, rl);
        DECODE_PRINTF(",");
        destval = fetch_data_byte(destoffset);
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        test_byte(destval, *srcreg);
    } else {                     /* register to register */
        destreg = DECODE_RM_BYTE_REGISTER(rl);
        DECODE_PRINTF(",");
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        test_byte(*destreg, *srcreg);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x85
****************************************************************************/
static void x86emuOp_test_word_RM_R(X86EMU_sysEnv *env, u8 op1)
{
    int mod, rl, rh;
    u32 destoffset;

    START_OF_INSTR();
    DECODE_PRINTF("TEST\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    if (mod != 3) {
        if (env->x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;
            u32 *srcreg;

            destoffset = decode_rm_address(mod, rl);
            DECODE_PRINTF(",");
            destval = fetch_data_long(destoffset);
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            test_long(destval, *srcreg);
        } else {
            u16 destval;
            u16 *srcreg;

            destoffset = decode_rm_address(mod,rl);
            DECODE_PRINTF(",");
            destval = fetch_data_word(destoffset);
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            test_word(destval, *srcreg);
        }
    } else {                     /* register to register */
        if (env->x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg,*srcreg;

            destreg = DECODE_RM_LONG_REGISTER(rl);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            test_long(*destreg, *srcreg);
        } else {
            u16 *destreg,*srcreg;

            destreg = DECODE_RM_WORD_REGISTER(rl);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            test_word(*destreg, *srcreg);
        }
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x86
****************************************************************************/
static void x86emuOp_xchg_byte_RM_R(X86EMU_sysEnv *env, u8 op1)
{
    int mod, rl, rh;
    u8 *destreg, *srcreg;
    u32 destoffset;
    u8 destval;
    u8 tmp;

    START_OF_INSTR();
    DECODE_PRINTF("XCHG\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    if (mod != 3) {
        destoffset = decode_rm_address(mod, rl);
        DECODE_PRINTF(",");
        destval = fetch_data_byte(destoffset);
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        tmp = *srcreg;
        *srcreg = destval;
        destval = tmp;
        store_data_byte(destoffset, destval);
    } else {                     /* register to register */
        destreg = DECODE_RM_BYTE_REGISTER(rl);
        DECODE_PRINTF(",");
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        tmp = *srcreg;
        *srcreg = *destreg;
        *destreg = tmp;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x87
****************************************************************************/
static void x86emuOp_xchg_word_RM_R(X86EMU_sysEnv *env, u8 op1)
{
    int mod, rl, rh;
    u32 destoffset;

    START_OF_INSTR();
    DECODE_PRINTF("XCHG\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    if (mod != 3) {
        if (env->x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *srcreg;
            u32 destval,tmp;

            destoffset = decode_rm_address(mod, rl);
            DECODE_PRINTF(",");
            destval = fetch_data_long(destoffset);
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            tmp = *srcreg;
            *srcreg = destval;
            destval = tmp;
            store_data_long(destoffset, destval);
        } else {
            u16 *srcreg;
            u16 destval,tmp;

            destoffset = decode_rm_address(mod, rl);
            DECODE_PRINTF(",");
            destval = fetch_data_word(destoffset);
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            tmp = *srcreg;
            *srcreg = destval;
            destval = tmp;
            store_data_word(destoffset, destval);
        }
    } else {                     /* register to register */
        if (env->x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg,*srcreg;
            u32 tmp;

            destreg = DECODE_RM_LONG_REGISTER(rl);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            tmp = *srcreg;
            *srcreg = *destreg;
            *destreg = tmp;
        } else {
            u16 *destreg,*srcreg;
            u16 tmp;

            destreg = DECODE_RM_WORD_REGISTER(rl);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            tmp = *srcreg;
            *srcreg = *destreg;
            *destreg = tmp;
        }
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x88
****************************************************************************/
static void x86emuOp_mov_byte_RM_R(X86EMU_sysEnv *env, u8 op1)
{
    int mod, rl, rh;
    u8 *destreg, *srcreg;
    u32 destoffset;

    START_OF_INSTR();
    DECODE_PRINTF("MOV\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    if (mod != 3) {
        destoffset = decode_rm_address(mod, rl);
        DECODE_PRINTF(",");
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        store_data_byte(destoffset, *srcreg);
    } else {                     /* register to register */
        destreg = DECODE_RM_BYTE_REGISTER(rl);
        DECODE_PRINTF(",");
        srcreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = *srcreg;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x89
****************************************************************************/
static void x86emuOp_mov_word_RM_R(X86EMU_sysEnv *env, u8 op1)
{
    int mod, rl, rh;
    u32 destoffset;

    START_OF_INSTR();
    DECODE_PRINTF("MOV\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    if (mod != 3) {
        if (env->x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *srcreg;

            destoffset = decode_rm_address(mod, rl);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            store_data_long(destoffset, *srcreg);
        } else {
            u16 *srcreg;

            destoffset = decode_rm_address(mod, rl);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            store_data_word(destoffset, *srcreg);
        }
    } else {                     /* register to register */
        if (env->x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg,*srcreg;

            destreg = DECODE_RM_LONG_REGISTER(rl);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = *srcreg;
        } else {
            u16 *destreg,*srcreg;

            destreg = DECODE_RM_WORD_REGISTER(rl);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = *srcreg;
        }
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x8a
****************************************************************************/
static void x86emuOp_mov_byte_R_RM(X86EMU_sysEnv *env, u8 op1)
{
    int mod, rl, rh;
    u8 *destreg, *srcreg;
    u32 srcoffset;
    u8 srcval;

    START_OF_INSTR();
    DECODE_PRINTF("MOV\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    if (mod != 3) {
        destreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF(",");
        srcoffset = decode_rm_address(mod, rl);
        srcval = fetch_data_byte(srcoffset);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = srcval;
    } else {                     /* register to register */
        destreg = DECODE_RM_BYTE_REGISTER(rh);
        DECODE_PRINTF(",");
        srcreg = DECODE_RM_BYTE_REGISTER(rl);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = *srcreg;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x8b
****************************************************************************/
static void x86emuOp_mov_word_R_RM(X86EMU_sysEnv *env, u8 op1)
{
    int mod, rl, rh;
    u32 srcoffset;

    START_OF_INSTR();
    DECODE_PRINTF("MOV\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    if (mod != 3) {
        if (env->x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;
            u32 srcval;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm_address(mod, rl);
            srcval = fetch_data_long(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = srcval;
        } else {
            u16 *destreg;
            u16 srcval;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcoffset = decode_rm_address(mod, rl);
            srcval = fetch_data_word(srcoffset);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = srcval;
        }
    } else {                     /* register to register */
        if (env->x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg, *srcreg;

            destreg = DECODE_RM_LONG_REGISTER(rh);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_LONG_REGISTER(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = *srcreg;
        } else {
            u16 *destreg, *srcreg;

            destreg = DECODE_RM_WORD_REGISTER(rh);
            DECODE_PRINTF(",");
            srcreg = DECODE_RM_WORD_REGISTER(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = *srcreg;
        }
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x8c
****************************************************************************/
static void x86emuOp_mov_word_RM_SR(X86EMU_sysEnv *env, u8 op1)
{
    int mod, rl, rh;
    u16 *destreg, *srcreg;
    u32 destoffset;
    u16 destval;

    START_OF_INSTR();
    DECODE_PRINTF("MOV\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    if (mod != 3) {
        destoffset = decode_rm_address(mod, rl);
        DECODE_PRINTF(",");
        srcreg = decode_rm_seg_register(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        destval = *srcreg;
        store_data_word(destoffset, destval);
    } else {                     /* register to register */
        destreg = DECODE_RM_WORD_REGISTER(rl);
        DECODE_PRINTF(",");
        srcreg = decode_rm_seg_register(rh);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = *srcreg;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x8d
****************************************************************************/
static void x86emuOp_lea_word_R_M(X86EMU_sysEnv *env, u8 op1)
{
    int mod, rl, rh;
    u32 destoffset;

    START_OF_INSTR();
    DECODE_PRINTF("LEA\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    if (mod != 3) {
	if (env->x86.mode & SYSMODE_PREFIX_DATA) {
	    u32 *srcreg;
	    srcreg = DECODE_RM_LONG_REGISTER(rh);
	    DECODE_PRINTF(",");
	    destoffset = decode_rm_address(mod, rl);
	    DECODE_PRINTF("\n");
	    TRACE_AND_STEP();
	    *srcreg = destoffset;
	} else {
	    u16 *srcreg;
	    srcreg = DECODE_RM_WORD_REGISTER(rh);
	    DECODE_PRINTF(",");
	    destoffset = decode_rm_address(mod, rl);
	    DECODE_PRINTF("\n");
	    TRACE_AND_STEP();
	    *srcreg = (u16)destoffset;
	}
    } else {                     /* register to register */
        /* undefined.  Do nothing. */
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x8e
****************************************************************************/
static void x86emuOp_mov_word_SR_RM(X86EMU_sysEnv *env, u8 op1)
{
    int mod, rl, rh;
    u16 *destreg, *srcreg;
    u32 srcoffset;
    u16 srcval;

    START_OF_INSTR();
    DECODE_PRINTF("MOV\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    if (mod != 3) {
        destreg = decode_rm_seg_register(rh);
        DECODE_PRINTF(",");
        srcoffset = decode_rm_address(mod, rl);
        srcval = fetch_data_word(srcoffset);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = srcval;
    } else {                     /* register to register */
        destreg = decode_rm_seg_register(rh);
        DECODE_PRINTF(",");
        srcreg = DECODE_RM_WORD_REGISTER(rl);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *destreg = *srcreg;
    }
    /*
     * Clean up, and reset all the R_xSP pointers to the correct
     * locations.  This is about 3x too much overhead (doing all the
     * segreg ptrs when only one is needed, but this instruction
     * *cannot* be that common, and this isn't too much work anyway.
     */
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x8f
****************************************************************************/
static void x86emuOp_pop_RM(X86EMU_sysEnv *env, u8 op1)
{
    int mod, rl, rh;
    u32 destoffset;

    START_OF_INSTR();
    DECODE_PRINTF("POP\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    if (rh != 0) {
        DECODE_PRINTF("ILLEGAL DECODE OF OPCODE 8F\n");
        HALT_SYS();
    }
    if (mod != 3) {
        if (env->x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;

            destoffset = decode_rm_address(mod, rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = pop_long();
            store_data_long(destoffset, destval);
        } else {
            u16 destval;

            destoffset = decode_rm_address(mod, rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            destval = pop_word();
            store_data_word(destoffset, destval);
        }
    } else {                     /* register to register */
        if (env->x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;

            destreg = DECODE_RM_LONG_REGISTER(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = pop_long();
        } else {
            u16 *destreg;

            destreg = DECODE_RM_WORD_REGISTER(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = pop_word();
        }
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97
****************************************************************************/
static void x86emuOp_xchg_word_AX_R(X86EMU_sysEnv *env, u8 op1)
{
    int r = op1 & 7;

    START_OF_INSTR();
    DECODE_PRINTF("XCHG\t");
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
	u32 *reg, tmp;

	DECODE_PRINTF("EAX,");
	reg = DECODE_RM_LONG_REGISTER(r);
	DECODE_PRINTF("\n");
	TRACE_AND_STEP();
	tmp = env->x86.R_EAX;
	env->x86.R_EAX = *reg;
	*reg = tmp;
    } else {
	u16 *reg, tmp;

	DECODE_PRINTF("AX,");
	reg = DECODE_RM_WORD_REGISTER(r);
	DECODE_PRINTF("\n");
	TRACE_AND_STEP();
	tmp = env->x86.R_AX;
	env->x86.R_AX = *reg;
	*reg = tmp;
    }
    TRACE_AND_STEP();
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x98
****************************************************************************/
static void x86emuOp_cbw(X86EMU_sysEnv *env, u8 op1)
{
    START_OF_INSTR();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("CWDE\n");
    } else {
        DECODE_PRINTF("CBW\n");
    }
    TRACE_AND_STEP();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
        if (env->x86.R_AX & 0x8000) {
            env->x86.R_EAX |= 0xffff0000;
        } else {
            env->x86.R_EAX &= 0x0000ffff;
        }
    } else {
        if (env->x86.R_AL & 0x80) {
            env->x86.R_AH = 0xff;
        } else {
            env->x86.R_AH = 0x0;
        }
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x99
****************************************************************************/
static void x86emuOp_cwd(X86EMU_sysEnv *env, u8 op1)
{
    START_OF_INSTR();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("CDQ\n");
    } else {
        DECODE_PRINTF("CWD\n");
    }
    DECODE_PRINTF("CWD\n");
    TRACE_AND_STEP();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
        if (env->x86.R_EAX & 0x80000000) {
            env->x86.R_EDX = 0xffffffff;
        } else {
            env->x86.R_EDX = 0x0;
        }
    } else {
        if (env->x86.R_AX & 0x8000) {
            env->x86.R_DX = 0xffff;
        } else {
            env->x86.R_DX = 0x0;
        }
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x9a
****************************************************************************/
static void x86emuOp_call_far_IMM(X86EMU_sysEnv *env, u8 op1)
{
    u16 farseg, faroff;

    START_OF_INSTR();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
	DECODE_PRINTF("CALLD\t");
	faroff = fetch_long_imm();
    } else {
	DECODE_PRINTF("CALL\t");
	faroff = fetch_word_imm();
    }
    farseg = fetch_word_imm();
    DECODE_PRINTF2("%04x:", farseg);
    DECODE_PRINTF2("%04x\n", faroff);
    CALL_TRACE(env->x86.saved_cs, env->x86.saved_ip, farseg, faroff, "FAR ");

    /* XXX
     * 
     * Hooked interrupt vectors calling into our "BIOS" will cause
     * problems unless all intersegment stuff is checked for BIOS
     * access.  Check needed here.  For moment, let it alone.
     */
    TRACE_AND_STEP();
    push_word(env->x86.R_CS);
    env->x86.R_CS = farseg;
    push_word(env->x86.R_IP);
    env->x86.R_IP = faroff;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x9b
****************************************************************************/
static void x86emuOp_wait(X86EMU_sysEnv *env, u8 op1)
{
    START_OF_INSTR();
    DECODE_PRINTF("WAIT\n");
    TRACE_AND_STEP();
    /* NADA.  */
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x9c
****************************************************************************/
static void x86emuOp_pushf_word(X86EMU_sysEnv *env, u8 op1)
{
    u32 flags;

    START_OF_INSTR();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("PUSHFD\n");
    } else {
        DECODE_PRINTF("PUSHF\n");
    }
    TRACE_AND_STEP();

    /* clear out *all* bits not representing flags, and turn on real bits */
    flags = (env->x86.R_EFLG & F_MSK) | F_ALWAYS_ON;
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
        push_long(flags);
    } else {
        push_word((u16)flags);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x9d
****************************************************************************/
static void x86emuOp_popf_word(X86EMU_sysEnv *env, u8 op1)
{
    START_OF_INSTR();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("POPFD\n");
    } else {
        DECODE_PRINTF("POPF\n");
    }
    TRACE_AND_STEP();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
	u32 temp = pop_long();
	temp ^= env->x86.R_EFLG;
	temp &= F_MUTABLE;
        env->x86.R_EFLG ^= temp;
    } else {
        env->x86.R_FLG = pop_word();
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x9e
****************************************************************************/
static void x86emuOp_sahf(X86EMU_sysEnv *env, u8 op1)
{
    START_OF_INSTR();
    DECODE_PRINTF("SAHF\n");
    TRACE_AND_STEP();
    /* clear the lower bits of the flag register */
    env->x86.R_FLG &= 0xffffff00;
    /* or in the AH register into the flags register */
    env->x86.R_FLG |= env->x86.R_AH;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0x9f
****************************************************************************/
static void x86emuOp_lahf(X86EMU_sysEnv *env, u8 op1)
{
    START_OF_INSTR();
    DECODE_PRINTF("LAHF\n");
    TRACE_AND_STEP();
    env->x86.R_AH = (u8)(env->x86.R_FLG & 0xff);
    /*undocumented TC++ behavior??? Nope.  It's documented, but
      you have too look real hard to notice it. */
    env->x86.R_AH |= 0x2;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xa0
****************************************************************************/
static void x86emuOp_mov_AL_M_IMM(X86EMU_sysEnv *env, u8 op1)
{
    u16 offset;

    START_OF_INSTR();
    DECODE_PRINTF("MOV\tAL,");
    offset = fetch_word_imm();
    DECODE_PRINTF2("[%04x]\n", offset);
    TRACE_AND_STEP();
    env->x86.R_AL = fetch_data_byte(offset);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xa1
****************************************************************************/
static void x86emuOp_mov_AX_M_IMM(X86EMU_sysEnv *env, u8 op1)
{
    u16 offset;

    START_OF_INSTR();
    offset = fetch_word_imm();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF2("MOV\tEAX,[%04x]\n", offset);
    } else {
        DECODE_PRINTF2("MOV\tAX,[%04x]\n", offset);
    }
    TRACE_AND_STEP();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
        env->x86.R_EAX = fetch_data_long(offset);
    } else {
        env->x86.R_AX = fetch_data_word(offset);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xa2
****************************************************************************/
static void x86emuOp_mov_M_AL_IMM(X86EMU_sysEnv *env, u8 op1)
{
    u16 offset;

    START_OF_INSTR();
    DECODE_PRINTF("MOV\t");
    offset = fetch_word_imm();
    DECODE_PRINTF2("[%04x],AL\n", offset);
    TRACE_AND_STEP();
    store_data_byte(offset, env->x86.R_AL);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xa3
****************************************************************************/
static void x86emuOp_mov_M_AX_IMM(X86EMU_sysEnv *env, u8 op1)
{
    u16 offset;

    START_OF_INSTR();
    offset = fetch_word_imm();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF2("MOV\t[%04x],EAX\n", offset);
    } else {
        DECODE_PRINTF2("MOV\t[%04x],AX\n", offset);
    }
    TRACE_AND_STEP();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
        store_data_long(offset, env->x86.R_EAX);
    } else {
        store_data_word(offset, env->x86.R_AX);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xa4
****************************************************************************/
static void x86emuOp_movs_byte(X86EMU_sysEnv *env, u8 op1)
{
    u8  val;
    u32 count;
    int inc;

    START_OF_INSTR();
    DECODE_PRINTF("MOVS\tBYTE\n");
    if (ACCESS_FLAG(F_DF))   /* down */
        inc = -1;
    else
        inc = 1;
    TRACE_AND_STEP();
    count = 1;
    if (env->x86.mode & (SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE)) {
        /* dont care whether REPE or REPNE */
        /* move them until CX is ZERO. */
        count = env->x86.R_CX;
        env->x86.R_CX = 0;
        env->x86.mode &= ~(SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE);
    }
    while (count--) {
        val = fetch_data_byte(env->x86.R_SI);
        store_data_byte_abs(env->x86.R_ES, env->x86.R_DI, val);
        env->x86.R_SI += inc;
        env->x86.R_DI += inc;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xa5
****************************************************************************/
static void x86emuOp_movs_word(X86EMU_sysEnv *env, u8 op1)
{
    u32 val;
    int inc;
    u32 count;

    START_OF_INSTR();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("MOVS\tDWORD\n");
        if (ACCESS_FLAG(F_DF))      /* down */
            inc = -4;
        else
            inc = 4;
    } else {
        DECODE_PRINTF("MOVS\tWORD\n");
        if (ACCESS_FLAG(F_DF))      /* down */
            inc = -2;
        else
            inc = 2;
    }
    TRACE_AND_STEP();
    count = 1;
    if (env->x86.mode & (SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE)) {
        /* dont care whether REPE or REPNE */
        /* move them until CX is ZERO. */
        count = env->x86.R_CX;
        env->x86.R_CX = 0;
        env->x86.mode &= ~(SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE);
    }
    while (count--) {
        if (env->x86.mode & SYSMODE_PREFIX_DATA) {
            val = fetch_data_long(env->x86.R_SI);
            store_data_long_abs(env->x86.R_ES, env->x86.R_DI, val);
        } else {
            val = fetch_data_word(env->x86.R_SI);
            store_data_word_abs(env->x86.R_ES, env->x86.R_DI, (u16)val);
        }
        env->x86.R_SI += inc;
        env->x86.R_DI += inc;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xa6
****************************************************************************/
static void x86emuOp_cmps_byte(X86EMU_sysEnv *env, u8 op1)
{
    s8 val1, val2;
    int inc;

    START_OF_INSTR();
    DECODE_PRINTF("CMPS\tBYTE\n");
    TRACE_AND_STEP();
    if (ACCESS_FLAG(F_DF))   /* down */
        inc = -1;
    else
        inc = 1;

    if (env->x86.mode & SYSMODE_PREFIX_REPE) {
        /* REPE  */
        /* move them until CX is ZERO. */
        while (env->x86.R_CX != 0) {
            val1 = fetch_data_byte(env->x86.R_SI);
            val2 = fetch_data_byte_abs(env->x86.R_ES, env->x86.R_DI);
	    cmp_byte(val1, val2);
            env->x86.R_CX -= 1;
            env->x86.R_SI += inc;
            env->x86.R_DI += inc;
            if (ACCESS_FLAG(F_ZF) == 0)
                break;
        }
        env->x86.mode &= ~SYSMODE_PREFIX_REPE;
    } else if (env->x86.mode & SYSMODE_PREFIX_REPNE) {
        /* REPNE  */
        /* move them until CX is ZERO. */
        while (env->x86.R_CX != 0) {
            val1 = fetch_data_byte(env->x86.R_SI);
            val2 = fetch_data_byte_abs(env->x86.R_ES, env->x86.R_DI);
            cmp_byte(val1, val2);
            env->x86.R_CX -= 1;
            env->x86.R_SI += inc;
            env->x86.R_DI += inc;
            if (ACCESS_FLAG(F_ZF))
                break;          /* zero flag set means equal */
        }
        env->x86.mode &= ~SYSMODE_PREFIX_REPNE;
    } else {
        val1 = fetch_data_byte(env->x86.R_SI);
        val2 = fetch_data_byte_abs(env->x86.R_ES, env->x86.R_DI);
        cmp_byte(val1, val2);
        env->x86.R_SI += inc;
        env->x86.R_DI += inc;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xa7
****************************************************************************/
static void x86emuOp_cmps_word(X86EMU_sysEnv *env, u8 op1)
{
    u32 val1,val2;
    int inc;

    START_OF_INSTR();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("CMPS\tDWORD\n");
        if (ACCESS_FLAG(F_DF))   /* down */
            inc = -4;
        else
            inc = 4;
    } else {
        DECODE_PRINTF("CMPS\tWORD\n");
        if (ACCESS_FLAG(F_DF))   /* down */
            inc = -2;
        else
            inc = 2;
    }
    TRACE_AND_STEP();
    if (env->x86.mode & SYSMODE_PREFIX_REPE) {
        /* REPE  */
        /* move them until CX is ZERO. */
        while (env->x86.R_CX != 0) {
            if (env->x86.mode & SYSMODE_PREFIX_DATA) {
                val1 = fetch_data_long(env->x86.R_SI);
                val2 = fetch_data_long_abs(env->x86.R_ES, env->x86.R_DI);
                cmp_long(val1, val2);
            } else {
                val1 = fetch_data_word(env->x86.R_SI);
                val2 = fetch_data_word_abs(env->x86.R_ES, env->x86.R_DI);
                cmp_word((u16)val1, (u16)val2);
            }
            env->x86.R_CX -= 1;
            env->x86.R_SI += inc;
            env->x86.R_DI += inc;
            if (ACCESS_FLAG(F_ZF) == 0)
                break;
        }
        env->x86.mode &= ~SYSMODE_PREFIX_REPE;
    } else if (env->x86.mode & SYSMODE_PREFIX_REPNE) {
        /* REPNE  */
        /* move them until CX is ZERO. */
        while (env->x86.R_CX != 0) {
            if (env->x86.mode & SYSMODE_PREFIX_DATA) {
                val1 = fetch_data_long(env->x86.R_SI);
                val2 = fetch_data_long_abs(env->x86.R_ES, env->x86.R_DI);
                cmp_long(val1, val2);
            } else {
                val1 = fetch_data_word(env->x86.R_SI);
                val2 = fetch_data_word_abs(env->x86.R_ES, env->x86.R_DI);
                cmp_word((u16)val1, (u16)val2);
            }
            env->x86.R_CX -= 1;
            env->x86.R_SI += inc;
            env->x86.R_DI += inc;
            if (ACCESS_FLAG(F_ZF))
                break;          /* zero flag set means equal */
        }
        env->x86.mode &= ~SYSMODE_PREFIX_REPNE;
    } else {
        if (env->x86.mode & SYSMODE_PREFIX_DATA) {
            val1 = fetch_data_long(env->x86.R_SI);
            val2 = fetch_data_long_abs(env->x86.R_ES, env->x86.R_DI);
            cmp_long(val1, val2);
        } else {
            val1 = fetch_data_word(env->x86.R_SI);
            val2 = fetch_data_word_abs(env->x86.R_ES, env->x86.R_DI);
            cmp_word((u16)val1, (u16)val2);
        }
        env->x86.R_SI += inc;
        env->x86.R_DI += inc;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xa8
****************************************************************************/
static void x86emuOp_test_AL_IMM(X86EMU_sysEnv *env, u8 op1)
{
    u8 imm;

    START_OF_INSTR();
    DECODE_PRINTF("TEST\tAL,");
    imm = fetch_byte_imm();
    DECODE_PRINTF2("%02x\n", imm);
    TRACE_AND_STEP();
    test_byte(env->x86.R_AL, (u8)imm);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xa9
****************************************************************************/
static void x86emuOp_test_AX_IMM(X86EMU_sysEnv *env, u8 op1)
{
    START_OF_INSTR();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
	u32 srcval;
        srcval = fetch_long_imm();
        DECODE_PRINTF2("TEST\tEAX,%08x\n", srcval);
	TRACE_AND_STEP();
	test_long(env->x86.R_EAX, srcval);
    } else {
	u16 srcval;
        srcval = fetch_word_imm();
        DECODE_PRINTF2("TEST\tEAX,%04x\n", srcval);
	TRACE_AND_STEP();
	test_word(env->x86.R_AX, (u16)srcval);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xaa
****************************************************************************/
static void x86emuOp_stos_byte(X86EMU_sysEnv *env, u8 op1)
{
    int inc;

    START_OF_INSTR();
    DECODE_PRINTF("STOS\tBYTE\n");
    if (ACCESS_FLAG(F_DF))   /* down */
        inc = -1;
    else
        inc = 1;
    TRACE_AND_STEP();
    if (env->x86.mode & (SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE)) {
        /* dont care whether REPE or REPNE */
        /* move them until CX is ZERO. */
        while (env->x86.R_CX != 0) {
            store_data_byte_abs(env->x86.R_ES, env->x86.R_DI, env->x86.R_AL);
            env->x86.R_CX -= 1;
            env->x86.R_DI += inc;
        }
        env->x86.mode &= ~(SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE);
    } else {
        store_data_byte_abs(env->x86.R_ES, env->x86.R_DI, env->x86.R_AL);
        env->x86.R_DI += inc;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xab
****************************************************************************/
static void x86emuOp_stos_word(X86EMU_sysEnv *env, u8 op1)
{
    int inc;
    u32 count;

    START_OF_INSTR();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("STOS\tDWORD\n");
        if (ACCESS_FLAG(F_DF))   /* down */
            inc = -4;
        else
            inc = 4;
    } else {
        DECODE_PRINTF("STOS\tWORD\n");
        if (ACCESS_FLAG(F_DF))   /* down */
            inc = -2;
        else
            inc = 2;
    }
    TRACE_AND_STEP();
    count = 1;
    if (env->x86.mode & (SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE)) {
        /* dont care whether REPE or REPNE */
        /* move them until CX is ZERO. */
        count = env->x86.R_CX;
        env->x86.R_CX = 0;
        env->x86.mode &= ~(SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE);
    }
    while (count--) {
        if (env->x86.mode & SYSMODE_PREFIX_DATA) {
            store_data_long_abs(env->x86.R_ES, env->x86.R_DI, env->x86.R_EAX);
        } else {
            store_data_word_abs(env->x86.R_ES, env->x86.R_DI, env->x86.R_AX);
        }
        env->x86.R_DI += inc;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xac
****************************************************************************/
static void x86emuOp_lods_byte(X86EMU_sysEnv *env, u8 op1)
{
    int inc;

    START_OF_INSTR();
    DECODE_PRINTF("LODS\tBYTE\n");
    TRACE_AND_STEP();
    if (ACCESS_FLAG(F_DF))   /* down */
        inc = -1;
    else
        inc = 1;
    if (env->x86.mode & (SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE)) {
        /* dont care whether REPE or REPNE */
        /* move them until CX is ZERO. */
        while (env->x86.R_CX != 0) {
            env->x86.R_AL = fetch_data_byte(env->x86.R_SI);
            env->x86.R_CX -= 1;
            env->x86.R_SI += inc;
        }
        env->x86.mode &= ~(SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE);
    } else {
        env->x86.R_AL = fetch_data_byte(env->x86.R_SI);
        env->x86.R_SI += inc;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xad
****************************************************************************/
static void x86emuOp_lods_word(X86EMU_sysEnv *env, u8 op1)
{
    int inc;
    u32 count;

    START_OF_INSTR();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("LODS\tDWORD\n");
        if (ACCESS_FLAG(F_DF))   /* down */
            inc = -4;
        else
            inc = 4;
    } else {
        DECODE_PRINTF("LODS\tWORD\n");
        if (ACCESS_FLAG(F_DF))   /* down */
            inc = -2;
        else
            inc = 2;
    }
    TRACE_AND_STEP();
    count = 1;
    if (env->x86.mode & (SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE)) {
        /* dont care whether REPE or REPNE */
        /* move them until CX is ZERO. */
        count = env->x86.R_CX;
        env->x86.R_CX = 0;
        env->x86.mode &= ~(SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE);
    }
    while (count--) {
        if (env->x86.mode & SYSMODE_PREFIX_DATA) {
            env->x86.R_EAX = fetch_data_long(env->x86.R_SI);
        } else {
            env->x86.R_AX = fetch_data_word(env->x86.R_SI);
        }
        env->x86.R_SI += inc;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xae
****************************************************************************/
static void x86emuOp_scas_byte(X86EMU_sysEnv *env, u8 op1)
{
    s8 val2;
    int inc;

    START_OF_INSTR();
    DECODE_PRINTF("SCAS\tBYTE\n");
    TRACE_AND_STEP();
    if (ACCESS_FLAG(F_DF))   /* down */
        inc = -1;
    else
        inc = 1;
    if (env->x86.mode & SYSMODE_PREFIX_REPE) {
        /* REPE  */
        /* move them until CX is ZERO. */
        while (env->x86.R_CX != 0) {
            val2 = fetch_data_byte_abs(env->x86.R_ES, env->x86.R_DI);
            cmp_byte(env->x86.R_AL, val2);
            env->x86.R_CX -= 1;
            env->x86.R_DI += inc;
            if (ACCESS_FLAG(F_ZF) == 0)
                break;
        }
        env->x86.mode &= ~SYSMODE_PREFIX_REPE;
    } else if (env->x86.mode & SYSMODE_PREFIX_REPNE) {
        /* REPNE  */
        /* move them until CX is ZERO. */
        while (env->x86.R_CX != 0) {
            val2 = fetch_data_byte_abs(env->x86.R_ES, env->x86.R_DI);
            cmp_byte(env->x86.R_AL, val2);
            env->x86.R_CX -= 1;
            env->x86.R_DI += inc;
            if (ACCESS_FLAG(F_ZF))
                break;          /* zero flag set means equal */
        }
        env->x86.mode &= ~SYSMODE_PREFIX_REPNE;
    } else {
        val2 = fetch_data_byte_abs(env->x86.R_ES, env->x86.R_DI);
        cmp_byte(env->x86.R_AL, val2);
        env->x86.R_DI += inc;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xaf
****************************************************************************/
static void x86emuOp_scas_word(X86EMU_sysEnv *env, u8 op1)
{
    int inc;
    u32 val;

    START_OF_INSTR();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("SCAS\tDWORD\n");
        if (ACCESS_FLAG(F_DF))   /* down */
            inc = -4;
        else
            inc = 4;
    } else {
        DECODE_PRINTF("SCAS\tWORD\n");
        if (ACCESS_FLAG(F_DF))   /* down */
            inc = -2;
        else
            inc = 2;
    }
    TRACE_AND_STEP();
    if (env->x86.mode & SYSMODE_PREFIX_REPE) {
        /* REPE  */
        /* move them until CX is ZERO. */
        while (env->x86.R_CX != 0) {
            if (env->x86.mode & SYSMODE_PREFIX_DATA) {
                val = fetch_data_long_abs(env->x86.R_ES, env->x86.R_DI);
                cmp_long(env->x86.R_EAX, val);
            } else {
                val = fetch_data_word_abs(env->x86.R_ES, env->x86.R_DI);
                cmp_word(env->x86.R_AX, (u16)val);
            }
            env->x86.R_CX -= 1;
            env->x86.R_DI += inc;
            if (ACCESS_FLAG(F_ZF) == 0)
                break;
        }
        env->x86.mode &= ~SYSMODE_PREFIX_REPE;
    } else if (env->x86.mode & SYSMODE_PREFIX_REPNE) {
        /* REPNE  */
        /* move them until CX is ZERO. */
        while (env->x86.R_CX != 0) {
            if (env->x86.mode & SYSMODE_PREFIX_DATA) {
                val = fetch_data_long_abs(env->x86.R_ES, env->x86.R_DI);
                cmp_long(env->x86.R_EAX, val);
            } else {
                val = fetch_data_word_abs(env->x86.R_ES, env->x86.R_DI);
                cmp_word(env->x86.R_AX, (u16)val);
            }
            env->x86.R_CX -= 1;
            env->x86.R_DI += inc;
            if (ACCESS_FLAG(F_ZF))
                break;          /* zero flag set means equal */
        }
        env->x86.mode &= ~SYSMODE_PREFIX_REPNE;
    } else {
        if (env->x86.mode & SYSMODE_PREFIX_DATA) {
            val = fetch_data_long_abs(env->x86.R_ES, env->x86.R_DI);
            cmp_long(env->x86.R_EAX, val);
        } else {
            val = fetch_data_word_abs(env->x86.R_ES, env->x86.R_DI);
            cmp_word(env->x86.R_AX, (u16)val);
        }
        env->x86.R_DI += inc;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7 
****************************************************************************/
static void x86emuOp_mov_byte_R_IMM(X86EMU_sysEnv *env, u8 op1)
{
    u8 imm;
    u8 *reg;
    int r = op1 & 7;

    START_OF_INSTR();
    DECODE_PRINTF("MOV\t");
    reg = DECODE_RM_BYTE_REGISTER(r);
    DECODE_PRINTF(",");
    imm = fetch_byte_imm();
    DECODE_PRINTF2("%02x\n", imm);
    TRACE_AND_STEP();
    *reg = imm;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf
****************************************************************************/
static void x86emuOp_mov_word_R_IMM(X86EMU_sysEnv *env, u8 op1)
{
    int r = op1 & 7;

    START_OF_INSTR();
    DECODE_PRINTF("MOV\t");
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
	u32 srcval, *reg;
	reg = DECODE_RM_LONG_REGISTER(r);
        DECODE_PRINTF(",");
        srcval = fetch_long_imm();
	DECODE_PRINTF2("%08x\n", srcval);
	TRACE_AND_STEP();
	*reg = srcval;
    } else {
	u16 srcval, *reg;
	reg = DECODE_RM_WORD_REGISTER(r);
        DECODE_PRINTF(",");
        srcval = fetch_word_imm();
	DECODE_PRINTF2("%04x\n", srcval);
	TRACE_AND_STEP();
	*reg = srcval;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

static const char *const shift_op_names[] = {
    "ROL", "ROR", "RCL", "RCR", "SHL", "SHR", "SAL", "SAR"
};

/* used by opcodes c0, d0, and d2. */
static u8(*shift_byte_operation[])(X86EMU_sysEnv *env, u8 d, u8 s) =
{
    x86emuPrimOp_rol_byte,
    x86emuPrimOp_ror_byte,
    x86emuPrimOp_rcl_byte,
    x86emuPrimOp_rcr_byte,
    x86emuPrimOp_shl_byte,
    x86emuPrimOp_shr_byte,
    x86emuPrimOp_shl_byte,           /* sal_byte === shl_byte  by definition */
    x86emuPrimOp_sar_byte,
};

/* used by opcodes c1, d1, and d3. */
static u16(*shift_word_operation[])(X86EMU_sysEnv *env, u16 s, u8 d) =
{
    x86emuPrimOp_rol_word,
    x86emuPrimOp_ror_word,
    x86emuPrimOp_rcl_word,
    x86emuPrimOp_rcr_word,
    x86emuPrimOp_shl_word,
    x86emuPrimOp_shr_word,
    x86emuPrimOp_shl_word,           /* sal_byte === shl_byte  by definition */
    x86emuPrimOp_sar_word,
};

/* used by opcodes c1, d1, and d3. */
static u32 (*shift_long_operation[])(X86EMU_sysEnv *env, u32 s, u8 d) =
{
    x86emuPrimOp_rol_long,
    x86emuPrimOp_ror_long,
    x86emuPrimOp_rcl_long,
    x86emuPrimOp_rcr_long,
    x86emuPrimOp_shl_long,
    x86emuPrimOp_shr_long,
    x86emuPrimOp_shl_long,           /* sal_byte === shl_byte  by definition */
    x86emuPrimOp_sar_long,
};


/****************************************************************************
REMARKS:
Handles opcode 0xc0
****************************************************************************/
static void x86emuOp_opcC0_byte_RM_MEM(X86EMU_sysEnv *env, u8 op1)
{
    int mod, rl, rh;
    u8 *destreg;
    u32 destoffset;
    u8 destval;
    u8 amt;

    /*
     * Yet another weirdo special case instruction format.  Part of
     * the opcode held below in "RH".  Doubly nested case would
     * result, except that the decoded instruction
     */
    START_OF_INSTR();
    FETCH_DECODE_MODRM(mod, rh, rl);
    DECODE_PRINTF2("%s\t", shift_op_names[rh]);
    /* know operation, decode the mod byte to find the addressing
       mode. */
    if (mod != 3) {
        DECODE_PRINTF("BYTE PTR ");
        destoffset = decode_rm_address(mod, rl);
        amt = fetch_byte_imm();
        DECODE_PRINTF2(",%x\n", amt);
        destval = fetch_data_byte(destoffset);
        TRACE_AND_STEP();
        destval = (*shift_byte_operation[rh]) (env, destval, amt);
        store_data_byte(destoffset, destval);
    } else {                     /* register to register */
        destreg = DECODE_RM_BYTE_REGISTER(rl);
        amt = fetch_byte_imm();
        DECODE_PRINTF2(",%x\n", amt);
        TRACE_AND_STEP();
        destval = (*shift_byte_operation[rh]) (env, *destreg, amt);
        *destreg = destval;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}


/****************************************************************************
REMARKS:
Handles opcode 0xc1
****************************************************************************/
static void x86emuOp_opcC1_word_RM_MEM(X86EMU_sysEnv *env, u8 op1)
{
    int mod, rl, rh;
    u32 destoffset;
    u8 amt;

    /*
     * Yet another weirdo special case instruction format.  Part of
     * the opcode held below in "RH".  Doubly nested case would
     * result, except that the decoded instruction
     */
    START_OF_INSTR();
    FETCH_DECODE_MODRM(mod, rh, rl);
    DECODE_PRINTF2("%s\t", shift_op_names[rh]);
    /* know operation, decode the mod byte to find the addressing
       mode. */
    if (mod != 3) {
        if (env->x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;

            DECODE_PRINTF("DWORD PTR ");
            destoffset = decode_rm_address(mod, rl);
            amt = fetch_byte_imm();
            DECODE_PRINTF2(",%x\n", amt);
            destval = fetch_data_long(destoffset);
            TRACE_AND_STEP();
            destval = (*shift_long_operation[rh]) (env, destval, amt);
            store_data_long(destoffset, destval);
        } else {
            u16 destval;

            DECODE_PRINTF("WORD PTR ");
            destoffset = decode_rm_address(mod, rl);
            amt = fetch_byte_imm();
            DECODE_PRINTF2(",%x\n", amt);
            destval = fetch_data_word(destoffset);
            TRACE_AND_STEP();
            destval = (*shift_word_operation[rh]) (env, destval, amt);
            store_data_word(destoffset, destval);
        }
    } else {                     /* register to register */
        if (env->x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;

            destreg = DECODE_RM_LONG_REGISTER(rl);
            amt = fetch_byte_imm();
            DECODE_PRINTF2(",%x\n", amt);
            TRACE_AND_STEP();
            *destreg = (*shift_long_operation[rh]) (env, *destreg, amt);
        } else {
            u16 *destreg;

            destreg = DECODE_RM_WORD_REGISTER(rl);
            amt = fetch_byte_imm();
            DECODE_PRINTF2(",%x\n", amt);
            TRACE_AND_STEP();
            *destreg = (*shift_word_operation[rh]) (env, *destreg, amt);
        }
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xc2
****************************************************************************/
static void x86emuOp_ret_near_IMM(X86EMU_sysEnv *env, u8 op1)
{
    u16 imm;

    START_OF_INSTR();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
	DECODE_PRINTF("RETD\t");
    } else {
	DECODE_PRINTF("RET\t");
    }
    imm = fetch_word_imm();
    DECODE_PRINTF2("%04x\n", imm);
    RETURN_TRACE("RET",env->x86.saved_cs,env->x86.saved_ip);
    TRACE_AND_STEP();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
	env->x86.R_IP = pop_long();
    } else {
	env->x86.R_IP = pop_word();
    }
    env->x86.R_SP += imm;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xc3
****************************************************************************/
static void x86emuOp_ret_near(X86EMU_sysEnv *env, u8 op1)
{
    START_OF_INSTR();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
	DECODE_PRINTF("RETD\n");
    } else {
	DECODE_PRINTF("RET\n");
    }
    RETURN_TRACE("RET",env->x86.saved_cs,env->x86.saved_ip);
    TRACE_AND_STEP();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
	env->x86.R_IP = pop_long();
    } else {
	env->x86.R_IP = pop_word();
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xc4
****************************************************************************/
static void x86emuOp_les_R_IMM(X86EMU_sysEnv *env, u8 op1)
{
    int mod, rh, rl;
    u16 *dstreg;
    u32 srcoffset;

    START_OF_INSTR();
    DECODE_PRINTF("LES\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    if (mod != 3) {
        dstreg = DECODE_RM_WORD_REGISTER(rh);
        DECODE_PRINTF(",");
        srcoffset = decode_rm_address(mod, rl);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *dstreg = fetch_data_word(srcoffset);
        env->x86.R_ES = fetch_data_word(srcoffset + 2);
    } else {                     /* register to register */
        /* UNDEFINED! */
        TRACE_AND_STEP();
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xc5
****************************************************************************/
static void x86emuOp_lds_R_IMM(X86EMU_sysEnv *env, u8 op1)
{
    int mod, rh, rl;
    u16 *dstreg;
    u32 srcoffset;

    START_OF_INSTR();
    DECODE_PRINTF("LDS\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    if (mod != 3) {
        dstreg = DECODE_RM_WORD_REGISTER(rh);
        DECODE_PRINTF(",");
        srcoffset = decode_rm_address(mod, rl);
        DECODE_PRINTF("\n");
        TRACE_AND_STEP();
        *dstreg = fetch_data_word(srcoffset);
        env->x86.R_DS = fetch_data_word(srcoffset + 2);
    } else {                     /* register to register */
        /* UNDEFINED! */
        TRACE_AND_STEP();
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xc6
****************************************************************************/
static void x86emuOp_mov_byte_RM_IMM(X86EMU_sysEnv *env, u8 op1)
{
    int mod, rl, rh;
    u8 *destreg;
    u32 destoffset;
    u8 imm;

    START_OF_INSTR();
    DECODE_PRINTF("MOV\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    if (rh != 0) {
        DECODE_PRINTF("ILLEGAL DECODE OF OPCODE c6\n");
        HALT_SYS();
    }
    if (mod != 3) {
        DECODE_PRINTF("BYTE PTR ");
        destoffset = decode_rm_address(mod, rl);
        imm = fetch_byte_imm();
        DECODE_PRINTF2(",%02x\n", imm);
        TRACE_AND_STEP();
        store_data_byte(destoffset, imm);
    } else {                     /* register to register */
        destreg = DECODE_RM_BYTE_REGISTER(rl);
        imm = fetch_byte_imm();
        DECODE_PRINTF2(",%02x\n", imm);
        TRACE_AND_STEP();
        *destreg = imm;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xc7
****************************************************************************/
static void x86emuOp_mov_word_RM_IMM(X86EMU_sysEnv *env, u8 op1)
{
    int mod, rl, rh;
    u32 destoffset;

    START_OF_INSTR();
    DECODE_PRINTF("MOV\t");
    FETCH_DECODE_MODRM(mod, rh, rl);
    if (rh != 0) {
        DECODE_PRINTF("ILLEGAL DECODE OF OPCODE 8F\n");
        HALT_SYS();
    }
    if (mod != 3) {
        if (env->x86.mode & SYSMODE_PREFIX_DATA) {
            u32 imm;

            DECODE_PRINTF("DWORD PTR ");
            destoffset = decode_rm_address(mod, rl);
            imm = fetch_long_imm();
            DECODE_PRINTF2(",%08x\n", imm);
            TRACE_AND_STEP();
            store_data_long(destoffset, imm);
        } else {
            u16 imm;

            DECODE_PRINTF("WORD PTR ");
            destoffset = decode_rm_address(mod, rl);
            imm = fetch_word_imm();
            DECODE_PRINTF2(",%04x\n", imm);
            TRACE_AND_STEP();
            store_data_word(destoffset, imm);
        }
    } else {                     /* register to register */
        if (env->x86.mode & SYSMODE_PREFIX_DATA) {
	    u32 *destreg;
	    u32 imm;

            destreg = DECODE_RM_LONG_REGISTER(rl);
            imm = fetch_long_imm();
            DECODE_PRINTF2(",%08x\n", imm);
            TRACE_AND_STEP();
            *destreg = imm;
        } else {
	    u16 *destreg;
	    u16 imm;

            destreg = DECODE_RM_WORD_REGISTER(rl);
            imm = fetch_word_imm();
            DECODE_PRINTF2(",%04x\n", imm);
            TRACE_AND_STEP();
            *destreg = imm;
        }
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xc8
****************************************************************************/
static void x86emuOp_enter(X86EMU_sysEnv *env, u8 op1)
{
    u16 local,frame_pointer;
    u8  nesting;
    int i;

    START_OF_INSTR();
    local = fetch_word_imm();
    nesting = fetch_byte_imm();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
	DECODE_PRINTF2("ENTERD %04x\n", local);
	DECODE_PRINTF2(",%x\n", nesting);
	TRACE_AND_STEP();
	push_long(env->x86.R_BP);
	frame_pointer = env->x86.R_SP;
	if (nesting > 0) {
	    for (i = 1; i < nesting; i++) {
		env->x86.R_BP -= 4;
		push_long(fetch_data_word_abs(env->x86.R_SS, env->x86.R_BP));
	    }
	    push_long(frame_pointer);
	}
    } else {
	DECODE_PRINTF2("ENTER %04x\n", local);
	DECODE_PRINTF2(",%x\n", nesting);
	TRACE_AND_STEP();
	push_word(env->x86.R_BP);
	frame_pointer = env->x86.R_SP;
	if (nesting > 0) {
	    for (i = 1; i < nesting; i++) {
		env->x86.R_BP -= 2;
		push_word(fetch_data_word_abs(env->x86.R_SS, env->x86.R_BP));
	    }
	    push_word(frame_pointer);
	}
    }
    env->x86.R_BP = frame_pointer;
    env->x86.R_SP = (u16)(env->x86.R_SP - local);    
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xc9
****************************************************************************/
static void x86emuOp_leave(X86EMU_sysEnv *env, u8 op1)
{
    START_OF_INSTR();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
	DECODE_PRINTF("LEAVED\n");
	TRACE_AND_STEP();
	env->x86.R_SP = env->x86.R_BP;
	env->x86.R_BP = pop_long();
    } else {
	DECODE_PRINTF("LEAVE\n");
	TRACE_AND_STEP();
	env->x86.R_SP = env->x86.R_BP;
	env->x86.R_BP = pop_word();
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xca
****************************************************************************/
static void x86emuOp_ret_far_IMM(X86EMU_sysEnv *env, u8 op1)
{
    u16 imm;

    START_OF_INSTR();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
	DECODE_PRINTF("RETFD\t");
    } else {
	DECODE_PRINTF("RETF\t");
    }
    imm = fetch_word_imm();
    DECODE_PRINTF2("%04x\n", imm);
    RETURN_TRACE("RETF",env->x86.saved_cs,env->x86.saved_ip);
    TRACE_AND_STEP();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
	env->x86.R_IP = pop_long();
	env->x86.R_CS = pop_long();
    } else {
	env->x86.R_IP = pop_word();
	env->x86.R_CS = pop_word();
    }
    env->x86.R_SP += imm;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xcb
****************************************************************************/
static void x86emuOp_ret_far(X86EMU_sysEnv *env, u8 op1)
{
    START_OF_INSTR();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
	DECODE_PRINTF("RETFD\n");
    } else {
	DECODE_PRINTF("RETF\n");
    }
    RETURN_TRACE("RETF",env->x86.saved_cs,env->x86.saved_ip);
    TRACE_AND_STEP();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
	env->x86.R_IP = pop_long();
	env->x86.R_CS = pop_long();
    } else {
	env->x86.R_IP = pop_word();
	env->x86.R_CS = pop_word();
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xcc
****************************************************************************/
static void x86emuOp_int3(X86EMU_sysEnv *env, u8 op1)
{
    START_OF_INSTR();
    DECODE_PRINTF("INT 3\n");
    TRACE_AND_STEP();
    if (!env->intrfunc || !env->intrfunc(env, 3)) {
        push_word((u16)env->x86.R_FLG);
        CLEAR_FLAG(F_IF);
        CLEAR_FLAG(F_TF);
        push_word(env->x86.R_CS);
        env->x86.R_CS = mem_access_word(3 * 4 + 2);
        push_word(env->x86.R_IP);
        env->x86.R_IP = mem_access_word(3 * 4);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xcd
****************************************************************************/
static void x86emuOp_int_IMM(X86EMU_sysEnv *env, u8 op1)
{
    u8 intnum;

    START_OF_INSTR();
    DECODE_PRINTF("INT\t");
    intnum = fetch_byte_imm();
    DECODE_PRINTF2("%02x\n", intnum);
    TRACE_AND_STEP();
    if (!env->intrfunc || !env->intrfunc(env, intnum)) {
        push_word((u16)env->x86.R_FLG);
        CLEAR_FLAG(F_IF);
        CLEAR_FLAG(F_TF);
        push_word(env->x86.R_CS);
        env->x86.R_CS = mem_access_word(intnum * 4 + 2);
        push_word(env->x86.R_IP);
        env->x86.R_IP = mem_access_word(intnum * 4);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xce
****************************************************************************/
static void x86emuOp_into(X86EMU_sysEnv *env, u8 op1)
{
    START_OF_INSTR();
    DECODE_PRINTF("INTO\n");
    TRACE_AND_STEP();
    if (ACCESS_FLAG(F_OF)) {
	if (!env->intrfunc || !env->intrfunc(env, 4)) {
            push_word((u16)env->x86.R_FLG);
            CLEAR_FLAG(F_IF);
            CLEAR_FLAG(F_TF);
            push_word(env->x86.R_CS);
            env->x86.R_CS = mem_access_word(4 * 4 + 2);
            push_word(env->x86.R_IP);
            env->x86.R_IP = mem_access_word(4 * 4);
        }
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xcf
****************************************************************************/
static void x86emuOp_iret(X86EMU_sysEnv *env, u8 op1)
{
    START_OF_INSTR();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
	DECODE_PRINTF("IRETD\n");
	TRACE_AND_STEP();
	env->x86.R_IP = pop_long();
	env->x86.R_CS = pop_long();
	env->x86.R_FLG = pop_long();
    } else {
	DECODE_PRINTF("IRET\n");
	TRACE_AND_STEP();
	env->x86.R_IP = pop_word();
	env->x86.R_CS = pop_word();
	env->x86.R_FLG = pop_word();
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xd0
****************************************************************************/
static void x86emuOp_opcD0_byte_RM_1(X86EMU_sysEnv *env, u8 op1)
{
    int mod, rl, rh;
    u8 *destreg;
    u32 destoffset;
    u8 destval;

    /*
     * Yet another weirdo special case instruction format.  Part of
     * the opcode held below in "RH".  Doubly nested case would
     * result, except that the decoded instruction
     */
    START_OF_INSTR();
    FETCH_DECODE_MODRM(mod, rh, rl);
    DECODE_PRINTF2("%s\t", shift_op_names[rh]);
    /* know operation, decode the mod byte to find the addressing
       mode. */
    if (mod != 3) {
        DECODE_PRINTF("BYTE PTR ");
        destoffset = decode_rm_address(mod, rl);
        DECODE_PRINTF(",1\n");
        destval = fetch_data_byte(destoffset);
        TRACE_AND_STEP();
        destval = (*shift_byte_operation[rh]) (env, destval, 1);
        store_data_byte(destoffset, destval);
    } else {                     /* register to register */
        destreg = DECODE_RM_BYTE_REGISTER(rl);
        DECODE_PRINTF(",1\n");
        TRACE_AND_STEP();
        destval = (*shift_byte_operation[rh]) (env, *destreg, 1);
        *destreg = destval;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xd1
****************************************************************************/
static void x86emuOp_opcD1_word_RM_1(X86EMU_sysEnv *env, u8 op1)
{
    int mod, rl, rh;
    u32 destoffset;

    /*
     * Yet another weirdo special case instruction format.  Part of
     * the opcode held below in "RH".  Doubly nested case would
     * result, except that the decoded instruction
     */
    START_OF_INSTR();
    FETCH_DECODE_MODRM(mod, rh, rl);
    DECODE_PRINTF2("%s\t", shift_op_names[rh]);
    /* know operation, decode the mod byte to find the addressing
       mode. */
    if (mod != 3) {
        if (env->x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;

            DECODE_PRINTF("DWORD PTR ");
            destoffset = decode_rm_address(mod, rl);
            DECODE_PRINTF(",1\n");
            destval = fetch_data_long(destoffset);
            TRACE_AND_STEP();
            destval = (*shift_long_operation[rh]) (env, destval, 1);
            store_data_long(destoffset, destval);
        } else {
            u16 destval;

            DECODE_PRINTF("WORD PTR ");
            destoffset = decode_rm_address(mod, rl);
            DECODE_PRINTF(",1\n");
            destval = fetch_data_word(destoffset);
            TRACE_AND_STEP();
            destval = (*shift_word_operation[rh]) (env, destval, 1);
            store_data_word(destoffset, destval);
        }
    } else {                     /* register to register */
        if (env->x86.mode & SYSMODE_PREFIX_DATA) {
	    u32 destval;
	    u32 *destreg;

            destreg = DECODE_RM_LONG_REGISTER(rl);
            DECODE_PRINTF(",1\n");
            TRACE_AND_STEP();
            destval = (*shift_long_operation[rh]) (env, *destreg, 1);
            *destreg = destval;
        } else {
	    u16 destval;
	    u16 *destreg;

            destreg = DECODE_RM_WORD_REGISTER(rl);
            DECODE_PRINTF(",1\n");
            TRACE_AND_STEP();
            destval = (*shift_word_operation[rh]) (env, *destreg, 1);
            *destreg = destval;
        }
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xd2
****************************************************************************/
static void x86emuOp_opcD2_byte_RM_CL(X86EMU_sysEnv *env, u8 op1)
{
    int mod, rl, rh;
    u8 *destreg;
    u32 destoffset;
    u8 destval;
    u8 amt;

    /*
     * Yet another weirdo special case instruction format.  Part of
     * the opcode held below in "RH".  Doubly nested case would
     * result, except that the decoded instruction
     */
    START_OF_INSTR();
    FETCH_DECODE_MODRM(mod, rh, rl);
    DECODE_PRINTF2("%s\t", shift_op_names[rh]);
    /* know operation, decode the mod byte to find the addressing
       mode. */
    amt = env->x86.R_CL;
    if (mod != 3) {
        DECODE_PRINTF("BYTE PTR ");
        destoffset = decode_rm_address(mod, rl);
        DECODE_PRINTF(",CL\n");
        destval = fetch_data_byte(destoffset);
        TRACE_AND_STEP();
        destval = (*shift_byte_operation[rh]) (env, destval, amt);
        store_data_byte(destoffset, destval);
    } else {                     /* register to register */
        destreg = DECODE_RM_BYTE_REGISTER(rl);
        DECODE_PRINTF(",CL\n");
        TRACE_AND_STEP();
        destval = (*shift_byte_operation[rh]) (env, *destreg, amt);
        *destreg = destval;
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xd3
****************************************************************************/
static void x86emuOp_opcD3_word_RM_CL(X86EMU_sysEnv *env, u8 op1)
{
    int mod, rl, rh;
    u32 destoffset;
    u8 amt;

    /*
     * Yet another weirdo special case instruction format.  Part of
     * the opcode held below in "RH".  Doubly nested case would
     * result, except that the decoded instruction
     */
    START_OF_INSTR();
    FETCH_DECODE_MODRM(mod, rh, rl);
    DECODE_PRINTF2("%s\t", shift_op_names[rh]);
    /* know operation, decode the mod byte to find the addressing
       mode. */
    amt = env->x86.R_CL;
    if (mod != 3) {
        if (env->x86.mode & SYSMODE_PREFIX_DATA) {
            u32 destval;

            DECODE_PRINTF("DWORD PTR ");
            destoffset = decode_rm_address(mod, rl);
            DECODE_PRINTF(",CL\n");
            destval = fetch_data_long(destoffset);
            TRACE_AND_STEP();
            destval = (*shift_long_operation[rh]) (env, destval, amt);
            store_data_long(destoffset, destval);
        } else {
            u16 destval;

            DECODE_PRINTF("WORD PTR ");
            destoffset = decode_rm_address(mod, rl);
            DECODE_PRINTF(",CL\n");
            destval = fetch_data_word(destoffset);
            TRACE_AND_STEP();
            destval = (*shift_word_operation[rh]) (env, destval, amt);
            store_data_word(destoffset, destval);
        }
    } else {                     /* register to register */
        if (env->x86.mode & SYSMODE_PREFIX_DATA) {
            u32 *destreg;

            destreg = DECODE_RM_LONG_REGISTER(rl);
            DECODE_PRINTF(",CL\n");
            TRACE_AND_STEP();
            *destreg = (*shift_long_operation[rh]) (env, *destreg, amt);
        } else {
            u16 *destreg;

            destreg = DECODE_RM_WORD_REGISTER(rl);
            DECODE_PRINTF(",CL\n");
            TRACE_AND_STEP();
            *destreg = (*shift_word_operation[rh]) (env, *destreg, amt);
        }
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xd4
****************************************************************************/
static void x86emuOp_aam(X86EMU_sysEnv *env, u8 op1)
{
    u8 a;

    START_OF_INSTR();
    DECODE_PRINTF("AAM\n");
    a = fetch_byte_imm();      /* this is a stupid encoding. */
    if (a != 10) {
        DECODE_PRINTF("ERROR DECODING AAM\n");
        TRACE_REGS();
        HALT_SYS();
    }
    TRACE_AND_STEP();
    /* note the type change here --- returning AL and AH in AX. */
    env->x86.R_AX = aam_word(env->x86.R_AL);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xd5
****************************************************************************/
static void x86emuOp_aad(X86EMU_sysEnv *env, u8 op1)
{
    START_OF_INSTR();
    DECODE_PRINTF("AAD\n");
    (void) fetch_byte_imm();
    TRACE_AND_STEP();
    env->x86.R_AX = aad_word(env->x86.R_AX);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/* opcode 0xd6 ILLEGAL OPCODE */

/****************************************************************************
REMARKS:
Handles opcode 0xd7
****************************************************************************/
static void x86emuOp_xlat(X86EMU_sysEnv *env, u8 op1)
{
    u16 addr;

    START_OF_INSTR();
    DECODE_PRINTF("XLAT\n");
    TRACE_AND_STEP();
    addr = (u16)(env->x86.R_BX + (u8)env->x86.R_AL);
    env->x86.R_AL = fetch_data_byte(addr);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/* instuctions  D8 .. DF are in i87_ops.c */

/****************************************************************************
REMARKS:
Handles opcode 0xe0
****************************************************************************/
static void x86emuOp_loopne(X86EMU_sysEnv *env, u8 op1)
{
    s16 ip;

    START_OF_INSTR();
    DECODE_PRINTF("LOOPNE\t");
    ip = (s8) fetch_byte_imm();
    ip += (s16) env->x86.R_IP;
    DECODE_PRINTF2("%04x\n", ip);
    TRACE_AND_STEP();
    env->x86.R_CX -= 1;
    if (env->x86.R_CX != 0 && !ACCESS_FLAG(F_ZF))      /* CX != 0 and !ZF */
        env->x86.R_IP = ip;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xe1
****************************************************************************/
static void x86emuOp_loope(X86EMU_sysEnv *env, u8 op1)
{
    s16 ip;

    START_OF_INSTR();
    DECODE_PRINTF("LOOPE\t");
    ip = (s8) fetch_byte_imm();
    ip += (s16) env->x86.R_IP;
    DECODE_PRINTF2("%04x\n", ip);
    TRACE_AND_STEP();
    env->x86.R_CX -= 1;
    if (env->x86.R_CX != 0 && ACCESS_FLAG(F_ZF))       /* CX != 0 and ZF */
        env->x86.R_IP = ip;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xe2
****************************************************************************/
static void x86emuOp_loop(X86EMU_sysEnv *env, u8 op1)
{
    s16 ip;

    START_OF_INSTR();
    DECODE_PRINTF("LOOP\t");
    ip = (s8) fetch_byte_imm();
    ip += (s16) env->x86.R_IP;
    DECODE_PRINTF2("%04x\n", ip);
    TRACE_AND_STEP();
    env->x86.R_CX -= 1;
    if (env->x86.R_CX != 0)
        env->x86.R_IP = ip;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xe3
****************************************************************************/
static void x86emuOp_jcxz(X86EMU_sysEnv *env, u8 op1)
{
    u16 target;
    s8  offset;
    u32 reg;

    /* jump to byte offset if overflow flag is set */
    START_OF_INSTR();
    if (env->x86.mode & SYSMODE_PREFIX_ADDR) {
	DECODE_PRINTF("JECXZ\t");
	reg = env->x86.R_ECX;
    } else {
	DECODE_PRINTF("JCXZ\t");
	reg = env->x86.R_CX;
    }
    offset = (s8)fetch_byte_imm();
    target = (u16)(env->x86.R_IP + offset);
    DECODE_PRINTF2("%04x\n", target);
    TRACE_AND_STEP();
    if (reg == 0)
        env->x86.R_IP = target;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xe4
****************************************************************************/
static void x86emuOp_in_byte_AL_IMM(X86EMU_sysEnv *env, u8 op1)
{
    u8 port;

    START_OF_INSTR();
    DECODE_PRINTF("IN\t");
    port = (u8) fetch_byte_imm();
    DECODE_PRINTF2("%02x,AL\n", port);
    TRACE_AND_STEP();
    env->x86.R_AL = sys_inb(port);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xe5
****************************************************************************/
static void x86emuOp_in_word_AX_IMM(X86EMU_sysEnv *env, u8 op1)
{
    u8 port;

    START_OF_INSTR();
    DECODE_PRINTF("IN\t");
    port = (u8) fetch_byte_imm();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF2("EAX,%02x\n", port);
    } else {
        DECODE_PRINTF2("AX,%02x\n", port);
    }
    TRACE_AND_STEP();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
        env->x86.R_EAX = sys_inl(port);
    } else {
        env->x86.R_AX = sys_inw(port);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xe6
****************************************************************************/
static void x86emuOp_out_byte_IMM_AL(X86EMU_sysEnv *env, u8 op1)
{
    u8 port;

    START_OF_INSTR();
    DECODE_PRINTF("OUT\t");
    port = (u8) fetch_byte_imm();
    DECODE_PRINTF2("%02x,AL\n", port);
    TRACE_AND_STEP();
    sys_outb(port, env->x86.R_AL);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xe7
****************************************************************************/
static void x86emuOp_out_word_IMM_AX(X86EMU_sysEnv *env, u8 op1)
{
    u8 port;

    START_OF_INSTR();
    DECODE_PRINTF("OUT\t");
    port = (u8) fetch_byte_imm();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF2("%02x,EAX\n", port);
    } else {
        DECODE_PRINTF2("%02x,AX\n", port);
    }
    TRACE_AND_STEP();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
        sys_outl(port, env->x86.R_EAX);
    } else {
        sys_outw(port, env->x86.R_AX);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xe8
****************************************************************************/
static void x86emuOp_call_near_IMM(X86EMU_sysEnv *env, u8 op1)
{
    s32 ip;

    START_OF_INSTR();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
	DECODE_PRINTF("CALLD\t");
	ip = fetch_long_imm();
    } else {
	DECODE_PRINTF("CALL\t");
	ip = (s16)fetch_word_imm();
    }
    ip += (s16) env->x86.R_IP;    /* CHECK SIGN */
    DECODE_PRINTF2("%04x\n", (u16)ip);
    CALL_TRACE(env->x86.saved_cs, env->x86.saved_ip, env->x86.R_CS, ip, "");
    TRACE_AND_STEP();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
	push_long(env->x86.R_IP);
    } else {
	push_word(env->x86.R_IP);
    }
    env->x86.R_IP = ip;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xe9
****************************************************************************/
static void x86emuOp_jump_near_IMM(X86EMU_sysEnv *env, u8 op1)
{
    s32 ip;

    START_OF_INSTR();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
	DECODE_PRINTF("JMPD\t");
	ip = fetch_long_imm();
    } else {
	DECODE_PRINTF("JMP\t");
	ip = (s16)fetch_word_imm();
    }
    ip += (s16)env->x86.R_IP;
    DECODE_PRINTF2("%04x\n", (u16)ip);
    TRACE_AND_STEP();
    env->x86.R_IP = (u16)ip;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xea
****************************************************************************/
static void x86emuOp_jump_far_IMM(X86EMU_sysEnv *env, u8 op1)
{
    u16 cs, ip;

    START_OF_INSTR();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
	DECODE_PRINTF("JMPD\tFAR ");
	ip = fetch_long_imm();
    } else {
	DECODE_PRINTF("JMP\tFAR ");
	ip = fetch_word_imm();
    }
    cs = fetch_word_imm();
    DECODE_PRINTF2("%04x:", cs);
    DECODE_PRINTF2("%04x\n", ip);
    TRACE_AND_STEP();
    env->x86.R_IP = ip;
    env->x86.R_CS = cs;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xeb
****************************************************************************/
static void x86emuOp_jump_byte_IMM(X86EMU_sysEnv *env, u8 op1)
{
    u16 target;
    s8 offset;

    START_OF_INSTR();
    DECODE_PRINTF("JMP\t");
    offset = (s8)fetch_byte_imm();
    target = (u16)(env->x86.R_IP + offset);
    DECODE_PRINTF2("%04x\n", target);
    TRACE_AND_STEP();
    env->x86.R_IP = target;
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xec
****************************************************************************/
static void x86emuOp_in_byte_AL_DX(X86EMU_sysEnv *env, u8 op1)
{
    START_OF_INSTR();
    DECODE_PRINTF("IN\tAL,DX\n");
    TRACE_AND_STEP();
    env->x86.R_AL = sys_inb(env->x86.R_DX);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xed
****************************************************************************/
static void x86emuOp_in_word_AX_DX(X86EMU_sysEnv *env, u8 op1)
{
    START_OF_INSTR();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("IN\tEAX,DX\n");
    } else {
        DECODE_PRINTF("IN\tAX,DX\n");
    }
    TRACE_AND_STEP();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
        env->x86.R_EAX = sys_inl(env->x86.R_DX);
    } else {
        env->x86.R_AX = sys_inw(env->x86.R_DX);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xee
****************************************************************************/
static void x86emuOp_out_byte_DX_AL(X86EMU_sysEnv *env, u8 op1)
{
    START_OF_INSTR();
    DECODE_PRINTF("OUT\tDX,AL\n");
    TRACE_AND_STEP();
    sys_outb(env->x86.R_DX, env->x86.R_AL);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xef
****************************************************************************/
static void x86emuOp_out_word_DX_AX(X86EMU_sysEnv *env, u8 op1)
{
    START_OF_INSTR();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
        DECODE_PRINTF("OUT\tDX,EAX\n");
    } else {
        DECODE_PRINTF("OUT\tDX,AX\n");
    }
    TRACE_AND_STEP();
    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
        sys_outl(env->x86.R_DX, env->x86.R_EAX);
    } else {
        sys_outw(env->x86.R_DX, env->x86.R_AX);
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xf0
****************************************************************************/
static void x86emuOp_lock(X86EMU_sysEnv *env, u8 op1)
{
    START_OF_INSTR();
    DECODE_PRINTF("LOCK:\n");
    TRACE_AND_STEP();
    /* note no DECODE_CLEAR_SEGOVR here. */
    END_OF_INSTR();
}

/*opcode 0xf1 ILLEGAL OPERATION */

/****************************************************************************
REMARKS:
Handles opcode 0xf2
****************************************************************************/
static void x86emuOp_repne(X86EMU_sysEnv *env, u8 op1)
{
    START_OF_INSTR();
    DECODE_PRINTF("REPNE\n");
    TRACE_AND_STEP();
    env->x86.mode |= SYSMODE_PREFIX_REPNE;
    /* note no DECODE_CLEAR_SEGOVR here. */
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xf3
****************************************************************************/
static void x86emuOp_repe(X86EMU_sysEnv *env, u8 op1)
{
    START_OF_INSTR();
    DECODE_PRINTF("REPE\n");
    TRACE_AND_STEP();
    env->x86.mode |= SYSMODE_PREFIX_REPE;
    /* note no DECODE_CLEAR_SEGOVR here. */
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xf4
****************************************************************************/
static void x86emuOp_halt(X86EMU_sysEnv *env, u8 op1)
{
    START_OF_INSTR();
    DECODE_PRINTF("HALT\n");
    TRACE_AND_STEP();
    HALT_SYS();
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xf5
****************************************************************************/
static void x86emuOp_cmc(X86EMU_sysEnv *env, u8 op1)
{
    /* complement the carry flag. */
    START_OF_INSTR();
    DECODE_PRINTF("CMC\n");
    TRACE_AND_STEP();
    TOGGLE_FLAG(F_CF);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xf6
****************************************************************************/
static void x86emuOp_opcF6_byte_RM(X86EMU_sysEnv *env, u8 op1)
{
    int mod, rl, rh;
    u8 *destreg;
    u32 destoffset;
    u8 destval, srcval;

    /* long, drawn out code follows.  Double switch for a total
       of 32 cases.  */
    START_OF_INSTR();
    FETCH_DECODE_MODRM(mod, rh, rl);
    if (mod != 3) {                     /* mod=00 */
        switch (rh) {
        case 0:         /* test byte imm */
            DECODE_PRINTF("TEST\tBYTE PTR ");
            destoffset = decode_rm_address(mod, rl);
            DECODE_PRINTF(",");
            srcval = fetch_byte_imm();
            DECODE_PRINTF2("%02x\n", srcval);
            destval = fetch_data_byte(destoffset);
            TRACE_AND_STEP();
            test_byte(destval, srcval);
            break;
        case 1:
            DECODE_PRINTF("ILLEGAL OP MOD=00 RH=01 OP=F6\n");
            HALT_SYS();
            break;
        case 2:
            DECODE_PRINTF("NOT\tBYTE PTR ");
            destoffset = decode_rm_address(mod, rl);
            DECODE_PRINTF("\n");
            destval = fetch_data_byte(destoffset);
            TRACE_AND_STEP();
            destval = not_byte(destval);
            store_data_byte(destoffset, destval);
            break;
        case 3:
            DECODE_PRINTF("NEG\tBYTE PTR ");
            destoffset = decode_rm_address(mod, rl);
            DECODE_PRINTF("\n");
            destval = fetch_data_byte(destoffset);
            TRACE_AND_STEP();
            destval = neg_byte(destval);
            store_data_byte(destoffset, destval);
            break;
        case 4:
            DECODE_PRINTF("MUL\tBYTE PTR ");
            destoffset = decode_rm_address(mod, rl);
            DECODE_PRINTF("\n");
            destval = fetch_data_byte(destoffset);
            TRACE_AND_STEP();
            mul_byte(destval);
            break;
        case 5:
            DECODE_PRINTF("IMUL\tBYTE PTR ");
            destoffset = decode_rm_address(mod, rl);
            DECODE_PRINTF("\n");
            destval = fetch_data_byte(destoffset);
            TRACE_AND_STEP();
            imul_byte(destval);
            break;
        case 6:
            DECODE_PRINTF("DIV\tBYTE PTR ");
            destoffset = decode_rm_address(mod, rl);
            DECODE_PRINTF("\n");
            destval = fetch_data_byte(destoffset);
            TRACE_AND_STEP();
            div_byte(destval);
            break;
        case 7:
            DECODE_PRINTF("IDIV\tBYTE PTR ");
            destoffset = decode_rm_address(mod, rl);
            DECODE_PRINTF("\n");
            destval = fetch_data_byte(destoffset);
            TRACE_AND_STEP();
            idiv_byte(destval);
            break;
        }
    } else {                     /* mod=11 */
        switch (rh) {
        case 0:         /* test byte imm */
            DECODE_PRINTF("TEST\t");
            destreg = DECODE_RM_BYTE_REGISTER(rl);
            DECODE_PRINTF(",");
            srcval = fetch_byte_imm();
            DECODE_PRINTF2("%02x\n", srcval);
            TRACE_AND_STEP();
            test_byte(*destreg, srcval);
            break;
        case 1:
            DECODE_PRINTF("ILLEGAL OP MOD=00 RH=01 OP=F6\n");
            HALT_SYS();
            break;
        case 2:
            DECODE_PRINTF("NOT\t");
            destreg = DECODE_RM_BYTE_REGISTER(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = not_byte(*destreg);
            break;
        case 3:
            DECODE_PRINTF("NEG\t");
            destreg = DECODE_RM_BYTE_REGISTER(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            *destreg = neg_byte(*destreg);
            break;
        case 4:
            DECODE_PRINTF("MUL\t");
            destreg = DECODE_RM_BYTE_REGISTER(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            mul_byte(*destreg);      /*!!!  */
            break;
        case 5:
            DECODE_PRINTF("IMUL\t");
            destreg = DECODE_RM_BYTE_REGISTER(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            imul_byte(*destreg);
            break;
        case 6:
            DECODE_PRINTF("DIV\t");
            destreg = DECODE_RM_BYTE_REGISTER(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            div_byte(*destreg);
            break;
        case 7:
            DECODE_PRINTF("IDIV\t");
            destreg = DECODE_RM_BYTE_REGISTER(rl);
            DECODE_PRINTF("\n");
            TRACE_AND_STEP();
            idiv_byte(*destreg);
            break;
        }
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xf7
****************************************************************************/
static void x86emuOp_opcF7_word_RM(X86EMU_sysEnv *env, u8 op1)
{
    int mod, rl, rh;
    u32 destoffset;

    /* long, drawn out code follows.  Double switch for a total
       of 32 cases.  */
    START_OF_INSTR();
    FETCH_DECODE_MODRM(mod, rh, rl);
    if (mod != 3) {                     /* mod=00 */
        switch (rh) {
        case 0:         /* test word imm */
            if (env->x86.mode & SYSMODE_PREFIX_DATA) {
                u32 destval,srcval;

                DECODE_PRINTF("TEST\tDWORD PTR ");
                destoffset = decode_rm_address(mod, rl);
                DECODE_PRINTF(",");
                srcval = fetch_long_imm();
                DECODE_PRINTF2("%08x\n", srcval);
                destval = fetch_data_long(destoffset);
                TRACE_AND_STEP();
                test_long(destval, srcval);
            } else {
                u16 destval,srcval;

                DECODE_PRINTF("TEST\tWORD PTR ");
                destoffset = decode_rm_address(mod, rl);
                DECODE_PRINTF(",");
                srcval = fetch_word_imm();
                DECODE_PRINTF2("%04x\n", srcval);
                destval = fetch_data_word(destoffset);
                TRACE_AND_STEP();
                test_word(destval, srcval);
            }
            break;
        case 1:
            DECODE_PRINTF("ILLEGAL OP MOD=00 RH=01 OP=F7\n");
            HALT_SYS();
            break;
        case 2:
            if (env->x86.mode & SYSMODE_PREFIX_DATA) {
                u32 destval;

                DECODE_PRINTF("NOT\tDWORD PTR ");
                destoffset = decode_rm_address(mod, rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_long(destoffset);
                TRACE_AND_STEP();
                destval = not_long(destval);
                store_data_long(destoffset, destval);
            } else {
                u16 destval;

                DECODE_PRINTF("NOT\tWORD PTR ");
                destoffset = decode_rm_address(mod, rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_word(destoffset);
                TRACE_AND_STEP();
                destval = not_word(destval);
                store_data_word(destoffset, destval);
            }
            break;
        case 3:
            if (env->x86.mode & SYSMODE_PREFIX_DATA) {
                u32 destval;

                DECODE_PRINTF("NEG\tDWORD PTR ");
                destoffset = decode_rm_address(mod, rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_long(destoffset);
                TRACE_AND_STEP();
                destval = neg_long(destval);
                store_data_long(destoffset, destval);
            } else {
                u16 destval;

                DECODE_PRINTF("NEG\tWORD PTR ");
                destoffset = decode_rm_address(mod, rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_word(destoffset);
                TRACE_AND_STEP();
                destval = neg_word(destval);
                store_data_word(destoffset, destval);
            }
            break;
        case 4:
            if (env->x86.mode & SYSMODE_PREFIX_DATA) {
                u32 destval;

                DECODE_PRINTF("MUL\tDWORD PTR ");
                destoffset = decode_rm_address(mod, rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_long(destoffset);
                TRACE_AND_STEP();
                mul_long(destval);
            } else {
                u16 destval;

                DECODE_PRINTF("MUL\tWORD PTR ");
                destoffset = decode_rm_address(mod, rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_word(destoffset);
                TRACE_AND_STEP();
                mul_word(destval);
            }
            break;
        case 5:
            if (env->x86.mode & SYSMODE_PREFIX_DATA) {
                u32 destval;

                DECODE_PRINTF("IMUL\tDWORD PTR ");
                destoffset = decode_rm_address(mod, rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_long(destoffset);
                TRACE_AND_STEP();
                imul_long(destval);
            } else {
                u16 destval;

                DECODE_PRINTF("IMUL\tWORD PTR ");
                destoffset = decode_rm_address(mod, rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_word(destoffset);
                TRACE_AND_STEP();
                imul_word(destval);
            }
            break;
        case 6:
            if (env->x86.mode & SYSMODE_PREFIX_DATA) {
                u32 destval;

                DECODE_PRINTF("DIV\tDWORD PTR ");
                destoffset = decode_rm_address(mod, rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_long(destoffset);
                TRACE_AND_STEP();
                div_long(destval);
            } else {
                u16 destval;

                DECODE_PRINTF("DIV\tWORD PTR ");
                destoffset = decode_rm_address(mod, rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_word(destoffset);
                TRACE_AND_STEP();
                div_word(destval);
            }
            break;
        case 7:
            if (env->x86.mode & SYSMODE_PREFIX_DATA) {
                u32 destval;

                DECODE_PRINTF("IDIV\tDWORD PTR ");
                destoffset = decode_rm_address(mod, rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_long(destoffset);
                TRACE_AND_STEP();
                idiv_long(destval);
            } else {
                u16 destval;

                DECODE_PRINTF("IDIV\tWORD PTR ");
                destoffset = decode_rm_address(mod, rl);
                DECODE_PRINTF("\n");
                destval = fetch_data_word(destoffset);
                TRACE_AND_STEP();
                idiv_word(destval);
            }
            break;
        }
    } else {
        switch (rh) {
        case 0:         /* test word imm */
            if (env->x86.mode & SYSMODE_PREFIX_DATA) {
                u32 *destreg;
                u32 srcval;

                DECODE_PRINTF("TEST\t");
                destreg = DECODE_RM_LONG_REGISTER(rl);
                DECODE_PRINTF(",");
                srcval = fetch_long_imm();
                DECODE_PRINTF2("%08x\n", srcval);
                TRACE_AND_STEP();
                test_long(*destreg, srcval);
            } else {
                u16 *destreg;
                u16 srcval;

                DECODE_PRINTF("TEST\t");
                destreg = DECODE_RM_WORD_REGISTER(rl);
                DECODE_PRINTF(",");
                srcval = fetch_word_imm();
                DECODE_PRINTF2("%04x\n", srcval);
                TRACE_AND_STEP();
                test_word(*destreg, srcval);
            }
            break;
        case 1:
            DECODE_PRINTF("ILLEGAL OP MOD=00 RH=01 OP=F6\n");
            HALT_SYS();
            break;
        case 2:
            if (env->x86.mode & SYSMODE_PREFIX_DATA) {
                u32 *destreg;

                DECODE_PRINTF("NOT\t");
                destreg = DECODE_RM_LONG_REGISTER(rl);
                DECODE_PRINTF("\n");
                TRACE_AND_STEP();
                *destreg = not_long(*destreg);
            } else {
                u16 *destreg;

                DECODE_PRINTF("NOT\t");
                destreg = DECODE_RM_WORD_REGISTER(rl);
                DECODE_PRINTF("\n");
                TRACE_AND_STEP();
                *destreg = not_word(*destreg);
            }
            break;
        case 3:
            if (env->x86.mode & SYSMODE_PREFIX_DATA) {
                u32 *destreg;

                DECODE_PRINTF("NEG\t");
                destreg = DECODE_RM_LONG_REGISTER(rl);
                DECODE_PRINTF("\n");
                TRACE_AND_STEP();
                *destreg = neg_long(*destreg);
            } else {
                u16 *destreg;

                DECODE_PRINTF("NEG\t");
                destreg = DECODE_RM_WORD_REGISTER(rl);
                DECODE_PRINTF("\n");
                TRACE_AND_STEP();
                *destreg = neg_word(*destreg);
            }
            break;
        case 4:
            if (env->x86.mode & SYSMODE_PREFIX_DATA) {
                u32 *destreg;

                DECODE_PRINTF("MUL\t");
                destreg = DECODE_RM_LONG_REGISTER(rl);
                DECODE_PRINTF("\n");
                TRACE_AND_STEP();
                mul_long(*destreg);      /*!!!  */
            } else {
                u16 *destreg;

                DECODE_PRINTF("MUL\t");
                destreg = DECODE_RM_WORD_REGISTER(rl);
                DECODE_PRINTF("\n");
                TRACE_AND_STEP();
                mul_word(*destreg);      /*!!!  */
            }
            break;
        case 5:
            if (env->x86.mode & SYSMODE_PREFIX_DATA) {
                u32 *destreg;

                DECODE_PRINTF("IMUL\t");
                destreg = DECODE_RM_LONG_REGISTER(rl);
                DECODE_PRINTF("\n");
                TRACE_AND_STEP();
                imul_long(*destreg);
            } else {
                u16 *destreg;

                DECODE_PRINTF("IMUL\t");
                destreg = DECODE_RM_WORD_REGISTER(rl);
                DECODE_PRINTF("\n");
                TRACE_AND_STEP();
                imul_word(*destreg);
            }
            break;
        case 6:
            if (env->x86.mode & SYSMODE_PREFIX_DATA) {
                u32 *destreg;

                DECODE_PRINTF("DIV\t");
                destreg = DECODE_RM_LONG_REGISTER(rl);
                DECODE_PRINTF("\n");
                TRACE_AND_STEP();
                div_long(*destreg);
            } else {
                u16 *destreg;

                DECODE_PRINTF("DIV\t");
                destreg = DECODE_RM_WORD_REGISTER(rl);
                DECODE_PRINTF("\n");
                TRACE_AND_STEP();
                div_word(*destreg);
            }
            break;
        case 7:
            if (env->x86.mode & SYSMODE_PREFIX_DATA) {
                u32 *destreg;

                DECODE_PRINTF("IDIV\t");
                destreg = DECODE_RM_LONG_REGISTER(rl);
                DECODE_PRINTF("\n");
                TRACE_AND_STEP();
                idiv_long(*destreg);
            } else {
                u16 *destreg;

                DECODE_PRINTF("IDIV\t");
                destreg = DECODE_RM_WORD_REGISTER(rl);
                DECODE_PRINTF("\n");
                TRACE_AND_STEP();
                idiv_word(*destreg);
            }
            break;
        }
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xf8
****************************************************************************/
static void x86emuOp_clc(X86EMU_sysEnv *env, u8 op1)
{
    /* clear the carry flag. */
    START_OF_INSTR();
    DECODE_PRINTF("CLC\n");
    TRACE_AND_STEP();
    CLEAR_FLAG(F_CF);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xf9
****************************************************************************/
static void x86emuOp_stc(X86EMU_sysEnv *env, u8 op1)
{
    /* set the carry flag. */
    START_OF_INSTR();
    DECODE_PRINTF("STC\n");
    TRACE_AND_STEP();
    SET_FLAG(F_CF);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xfa
****************************************************************************/
static void x86emuOp_cli(X86EMU_sysEnv *env, u8 op1)
{
    /* clear interrupts. */
    START_OF_INSTR();
    DECODE_PRINTF("CLI\n");
    TRACE_AND_STEP();
    CLEAR_FLAG(F_IF);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xfb
****************************************************************************/
static void x86emuOp_sti(X86EMU_sysEnv *env, u8 op1)
{
    /* enable  interrupts. */
    START_OF_INSTR();
    DECODE_PRINTF("STI\n");
    TRACE_AND_STEP();
    SET_FLAG(F_IF);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xfc
****************************************************************************/
static void x86emuOp_cld(X86EMU_sysEnv *env, u8 op1)
{
    /* clear interrupts. */
    START_OF_INSTR();
    DECODE_PRINTF("CLD\n");
    TRACE_AND_STEP();
    CLEAR_FLAG(F_DF);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xfd
****************************************************************************/
static void x86emuOp_std(X86EMU_sysEnv *env, u8 op1)
{
    /* clear interrupts. */
    START_OF_INSTR();
    DECODE_PRINTF("STD\n");
    TRACE_AND_STEP();
    SET_FLAG(F_DF);
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xfe
****************************************************************************/
static void x86emuOp_opcFE_byte_RM(X86EMU_sysEnv *env, u8 op1)
{
    int mod, rh, rl;
    u8 destval;
    u32 destoffset;
    u8 *destreg;

    /* Yet another special case instruction. */
    START_OF_INSTR();
    FETCH_DECODE_MODRM(mod, rh, rl);
    /* XXX DECODE_PRINTF may be changed to something more
       general, so that it is important to leave the strings
       in the same format, even though the result is that the 
       above test is done twice. */

    switch (rh) {
    case 0:
	DECODE_PRINTF("INC\t");
	break;
    case 1:
	DECODE_PRINTF("DEC\t");
	break;
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
	DECODE_PRINTF2("ILLEGAL OP MAJOR OP 0xFE MINOR OP %x \n", mod);
	HALT_SYS();
	break;
    }
    if (mod != 3) {
        DECODE_PRINTF("BYTE PTR ");
        destoffset = decode_rm_address(mod, rl);
        DECODE_PRINTF("\n");
        switch (rh) {
        case 0:         /* inc word ptr ... */
            destval = fetch_data_byte(destoffset);
            TRACE_AND_STEP();
            destval = inc_byte(destval);
            store_data_byte(destoffset, destval);
            break;
        case 1:         /* dec word ptr ... */
            destval = fetch_data_byte(destoffset);
            TRACE_AND_STEP();
            destval = dec_byte(destval);
            store_data_byte(destoffset, destval);
            break;
        }
    } else {
        destreg = DECODE_RM_BYTE_REGISTER(rl);
        DECODE_PRINTF("\n");
        switch (rh) {
        case 0:
            TRACE_AND_STEP();
            *destreg = inc_byte(*destreg);
            break;
        case 1:
            TRACE_AND_STEP();
            *destreg = dec_byte(*destreg);
            break;
        }
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/****************************************************************************
REMARKS:
Handles opcode 0xff
****************************************************************************/
static void x86emuOp_opcFF_word_RM(X86EMU_sysEnv *env, u8 op1)
{
    int mod, rh, rl;

    /* Yet another special case instruction. */
    START_OF_INSTR();
    FETCH_DECODE_MODRM(mod, rh, rl);
    /* XXX DECODE_PRINTF may be changed to something more
       general, so that it is important to leave the strings
       in the same format, even though the result is that the
       above test is done twice. */

    switch (rh) {
    case 0:
	if (env->x86.mode & SYSMODE_PREFIX_DATA) {
	    DECODE_PRINTF("INC\tDWORD PTR ");
	} else {
	    DECODE_PRINTF("INC\tWORD PTR ");
	}
	break;
    case 1:
	if (env->x86.mode & SYSMODE_PREFIX_DATA) {
	    DECODE_PRINTF("DEC\tDWORD PTR ");
	} else {
	    DECODE_PRINTF("DEC\tWORD PTR ");
	}
	break;
    case 2:
	DECODE_PRINTF("CALL\t");
	break;
    case 3:
	DECODE_PRINTF("CALL\tFAR ");
	break;
    case 4:
	DECODE_PRINTF("JMP\t");
	break;
    case 5:
	DECODE_PRINTF("JMP\tFAR ");
	break;
    case 6:
	DECODE_PRINTF("PUSH\t");
	break;
    case 7:
	DECODE_PRINTF("ILLEGAL DECODING OF OPCODE FF\t");
	HALT_SYS();
	break;
    }

    if (mod != 3) {
	u32 destoffset = 0;
	destoffset = decode_rm_address(mod, rl);

	DECODE_PRINTF("\n");

        switch (rh) {
        case 0:         /* inc word ptr ... */
            if (env->x86.mode & SYSMODE_PREFIX_DATA) {
                u32 destval;

                destval = fetch_data_long(destoffset);
                TRACE_AND_STEP();
                destval = inc_long(destval);
                store_data_long(destoffset, destval);
            } else {
                u16 destval;

                destval = fetch_data_word(destoffset);
                TRACE_AND_STEP();
                destval = inc_word(destval);
                store_data_word(destoffset, destval);
            }
            break;
        case 1:         /* dec word ptr ... */
            if (env->x86.mode & SYSMODE_PREFIX_DATA) {
                u32 destval;

                destval = fetch_data_long(destoffset);
                TRACE_AND_STEP();
                destval = dec_long(destval);
                store_data_long(destoffset, destval);
            } else {
                u16 destval;

                destval = fetch_data_word(destoffset);
                TRACE_AND_STEP();
                destval = dec_word(destval);
                store_data_word(destoffset, destval);
            }
            break;
        case 2:         /* call word ptr ... */
	    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
		u32 destval;

		destval = fetch_data_long(destoffset);
		TRACE_AND_STEP();
		push_long(env->x86.R_IP);
		env->x86.R_IP = destval;
	    } else {
		u16 destval;

		destval = fetch_data_word(destoffset);
		TRACE_AND_STEP();
		push_word(env->x86.R_IP);
		env->x86.R_IP = destval;
	    }
            break;
        case 3:         /* call far ptr ... */
	    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
		u32 destval, destval2;

		destval = fetch_data_long(destoffset);
		destval2 = fetch_data_word(destoffset + 4);
		TRACE_AND_STEP();
		push_long(env->x86.R_CS);
		env->x86.R_CS = destval2;
		push_long(env->x86.R_IP);
		env->x86.R_IP = destval;		
	    } else {
		u16 destval, destval2;

		destval = fetch_data_word(destoffset);
		destval2 = fetch_data_word(destoffset + 2);
		TRACE_AND_STEP();
		push_word(env->x86.R_CS);
		env->x86.R_CS = destval2;
		push_word(env->x86.R_IP);
		env->x86.R_IP = destval;
	    }
            break;
        case 4:         /* jmp word ptr ... */
	    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
		u32 destval;
		
		destval = fetch_data_long(destoffset);
		TRACE_AND_STEP();
		env->x86.R_IP = destval;
	    } else {
		u16 destval;

		destval = fetch_data_word(destoffset);
		TRACE_AND_STEP();
		env->x86.R_IP = destval;
	    }
            break;
        case 5:         /* jmp far ptr ... */
	    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
		u32 destval, destval2;

		destval = fetch_data_long(destoffset);
		destval2 = fetch_data_word(destoffset + 4);
		TRACE_AND_STEP();
		env->x86.R_IP = destval;
		env->x86.R_CS = destval2;
	    } else {
		u16 destval, destval2;

		destval = fetch_data_word(destoffset);
		destval2 = fetch_data_word(destoffset + 2);
		TRACE_AND_STEP();
		env->x86.R_IP = destval;
		env->x86.R_CS = destval2;
	    }
            break;
        case 6:         /*  push word ptr ... */
            if (env->x86.mode & SYSMODE_PREFIX_DATA) {
                u32 destval;

                destval = fetch_data_long(destoffset);
                TRACE_AND_STEP();
                push_long(destval);
            } else {
                u16 destval;

                destval = fetch_data_word(destoffset);
                TRACE_AND_STEP();
                push_word(destval);
            }
            break;
        }	
    } else {
        switch (rh) {
        case 0:
            if (env->x86.mode & SYSMODE_PREFIX_DATA) {
                u32 *destreg;

                destreg = DECODE_RM_LONG_REGISTER(rl);
                DECODE_PRINTF("\n");
                TRACE_AND_STEP();
                *destreg = inc_long(*destreg);
            } else {
                u16 *destreg;

                destreg = DECODE_RM_WORD_REGISTER(rl);
                DECODE_PRINTF("\n");
                TRACE_AND_STEP();
                *destreg = inc_word(*destreg);
            }
            break;
        case 1:
            if (env->x86.mode & SYSMODE_PREFIX_DATA) {
                u32 *destreg;

                destreg = DECODE_RM_LONG_REGISTER(rl);
                DECODE_PRINTF("\n");
                TRACE_AND_STEP();
                *destreg = dec_long(*destreg);
            } else {
                u16 *destreg;

                destreg = DECODE_RM_WORD_REGISTER(rl);
                DECODE_PRINTF("\n");
                TRACE_AND_STEP();
                *destreg = dec_word(*destreg);
            }
            break;
        case 2:         /* call word ptr ... */
	    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
		u32 *destreg;

		destreg = DECODE_RM_LONG_REGISTER(rl);
		DECODE_PRINTF("\n");
		TRACE_AND_STEP();
		push_long(env->x86.R_IP);
		env->x86.R_IP = *destreg;
	    } else {
		u16 *destreg;

		destreg = DECODE_RM_WORD_REGISTER(rl);
		DECODE_PRINTF("\n");
		TRACE_AND_STEP();
		push_word(env->x86.R_IP);
		env->x86.R_IP = *destreg;
	    }
            break;
        case 3:         /* jmp far ptr ... */
            DECODE_PRINTF("OPERATION UNDEFINED 0XFF \n");
            TRACE_AND_STEP();
            HALT_SYS();
            break;

        case 4:         /* jmp  ... */
	    if (env->x86.mode & SYSMODE_PREFIX_DATA) {
		u32 *destreg;

		destreg = DECODE_RM_LONG_REGISTER(rl);
		DECODE_PRINTF("\n");
		TRACE_AND_STEP();
		env->x86.R_IP = *destreg;		
	    } else {
		u16 *destreg;

		destreg = DECODE_RM_WORD_REGISTER(rl);
		DECODE_PRINTF("\n");
		TRACE_AND_STEP();
		env->x86.R_IP = *destreg;
	    }
            break;
        case 5:         /* jmp far ptr ... */
            DECODE_PRINTF("OPERATION UNDEFINED 0XFF \n");
            TRACE_AND_STEP();
            HALT_SYS();
            break;
        case 6:  /*  push word ptr ... */
            if (env->x86.mode & SYSMODE_PREFIX_DATA) {
                u32 *destreg;

                destreg = DECODE_RM_LONG_REGISTER(rl);
                DECODE_PRINTF("\n");
                TRACE_AND_STEP();
                push_long(*destreg);
            } else {
                u16 *destreg;

                destreg = DECODE_RM_WORD_REGISTER(rl);
                DECODE_PRINTF("\n");
                TRACE_AND_STEP();
                push_word(*destreg);
            }
            break;
        }
    }
    DECODE_CLEAR_SEGOVR();
    END_OF_INSTR();
}

/***************************************************************************
 * Single byte operation code table:
 **************************************************************************/
void (*x86emu_optab[256])(X86EMU_sysEnv *, u8) =
{
/*  0x00 */ x86emuOp_alu_byte_RM_R,	/* add */
/*  0x01 */ x86emuOp_alu_word_RM_R,
/*  0x02 */ x86emuOp_alu_byte_R_RM,
/*  0x03 */ x86emuOp_alu_word_R_RM,
/*  0x04 */ x86emuOp_alu_byte_AL_IMM,
/*  0x05 */ x86emuOp_alu_word_AX_IMM,
/*  0x06 */ x86emuOp_push_SR,
/*  0x07 */ x86emuOp_pop_SR,

/*  0x08 */ x86emuOp_alu_byte_RM_R,	/* or */
/*  0x09 */ x86emuOp_alu_word_RM_R,
/*  0x0a */ x86emuOp_alu_byte_R_RM,
/*  0x0b */ x86emuOp_alu_word_R_RM,
/*  0x0c */ x86emuOp_alu_byte_AL_IMM,
/*  0x0d */ x86emuOp_alu_word_AX_IMM,
/*  0x0e */ x86emuOp_push_SR,
/*  0x0f */ x86emuOp_two_byte,

/*  0x10 */ x86emuOp_alu_byte_RM_R,	/* adc */
/*  0x11 */ x86emuOp_alu_word_RM_R,
/*  0x12 */ x86emuOp_alu_byte_R_RM,
/*  0x13 */ x86emuOp_alu_word_R_RM,
/*  0x14 */ x86emuOp_alu_byte_AL_IMM,
/*  0x15 */ x86emuOp_alu_word_AX_IMM,
/*  0x16 */ x86emuOp_push_SR,
/*  0x17 */ x86emuOp_pop_SR,

/*  0x18 */ x86emuOp_alu_byte_RM_R,	/* sbb */
/*  0x19 */ x86emuOp_alu_word_RM_R,
/*  0x1a */ x86emuOp_alu_byte_R_RM,
/*  0x1b */ x86emuOp_alu_word_R_RM,
/*  0x1c */ x86emuOp_alu_byte_AL_IMM,
/*  0x1d */ x86emuOp_alu_word_AX_IMM,
/*  0x1e */ x86emuOp_push_SR,
/*  0x1f */ x86emuOp_pop_SR,

/*  0x20 */ x86emuOp_alu_byte_RM_R,	/* and */
/*  0x21 */ x86emuOp_alu_word_RM_R,
/*  0x22 */ x86emuOp_alu_byte_R_RM,
/*  0x23 */ x86emuOp_alu_word_R_RM,
/*  0x24 */ x86emuOp_alu_byte_AL_IMM,
/*  0x25 */ x86emuOp_alu_word_AX_IMM,
/*  0x26 */ x86emuOp_segovr_ES,
/*  0x27 */ x86emuOp_daa,

/*  0x28 */ x86emuOp_alu_byte_RM_R,	/* sub */
/*  0x29 */ x86emuOp_alu_word_RM_R,
/*  0x2a */ x86emuOp_alu_byte_R_RM,
/*  0x2b */ x86emuOp_alu_word_R_RM,
/*  0x2c */ x86emuOp_alu_byte_AL_IMM,
/*  0x2d */ x86emuOp_alu_word_AX_IMM,
/*  0x2e */ x86emuOp_segovr_CS,
/*  0x2f */ x86emuOp_das,

/*  0x30 */ x86emuOp_alu_byte_RM_R,	/* xor */
/*  0x31 */ x86emuOp_alu_word_RM_R,
/*  0x32 */ x86emuOp_alu_byte_R_RM,
/*  0x33 */ x86emuOp_alu_word_R_RM,
/*  0x34 */ x86emuOp_alu_byte_AL_IMM,
/*  0x35 */ x86emuOp_alu_word_AX_IMM,
/*  0x36 */ x86emuOp_segovr_SS,
/*  0x37 */ x86emuOp_aaa,

/*  0x38 */ x86emuOp_alu_byte_RM_R,	/* cmp */
/*  0x39 */ x86emuOp_alu_word_RM_R,
/*  0x3a */ x86emuOp_alu_byte_R_RM,
/*  0x3b */ x86emuOp_alu_word_R_RM,
/*  0x3c */ x86emuOp_alu_byte_AL_IMM,
/*  0x3d */ x86emuOp_alu_word_AX_IMM,
/*  0x3e */ x86emuOp_segovr_DS,
/*  0x3f */ x86emuOp_aas,

/*  0x40 */ x86emuOp_inc_R,
/*  0x41 */ x86emuOp_inc_R,
/*  0x42 */ x86emuOp_inc_R,
/*  0x43 */ x86emuOp_inc_R,
/*  0x44 */ x86emuOp_inc_R,
/*  0x45 */ x86emuOp_inc_R,
/*  0x46 */ x86emuOp_inc_R,
/*  0x47 */ x86emuOp_inc_R,

/*  0x48 */ x86emuOp_dec_R,
/*  0x49 */ x86emuOp_dec_R,
/*  0x4a */ x86emuOp_dec_R,
/*  0x4b */ x86emuOp_dec_R,
/*  0x4c */ x86emuOp_dec_R,
/*  0x4d */ x86emuOp_dec_R,
/*  0x4e */ x86emuOp_dec_R,
/*  0x4f */ x86emuOp_dec_R,

/*  0x50 */ x86emuOp_push_R,
/*  0x51 */ x86emuOp_push_R,
/*  0x52 */ x86emuOp_push_R,
/*  0x53 */ x86emuOp_push_R,
/*  0x54 */ x86emuOp_push_R,
/*  0x55 */ x86emuOp_push_R,
/*  0x56 */ x86emuOp_push_R,
/*  0x57 */ x86emuOp_push_R,

/*  0x58 */ x86emuOp_pop_R,
/*  0x59 */ x86emuOp_pop_R,
/*  0x5a */ x86emuOp_pop_R,
/*  0x5b */ x86emuOp_pop_R,
/*  0x5c */ x86emuOp_pop_R,
/*  0x5d */ x86emuOp_pop_R,
/*  0x5e */ x86emuOp_pop_R,
/*  0x5f */ x86emuOp_pop_R,

/*  0x60 */ x86emuOp_push_all,
/*  0x61 */ x86emuOp_pop_all,
/*  0x62 */ x86emuOp_illegal_op,   /* bound */
/*  0x63 */ x86emuOp_illegal_op,   /* arpl */
/*  0x64 */ x86emuOp_segovr_FS,
/*  0x65 */ x86emuOp_segovr_GS,
/*  0x66 */ x86emuOp_prefix_data,
/*  0x67 */ x86emuOp_prefix_addr,

/*  0x68 */ x86emuOp_push_word_IMM,
/*  0x69 */ x86emuOp_imul_word_IMM,
/*  0x6a */ x86emuOp_push_byte_IMM,
/*  0x6b */ x86emuOp_imul_byte_IMM,
/*  0x6c */ x86emuOp_ins_byte,
/*  0x6d */ x86emuOp_ins_word,
/*  0x6e */ x86emuOp_outs_byte,
/*  0x6f */ x86emuOp_outs_word,

/*  0x70 */ x86emuOp_jump_near_cc,
/*  0x71 */ x86emuOp_jump_near_cc,
/*  0x72 */ x86emuOp_jump_near_cc,
/*  0x73 */ x86emuOp_jump_near_cc,
/*  0x74 */ x86emuOp_jump_near_cc,
/*  0x75 */ x86emuOp_jump_near_cc,
/*  0x76 */ x86emuOp_jump_near_cc,
/*  0x77 */ x86emuOp_jump_near_cc,

/*  0x78 */ x86emuOp_jump_near_cc,
/*  0x79 */ x86emuOp_jump_near_cc,
/*  0x7a */ x86emuOp_jump_near_cc,
/*  0x7b */ x86emuOp_jump_near_cc,
/*  0x7c */ x86emuOp_jump_near_cc,
/*  0x7d */ x86emuOp_jump_near_cc,
/*  0x7e */ x86emuOp_jump_near_cc,
/*  0x7f */ x86emuOp_jump_near_cc,

/*  0x80 */ x86emuOp_opc80_byte_RM_IMM,
/*  0x81 */ x86emuOp_opc81_word_RM_IMM,
/*  0x82 */ x86emuOp_illegal_op,
/*  0x83 */ x86emuOp_opc83_word_RM_IMM,
/*  0x84 */ x86emuOp_test_byte_RM_R,
/*  0x85 */ x86emuOp_test_word_RM_R,
/*  0x86 */ x86emuOp_xchg_byte_RM_R,
/*  0x87 */ x86emuOp_xchg_word_RM_R,

/*  0x88 */ x86emuOp_mov_byte_RM_R,
/*  0x89 */ x86emuOp_mov_word_RM_R,
/*  0x8a */ x86emuOp_mov_byte_R_RM,
/*  0x8b */ x86emuOp_mov_word_R_RM,
/*  0x8c */ x86emuOp_mov_word_RM_SR,
/*  0x8d */ x86emuOp_lea_word_R_M,
/*  0x8e */ x86emuOp_mov_word_SR_RM,
/*  0x8f */ x86emuOp_pop_RM,

/*  0x90 */ x86emuOp_xchg_word_AX_R,
/*  0x91 */ x86emuOp_xchg_word_AX_R,
/*  0x92 */ x86emuOp_xchg_word_AX_R,
/*  0x93 */ x86emuOp_xchg_word_AX_R,
/*  0x94 */ x86emuOp_xchg_word_AX_R,
/*  0x95 */ x86emuOp_xchg_word_AX_R,
/*  0x96 */ x86emuOp_xchg_word_AX_R,
/*  0x97 */ x86emuOp_xchg_word_AX_R,

/*  0x98 */ x86emuOp_cbw,
/*  0x99 */ x86emuOp_cwd,
/*  0x9a */ x86emuOp_call_far_IMM,
/*  0x9b */ x86emuOp_wait,
/*  0x9c */ x86emuOp_pushf_word,
/*  0x9d */ x86emuOp_popf_word,
/*  0x9e */ x86emuOp_sahf,
/*  0x9f */ x86emuOp_lahf,

/*  0xa0 */ x86emuOp_mov_AL_M_IMM,
/*  0xa1 */ x86emuOp_mov_AX_M_IMM,
/*  0xa2 */ x86emuOp_mov_M_AL_IMM,
/*  0xa3 */ x86emuOp_mov_M_AX_IMM,
/*  0xa4 */ x86emuOp_movs_byte,
/*  0xa5 */ x86emuOp_movs_word,
/*  0xa6 */ x86emuOp_cmps_byte,
/*  0xa7 */ x86emuOp_cmps_word,
/*  0xa8 */ x86emuOp_test_AL_IMM,
/*  0xa9 */ x86emuOp_test_AX_IMM,
/*  0xaa */ x86emuOp_stos_byte,
/*  0xab */ x86emuOp_stos_word,
/*  0xac */ x86emuOp_lods_byte,
/*  0xad */ x86emuOp_lods_word,
/*  0xae */ x86emuOp_scas_byte,
/*  0xaf */ x86emuOp_scas_word,

/*  0xb0 */ x86emuOp_mov_byte_R_IMM,
/*  0xb1 */ x86emuOp_mov_byte_R_IMM,
/*  0xb2 */ x86emuOp_mov_byte_R_IMM,
/*  0xb3 */ x86emuOp_mov_byte_R_IMM,
/*  0xb4 */ x86emuOp_mov_byte_R_IMM,
/*  0xb5 */ x86emuOp_mov_byte_R_IMM,
/*  0xb6 */ x86emuOp_mov_byte_R_IMM,
/*  0xb7 */ x86emuOp_mov_byte_R_IMM,

/*  0xb8 */ x86emuOp_mov_word_R_IMM,
/*  0xb9 */ x86emuOp_mov_word_R_IMM,
/*  0xba */ x86emuOp_mov_word_R_IMM,
/*  0xbb */ x86emuOp_mov_word_R_IMM,
/*  0xbc */ x86emuOp_mov_word_R_IMM,
/*  0xbd */ x86emuOp_mov_word_R_IMM,
/*  0xbe */ x86emuOp_mov_word_R_IMM,
/*  0xbf */ x86emuOp_mov_word_R_IMM,

/*  0xc0 */ x86emuOp_opcC0_byte_RM_MEM,
/*  0xc1 */ x86emuOp_opcC1_word_RM_MEM,
/*  0xc2 */ x86emuOp_ret_near_IMM,
/*  0xc3 */ x86emuOp_ret_near,
/*  0xc4 */ x86emuOp_les_R_IMM,
/*  0xc5 */ x86emuOp_lds_R_IMM,
/*  0xc6 */ x86emuOp_mov_byte_RM_IMM,
/*  0xc7 */ x86emuOp_mov_word_RM_IMM,
/*  0xc8 */ x86emuOp_enter,
/*  0xc9 */ x86emuOp_leave,
/*  0xca */ x86emuOp_ret_far_IMM,
/*  0xcb */ x86emuOp_ret_far,
/*  0xcc */ x86emuOp_int3,
/*  0xcd */ x86emuOp_int_IMM,
/*  0xce */ x86emuOp_into,
/*  0xcf */ x86emuOp_iret,

/*  0xd0 */ x86emuOp_opcD0_byte_RM_1,
/*  0xd1 */ x86emuOp_opcD1_word_RM_1,
/*  0xd2 */ x86emuOp_opcD2_byte_RM_CL,
/*  0xd3 */ x86emuOp_opcD3_word_RM_CL,
/*  0xd4 */ x86emuOp_aam,
/*  0xd5 */ x86emuOp_aad,
/*  0xd6 */ x86emuOp_illegal_op,   /* Undocumented SETALC instruction */
/*  0xd7 */ x86emuOp_xlat,
/*  0xd8 */ x86emuOp_illegal_op, /* x86emuOp_esc_coprocess_d8, */
/*  0xd9 */ x86emuOp_illegal_op, /* x86emuOp_esc_coprocess_d9, */
/*  0xda */ x86emuOp_illegal_op, /* x86emuOp_esc_coprocess_da, */
/*  0xdb */ x86emuOp_illegal_op, /* x86emuOp_esc_coprocess_db, */
/*  0xdc */ x86emuOp_illegal_op, /* x86emuOp_esc_coprocess_dc, */
/*  0xdd */ x86emuOp_illegal_op, /* x86emuOp_esc_coprocess_dd, */
/*  0xde */ x86emuOp_illegal_op, /* x86emuOp_esc_coprocess_de, */
/*  0xdf */ x86emuOp_illegal_op, /* x86emuOp_esc_coprocess_df, */

/*  0xe0 */ x86emuOp_loopne,
/*  0xe1 */ x86emuOp_loope,
/*  0xe2 */ x86emuOp_loop,
/*  0xe3 */ x86emuOp_jcxz,
/*  0xe4 */ x86emuOp_in_byte_AL_IMM,
/*  0xe5 */ x86emuOp_in_word_AX_IMM,
/*  0xe6 */ x86emuOp_out_byte_IMM_AL,
/*  0xe7 */ x86emuOp_out_word_IMM_AX,

/*  0xe8 */ x86emuOp_call_near_IMM,
/*  0xe9 */ x86emuOp_jump_near_IMM,
/*  0xea */ x86emuOp_jump_far_IMM,
/*  0xeb */ x86emuOp_jump_byte_IMM,
/*  0xec */ x86emuOp_in_byte_AL_DX,
/*  0xed */ x86emuOp_in_word_AX_DX,
/*  0xee */ x86emuOp_out_byte_DX_AL,
/*  0xef */ x86emuOp_out_word_DX_AX,

/*  0xf0 */ x86emuOp_lock,
/*  0xf1 */ x86emuOp_illegal_op,
/*  0xf2 */ x86emuOp_repne,
/*  0xf3 */ x86emuOp_repe,
/*  0xf4 */ x86emuOp_halt,
/*  0xf5 */ x86emuOp_cmc,
/*  0xf6 */ x86emuOp_opcF6_byte_RM,
/*  0xf7 */ x86emuOp_opcF7_word_RM,

/*  0xf8 */ x86emuOp_clc,
/*  0xf9 */ x86emuOp_stc,
/*  0xfa */ x86emuOp_cli,
/*  0xfb */ x86emuOp_sti,
/*  0xfc */ x86emuOp_cld,
/*  0xfd */ x86emuOp_std,
/*  0xfe */ x86emuOp_opcFE_byte_RM,
/*  0xff */ x86emuOp_opcFF_word_RM,
};
