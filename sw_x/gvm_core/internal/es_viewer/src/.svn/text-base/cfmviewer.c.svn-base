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

#include "cfmviewer.h"

#include "vpl_conv.h"
#include <string.h>
#include <stddef.h>

#define HASH_RECORD_SIZE 32
#define EXEC_RECORD_SIZE 1
#define ZERO_RECORD_SIZE 1

static int getHashSection(const void *_header, const void *_records,
                          u32 offset,
                          void *_outHeader, void *_outRecords, u32 *outNumRecords)
{
    CfmHashSectionHeader *header = (CfmHashSectionHeader*)_header;
    u8 *records = (u8*)_records;
    CfmHashSectionHeader *outHeader = (CfmHashSectionHeader*)_outHeader;
    u8 *outRecords = (u8*)_outRecords;

    u32 numRecords = VPLConv_ntoh_u32(header->numRecords);

    if (outHeader != NULL) {
        memcpy(outHeader, header, sizeof(CfmHashSectionHeader));
        outHeader->numRecords = numRecords;
    }

    if (outRecords != NULL && outNumRecords != NULL) {
        u32 numRecsToCopy;
        if (offset >= numRecords)
            numRecsToCopy = 0;
        else if (numRecords - offset > *outNumRecords)
            numRecsToCopy = *outNumRecords;
        else
            numRecsToCopy = numRecords - offset;
        if (numRecsToCopy > 0)
            memcpy(outRecords, records + offset * HASH_RECORD_SIZE, numRecsToCopy * HASH_RECORD_SIZE);
        *outNumRecords = numRecsToCopy;
    }

    return CFMV_ERR_OK;
}

static int getZeroSection(const void *_header, const void *_records, 
                          u32 offset,
                          void *_outHeader, void *_outRecords, u32 *outNumRecords)
{
    CfmZeroSectionHeader *header = (CfmZeroSectionHeader*)_header;
    u8 *records = (u8*)_records;
    CfmZeroSectionHeader *outHeader = (CfmZeroSectionHeader*)_outHeader;
    u8 *outRecords = (u8*)_outRecords;

    u32 numRecords = VPLConv_ntoh_u32(header->numRecords);

    if (outHeader != NULL) {
        memcpy(outHeader, header, sizeof(CfmZeroSectionHeader));
        outHeader->numRecords = numRecords;
    }

    if (outRecords != NULL && outNumRecords != NULL) {
        u32 numRecsToCopy;
        if (offset >= numRecords)
            numRecsToCopy = 0;
        else if (numRecords - offset > *outNumRecords)
            numRecsToCopy = *outNumRecords;
        else
            numRecsToCopy = numRecords - offset;
        if (numRecsToCopy > 0)
            memcpy(outRecords, records + offset * ZERO_RECORD_SIZE, numRecsToCopy * ZERO_RECORD_SIZE);
        *outNumRecords = numRecsToCopy;
    }

    return CFMV_ERR_OK;
}

static struct {
    CfmSections type;
    int (*handler)(const void*, const void*, u32, void*, void*, u32*);
} sectionHandler[] ={
    { CFM_HASH_SECTION, getHashSection },
    { CFM_ZERO_SECTION, getZeroSection },
};

static int (*getSectionHandler(CfmSections type))(const void*, const void*, u32, void*, void*, u32*)
{
    int i;
    for (i = 0; i < sizeof(sectionHandler)/sizeof(sectionHandler[0]); i++) {
        if (sectionHandler[i].type == type)
            return sectionHandler[i].handler;
    }
    return NULL;
}

int cfmv_getSection(const void *_cfm, u32 cfmSize, CfmSections type, u32 offset,
                    void *outHeader, void *outRecords, u32 *outNumRecords)
{
    CfmHeader *cfm;
    CfmSectionHeader *hdr;
    int i;
    int numSections;

    if (_cfm == NULL)
        return CFMV_ERR_BAD_ARG;
    if (outHeader == NULL && (outRecords == NULL || outNumRecords == NULL))
        return CFMV_ERR_BAD_ARG;
    switch (type) {
    case CFM_HASH_SECTION:
    case CFM_ZERO_SECTION:
        break;
    case CFM_DISC1_SECTION:
    case CFM_DISC2_SECTION:
    case CFM_NUM_SECTIONS:
    default:
        return CFMV_ERR_INVALID_SECTION_TYPE;
    }
    
    cfm = (CfmHeader*)_cfm;
    if (VPLConv_ntoh_u16(cfm->version) != 1)
        return CFMV_ERR_UNSUPPORTED_CFM_VERSION;
    numSections = cfm->numSections;
    hdr = (CfmSectionHeader*)((u8*)_cfm + VPLConv_ntoh_u32(cfm->sectionOffset));

    for (i = 0; i < numSections; i++) {
        if (hdr->sectionType == type) {
            int (*handler)(const void*, const void*, u32, void*, void*, u32*) = getSectionHandler(type);
            if (handler == NULL)
                return CFMV_ERR_INVALID_SECTION_TYPE;
            return handler((u8*)_cfm + VPLConv_ntoh_u32(hdr->payloadHeaderOffset),
                           (u8*)_cfm + VPLConv_ntoh_u32(hdr->payloadDataOffset),
                           offset, outHeader, outRecords, outNumRecords);
        }
        hdr = (CfmSectionHeader*)((u8*)_cfm + VPLConv_ntoh_u32(hdr->nextSectionOffset));
    }

    // if you got here, the specific section was not found
    if (outNumRecords != NULL)
        *outNumRecords = 0;

    return CFMV_ERR_OK;
}
