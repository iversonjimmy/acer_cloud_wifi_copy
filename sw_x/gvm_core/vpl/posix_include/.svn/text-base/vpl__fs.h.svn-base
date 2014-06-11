//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __VPL__FS_H__
#define __VPL__FS_H__

/// @file
/// Platform-private definitions, please do not include this header directly.

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

//% This blind pointer gets cast to type DIR in the implementation.
typedef struct {
    void* ptr;
} VPLFS_dir__t;

// Max for yaffs2 and ext4 is 256 bytes.
#define VPLFS__DIRENT_FILENAME_MAX 256

typedef int VPLFS_MonitorHandle;

typedef off_t VPLFS_file_size__t;
#define FMTu_VPLFS_file_size__t FMTu_off_t

#ifdef IOS
    /// Get current application's home directory.
    ///
    /// @param[out] homeDirectory Contains the full path string of the home directory on success.
    /// @retval #VPL_OK Success; @a homeDirectory will have the home directory string.
    /// @retval #VPL_ERR_FAULT @a homeDirectory is NULL.
    /// @retval #VPL_ERR_NOENT The system can't get the string of the path.
    int _VPLFS__GetHomeDirectory(char** homeDirectory);
#endif
    
#ifdef __cplusplus
}
#endif

#endif // include guard
