/****************************************************************************
*
*						Realmode X86 Emulator Library
*
*	Copyright(C) 1996-1999 SciTech Software, Inc.
*	Copyright(C) David Mosberger-Tang
*	Copyright(C) 1999 Egbert Eich
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
* Description:  Header file for instruction decoding logic.
*
****************************************************************************/

#ifndef __X86EMU_DECODE_H
#define __X86EMU_DECODE_H

/*---------------------- Macros and type definitions ----------------------*/

/* Instruction Decoding Stuff */

#define FETCH_DECODE_MODRM(mod,rh,rl) 	fetch_decode_modrm(&mod,&rh,&rl)
#define DECODE_RM_BYTE_REGISTER(r)    	decode_rm_byte_register(r)
#define DECODE_RM_WORD_REGISTER(r)    	decode_rm_word_register(r)
#define DECODE_RM_LONG_REGISTER(r)    	decode_rm_long_register(r)
#define DECODE_CLEAR_SEGOVR()         	env->x86.mode &= ~SYSMODE_CLRMASK

/*-------------------------- Function Prototypes --------------------------*/

#ifdef  __cplusplus
extern "C" {            			/* Use "C" linkage when in C++ mode */
#endif

void	x86emu_intr_raise(X86EMU_sysEnv *env, u8 type);
void	x86emu_fetch_decode_modrm(X86EMU_sysEnv *env, int *mod,int *regh,int *regl);
u8	x86emu_fetch_byte_imm(X86EMU_sysEnv *env);
u16	x86emu_fetch_word_imm(X86EMU_sysEnv *env);
u32	x86emu_fetch_long_imm(X86EMU_sysEnv *env);
u8	x86emu_fetch_data_byte(X86EMU_sysEnv *env, u16 offset);
u8	x86emu_fetch_data_byte_abs(X86EMU_sysEnv *env, u16 segment, u16 offset);
u16	x86emu_fetch_data_word(X86EMU_sysEnv *env, u16 offset);
u16	x86emu_fetch_data_word_abs(X86EMU_sysEnv *env, u16 segment, u16 offset);
u32	x86emu_fetch_data_long(X86EMU_sysEnv *env, u16 offset);
u32	x86emu_fetch_data_long_abs(X86EMU_sysEnv *env, u16 segment, u16 offset);
void	x86emu_store_data_byte(X86EMU_sysEnv *env, u16 offset, u8 val);
void	x86emu_store_data_byte_abs(X86EMU_sysEnv *env, u16 segment, u16 offset, u8 val);
void	x86emu_store_data_word(X86EMU_sysEnv *env, u16 offset, u16 val);
void	x86emu_store_data_word_abs(X86EMU_sysEnv *env, u16 segment, u16 offset, u16 val);
void	x86emu_store_data_long(X86EMU_sysEnv *env, u16 offset, u32 val);
void	x86emu_store_data_long_abs(X86EMU_sysEnv *env, u16 segment, u16 offset, u32 val);
u8*	x86emu_decode_rm_byte_register(X86EMU_sysEnv *env, int reg);
u16*	x86emu_decode_rm_word_register(X86EMU_sysEnv *env, int reg);
u32*	x86emu_decode_rm_long_register(X86EMU_sysEnv *env, int reg);
u16*	x86emu_decode_rm_seg_register(X86EMU_sysEnv *env, int reg);
u32	x86emu_decode_rm00_address(X86EMU_sysEnv *env, int rm);
u32	x86emu_decode_rm01_address(X86EMU_sysEnv *env, int rm);
u32	x86emu_decode_rm10_address(X86EMU_sysEnv *env, int rm);
u32	x86emu_decode_sib_address(X86EMU_sysEnv *env, int sib, int mod);

static inline
u32	x86emu_decode_rm_address(X86EMU_sysEnv *env, int mod, int rm) {
    if (mod == 0) {
	return x86emu_decode_rm00_address(env, rm);
    } else if (mod == 1) {
	return x86emu_decode_rm01_address(env, rm);
    } else {
	return x86emu_decode_rm10_address(env, rm);
    }
}

static inline
const char *x86emu_decode_cond_name(u8 op) {
    static const char *const __x86emu_cond_names[] = {
	"O", "NO", "B", "AE", "Z", "NZ", "BE", "A",
	"S", "NS", "P", "NP", "L", "GE", "LE", "G",
    };
    return __x86emu_cond_names[op & 0xf];
};

static inline
int x86emu_decode_test_cond(X86EMU_sysEnv *env, u8 op)
{
#define xorl(a,b)   ((a) && !(b)) || (!(a) && (b))
    switch (op & 0xf) {
    case 0x0:
        return ACCESS_FLAG(F_OF);
    case 0x1:
        return !ACCESS_FLAG(F_OF);
    case 0x2:
        return ACCESS_FLAG(F_CF);
    case 0x3:
        return !ACCESS_FLAG(F_CF);
    case 0x4:
        return ACCESS_FLAG(F_ZF);
    case 0x5:
        return !ACCESS_FLAG(F_ZF);
    case 0x6:
        return ACCESS_FLAG(F_CF) || ACCESS_FLAG(F_ZF);
    case 0x7:
        return !(ACCESS_FLAG(F_CF) || ACCESS_FLAG(F_ZF));
    case 0x8:
        return ACCESS_FLAG(F_SF);
    case 0x9:
        return !ACCESS_FLAG(F_SF);
    case 0xa:
        return ACCESS_FLAG(F_PF);
    case 0xb:
        return !ACCESS_FLAG(F_PF);
    case 0xc:
        return xorl(ACCESS_FLAG(F_SF), ACCESS_FLAG(F_OF));
    case 0xd:
        return !(xorl(ACCESS_FLAG(F_SF), ACCESS_FLAG(F_OF)));
    case 0xe:
        return (xorl(ACCESS_FLAG(F_SF), ACCESS_FLAG(F_OF)) || ACCESS_FLAG(F_ZF));
    case 0xf:
        return !(xorl(ACCESS_FLAG(F_SF), ACCESS_FLAG(F_OF)) || ACCESS_FLAG(F_ZF));
    }
    return 0;
}

#ifdef  __cplusplus
}                       			/* End of "C" linkage for C++   	*/
#endif

#define	fetch_decode_modrm(a, b, c)	x86emu_fetch_decode_modrm(env, a, b, c)
#define	fetch_byte_imm()		x86emu_fetch_byte_imm(env)
#define	fetch_word_imm()		x86emu_fetch_word_imm(env)
#define	fetch_long_imm()		x86emu_fetch_long_imm(env)
#define	fetch_data_byte(a)		x86emu_fetch_data_byte(env, a)
#define	fetch_data_byte_abs(a, b)	x86emu_fetch_data_byte_abs(env, a, b)
#define	fetch_data_word(a)		x86emu_fetch_data_word(env, a)
#define	fetch_data_word_abs(a, b)	x86emu_fetch_data_word_abs(env, a, b)
#define	fetch_data_long(a)		x86emu_fetch_data_long(env, a)
#define	fetch_data_long_abs(a, b)	x86emu_fetch_data_long_abs(env, a, b)
#define	store_data_byte(a, b)		x86emu_store_data_byte(env, a, b)
#define	store_data_byte_abs(a, b, c)	x86emu_store_data_byte_abs(env, a, b, c)
#define	store_data_word(a, b)		x86emu_store_data_word(env, a, b)
#define	store_data_word_abs(a, b, c)	x86emu_store_data_word_abs(env, a, b, c)
#define	store_data_long(a, b)		x86emu_store_data_long(env, a, b)
#define	store_data_long_abs(a, b, c)	x86emu_store_data_long_abs(env, a, b, c)
#define	decode_rm_byte_register(a)	x86emu_decode_rm_byte_register(env, a)
#define	decode_rm_word_register(a)	x86emu_decode_rm_word_register(env, a)
#define	decode_rm_long_register(a)	x86emu_decode_rm_long_register(env, a)
#define	decode_rm_seg_register(a)	x86emu_decode_rm_seg_register(env, a)
#define	decode_sib_address(a, b)	x86emu_decode_sib_address(env, a, b)
#define decode_rm_address(a, b)		x86emu_decode_rm_address(env, a, b)

#define sys_rdb(a) env->memfuncs.rdb(env, a)
#define sys_rdw(a) env->memfuncs.rdw(env, a)
#define sys_rdl(a) env->memfuncs.rdl(env, a)
#define sys_wrb(a, v) env->memfuncs.wrb(env, a, v)
#define sys_wrw(a, v) env->memfuncs.wrw(env, a, v)
#define sys_wrl(a, v) env->memfuncs.wrl(env, a, v)

#define sys_outb(p, v) env->piofuncs.outb(env, p, v)
#define sys_outw(p, v) env->piofuncs.outw(env, p, v)
#define sys_outl(p, v) env->piofuncs.outl(env, p, v)
#define sys_inb(p) env->piofuncs.inb(env, p)
#define sys_inw(p) env->piofuncs.inw(env, p)
#define sys_inl(p) env->piofuncs.inl(env, p)

#endif /* __X86EMU_DECODE_H */
