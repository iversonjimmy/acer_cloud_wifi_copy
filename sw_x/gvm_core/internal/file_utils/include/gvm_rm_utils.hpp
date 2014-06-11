//
//  Copyright 2013 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER CLOUD
//  TECHNOLOGY INC.
//

#ifndef GVM_RM_UTILS_HPP_06_27_2013
#define GVM_RM_UTILS_HPP_06_27_2013

#include <string>
#include <vector>

// NoteTempDirectory:
// See https://bugs.ctbg.acer.com/show_bug.cgi?id=2977
// For win32, index may hold a handle to the file, causing the delete to succeed
// but the file to go into an state where it's not gone but not accessible (until
// all handles are released).  When the parent directory is deleted, then a
// "ACCESS" error will be encountered because the file is still there.
//
// For posix, this is not an issue.  The delete is allowed, and the delete to
// the parent directory is allowed as well.  The handles remain valid
// and the file only actually deleted when all handles are closed.
//
// The solution is to move the file to a temporary directory (which is allowed
// even if there are open handles) and delete that file in the temporary
// directory.
int Util_rmFileOpenHandleSafe(const std::string& filepathToRemove,
                              const std::string& tempDirectory);

/// Removes all elements of a given path.  Fairly dangerous operation, symbolic
/// links are not followed so long as VPL_Stat of a symbolic link NEVER returns
/// VPLFS_TYPE_DIR.
/// @param path Path to remove all elements of.  If any errors are encountered, we
///             still attempt to remove sibling entries.
/// @param tempDirectory directory for win32 deletion, see "NoteTempDirectory:" above
int Util_rmRecursive(const std::string& path,
                     const std::string& tempDirectory);

/// Removes all elements of a given path.  Fairly dangerous operation, symbolic
/// links are not followed so long as VPL_Stat of a symbolic link NEVER returns
/// VPLFS_TYPE_DIR.
/// @param path Path to remove all elements of.  If any errors are encountered, we
///             still attempt to remove sibling entries.
/// @param tempDirectory directory for win32 deletion, see "NoteTempDirectory:" above
/// @param excludeDir The directory name of directory to exclude. This directory
///                   may appear anywhere in the file tree to delete.
int Util_rmRecursiveExcludeDir(const std::string& path,
                               const std::string& tempDirectory,
                               const std::string& excludeDir);

/// Removes all elements of a given path.  Fairly dangerous operation, symbolic
/// links are not followed so long as VPL_Stat of a symbolic link NEVER returns
/// VPLFS_TYPE_DIR.
/// @param path Path to remove all elements of.  If any errors are encountered, we
///             still attempt to remove sibling entries.
/// @param tempDirectory directory for win32 deletion, see "NoteTempDirectory:" above
/// @param excludeDirs The names of directories to exclude. Directory names
///                   may appear anywhere in the file tree to delete.
int Util_rmRecursiveExcludeDirs(const std::string& path,
                                const std::string& tempDirectory,
                                const std::vector<std::string>& excludeDirs);

#endif /* GVM_RM_UTILS_HPP_06_27_2013 */
