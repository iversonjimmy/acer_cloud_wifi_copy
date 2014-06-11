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
* Description:  Header file for primitive operation functions.
*
****************************************************************************/

#ifndef __X86EMU_PRIM_OPS_H
#define __X86EMU_PRIM_OPS_H

#ifdef  __cplusplus
extern "C" {            			/* Use "C" linkage when in C++ mode */
#endif

u16	x86emuPrimOp_aaa_word(X86EMU_sysEnv *env, u16 d);
u16	x86emuPrimOp_aas_word(X86EMU_sysEnv *env, u16 d);
u16	x86emuPrimOp_aad_word(X86EMU_sysEnv *env, u16 d);
u16	x86emuPrimOp_aam_word(X86EMU_sysEnv *env, u8 d);
u8	x86emuPrimOp_adc_byte(X86EMU_sysEnv *env, u8 d, u8 s);
u16	x86emuPrimOp_adc_word(X86EMU_sysEnv *env, u16 d, u16 s);
u32	x86emuPrimOp_adc_long(X86EMU_sysEnv *env, u32 d, u32 s);
u8	x86emuPrimOp_add_byte(X86EMU_sysEnv *env, u8 d, u8 s);
u16	x86emuPrimOp_add_word(X86EMU_sysEnv *env, u16 d, u16 s);
u32	x86emuPrimOp_add_long(X86EMU_sysEnv *env, u32 d, u32 s);
u8	x86emuPrimOp_and_byte(X86EMU_sysEnv *env, u8 d, u8 s);
u16	x86emuPrimOp_and_word(X86EMU_sysEnv *env, u16 d, u16 s);
u32	x86emuPrimOp_and_long(X86EMU_sysEnv *env, u32 d, u32 s);
u8	x86emuPrimOp_cmp_byte(X86EMU_sysEnv *env, u8 d, u8 s);
u16	x86emuPrimOp_cmp_word(X86EMU_sysEnv *env, u16 d, u16 s);
u32	x86emuPrimOp_cmp_long(X86EMU_sysEnv *env, u32 d, u32 s);
u8	x86emuPrimOp_daa_byte(X86EMU_sysEnv *env, u8 d);
u8	x86emuPrimOp_das_byte(X86EMU_sysEnv *env, u8 d);
u8	x86emuPrimOp_dec_byte(X86EMU_sysEnv *env, u8 d);
u16	x86emuPrimOp_dec_word(X86EMU_sysEnv *env, u16 d);
u32	x86emuPrimOp_dec_long(X86EMU_sysEnv *env, u32 d);
u8	x86emuPrimOp_inc_byte(X86EMU_sysEnv *env, u8 d);
u16	x86emuPrimOp_inc_word(X86EMU_sysEnv *env, u16 d);
u32	x86emuPrimOp_inc_long(X86EMU_sysEnv *env, u32 d);
u8	x86emuPrimOp_or_byte(X86EMU_sysEnv *env, u8 d, u8 s);
u16	x86emuPrimOp_or_word(X86EMU_sysEnv *env, u16 d, u16 s);
u32	x86emuPrimOp_or_long(X86EMU_sysEnv *env, u32 d, u32 s);
u8	x86emuPrimOp_neg_byte(X86EMU_sysEnv *env, u8 s);
u16	x86emuPrimOp_neg_word(X86EMU_sysEnv *env, u16 s);
u32	x86emuPrimOp_neg_long(X86EMU_sysEnv *env, u32 s);
u8	x86emuPrimOp_not_byte(X86EMU_sysEnv *env, u8 s);
u16	x86emuPrimOp_not_word(X86EMU_sysEnv *env, u16 s);
u32	x86emuPrimOp_not_long(X86EMU_sysEnv *env, u32 s);
u8	x86emuPrimOp_rcl_byte(X86EMU_sysEnv *env, u8 d, u8 s);
u16	x86emuPrimOp_rcl_word(X86EMU_sysEnv *env, u16 d, u8 s);
u32	x86emuPrimOp_rcl_long(X86EMU_sysEnv *env, u32 d, u8 s);
u8	x86emuPrimOp_rcr_byte(X86EMU_sysEnv *env, u8 d, u8 s);
u16	x86emuPrimOp_rcr_word(X86EMU_sysEnv *env, u16 d, u8 s);
u32	x86emuPrimOp_rcr_long(X86EMU_sysEnv *env, u32 d, u8 s);
u8	x86emuPrimOp_rol_byte(X86EMU_sysEnv *env, u8 d, u8 s);
u16	x86emuPrimOp_rol_word(X86EMU_sysEnv *env, u16 d, u8 s);
u32	x86emuPrimOp_rol_long(X86EMU_sysEnv *env, u32 d, u8 s);
u8	x86emuPrimOp_ror_byte(X86EMU_sysEnv *env, u8 d, u8 s);
u16	x86emuPrimOp_ror_word(X86EMU_sysEnv *env, u16 d, u8 s);
u32	x86emuPrimOp_ror_long(X86EMU_sysEnv *env, u32 d, u8 s);
u8	x86emuPrimOp_shl_byte(X86EMU_sysEnv *env, u8 d, u8 s);
u16	x86emuPrimOp_shl_word(X86EMU_sysEnv *env, u16 d, u8 s);
u32	x86emuPrimOp_shl_long(X86EMU_sysEnv *env, u32 d, u8 s);
u8	x86emuPrimOp_shr_byte(X86EMU_sysEnv *env, u8 d, u8 s);
u16	x86emuPrimOp_shr_word(X86EMU_sysEnv *env, u16 d, u8 s);
u32	x86emuPrimOp_shr_long(X86EMU_sysEnv *env, u32 d, u8 s);
u8	x86emuPrimOp_sar_byte(X86EMU_sysEnv *env, u8 d, u8 s);
u16	x86emuPrimOp_sar_word(X86EMU_sysEnv *env, u16 d, u8 s);
u32	x86emuPrimOp_sar_long(X86EMU_sysEnv *env, u32 d, u8 s);
u16	x86emuPrimOp_shld_word(X86EMU_sysEnv *env, u16 d, u16 fill, u8 s);
u32	x86emuPrimOp_shld_long(X86EMU_sysEnv *env, u32 d, u32 fill, u8 s);
u16	x86emuPrimOp_shrd_word(X86EMU_sysEnv *env, u16 d, u16 fill, u8 s);
u32	x86emuPrimOp_shrd_long(X86EMU_sysEnv *env, u32 d, u32 fill, u8 s);
u8	x86emuPrimOp_sbb_byte(X86EMU_sysEnv *env, u8 d, u8 s);
u16	x86emuPrimOp_sbb_word(X86EMU_sysEnv *env, u16 d, u16 s);
u32	x86emuPrimOp_sbb_long(X86EMU_sysEnv *env, u32 d, u32 s);
u8	x86emuPrimOp_sub_byte(X86EMU_sysEnv *env, u8 d, u8 s);
u16	x86emuPrimOp_sub_word(X86EMU_sysEnv *env, u16 d, u16 s);
u32	x86emuPrimOp_sub_long(X86EMU_sysEnv *env, u32 d, u32 s);
void	x86emuPrimOp_test_byte(X86EMU_sysEnv *env, u8 d, u8 s);
void	x86emuPrimOp_test_word(X86EMU_sysEnv *env, u16 d, u16 s);
void	x86emuPrimOp_test_long(X86EMU_sysEnv *env, u32 d, u32 s);
u8	x86emuPrimOp_xor_byte(X86EMU_sysEnv *env, u8 d, u8 s);
u16	x86emuPrimOp_xor_word(X86EMU_sysEnv *env, u16 d, u16 s);
u32	x86emuPrimOp_xor_long(X86EMU_sysEnv *env, u32 d, u32 s);
void	x86emuPrimOp_imul_byte(X86EMU_sysEnv *env, u8 s);
void	x86emuPrimOp_imul_word(X86EMU_sysEnv *env, u16 s);
void	x86emuPrimOp_imul_long(X86EMU_sysEnv *env, u32 s);
void	x86emuPrimOp_imul_long_direct(X86EMU_sysEnv *env, u32 *res_lo, u32* res_hi,u32 d, u32 s);
void	x86emuPrimOp_mul_byte(X86EMU_sysEnv *env, u8 s);
void	x86emuPrimOp_mul_word(X86EMU_sysEnv *env, u16 s);
void	x86emuPrimOp_mul_long(X86EMU_sysEnv *env, u32 s);
void	x86emuPrimOp_idiv_byte(X86EMU_sysEnv *env, u8 s);
void	x86emuPrimOp_idiv_word(X86EMU_sysEnv *env, u16 s);
void	x86emuPrimOp_idiv_long(X86EMU_sysEnv *env, u32 s);
void	x86emuPrimOp_div_byte(X86EMU_sysEnv *env, u8 s);
void	x86emuPrimOp_div_word(X86EMU_sysEnv *env, u16 s);
void	x86emuPrimOp_div_long(X86EMU_sysEnv *env, u32 s);
void	x86emuPrimOp_ins(X86EMU_sysEnv *env, int size);
void	x86emuPrimOp_outs(X86EMU_sysEnv *env, int size);
u16	x86emuPrimOp_mem_access_word(X86EMU_sysEnv *env, int addr);
void	x86emuPrimOp_push_word(X86EMU_sysEnv *env, u16 w);
void	x86emuPrimOp_push_long(X86EMU_sysEnv *env, u32 w);
u16	x86emuPrimOp_pop_word(X86EMU_sysEnv *env);
u32	x86emuPrimOp_pop_long(X86EMU_sysEnv *env);

#ifdef  __cplusplus
}                       			/* End of "C" linkage for C++   	*/
#endif

#if 1
#define	aaa_word(a)	x86emuPrimOp_aaa_word(env, a)
#define	aas_word(a)	x86emuPrimOp_aas_word(env, a)
#define	aad_word(a)	x86emuPrimOp_aad_word(env, a)
#define	aam_word(a)	x86emuPrimOp_aam_word(env, a)
#define	adc_byte(a, b)	x86emuPrimOp_adc_byte(env, a, b)
#define	adc_word(a, b)	x86emuPrimOp_adc_word(env, a, b)
#define	adc_long(a, b)	x86emuPrimOp_adc_long(env, a, b)
#define	add_byte(a, b)	x86emuPrimOp_add_byte(env, a, b)
#define	add_word(a, b)	x86emuPrimOp_add_word(env, a, b)
#define	add_long(a, b)	x86emuPrimOp_add_long(env, a, b)
#define	and_byte(a, b)	x86emuPrimOp_and_byte(env, a, b)
#define	and_word(a, b)	x86emuPrimOp_and_word(env, a, b)
#define	and_long(a, b)	x86emuPrimOp_and_long(env, a, b)
#define	cmp_byte(a, b)	x86emuPrimOp_cmp_byte(env, a, b)
#define	cmp_word(a, b)	x86emuPrimOp_cmp_word(env, a, b)
#define	cmp_long(a, b)	x86emuPrimOp_cmp_long(env, a, b)
#define	daa_byte(a)	x86emuPrimOp_daa_byte(env, a)
#define	das_byte(a)	x86emuPrimOp_das_byte(env, a)
#define	dec_byte(a)	x86emuPrimOp_dec_byte(env, a)
#define	dec_word(a)	x86emuPrimOp_dec_word(env, a)
#define	dec_long(a)	x86emuPrimOp_dec_long(env, a)
#define	inc_byte(a)	x86emuPrimOp_inc_byte(env, a)
#define	inc_word(a)	x86emuPrimOp_inc_word(env, a)
#define	inc_long(a)	x86emuPrimOp_inc_long(env, a)
#define	or_byte(a, b)	x86emuPrimOp_or_byte(env, a, b)
#define	or_word(a, b)	x86emuPrimOp_or_word(env, a, b)
#define	or_long(a, b)	x86emuPrimOp_or_long(env, a, b)
#define	neg_byte(a)	x86emuPrimOp_neg_byte(env, a)
#define	neg_word(a)	x86emuPrimOp_neg_word(env, a)
#define	neg_long(a)	x86emuPrimOp_neg_long(env, a)
#define	not_byte(a)	x86emuPrimOp_not_byte(env, a)
#define	not_word(a)	x86emuPrimOp_not_word(env, a)
#define	not_long(a)	x86emuPrimOp_not_long(env, a)
#define	rcl_byte(a, b)	x86emuPrimOp_rcl_byte(env, a, b)
#define	rcl_word(a, b)	x86emuPrimOp_rcl_word(env, a, b)
#define	rcl_long(a, b)	x86emuPrimOp_rcl_long(env, a, b)
#define	rcr_byte(a, b)	x86emuPrimOp_rcr_byte(env, a, b)
#define	rcr_word(a, b)	x86emuPrimOp_rcr_word(env, a, b)
#define	rcr_long(a, b)	x86emuPrimOp_rcr_long(env, a, b)
#define	rol_byte(a, b)	x86emuPrimOp_rol_byte(env, a, b)
#define	rol_word(a, b)	x86emuPrimOp_rol_word(env, a, b)
#define	rol_long(a, b)	x86emuPrimOp_rol_long(env, a, b)
#define	ror_byte(a, b)	x86emuPrimOp_ror_byte(env, a, b)
#define	ror_word(a, b)	x86emuPrimOp_ror_word(env, a, b)
#define	ror_long(a, b)	x86emuPrimOp_ror_long(env, a, b)
#define	shl_byte(a, b)	x86emuPrimOp_shl_byte(env, a, b)
#define	shl_word(a, b)	x86emuPrimOp_shl_word(env, a, b)
#define	shl_long(a, b)	x86emuPrimOp_shl_long(env, a, b)
#define	shr_byte(a, b)	x86emuPrimOp_shr_byte(env, a, b)
#define	shr_word(a, b)	x86emuPrimOp_shr_word(env, a, b)
#define	shr_long(a, b)	x86emuPrimOp_shr_long(env, a, b)
#define	sar_byte(a, b)	x86emuPrimOp_sar_byte(env, a, b)
#define	sar_word(a, b)	x86emuPrimOp_sar_word(env, a, b)
#define	sar_long(a, b)	x86emuPrimOp_sar_long(env, a, b)
#define	shld_word(a, b, c)	x86emuPrimOp_shld_word(env, a, b, c)
#define	shld_long(a, b, c)	x86emuPrimOp_shld_long(env, a, b, c)
#define	shrd_word(a, b, c)	x86emuPrimOp_shrd_word(env, a, b, c)
#define	shrd_long(a, b, c)	x86emuPrimOp_shrd_long(env, a, b, c)
#define	sbb_byte(a, b)	x86emuPrimOp_sbb_byte(env, a, b)
#define	sbb_word(a, b)	x86emuPrimOp_sbb_word(env, a, b)
#define	sbb_long(a, b)	x86emuPrimOp_sbb_long(env, a, b)
#define	sub_byte(a, b)	x86emuPrimOp_sub_byte(env, a, b)
#define	sub_word(a, b)	x86emuPrimOp_sub_word(env, a, b)
#define	sub_long(a, b)	x86emuPrimOp_sub_long(env, a, b)
#define	test_byte(a, b)	x86emuPrimOp_test_byte(env, a, b)
#define	test_word(a, b)	x86emuPrimOp_test_word(env, a, b)
#define	test_long(a, b)	x86emuPrimOp_test_long(env, a, b)
#define	xor_byte(a, b)	x86emuPrimOp_xor_byte(env, a, b)
#define	xor_word(a, b)	x86emuPrimOp_xor_word(env, a, b)
#define	xor_long(a, b)	x86emuPrimOp_xor_long(env, a, b)
#define	imul_byte(a)	x86emuPrimOp_imul_byte(env, a)
#define	imul_word(a)	x86emuPrimOp_imul_word(env, a)
#define	imul_long(a)	x86emuPrimOp_imul_long(env, a)
#define	imul_long_direct(a, b, c, d)	x86emuPrimOp_imul_long_direct(env, a, b, c, d)
#define	mul_byte(a)	x86emuPrimOp_mul_byte(env, a)
#define	mul_word(a)	x86emuPrimOp_mul_word(env, a)
#define	mul_long(a)	x86emuPrimOp_mul_long(env, a)
#define	idiv_byte(a)	x86emuPrimOp_idiv_byte(env, a)
#define	idiv_word(a)	x86emuPrimOp_idiv_word(env, a)
#define	idiv_long(a)	x86emuPrimOp_idiv_long(env, a)
#define	div_byte(a)	x86emuPrimOp_div_byte(env, a)
#define	div_word(a)	x86emuPrimOp_div_word(env, a)
#define	div_long(a)	x86emuPrimOp_div_long(env, a)
#define	ins(a)	x86emuPrimOp_ins(env, a)
#define	outs(a)	x86emuPrimOp_outs(env, a)
#define	mem_access_word(a)	x86emuPrimOp_mem_access_word(env, a)
#define	push_word(a)	x86emuPrimOp_push_word(env, a)
#define	push_long(a)	x86emuPrimOp_push_long(env, a)
#define	pop_word()	x86emuPrimOp_pop_word(env)
#define	pop_long()	x86emuPrimOp_pop_long(env)
#endif

#endif /* __X86EMU_PRIM_OPS_H */
