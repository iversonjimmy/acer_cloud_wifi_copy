#include "TargetLocalDevice.hpp"
#include "common_utils.hpp"
#include "dx_common.h"
#include "ccd_utils.hpp"

#include <vplex_plat.h>
#include <vplex_file.h>
#include <vplu_format.h>
#include <gvm_file_utils.h>
#include <gvm_file_utils.hpp>
#include <log.h>

#include <sstream>

#include "common_utils.hpp"

#if defined(WIN32)
#include <comdef.h>
#endif

TargetLocalDevice::TargetLocalDevice()
{
}

TargetLocalDevice::~TargetLocalDevice()
{
}

std::string TargetLocalDevice::getDeviceName()
{
    std::string deviceName;

    char hostname[256];
    int rc = VPL_GetLocalHostname(hostname, sizeof(hostname));
    if (rc < 0) {
        LOG_ERROR("Failed to get local host name: %d", rc);
        goto out;
    }

    deviceName.assign(hostname);
    if (testInstanceNum) {
        std::ostringstream instanceSuffix;
        instanceSuffix << "_" << testInstanceNum;
        deviceName.append(instanceSuffix.str());
    }

 out:
    return deviceName;
}

std::string TargetLocalDevice::getDeviceClass()
{
    return "Notebook";
}

std::string TargetLocalDevice::getOsVersion()
{
    std::string osVersion;

#if defined(LINUX)
    osVersion = "Linux";
#elif defined(WIN32)
    osVersion = "Windows 7";
#else
#error unknown platform
#endif

    return osVersion;
}

bool TargetLocalDevice::getIsAcerDevice()
{
    return true;
}

bool TargetLocalDevice::getDeviceHasCamera()
{
    return true;
}

int TargetLocalDevice::pushCcdConfig(const std::string &config)
{
    int rc = 0;

    std::string ccdConfPath;
    rc = getCcdAppDataPath(ccdConfPath);
    if (rc != 0) {
        LOG_ERROR("Failed to get CCD App Data path: %d", rc);
        goto end;
    }
    ccdConfPath.append(DIR_DELIM "conf" DIR_DELIM "ccd.conf");

    rc = pushFileContent(config, ccdConfPath);
    if (rc != 0) {
        LOG_ERROR("Failed to write ccd config to %s: %d", ccdConfPath.c_str(), rc);
        goto end;
    }

 end:
    return rc;
}

int TargetLocalDevice::pullCcdConfig(std::string &config)
{
    int rc = 0;

    std::string ccdConfPath;
    rc = getCcdAppDataPath(ccdConfPath);
    if (rc != 0) {
        LOG_ERROR("Failed to get CCD App Data path: %d", rc);
        goto end;
    }
    ccdConfPath.append(DIR_DELIM "conf" DIR_DELIM "ccd.conf");

    if (VPLFile_CheckAccess(ccdConfPath.c_str(), VPLFILE_CHECKACCESS_EXISTS) != VPL_OK) {
        // ccd.conf does not exist -> return empty config
        config.clear();
        goto end;
    }

    rc = pullFileContent(ccdConfPath, config);
    if (rc != 0) {
        LOG_ERROR("Failed to read ccd config from %s: %d", ccdConfPath.c_str(), rc);
        goto end;
    }

 end:
    return rc;
}

int TargetLocalDevice::removeDeviceCredentials()
{
    int rc = 0;
    std::string ccdDevCredPath;

    rc = getCcdAppDataPath(ccdDevCredPath);
    if (rc != 0) {
        LOG_ERROR("Failed to get CCD App Data path: %d", rc);
        goto end;
    }
    ccdDevCredPath.append(DIR_DELIM "cc");

    Util_rm_dash_rf(ccdDevCredPath);

 end:
    return rc;
}

int TargetLocalDevice::getCcdAppDataPath(std::string &path)
{
    return ::getCcdAppDataPath(path);  // for now, call the one defined in ccd_utils.hpp
}

int TargetLocalDevice::getDxRootPath(std::string &path)
{
    return ::getDxRootPath(path);  // for now, call the one defined in ccd_utils.hpp
}

int TargetLocalDevice::getWorkDir(std::string &path)
{
    int rc = 0;
    path = ::getDxshellTempFolder();
    return rc;  // for now, call the one defined in dx_command.h
}

int TargetLocalDevice::getDirectorySeparator(std::string &separator)
{
    int rc = 0;
#ifdef WIN32
    separator.assign("\\");
#else
    separator.assign("/");
#endif
    return rc;
}

#if defined(WIN32)
static bool win32_com_initialized = false;
#endif
int TargetLocalDevice::getAliasPath(const std::string &alias, std::string &path)
{
    int rv = VPL_ERR_FAIL;

#if defined(WIN32)
    // Call CoInitialized to utilize the VPL_GetLibrary function
    if (win32_com_initialized == false) {
        HRESULT hres;

        // Initialize COM
        hres =  CoInitialize(NULL);
        if (FAILED(hres)) {
            return VPL_ERR_FAIL;
        }
        win32_com_initialized = true;
    }

    char *tmp_path = NULL;

    // get local app / user profile folder path
    if (alias == "LOCALAPPDATA") {
        rv = _VPLFS__GetLocalAppDataPath(&tmp_path);
        if (rv != VPL_OK || tmp_path == NULL) {
            LOG_ERROR("Unable to get local app folder path, rv = %d", rv);
            return rv;
        } else {
            path = tmp_path;
            free(tmp_path);
            return VPL_OK;
        }
    } else if (alias == "USERPROFILE") {
        rv = _VPLFS__GetProfilePath(&tmp_path);
        if (rv != VPL_OK || tmp_path == NULL) {
            LOG_ERROR("Unable to get user profile folder path, rv = %d", rv);
            return VPL_ERR_FAIL;
        } else {
            path = tmp_path;
            free(tmp_path);
            return VPL_OK;
        }
    }
#endif
    if (rv != VPL_OK) {
        LOG_ERROR("No path mapping for alias %s", alias.c_str());
    }
    return rv;
}

int TargetLocalDevice::createDir(const std::string &targetPath, int mode, bool last)
{
    int rc = 0;

    rc = Util_CreatePath(targetPath.c_str(), last);
    if (rc == VPL_OK) {
        LOG_ALWAYS("Success to Util_CreatePath");
    } else {
        LOG_ERROR ("Failed to Util_CreatePath: %s", targetPath.c_str());
    }

    return rc;
}

int TargetLocalDevice::readDir(const std::string &targetPath, VPLFS_dirent_t &entry)
{
    int rc = 0;
    std::set<std::string, VPLFS_dir_t>::iterator it;

    if (fsdir_map.find(targetPath) == fsdir_map.end()) {
        LOG_ERROR("Didn't open dir: %s", targetPath.c_str());
        return VPL_ERR_BADF;
    }

    if (fsdir_map.find(targetPath) != fsdir_map.end()) {
        rc = VPLFS_Readdir(&(fsdir_map.find(targetPath)->second), &entry);
        if (rc != VPL_OK) {
            LOG_ERROR("Failed read dir: %d, %s", rc, targetPath.c_str());
        }
    }
    else {
        rc = VPL_ERR_BADF;
    }

    return rc;
}

int TargetLocalDevice::removeDirRecursive(const std::string &targetPath)
{
    int rc = 0;

    rc = Util_rm_dash_rf(targetPath);
    if (rc == VPL_OK) {
        LOG_ALWAYS("Success to Util_rm_dash_rf");
    } else {
        LOG_ERROR ("Failed to Util_rm_dash_rf: %s", targetPath.c_str());
    }

    return rc;
}

int TargetLocalDevice::openDir(const std::string &targetPath)
{
    int rc = 0;
    VPLFS_dir_t dir_folder;
    std::pair<std::map<std::string, VPLFS_dir_t>::iterator, bool> ret;

    rc = VPLFS_Opendir(targetPath.c_str(), &dir_folder);
    if(rc != VPL_OK) {
        LOG_ERROR("Unable to open %s:%d", targetPath.c_str(), rc);
    }
    else {
        ret = fsdir_map.insert(std::pair<std::string, VPLFS_dir_t>(targetPath, dir_folder));
    }

    return rc;
}

int TargetLocalDevice::closeDir(const std::string &targetPath)
{
    int rc = 0;

    if (fsdir_map.find(targetPath) != fsdir_map.end()) {
        rc = VPLFS_Closedir(&(fsdir_map[targetPath]));
        if (rc == VPL_OK) {
            this->fsdir_map.erase(targetPath);
        }
    }
    else {
        rc = VPL_ERR_BADF;
    }

    return rc;
}

int TargetLocalDevice::readLibrary(const std::string &library_type, std::vector<std::pair<std::string, std::string> > &folders)
{
    int rc = 0;
#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
    int rv = 0;
    char *librariesPath = NULL;

    // Call CoInitialized to utilize the VPL_GetLibrary function
    if (win32_com_initialized == false) {
        HRESULT hres;

        // Initialize COM
        hres =  CoInitialize(NULL);
        if (FAILED(hres)) {
            return VPL_ERR_FAIL;
        }
        win32_com_initialized = true;
    }

    folders.clear();
    if (_VPLFS__GetLibrariesPath(&librariesPath) == VPL_OK) {
        VPLFS_dir_t dir;
        VPLFS_dirent_t dirent;

        if (VPLFS_Opendir(librariesPath, &dir) == VPL_OK) {
            while (VPLFS_Readdir(&dir, &dirent) == VPL_OK) {
                char *p = strstr(dirent.filename, ".library-ms");
                if (p != NULL && p[strlen(".library-ms")] == '\0') {
                    // found library description file
                    std::string libDescFilePath;
                    libDescFilePath.assign(librariesPath).append("/").append(dirent.filename);

                    // grab both localized and non-localized name of the library folders
                    _VPLFS__LibInfo libinfo;
                    rv = _VPLFS__GetLibraryFolders(libDescFilePath.c_str(), &libinfo);

                    if (libinfo.folder_type != library_type) {
                        // doens't match
                        //LOG_ALWAYS("folder_type = %s, request_type = %s", libinfo.folder_type.c_str(), library_type.c_str());
                        continue;
                    }

                    // subfolders
                    std::map<std::string, _VPLFS__LibFolderInfo>::const_iterator it;
                    for (it = libinfo.m.begin(); it != libinfo.m.end(); it++) {
                        std::string subfolder = it->second.path.c_str();
                        std::string subfolder_virt = "Libraries/" + libinfo.n_name + "/" + it->second.n_name;
                        //LOG_ALWAYS("-@-> Real: %s, Virt: %s", subfolder.c_str(),  subfolder_virt.c_str());
                        // put the real folder before the virtual folder to do the clean-up
                        folders.push_back(std::make_pair(subfolder, subfolder_virt));
                    }
                }
            }
            VPLFS_Closedir(&dir);
        }
        if (librariesPath) {
            free(librariesPath);
            librariesPath = NULL;
        }
    }
#endif //defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
    return rc;
}

static int file_copy_pulse(const char* src, const char* dst,
    const int timeMs, const int pulseSizeKb)
{

    const int BUF_SIZE = 16 * 1024;

    int rv = 0;
    VPLFS_stat_t stat_src;
    VPLFile_handle_t handle_src = 0, handle_dst = 0;
    char *src_buf = NULL;
    ssize_t src_read, dst_write;
    VPLFile_offset_t offset;

    if (src == NULL || dst == NULL) {
        LOG_ERROR("file path is NULL");
        return -1;
    }
    if (strcmp(src, dst) == 0) {
        LOG_ALWAYS("source and target path are the same");
        return 0;
    }
    LOG_ALWAYS("Copy from %s to %s", src, dst);
    if (VPLFS_Stat(src, &stat_src)) {
        LOG_ERROR("fail to stat src file: %s", src);
        return -1;
    }
    if (stat_src.type != VPLFS_TYPE_FILE) {
        LOG_ERROR("unable to copy non-file: %s, type = %d", src, stat_src.type);
        return -1;
    }
    if (strcmp(src, dst) == 0) {
        LOG_ALWAYS("source and target path are the same");
        return 0;
    }
    // open file
    handle_src = VPLFile_Open(src, VPLFILE_OPENFLAG_READONLY, 0);
    if (!VPLFile_IsValidHandle(handle_src)) {
        LOG_ERROR("cannot open src file: %s", src);
        rv = -1;
        goto exit;
    }
    handle_dst = VPLFile_Open(dst, VPLFILE_OPENFLAG_CREATE | VPLFILE_OPENFLAG_TRUNCATE | VPLFILE_OPENFLAG_WRITEONLY, 0777);
    if (!VPLFile_IsValidHandle(handle_dst)) {
        LOG_ERROR("cannot open dst file: %s", dst);
        rv = -1;
        goto exit;
    }
    src_buf = new char[BUF_SIZE];
    if (src_buf == NULL) {
        LOG_ERROR("cannot allocate memory for src buffer");
        rv = -1;
        goto exit;
    }
    offset = 0;
    while (rv == 0) {
        ssize_t pulseBytes = pulseSizeKb*1000;
        do {
            ssize_t transferBytes = pulseBytes;
            if(pulseBytes > BUF_SIZE) {
                transferBytes = BUF_SIZE;
            }
            src_read = VPLFile_Read(handle_src, src_buf, transferBytes);
            if (src_read <= 0) {
                LOG_ERROR("unable to read file: %d", src_read);
                rv = -1;
                break;
            }
            dst_write = VPLFile_Write(handle_dst, src_buf, src_read);
            if (dst_write < 0) {
                LOG_ERROR("unable to write file: %d", dst_write);
                rv = -1;
                break;
            }
            if (src_read != dst_write) {
                LOG_ERROR("src_read(%d) != dst_write(%d)", src_read, dst_write);
                rv = -1;
                break;
            }
            offset += src_read;
            pulseBytes -= src_read;
        } while (pulseBytes > 0);

        if(src_read <= 0) {
            break;
        }
        VPLThread_Sleep(VPLTIME_FROM_MILLISEC(timeMs));
    }

    LOG_DEBUG("Total length: "FMTu_VPLFile_offset__t, offset);
exit:
    if (VPLFile_IsValidHandle(handle_src)) {
        VPLFile_Close(handle_src);
    }
    if (VPLFile_IsValidHandle(handle_dst)) {
        VPLFile_Close(handle_dst);
        if (rv) {
            int srv;
            // error happens while copying files. remove it.
            srv = VPLFile_Delete(dst);
            if (srv) {
                LOG_ERROR("Failed to cleanup file %s, %d", dst, srv);
            }
        }
    }
    if (src_buf != NULL) {
        delete [] src_buf;
        src_buf = NULL;
    }
    return rv;
}

int TargetLocalDevice::pushFile(const std::string &hostPath, const std::string &targetPath)
{
    return file_copy(hostPath.c_str(), targetPath.c_str());
}

int TargetLocalDevice::pushFileSlow(const std::string &hostPath, const std::string &targetPath,
    const int timeMs, const int pulseSizeKb)
{
    return file_copy_pulse(hostPath.c_str(), targetPath.c_str(), timeMs, pulseSizeKb);
}

int TargetLocalDevice::pullFile(const std::string &targetPath, const std::string &hostPath)
{
    return file_copy(targetPath.c_str(), hostPath.c_str());
}

int TargetLocalDevice::deleteFile(const std::string &targetPath)
{
    return Util_rm_dash_rf(targetPath);
}

int TargetLocalDevice::touchFile(const std::string &targetPath)
{
    return VPLFile_SetTime(targetPath.c_str(), VPLTime_GetTime());
}

int TargetLocalDevice::statFile(const std::string &targetPath, VPLFS_stat_t &stat)
{
    return VPLFS_Stat(targetPath.c_str(), &stat);
}

int TargetLocalDevice::renameFile(const std::string &srcPath, const std::string &dstPath)
{
    return VPLFile_Rename(srcPath.c_str(), dstPath.c_str());
}

int TargetLocalDevice::pushFileContent(const std::string &content, const std::string &targetPath)
{
    int rc = 0;

    rc = Util_WriteFile(targetPath.c_str(), content.data(), content.size());
    if (rc != VPL_OK) {
        LOG_ERROR("Failed to write to %s: %d", targetPath.c_str(), rc);
        goto end;
    }

 end:
    return rc;
}

int TargetLocalDevice::pullFileContent(const std::string &targetPath, std::string &content)
{
    int rc = 0;
    char *fileContentBuf = NULL;
    int fileSize = -1;

    rc = VPLFile_CheckAccess(targetPath.c_str(), VPLFILE_CHECKACCESS_EXISTS);
    if (rc != VPL_OK) {
        LOG_ERROR("Failed to access file at %s: %d", targetPath.c_str(), rc);
        goto end;
    }

    fileSize = Util_ReadFile(targetPath.c_str(), (void**)&fileContentBuf, 0);
    if (fileSize < 0) {
        rc = fileSize;
        LOG_ERROR("Failed to read from %s: %d", targetPath.c_str(), rc);
        goto end;
    }
    content.assign(fileContentBuf, fileSize);

 end:
    if (fileContentBuf != NULL) {
        free(fileContentBuf);
    }
    return rc;
}

int TargetLocalDevice::getFileSize(const std::string &filepath, uint64_t &fileSize)
{
    int rc = 0;
    VPLFS_stat_t stat;
    rc = VPLFS_Stat(filepath.c_str(), &stat);
    if (rc != VPL_OK) {
        goto end;
    }

    fileSize = stat.size;
end:
    return rc;
}

int TargetLocalDevice::getDxRemoteRoot(std::string &path)
{
    return ::getCcdAppDataPath(path);
}

MMError TargetLocalDevice::MSABeginCatalog(const ccd::BeginCatalogInput& input)
{
    return CCDIMSABeginCatalog(input);
}

MMError TargetLocalDevice::MSACommitCatalog(const ccd::CommitCatalogInput& input)
{
    return CCDIMSACommitCatalog(input);
}

MMError TargetLocalDevice::MSAEndCatalog(const ccd::EndCatalogInput& input)
{
    return CCDIMSAEndCatalog(input);
}

MMError TargetLocalDevice::MSABeginMetadataTransaction(const ccd::BeginMetadataTransactionInput& input)
{
    return CCDIMSABeginMetadataTransaction(input);
}

MMError TargetLocalDevice::MSAUpdateMetadata(const ccd::UpdateMetadataInput& input)
{
    return CCDIMSAUpdateMetadata(input);
}

MMError TargetLocalDevice::MSADeleteMetadata(const ccd::DeleteMetadataInput& input)
{
    return CCDIMSADeleteMetadata(input);
}

MMError TargetLocalDevice::MSACommitMetadataTransaction(void)
{
    return CCDIMSACommitMetadataTransaction();
}

MMError TargetLocalDevice::MSAGetMetadataSyncState(media_metadata::GetMetadataSyncStateOutput& output)
{
    return CCDIMSAGetMetadataSyncState(output);
}

MMError TargetLocalDevice::MSADeleteCollection(const ccd::DeleteCollectionInput& input)
{
    return CCDIMSADeleteCollection(input);
}

MMError TargetLocalDevice::MSADeleteCatalog(const ccd::DeleteCatalogInput& input)
{
    return CCDIMSADeleteCatalog(input);
}

MMError TargetLocalDevice::MSAListCollections(media_metadata::ListCollectionsOutput& output)
{
    return CCDIMSAListCollections(output);
}

MMError TargetLocalDevice::MSAGetCollectionDetails(const ccd::GetCollectionDetailsInput& input,
                                                   ccd::GetCollectionDetailsOutput& output)
{
    return CCDIMSAGetCollectionDetails(input, output);
}

int TargetLocalDevice::tsTest(const TSTestParameters& test, TSTestResult& result)
{
    return runTsTest(test, result);
}

int TargetLocalDevice::checkRemoteAgent(void)
{
    return 0;
}

int TargetLocalDevice::setFilePermission(const std::string &path, const std::string &mode)
{
    LOG_WARN("Not yet implemented");   
    return 0;
}
