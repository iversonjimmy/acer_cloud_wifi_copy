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

#if defined(GVM) && !defined(CPP_ES)
#include <es.hpp>
#endif

USING_ES_NAMESPACE
USING_ISTORAGE_NAMESPACE

#include "es_container.h"
#include "es_storage.h"

#ifndef UNUSED
#define UNUSED(x)   (void)(x)
#endif

ES_NAMESPACE_START


static bool __initialized = false;

static IInputStream *__ticket;

static u8 __buf[SIZE_SHA_ALIGN(sizeof(ESTicket) + sizeof(ESV1TicketHeader))]
                ATTR_SHA_ALIGN;
static ESTicket * const __tp = (ESTicket *) __buf;

static const void **__certs = NULL;
static u32 __nCerts = 0;

static bool __doVerify = true;

// Buffer for data manipulation
static u8 __dataBuf[SIZE_SHA_ALIGN(1024)] ATTR_SHA_ALIGN;


ETicket::ETicket()
{
#if !defined(ES_READ_ONLY)
    IOSC_Initialize();
#endif  // ES_READ_ONLY
}


ETicket::~ETicket()
{
    __initialized = false;
}


ESError
ETicket::Set(IInputStream &ticket, const void *certs[], u32 nCerts,
             bool doVerify)
{
    ESError rv = ES_ERR_OK;

#if defined(ES_READ_ONLY)
    if (doVerify) {
        esLog(ES_DEBUG_ERROR, "Cannot verify ticket in read-only mode\n");
        rv = ES_ERR_INVALID;
        goto end;
    }
#endif  // ES_READ_ONLY

    if (doVerify && (!certs || nCerts == 0)) {
        esLog(ES_DEBUG_ERROR, "Invalid arguments\n");
        rv = ES_ERR_INVALID;
        goto end;
    }

    if ((rv = esVerifyTicket(ticket, certs, nCerts, doVerify, NULL, NULL, __buf)) != ES_ERR_OK) {
        goto end;
    }

    __ticket = &ticket;
    __certs = certs;
    __nCerts = nCerts;
    __doVerify = doVerify;
    __initialized = true;
    
end:
    return rv;
}


ESError
ETicket::GetSize(u32 *outSize)
{
    ESError rv = ES_ERR_OK;
    ESV1TicketHeader *v1;

    if (outSize == NULL || !__initialized) {
        esLog(ES_DEBUG_ERROR, "Invalid arguments or state\n");
        rv = ES_ERR_INVALID;
        goto end;
    }

    v1 = (ESV1TicketHeader *) &__tp[1];
    *outSize = sizeof(ESTicket) + ntohl(v1->ticketSize);

end:
    return rv;
}


ESError
ETicket::GetTicketId(ESTicketId *outTicketId)
{
    ESError rv = ES_ERR_OK;

    if (outTicketId == NULL || !__initialized) {
        esLog(ES_DEBUG_ERROR, "Invalid arguments or state\n");
        rv = ES_ERR_INVALID;
        goto end;
    }

    *outTicketId = ntohll(__tp->ticketId);

end:
    return rv;
}


ESError
ETicket::GetTicketVersion(ESTicketVersion *outTicketVersion)
{
    ESError rv = ES_ERR_OK;

    if (outTicketVersion == NULL || !__initialized) {
        esLog(ES_DEBUG_ERROR, "Invalid arguments or state\n");
        rv = ES_ERR_INVALID;
        goto end;
    }

    *outTicketVersion = ntohs(__tp->ticketVersion);

end:
    return rv;
}


ESError
ETicket::GetTitleId(ESTitleId *outTitleId)
{
    ESError rv = ES_ERR_OK;

    if (outTitleId == NULL || !__initialized) {
        esLog(ES_DEBUG_ERROR, "Invalid arguments or state\n");
        rv = ES_ERR_INVALID;
        goto end;
    }

    *outTitleId = ntohll(__tp->titleId);

end:
    return rv;
}


ESError
ETicket::GetDeviceId(ESDeviceId *outDeviceId)
{
    ESError rv = ES_ERR_OK;

    if (outDeviceId == NULL || !__initialized) {
        esLog(ES_DEBUG_ERROR, "Invalid arguments or state\n");
        rv = ES_ERR_INVALID;
        goto end;
    }

    *outDeviceId = ntohl(__tp->deviceId);

end:
    return rv;
}


ESError
ETicket::GetPreInstallationFlag(bool *outFlag)
{
    ESError rv = ES_ERR_OK;

    if (outFlag == NULL || !__initialized) {
        esLog(ES_DEBUG_ERROR, "Invalid arguments or state\n");
        rv = ES_ERR_INVALID;
        goto end;
    }

    *outFlag = (ntohs(__tp->propertyMask) & ES_TICKET_PRE_INSTALL_FLAG) != 0;

end:
    return rv;
}


ESError
ETicket::GetLicenseType(ESLicenseType *outLicenseType)
{
    ESError rv = ES_ERR_OK;

    if (outLicenseType == NULL || !__initialized) {
        esLog(ES_DEBUG_ERROR, "Invalid arguments or state\n");
        rv = ES_ERR_INVALID;
        goto end;
    }

    *outLicenseType = __tp->licenseType;

end:
    return rv;
}


ESError
ETicket::GetSysAccessMask(ESSysAccessMask *outSysAccessMask)
{
    ESError rv = ES_ERR_OK;

    if (outSysAccessMask == NULL || !__initialized) {
        esLog(ES_DEBUG_ERROR, "Invalid arguments or state\n");
        rv = ES_ERR_INVALID;
        goto end;
    }

    memcpy(outSysAccessMask, &__tp->sysAccessMask, sizeof(ESSysAccessMask));

end:
    return rv;
}


ESError
ETicket::GetLimits(ESLpEntry *outLimits, u32 *outNumLimits)
{
    ESError rv = ES_ERR_OK;
    ESLpEntry *limits;
    u32 i, j;

    if (outLimits == NULL || outNumLimits == NULL || !__initialized) {
        esLog(ES_DEBUG_ERROR, "Invalid arguments or state\n");
        rv = ES_ERR_INVALID;
        goto end;
    }

    limits = __tp->limits;
    for (i = 0, j = 0; i < ES_MAX_LIMIT_TYPE; i++) {
        if (limits[i].code != 0) {
            outLimits[j].code = ntohl(limits[i].code);
            outLimits[j].limit= ntohl(limits[i].limit);

            j++;
        }
    }

    *outNumLimits = j;

end:
    return rv;
}


ESError
ETicket::GetItemRights(ESItemType type, u32 offset,
                       void *outItemRights, u32 *outNumItemRights,
                       u32 *outTotal)
{
    ESError rv = ES_ERR_OK;
    ESV1TicketHeader *v1;
    ESV1SectionHeader *sectHdrPtr;
    ESItemRightsGet itemRightsGet;
    u32 i, pos, size, itemSize, maxOutNumItemRights;

    if (outItemRights == NULL || outNumItemRights == NULL || *outNumItemRights == 0 || outTotal == NULL || !__initialized) {
        esLog(ES_DEBUG_ERROR, "Invalid arguments or state\n");
        rv = ES_ERR_INVALID;
        goto end;
    }

    switch (type) {
        case ES_ITEM_RIGHT_PERMANENT:
            itemSize = sizeof(ESV1PermanentRecord);
            break;

        case ES_ITEM_RIGHT_SUBSCRIPTION:
            itemSize = sizeof(ESV1SubscriptionRecord);
            break;

        case ES_ITEM_RIGHT_CONTENT:
            itemSize = sizeof(ESV1ContentRecord);
            break;

        case ES_ITEM_RIGHT_CONTENT_CONSUMPTION:
            itemSize = sizeof(ESV1ContentConsumptionRecord);
            break;

        case ES_ITEM_RIGHT_ACCESS_TITLE:
            itemSize = sizeof(ESV1AccessTitleRecord);
            break;

        default:
            esLog(ES_DEBUG_ERROR, "Unrecognized item type, %u\n", type);
            rv = ES_ERR_INVALID;
            goto end;
    }

    /*
     * Reading and verifying the ticket rights at the same time is tough
     * since reading requires random access and verifying requires
     * sequential access.  The strategy here is to find out the locations
     * of the needed data without verification at first, then verify and
     * interpret the data at the remembered locations simultaneously
     */

    // Initialize the structure used for sequential verification
    itemRightsGet.itemType = type;
    itemRightsGet.itemSize = itemSize;
    itemRightsGet.sectHdrs = NULL;
    itemRightsGet.outRights = NULL;

    // Read the section headers
    v1 = (ESV1TicketHeader *) &__tp[1];
    pos = sizeof(ESTicket) + ntohl(v1->sectHdrOfst);
    if ((rv = esSeek(*__ticket, pos)) != ES_ERR_OK) {
        goto end;
    }

    size = ntohs(v1->nSectHdrs) * ntohs(v1->sectHdrEntrySize);
    if (size > sizeof(__dataBuf)) {
        esLog(ES_DEBUG_ERROR, "Too many section headers, size=%d\n", size);
        rv = ES_ERR_INVALID;
        goto end;
    }

    /*
     * WARNING: the section headers have NOT been verified.  They should
     * be used with extreme care and their values should not be trusted
     * until they are verified later
     */

    if ((rv = esRead(*__ticket, size, __dataBuf)) != ES_ERR_OK) {
        goto end;
    }
    sectHdrPtr = (ESV1SectionHeader *) __dataBuf;

    // Remember the section headers for verification later
    itemRightsGet.itemType = type;
    itemRightsGet.sectHdrsOffset = sizeof(ESTicket) + ntohl(v1->sectHdrOfst);
    itemRightsGet.sectHdrsSize = ntohs(v1->nSectHdrs) * ntohs(v1->sectHdrEntrySize);
    itemRightsGet.sectHdrs = sectHdrPtr;

    /*
     * Find the section header for the specified type.  For simplicity,
     * it's assumed that all the items of the same type are grouped
     * under the same section
     */
    for (i = 0; i < ntohs(v1->nSectHdrs); i++) {
        if (ntohs(sectHdrPtr[i].sectionType) != type) {
            continue;
        }

        // Make sure the section is not compressed
        if (ntohs(sectHdrPtr[i].flags) & ES_ETS_COMPRESSED) {
            esLog(ES_DEBUG_ERROR, "Ticket compression not supported\n");
            rv = ES_ERR_INVALID;
            goto end;
        }

        break;
    }
    if (i == ntohs(v1->nSectHdrs)) {
        *outTotal = 0;
        *outNumItemRights = 0;
        goto verify;
    }

    // Find the location of the actual ticket records
    *outTotal = ntohl(sectHdrPtr[i].nRecords);

    if (offset >= *outTotal) {
        *outNumItemRights = 0;
        goto verify;
    }

    if (offset + *outNumItemRights > *outTotal) {
        maxOutNumItemRights = *outNumItemRights;
        *outNumItemRights = *outTotal - offset;

        /*
         * Error check to avoid buffer overflow, but this case should
         * never happen in real life
         */
        if (*outNumItemRights > maxOutNumItemRights) {
            esLog(ES_DEBUG_ERROR, "Unexpected value computed for outNumItemRights %u\n", *outNumItemRights);
            rv = ES_ERR_FAIL;
            goto end;
        }
    }

    itemRightsGet.rightsOffset = sizeof(ESTicket) + ntohl(sectHdrPtr[i].sectOfst) + (offset * itemSize);
    itemRightsGet.nRights = *outNumItemRights;
    itemRightsGet.outRights = outItemRights;

verify:
    if ((rv = esSeek(*__ticket, 0)) != ES_ERR_OK) {
        goto end;
    }

    if ((rv = esVerifyTicket(*__ticket, __certs, __nCerts, __doVerify, NULL, &itemRightsGet, __buf)) != ES_ERR_OK) {
        goto end;
    }

end:
    return rv;
}


Result
ETicket::GetLastSystemResult()
{
    return esGetLastSystemResult();
}

ES_NAMESPACE_END
