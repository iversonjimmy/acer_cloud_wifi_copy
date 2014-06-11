/*
 *               Copyright (C) 2010, BroadOn Communications Corp.
 *
 *  These coded instructions, statements, and computer programs contain
 *  unpublished  proprietary information of BroadOn Communications Corp.,
 *  and  are protected by Federal copyright law. They may not be disclosed
 *  to  third  parties or copied or duplicated in any form, in whole or in
 *  part, without the prior written consent of BroadOn Communications Corp.
 *
 */


#ifndef __IOSENDIAN_H__
#define __IOSENDIAN_H__

// Endianness conversion support
#define TRUNCATE_TO_U32(x)  ((u32)((x) & 0xffffffff))

// Define endian conversion macro only if it hasn't already been defined.
// In particular, when compiling for a linux kernel module, 
// we want to prefer the definition in the linux kernel header over ours.

#ifdef ANDROID
# include <endian.h>
#elif defined(HOST_IS_BIG_ENDIAN)
# ifndef htons
    #define htons(x)   (x)
# endif
# ifndef ntohs
    #define ntohs(x)   (x)
# endif
# ifndef htonl
    #define htonl(x)   (x)
# endif
# ifndef ntohl
    #define ntohl(x)   (x)
# endif
#elif defined(_MSC_VER) && !defined(VPL_PLAT_IS_WINRT)
#include <winsock2.h>
#else
# ifndef ntohs
    #define ntohs(x)   ((u16)((((x) & 0xff) << 8) | (((x) & 0xff00) >> 8)))
# endif
# ifndef ntohl
    #define ntohl(x)   ((((x) & 0xff) << 24) | (((x) & 0xff00) << 8) | (((x) & 0xff0000) >> 8) | (((x) & 0xff000000) >> 24))
# endif
# ifndef htons
    #define htons(x)   ntohs(x)
# endif
# ifndef htonl
    #define htonl(x)   ntohl(x)
# endif
#endif

#if defined(HOST_IS_BIG_ENDIAN)
# ifndef htonll
#   define htonll(x)  (x)
# endif
# ifndef ntohll
#   define ntohll(x)  (x)
# endif
#else
# ifndef htonll
#   define htonll(x)  ntohll(x)
# endif
# ifndef ntohll
#   define ntohll(x) ((u64)( ((u64)((ntohl(TRUNCATE_TO_U32(x)))) << 32) | (ntohl(TRUNCATE_TO_U32(((u64)(x))>>32))) ))
# endif
#endif

#endif  // __IOSENDIAN_H__
