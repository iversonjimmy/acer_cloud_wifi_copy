//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __PROTOBUF_FILE_WRITER_HPP__
#define __PROTOBUF_FILE_WRITER_HPP__

//============================================================================
/// @file
/// Class to open a file for writing, provide the type of
/// output stream used by protocol buffers, and clean
/// up when finished.
//============================================================================

#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include "vplu_common.h"

class ProtobufFileWriter
{
public:
    ProtobufFileWriter();

    ~ProtobufFileWriter() { releaseResources(); }

    void releaseResources();

    /// @param mode See #VPLFile_Open().
    int open(const char* filePath);

    google::protobuf::io::ZeroCopyOutputStream* getOutputStream() { return outStream; }

private:
    VPL_DISABLE_COPY_AND_ASSIGN(ProtobufFileWriter);

    google::protobuf::io::ZeroCopyOutputStream* outStream;

    int fileDescriptor;
};

#endif // include guard
