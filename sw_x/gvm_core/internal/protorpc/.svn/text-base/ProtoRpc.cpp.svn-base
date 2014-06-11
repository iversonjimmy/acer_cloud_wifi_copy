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

#include "ProtoRpc.h"

bool
writeExactly(ZeroCopyOutputStream* stream, const void* bytes, int numBytes)
{
    bool success = true;

    while (numBytes > 0) {
        void* buffer = NULL;
        int bufferLen = 0;
        success &= stream->Next(&buffer, &bufferLen);
        if (!success) {
            break;
        }
        int numToWrite = (bufferLen < numBytes ? bufferLen : numBytes);
        memcpy(buffer, bytes, numToWrite);
        bytes = (const void*)(((const int8_t*)bytes) + numToWrite);
        numBytes -= numToWrite;
        if (numToWrite < bufferLen) {
            // Got too much buffer, back up
            stream->BackUp(bufferLen - numToWrite);
        }
    }

    return success;
}

bool
readExactly(ZeroCopyInputStream* stream, void* bytes, int numBytes)
{
    bool success = true;

    while (numBytes > 0) {
        const void* buffer = NULL;
        int bufferLen = 0;
        success &= stream->Next(&buffer, &bufferLen);
        if (!success) {
            break;
        }
        int numToRead = (bufferLen < numBytes ? bufferLen : numBytes);
        memcpy(bytes, buffer, numToRead);
        bytes = (void*)(((int8_t*)bytes) + numToRead);
        numBytes -= numToRead;
        if (numToRead < bufferLen) {
            // Got too many bytes, back up
            stream->BackUp(bufferLen - numToRead);
        }
    }

    return success;
}
