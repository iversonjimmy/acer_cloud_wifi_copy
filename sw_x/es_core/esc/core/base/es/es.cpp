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
#include <iosc.h>
#include <es.h>

USING_ES_NAMESPACE
USING_ISTORAGE_NAMESPACE

#include "es_container.h"
#include "es_device.h"
#include "es_storage.h"
#include "es_ticket.h"
#include "es_title.h"


ES_NAMESPACE_START


ETicketService::ETicketService()
{
    IOSC_Initialize();
}


ETicketService::~ETicketService()
{
    esResetTitleContext();
}


ESError
ETicketService::ImportTicket(IInputStream &ticket,
                             const void *certs[], u32 nCerts,
                             IOutputStream &outTicket)
{
    return esImportTicket(ticket, certs, nCerts, outTicket);
}


ESError
ETicketService::ImportTitleInit(ESTitleId titleId,
                                IInputStream &ticket, IInputStream &tmd,
                                const void *certs[], u32 nCerts)
{
    return esImportTitleInit(titleId, ticket, tmd, certs, nCerts);
}


ESError
ETicketService::ImportContentBegin(ESContentId contentId,
                                   IInputStream &tmd,
                                   const void *certs[], u32 nCerts)
{
    return esImportContentBegin(contentId, tmd, certs, nCerts);
}


ESError
ETicketService::ImportContentData(const void *buf, u32 bufSize, void *outBuf)
{
    return esImportContentData(buf, bufSize, outBuf);
}


ESError
ETicketService::ImportContentCheckPoint(IOutputStream &outImportCtx)
{
    return esImportContentCheckPoint(outImportCtx);
}


ESError
ETicketService::ImportContentResume(IInputStream &outImportCtx,
                                    u64 *outOffset)
{
    return esImportContentResume(outImportCtx, outOffset);
}


ESError
ETicketService::ImportContentEnd()
{
    return esImportContentEnd();
}


ESError
ETicketService::ImportTitleDone()
{
    return esImportTitleDone();
}


ESError
ETicketService::GetImportContextSize(u32 *outImportCtxSize)
{
    return esGetImportContextSize(outImportCtxSize);
}


ESError
ETicketService::ExportTicket(IInputStream &ticket,
                             const void *certs[], u32 nCerts,
                             IOutputStream &outTicket)
{
    return esExportTicket(ticket, certs, nCerts, outTicket);
}


ESError
ETicketService::ExportTitleInit(ESTitleId titleId,
                                IInputStream &ticket, IInputStream &tmd,
                                const void *certs[], u32 nCerts)
{
    return esExportTitleInit(titleId, ticket, tmd, certs, nCerts);
}


ESError
ETicketService::ExportContentBegin(ESContentId contentId,
                                   IInputStream &tmd,
                                   const void *certs[], u32 nCerts)
{
    return esExportContentBegin(contentId, tmd, certs, nCerts);
}


ESError
ETicketService::ExportContentData(const void *buf, u32 bufSize, void *outBuf)
{
    return esExportContentData(buf, bufSize, outBuf);
}


ESError
ETicketService::ExportContentCheckPoint(IOutputStream &outExportCtx)
{
    return esExportContentCheckPoint(outExportCtx);
}


ESError
ETicketService::ExportContentResume(IInputStream &outExportCtx,
                                    u64 *outOffset)
{
    return esExportContentResume(outExportCtx, outOffset);
}


ESError
ETicketService::ExportContentEnd()
{
    return esExportContentEnd();
}


ESError
ETicketService::ExportTitleDone()
{
    return esExportTitleDone();
}


ESError
ETicketService::GetExportContextSize(u32 *outExportCtxSize)
{
    return esGetExportContextSize(outExportCtxSize);
}


ESError
ETicketService::Sign(ESTitleId signerTitleId,
                     IInputStream &data, u32 dataSize,
                     void *outSig, u32 outSigSize,
                     void *outCert, u32 outCertSize)
{
    return esSign(signerTitleId, data, dataSize, outSig, outSigSize, outCert, outCertSize);
}


ESError
ETicketService::VerifySign(ESTitleId signerTitleId,
                           ISTORAGE_SYM(IInputStream) &data, u32 dataSize,
                           const void *sig, u32 sigSize, ESSigType sigType,
                           const void *certs[], u32 nCerts)
{
    return esVerifySign(signerTitleId, data, dataSize, sig, sigSize, sigType, certs, nCerts);
}


ESError
ETicketService::VerifyCerts(const void *certs[], u32 nCerts,
                            const void *verifyChain[], u32 nChainCerts)
{
    return esVerifyCerts(certs, nCerts, verifyChain, nChainCerts);
}


ESError
ETicketService::GetDeviceCert(void *outDeviceCert, u32 outDeviceCertSize)
{
    return esGetDeviceCert(outDeviceCert, outDeviceCertSize);
}


ESError
ETicketService::GetDeviceId(ESDeviceId *outDeviceId)
{
    return esGetDeviceId(outDeviceId);
}


Result
ETicketService::GetLastSystemResult()
{
    return esGetLastSystemResult();
}

ES_NAMESPACE_END
