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

#ifndef __PROTOCHANNEL_H__
#define __PROTOCHANNEL_H__

#include <string>

#include <google/protobuf/io/zero_copy_stream.h>

#include "ProtoRpc.h"

/**
 * Represents a channel over which protocol buffer messages can be sent and
 * received.  Subclasses should implement the inputStream and outputStream
 * methods.  Multiple messages can be sent sequentially over the same channel.
 */
class ProtoChannel
{
public:
    /// Returns true if a message was successfully written.  If an error
    /// occurs, check #outputStreamError() to see if there was a stream
    /// I/O error or a message format error.  The caller should check there
    /// is not an I/O error prior to calling this method, to be sure.
    bool writeMessage(const ProtoMessage& message, DebugMsgCallback debugCallback);

    /// Returns true if a message was successfully parsed.  If an error
    /// occurs, check #inputStreamError() to see if there was a stream I/O
    /// error or a message format error.  The caller should check there
    /// is not an I/O error prior to calling this method, to be sure.
    bool extractMessage(ProtoMessage& message);

    /// Returns true if an I/O error has occurred on the input stream.  I/O
    /// errors are permanent; after an error, the input stream will remain
    /// unusable.
    virtual bool inputStreamError(void) const = 0;

    /// Gets an error detail message if an input stream I/O error occurs.
    virtual void inputStreamErrorDetail(std::string& out) const = 0;

    /// Returns true if an I/O error has occurred on the output stream.  I/O
    /// errors are permanent; after an error, the output stream will remain
    /// unusable.
    virtual bool outputStreamError(void) const = 0;

    /// Gets an error detail message if an output stream I/O error occurs.
    virtual void outputStreamErrorDetail(std::string& out) const = 0;

    /// Flushes the output stream.
    virtual bool flushOutputStream(void) = 0;

protected:
    virtual ZeroCopyInputStream* inputStream(void) = 0;
    virtual ZeroCopyOutputStream* outputStream(void) = 0;

    ProtoChannel(void);
    virtual ~ProtoChannel(void);
};

#endif /* __PROTOCHANNEL_H__ */
