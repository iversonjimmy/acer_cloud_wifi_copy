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


#include "es_platform.h"
#include "istorage.h"
#include "esi.h"
#include "es_storage.h"
#ifdef _KERNEL_MODULE
#include "core_glue.h"
#endif

static Result __result;


ESError
esSeek(IInputStream *reader, u32 pos)
{
    ESError rv = ES_ERR_OK;

    __result = rdrTrySetPosition(reader, pos);
    if (!ResultIsSuccess(__result)) {
        esLog(ES_DEBUG_ERROR, "Failed to seek to %u\n", pos);
        rv = ES_ERR_STORAGE;
        goto end;
    }

end:
    return rv;
}

ESError
esRead(IInputStream *reader, u32 size, void *outBuf)
{
    ESError rv = ES_ERR_OK;
    u32 pOut;

    __result = rdrTryRead(reader, &pOut, outBuf, size);
    if (!ResultIsSuccess(__result)) {
        esLog(ES_DEBUG_ERROR, "Failed to read %u bytes\n", size);
        rv = ES_ERR_STORAGE;
        goto end;
    }

    if (pOut != size) {
        esLog(ES_DEBUG_ERROR, "read: Unexpected size %u:%u\n", pOut, size);
        rv = ES_ERR_STORAGE_SIZE;
        goto end;
    }

end:
    return rv;
}


ESError
esWrite(IOutputStream *writer, u32 size, const void *buf)
{
    ESError rv = ES_ERR_OK;
    u32 pOut;

    __result = wrtrTryWrite(writer, &pOut, buf, size);
    if (!ResultIsSuccess(__result)) {
        esLog(ES_DEBUG_ERROR, "Failed to write %u bytes\n", size);
        rv = ES_ERR_STORAGE;
        goto end;
    }

    if (pOut != size) {
        esLog(ES_DEBUG_ERROR, "write: Unexpected size %u:%u\n", pOut, size);
        rv = ES_ERR_STORAGE_SIZE;
        goto end;
    }

end:
    return rv;
}


Result
esGetLastSystemResult()
{
    return __result;
}
