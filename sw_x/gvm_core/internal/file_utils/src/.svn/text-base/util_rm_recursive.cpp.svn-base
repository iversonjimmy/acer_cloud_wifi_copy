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
#include "gvm_rm_utils.hpp"

#include "gvm_file_utils.hpp"

#include "vplex_file.h"

#include "vpl_fs.h"

#include <set>
#include <vector>

#include "log.h"

// See header file, See https://bugs.ctbg.acer.com/show_bug.cgi?id=2977
int Util_rmFileOpenHandleSafe(const std::string& filepathToRemove,
                              const std::string& tempDirectory)
{
    int rc;
    std::string toDelete;

#ifdef VPL_PLAT_IS_WIN_DESKTOP_MODE
    rc = Util_CreateDir(tempDirectory.c_str());
    if(rc != 0) {
        LOG_ERROR("Could not create tempDir(%s):%d, trying to remove(%s).  Continuing",
                  tempDirectory.c_str(), rc, filepathToRemove.c_str());
    }

    static u64 initialTime = VPLTime_ToNanosec(VPLTime_GetTime());
    do {  // Generate unique filename
        //==============================================================
        //==== WARNING: Code only valid for win32 platform!
        VPLThread_t winThread = VPLThread_Self();
        u32 threadId = winThread.id;  // This is unique, unless the resource is reused.
        //==== WARNING: Code only valid for win32 platform!
        //==============================================================

        const int MAX_LEN = 128;
        char fileString[MAX_LEN];
        snprintf(fileString, MAX_LEN, "%08"PRIx32"_%016"PRIx64".del",
                 threadId, initialTime);
        initialTime++;
        Util_appendToAbsPath(tempDirectory, fileString, /*OUT*/toDelete);

        rc = VPLFile_CheckAccess(toDelete.c_str(), 0);
        if(rc == 0) {
            LOG_WARN("Unique file already exist(%s), (%s, %s), trying to delete(%s). "
                     "Generating another.",
                     toDelete.c_str(), tempDirectory.c_str(), fileString,
                     filepathToRemove.c_str());
        }
    }while (rc == 0);

    rc = VPLFile_Rename(filepathToRemove.c_str(), toDelete.c_str());
    if(rc != 0) {
        LOG_ERROR("VPLFile_Rename(%s->%s):%d",
                  filepathToRemove.c_str(), toDelete.c_str(), rc);
        return rc;
    }

#else  // !VPL_PLAT_IS_WIN_DESKTOP_MODE
    toDelete = filepathToRemove;
#endif  // VPL_PLAT_IS_WIN_DESKTOP_MODE

    rc = VPLFile_Delete(toDelete.c_str());
    return rc;
}

static int helper_rm_dash_rf(const std::string& path,
                             const std::string& tempPath,
                             const std::vector<std::string>& excludeDirs)
{
    int rv = 0;
    int rc = 0;
    VPLFS_stat_t statBufOne;
    rc = VPLFS_Stat(path.c_str(), &statBufOne);
    if(rc != VPL_OK) {
        LOG_WARN("removing path %s that does not exist.%d. Count as success.",
                 path.c_str(), rc);
        return 0;
    }

    std::vector<std::string> dirPaths;
    if(statBufOne.type == VPLFS_TYPE_FILE) {
       rc = Util_rmFileOpenHandleSafe(path.c_str(), tempPath.c_str());
       if(rc != VPL_OK) {
           LOG_ERROR("File unlink unsuccessful: %s, (%d)",
                     path.c_str(), rc);
       }
       return rc;
    } else {
        dirPaths.push_back(path);
    }

    // If there are directories that should not or cannot be deleted, then
    // side step around them and delete everything else.
    std::set<std::string> ignoreAndErrorDirs;

    while(!dirPaths.empty()) {
        VPLFS_dir_t dp;
        VPLFS_dirent_t dirp;
        std::string currDir(dirPaths.back());

        if((rc = VPLFS_Opendir(currDir.c_str(), &dp)) != VPL_OK) {
            LOG_ERROR("Error(%d) opening %s",
                      rc, currDir.c_str());
            dirPaths.pop_back();
            continue;
        }

        bool dirAdded = false;
        while(VPLFS_Readdir(&dp, &dirp) == VPL_OK) {
            std::string dirent(dirp.filename);
            std::string absFile;
            Util_appendToAbsPath(currDir, dirent, absFile);
            VPLFS_stat_t statBuf;

            if(dirent == "." || dirent == "..") {
                continue;
            }

            if((rc = VPLFS_Stat(absFile.c_str(), &statBuf)) != VPL_OK) {
                LOG_ERROR("Error(%d) using stat on (%s,%s), type:%d",
                          rc, currDir.c_str(), dirent.c_str(), (int)dirp.type);
                continue;
            }
            if(statBuf.type == VPLFS_TYPE_FILE) {
                rc = Util_rmFileOpenHandleSafe(absFile.c_str(), tempPath.c_str());
                if(rc != VPL_OK) {
                    LOG_ERROR("File unlink unsuccessful: %s, (%d)",
                              absFile.c_str(), rc);
                    rv = rc;
                    continue;
                }
            } else { // This is a directory
                std::string toPushBack(absFile);

                // Check to make sure this directory is not excluded.
                bool foundExcluded = false;
                for(std::vector<std::string>::const_iterator iter = excludeDirs.begin();
                    iter!=excludeDirs.end();++iter)
                {
                    if(dirent == *iter) {
                        // Directory name matches excluded directory string.
                        foundExcluded = true;
                        break;
                    }
                }

                if(foundExcluded) {
                    // Ignoring this excluded directory by not putting in dirPaths.
                    // Preventing parent directory from being deleted by adding
                    // to ignoreAndErrorDirs.
                    ignoreAndErrorDirs.insert(currDir);
                }else if (ignoreAndErrorDirs.find(toPushBack) ==
                          ignoreAndErrorDirs.end())
                {   // This directory did not have an error deleting, safe to traverse.
                    dirAdded = true;
                    dirPaths.push_back(toPushBack);
                } else
                {   // This directory is not to be deleted.  Add parent directory
                    // to the list of ignored/error dirs.
                    ignoreAndErrorDirs.insert(currDir);
                }
            }
        }

        VPLFS_Closedir(&dp);

        if (!dirAdded)
        {   // Directory did not have subDirectory that needs traversing.
            // Delete it if we can.
            std::set<std::string>::iterator isError = ignoreAndErrorDirs.find(currDir);
            if (isError == ignoreAndErrorDirs.end()) {
                rc = VPLDir_Delete(currDir.c_str());
                if(rc != VPL_OK) {
                    LOG_ERROR("rmdir unsuccessful: %s, (%d)",
                              currDir.c_str(), rc);
                    ignoreAndErrorDirs.insert(currDir);
                    rv = rc;
                }
            } else {
                LOG_INFO("Skipping delete of dir(%s)", currDir.c_str());
            }
            dirPaths.pop_back();
        }
    }
    return rv;
}

int Util_rmRecursive(const std::string& path,
                     const std::string& tempDirectory)
{
    int rc;
    std::vector<std::string> empty;
    rc = helper_rm_dash_rf(path,
                           tempDirectory,
                           empty);
    return rc;
}

int Util_rmRecursiveExcludeDir(const std::string& path,
                               const std::string& tempDirectory,
                               const std::string& excludeDir)
{
    int rc;
    std::vector<std::string> singleExclude;
    singleExclude.push_back(excludeDir);
    rc = helper_rm_dash_rf(path,
                           tempDirectory,
                           singleExclude);
    return rc;
}

int Util_rmRecursiveExcludeDirs(const std::string& path,
                                const std::string& tempDirectory,
                                const std::vector<std::string>& excludeDirs)
{
    int rc;
    rc = helper_rm_dash_rf(path,
                           tempDirectory,
                           excludeDirs);
    return rc;
}
