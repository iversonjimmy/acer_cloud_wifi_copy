#include "vplex_file.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <utime.h>
#include <stdarg.h>
#include <errno.h>

int VPLFile_CheckAccess(const char *pathname, int mode)
{
    int rv = VPL_OK;
    if (access(pathname, mode) == -1) {
        rv = VPLError_XlatErrno(errno);
    }
    return rv;
}

VPLFile_handle_t VPLFile_Open(const char *pathname, int flags, int mode)
{
    VPLFile_handle_t h;

    if (flags & O_CREAT) {
        h = open(pathname, flags, mode);
    }
    else {
        h = open(pathname, flags);
    }

    if (h == -1) {
        h = VPLError_XlatErrno(errno);
    }

    return h;
}

int VPLFile_IsValidHandle(VPLFile_handle_t h)
{
    return h >= 0;
}

ssize_t VPLFile_Write(VPLFile_handle_t h, const void *buffer, size_t bufsize)
{
    ssize_t nbytes = write(h, buffer, bufsize);
    if (nbytes == -1) {
        nbytes = VPLError_XlatErrno(errno);
    }
    return nbytes;
}

ssize_t VPLFile_Read(VPLFile_handle_t h, void *buffer, size_t bufsize)
{
    ssize_t nbytes = read(h, buffer, bufsize);
    if (nbytes == -1) {
        nbytes = VPLError_XlatErrno(errno);
    }
    return nbytes;
}

ssize_t VPLFile_WriteAt(VPLFile_handle_t h, const void *buffer, size_t bufsize, VPLFile_offset_t offset)
{
    // Android NDK expects void* for 2nd arg
    ssize_t nbytes = pwrite(h, (void*)buffer, bufsize, offset);
    if (nbytes == -1) {
        nbytes = VPLError_XlatErrno(errno);
    }
    return nbytes;
}

ssize_t VPLFile_ReadAt(VPLFile_handle_t h, void *buffer, size_t bufsize, VPLFile_offset_t offset)
{
    ssize_t nbytes = pread(h, buffer, bufsize, offset);
    if (nbytes == -1) {
        nbytes = VPLError_XlatErrno(errno);
    }
    return nbytes;
}

int VPLFile_CreateTemp(char* filename_in_out, size_t bufSize)
{
    (void)(bufSize);
    return mkstemp(filename_in_out);
}

int VPLFile_TruncateAt(VPLFile_handle_t h, VPLFile_offset_t length)
{
    int rv = VPL_OK;
    if (ftruncate(h, length) == -1) {
        rv = VPLError_XlatErrno(errno);
    }
    return rv;
}

int VPLFile_Sync(VPLFile_handle_t h)
{
    int rv = VPL_OK;
    if (
#if defined(ANDROID) || defined(IOS)
        fsync(h)
#else
        fdatasync(h)
#endif
        == -1) {
        rv = VPLError_XlatErrno(errno);
    }
    return rv;
}

VPLFile_offset_t VPLFile_Seek(VPLFile_handle_t h, VPLFile_offset_t offset, int whence)
{
    VPLFile_offset_t rv = -1;

    rv = lseek(h, offset, whence);
    if(rv == -1) {
        rv = VPLError_XlatErrno(errno);
    }

    return rv;
}

int VPLFile_Close(VPLFile_handle_t h)
{
    int rv = VPL_OK;
    if (close(h) == -1) {
        rv = VPLError_XlatErrno(errno);
    }
    return rv;
}

FILE *VPLFile_FOpen(const char *pathname, const char *mode)
{
    return fopen(pathname, mode);
}

int VPLFile_Delete(const char *pathname)
{
    int rv = VPL_OK;
    if (unlink(pathname) != 0) {
        rv = VPLError_XlatErrno(errno);
    }
    return rv;
}

int VPLFile_SetTime(const char *pathname, VPLTime_t time)
{
    int rv = VPL_OK;
    struct utimbuf buf;
    buf.actime = VPLTIME_TO_SEC(time);
    buf.modtime = VPLTIME_TO_SEC(time);
    if (utime(pathname, &buf) == -1) {
        rv = VPLError_XlatErrno(errno);
    }
    return rv;
}

int VPLFile_Rename(const char *oldpath, const char *newpath)
{
    int rv = VPL_OK;
    if (rename(oldpath, newpath) != 0) {
        rv = VPLError_XlatErrno(errno);
    }
    return rv;
}

int VPLFile_SetAttribute(const char *path, u32 attrs, u32 maskbits)
{
    int rv = VPL_OK;

    if (attrs & ~(VPLFILE_ATTRIBUTE_MASK)) {
        rv = VPL_ERR_INVALID;
        goto end;
    }

    /* nothing to do for VPLFILE_ATTRIBUTE_HIDDEN on POSIX: just ignore */

 end:
    return rv;
}

int VPLDir_Create(const char *pathname, int mode)
{
    int rv = VPL_OK;
    if (mkdir(pathname, mode) != 0) {
        rv = VPLError_XlatErrno(errno);
    }
    return rv;
}

int VPLDir_Delete(const char *pathname)
{
    int rv = VPL_OK;
    if (rmdir(pathname) != 0) {
        rv = VPLError_XlatErrno(errno);
    }
    return rv;
}

int VPLPipe_Create(VPLFile_handle_t handles[2])
{
    return pipe(handles);
}

int VPLFS_FStat(VPLFile_handle_t fd, VPLFS_stat_t* buf)
{
    struct stat filestat;
    int rc = -1;
    int rv = VPL_OK;
    
    if (!VPLFile_IsValidHandle(fd) || buf == NULL) {
        rv = VPL_ERR_INVALID;
    }
    else {
        buf->isHidden = VPL_FALSE;  // posix never hides files.
        buf->isSymLink = VPL_FALSE;
        rc = fstat(fd, &filestat);
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
            if (S_ISLNK(filestat.st_mode)) {
                buf->isSymLink = VPL_TRUE;
            }
        }
    }
    
    return rv;
}
