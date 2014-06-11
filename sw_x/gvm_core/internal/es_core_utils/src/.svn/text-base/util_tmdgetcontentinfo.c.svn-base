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

#include "es_core_utils.h"

#include "gvm_errors.h"
#include "log.h"
#include "tmdviewer.h"
#include "vplex_assert.h"

int
Util_TmdGetContentInfo(const void* tmd, u32 tmdSize,
        u32* numContentInfos_out, ESContentInfo** contentInfos_out)
{
    int rv;
    u32 numContentInfos = 0;
    u32 numTotalContents;
    
    ASSERT_NOT_NULL(numContentInfos_out);
    ASSERT_NOT_NULL(contentInfos_out);
    *contentInfos_out = NULL;
    
    // pull out desired content info from ES library
    rv = tmdv_getContentInfos(tmd,
                              tmdSize,
                              0,
                              NULL,
                              &numContentInfos,
                              &numTotalContents);
    if (rv != 0) {
        LOG_ERROR("tmdv_getContentInfos failed: %d", rv);
        goto done;
    }
    
    {
        ESContentInfo* contentInfoArray = (ESContentInfo*)malloc(sizeof(ESContentInfo) * numTotalContents);
        if (contentInfoArray == NULL) {
            LOG_ERROR("malloc failed, numTotalContents="FMTu32, numTotalContents);
            rv = UTIL_ERR_NO_MEM;
            goto done;
        }
        numContentInfos = numTotalContents;
        rv = tmdv_getContentInfos(tmd,
                                  tmdSize,
                                  0,
                                  contentInfoArray,
                                  numContentInfos_out,
                                  &numTotalContents);
        if (rv != 0) {
            LOG_ERROR("tmdv_getContentInfos failed: %d", rv);
            free(contentInfoArray);
            goto done;
        }
        *contentInfos_out = contentInfoArray;
        ASSERT_EQUAL(*numContentInfos_out, numContentInfos, FMTu32);
        ASSERT_EQUAL(numTotalContents, numContentInfos, FMTu32);
    }
done:
    return rv;
}
