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

#ifndef __PROTORPC_H__
#define __PROTORPC_H__

#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/message.h>
#include "rpc.pb.h"

using google::protobuf::io::ZeroCopyInputStream;
using google::protobuf::io::ZeroCopyOutputStream;

/// Typedef so that at compile time we can choose the full or lite runtime
//% TODO: this should depend on debug vs release.
typedef google::protobuf::Message ProtoMessage;

/// Helper to write a buffer to a zero copy stream and handle excess buffer
/// space provided by the stream.
bool writeExactly(ZeroCopyOutputStream* stream, const void* bytes, int numBytes);

/// Helper to read an exact number of bytes from a zero copy stream and
/// handle any excess read-ahead performed by the stream.
bool readExactly(ZeroCopyInputStream* stream, void* bytes, int numBytes);

/// Type of function that can be provided to receive informational messages from the RPC layer.
typedef void (DebugMsgCallback)(const char* msg);
typedef void (DebugRequestMsgCallback)(const std::string& methodName,
                                       bool isValid,
                                       const ProtoMessage& reqMsg);
typedef void (DebugResponseMsgCallback)(const std::string& methodName,
                                        const RpcStatus& status,
                                        const ProtoMessage* respMsg);

static inline
void debugLogMsg(DebugMsgCallback debugCallback, const std::string& msg)
{
    if (debugCallback != NULL) {
        debugCallback(msg.c_str());
    }
}

#endif // include guard
