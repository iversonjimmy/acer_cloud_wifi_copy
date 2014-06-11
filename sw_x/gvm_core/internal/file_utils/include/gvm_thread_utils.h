/*
 *               Copyright (C) 2009, BroadOn Communications Corp.
 *
 *  These coded instructions, statements, and computer programs contain
 *  unpublished  proprietary information of BroadOn Communications Corp.,
 *  and  are protected by Federal copyright law. They may not be disclosed
 *  to  third  parties or copied or duplicated in any form, in whole or in
 *  part, without the prior written consent of BroadOn Communications Corp.
 *
 */

#ifndef _GVM_THREAD_UTILS_H_
#define _GVM_THREAD_UTILS_H_

#include "vplu_types.h"

#include "vpl_th.h" 

#ifdef  __cplusplus
extern "C" {
#endif

#ifdef ANDROID
// On Android, res_queryN() (which is used within getaddrinfo()) declares a 64K buffer on the stack!
#  define UTIL_DEFAULT_THREAD_STACK_SIZE  (192 * 1024)
#else
#  define UTIL_DEFAULT_THREAD_STACK_SIZE  (16 * 1024)
#endif

/// A convenient wrapper for #VPLDetachableThread_Create().
/// @note If you set @a isJoinable to true, you MUST call #VPLDetachableThread_Join() or
///     #VPLDetachableThread_Detach() to avoid leaking resources.
/// @param[out] threadId_out You can specify NULL if isJoinable is false and you don't need the handle.
int Util_SpawnThread(VPLDetachableThread_fn_t threadFunc,
                     void* threadArg,
                     size_t stackSize,
                     VPL_BOOL isJoinable,
                     VPLDetachableThreadHandle_t* threadId_out);

#ifdef  __cplusplus
}
#endif

#endif // include guard
