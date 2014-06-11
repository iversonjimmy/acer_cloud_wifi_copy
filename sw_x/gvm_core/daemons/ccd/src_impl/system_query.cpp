//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

//============================================================================
/// @file
/// 
//============================================================================

#include "system_query.hpp"

#include "tmdviewer.h"
#include "ccd_util.hpp"
#if !defined(ANDROID) && !defined(WIN32)
#include <sys/statvfs.h>
#endif

#if defined(IOS) // fsblkcnt_t in iOS is "unsigned int"
#define FMT_FSBLKCNT  FMTu32
#define FMT_FSFILCNT  FMTu32
#else
#define FMT_FSBLKCNT  FMTu64
#define FMT_FSFILCNT  FMTu64
#endif

// Minimum milestone required to be downloaded for title to be playable
#define MIN_PLAYABLE_MILESTONE 1

int
Query_GetFreeDiskSpace(const char* path, ccd::DiskInfo& diskInfo_out)
{
#if !defined(ANDROID) && !defined(WIN32)
    struct statvfs info;
    int rv = statvfs(path, &info);
    if (rv < 0) {
        LOG_ERROR("statvfs returned %d: %s", errno, strerror(errno));
        return CCD_ERROR_STATVFS;
    }
    diskInfo_out.set_total_size_bytes(info.f_blocks * info.f_frsize);
    LOG_INFO("size of fs = "FMT_FSBLKCNT" fragments ("FMTu64" bytes)", info.f_blocks, diskInfo_out.total_size_bytes());

    diskInfo_out.set_available_inodes(info.f_favail);
    LOG_INFO("free inodes for non-root = "FMTu64, diskInfo_out.available_inodes());

    diskInfo_out.set_free_space_bytes(info.f_bsize * info.f_bavail);
    LOG_INFO("free blocks for non-root = "FMT_FSBLKCNT" ("FMTu64" bytes)", info.f_bavail, diskInfo_out.free_space_bytes());

    LOG_DEBUG("file system block size = %lu", info.f_bsize);
    LOG_DEBUG("fragment size = %lu", info.f_frsize);
    u64 free_blocks_size = info.f_bsize * info.f_bfree;
    LOG_DEBUG("free blocks = "FMT_FSBLKCNT" ("FMTu64" bytes)", info.f_bfree, free_blocks_size);
    LOG_DEBUG("inodes = "FMT_FSFILCNT, info.f_files);
    LOG_DEBUG("free inodes = "FMT_FSFILCNT, info.f_ffree);
    LOG_DEBUG("file system ID = 0x%lx", info.f_fsid);
    LOG_DEBUG("mount flags = 0x%lx", info.f_flag);
    LOG_DEBUG("maximum filename length = %lu", info.f_namemax);

    return CCD_OK;
#else
    return CCD_ERROR_FEATURE_DISABLED;
#endif
}
