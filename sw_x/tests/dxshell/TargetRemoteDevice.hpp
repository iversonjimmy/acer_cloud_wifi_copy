#ifndef _TARGET_REMOTE_DEVICE_HPP_
#define _TARGET_REMOTE_DEVICE_HPP_

#include "TargetDevice.hpp"
#include <vpl_socket.h>
#include <vplu_types.h>
#include "dx_remote_agent.pb.h"

#include <set>

class TargetRemoteDevice : public TargetDevice {
public:
    TargetRemoteDevice();
    TargetRemoteDevice(VPLNet_addr_t ipaddr, VPLNet_port_t port);
    virtual ~TargetRemoteDevice();

    virtual std::string getDeviceName();
    virtual std::string getDeviceClass();
    virtual std::string getOsVersion();
    virtual bool getIsAcerDevice();
    virtual bool getDeviceHasCamera();

    virtual int pushCcdConfig(const std::string &config);
    virtual int pullCcdConfig(std::string &config);

    virtual int removeDeviceCredentials();

    virtual int getAliasPath(const std::string &alias, std::string &path);
    virtual int getCcdAppDataPath(std::string &path);
    virtual int getDirectorySeparator(std::string &separator);
    virtual int getDxRemoteRoot(std::string &path);
    virtual int getDxRootPath(std::string &path);
    virtual int getWorkDir(std::string &path);

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
    VPLNet_addr_t ipaddr;  // 0 means unset
    VPLNet_port_t port;    // 0 means unset

    int query();
    bool isQueryNoneInfo();
    bool isCached;
    std::string deviceName;
    std::string deviceClass;
    std::string osVersion;
    bool isAcerDevice;
    bool deviceHasCamera;
    std::set<std::string> opendir_set;

    int InitSocket();
    void FreeSocket();
    int SendProtoSize(uint32_t reqSize);
    int SendProtoRequest(igware::dxshell::DxRemoteFileTransfer &myReq);
    int RecvProtoSize(uint32_t &resSize);
    int RecvProtoResponse(uint32_t resSize, igware::dxshell::DxRemoteFileTransfer &myRes);

    int pushFileGeneric(const std::string &hostPath, const std::string &targetPath,
                        const int timeMs, const int pulseSizeKb);

    int checkIpAddrPort();

    int createDirHelper(const std::string &targetPath, int mode);

    int msaGenericHandler(igware::dxshell::DxRemoteMSA_Function func, const std::string &input, std::string &output);

    VPLSocket_addr_t sockAddr;
    VPLSocket_t socket;
    int reUse;
    int noDelay;
};

#endif // _TARGET_REMOTE_DEVICE_HPP_
