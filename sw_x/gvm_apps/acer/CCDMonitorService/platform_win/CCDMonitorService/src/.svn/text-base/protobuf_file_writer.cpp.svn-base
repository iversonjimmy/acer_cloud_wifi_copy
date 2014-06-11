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
#include "log.h"
#include "gvm_errors.h"
#include <errno.h>
#include <Windows.h>
#include <io.h>
#include <fcntl.h>

ProtobufFileWriter::ProtobufFileWriter() :
        outStream(NULL),
        fileDescriptor(-1)
{
}

void ProtobufFileWriter::releaseResources()
{
    if (outStream != NULL) {
        delete(outStream);
        outStream = NULL;
    }

    if (fileDescriptor >= 0) {
        int rv = _close(fileDescriptor);
        if (rv < 0) {
            LOG_ERROR("VPLFile_Close() returned %s", strerror(errno));
        }
        fileDescriptor = -1;
    }
}

int ProtobufFileWriter::open(const char* filePath)
{
    int flags = 0;
    releaseResources();
    // Ensure that the parent directory exists.
    //Util_CreatePath(filePath, VPL_FALSE);

    HANDLE h32 = CreateFileA(filePath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h32 == INVALID_HANDLE_VALUE) {
        LOG_ERROR("open(%s) returned %s", filePath, strerror(errno));
        return UTIL_ERR_FOPEN;
    }

    flags &= _O_APPEND | _O_RDONLY | _O_TEXT;  // only attributes described in http://msdn.microsoft.com/en-us/library/bdts1c9x.aspx
    fileDescriptor = _open_osfhandle((intptr_t)h32, flags);
    if (fileDescriptor < 0) {
        LOG_ERROR("open(%s) returned %s", filePath, strerror(errno));
        return UTIL_ERR_FOPEN;
    }

    outStream = new google::protobuf::io::FileOutputStream(fileDescriptor);
    if (outStream == NULL) {
        LOG_ERROR("Alloc failed");
        // no need to close the file descriptor here
        return UTIL_ERR_NO_MEM;
    }
    return GVM_OK;
}
