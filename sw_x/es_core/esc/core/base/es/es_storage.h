/*
 *               Copyright (C) 2009, BroadOn Communications Corp.
 *
 *  These coded instructions, statements, and computer programs contain
 *  unpublished  proprietary information of BroadOn Communications Corp.,
 *  and  are protected by Federal copyright law. They may not be disclosed
 *  to  third  parties or copied or duplicated in any form, in whole or in
 *  part, without the prior written consent of BroadOn Communications Corp.
 *
 */


#ifndef __ES_STORAGE_H__
#define __ES_STORAGE_H__


#include "es_utils.h"

#if defined(__cplusplus) && defined(GVM) && !defined(CPP_ES)
extern "C" {
#endif

ES_NAMESPACE_START

ESError esSeek(IInputStream __REFVSPTR reader, u32 pos);

#if defined(__cplusplus) && (!defined(GVM) || defined(CPP_ES))
ESError esSeek(IOutputStream __REFVSPTR writer, u32 pos);
#endif // __cplusplus


ESError esRead(IInputStream __REFVSPTR reader, u32 size, void *outBuf);

ESError esWrite(IOutputStream __REFVSPTR writer, u32 size, const void *buf);


Result esGetLastSystemResult(void);

ES_NAMESPACE_END

#if defined(__cplusplus) && defined(GVM) && !defined(CPP_ES)
} // extern "C"
#endif

#endif  // __ES_STORAGE_H__
