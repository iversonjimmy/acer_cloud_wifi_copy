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
#include "vpl_fs.h"
#include "vplex_file.h"
#include "log.h"
#include "gvm_errors.h"
#include <errno.h>
#include <limits.h>

int Util_ReadFile(const char* filePath, void** buf_out, unsigned int extraBytes)
{
    VPLFile_handle_t h = VPLFILE_INVALID_HANDLE;
    VPLFS_stat_t sb;
    void *buf;
    int rv;
    int allocSize;
    LOG_FUNC_ENTRY(LOG_LEVEL_DEBUG);

    if (filePath == NULL || buf_out == NULL) {
        return UTIL_ERR_INVALID;
    }
    *buf_out = NULL;

    // Figure out how big the file is.
    rv = VPLFS_Stat(filePath, &sb);
    if (rv != VPL_OK) {
        if (rv == VPL_ERR_NOENT) {
            LOG_INFO("VPLFS_Stat: %s doesn't exist", filePath);
        } else {
            LOG_ERROR("VPLFS_Stat %s failed: %d", filePath, rv);
        }
        goto fail_stat;
    }
    
    // Do some important checks to prevent buffer overflows.
    if (sb.size > INT_MAX) {
        rv = UTIL_ERR_TOO_BIG;
        goto fail_stat;
    }
    rv = (int)sb.size;
    allocSize = rv + extraBytes;
    if (allocSize < rv) {
        rv = UTIL_ERR_TOO_BIG;
        goto fail_stat;
    }
    
    buf = malloc(allocSize);
    if (buf == NULL) {
        rv = UTIL_ERR_NO_MEM;
        goto fail_buf_malloc;
    }

    h = VPLFile_Open(filePath, VPLFILE_OPENFLAG_READONLY, 0);
    if (!VPLFile_IsValidHandle(h)) {
        LOG_ERROR("Failed to open file %s", filePath);
        rv = UTIL_ERR_FOPEN;
        goto fail_fopen;
    }

    if (VPLFile_Read(h, buf, rv) != (ssize_t)rv) {
        LOG_ERROR("Failed to read from %s", filePath);
        rv = UTIL_ERR_FREAD;
        goto fail_fread;
    }
    *buf_out = buf;
    goto done;

 fail_fread:
 fail_fopen:
    free(buf);
 fail_buf_malloc:
 fail_stat:
 done:
    if(VPLFile_IsValidHandle(h)) VPLFile_Close(h);
    return rv;
}
