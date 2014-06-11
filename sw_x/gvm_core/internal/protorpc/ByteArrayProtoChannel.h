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

#ifndef __BYTEARRAYPROTOCHANNEL_H__
#define __BYTEARRAYPROTOCHANNEL_H__

#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

#include "ProtoChannel.h"

/**
 * Implementation of ProtoChannel that uses byte arrays.
 */
class ByteArrayProtoChannel : public ProtoChannel
{
public:
    ByteArrayProtoChannel(void* arrayIn, size_t arrayInLen);
    virtual ~ByteArrayProtoChannel(void);

    virtual bool inputStreamError(void) const;
    virtual void inputStreamErrorDetail(std::string& out) const;
    virtual bool outputStreamError(void) const;
    virtual void outputStreamErrorDetail(std::string& out) const;
    virtual bool flushOutputStream(void);

    // We can probably do something more efficient than using a string
    // and eliminate at least one buffer copy. But this will do for now.
    const std::string& getOutputAsString() { return _outString; }

protected:
    virtual ZeroCopyInputStream* inputStream(void);
    virtual ZeroCopyOutputStream* outputStream(void);

    // Start with a larger initial capacity to avoid extra copying.
    class ReservedString : public std::string {
      public:
        ReservedString(size_t initialCapacityBytes) {
            reserve(initialCapacityBytes);
        }
    };

private:
    google::protobuf::io::ArrayInputStream _inStream;
    ReservedString _outString;
    google::protobuf::io::StringOutputStream _outStream;
};

#endif // include guard
