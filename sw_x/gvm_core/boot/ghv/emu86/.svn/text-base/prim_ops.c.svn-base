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
* Description:  This file contains the code to implement the primitive
*				machine operations used by the emulation code in ops.c
*
* Carry Chain Calculation
*
* This represents a somewhat expensive calculation which is
* apparently required to emulate the setting of the OF and AF flag.
* The latter is not so important, but the former is.  The overflow
* flag is the XOR of the top two bits of the carry chain for an
* addition (similar for subtraction).  Since we do not want to
* simulate the addition in a bitwise manner, we try to calculate the
* carry chain given the two operands and the result.
*
* So, given the following table, which represents the addition of two
* bits, we can derive a formula for the carry chain.
*
* a   b   cin   r     cout
* 0   0   0     0     0
* 0   0   1     1     0
* 0   1   0     1     0
* 0   1   1     0     1
* 1   0   0     1     0
* 1   0   1     0     1
* 1   1   0     0     1
* 1   1   1     1     1
*
* Construction of table for cout:
*
* ab
* r  \  00   01   11  10
* |------------------
* 0  |   0    1    1   1
* 1  |   0    0    1   0
*
* By inspection, one gets:  cc = ab +  r'(a + b)
*
* That represents alot of operations, but NO CHOICE....
*
* Borrow Chain Calculation.
*
* The following table represents the subtraction of two bits, from
* which we can derive a formula for the borrow chain.
*
* a   b   bin   r     bout
* 0   0   0     0     0
* 0   0   1     1     1
* 0   1   0     1     1
* 0   1   1     0     1
* 1   0   0     1     0
* 1   0   1     0     0
* 1   1   0     0     0
* 1   1   1     1     1
*
* Construction of table for cout:
*
* ab
* r  \  00   01   11  10
* |------------------
* 0  |   0    1    0   0
* 1  |   1    1    1   0
*
* By inspection, one gets:  bc = a'b +  r(a' + b)
*
****************************************************************************/

#define	PRIM_OPS_NO_REDEFINE_ASM
#include "x86emu/x86emui.h"

/*------------------------- Global Variables ------------------------------*/

#ifndef	__HAVE_INLINE_ASSEMBLER__

static u32 x86emu_parity_tab[8] =
    {
	0x96696996,
	0x69969669,
	0x69969669,
	0x96696996,
	0x69969669,
	0x96696996,
	0x96696996,
	0x69969669,
    };

#endif

#define PARITY(x)   (((x86emu_parity_tab[(x) / 32] >> ((x) % 32)) & 1) == 0)
#define XOR2(x) 	(((x) ^ ((x)>>1)) & 0x1)

static inline int
abs(int x)
{
    if (x < 0) {
	return -x;
    }
    return x;
}

/*----------------------------- Implementation ----------------------------*/

/****************************************************************************
REMARKS:
Implements the AAA instruction and side effects.
****************************************************************************/
u16 x86emuPrimOp_aaa_word(X86EMU_sysEnv *env, u16 d)
{
    u16	res;
    if ((d & 0xf) > 0x9 || ACCESS_FLAG(F_AF)) {
	d += 0x6;
	d += 0x100;
	SET_FLAG(F_AF);
	SET_FLAG(F_CF);
    } else {
	CLEAR_FLAG(F_CF);
	CLEAR_FLAG(F_AF);
    }
    res = (u16)(d & 0xFF0F);
    CLEAR_FLAG(F_SF);
    CONDITIONAL_SET_FLAG(res == 0, F_ZF);
    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);
    return res;
}

/****************************************************************************
REMARKS:
Implements the AAA instruction and side effects.
****************************************************************************/
u16 x86emuPrimOp_aas_word(X86EMU_sysEnv *env, u16 d)
{
    u16	res;
    if ((d & 0xf) > 0x9 || ACCESS_FLAG(F_AF)) {
	d -= 0x6;
	d -= 0x100;
	SET_FLAG(F_AF);
	SET_FLAG(F_CF);
    } else {
	CLEAR_FLAG(F_CF);
	CLEAR_FLAG(F_AF);
    }
    res = (u16)(d & 0xFF0F);
    CLEAR_FLAG(F_SF);
    CONDITIONAL_SET_FLAG(res == 0, F_ZF);
    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);
    return res;
}

/****************************************************************************
REMARKS:
Implements the AAD instruction and side effects.
****************************************************************************/
u16 x86emuPrimOp_aad_word(X86EMU_sysEnv *env, u16 d)
{
    u16 l;
    u8 hb, lb;

    hb = (u8)((d >> 8) & 0xff);
    lb = (u8)((d & 0xff));
    l = (u16)((lb + 10 * hb) & 0xFF);

    CLEAR_FLAG(F_CF);
    CLEAR_FLAG(F_AF);
    CLEAR_FLAG(F_OF);
    CONDITIONAL_SET_FLAG(l & 0x80, F_SF);
    CONDITIONAL_SET_FLAG(l == 0, F_ZF);
    CONDITIONAL_SET_FLAG(PARITY(l & 0xff), F_PF);
    return l;
}

/****************************************************************************
REMARKS:
Implements the AAM instruction and side effects.
****************************************************************************/
u16 x86emuPrimOp_aam_word(X86EMU_sysEnv *env, u8 d)
{
    u16 h, l;

    h = (u16)(d / 10);
    l = (u16)(d % 10);
    l |= (u16)(h << 8);

    CLEAR_FLAG(F_CF);
    CLEAR_FLAG(F_AF);
    CLEAR_FLAG(F_OF);
    CONDITIONAL_SET_FLAG(l & 0x80, F_SF);
    CONDITIONAL_SET_FLAG(l == 0, F_ZF);
    CONDITIONAL_SET_FLAG(PARITY(l & 0xff), F_PF);
    return l;
}

/****************************************************************************
REMARKS:
Implements the ADC instruction and side effects.
****************************************************************************/
u8 x86emuPrimOp_adc_byte(X86EMU_sysEnv *env, u8 d, u8 s)
{
    register u32 res;   /* all operands in native machine order */
    register u32 cc;

    if (ACCESS_FLAG(F_CF))
	res = 1 + d + s;
    else
	res = d + s;

    CONDITIONAL_SET_FLAG(res & 0x100, F_CF);
    CONDITIONAL_SET_FLAG((res & 0xff) == 0, F_ZF);
    CONDITIONAL_SET_FLAG(res & 0x80, F_SF);
    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);

    /* calculate the carry chain  SEE NOTE AT TOP. */
    cc = (s & d) | ((~res) & (s | d));
    CONDITIONAL_SET_FLAG(XOR2(cc >> 6), F_OF);
    CONDITIONAL_SET_FLAG(cc & 0x8, F_AF);
    return (u8)res;
}

/****************************************************************************
REMARKS:
Implements the ADC instruction and side effects.
****************************************************************************/
u16 x86emuPrimOp_adc_word(X86EMU_sysEnv *env, u16 d, u16 s)
{
    register u32 res;   /* all operands in native machine order */
    register u32 cc;

    if (ACCESS_FLAG(F_CF))
	res = 1 + d + s;
    else
	res = d + s;

    CONDITIONAL_SET_FLAG(res & 0x10000, F_CF);
    CONDITIONAL_SET_FLAG((res & 0xffff) == 0, F_ZF);
    CONDITIONAL_SET_FLAG(res & 0x8000, F_SF);
    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);

    /* calculate the carry chain  SEE NOTE AT TOP. */
    cc = (s & d) | ((~res) & (s | d));
    CONDITIONAL_SET_FLAG(XOR2(cc >> 14), F_OF);
    CONDITIONAL_SET_FLAG(cc & 0x8, F_AF);
    return (u16)res;
}

/****************************************************************************
REMARKS:
Implements the ADC instruction and side effects.
****************************************************************************/
u32 x86emuPrimOp_adc_long(X86EMU_sysEnv *env, u32 d, u32 s)
{
    register u32 lo;	/* all operands in native machine order */
    register u32 hi;
    register u32 res;
    register u32 cc;

    if (ACCESS_FLAG(F_CF)) {
	lo = 1 + (d & 0xFFFF) + (s & 0xFFFF);
	res = 1 + d + s;
    }
    else {
	lo = (d & 0xFFFF) + (s & 0xFFFF);
	res = d + s;
    }
    hi = (lo >> 16) + (d >> 16) + (s >> 16);

    CONDITIONAL_SET_FLAG(hi & 0x10000, F_CF);
    CONDITIONAL_SET_FLAG((res & 0xffffffff) == 0, F_ZF);
    CONDITIONAL_SET_FLAG(res & 0x80000000, F_SF);
    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);

    /* calculate the carry chain  SEE NOTE AT TOP. */
    cc = (s & d) | ((~res) & (s | d));
    CONDITIONAL_SET_FLAG(XOR2(cc >> 30), F_OF);
    CONDITIONAL_SET_FLAG(cc & 0x8, F_AF);
    return res;
}

/****************************************************************************
REMARKS:
Implements the ADD instruction and side effects.
****************************************************************************/
u8 x86emuPrimOp_add_byte(X86EMU_sysEnv *env, u8 d, u8 s)
{
    register u32 res;   /* all operands in native machine order */
    register u32 cc;

    res = d + s;
    CONDITIONAL_SET_FLAG(res & 0x100, F_CF);
    CONDITIONAL_SET_FLAG((res & 0xff) == 0, F_ZF);
    CONDITIONAL_SET_FLAG(res & 0x80, F_SF);
    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);

    /* calculate the carry chain  SEE NOTE AT TOP. */
    cc = (s & d) | ((~res) & (s | d));
    CONDITIONAL_SET_FLAG(XOR2(cc >> 6), F_OF);
    CONDITIONAL_SET_FLAG(cc & 0x8, F_AF);
    return (u8)res;
}

/****************************************************************************
REMARKS:
Implements the ADD instruction and side effects.
****************************************************************************/
u16 x86emuPrimOp_add_word(X86EMU_sysEnv *env, u16 d, u16 s)
{
    register u32 res;   /* all operands in native machine order */
    register u32 cc;

    res = d + s;
    CONDITIONAL_SET_FLAG(res & 0x10000, F_CF);
    CONDITIONAL_SET_FLAG((res & 0xffff) == 0, F_ZF);
    CONDITIONAL_SET_FLAG(res & 0x8000, F_SF);
    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);

    /* calculate the carry chain  SEE NOTE AT TOP. */
    cc = (s & d) | ((~res) & (s | d));
    CONDITIONAL_SET_FLAG(XOR2(cc >> 14), F_OF);
    CONDITIONAL_SET_FLAG(cc & 0x8, F_AF);
    return (u16)res;
}

/****************************************************************************
REMARKS:
Implements the ADD instruction and side effects.
****************************************************************************/
u32 x86emuPrimOp_add_long(X86EMU_sysEnv *env, u32 d, u32 s)
{
    register u32 lo;	/* all operands in native machine order */
    register u32 hi;
    register u32 res;
    register u32 cc;

    lo = (d & 0xFFFF) + (s & 0xFFFF);
    res = d + s;
    hi = (lo >> 16) + (d >> 16) + (s >> 16);

    CONDITIONAL_SET_FLAG(hi & 0x10000, F_CF);
    CONDITIONAL_SET_FLAG((res & 0xffffffff) == 0, F_ZF);
    CONDITIONAL_SET_FLAG(res & 0x80000000, F_SF);
    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);

    /* calculate the carry chain  SEE NOTE AT TOP. */
    cc = (s & d) | ((~res) & (s | d));
    CONDITIONAL_SET_FLAG(XOR2(cc >> 30), F_OF);
    CONDITIONAL_SET_FLAG(cc & 0x8, F_AF);

    return res;
}

/****************************************************************************
REMARKS:
Implements the AND instruction and side effects.
****************************************************************************/
u8 x86emuPrimOp_and_byte(X86EMU_sysEnv *env, u8 d, u8 s)
{
    register u8 res;    /* all operands in native machine order */

    res = d & s;

    /* set the flags  */
    CLEAR_FLAG(F_OF);
    CLEAR_FLAG(F_CF);
    CLEAR_FLAG(F_AF);
    CONDITIONAL_SET_FLAG(res & 0x80, F_SF);
    CONDITIONAL_SET_FLAG(res == 0, F_ZF);
    CONDITIONAL_SET_FLAG(PARITY(res), F_PF);
    return res;
}

/****************************************************************************
REMARKS:
Implements the AND instruction and side effects.
****************************************************************************/
u16 x86emuPrimOp_and_word(X86EMU_sysEnv *env, u16 d, u16 s)
{
    register u16 res;   /* all operands in native machine order */

    res = d & s;

    /* set the flags  */
    CLEAR_FLAG(F_OF);
    CLEAR_FLAG(F_CF);
    CLEAR_FLAG(F_AF);
    CONDITIONAL_SET_FLAG(res & 0x8000, F_SF);
    CONDITIONAL_SET_FLAG(res == 0, F_ZF);
    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);
    return res;
}

/****************************************************************************
REMARKS:
Implements the AND instruction and side effects.
****************************************************************************/
u32 x86emuPrimOp_and_long(X86EMU_sysEnv *env, u32 d, u32 s)
{
    register u32 res;   /* all operands in native machine order */

    res = d & s;

    /* set the flags  */
    CLEAR_FLAG(F_OF);
    CLEAR_FLAG(F_CF);
    CLEAR_FLAG(F_AF);
    CONDITIONAL_SET_FLAG(res & 0x80000000, F_SF);
    CONDITIONAL_SET_FLAG(res == 0, F_ZF);
    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);
    return res;
}

/****************************************************************************
REMARKS:
Implements the CMP instruction and side effects.
****************************************************************************/
u8 x86emuPrimOp_cmp_byte(X86EMU_sysEnv *env, u8 d, u8 s)
{
    register u32 res;   /* all operands in native machine order */
    register u32 bc;

    res = d - s;
    CLEAR_FLAG(F_CF);
    CONDITIONAL_SET_FLAG(res & 0x80, F_SF);
    CONDITIONAL_SET_FLAG((res & 0xff) == 0, F_ZF);
    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);

    /* calculate the borrow chain.  See note at top */
    bc = (res & (~d | s)) | (~d & s);
    CONDITIONAL_SET_FLAG(bc & 0x80, F_CF);
    CONDITIONAL_SET_FLAG(XOR2(bc >> 6), F_OF);
    CONDITIONAL_SET_FLAG(bc & 0x8, F_AF);
    return d;
}

/****************************************************************************
REMARKS:
Implements the CMP instruction and side effects.
****************************************************************************/
u16 x86emuPrimOp_cmp_word(X86EMU_sysEnv *env, u16 d, u16 s)
{
    register u32 res;   /* all operands in native machine order */
    register u32 bc;

    res = d - s;
    CONDITIONAL_SET_FLAG(res & 0x8000, F_SF);
    CONDITIONAL_SET_FLAG((res & 0xffff) == 0, F_ZF);
    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);

    /* calculate the borrow chain.  See note at top */
    bc = (res & (~d | s)) | (~d & s);
    CONDITIONAL_SET_FLAG(bc & 0x8000, F_CF);
    CONDITIONAL_SET_FLAG(XOR2(bc >> 14), F_OF);
    CONDITIONAL_SET_FLAG(bc & 0x8, F_AF);
    return d;
}

/****************************************************************************
REMARKS:
Implements the CMP instruction and side effects.
****************************************************************************/
u32 x86emuPrimOp_cmp_long(X86EMU_sysEnv *env, u32 d, u32 s)
{
    register u32 res;   /* all operands in native machine order */
    register u32 bc;

    res = d - s;
    CONDITIONAL_SET_FLAG(res & 0x80000000, F_SF);
    CONDITIONAL_SET_FLAG((res & 0xffffffff) == 0, F_ZF);
    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);

    /* calculate the borrow chain.  See note at top */
    bc = (res & (~d | s)) | (~d & s);
    CONDITIONAL_SET_FLAG(bc & 0x80000000, F_CF);
    CONDITIONAL_SET_FLAG(XOR2(bc >> 30), F_OF);
    CONDITIONAL_SET_FLAG(bc & 0x8, F_AF);
    return d;
}

/****************************************************************************
REMARKS:
Implements the DAA instruction and side effects.
****************************************************************************/
u8 x86emuPrimOp_daa_byte(X86EMU_sysEnv *env, u8 d)
{
    u32 res = d;
    if ((d & 0xf) > 9 || ACCESS_FLAG(F_AF)) {
	res += 6;
	SET_FLAG(F_AF);
    }
    if (res > 0x9F || ACCESS_FLAG(F_CF)) {
	res += 0x60;
	SET_FLAG(F_CF);
    }
    CONDITIONAL_SET_FLAG(res & 0x80, F_SF);
    CONDITIONAL_SET_FLAG((res & 0xFF) == 0, F_ZF);
    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);
    return (u8)res;
}

/****************************************************************************
REMARKS:
Implements the DAS instruction and side effects.
****************************************************************************/
u8 x86emuPrimOp_das_byte(X86EMU_sysEnv *env, u8 d)
{
    if ((d & 0xf) > 9 || ACCESS_FLAG(F_AF)) {
	d -= 6;
	SET_FLAG(F_AF);
    }
    if (d > 0x9F || ACCESS_FLAG(F_CF)) {
	d -= 0x60;
	SET_FLAG(F_CF);
    }
    CONDITIONAL_SET_FLAG(d & 0x80, F_SF);
    CONDITIONAL_SET_FLAG(d == 0, F_ZF);
    CONDITIONAL_SET_FLAG(PARITY(d & 0xff), F_PF);
    return d;
}

/****************************************************************************
REMARKS:
Implements the DEC instruction and side effects.
****************************************************************************/
u8 x86emuPrimOp_dec_byte(X86EMU_sysEnv *env, u8 d)
{
    register u32 res;   /* all operands in native machine order */
    register u32 bc;

    res = d - 1;
    CONDITIONAL_SET_FLAG(res & 0x80, F_SF);
    CONDITIONAL_SET_FLAG((res & 0xff) == 0, F_ZF);
    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);

    /* calculate the borrow chain.  See note at top */
    /* based on sub_byte, uses s==1.  */
    bc = (res & (~d | 1)) | (~d & 1);
    /* carry flag unchanged */
    CONDITIONAL_SET_FLAG(XOR2(bc >> 6), F_OF);
    CONDITIONAL_SET_FLAG(bc & 0x8, F_AF);
    return (u8)res;
}

/****************************************************************************
REMARKS:
Implements the DEC instruction and side effects.
****************************************************************************/
u16 x86emuPrimOp_dec_word(X86EMU_sysEnv *env, u16 d)
{
    register u32 res;   /* all operands in native machine order */
    register u32 bc;

    res = d - 1;
    CONDITIONAL_SET_FLAG(res & 0x8000, F_SF);
    CONDITIONAL_SET_FLAG((res & 0xffff) == 0, F_ZF);
    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);

    /* calculate the borrow chain.  See note at top */
    /* based on the sub_byte routine, with s==1 */
    bc = (res & (~d | 1)) | (~d & 1);
    /* carry flag unchanged */
    CONDITIONAL_SET_FLAG(XOR2(bc >> 14), F_OF);
    CONDITIONAL_SET_FLAG(bc & 0x8, F_AF);
    return (u16)res;
}

/****************************************************************************
REMARKS:
Implements the DEC instruction and side effects.
****************************************************************************/
u32 x86emuPrimOp_dec_long(X86EMU_sysEnv *env, u32 d)
{
    register u32 res;   /* all operands in native machine order */
    register u32 bc;

    res = d - 1;

    CONDITIONAL_SET_FLAG(res & 0x80000000, F_SF);
    CONDITIONAL_SET_FLAG((res & 0xffffffff) == 0, F_ZF);
    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);

    /* calculate the borrow chain.  See note at top */
    bc = (res & (~d | 1)) | (~d & 1);
    /* carry flag unchanged */
    CONDITIONAL_SET_FLAG(XOR2(bc >> 30), F_OF);
    CONDITIONAL_SET_FLAG(bc & 0x8, F_AF);
    return res;
}

/****************************************************************************
REMARKS:
Implements the INC instruction and side effects.
****************************************************************************/
u8 x86emuPrimOp_inc_byte(X86EMU_sysEnv *env, u8 d)
{
    register u32 res;   /* all operands in native machine order */
    register u32 cc;

    res = d + 1;
    CONDITIONAL_SET_FLAG((res & 0xff) == 0, F_ZF);
    CONDITIONAL_SET_FLAG(res & 0x80, F_SF);
    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);

    /* calculate the carry chain  SEE NOTE AT TOP. */
    cc = ((1 & d) | (~res)) & (1 | d);
    CONDITIONAL_SET_FLAG(XOR2(cc >> 6), F_OF);
    CONDITIONAL_SET_FLAG(cc & 0x8, F_AF);
    return (u8)res;
}

/****************************************************************************
REMARKS:
Implements the INC instruction and side effects.
****************************************************************************/
u16 x86emuPrimOp_inc_word(X86EMU_sysEnv *env, u16 d)
{
    register u32 res;   /* all operands in native machine order */
    register u32 cc;

    res = d + 1;
    CONDITIONAL_SET_FLAG((res & 0xffff) == 0, F_ZF);
    CONDITIONAL_SET_FLAG(res & 0x8000, F_SF);
    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);

    /* calculate the carry chain  SEE NOTE AT TOP. */
    cc = (1 & d) | ((~res) & (1 | d));
    CONDITIONAL_SET_FLAG(XOR2(cc >> 14), F_OF);
    CONDITIONAL_SET_FLAG(cc & 0x8, F_AF);
    return (u16)res;
}

/****************************************************************************
REMARKS:
Implements the INC instruction and side effects.
****************************************************************************/
u32 x86emuPrimOp_inc_long(X86EMU_sysEnv *env, u32 d)
{
    register u32 res;   /* all operands in native machine order */
    register u32 cc;

    res = d + 1;
    CONDITIONAL_SET_FLAG((res & 0xffffffff) == 0, F_ZF);
    CONDITIONAL_SET_FLAG(res & 0x80000000, F_SF);
    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);

    /* calculate the carry chain  SEE NOTE AT TOP. */
    cc = (1 & d) | ((~res) & (1 | d));
    CONDITIONAL_SET_FLAG(XOR2(cc >> 30), F_OF);
    CONDITIONAL_SET_FLAG(cc & 0x8, F_AF);
    return res;
}

/****************************************************************************
REMARKS:
Implements the OR instruction and side effects.
****************************************************************************/
u8 x86emuPrimOp_or_byte(X86EMU_sysEnv *env, u8 d, u8 s)
{
    register u8 res;    /* all operands in native machine order */

    res = d | s;
    CLEAR_FLAG(F_OF);
    CLEAR_FLAG(F_CF);
    CLEAR_FLAG(F_AF);
    CONDITIONAL_SET_FLAG(res & 0x80, F_SF);
    CONDITIONAL_SET_FLAG(res == 0, F_ZF);
    CONDITIONAL_SET_FLAG(PARITY(res), F_PF);
    return res;
}

/****************************************************************************
REMARKS:
Implements the OR instruction and side effects.
****************************************************************************/
u16 x86emuPrimOp_or_word(X86EMU_sysEnv *env, u16 d, u16 s)
{
    register u16 res;   /* all operands in native machine order */

    res = d | s;
    /* set the carry flag to be bit 8 */
    CLEAR_FLAG(F_OF);
    CLEAR_FLAG(F_CF);
    CLEAR_FLAG(F_AF);
    CONDITIONAL_SET_FLAG(res & 0x8000, F_SF);
    CONDITIONAL_SET_FLAG(res == 0, F_ZF);
    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);
    return res;
}

/****************************************************************************
REMARKS:
Implements the OR instruction and side effects.
****************************************************************************/
u32 x86emuPrimOp_or_long(X86EMU_sysEnv *env, u32 d, u32 s)
{
    register u32 res;   /* all operands in native machine order */

    res = d | s;

    /* set the carry flag to be bit 8 */
    CLEAR_FLAG(F_OF);
    CLEAR_FLAG(F_CF);
    CLEAR_FLAG(F_AF);
    CONDITIONAL_SET_FLAG(res & 0x80000000, F_SF);
    CONDITIONAL_SET_FLAG(res == 0, F_ZF);
    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);
    return res;
}

/****************************************************************************
REMARKS:
Implements the OR instruction and side effects.
****************************************************************************/
u8 x86emuPrimOp_neg_byte(X86EMU_sysEnv *env, u8 s)
{
    register u8 res;
    register u8 bc;

    CONDITIONAL_SET_FLAG(s != 0, F_CF);
    res = (u8)-s;
    CONDITIONAL_SET_FLAG((res & 0xff) == 0, F_ZF);
    CONDITIONAL_SET_FLAG(res & 0x80, F_SF);
    CONDITIONAL_SET_FLAG(PARITY(res), F_PF);
    /* calculate the borrow chain --- modified such that d=0.
       substitutiing d=0 into     bc= res&(~d|s)|(~d&s);
       (the one used for sub) and simplifying, since ~d=0xff...,
       ~d|s == 0xffff..., and res&0xfff... == res.  Similarly
       ~d&s == s.  So the simplified result is: */
    bc = res | s;
    CONDITIONAL_SET_FLAG(XOR2(bc >> 6), F_OF);
    CONDITIONAL_SET_FLAG(bc & 0x8, F_AF);
    return res;
}

/****************************************************************************
REMARKS:
Implements the OR instruction and side effects.
****************************************************************************/
u16 x86emuPrimOp_neg_word(X86EMU_sysEnv *env, u16 s)
{
    register u16 res;
    register u16 bc;

    CONDITIONAL_SET_FLAG(s != 0, F_CF);
    res = (u16)-s;
    CONDITIONAL_SET_FLAG((res & 0xffff) == 0, F_ZF);
    CONDITIONAL_SET_FLAG(res & 0x8000, F_SF);
    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);

    /* calculate the borrow chain --- modified such that d=0.
       substitutiing d=0 into     bc= res&(~d|s)|(~d&s);
       (the one used for sub) and simplifying, since ~d=0xff...,
       ~d|s == 0xffff..., and res&0xfff... == res.  Similarly
       ~d&s == s.  So the simplified result is: */
    bc = res | s;
    CONDITIONAL_SET_FLAG(XOR2(bc >> 14), F_OF);
    CONDITIONAL_SET_FLAG(bc & 0x8, F_AF);
    return res;
}

/****************************************************************************
REMARKS:
Implements the OR instruction and side effects.
****************************************************************************/
u32 x86emuPrimOp_neg_long(X86EMU_sysEnv *env, u32 s)
{
    register u32 res;
    register u32 bc;

    CONDITIONAL_SET_FLAG(s != 0, F_CF);
    res = (u32)-s;
    CONDITIONAL_SET_FLAG((res & 0xffffffff) == 0, F_ZF);
    CONDITIONAL_SET_FLAG(res & 0x80000000, F_SF);
    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);

    /* calculate the borrow chain --- modified such that d=0.
       substitutiing d=0 into     bc= res&(~d|s)|(~d&s);
       (the one used for sub) and simplifying, since ~d=0xff...,
       ~d|s == 0xffff..., and res&0xfff... == res.  Similarly
       ~d&s == s.  So the simplified result is: */
    bc = res | s;
    CONDITIONAL_SET_FLAG(XOR2(bc >> 30), F_OF);
    CONDITIONAL_SET_FLAG(bc & 0x8, F_AF);
    return res;
}

/****************************************************************************
REMARKS:
Implements the NOT instruction and side effects.
****************************************************************************/
u8 x86emuPrimOp_not_byte(X86EMU_sysEnv *env, u8 s)
{
    return ~s;
}

/****************************************************************************
REMARKS:
Implements the NOT instruction and side effects.
****************************************************************************/
u16 x86emuPrimOp_not_word(X86EMU_sysEnv *env, u16 s)
{
    return ~s;
}

/****************************************************************************
REMARKS:
Implements the NOT instruction and side effects.
****************************************************************************/
u32 x86emuPrimOp_not_long(X86EMU_sysEnv *env, u32 s)
{
    return ~s;
}

/****************************************************************************
REMARKS:
Implements the RCL instruction and side effects.
****************************************************************************/
u8 x86emuPrimOp_rcl_byte(X86EMU_sysEnv *env, u8 d, u8 s)
{
    register unsigned int res, cnt, mask, cf;

    /* s is the rotate distance.  It varies from 0 - 8. */
    /* have

       CF  B_7 B_6 B_5 B_4 B_3 B_2 B_1 B_0 

       want to rotate through the carry by "s" bits.  We could 
       loop, but that's inefficient.  So the width is 9,
       and we split into three parts:

       The new carry flag   (was B_n)
       the stuff in B_n-1 .. B_0
       the stuff in B_7 .. B_n+1

       The new rotate is done mod 9, and given this,
       for a rotation of n bits (mod 9) the new carry flag is
       then located n bits from the MSB.  The low part is 
       then shifted up cnt bits, and the high part is or'd
       in.  Using CAPS for new values, and lowercase for the 
       original values, this can be expressed as:

       IF n > 0 
       1) CF <-  b_(8-n)
       2) B_(7) .. B_(n)  <-  b_(8-(n+1)) .. b_0
       3) B_(n-1) <- cf
       4) B_(n-2) .. B_0 <-  b_7 .. b_(8-(n-1))
    */
    res = d;
    if ((cnt = s % 9) != 0) {
        /* extract the new CARRY FLAG. */
        /* CF <-  b_(8-n)             */
        cf = (d >> (8 - cnt)) & 0x1;

        /* get the low stuff which rotated 
           into the range B_7 .. B_cnt */
        /* B_(7) .. B_(n)  <-  b_(8-(n+1)) .. b_0  */
        /* note that the right hand side done by the mask */
	res = (d << cnt) & 0xff;

        /* now the high stuff which rotated around 
           into the positions B_cnt-2 .. B_0 */
        /* B_(n-2) .. B_0 <-  b_7 .. b_(8-(n-1)) */
        /* shift it downward, 7-(n-2) = 9-n positions. 
           and mask off the result before or'ing in. 
	*/
        mask = (1 << (cnt - 1)) - 1;
        res |= (d >> (9 - cnt)) & mask;

        /* if the carry flag was set, or it in.  */
	if (ACCESS_FLAG(F_CF)) {     /* carry flag is set */
            /*  B_(n-1) <- cf */
            res |= 1 << (cnt - 1);
        }
        /* set the new carry flag, based on the variable "cf" */
	CONDITIONAL_SET_FLAG(cf, F_CF);
        /* OVERFLOW is set *IFF* cnt==1, then it is the 
           xor of CF and the most significant bit.  Blecck. */
        /* parenthesized this expression since it appears to
           be causing OF to be misset */
        CONDITIONAL_SET_FLAG(cnt == 1 && XOR2(cf + ((res >> 6) & 0x2)),
			     F_OF);

    }
    return (u8)res;
}

/****************************************************************************
REMARKS:
Implements the RCL instruction and side effects.
****************************************************************************/
u16 x86emuPrimOp_rcl_word(X86EMU_sysEnv *env, u16 d, u8 s)
{
    register unsigned int res, cnt, mask, cf;

    res = d;
    if ((cnt = s % 17) != 0) {
	cf = (d >> (16 - cnt)) & 0x1;
	res = (d << cnt) & 0xffff;
	mask = (1 << (cnt - 1)) - 1;
	res |= (d >> (17 - cnt)) & mask;
	if (ACCESS_FLAG(F_CF)) {
	    res |= 1 << (cnt - 1);
	}
	CONDITIONAL_SET_FLAG(cf, F_CF);
	CONDITIONAL_SET_FLAG(cnt == 1 && XOR2(cf + ((res >> 14) & 0x2)),
			     F_OF);
    }
    return (u16)res;
}

/****************************************************************************
REMARKS:
Implements the RCL instruction and side effects.
****************************************************************************/
u32 x86emuPrimOp_rcl_long(X86EMU_sysEnv *env, u32 d, u8 s)
{
    register u32 res, cnt, mask, cf;

    res = d;
    if ((cnt = s % 33) != 0) {
	cf = (d >> (32 - cnt)) & 0x1;
	res = (d << cnt) & 0xffffffff;
	mask = (1 << (cnt - 1)) - 1;
	res |= (d >> (33 - cnt)) & mask;
	if (ACCESS_FLAG(F_CF)) {     /* carry flag is set */
	    res |= 1 << (cnt - 1);
	}
	CONDITIONAL_SET_FLAG(cf, F_CF);
	CONDITIONAL_SET_FLAG(cnt == 1 && XOR2(cf + ((res >> 30) & 0x2)),
			     F_OF);
    }
    return res;
}

/****************************************************************************
REMARKS:
Implements the RCR instruction and side effects.
****************************************************************************/
u8 x86emuPrimOp_rcr_byte(X86EMU_sysEnv *env, u8 d, u8 s)
{
    u32	res, cnt;
    u32	mask, cf, ocf = 0;

    /* rotate right through carry */
    /* 
       s is the rotate distance.  It varies from 0 - 8.
       d is the byte object rotated.  

       have 

       CF  B_7 B_6 B_5 B_4 B_3 B_2 B_1 B_0 

       The new rotate is done mod 9, and given this,
       for a rotation of n bits (mod 9) the new carry flag is
       then located n bits from the LSB.  The low part is 
       then shifted up cnt bits, and the high part is or'd
       in.  Using CAPS for new values, and lowercase for the 
       original values, this can be expressed as:

       IF n > 0 
       1) CF <-  b_(n-1)
       2) B_(8-(n+1)) .. B_(0)  <-  b_(7) .. b_(n)
       3) B_(8-n) <- cf
       4) B_(7) .. B_(8-(n-1)) <-  b_(n-2) .. b_(0)
    */
    res = d;
    if ((cnt = s % 9) != 0) {
        /* extract the new CARRY FLAG. */
        /* CF <-  b_(n-1)              */
        if (cnt == 1) {
            cf = d & 0x1;
            /* note hackery here.  Access_flag(..) evaluates to either
               0 if flag not set
               non-zero if flag is set.
               doing access_flag(..) != 0 casts that into either 
	       0..1 in any representation of the flags register
               (i.e. packed bit array or unpacked.)
	    */
	    ocf = ACCESS_FLAG(F_CF) != 0;
        } else
            cf = (d >> (cnt - 1)) & 0x1;

        /* B_(8-(n+1)) .. B_(0)  <-  b_(7) .. b_n  */
        /* note that the right hand side done by the mask
           This is effectively done by shifting the 
           object to the right.  The result must be masked,
           in case the object came in and was treated 
           as a negative number.  Needed??? */

        mask = (1 << (8 - cnt)) - 1;
        res = (d >> cnt) & mask;

        /* now the high stuff which rotated around 
           into the positions B_cnt-2 .. B_0 */
        /* B_(7) .. B_(8-(n-1)) <-  b_(n-2) .. b_(0) */
        /* shift it downward, 7-(n-2) = 9-n positions. 
           and mask off the result before or'ing in. 
	*/
        res |= (d << (9 - cnt));

        /* if the carry flag was set, or it in.  */
	if (ACCESS_FLAG(F_CF)) {     /* carry flag is set */
            /*  B_(8-n) <- cf */
            res |= 1 << (8 - cnt);
        }
        /* set the new carry flag, based on the variable "cf" */
	CONDITIONAL_SET_FLAG(cf, F_CF);
        /* OVERFLOW is set *IFF* cnt==1, then it is the 
           xor of CF and the most significant bit.  Blecck. */
        /* parenthesized... */
	if (cnt == 1) {
	    CONDITIONAL_SET_FLAG(XOR2(ocf + ((d >> 6) & 0x2)),
				 F_OF);
	}
    }
    return (u8)res;
}

/****************************************************************************
REMARKS:
Implements the RCR instruction and side effects.
****************************************************************************/
u16 x86emuPrimOp_rcr_word(X86EMU_sysEnv *env, u16 d, u8 s)
{
    u32 res, cnt;
    u32	mask, cf, ocf = 0;

    /* rotate right through carry */
    res = d;
    if ((cnt = s % 17) != 0) {
	if (cnt == 1) {
	    cf = d & 0x1;
	    ocf = ACCESS_FLAG(F_CF) != 0;
	} else
	    cf = (d >> (cnt - 1)) & 0x1;
	mask = (1 << (16 - cnt)) - 1;
	res = (d >> cnt) & mask;
	res |= (d << (17 - cnt));
	if (ACCESS_FLAG(F_CF)) {
	    res |= 1 << (16 - cnt);
	}
	CONDITIONAL_SET_FLAG(cf, F_CF);
	if (cnt == 1) {
	    CONDITIONAL_SET_FLAG(XOR2(ocf + ((d >> 14) & 0x2)),
				 F_OF);
	}
    }
    return (u16)res;
}

/****************************************************************************
REMARKS:
Implements the RCR instruction and side effects.
****************************************************************************/
u32 x86emuPrimOp_rcr_long(X86EMU_sysEnv *env, u32 d, u8 s)
{
    u32 res, cnt;
    u32 mask, cf, ocf = 0;

    /* rotate right through carry */
    res = d;
    if ((cnt = s % 33) != 0) {
	if (cnt == 1) {
	    cf = d & 0x1;
	    ocf = ACCESS_FLAG(F_CF) != 0;
	} else
	    cf = (d >> (cnt - 1)) & 0x1;
	mask = (1 << (32 - cnt)) - 1;
	res = (d >> cnt) & mask;
	if (cnt != 1)
	    res |= (d << (33 - cnt));
	if (ACCESS_FLAG(F_CF)) {     /* carry flag is set */
	    res |= 1 << (32 - cnt);
	}
	CONDITIONAL_SET_FLAG(cf, F_CF);
	if (cnt == 1) {
	    CONDITIONAL_SET_FLAG(XOR2(ocf + ((d >> 30) & 0x2)),
				 F_OF);
	}
    }
    return res;
}

/****************************************************************************
REMARKS:
Implements the ROL instruction and side effects.
****************************************************************************/
u8 x86emuPrimOp_rol_byte(X86EMU_sysEnv *env, u8 d, u8 s)
{
    register unsigned int res, cnt, mask;

    /* rotate left */
    /* 
       s is the rotate distance.  It varies from 0 - 8.
       d is the byte object rotated.  

       have 

       CF  B_7 ... B_0 

       The new rotate is done mod 8.
       Much simpler than the "rcl" or "rcr" operations.

       IF n > 0 
       1) B_(7) .. B_(n)  <-  b_(8-(n+1)) .. b_(0)
       2) B_(n-1) .. B_(0) <-  b_(7) .. b_(8-n)
    */
    res = d;
    if ((cnt = s % 8) != 0) {
	/* B_(7) .. B_(n)  <-  b_(8-(n+1)) .. b_(0) */
	res = (d << cnt);

	/* B_(n-1) .. B_(0) <-  b_(7) .. b_(8-n) */
	mask = (1 << cnt) - 1;
	res |= (d >> (8 - cnt)) & mask;

	/* set the new carry flag, Note that it is the low order
	   bit of the result!!!                               */
	CONDITIONAL_SET_FLAG(res & 0x1, F_CF);
	/* OVERFLOW is set *IFF* s==1, then it is the
	   xor of CF and the most significant bit.  Blecck. */
	CONDITIONAL_SET_FLAG(s == 1 &&
			     XOR2((res & 0x1) + ((res >> 6) & 0x2)),
			     F_OF);
    } if (s != 0) {
	/* set the new carry flag, Note that it is the low order
	   bit of the result!!!                               */
	CONDITIONAL_SET_FLAG(res & 0x1, F_CF);
    }
    return (u8)res;
}

/****************************************************************************
REMARKS:
Implements the ROL instruction and side effects.
****************************************************************************/
u16 x86emuPrimOp_rol_word(X86EMU_sysEnv *env, u16 d, u8 s)
{
    register unsigned int res, cnt, mask;

    res = d;
    if ((cnt = s % 16) != 0) {
	res = (d << cnt);
	mask = (1 << cnt) - 1;
	res |= (d >> (16 - cnt)) & mask;
	CONDITIONAL_SET_FLAG(res & 0x1, F_CF);
	CONDITIONAL_SET_FLAG(s == 1 &&
			     XOR2((res & 0x1) + ((res >> 14) & 0x2)),
			     F_OF);
    } if (s != 0) {
	/* set the new carry flag, Note that it is the low order
	   bit of the result!!!                               */
	CONDITIONAL_SET_FLAG(res & 0x1, F_CF);
    }
    return (u16)res;
}

/****************************************************************************
REMARKS:
Implements the ROL instruction and side effects.
****************************************************************************/
u32 x86emuPrimOp_rol_long(X86EMU_sysEnv *env, u32 d, u8 s)
{
    register u32 res, cnt, mask;

    res = d;
    if ((cnt = s % 32) != 0) {
	res = (d << cnt);
	mask = (1 << cnt) - 1;
	res |= (d >> (32 - cnt)) & mask;
	CONDITIONAL_SET_FLAG(res & 0x1, F_CF);
	CONDITIONAL_SET_FLAG(s == 1 &&
			     XOR2((res & 0x1) + ((res >> 30) & 0x2)),
			     F_OF);
    } if (s != 0) {
	/* set the new carry flag, Note that it is the low order
	   bit of the result!!!                               */
	CONDITIONAL_SET_FLAG(res & 0x1, F_CF);
    }
    return res;
}

/****************************************************************************
REMARKS:
Implements the ROR instruction and side effects.
****************************************************************************/
u8 x86emuPrimOp_ror_byte(X86EMU_sysEnv *env, u8 d, u8 s)
{
    register unsigned int res, cnt, mask;

    /* rotate right */
    /* 
       s is the rotate distance.  It varies from 0 - 8.
       d is the byte object rotated.  

       have 

       B_7 ... B_0 

       The rotate is done mod 8.

       IF n > 0 
       1) B_(8-(n+1)) .. B_(0)  <-  b_(7) .. b_(n)
       2) B_(7) .. B_(8-n) <-  b_(n-1) .. b_(0)
    */
    res = d;
    if ((cnt = s % 8) != 0) {           /* not a typo, do nada if cnt==0 */
        /* B_(7) .. B_(8-n) <-  b_(n-1) .. b_(0) */
        res = (d << (8 - cnt));

        /* B_(8-(n+1)) .. B_(0)  <-  b_(7) .. b_(n) */
        mask = (1 << (8 - cnt)) - 1;
        res |= (d >> (cnt)) & mask;

        /* set the new carry flag, Note that it is the low order 
           bit of the result!!!                               */
	CONDITIONAL_SET_FLAG(res & 0x80, F_CF);
	/* OVERFLOW is set *IFF* s==1, then it is the
           xor of the two most significant bits.  Blecck. */
	CONDITIONAL_SET_FLAG(s == 1 && XOR2(res >> 6), F_OF);
    } else if (s != 0) {
	/* set the new carry flag, Note that it is the low order
	   bit of the result!!!                               */
	CONDITIONAL_SET_FLAG(res & 0x80, F_CF);
    }
    return (u8)res;
}

/****************************************************************************
REMARKS:
Implements the ROR instruction and side effects.
****************************************************************************/
u16 x86emuPrimOp_ror_word(X86EMU_sysEnv *env, u16 d, u8 s)
{
    register unsigned int res, cnt, mask;

    res = d;
    if ((cnt = s % 16) != 0) {
	res = (d << (16 - cnt));
	mask = (1 << (16 - cnt)) - 1;
	res |= (d >> (cnt)) & mask;
	CONDITIONAL_SET_FLAG(res & 0x8000, F_CF);
	CONDITIONAL_SET_FLAG(s == 1 && XOR2(res >> 14), F_OF);
    } else if (s != 0) {
	/* set the new carry flag, Note that it is the low order
	   bit of the result!!!                               */
	CONDITIONAL_SET_FLAG(res & 0x8000, F_CF);
    }
    return (u16)res;
}

/****************************************************************************
REMARKS:
Implements the ROR instruction and side effects.
****************************************************************************/
u32 x86emuPrimOp_ror_long(X86EMU_sysEnv *env, u32 d, u8 s)
{
    register u32 res, cnt, mask;

    res = d;
    if ((cnt = s % 32) != 0) {
	res = (d << (32 - cnt));
	mask = (1 << (32 - cnt)) - 1;
	res |= (d >> (cnt)) & mask;
	CONDITIONAL_SET_FLAG(res & 0x80000000, F_CF);
	CONDITIONAL_SET_FLAG(s == 1 && XOR2(res >> 30), F_OF);
    } else if (s != 0) {
	/* set the new carry flag, Note that it is the low order
	   bit of the result!!!                               */
	CONDITIONAL_SET_FLAG(res & 0x80000000, F_CF);
    }
    return res;
}

/****************************************************************************
REMARKS:
Implements the SHL instruction and side effects.
****************************************************************************/
u8 x86emuPrimOp_shl_byte(X86EMU_sysEnv *env, u8 d, u8 s)
{
    unsigned int cnt, res, cf;

    if (s < 8) {
	cnt = s % 8;

	/* last bit shifted out goes into carry flag */
	if (cnt > 0) {
	    res = d << cnt;
	    cf = d & (1 << (8 - cnt));
	    CONDITIONAL_SET_FLAG(cf, F_CF);
	    CONDITIONAL_SET_FLAG((res & 0xff) == 0, F_ZF);
	    CONDITIONAL_SET_FLAG(res & 0x80, F_SF);
	    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);
	} else {
	    res = (u8) d;
	}

	if (cnt == 1) {
	    /* Needs simplification. */
	    CONDITIONAL_SET_FLAG(
				 (((res & 0x80) == 0x80) ^
				  (ACCESS_FLAG(F_CF) != 0)),
				 /* was (env->x86.R_FLG&F_CF)==F_CF)), */
				 F_OF);
	} else {
	    CLEAR_FLAG(F_OF);
	}
    } else {
	res = 0;
	CONDITIONAL_SET_FLAG((d << (s-1)) & 0x80, F_CF);
	CLEAR_FLAG(F_OF);
	CLEAR_FLAG(F_SF);
	SET_FLAG(F_PF);
	SET_FLAG(F_ZF);
    }
    return (u8)res;
}

/****************************************************************************
REMARKS:
Implements the SHL instruction and side effects.
****************************************************************************/
u16 x86emuPrimOp_shl_word(X86EMU_sysEnv *env, u16 d, u8 s)
{
    unsigned int cnt, res, cf;

    if (s < 16) {
	cnt = s % 16;
	if (cnt > 0) {
	    res = d << cnt;
	    cf = d & (1 << (16 - cnt));
	    CONDITIONAL_SET_FLAG(cf, F_CF);
	    CONDITIONAL_SET_FLAG((res & 0xffff) == 0, F_ZF);
	    CONDITIONAL_SET_FLAG(res & 0x8000, F_SF);
	    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);
	} else {
	    res = (u16) d;
	}

	if (cnt == 1) {
	    CONDITIONAL_SET_FLAG(
				 (((res & 0x8000) == 0x8000) ^
				  (ACCESS_FLAG(F_CF) != 0)),
				 F_OF);
        } else {
	    CLEAR_FLAG(F_OF);
        }
    } else {
	res = 0;
	CONDITIONAL_SET_FLAG((d << (s-1)) & 0x8000, F_CF);
	CLEAR_FLAG(F_OF);
	CLEAR_FLAG(F_SF);
	SET_FLAG(F_PF);
	SET_FLAG(F_ZF);
    }
    return (u16)res;
}

/****************************************************************************
REMARKS:
Implements the SHL instruction and side effects.
****************************************************************************/
u32 x86emuPrimOp_shl_long(X86EMU_sysEnv *env, u32 d, u8 s)
{
    unsigned int cnt, res, cf;

    if (s < 32) {
	cnt = s % 32;
	if (cnt > 0) {
	    res = d << cnt;
	    cf = d & (1 << (32 - cnt));
	    CONDITIONAL_SET_FLAG(cf, F_CF);
	    CONDITIONAL_SET_FLAG((res & 0xffffffff) == 0, F_ZF);
	    CONDITIONAL_SET_FLAG(res & 0x80000000, F_SF);
	    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);
	} else {
	    res = d;
	}
	if (cnt == 1) {
	    CONDITIONAL_SET_FLAG((((res & 0x80000000) == 0x80000000) ^
				  (ACCESS_FLAG(F_CF) != 0)), F_OF);
	} else {
	    CLEAR_FLAG(F_OF);
	}
    } else {
	res = 0;
	CONDITIONAL_SET_FLAG((d << (s-1)) & 0x80000000, F_CF);
	CLEAR_FLAG(F_OF);
	CLEAR_FLAG(F_SF);
	SET_FLAG(F_PF);
	SET_FLAG(F_ZF);
    }
    return res;
}

/****************************************************************************
REMARKS:
Implements the SHR instruction and side effects.
****************************************************************************/
u8 x86emuPrimOp_shr_byte(X86EMU_sysEnv *env, u8 d, u8 s)
{
    unsigned int cnt, res, cf;

    if (s < 8) {
	cnt = s % 8;
	if (cnt > 0) {
	    cf = d & (1 << (cnt - 1));
	    res = d >> cnt;
	    CONDITIONAL_SET_FLAG(cf, F_CF);
	    CONDITIONAL_SET_FLAG((res & 0xff) == 0, F_ZF);
	    CONDITIONAL_SET_FLAG(res & 0x80, F_SF);
	    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);
	} else {
	    res = (u8) d;
	}

	if (cnt == 1) {
	    CONDITIONAL_SET_FLAG(XOR2(res >> 6), F_OF);
	} else {
	    CLEAR_FLAG(F_OF);
	}
    } else {
	res = 0;
	CONDITIONAL_SET_FLAG((d >> (s-1)) & 0x1, F_CF);
	CLEAR_FLAG(F_OF);
	CLEAR_FLAG(F_SF);
	SET_FLAG(F_PF);
	SET_FLAG(F_ZF);
    }
    return (u8)res;
}

/****************************************************************************
REMARKS:
Implements the SHR instruction and side effects.
****************************************************************************/
u16 x86emuPrimOp_shr_word(X86EMU_sysEnv *env, u16 d, u8 s)
{
    unsigned int cnt, res, cf;

    if (s < 16) {
	cnt = s % 16;
	if (cnt > 0) {
	    cf = d & (1 << (cnt - 1));
	    res = d >> cnt;
	    CONDITIONAL_SET_FLAG(cf, F_CF);
	    CONDITIONAL_SET_FLAG((res & 0xffff) == 0, F_ZF);
	    CONDITIONAL_SET_FLAG(res & 0x8000, F_SF);
	    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);
	} else {
	    res = d;
	}

	if (cnt == 1) {
	    CONDITIONAL_SET_FLAG(XOR2(res >> 14), F_OF);
        } else {
	    CLEAR_FLAG(F_OF);
        }
    } else {
	res = 0;
	CLEAR_FLAG(F_CF);
	CLEAR_FLAG(F_OF);
	SET_FLAG(F_ZF);
	CLEAR_FLAG(F_SF);
	CLEAR_FLAG(F_PF);
    }
    return (u16)res;
}

/****************************************************************************
REMARKS:
Implements the SHR instruction and side effects.
****************************************************************************/
u32 x86emuPrimOp_shr_long(X86EMU_sysEnv *env, u32 d, u8 s)
{
    unsigned int cnt, res, cf;

    if (s < 32) {
	cnt = s % 32;
	if (cnt > 0) {
	    cf = d & (1 << (cnt - 1));
	    res = d >> cnt;
	    CONDITIONAL_SET_FLAG(cf, F_CF);
	    CONDITIONAL_SET_FLAG((res & 0xffffffff) == 0, F_ZF);
	    CONDITIONAL_SET_FLAG(res & 0x80000000, F_SF);
	    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);
        } else {
            res = d;
        }
        if (cnt == 1) {
	    CONDITIONAL_SET_FLAG(XOR2(res >> 30), F_OF);
        } else {
	    CLEAR_FLAG(F_OF);
        }
    } else {
        res = 0;
	CLEAR_FLAG(F_CF);
	CLEAR_FLAG(F_OF);
	SET_FLAG(F_ZF);
	CLEAR_FLAG(F_SF);
	CLEAR_FLAG(F_PF);
    }
    return res;
}

/****************************************************************************
REMARKS:
Implements the SAR instruction and side effects.
****************************************************************************/
u8 x86emuPrimOp_sar_byte(X86EMU_sysEnv *env, u8 d, u8 s)
{
    unsigned int cnt, res, cf, mask, sf;

    res = d;
    sf = d & 0x80;
    cnt = s % 8;
    if (cnt > 0 && cnt < 8) {
	mask = (1 << (8 - cnt)) - 1;
	cf = d & (1 << (cnt - 1));
	res = (d >> cnt) & mask;
	CONDITIONAL_SET_FLAG(cf, F_CF);
	if (sf) {
	    res |= ~mask;
	}
	CONDITIONAL_SET_FLAG((res & 0xff) == 0, F_ZF);
	CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);
	CONDITIONAL_SET_FLAG(res & 0x80, F_SF);
    } else if (cnt >= 8) {
        if (sf) {
            res = 0xff;
	    SET_FLAG(F_CF);
	    CLEAR_FLAG(F_ZF);
	    SET_FLAG(F_SF);
	    SET_FLAG(F_PF);
	} else {
	    res = 0;
	    CLEAR_FLAG(F_CF);
	    SET_FLAG(F_ZF);
	    CLEAR_FLAG(F_SF);
	    CLEAR_FLAG(F_PF);
	}
    }
    return (u8)res;
}

/****************************************************************************
REMARKS:
Implements the SAR instruction and side effects.
****************************************************************************/
u16 x86emuPrimOp_sar_word(X86EMU_sysEnv *env, u16 d, u8 s)
{
    unsigned int cnt, res, cf, mask, sf;

    sf = d & 0x8000;
    cnt = s % 16;
    res = d;
    if (cnt > 0 && cnt < 16) {
        mask = (1 << (16 - cnt)) - 1;
        cf = d & (1 << (cnt - 1));
        res = (d >> cnt) & mask;
	CONDITIONAL_SET_FLAG(cf, F_CF);
        if (sf) {
            res |= ~mask;
        }
	CONDITIONAL_SET_FLAG((res & 0xffff) == 0, F_ZF);
	CONDITIONAL_SET_FLAG(res & 0x8000, F_SF);
	CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);
    } else if (cnt >= 16) {
        if (sf) {
            res = 0xffff;
	    SET_FLAG(F_CF);
	    CLEAR_FLAG(F_ZF);
	    SET_FLAG(F_SF);
	    SET_FLAG(F_PF);
        } else {
            res = 0;
	    CLEAR_FLAG(F_CF);
	    SET_FLAG(F_ZF);
	    CLEAR_FLAG(F_SF);
	    CLEAR_FLAG(F_PF);
        }
    }
    return (u16)res;
}

/****************************************************************************
REMARKS:
Implements the SAR instruction and side effects.
****************************************************************************/
u32 x86emuPrimOp_sar_long(X86EMU_sysEnv *env, u32 d, u8 s)
{
    u32 cnt, res, cf, mask, sf;

    sf = d & 0x80000000;
    cnt = s % 32;
    res = d;
    if (cnt > 0 && cnt < 32) {
        mask = (1 << (32 - cnt)) - 1;
	cf = d & (1 << (cnt - 1));
        res = (d >> cnt) & mask;
	CONDITIONAL_SET_FLAG(cf, F_CF);
        if (sf) {
            res |= ~mask;
        }
	CONDITIONAL_SET_FLAG((res & 0xffffffff) == 0, F_ZF);
	CONDITIONAL_SET_FLAG(res & 0x80000000, F_SF);
	CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);
    } else if (cnt >= 32) {
        if (sf) {
            res = 0xffffffff;
	    SET_FLAG(F_CF);
	    CLEAR_FLAG(F_ZF);
	    SET_FLAG(F_SF);
	    SET_FLAG(F_PF);
	} else {
	    res = 0;
	    CLEAR_FLAG(F_CF);
	    SET_FLAG(F_ZF);
	    CLEAR_FLAG(F_SF);
	    CLEAR_FLAG(F_PF);
	}
    }
    return res;
}

/****************************************************************************
REMARKS:
Implements the SHLD instruction and side effects.
****************************************************************************/
u16 x86emuPrimOp_shld_word(X86EMU_sysEnv *env, u16 d, u16 fill, u8 s)
{
    unsigned int cnt, res, cf;

    if (s < 16) {
	cnt = s % 16;
	if (cnt > 0) {
	    res = (d << cnt) | (fill >> (16-cnt));
	    cf = d & (1 << (16 - cnt));
	    CONDITIONAL_SET_FLAG(cf, F_CF);
	    CONDITIONAL_SET_FLAG((res & 0xffff) == 0, F_ZF);
	    CONDITIONAL_SET_FLAG(res & 0x8000, F_SF);
	    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);
	} else {
	    res = d;
	}
	if (cnt == 1) {
	    CONDITIONAL_SET_FLAG((((res & 0x8000) == 0x8000) ^
				  (ACCESS_FLAG(F_CF) != 0)), F_OF);
	} else {
	    CLEAR_FLAG(F_OF);
	}
    } else {
	res = 0;
	CONDITIONAL_SET_FLAG((d << (s-1)) & 0x8000, F_CF);
	CLEAR_FLAG(F_OF);
	CLEAR_FLAG(F_SF);
	SET_FLAG(F_PF);
	SET_FLAG(F_ZF);
    }
    return (u16)res;
}

/****************************************************************************
REMARKS:
Implements the SHLD instruction and side effects.
****************************************************************************/
u32 x86emuPrimOp_shld_long(X86EMU_sysEnv *env, u32 d, u32 fill, u8 s)
{
    unsigned int cnt, res, cf;

    if (s < 32) {
	cnt = s % 32;
	if (cnt > 0) {
	    res = (d << cnt) | (fill >> (32-cnt));
	    cf = d & (1 << (32 - cnt));
	    CONDITIONAL_SET_FLAG(cf, F_CF);
	    CONDITIONAL_SET_FLAG((res & 0xffffffff) == 0, F_ZF);
	    CONDITIONAL_SET_FLAG(res & 0x80000000, F_SF);
	    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);
	} else {
	    res = d;
	}
	if (cnt == 1) {
	    CONDITIONAL_SET_FLAG((((res & 0x80000000) == 0x80000000) ^
				  (ACCESS_FLAG(F_CF) != 0)), F_OF);
	} else {
	    CLEAR_FLAG(F_OF);
	}
    } else {
	res = 0;
	CONDITIONAL_SET_FLAG((d << (s-1)) & 0x80000000, F_CF);
	CLEAR_FLAG(F_OF);
	CLEAR_FLAG(F_SF);
	SET_FLAG(F_PF);
	SET_FLAG(F_ZF);
    }
    return res;
}

/****************************************************************************
REMARKS:
Implements the SHRD instruction and side effects.
****************************************************************************/
u16 x86emuPrimOp_shrd_word(X86EMU_sysEnv *env, u16 d, u16 fill, u8 s)
{
    unsigned int cnt, res, cf;

    if (s < 16) {
	cnt = s % 16;
	if (cnt > 0) {
	    cf = d & (1 << (cnt - 1));
	    res = (d >> cnt) | (fill << (16 - cnt));
	    CONDITIONAL_SET_FLAG(cf, F_CF);
	    CONDITIONAL_SET_FLAG((res & 0xffff) == 0, F_ZF);
	    CONDITIONAL_SET_FLAG(res & 0x8000, F_SF);
	    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);
	} else {
	    res = d;
	}

	if (cnt == 1) {
	    CONDITIONAL_SET_FLAG(XOR2(res >> 14), F_OF);
        } else {
	    CLEAR_FLAG(F_OF);
        }
    } else {
	res = 0;
	CLEAR_FLAG(F_CF);
	CLEAR_FLAG(F_OF);
	SET_FLAG(F_ZF);
	CLEAR_FLAG(F_SF);
	CLEAR_FLAG(F_PF);
    }
    return (u16)res;
}

/****************************************************************************
REMARKS:
Implements the SHRD instruction and side effects.
****************************************************************************/
u32 x86emuPrimOp_shrd_long(X86EMU_sysEnv *env, u32 d, u32 fill, u8 s)
{
    unsigned int cnt, res, cf;

    if (s < 32) {
	cnt = s % 32;
	if (cnt > 0) {
	    cf = d & (1 << (cnt - 1));
	    res = (d >> cnt) | (fill << (32 - cnt));
	    CONDITIONAL_SET_FLAG(cf, F_CF);
	    CONDITIONAL_SET_FLAG((res & 0xffffffff) == 0, F_ZF);
	    CONDITIONAL_SET_FLAG(res & 0x80000000, F_SF);
	    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);
	} else {
	    res = d;
	}
	if (cnt == 1) {
	    CONDITIONAL_SET_FLAG(XOR2(res >> 30), F_OF);
        } else {
	    CLEAR_FLAG(F_OF);
        }
    } else {
	res = 0;
	CLEAR_FLAG(F_CF);
	CLEAR_FLAG(F_OF);
	SET_FLAG(F_ZF);
	CLEAR_FLAG(F_SF);
	CLEAR_FLAG(F_PF);
    }
    return res;
}

/****************************************************************************
REMARKS:
Implements the SBB instruction and side effects.
****************************************************************************/
u8 x86emuPrimOp_sbb_byte(X86EMU_sysEnv *env, u8 d, u8 s)
{
    register u32 res;   /* all operands in native machine order */
    register u32 bc;

    if (ACCESS_FLAG(F_CF))
	res = d - s - 1;
    else
	res = d - s;
    CONDITIONAL_SET_FLAG(res & 0x80, F_SF);
    CONDITIONAL_SET_FLAG((res & 0xff) == 0, F_ZF);
    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);

    /* calculate the borrow chain.  See note at top */
    bc = (res & (~d | s)) | (~d & s);
    CONDITIONAL_SET_FLAG(bc & 0x80, F_CF);
    CONDITIONAL_SET_FLAG(XOR2(bc >> 6), F_OF);
    CONDITIONAL_SET_FLAG(bc & 0x8, F_AF);
    return (u8)res;
}

/****************************************************************************
REMARKS:
Implements the SBB instruction and side effects.
****************************************************************************/
u16 x86emuPrimOp_sbb_word(X86EMU_sysEnv *env, u16 d, u16 s)
{
    register u32 res;   /* all operands in native machine order */
    register u32 bc;

    if (ACCESS_FLAG(F_CF))
        res = d - s - 1;
    else
        res = d - s;
    CONDITIONAL_SET_FLAG(res & 0x8000, F_SF);
    CONDITIONAL_SET_FLAG((res & 0xffff) == 0, F_ZF);
    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);

    /* calculate the borrow chain.  See note at top */
    bc = (res & (~d | s)) | (~d & s);
    CONDITIONAL_SET_FLAG(bc & 0x8000, F_CF);
    CONDITIONAL_SET_FLAG(XOR2(bc >> 14), F_OF);
    CONDITIONAL_SET_FLAG(bc & 0x8, F_AF);
    return (u16)res;
}

/****************************************************************************
REMARKS:
Implements the SBB instruction and side effects.
****************************************************************************/
u32 x86emuPrimOp_sbb_long(X86EMU_sysEnv *env, u32 d, u32 s)
{
    register u32 res;   /* all operands in native machine order */
    register u32 bc;

    if (ACCESS_FLAG(F_CF))
        res = d - s - 1;
    else
        res = d - s;
    CONDITIONAL_SET_FLAG(res & 0x80000000, F_SF);
    CONDITIONAL_SET_FLAG((res & 0xffffffff) == 0, F_ZF);
    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);

    /* calculate the borrow chain.  See note at top */
    bc = (res & (~d | s)) | (~d & s);
    CONDITIONAL_SET_FLAG(bc & 0x80000000, F_CF);
    CONDITIONAL_SET_FLAG(XOR2(bc >> 30), F_OF);
    CONDITIONAL_SET_FLAG(bc & 0x8, F_AF);
    return res;
}

/****************************************************************************
REMARKS:
Implements the SUB instruction and side effects.
****************************************************************************/
u8 x86emuPrimOp_sub_byte(X86EMU_sysEnv *env, u8 d, u8 s)
{
    register u32 res;   /* all operands in native machine order */
    register u32 bc;

    res = d - s;
    CONDITIONAL_SET_FLAG(res & 0x80, F_SF);
    CONDITIONAL_SET_FLAG((res & 0xff) == 0, F_ZF);
    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);

    /* calculate the borrow chain.  See note at top */
    bc = (res & (~d | s)) | (~d & s);
    CONDITIONAL_SET_FLAG(bc & 0x80, F_CF);
    CONDITIONAL_SET_FLAG(XOR2(bc >> 6), F_OF);
    CONDITIONAL_SET_FLAG(bc & 0x8, F_AF);
    return (u8)res;
}

/****************************************************************************
REMARKS:
Implements the SUB instruction and side effects.
****************************************************************************/
u16 x86emuPrimOp_sub_word(X86EMU_sysEnv *env, u16 d, u16 s)
{
    register u32 res;   /* all operands in native machine order */
    register u32 bc;

    res = d - s;
    CONDITIONAL_SET_FLAG(res & 0x8000, F_SF);
    CONDITIONAL_SET_FLAG((res & 0xffff) == 0, F_ZF);
    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);

    /* calculate the borrow chain.  See note at top */
    bc = (res & (~d | s)) | (~d & s);
    CONDITIONAL_SET_FLAG(bc & 0x8000, F_CF);
    CONDITIONAL_SET_FLAG(XOR2(bc >> 14), F_OF);
    CONDITIONAL_SET_FLAG(bc & 0x8, F_AF);
    return (u16)res;
}

/****************************************************************************
REMARKS:
Implements the SUB instruction and side effects.
****************************************************************************/
u32 x86emuPrimOp_sub_long(X86EMU_sysEnv *env, u32 d, u32 s)
{
    register u32 res;   /* all operands in native machine order */
    register u32 bc;

    res = d - s;
    CONDITIONAL_SET_FLAG(res & 0x80000000, F_SF);
    CONDITIONAL_SET_FLAG((res & 0xffffffff) == 0, F_ZF);
    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);

    /* calculate the borrow chain.  See note at top */
    bc = (res & (~d | s)) | (~d & s);
    CONDITIONAL_SET_FLAG(bc & 0x80000000, F_CF);
    CONDITIONAL_SET_FLAG(XOR2(bc >> 30), F_OF);
    CONDITIONAL_SET_FLAG(bc & 0x8, F_AF);
    return res;
}

/****************************************************************************
REMARKS:
Implements the TEST instruction and side effects.
****************************************************************************/
void x86emuPrimOp_test_byte(X86EMU_sysEnv *env, u8 d, u8 s)
{
    register u32 res;   /* all operands in native machine order */

    res = d & s;

    CLEAR_FLAG(F_OF);
    CONDITIONAL_SET_FLAG(res & 0x80, F_SF);
    CONDITIONAL_SET_FLAG(res == 0, F_ZF);
    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);
    /* AF == dont care */
    CLEAR_FLAG(F_CF);
}

/****************************************************************************
REMARKS:
Implements the TEST instruction and side effects.
****************************************************************************/
void x86emuPrimOp_test_word(X86EMU_sysEnv *env, u16 d, u16 s)
{
    register u32 res;   /* all operands in native machine order */

    res = d & s;

    CLEAR_FLAG(F_OF);
    CONDITIONAL_SET_FLAG(res & 0x8000, F_SF);
    CONDITIONAL_SET_FLAG(res == 0, F_ZF);
    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);
    /* AF == dont care */
    CLEAR_FLAG(F_CF);
}

/****************************************************************************
REMARKS:
Implements the TEST instruction and side effects.
****************************************************************************/
void x86emuPrimOp_test_long(X86EMU_sysEnv *env, u32 d, u32 s)
{
    register u32 res;   /* all operands in native machine order */

    res = d & s;

    CLEAR_FLAG(F_OF);
    CONDITIONAL_SET_FLAG(res & 0x80000000, F_SF);
    CONDITIONAL_SET_FLAG(res == 0, F_ZF);
    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);
    /* AF == dont care */
    CLEAR_FLAG(F_CF);
}

/****************************************************************************
REMARKS:
Implements the XOR instruction and side effects.
****************************************************************************/
u8 x86emuPrimOp_xor_byte(X86EMU_sysEnv *env, u8 d, u8 s)
{
    register u8 res;    /* all operands in native machine order */

    res = d ^ s;
    CLEAR_FLAG(F_OF);
    CONDITIONAL_SET_FLAG(res & 0x80, F_SF);
    CONDITIONAL_SET_FLAG(res == 0, F_ZF);
    CONDITIONAL_SET_FLAG(PARITY(res), F_PF);
    CLEAR_FLAG(F_CF);
    CLEAR_FLAG(F_AF);
    return res;
}

/****************************************************************************
REMARKS:
Implements the XOR instruction and side effects.
****************************************************************************/
u16 x86emuPrimOp_xor_word(X86EMU_sysEnv *env, u16 d, u16 s)
{
    register u16 res;   /* all operands in native machine order */

    res = d ^ s;
    CLEAR_FLAG(F_OF);
    CONDITIONAL_SET_FLAG(res & 0x8000, F_SF);
    CONDITIONAL_SET_FLAG(res == 0, F_ZF);
    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);
    CLEAR_FLAG(F_CF);
    CLEAR_FLAG(F_AF);
    return res;
}

/****************************************************************************
REMARKS:
Implements the XOR instruction and side effects.
****************************************************************************/
u32 x86emuPrimOp_xor_long(X86EMU_sysEnv *env, u32 d, u32 s)
{
    register u32 res;   /* all operands in native machine order */

    res = d ^ s;
    CLEAR_FLAG(F_OF);
    CONDITIONAL_SET_FLAG(res & 0x80000000, F_SF);
    CONDITIONAL_SET_FLAG(res == 0, F_ZF);
    CONDITIONAL_SET_FLAG(PARITY(res & 0xff), F_PF);
    CLEAR_FLAG(F_CF);
    CLEAR_FLAG(F_AF);
    return res;
}

/****************************************************************************
REMARKS:
Implements the IMUL instruction and side effects.
****************************************************************************/
void x86emuPrimOp_imul_byte(X86EMU_sysEnv *env, u8 s)
{
    s16 res = (s16)((s8)env->x86.R_AL * (s8)s);

    env->x86.R_AX = res;
    if (((env->x86.R_AL & 0x80) == 0 && env->x86.R_AH == 0x00) ||
	((env->x86.R_AL & 0x80) != 0 && env->x86.R_AH == 0xFF)) {
	CLEAR_FLAG(F_CF);
	CLEAR_FLAG(F_OF);
    } else {
	SET_FLAG(F_CF);
	SET_FLAG(F_OF);
    }
}

/****************************************************************************
REMARKS:
Implements the IMUL instruction and side effects.
****************************************************************************/
void x86emuPrimOp_imul_word(X86EMU_sysEnv *env, u16 s)
{
    s32 res = (s16)env->x86.R_AX * (s16)s;

    env->x86.R_AX = (u16)res;
    env->x86.R_DX = (u16)(res >> 16);
    if (((env->x86.R_AX & 0x8000) == 0 && env->x86.R_DX == 0x00) ||
	((env->x86.R_AX & 0x8000) != 0 && env->x86.R_DX == 0xFF)) {
	CLEAR_FLAG(F_CF);
	CLEAR_FLAG(F_OF);
    } else {
	SET_FLAG(F_CF);
	SET_FLAG(F_OF);
    }
}

/****************************************************************************
REMARKS:
Implements the IMUL instruction and side effects.
****************************************************************************/
void x86emuPrimOp_imul_long_direct(X86EMU_sysEnv *env, u32 *res_lo, u32* res_hi,u32 d, u32 s)
{
    s64 res = (s64)(s32)d * (s32)s;

    *res_lo = (u32)res;
    *res_hi = (u32)(res >> 32);
}

/****************************************************************************
REMARKS:
Implements the IMUL instruction and side effects.
****************************************************************************/
void x86emuPrimOp_imul_long(X86EMU_sysEnv *env, u32 s)
{
    imul_long_direct(&env->x86.R_EAX,&env->x86.R_EDX,env->x86.R_EAX,s);
    if (((env->x86.R_EAX & 0x80000000) == 0 && env->x86.R_EDX == 0x00) ||
	((env->x86.R_EAX & 0x80000000) != 0 && env->x86.R_EDX == 0xFF)) {
	CLEAR_FLAG(F_CF);
	CLEAR_FLAG(F_OF);
    } else {
	SET_FLAG(F_CF);
	SET_FLAG(F_OF);
    }
}

/****************************************************************************
REMARKS:
Implements the MUL instruction and side effects.
****************************************************************************/
void x86emuPrimOp_mul_byte(X86EMU_sysEnv *env, u8 s)
{
    u16 res = (u16)(env->x86.R_AL * s);

    env->x86.R_AX = res;
    if (env->x86.R_AH == 0) {
	CLEAR_FLAG(F_CF);
	CLEAR_FLAG(F_OF);
    } else {
	SET_FLAG(F_CF);
	SET_FLAG(F_OF);
    }
}

/****************************************************************************
REMARKS:
Implements the MUL instruction and side effects.
****************************************************************************/
void x86emuPrimOp_mul_word(X86EMU_sysEnv *env, u16 s)
{
    u32 res = env->x86.R_AX * s;

    env->x86.R_AX = (u16)res;
    env->x86.R_DX = (u16)(res >> 16);
    if (env->x86.R_DX == 0) {
	CLEAR_FLAG(F_CF);
	CLEAR_FLAG(F_OF);
    } else {
	SET_FLAG(F_CF);
	SET_FLAG(F_OF);
    }
}

/****************************************************************************
REMARKS:
Implements the MUL instruction and side effects.
****************************************************************************/
void x86emuPrimOp_mul_long(X86EMU_sysEnv *env, u32 s)
{
    u64 res = (u64)env->x86.R_EAX * s;

    env->x86.R_EAX = (u32)res;
    env->x86.R_EDX = (u32)(res >> 32);

    if (env->x86.R_EDX == 0) {
	CLEAR_FLAG(F_CF);
	CLEAR_FLAG(F_OF);
    } else {
	SET_FLAG(F_CF);
	SET_FLAG(F_OF);
    }
}

/****************************************************************************
REMARKS:
Implements the IDIV instruction and side effects.
****************************************************************************/
void x86emuPrimOp_idiv_byte(X86EMU_sysEnv *env, u8 s)
{
    s32 dvd, div, mod;

    dvd = (s16)env->x86.R_AX;
    if (s == 0) {
	x86emu_intr_raise(env, 0);
        return;
    }
    div = dvd / (s8)s;
    mod = dvd % (s8)s;
    if (abs(div) > 0x7f) {
	x86emu_intr_raise(env, 0);
	return;
    }
    env->x86.R_AL = (s8) div;
    env->x86.R_AH = (s8) mod;
}

/****************************************************************************
REMARKS:
Implements the IDIV instruction and side effects.
****************************************************************************/
void x86emuPrimOp_idiv_word(X86EMU_sysEnv *env, u16 s)
{
    s32 dvd, div, mod;

    dvd = (((s32)env->x86.R_DX) << 16) | env->x86.R_AX;
    if (s == 0) {
	x86emu_intr_raise(env, 0);
	return;
    }
    div = dvd / (s16)s;
    mod = dvd % (s16)s;
    if (abs(div) > 0x7fff) {
	x86emu_intr_raise(env, 0);
	return;
    }
    CLEAR_FLAG(F_CF);
    CLEAR_FLAG(F_SF);
    CONDITIONAL_SET_FLAG(div == 0, F_ZF);
    CONDITIONAL_SET_FLAG(PARITY(mod & 0xff), F_PF);

    env->x86.R_AX = (u16)div;
    env->x86.R_DX = (u16)mod;
}

/****************************************************************************
REMARKS:
Implements the IDIV instruction and side effects.
****************************************************************************/
void x86emuPrimOp_idiv_long(X86EMU_sysEnv *env, u32 s)
{
    s64 dvd, div, mod;

    dvd = (((s64)env->x86.R_EDX) << 32) | env->x86.R_EAX;
    if (s == 0) {
	x86emu_intr_raise(env, 0);
	return;
    }
    div = dvd / (s32)s;
    mod = dvd % (s32)s;
    if (abs(div) > 0x7fffffff) {
	x86emu_intr_raise(env, 0);
	return;
    }

    CLEAR_FLAG(F_CF);
    CLEAR_FLAG(F_AF);
    CLEAR_FLAG(F_SF);
    SET_FLAG(F_ZF);
    CONDITIONAL_SET_FLAG(PARITY(mod & 0xff), F_PF);

    env->x86.R_EAX = (u32)div;
    env->x86.R_EDX = (u32)mod;
}

/****************************************************************************
REMARKS:
Implements the DIV instruction and side effects.
****************************************************************************/
void x86emuPrimOp_div_byte(X86EMU_sysEnv *env, u8 s)
{
    u32 dvd, div, mod;

    dvd = env->x86.R_AX;
    if (s == 0) {
	x86emu_intr_raise(env, 0);
        return;
    }
    div = dvd / (u8)s;
    mod = dvd % (u8)s;
    if (abs(div) > 0xff) {
	x86emu_intr_raise(env, 0);
        return;
    }
    env->x86.R_AL = (u8)div;
    env->x86.R_AH = (u8)mod;
}

/****************************************************************************
REMARKS:
Implements the DIV instruction and side effects.
****************************************************************************/
void x86emuPrimOp_div_word(X86EMU_sysEnv *env, u16 s)
{
    u32 dvd, div, mod;

    dvd = (((u32)env->x86.R_DX) << 16) | env->x86.R_AX;
    if (s == 0) {
	x86emu_intr_raise(env, 0);
        return;
    }
    div = dvd / (u16)s;
    mod = dvd % (u16)s;
    if (abs(div) > 0xffff) {
	x86emu_intr_raise(env, 0);
	return;
    }
    CLEAR_FLAG(F_CF);
    CLEAR_FLAG(F_SF);
    CONDITIONAL_SET_FLAG(div == 0, F_ZF);
    CONDITIONAL_SET_FLAG(PARITY(mod & 0xff), F_PF);

    env->x86.R_AX = (u16)div;
    env->x86.R_DX = (u16)mod;
}

/****************************************************************************
REMARKS:
Implements the DIV instruction and side effects.
****************************************************************************/
void x86emuPrimOp_div_long(X86EMU_sysEnv *env, u32 s)
{
    u64 dvd, div, mod;

    dvd = (((u64)env->x86.R_EDX) << 32) | env->x86.R_EAX;
    if (s == 0) {
	x86emu_intr_raise(env, 0);
	return;
    }
    div = dvd / (u32)s;
    mod = dvd % (u32)s;
    if (abs(div) > 0xffffffff) {
	x86emu_intr_raise(env, 0);
	return;
    }
    CLEAR_FLAG(F_CF);
    CLEAR_FLAG(F_AF);
    CLEAR_FLAG(F_SF);
    SET_FLAG(F_ZF);
    CONDITIONAL_SET_FLAG(PARITY(mod & 0xff), F_PF);

    env->x86.R_EAX = (u32)div;
    env->x86.R_EDX = (u32)mod;
}

/****************************************************************************
REMARKS:
Implements the IN string instruction and side effects.
****************************************************************************/
void x86emuPrimOp_ins(X86EMU_sysEnv *env, int size)
{
    int inc = size;

    if (ACCESS_FLAG(F_DF)) {
	inc = -size;
    }
    if (env->x86.mode & (SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE)) {
        /* dont care whether REPE or REPNE */
        /* in until CX is ZERO. */
	u32 count = ((env->x86.mode & SYSMODE_PREFIX_DATA) ?
		     env->x86.R_ECX : env->x86.R_CX);
        switch (size) {
	case 1:
            while (count--) {
		store_data_byte_abs(env->x86.R_ES, env->x86.R_DI,
				    sys_inb(env->x86.R_DX));
		env->x86.R_DI += inc;
            }
            break;

	case 2:
            while (count--) {
		store_data_word_abs(env->x86.R_ES, env->x86.R_DI,
				    sys_inw(env->x86.R_DX));
		env->x86.R_DI += inc;
            }
            break;
	case 4:
            while (count--) {
		store_data_long_abs(env->x86.R_ES, env->x86.R_DI,
				    sys_inl(env->x86.R_DX));
		env->x86.R_DI += inc;
                break;
            }
        }
	env->x86.R_CX = 0;
	if (env->x86.mode & SYSMODE_PREFIX_DATA) {
	    env->x86.R_ECX = 0;
        }
	env->x86.mode &= ~(SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE);
    } else {
        switch (size) {
	case 1:
	    store_data_byte_abs(env->x86.R_ES, env->x86.R_DI,
				sys_inb(env->x86.R_DX));
            break;
	case 2:
	    store_data_word_abs(env->x86.R_ES, env->x86.R_DI,
				sys_inw(env->x86.R_DX));
            break;
	case 4:
	    store_data_long_abs(env->x86.R_ES, env->x86.R_DI,
				sys_inl(env->x86.R_DX));
            break;
        }
	env->x86.R_DI += inc;
    }
}

/****************************************************************************
REMARKS:
Implements the OUT string instruction and side effects.
****************************************************************************/
void x86emuPrimOp_outs(X86EMU_sysEnv *env, int size)
{
    int inc = size;

    if (ACCESS_FLAG(F_DF)) {
        inc = -size;
    }
    if (env->x86.mode & (SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE)) {
        /* dont care whether REPE or REPNE */
        /* out until CX is ZERO. */
	u32 count = ((env->x86.mode & SYSMODE_PREFIX_DATA) ?
		     env->x86.R_ECX : env->x86.R_CX);
        switch (size) {
	case 1:
            while (count--) {
		sys_outb(env->x86.R_DX,
			 fetch_data_byte_abs(env->x86.R_ES, env->x86.R_SI));
		env->x86.R_SI += inc;
            }
            break;

	case 2:
            while (count--) {
		sys_outw(env->x86.R_DX,
			 fetch_data_word_abs(env->x86.R_ES, env->x86.R_SI));
		env->x86.R_SI += inc;
            }
            break;
	case 4:
            while (count--) {
		sys_outl(env->x86.R_DX,
			 fetch_data_long_abs(env->x86.R_ES, env->x86.R_SI));
		env->x86.R_SI += inc;
                break;
            }
        }
	env->x86.R_CX = 0;
	if (env->x86.mode & SYSMODE_PREFIX_DATA) {
	    env->x86.R_ECX = 0;
        }
	env->x86.mode &= ~(SYSMODE_PREFIX_REPE | SYSMODE_PREFIX_REPNE);
    } else {
        switch (size) {
	case 1:
	    sys_outb(env->x86.R_DX,
		     fetch_data_byte_abs(env->x86.R_ES, env->x86.R_SI));
            break;
	case 2:
	    sys_outw(env->x86.R_DX,
		     fetch_data_word_abs(env->x86.R_ES, env->x86.R_SI));
            break;
	case 4:
	    sys_outl(env->x86.R_DX,
		     fetch_data_long_abs(env->x86.R_ES, env->x86.R_SI));
            break;
        }
	env->x86.R_SI += inc;
    }
}

/****************************************************************************
PARAMETERS:
addr	- Address to fetch word from

REMARKS:
Fetches a word from emulator memory using an absolute address.
****************************************************************************/
u16 x86emuPrimOp_mem_access_word(X86EMU_sysEnv *env, int addr)
{
    check_mem_access(addr);
    return sys_rdw(addr);
}

/****************************************************************************
REMARKS:
Pushes a word onto the stack.

NOTE: Do not inline this, as (*sys_wrX) is already inline!
****************************************************************************/
void x86emuPrimOp_push_word(X86EMU_sysEnv *env, u16 w)
{
    check_sp_access();
    env->x86.R_SP -= 2;
    sys_wrw(((u32)env->x86.R_SS << 4)  + env->x86.R_SP, w);
}

/****************************************************************************
REMARKS:
Pushes a long onto the stack.

NOTE: Do not inline this, as (*sys_wrX) is already inline!
****************************************************************************/
void x86emuPrimOp_push_long(X86EMU_sysEnv *env, u32 w)
{
    check_sp_access();
    env->x86.R_SP -= 4;
    sys_wrl(((u32)env->x86.R_SS << 4)  + env->x86.R_SP, w);
}

/****************************************************************************
REMARKS:
Pops a word from the stack.

NOTE: Do not inline this, as (*sys_rdX) is already inline!
****************************************************************************/
u16 x86emuPrimOp_pop_word(X86EMU_sysEnv *env)
{
    register u16 res;

    check_sp_access();
    res = sys_rdw(((u32)env->x86.R_SS << 4)  + env->x86.R_SP);
    env->x86.R_SP += 2;
    return res;
}

/****************************************************************************
REMARKS:
Pops a long from the stack.

NOTE: Do not inline this, as (*sys_rdX) is already inline!
****************************************************************************/
u32 x86emuPrimOp_pop_long(X86EMU_sysEnv *env)
{
    register u32 res;

    check_sp_access();
    res = sys_rdl(((u32)env->x86.R_SS << 4)  + env->x86.R_SP);
    env->x86.R_SP += 4;
    return res;
}
