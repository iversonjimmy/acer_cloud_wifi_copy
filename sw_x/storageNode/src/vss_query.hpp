/*
 *  Copyright 2010 iGware Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND 
 *  TRADE SECRETS OF IGWARE INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT 
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
 *
 */

#ifndef STORAGE_NODE__VSS_QUERY_HPP__
#define STORAGE_NODE__VSS_QUERY_HPP__

#include "vpl_th.h"

#include "vplex_vs_directory.h"
#include "generic_cache.hpp"

#include <utility>

class vss_query {
private:
    VPL_DISABLE_COPY_AND_ASSIGN(vss_query);

    VPLMutex_t mutex;

    std::string serverName;
    VPLVsDirectory_ProxyHandle_t proxyHandle;
    vplex::vsDirectory::SessionInfo session;
    std::string sessionSecret;
    u64 userId; // Must be the same as used for session.
    u64 clusterId; // This device's ID.

    typedef struct {
        u64 uid;
        bool isSelfSession;
        char serviceTicket[20];
    } vss_session_data;
    genericCache<u64, vss_session_data> sessionCache;

    genericCache<std::pair<u64, u64>, u64> datasetCache;
    genericCache<std::pair<u64, u64>, vplex::vsDirectory::DatasetDetail> datasetDetailCache;
    genericCache<u64, std::map<u64, bool> > deviceCache;
    std::map<std::pair<u64, u64>, std::string> devSpecTickets;
    std::map<u64, std::string> sessionSecrets;

public:
    vss_query();
    ~vss_query();

    /// Initialize query mechanism.
    /// @param name FQDN of VSDS server. Buffer is copied.
    int init(const char* vsdsHostname, u16 port, u64 clusterId, u64 userId,
             const vplex::vsDirectory::SessionInfo& session);

    /// Register a storage cluster.
    /// @return Zero on success, nonzero on failure.
    /// @param featureVirtDriveCapable indicates whether the storage node can
    ///     support virtual drives.
    /// @param featureMediaServerCapable indicates whether the storage node can
    ///     support serving media.
    /// @param featureFSDatasetTypeCapable indicates whether the storage node can
    ///     support exposing FS dataset.
    /// @note Cluster is named "PersonalStorageNode-<id>", with <id> the
    ///     hex string for @a clusterId using [0-9A-F]"
    int registerStorageNode(bool featureVirtDriveCapable,
                            bool featureMediaServerCapable,
                            bool featureRemoteFileAccessCapable,
                            bool featureFSDatasetTypeCapable,
                            bool featureMyStorageServerCapable);

    /// Link a storage cluster to a user so the user may allocate datasets
    /// at that storage cluster.
    /// @return Zero on success, nonzero on failure.
    /// @notes The added storage is named "PersoalStorageNode-<id>",
    /// with <id> the hex string for @a clusterId using [0-9A-F]"
    int addUserStorageNode();

    /// Add the specified dataset to this cluster.
    /// @param name of dataset.
    /// @param type of dataset.
    /// @return Zero on success, nonzero on failure.
    int addDataSet(const std::string& name, vplex::vsDirectory::DatasetType type);

    /// Find the correct login session for a given request.
    /// Use the session handle of the request to find the session.
    /// @param handle - Session handle for session to find.
    /// @param uid - When session found, session's user ID.
    /// @param serviceTicket - When session found, the service ticket.
    /// @return Zero on success, nonzero on failure.
    int findVssSession(u64 handle, 
                       u64& uid,
                       std::string& serviceTicket);

    /// find out the link state of the specified device ID for the
    /// specified user.
    /// @param uid User ID
    /// @param did Dataset ID
    /// @return bool, true is linked, false is not linked.
    bool isDeviceLinked(u64 uid, u64 device_id);

    int getDeviceSpecificTicket(u64 uid, u64 device_id, std::string& ticket);

    void setSessionSecret(u64 uid, const std::string& secret);
    
    /// Find a given dataset's assigned storage unit(s).
    /// @param uid User ID
    /// @param did Dataset ID
    /// @param cluster on success, the dataset's cluster ID.
    int findDatasetStorage(u64 uid,
                           u64 did,
                           u64& cluster);

    /// Find details about a given dataset.
    /// @param uid User ID
    /// @param did Dataset ID
    /// @param info On success, details about the dataset.
    int findDatasetDetail(u64 uid,
                          u64 did,
                          vplex::vsDirectory::DatasetDetail& detail);

    /// Update storage cluster connectivity information so clients can be told
    /// how to connect.
    int updateConnection(const std::string& hostname,
                         u16 vssi_port,
                         u16 secure_clearfi_port,
                         u16 ts_port);

    /// Update storage cluster feature information so clients can find the
    /// services they need.
    /// @param set_media_server tells whether to modify the state of the
    /// media server enable.
    /// @param enable_media_server enables/disables the media server.
    /// @param set_virt_drive tells whether to modify the state of the
    /// virt drive enable.
    /// @param enable_virt_drive enables/disables the virt drive.
    /// @param set_remote_file_access tells whether to modify the state of the
    /// remote file access enable.
    /// @param enable_remote_file_access enables/disables remote file access.
    int updateFeatures(bool set_media_server,
                       bool enable_media_server,
                       bool set_virt_drive,
                       bool enable_virt_drive,
                       bool set_remote_file_access,
                       bool enable_remote_file_access,
                       bool set_fsdatasettype_support,
                       bool enable_fsdatasettype_support);


    /// Update the dataset's size and version.
    int updateDatasetStats(u64 uid,
                           u64 did,
                           u64 size,
                           u64 version);

    /// Is the network interface on a trusted network?
    /// If so, clients may request lowered security for connections to reduce computation overhead.
    /// If not, all client traffic must use high-security settings.
    bool isTrustedNetwork();
};

#endif // include guard
