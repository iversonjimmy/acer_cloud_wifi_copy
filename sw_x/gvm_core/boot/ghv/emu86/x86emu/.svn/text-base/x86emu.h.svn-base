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
* Description:  Header file for public specific functions.
*               Any application linking against us should only
*               include this header
*
****************************************************************************/

#ifndef __X86EMU_X86EMU_H
#define __X86EMU_X86EMU_H

#include "x86emu_types.h"
#include "regs.h"
#include "printf.h"

/*---------------------- Macros and type definitions ----------------------*/

/*  #pragma	pack(1) */  /* Don't pack structs with function pointers! */

/****************************************************************************
REMARKS:
Data structure containing ponters to programmed I/O functions used by the
emulator. This is used so that the user program can hook all programmed
I/O for the emulator to handled as necessary by the user program. By
default the emulator contains simple functions that do not do access the
hardware in any way. To allow the emualtor access the hardware, you will
need to override the programmed I/O functions using the X86EMU_setupPioFuncs
function.

HEADER:
x86emu.h

MEMBERS:
inb		- Function to read a byte from an I/O port
inw		- Function to read a word from an I/O port
inl     - Function to read a dword from an I/O port
outb	- Function to write a byte to an I/O port
outw    - Function to write a word to an I/O port
outl    - Function to write a dword to an I/O port
****************************************************************************/
typedef struct {
    u8  	(*inb)(void *, X86EMU_pioAddr addr);
    u16 	(*inw)(void *, X86EMU_pioAddr addr);
    u32 	(*inl)(void *, X86EMU_pioAddr addr);
    void 	(*outb)(void *, X86EMU_pioAddr addr, u8 val);
    void 	(*outw)(void *, X86EMU_pioAddr addr, u16 val);
    void 	(*outl)(void *, X86EMU_pioAddr addr, u32 val);
} X86EMU_pioFuncs;

/****************************************************************************
REMARKS:
Data structure containing ponters to memory access functions used by the
emulator. This is used so that the user program can hook all memory
access functions as necessary for the emulator. By default the emulator
contains simple functions that only access the internal memory of the
emulator. If you need specialised functions to handle access to different
types of memory (ie: hardware framebuffer accesses and BIOS memory access
etc), you will need to override this using the X86EMU_setupMemFuncs
function.

HEADER:
x86emu.h

MEMBERS:
rdb		- Function to read a byte from an address
rdw		- Function to read a word from an address
rdl     - Function to read a dword from an address
wrb		- Function to write a byte to an address
wrw    	- Function to write a word to an address
wrl    	- Function to write a dword to an address
****************************************************************************/
typedef struct {
    u8  	(*rdb)(void *, u32 addr);
    u16 	(*rdw)(void *, u32 addr);
    u32 	(*rdl)(void *, u32 addr);
    void 	(*wrb)(void *, u32 addr, u8 val);
    void 	(*wrw)(void *, u32 addr, u16 val);
    void	(*wrl)(void *, u32 addr, u32 val);
} X86EMU_memFuncs;


typedef struct {
    int (*ill_op) (void *, u8 op);
    int (*ill_op2)(void *, u8 up2);
} X86EMU_illFuncs;

/*--------------------- type definitions -----------------------------------*/
/* Returns 1 if special-handled, or 0 to request normal handling */
typedef int (*X86EMU_intrFunc)(void *, int num);


/****************************************************************************
REMARKS:
Structure maintaining the emulator machine state.

MEMBERS:
mem_base		- Base real mode memory for the emulator
mem_size		- Size of the real mode memory block for the emulator
private			- private data pointer
x86			- X86 registers
****************************************************************************/
typedef struct {
	X86EMU_regs	x86;
        X86EMU_memFuncs	memfuncs;
        X86EMU_pioFuncs	piofuncs;
	X86EMU_illFuncs illfuncs;
	X86EMU_intrFunc intrfunc;
	u32 debugFlags;
} X86EMU_sysEnv;


/*-------------------------- Function Prototypes --------------------------*/

#ifdef  __cplusplus
extern "C" {            			/* Use "C" linkage when in C++ mode */
#endif

/* decode.c */

void 	X86EMU_exec(X86EMU_sysEnv *env);
void 	X86EMU_halt_sys(X86EMU_sysEnv *env);

#ifdef	DEBUG
#define	HALT_SYS()	\
	printf("halt_sys: file %s, line %d\n", __FILE__, __LINE__), \
	X86EMU_halt_sys(env)
#else
#define	HALT_SYS()	X86EMU_halt_sys(env)
#endif

/* Debug options */

#define DEBUG_DECODE_F          0x000001 /* print decoded instruction  */
#define DEBUG_TRACE_F           0x000002 /* dump regs before/after execution */
#define DEBUG_STEP_F            0x000004
#define DEBUG_DISASSEMBLE_F     0x000008
#define DEBUG_BREAK_F           0x000010
#define DEBUG_SVC_F             0x000020
#define DEBUG_FS_F              0x000080
#define DEBUG_PROC_F            0x000100
#define DEBUG_SYSINT_F          0x000200 /* bios system interrupts. */
#define DEBUG_TRACECALL_F       0x000400
#define DEBUG_INSTRUMENT_F      0x000800
#define DEBUG_MEM_TRACE_F       0x001000 
#define DEBUG_IO_TRACE_F        0x002000 
#define DEBUG_TRACECALL_REGS_F  0x004000
#define DEBUG_DECODE_NOPRINT_F  0x008000 
#define DEBUG_SAVE_IP_CS_F      0x010000
#define DEBUG_SYS_F             (DEBUG_SVC_F|DEBUG_FS_F|DEBUG_PROC_F)

#ifdef DEBUG

void 	X86EMU_trace_regs(X86EMU_sysEnv *env);
void 	X86EMU_trace_xregs(X86EMU_sysEnv *env);
void 	X86EMU_dump_memory(X86EMU_sysEnv *env, u16 seg, u16 off, u32 amt);
int 	X86EMU_trace_on(X86EMU_sysEnv *env);
int 	X86EMU_trace_off(X86EMU_sysEnv *env);

#else

static inline void	X86EMU_trace_regs(X86EMU_sysEnv *env) {}
static inline void	X86EMU_trace_xregs(X86EMU_sysEnv *env) {}
static inline void	X86EMU_dump_memory(X86EMU_sysEnv *env, u16 seg, u16 off, u32 amt) {}
static inline int	X86EMU_trace_on(X86EMU_sysEnv *env) { return -1; }
static inline int	X86EMU_trace_off(X86EMU_sysEnv *env) { return -1; }

#endif

#ifdef  __cplusplus
}                       			/* End of "C" linkage for C++   	*/
#endif

#endif /* __X86EMU_X86EMU_H */
