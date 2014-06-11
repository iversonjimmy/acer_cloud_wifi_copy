//  Copyright 2014 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include <gvm_file_utils.hpp>
#include <ccdi_client_tcp.hpp>
#include "scopeguard.hpp"

#include "autotest_common_utils.hpp"
#include "dx_common.h"
#include "common_utils.hpp"
#include "ccd_utils.hpp"
#include "fs_test.hpp"
#include "TargetDevice.hpp"
#include "EventQueue.hpp"

#include "autotest_remotefile.hpp"

#include "cJSON2.h"

#include <iostream>
#include <sstream>
#include <string>

typedef struct folder_item {
    std::string path;
    bool allow;
    bool virt_folder;
    int  file_permission;    // VPL_FILE_MODE_XXX
    int  dir_permission;     // VPL_FILE_MODE_XXX
    folder_item(std::string path, bool allow, bool virt_folder,
                int file_permission = (VPLFILE_MODE_IRUSR | VPLFILE_MODE_IWUSR), int dir_permission = (VPLFILE_MODE_IRUSR | VPLFILE_MODE_IWUSR))
        : path(path), allow(allow), virt_folder(virt_folder), file_permission(file_permission), dir_permission(dir_permission) {};
} folder_item;

static inline bool is_read(int file_permission)
{
    if(file_permission & VPLFILE_MODE_IRUSR)
        return true;
    return false;
}

static inline bool is_write(int file_permission)
{
    if(file_permission & VPLFILE_MODE_IWUSR)
        return true;
    return false;
}

// pathname (std::string), allow access (bool), used by remote file
typedef std::vector<folder_item> folder_access_rules;

int remotefile_feature_enable(int cloudpc,
                                     int client,
                                     u64 userId,
                                     u64 deviceId,
                                     bool enable_rf,
                                     bool enable_media,
                                     std::string alias_cloudpc="",
                                     std::string alias_client="")
{
    int rv = VPL_OK;
    ccd::ListUserStorageInput psnrequest;
    ccd::ListUserStorageOutput psnlistSnOut;
    const char *testStr = "UpdatePsn";

    SET_TARGET_MACHINE("RemoteFile", alias_cloudpc.c_str(), rv);
    if (rv < 0) {
        setCcdTestInstanceNum(cloudpc);
    }
    {
        const char *testArg[] = { testStr, (enable_rf? "-R" : "-r") };
        if(enable_rf) {
        LOG_ALWAYS("Enable RF!");
        }
        else {
            LOG_ALWAYS("Disable RF!");
        }
        rv = update_psn(2, testArg);
        if (rv != VPL_OK) {
            LOG_ERROR("Error while updating psn flag for remotefile: flag = %s, rv = %d", testArg[1], rv);
            return rv;
        }
    }
    {
        const char *testArg[] = { testStr, (enable_media? "-M" : "-m") };
        if(enable_media) {
        LOG_ALWAYS("Enable Media_RF!");
        }
        else {
            LOG_ALWAYS("Disable Media_RF!");
        }
        rv = update_psn(2, testArg);
        if (rv != VPL_OK) {
            LOG_ERROR("Error while updating psn flag for mediaserver: flag = %s, rv = %d", testArg[1], rv);
            return rv;
        }
    }

    psnrequest.set_user_id(userId);
    psnrequest.set_only_use_cache(false);

#define SYNC_RF_FLAGS(instance, exit_symbol, alias) \
    BEGIN_MULTI_STATEMENT_MACRO \
    SET_TARGET_MACHINE("SdkRemoteFileRelease", alias.c_str(), rv); \
    if (rv < 0) { \
        setCcdTestInstanceNum(instance); \
    } \
    LOG_ALWAYS("\n\n== %s RemoteFile feature / %s MediaServer feature on instance #%d ==", \
               (enable_rf? "Enabling" : "Disabling"), \
               (enable_media? "Enabling" : "Disabling"), instance); \
    int retry = 0; \
    while (retry < 60) { \
        rv = CCDIListUserStorage(psnrequest, psnlistSnOut); \
        if (rv != 0) { \
            LOG_ERROR("CCDIListUserStorage for user("FMTu64") failed %d", userId, rv); \
            return rv; \
        } else { \
            LOG_ALWAYS("CCDIListUserStorage OK - %d", psnlistSnOut.user_storage_size()); \
            for (int i = 0; i < psnlistSnOut.user_storage_size(); i++) { \
                if (psnlistSnOut.user_storage(i).storageclusterid() == deviceId) { \
                    LOG_DEBUG("RF expect %d - %d current", psnlistSnOut.user_storage(i).featureremotefileaccessenabled(), enable_rf); \
                    LOG_DEBUG("MF expect %d - %d current", psnlistSnOut.user_storage(i).featuremediaserverenabled(), enable_media); \
                    if ((psnlistSnOut.user_storage(i).featureremotefileaccessenabled() == enable_rf) && \
                        (psnlistSnOut.user_storage(i).featuremediaserverenabled() == enable_media)) { \
                        goto exit_symbol; \
                    } \
                    /* find device, but flags doesn't matched. break for next round */ \
                    break; \
                } \
            } \
        } \
        VPLThread_Sleep(VPLTIME_FROM_MILLISEC(1000)); \
        retry++; \
        LOG_ALWAYS("Retry (%d) times to get PSN remotefile flag to %s / mediaserver flag to %s", \
                   retry, (enable_rf? "Enable" : "Disable"), \
                   (enable_media? "Enable" : "Disable")); \
    } \
    END_MULTI_STATEMENT_MACRO

    SYNC_RF_FLAGS(cloudpc, cloudpc_synced, alias_cloudpc);
cloudpc_synced:
    SYNC_RF_FLAGS(client, client_synced, alias_client);
client_synced:

#undef SYNC_RF_FLAGS

    //This will force ccd to update dataset info
    {
        rv = dumpDatasetList(userId);
        if (rv != 0) {
            LOG_ERROR("Fail to get dataset id:%d", rv);
        }
    }

exit:
    return rv;
}

static std::string convert_to_win32_virt_folder(const char *path)
{
    std::string tmp;
    size_t idx;

    // adjust C:/ to Computer/C
    tmp.assign("Computer/").append(path);
    idx = tmp.find_first_of(":");
    if (idx != std::string::npos) {
        tmp.erase(idx, 1);
    }
    return tmp;
}

static std::string convert_to_win32_real_folder(const char *path)
{
    std::string tmp = path;

    // adjust C:/ to Computer/C
    if (tmp.find("Computer/") == 0) {
        tmp.erase(0, strlen("Computer/"));
        if (tmp.length() == 1) {
            tmp.append(":/");
            return tmp;
        } else if (tmp[1] == '/') {
            tmp.insert(1, ":");
            return tmp;
        }
    }
    return path;
}

static int remotefile_wait_async_upload_done(bool is_media, bool expect_fail)
{
    int rv = VPL_OK;

    std::stringstream ss;
    std::string response;
    std::vector<std::string> all_handles;

    cJSON2 *async_root = NULL;
    cJSON2 *jsonAllProgress = NULL;
    cJSON2 *jsonResponse = NULL;
    cJSON2 *node = NULL;
    cJSON2 *subNode = NULL;

    RF_GET_ALL_PROGRESS_SKIP(false /* doesn't matter */, response, rv);
    if (rv != 0) {
        LOG_ERROR("Unable to get the async task status, err = %d", rv);
        goto exit;
    }

    jsonAllProgress = cJSON2_Parse(response.c_str());
    if (jsonAllProgress == NULL) {
        LOG_ERROR("Unable to parse async task status as JSON");
        rv = VPL_ERR_FAIL;
        goto exit;
    }

    // get the deviceList array object
    async_root = cJSON2_GetObjectItem(jsonAllProgress, "requestList");
    if (async_root == NULL) {
        LOG_ERROR("Unable to get the \"requestList\" from the async task JSON response");
        rv = VPL_ERR_FAIL;
        goto exit;
    }

    for (int i = 0; i < cJSON2_GetArraySize(async_root); i++) {
        node = cJSON2_GetArrayItem(async_root, i);
        if (node == NULL) {
            continue;
        }
        subNode = cJSON2_GetObjectItem(node, "id");
        if (subNode == NULL || subNode->type != cJSON2_Number) {
            continue;
        }
        ss.str("");
        ss << subNode->valueint;
        all_handles.push_back(ss.str());
    }

    if (all_handles.empty() == true) {
        LOG_ERROR("No async handles in the async task queue");
        rv = VPL_ERR_FAIL;
        goto exit;
    }

    do {
        for (unsigned int i = 0; i < all_handles.size(); i++) {
            // getprogress <handle>

            // check if able to get progress
            RF_GET_PROGRESS_SKIP(is_media, all_handles[i].c_str(), response, rv);
            if (rv != 0) {
                LOG_ERROR("Unable to get the async task(%s) status, err = %d",
                          all_handles[i].c_str(), rv);
                rv = VPL_ERR_FAIL;
                goto exit;
            }

            // check if response is valid JSON format
            jsonResponse = cJSON2_Parse(response.c_str());
            if (jsonResponse == NULL) {
                LOG_ERROR("Unable to parse async task(%s) status as JSON",
                          all_handles[i].c_str());
                rv = VPL_ERR_FAIL;
                goto exit;
            }
            // check if response is valid async task response
            async_root = cJSON2_GetObjectItem(jsonResponse, "status");
            if (async_root == NULL || async_root->type != cJSON2_String) {
                LOG_ERROR("Async status (%s) \"status\" is not type string",
                          all_handles[i].c_str());
                rv = VPL_ERR_FAIL;
                goto exit;
            }
            cJSON2 *totalsize = cJSON2_GetObjectItem(jsonResponse, "totalSize");
            if (totalsize == NULL || totalsize->type != cJSON2_Number) {
                LOG_ERROR("Async status (%s) \"totalSize\" is not type number",
                          all_handles[i].c_str());
                rv = VPL_ERR_FAIL;
                goto exit;
            }
            cJSON2 *xferedsize = cJSON2_GetObjectItem(jsonResponse, "xferedSize");
            if (xferedsize == NULL || xferedsize->type != cJSON2_Number) {
                LOG_ERROR("Async status (%s) \"xferedSize\" is not type number",
                          all_handles[i].c_str());
                rv = VPL_ERR_FAIL;
                goto exit;
            }
            // wait for all handles finished no matter it's success or failed
            // sample response: {"id":1,"status":"done","totalSize":1024,"xferedSize":1024}
            if (strcmp("done", async_root->valuestring) == 0) {
                if (xferedsize->valueint != totalsize->valueint) {
                    LOG_ERROR("wrong size when async status (%s) is done. xferedSize="FMTs64" != totalSize="FMTs64,
                              all_handles[i].c_str(), xferedsize->valueint, totalsize->valueint);
                    rv = VPL_ERR_FAIL;
                    goto exit;
                }
                if (expect_fail == true) {
                    LOG_ERROR("successfully upload the file (%s) while we are expecting error",
                              all_handles[i].c_str());
                    rv = VPL_ERR_FAIL;
                    goto exit;
                }
                // pop
                all_handles.erase(all_handles.begin()+i);
            } else if (strcmp("error", async_root->valuestring) == 0) {
                if (expect_fail == false) {
                    LOG_ERROR("Async status (%s) is error", all_handles[i].c_str());
                    rv = VPL_ERR_FAIL;
                    goto exit;
                }
                // pop, we are expecting fail on the async upload.
                // clear the status instead of exit directly
                all_handles.erase(all_handles.begin()+i);
            } else if (strcmp("active", async_root->valuestring) == 0) {
                if (xferedsize->valueint > totalsize->valueint) {
                    LOG_ERROR("wrong xferedsize when async status (%s) is active. xferedSize="FMTs64" > totalSize="FMTs64,
                              all_handles[i].c_str(), xferedsize->valueint, totalsize->valueint);
                    rv = VPL_ERR_FAIL;
                    // exit directly, for it's an error
                    goto exit;
                }
            }
            if (jsonResponse != NULL) {
                cJSON2_Delete(jsonResponse);
                jsonResponse = NULL;
            }
        }
        // wait for file upload
        if (all_handles.empty() != true) {
            VPLThread_Sleep(VPLTIME_FROM_MILLISEC(300));
        } else {
            break;
        }
    } while (true);

exit:
    if (jsonResponse) {
        cJSON2_Delete(jsonResponse);
        jsonResponse = NULL;
    }
    if (jsonAllProgress == NULL) {
        cJSON2_Delete(jsonAllProgress);
    }
    return rv;
}

// Input: full file/dir path
// Output: filename or directory name
static std::string getFilenameFromPath(const std::string path) {

    size_t last;

    // possible file/dir path looks like /A/B/C/filename.ext or /A/B/C////
    // strip the trailing slashes and make it looks like : /A/B/C
    last = path.find_last_not_of("/");
    if (last != std::string::npos) {
        size_t start;
        // search for the last "/" which should now precede the filename
        start = path.find_last_of("/", last);
        if (start != std::string::npos) {
            return path.substr(start+1, last-start);
        } else {
            // cannot find any "/", return the whole string w/o trailing "/"
            return path.substr(0, last+1);
        }
    } else {
        // we cannot found any non "/". it can only be "////"
        if (path.size() > 0) {
            // since we don't find last "/", and the first character is "/"
            // then it should be string looks like "//////". Reduce into "/"
            // and return directly
            return "/";
        } else {
            // can only be empty string
            return "";
        }
    }
}

static int remotefile_access_checking(const std::string folderpath,
                                       const std::string &datasetId_str,
                                       const std::string &tmpfile,
                                       const std::string &tmpfile_local,
                                       std::vector <std::string> &filelists,
                                       std::vector <std::string> &filelists_local,
                                       bool is_media_rf,
                                       bool allowed_access,
                                       bool is_virt_folder,
                                       int  file_permission,
                                       int  dir_permission)
{
    int rv = VPL_OK;
    std::string response;
    int cloudPCId = 1;
    int clientPCId = 2;
    std::string alias_cloudpc = "CloudPC";
    std::string alias_client = "MD";

    TargetDevice *target = getTargetDevice();

    std::string cloudPCOSVersion;
    std::string separator;
    std::string subfolder = folderpath + "/rf_test";
    std::string subfolder2 = subfolder + "2";

    rv = target->getDirectorySeparator(separator);
    if (rv != 0) {
        LOG_ERROR("Unable to get the separator from target device, rv = %d", rv);
        goto exit;
    }

    LOG_ALWAYS("Separator = %s", separator.c_str());
    LOG_ALWAYS("Is media rf? %d", is_media_rf);
    LOG_ALWAYS("Allow access? %d", allowed_access);
    LOG_ALWAYS("Is virtual folder? %d", is_virt_folder);
    LOG_ALWAYS("File permission? %d", file_permission);
    LOG_ALWAYS("Dir permission? %d", dir_permission);
    LOG_ALWAYS("Testing folder: %s", folderpath.c_str());
    LOG_ALWAYS("Sub-folder: %s", subfolder.c_str());
    LOG_ALWAYS("Sub-folder (renamed): %s", subfolder2.c_str());
    LOG_ALWAYS("Tmp file: %s", tmpfile.c_str());
    LOG_ALWAYS("Tmp (local) file: %s", tmpfile_local.c_str());
    for (unsigned int i = 0; i < filelists.size(); i++) {
        LOG_ALWAYS("Upload file[%d]: %s", i, filelists[i].c_str());
    }

    // only clean-up the win32
    // for cloudnode, since the account is delete and recreate again before the testing
    // should check the os version of CloudPC
    delete target;
    target = NULL;
    SET_TARGET_MACHINE("RemoteFile", alias_cloudpc.c_str(), rv);
    if (rv < 0) {
        setCcdTestInstanceNum(cloudPCId);
    }

    target = getTargetDevice();
    LOG_ALWAYS("CloudPC osVersion: %s", target->getOsVersion().c_str());
    if (target->getOsVersion().find(OS_WINDOWS) != std::string::npos) {
        /* clean-up directory */
        std::string real_folder = convert_to_win32_real_folder(subfolder.c_str());
        std::string real_folder2 = convert_to_win32_real_folder(subfolder2.c_str());
        if (is_virt_folder) {
            std::string tmp = real_folder;
            size_t found = tmp.find("C::");
            if (found != std::string::npos) {
                tmp = tmp.substr(found);
                std::replace(tmp.begin(), tmp.end(), ':', '/');
                found = tmp.find_first_of("/", 0);
                if (found != std::string::npos) {
                    tmp.replace(found, 1, ":");
                }
            }
            real_folder = tmp;
            tmp = real_folder2;
            found = tmp.find("C::");
            if (found != std::string::npos) {
                tmp = tmp.substr(found);
                std::replace(tmp.begin(), tmp.end(), ':', '/');
                found = tmp.find_first_of("/", 0);
                if (found != std::string::npos) {
                    tmp.replace(found, 1, ":");
                }
            }
            real_folder2 = tmp;
        }
        LOG_ALWAYS("Clean-up folder: %s", real_folder.c_str());
        rv = target->removeDirRecursive(convert_path_convention(separator, real_folder));
        if (rv != 0) {
            LOG_ERROR("Unable to remove folder : %s, rv = %d", real_folder.c_str(), rv);
            rv = VPL_ERR_FAIL;
            goto exit;
        }
        LOG_ALWAYS("Clean-up folder: %s", real_folder2.c_str());
        rv = target->removeDirRecursive(convert_path_convention(separator, real_folder2));
        if (rv != 0) {
            LOG_ERROR("Unable to remove folder : %s, rv = %d", real_folder2.c_str(), rv);
            rv = VPL_ERR_FAIL;
            goto exit;
        }
    }

    delete target;
    target = NULL;
    SET_TARGET_MACHINE("RemoteFile", alias_client.c_str(), rv);
    if (rv < 0) {
        setCcdTestInstanceNum(clientPCId);
    }
    target = getTargetDevice();

    LOG_ALWAYS("\n\n==> read and try to make directory: %s\n", folderpath.c_str());

    // assumption, /rf is always granted w/ the permission (unless blocked by the UACL. but we should avoid that)
    // try access and create folder is not exist by /rf
    RF_READ_METADATA_SKIP(is_media_rf /* /rf */, datasetId_str, folderpath.c_str(), response, rv);
    if (rv != VPL_OK) {
        LOG_ERROR("Folder might not exist, try to create one: %s", folderpath.c_str());
        RF_MAKE_DIR_SKIP(is_media_rf /* /rf */, datasetId_str, folderpath.c_str(), response, rv);
        if (rv != VPL_OK && allowed_access == true) {
            LOG_ERROR("Unable to make directory: %s", folderpath.c_str());
            rv = VPL_ERR_FAIL;
            goto exit;
        }
    }

    LOG_ALWAYS("\n\n==> try to access directory: %s\n", folderpath.c_str());

    // try access folder
    RF_READ_METADATA_SKIP(is_media_rf, datasetId_str, folderpath.c_str(), response, rv);
    if (rv != VPL_OK && allowed_access == true) {
        LOG_ERROR("Unable to read metadata: %s", folderpath.c_str());
        rv = VPL_ERR_FAIL;
        goto exit;
    }
    if (rv == VPL_OK && allowed_access == false) {
        LOG_ERROR("Able to read metadata: %s", folderpath.c_str());
        rv = VPL_ERR_FAIL;
        goto exit;
    }

    LOG_ALWAYS("\n\n==> try to setpermission on directory: %s\n", folderpath.c_str());

    RF_SETPERM_SKIP(is_media_rf, datasetId_str, folderpath.c_str(), "isArchive=false", response, rv);
    if (rv != VPL_OK && allowed_access == true) {
        LOG_ERROR("Unable to set permission: %s", folderpath.c_str());
        rv = VPL_ERR_FAIL;
        goto exit;
    }
    if (rv == VPL_OK && allowed_access == false) {
        LOG_ERROR("Able to set permission: %s", folderpath.c_str());
        rv = VPL_ERR_FAIL;
        goto exit;
    }
    // check permission after set?

    LOG_ALWAYS("\n\n==> try to make directory: %s\n", subfolder.c_str());

    RF_MAKE_DIR_SKIP(is_media_rf, datasetId_str, subfolder.c_str(), response, rv);
    if (rv != VPL_OK && allowed_access == true) {
        LOG_ERROR("Unable to make directory: %s", subfolder.c_str());
        rv = VPL_ERR_FAIL;
        goto exit;
    }
    if (rv == VPL_OK && allowed_access == false) {
        LOG_ERROR("Able to make directory: %s", subfolder.c_str());
        rv = VPL_ERR_FAIL;
        goto exit;
    }

    LOG_ALWAYS("\n\n==> try to read directory: %s\n", subfolder.c_str());

    RF_READ_DIR_SKIP(is_media_rf, datasetId_str, subfolder.c_str(), response, rv);
    if (rv != VPL_OK && allowed_access == true) {
        LOG_ERROR("Unable to read metadata: %s", subfolder.c_str());
        rv = VPL_ERR_FAIL;
        goto exit;
    }
    if (rv == VPL_OK && allowed_access == false) {
        LOG_ERROR("Able to read metadata: %s", subfolder.c_str());
        rv = VPL_ERR_FAIL;
        goto exit;
    }

    LOG_ALWAYS("\n\n==> try to move directory: %s -> %s\n", subfolder.c_str(), subfolder2.c_str());

    RF_MOVE_DIR_SKIP(is_media_rf, datasetId_str, subfolder.c_str(), subfolder2.c_str(), response, rv);
    if (rv != VPL_OK && allowed_access == true) {
        LOG_ERROR("Unable to move directory: %s -> %s", subfolder.c_str(), subfolder2.c_str());
        rv = VPL_ERR_FAIL;
        goto exit;
    }
    if (rv == VPL_OK && allowed_access == false) {
        LOG_ERROR("Able to move directory: %s -> %s", subfolder.c_str(), subfolder2.c_str());
        rv = VPL_ERR_FAIL;
        goto exit;
    }

    LOG_ALWAYS("\n\n==> try to access deleted directory (should be failed): %s\n", subfolder.c_str());

    RF_READ_METADATA_SKIP(is_media_rf, datasetId_str, subfolder.c_str(), response, rv); // XXX expected fail
    if (rv == VPL_OK) {
        LOG_ERROR("Able to read metadata after moved directory: %s -> %s", subfolder.c_str(), subfolder2.c_str());
        rv = VPL_ERR_FAIL;
        goto exit;
    }

    LOG_ALWAYS("\n\n==> try to access directory: %s\n", subfolder2.c_str());

    RF_READ_METADATA_SKIP(is_media_rf, datasetId_str, subfolder2.c_str(), response, rv);
    if (rv != VPL_OK && allowed_access == true) {
        LOG_ERROR("Unable to read metadata: %s", subfolder2.c_str());
        rv = VPL_ERR_FAIL;
        goto exit;
    }
    if (rv == VPL_OK && allowed_access == false) {
        LOG_ERROR("Able to read metadata: %s", subfolder2.c_str());
        rv = VPL_ERR_FAIL;
        goto exit;
    }

    // Test RF operation on files, upload/download/copy/move/read/delete
    // for-loop to go through filelists
    for (unsigned int i = 0; i < filelists.size(); i++) {
        std::string upload_file = filelists[i];
        std::string file1 = subfolder2 + "/" + getFilenameFromPath(filelists[i]);

        LOG_ALWAYS("\n\n==> submit upload file[%d]: %s --> %s\n", i, filelists[i].c_str(), file1.c_str());

        // upload file to file1 at storage node
        // it's always success to submit the task, but the status will be error if it's not allowable
        RF_XUPLOAD_SKIP(is_media_rf, datasetId_str, upload_file.c_str(), file1.c_str(), response, rv);
        if (rv != VPL_OK) {
            LOG_ERROR("Unable to submit xupload task: %s -> %s", upload_file.c_str(), file1.c_str());
            rv = VPL_ERR_FAIL;
            goto exit;
        }
    }

    if (filelists.size() > 0) {
        LOG_ALWAYS("\n\n==> wait for files to be uploaded, size = %d\n", filelists.size());

        // Wait for uploaded by get progress
        rv = remotefile_wait_async_upload_done(is_media_rf, !allowed_access);
        if (rv != VPL_OK && allowed_access == true) {
            LOG_ERROR("Async upload failed");
            rv = VPL_ERR_FAIL;
            goto exit;
        }
        if (rv != VPL_OK && allowed_access == false) {
            LOG_ERROR("Able to get unfinished async upload status and wait for it done");
            rv = VPL_ERR_FAIL;
            goto exit;
        }

        LOG_ALWAYS("\n\n==> try to read directory again: %s\n", subfolder2.c_str());

        RF_READ_DIR_SKIP(is_media_rf, datasetId_str, subfolder2.c_str(), response, rv);
        if (rv != VPL_OK && allowed_access == true) {
            LOG_ERROR("Unable to read metadata: %s", subfolder2.c_str());
            rv = VPL_ERR_FAIL;
            goto exit;
        }
        if (rv == VPL_OK && allowed_access == false) {
            LOG_ERROR("Able to read metadata: %s", subfolder2.c_str());
            rv = VPL_ERR_FAIL;
            goto exit;
        }

       

        // only perform on the first file
        std::string upload_file = filelists[0];
        std::string upload_file_local = filelists_local[0];
        std::string file1 = subfolder2 + "/" + getFilenameFromPath(filelists[0]);
        std::string file2 = file1 + "2";    //copy file1 to file2
        std::string file3 = file1 + "3";    //move file1 to file3
        std::string file4 = subfolder2 + "/" + getFilenameFromPath(filelists[1]);
        std::string file5 = file4 + "2";    //copy file4 to file5

        // set file1 permission on remote after uploading
        // if we need set parent permission later, we need to set the file first(to be not inheriated from parent)
        if(!is_write(file_permission) || !is_write(dir_permission)){
            std::string mode;
            if(is_write(file_permission))
                mode = "rwx";
            else if(is_read(file_permission))
                mode = "rx";
            else
                mode = "n";

            LOG_ALWAYS("\n\n==> try to setFilePermission: %s -> %d\n", file1.c_str(), file_permission);
            std::string real_folder = convert_to_win32_real_folder(subfolder2.c_str());

            std::string real_file;   
            real_file = real_folder + "/" + getFilenameFromPath(filelists[0]);

            //set target to cloudpc
            delete target;
            target = NULL;
            SET_TARGET_MACHINE("RemoteFile", alias_cloudpc.c_str(), rv);
            target = getTargetDevice();

            rv = target->setFilePermission(real_file, mode);
            if (rv != VPL_OK && allowed_access == true) {
                LOG_ERROR("Unable to set permission, rv = %d", rv);
                goto exit;
            }
            real_file = real_folder + "/" + getFilenameFromPath(filelists[1]);
            rv = target->setFilePermission(real_file, mode);
            if (rv != VPL_OK && allowed_access == true) {
                LOG_ERROR("Unable to set permission, rv = %d", rv);
                goto exit;
            }
        }

        // set parent folder permission on remote after uploading
        if(!is_write(dir_permission)){
            std::string mode;
            if(is_read(dir_permission))
                mode = "rx";
            else
                mode = "n";

            std::string real_folder = convert_to_win32_real_folder(subfolder2.c_str());
            LOG_ALWAYS("\n\n==> try to setFilePermission: %s -> %d\n", real_folder.c_str(), dir_permission);

            //set target to cloudpc
            delete target;
            target = NULL;
            SET_TARGET_MACHINE("RemoteFile", alias_cloudpc.c_str(), rv);
            target = getTargetDevice();

            rv = target->setFilePermission(real_folder, mode);
            if (rv != VPL_OK && allowed_access == true) {
                LOG_ERROR("Unable to set permission, rv = %d", rv);
                goto exit;
            }
        }

        // clean-up download file
        delete target;
        target = NULL;
        SET_TARGET_MACHINE("RemoteFile", alias_client.c_str(), rv);
        target = getTargetDevice();

        rv = target->deleteFile(convert_path_convention(separator, tmpfile));
        if (rv != 0 && rv != VPL_ERR_NOENT) {
            LOG_ERROR("Unable to delete file: %s, rv = %d", tmpfile.c_str(), rv);
            rv = VPL_ERR_FAIL;
            goto exit;
        }
        Util_rm_dash_rf(tmpfile_local);

        LOG_ALWAYS("\n\n==> try to download file: %s -> %s\n", file1.c_str(), tmpfile.c_str());

        // verify uploaded file by download and compare with the upload file
        RF_DOWNLOAD_SKIP(is_media_rf, datasetId_str, file1.c_str(), tmpfile.c_str(), response, rv);
        if (rv != VPL_OK && allowed_access == true && is_read(file_permission)) {
            LOG_ERROR("Unable to download file: %s -> %s", file1.c_str(), tmpfile.c_str());
            rv = VPL_ERR_FAIL;
            goto exit;
        }
        if (rv == VPL_OK && (allowed_access == false || !is_read(file_permission))) {
            LOG_ERROR("Able to download file: %s -> %s", file1.c_str(), tmpfile.c_str());
            rv = VPL_ERR_FAIL;
            goto exit;
        }
        // compare file content
        if (allowed_access == true && is_read(file_permission)) {
            rv = target->pullFile(convert_path_convention(separator, tmpfile), tmpfile_local);
            if (rv != 0) {
                LOG_ERROR("Unable to pull file from client-pc to control-pc: %s, %s",
                          tmpfile.c_str(), tmpfile_local.c_str());
                rv = VPL_ERR_FAIL;
                goto exit;
            }
            rv = file_compare(tmpfile_local.c_str(), upload_file_local.c_str());
            if (rv != VPL_OK) {
                LOG_ERROR("Download file mismatched: %s <> %s", tmpfile.c_str(), upload_file.c_str());
                rv = VPL_ERR_FAIL;
                goto exit;
            }
        }

#if !defined(CLOUDNODE)
        //bug15662, set permission on file will fail on cloudnode
        LOG_ALWAYS("\n\n==> try to set permission(RF_SETPERM) of file: %s -> isArchive=false\n", file1.c_str());

        // setperm
        RF_SETPERM_SKIP(is_media_rf, datasetId_str, file1.c_str(), "isArchive=false", response, rv);
        if (rv != VPL_OK && allowed_access == true && is_read(file_permission) && is_write(file_permission)) {
            LOG_ERROR("Unable to setpermission file: %s -> isArchive=false", file1.c_str());
            rv = VPL_ERR_FAIL;
            goto exit;
        }
        if (rv == VPL_OK && (allowed_access == false || !is_read(file_permission) || !is_write(file_permission))) {
            LOG_ERROR("Able to setpermission file: %s -> isArchive=false", file1.c_str());
            rv = VPL_ERR_FAIL;
            goto exit;
        }
#endif

        LOG_ALWAYS("\n\n==> try to copy file: %s -> %s\n", file1.c_str(), file2.c_str());

        // copy file1 to file2, move file1 to file3
        RF_COPY_FILE_SKIP(is_media_rf, datasetId_str, file1.c_str(), file2.c_str(), response, rv);
        if (rv != VPL_OK && allowed_access == true && is_read(file_permission) && is_write(dir_permission)) {
            LOG_ERROR("Unable to copy file: %s -> %s", file1.c_str(), file2.c_str());
            rv = VPL_ERR_FAIL;
            goto exit;
        }
        if (rv == VPL_OK && (allowed_access == false || !is_read(file_permission) || !is_write(dir_permission))) {
            LOG_ERROR("Able to copy file: %s -> %s", file1.c_str(), file2.c_str());
            rv = VPL_ERR_FAIL;
            goto exit;
        }

        LOG_ALWAYS("\n\n==> try to move file: %s -> %s\n", file1.c_str(), file3.c_str());

        RF_MOVE_FILE_SKIP(is_media_rf, datasetId_str, file1.c_str(), file3.c_str(), response, rv);
        if (rv != VPL_OK && allowed_access == true && is_read(file_permission) && is_write(file_permission) && is_write(dir_permission)) {
            LOG_ERROR("Unable to move file: %s -> %s", file1.c_str(), file3.c_str());
            rv = VPL_ERR_FAIL;
            goto exit;
        }
        if (rv == VPL_OK && (allowed_access == false || !is_read(file_permission) || !is_write(file_permission) || !is_write(dir_permission))) {
            LOG_ERROR("Able to move file: %s -> %s", file1.c_str(), file3.c_str());
            rv = VPL_ERR_FAIL;
            goto exit;
        }

        LOG_ALWAYS("\n\n==> try to copy large file(1M): %s -> %s\n", file4.c_str(), file5.c_str());

        // copy file4 to file5
        RF_COPY_FILE_SKIP(is_media_rf, datasetId_str, file4.c_str(), file5.c_str(), response, rv);
        if (rv != VPL_OK && allowed_access == true && is_read(file_permission) && is_write(dir_permission)) {
            LOG_ERROR("Unable to copy file: %s -> %s", file4.c_str(), file5.c_str());
            rv = VPL_ERR_FAIL;
            goto exit;
        }
        if (rv == VPL_OK && (allowed_access == false || !is_read(file_permission) || !is_write(dir_permission))) {
            LOG_ERROR("Able to copy file: %s -> %s", file4.c_str(), file5.c_str());
            rv = VPL_ERR_FAIL;
            goto exit;
        }

        LOG_ALWAYS("\n\n==> try to access moved src file (should be failed): %s\n", file1.c_str());

        // read file1,2,3 after move/copy (file1 should be missing)
        RF_READ_METADATA_SKIP(is_media_rf, datasetId_str, file1.c_str(), response, rv); // XXX expected fail
        if (rv == VPL_OK && is_write(file_permission) && is_write(dir_permission)) {
            LOG_ERROR("Able to read metadata after moved file: %s -> %s", file1.c_str(), file3.c_str());
            rv = VPL_ERR_FAIL;
            goto exit;
        }

        LOG_ALWAYS("\n\n==> try to access copied file: %s\n", file2.c_str());

        RF_READ_METADATA_SKIP(is_media_rf, datasetId_str, file2.c_str(), response, rv);
        if (rv != VPL_OK && allowed_access == true && is_read(file_permission) && is_write(dir_permission)) {
            LOG_ERROR("Unable to read metadata: %s", file2.c_str());
            rv = VPL_ERR_FAIL;
            goto exit;
        }
        if (rv == VPL_OK && (allowed_access == false || !is_read(file_permission) || !is_write(dir_permission))) {
            LOG_ERROR("Able to read metadata: %s", file2.c_str());
            rv = VPL_ERR_FAIL;
            goto exit;
        }

        LOG_ALWAYS("\n\n==> try to access copied file: %s\n", file5.c_str());

        RF_READ_METADATA_SKIP(is_media_rf, datasetId_str, file5.c_str(), response, rv);
        if (rv != VPL_OK && allowed_access == true && is_read(file_permission) && is_write(dir_permission)) {
            LOG_ERROR("Unable to read metadata: %s", file5.c_str());
            rv = VPL_ERR_FAIL;
            goto exit;
        }
        if (rv == VPL_OK && (allowed_access == false || !is_read(file_permission) || !is_write(dir_permission))) {
            LOG_ERROR("Able to read metadata: %s", file5.c_str());
            rv = VPL_ERR_FAIL;
            goto exit;
        }

        LOG_ALWAYS("\n\n==> try to access moved target file: %s\n", file3.c_str());

        RF_READ_METADATA_SKIP(is_media_rf, datasetId_str, file3.c_str(), response, rv);
        if (rv != VPL_OK && allowed_access == true && is_read(file_permission && is_write(file_permission) && is_write(dir_permission))) {
            LOG_ERROR("Unable to read metadata: %s", file3.c_str());
            rv = VPL_ERR_FAIL;
            goto exit;
        }
        if (rv == VPL_OK && (allowed_access == false || !is_read(file_permission) || !is_write(file_permission) || !is_write(dir_permission))) {
            LOG_ERROR("Able to read metadata: %s", file3.c_str());
            rv = VPL_ERR_FAIL;
            goto exit;
        }

        LOG_ALWAYS("\n\n==> clean-up files\n");

        // clean-up file 1, 2, 3
        //file1, unless it's not writable, file1 should have been moved to file2 and no longer exist
        if(!is_write(file_permission) || !is_write(dir_permission)){
            RF_DELETE_FILE_SKIP(is_media_rf, datasetId_str, file1.c_str(), response, rv);
            //if dir is read-only, but file is read-write, it still can be deleted
            //http://windows.microsoft.com/en-us/windows-vista/prevent-changes-to-a-file-or-folder-read-only
            if (rv != VPL_OK && allowed_access == true && is_read(file_permission) && is_write(file_permission)) {
                LOG_ERROR("Unable to delete file: %s", file1.c_str());
                rv = VPL_ERR_FAIL;
                goto exit;
            }
            if (rv == VPL_OK && (allowed_access == false || !is_read(file_permission) || !is_write(file_permission))) {
                LOG_ERROR("Able to delete file: %s", file1.c_str());
                rv = VPL_ERR_FAIL;
                goto exit;
            }
        }
        //file2 
        RF_DELETE_FILE_SKIP(is_media_rf, datasetId_str, file2.c_str(), response, rv);
        if (rv != VPL_OK && allowed_access == true && is_read(file_permission) && is_write(dir_permission)) {
            LOG_ERROR("Unable to delete file: %s", file2.c_str());
            rv = VPL_ERR_FAIL;
            goto exit;
        }
        if (rv == VPL_OK && (allowed_access == false || !is_read(file_permission) || !is_write(dir_permission))) {
            LOG_ERROR("Able to delete file: %s", file2.c_str());
            rv = VPL_ERR_FAIL;
            goto exit;
        }
        RF_DELETE_FILE_SKIP(is_media_rf, datasetId_str, file3.c_str(), response, rv);
        if (rv != VPL_OK && allowed_access == true && is_read(file_permission) && is_write(file_permission) && is_write(dir_permission)) {
            LOG_ERROR("Unable to delete file: %s", file3.c_str());
            rv = VPL_ERR_FAIL;
            goto exit;
        }
        if (rv == VPL_OK && (allowed_access == false || !is_read(file_permission) || !is_write(file_permission) || !is_write(dir_permission))) {
            LOG_ERROR("Able to delete file: %s", file3.c_str());
            rv = VPL_ERR_FAIL;
            goto exit;
        }
        RF_READ_METADATA_SKIP(is_media_rf, datasetId_str, file2.c_str(), response, rv); // XXX expected fail
        if (rv == VPL_OK) {
            LOG_ERROR("Able to read metadata: %s", file2.c_str());
            rv = VPL_ERR_FAIL;
            goto exit;
        }
        RF_READ_METADATA_SKIP(is_media_rf, datasetId_str, file3.c_str(), response, rv); // XXX expected fail
        if (rv == VPL_OK) {
            LOG_ERROR("Able to read metadata: %s", file3.c_str());
            rv = VPL_ERR_FAIL;
            goto exit;
        }
        // clean-up file5
        RF_DELETE_FILE_SKIP(is_media_rf, datasetId_str, file5.c_str(), response, rv);
        if (rv != VPL_OK && allowed_access == true && is_read(file_permission) && is_write(dir_permission)) {
            LOG_ERROR("Unable to delete file: %s", file5.c_str());
            rv = VPL_ERR_FAIL;
            goto exit;
        }
        if (rv == VPL_OK && (allowed_access == false || !is_read(file_permission) || !is_write(dir_permission))) {
            LOG_ERROR("Able to delete file: %s", file5.c_str());
            rv = VPL_ERR_FAIL;
            goto exit;
        }
        RF_READ_METADATA_SKIP(is_media_rf, datasetId_str, file5.c_str(), response, rv); // XXX expected fail
        if (rv == VPL_OK) {
            LOG_ERROR("Able to read metadata: %s", file5.c_str());
            rv = VPL_ERR_FAIL;
            goto exit;
        }
    }

    // set parent folder full permission before cleanup
    if(!is_write(dir_permission)){

        std::string real_folder = convert_to_win32_real_folder(subfolder2.c_str());
        LOG_ALWAYS("\n\n==> try to setFilePermission: %s -> %d\n", real_folder.c_str(), dir_permission);

        //set target to cloudpc
        delete target;
        target = NULL;
        SET_TARGET_MACHINE("RemoteFile", alias_cloudpc.c_str(), rv);
        target = getTargetDevice();

        rv = target->setFilePermission(real_folder, "rwx");
        if (rv != VPL_OK && allowed_access == true) {
            LOG_ERROR("Unable to set permission, rv = %d", rv);
            goto exit;
        }

        //set target to MD
        delete target;
        target = NULL;
        SET_TARGET_MACHINE("RemoteFile", alias_client.c_str(), rv);
        target = getTargetDevice();
    }

    // for-loop to go through filelists for clean-up
    for (unsigned int i = 0; i < filelists.size(); i++) {
        std::string upload_file = filelists[i];
        std::string file1 = subfolder2 + "/" + getFilenameFromPath(filelists[i]);

        LOG_ALWAYS("\n\n==> delete file[%d]: %s\n", i, file1.c_str());

        //skip first 1. for it's cleaned up above
        if(i == 0 && is_write(file_permission)){
            continue;
        }
        // reset file full permission for file1/file4 before clean-up
        if(!is_write(file_permission)){

            LOG_ALWAYS("\n\n==> try to setFilePermission: %s -> %d\n", file1.c_str(), file_permission);
            std::string real_folder = convert_to_win32_real_folder(subfolder2.c_str());

            std::string real_file;   
            real_file = real_folder + "/" + getFilenameFromPath(filelists[i]);

            //set target to cloudpc
            delete target;
            target = NULL;
            SET_TARGET_MACHINE("RemoteFile", alias_cloudpc.c_str(), rv);
            target = getTargetDevice();

            rv = target->setFilePermission(real_file, "rwx");
            if (rv != VPL_OK && allowed_access == true) {
                LOG_ERROR("Unable to set permission, rv = %d", rv);
                goto exit;
            }
        }

        delete target;
        target = NULL;
        SET_TARGET_MACHINE("RemoteFile", alias_client.c_str(), rv);
        target = getTargetDevice();
        // delete file
        RF_DELETE_FILE_SKIP(is_media_rf, datasetId_str, file1.c_str(), response, rv);
        if (rv != VPL_OK && allowed_access == true) {
            LOG_ERROR("Unable to delete file: %s", file1.c_str());
            rv = VPL_ERR_FAIL;
            goto exit;
        }
        if (rv == VPL_OK && allowed_access == false) {
            LOG_ERROR("Able to delete file: %s", file1.c_str());
            rv = VPL_ERR_FAIL;
            goto exit;
        }
        // try accesss deleted file
        RF_READ_METADATA_SKIP(is_media_rf, datasetId_str, file1.c_str(), response, rv); // XXX expected fail
        if (rv == VPL_OK) {
            LOG_ERROR("Able to read metadata: %s", file1.c_str());
            rv = VPL_ERR_FAIL;
            goto exit;
        }
    }

    LOG_ALWAYS("\n\n==> clean-up dir: %s\n", subfolder2.c_str());

    // clean-up directory
    RF_DELETE_DIR_SKIP(is_media_rf, datasetId_str, subfolder2.c_str(), response, rv);
    if (rv != VPL_OK && allowed_access == true) {
        LOG_ERROR("Unable to delete dir: %s", subfolder2.c_str());
        rv = VPL_ERR_FAIL;
        goto exit;
    }
    if (rv == VPL_OK && allowed_access == false) {
        LOG_ERROR("Able to delete dir: %s", subfolder2.c_str());
        rv = VPL_ERR_FAIL;
        goto exit;
    }

    RF_READ_METADATA_SKIP(is_media_rf, datasetId_str, subfolder2.c_str(), response, rv); // XXX expected fail
    if (rv == VPL_OK) {
        LOG_ERROR("Able to read metadata: %s", subfolder2.c_str());
        rv = VPL_ERR_FAIL;
        goto exit;
    }
    // if we gets here, then it's passed
    rv = VPL_OK;

exit:
    delete target;

    return rv;
}

static void get_media_libraries(folder_access_rules &folder, bool is_media_rf, bool is_full_test)
{
    TargetDevice *target = getTargetDevice();
    int rv = 0;

    std::vector<std::string> lib_types;
    std::vector<std::string>::iterator lib_it;

    std::vector<std::pair<std::string, std::string> > pathes;
    std::vector<std::pair<std::string, std::string> >::iterator it;

    // (Video/Music/Photo/Documents/Generic)
    if (is_full_test && is_media_rf) {
        lib_types.push_back("Video");
        lib_types.push_back("Music");
        lib_types.push_back("Photo");
    }
    else if (is_full_test) {
        lib_types.push_back("Video");
        lib_types.push_back("Music");
        lib_types.push_back("Photo");
        lib_types.push_back("Documents");
        lib_types.push_back("Generic");
    }
    else {
        lib_types.push_back("Photo");
    }

    bool allow_access = false;
    //FIXME: why we clear folder here, this reference already have some folders before this function callled
    folder.clear();
    for (lib_it = lib_types.begin(); lib_it != lib_types.end(); lib_it++) {

        LOG_ALWAYS("--> Appending %s library for %s", lib_it->c_str(), (is_media_rf? "MediaRF" : "RF"));

        pathes.clear();
        rv = target->readLibrary(*lib_it, pathes);

        if (*lib_it == "Music" ||
            *lib_it == "Video" ||
            *lib_it == "Photo") {
            allow_access = true;
        } else {
            allow_access = is_media_rf == true? false : true;
        }

        // subfolders
        bool found_public = false;
        bool found_users = false;
        for (it = pathes.begin(); it != pathes.end(); it++) {
            std::string subfolder = convert_to_win32_virt_folder(it->first.c_str());
            std::string subfolder_virt = it->second;

            if (subfolder.find("Users/Public") != std::string::npos) {
                if (found_public == true) {
                    continue;
                } else {
                    found_public = true;
                }
            } else {
                if (found_users == true) {
                    continue;
                } else {
                    found_users = true;
                }
            }
            LOG_ALWAYS("---> Real: %s, Virt: %s", subfolder.c_str(),  subfolder_virt.c_str());
            // put the real folder before the virtual folder to do the clean-up
            folder.push_back(folder_item(subfolder, allow_access, false));
            folder.push_back(folder_item(subfolder_virt, allow_access, true));
            // if we found one public + one user's private folder then break
            if (found_users && found_public) {
                break;
            }
        }
    }
    delete target;
}

static int remotefile_read_dir_pagination(bool is_media_rf,
                                          const std::string &datasetId_str,
                                          const std::string &read_dir_pagi,
                                          const std::vector< std::pair<std::string, VPLFS_file_size_t> > &tmp_filelist,
                                          const std::string &sortBy)
{
    int rv = VPL_OK;
    int max = 5;
    int index = 2;
    std::string response;
    std::ostringstream oss;
    std::string sort_params;
    cJSON2 *jsonResponse = NULL;
    cJSON2 *filelist  = NULL;

    cJSON2 *node = NULL;
    cJSON2 *nextnode = NULL;
    cJSON2 *subNode = NULL;
    cJSON2 *nextsubNode = NULL;

    oss << "sortBy=" << sortBy << ";index=" << index << ";max=" << max;
    sort_params = oss.str();

    LOG_ALWAYS("\n\n== ReadDirWithPagination(%s / %s) ==", read_dir_pagi.c_str(), sort_params.c_str());

    if ((max+index-1) > (int)tmp_filelist.size()) {
        LOG_ERROR("Index + Max is bigger then the total file nubers");
        return VPL_ERR_FAIL;
    }

    // read dir with pagination parameters
    RF_READ_DIR_PAGI(is_media_rf, datasetId_str, read_dir_pagi.c_str(), sort_params.c_str(), response, rv);
    jsonResponse = cJSON2_Parse(response.c_str());
    if (jsonResponse == NULL) {
        LOG_ERROR("Invalid JSON response from the read directory operation");
        rv = VPL_ERR_FAIL;
        goto exit;
    }
    // get the deviceList array object
    filelist = cJSON2_GetObjectItem(jsonResponse, "fileList");
    if (filelist == NULL) {
        LOG_ERROR("Invalid JSON response from the read directory operation");
        rv = VPL_ERR_FAIL;
        goto exit;
    }

    if (cJSON2_GetArraySize(filelist) != max) {
        LOG_ERROR("Unexpected file number %d, expected %d", cJSON2_GetArraySize(filelist), max);
        rv = VPL_ERR_FAIL;
        goto exit;
    }

    for (int i = 0; i < cJSON2_GetArraySize(filelist) - 1; i++) {
        node = cJSON2_GetArrayItem(filelist, i);
        if (node == NULL) {
            continue;
        }
        nextnode = cJSON2_GetArrayItem(filelist, i+1);
        if (nextnode == NULL) {
            continue;
        }
        if (sortBy == "alpha") {
            subNode = cJSON2_GetObjectItem(node, "name");
            if (subNode == NULL || subNode->type != cJSON2_String) {
                LOG_ERROR("Unable to get the name entry");
                rv = VPL_ERR_FAIL;
                break;
            }
            nextsubNode = cJSON2_GetObjectItem(nextnode, "name");
            if (nextsubNode == NULL || nextsubNode->type != cJSON2_String) {
                LOG_ERROR("Unable to get the next name entry");
                rv = VPL_ERR_FAIL;
                break;
            }
            std::string name = subNode->valuestring;
            std::string nextname = nextsubNode->valuestring;
            if (name > nextname) {
                LOG_ERROR("File are not sorted in alphabetical order!");
                rv = VPL_ERR_FAIL;
                break;
            }
        } else if (sortBy == "time") {
            subNode = cJSON2_GetObjectItem(node, "lastChanged");
            if (subNode == NULL || subNode->type != cJSON2_Number) {
                LOG_ERROR("Unable to get the lastChanged entry");
                rv = VPL_ERR_FAIL;
                break;
            }
            nextsubNode = cJSON2_GetObjectItem(nextnode, "lastChanged");
            if (nextsubNode == NULL || nextsubNode->type != cJSON2_Number) {
                LOG_ERROR("Unable to get the next lastChanged entry");
                rv = VPL_ERR_FAIL;
                break;
            }
            s64 time = subNode->valueint;
            s64 nexttime = nextsubNode->valueint;
            if (time < nexttime) {
                LOG_ERROR("File are not sorted in time order!");
                rv = VPL_ERR_FAIL;
                break;
            }
        } else if (sortBy == "size") {
            subNode = cJSON2_GetObjectItem(node, "size");
            if (subNode == NULL || subNode->type != cJSON2_Number) {
                LOG_ERROR("Unable to get the size entry");
                rv = VPL_ERR_FAIL;
                break;
            }
            nextsubNode = cJSON2_GetObjectItem(nextnode, "size");
            if (nextsubNode == NULL || nextsubNode->type != cJSON2_Number) {
                LOG_ERROR("Unable to get the next size entry");
                rv = VPL_ERR_FAIL;
                break;
            }
            s64 size = subNode->valueint;
            s64 nextsize = nextsubNode->valueint;
            if (size < nextsize) {
                LOG_ERROR("File are not sorted in file size order!");
                rv = VPL_ERR_FAIL;
                break;
            }
        } else {
            LOG_ERROR("Requested sortBy method is not supported, sortBy=%s", sortBy.c_str());
            rv = VPL_ERR_FAIL;
            break;
        }
    }

exit:
    if (jsonResponse != NULL) {
        cJSON2_Delete(jsonResponse);
    }
    return rv;
}

struct RFDownloadParams
{
    RFDownloadParams* rfdownload_params;
    VPLMutex_t *mutex;
    bool is_media;
    std::string alias;
    std::string datasetId_str;
    std::string download_from;
    std::string download_to;
    std::string tc_name;
};

static VPLThread_return_t downloader(VPLThread_arg_t arg)
{
    RFDownloadParams *download_params = (RFDownloadParams*)arg;

    LOG_ALWAYS("Download file: %s to %s", download_params->download_from.c_str(),
        download_params->download_to.c_str());

    int rv = 0;
    u64 userId = 0;
    std::string response;
    std::stringstream ss_ip;
    std::stringstream ss_port;
    std::string ip_addr_str;
    std::string port_str;
    VPLNet_addr_t ip_addr;
    VPLNet_port_t port_num;

    rv = VPLMutex_Lock(download_params->mutex);
    if (rv != VPL_OK) {
        LOG_ERROR("VPLMutex_Lock failed %d", rv);
        goto exit;
    }
    rv = set_target_machine(download_params->alias.c_str(), ip_addr, port_num);
    if (rv == VPL_ERR_FAIL) {
        LOG_ERROR("set_target_machine %s failed!", download_params->alias.c_str());
    }
    ss_ip.clear();
    ss_ip << ip_addr << std::endl;
    ss_port.clear();
    ss_port << port_num << std::endl;
    rv = VPLMutex_Unlock(download_params->mutex);
    if (rv != VPL_OK) {
        LOG_ERROR("VPLMutex_Unlock failed %d", rv);
        goto exit;
    }

    ip_addr_str = ss_ip.str();
    port_str = ss_port.str();
    rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        goto exit;
    }
    rv = fs_test_download_by_ip_port(userId, download_params->datasetId_str, download_params->download_from, download_params->download_to,
        "", ip_addr_str, port_str, response, download_params->is_media);
    if (rv == 0) {
        LOG_ALWAYS("Download %s to %s PASS!!!", download_params->download_from.c_str(), download_params->download_to.c_str());
    }
    else {
        LOG_ERROR("Download %s to %s FAIL!!!", download_params->download_from.c_str(), download_params->download_to.c_str());
    }

exit:
    return (VPLThread_return_t)rv;
}

#define CREATE_REMOTEFILE_DOWNLOAD_THREAD(testStr, params) \
    BEGIN_MULTI_STATEMENT_MACRO \
    VPLThread_t thread; \
    LOG_ALWAYS("Create download thread!!!"); \
    rv = VPLThread_Create(&thread, downloader, (VPLThread_arg_t)&params, NULL, "downloader"); \
    if (rv != VPL_OK) { \
        LOG_ERROR("Failed to spawn event listen thread: %d", rv); \
    } \
    CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, testStr, rv); \
    END_MULTI_STATEMENT_MACRO

#define RF_UPLOAD_DIR "[LOCALAPPDATA]/clear.fi/rf_autotest"
#define RF_DUMMY_1K_FILE "dummy1KB.raw"
#define RF_DUMMY_1M_FILE "dummy1MB.raw"
#define RF_DUMMY_20M_FILE "dummy20MB.raw"

int autotest_sdk_release_remotefile_basic(bool is_media,
                                          bool is_support_media,
                                                 bool full,
                                                 u64 userId,
                                                 u64 deviceId,
                                                 const std::string &datasetId_str,
                                                 const std::string &work_dir,
                                                 const std::string &work_dir_client,
                                                 const std::string &work_dir_local,
                                                 const std::string &upload_dir,
                                                 const std::vector< std::pair<std::string, VPLFS_file_size_t> > &temp_filelist,
                                                 const std::string &cloudPCSeparator,
                                                 const std::string &clientPCSeparator,
                                                 const std::string &clientPCSeparator2)
{

    int rv = 0;
    std::string response;
    std::string alias_cloudpc = "CloudPC";
    std::string alias_client  = "MD";
    std::string alias_client2 = "Client";
    //int active_req_num = -1;
    VPLFS_file_size_t upload_size = 0;
    RFDownloadParams params;
    std::stringstream ss_ip;
    std::stringstream ss_port;
    std::string ip_addr_str;
    std::string port_str;
    VPLNet_addr_t ip_addr;
    VPLNet_port_t port;
    VPLMutex_t thread_state_mutex;
    //std::vector< std::pair<std::string, VPLFS_file_size_t> > temp_filelist;

    std::string upload_file_copy = upload_dir+"/"+RF_TEST_LARGE_FILE+".copy";
    std::string upload_file      = upload_dir+"/"+RF_TEST_LARGE_FILE;
    std::string upload_file2     = upload_dir+"/"+RF_DUMMY_20M_FILE;

    std::string test_clip_file       = work_dir        + "/" + RF_TEST_LARGE_FILE;
    std::string test_clip_file2      = work_dir_client + "/" + RF_DUMMY_20M_FILE;
    std::string test_clip_file_local = work_dir_local  + "/" + RF_TEST_LARGE_FILE;

    std::string download_file        = work_dir        + "/" + RF_TEST_LARGE_FILE + ".clone";
    std::string download_file2       = work_dir_client + "/" + RF_DUMMY_20M_FILE  + ".clone2";
    std::string download_file_local  = work_dir_local  + "/" + RF_TEST_LARGE_FILE + ".clone";
    std::string download_file_local2 = work_dir_client + "/" + RF_DUMMY_20M_FILE  + ".clone";

    std::string dummy20MB_local      = work_dir_local  + "/" + RF_DUMMY_20M_FILE;

    const char* TEST_REMOTEFILE_STR;
    if (is_media) {
        TEST_REMOTEFILE_STR = "SdkRemoteFileRelease_MediaRF";
    } else {
        TEST_REMOTEFILE_STR = "SdkRemoteFileRelease_RF";
    }

    {
        VPLFS_stat_t stat;
        rv = VPLFS_Stat(test_clip_file_local.c_str(), &stat);
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "StatTestClipFile", rv);
        upload_size = stat.size;
    }

    rv = VPLMutex_Init(&thread_state_mutex);
    if (rv != VPL_OK) {
        LOG_ERROR("VPLMutex_Init() %d", rv);
        goto exit;
    }


    LOG_ALWAYS("\n\n==== Test RemoteFile ReadDir with pagination parameters - %s ====", TEST_REMOTEFILE_STR);

    // create the directory before start subsequent testing
    rv = remotefile_mkdir_recursive(upload_dir, datasetId_str, is_media);
    CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "PrepareDirectoryForReadDirPagination", rv);

    // === start testing read directory with pagination parameters ===
    // async upload file to the upload_dir
    for (unsigned int i = 0; i < temp_filelist.size(); i++) {
        std::string upload_file = temp_filelist[i].first;
        std::string file = upload_dir + "/" + getFilenameFromPath(upload_file);
        RF_XUPLOAD_SKIP(is_media, datasetId_str, upload_file.c_str(), file.c_str(), response, rv);
        if (rv != VPL_OK) {
            LOG_ERROR("Unable to submit xupload task: %s -> %s", upload_file.c_str(), file.c_str());
            CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "AsyncUploadFilesToUploadDir", rv);
            return VPL_ERR_FAIL;
        }
    }

    // wait for async upload complete
    rv = remotefile_wait_async_upload_done(is_media, false);
    if (rv != VPL_OK) {
        LOG_ERROR("Async upload failed");
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "WaitForAsyncUploadFilesComplete", rv);
        return VPL_ERR_FAIL;
    }

    // read dir with pagination parameters
    rv = remotefile_read_dir_pagination(is_media, datasetId_str, upload_dir, temp_filelist, "alpha");
    CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "ReadDirPaginationSortedByAlpha", rv);
    rv = remotefile_read_dir_pagination(is_media, datasetId_str, upload_dir, temp_filelist, "time");
    CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "ReadDirPaginationSortedByTime", rv);
    rv = remotefile_read_dir_pagination(is_media, datasetId_str, upload_dir, temp_filelist, "size");
    CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "ReadDirPaginationSortedBySize", rv);

    // clean-up files
    for (unsigned int i = 0; i < temp_filelist.size(); i++) {
        std::string upload_file = temp_filelist[i].first;
        std::string file = upload_dir + "/" + getFilenameFromPath(upload_file);
        RF_DELETE_FILE_SKIP(is_media, datasetId_str, file.c_str(), response, rv);
        if (rv != VPL_OK) {
            LOG_ALWAYS("Fail to clean-up file : %s", file.c_str());
            break;
        }
    }
    CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "CleanUpReadDirPaginationFiles", rv);
    // === done testing read directory with pagination parameters ===

    // testing feature flag enable/disable
#define TEST_FEATURE_ENABLE_DISABLE(op_name, op_name2) \
    BEGIN_MULTI_STATEMENT_MACRO \
    LOG_ALWAYS("\n\n==== Test "op_name" feautre enable/disable ===="); \
    LOG_ALWAYS("\n\n== Disable "op_name" feature on CloudPC/Client =="); \
    rv = remotefile_feature_enable(-1, -1, userId, deviceId, is_media, !is_media, alias_cloudpc, alias_client); \
    CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "CheckPsn"op_name"FeatureFlag", rv); \
    LOG_ALWAYS("\n\n==  Access "op_name" on CloudPC when "op_name" is disabled =="); \
    SET_TARGET_MACHINE(TEST_REMOTEFILE_STR, alias_client.c_str(), rv); \
    if (rv < 0) { \
        LOG_ERROR("SET_TARGET_MACHINE %s failed, %d", alias_client.c_str(), rv); \
    } \
    RF_READ_DIR_SKIP(is_media, datasetId_str, upload_dir.c_str(), response, rv); /*expect to fail*/ \
    rv = rv == 0? -1 : 0; \
    CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "AccessWhenFeature"op_name"Disabled", rv); \
    if (is_support_media) { \
        RF_READ_DIR_SKIP(!is_media, datasetId_str, upload_dir.c_str(), response, rv); /*expect to succeed*/ \
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "AccessWhenFeature"op_name2"Enabled", rv); \
    } \
    LOG_ALWAYS("\n\n== Enable "op_name" feature on CloudPC/Client =="); \
    rv = remotefile_feature_enable(-1, -1, userId, deviceId, !is_media, is_media, alias_cloudpc, alias_client); \
    CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "CheckPsn"op_name"FeatureFlag", rv); \
    LOG_ALWAYS("\n\n== Access "op_name" on CloudPC when "op_name" is enabled =="); \
    SET_TARGET_MACHINE(TEST_REMOTEFILE_STR, alias_client.c_str(), rv); \
    if (rv < 0) { \
        LOG_ERROR("SET_TARGET_MACHINE %s failed, %d", alias_client.c_str(), rv); \
    } \
    RF_READ_DIR_SKIP(is_media, datasetId_str, upload_dir.c_str(), response, rv); /*expect to succeed*/ \
    CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "AccessWhenFeature"op_name"Enabled", rv); \
    END_MULTI_STATEMENT_MACRO

    if (is_media) {
        TEST_FEATURE_ENABLE_DISABLE("MediaServer", "RemoteFile");
    } else {
        TEST_FEATURE_ENABLE_DISABLE("RemoteFile", "MediaServer");
    }

#undef TEST_FEATURE_ENABLE_DISABLE

    LOG_ALWAYS("\n\n==== Test RemoteFile Uplooad/Download/Mkdir/DeleteDir/ReadDir - %s ====", TEST_REMOTEFILE_STR);

    // create the directory recursively.
    rv = remotefile_mkdir_recursive(upload_dir, datasetId_str, is_media);
    CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "MakeDirRecursively", rv);

    RF_READ_DIR(is_media, datasetId_str, upload_dir.c_str(), response, rv);

    // Upload files to same path from 2 devices at the same time
    if (full) {
        SET_TARGET_MACHINE(TEST_REMOTEFILE_STR, alias_client2.c_str(), rv);
        if (rv < 0) {
            LOG_ERROR("SET_TARGET_MACHINE %s failed, %d", alias_client2.c_str(), rv); \
        }

        //This will force ccd to update dataset info
        rv = dumpDatasetList(userId);
        if (rv != 0) {
            LOG_ERROR("Fail to dump dataset list: %d", rv);
            goto exit;
        }
        RF_XUPLOAD(is_media, datasetId_str, test_clip_file2.c_str(), upload_file2.c_str(), response, rv);
    }

    SET_TARGET_MACHINE(TEST_REMOTEFILE_STR, alias_client.c_str(), rv);
    if (rv < 0) {
        LOG_ERROR("SET_TARGET_MACHINE %s failed, %d", alias_client.c_str(), rv); \
    }
    RF_XUPLOAD(is_media, datasetId_str, test_clip_file.c_str(), upload_file.c_str(), response, rv);

    rv = remotefile_wait_async_upload_done(is_media, false);
    CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "WaitForAsyncUploadDone_MD", rv);

    if (full) {
        SET_TARGET_MACHINE(TEST_REMOTEFILE_STR, alias_client2.c_str(), rv);
        if (rv < 0) {
            LOG_ERROR("SET_TARGET_MACHINE %s failed, %d", alias_client2.c_str(), rv); \
        }
        rv = remotefile_wait_async_upload_done(is_media, false);
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "WaitForAsyncUploadDone_Client", rv);
    }

    SET_TARGET_MACHINE(TEST_REMOTEFILE_STR, alias_client.c_str(), rv);
    if (rv < 0) {
        LOG_ERROR("SET_TARGET_MACHINE %s failed, %d", alias_client.c_str(), rv); \
    }
    {
        cJSON2 *metadata = NULL;
        cJSON2 *jsonResponse = NULL;

        RF_READ_METADATA(is_media, datasetId_str, upload_file.c_str(), response, rv);

        jsonResponse = cJSON2_Parse(response.c_str());
        if (jsonResponse == NULL) {
            rv = -1;
        }
        metadata = cJSON2_GetObjectItem(jsonResponse, "size");
        if (metadata == NULL || metadata->type != cJSON2_Number) {
            LOG_ERROR("error while parsing metadata");
            rv = -1;
        } else if (upload_size != metadata->valueint) {
            LOG_ERROR("size doesn't match: upload_size("FMTu64"), file metadata size("FMTu64")", (u64)upload_size, (u64)metadata->valueint);
            rv = -1;
        }
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "VerifyUploadFileSize", rv);

        if (jsonResponse) {
            cJSON2_Delete(jsonResponse);
            jsonResponse = NULL;
        }
    }

    if (full) {
        LOG_ALWAYS("upload_file: %s, upload_file_copy: %s", upload_file.c_str(), upload_file_copy.c_str());
        RF_COPY_FILE_SKIP(!is_media, datasetId_str, upload_file.c_str(), upload_file_copy.c_str(), response, rv);
        rv = rv == 0? -1 : 0;;
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "CopyFileWhenAccessDisabled", rv);
        RF_COPY_FILE_SKIP(is_media, datasetId_str, upload_file.c_str(), upload_file_copy.c_str(), response, rv);
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "CopyFileWhenAccessEnabled", rv);

        RF_READ_DIR(is_media, datasetId_str, upload_dir.c_str(), response, rv);
    }

    // clean-up download file from host and client side before starting download from cloudpc
    Util_rm_dash_rf(download_file_local);
    {
        TargetDevice *target = getTargetDevice();
        rv = target->deleteFile(convert_path_convention(clientPCSeparator, download_file));
        delete target;
        if (rv != 0 && rv != VPL_ERR_NOENT) {
            LOG_ERROR("Unable to clean-up file: %s, rv = %d", download_file.c_str(), rv);
            goto exit;
        }
    }

    if (full) {
        download_file_local.assign(work_dir_local.c_str());
        download_file_local.append("/");
        download_file_local.append("dummy20.raw");
        download_file_local.append(".clone");
        download_file_local2.assign(work_dir_client.c_str());
        download_file_local2.append("/");
        download_file_local2.append("dummy20.raw");
        download_file_local2.append(".clone");
        download_file.assign(work_dir.c_str());
        download_file.append("/");
        download_file.append("dummy20.raw");
        download_file.append(".clone");

        //// clean-up download_file2
        SET_TARGET_MACHINE(TEST_REMOTEFILE_STR, alias_client2.c_str(), rv);
        if (rv < 0) {
            LOG_ERROR("SET_TARGET_MACHINE %s failed, %d", alias_client2.c_str(), rv); \
        }
        {
            TargetDevice *target = getTargetDevice();
            rv = target->deleteFile(convert_path_convention(clientPCSeparator2, download_file2));
            delete target;
            if (rv != 0 && rv != VPL_ERR_NOENT) {
                LOG_ERROR("Unable to clean-up file: %s, rv = %d", download_file2.c_str(), rv);
                goto exit;
            }
        }

        params.mutex = &thread_state_mutex;
        params.alias = alias_client2;
        params.is_media = is_media;
        params.datasetId_str = datasetId_str;
        params.download_from = upload_file2;
        params.download_to = download_file2;
        params.tc_name = TEST_REMOTEFILE_STR;
        CREATE_REMOTEFILE_DOWNLOAD_THREAD("CreateThreadToDownloadFile", params);

        rv = set_target_machine(alias_client.c_str(), ip_addr, port);
        if (rv == VPL_ERR_FAIL) {
            LOG_ERROR("set_target_machine %s failed!", alias_client.c_str());
        }
        ss_ip.clear();
        ss_ip << ip_addr << std::endl;
        ss_port.clear();
        ss_port << port << std::endl;
        ip_addr_str = ss_ip.str();
        port_str = ss_port.str();
        rv = fs_test_download_by_ip_port(userId, datasetId_str, upload_file2, download_file,
                "", ip_addr_str, port_str, response, is_media);
        if (rv == 0) {
            LOG_ALWAYS("Download %s to %s PASS!!!", upload_file2.c_str(), download_file.c_str());
        }
        else {
            LOG_ERROR("Download %s to %s FAIL!!!", upload_file2.c_str(), download_file.c_str());
        }
    }
    else {
        RF_DOWNLOAD(is_media, datasetId_str, upload_file.c_str(), download_file.c_str(), response, rv);
    }

    // compare the download file w/ the test_clip @ control-side
    SET_TARGET_MACHINE(TEST_REMOTEFILE_STR, alias_client.c_str(), rv);
    {
        TargetDevice *target = getTargetDevice();
        rv = target->pullFile(convert_path_convention(clientPCSeparator, download_file),
                download_file_local);
        delete target;
        if (rv != 0) {
            LOG_ERROR("Unable to clean-up file: %s, rv = %d", download_file.c_str(), rv);
            goto exit;
        }
        if (full) {
            rv = file_compare(dummy20MB_local.c_str(), download_file_local.c_str());
        } else {
            rv = file_compare(test_clip_file_local.c_str(), download_file_local.c_str());
        }
    }
    CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_REMOTEFILE_STR, "VerifyFileDownloaded", rv, "9710");

    // clean-up download file from host and client side before starting download from cloudpc
    Util_rm_dash_rf(download_file_local);
    {
        TargetDevice *target = getTargetDevice();
        rv = target->deleteFile(convert_path_convention(clientPCSeparator, download_file));
        delete target;
        if (rv != 0 && rv != VPL_ERR_NOENT) {
            LOG_ERROR("Unable to clean-up file: %s, rv = %d", download_file.c_str(), rv);
            goto exit;
        }
    }
    RF_DOWNLOAD_RANGE(is_media, datasetId_str, upload_file.c_str(), download_file.c_str(), "1000-2000", response, rv);
    // compare the download file w/ the test_clip @ control-side
    if (full) {
        download_file_local.assign(work_dir_local.c_str());
        download_file_local.append("/");
        download_file_local.append(RF_TEST_LARGE_FILE);
        download_file_local.append(".clone");
    }
    {
        TargetDevice *target = getTargetDevice();
        rv = target->pullFile(convert_path_convention(clientPCSeparator, download_file),
                download_file_local);
        delete target;
        if (rv != 0) {
            LOG_ERROR("Unable to clean-up file: %s, rv = %d", download_file.c_str(), rv);
            goto exit;
        }
        rv = file_compare_range(test_clip_file_local.c_str(), download_file_local.c_str(), 1000, 2000-1000+1);
    }
    CHECK_AND_PRINT_EXPECTED_TO_FAIL(TEST_REMOTEFILE_STR, "VerifyFileDownloadedRange", rv, "9710");

    RF_DELETE_FILE(is_media, datasetId_str, upload_file.c_str(), response, rv);
    RF_READ_METADATA_SKIP(is_media, datasetId_str, upload_file.c_str(), response, rv); // expected failed
    rv = rv == 0? -1 : 0;;
    CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "VerifyFileDeleted", rv);

    if (full) {
        RF_DELETE_FILE(is_media, datasetId_str, upload_file_copy.c_str(), response, rv);
        RF_READ_METADATA_SKIP(is_media, datasetId_str, upload_file_copy.c_str(), response, rv); // expected failed
        rv = rv == 0? -1 : 0;;
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "VerifyFileDeleted", rv);

        SET_TARGET_MACHINE(TEST_REMOTEFILE_STR, alias_client2.c_str(), rv);
        if (rv < 0) {
            LOG_ERROR("SET_TARGET_MACHINE %s failed, %d", alias_client2.c_str(), rv);
        }
        // should wait for client finishes downloading before deleting file upload_file2
        {
            int retry = 0;
            VPLFS_stat_t stat_src, stat_dst;
            if (VPLFS_Stat(dummy20MB_local.c_str(), &stat_src)) {
                LOG_ERROR("fail to stat src file: %s", dummy20MB_local.c_str());
            }
            TargetDevice *target = getTargetDevice();
            while (retry < 10) {
                rv = target->statFile(download_file2, stat_dst);
                if (rv != 0) {
                    LOG_ERROR("fail to stat download file: %s", download_file2.c_str());
                }
                else if (stat_src.size == stat_dst.size){
                    LOG_ALWAYS("Client download %s finish!!!", upload_file2.c_str());
                    break;
                }
                else {
                    VPLThread_Sleep(VPLTIME_FROM_SEC(1));
                }
            }
            rv = target->pullFile(convert_path_convention(clientPCSeparator2, download_file2),
                    download_file_local2);
            delete target;
            if (rv != 0) {
                LOG_ERROR("Unable to clean-up file: %s, rv = %d", download_file2.c_str(), rv);
                goto exit;
            }
            rv = file_compare(dummy20MB_local.c_str(), download_file_local2.c_str());
            CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "ClientVerifyFileDownloaded", rv);
        }

        RF_DELETE_FILE(is_media, datasetId_str, upload_file2.c_str(), response, rv);
        RF_READ_METADATA_SKIP(is_media, datasetId_str, upload_file2.c_str(), response, rv); // expected failed
        rv = rv == 0? -1 : 0;;
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "VerifyFileDeleted", rv);
    }

    SET_TARGET_MACHINE(TEST_REMOTEFILE_STR, alias_client.c_str(), rv);
    if (rv < 0) {
        LOG_ERROR("SET_TARGET_MACHINE %s failed, %d", alias_client.c_str(), rv);
    }


    //delete sub-folder when it's not empty
    {
        std::string upload_dir_foo    = upload_dir + "/foo";
        std::string upload_dir_foobar = upload_dir + "/foo/bar";

        rv = remotefile_mkdir_recursive(upload_dir_foobar, datasetId_str, is_media);
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "MakeDirRecursively", rv);

        RF_DELETE_DIR_SKIP(is_media, datasetId_str, upload_dir_foo.c_str(), response, rv);  //expected failed
#ifndef WIN32
        //win32 can still delete sub-dir, others will fail
        rv = rv == 0? -1 : 0;
#endif
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "VerifyDirDeleted_NonEmpty", rv);
        //cleanup
        RF_DELETE_DIR_SKIP(is_media, datasetId_str, upload_dir_foobar.c_str(), response, rv);
        RF_DELETE_DIR_SKIP(is_media, datasetId_str, upload_dir_foo.c_str(), response, rv);
    }

    RF_DELETE_DIR(is_media, datasetId_str, upload_dir.c_str(), response, rv);
    RF_READ_METADATA_SKIP(is_media, datasetId_str, upload_dir.c_str(), response, rv); // expected failed
    rv = rv == 0? -1 : 0;;
    CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "VerifyDirectoryDeleted", rv);

exit:
    VPLMutex_Destroy(&thread_state_mutex);
    return rv;
}

int do_autotest_sdk_release_remotefile(int argc, const char* argv[]) {

    const char *TEST_REMOTEFILE_STR = "SdkRemoteFileRelease";
    u64 userId = 0;
    u64 deviceId = 0;
    u64 datasetId = 0;
    std::stringstream ss;
    std::string deviceId_str;
    std::string datasetId_str;
    std::string response;
    std::string test_clip_file;
    std::string test_clip_file2;
    std::string test_clip_file_local;
    std::string upload_dir = RF_UPLOAD_DIR;
    std::string upload_file = upload_dir+"/"+RF_TEST_LARGE_FILE;
    std::string upload_file_copy = upload_dir+"/"+RF_TEST_LARGE_FILE+".copy";
    std::string upload_file2 = upload_dir+"/"+RF_DUMMY_20M_FILE;

    std::string dummy1KB_local = "/";
    dummy1KB_local += RF_DUMMY_1K_FILE;
    std::string dummy1MB_local = "/";
    dummy1MB_local += RF_DUMMY_1M_FILE;
    std::string dummy20MB_local = "/";
    dummy20MB_local += RF_DUMMY_20M_FILE;

    std::string dummy1KB = "/";
    dummy1KB += RF_DUMMY_1K_FILE;
    std::string dummy1MB = "/";
    dummy1MB += RF_DUMMY_1M_FILE;

    int cloudPCId = 1;
    int clientPCId = 2;
    int clientPCId2 = 3;
    std::string alias_cloudpc = "CloudPC";
    std::string alias_client = "MD";
    std::string alias_client2 = "Client";
    std::string cloudPCOSVersion;
    std::string clientPCOSVersion;
    std::string clientPCOSVersion2;

    std::string cloudPCSeparator;
    std::string clientPCSeparator;
    std::string clientPCSeparator2;

    std::string localapp_folder;
    std::string userprofile_folder;
    // bypass [LOCALAPPDATA], concate later for clean-up purpose
    std::string upload_dir_win32 = upload_dir.substr(upload_dir.find_first_of("/"));

    // file path (str), file size (u32)
    static const u32 MAX_SIZE = 1*1024*1024;
    static const u32 NR_FILES = 10;
    std::vector< std::pair<std::string, VPLFS_file_size_t> > temp_filelist;
    std::vector< std::pair<std::string, VPLFS_file_size_t> > temp_filelist_local;

    // XXX need to figure out how to alter to the new way
    std::string work_dir;
    std::string work_dir_client;
    std::string work_dir_local;
    std::string download_file;
    std::string download_file2;
    std::string download_file_local;
    std::string download_file_local2;
    VPLFS_file_size_t upload_size;
    cJSON2 *jsonResponse = NULL;
    RFDownloadParams params;
    std::stringstream ss_ip;
    std::stringstream ss_port;
    std::string ip_addr_str;
    std::string port_str;
    //VPLNet_addr_t ip_addr;
    //VPLNet_port_t port;
    VPLMutex_t thread_state_mutex;

    int rv = 0;
    bool is_media = false;
    bool full = false;
    bool is_support_media = true;

    const char *domain = NULL;
    const char *username = NULL;
    const char *password = NULL;
    int pause_delay = 0;

    if (checkHelp(argc, argv) || (argc < 4)) {
        printf("AutoTest %s [-f|--fulltest] [-p nsec] <domain> <username> <password>\n", argv[0]);
        return 0;   // No arguments needed 
    }

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--fulltest") == 0) {
                full = true;
            }
            else if (strcmp(argv[i], "-p") == 0) {
                if (i+i < argc) {
                    pause_delay = atoi(argv[++i]);
                }
                else {
                    LOG_ERROR("-p with no arg");
                    return 0;
                }
            }
            else {
                LOG_ERROR("Unknown option %s", argv[i]);
                return 0;
            }
        }
        else if (!domain) {
            domain = argv[i];
        }
        else if (!username) {
            username = argv[i];
        }
        else if (!password) {
            password = argv[i];
        }
        else {
            LOG_ERROR("Unexpected word %s", argv[i]);
            return 0;
        }
    }

    LOG_ALWAYS("Testing AutoTest SDK Release RemoteFile: Domain(%s) User(%s) Password(%s)", domain, username, password);

    rv = VPLMutex_Init(&thread_state_mutex);
    if (rv != VPL_OK) {
        LOG_ERROR("VPLMutex_Init() %d", rv);
        goto exit;
    }
    // Does a hard stop for all ccds
    {
        const char *testArg[] = { "StopCCD" };
        stop_ccd_hard(1, testArg);
    }

    LOG_ALWAYS("\n\n==== Launching Cloud PC CCD ====");
    SET_TARGET_MACHINE(TEST_REMOTEFILE_STR, alias_cloudpc.c_str(), rv);
    if (rv < 0) {
        setCcdTestInstanceNum(cloudPCId);
    }

    CHECK_LINK_REMOTE_AGENT(alias_cloudpc, TEST_REMOTEFILE_STR, rv);

    {
        QUERY_TARGET_OSVERSION(cloudPCOSVersion, TEST_REMOTEFILE_STR, rv);
    }

    {
        TargetDevice *target = getTargetDevice();
 
        rv = target->getDirectorySeparator(cloudPCSeparator);
        if (rv != 0) {
            LOG_ERROR("Unable to get the client pc path separator, rv = %d", rv);
            goto exit;
        }

        if (target != NULL) {
            delete target;
            target = NULL;
        }
    }

    START_CCD(TEST_REMOTEFILE_STR, rv);
    START_CLOUDPC(username, password, TEST_REMOTEFILE_STR, true, rv);

    LOG_ALWAYS("\n\n==== Launching MD CCD ====");
    SET_TARGET_MACHINE(TEST_REMOTEFILE_STR, alias_client.c_str(), rv);
    if (rv < 0) {
        setCcdTestInstanceNum(clientPCId);
    }

    CHECK_LINK_REMOTE_AGENT(alias_client, TEST_REMOTEFILE_STR, rv);

    {
        QUERY_TARGET_OSVERSION(clientPCOSVersion, TEST_REMOTEFILE_STR, rv);
    }

    {
        TargetDevice *target = getTargetDevice();

        rv = target->getDxRemoteRoot(work_dir);
        if (rv != 0) {
            LOG_ERROR("Unable to get the work directory from target device, rv = %d", rv);
            delete target;
            goto exit;
        }
        rv = target->getDirectorySeparator(clientPCSeparator);
        if (rv != 0) {
            LOG_ERROR("Unable to get the client pc path separator, rv = %d", rv);
            goto exit;
        }

        if( target != NULL) {
            delete target;
            target =NULL;
        }
    }

    if (isWindows(clientPCOSVersion) || clientPCOSVersion.compare(OS_LINUX) == 0) {
        START_CCD(TEST_REMOTEFILE_STR, rv);
    }

    UPDATE_APP_STATE(TEST_REMOTEFILE_STR, rv);

    START_CLIENT(username, password, TEST_REMOTEFILE_STR, true, rv);

    LOG_ALWAYS("Cloud  PC OS Version: %s", cloudPCOSVersion.c_str());
    LOG_ALWAYS("Client PC OS Version: %s", clientPCOSVersion.c_str());

    LOG_ALWAYS("\n\n==== RemoteFile ====");

    if (pause_delay > 0) {
        LOG_ALWAYS("\n\n== Pausing for %d secs ==", pause_delay);
        VPLThread_Sleep(VPLTime_FromSec(pause_delay));
    }

    // convert to forward slashes no matter how
    std::replace(work_dir.begin(), work_dir.end(), '\\', '/');

    // get test clip file path
    rv = getCurDir(work_dir_local);
    if (rv < 0) {
        LOG_ERROR("failed to get current dir. error = %d", rv);
        goto exit;
    }
#ifdef WIN32
    std::replace(work_dir_local.begin(), work_dir_local.end(), '\\', '/');
#endif

    test_clip_file.assign(work_dir.c_str());
    test_clip_file.append("/");
    test_clip_file.append(RF_TEST_LARGE_FILE);

    test_clip_file_local.assign(work_dir_local.c_str());
    test_clip_file_local.append("/");
    test_clip_file_local.append(RF_TEST_LARGE_FILE);

    download_file.assign(work_dir.c_str());
    download_file.append("/");
    download_file.append(RF_TEST_LARGE_FILE);
    download_file.append(".clone");

    download_file_local.assign(work_dir_local.c_str());
    download_file_local.append("/");
    download_file_local.append(RF_TEST_LARGE_FILE);
    download_file_local.append(".clone");

    dummy1KB.insert(0, work_dir);
    dummy1MB.insert(0, work_dir);

    dummy1KB_local.insert(0, work_dir_local);
    dummy1MB_local.insert(0, work_dir_local);

    LOG_ALWAYS("Work directory = %s", work_dir.c_str());
    LOG_ALWAYS("Work directory (local) = %s", work_dir_local.c_str());
    LOG_ALWAYS("Test upload file = %s", test_clip_file.c_str());
    LOG_ALWAYS("Test upload file (local) = %s", test_clip_file_local.c_str());
    LOG_ALWAYS("CloudPC upload dir = %s, file = %s", upload_dir.c_str(), upload_file.c_str());
    LOG_ALWAYS("CloudPC upload dir = %s, file2 = %s", upload_dir.c_str(), upload_file2.c_str());
    LOG_ALWAYS("Dummy 1KB = %s, 1MB = %s", dummy1KB.c_str(), dummy1MB.c_str());
    LOG_ALWAYS("Dummy (local) 1KB = %s, 1MB = %s", dummy1KB_local.c_str(), dummy1MB_local.c_str());
    LOG_ALWAYS("Download file = %s, Download file2 = %s", download_file.c_str(), download_file2.c_str());
    LOG_ALWAYS("Download file (local) = %s", download_file_local.c_str());

    LOG_ALWAYS("\n\n==== Launching Client CCD ====");
    SET_TARGET_MACHINE(TEST_REMOTEFILE_STR, alias_client2.c_str(), rv);
    if (rv < 0) {
        setCcdTestInstanceNum(clientPCId2);
    }

    CHECK_LINK_REMOTE_AGENT(alias_client2, TEST_REMOTEFILE_STR, rv);

    {
        QUERY_TARGET_OSVERSION(clientPCOSVersion2, TEST_REMOTEFILE_STR, rv);
    }

    {
        TargetDevice *target = getTargetDevice();

        rv = target->getDxRemoteRoot(work_dir_client);
        if (rv != 0) {
            LOG_ERROR("Unable to get the work directory from target device, rv = %d", rv);
            delete target;
            goto exit;
        }
        rv = target->getDirectorySeparator(clientPCSeparator2);
        if (rv != 0) {
            LOG_ERROR("Unable to get the client pc path separator, rv = %d", rv);
            goto exit;
        }

        if( target != NULL) {
            delete target;
            target =NULL;
        }
    }

    if (isWindows(clientPCOSVersion2) || clientPCOSVersion2.compare(OS_LINUX) == 0) {
        START_CCD(TEST_REMOTEFILE_STR, rv);
    }

    START_CLIENT(username, password, TEST_REMOTEFILE_STR, true, rv);

    // convert to forward slashes no matter how
    std::replace(work_dir_client.begin(), work_dir_client.end(), '\\', '/');

    if (full) {
        dummy20MB_local.insert(0, work_dir_local);
        LOG_ALWAYS("dummy20MB_local: %s", dummy20MB_local.c_str());

        download_file2.assign(work_dir_client.c_str());
        download_file2.append("/");
        download_file2.append(RF_DUMMY_20M_FILE);
        download_file2.append(".clone2");

        Util_rm_dash_rf(dummy20MB_local);
        rv = create_dummy_file(dummy20MB_local.c_str(), 20*1024*1024);
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "CreateDummy20MBFileForXUPLOAD", rv);

        test_clip_file2.assign(work_dir_client.c_str());
        test_clip_file2.append("/");
        test_clip_file2.append(RF_DUMMY_20M_FILE);
        LOG_ALWAYS("test_clip_file2: %s", test_clip_file2.c_str());

        // push dummy20MB to the target machine for XUPLOAD test
        {
            TargetDevice *target = getTargetDevice();
            rv = target->pushFile(dummy20MB_local,
                                  convert_path_convention(clientPCSeparator, test_clip_file2));
            if (rv != 0) {
                // clean-up remote upload directory which is used in the subsequent testing for Win32
                // for cloudnode, since the user is delete and create everytime. the directories are always clean
                LOG_ERROR("Fail to push file from local to remote: %s --> %s, rv = %d",
                          dummy20MB_local.c_str(), test_clip_file2.c_str(), rv);
                goto exit;
            }
            delete target;
        }
        // delete file which will be downloaded to the target machine
        {
            TargetDevice *target = getTargetDevice();
            rv = target->deleteFile(convert_path_convention(clientPCSeparator2, download_file2));
            if (rv != 0 && rv != VPL_ERR_NOENT) {
                LOG_ERROR("Fail to clean-up file: %s, rv = %d", download_file2.c_str(), rv);
                goto exit;
            }
            delete target;
        }
    }

    // clean-up before testing
    LOG_ALWAYS("\n\n== CleanUp download/upload directory ==");

    // download file is located at client pc
    SET_TARGET_MACHINE(TEST_REMOTEFILE_STR, alias_client.c_str(), rv);
    if (rv < 0) {
        setCcdTestInstanceNum(clientPCId);
    }

    {
        TargetDevice *target = getTargetDevice();
        rv = target->deleteFile(convert_path_convention(clientPCSeparator, download_file));
        if (rv != 0 && rv != VPL_ERR_NOENT) {
            LOG_ERROR("Fail to clean-up file: %s, rv = %d", download_file.c_str(), rv);
            goto exit;
        }
        delete target;
    }

    Util_rm_dash_rf(download_file_local);

    // upload folder is located at cloud pc
    SET_TARGET_MACHINE(TEST_REMOTEFILE_STR, alias_cloudpc.c_str(), rv);
    if (rv < 0) {
        setCcdTestInstanceNum(cloudPCId);
    }

    // get local app / user profile folder path
    {
        TargetDevice *target = getTargetDevice();
        std::string path;
        if (isWindows(cloudPCOSVersion)) {
            rv = target->getAliasPath("LOCALAPPDATA", path);
            if (rv != VPL_OK) {
                LOG_ERROR("Unable to get local app folder path, rv = %d", rv);
                rv = VPL_ERR_FAIL;
            } else {
                localapp_folder = convert_to_win32_virt_folder(path.c_str());
            }
            if (rv == VPL_OK) {
                rv = target->getAliasPath("USERPROFILE", path);
                if (rv != VPL_OK) {
                    LOG_ERROR("Unable to get user profile folder path, rv = %d", rv);
                    rv = VPL_ERR_FAIL;
                } else {
                    userprofile_folder = convert_to_win32_virt_folder(path.c_str());
                }
            }
            CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "GetKnownFolderPaths", rv);
        }
        delete target;
    }
    LOG_ALWAYS("LOCALAPP = %s, USERPROFILE = %s", localapp_folder.c_str(), userprofile_folder.c_str());

    {
        TargetDevice *target = getTargetDevice();
        if (isWindows(cloudPCOSVersion)) {
            std::string target_dir = convert_to_win32_real_folder((localapp_folder+upload_dir_win32).c_str());
            rv = target->removeDirRecursive(target_dir);
            if (rv != 0) {
                // clean-up remote upload directory which is used in the subsequent testing for Win32
                // for cloudnode, since the user is delete and create everytime. the directories are always clean
                LOG_ERROR("Fail to clean-up directory: %s, rv = %d", target_dir.c_str(), rv);
                goto exit;
            }
        }
        delete target;
    }

    // make sure the test clip exist
    // push the test clip from local to remote. therefore
    // 1. stat locally
    // 2. push to remote

    SET_TARGET_MACHINE(TEST_REMOTEFILE_STR, alias_client.c_str(), rv);
    if (rv < 0) {
        setCcdTestInstanceNum(clientPCId);
    }

    {
        VPLFS_stat_t stat;

        rv = VPLFS_Stat(test_clip_file_local.c_str(), &stat);
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "StatTestClipFile", rv);
        upload_size = stat.size;
    }

    {
        TargetDevice *target = getTargetDevice();
        rv = target->pushFile(test_clip_file_local,
                              convert_path_convention(clientPCSeparator, test_clip_file));
        if (rv != 0) {
            // clean-up remote upload directory which is used in the subsequent testing for Win32
            // for cloudnode, since the user is delete and create everytime. the directories are always clean
            LOG_ERROR("Fail to push file from local to remote: %s --> %s, rv = %d",
                      test_clip_file_local.c_str(), test_clip_file.c_str(), rv);
            goto exit;
        }
        delete target;
    }

    // make sure both cloudpc/client has the device linked info updated
    LOG_ALWAYS("\n\n== Checking cloudpc and Client device link status ==");
    {
        std::vector<u64> deviceIds;
        u64 cloudPCDeviceId = 0;
        u64 MDDeviceId = 0;
        u64 clientPCDeviceId = 0;
        const char *testCloudStr = "CheckCloudPCDeviceLinkStatus";
        const char *testMDStr = "CheckMDDeviceLinkStatus";
        const char *testClientStr = "CheckClientPCDeviceLinkStatus";

        SET_TARGET_MACHINE(TEST_REMOTEFILE_STR, alias_cloudpc.c_str(), rv);
        if (rv < 0) {
            setCcdTestInstanceNum(cloudPCId);
        }

        rv = getUserIdBasic(&userId);
        if (rv != 0) {
            LOG_ERROR("Fail to get user id:%d", rv);
            CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, testCloudStr, rv);
        }

        rv = getDeviceId(&cloudPCDeviceId);
        if (rv != 0) {
            LOG_ERROR("Fail to get CloudPC device id:%d", rv);
            CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, testCloudStr, rv);
        }

        SET_TARGET_MACHINE(TEST_REMOTEFILE_STR, alias_client.c_str(), rv);
        if (rv < 0) {
            setCcdTestInstanceNum(clientPCId);
        }

        rv = getDeviceId(&MDDeviceId);
        if (rv != 0) {
            LOG_ERROR("Fail to get MD device id:%d", rv);
            CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, testCloudStr, rv);
        }

        SET_TARGET_MACHINE(TEST_REMOTEFILE_STR, alias_client2.c_str(), rv);
        if (rv < 0) {
            setCcdTestInstanceNum(clientPCId2);
        }

        rv = getDeviceId(&clientPCDeviceId);
        if (rv != 0) {
            LOG_ERROR("Fail to get Client device id:%d", rv);
            CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, testCloudStr, rv);
        }

        deviceIds.push_back(cloudPCDeviceId);
        LOG_ALWAYS("Add Device Id "FMTu64, cloudPCDeviceId);
        deviceIds.push_back(MDDeviceId);
        LOG_ALWAYS("Add Device Id "FMTu64, MDDeviceId);
        deviceIds.push_back(clientPCDeviceId);
        LOG_ALWAYS("Add Device Id "FMTu64, clientPCDeviceId);

        rv = wait_for_devices_to_be_online_by_alias(TEST_REMOTEFILE_STR, alias_cloudpc, cloudPCId, userId, deviceIds, 20);
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, testCloudStr, rv);
        rv = wait_for_devices_to_be_online_by_alias(TEST_REMOTEFILE_STR, alias_client, clientPCId, userId, deviceIds, 20);
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, testMDStr, rv);
        rv = wait_for_devices_to_be_online_by_alias(TEST_REMOTEFILE_STR, alias_client2, clientPCId2, userId, deviceIds, 20);
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, testClientStr, rv);
    }

    SET_TARGET_MACHINE(TEST_REMOTEFILE_STR, alias_client.c_str(), rv);
    if (rv < 0) {
        setCcdTestInstanceNum(clientPCId);
    }

    LOG_ALWAYS("\n\n== ListDevices ==");
    {
        cJSON2 *devicelist = NULL;

        RF_LISTDEVICE_SKIP(is_media, response, rv);
        if (rv == 0) {
            jsonResponse = cJSON2_Parse(response.c_str());
            if (jsonResponse == NULL) {
                rv = -1;
            }
        }
        if (rv == 0) {
            // get the deviceList array object
            devicelist = cJSON2_GetObjectItem(jsonResponse, "deviceList");
            if (devicelist == NULL) {
                rv = -1;
            }
        }
        if (rv == 0) {
            rv = -1;
            cJSON2 *node = NULL;
            cJSON2 *subNode = NULL;
            bool found = false;
            for (int i = 0; i < cJSON2_GetArraySize(devicelist); i++) {
                node = cJSON2_GetArrayItem(devicelist, i);
                if (node == NULL) {
                    continue;
                }
                subNode = cJSON2_GetObjectItem(node, "isPsn");
                if (subNode == NULL || subNode->type != cJSON2_True) {
                    continue;
                }
                subNode = cJSON2_GetObjectItem(node, "deviceId");
                if (subNode == NULL || subNode->type != cJSON2_Number) {
                    continue;
                }
                if (!found) {
                    deviceId = (u64) subNode->valueint;
                    ss.str("");
                    ss << deviceId;
                    deviceId_str = ss.str();
                    rv = 0;
                    found = true;
                } else {
                    // more than one PSN. report error
                    LOG_ERROR("More than isPsn device found!");
                    rv = -1;
                    break;
                }
            }
        }
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "ListDevices", rv);

        if (jsonResponse) {
            cJSON2_Delete(jsonResponse);
            jsonResponse = NULL;
        }
    }

    LOG_ALWAYS("\n\n== ListDatasets ==");
    {
        cJSON2 *datasetlist  = NULL;
        RF_LISTDATASET_SKIP(is_media, response, rv);
        if (rv == 0) {
            jsonResponse = cJSON2_Parse(response.c_str());
            if (jsonResponse == NULL) {
                rv = -1;
            }
        }
        if (rv == 0) {
            // get the deviceList array object
            datasetlist = cJSON2_GetObjectItem(jsonResponse, "datasetList");
            if (datasetlist == NULL) {
                rv = -1;
            }
        }
        if (rv == 0) {
            rv = -1;
            cJSON2 *node = NULL;
            cJSON2 *subNode = NULL;
            bool found = false;
            for (int i = 0; i < cJSON2_GetArraySize(datasetlist); i++) {
                node = cJSON2_GetArrayItem(datasetlist, i);
                if (node == NULL) {
                    continue;
                }
                subNode = cJSON2_GetObjectItem(node, "clusterId");
                if (subNode == NULL || subNode->type != cJSON2_Number || subNode->valueint != deviceId) {
                    continue;
                }
                subNode = cJSON2_GetObjectItem(node, "datasetType");
                if (subNode == NULL || subNode->type != cJSON2_String ||
#if !defined(CLOUDNODE)
                    strcmp(subNode->valuestring, "FS") != 0
#else
                    strcmp(subNode->valuestring, "VIRT_DRIVE") != 0
#endif //!defined(CLOUDNODE)
                ) {
                    continue;
                }
                subNode = cJSON2_GetObjectItem(node, "datasetId");
                if (subNode == NULL || subNode->type != cJSON2_Number) {
                    continue;
                }
                if (!found) {
                    datasetId = (u64) subNode->valueint;
                    ss.str("");
                    ss << datasetId;
                    datasetId_str = ss.str();
                    rv = 0;
                    found = true;
                    rv = 0;
                } else {
                    // more than one PSN. report error
                    LOG_ERROR("More than FS dataset found!");
                    rv = -1;
                    break;
                }
            }
        }
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "ListDatasets", rv);

        if (jsonResponse) {
            cJSON2_Delete(jsonResponse);
            jsonResponse = NULL;
        }
    }

    rv = remotefile_feature_enable(cloudPCId, clientPCId, userId, deviceId, true, true, alias_cloudpc, alias_client);
    CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "EnableRFnMediaRFFeatureFlags", rv);

    // perform /rf, /media_rf access rule checking
    {
        std::map <std::string, folder_access_rules> test_cases;
        std::vector <std::string> filelists;
        std::vector <std::string> filelists_local;

        folder_access_rules media_rf_rules;
        folder_access_rules rf_rules;

#if defined(CLOUDNODE)
        // XXX alter to use the cloudPCOSVersion
        {
            // /media_rf allowed
            media_rf_rules.push_back(folder_item("/clear.fi", true, false));
            media_rf_rules.push_back(folder_item("[LOCALAPPDATA]/clear.fi", true, true));
            // - it happens that the [USERPROFILE] and [LOCALAPPDATA] are both mappd to "/"
            // - Therefore, media_rf is allowed to access [USERPROFILE]/clear.fi
            media_rf_rules.push_back(folder_item("[USERPROFILE]/clear.fi", true, true));
            // /media_rf denied
            media_rf_rules.push_back(folder_item("/", false, false));
            media_rf_rules.push_back(folder_item("/remote_file", false, false));
            media_rf_rules.push_back(folder_item("[LOCALAPPDATA]", false, true));
            media_rf_rules.push_back(folder_item("[USERPROFILE]", false, true));
            media_rf_rules.push_back(folder_item("[LOCALAPPDATA]/CLEAR.FI", false, true)); // case sensitive
            media_rf_rules.push_back(folder_item("[LOCALAPPDATA]/clear.fiXXX", false, true)); // match whole string

            // rf allowed
            rf_rules.push_back(folder_item("/clear.fi", true, false));
            rf_rules.push_back(folder_item("/remote_file", true, false));
            rf_rules.push_back(folder_item("[LOCALAPPDATA]/clear.fi", true, true));
            rf_rules.push_back(folder_item("[USERPROFILE]/clear.fi", true, true));
            rf_rules.push_back(folder_item("[LOCALAPPDATA]", true, true));
            rf_rules.push_back(folder_item("[USERPROFILE]", true, true));
            rf_rules.push_back(folder_item("[LOCALAPPDATA]/CLEAR.FI", true, true)); // case sensitive, but allowed

            test_cases["RF"] = rf_rules;
            test_cases["MediaRF"] = media_rf_rules;
        }
#endif
        if (isWindows(cloudPCOSVersion)) {
            // XXX put the real folder path before the virtual folder to make sure the folder is clean-up before testing

            // /media_rf allowed
            media_rf_rules.push_back(folder_item(localapp_folder + "/clear.fi", true, false));
            media_rf_rules.push_back(folder_item("[LOCALAPPDATA]/clear.fi", true, true));
            media_rf_rules.push_back(folder_item(userprofile_folder + "/Picstream", true, false));
            media_rf_rules.push_back(folder_item("[USERPROFILE]/Picstream", true, true));
            // /media_rf denied
            media_rf_rules.push_back(folder_item("Computer/C", false, false));
            media_rf_rules.push_back(folder_item("[LOCALAPPDATA]", false, true));
            media_rf_rules.push_back(folder_item("[USERPROFILE]", false, true));
            media_rf_rules.push_back(folder_item(localapp_folder+"/CLEAR.FI", false, false)); // case sensitive
            media_rf_rules.push_back(folder_item("[LOCALAPPDATA]/CLEAR.FI", false, true)); // case sensitive
            media_rf_rules.push_back(folder_item(localapp_folder+"/clear.fiXXX", false, false)); // match whole string
            media_rf_rules.push_back(folder_item("[LOCALAPPDATA]/clear.fiXXX", false, true)); // match whole string

            // switch to cloudpc for we are trying to extract the library pathes from cloudpc
            SET_TARGET_MACHINE(TEST_REMOTEFILE_STR, alias_cloudpc.c_str(), rv);
            if (rv < 0) {
                setCcdTestInstanceNum(cloudPCId);
            }

            // get library rules
            get_media_libraries(media_rf_rules, true, full);

            if(full){
                media_rf_rules.push_back(folder_item("Computer/C/Users/Public/Pictures", true, false, 0));
                media_rf_rules.push_back(folder_item("Computer/C/Users/Public/Pictures", true, false, VPLFILE_MODE_IRUSR));
                media_rf_rules.push_back(folder_item("Computer/C/Users/Public/Pictures", true, false, VPLFILE_MODE_IRUSR, VPLFILE_MODE_IRUSR));
                media_rf_rules.push_back(folder_item("Computer/C/Users/Public/Pictures", true, false, VPLFILE_MODE_IRUSR|VPLFILE_MODE_IWUSR, VPLFILE_MODE_IRUSR));
            }


            // rf allowed
            rf_rules.push_back(folder_item(localapp_folder + "/clear.fi", true, false));
            rf_rules.push_back(folder_item("[LOCALAPPDATA]/clear.fi", true, true));
            rf_rules.push_back(folder_item(userprofile_folder + "/Picstream", true, false));
            rf_rules.push_back(folder_item("[USERPROFILE]/Picstream", true, true));
            rf_rules.push_back(folder_item("Computer/C", true, false));
            rf_rules.push_back(folder_item("[LOCALAPPDATA]", true, true));
            rf_rules.push_back(folder_item("[USERPROFILE]", true, true));
            rf_rules.push_back(folder_item(localapp_folder+"/CLEAR.FI", true, false)); // case sensitive
            rf_rules.push_back(folder_item("[LOCALAPPDATA]/CLEAR.FI", true, true)); // case sensitive
            rf_rules.push_back(folder_item(localapp_folder+"/clear.fiXXX", true, false)); // match whole string
            rf_rules.push_back(folder_item("[LOCALAPPDATA]/clear.fiXXX", true, true)); // match whole string

            get_media_libraries(rf_rules, false, full);
            if(full){
                rf_rules.push_back(folder_item("Computer/C/Users/Public", true, false, 0));
                rf_rules.push_back(folder_item("Computer/C/Users/Public", true, false, VPLFILE_MODE_IRUSR));
                rf_rules.push_back(folder_item("Computer/C/Users/Public", true, false, VPLFILE_MODE_IRUSR, VPLFILE_MODE_IRUSR));
                rf_rules.push_back(folder_item("Computer/C/Users/Public", true, false, VPLFILE_MODE_IRUSR|VPLFILE_MODE_IWUSR, VPLFILE_MODE_IRUSR));
            }

            test_cases["RF"] = rf_rules;
            test_cases["MediaRF"] = media_rf_rules;
        }

        std::map <std::string, folder_access_rules>::iterator tc_it;

        // clean-up dummy files @ client-side
        SET_TARGET_MACHINE(TEST_REMOTEFILE_STR, alias_client.c_str(), rv);
        if (rv < 0) {
            setCcdTestInstanceNum(clientPCId);
        }

        {
            TargetDevice *target = getTargetDevice();
            target->deleteFile(convert_path_convention(clientPCSeparator, dummy1KB));
            target->deleteFile(convert_path_convention(clientPCSeparator, dummy1MB));
            delete target;
        }

        // clean-up dummy files @ control-side
        Util_rm_dash_rf(dummy1KB_local);
        Util_rm_dash_rf(dummy1MB_local);

        // create dummy files @ control-side
        rv = create_dummy_file(dummy1KB_local.c_str(), 1*1024);
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "PrepareDataForCheckAccessRules", rv);
        rv = create_dummy_file(dummy1MB_local.c_str(), 1*1024*1024);
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "PrepareDataForCheckAccessRules", rv);

        // push dummy files from control-side to client-side
        {
            TargetDevice *target = getTargetDevice();
            rv = target->pushFile(dummy1KB_local,
                                  convert_path_convention(clientPCSeparator, dummy1KB));
            if (rv == 0) {
                rv = target->pushFile(dummy1MB_local,
                                      convert_path_convention(clientPCSeparator, dummy1MB));
            }
            delete target;
        }
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "PrepareDataForCheckAccessRules", rv);

        // prepare file list
        filelists.push_back(dummy1KB);
        filelists.push_back(dummy1MB);

        filelists_local.push_back(dummy1KB_local);
        filelists_local.push_back(dummy1MB_local);

        for (tc_it = test_cases.begin(); tc_it != test_cases.end(); tc_it++) {
            bool is_media_rf = tc_it->first == "RF"? false : true;

            rv = remotefile_feature_enable(cloudPCId, clientPCId, userId, deviceId,!is_media_rf,is_media_rf, alias_cloudpc, alias_client);
            CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, is_media_rf ? "DisableRFEnableMediaRFFlags" : "EnableRFDisableMediaRFFlags" , rv);

            std::string test_str = "SdkRemoteFileRelease_"+tc_it->first;

            folder_access_rules::iterator folder_it;
            for (folder_it = tc_it->second.begin(); folder_it != tc_it->second.end(); folder_it++) {
                std::string tc_name = "CheckAccessRule("+folder_it->path+")";
                LOG_ALWAYS("\n\n=> Testing namespace = %s, folder = %s, allow access = %d, is_virt_folder = %d\n",
                           tc_it->first.c_str(), folder_it->path.c_str(), folder_it->allow, folder_it->virt_folder);
                rv = remotefile_access_checking(folder_it->path,
                                                 datasetId_str,
                                                 download_file,
                                                 download_file_local,
                                                 filelists,
                                                 filelists_local,
                                                 is_media_rf,
                                                 folder_it->allow,
                                                 folder_it->virt_folder,
                                                 folder_it->file_permission,
                                                 folder_it->dir_permission);
                                                
                // check the retval and break if err
                CHECK_AND_PRINT_RESULT(test_str.c_str(), tc_name.c_str(), rv);
            }
        }

        // clean-up dummy files @ client-side after the security rule testing
        SET_TARGET_MACHINE(TEST_REMOTEFILE_STR, alias_client.c_str(), rv);
        if (rv < 0) {
            setCcdTestInstanceNum(clientPCId);
        }

        {
            TargetDevice *target = getTargetDevice();
            target->deleteFile(convert_path_convention(clientPCSeparator, dummy1KB));
            target->deleteFile(convert_path_convention(clientPCSeparator, dummy1MB));
            delete target;
        }

        // clean-up dummy files @ control-side
        Util_rm_dash_rf(dummy1KB_local);
        Util_rm_dash_rf(dummy1MB_local);
    }

    if (isWindows(cloudPCOSVersion)) {
        // only do this on windows platform.
        LOG_ALWAYS("\n\n== /rf ReadDir(Computer)");
        RF_READ_DIR(false, datasetId_str, "Computer", response, rv);

        LOG_ALWAYS("\n\n== /rf ReadDir(Libraries) ==");
        RF_READ_DIR(false, datasetId_str, "Libraries", response, rv);

        rv = remotefile_feature_enable(cloudPCId, clientPCId, userId, deviceId, false, true, alias_cloudpc, alias_client);
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR,  "DisableRFEnableMediaRFFlags", rv);

        // /media_rf can read Libraries
        LOG_ALWAYS("\n\n== /media_rf ReadDir(Libraries) ==");
        RF_READ_DIR(true, datasetId_str, "Libraries", response, rv);
    }

    // create temporarily files and push from control to client side
    for (unsigned int i = 0; i < NR_FILES; i++) {
        std::ostringstream oss;
        oss << work_dir_local << "/temp" << i;
        temp_filelist_local.push_back(std::make_pair(oss.str(), (MAX_SIZE/(i+1))));
        oss.str("");
        oss << work_dir << "/temp" << i;
        temp_filelist.push_back(std::make_pair(oss.str(), (MAX_SIZE/(i+1))));

        LOG_ALWAYS("Create local file[%d] = %s, remote file = %s, size = "FMTu64,
                   i, temp_filelist_local[i].first.c_str(), temp_filelist[i].first.c_str(),
                   temp_filelist[i].second);
        // clean-up control-side dummy files
        Util_rm_dash_rf(temp_filelist_local[i].first);
        // create control-side dummy files
        rv = create_dummy_file(temp_filelist_local[i].first.c_str(), temp_filelist_local[i].second);
        if (rv != 0) {
            break;
        }

        // clean-up client-side dummy files and push control dummy file to client
        {
            TargetDevice *target = getTargetDevice();
            rv = target->deleteFile(convert_path_convention(clientPCSeparator, temp_filelist[i].first));
            if (rv != 0 && rv != VPL_ERR_NOENT) {
                LOG_ERROR("Fail to clean-up client-side dummy file: %s, rv = %d",
                          temp_filelist[i].first.c_str(), rv);
                delete target;
                break;
            }
            rv = target->pushFile(temp_filelist_local[i].first,
                                  convert_path_convention(clientPCSeparator, temp_filelist[i].first));
            if (rv != 0 && rv != VPL_ERR_NOENT) {
                LOG_ERROR("Fail to push dummy file from control to client: %s -> %s, rv = %d",
                          temp_filelist_local[i].first.c_str(), temp_filelist[i].first.c_str(), rv);
                delete target;
                break;
            }
            delete target;
        }
    }
    CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "PrepareDataForReadDirPagination", rv);

    // feature flag enable/disable test case enhancement
    // - what we are trying to do here is to disable/enable both RF & MediaRF flags and see
    //   if the server complies
    {
        LOG_ALWAYS("\n\n==== Test disable/enable both RemoteFile/MediaFile feautre ====");

        // directory preparation
        rv = remotefile_feature_enable(cloudPCId, clientPCId, userId, deviceId, true, true, alias_cloudpc, alias_client);
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR,  "EnableRFEnableMediaRFFlags", rv);

        rv = remotefile_mkdir_recursive(upload_dir, datasetId_str, false);
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "PrepareDirectoryForFeatureFlagTest", rv);

        LOG_ALWAYS("\n\n== Disable RemoteFile/MediaFile feature on CloudPC/Client ==");
        rv = remotefile_feature_enable(cloudPCId, clientPCId, userId, deviceId, false, false, alias_cloudpc, alias_client);
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "DisableRFnMediaRFFeatureFlags", rv);
        LOG_ALWAYS("\n\n==  Access RemoteFile/MediaFile on CloudPC when they are disabled ==");

        SET_TARGET_MACHINE(TEST_REMOTEFILE_STR, alias_client.c_str(), rv);
        if (rv < 0) {
            setCcdTestInstanceNum(clientPCId);
        }
        RF_READ_DIR_SKIP(false, datasetId_str, upload_dir.c_str(), response, rv); /*expect to fail*/
        rv = rv == 0? -1 : 0;
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "AccessWhenFeatureRFnMediaRFDisabled", rv);
        RF_READ_DIR_SKIP(true, datasetId_str, upload_dir.c_str(), response, rv); /*expect to fail*/
        rv = rv == 0? -1 : 0;
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "AccessWhenFeatureRFnMediaRFDisabled", rv);
        LOG_ALWAYS("\n\n== Enable RemoteFile/MediaFile feature on CloudPC/Client ==");
        rv = remotefile_feature_enable(cloudPCId, clientPCId, userId, deviceId, true, true, alias_cloudpc, alias_client);
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "EnableRFnMediaRFFeatureFlags", rv);
        LOG_ALWAYS("\n\n==  Access RemoteFile/MediaFile on CloudPC when they are enable ==");
        SET_TARGET_MACHINE(TEST_REMOTEFILE_STR, alias_client.c_str(), rv);
        if (rv < 0) {
            setCcdTestInstanceNum(clientPCId);
        }

        rv = remotefile_feature_enable(cloudPCId, clientPCId, userId, deviceId, true, false,alias_cloudpc, alias_client);
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "EnableRFDisableMediaRFFeatureFlags", rv);

        RF_READ_DIR_SKIP(false, datasetId_str, upload_dir.c_str(), response, rv); /*expect to succeed*/
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "AccessWhenFeatureRFnMediaRFEnabled", rv);

        rv = remotefile_feature_enable(cloudPCId, clientPCId, userId, deviceId, false, true ,alias_cloudpc, alias_client);
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "DisableRFEnableMediaRFFeatureFlags", rv)

        RF_READ_DIR_SKIP(true, datasetId_str, upload_dir.c_str(), response, rv); /*expect to succeed*/
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "AccessWhenFeatureRFnMediaRFEnabled", rv);

        rv = remotefile_feature_enable(cloudPCId, clientPCId, userId, deviceId, true, false, alias_cloudpc, alias_client);
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "EnableRFDisableMediaRFFeatureFlags", rv)

        // clean-up directory
        RF_DELETE_DIR_SKIP(false, datasetId_str, upload_dir.c_str(), response, rv);
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "CleanUpDirectoryForFeatureFlagTesting", rv);
    }

    // original autotest cases
    is_media = false;
    do {
        if (is_media) {
            TEST_REMOTEFILE_STR = "SdkRemoteFileRelease_MediaRF";
        } else {
            TEST_REMOTEFILE_STR = "SdkRemoteFileRelease_RF";
        }

        rv = remotefile_feature_enable(cloudPCId, clientPCId, userId, deviceId, !is_media, is_media, alias_cloudpc, alias_client);
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, is_media ? "DisableRFEnableMediaRFFlags" : "EnableRFDisableMediaRFFlags" , rv);

        rv = autotest_sdk_release_remotefile_basic(is_media,
                                                   is_support_media,
                                                   full, 
                                                   userId,
                                                   deviceId,
                                                   datasetId_str,
                                                   work_dir,
                                                   work_dir_client,
                                                   work_dir_local,
                                                   upload_dir,
                                                   temp_filelist,
                                                   cloudPCSeparator,
                                                   clientPCSeparator,
                                                   clientPCSeparator2);

        if(rv != 0)
            goto exit;

        if (is_media) {
            LOG_ALWAYS("finish testing /rf and /media_rf. break the loop");
            break;
        } else {
            // test /media_rf
            is_media = true;
        }
    } while (true);

    //upload 1000 files
    if(full && (isWindows(clientPCOSVersion) || clientPCOSVersion.compare(OS_LINUX) == 0)){

        rv = remotefile_feature_enable(-1, -1, userId, deviceId, true, true, alias_cloudpc, alias_client);
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "EnableRFnMediaRFFeatureFlags", rv);

        // create the directory recursively.
        rv = remotefile_mkdir_recursive(upload_dir, datasetId_str, is_media);
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "MakeDirRecursively", rv);

        RF_READ_DIR(is_media, datasetId_str, upload_dir.c_str(), response, rv);

        // push dummy files from control-side to client-side
        {
            // clean-up dummy files @ control-side
            Util_rm_dash_rf(dummy1KB_local);

            // create dummy files @ control-side
            rv = create_dummy_file(dummy1KB_local.c_str(), 16);

            if(rv == 0){
                TargetDevice *target = getTargetDevice();
                rv = target->pushFile(dummy1KB_local,
                        convert_path_convention(clientPCSeparator, dummy1KB));
                delete target;
            }
        }
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "PrepareDataForUpload", rv);


        for (unsigned int i = 0; i < 1000; i++) {
            std::string upload_file = dummy1KB;
            std::string rf_file;
            std::stringstream ss;
            ss << upload_dir << "/file" << i;
            rf_file = ss.str();
            RF_XUPLOAD_SKIP(false, datasetId_str, upload_file.c_str(), rf_file.c_str(), response, rv);
            if (rv != VPL_OK) {
                LOG_ERROR("Unable to submit xupload task: %s -> %s", upload_file.c_str(), rf_file.c_str());
            }
        }
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "AsyncUploadFilesToUploadDir_1000", rv);

        // wait for async upload complete
        rv = remotefile_wait_async_upload_done(false, false);
        if (rv != VPL_OK) {
            LOG_ERROR("Async upload failed, %d", rv);
        }
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "WaitForAsyncUploadFilesComplete", rv);
    }

    // clean-up temporarily files
    Util_rm_dash_rf(download_file_local);
    for (unsigned int i = 0; i < temp_filelist_local.size(); i++) {
        Util_rm_dash_rf(temp_filelist_local[i].first);
    }
    {
        TargetDevice *target = getTargetDevice();
        target->deleteFile(convert_path_convention(clientPCSeparator, download_file)); // don't care the return for it's cleaning up?
        target->deleteFile(convert_path_convention(clientPCSeparator, test_clip_file)); // don't care the return for it's cleaning up?
        for (unsigned int i = 0; i < temp_filelist.size(); i++) {
            target->deleteFile(convert_path_convention(clientPCSeparator, temp_filelist[i].first)); // don't care the return for it's cleaning up?
        }
        delete target;
    }

    SET_TARGET_MACHINE(TEST_REMOTEFILE_STR, alias_client2.c_str(), rv);
    if (rv < 0) {
        setCcdTestInstanceNum(clientPCId2);
    }
    {
        TargetDevice *target = getTargetDevice();
        if (full) {
            target->deleteFile(convert_path_convention(clientPCSeparator2, download_file2));
        }
        target->deleteFile(convert_path_convention(clientPCSeparator2, test_clip_file2)); // don't care the return for it's cleaning up?
        delete target;
    }
    Util_rm_dash_rf(dummy20MB_local);

exit:
    rv = VPLMutex_Destroy(&thread_state_mutex);
    if (rv != VPL_OK) {
        LOG_ERROR("VPLMutex_Destroy() %d", rv);
    }
    if (jsonResponse) {
        cJSON2_Delete(jsonResponse);
        jsonResponse = NULL;
    }

    LOG_ALWAYS("\n\n== Freeing cloud PC ==");
    if (set_target_machine(alias_cloudpc.c_str()) < 0) {
        setCcdTestInstanceNum(cloudPCId);
    }

    {
        const char *testArg[] = { "StopCloudPC" };
        stop_cloudpc(1, testArg);
    }

    {
        const char *testArg[] = { "StopCCD" };
        stop_ccd_soft(1, testArg);
    }

    LOG_ALWAYS("\n\n== Freeing MD ==");
    if (set_target_machine(alias_client.c_str()) < 0) {
        setCcdTestInstanceNum(clientPCId);
    }

    {
        const char *testArg[] = { "StopClient" };
        stop_client(1, testArg);
    }

    if (isWindows(clientPCOSVersion) || clientPCOSVersion.compare(OS_LINUX) == 0) {
        const char *testArg[] = { "StopCCD" };
        stop_ccd_soft(1, testArg);
    }

    LOG_ALWAYS("\n\n== Freeing client ==");
    if (set_target_machine(alias_client2.c_str()) < 0) {
        setCcdTestInstanceNum(clientPCId2);
    }

    {
        const char *testArg[] = { "StopClient" };
        stop_client(1, testArg);
    }

    if (isWindows(clientPCOSVersion) || clientPCOSVersion.compare(OS_LINUX) == 0) {
        const char *testArg[] = { "StopCCD" };
        stop_ccd_soft(1, testArg);
    }
    return rv;
}

int do_autotest_sdk_release_remotefile_vcs(int argc, const char* argv[]){

    // This test suite is for testing /rf access VCS dataset
    // This is the main function which does client and cloud PC startup and info retrieving
    // The real functional test will be located in the functions named "autotest_sdk_release_remotefile_vcs_xxx"
    const char *TEST_REMOTEFILE_STR = "SdkRemoteFileVCS";

    int rv = 0;

    const char *domain = NULL;
    const char *username = NULL;
    const char *password = NULL;
    int pause_delay = 0;

    u64 userId = 0;
    int cloudPCId = 1;
    int clientPCId = 2;
    std::string alias_cloudpc = "CloudPC";
    std::string alias_client = "MD";
    std::string cloudPCOSVersion;
    std::string clientPCOSVersion;

    std::string cloudPCSeparator;
    std::string clientPCSeparator;

    std::string work_dir_md;
    std::string work_dir_local;

    if (checkHelp(argc, argv) || (argc < 4)) {
        printf("AutoTest %s <domain> <username> <password>\n", argv[0]);
        return 0;   // No arguments needed
    }

    for (int i = 1; i < argc; i++) {
        if (!domain) {
            domain = argv[i];
        }
        else if (!username) {
            username = argv[i];
        }
        else if (!password) {
            password = argv[i];
        }
        else {
            LOG_ERROR("Unexpected word %s", argv[i]);
            return 0;
        }
    }

    LOG_ALWAYS("Testing AutoTest SDK Release RemoteFile VCS: Domain(%s) User(%s) Password(%s)", domain, username, password);

    // Does a hard stop for all ccds
    {
        const char *testArg[] = { "StopCCD" };
        stop_ccd_hard(1, testArg);
    }

    LOG_ALWAYS("\n\n==== Launching Cloud PC CCD ====");
    SET_TARGET_MACHINE(TEST_REMOTEFILE_STR, alias_cloudpc.c_str(), rv);
    if (rv < 0) {
        setCcdTestInstanceNum(cloudPCId);
    }

    CHECK_LINK_REMOTE_AGENT(alias_cloudpc, TEST_REMOTEFILE_STR, rv);

    {
        QUERY_TARGET_OSVERSION(cloudPCOSVersion, TEST_REMOTEFILE_STR, rv);
    }

    {
        TargetDevice *target = getTargetDevice();

        rv = target->getDirectorySeparator(cloudPCSeparator);
        if (rv != 0) {
            LOG_ERROR("Unable to get the client pc path separator, rv = %d", rv);
            goto exit;
        }

        if (target != NULL) {
            delete target;
            target = NULL;
        }
    }

    START_CCD(TEST_REMOTEFILE_STR, rv);
    START_CLOUDPC(username, password, TEST_REMOTEFILE_STR, true, rv);

    LOG_ALWAYS("\n\n==== Launching MD CCD ====");
    SET_TARGET_MACHINE(TEST_REMOTEFILE_STR, alias_client.c_str(), rv);
    if (rv < 0) {
        setCcdTestInstanceNum(clientPCId);
    }

    CHECK_LINK_REMOTE_AGENT(alias_client, TEST_REMOTEFILE_STR, rv);

    {
        QUERY_TARGET_OSVERSION(clientPCOSVersion, TEST_REMOTEFILE_STR, rv);
    }

    {
        TargetDevice *target = getTargetDevice();

        rv = target->getDxRemoteRoot(/*out*/work_dir_md);
        if (rv != 0) {
            LOG_ERROR("Unable to get the work directory from target device, rv = %d", rv);
            delete target;
            goto exit;
        }
        rv = target->getDirectorySeparator(/*out*/clientPCSeparator);
        if (rv != 0) {
            LOG_ERROR("Unable to get the client pc path separator, rv = %d", rv);
            goto exit;
        }

        if( target != NULL) {
            delete target;
            target =NULL;
        }
    }

    if (isWindows(clientPCOSVersion) || clientPCOSVersion.compare(OS_LINUX) == 0) {
        START_CCD(TEST_REMOTEFILE_STR, rv);
    }

    START_CLIENT(username, password, TEST_REMOTEFILE_STR, true, rv);

    LOG_ALWAYS("Cloud  PC OS Version: %s", cloudPCOSVersion.c_str());
    LOG_ALWAYS("Client_MD PC OS Version: %s", clientPCOSVersion.c_str());

    LOG_ALWAYS("\n\n==== RemoteFile VCS ====");

    if (pause_delay > 0) {
        LOG_ALWAYS("\n\n== Pausing for %d secs ==", pause_delay);
        VPLThread_Sleep(VPLTime_FromSec(pause_delay));
    }

    // convert to forward slashes no matter how
    std::replace(work_dir_md.begin(), work_dir_md.end(), '\\', '/');

    // get test clip file path
    rv = getCurDir(work_dir_local);
    if (rv < 0) {
        LOG_ERROR("failed to get current dir. error = %d", rv);
        goto exit;
    }
#ifdef WIN32
    std::replace(work_dir_local.begin(), work_dir_local.end(), '\\', '/');
#endif // WIN32

    LOG_ALWAYS("Work directory = %s", work_dir_md.c_str());
    LOG_ALWAYS("Work directory (local) = %s", work_dir_local.c_str());

    // Get userId
    SET_TARGET_MACHINE(TEST_REMOTEFILE_STR, alias_cloudpc.c_str(), rv);
    if (rv < 0) {
        setCcdTestInstanceNum(cloudPCId);
    }

    rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Fail to get user id:%d", rv);
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "getCloudPcUserId", rv);
    }

    //-- Functional test cases
    rv = remotefile_vcs_internet_clipboard_basic(userId, work_dir_md, work_dir_local, clientPCSeparator);
    CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "remotefile_vcs_internet_clipboard_basic", rv);
    //--

exit:
    LOG_ALWAYS("\n\n== Freeing cloud PC ==");
    SET_TARGET_MACHINE(TEST_REMOTEFILE_STR, alias_cloudpc.c_str(), rv);
    if (rv < 0) {
        setCcdTestInstanceNum(cloudPCId);
    }

    {
        const char *testArg[] = { "StopCloudPC" };
        stop_cloudpc(1, testArg);
    }

    {
        const char *testArg[] = { "StopCCD" };
        stop_ccd_soft(1, testArg);
    }

    LOG_ALWAYS("\n\n== Freeing MD ==");
    SET_TARGET_MACHINE(TEST_REMOTEFILE_STR, alias_client.c_str(), rv);
    if (rv < 0) {
        setCcdTestInstanceNum(clientPCId);
    }

    {
        const char *testArg[] = { "StopClient" };
        stop_client(1, testArg);
    }

    if (isWindows(clientPCOSVersion) || clientPCOSVersion.compare(OS_LINUX) == 0) {
        const char *testArg[] = { "StopCCD" };
        stop_ccd_soft(1, testArg);
    }
    return rv;
}

int remotefile_vcs_internet_clipboard_basic(u64 userId, const std::string &work_dir_md, const std::string &work_dir_local, const std::string &clientPCSeparator) {

    // Internet Clipboard dataset accessing basic test
    // https://wiki.ctbg.acer.com/wiki/index.php/Internet_Clipboard_CCD_Design
    // --
    // Test procedure:
    //   * <Cloud PC>Add Internet Clipboard dataset
    //   * Get dataset id
    //   * Make dir "/rf_vcs_autotest"
    //   * Read dir "/rf_vcs_autotest"
    //   * Async upload RF_TEST_LARGE_FILE to dir "/rf_vcs_autotest"
    //   * Read filemetadata "/rf_vcs_autotest/RF_TEST_LARGE_FILE"
    //     * Verify the file size
    //   * Download file "/rf_vcs_autotest/RF_TEST_LARGE_FILE"
    //     * Compare with the original RF_TEST_LARGE_FILE
    //   * Delete file "/rf_vcs_autotest/RF_TEST_LARGE_FILE"
    //     * Read filemetadata "/rf_vcs_autotest/RF_TEST_LARGE_FILE" for verifying
    //   * Delete dir "/rf_vcs_autotest"
    //     * Read dir "/rf_vcs_autotest" for verifying
    //   * <Cloud PC>Delete Internet Clipboard dataset
    // --
    const char *TEST_REMOTEFILE_STR = "SdkRemoteFileVCSInternetClipboard";
    u64 datasetId = 0;
    std::string datasetId_str;

    std::string test_clip_file;
    std::string test_clip_file_local;

    const char *RF_VCS_UPLOAD_DIR = "/rf_vcs_autotest";
    std::string upload_dir = RF_VCS_UPLOAD_DIR;
    std::string upload_file = upload_dir+"/"+RF_TEST_LARGE_FILE;

    std::string download_file;
    std::string download_file_local;
    VPLFS_file_size_t upload_size;

    test_clip_file.assign(work_dir_md.c_str());
    test_clip_file.append("/");
    test_clip_file.append(RF_TEST_LARGE_FILE);

    test_clip_file_local.assign(work_dir_local.c_str());
    test_clip_file_local.append("/");
    test_clip_file_local.append(RF_TEST_LARGE_FILE);

    download_file.assign(work_dir_md.c_str());
    download_file.append("/");
    download_file.append(RF_TEST_LARGE_FILE);
    download_file.append(".clone");

    download_file_local.assign(work_dir_local.c_str());
    download_file_local.append("/");
    download_file_local.append(RF_TEST_LARGE_FILE);
    download_file_local.append(".clone");

    LOG_ALWAYS("Test upload file = %s", test_clip_file.c_str());
    LOG_ALWAYS("Test upload file (local) = %s", test_clip_file_local.c_str());
    LOG_ALWAYS("Download file = %s", download_file.c_str());

    int cloudPCId = 1;
    int clientPCId = 2;
    std::string alias_cloudpc = "CloudPC";
    std::string alias_client = "MD";

    int rv = 0;
    bool is_media = false;

    // Add Internet Clipboard dataset
    LOG_ALWAYS("\n\n== Adding Internet Clipboard dataset ==");
    SET_TARGET_MACHINE(TEST_REMOTEFILE_STR, alias_cloudpc.c_str(), rv);
    if (rv < 0) {
        setCcdTestInstanceNum(cloudPCId);
    }

    {
        ccd::AddDatasetInput addDatasetInput;
        addDatasetInput.set_user_id(userId);
        addDatasetInput.set_dataset_name("Internet Clipboard");
        addDatasetInput.set_dataset_type(ccd::NEW_DATASET_TYPE_CACHE);
        ccd::AddDatasetOutput addDatasetOutput;
        rv = CCDIAddDataset(addDatasetInput, addDatasetOutput);
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "CCDIAddDataset", rv);

        datasetId = addDatasetOutput.dataset_id();
    }

    {
        std::stringstream ss;
        ss << datasetId;
        datasetId_str = ss.str();
    }

    // Poll until the dataset info has been updated to client(30 seconds, 1 second delay between each run)
    SET_TARGET_MACHINE(TEST_REMOTEFILE_STR, alias_client.c_str(), rv);
    if (rv < 0) {
        setCcdTestInstanceNum(clientPCId);
    }
    {
        VPLTime_t startTime = VPLTime_GetTimeStamp();
        VPLTime_t endTime = startTime + VPLTime_FromSec(30);

        bool isTimeout = true;
        while(VPLTime_GetTimeStamp() < endTime) {
            u64 clientDatasetId = 0;
            rv = getDatasetId(userId, "Internet Clipboard", clientDatasetId);
            if (rv == 0) {
                if (clientDatasetId != datasetId) {
                    LOG_ERROR("Retrieved datasetId:"FMTu64" doesn't match with the one returned from CCDIAddDataset:"FMTu64, clientDatasetId, datasetId);
                    rv = -1;
                } else {
                    LOG_ALWAYS("datasetId = "FMTu64, clientDatasetId);
                }
                isTimeout = false;
                break;
            } else {
                VPLThread_Sleep(VPLTIME_FROM_SEC(1));
            }
        }
        if (isTimeout) {
            LOG_ERROR("Retrieve datasetId timeout(30 sec)");
            rv = -1;
        }
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "getDatasetId", rv);
    }

    LOG_ALWAYS("\n\n== Testing AutoTest SDK Release RemoteFile VCS Internet Clipboard ==");

    // Pushing file to MD
    SET_TARGET_MACHINE(TEST_REMOTEFILE_STR, alias_client.c_str(), rv);
    if (rv < 0) {
        setCcdTestInstanceNum(clientPCId);
    }
    {
        VPLFS_stat_t stat;

        rv = VPLFS_Stat(test_clip_file_local.c_str(), &stat);
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "StatTestClipFile", rv);
        upload_size = stat.size;
    }
    {
        TargetDevice *target = getTargetDevice();
        rv = target->pushFile(test_clip_file_local,
                              convert_path_convention(clientPCSeparator, test_clip_file));
        if (rv != 0) {
            // clean-up remote upload directory which is used in the subsequent testing for Win32
            // for cloudnode, since the user is delete and create everytime. the directories are always clean
            LOG_ERROR("Fail to push file from local to remote: %s --> %s, rv = %d",
                      test_clip_file_local.c_str(), test_clip_file.c_str(), rv);
            goto exit;
        }
        delete target;
    }

    // clean-up before testing
    LOG_ALWAYS("\n\n== CleanUp download directory ==");

    // download file is located at MD client
    SET_TARGET_MACHINE(TEST_REMOTEFILE_STR, alias_client.c_str(), rv);
    if (rv < 0) {
        setCcdTestInstanceNum(clientPCId);
    }

    {
        TargetDevice *target = getTargetDevice();
        rv = target->deleteFile(convert_path_convention(clientPCSeparator, download_file));
        if (rv != 0 && rv != VPL_ERR_NOENT) {
            LOG_ERROR("Fail to clean-up file: %s, rv = %d", download_file.c_str(), rv);
            goto exit;
        }
        delete target;
    }

    // Main functional testing codes
    {
        std::string response;
        TEST_REMOTEFILE_STR = "SdkRemoteFile_RF_VCS_InternetClipboard";

        LOG_ALWAYS("\n\n==== Test RemoteFile VCS Internet Clipboard Upload/Download/Mkdir/DeleteDir/ReadDir - %s ====", TEST_REMOTEFILE_STR);

        // create the directory recursively.
        rv = remotefile_mkdir_recursive(upload_dir, datasetId_str, is_media);
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "MakeDirRecursively", rv);

        RF_READ_DIR(is_media, datasetId_str, upload_dir.c_str(), response, rv);

        RF_XUPLOAD(is_media, datasetId_str, test_clip_file.c_str(), upload_file.c_str(), response, rv);
        rv = remotefile_wait_async_upload_done(is_media, false);
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "WaitForAsyncUploadDone", rv);

        {
            cJSON2 *metadata = NULL;
            cJSON2 *jsonResponse = NULL;

            RF_READ_METADATA(is_media, datasetId_str, upload_file.c_str(), response, rv);

            jsonResponse = cJSON2_Parse(response.c_str());
            if (jsonResponse == NULL) {
                rv = -1;
            }

            metadata = cJSON2_GetObjectItem(jsonResponse, "revisionList");
            if (metadata == NULL || cJSON2_GetArraySize(metadata) == 0) {
                LOG_ERROR("error while parsing metadata");
                rv = -1;
            }

            // There is only 1 file revision
            cJSON2 *node = cJSON2_GetArrayItem(metadata, 0);
            if (node == NULL) {
                LOG_ERROR("error while parsing metadata for revision array");
                rv = -1;
            }

            cJSON2 *sizeNode = cJSON2_GetObjectItem(node, "size");
            if (sizeNode == NULL || sizeNode->type != cJSON2_Number) {
                LOG_ERROR("error while parsing metadata from revisionList");
                rv = -1;
            } else if (upload_size != sizeNode->valueint) {
                LOG_ERROR("size doesn't match: upload_size("FMTu64"), file metadata size("FMTu64")", (u64)upload_size, (u64)metadata->valueint);
                rv = -1;
            }
            CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "VerifyUploadFileSize", rv);

            if (jsonResponse) {
                cJSON2_Delete(jsonResponse);
                jsonResponse = NULL;
            }
        }

        RF_DOWNLOAD(is_media, datasetId_str, upload_file.c_str(), download_file.c_str(), response, rv);

        // Pull the downloaded file and compare it with the original upload file
        {
            TargetDevice *target = getTargetDevice();
            rv = target->pullFile(convert_path_convention(clientPCSeparator, download_file), download_file_local);
            if (rv != 0) {
                LOG_ERROR("Unable to pull file from client-pc to control-pc: %s, %s",
                        download_file.c_str(), download_file_local.c_str());
                rv = VPL_ERR_FAIL;
                goto exit;
            }
            delete target;
        }
        rv = file_compare(test_clip_file_local.c_str(), download_file_local.c_str());
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "VerifyFileDownloaded", rv);

        RF_DELETE_FILE(is_media, datasetId_str, upload_file.c_str(), response, rv);
        RF_READ_METADATA_SKIP(is_media, datasetId_str, upload_file.c_str(), response, rv); // expected failed
        rv = rv == 0? -1 : 0;
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "VerifyFileDeleted", rv);

        RF_DELETE_DIR(is_media, datasetId_str, upload_dir.c_str(), response, rv);
        RF_READ_DIR_SKIP(is_media, datasetId_str, upload_dir.c_str(), response, rv); // expected failed
        rv = rv == 0? -1 : 0;
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "VerifyDirectoryDeleted", rv);

    }

    LOG_ALWAYS("\n\n== Deleting Internet Clipboard dataset ==");
    SET_TARGET_MACHINE(TEST_REMOTEFILE_STR, alias_cloudpc.c_str(), rv);
    if (rv < 0) {
        setCcdTestInstanceNum(cloudPCId);
    }
    {
        ccd::DeleteDatasetInput deleteDatasetInput;
        deleteDatasetInput.set_user_id(userId);
        deleteDatasetInput.set_dataset_id(datasetId);
        CCDIDeleteDataset(deleteDatasetInput);
        CHECK_AND_PRINT_RESULT(TEST_REMOTEFILE_STR, "CCDIDeleteDataset", rv);
    }

    LOG_ALWAYS("finish testing /rf for Internet Clipboard dataset");

    // clean-up temporarily files
    Util_rm_dash_rf(download_file_local);
    SET_TARGET_MACHINE(TEST_REMOTEFILE_STR, alias_client.c_str(), rv);
    if (rv < 0) {
        setCcdTestInstanceNum(clientPCId);
        rv = 0;
    }
    {
        TargetDevice *target = getTargetDevice();
        target->deleteFile(convert_path_convention(clientPCSeparator, download_file)); // don't care the return for it's cleaning up?
        target->deleteFile(convert_path_convention(clientPCSeparator, test_clip_file)); // don't care the return for it's cleaning up?
        delete target;
    }

exit:
    return rv;
}
