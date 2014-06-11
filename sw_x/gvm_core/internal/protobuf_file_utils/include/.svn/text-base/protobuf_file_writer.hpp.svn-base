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
#include "vplex_file.h"

class VPLFileOutStream : public google::protobuf::io::CopyingOutputStream 
{
public:
    VPLFileOutStream(VPLFile_handle_t handle);
    ~VPLFileOutStream();

    // implements CopyingOutputStream
    bool Write(const void* buffer, int size);

private:
    VPLFile_handle_t handle;
};

class ProtobufFileWriter
{
public:
    ProtobufFileWriter();

    ~ProtobufFileWriter() { releaseResources(); }

    void releaseResources();

    /// @param mode See #VPLFile_Open().
    int open(const char* filePath, int mode);

    google::protobuf::io::ZeroCopyOutputStream* getOutputStream() { return outStream; }

private:
    VPL_DISABLE_COPY_AND_ASSIGN(ProtobufFileWriter);

    google::protobuf::io::ZeroCopyOutputStream* outStream;
    // TODO: treats VPLFile_handle_t as int
    int fileDescriptor;
#ifdef VPL_PLAT_IS_WINRT
    // WinRT doesn't support file descriptor
    // thus using VPLFileOutStream which implements CopyingOutputStream
    // https://developers.google.com/protocol-buffers/docs/reference/cpp/google.protobuf.io.zero_copy_stream_impl_lite#CopyingOutputStream
    VPLFileOutStream *fileOutStream;
#endif
};

#endif // include guard
