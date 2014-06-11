//
//  Copyright 2010 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include "vpl_fs.h"
#include "vplu.h"

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#ifndef ANDROID
#include <sys/statvfs.h>
#else
#include <sys/vfs.h>
#define statvfs statfs
#define fstatvfs fstatfs
#endif
#include <unistd.h>
#include <errno.h>
#include <string.h>

#ifdef IOS
#include "NSDirectoryWrapper-C-Interface.h"
#endif

int 
VPLFS_Stat(const char* filename, VPLFS_stat_t* buf)
{
    struct stat filestat;
    int rc = -1;
    int rv = VPL_OK;

    if (buf == NULL) {
        rv = VPL_ERR_INVALID;
    }
    else {
        buf->isHidden = VPL_FALSE;  // posix never hides files.
        buf->isSymLink = VPL_FALSE;
        rc = lstat(filename, &filestat);
        if (rc == -1) {
            rv = VPLError_XlatErrno(errno);
        } 
        else {
            buf->size = filestat.st_size;
            if(S_ISREG(filestat.st_mode)) {
                buf->type = VPLFS_TYPE_FILE;
            }
            else if(S_ISDIR(filestat.st_mode)) {
                buf->type = VPLFS_TYPE_DIR;
            }
            else {
                buf->type = VPLFS_TYPE_OTHER;
            }
            buf->atime = filestat.st_atime;
            buf->mtime = filestat.st_mtime;
            buf->ctime = filestat.st_ctime;

#ifdef LINUX
            buf->vpl_atime  = VPLTime_FromSec(filestat.st_atim.tv_sec) + 
                              VPLTime_FromNanosec(filestat.st_atim.tv_nsec);

            buf->vpl_mtime  = VPLTime_FromSec(filestat.st_mtim.tv_sec) + 
                              VPLTime_FromNanosec(filestat.st_mtim.tv_nsec);

            buf->vpl_ctime  = VPLTime_FromSec(filestat.st_ctim.tv_sec) + 
                              VPLTime_FromNanosec(filestat.st_ctim.tv_nsec);
#else
            buf->vpl_atime  = VPLTime_FromSec(buf->atime);
            buf->vpl_mtime  = VPLTime_FromSec(buf->mtime);
            buf->vpl_ctime  = VPLTime_FromSec(buf->ctime);
#endif

            if(S_ISLNK(filestat.st_mode)){
                buf->isSymLink = VPL_TRUE;

                // buf->size is the length of the referent path
                // call stat() to get the size of the referent file
                rc = stat(filename, &filestat);
                if (rc == 0) {
                    buf->size = filestat.st_size;
                }
            }
        }
    }

    return rv;
}

void VPLFS_Sync(void)
{
    sync();
}

int 
VPLFS_Opendir(const char* name, VPLFS_dir_t* dir)
{
    int rv = VPL_OK;

    if(dir == NULL) {
        rv = VPL_ERR_INVALID;
    }
    else {
        DIR* dh = opendir(name);
        if (dh == NULL) {
            rv = VPLError_XlatErrno(errno);
        } 
        dir->ptr = dh;
    }

    return rv;
}

int 
VPLFS_Closedir(VPLFS_dir_t* dir)
{
    int rv = VPL_OK;

    if(dir == NULL) {
        rv = VPL_ERR_INVALID;
    }
    else if(dir->ptr == NULL) {
        rv = VPL_ERR_BADF;
    }
    else {
        DIR* dh = (DIR*)(dir->ptr);
        int rc = closedir(dh);
        if(rc == -1) {
            rv = VPLError_XlatErrno(errno);
        }
        else {
            dir->ptr = NULL;
        }
    }
    
    return rv;
}

int 
VPLFS_Readdir(VPLFS_dir_t* dir, VPLFS_dirent_t* entry)
{
    int rv = VPL_OK;

    if(dir == NULL || entry == NULL) {
        rv = VPL_ERR_INVALID;
    }
    else if(dir->ptr == NULL) {
        rv = VPL_ERR_BADF;
    }
    else {
        DIR* dh = (DIR*)(dir->ptr);
        struct dirent* rc;
        errno = 0;
        rc = readdir(dh);
        if(rc == NULL) {
            if(errno == 0) {
                rv = VPL_ERR_MAX;
            }
            else {
                rv = VPLError_XlatErrno(errno);
            }
        }
        else {
            switch(rc->d_type) {
            case DT_REG:
                entry->type = VPLFS_TYPE_FILE;
                break;
            case DT_DIR:
                entry->type = VPLFS_TYPE_DIR;
                break;
            default:
                entry->type = VPLFS_TYPE_OTHER;
                break;
            }
            strncpy(entry->filename, rc->d_name, 255);
            entry->filename[255] = '\0'; // insurance
        }
    }
    
    return rv;
}

int 
VPLFS_Rewinddir(VPLFS_dir_t* dir)
{
    int rv = VPL_OK;

    if(dir == NULL) {
        rv = VPL_ERR_INVALID;
    }
    else if(dir->ptr == NULL) {
        rv = VPL_ERR_BADF;
    }
    else {
        DIR* dh = (DIR*)(dir->ptr);
        rewinddir(dh);
    }
    
    return rv;
}

#ifndef ANDROID
int 
VPLFS_Seekdir(VPLFS_dir_t* dir, size_t pos)
{
    int rv = VPL_OK;
    
    if(dir == NULL) {
        rv = VPL_ERR_INVALID;
    }
    else if(dir->ptr == NULL) {
        rv = VPL_ERR_BADF;
    }
    else {
        DIR* dh = (DIR*)(dir->ptr);
        off_t offset = (off_t)(pos);
        seekdir(dh, offset);
    }
    
    return rv;
}

int 
VPLFS_Telldir(VPLFS_dir_t* dir, size_t* pos)
{
    int rv = VPL_OK;
    
    if(dir == NULL || pos == NULL) {
        rv = VPL_ERR_INVALID;
    }
    else if(dir->ptr == NULL) {
        rv = VPL_ERR_BADF;
    }
    else {
        DIR* dh = (DIR*)(dir->ptr);
        off_t offset;
        offset = telldir(dh);
        if(offset == -1) {
            rv = VPLError_XlatErrno(errno); 
        }
        else {
            *pos = (size_t)(offset);
        }
    }
    
    return rv;
}
#endif

int 
VPLFS_Mkdir(const char* pathName)
{
    int rv = VPL_OK;
    // Always create directories with mode 0777 - universal access.
    int rc = mkdir(pathName, 0777);
    if(rc == -1) {
        rv = VPLError_XlatErrno(errno); 
    }

    return rv;
}

int 
VPLFS_Rmdir(const char* pathName)
{
    int rv = VPL_OK;
#ifdef IOS
    // Special case: On iOS, when delete a '.' dir, it returns error-VPL_ERR_BUSY, but in vplTest_fs.c, it expect to get VPL_ERR_INVALID.
    if (pathName == NULL || (strlen(pathName) == 1 && pathName[0] == '.')) {
        return VPL_ERR_INVALID;
    }
#endif
    int rc = rmdir(pathName);
    if(rc == -1) {
        rv = VPLError_XlatErrno(errno); 
    }

    return rv;
}

int
VPLFS_GetSpace(const char* directory, u64* disk_size, u64* avail_size)
{
    int rv = VPL_OK;
    struct statvfs stat;

    if(directory == NULL || disk_size == NULL || avail_size == NULL) {
        return VPL_ERR_INVALID;
    }

    int rc = statvfs(directory, &stat);
    if(rc == -1) {
        rv = VPLError_XlatErrno(errno);
    } else {
        *disk_size = stat.f_frsize * stat.f_blocks - 
                     (stat.f_bfree - stat.f_bavail) * stat.f_bsize;
        *avail_size = stat.f_bsize * stat.f_bavail;
    }

    return rv;
}

#ifdef IOS
int
_VPLFS__GetHomeDirectory(char** homeDirectory)
{
    int rv = VPL_OK;
    
    if (homeDirectory == NULL) {
        return VPL_ERR_FAULT;
    }
    
    *homeDirectory = (char*)NSDirectory_GetHomeDirectory();
    
    if (*homeDirectory == NULL) {
        rv = VPL_ERR_NOENT;
    }
    
    return rv;
}
#endif
