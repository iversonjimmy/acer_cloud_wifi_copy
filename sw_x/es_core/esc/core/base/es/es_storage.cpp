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


#include <estypes.h>
#include <istorage.h>

USING_ES_NAMESPACE
USING_ISTORAGE_NAMESPACE

#include "esi.h"
#include "es_storage.h"

ES_NAMESPACE_START


static Result __result;


ESError
esSeek(IInputStream &reader, u32 pos)
{
    ESError rv = ES_ERR_OK;

    __result = reader.TrySetPosition(pos);
    if (!__result.IsSuccess()) {
        esLog(ES_DEBUG_ERROR, "Failed to seek to %u\n", pos);
        rv = ES_ERR_STORAGE;
        goto end;
    }

end:
    return rv;
}


ESError
esSeek(IOutputStream &writer, u32 pos)
{
    ESError rv = ES_ERR_OK;

    __result = writer.TrySetPosition(pos);
    if (!__result.IsSuccess()) {
        esLog(ES_DEBUG_ERROR, "Failed to seek to %u\n", pos);
        rv = ES_ERR_STORAGE;
        goto end;
    }

end:
    return rv;
}



ESError
esRead(IInputStream &reader, u32 size, void *outBuf)
{
    ESError rv = ES_ERR_OK;
    s32 pOut;

    __result = reader.TryRead(&pOut, outBuf, size);
    if (!__result.IsSuccess()) {
        esLog(ES_DEBUG_ERROR, "Failed to read %u bytes\n", size);
        rv = ES_ERR_STORAGE;
        goto end;
    }

    if (((u32) pOut) != size) {
        esLog(ES_DEBUG_ERROR, "Unexpected size %u:%u\n", pOut, size);
        rv = ES_ERR_STORAGE_SIZE;
        goto end;
    }

end:
    return rv;
}


ESError
esWrite(IOutputStream &writer, u32 size, const void *buf)
{
    ESError rv = ES_ERR_OK;
    s32 pOut;

    __result = writer.TryWrite(&pOut, buf, size);
    if (!__result.IsSuccess()) {
        esLog(ES_DEBUG_ERROR, "Failed to write %u bytes\n", size);
        rv = ES_ERR_STORAGE;
        goto end;
    }

    if (((u32) pOut) != size) {
        esLog(ES_DEBUG_ERROR, "Unexpected size %u:%u\n", pOut, size);
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

ES_NAMESPACE_END
