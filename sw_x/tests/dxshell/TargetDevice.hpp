#ifndef _TARGET_DEVICE_HPP_
#define _TARGET_DEVICE_HPP_

#include <vpl_types.h>
#include "vpl_fs.h"

#include <string>

// for MSA
#include "media_metadata_errors.hpp"
#include <ccdi.hpp>

// for tsTest
#include "ts_test.hpp"

#define OS_WINDOWS_RT "WindowsRT"
#define OS_WINDOWS    "Windows"
#define OS_ANDROID    "Android"
#define OS_iOS        "iOS"
#define OS_LINUX      "Linux"
#define OS_ORBE       "Orbe"

class TargetDevice {
public:
    TargetDevice();
    virtual ~TargetDevice();

    virtual std::string getDeviceName() = 0;
    virtual std::string getDeviceClass() = 0;
    virtual std::string getOsVersion() = 0;
    virtual bool getIsAcerDevice() = 0;
    virtual bool getDeviceHasCamera() = 0;
//    virtual bool getDeviceIsMediaServer() = 0;
//    virtual bool getDeviceIsVirtDrive() = 0;

    // push/pull to/from ccd.conf
    virtual int pushCcdConfig(const std::string &config) = 0;
    virtual int pullCcdConfig(std::string &config) = 0;

    virtual int removeDeviceCredentials() = 0;

    // get path on device
    virtual int getCcdAppDataPath(std::string &path) = 0;
    virtual int getDxRootPath(std::string &path) = 0;
    virtual int getWorkDir(std::string &path) = 0;
    virtual int getDirectorySeparator(std::string &separator) = 0;
    virtual int getAliasPath(const std::string &alias, std::string &path) = 0;

    // file transfer, similar to adb push/pull
    virtual int pushFile(const std::string &hostPath, const std::string &targetPath) = 0;
    virtual int pushFileSlow(const std::string &hostPath, const std::string &targetPath, const int timeMs, const int pulseSizeKb) = 0;
    virtual int pullFile(const std::string &targetPath, const std::string &hostPath) = 0;
    virtual int deleteFile(const std::string &targetPath) = 0;
    virtual int touchFile(const std::string &targetPath) = 0;
    virtual int statFile(const std::string &targetPath, VPLFS_stat_t &stat) = 0;
    virtual int renameFile(const std::string &srcPath, const std::string &dstPath) = 0;

    // directory operation
    virtual int createDir(const std::string &targetPath, int mode, bool last = VPL_TRUE) = 0;
    virtual int readDir(const std::string &targetPath, VPLFS_dirent_t &entry) = 0;
    virtual int removeDirRecursive(const std::string &targetPath) = 0;
    virtual int openDir(const std::string &targetPath) = 0;
    virtual int closeDir(const std::string &targetPath) = 0;
    virtual int readLibrary(const std::string &library_type, std::vector<std::pair<std::string, std::string> > &folders) = 0;

    // target-file content transfer
    virtual int pushFileContent(const std::string &content, const std::string &targetPath) = 0;
    virtual int pullFileContent(const std::string &targetPath, std::string &content) = 0;

    virtual int getFileSize(const std::string &filepath, uint64_t &fileSize) = 0;

    virtual int getDxRemoteRoot(std::string &path) = 0;

    virtual int setFilePermission(const std::string &path, const std::string &mode) = 0;

    virtual MMError MSABeginCatalog(const ccd::BeginCatalogInput& input) = 0;
    virtual MMError MSACommitCatalog(const ccd::CommitCatalogInput& input) = 0;
    virtual MMError MSAEndCatalog(const ccd::EndCatalogInput& input) = 0;
    virtual MMError MSABeginMetadataTransaction(const ccd::BeginMetadataTransactionInput& input) = 0;
    virtual MMError MSAUpdateMetadata(const ccd::UpdateMetadataInput& input) = 0;
    virtual MMError MSADeleteMetadata(const ccd::DeleteMetadataInput& input) = 0;
    //MMError MSAResetCache(void);  // this is DEPRECATED
    virtual MMError MSACommitMetadataTransaction(void) = 0;
    virtual MMError MSAGetMetadataSyncState(media_metadata::GetMetadataSyncStateOutput& output) = 0;
    virtual MMError MSADeleteCollection(const ccd::DeleteCollectionInput& input) = 0;
    virtual MMError MSADeleteCatalog(const ccd::DeleteCatalogInput& input) = 0;
    virtual MMError MSAListCollections(media_metadata::ListCollectionsOutput& output) = 0;
    virtual MMError MSAGetCollectionDetails(const ccd::GetCollectionDetailsInput& input,
                                            ccd::GetCollectionDetailsOutput& output) = 0;
    virtual int tsTest(const TSTestParameters& test, TSTestResult& result) = 0;

    virtual int checkRemoteAgent(void) = 0;
};

TargetDevice *getTargetDevice();

bool isIOS(const std::string &os);

#endif // _TARGET_DEVICE_HPP_
