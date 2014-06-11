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


#ifndef __ES_TITLE_H__
#define __ES_TITLE_H__


#include "es_utils.h"

ES_NAMESPACE_START

void esResetTitleContext();


ESError esImportTitleInit(ESTitleId titleId,
                          IInputStream &ticket, IInputStream &tmd,
                          const void *certs[], u32 nCerts);

ESError esImportContentBegin(ESContentId contentId,
                             IInputStream &tmd,
                             const void *certs[], u32 nCerts);

ESError esImportContentData(const void *buf, u32 bufSize, void *outBuf);

ESError esImportContentCheckPoint(IOutputStream &outImportCtx);

ESError esImportContentResume(IInputStream &importCtx, u64 *outOffset);

ESError esImportContentEnd();

ESError esImportTitleDone();

ESError esGetImportContextSize(u32 *outImportCtxSize);


ESError esExportTitleInit(ESTitleId titleId,
                          IInputStream &ticket, IInputStream &tmd,
                          const void *certs[], u32 nCerts);

ESError esExportContentBegin(ESContentId contentId,
                             IInputStream &tmd,
                             const void *certs[], u32 nCerts);

ESError esExportContentData(const void *buf, u32 bufSize, void *outBuf);

ESError esExportContentCheckPoint(IOutputStream &outExportCtx);

ESError esExportContentResume(IInputStream &exportCtx, u64 *outOffset);

ESError esExportContentEnd();

ESError esExportTitleDone();

ESError esGetExportContextSize(u32 *outExportCtxSize);

ES_NAMESPACE_END

#endif  // __ES_TITLE_H__
