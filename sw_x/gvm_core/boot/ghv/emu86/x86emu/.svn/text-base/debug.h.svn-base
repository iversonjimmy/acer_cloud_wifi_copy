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
 * Description:  Header file for debug definitions.
 *
 ****************************************************************************/

#ifndef __X86EMU_DEBUG_H
#define __X86EMU_DEBUG_H

#define DECODE_PRINTF(x) if ((env->debugFlags & DEBUG_DECODE_F)) printf(x)
#define DECODE_PRINTF2(x,y) if ((env->debugFlags & DEBUG_DECODE_F)) printf(x,y)
#define SAVE_IP_CS(x,y) if ((env->debugFlags & DEBUG_DECODE_F)) printf("%04x:%04x:\t", env->x86.R_CS, env->x86.R_IP)

#define INC_DECODED_INST_LEN(x)
#define TRACE_REGS()
#define SINGLE_STEP()
#define TRACE_AND_STEP()			\
    TRACE_REGS();				\
    SINGLE_STEP()

#define START_OF_INSTR()
#define END_OF_INSTR()
#define END_OF_INSTR_NO_TRACE()

#define CALL_TRACE(u,v,w,x,s)
#define RETURN_TRACE(n,u,v)

/*-------------------------- Function Prototypes --------------------------*/

#ifdef  __cplusplus
extern "C" {            			/* Use "C" linkage when in C++ mode */
#endif

#ifdef DEBUG

extern void x86emu_inc_decoded_inst_len (X86EMU_sysEnv *env, int x);
extern void x86emu_decode_printf (X86EMU_sysEnv *env, char *x);
extern void x86emu_decode_printf2 (X86EMU_sysEnv *env, char *x, int y);
extern void x86emu_just_disassemble (X86EMU_sysEnv *env);
extern void x86emu_single_step (X86EMU_sysEnv *env);
extern void x86emu_end_instr (X86EMU_sysEnv *env);
extern void x86emu_dump_regs (X86EMU_sysEnv *env);
extern void x86emu_dump_xregs (X86EMU_sysEnv *env);
extern void x86emu_print_int_vect (X86EMU_sysEnv *env, u16 iv);
extern void x86emu_instrument_instruction (X86EMU_sysEnv *env);
extern void x86emu_check_ip_access (X86EMU_sysEnv *env);
extern void x86emu_check_sp_access (X86EMU_sysEnv *env);
extern void x86emu_check_mem_access (X86EMU_sysEnv *env, u32 p);
extern void x86emu_check_data_access (X86EMU_sysEnv *env, u16 s, u16 o);

#else

static inline void x86emu_inc_decoded_inst_len (X86EMU_sysEnv *env, int x) { }
static inline void x86emu_decode_printf (X86EMU_sysEnv *env, char *x) { }
static inline void x86emu_decode_printf2 (X86EMU_sysEnv *env, char *x, int y) { }
static inline void x86emu_just_disassemble (X86EMU_sysEnv *env) { }
static inline void x86emu_single_step (X86EMU_sysEnv *env) { }
static inline void x86emu_end_instr (X86EMU_sysEnv *env) { }
static inline void x86emu_dump_regs (X86EMU_sysEnv *env) { }
static inline void x86emu_dump_xregs (X86EMU_sysEnv *env) { }
static inline void x86emu_print_int_vect (X86EMU_sysEnv *env, u16 iv) { }
static inline void x86emu_instrument_instruction (X86EMU_sysEnv *env) { }
static inline void x86emu_check_ip_access (X86EMU_sysEnv *env) { }
static inline void x86emu_check_sp_access (X86EMU_sysEnv *env) { }
static inline void x86emu_check_mem_access (X86EMU_sysEnv *env, u32 p) { }
static inline void x86emu_check_data_access (X86EMU_sysEnv *env, u16 s, u16 o) { }

#endif

#ifdef  __cplusplus
}                  			/* End of "C" linkage for C++ */
#endif

#define check_ip_access() x86emu_check_ip_access(env)
#define check_sp_access() x86emu_check_sp_access(env)
#define check_mem_access(a) x86emu_check_mem_access(env,a)
#define check_data_access(s, o) x86emu_check_data_access(env, s, o)

#endif /* __X86EMU_DEBUG_H */
