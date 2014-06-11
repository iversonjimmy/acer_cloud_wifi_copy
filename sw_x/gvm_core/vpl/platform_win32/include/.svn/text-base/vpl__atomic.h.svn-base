/*
*                Copyright (C) 2009-2010, BroadOn Communications Corp.
*
*   These coded instructions, statements, and computer programs contain
*   unpublished  proprietary information of BroadOn Communications Corp.,
*   and  are protected by Federal copyright law. They may not be disclosed
*   to  third  parties or copied or duplicated in any form, in whole or in
*   part, without the prior written consent of BroadOn Communications Corp.
*
*/

#ifndef __VPL__WIN32_PLAT_H__
#define __VPL__WIN32_PLAT_H__

#include <vpl__plat.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int32_t
VPLAtomic_add(volatile int32_t * operand, int32_t incr) 
{
    // Issue the atomic Exchange-and-Add. 
    // Save the resulting original pre-add  value in |temp|.
    int32_t oldval =  (int32_t) InterlockedExchangeAdd((volatile LONG*)operand, incr);
    
    // The result of _this_ atomic add-and-exchange must be
    // the original value (now in temp) plus |incr|.
    return (oldval + incr);
}

#ifdef  __cplusplus
}
#endif

#endif // include guard
