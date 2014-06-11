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


#ifndef __ISTORAGE_H__
#define __ISTORAGE_H__

#include <core_glue.h>

typedef s32 Result;
#define ResultIsSuccess(res)	(res == 0)
#define ResultGetSummary(res)	(res);
#define ResulSet(res, val)	res = val;

typedef struct iStorage_s IInputStream;
// #define IOutputStream IInputStream
typedef struct iStorage_s IOutputStream;

Result rdrTryRead(IInputStream *rdr, u32 *pOut, void *buffer, u32 size);
Result rdrTrySetPosition(IInputStream *rdr, s64 position);
Result rdrTryGetPosition(IInputStream *rdr, s64 *pOut);
Result rdrTryGetSize(IInputStream *rdr, s64 *pOut);
Result wrtrTryWrite(IOutputStream *wrtr, u32 *pOut, const void *buffer, u32 size);
Result wrtrTrySetPosition(IOutputStream *wrtr, s64 position);
Result wrtrTryGetPosition(IOutputStream *wrtr, s64 *pOut);
Result wrtrTryGetSize(IOutputStream *wrtr, s64 *pOut);

#define USING_ISTORAGE_NAMESPACE
#define RESULT_NS(sym) sym

#endif // __ISTORAGE_H__
