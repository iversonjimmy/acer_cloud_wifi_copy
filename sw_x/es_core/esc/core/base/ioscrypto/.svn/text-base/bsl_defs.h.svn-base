/*
 *               Copyright (C) 2005, BroadOn Communications Corp.
 *
 *  These coded instructions, statements, and computer programs contain
 *  unpublished  proprietary information of BroadOn Communications Corp.,
 *  and  are protected by Federal copyright law. They may not be disclosed
 *  to  third  parties or copied or duplicated in any form, in whole or in
 *  part, without the prior written consent of BroadOn Communications Corp.
 *
 */

#ifndef __BSL_DEFS_H__
#define __BSL_DEFS_H__

/* 
 * Not exposed
 */

#define BSL_TYPE_SHIFT 4
#define BSL_TYPE_MASK 0xf0
#define BSL_SUBTYPE_MASK 0x0f
#define BSL_MAXRAND_BYTES 32
#define BSL_MAXSECRET_BYTES 32
#define BSL_SHAHASH_WORDS 5
#define BSL_SHABLOCK_WORDS 16

#define BSL_PROT_NO_EXPORT 0
#define BSL_PROT_PTXT_EXPORT 1
#define BSL_PROT_ENC_EXPORT 2

#define BSL_MAX_DEFAULT_HANDLES    11

typedef struct {
  u32 digest[5];          /* message digest */
  u32 count_lo, count_hi; /* 64-bit bit count */
  u32 data[16];           /* SHA data buffer */
} BSLShaHwContext;

#endif  // __BSL_DEFS_H__
