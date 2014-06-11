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


#include <es.h>
#include <iosc.h>

USING_ES_NAMESPACE
USING_ISTORAGE_NAMESPACE

#include "es_container.h"
#include "es_storage.h"

ES_NAMESPACE_START


static bool __initialized = false;

static IInputStream *__tmd;

static u8 __buf[SIZE_SHA_ALIGN(sizeof(ESV1TitleMeta))] ATTR_SHA_ALIGN;
static ESV1TitleMeta * const __mp = (ESV1TitleMeta *) __buf;

static bool __doVerify = false;


TitleMetaData::TitleMetaData()
{
#if !defined(ES_READ_ONLY)
    IOSC_Initialize();
#endif  // ES_READ_ONLY
}


TitleMetaData::~TitleMetaData()
{
    __initialized = false;
}


ESError
TitleMetaData::Set(IInputStream &tmd, const void *certs[], u32 nCerts,
                   bool doVerify)
{
    ESError rv = ES_ERR_OK;

#if defined(ES_READ_ONLY)
    if (doVerify) {
        esLog(ES_DEBUG_ERROR, "Cannot verify TMD in read-only mode\n");
        rv = ES_ERR_INVALID;
        goto end;
    }
#endif  // ES_READ_ONLY

    if (doVerify && (!certs || nCerts == 0)) {
        esLog(ES_DEBUG_ERROR, "Invalid arguments\n");
        rv = ES_ERR_INVALID;
        goto end;
    }

    if (doVerify) {
        if ((rv = esVerifyTmdFixed(tmd, certs, nCerts, __mp)) != ES_ERR_OK) {
            goto end;
        }
    } else {
        if ((rv = esSeek(tmd, 0)) != ES_ERR_OK) {
            goto end;
        }

        if ((rv = esRead(tmd, sizeof(ESV1TitleMeta), __mp)) != ES_ERR_OK) {
            goto end;
        }
    }

    __tmd = &tmd;
    __doVerify = doVerify;
    __initialized = true;

end:
    return rv;
}


ESError
TitleMetaData::GetSize(u32 *outSize)
{
    ESError rv = ES_ERR_OK;

    if (outSize == NULL || !__initialized) {
        esLog(ES_DEBUG_ERROR, "Invalid arguments or state\n");
        rv = ES_ERR_INVALID;
        goto end;
    }

    *outSize = sizeof(ESV1TitleMeta) + (sizeof(ESV1ContentMeta) * ntohs(__mp->head.numContents));

end:
    return rv;
}


ESError
TitleMetaData::GetTitleId(ESTitleId *outTitleId)
{
    ESError rv = ES_ERR_OK;

    if (outTitleId == NULL || !__initialized) {
        esLog(ES_DEBUG_ERROR, "Invalid arguments or state\n");
        rv = ES_ERR_INVALID;
        goto end;
    }

    *outTitleId = ntohll(__mp->head.titleId);

end:
    return rv;
}


ESError
TitleMetaData::GetTitleVersion(ESTitleVersion *outTitleVersion)
{
    ESError rv = ES_ERR_OK;

    if (outTitleVersion == NULL || !__initialized) {
        esLog(ES_DEBUG_ERROR, "Invalid arguments or state\n");
        rv = ES_ERR_INVALID;
        goto end;
    }

    *outTitleVersion = ntohs(__mp->head.titleVersion);

end:
    return rv;
}


ESError
TitleMetaData::GetMinorTitleVersion(ESMinorTitleVersion *outMinorTitleVersion)
{
    ESError rv = ES_ERR_OK;

    if (outMinorTitleVersion == NULL || !__initialized) {
        esLog(ES_DEBUG_ERROR, "Invalid arguments or state\n");
        rv = ES_ERR_INVALID;
        goto end;
    }

    *outMinorTitleVersion = ntohs(__mp->head.minorTitleVersion);

end:
    return rv;
}


ESError
TitleMetaData::GetSysVersion(ESSysVersion *outSysVersion)
{
    ESError rv = ES_ERR_OK;

    if (outSysVersion == NULL || !__initialized) {
        esLog(ES_DEBUG_ERROR, "Invalid arguments or state\n");
        rv = ES_ERR_INVALID;
        goto end;
    }

    *outSysVersion = ntohll(__mp->head.sysVersion);

end:
    return rv;
}


ESError
TitleMetaData::GetTitleType(ESTitleType *outTitleType)
{
    ESError rv = ES_ERR_OK;

    if (outTitleType == NULL || !__initialized) {
        esLog(ES_DEBUG_ERROR, "Invalid arguments or state\n");
        rv = ES_ERR_INVALID;
        goto end;
    }

    *outTitleType = ntohl(__mp->head.type);

end:
    return rv;
}


ESError
TitleMetaData::GetGroupId(u16 *outGroupId)
{
    ESError rv = ES_ERR_OK;

    if (outGroupId == NULL || !__initialized) {
        esLog(ES_DEBUG_ERROR, "Invalid arguments or state\n");
        rv = ES_ERR_INVALID;
        goto end;
    }

    *outGroupId = ntohs(__mp->head.groupId);

end:
    return rv;
}


ESError
TitleMetaData::GetAccessRights(u32 *outAccessRights)
{
    ESError rv = ES_ERR_OK;

    if (outAccessRights == NULL || !__initialized) {
        esLog(ES_DEBUG_ERROR, "Invalid arguments or state\n");
        rv = ES_ERR_INVALID;
        goto end;
    }

    *outAccessRights = ntohl(__mp->head.accessRights);

end:
    return rv;
}


ESError
TitleMetaData::GetBootContentIndex(ESContentIndex *outContentIndex)
{
    ESError rv = ES_ERR_OK;

    if (outContentIndex == NULL || !__initialized) {
        esLog(ES_DEBUG_ERROR, "Invalid arguments or state\n");
        rv = ES_ERR_INVALID;
        goto end;
    }

    *outContentIndex = ntohs(__mp->head.bootIndex);

end:
    return rv;
}


ESError
TitleMetaData::GetCustomData(u8 *outCustomData, u32 *outCustomDataLen)
{
    ESError rv = ES_ERR_OK;
    u32 copyLen;

    if (outCustomDataLen == NULL || !__initialized || ES_TMD_CUSTOM_DATA_LEN != sizeof(ESTmdCustomData)) {
        esLog(ES_DEBUG_ERROR, "Invalid arguments or state\n");
        rv = ES_ERR_INVALID;
        goto end;
    }

    if (outCustomData == NULL) {
        *outCustomDataLen = ES_TMD_CUSTOM_DATA_LEN;
        goto end;
    }

    copyLen = *outCustomDataLen;
    if (copyLen > ES_TMD_CUSTOM_DATA_LEN) {
        copyLen = ES_TMD_CUSTOM_DATA_LEN;
    }

    memcpy(outCustomData, __mp->head.customData, copyLen);
    *outCustomDataLen = copyLen;

end:
    return rv;
}


ESError
TitleMetaData::GetContentInfos(u32 offset, ESContentInfo *outContentInfos,
                               u32 *outNumContentInfos, u32 *outTotal)
{
    ESError rv = ES_ERR_OK;
    ESContentMetaGet cmdGet;
    ESV1ContentMetaGroup *cmdGroup;
    u32 i, nCopied = 0, maxOutNumContentInfos;

    if (outContentInfos == NULL || outNumContentInfos == NULL || *outNumContentInfos == 0 || outTotal == NULL || !__initialized) {
        esLog(ES_DEBUG_ERROR, "Invalid arguments or state\n");
        rv = ES_ERR_INVALID;
        goto end;
    }

    // Check the number of contents against the input parameters
    *outTotal = ntohs(__mp->head.numContents);

    if (offset >= *outTotal) {
        // Done
        *outNumContentInfos = 0;
        goto end;
    }

    if (offset + *outNumContentInfos > *outTotal) {
        maxOutNumContentInfos = *outNumContentInfos;
        *outNumContentInfos = *outTotal - offset;

        /*
         * Error check to avoid buffer overflow, but this case should
         * never happen in real life
         */
        if (*outNumContentInfos > maxOutNumContentInfos) {
            esLog(ES_DEBUG_ERROR, "Unexpected value computed for outNumContentInfos %u\n", *outNumContentInfos);
            rv = ES_ERR_FAIL;
            goto end;
        }
    }

    for (i = 0; i < ES_MAX_CMD_GROUPS; i++) {
        if (nCopied >= *outNumContentInfos) {
            break;
        }

        // Copy the requested content info from each CMD group
        cmdGroup = &__mp->v1Head.cmdGroups[i];
        if (offset >= ntohs(cmdGroup->nCmds)) {
            offset -= ntohs(cmdGroup->nCmds);
            continue;
        }

        cmdGet.offset = offset;
        if (nCopied + ntohs(cmdGroup->nCmds) - offset > *outNumContentInfos) {
            cmdGet.nInfos = *outNumContentInfos - nCopied;
        } else {
            cmdGet.nInfos = ntohs(cmdGroup->nCmds) - offset;

            /*
             * Error check to avoid buffer overflow, but this case should
             * never happen in real life
             */
            if (cmdGet.nInfos > (*outNumContentInfos - nCopied)) {
                esLog(ES_DEBUG_ERROR, "Unexpected value computed for nInfos %u\n", cmdGet.nInfos);
                rv = ES_ERR_FAIL;
                goto end;
            }
        }
        cmdGet.outInfos = &outContentInfos[nCopied];

        if ((rv = esVerifyCmdGroup(*__tmd, __mp->v1Head.cmdGroups, i, __doVerify, NULL, NULL, &cmdGet)) != ES_ERR_OK) {
            goto end;
        }

        nCopied += cmdGet.nInfos;
        offset = 0;     // After the first group, always start from offset 0
    }

end:
    return rv;
}


ESError
TitleMetaData::FindContentInfos(ESContentIndex *indexes, u32 nIndexes,
                                ESContentInfo *outContentInfos)
{
    ESError rv = ES_ERR_OK;
    ESV1TitleMetaHeader *v1Head;
    ESContentMetaSearchByIndex cmdSearchByIndex;
    u32 i, j, nFound = 0;

    if (indexes == NULL || nIndexes == 0 || outContentInfos == NULL || !__initialized) {
        esLog(ES_DEBUG_ERROR, "Invalid arguments or state\n");
        rv = ES_ERR_INVALID;
        goto end;
    }

    v1Head = &__mp->v1Head;
    for (i = 0; i < ES_MAX_CMD_GROUPS; i++) {
        if (ntohs(v1Head->cmdGroups[i].nCmds == 0)) {
            continue;
        }

        // Check if any of the specified indexes are in this group
        for (j = 0; j < nIndexes; j++) {
            if (ntohs(v1Head->cmdGroups[i].offset) > indexes[j]) {
                continue;
            }

            if (i == (ES_MAX_CMD_GROUPS - 1)) {
                break;
            }

            if (ntohs(v1Head->cmdGroups[i + 1].nCmds) != 0 && ntohs(v1Head->cmdGroups[i + 1].offset) <= indexes[j]) {
                continue;
            }

            break;
        }

        if (j == nIndexes) {
            continue;
        }

        // At least one of the specified indexes is in this group 
        cmdSearchByIndex.indexes = indexes;
        cmdSearchByIndex.nIndexes = nIndexes;
        cmdSearchByIndex.outInfos = outContentInfos;

        if ((rv = esVerifyCmdGroup(*__tmd, v1Head->cmdGroups, i, __doVerify, &cmdSearchByIndex, NULL, NULL)) != ES_ERR_OK) {
            goto end;
        }

        nFound += cmdSearchByIndex.nFound;
        if (nFound == nIndexes) {
            break;
        }
    }

    if (nFound != nIndexes) {
        esLog(ES_DEBUG_ERROR, "Failed to find all specified content indexes\n");
        rv = ES_ERR_INVALID;
        goto end;
    }

end:
    return rv;
}


Result
TitleMetaData::GetLastSystemResult()
{
    return esGetLastSystemResult();
}

ES_NAMESPACE_END
