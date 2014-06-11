#include <vpl_plat.h>

#include "VcsTestUtils.hpp"

#include "gvm_file_utils.hpp"
#include "vcs_util.hpp"
#include "vplu_format.h"

#include <log.h>

static const u64 VCS_GET_DIR_PAGE_SIZE = 500;

int vcsRmFolderRecursive(const VcsSession& vcsSession,
                         const VcsDataset& dataset,
                         const std::string& folderArg,
                         u64 compId,
                         bool removeContentsOnly,
                         bool printLog)
{
    int rv = 0;
    int rc;

    // Vector of Pair of (directory name, visited_flag)
    std::vector< VcsFolder > visitedDirectories;
    std::string rootFolderName;
    Util_trimSlashes(folderArg, rootFolderName);

    VcsFolder rootFolder;
    rootFolder.clear();
    rootFolder.name = rootFolderName;
    rootFolder.compId = compId;
    visitedDirectories.push_back(rootFolder);

    VcsFolder folder;
    VcsGetDirResponse folderOut;

    if (removeContentsOnly)
    {
        u64 filesSeen = 0;
        u64 numEntries;
        std::vector< VcsFile > filesToDeleteNow;
        do {
            VPLHttp2 httpHandleReadDir;
            rc = vcs_read_folder_paged(vcsSession,
                                       httpHandleReadDir,
                                       dataset,
                                       folder.name,
                                       folder.compId,
                                       (filesSeen + 1), // VCS starts at 1 instead of 0 for some reason
                                       VCS_GET_DIR_PAGE_SIZE,
                                       printLog,
                                       folderOut);
            if(rc != 0) {
                LOG_ERROR("vcs_read_folder:%d, dset:"FMTu64", folder:%s",
                          rc, dataset.id, folder.name.c_str());
                if(rv==0) { rv = rc; }
                break;
            }

            filesToDeleteNow.insert(filesToDeleteNow.end(),
                                    folderOut.files.begin(),
                                    folderOut.files.end());

            for(u32 dirIndex=0; dirIndex<folderOut.dirs.size(); ++dirIndex) {
                VcsFolder newDir;
                newDir.clear();
                newDir = folderOut.dirs[dirIndex];
                newDir.name = folder.name+"/"+folderOut.dirs[dirIndex].name;
                visitedDirectories.push_back(newDir);
            }

            // Get ready to process the next page of vcs_read_folder results.
            u64 entriesInCurrRequest = folderOut.files.size() + folderOut.dirs.size();
            filesSeen += entriesInCurrRequest;
            if(entriesInCurrRequest == 0 && filesSeen < folderOut.numOfFiles) {
                LOG_ERROR("No entries in getDir(%s) response:"FMTu64"/"FMTu64,
                          folder.name.c_str(),
                          filesSeen, folderOut.numOfFiles);
                break;
            }
            numEntries = folderOut.numOfFiles;
        } while (filesSeen < numEntries);

        // Delete all the files first
        for(std::vector< VcsFile >::iterator fileToDeleteIter = filesToDeleteNow.begin();
                fileToDeleteIter != filesToDeleteNow.end();
                ++fileToDeleteIter)
        {
            VPLHttp2 httpHandleDeleteFile;
            rc = vcs_delete_file(vcsSession,
                                 httpHandleDeleteFile,
                                 dataset,
                                 folder.name+"/"+fileToDeleteIter->name,
                                 fileToDeleteIter->compId,
                                 fileToDeleteIter->latestRevision.revision,
                                 printLog);
            if(rc != 0) {
                LOG_ERROR("Error deleting %s",
                          (folder.name+"/"+fileToDeleteIter->name).c_str());
                if(rv==0) { rv = rc; }
                continue;
            }
            LOG_ALWAYS("Successfully deleted:%s",
                       (folder.name+"/"+fileToDeleteIter->name).c_str());
        }
    }

    while(visitedDirectories.size()>0)
    {   // Try and delete all the directories LIFO order.  Directories should be empty.
        // It's an error if the directory delete does not complete.
        VcsFolder visitedFolder;
        visitedFolder = visitedDirectories.back();
        visitedDirectories.pop_back();
        if(!removeContentsOnly || (rootFolder.name != visitedFolder.name))
        {   // Need to delete this folder (delete skipped when user specifies
            // not to delete root folder and the folder is root
            VPLHttp2 httpHandleDeleteDir2;
            rc = vcs_delete_dir(vcsSession,
                                httpHandleDeleteDir2,
                                dataset,
                                visitedFolder.name,
                                visitedFolder.compId,
                                true,  // recursive
                                false, 0,  // No dataset version
                                printLog);
            if(rc != 0) {
                LOG_ERROR("vcs_delete_dir failed:%s, %d", visitedFolder.name.c_str(), rc);
                if(rv==0) { rv = rc; }
            }
        }
    }

    return rv;
}
