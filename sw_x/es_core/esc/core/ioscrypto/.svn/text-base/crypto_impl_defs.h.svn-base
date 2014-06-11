/*
 *  Copyright 2010 iGware, Inc.
 *  All Rights Reserved.
 *
 *  This software contains confidential information and 
 *  trade secrets of iGware, Inc.
 *  Use, disclosure or reproduction is prohibited without 
 *  the prior express written permission of iGware, Inc.
 */

/*
 *               Copyright (C) 2010, BroadOn Communications Corp.
 *
 *  These coded instructions, statements, and computer programs contain
 *  unpublished  proprietary information of BroadOn Communications Corp.,
 *  and  are protected by Federal copyright law. They may not be disclosed
 *  to  third  parties or copied or duplicated in any form, in whole or in
 *  part, without the prior written consent of BroadOn Communications Corp.
 */

#ifndef __CRYPTO_IMPL_DEFS_H__
#define __CRYPTO_IMPL_DEFS_H__

/*
 * Platform specific definitions exposed as part of the IOSC external API
 */

/*
 * No alignment constraints on GVM
 */
#define IOSC_AES_ADDR_ALIGN     1
#define IOSC_SHA1_ADDR_ALIGN    1
#define IOSC_SHA256_ADDR_ALIGN  1
#define IOSC_RSA_ADDR_ALIGN     1

#define ATTR_AES_ALIGN    // __attribute__ ((aligned(IOSC_AES_ADDR_ALIGN)))
#define ATTR_SHA1_ALIGN   // __attribute__ ((aligned(IOSC_SHA1_ADDR_ALIGN)))
#define ATTR_SHA256_ALIGN // __attribute__ ((aligned(IOSC_SHA256_ADDR_ALIGN)))
#define ATTR_SHA_ALIGN    // __attribute__ ((aligned(IOSC_SHA256_ADDR_ALIGN)))
#define ATTR_RSA_ALIGN    // __attribute__ ((aligned(IOSC_RSA_ADDR_ALIGN)))

/*
 * Macros to align sizes for array declarations
 */
#define IOSC_ROUND_UP(n,sz)     (((n) + ((sz) - 1)) & ~((sz) - 1))
#define SIZE_AES_ALIGN(n)       (n)   // IOSC_ROUND_UP(n,IOSC_AES_ADDR_ALIGN)
#define SIZE_SHA1_ALIGN(n)      (n)   // IOSC_ROUND_UP(n,IOSC_SHA1_ADDR_ALIGN)
#define SIZE_SHA256_ALIGN(n)    (n)   // IOSC_ROUND_UP(n,IOSC_SHA256_ADDR_ALIGN)
#define SIZE_SHA_ALIGN(n)       (n)   // IOSC_ROUND_UP(n,IOSC_SHA256_ADDR_ALIGN)
#define SIZE_RSA_ALIGN(n)       (n)   // IOSC_ROUND_UP(n,IOSC_RSA_ADDR_ALIGN)

/*
 * Hash Context Size - max of any hash functions supported
 */
#define IOSC_HASH_CONTEXT_SIZE  112

#endif // __CRYPTO_IMPL_DEFS_H__
