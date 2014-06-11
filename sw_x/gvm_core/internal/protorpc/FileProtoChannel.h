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

#ifndef __FILEPROTOCHANNEL_H__
#define __FILEPROTOCHANNEL_H__

#include <google/protobuf/io/zero_copy_stream_impl.h>

#include "ProtoChannel.h"

using google::protobuf::io::FileInputStream;
using google::protobuf::io::FileOutputStream;

/**
 * Implementation of ProtoChannel that uses a file descriptor.
 */
class FileProtoChannel : public ProtoChannel
{
public:
    FileProtoChannel(int fd, bool closeOnDelete, bool isSocket = false);
    virtual ~FileProtoChannel(void);

    virtual bool inputStreamError(void) const;
    virtual void inputStreamErrorDetail(std::string& out) const;
    virtual bool outputStreamError(void) const;
    virtual void outputStreamErrorDetail(std::string& out) const;
    virtual bool flushOutputStream(void);

protected:
    virtual ZeroCopyInputStream* inputStream(void);
    virtual ZeroCopyOutputStream* outputStream(void);

private:
    // Marking mutable because FileInputStream.GetErrno() should be const but isn't.
    mutable FileInputStream _in;
    // Marking mutable because FileOutputStream.GetErrno() should be const but isn't.
    mutable FileOutputStream _out;
};

#endif /* __FILEPROTOCHANNEL_H__ */
