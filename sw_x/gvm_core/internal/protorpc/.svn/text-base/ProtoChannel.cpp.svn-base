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

#include "vplu_missing.h"

#include "ProtoChannel.h"

#include <google/protobuf/io/zero_copy_stream_impl.h>

ProtoChannel::ProtoChannel(void)
{}

ProtoChannel::~ProtoChannel(void)
{}

bool
ProtoChannel::writeMessage(const ProtoMessage& message, DebugMsgCallback debugCallback)
{
    bool success = true;
    if (outputStreamError()) {
        success = false;
        debugLogMsg(debugCallback, "outputStreamError");
    } else if (!message.IsInitialized()) {
        success = false;
        debugLogMsg(debugCallback, "message is not fully initialized!");
    } else {
        uint32_t bytes = (uint32_t)message.ByteSize();
        ZeroCopyOutputStream* out = outputStream();
        {
            google::protobuf::io::CodedOutputStream tempStream(out);
            tempStream.WriteVarint32(bytes);
            success &= !tempStream.HadError();
        }
        if (!success) {
            debugLogMsg(debugCallback, "WriteVarint32 failed!");
        } else {
            success &= message.SerializeToZeroCopyStream(out);
            // If serialization failed, assume it was an I/O error
            // because we already checked IsInitialized.
            // TODO: could double-check this.
            if (!success) {
                debugLogMsg(debugCallback, "Failed in SerializeToZeroCopyStream!");
            }
        }
    }
    return success;
}

bool
ProtoChannel::extractMessage(ProtoMessage& message)
{
    bool success = true;
    if (inputStreamError()) {
        success = false;
    } else {
        uint32_t bytes;
        ZeroCopyInputStream* in = inputStream();
        {
            google::protobuf::io::CodedInputStream tempStream(in);
            success &= tempStream.ReadVarint32(&bytes);
        }
        if (success) {
            int64_t bytesReadPre = in->ByteCount();
            // TODO: fix this in ParseFromBoundedZeroCopyStream: even if you ask
            //   for 0 bytes, it will still try to read from the socket... which
            //   causes it to wait forever if there is nothing to read.
            if (bytes > 0) {
                success &= message.ParseFromBoundedZeroCopyStream(in, (int)bytes);
                if ((!success) && (!inputStreamError())) {
                    // Encountered malformed message, but the stream is ok.
                    // Make sure it is positioned for the next message.
                    int64_t bytesReadPost = in->ByteCount();
                    uint32_t bytesRead = (uint32_t)(bytesReadPost - bytesReadPre);
                    if (bytesRead < bytes) {
                        // Don't need to check return value.  If this fails,
                        // there will be an error on the stream, so it does not
                        // matter that the positioning failed.
                        in->Skip((int)(bytes - bytesRead));
                    }
                }
            }
        }
    }
    return success;
}
