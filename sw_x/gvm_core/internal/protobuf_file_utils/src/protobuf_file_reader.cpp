//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include "protobuf_file_reader.hpp"
#include "vplex_file.h"
#include "log.h"
#include "gvm_errors.h"
#include "gvm_file_utils.h"
#include <errno.h>

VPLFileInStream::VPLFileInStream(VPLFile_handle_t hFile)
{
    handle = hFile;
}

VPLFileInStream::~VPLFileInStream()
{
    handle = VPLFILE_INVALID_HANDLE;
}

int VPLFileInStream::Read(void * buffer, int size)
{
    int byteRead = 0;
    // Reads up to "size" bytes into the given buffer. 
    byteRead = VPLFile_Read(handle, buffer, size);

    return byteRead;
}

ProtobufFileReader::ProtobufFileReader() :
        inStream(NULL),
        fileDescriptor(-1)
{
#ifdef VPL_PLAT_IS_WINRT
        fileInStream = NULL;
#endif
}

void ProtobufFileReader::releaseResources()
{
    if (inStream != NULL) {
        delete(inStream);
        inStream = NULL;
    }
#ifdef VPL_PLAT_IS_WINRT
    if (fileInStream != NULL) {
        delete fileInStream;
        fileInStream = NULL;
    }
#endif
    if (fileDescriptor >= 0) {
        int rv = VPLFile_Close(fileDescriptor);
        if (rv < 0) {
            LOG_ERROR("close() returned %d", rv);
        }
        fileDescriptor = VPLFILE_INVALID_HANDLE;
    }
}

int ProtobufFileReader::open(const char* filePath, bool errorIfNotExist)
{
    releaseResources();
    // Ensure that the parent directory exists.
    Util_CreatePath(filePath, VPL_FALSE);
    fileDescriptor = VPLFile_Open(filePath, VPLFILE_OPENFLAG_READONLY, 0);
    if (fileDescriptor < 0) {
        if (errorIfNotExist) {
            LOG_ERROR("open(%s) returned %d", filePath, fileDescriptor);
        }
        return UTIL_ERR_FOPEN;
    }
#ifdef VPL_PLAT_IS_WINRT
    fileInStream = new VPLFileInStream(fileDescriptor);
    inStream = new google::protobuf::io::CopyingInputStreamAdaptor(fileInStream);
#else
    inStream = new google::protobuf::io::FileInputStream(fileDescriptor);
#endif
    if (inStream == NULL) {
        LOG_ERROR("Alloc failed");
        // no need to close the file descriptor here
        return UTIL_ERR_NO_MEM;
    }
    return GVM_OK;
}
