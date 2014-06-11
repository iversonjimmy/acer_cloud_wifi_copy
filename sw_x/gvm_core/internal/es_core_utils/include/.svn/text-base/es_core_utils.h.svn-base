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

#ifndef _ES_CORE_UTILS_H_
#define _ES_CORE_UTILS_H_

#include "vplu_types.h"
#include "estypes.h"

#ifdef  __cplusplus
extern "C" {
#endif

/// Similar to #tmdv_getContentInfos() but allocates the #ESContentInfo array automatically.
/// Unlike #tmdv_getContentInfos(), @a numContentInfos_out is out-only.
/// Caller must free *contentInfos_out when finished with it.  *contentInfos_out will be set
/// to NULL on failure, so it should be safe to call free on it in either case.
int Util_TmdGetContentInfo(const void *tmd, u32 tmdSize,
        u32* numContentInfos_out, ESContentInfo** contentInfos_out);

#ifdef  __cplusplus
}
#endif

#endif // include guard
