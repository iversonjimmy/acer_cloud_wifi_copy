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
#include "vss_query.hpp"

#include <sstream>

#include "vplex_trace.h"
#include "vplex_vs_directory.h"
#include "ccdi.hpp"

#include "cslsha.h"
#include "scopeguard.hpp"
#include "ucf.h"
#include "gvm_misc_utils.h"

using namespace std;

vss_query::vss_query()
{
    VPLMutex_Init(&mutex);
}

vss_query::~vss_query()
{
    VPLVsDirectory_DestroyProxy(proxyHandle);
    VPLMutex_Destroy(&mutex);
}

int vss_query::init(const char* vsdsHostname, u16 port,
                    u64 clusterId, u64 userId,
                    const vplex::vsDirectory::SessionInfo& session)
{
    this->session = session;
    this->userId = userId;
    this->clusterId = clusterId;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      "Creating VS Directory proxy to %s:%d.",
                      vsdsHostname, port);
    serverName = vsdsHostname;

    sessionCache.setExpireAfter(VPLTIME_FROM_SEC(12*60*60));
    sessionCache.setCacheLimit(10); // More than a reasonable number of devices for a single user.

    datasetCache.setExpireAfter(VPLTIME_FROM_SEC(12*60*60));
    datasetCache.setCacheLimit(100); // More than a reasonable number of datasets for a single user.
    
    datasetDetailCache.setExpireAfter(VPLTIME_FROM_SEC(12*60*60));
    datasetDetailCache.setCacheLimit(100); // More than a reasonable number of datasets for a single user.

    // The caching seems really messed up to me. It seem to be multi-user
    // but the above suggests it isn't. It seems undecided.
    deviceCache.setExpireAfter(VPLTIME_FROM_SEC(5*60));
    deviceCache.setCacheLimit(100); // 100 users allowed.

    return VPLVsDirectory_CreateProxy(serverName.c_str(), port, &proxyHandle);
}

int vss_query::registerStorageNode(bool featureVirtDriveCapable,
                                   bool featureMediaServerCapable,
                                   bool featureRemoteFileAccessCapable,
                                   bool featureFsDatasetTypeCapable,
                                   bool featureMyStorageServerCapable)
{
    int rv = 0;

    vplex::vsDirectory::CreatePersonalStorageNodeInput query;
    vplex::vsDirectory::CreatePersonalStorageNodeOutput response;
    vplex::vsDirectory::SessionInfo* query_session = query.mutable_session();
    stringstream namestream;
    string name;

    VPLMutex_Lock(&mutex);

    namestream << "PersonalStorageNode-" << hex << uppercase << clusterId;
    name = namestream.str();

    *query_session = session;
    query.set_userid(userId);
    query.set_clusterid(clusterId);
    query.set_clustername(name);
    query.set_featurevirtdrivecapable(featureVirtDriveCapable);
    query.set_featuremediaservercapable(featureMediaServerCapable);
    query.set_featureremotefileaccesscapable(featureRemoteFileAccessCapable);
    query.set_featurefsdatasettypecapable(featureFsDatasetTypeCapable);
    query.set_featuremystorageservercapable(featureMyStorageServerCapable);

    rv = VPLVsDirectory_CreatePersonalStorageNode(proxyHandle,
                                                  VPLTIME_FROM_SEC(30),
                                                  query, response);
    VPLMutex_Unlock(&mutex);

    // A negative return code does not always indicate a problem here.
    return rv;
}

int vss_query::addUserStorageNode()
{
    int rv = 0;

    bool linked = false;

    VPLMutex_Lock(&mutex);

    // First check to see if the storage is already linked.
    {
        vplex::vsDirectory::ListUserStorageInput query;
        vplex::vsDirectory::ListUserStorageOutput response;
        vplex::vsDirectory::SessionInfo* query_session = query.mutable_session();

        *query_session = session;
        query.set_userid(userId);

        rv = VPLVsDirectory_ListUserStorage(proxyHandle,
                                           VPLTIME_FROM_SEC(30),
                                           query, response);
        if(rv != 0) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "VPLVsDirectory_AddUserStorage() failed with code %d.", rv);
            goto exit;
        }

        for(int i = 0; i < response.storageassignments_size(); i++) {
            if(clusterId == response.storageassignments(i).storageclusterid()) {
                linked = true;
                break;
            }
        }        
    }

    if(!linked) {
        vplex::vsDirectory::AddUserStorageInput query;
        vplex::vsDirectory::AddUserStorageOutput response;
        vplex::vsDirectory::SessionInfo* query_session = query.mutable_session();
        stringstream namestream;
        string name;
        
        namestream << "PersonalStorageNode-" << hex << uppercase << clusterId;
        name = namestream.str();
        
        *query_session = session;
        query.set_userid(userId);
        query.set_storageclusterid(clusterId);
        query.set_storagename(name);
        query.set_usagelimit(0);
        
        rv = VPLVsDirectory_AddUserStorage(proxyHandle,
                                           VPLTIME_FROM_SEC(30),
                                           query, response);
        if(rv != 0) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "VPLVsDirectory_AddUserStorage() failed with code %d.", rv);
            return rv;
        }
    }

 exit:
    VPLMutex_Unlock(&mutex);

    return rv;
}

int vss_query::addDataSet(const std::string& name,
                          vplex::vsDirectory::DatasetType type)
{
    int rv = 0;

    vplex::vsDirectory::AddDataSetInput query;
    vplex::vsDirectory::AddDataSetOutput response;
    vplex::vsDirectory::SessionInfo* query_session = query.mutable_session();
    *query_session = session;
    query.set_userid(userId);
    query.set_datasetname(name);
    query.set_datasettypeid(type);
    query.set_storageclusterid(clusterId);

    rv = VPLVsDirectory_AddDataSet(proxyHandle,
                                       VPLTIME_FROM_SEC(30),
                                       query, response);

    // VSDS returns a generic fail if the dataset already exists.
    // The cache at this level works off of uid/did pairs and only
    // caches datasets it knows about. So, the cache is of no use to
    // us because we don't know the did. Until we sort this out, assume
    // that the -322200 means it already exists
    if((rv != 0) && (rv != -32200)) {

        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "VPLVsDirectory_AddDataSet(%s, %d) failed with code %d.", 
                         name.c_str(), type, rv);
        return rv;
    }
    rv = 0;

    return rv;
}

int vss_query::findVssSession(u64 handle, 
                              u64& uid,
                              std::string& serviceTicket)
{
    int rv = 0;
    vss_session_data data;
    vplex::vsDirectory::GetLoginSessionInput query;
    vplex::vsDirectory::GetLoginSessionOutput response;
    vplex::vsDirectory::SessionInfo* query_session = query.mutable_session();

    VPLMutex_Lock(&mutex);

    if(sessionCache.getData(handle, data)) {
        uid = data.uid;
        serviceTicket.assign(data.serviceTicket, 20);
        goto exit;
    }
    
    *query_session = session;
    query.set_userid(userId);
    query.set_deviceid(clusterId);
    query.set_sessionhandle(handle);

    rv = VPLVsDirectory_GetLoginSession(proxyHandle,
                                        VPLTIME_FROM_SEC(30),
                                        query, response);

    if(rv != 0) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                          "VPLVsDirectory_GetLoginSession() failed with code %d.", rv);
    }
    else {
        serviceTicket = response.serviceticket();
        uid = response.userid();

        data.uid = uid;
        memcpy(data.serviceTicket, serviceTicket.data(), 20);
        sessionCache.putData(handle, data);
    }

 exit:
    VPLMutex_Unlock(&mutex);

    return rv;
}

bool vss_query::isDeviceLinked(u64 uid, u64 device_id)
{
    bool is_linked = false;
    std::map<u64, bool> devMap;

    VPLTRACE_LOG_FINE(TRACE_BVS, 0, "Device check for "FMTu64" "FMTu64,
        uid, device_id);

    VPLMutex_Lock(&mutex);
    bool cacheHit = deviceCache.getData(uid, devMap);

    if(!cacheHit || (cacheHit && devMap.find(device_id) == devMap.end())) {
        // update the cache
        int rv = 0;
        ccd::ListLinkedDevicesInput request;
        ccd::ListLinkedDevicesOutput response;
        request.set_user_id(uid);
        request.set_only_use_cache(true);

        rv = CCDIListLinkedDevices(request, response);
        if(rv != CCD_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "CCDIListLinkedDevices() failed "
                "with code %d.", rv);
            goto exit;
        }

        for(int i = 0; i < response.devices_size(); i++) {
            VPLTRACE_LOG_FINE(TRACE_BVS, 0, "user "FMTu64" has dev "FMTu64,
                uid, response.devices(i).device_id());
            devMap[response.devices(i).device_id()] = true;
        }
        if (cacheHit) {
            deviceCache.removeData(uid);
        }
        deviceCache.putData(uid, devMap);
    }

    if ( devMap.find(device_id) != devMap.end() ) {
        is_linked = true;
    }

 exit:
    VPLMutex_Unlock(&mutex);

    VPLTRACE_LOG_FINE(TRACE_BVS, 0, "Device check for "FMTu64" "FMTu64 " is %s",
        uid, device_id, is_linked ? "linked" : "not linked");

    return is_linked;
}

// Putting this here because it's roughly related.
int vss_query::getDeviceSpecificTicket(u64 uid, u64 did,
                                       std::string& service_ticket)
{
    int rv = 0;
    string secret;

    VPLTRACE_LOG_FINE(TRACE_BVS, 0, "ticket for "FMTu64" "FMTu64,
        uid, did);

    service_ticket.erase();

    VPLMutex_Lock(&mutex);
    if ( devSpecTickets.find(make_pair(uid, did)) != devSpecTickets.end() ) {
        service_ticket = devSpecTickets[make_pair(uid, did)];
        goto exit;
    }

    if ( sessionSecrets.find(uid) == sessionSecrets.end() ) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "no session secret for "FMTu64, uid);
        rv = -1;
        goto exit;
    }
    secret = sessionSecrets[uid];

    // generate the ticket and add it to the cache.
    {
        char* secret64 = NULL;
        size_t secret64len = 0;
        UCFBuffer ticket;
        CSL_ShaContext context;
        char buf[40];
        const char *serviceId = "Virtual Storage";

        Util_EncodeBase64(secret.c_str(), secret.size(),
                          &secret64, &secret64len, VPL_FALSE, VPL_FALSE);
        ON_BLOCK_EXIT(free, secret64);

        memset(&ticket, 0, sizeof(ticket));

        CSL_ResetSha(&context);
        CSL_InputSha(&context, secret64, secret64len);
        CSL_InputSha(&context, serviceId, strlen(serviceId));

        snprintf(buf, sizeof(buf), FMTu64, did);
        CSL_InputSha(&context, buf, strlen(buf));

        ticket.size = UCF_SESSION_SECRET_LENGTH;
#if (UCF_SESSION_SECRET_LENGTH != CSL_SHA1_DIGESTSIZE)
#  error "Compile-time check failed!"
#endif
        ticket.data = (char*)malloc(ticket.size);
        CSL_ResultSha(&context, (u8*)ticket.data);
        service_ticket.assign(ticket.data, ticket.size);
        free(ticket.data);

        devSpecTickets[make_pair(uid, did)] = service_ticket;
    }

 exit:
    VPLMutex_Unlock(&mutex);

    VPLTRACE_LOG_FINE(TRACE_BVS, 0, "ticket for "FMTu64" "FMTu64" - %d",
        uid, did, rv);

    return rv;
}

// Putting this here because it's roughly related.
void vss_query::setSessionSecret(u64 uid, const std::string& secret)
{
    VPLTRACE_LOG_FINE(TRACE_BVS, 0, "set session secret for "FMTu64, uid);

    VPLMutex_Lock(&mutex);
    sessionSecrets[uid] = secret;
    VPLMutex_Unlock(&mutex);
}

int vss_query::findDatasetStorage(u64 uid,
                                  u64 did,
                                  u64& cluster)
{
    int rv = 0;
    ccd::ListOwnedDatasetsInput request;
    ccd::ListOwnedDatasetsOutput response;

    // Default values meaning no assignments known.
    cluster = 0;

    VPLMutex_Lock(&mutex);
    bool cacheHit = datasetCache.getData(make_pair(uid, did), cluster);
    VPLMutex_Unlock(&mutex);
    if(cacheHit) {
        goto exit;
    }

    request.set_user_id(uid);
    request.set_only_use_cache(true);

    rv = CCDIListOwnedDatasets(request, response);
    if(rv != CCD_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "CCDIListOwnedDatasets() failed with code %d.", rv);
    }
    else {
        for(int i = 0; i < response.dataset_details_size(); i++) {
            if(response.dataset_details(i).has_clusterid()) {
                if(response.dataset_details(i).datasetid() == did) {
                    cluster = response.dataset_details(i).clusterid();

                    VPLMutex_Lock(&mutex);
                    datasetCache.putData(make_pair(uid, did), cluster);
                    VPLMutex_Unlock(&mutex);

                    break;
                }
            }
        }
    }

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      "Dataset "FMTu64":"FMTu64" is in cluster "FMTu64".",
                      uid, did, cluster);

 exit:
    return rv;
}

int vss_query::findDatasetDetail(u64 uid,
                                 u64 did,
                                 vplex::vsDirectory::DatasetDetail& detail)
{
    int rv = 0;
    ccd::ListOwnedDatasetsInput request;
    ccd::ListOwnedDatasetsOutput response;

    // Default values meaning no assignments known.
    //detail.set_clusterid(0);

    VPLMutex_Lock(&mutex);
    bool cacheHit = datasetDetailCache.getData(make_pair(uid, did), detail);
    VPLMutex_Unlock(&mutex);
    if(cacheHit) {
        goto exit;
    }

    request.set_user_id(uid);
    request.set_only_use_cache(true);

    rv = CCDIListOwnedDatasets(request, response);
    if(rv != CCD_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "CCDIListOwnedDatasets() failed with code %d.", rv);
    } 
    else {
        for(int i = 0; i < response.dataset_details_size(); i++) {
            if(response.dataset_details(i).has_clusterid()) {
                if(response.dataset_details(i).datasetid() == did) {
                    detail = response.dataset_details(i);

                    VPLMutex_Lock(&mutex);
                    datasetDetailCache.putData(make_pair(uid, did), detail);
                    VPLMutex_Unlock(&mutex);

                    break;
                }
            }
        }
    }

#if 0
    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      "Dataset "FMTu64":"FMTu64" is %s with type %d in cluster "FMTu64".",
                      uid, did, detail.datasetname().c_str(), detail.datasettype(), detail.clusterid());
#endif

 exit:
    return rv;
}

int vss_query::updateConnection(const std::string& hostname,
                                u16 vssi_port,
                                u16 secure_clearfi_port,
                                u16 ts_port)
{
    int rv = 0;

    vplex::vsDirectory::UpdateStorageNodeConnectionInput query;
    vplex::vsDirectory::UpdateStorageNodeConnectionOutput response;
    vplex::vsDirectory::SessionInfo* query_session = query.mutable_session();

    VPLMutex_Lock(&mutex);

    *query_session = session;
    query.set_userid(userId);
    query.set_clusterid(clusterId);
    query.set_reportedname(hostname);
    query.set_reportedport(vssi_port);
    query.set_reportedhttpport(0); // unused, but required.
    query.set_reportedclearfiport(ts_port); // repurposed
    query.set_reportedclearfisecureport(secure_clearfi_port);
    query.set_accesshandle(session.sessionhandle());
    query.set_accessticket(session.serviceticket());

    rv = VPLVsDirectory_UpdateStorageNodeConnection(proxyHandle,
                                                    VPLTIME_FROM_SEC(30),
                                                    query, response);
    if(rv != 0) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                          "VPLVsDirectory_UpdateStorageNodeConnection() failed with code %d.", rv);
    }
    else {
        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                            "Updated connection info for cluster "FMTx64", host %s, vssi_port %u, secure_clearfi_port %u, ts_port %u",
                            clusterId, hostname.c_str(), vssi_port, secure_clearfi_port, ts_port);
    }

    VPLMutex_Unlock(&mutex);

    return rv;
}

int vss_query::updateFeatures(bool set_media_server,
                              bool enable_media_server,
                              bool set_virt_drive,
                              bool enable_virt_drive,
                              bool set_remote_file_access,
                              bool enable_remote_file_access,
                              bool set_fsdatasettype_support,
                              bool enable_fsdatasettype_support)
{
    int rv = 0;

    vplex::vsDirectory::UpdateStorageNodeFeaturesInput query;
    vplex::vsDirectory::UpdateStorageNodeFeaturesOutput response;
    vplex::vsDirectory::SessionInfo* query_session = query.mutable_session();

    *query_session = session;
    query.set_userid(userId);
    query.set_clusterid(clusterId);
    if ( set_media_server ) {
        query.set_featuremediaserverenabled(enable_media_server);
    }
    if ( set_virt_drive ) {
        query.set_featurevirtdriveenabled(enable_virt_drive);
    }
    if ( set_remote_file_access ) {
        query.set_featureremotefileaccessenabled(enable_remote_file_access);
    }
    if ( set_fsdatasettype_support ) {
        query.set_featurefsdatasettypeenabled(enable_fsdatasettype_support);
    }

    rv = VPLVsDirectory_UpdateStorageNodeFeatures(proxyHandle,
                                                  VPLTIME_FROM_SEC(30),
                                                  query, response);
    if(rv != 0) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                          "VPLVsDirectory_UpdateStorageNodeFeatures() failed with code %d.", rv);
    }
    else {
        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                            "Updated feature info for cluster "FMTx64", MediaServerEnabled: %s VirtDriveEnabled: %s RemoteFileAccessEnabled: %s FSDatasetTypeEnabled: %s",
                            clusterId,
                            (enable_media_server) ? "true" : "false",
                            (enable_virt_drive) ? "true" : "false",
                            (enable_remote_file_access) ? "true" : "false",
                            (enable_fsdatasettype_support) ? "true" : "false");
    }
    return rv;
}

int vss_query::updateDatasetStats(u64 uid,
                                  u64 did,
                                  u64 size,
                                  u64 version)
{
    int rv = 0;

    vplex::vsDirectory::UpdatePSNDatasetStatusInput query;
    vplex::vsDirectory::UpdatePSNDatasetStatusOutput response;
    vplex::vsDirectory::SessionInfo* query_session = query.mutable_session();

    *query_session = session;
    query.set_userid(userId);
    query.set_clusterid(clusterId);
    query.set_datasetuserid(uid);
    query.set_datasetid(did);
    query.set_datasetsize(size);
    query.set_datasetversion(version);

    rv = VPLVsDirectory_UpdatePSNDatasetStatus(proxyHandle,
                                               VPLTIME_FROM_SEC(30),
                                               query, response);
    if(rv != 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "VPLVsDirectory_UpdatePSNDatasetStatus() failed with code %d.", rv);
    }

    return rv;
}

bool vss_query::isTrustedNetwork()
{
    bool rv = false;

    // TODO: Get trusted network status from CCD.
    // FORNOW: Orbe trusts networks. Non-Orbe does not.
#ifdef CLOUDNODE
    rv = true;
#endif

    return rv;
}
