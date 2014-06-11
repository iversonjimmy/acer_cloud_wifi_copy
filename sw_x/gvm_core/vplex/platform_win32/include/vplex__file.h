#ifndef __VPLEX__FILE_H__
#define __VPLEX__FILE_H__

#include <fcntl.h>
#include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif

// http://msdn.microsoft.com/en-us/library/1w06ktdy%28v=VS.90%29.aspx
#define VPLFILE__CHECKACCESS_READ    (04)
#define VPLFILE__CHECKACCESS_WRITE   (02)
#define VPLFILE__CHECKACCESS_EXECUTE (04) // Win doesn't have an execute bit for files; only "read" is needed.
#define VPLFILE__CHECKACCESS_EXISTS  (00)

#define VPLFILE__OPENFLAG_CREATE    _O_CREAT
#define VPLFILE__OPENFLAG_READONLY  _O_RDONLY
#define VPLFILE__OPENFLAG_WRITEONLY _O_WRONLY
#define VPLFILE__OPENFLAG_READWRITE _O_RDWR
#define VPLFILE__OPENFLAG_TRUNCATE  _O_TRUNC
#define VPLFILE__OPENFLAG_APPEND    _O_APPEND
#define VPLFILE__OPENFLAG_EXCLUSIVE _O_EXCL

#define VPLFILE__SEEK_SET SEEK_SET
#define VPLFILE__SEEK_CUR SEEK_CUR
#define VPLFILE__SEEK_END SEEK_END

#define VPLFILE__MODE_IRUSR  _S_IREAD
#define VPLFILE__MODE_IWUSR  _S_IWRITE

#define VPLFILE__INVALID_HANDLE (-1)

typedef int VPLFile_handle_t;
#define __VPLFile_handle_t_defined

//% Should really be off64_t, but MSVC doesn't seem to have that.
// Note that off_t and VPLFile_offset_t should always be signed.
typedef int64_t VPLFile_offset_t;
#define __VPLFile_offset_t_defined

#define FMTu_VPLFile_offset__t "%I64d"

#ifdef  __cplusplus
}
#endif

#endif // include guard
