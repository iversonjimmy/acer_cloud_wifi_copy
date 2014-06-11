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

#include "vpl_string.h"
#include <estypes.h>
#include <istorage.h>
#include <iosc.h>
#include "cpp_c.h"
#ifdef _KERNEL_MODULE
#include "core_glue.h"
#define esLog(l,...) (void)(0)
#endif
// following is def of ES_LOG_ERROR is a hack
#define ES_LOG_ERROR (0)

USING_ES_NAMESPACE
USING_ISTORAGE_NAMESPACE

#include "es_container.h"
#include "es_storage.h"


ES_NAMESPACE_START


// Values for sanity checks
#define ES_ABSOLUTE_MAX_TICKET_SIZE     (128 * 1024 * 1024)
#define ES_ABSOLUTE_MAX_RIGHTS          (1 * 1024 * 1024)


// Context info for the __getItemRights function
typedef struct {
    u32     nCopied;
    u32     leftOverBytes;
    union {
        ESV1PermanentRecord             perm;
        ESV1SubscriptionRecord          sub;
        ESV1ContentRecord               cnt;
        ESV1ContentConsumptionRecord    cntLimit;
        ESV1AccessTitleRecord           accTitle;
    } record;
} ESGetItemRightsContext;


// Buffer used for data manipulation
static u8 __buf[4 * 1024] ATTR_SHA256_ALIGN;

static ESError
__decodeItemRight(void *src, ESItemType type, void *dst, u32 nOffset)
{
    ESError rv = ES_ERR_OK;
    u8 *rp;
    ESPermanentItemRight *perm;
    ESSubscriptionItemRight *sub;
    ESContentItemRight *cnt;
    ESContentConsumptionItemRight *cntLmt;
    ESAccessTitleItemRight *accTitle;

    rp = (u8 *) src;

    switch (type) {
    case ES_ITEM_RIGHT_PERMANENT:
        perm = (ESPermanentItemRight *) dst;
        perm += nOffset;

        memcpy(perm->referenceId, rp, ES_REFERENCE_ID_LEN);
        rp += ES_REFERENCE_ID_LEN;
        perm->referenceIdAttr = ntohl(*((u32 *) rp));

        break;

    case ES_ITEM_RIGHT_SUBSCRIPTION:
        sub = (ESSubscriptionItemRight *) dst;
        sub += nOffset;

        sub->limit = ntohl(*((u32 *) rp));
        rp += sizeof(u32);
        memcpy(sub->referenceId, rp, ES_REFERENCE_ID_LEN);
        rp += ES_REFERENCE_ID_LEN;
        sub->referenceIdAttr = ntohl(*((u32 *) rp));

        break;

    case ES_ITEM_RIGHT_CONTENT:
        cnt = (ESContentItemRight *) dst;
        cnt += nOffset;

        cnt->offset = (ESContentIndex) ntohl(*((u32 *) rp));
        rp += sizeof(u32);
        memcpy(cnt->accessMask, rp, ES_CONTENT_ITEM_ACCESS_MASK_LEN);

        break;

    case ES_ITEM_RIGHT_CONTENT_CONSUMPTION:
        cntLmt = (ESContentConsumptionItemRight *) dst;
        cntLmt += nOffset;

        cntLmt->index = ntohs(*((ESContentIndex *) rp));
        rp += sizeof(ESContentIndex);
        cntLmt->code = (ESLimitCode) ntohs(*((u16 *) rp));
        rp += sizeof(u16);
        cntLmt->limit = ntohl(*((u32 *) rp));

        break;

    case ES_ITEM_RIGHT_ACCESS_TITLE:
        accTitle = (ESAccessTitleItemRight *) dst;
        accTitle += nOffset;

        accTitle->accessTitleId = ntohll(*((u64 *) rp));
        rp += sizeof(u64);
        accTitle->accessTitleMask = ntohll(*((u64 *) rp));

        break;

    default:
        esLog(ES_DEBUG_ERROR, "Invalid item right type, %d\n", type);
        rv = ES_ERR_INVALID;
        goto end;
    }

end:
    return rv;
}


static ESError
__getItemRights(u8 *buf, u32 offset, u32 size, ESItemRightsGet *itemRightsGet,
                ESGetItemRightsContext *ctx)
{
    ESError rv = ES_ERR_OK;
    u32 overlapOffset = 0, overlapBytes = 0, rightsOffset;
    u32 availableSize, copiedSize = 0, remainingSize;

    /*
     * This function relies on some unverified values.  To be safe, check
     * the inputs and use them carefully
     */
    if (offset > ES_ABSOLUTE_MAX_TICKET_SIZE ||
        itemRightsGet->sectHdrs == NULL ||
        itemRightsGet->sectHdrsOffset > ES_ABSOLUTE_MAX_TICKET_SIZE ||
        itemRightsGet->sectHdrsSize > ES_ABSOLUTE_MAX_TICKET_SIZE) {
        esLog(ES_DEBUG_ERROR, "Arguments failed sanity check, %u:%u:%u\n", offset, itemRightsGet->sectHdrsOffset, itemRightsGet->sectHdrsSize);
        rv = ES_ERR_INVALID;
        goto end;
    }

    if (itemRightsGet->outRights != NULL &&
        (itemRightsGet->rightsOffset > ES_ABSOLUTE_MAX_TICKET_SIZE ||
         itemRightsGet->nRights > ES_ABSOLUTE_MAX_RIGHTS)) {
        esLog(ES_DEBUG_ERROR, "Arguments failed sanity check, %u:%u\n", itemRightsGet->rightsOffset, itemRightsGet->nRights);
        rv = ES_ERR_INVALID;
        goto end;
    }

    /*
     * Check if this portion of the ticket overlaps with the section
     * headers.  If it does, then check the overlap bytes for match
     */
    if (itemRightsGet->sectHdrsOffset >= offset && itemRightsGet->sectHdrsOffset < (offset + size)) {
        overlapOffset = itemRightsGet->sectHdrsOffset;

        overlapBytes = offset + size - itemRightsGet->sectHdrsOffset;
        if (overlapBytes > itemRightsGet->sectHdrsSize) {
            overlapBytes = itemRightsGet->sectHdrsSize;
        }
    } else if (offset >= itemRightsGet->sectHdrsOffset && offset < (itemRightsGet->sectHdrsOffset + itemRightsGet->sectHdrsSize)) {
        overlapOffset = offset;

        overlapBytes = itemRightsGet->sectHdrsOffset + itemRightsGet->sectHdrsSize - offset;
        if (overlapBytes > size) {
            overlapBytes = size;
        }
    }

    if (overlapBytes > 0) {
        if (memcmp(&buf[overlapOffset - offset], &((u8 *) itemRightsGet->sectHdrs)[overlapOffset - itemRightsGet->sectHdrsOffset], overlapBytes) != 0) {
            esLog(ES_DEBUG_ERROR, "Failed to check section headers\n");
            rv = ES_ERR_INVALID;
            goto end;
        }
    }

    /*
     * Check if this portion of the ticket overlaps with the requested
     * item rights.  If it does, then copy out the item rights to the
     * output buffer.  Beware of partial records since it's possible
     * that only part of a record is available on this portion of the
     * ticket
     */
    if (itemRightsGet->outRights) {
        rightsOffset = itemRightsGet->rightsOffset + (ctx->nCopied * itemRightsGet->itemSize) + ctx->leftOverBytes;
        if (rightsOffset >= offset && rightsOffset < (offset + size)) {
            availableSize = offset + size - rightsOffset;

            while (copiedSize < availableSize) {
                /*
                 * Error check to avoid buffer overflow, but this case
                 * should never happen in real life
                 */
                if ((rightsOffset + copiedSize - offset) > size) {
                    esLog(ES_DEBUG_ERROR, "Unexpected values\n");
                    rv = ES_ERR_FAIL;
                    goto end;
                }

                if (ctx->nCopied >= itemRightsGet->nRights) {
                    break;
                }

                remainingSize = availableSize - copiedSize;

                // Check the context for a previous partial item
                if (ctx->leftOverBytes > 0) {
                    if (remainingSize < (itemRightsGet->itemSize - ctx->leftOverBytes)) {
                        /*
                         * Error check to avoid buffer overflow, but this
                         * case should never happen in real life
                         */
                        if (ctx->leftOverBytes > sizeof(ctx->record) ||
                                remainingSize > sizeof(ctx->record) ||
                                (ctx->leftOverBytes + remainingSize) > sizeof(ctx->record)) {
                            esLog(ES_DEBUG_ERROR, "Unexpected values\n");
                            rv = ES_ERR_FAIL;
                            goto end;
                        }

                        memcpy(&((u8 *) &ctx->record)[ctx->leftOverBytes],
                               &buf[rightsOffset + copiedSize - offset],
                               remainingSize);
                        ctx->leftOverBytes += remainingSize;

                        break;
                    } else {
                        /*
                         * Error check to avoid buffer overflow, but this
                         * case should never happen in real life
                         */
                        if (ctx->leftOverBytes > sizeof(ctx->record) ||
                                ctx->leftOverBytes > itemRightsGet->itemSize) {
                            esLog(ES_DEBUG_ERROR, "Unexpected values\n");
                            rv = ES_ERR_FAIL;
                            goto end;
                        }

                        memcpy(&((u8 *) &ctx->record)[ctx->leftOverBytes],
                               &buf[rightsOffset + copiedSize - offset],
                               itemRightsGet->itemSize - ctx->leftOverBytes);

                        if ((rv = __decodeItemRight(
                                    (u8 *) &ctx->record, itemRightsGet->itemType,
                                    (u8 *) itemRightsGet->outRights, ctx->nCopied))
                                        != ES_ERR_OK) {
                            goto end;
                        }

                        ctx->nCopied++;
                        copiedSize += itemRightsGet->itemSize - ctx->leftOverBytes;
                        ctx->leftOverBytes = 0;

                        continue;
                    }
                }

                // Check for full items
                if (remainingSize >= itemRightsGet->itemSize) {
                    if ((rv = __decodeItemRight(
                                &buf[rightsOffset + copiedSize - offset],
                                itemRightsGet->itemType,
                                (u8 *) itemRightsGet->outRights, ctx->nCopied))
                                    != ES_ERR_OK) {
                        goto end;
                    }

                    ctx->nCopied++;
                    copiedSize += itemRightsGet->itemSize;

                    continue;
                }

                // Remember the partial item for the next call
                memcpy(&ctx->record, &buf[rightsOffset + copiedSize - offset], remainingSize);
                ctx->leftOverBytes = remainingSize;

                break;
            }
        }
    }

end:
    return rv;
}

typedef struct {
    ESGetItemRightsContext itemRightsCtx;
#if !defined(ES_READ_ONLY)
    IOSCHashContext hashCtx;
    IOSCHash256 hash;
#endif  // ES_READ_ONLY
} esVerifyTicket_big_locals;

ESError
esVerifyTicket(IInputStream __REFVSPTR ticket, const void *certs[], u32 nCerts,
               bool doVerify, ESTicketWriter *ticketWriter,
               ESItemRightsGet *itemRightsGet, void *outTicket)
{
    ESError rv = ES_ERR_OK;
    ESTicket *tp;
    ESV1TicketHeader *v1;
    IOSCPublicKeyHandle hIssuerPubKey = 0;
    u32 initialSize, remainingSize, bytes, nRead;
    u32 readPos = 0;
    u32 writePos = 0;
#if !defined(ES_READ_ONLY)
    u32 headerSize;
#endif  // ES_READ_ONLY

    esVerifyTicket_big_locals *big_locals;
#ifdef __KERNEL__
    big_locals = kmalloc(sizeof(*big_locals), GFP_KERNEL);
    if (!big_locals)
        return ES_ERR_NO_MEMORY;
#else
    esVerifyTicket_big_locals _big_locals;
    big_locals = &_big_locals;
#endif

    // Read the v0 portion and v1 header of the ticket
    if ((rv = esSeek(ticket, 0)) != ES_ERR_OK) {
        goto end;
    }

    initialSize = sizeof(ESTicket) + sizeof(ESV1TicketHeader);
    if ((rv = esRead(ticket, initialSize, outTicket)) != ES_ERR_OK) {
        goto end;
    }
    readPos = initialSize;
    tp = (ESTicket *) outTicket;

    // Only support RSA-2048 with SHA-256
    if (ntohl(tp->sig.sigType) != IOSC_SIG_RSA2048_H256) {
        esLog(ES_DEBUG_ERROR, "Unsupported signature type, %u\n", ntohl(tp->sig.sigType));
        rv = ES_ERR_INCORRECT_SIG_TYPE;
        goto end;
    }

    // Only support version 1
    if (tp->version != 1) {
        esLog(ES_DEBUG_ERROR, "Unsupported ticket version, %d\n", tp->version);
        rv = ES_ERR_INCORRECT_TICKET_VERSION;
        goto end;
    }

    // Verify the certs
    if (doVerify) {
        if ((rv = ESI_VerifyContainer(ES_CONTAINER_TKT, NULL, 0, tp->sig.sig, (const char *) tp->sig.issuer, certs, nCerts, 0, &hIssuerPubKey)) != ES_ERR_OK) {
            goto end;
        }
    }

    // Unpersonalize in the case of import
    if (ticketWriter && ticketWriter->isImport && ntohl(tp->deviceId) != ES_COMMON_TICKET_DEVICE_ID) {
        if ((rv = ESI_UnpersonalizeTicket(tp)) != ES_ERR_OK) {
            goto end;
        }
    }

    /*
     * Verify the ticket.  Since the ticket could potentially be large,
     * it needs to be read in by chunks
     */
    if (doVerify) {
#if !defined(ES_READ_ONLY)
        headerSize = sizeof(IOSCCertSigType) + sizeof(IOSCRsaSig2048) + sizeof(IOSCSigDummy);

        if ((rv = IOSC_GenerateHash(big_locals->hashCtx, &((u8 *) tp)[headerSize], initialSize - headerSize, IOSC_SHA256_INIT, NULL)) != IOSC_ERROR_OK) {
            esLog(ES_DEBUG_ERROR, "Failed to generate hash - init, rv=%d\n", rv);
            rv = ESI_Translate_IOSC_Error(rv);
            goto end;
        }
#endif  // ES_READ_ONLY
    }

    // Personalize in the case of export
    if (ticketWriter && !ticketWriter->isImport && ntohl(tp->deviceId) != ES_COMMON_TICKET_DEVICE_ID) {
        if ((rv = ESI_PersonalizeTicket(tp)) != ES_ERR_OK) {
            goto end;
        }
    }

    if (ticketWriter) {
        if ((rv = esSeek(__REFPASSPTR(ticketWriter->ticketDest), 0)) != ES_ERR_OK) {
            goto end;
        }

        if ((rv = esWrite(__REFPASSPTR(ticketWriter->ticketDest), initialSize, tp)) != ES_ERR_OK) {
            goto end;
        }
        writePos = initialSize;
    }

    /*
     * Error check to avoid buffer overflow, but this case should
     * never happen in real life
     */
    v1 = (ESV1TicketHeader *) &tp[1];
    if (ntohl(v1->ticketSize) < sizeof(ESV1TicketHeader) || ntohl(v1->ticketSize) > ES_ABSOLUTE_MAX_TICKET_SIZE) {
        esLog(ES_DEBUG_ERROR, "Unexpected v1 ticket size, %u\n", ntohl(v1->ticketSize));
        rv = ES_ERR_FAIL;
        goto end;
    }

    // Hash the rest of the ticket and write it out in parallel
    remainingSize = sizeof(ESTicket) + ntohl(v1->ticketSize) - initialSize;
    bytes = 0;

    big_locals->itemRightsCtx.nCopied = 0;
    big_locals->itemRightsCtx.leftOverBytes = 0;

    while (bytes < remainingSize) {
        nRead = ES_MIN(remainingSize - bytes, sizeof(__buf));

        /*
         * The input ticket stream and the output ticketwriter
         * stream may be the same, so the read and write positions
         * must be maintained explicitly in this code.
         */
        if ((rv = esSeek(ticket, readPos)) != ES_ERR_OK) {
            goto end;
        }
        if ((rv = esRead(ticket, nRead, __buf)) != ES_ERR_OK) {
            goto end;
        }

        readPos += nRead;

        if (doVerify) {
#if !defined(ES_READ_ONLY)
            if ((rv = IOSC_GenerateHash(big_locals->hashCtx, __buf, nRead, IOSC_SHA256_UPDATE, NULL)) != IOSC_ERROR_OK) {
                esLog(ES_DEBUG_ERROR, "Failed to generate hash - update, rv=%d\n", rv);
                rv = ESI_Translate_IOSC_Error(rv);
                goto end;
            }
#endif  // ES_READ_ONLY
        }

        if (itemRightsGet != NULL) {
            if ((rv = __getItemRights(__buf, initialSize + bytes, nRead, itemRightsGet, &big_locals->itemRightsCtx)) != ES_ERR_OK) {
                goto end;
            }
        }

        if (ticketWriter) {
            /*
             * The input ticket stream and the output ticketwriter
             * stream may be the same, so the read and write positions
             * must be maintained explicitly in this code.
             */
            if ((rv = esSeek(__REFPASSPTR(ticketWriter->ticketDest), writePos)) != ES_ERR_OK) {
                goto end;
            }
            if ((rv = esWrite(__REFPASSPTR(ticketWriter->ticketDest), nRead, __buf)) != ES_ERR_OK) {
                goto end;
            }
            writePos += nRead;
        }

        bytes += nRead;
    }

    if (doVerify) {
#if !defined(ES_READ_ONLY)
        if ((rv = IOSC_GenerateHash(big_locals->hashCtx, NULL, 0, IOSC_SHA256_FINAL, big_locals->hash)) != IOSC_ERROR_OK) {
            esLog(ES_DEBUG_ERROR, "Failed to generate hash - final, rv=%d\n", rv);
            rv = ESI_Translate_IOSC_Error(rv);
            goto end;
        }

        if ((rv = IOSC_VerifyPublicKeySign(big_locals->hash, sizeof(big_locals->hash), hIssuerPubKey, tp->sig.sig)) != IOSC_ERROR_OK) {
            esLog(ES_DEBUG_ERROR, "Failed to verify ticket signature, rv=%d\n", rv);
            rv = ESI_Translate_IOSC_Error(rv);
            goto end;
        }
#endif  // ES_READ_ONLY
    }

end:
#ifdef __KERNEL__
    kfree(big_locals);
#endif
    return rv;
}


ESError
esVerifyTmdFixed(IInputStream __REFVSPTR tmd, const void *certs[], u32 nCerts,
                 ESV1TitleMeta *outTmd)
{
    ESError rv = ES_ERR_OK;
    u32 dataSize;
#if !defined(ES_READ_ONLY)
    IOSCHashContext hashCtx;
    IOSCHash256 hash;
#endif  // ES_READ_ONLY

    // Read the TMD header
    if ((rv = esSeek(tmd, 0)) != ES_ERR_OK) {
        goto end;
    }

    if ((rv = esRead(tmd, sizeof(ESV1TitleMeta), outTmd)) != ES_ERR_OK) {
        goto end;
    }

    // Only support RSA-2048 with SHA-256
    if (ntohl(outTmd->sig.sigType) != IOSC_SIG_RSA2048_H256) {
        esLog(ES_DEBUG_ERROR, "Unsupported signature type, %u\n", ntohl(outTmd->sig.sigType));
        rv = ES_ERR_INCORRECT_SIG_TYPE;
        goto end;
    }

    // Only support version 1
    if (outTmd->head.version != 1) {
        esLog(ES_DEBUG_ERROR, "Unsupported TMD version, %d\n", outTmd->head.version);
        rv = ES_ERR_INCORRECT_TMD_VERSION;
        goto end;
    }

    // Verify the certs and the (TMD header + v1 TMD header hash)
    dataSize = sizeof(IOSCSigRsa2048) + sizeof(ESTitleMetaHeader) + sizeof(IOSCHash256);

    if ((rv = ESI_VerifyContainer(ES_CONTAINER_TMD, outTmd, dataSize, outTmd->sig.sig, (const char *) outTmd->sig.issuer, certs, nCerts, 0, NULL)) != ES_ERR_OK) {
        goto end;
    }

#if !defined(ES_READ_ONLY)
    // Verify the v1 TMD header using the v1 TMD header hash
    if ((rv = IOSC_GenerateHash(hashCtx, NULL, 0, IOSC_SHA256_INIT, NULL)) != IOSC_ERROR_OK) {
        esLog(ES_DEBUG_ERROR, "Failed to generate hash - init, rv=%d\n", rv);
        rv = ESI_Translate_IOSC_Error(rv);
        goto end;
    }

    if ((rv = IOSC_GenerateHash(hashCtx, (u8 *) &outTmd->v1Head.cmdGroups[0], ES_MAX_CMD_GROUPS * sizeof(ESV1ContentMetaGroup), IOSC_SHA256_FINAL, hash)) != IOSC_ERROR_OK) {
        esLog(ES_DEBUG_ERROR, "Failed to generate hash - final, rv=%d\n", rv);
        rv = ESI_Translate_IOSC_Error(rv);
        goto end;
    }

    if (memcmp(hash, outTmd->v1Head.hash, sizeof(IOSCHash256)) != 0) {
        esLog(ES_DEBUG_ERROR, "Failed to verify hash\n");
        rv = ES_ERR_VERIFICATION;
        goto end;
    }
#endif  // ES_READ_ONLY

end:
    return rv;
}


ESError
esVerifyCmdGroup(IInputStream __REFVSPTR tmd, ESV1ContentMetaGroup *groups,
                 u32 groupIndex, bool doVerify,
                 ESContentMetaSearchByIndex *cmdSearchByIndex,
                 ESContentMetaSearchById *cmdSearchById,
                 ESContentMetaGet *cmdGet)
{
    ESError rv = ES_ERR_OK;
    ESV1ContentMetaGroup *cmdGroup;
    u32 baseCmd = 0, pos, maxCmds, remainingCmds, nCmds, i, j;
    u32 curOffset = 0, nCopied = 0;
    ESV1ContentMeta *cmd;
#if !defined(ES_READ_ONLY)
    IOSCHashContext hashCtx;
    IOSCHash256 hash;
#endif  // ES_READ_ONLY

    if (cmdSearchByIndex != NULL) {
        cmdSearchByIndex->nFound = 0;
    }
    if (cmdSearchById != NULL) {
        cmdSearchById->found = false;
    }

    cmdGroup = &groups[groupIndex];
    if (ntohs(cmdGroup->nCmds) == 0) {
        // Nothing to verify
        goto end;
    }

    /* 
     * Read the CMDs into the global buffer for verification.  Since the
     * number of CMDs could be large, they need to be read in by chunks
     */
    for (i = 0; i < groupIndex; i++) {
        baseCmd += ntohs(groups[i].nCmds);
    }

    pos = sizeof(ESV1TitleMeta) + (baseCmd * sizeof(ESV1ContentMeta));
    if ((rv = esSeek(tmd, pos)) != ES_ERR_OK) {
        goto end;
    }

    if (doVerify) {
#if !defined(ES_READ_ONLY)
        if ((rv = IOSC_GenerateHash(hashCtx, NULL, 0, IOSC_SHA256_INIT, NULL)) != IOSC_ERROR_OK) {
            esLog(ES_DEBUG_ERROR, "Failed to generate group hash - init, rv=%d\n", rv);
            rv = ESI_Translate_IOSC_Error(rv);
            goto end;
        }
#endif  // ES_READ_ONLY
    }

    remainingCmds = ntohs(cmdGroup->nCmds);
    maxCmds = sizeof(__buf) / sizeof(ESV1ContentMeta);

    while (remainingCmds > 0) {
        nCmds = ES_MIN(remainingCmds, maxCmds);

        if ((rv = esRead(tmd, nCmds * sizeof(ESV1ContentMeta), __buf)) != ES_ERR_OK) {
            goto end;
        }

        if (doVerify) {
#if !defined(ES_READ_ONLY)
            if ((rv = IOSC_GenerateHash(hashCtx, __buf, nCmds * sizeof(ESV1ContentMeta), IOSC_SHA256_UPDATE, NULL)) != IOSC_ERROR_OK) {
                esLog(ES_DEBUG_ERROR, "Failed to generate group hash - update, rv=%d\n", rv);
                rv = ESI_Translate_IOSC_Error(rv);
                goto end;
            }
#endif  // ES_READ_ONLY
        }

        // Search for the requested CMD
        cmd = (ESV1ContentMeta *) __buf;
        for (i = 0; i < nCmds; i++) {
            if (cmdSearchByIndex != NULL) {
                for (j = 0; j < cmdSearchByIndex->nIndexes; j++) {
                    if (cmdSearchByIndex->indexes[j] == ntohs(cmd->index)) {
                        cmdSearchByIndex->outInfos[j].id = ntohl(cmd->cid);
                        cmdSearchByIndex->outInfos[j].index = ntohs(cmd->index);
                        cmdSearchByIndex->outInfos[j].type = ntohs(cmd->type);
                        cmdSearchByIndex->outInfos[j].size = ntohll(cmd->size);

                        cmdSearchByIndex->nFound++;
                        break;
                    }
                }
            }

            if (cmdSearchById != NULL) {
                if (cmdSearchById->id == ntohl(cmd->cid)) {
                    *cmdSearchById->outCmd = *cmd;
                    cmdSearchById->found = true;
                }
            }

            if (cmdGet != NULL) {
                if (curOffset >= cmdGet->offset && curOffset < (cmdGet->offset + cmdGet->nInfos)) {
                    cmdGet->outInfos[nCopied].id = ntohl(cmd->cid);
                    cmdGet->outInfos[nCopied].index = ntohs(cmd->index);
                    cmdGet->outInfos[nCopied].type = ntohs(cmd->type);
                    cmdGet->outInfos[nCopied].size = ntohll(cmd->size);

                    nCopied++;
                }
            }

            cmd++;
            curOffset++;
        }

        remainingCmds -= nCmds;
    }

    if (doVerify) {
#if !defined(ES_READ_ONLY)
        if ((rv = IOSC_GenerateHash(hashCtx, NULL, 0, IOSC_SHA256_FINAL, hash)) != IOSC_ERROR_OK) {
            esLog(ES_DEBUG_ERROR, "Failed to generate group hash - final, rv=%d\n", rv);
            rv = ESI_Translate_IOSC_Error(rv);
            goto end;
        }

        if (memcmp(hash, cmdGroup->groupHash, sizeof(IOSCHash256)) != 0) {
            esLog(ES_DEBUG_ERROR, "Failed to verify group hash\n");
            rv = ES_ERR_VERIFICATION;
            goto end;
        }
#endif  // ES_READ_ONLY
    }

end:
    return rv;
}

ESError
esVerifyTmdContent(IInputStream __REFVSPTR tmd, ESContentId contentId,
		   ESV1TitleMeta *inTmd, ESV1ContentMeta *outCmd)
{
    ESError rv = ES_ERR_OK;
    u32 i;
    ESContentMetaSearchById cmdSearchById, *cmdSearchByIdPtr = NULL;
    bool found = false;

    // Verify each individual CMD group
    for (i = 0; i < ES_MAX_CMD_GROUPS; i++) {
        if (outCmd) {
            cmdSearchById.id = contentId;
            cmdSearchById.outCmd = outCmd;

            cmdSearchByIdPtr = &cmdSearchById;
        }

        if ((rv = esVerifyCmdGroup(tmd, inTmd->v1Head.cmdGroups, i, true, NULL, cmdSearchByIdPtr, NULL)) != ES_ERR_OK) {
            goto end;
        }

        if (cmdSearchByIdPtr && cmdSearchByIdPtr->found) {
            found = true;
        }
    }

    if (outCmd != NULL && !found) {
        esLog(ES_DEBUG_ERROR, "Failed to find content ID\n");
        rv = ES_ERR_INVALID;
        goto end;
    }

end:
    return rv;

}

ESError
esVerifyTmd(IInputStream __REFVSPTR tmd, const void *certs[], u32 nCerts,
            ESContentId contentId, ESV1TitleMeta *outTmd,
            ESV1ContentMeta *outCmd)
{
    ESError rv = ES_ERR_OK;

    if ((rv = esVerifyTmdFixed(tmd, certs, nCerts, outTmd)) != ES_ERR_OK) {
        goto end;
    }

    rv = esVerifyTmdContent(tmd, contentId, outTmd, outCmd);

end:
    return rv;
}


#if !defined(ES_READ_ONLY)
static ESError
__signHelper(ESTitleId signerTitleId, IOSCHash256 hash,
             IOSCEccSig *outSig, IOSCEccEccCert *outCert)
{
    ESError rv = ES_ERR_OK;
    IOSCSecretKeyHandle hSignKey = 0;
    IOSCCertName certName;
    u32 titleIdHigh, titleIdLow;

    // Generate a private key and ephemeral cert
    if ((rv = IOSC_CreateObject(&hSignKey, IOSC_SECRETKEY_TYPE, IOSC_ECC233_SUBTYPE)) != IOSC_ERROR_OK) {
        esLog(ES_DEBUG_ERROR, "Failed to create object for secret key, rv=%d\n", rv);
        rv = ESI_Translate_IOSC_Error(rv);
        goto end;
    }

    if ((rv = IOSC_GenerateKey(hSignKey)) != IOSC_ERROR_OK) {
        esLog(ES_DEBUG_ERROR, "Failed to generate secret key, rv=%d\n", rv);
        rv = ESI_Translate_IOSC_Error(rv);
        goto end;
    }

    titleIdHigh = (u32) (signerTitleId >> 32);
    titleIdLow = (u32) signerTitleId;
    memset(certName, 0, sizeof(IOSCCertName));
    VPL_snprintf((char *) certName, sizeof(ES_APP_CERT_PREFIX) + 16, "%s%08x%08x", ES_APP_CERT_PREFIX, titleIdHigh, titleIdLow);

    if ((rv = IOSC_GenerateCertificate(hSignKey, certName, (IOSCEccSignedCert *) outCert)) != IOSC_ERROR_OK) {
        esLog(ES_DEBUG_ERROR, "Failed to generate cert, rv=%d\n", rv);
        rv = ESI_Translate_IOSC_Error(rv);
        goto end;
    }

    // Sign with the private key
    if ((rv = IOSC_GeneratePublicKeySign(hash, sizeof(IOSCHash256), hSignKey, (u8 *) outSig)) != IOSC_ERROR_OK) {
        esLog(ES_DEBUG_ERROR, "Failed to sign, rv=%d\n", rv);
        rv = ESI_Translate_IOSC_Error(rv);
        goto end;
    }

end:
    if (hSignKey != 0) {
        (void) IOSC_DeleteObject(hSignKey);
    }

    return rv;
}


ESError
esSign(ESTitleId signerTitleId, IInputStream __REFVSPTR data, u32 dataSize,
       void *outSig, u32 outSigSize, void *outCert, u32 outCertSize)
{
    ESError rv = ES_ERR_OK;
    IOSCHashContext hashCtx;
    IOSCHash256 hash;
    u32 bytes, remainingSize;

    if (dataSize == 0 || outSig == NULL || outSigSize != ES_SIGNATURE_SIZE || outCert == NULL || outCertSize != ES_SIGNING_CERT_SIZE || sizeof(IOSCEccSig) != ES_SIGNATURE_SIZE || sizeof(IOSCEccSignedCert) != ES_SIGNING_CERT_SIZE) {
        esLog(ES_DEBUG_ERROR, "Invalid arguments\n");
        rv = ES_ERR_INVALID;
        goto end;
    }

    // Compute data hash
    if ((rv = IOSC_GenerateHash(hashCtx, NULL, 0, IOSC_SHA256_INIT, NULL)) != IOSC_ERROR_OK) {
        esLog(ES_DEBUG_ERROR, "Failed to generate hash - init, rv=%d\n", rv);
        rv = ESI_Translate_IOSC_Error(rv);
        goto end;
    }

    if ((rv = esSeek(data, 0)) != ES_ERR_OK) {
        goto end;
    }

    remainingSize = dataSize;
    while (remainingSize > 0) {
        bytes = ES_MIN(remainingSize, sizeof(__buf));

        if ((rv = esRead(data, bytes, __buf)) != ES_ERR_OK) {
            goto end;
        }

        if ((rv = IOSC_GenerateHash(hashCtx, __buf, bytes, IOSC_SHA256_UPDATE, NULL)) != IOSC_ERROR_OK) {
            esLog(ES_DEBUG_ERROR, "Failed to generate hash - update, rv=%d\n", rv);
            rv = ESI_Translate_IOSC_Error(rv);
            goto end;
        }

        remainingSize -= bytes;
    }

    if ((rv = IOSC_GenerateHash(hashCtx, NULL, 0, IOSC_SHA256_FINAL, hash)) != IOSC_ERROR_OK) {
        esLog(ES_DEBUG_ERROR, "Failed to generate hash - final, rv=%d\n", rv);
        rv = ESI_Translate_IOSC_Error(rv);
        goto end;
    }

    if ((rv = __signHelper(signerTitleId, hash, (IOSCEccSig *) outSig, (IOSCEccEccCert *) outCert)) != ES_ERR_OK) {
        goto end;
    }

end:
    return rv;
}


ESError
esSignData(ESTitleId signerTitleId, const void *data, u32 dataSize,
           IOSCEccSig *outSig, IOSCEccEccCert *outCert)
{
    ESError rv = ES_ERR_OK;
    IOSCHashContext hashCtx;
    IOSCHash256 hash;

    if (dataSize == 0 || outSig == NULL || outCert == NULL) {
        esLog(ES_DEBUG_ERROR, "Invalid arguments\n");
        rv = ES_ERR_INVALID;
        goto end;
    }

    // Compute data hash
    if ((rv = IOSC_GenerateHash(hashCtx, NULL, 0, IOSC_SHA256_INIT, NULL)) != IOSC_ERROR_OK) {
        esLog(ES_DEBUG_ERROR, "Failed to generate hash - init, rv=%d\n", rv);
        rv = ESI_Translate_IOSC_Error(rv);
        goto end;
    }

    if ((rv = IOSC_GenerateHash(hashCtx, (u8 *) data, dataSize, IOSC_SHA256_FINAL, hash)) != IOSC_ERROR_OK) {
        esLog(ES_DEBUG_ERROR, "Failed to generate hash - update, rv=%d\n", rv);
        rv = ESI_Translate_IOSC_Error(rv);
        goto end;
    }

    if ((rv = __signHelper(signerTitleId, hash, outSig, outCert)) != ES_ERR_OK) {
        goto end;
    }

end:
    return rv;
}


ESError
esVerifyDeviceSign(ESTitleId signerTitleId,
                   IInputStream __REFVSPTR data, u32 dataSize,
                   const IOSCEccSig *sig, ESSigType sigType,
                   const IOSCEccEccCert *signingCert,
                   IOSCPublicKeyHandle hDevicePubKey)
{
    ESError rv = ES_ERR_OK;
    IOSCPublicKeyHandle hSigningPubKey = 0;
    IOSCHashContext hashCtx;
    IOSCHash256 hash256;
    IOSCHash hash;
    IOSCCertSigType certSigType;
    u8 *hashPtr;
    u32 hashSize, hashType, bytes, remainingSize, hSize, dSize;

    (void) signerTitleId;

    if (dataSize == 0 || sig == NULL || (sigType != ES_SIG_TYPE_ECC_SHA1 && sigType != ES_SIG_TYPE_ECC_SHA256) || signingCert == NULL || hDevicePubKey == 0) {
        esLog(ES_LOG_ERROR, "Invalid arguments\n");
        rv = ES_ERR_INVALID;
        goto end;
    }

    // Verify signing cert
    certSigType = ntohl(signingCert->sig.sigType);
    if (certSigType == IOSC_SIG_ECC_H256) {
        hashPtr = hash256;
        hashSize = sizeof(hash256);
        hashType = IOSC_HASH_SHA256;
    } else if (certSigType == IOSC_SIG_ECC) {
        hashPtr = hash;
        hashSize = sizeof(hash);
        hashType = IOSC_HASH_SHA1;
    } else {
        esLog(ES_LOG_ERROR, "Invalid signing cert sig type %u\n", certSigType);
        rv = ES_ERR_INVALID;
        goto end;
    }

    hSize = sizeof(IOSCCertSigType) + sizeof(IOSCEccSig) + sizeof(IOSCEccPublicPad) + sizeof(IOSCSigDummy);
    dSize = sizeof(IOSCEccEccCert) - hSize;

    if ((rv = IOSC_GenerateHash(hashCtx, NULL, 0, IOSC_HASH_FIRST | hashType, NULL)) != IOSC_ERROR_OK) {
        esLog(ES_DEBUG_ERROR, "Failed to generate hash - init, rv=%d\n", rv);
        rv = ESI_Translate_IOSC_Error(rv);
        goto end;
    }

    if ((rv = IOSC_GenerateHash(hashCtx, ((u8 *) signingCert) + hSize, dSize, IOSC_HASH_LAST | hashType, hashPtr)) != IOSC_ERROR_OK) {
        esLog(ES_DEBUG_ERROR, "Failed to generate hash - final, rv=%d\n", rv);
        rv = ESI_Translate_IOSC_Error(rv);
        goto end;
    }

    if ((rv = IOSC_VerifyPublicKeySign(hashPtr, hashSize, hDevicePubKey, (u8 *) signingCert->sig.sig)) != IOSC_ERROR_OK) {
        esLog(ES_DEBUG_ERROR, "Failed to verify signingCert, rv=%d\n", rv);
        rv = ESI_Translate_IOSC_Error(rv);
        goto end;
    }

    // Compute data hash
    if (sigType == ES_SIG_TYPE_ECC_SHA256) {
        hashPtr = hash256;
        hashSize = sizeof(hash256);
        hashType = IOSC_HASH_SHA256;
    } else if (sigType == ES_SIG_TYPE_ECC_SHA1) {
        hashPtr = hash;
        hashSize = sizeof(hash);
        hashType = IOSC_HASH_SHA1;
    } else {
        esLog(ES_LOG_ERROR, "Invalid data sig type %u\n", sigType);
        rv = ES_ERR_INVALID;
        goto end;
    }

    if ((rv = IOSC_GenerateHash(hashCtx, NULL, 0, IOSC_HASH_FIRST | hashType, NULL)) != IOSC_ERROR_OK) {
        esLog(ES_DEBUG_ERROR, "Failed to generate hash - init, rv=%d\n", rv);
        rv = ESI_Translate_IOSC_Error(rv);
        goto end;
    }

    if ((rv = esSeek(data, 0)) != ES_ERR_OK) {
        goto end;
    }

    remainingSize = dataSize;
    while (remainingSize > 0) {
        bytes = ES_MIN(remainingSize, sizeof(__buf));

        if ((rv = esRead(data, bytes, __buf)) != ES_ERR_OK) {
            goto end;
        }

        if ((rv = IOSC_GenerateHash(hashCtx, __buf, bytes, IOSC_HASH_MIDDLE | hashType, NULL)) != IOSC_ERROR_OK) {
            esLog(ES_DEBUG_ERROR, "Failed to generate hash - update, rv=%d\n", rv);
            rv = ESI_Translate_IOSC_Error(rv);
            goto end;
        }

        remainingSize -= bytes;
    }

    if ((rv = IOSC_GenerateHash(hashCtx, NULL, 0, IOSC_HASH_LAST | hashType, hashPtr)) != IOSC_ERROR_OK) {
        esLog(ES_DEBUG_ERROR, "Failed to generate hash - final, rv=%d\n", rv);
        rv = ESI_Translate_IOSC_Error(rv);
        goto end;
    }

    // Verify signature
    if ((rv = IOSC_CreateObject(&hSigningPubKey, IOSC_PUBLICKEY_TYPE, IOSC_ECC233_SUBTYPE)) != IOSC_ERROR_OK) {
        esLog(ES_DEBUG_ERROR, "Failed to create object for signingPubKey, rv=%d\n", rv);
        rv = ESI_Translate_IOSC_Error(rv);
        goto end;
    }

    if ((rv = IOSC_ImportPublicKey((u8 *) signingCert->pubKey, NULL, hSigningPubKey)) != IOSC_ERROR_OK) {
        esLog(ES_DEBUG_ERROR, "Failed to import public key for signingPubKey, rv=%d\n", rv);
        rv = ESI_Translate_IOSC_Error(rv);
        goto end;
    }

    if ((rv = IOSC_VerifyPublicKeySign(hashPtr, hashSize, hSigningPubKey, (u8 *) sig)) != IOSC_ERROR_OK) {
        esLog(ES_DEBUG_ERROR, "Failed to verify, rv=%d\n", rv);
        rv = ESI_Translate_IOSC_Error(rv);
        goto end;
    }

end:
    if (hSigningPubKey) {
        (void) IOSC_DeleteObject(hSigningPubKey);
    }

    return rv;
}


ESError
esVerifySign(ESTitleId signerTitleId,
             IInputStream __REFVSPTR data, u32 dataSize,
             const void *sig, u32 sigSize, ESSigType sigType,
             const void *certs[], u32 nCerts)
{
    ESError rv = ES_ERR_OK;
    IOSCCertName certName;
    IOSCEccEccCert *signingCert, *deviceCert;
    IOSCPublicKeyHandle hDevicePubKey = 0;
    char *deviceId, *msId;
    u32 signingCertSize, deviceCertSize, titleIdHigh, titleIdLow;

    if (dataSize == 0 || sig == NULL || sigSize != ES_SIGNATURE_SIZE || (sigType != ES_SIG_TYPE_ECC_SHA1 && sigType != ES_SIG_TYPE_ECC_SHA256) || certs == NULL || nCerts == 0 || sizeof(IOSCEccSig) != ES_SIGNATURE_SIZE) {
        esLog(ES_DEBUG_ERROR, "Invalid arguments\n");
        rv = ES_ERR_INVALID;
        goto end;
    }

    // Find signing and device certs
    titleIdHigh = (u32) (signerTitleId >> 32);
    titleIdLow = (u32) signerTitleId;
    memset(certName, 0, sizeof(IOSCCertName));
    VPL_snprintf((char *) certName, sizeof(ES_APP_CERT_PREFIX) + 16, "%s%08x%08x", ES_APP_CERT_PREFIX, titleIdHigh, titleIdLow);

    if ((rv = ESI_FindCert((char *) certName, 0, certs, nCerts, (void **) &signingCert, &signingCertSize, &deviceId)) != ES_ERR_OK) {
        goto end;
    }

    if ((rv = ESI_FindCert(deviceId, 1, certs, nCerts, (void **) &deviceCert, &deviceCertSize, &msId)) != ES_ERR_OK) {
        goto end;
    }

    // Verify device cert
    if ((rv = IOSC_CreateObject(&hDevicePubKey, IOSC_PUBLICKEY_TYPE, IOSC_ECC233_SUBTYPE)) != IOSC_ERROR_OK) {
        esLog(ES_DEBUG_ERROR, "Failed to create object for public key, rv=%d\n", rv);
        rv = ESI_Translate_IOSC_Error(rv);
        goto end;
    }

    if ((rv = ESI_VerifyContainer(ES_CONTAINER_DEV, deviceCert, deviceCertSize, deviceCert->sig.sig, (const char *) deviceCert->sig.issuer, certs, nCerts, hDevicePubKey, NULL)) != ES_ERR_OK) {
        goto end;
    }

    if ((rv = esVerifyDeviceSign(signerTitleId, data, dataSize, (const IOSCEccSig *) sig, sigType, (const IOSCEccEccCert *) signingCert, hDevicePubKey)) != ES_ERR_OK) {
        goto end;
    }

end:
    if (hDevicePubKey) {
        (void) IOSC_DeleteObject(hDevicePubKey);
    }

    return rv;
}


#define ES_MAX_CERTS    32  /* need an arbitrary limit */

typedef struct {
    u8 isLeaf[ES_MAX_CERTS];
    void *leafCerts[ES_MAX_CERTS];
    void *chainCerts[ES_MAX_CERTS];
    IOSCName fullName;  /* fully qualified "Issuer-Subject" name */
} esVerifyCerts_big_locals;

ESError
esVerifyCerts(const void *certs[], u32 nCerts,
              const void *verifyChain[], u32 nVerifyCerts)
{
    ESError rv = ES_ERR_OK;
    u32 i, j;
    u32 nLeafCerts;
    u32 nChainCerts;
    u8 *subject;
    u8 *issuer;
    const char *separator;

    esVerifyCerts_big_locals *big_locals;
#ifdef __KERNEL__
    big_locals = kmalloc(sizeof(*big_locals), GFP_KERNEL);
    if (!big_locals)
        return ES_ERR_NO_MEMORY;
#else
    esVerifyCerts_big_locals _big_locals;
    big_locals = &_big_locals;
#endif

    if (nCerts + nVerifyCerts > ES_MAX_CERTS) {
        rv = ES_ERR_INVALID;
        goto end;
    }

    /*
     * These cases would fall through and return rv = ES_ERR_OK, but better to
     * fail and indicate that nothing useful happened.
     */
    if (nCerts == 0 || nCerts + nVerifyCerts == 0) {
        rv = ES_ERR_INVALID;
        goto end;
    }

    /*
     * If a verify chain is supplied, the processing is simpler.
     */
    if (verifyChain == NULL || nVerifyCerts == 0) {
        goto emptyChainCase;
    }

    for (i = 0; i < nCerts; i++) {
        rv = ESI_VerifyCert(certs[i], verifyChain, nVerifyCerts);
        if (rv != ES_ERR_OK) {
            goto end;
        }
    }

    rv = ES_ERR_OK;
    goto end;

emptyChainCase:
    /*
     * Handle the case of starting from scratch with an empty cert store.
     *
     * This is functionally equivalent to a topological sort on the certs using
     * 'x' is issuer of 'y' as the relationship and then finding the nodes with
     * no outgoing edges.  Note that this solution conserves memory and stack at
     * the expense of N**2 running time, but N is limited to ES_MAX_CERTS.  
     *
     * No assumption is made about the order of the certs in the input lists.
     */
    for (i = 0; i < nCerts; i++) {
        rv = ESI_GetCertNames(certs[i], &issuer, &subject);
        if (rv != ES_ERR_OK) {
            goto end;
        }

        /*
         * It is not safe to handle a self-signed certificate, unless the public key is already
         * known by the client code.  Anyone with OpenSSL can manufacture a consistent self-signed
         * certificate with whatever issuer and subject names are required.  Since this code
         * needs the root public keys to be baked in, there is no real point in dealing with root
         * certificates (the iGware root certs are not distributed for this reason).
         */
        if (strncmp((const char *) issuer, (const char *) subject, sizeof(IOSCName)) == 0) {
            esLog(ES_DEBUG_ERROR, "Self-signed certificate (root cert) rejected: %s\n", subject);
            rv = ES_ERR_VERIFICATION;
            goto end;
        }

        separator = "-";
        VPL_snprintf((char *) big_locals->fullName, sizeof(big_locals->fullName), "%s%s%s", 
            (const char *) issuer, (const char *) separator, (const char *) subject);
        for (j = 0; j < nCerts; j++) {
            if (j == i) continue;
            rv = ESI_GetCertNames(certs[j], &issuer, &subject);
            if (rv != ES_ERR_OK) {
                goto end;
            }
            if (strncmp((const char *) big_locals->fullName, (const char *) issuer, sizeof(big_locals->fullName)) == 0) {
                /*
                 * certs[i] is issuer of certs[j]
                 */
                big_locals->isLeaf[i] = 0;
                break;
            }
        }
        if (j == nCerts) {
            big_locals->isLeaf[i] = 1;
        }
    }

    nLeafCerts = 0;
    nChainCerts = 0;
    for (i = 0; i < nCerts; i++) {
        if (big_locals->isLeaf[i]) big_locals->leafCerts[nLeafCerts++] = (void *) certs[i];
        else big_locals->chainCerts[nChainCerts++] = (void *) certs[i];
    }

    for (i = 0; i < nLeafCerts; i++) {
        rv = ESI_VerifyCert(big_locals->leafCerts[i], (const void **) big_locals->chainCerts, nChainCerts);
        if (rv != ES_ERR_OK) {
            goto end;
        }
    }

end:
#ifdef __KERNEL__
    kfree(big_locals);
#endif
    return rv;
}
#endif  // ES_READ_ONLY

ES_NAMESPACE_END
