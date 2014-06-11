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


#ifndef __ES_CONTAINER_H__
#define __ES_CONTAINER_H__


#include "es_utils.h"
#include <esi.h>

#if defined(__cplusplus) && defined(GVM) && !defined(CPP_ES)
extern "C" {
#endif

ES_NAMESPACE_START

typedef struct {
    IOutputStream     *ticketDest;
    bool                isImport;
} ESTicketWriter;

typedef struct {
    ESItemType          itemType;
    u32                 itemSize;
    u32                 sectHdrsOffset;
    u32                 sectHdrsSize;
    ESV1SectionHeader  *sectHdrs;
    u32                 rightsOffset;
    u32                 nRights;
    void               *outRights;
} ESItemRightsGet;


typedef struct {
    ESContentIndex     *indexes;
    u32                 nIndexes;
    ESContentInfo      *outInfos;
    u32                 nFound;
} ESContentMetaSearchByIndex;

typedef struct {
    ESContentId         id;
    ESV1ContentMeta    *outCmd;
    bool                found;
} ESContentMetaSearchById;

typedef struct {
    u32                 offset;
    u32                 nInfos;
    ESContentInfo      *outInfos;
} ESContentMetaGet;


ESError esVerifyTicket(IInputStream __REFVSPTR ticket,
                       const void *certs[], u32 nCerts,
                       bool doVerify, ESTicketWriter *ticketWriter,
                       ESItemRightsGet *itemRightsGet, void *outTicket);

ESError esVerifyTmdFixed(IInputStream __REFVSPTR tmd,
                         const void *certs[], u32 nCerts,
                         ESV1TitleMeta *outTmd);

ESError esVerifyCmdGroup(IInputStream __REFVSPTR tmd,
                         ESV1ContentMetaGroup *group,
                         u32 groupIndex, bool doVerify,
                         ESContentMetaSearchByIndex *cmdSearchByIndex,
                         ESContentMetaSearchById *cmdSearchById,
                         ESContentMetaGet *cmdGet);

ESError esVerifyTmd(IInputStream __REFVSPTR tmd,
                    const void *certs[], u32 nCerts,
                    ESContentId contentId,
                    ESV1TitleMeta *outTmd, ESV1ContentMeta *outCmd);

ESError
esVerifyTmdContent(IInputStream __REFVSPTR tmd, ESContentId contentId,
		           ESV1TitleMeta *inTmd, ESV1ContentMeta *outCmd);

ESError esSign(ESTitleId signerTitleId,
               IInputStream __REFVSPTR data, u32 dataSize,
               void *outSig, u32 outSigSize, void *outCert, u32 outCertSize);

ESError esVerifyCerts(const void *certs[], u32 nCerts,
                      const void *verifyChain[], u32 nChainCerts);

ESError esSignData(ESTitleId signerTitleId, const void *data, u32 dataSize,
                   IOSCEccSig *outSig, IOSCEccEccCert *outCert);

ESError esVerifyDeviceSign(ESTitleId signerTitleId,
                           IInputStream __REFVSPTR data, u32 dataSize,
                           const IOSCEccSig *sig, ESSigType sigType,
                           const IOSCEccEccCert *signingCert,
                           IOSCPublicKeyHandle hDevicePubKey);

ESError esVerifySign(ESTitleId signerTitleId,
                     IInputStream __REFVSPTR data, u32 dataSize,
                     const void *sig, u32 sigSize, ESSigType sigType,
                     const void *certs[], u32 nCerts);

ES_NAMESPACE_END

#if defined(__cplusplus) && defined(GVM) && !defined(CPP_ES)
} // extern "C"
#endif

#endif  // __ES_CONTAINER_H__
