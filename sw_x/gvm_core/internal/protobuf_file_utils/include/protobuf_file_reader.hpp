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
#include "vplex_file.h"

class VPLFileInStream : public google::protobuf::io::CopyingInputStream
{
public:
    VPLFileInStream(VPLFile_handle_t hFile);
    ~VPLFileInStream();

    // implements CopyingInputStream
    int Read(void * buffer, int size);

private:
    VPLFile_handle_t handle;
};

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
    // TODO: treats VPLFile_handle_t as int
    int fileDescriptor;
#ifdef VPL_PLAT_IS_WINRT
    // WinRT doesn't support file descriptor
    // thus using VPLFileInStream which implements CopyingInputStream
    // https://developers.google.com/protocol-buffers/docs/reference/cpp/google.protobuf.io.zero_copy_stream_impl_lite#CopyingInputStream
    VPLFileInStream *fileInStream;
#endif

};

#endif // include guard
