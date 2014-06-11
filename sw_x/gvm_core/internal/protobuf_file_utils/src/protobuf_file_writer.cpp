//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include "protobuf_file_writer.hpp"
#include "vplex_file.h"
#include "log.h"
#include "gvm_errors.h"
#include "gvm_file_utils.h"
#include <errno.h>

VPLFileOutStream::VPLFileOutStream(VPLFile_handle_t hFile)
{
    handle = hFile;
}

VPLFileOutStream::~VPLFileOutStream()
{
    handle = VPLFILE_INVALID_HANDLE;
}

bool VPLFileOutStream::Write(const void* buffer, int size)
{
    ssize_t byteWritten = VPLFile_Write(handle, buffer, size);
    
    // Writes "size" bytes from the given buffer to the output. 
    if (byteWritten > 0 && byteWritten == size)
        return true;
    else
        return false;
}

ProtobufFileWriter::ProtobufFileWriter() :
        outStream(NULL),
        fileDescriptor(-1)
{
#ifdef VPL_PLAT_IS_WINRT
    fileOutStream = NULL;
#endif
}

void ProtobufFileWriter::releaseResources()
{
    if (outStream != NULL) {
        delete(outStream);
        outStream = NULL;
    }
#ifdef VPL_PLAT_IS_WINRT
    if (fileOutStream != NULL) {
        delete fileOutStream;
        fileOutStream = NULL;
    }
#endif
    if (fileDescriptor >= 0) {
        int rv;
        // Bug 9943: Make sure that the data is flushed to disk before proceeding.
        {
            rv = VPLFile_Sync(fileDescriptor);
            if (rv < 0) {
                LOG_ERROR("VPLFile_Sync() returned %d", rv);
            }
        }
        rv = VPLFile_Close(fileDescriptor);
        if (rv < 0) {
            LOG_ERROR("VPLFile_Close() returned %d", rv);
        }
        fileDescriptor = VPLFILE_INVALID_HANDLE;
    }
}

int ProtobufFileWriter::open(const char* filePath, int mode)
{
    releaseResources();
    // Ensure that the parent directory exists.
    Util_CreatePath(filePath, VPL_FALSE);
    fileDescriptor = VPLFile_Open(filePath,
            VPLFILE_OPENFLAG_WRITEONLY | VPLFILE_OPENFLAG_CREATE | VPLFILE_OPENFLAG_TRUNCATE,
            mode);
    if (fileDescriptor < 0) {
        LOG_ERROR("open(%s) returned %d", filePath, fileDescriptor);
        return UTIL_ERR_FOPEN;
    }
#ifdef VPL_PLAT_IS_WINRT
    fileOutStream = new VPLFileOutStream(fileDescriptor);
    outStream = new google::protobuf::io::CopyingOutputStreamAdaptor(fileOutStream);
#else
    outStream = new google::protobuf::io::FileOutputStream(fileDescriptor);
#endif
    if (outStream == NULL) {
        LOG_ERROR("Alloc failed");
        // no need to close the file descriptor here
        return UTIL_ERR_NO_MEM;
    }
    return GVM_OK;
}
