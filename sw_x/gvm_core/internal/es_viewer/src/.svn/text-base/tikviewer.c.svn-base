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
#include "tikviewer.h"

#include "esitypes.h"
#include "vpl_conv.h"
#include <string.h>
#include <stddef.h>

#define MAY_HAVE_CERTS_APPENDED 1

static int is_good_tik_size(const void *_tik, u32 tikSize)
{
    if (tikSize < sizeof(ESTicket) + sizeof(ESV1TicketHeader))  // too short to be valid
        return 0;

    {
        ESTicket *tik = (ESTicket*)_tik;
        ESV1TicketHeader *v1h = (ESV1TicketHeader*)&tik[1];
        u32 expectedSize = sizeof(ESTicket) + VPLConv_ntoh_u32(v1h->ticketSize);
#ifdef MAY_HAVE_CERTS_APPENDED
        return tikSize >= expectedSize;
#else
        return tikSize == expectedSize;
#endif
    }
}

int tikv_getTicketId(const void *_tik, u32 tikSize, ESTicketId *outTicketId)
{
    if (_tik == NULL || outTicketId == NULL)
        return TIKV_ERR_BAD_ARG;
    if (!is_good_tik_size(_tik, tikSize))
        return TIKV_ERR_BAD_TIK_SIZE;

    {
        ESTicket *tik = (ESTicket*)_tik;
        *outTicketId = VPLConv_ntoh_u64(tik->ticketId);
    }
    return TIKV_ERR_OK;
}

int tikv_getTicketVersion(const void *_tik, u32 tikSize, ESTicketVersion *outTicketVersion)
{
    if (_tik == NULL || outTicketVersion == NULL)
        return TIKV_ERR_BAD_ARG;
    if (!is_good_tik_size(_tik, tikSize))
        return TIKV_ERR_BAD_TIK_SIZE;

    {
        ESTicket *tik = (ESTicket*)_tik;
        *outTicketVersion = VPLConv_ntoh_u16(tik->ticketVersion);
    }
    return TIKV_ERR_OK;
}

int tikv_getTitleId(const void *_tik, u32 tikSize, ESTitleId *outTitleId)
{
    if (_tik == NULL || outTitleId == NULL)
        return TIKV_ERR_BAD_ARG;
    if (!is_good_tik_size(_tik, tikSize))
        return TIKV_ERR_BAD_TIK_SIZE;

    {
        ESTicket *tik = (ESTicket*)_tik;
        *outTitleId = VPLConv_ntoh_u64(tik->titleId);
    }
    return TIKV_ERR_OK;
}

int tikv_getDeviceId(const void *_tik, u32 tikSize, ESDeviceId *outDeviceId)
{
    if (_tik == NULL || outDeviceId == NULL)
        return TIKV_ERR_BAD_ARG;
    if (!is_good_tik_size(_tik, tikSize))
        return TIKV_ERR_BAD_TIK_SIZE;

    {
        ESTicket *tik = (ESTicket*)_tik;
        *outDeviceId = VPLConv_ntoh_u32(tik->deviceId);
    }
    return TIKV_ERR_OK;
}

int tikv_getPreInstallationFlag(const void *_tik, u32 tikSize, int *outFlag)
{
    if (_tik == NULL || outFlag == NULL)
        return TIKV_ERR_BAD_ARG;
    if (!is_good_tik_size(_tik, tikSize))
        return TIKV_ERR_BAD_TIK_SIZE;

    {
        ESTicket *tik = (ESTicket*)_tik;
        *outFlag = (VPLConv_ntoh_u16(tik->propertyMask) & ES_TICKET_PRE_INSTALL_FLAG) != 0;
    }
    return TIKV_ERR_OK;
}

int tikv_getLicenseType(const void *_tik, u32 tikSize, ESLicenseType *outLicenseType)
{
    if (_tik == NULL || outLicenseType == NULL)
        return TIKV_ERR_BAD_ARG;
    if (!is_good_tik_size(_tik, tikSize))
        return TIKV_ERR_BAD_TIK_SIZE;

    {
        ESTicket *tik = (ESTicket*)_tik;
        *outLicenseType = tik->licenseType;
    }
    return TIKV_ERR_OK;
}

int tikv_getSysAccessMask(const void *_tik, u32 tikSize, ESSysAccessMask outSysAccessMask)
{
    if (_tik == NULL || outSysAccessMask == NULL)
        return TIKV_ERR_BAD_ARG;
    if (!is_good_tik_size(_tik, tikSize))
        return TIKV_ERR_BAD_TIK_SIZE;

    {
        ESTicket *tik = (ESTicket*)_tik;
        memcpy(outSysAccessMask, tik->sysAccessMask, sizeof(ESSysAccessMask));
    }
    return TIKV_ERR_OK;
}

int tikv_getLimits(const void *_tik, u32 tikSize, ESLpEntry *outLimits, u32 *outNumLimits)
{
    if (_tik == NULL || outLimits == NULL || outNumLimits == NULL)
        return TIKV_ERR_BAD_ARG;
    if (!is_good_tik_size(_tik, tikSize))
        return TIKV_ERR_BAD_TIK_SIZE;

    {
        ESTicket *tik = (ESTicket*)_tik;
        int i;  // index into tik->limit[]s
        u32 j = 0;  // index into outLimits
        for (i = 0; i < ES_MAX_LIMIT_TYPE; i++) {
            if (tik->limits[i].code != 0) {
                outLimits[j].code = VPLConv_ntoh_u32(tik->limits[i].code);
                outLimits[j].limit = VPLConv_ntoh_u32(tik->limits[i].limit);
                j++;
            }
        }
        *outNumLimits = j;
    }
    return TIKV_ERR_OK;
}

static int getSubscriptionItemRights(void *_records, u32 numRecords, 
                                     u32 offset, 
                                     void *_outItemRights, u32 *outNumItemRights, 
                                     u32 *outTotal)
{
    ESV1SubscriptionRecord *records = (ESV1SubscriptionRecord*)_records;
    ESSubscriptionItemRight *outItemRights = (ESSubscriptionItemRight*)_outItemRights;

    if (outTotal != NULL)
        *outTotal = numRecords;

    if (outItemRights != NULL && outNumItemRights != NULL) {
        u32 i;  // index into outItemRights[]
        for (i = 0; i < *outNumItemRights; i++) {
            if (i + offset >= numRecords)  // no more records in section
                break;
            outItemRights[i].limit = VPLConv_ntoh_u32(records[i + offset].limit);
            memcpy(outItemRights[i].referenceId, records[i + offset].referenceId, ES_REFERENCE_ID_LEN);
            outItemRights[i].referenceIdAttr = VPLConv_ntoh_u32(records[i + offset].referenceIdAttr);
        }
        *outNumItemRights = i;
    }

    return TIKV_ERR_OK;
}

static int getPermanentItemRights(void *_records, u32 numRecords, 
                                  u32 offset, 
                                  void *_outItemRights, u32 *outNumItemRights, 
                                  u32 *outTotal)
{
    ESV1PermanentRecord *records = (ESV1PermanentRecord*)_records;
    ESPermanentItemRight *outItemRights = (ESPermanentItemRight*)_outItemRights;

    if (outTotal != NULL)
        *outTotal = numRecords;

    if (outItemRights != NULL && outNumItemRights != NULL) {
        u32 i;  // index into outItemRights[]
        for (i = 0; i < *outNumItemRights; i++) {
            if (i + offset >= numRecords)  // no more records in section
                break;
            memcpy(outItemRights[i].referenceId, records[i + offset].referenceId, ES_REFERENCE_ID_LEN);
            outItemRights[i].referenceIdAttr = VPLConv_ntoh_u32(records[i + offset].referenceIdAttr);
        }
        *outNumItemRights = i;
    }

    return TIKV_ERR_OK;
}

static int getContentItemRights(void *_records, u32 numRecords, 
                                u32 offset, 
                                void *_outItemRights, u32 *outNumItemRights, 
                                u32 *outTotal)
{
    ESV1ContentRecord *records = (ESV1ContentRecord*)_records;
    ESContentItemRight *outItemRights = (ESContentItemRight*)_outItemRights;

    if (outTotal != NULL)
        *outTotal = numRecords;

    if (outItemRights != NULL && outNumItemRights != NULL) {
        u32 i;  // index into outItemRights[]
        for (i = 0; i < *outNumItemRights; i++) {
            if (i + offset >= numRecords)  // no more records in section
                break;
            // This is an oddity in the spec; it's a 32-bit field, but only 16-bits are meaningful.
            outItemRights[i].offset = (u16)(VPLConv_ntoh_u32(records[i + offset].offset));
            memcpy(outItemRights[i].accessMask, records[i + offset].accessMask, ES_CONTENT_ITEM_ACCESS_MASK_LEN);
        }
        *outNumItemRights = i;
    }

    return TIKV_ERR_OK;
}

static int getContentConsumptionItemRights(void *_records, u32 numRecords, 
                                           u32 offset, 
                                           void *_outItemRights, u32 *outNumItemRights, 
                                           u32 *outTotal)
{
    ESV1ContentConsumptionRecord *records = (ESV1ContentConsumptionRecord*)_records;
    ESContentConsumptionItemRight *outItemRights = (ESContentConsumptionItemRight*)_outItemRights;

    if (outTotal != NULL)
        *outTotal = numRecords;

    if (outItemRights != NULL && outNumItemRights != NULL) {
        u32 i;  // index into outItemRights[]
        for (i = 0; i < *outNumItemRights; i++) {
            if (i + offset >= numRecords)  // no more records in section
                break;
            outItemRights[i].index = VPLConv_ntoh_u16(records[i + offset].index);
            outItemRights[i].code = VPLConv_ntoh_u16(records[i + offset].code);
            outItemRights[i].limit = VPLConv_ntoh_u32(records[i + offset].limit);
        }
        *outNumItemRights = i;
    }

    return TIKV_ERR_OK;
}

static int getAccessTitleItemRights(void *_records, u32 numRecords, 
                                    u32 offset, 
                                    void *_outItemRights, u32 *outNumItemRights, 
                                    u32 *outTotal)
{
    ESV1AccessTitleRecord *records = (ESV1AccessTitleRecord*)_records;
    ESAccessTitleItemRight *outItemRights = (ESAccessTitleItemRight*)_outItemRights;

    if (outTotal != NULL)
        *outTotal = numRecords;

    if (outItemRights != NULL && outNumItemRights != NULL) {
        u32 i;  // index into outItemRights[]
        for (i = 0; i < *outNumItemRights; i++) {
            if (i + offset >= numRecords)  // no more records in section
                break;
            outItemRights[i].accessTitleId = VPLConv_ntoh_u64(records[i + offset].accessTitleId);
            outItemRights[i].accessTitleMask = VPLConv_ntoh_u64(records[i + offset].accessTitleMask);
        }
        *outNumItemRights = i;
    }

    return TIKV_ERR_OK;
}

static struct {
    ESItemType type;
    int (*handler)(void*, u32, u32, void*, u32*, u32*);
} itemRightsHandler[] ={
    { ES_ITEM_RIGHT_SUBSCRIPTION, getSubscriptionItemRights },
    { ES_ITEM_RIGHT_PERMANENT, getPermanentItemRights },
    { ES_ITEM_RIGHT_CONTENT, getContentItemRights },
    { ES_ITEM_RIGHT_CONTENT_CONSUMPTION, getContentConsumptionItemRights },
    { ES_ITEM_RIGHT_ACCESS_TITLE, getAccessTitleItemRights },
};

static int (*getItemRightsHandler(ESItemType type))(void*, u32, u32, void*, u32*, u32*)
{
    int i;
    for (i = 0; i < sizeof(itemRightsHandler)/sizeof(itemRightsHandler[0]); i++) {
        if (itemRightsHandler[i].type == type)
            return itemRightsHandler[i].handler;
    }
    return NULL;
}

int tikv_getItemRights(const void *_tik, u32 tikSize, 
                       ESItemType type, u32 offset, 
                       void *outItemRights, u32 *outNumItemRights, u32 *outTotal)
{
    if (_tik == NULL)
        return TIKV_ERR_BAD_ARG;
    if (!is_good_tik_size(_tik, tikSize))
        return TIKV_ERR_BAD_TIK_SIZE;
    if ((outItemRights == NULL || outNumItemRights == NULL) &&
        outTotal == NULL)
        return TIKV_ERR_BAD_ARG;
    switch (type) {
    case ES_ITEM_RIGHT_SUBSCRIPTION:
    case ES_ITEM_RIGHT_PERMANENT:
    case ES_ITEM_RIGHT_CONTENT:
    case ES_ITEM_RIGHT_CONTENT_CONSUMPTION:
    case ES_ITEM_RIGHT_ACCESS_TITLE:
        break;
    default:
        return TIKV_ERR_INVALID_ITEMRIGHT_TYPE;
    }

    {
        ESTicket *tik = (ESTicket*)_tik;
        ESV1TicketHeader *v1Hdr = (ESV1TicketHeader*)&tik[1];
        u32 v1SectHdrOffset = VPLConv_ntoh_u32(v1Hdr->sectHdrOfst);
        u16 v1SectHdrNumEntries = VPLConv_ntoh_u16(v1Hdr->nSectHdrs);
        ESV1SectionHeader *v1SectHeader = (ESV1SectionHeader*)((u8*)v1Hdr + v1SectHdrOffset);

        int i;
        for (i = 0; i < v1SectHdrNumEntries; i++) {
            if (VPLConv_ntoh_u16(v1SectHeader[i].sectionType) == type) {
                int (*handler)(void*, u32, u32, void*, u32*, u32*) = getItemRightsHandler(type);
                if (handler == NULL)
                    return TIKV_ERR_INVALID_ITEMRIGHT_TYPE;
                return handler((u8*)v1Hdr + VPLConv_ntoh_u32(v1SectHeader[i].sectOfst),
                               VPLConv_ntoh_u32(v1SectHeader[i].nRecords),
                               offset,
                               outItemRights, 
                               outNumItemRights, 
                               outTotal);
            }
        }
    }

    // if you got here, no record of requested type was found
    if (outNumItemRights != NULL)
        *outNumItemRights = 0;
    if (outTotal != NULL)
        *outTotal = 0;

    return TIKV_ERR_OK;
}

