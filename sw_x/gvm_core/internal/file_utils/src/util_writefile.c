/*
 *               Copyright (C) 2009, BroadOn Communications Corp.
 *
 *  These coded instructions, statements, and computer programs contain
 *  unpublished  proprietary information of BroadOn Communications Corp.,
 *  and  are protected by Federal copyright law. They may not be disclosed
 *  to  third  parties or copied or duplicated in any form, in whole or in
 *  part, without the prior written consent of BroadOn Communications Corp.
 *
 */

#include "gvm_file_utils.h"

#include "vpl_plat.h"
#include "vplex_file.h"
#include "log.h"
#include "gvm_errors.h"
#include <errno.h>
#include <limits.h>

int Util_WriteFile(const char* filePath, const void* buf, size_t buflen)
{
    int rv;
    VPLFile_handle_t h = VPLFILE_INVALID_HANDLE;
    size_t written;

    LOG_FUNC_ENTRY(LOG_LEVEL_DEBUG);
    
    if (filePath == NULL || ((buf == NULL) && (buflen > 0))) {
        return UTIL_ERR_INVALID;
    }

    // Ensure that the parent directory exists.
    Util_CreatePath(filePath, VPL_FALSE);

    h = VPLFile_Open(filePath, VPLFILE_OPENFLAG_CREATE | VPLFILE_OPENFLAG_TRUNCATE | VPLFILE_OPENFLAG_WRITEONLY, VPLFILE_MODE_IRUSR | VPLFILE_MODE_IWUSR);
    if (!VPLFile_IsValidHandle(h)) {
        LOG_ERROR("Failed to open file %s", filePath);
        return UTIL_ERR_FOPEN;
    }

    written = VPLFile_Write(h, buf, buflen);
    if (written != buflen) {
        LOG_ERROR("fwrite %s failed", filePath);
        rv = UTIL_ERR_FWRITE;
        goto done;
    }
    rv = GVM_OK;
 done:
    if(VPLFile_IsValidHandle(h)) VPLFile_Close(h);
    return rv;
}
