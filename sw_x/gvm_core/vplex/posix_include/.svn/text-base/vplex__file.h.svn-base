#ifndef __VPLEX__FILE_H__
#define __VPLEX__FILE_H__

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VPLFILE__CHECKACCESS_READ    R_OK
#define VPLFILE__CHECKACCESS_WRITE   W_OK
#define VPLFILE__CHECKACCESS_EXECUTE X_OK
#define VPLFILE__CHECKACCESS_EXISTS  F_OK

#define VPLFILE__OPENFLAG_CREATE    O_CREAT
#define VPLFILE__OPENFLAG_READONLY  O_RDONLY
#define VPLFILE__OPENFLAG_WRITEONLY O_WRONLY
#define VPLFILE__OPENFLAG_READWRITE O_RDWR
#define VPLFILE__OPENFLAG_TRUNCATE  O_TRUNC
#define VPLFILE__OPENFLAG_APPEND    O_APPEND
#define VPLFILE__OPENFLAG_EXCLUSIVE O_EXCL

#define VPLFILE__SEEK_SET SEEK_SET
#define VPLFILE__SEEK_CUR SEEK_CUR
#define VPLFILE__SEEK_END SEEK_END

#define VPLFILE__MODE_IRUSR     S_IRUSR
#define VPLFILE__MODE_IWUSR     S_IWUSR

#define VPLFILE__INVALID_HANDLE (-1)

typedef int VPLFile_handle_t;
#define __VPLFile_handle_t_defined

// Note that off_t and VPLFile_offset_t should always be signed.
typedef off_t VPLFile_offset_t;
#define __VPLFile_offset_t_defined

#ifdef ANDROID
#define FMTu_VPLFile_offset__t "%lu"
#else
#define FMTu_VPLFile_offset__t "%llu"
#endif

#ifdef  __cplusplus
}
#endif

#endif // include guard
