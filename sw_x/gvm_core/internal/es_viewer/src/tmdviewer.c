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
#include "tmdviewer.h"

#include "esitypes.h"
#include "vpl_conv.h"
#include <string.h>
#include <stddef.h>

#define MAY_HAVE_CERTS_APPENDED 1

static int is_good_tmd_size(const void *_tmd, u32 tmdSize)
{
    if (tmdSize < sizeof(ESV1TitleMeta))  // too short to be valid
        return 0;

    {
        ESV1TitleMeta *tmd = (ESV1TitleMeta*)_tmd;
        u32 expectedSize = sizeof(ESV1TitleMeta) + sizeof(ESV1ContentMeta) * VPLConv_ntoh_u16(tmd->head.numContents);

#ifdef MAY_HAVE_CERTS_APPENDED
        return tmdSize >= expectedSize;
#else
        return tmdSize == expectedSize;
#endif
    }
}

int tmdv_getTitleId(const void *_tmd, u32 tmdSize, ESTitleId *outTitleId)
{
    if (_tmd == NULL || outTitleId == NULL)
        return TMDV_ERR_BAD_ARG;
    if (!is_good_tmd_size(_tmd, tmdSize))
        return TMDV_ERR_BAD_TMD_SIZE;

    {
        ESV1TitleMeta *tmd = (ESV1TitleMeta*)_tmd;
        *outTitleId = VPLConv_ntoh_u64(tmd->head.titleId);
    }
    return TMDV_ERR_OK;
}

int tmdv_getTitleVersion(const void *_tmd, u32 tmdSize, ESTitleVersion *outTitleVersion)
{
    if (_tmd == NULL || outTitleVersion == NULL)
        return TMDV_ERR_BAD_ARG;
    if (!is_good_tmd_size(_tmd, tmdSize))
        return TMDV_ERR_BAD_TMD_SIZE;

    {
        ESV1TitleMeta *tmd = (ESV1TitleMeta*)_tmd;
        *outTitleVersion = VPLConv_ntoh_u16(tmd->head.titleVersion);
    }
    return TMDV_ERR_OK;
}

int tmdv_getMinorTitleVersion(const void *_tmd, u32 tmdSize, ESMinorTitleVersion *outMinorTitleVersion)
{
    if (_tmd == NULL || outMinorTitleVersion == NULL)
        return TMDV_ERR_BAD_ARG;
    if (!is_good_tmd_size(_tmd, tmdSize))
        return TMDV_ERR_BAD_TMD_SIZE;

    {
        ESV1TitleMeta *tmd = (ESV1TitleMeta*)_tmd;
        *outMinorTitleVersion = VPLConv_ntoh_u16(tmd->head.minorTitleVersion);
    }
    return TMDV_ERR_OK;
}

int tmdv_getSysVersion(const void *_tmd, u32 tmdSize, ESSysVersion *outSysVersion)
{
    if (_tmd == NULL || outSysVersion == NULL)
        return TMDV_ERR_BAD_ARG;
    if (!is_good_tmd_size(_tmd, tmdSize))
        return TMDV_ERR_BAD_TMD_SIZE;

    {
        ESV1TitleMeta *tmd = (ESV1TitleMeta*)_tmd;
        *outSysVersion = VPLConv_ntoh_u64(tmd->head.sysVersion);
    }
    return TMDV_ERR_OK;
}

int tmdv_getTitleType(const void *_tmd, u32 tmdSize, ESTitleType *outTitleType)
{
    if (_tmd == NULL || outTitleType == NULL)
        return TMDV_ERR_BAD_ARG;
    if (!is_good_tmd_size(_tmd, tmdSize))
        return TMDV_ERR_BAD_TMD_SIZE;

    {
        ESV1TitleMeta *tmd = (ESV1TitleMeta*)_tmd;
        *outTitleType = VPLConv_ntoh_u32(tmd->head.type);
    }
    return TMDV_ERR_OK;
}

int tmdv_getGroupId(const void *_tmd, u32 tmdSize, u16 *outGroupId)
{
    if (_tmd == NULL || outGroupId == NULL)
        return TMDV_ERR_BAD_ARG;
    if (!is_good_tmd_size(_tmd, tmdSize))
        return TMDV_ERR_BAD_TMD_SIZE;
    
    {
        ESV1TitleMeta *tmd = (ESV1TitleMeta*)_tmd;
        *outGroupId = VPLConv_ntoh_u16(tmd->head.groupId);
    }
    return TMDV_ERR_OK;
}

int tmdv_getAccessRights(const void *_tmd, u32 tmdSize, u32 *outAccessRights)
{
    if (_tmd == NULL || outAccessRights == NULL)
        return TMDV_ERR_BAD_ARG;
    if (!is_good_tmd_size(_tmd, tmdSize))
        return TMDV_ERR_BAD_TMD_SIZE;

    {
        ESV1TitleMeta *tmd = (ESV1TitleMeta*)_tmd;
        *outAccessRights = VPLConv_ntoh_u32(tmd->head.accessRights);
    }
    return TMDV_ERR_OK;
}

int tmdv_getBootContentIndex(const void *_tmd, u32 tmdSize, ESContentIndex *outBootContentIndex)
{
    if (_tmd == NULL || outBootContentIndex == NULL)
        return TMDV_ERR_BAD_ARG;
    if (!is_good_tmd_size(_tmd, tmdSize))
        return TMDV_ERR_BAD_TMD_SIZE;

    {
        ESV1TitleMeta *tmd = (ESV1TitleMeta*)_tmd;
        *outBootContentIndex = VPLConv_ntoh_u16(tmd->head.bootIndex);
    }
    return TMDV_ERR_OK;
}

int tmdv_getCustomData(const void *_tmd, u32 tmdSize, u8 *outCustomData, u32 *outCustomDataLen)
{
    if (_tmd == NULL || outCustomData == NULL || outCustomDataLen == NULL)
        return TMDV_ERR_BAD_ARG;
    if (!is_good_tmd_size(_tmd, tmdSize))
        return TMDV_ERR_BAD_TMD_SIZE;

    {
        ESV1TitleMeta *tmd = (ESV1TitleMeta*)_tmd;
        size_t copySize = *outCustomDataLen > sizeof(ESTmdCustomData) ? sizeof(ESTmdCustomData) : *outCustomDataLen;
        memcpy(outCustomData, tmd->head.customData, copySize);
        *outCustomDataLen = copySize;
    }
    return TMDV_ERR_OK;
}

int tmdv_getContentInfos(const void *_tmd, u32 tmdSize, u32 offset, ESContentInfo outContentInfos[], u32 *outNumContentInfos, u32 *outTotal)
{
    if (_tmd == NULL)
        return TMDV_ERR_BAD_ARG;
    if (!is_good_tmd_size(_tmd, tmdSize))
        return TMDV_ERR_BAD_TMD_SIZE;
    if ((outContentInfos == NULL || outNumContentInfos == NULL) &&
        outTotal == NULL)
        return TMDV_ERR_BAD_ARG;

    {
        ESV1TitleMeta *tmd = (ESV1TitleMeta*)_tmd;
        ESV1ContentMeta *contents = (ESV1ContentMeta*)&tmd[1];
        u16 numContents = VPLConv_ntoh_u16(tmd->head.numContents);

        if (outTotal != NULL)
            *outTotal = numContents;

        if (outContentInfos != NULL && outNumContentInfos != NULL) {
            u32 i;
            for (i = 0; i < *outNumContentInfos; i++) {
                if (i + offset >= numContents)
                    break;
                outContentInfos[i].id = VPLConv_ntoh_u32(contents[i + offset].cid);
                outContentInfos[i].index = VPLConv_ntoh_u16(contents[i + offset].index);
                outContentInfos[i].type = VPLConv_ntoh_u16(contents[i + offset].type);
                outContentInfos[i].size = VPLConv_ntoh_u64(contents[i + offset].size);
            }
            *outNumContentInfos = i;
        }
    }
    return TMDV_ERR_OK;
}

int tmdv_findContentInfos(const void *_tmd, u32 tmdSize, ESContentIndex indexes[], u32 nIndexes, ESContentInfo outContentInfos[], u32 *outNumContentInfos)
{
    if (_tmd == NULL || indexes == NULL || outContentInfos == NULL || outNumContentInfos == NULL)
        return TMDV_ERR_BAD_ARG;
    if (!is_good_tmd_size(_tmd, tmdSize))
        return TMDV_ERR_BAD_TMD_SIZE;

    {
        ESV1TitleMeta *tmd = (ESV1TitleMeta*)_tmd;
        ESV1ContentMeta *contents = (ESV1ContentMeta*)&tmd[1];

        int i = 0;  // index into contents[]
        int j = 0;  // index into indexes[]
        u32 k = 0;  // index into outContentInfos[]
        while (i < tmd->head.numContents && j < nIndexes && k < *outNumContentInfos) {
            if (VPLConv_ntoh_u16(contents[i].index) == indexes[j]) {
                outContentInfos[k].id = VPLConv_ntoh_u32(contents[i].cid);
                outContentInfos[k].index = VPLConv_ntoh_u16(contents[i].index);
                outContentInfos[k].type = VPLConv_ntoh_u16(contents[i].type);
                outContentInfos[k].size = VPLConv_ntoh_u64(contents[i].size);
                i++; j++; k++;
            }
            else if (VPLConv_ntoh_u16(contents[i].index) > indexes[j]) {
                j++;
            }
            else {  // contents[i].index < indexes[j]
                i++;
            }
        }
        *outNumContentInfos = k;
    }

    return TMDV_ERR_OK;
}


