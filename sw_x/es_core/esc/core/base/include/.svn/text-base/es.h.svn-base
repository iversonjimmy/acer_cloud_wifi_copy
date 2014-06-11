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


#ifndef __ES_H__
#define __ES_H__


/*
 * Storage objects are used in the following three cases:
 *  1) Ticket and TMD (i.e. large data structures)
 *  2) Data for signing and verification (i.e. size depends on user input)
 *  3) Checkpointing the import/export context to non-volatile storage
 */

#include "estypes.h"
#include "istorage.h"

ES_NAMESPACE_START

class ETicket
{
    public:
        // Initialization and shutdown
        ETicket();
        ~ETicket();

        ESError Set(ISTORAGE_SYM(IInputStream) &ticket, 
                    const void *certs[], u32 nCerts, bool doVerify);

        // Ticket information
        ESError GetSize(u32 *outSize);
        ESError GetTicketId(ESTicketId *outTicketId);
        ESError GetTicketVersion(ESTicketVersion *outTicketVersion);
        ESError GetTitleId(ESTitleId *outTitleId);
        ESError GetDeviceId(ESDeviceId *outDeviceId);
        ESError GetPreInstallationFlag(bool *outFlag);
        ESError GetLicenseType(ESLicenseType *outLicenseType);
        ESError GetSysAccessMask(ESSysAccessMask *outSysAccessMask);
        ESError GetLimits(ESLpEntry *outLimits, u32 *outNumLimits);
        ESError GetItemRights(ESItemType type, u32 offset,
                              void *outItemRights, u32 *outNumItemRights,
                              u32 *outTotal);

        // Result of the last system operation
        Result GetLastSystemResult();
};


class TitleMetaData
{
    public:
        // Initialization and shutdown
        TitleMetaData();
        ~TitleMetaData();

        ESError Set(ISTORAGE_SYM(IInputStream) &tmd, const void *certs[], u32 nCerts,
                    bool doVerify);

        // TMD information
        ESError GetSize(u32 *outSize);
        ESError GetTitleId(ESTitleId *outTitleId);
        ESError GetTitleVersion(ESTitleVersion *outTitleVersion);
        ESError GetMinorTitleVersion(ESMinorTitleVersion *outMinorTitleVersion);
        ESError GetSysVersion(ESSysVersion *outSysVersion);
        ESError GetTitleType(ESTitleType *outTitleType);
        ESError GetGroupId(u16 *outGroupId);
        ESError GetAccessRights(u32 *outAccessRights);
        ESError GetBootContentIndex(ESContentIndex *outBootContentIndex);
        ESError GetCustomData(u8 *outCustomData, u32 *outCustomDataLen);
        ESError GetContentInfos(u32 offset, ESContentInfo *outContentInfos,
                                u32 *outNumContentInfos, u32 *outTotal);
        ESError FindContentInfos(ESContentIndex *indexes, u32 nIndexes,
                                 ESContentInfo *outContentInfos);

        // Result of the last system operation
        Result GetLastSystemResult();
};


class Certificate
{
    public:
        // Initialization and shutdown
        Certificate();
        ~Certificate();

        // Parse and merge certificate list
        ESError GetNumCertsInList(const void *certList, u32 certListSize,
                                  u32 *outNumCerts);
        ESError ParseCertList(const void *certList, u32 certListSize,
                              void *outCerts[], u32 *outCertSizes,
                              u32 *outNumCerts);
        ESError MergeCerts(const void *certs1[], u32 nCerts1,
                           const void *certs2[], u32 nCerts2,
                           void *outCerts[], u32 *outCertSizes,
                           u32 *outNumCerts);
};


class ETicketService
{
    public:
        // Initialization and shutdown
        ETicketService();
        ~ETicketService();

        // Import
        ESError ImportTicket(ISTORAGE_SYM(IInputStream) &ticket,
                             const void *certs[], u32 nCerts,
                             ISTORAGE_SYM(IOutputStream) &outTicket);

        ESError ImportTitleInit(ESTitleId titleId,
                                ISTORAGE_SYM(IInputStream) &ticket, 
                                ISTORAGE_SYM(IInputStream) &tmd,
                                const void *certs[], u32 nCerts);
        ESError ImportContentBegin(ESContentId contentId,
                                   ISTORAGE_SYM(IInputStream) &tmd,
                                   const void *certs[], u32 nCerts);
        ESError ImportContentData(const void *buf, u32 bufSize, void *outBuf);
        ESError ImportContentCheckPoint(ISTORAGE_SYM(IOutputStream) &outImportCtx);
        ESError ImportContentResume(ISTORAGE_SYM(IInputStream) &importCtx, u64 *outOffset);
        ESError ImportContentEnd();
        ESError ImportTitleDone();
        ESError GetImportContextSize(u32 *outImportCtxSize);

        // Export
        ESError ExportTicket(ISTORAGE_SYM(IInputStream) &ticket,
                             const void *certs[], u32 nCerts,
                             ISTORAGE_SYM(IOutputStream) &outTicket);

        ESError ExportTitleInit(ESTitleId titleId,
                                ISTORAGE_SYM(IInputStream) &ticket, 
                                ISTORAGE_SYM(IInputStream) &tmd,
                                const void *certs[], u32 nCerts);
        ESError ExportContentBegin(ESContentId contentId,
                                   ISTORAGE_SYM(IInputStream) &tmd,
                                   const void *certs[], u32 nCerts);
        ESError ExportContentData(const void *inBuf, u32 bufSize, void *outBuf);
        ESError ExportContentCheckPoint(ISTORAGE_SYM(IOutputStream) &outExportCtx);
        ESError ExportContentResume(ISTORAGE_SYM(IInputStream) &exportCtx, u64 *outOffset);
        ESError ExportContentEnd();
        ESError ExportTitleDone();
        ESError GetExportContextSize(u32 *outExportCtxSize);

        // Operations
        ESError Sign(ESTitleId signerTitleId, ISTORAGE_SYM(IInputStream) &data,
                     u32 dataSize, void *outSig, u32 outSigSize,
                     void *outCert, u32 outCertSize);
        ESError VerifySign(ESTitleId signerTitleId,
                           ISTORAGE_SYM(IInputStream) &data, u32 dataSize,
                           const void *sig, u32 sigSize, ESSigType sigType,
                           const void *certs[], u32 nCerts);
        ESError VerifyCerts(const void *certs[], u32 nCerts,
                            const void *verifyChain[], u32 nChainCerts);

        ESError GetDeviceCert(void *outDeviceCert, u32 outDeviceCertSize);
        ESError GetDeviceId(ESDeviceId *outDevId);

        // Result of the last system operation
        Result GetLastSystemResult();
};

ES_NAMESPACE_END

#endif  // __ES_H__
