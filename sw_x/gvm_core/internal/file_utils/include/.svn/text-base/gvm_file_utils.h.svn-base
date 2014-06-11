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

#ifndef _GVM_FILE_UTILS_H_
#define _GVM_FILE_UTILS_H_

#include "vplu_types.h"
#include <stddef.h> // for size_t

#ifdef  __cplusplus
extern "C" {
#endif

/// Creates the directory as well as any parents.
/// If the directory already exists, this will return success.
/// Permissions for any newly-created directories will be set to 0755 (or closest platform-equivalent).
/// Permissions on already-existing directories are not guaranteed to be changed.
/// @param path Absolute path on file system.
/// @param last If #VPL_TRUE, then the portion after the final "/" should also be created as a
///     directory.
/// @param loggingAllowed If #VPL_FALSE, this function will not log anything (this is needed to
///     avoid infinite recursion when the logger uses this function to set itself up).
/// @return #GVM_OK for success, else negative error code
int Util_CreatePathEx(const char* path, VPL_BOOL last, VPL_BOOL loggingAllowed);

static inline
int Util_CreatePath(const char* path, VPL_BOOL last) { return Util_CreatePathEx(path, last, VPL_TRUE); }

/// Creates the directory as well as any ancestors.
/// @param path should not have any trailing slashes.
/// @see Util_CreatePathEx
static inline
int Util_CreateDir(const char* path) { return Util_CreatePathEx(path, VPL_TRUE, VPL_TRUE); }

/// Creates the parent directory as well as any ancestors.
/// @param path should not have any trailing slashes.
/// @see Util_CreatePathEx
static inline
int Util_CreateParentDir(const char* path) { return Util_CreatePathEx(path, VPL_FALSE, VPL_TRUE); }

/// Read in the contents of a given file.
/// Allocates buffer large enough for the read data plus @a extraBytes.
/// Returns amount of data read, or negative error on failure.  Note that "amount of data read" does not
/// include @a extraBytes.
/// @note Caller must call #free() on the buffer when finished with it.
/// @param extraBytes Usually this should be 0, but you can specify larger if you need extra space at
///     the end of the allocated buffer, to add a null-terminator for example.  Note that the
///     extra memory will be uninitialized.
int Util_ReadFile(const char* filePath, void** buf_out, unsigned int extraBytes);

/// Write the buffer to the file, creating parent directories if needed.
int Util_WriteFile(const char* filePath, const void* buf, size_t buflen);

#ifdef  __cplusplus
}
#endif

#endif // include guard
