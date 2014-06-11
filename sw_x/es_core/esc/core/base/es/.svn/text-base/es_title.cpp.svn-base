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


#include "es_utils.h"
#include <estypes.h>
#include <iosc.h>

USING_ES_NAMESPACE
USING_ISTORAGE_NAMESPACE

#include "es_container.h"
#include "es_device.h"
#include "es_storage.h"
#include "es_title.h"


typedef u32 ESTitleOperation;

#define ES_TITLE_OPERATION_IMPORT   1
#define ES_TITLE_OPERATION_EXPORT   2


ES_NAMESPACE_START


#pragma pack(push, 4)

typedef struct {
    ESVersion           version;
    ESTitleOperation    op;
    ESTitleId           titleId;
    ESContentId         contentId;
    IOSCHashContext     hashCtx;
    IOSCAesIv           iv;
    u64                 processedSize;
    IOSCEccEccCert      signingCert;
    IOSCEccSig          signature;
} ESImportContext;

typedef ESImportContext ESExportContext;

typedef struct {
    bool                valid;
    ESV1ContentMeta     cmd;
    IOSCHashContext     hashCtx;
    IOSCAesIv           iv;
    u64                 processedSize;
} ESContentContext;

typedef struct {
    bool                valid;
    ESTitleOperation    op;
    ESTitleId           titleId;
    IOSCSecretKeyHandle hTitleKey;
    ESContentContext    contentCtx;
} ESTitleContext;

#pragma pack(pop)

static ESTitleContext __titleCtx ATTR_SHA_ALIGN = { false };

// ESImportContext == ESExportContent
static ESImportContext __importCtx ATTR_SHA_ALIGN;

// Title key buffer
static u8 __keyBuf[SIZE_AES_ALIGN(sizeof(IOSCAesKey))] ATTR_AES_ALIGN;

// IV buffer
static u8 __ivBuf[SIZE_AES_ALIGN(sizeof(IOSCAesIv))] ATTR_AES_ALIGN;


/*
 * Buffer size >= max(sizeof(ESTicket) + sizeof(ESV1TicketHeader),
 *                    sizeof(ESV1TitleMeta),
 *                    sizeof(ESImportContext),
 *                    sizeof(IOSCEccEccCert))
 */
static u8 __buf[SIZE_SHA_ALIGN(4096)] ATTR_SHA_ALIGN;


static ESError
__importTitleInit(ESTitleId titleId,
                  IInputStream &ticket, IInputStream &tmd,
                  const void *certs[], u32 nCerts,
                  ESTitleOperation op)
{
    ESError rv = ES_ERR_OK;
    ESTicket *tp;
    ESV1TitleMeta *mp;
    IOSCSecretKeyHandle comKey;

    /*
     * Since the compiler can evaluate this conditional, it doesn't
     * generate any runtime code unless it is going to fail.
     */
    if (sizeof(__titleCtx) != SIZE_AES_ALIGN(sizeof(__titleCtx))) {
        esLog(ES_DEBUG_ERROR, "Invalid data structure size\n");
        rv = ES_ERR_INVALID;
        goto end;
    }

    if (certs == NULL || nCerts == 0 || __titleCtx.valid) {
        esLog(ES_DEBUG_ERROR, "Invalid arguments or state\n");
        rv = ES_ERR_INVALID;
        goto end;
    }

    // Verify the ticket
    if ((rv = esVerifyTicket(ticket, certs, nCerts, true, NULL, NULL, __buf)) != ES_ERR_OK) {
        goto end;
    }
    tp = (ESTicket *) __buf;

    if (ntohll(tp->titleId) != titleId) {
        esLog(ES_DEBUG_ERROR, "Title ID doesn't match, 0x%16llx:0x%16llx\n", ntohll(tp->titleId), titleId);
        rv = ES_ERR_INVALID;
        goto end;
    }

    // Decrypt the title key
    if ((rv = IOSC_CreateObject(&__titleCtx.hTitleKey, IOSC_SECRETKEY_TYPE, IOSC_ENC_SUBTYPE)) != IOSC_ERROR_OK) {
        esLog(ES_DEBUG_ERROR, "Failed to create object for title key, rv=%d\n", rv);
        rv = ESI_Translate_IOSC_Error(rv);
        goto end;
    }

    // Use the common key id from the ticket to determine which common key
    comKey = IOSC_COMMON_KEYID_TO_HANDLE(tp->keyId);

    memset(__ivBuf, 0, sizeof(IOSCAesIv));
    memcpy(__ivBuf, (u8 *) &tp->titleId, sizeof(ESTitleId));
    memcpy(__keyBuf, tp->titleKey, sizeof(IOSCAesKey));

    if ((rv = IOSC_ImportSecretKey(__titleCtx.hTitleKey, 0, comKey, IOSC_NOSIGN_ENC, NULL, __ivBuf, __keyBuf)) != IOSC_ERROR_OK) {
        esLog(ES_DEBUG_ERROR, "Failed to import secret key, rv=%d\n", rv);
        rv = ESI_Translate_IOSC_Error(rv);
        goto end_delete;
    }

    // Verify the TMD
    if ((rv = esVerifyTmd(tmd, certs, nCerts, 0, (ESV1TitleMeta *) __buf, NULL)) != ES_ERR_OK) {
        goto end_delete;
    }
    mp = (ESV1TitleMeta *) __buf;

    if (ntohll(mp->head.titleId) != titleId) {
        esLog(ES_DEBUG_ERROR, "Title ID doesn't match, 0x%16llx:0x%16llx\n", ntohll(mp->head.titleId), titleId);
        rv = ES_ERR_INVALID;
        goto end_delete;
    }

    __titleCtx.op = op;
    __titleCtx.titleId = titleId;
    __titleCtx.contentCtx.valid = false;
    __titleCtx.valid = true;

end_delete:
    if (rv != ES_ERR_OK) {
        (void) IOSC_DeleteObject(__titleCtx.hTitleKey);
    }

end:
    return rv;
}


static ESError
__importContentBegin(ESContentId contentId,
                     IInputStream &tmd,
                     const void *certs[], u32 nCerts,
                     ESTitleOperation op)
{
    ESError rv = ES_ERR_OK;
    ESV1TitleMeta *mp;
    ESContentIndex index;

    if (certs == NULL || nCerts == 0 || !__titleCtx.valid || __titleCtx.contentCtx.valid) {
        esLog(ES_DEBUG_ERROR, "Invalid arguments or state\n");
        rv = ES_ERR_INVALID;
        goto end;
    }

    // Import is exactly the same as export
    (void) op;

    // Verify the TMD
    if ((rv = esVerifyTmd(tmd, certs, nCerts, contentId, (ESV1TitleMeta *) __buf, &__titleCtx.contentCtx.cmd)) != ES_ERR_OK) {
        goto end;
    }
    mp = (ESV1TitleMeta *) __buf;

    if (ntohll(mp->head.titleId) != __titleCtx.titleId) {
        esLog(ES_DEBUG_ERROR, "Title ID doesn't match, 0x%16llx:0x%16llx\n", ntohll(mp->head.titleId), __titleCtx.titleId);
        rv = ES_ERR_INVALID;
        goto end;
    }

    // Initialize the hash context
    if ((rv = IOSC_GenerateHash(__titleCtx.contentCtx.hashCtx, NULL, 0, IOSC_SHA256_INIT, NULL)) != IOSC_ERROR_OK) {
        esLog(ES_DEBUG_ERROR, "Failed to generate hash - init, rv=%d\n", rv);
        rv = ESI_Translate_IOSC_Error(rv);
        goto end;
    }

    // Initialize the IV
    index = __titleCtx.contentCtx.cmd.index;
    memset(__titleCtx.contentCtx.iv, 0, sizeof(IOSCAesIv));
    memcpy(__titleCtx.contentCtx.iv, (u8 *) &index, sizeof(ESContentIndex));

    __titleCtx.contentCtx.processedSize = 0;
    __titleCtx.contentCtx.valid = true;

end:
    return rv;
}


static ESError
__importContentData(const void *buf, u32 bufSize, void *outBuf, ESTitleOperation op)
{
    ESError rv = ES_ERR_OK;
    u32 actualSize;

    if (buf == NULL || bufSize == 0 || outBuf == NULL || !__titleCtx.valid || !__titleCtx.contentCtx.valid || __titleCtx.contentCtx.processedSize >= ntohll(__titleCtx.contentCtx.cmd.size)) {
        esLog(ES_DEBUG_ERROR, "Invalid arguments or state\n");
        rv = ES_ERR_INVALID;
        goto end;
    }

    /*
     * If the last block isn't aligned, then the actual size is smaller
     * than the input buffer size
     */
    if ((__titleCtx.contentCtx.processedSize + bufSize) > ntohll(__titleCtx.contentCtx.cmd.size)) {
        actualSize = ntohll(__titleCtx.contentCtx.cmd.size) - __titleCtx.contentCtx.processedSize;
    } else {
        actualSize = bufSize;
    }
    __titleCtx.contentCtx.processedSize += actualSize;

    // Hash and encrypt/decrypt content data
    memcpy(__ivBuf, __titleCtx.contentCtx.iv, sizeof(IOSCAesIv));
    if (op == ES_TITLE_OPERATION_EXPORT) {
        if ((rv = IOSC_GenerateHash(__titleCtx.contentCtx.hashCtx,
                                    (u8 *) buf, actualSize,
                                    IOSC_SHA256_UPDATE, NULL)) != IOSC_ERROR_OK) {
            esLog(ES_DEBUG_ERROR, "Failed to generate hash - update, rv=%d\n", rv);
            rv = ESI_Translate_IOSC_Error(rv);
            goto end;
        }

        // For export, assume that the input buffer is already padded
        if (ntohs(__titleCtx.contentCtx.cmd.type) & ES_CONTENT_TYPE_ENCRYPTED) {
            if ((rv = IOSC_Encrypt(__titleCtx.hTitleKey, __ivBuf,
                                   (u8 *) buf, bufSize,
                                   (u8 *) outBuf)) != IOSC_ERROR_OK) {
                esLog(ES_DEBUG_ERROR, "Failed to encrypt content data, rv=%d\n", rv);
                rv = ESI_Translate_IOSC_Error(rv);
                goto end;
            }
        } else {
            memcpy((u8 *) outBuf, (u8 *) buf, bufSize);
        }
    } else {
        if (ntohs(__titleCtx.contentCtx.cmd.type) & ES_CONTENT_TYPE_ENCRYPTED) {
            if ((rv = IOSC_Decrypt(__titleCtx.hTitleKey, __ivBuf,
                                   (u8 *) buf, bufSize,
                                   (u8 *) outBuf)) != IOSC_ERROR_OK) {
                esLog(ES_DEBUG_ERROR, "Failed to decrypt content data, rv=%d\n", rv);
                rv = ESI_Translate_IOSC_Error(rv);
                goto end;
            }
        } else {
            memcpy((u8 *) outBuf, (u8 *) buf, bufSize);
        }

        if ((rv = IOSC_GenerateHash(__titleCtx.contentCtx.hashCtx,
                                    (u8 *) outBuf, actualSize,
                                    IOSC_SHA256_UPDATE, NULL)) != IOSC_ERROR_OK) {
            esLog(ES_DEBUG_ERROR, "Failed to generate hash - update, rv=%d\n", rv);
            rv = ESI_Translate_IOSC_Error(rv);
            goto end;
        }
    }
    memcpy(__titleCtx.contentCtx.iv, __ivBuf, sizeof(IOSCAesIv));

end:
    return rv;
}


static ESError
__importContentCheckPoint(IOutputStream &outImportCtx, ESTitleOperation op)
{
    ESError rv = ES_ERR_OK;
    u32 sizeToSign;

    if (!__titleCtx.valid || !__titleCtx.contentCtx.valid) {
        esLog(ES_DEBUG_ERROR, "Invalid arguments or state\n");
        rv = ES_ERR_INVALID;
        goto end;
    }

    // Construct and save the import/export context
    memset(&__importCtx, 0, sizeof(__importCtx));
    __importCtx.op = op;
    __importCtx.version = 1;
    __importCtx.titleId = __titleCtx.titleId;
    __importCtx.contentId = ntohl(__titleCtx.contentCtx.cmd.cid);
    memcpy(__importCtx.hashCtx, __titleCtx.contentCtx.hashCtx, sizeof(IOSCHashContext));
    memcpy(__importCtx.iv, __titleCtx.contentCtx.iv, sizeof(IOSCAesIv));
    __importCtx.processedSize = __titleCtx.contentCtx.processedSize;

    // Sign the data
    sizeToSign = sizeof(__importCtx) - sizeof(__importCtx.signingCert) - sizeof(__importCtx.signature);
    memcpy(__buf, &__importCtx, sizeToSign);
    if ((rv = esSignData(0, __buf, sizeToSign, &__importCtx.signature, &__importCtx.signingCert)) != ES_ERR_OK) {
        goto end;
    }

    // Write out the entire import/export context
    if ((rv = esSeek(outImportCtx, 0)) != ES_ERR_OK) {
        goto end;
    }

    if ((rv = esWrite(outImportCtx, sizeof(__importCtx), &__importCtx)) != ES_ERR_OK) {
        goto end;
    }

end:
    return rv;
}


static ESError
__importContentResume(IInputStream &importCtx, u64 *outOffset, ESTitleOperation op)
{
    ESError rv = ES_ERR_OK;
    IOSCEccEccCert *deviceCert;
    IOSCPublicKeyHandle hDevicePubKey = 0;
    u32 sizeToVerify;

    if (!__titleCtx.valid || !__titleCtx.contentCtx.valid || __titleCtx.contentCtx.processedSize != 0 || outOffset == NULL) {
        esLog(ES_DEBUG_ERROR, "Invalid arguments or state\n");
        rv = ES_ERR_INVALID;
        goto end;
    }

    // Read and de-compose the import/export context
    if ((rv = esSeek(importCtx, 0)) != ES_ERR_OK) {
        goto end;
    }

    memset(&__importCtx, 0, sizeof(__importCtx));
    if ((rv = esRead(importCtx, sizeof(__importCtx), &__importCtx)) != ES_ERR_OK) {
        goto end;
    }

    // Verify the import/export context before use
    if ((rv = esGetDeviceCert(__buf, sizeof(IOSCEccSignedCert))) != ES_ERR_OK) {
        goto end;
    }
    deviceCert = (IOSCEccEccCert *) __buf;

    if ((rv = IOSC_CreateObject(&hDevicePubKey, IOSC_PUBLICKEY_TYPE, IOSC_ECC233_SUBTYPE)) != IOSC_ERROR_OK) {
        esLog(ES_DEBUG_ERROR, "Failed to create object for device public key, rv=%d\n", rv);
        rv = ESI_Translate_IOSC_Error(rv);
        goto end;
    }

    if ((rv = IOSC_ImportPublicKey(deviceCert->pubKey, NULL, hDevicePubKey)) != ES_ERR_OK) {
        esLog(ES_DEBUG_ERROR, "Failed to import device public key, rv=%d\n", rv);
        rv = ESI_Translate_IOSC_Error(rv);
        goto end;
    }

    memcpy(__buf, &__importCtx.signingCert, sizeof(__importCtx.signingCert));
    sizeToVerify = sizeof(__importCtx) - sizeof(__importCtx.signingCert) - sizeof(__importCtx.signature);
    if ((rv = esVerifyDeviceSign(0, importCtx, sizeToVerify, &__importCtx.signature, ES_SIG_TYPE_ECC_SHA256, &__importCtx.signingCert, hDevicePubKey)) != ES_ERR_OK) {
        goto end;
    }

    if (__importCtx.version != 1 || __importCtx.op != op || __importCtx.titleId != __titleCtx.titleId || __importCtx.contentId != ntohl(__titleCtx.contentCtx.cmd.cid)) {
        esLog(ES_DEBUG_ERROR, "Invalid import context %hhu %u, 0x%16llx, %u\n", __importCtx.version, __importCtx.op, __importCtx.titleId, __importCtx.contentId);
        rv = ES_ERR_INVALID;
        goto end;
    }

    memcpy(__titleCtx.contentCtx.hashCtx, __importCtx.hashCtx, sizeof(IOSCHashContext));
    memcpy(__titleCtx.contentCtx.iv, __importCtx.iv, sizeof(IOSCAesIv));
    __titleCtx.contentCtx.processedSize = __importCtx.processedSize;

    // Set the underlying hash context
    if ((rv = IOSC_GenerateHash(__titleCtx.contentCtx.hashCtx, NULL, 0, IOSC_SHA256_RESTART, NULL)) != IOSC_ERROR_OK) {
        esLog(ES_DEBUG_ERROR, "Failed to generate hash - restart, rv=%d\n", rv);
        rv = ESI_Translate_IOSC_Error(rv);
        goto end;
    }

    *outOffset = __importCtx.processedSize;

end:
    if (hDevicePubKey) {
        (void) IOSC_DeleteObject(hDevicePubKey);
    }

    return rv;
}


static ESError
__importContentEnd(ESTitleOperation op)
{
    ESError rv = ES_ERR_OK;
    IOSCHash256 hash;

    // Import is exactly the same as export
    (void) op;

    if (!__titleCtx.valid || !__titleCtx.contentCtx.valid || __titleCtx.contentCtx.processedSize != ntohll(__titleCtx.contentCtx.cmd.size)) {
        esLog(ES_DEBUG_ERROR, "Invalid arguments or state\n");
        rv = ES_ERR_INVALID;
        goto end;
    }

    // Verify the content hash
    if ((rv = IOSC_GenerateHash(__titleCtx.contentCtx.hashCtx, NULL, 0, IOSC_SHA256_FINAL, hash)) != IOSC_ERROR_OK) {
        esLog(ES_DEBUG_ERROR, "Failed to generate hash - final, rv=%d\n", rv);
        rv = ESI_Translate_IOSC_Error(rv);
        goto end;
    }

    if (memcmp(hash, __titleCtx.contentCtx.cmd.hash, sizeof(IOSCHash256)) != 0) {
        esLog(ES_DEBUG_ERROR, "Failed to verify content hash\n");
        rv = ES_ERR_VERIFICATION;
        goto end;
    }

    __titleCtx.contentCtx.valid = false;

end:
    return rv;
}


static ESError
__importTitleDone(ESTitleOperation op)
{
    ESError rv = ES_ERR_OK;

    // Import is exactly the same as export
    (void) op;

    if (!__titleCtx.valid) {
        esLog(ES_DEBUG_ERROR, "Invalid arguments or state\n");
        rv = ES_ERR_INVALID;
        goto end;
    }

    (void) IOSC_DeleteObject(__titleCtx.hTitleKey);
    __titleCtx.valid = false;

end:
    return rv;
}


static ESError
__getImportContextSize(u32 *outImportCtxSize, ESTitleOperation op)
{
    ESError rv = ES_ERR_OK;

    // Import is exactly the same as export
    (void) op;

    if (outImportCtxSize == NULL) {
        esLog(ES_DEBUG_ERROR, "Invalid arguments\n");
        rv = ES_ERR_INVALID;
        goto end;
    }

    *outImportCtxSize = sizeof(ESImportContext);

end:
    return rv;
}


void
esResetTitleContext()
{
    __titleCtx.valid = false;
}


ESError
esImportTitleInit(ESTitleId titleId,
                  IInputStream &ticket, IInputStream &tmd,
                  const void *certs[], u32 nCerts)
{
    return __importTitleInit(titleId, ticket, tmd, certs, nCerts, ES_TITLE_OPERATION_IMPORT);
}


ESError
esImportContentBegin(ESContentId contentId,
                     IInputStream &tmd,
                     const void *certs[], u32 nCerts)
{
    return __importContentBegin(contentId, tmd, certs, nCerts, ES_TITLE_OPERATION_IMPORT);
}


ESError
esImportContentData(const void *buf, u32 bufSize, void *outBuf)
{
    return __importContentData(buf, bufSize, outBuf, ES_TITLE_OPERATION_IMPORT);
}


ESError
esImportContentCheckPoint(IOutputStream &outImportCtx)
{
    return __importContentCheckPoint(outImportCtx, ES_TITLE_OPERATION_IMPORT);
}


ESError
esImportContentResume(IInputStream &importCtx, u64 *outOffset)
{
    return __importContentResume(importCtx, outOffset, ES_TITLE_OPERATION_IMPORT);
}


ESError
esImportContentEnd()
{
    return __importContentEnd(ES_TITLE_OPERATION_IMPORT);
}


ESError
esImportTitleDone()
{
    return __importTitleDone(ES_TITLE_OPERATION_IMPORT);
}


ESError
esGetImportContextSize(u32 *outImportCtxSize)
{
    return __getImportContextSize(outImportCtxSize, ES_TITLE_OPERATION_IMPORT);
}


ESError
esExportTitleInit(ESTitleId titleId,
                  IInputStream &ticket, IInputStream &tmd,
                  const void *certs[], u32 nCerts)
{
    return __importTitleInit(titleId, ticket, tmd, certs, nCerts, ES_TITLE_OPERATION_EXPORT);
}


ESError
esExportContentBegin(ESContentId contentId,
                     IInputStream &tmd,
                     const void *certs[], u32 nCerts)
{
    return __importContentBegin(contentId, tmd, certs, nCerts, ES_TITLE_OPERATION_EXPORT);
}


ESError
esExportContentData(const void *buf, u32 bufSize, void *outBuf)
{
    return __importContentData(buf, bufSize, outBuf, ES_TITLE_OPERATION_EXPORT);
}


ESError
esExportContentCheckPoint(IOutputStream &outExportCtx)
{
    return __importContentCheckPoint(outExportCtx, ES_TITLE_OPERATION_EXPORT);
}


ESError
esExportContentResume(IInputStream &exportCtx, u64 *outOffset)
{
    return __importContentResume(exportCtx, outOffset, ES_TITLE_OPERATION_EXPORT);
}


ESError
esExportContentEnd()
{
    return __importContentEnd(ES_TITLE_OPERATION_EXPORT);
}


ESError
esExportTitleDone()
{
    return __importTitleDone(ES_TITLE_OPERATION_EXPORT);
}


ESError
esGetExportContextSize(u32 *outExportCtxSize)
{
    return __getImportContextSize(outExportCtxSize, ES_TITLE_OPERATION_EXPORT);
}

ES_NAMESPACE_END
