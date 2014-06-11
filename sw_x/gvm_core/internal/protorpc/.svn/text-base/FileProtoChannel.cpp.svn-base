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

#include "vplu.h"

#include "FileProtoChannel.h"

#include "vpl_string.h"

FileProtoChannel::FileProtoChannel(int fd, bool closeOnDelete, bool isSocket)
    : _in(fd, isSocket), _out(fd, isSocket)
{
    // Only one of the streams should do the close
    _in.SetCloseOnDelete(closeOnDelete);
}

FileProtoChannel::~FileProtoChannel(void)
{}

bool
FileProtoChannel::inputStreamError(void) const
{
    return (_in.GetErrno() != 0);
}

void
FileProtoChannel::inputStreamErrorDetail(std::string& out) const
{
    int err = _in.GetErrno();
    if (err != 0) {
        char buf[VPL_STRERROR_ERRNO_SUGGESTED_BUFLEN];
        snprintf(buf, sizeof(buf), "Error %d: ", err);
        out = buf;
        out.append(VPL_strerror_errno(err, buf, sizeof(buf)));
    }
}

bool
FileProtoChannel::outputStreamError(void) const
{
    return (_out.GetErrno() != 0);
}

void
FileProtoChannel::outputStreamErrorDetail(std::string& out) const
{
    int err = _out.GetErrno();
    if (err != 0) {
        char buf[VPL_STRERROR_ERRNO_SUGGESTED_BUFLEN];
        snprintf(buf, sizeof(buf), "Error %d: ", err);
        out = buf;
        out.append(VPL_strerror_errno(err, buf, sizeof(buf)));
    }
}

bool
FileProtoChannel::flushOutputStream(void)
{
    return _out.Flush();
}

ZeroCopyInputStream*
FileProtoChannel::inputStream(void)
{
    return &_in;
}

ZeroCopyOutputStream*
FileProtoChannel::outputStream(void)
{
    return &_out;
}
