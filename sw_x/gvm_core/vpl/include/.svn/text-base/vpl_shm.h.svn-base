/*
 *  Copyright 2013 Acer Cloud Technology, Inc.
 *  All Rights Reserved.
 *
 *  This software contains confidential information and
 *  trade secrets of Acer Cloud Technology, Inc.
 *  Use, disclosure or reproduction is prohibited without
 *  the prior express written permission of Acer Cloud
 *  Technology, Inc.
 */

#ifndef __VPL_SHM_H__
#define __VPL_SHM_H__

//============================================================================
/// @file
/// Virtual Platform Layer API for shared memory management
//============================================================================

#include <sys/mman.h>
#include <sys/stat.h>   /* For mode constants */
#include <fcntl.h>        /* For O_* constants */

#ifdef __cplusplus
extern "C" {
#endif

//============================================================================
/// @defgroup VPLShm VPL Shared Memory API
///
/// VPL functionality for creating and manipulating shared memory.
///@{

/// Create/open a shared memory object
///
/// @param[in] name Shared object name
/// @param[in] oflag Open flags
/// @param[in] mode Open mode
/// @retval On success, the non-negative file descriptor.  On failure, a negative error code.
int VPLShm_Open(const char* name, int oflag, mode_t mode);

/// Unlink a shared memory object
///
/// @param[in] name Shared object name
/// @retval #VPL_OK Success
int VPLShm_Unlink(const char* name);

/// Close a file descriptor
///
/// @param[in] fd File descriptor
/// @retval #VPL_OK Success
int VPL_Close(int fd);

/// Truncate a file to a specified length
///
/// @param[in] fd File descriptor
/// @param[in] offset Offset to the file
/// @retval #VPL_OK Success
int VPL_Ftruncate(int fd, off_t offset);

/// Ensure disk space is allocated for the file
///
/// @param[in] fd File descriptor
/// @param[in] offset Offset to the file
/// @param[in] len Length to be allocated
/// @retval #VPL_OK Success
int VPL_Fallocate(int fd, off_t offset, off_t len);

/// Map files or devices into memory
///
/// @param[in] addr Memory address to place the mapping
/// @param[in] length Length of memory to be mapped
/// @param[in] prot Memory protection of the mapping
/// @param[in] flags Mapping properties
/// @param[in] fd File descriptor
/// @param[in] offset File offset
/// @param[out] out Memory address of the mapping
/// @retval #VPL_OK Success
int VPL_Mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset, void **out);

/// Unmap files or devices from memory
///
/// @param[in] addr Memory address of the mapping
/// @param[in] length Length of memory
/// @retval #VPL_OK Success
int VPL_Munmap(void *addr, size_t length);

//% end VPL Shared Memory Primitives
///@}
//============================================================================

#ifdef  __cplusplus
}
#endif

#endif // include guard
