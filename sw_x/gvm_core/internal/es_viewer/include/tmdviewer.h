/*
 *  Copyright 2011 iGware Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF IGWARE INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
 *
 */
#ifndef __TMDVIEWER_H__
#define __TMDVIEWER_H__

#include "estypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Error codes. */
enum {
    TMDV_ERR_OK = 0,
    TMDV_ERR_BAD_ARG,
    TMDV_ERR_BAD_TMD_SIZE,
};

/** Return the title ID. */
int tmdv_getTitleId(const void *tmd, u32 tmdSize, ESTitleId *outTitldId);

/** Return the title version. */
int tmdv_getTitleVersion(const void *tmd, u32 tmdSize, ESTitleVersion *outTitleVersion);

/** Return the minor title version. */
int tmdv_getMinorTitleVersion(const void *tmd, u32 tmdSize, ESMinorTitleVersion *outMinorTitleVersion);

/** Return the system software version. */
int tmdv_getSysVersion(const void *tmd, u32 tmdSize, ESSysVersion *outSysVersion);

/** Return the title type. */
int tmdv_getTitleType(const void *tmd, u32 tmdSize, ESTitleType *outTitleType);

/** Return the group ID. */
int tmdv_getGroupId(const void *tmd, u32 tmdSize, u16 *outGroupId);

/** Return the access rights. */
int tmdv_getAccessRights(const void *tmd, u32 tmdSize, u32 *outAccessRights);

/** Return the boot index. */
int tmdv_getBootContentIndex(const void *tmd, u32 tmdSize, ESContentIndex *outBootContentIndex);

/** Return the platform-specific custom data. */
int tmdv_getCustomData(const void *tmd, u32 tmdSize, u8 *outCustomData, u32 *outCustomDataLen);

/** Return content infos.
 * On call, outNumContentInfos contains the size of outContentInfos[].
 * On return, outNumContentInfos contains the number of entries returned in outContentInfos[].
 * On return, outTotal contains the number of contents in the TMD (regardless of how many were returned).
 * If offset > 0, that many contents will be skipped in the response.
 */
int tmdv_getContentInfos(const void *tmd, u32 tmdSize, u32 offset, ESContentInfo outContentInfos[], u32 *outNumContentInfos, u32 *outTotal);

/** Return content infos of specific content indices.
 * On call, outNumContentInfos contains the size of outContentInfos[].
 * On return, outNumContentInfos contains the number of entries returned in outContentInfos[].
 */
int tmdv_findContentInfos(const void *tmd, u32 tmdSize, ESContentIndex indexes[], u32 nIndexes, ESContentInfo outContentInfos[], u32 *outNumContentInfos);

#ifdef __cplusplus
}
#endif

#endif // include guard
