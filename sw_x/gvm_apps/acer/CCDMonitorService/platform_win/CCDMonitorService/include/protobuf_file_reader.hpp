//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __PROTOBUF_FILE_READER_HPP__
#define __PROTOBUF_FILE_READER_HPP__

//============================================================================
/// @file
/// Class to open a file for reading, provide the type of
/// input stream used by protocol buffers, and clean
/// up when finished.
//============================================================================

#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include "vplu_common.h"

class ProtobufFileReader
{
public:
    ProtobufFileReader();

    ~ProtobufFileReader() { releaseResources(); }

    void releaseResources();

    int open(const char* filePath, bool errorIfNotExist = true);

    google::protobuf::io::ZeroCopyInputStream* getInputStream() { return inStream; }

private:
    VPL_DISABLE_COPY_AND_ASSIGN(ProtobufFileReader);

    google::protobuf::io::ZeroCopyInputStream* inStream;

    int fileDescriptor;
};

#endif // include guard
