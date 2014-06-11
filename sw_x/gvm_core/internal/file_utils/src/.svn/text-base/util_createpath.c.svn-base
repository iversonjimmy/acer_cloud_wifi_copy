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

#ifdef VPL_PLAT_IS_WINRT
#include "vpl_fs.h"  // _VPLFS__GetLocalAppDataPath()
#endif
#include "vpl_plat.h"
#include "vpl_string.h"
#include "vplex_file.h"
#include "log.h"
#include "gvm_errors.h"
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

static inline int
mkdirHelper(const char* path)
{
    int rv = VPLDir_Create(path, 0755);
    if (rv == VPL_ERR_EXIST) {
        rv = VPL_OK;
    }
    return rv;
}

int
Util_CreatePathEx(const char* path, VPL_BOOL last, VPL_BOOL loggingAllowed)
{
    u32 i;
    int rv = CCD_OK;
    char* tmp_path = NULL;
#ifdef VPL_PLAT_IS_WINRT
    size_t localpath_length = 0;
#endif

    if (loggingAllowed) {
        LOG_FUNC_ENTRY(LOG_LEVEL_DEBUG);
    }

#ifdef VPL_PLAT_IS_WINRT
    {
        // metro app can only access LocalState folder in the sandbox.
        // path creation should be started from LocalState folder
        char *local_path;
        rv = _VPLFS__GetLocalAppDataPath(&local_path);
        if (rv != VPL_OK) {
            local_path = NULL;
            goto out;
        }
        localpath_length = strlen(local_path);
        for (i = 1; i < localpath_length; i++) {
            if (local_path[i] == '\\') {
                local_path[i] = '/';
            }
        }

        rv = strnicmp(local_path, path, localpath_length);
        if (local_path) {
            free(local_path);
            local_path = NULL;
        }
        if (rv != 0) {
            // TODO: implement path creation for win8 libraries
            rv = VPL_OK;
            goto out;
        }
    }
#endif

    tmp_path = VPL_strdup(path);
    if(tmp_path == NULL) {
        rv = UTIL_ERR_NO_MEM;
        goto out;
    }

    for (i = 1; tmp_path[i]; i++) {
#ifdef WIN32
        if (tmp_path[i] == '\\') {
            tmp_path[i] = '/';
        }
#endif
        if (tmp_path[i] == '/') {
#ifdef VPL_PLAT_IS_WINRT
            if (i < localpath_length) {
                // No need to create localpath or its parents.
                continue;
            }
#endif
            tmp_path[i] = '\0';
            rv = mkdirHelper(tmp_path);
            if (rv < 0) {
                if (loggingAllowed) {
                    LOG_ERROR("VPLDir_Create(%s) failed while creating \"%s\": %d", tmp_path, path, rv);
                }
                goto out;
            }
            tmp_path[i] = '/';
        }
    }

    if (last) {
        rv = mkdirHelper(path);
        if (rv < 0) {
            if (loggingAllowed) {
                LOG_ERROR("VPLDir_Create(%s) failed: %d", path, rv);
            }
            goto out;
        }
    }

out:
    if(tmp_path) free(tmp_path);
    return rv;
}
