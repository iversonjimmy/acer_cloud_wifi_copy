#ifndef _TARGET_LOCAL_DEVICE_HPP_
#define _TARGET_LOCAL_DEVICE_HPP_

#include "TargetDevice.hpp"
#include <set>
#include <map>

class TargetLocalDevice : public TargetDevice {
public:
    TargetLocalDevice();
    virtual ~TargetLocalDevice();

    virtual std::string getDeviceName();
    virtual std::string getDeviceClass();
    virtual std::string getOsVersion();
    virtual bool getIsAcerDevice();
    virtual bool getDeviceHasCamera();

    virtual int pushCcdConfig(const std::string &config);
    virtual int pullCcdConfig(std::string &config);

    virtual int removeDeviceCredentials();

    virtual int getCcdAppDataPath(std::string &path);
    virtual int getDxRootPath(std::string &path);
    virtual int getWorkDir(std::string &path);
    virtual int getDirectorySeparator(std::string &separator);
    virtual int getAliasPath(const std::string &alias, std::string &path);

    virtual int pushFile(const std::string &hostPath, const std::string &targetPath);
    virtual int pushFileSlow(const std::string &hostPath, const std::string &targetPath, const int timeMs, const int pulseSizeKb);
    virtual int pullFile(const std::string &targetPath, const std::string &hostPath);
    virtual int deleteFile(const std::string &targetPath);
    virtual int touchFile(const std::string &targetPath);
    virtual int statFile(const std::string &targetPath, VPLFS_stat_t &stat);
    virtual int renameFile(const std::string &srcPath, const std::string &dstPath);

    virtual int createDir(const std::string &targetPath, int mode, bool last);
    virtual int readDir(const std::string &targetPath, VPLFS_dirent_t &entry);
    virtual int removeDirRecursive(const std::string &targetPath);
    virtual int openDir(const std::string &targetPath);
    virtual int closeDir(const std::string &targetPath);
    virtual int readLibrary(const std::string &library_type, std::vector<std::pair<std::string, std::string> > &folders);

    virtual int pushFileContent(const std::string &content, const std::string &targetPath);
    virtual int pullFileContent(const std::string &targetPath, std::string &content);

    virtual int getFileSize(const std::string &filepath, uint64_t &fileSize);
    virtual int getDxRemoteRoot(std::string &path);
    virtual int setFilePermission(const std::string &path, const std::string &mode);

    virtual MMError MSABeginCatalog(const ccd::BeginCatalogInput& input);
    virtual MMError MSACommitCatalog(const ccd::CommitCatalogInput& input);
    virtual MMError MSAEndCatalog(const ccd::EndCatalogInput& input);
    virtual MMError MSABeginMetadataTransaction(const ccd::BeginMetadataTransactionInput& input);
    virtual MMError MSAUpdateMetadata(const ccd::UpdateMetadataInput& input);
    virtual MMError MSADeleteMetadata(const ccd::DeleteMetadataInput& input);
    virtual MMError MSACommitMetadataTransaction(void);
    virtual MMError MSAGetMetadataSyncState(media_metadata::GetMetadataSyncStateOutput& output);
    virtual MMError MSADeleteCollection(const ccd::DeleteCollectionInput& input);
    virtual MMError MSADeleteCatalog(const ccd::DeleteCatalogInput& input);
    virtual MMError MSAListCollections(media_metadata::ListCollectionsOutput& output);
    virtual MMError MSAGetCollectionDetails(const ccd::GetCollectionDetailsInput& input,
                                            ccd::GetCollectionDetailsOutput& output);
    virtual int tsTest(const TSTestParameters& test, TSTestResult& result);

    virtual int checkRemoteAgent(void);

private:
    std::map<std::string, VPLFS_dir_t> fsdir_map;
};

#endif // _TARGET_LOCAL_DEVICE_HPP_
