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
#include "log.h"
#include "gvm_errors.h"
#include <errno.h>
#include <Windows.h>
#include <io.h>
#include <fcntl.h>

ProtobufFileReader::ProtobufFileReader() :
        inStream(NULL),
        fileDescriptor(-1)
{
}

void ProtobufFileReader::releaseResources()
{
    if (inStream != NULL) {
        delete(inStream);
        inStream = NULL;
    }

    if (fileDescriptor >= 0) {
        int rv = _close(fileDescriptor);
        if (rv < 0) {
            LOG_ERROR("close() returned %s", strerror(errno));
        }
        fileDescriptor = -1;
    }
}

int ProtobufFileReader::open(const char* filePath, bool errorIfNotExist)
{
    int flags = 0;
    releaseResources();

    // Ensure that the parent directory exists.
    //Util_CreatePath(filePath, VPL_FALSE);

    HANDLE h32 = CreateFileA(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h32 == INVALID_HANDLE_VALUE) {
        if (errorIfNotExist) {
            LOG_ERROR("open(%s) returned %s", filePath, strerror(errno));
        }
        return UTIL_ERR_FOPEN;
    }

    flags &= _O_APPEND | _O_RDONLY | _O_TEXT;  // only attributes described in http://msdn.microsoft.com/en-us/library/bdts1c9x.aspx
    fileDescriptor = _open_osfhandle((intptr_t)h32, flags);
    if (fileDescriptor < 0) {
        LOG_ERROR("open(%s) returned %s", filePath, strerror(errno));
        return UTIL_ERR_FOPEN;
    }

    inStream = new google::protobuf::io::FileInputStream(fileDescriptor);
    if (inStream == NULL) {
        LOG_ERROR("Alloc failed");
        // no need to close the file descriptor here
        return UTIL_ERR_NO_MEM;
    }
    return GVM_OK;
}
