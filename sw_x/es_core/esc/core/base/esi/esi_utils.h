/*
 *               Copyright (C) 2010, BroadOn Communications Corp.
 *
 *  These coded instructions, statements, and computer programs contain
 *  unpublished proprietary information of BroadOn Communications Corp.,
 *  and are protected by federal copyright law. They may not be disclosed
 *  to third parties or copied or duplicated in any form, in whole or in
 *  part, without the prior written consent of BroadOn Communications Corp.
 *
 */


#ifndef __ESI_UTILS_H__
#define __ESI_UTILS_H__


#include <esitypes.h>
#include <iosc.h>
#include <ioslibc.h>


#define ES_DEBUG_CRIT               1
#define ES_DEBUG_ERROR              2
#define ES_DEBUG_WARN               4
#define ES_DEBUG_INFO               8
#define ES_DEBUG_NOTIFY             16
#define ES_DEBUG_DEBUG              32
#define ES_DEBUG_TRACE              64

#define ES_DEBUG_LEVEL              0x7


#if !defined(GHV)
#define ES_DEBUG_LOG
#endif // !GHV

#if !defined(esLog)

#if defined(ES_DEBUG_LOG)

#define esLog(l, ...)                                           \
    do {                                                        \
        if ((l) & ES_DEBUG_LEVEL) {                             \
            printf("[ES] "__VA_ARGS__);                         \
        }                                                       \
    } while (0)

#else

#define esLog(l, ...)               (void)(0)

#endif  // defined(ES_DEBUG_LOG)

#endif  // !defined(esLog)


#endif  // __ESI_UTILS_H__
