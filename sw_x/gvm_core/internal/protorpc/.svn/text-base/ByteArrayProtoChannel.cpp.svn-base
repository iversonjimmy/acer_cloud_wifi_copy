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

#include "ByteArrayProtoChannel.h"

#include <string.h>

ByteArrayProtoChannel::ByteArrayProtoChannel(void* arrayIn, size_t arrayInLen) :
    _inStream(arrayIn, (int)arrayInLen),
    _outString(1024),
    _outStream(&_outString) {}

ByteArrayProtoChannel::~ByteArrayProtoChannel(void)
{}

bool
ByteArrayProtoChannel::inputStreamError(void) const
{
    return false;
}

void
ByteArrayProtoChannel::inputStreamErrorDetail(std::string& out) const
{
}

bool
ByteArrayProtoChannel::outputStreamError(void) const
{
    return false;
}

void
ByteArrayProtoChannel::outputStreamErrorDetail(std::string& out) const
{
}

bool
ByteArrayProtoChannel::flushOutputStream(void)
{
    // Nothing to flush.
    return true;
}

ZeroCopyInputStream*
ByteArrayProtoChannel::inputStream(void)
{
    return &_inStream;
}

ZeroCopyOutputStream*
ByteArrayProtoChannel::outputStream(void)
{
    return &_outStream;
}
