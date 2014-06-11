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

#include "gvm_utils.h"

#include "vpl_plat.h"
#include "log.h"
#include "gvm_errors.h"
#include <errno.h>
#include <limits.h>
#ifdef ANDROID
# include <asm/page.h> // pthread.h on Android refers to PAGE_SIZE but doesn't include it!
#endif
#include <sys/stat.h>
#include <sys/types.h>

int
Util_SpawnThread(
        VPLDetachableThread_fn_t threadFunc,
        void* threadArg,
        size_t stackSize,
        VPL_BOOL isJoinable,
        VPLDetachableThreadHandle_t* threadId_out)
{
    int rv;
    VPLThread_attr_t threadAttrs;
    VPLDetachableThreadHandle_t tempThreadId;

    LOG_FUNC_ENTRY(LOG_LEVEL_DEBUG);
    rv = VPLThread_AttrInit(&threadAttrs);
    if (rv < 0) {
        LOG_ERROR("VPLThread_AttrInit returned %d", rv);
        goto out;
    }
    rv = VPLThread_AttrSetStackSize(&threadAttrs, VPLTHREAD_STACKSIZE_MIN + stackSize);
    if (rv < 0) {
        LOG_ERROR("VPLThread_AttrSetStackSize returned %d", rv);
        goto out2;
    }
    if (!isJoinable) {
        rv = VPLThread_AttrSetDetachState(&threadAttrs, VPL_TRUE);
        if (rv < 0) {
            LOG_ERROR("VPLThread_AttrSetDetachState returned %d", rv);
            goto out2;
        }
    }
    // Allow threadId_out to be NULL.
    if (threadId_out == NULL) {
        threadId_out = &tempThreadId;
    }
    rv = VPLDetachableThread_Create(threadId_out, threadFunc, threadArg, &threadAttrs, NULL);
    if (rv < 0) {
        LOG_ERROR("VPLDetachableThread_Create returned %d", rv);
    }
out2:
    VPLThread_AttrDestroy(&threadAttrs);
out:
    return rv;
}
