//
//  Copyright 2014 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER CLOUD
//  TECHNOLOGY INC.
//

#include "HttpSvc_Sn_Handler_rf.hpp"

#include "HttpSvc_Sn_MediaDsFile.hpp"
#include "HttpSvc_Sn_MediaFileSender.hpp"

#include "dataset.hpp"
#include "vss_server.hpp"

#include <ccdi.hpp>
#include <ccdi_orig_types.h>

#include <cJSON2.h>
#include <gvm_errors.h>
#include <HttpStream.hpp>
#include <HttpStream_Helper.hpp>
#include <log.h>

#include <scopeguard.hpp>
#include <vpl_conv.h>
#include <vplex_assert.h>
#include <vplex_http_util.hpp>
#include <vplu_mutex_autolock.hpp>
#include <vpl_lazy_init.h>

#include <cassert>
#include <sstream>
#include <string>

// SN_RF_REVERSE_COPY_MOVE_DIR - determines the direction of copy/move operation.
// 0: implementation agrees with spec (URI is the destination and the source is specified in the query).
// Otherwise: the direction is the opposite, and agrees with the implementation in strm_http.cpp.
// Bug 11696
#define SN_RF_REVERSE_COPY_MOVE_DIR 1

namespace HttpSvc {
    namespace Sn {
        namespace Handler_rf_Helper {
            class AliasMap {
            public:
                AliasMap();
                typedef std::map<std::string, std::string>::const_iterator const_iterator;
                const_iterator find(const std::string &key) const {
                    return m.find(key);
                }
                const_iterator end() const {
                    return m.end();
                }
            private:
                std::map<std::string, std::string> m;
            };

            class SyncboxAliasMap {
            public:
                SyncboxAliasMap();
                typedef std::map<std::string, std::string>::const_iterator const_iterator;
                const_iterator find(const std::string &key) const {
                    return m.find(key);
                }
                const_iterator end() const {
                    return m.end();
                }
            private:
                std::map<std::string, std::string> m;
            };

            VPLLazyInitMutex_t syncboxParamMutex = VPLLAZYINITMUTEX_INIT;
            /// Protected by syncboxParamMutex.
            //@{
            bool isSyncboxArchiveStorage;
            u64 syncboxDatasetId;
            std::string syncboxSyncFeaturePath;
            std::string syncboxStagingAreaPath;
            //@}

            inline const std::string getSyncboxArchiveStorageStagingAreaAlias(u64 datasetId) {
                std::ostringstream oss;
                oss << "stagingArea:" << datasetId;
                return oss.str();
            }

            inline const std::string getSyncboxRFAppSyncFolderAlias(u64 datasetId) {
                std::ostringstream oss;
                oss << "syncbox:" << datasetId;
                return oss.str();
            }


            void tryResolveAlias(std::string &path_in_out);
            bool isSyncboxAlias(const std::string& path);

            class PathValidator {
            public:
                struct Patterns {
                    std::set<std::string> exact_static;
                    std::set<std::string> prefix_static;
                    std::set<std::string> prefix_dynamic;
                };

                PathValidator(void (*initializer)(Patterns &patterns),
                              void (*refresher)(Patterns &patterns));
                bool IsValidPath(const std::string &path);
            private:
                void (*initializer)(Patterns &);
                void (*refresher)(Patterns &);
                Patterns patterns;
                static bool isValidPath_byExactMatch(const std::string &path, const std::set<std::string> &patterns);
                static bool isValidPath_byPrefixMatch(const std::string &path, const std::set<std::string> &patterns);
            };

            static PathValidator *mediaRfPathValidator_default = NULL;
            static PathValidator *mediaRfPathValidator_getDir = NULL;

            void PathValidator_MediaRfPatternInitializer_Default(PathValidator::Patterns &patterns);
            void PathValidator_MediaRfPatternInitializer_GetDir(PathValidator::Patterns &patterns);
            void PathValidator_MediaRfPatternRefresher_Default(PathValidator::Patterns &patterns);

            // Creation of PathValidator objects must be deferred until Windows user ID (SID) is known in VPL.
            // PathValidatorCreateHelper object coordinates the creation.
            // THIS IS MEANT TO BE A SINGLETON.
            class PathValidatorCreateHelper {
            public:
                PathValidatorCreateHelper() {
                    VPLMutex_Init(&mutex_mediaRf_default);
                    VPLMutex_Init(&mutex_mediaRf_getDir);
                }
                ~PathValidatorCreateHelper() {
                    if (mediaRfPathValidator_default) {
                        delete mediaRfPathValidator_default;
                    }
                    if (mediaRfPathValidator_getDir) {
                        delete mediaRfPathValidator_getDir;
                    }
                    VPLMutex_Destroy(&mutex_mediaRf_getDir);
                    VPLMutex_Destroy(&mutex_mediaRf_default);
                }
                int CreateIfNecessary_MediaRf_default() {
                    MutexAutoLock lock(&mutex_mediaRf_default);
                    if (!mediaRfPathValidator_default) {
                        mediaRfPathValidator_default = new (std::nothrow) PathValidator(PathValidator_MediaRfPatternInitializer_Default, 
                                                                                        PathValidator_MediaRfPatternRefresher_Default);
                        if (!mediaRfPathValidator_default) {
                            LOG_INFO("Failed to create PathValidator obj");
                            return TS_ERR_NO_MEM;
                        }
                    }
                    assert(mediaRfPathValidator_default);
                    return 0;
                }
                int CreateIfNecessary_MediaRf_getDir() {
                    MutexAutoLock lock(&mutex_mediaRf_getDir);
                    if (!mediaRfPathValidator_getDir) {
                        mediaRfPathValidator_getDir = new (std::nothrow) PathValidator(PathValidator_MediaRfPatternInitializer_GetDir, 
                                                                                       PathValidator_MediaRfPatternRefresher_Default);
                        if (!mediaRfPathValidator_getDir) {
                            LOG_INFO("Failed to create PathValidator obj");
                            return TS_ERR_NO_MEM;
                        }
                    }
                    assert(mediaRfPathValidator_getDir);
                    return 0;
                }
            private:
                VPLMutex_t mutex_mediaRf_default;
                VPLMutex_t mutex_mediaRf_getDir;
            };
            static PathValidatorCreateHelper pathValidatorCreateHelper;

            // E.g., C:/abc/def -> Computer/C/abc/def
            std::string normalizeWinPath(const char *path);

            // Return the parent directory.  If no parent, the root.
            std::string getParentPath(const std::string &path);
            void getParentPath(const std::string &path, std::string &parentDir);

            // Return "tail" in path.  If none, return empty string.
            std::string getFileNameFromPath(const std::string &path);
            void getFileNameFromPath(const std::string &path, std::string &fileName);

            struct EnabledFeatures {
                bool mediaServer;
                bool remoteFileAccess;
            };
            int getEnabledFeatures(u64 userId, u64 deviceId, EnabledFeatures &enabledFeatures);

            int checkPathPermission(dataset *ds, const std::string &path, bool mustExist, u32 accessMask, VPLFS_file_type_t expectedType, bool checkType, HttpStream *hs);
        }
    }
}

namespace HttpSvc {
    namespace Sn {
        namespace Handler_rf_Helper {
            VPLMutex_t AccessControlLocker::mutex;
            std::set<std::string> blackList;
            std::set<std::string> whiteList;
            std::set<std::string> userWhiteList;
            std::multimap<std::string,ccd::RemoteFileAccessControlDirSpec> acDirs;

            // Initialized by sw_x/storageNode/src/vss_server.cpp :: vss_server::start()
            // Same lifetime as vss_server.
            RemoteFileSearchManager* rfSearchManager = NULL;
        }
    }
}

HttpSvc::Sn::Handler_rf::Handler_rf(HttpStream *hs)
    : Handler(hs), datasetId(0), ds(NULL)
{
    LOG_INFO("Handler_rf[%p]: Created for HttpStream[%p]", this, hs);
}

HttpSvc::Sn::Handler_rf::~Handler_rf()
{
    if (ds) {
        ds->release();
    }

    LOG_INFO("Handler_rf[%p]: Destroyed", this);
}

int HttpSvc::Sn::Handler_rf::Run()
{
    LOG_INFO("Handler_rf[%p]: Run", this);

    const std::string &uri = hs->GetUri();
    // URI has the format: /rf/<objtype>/<datasetid>/<path>
    // Note that <path> does not include the literal '/'; to show hierarchy, '/' must be escaped.

    VPLHttp_SplitUri(uri, uri_tokens);
    if (uri_tokens.size() < 3) {
        LOG_ERROR("Handler_rf[%p]: Unexpected number of segments; uri %s", this, uri.c_str());
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Unexpected number of segments; uri " << uri << "\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return 0;
    }
    const std::string &uri_namespace = uri_tokens[0];
    const std::string &objtype = uri_tokens[1];
    datasetId = VPLConv_strToU64(uri_tokens[2].c_str(), NULL, 10);

    // Verify number of uri_tokens.  With the exception of whitelist, all
    // other rf API has 4 uri_tokens.
    if ((objtype != "whitelist" && uri_tokens.size() != 4) ||
        (objtype == "whitelist" && uri_tokens.size() == 3))
    {
        LOG_ERROR("Handler_rf[%p]: Unexpected number of segments; uri %s", this, uri.c_str());
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Unexpected number of segments; uri " << uri << "\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return 0;
    }

    int err = 0;
    Handler_rf_Helper::EnabledFeatures enabledFeatures;
    err = Handler_rf_Helper::getEnabledFeatures(hs->GetUserId(), vssServer->clusterId, enabledFeatures);
    if (err) {
        LOG_ERROR("Handler_rf[%p]: Failed to determine enabled features; err %d", this, err);
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Failed to determine enabled features\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 500, oss.str(), "application/json");
        return 0;  // reset error
    }

    // Initialize syncbox related variables used by Handler_rf_Helper functions
    {
        u64 syncboxDsetId;
        MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&Handler_rf_Helper::syncboxParamMutex));
        if (vssServer->getSyncboxSyncFeatureInfo(syncboxDsetId, Handler_rf_Helper::syncboxSyncFeaturePath) == true) {
            Handler_rf_Helper::syncboxDatasetId             = vssServer->getSyncboxArchiveStorageDatasetId();
            Handler_rf_Helper::isSyncboxArchiveStorage      = true;
            Handler_rf_Helper::syncboxStagingAreaPath       = vssServer->getSyncboxArchiveStorageStagingAreaPath(Handler_rf_Helper::syncboxDatasetId);    
        } else {
            Handler_rf_Helper::isSyncboxArchiveStorage      = false;
        }
    }

    if (uri_namespace == "rf") {
        // Test if this is a syncbox folder access. If so, bypass remoteFileAccess feature enable check
        bool isSyncboxAccess = false; 
        if (Handler_rf_Helper::isSyncboxArchiveStorage) { 
            std::string temp;
            err = VPLHttp_DecodeUri(uri_tokens[3], temp);
            if (err) {
                LOG_ERROR("Handler_rf[%p]: Failed to decode; segment %s, err %d", this, uri_tokens[3].c_str(), err);
                std::ostringstream oss;
                oss << "{\"errMsg\":\"Failed to decode; segment " << uri_tokens[3] << "\"}";
                HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
                return 0;  // reset error
            }
            isSyncboxAccess = Handler_rf_Helper::isSyncboxAlias(temp);
        }

        if (!enabledFeatures.remoteFileAccess && !isSyncboxAccess) {
            LOG_ERROR("Handler_rf[%p]: Request rejected; Remote File Access is disabled. isSyncboxAccess(%d) isSyncboxArchiveStorage(%d)", this, (int)isSyncboxAccess, (int)Handler_rf_Helper::isSyncboxArchiveStorage);
            std::ostringstream oss;
            oss << "{\"errMsg\":\"Remote File Access is disabled\"}";
            HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
            return 0;  // reset error
        }
    } else if (uri_namespace == "media_rf"){
        if (!enabledFeatures.mediaServer) {
            LOG_ERROR("Handler_rf[%p]: Request rejected; Media Server is disabled", this);
            std::ostringstream oss;
            oss << "{\"errMsg\":\"Media Server is disabled\"}";
            HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
            return 0;  // reset error
        }
    } else {
        // Bad table jump to this method.
        FAILED_ASSERT("Unrecognized namespace(%s). Expecting \"rf\" or \"media_rf\"",
                      uri_namespace.c_str());
    }

    err = vssServer->getDataset(hs->GetUserId(), datasetId, ds);
    if (err) {
        LOG_ERROR("Handler_rf[%p]: Failed to find dataset; dataset "FMTu64", err %d", this, datasetId, err);
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Failed to find dataset; dataset " << datasetId << "\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return 0;
    }

    if(objtype == "whitelist") {
        // Does not have 4th argument
    } else if (objtype == "search") {
        // See http://wiki.ctbg.acer.com/wiki/index.php/Remote_File_Filename_Search_Design
        search_func = uri_tokens[3];  // "begin", "get", or "end".
        // Search API also has queryKeyValue pairs (www.example.com?searchQueryId=1&startIndex=2...)
        VPLHttp_SplitUriQueryParams(uri, uriQueryArgs);
    } else {
        err = VPLHttp_DecodeUri(uri_tokens[3], path);
        if (err) {
            LOG_ERROR("Handler_rf[%p]: Failed to decode; segment %s, err %d", this, uri_tokens[3].c_str(), err);
            std::ostringstream oss;
            oss << "{\"errMsg\":\"Failed to decode; segment " << uri_tokens[3] << "\"}";
            HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
            return 0;  // reset error
        }
        Handler_rf_Helper::tryResolveAlias(/*IN_OUT*/ path);
    }

    JumpTableMap::const_iterator it = jumpTableMap_byObj.find(objtype);
    if (it == jumpTableMap_byObj.end()) {
        LOG_ERROR("Handler_rf[%p]: Unexpected objtype %s", this, objtype.c_str());
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Unexpected objtype " << objtype << "\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return 0;
    }
    return (this->*it->second)();
    // response set by subhandler
}

int HttpSvc::Sn::Handler_rf::handle_dir()
{
    const std::string &method = hs->GetMethod();

    JumpTableMap::const_iterator it = jumpTableMap_dir_byMethod.find(method);
    if (it == jumpTableMap_dir_byMethod.end()) {
        LOG_ERROR("Handler_rf[%p]: Unexpected method %s", this, method.c_str());
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Unexpected method " << method << "\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 405, oss.str(), "application/json");  // 405 = Method Not Allowed
        return 0;
    }

    return (this->*it->second)();
    // response set by subhandler
}

int HttpSvc::Sn::Handler_rf::handle_file()
{
    const std::string &method = hs->GetMethod();

    JumpTableMap::const_iterator it = jumpTableMap_file_byMethod.find(method);
    if (it == jumpTableMap_file_byMethod.end()) {
        LOG_ERROR("Handler_rf[%p]: Unexpected method %s", this, method.c_str());
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Unexpected method " << method << "\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 405, oss.str(), "application/json");  // 405 = Method Not Allowed
        return 0;
    }

    return (this->*it->second)();
    // response set by subhandler
}

int HttpSvc::Sn::Handler_rf::handle_filemetadata()
{
    const std::string &method = hs->GetMethod();

    JumpTableMap::const_iterator it = jumpTableMap_filemetadata_byMethod.find(method);
    if (it == jumpTableMap_filemetadata_byMethod.end()) {
        LOG_ERROR("Handler_rf[%p]: Unexpected method %s", this, method.c_str());
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Unexpected method " << method << "\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 405, oss.str(), "application/json");  // 405 = Method Not Allowed
        return 0;
    }

    return (this->*it->second)();
    // response set by subhandler
}

int HttpSvc::Sn::Handler_rf::handle_whitelist()
{
    const std::string &method = hs->GetMethod();

    JumpTableMap::const_iterator it = jumpTableMap_whitelist_byMethod.find(method);
    if (it == jumpTableMap_whitelist_byMethod.end()) {
        LOG_ERROR("Handler_rf[%p]: Unexpected method %s", this, method.c_str());
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Unexpected method " << method << "\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 405, oss.str(), "application/json");  // 405 = Method Not Allowed
        return 0;
    }

    return (this->*it->second)();
    // response set by subhandler
}

int HttpSvc::Sn::Handler_rf::handle_search()
{
    const std::string searchFunc = uri_tokens[3];
    int rv;
    if (searchFunc == "begin") {
        rv = handle_search_POST_begin();
        if (rv != 0) {
            LOG_ERROR("Handler_rf[%p]:handle_search_POST_begin:%d", this, rv);
        }
    } else if (searchFunc == "get") {
        rv = handle_search_POST_get();
        if (rv != 0) {
            LOG_ERROR("Handler_rf[%p]:handle_search_POST_get:%d", this, rv);
        }
    } else if (searchFunc == "end") {
        rv = handle_search_POST_end();
        if (rv != 0) {
            LOG_ERROR("Handler_rf[%p]:handle_search_POST_end:%d", this, rv);
        }
    } else {
        LOG_ERROR("Handler_rf[%p]:searchFunc(%s) unrecognized", this, searchFunc.c_str());
    }

    return 0;
}

int HttpSvc::Sn::Handler_rf::handle_dirmetadata()
{
    const std::string &method = hs->GetMethod();

    JumpTableMap::const_iterator it = jumpTableMap_dirmetadata_byMethod.find(method);
    if (it == jumpTableMap_dirmetadata_byMethod.end()) {
        LOG_ERROR("Handler_rf[%p]: Unexpected method %s", this, method.c_str());
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Unexpected method " << method << "\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 405, oss.str(), "application/json");  // 405 = Method Not Allowed
        return 0;
    }

    return (this->*it->second)();
    // response set by subhandler
}

int HttpSvc::Sn::Handler_rf::handle_dir_DELETE()
{
    int err = 0;

    err = Handler_rf_Helper::pathValidatorCreateHelper.CreateIfNecessary_MediaRf_default();
    if (err) {
        // err msg logged by CreateIfNecessary_MediaRf_default()
        return err;
    }

    if (uri_tokens[0] == "media_rf" && !Handler_rf_Helper::mediaRfPathValidator_default->IsValidPath(path)) {
        LOG_INFO("Handler_rf[%p]: No access via media_rf; path %s", this, path.c_str());
        std::ostringstream oss;
        oss << "{\"errMsg\":\"No access via media_rf; path " << path << "\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return 0;
    }

    {
        std::string parentPath;
        Handler_rf_Helper::getParentPath(path, parentPath);
        err = Handler_rf_Helper::checkPathPermission(ds, parentPath, /*mustExist*/true, VPLFILE_CHECK_PERMISSION_READ, VPLFS_TYPE_DIR, /*checkType*/true, hs);
        if (err) {
            // Error message logged and HTTP response set by checkPathPermission()
            return 0;  // reset error
        }
    }
    err = Handler_rf_Helper::checkPathPermission(ds, path, /*mustExist*/true, VPLFILE_CHECK_PERMISSION_DELETE, VPLFS_TYPE_DIR, /*checkType*/true, hs);
    if (err) {
        // Error message logged and HTTP response set by checkPathPermission()
        return 0;  // reset error
    }

    err = ds->remove_iw(path);
    if (!err) {
        err = ds->commit_iw();
    }
    if (err) {
        LOG_ERROR("Handler_rf[%p]: Failed to delete directory; dataset "FMTu64", path %s, err %d", this, ds->get_id().did, path.c_str(), err);
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Failed to delete directory; dataset " << ds->get_id().did << ", path " << path << "\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 500, oss.str(), "application/json");
        return 0;  // reset error
    }

    HttpStream_Helper::SetCompleteResponse(hs, 200);

    return 0;
}

int HttpSvc::Sn::Handler_rf::handle_dir_GET()
{
    int err = 0;

    err = Handler_rf_Helper::pathValidatorCreateHelper.CreateIfNecessary_MediaRf_getDir();
    if (err) {
        // err msg logged by CreateIfNecessary_MediaRf_getDir()
        return err;
    }

    if (uri_tokens[0] == "media_rf" && !Handler_rf_Helper::mediaRfPathValidator_getDir->IsValidPath(path)) {
        LOG_INFO("Handler_rf[%p]: No access via media_rf; path %s", this, path.c_str());
        std::ostringstream oss;
        oss << "{\"errMsg\":\"No access via media_rf; path " << path << "\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return 0;
    }

    err = Handler_rf_Helper::checkPathPermission(ds, path, /*mustExist*/true, VPLFILE_CHECK_PERMISSION_READ, VPLFS_TYPE_DIR, /*checkType*/true, hs);
    if (err) {
        // Error message logged and HTTP response set by checkPathPermission()
        return 0;  // reset error
    }

    struct Params {
        u32 index;
        u32 _max;
        std::string sortBy;
        Params() : index(1), _max(UINT_MAX), sortBy("time") {}
    };
    Params params;

    do {
        std::string q_index;
        err = hs->GetQuery("index", q_index);
        if (err == CCD_ERROR_NOT_FOUND) {
            // acceptable outcome - silently ignore
            err = 0;  // reset error
            break;
        }
        if (err) {
            LOG_WARN("Handler_rf[%p]: Unexpected error ignored: err %d", this, err);
            err = 0;  // reset error
            break;
        }
        params.index = atoi(q_index.c_str());
    } while (0);

    do {
        std::string q_max;
        err = hs->GetQuery("max", q_max);
        if (err == CCD_ERROR_NOT_FOUND) {
            // acceptable outcome - silently ignore
            err = 0;  // reset error
            break;
        }
        if (err) {
            LOG_WARN("Handler_rf[%p]: Unexpected error ignored: err %d", this, err);
            err = 0;  // reset error
            break;
        }
        params._max = atoi(q_max.c_str());
    } while (0);

    do {
        std::string q_sortBy;
        err = hs->GetQuery("sortBy", q_sortBy);
        if (err == CCD_ERROR_NOT_FOUND) {
            // acceptable outcome - silently ignore
            err = 0;  // reset error
            break;
        }
        if (err) {
            LOG_WARN("Handler_rf[%p]: Unexpected error ignored: err %d", this, err);
            err = 0;  // reset error
            break;
        }
        if (q_sortBy.empty()) {
            // unusual but silently ignore to keep the default
            break;
        }
        params.sortBy = q_sortBy;
    } while (0);

    std::string response;
    err = ds->read_dir2(path, response, /*json*/true, /*pagination*/true, params.sortBy, params.index, params._max);
    if (err) {
        LOG_ERROR("Handler_rf[%p]: Failed to read directory; dataset "FMTu64", path %s, err %d", this, ds->get_id().did, path.c_str(), err);
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Failed to read directory; dataset " << ds->get_id().did << ", path " << path << "\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 404, oss.str(), "application/json");
        return 0;
    }

    HttpStream_Helper::SetCompleteResponse(hs, 200, response, "application/json");

    return 0;
}

int HttpSvc::Sn::Handler_rf::handle_dir_POST()
{
    int err = 0;

    std::string copyFrom;
    bool copyFrom_found = false;
    err = hs->GetQuery("copyFrom", copyFrom);
    if (err == CCD_ERROR_NOT_FOUND) {
        // acceptable outcome - silently ignore
        err = 0;  // reset error
    }
    else if (err) {
        LOG_WARN("Handler_rf[%p]: Unexpected error ignored: err %d", this, err);
        err = 0;  // reset error
    }
    else {
        copyFrom_found = true;
    }

    std::string moveFrom;
    bool moveFrom_found = false;
    err = hs->GetQuery("moveFrom", moveFrom);
    if (err == CCD_ERROR_NOT_FOUND) {
        // acceptable outcome - silently ignore
        err = 0;  // reset error
    }
    else if (err) {
        LOG_WARN("Handler_rf[%p]: Unexpected error ignored; err %d", this, err);
        err = 0;  // reset error
    }
    else {
        moveFrom_found = true;
    }

    if (!copyFrom_found && !moveFrom_found) {
        // one of copyFrom or moveFrom must be specified
        LOG_ERROR("Handler_rf[%p]: Neither copyFrom nor moveFrom are in query string", this);
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Neither copyFrom nor moveFrom are in query string\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return 0;
    }
    if (copyFrom_found && moveFrom_found) {
        LOG_ERROR("Handler_rf[%p]: Both copyFrom and moveFrom are in query string", this);
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Both copyFrom and moveFrom are in query string\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return 0;
    }
    assert(copyFrom_found ^ moveFrom_found);

    if (copyFrom_found) {
        Handler_rf_Helper::tryResolveAlias(copyFrom);
        err = handle_dir_POST_copy(copyFrom);
    }
    else {
        assert(moveFrom_found);
        Handler_rf_Helper::tryResolveAlias(moveFrom);
        err = handle_dir_POST_move(moveFrom);
    }
    // HTTP response set by handle_dir_POST_{copy,move}()

    return err;
}

int HttpSvc::Sn::Handler_rf::handle_dir_PUT()
{
    int err = 0;

    err = Handler_rf_Helper::pathValidatorCreateHelper.CreateIfNecessary_MediaRf_default();
    if (err) {
        // err msg logged by CreateIfNecessary_MediaRf_default()
        return err;
    }

    if (uri_tokens[0] == "media_rf" && !Handler_rf_Helper::mediaRfPathValidator_default->IsValidPath(path)) {
        LOG_INFO("Handler_rf[%p]: No access via media_rf; path %s", this, path.c_str());
        std::ostringstream oss;
        oss << "{\"errMsg\":\"No access via media_rf; path " << path << "\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return 0;
    }

    {
        std::string parentPath;
        Handler_rf_Helper::getParentPath(path, parentPath);
        err = Handler_rf_Helper::checkPathPermission(ds, parentPath, /*mustExist*/true, VPLFILE_CHECK_PERMISSION_WRITE, VPLFS_TYPE_DIR, /*checkType*/true, hs);
        if (err) {
            // Error message logged and HTTP response set by checkPathPermission()
            return 0;  // reset error
        }
    }
    err = Handler_rf_Helper::checkPathPermission(ds, path, /*mustExist*/false, VPLFILE_CHECK_PERMISSION_WRITE, VPLFS_TYPE_DIR, /*checkType*/true, hs);
    if (err) {
        // Error message logged and HTTP response set by checkPathPermission()
        return 0;  // reset error
    }

    err = ds->make_directory_iw(path, 0);
    if (!err) {
        err = ds->commit_iw();
    }
    if (err) {
        LOG_ERROR("Handler_rf[%p]: Failed to create directory; dataset "FMTu64", path %s, err %d", this, ds->get_id().did, path.c_str(), err);
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Failed to create directory; dataset " << ds->get_id().did << ", path " << path << "\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 500, oss.str(), "application/json");
        return 0;  // reset error
    }

    HttpStream_Helper::SetCompleteResponse(hs, 200);

    return err;
}

int HttpSvc::Sn::Handler_rf::handle_file_DELETE()
{
    int err = 0;

    err = Handler_rf_Helper::pathValidatorCreateHelper.CreateIfNecessary_MediaRf_default();
    if (err) {
        // err msg logged by CreateIfNecessary_MediaRf_default()
        return err;
    }

    if (uri_tokens[0] == "media_rf" && !Handler_rf_Helper::mediaRfPathValidator_default->IsValidPath(path)) {
        LOG_INFO("Handler_rf[%p]: No access via media_rf; path %s", this, path.c_str());
        std::ostringstream oss;
        oss << "{\"errMsg\":\"No access via media_rf; path " << path << "\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return 0;
    }

    {
        std::string parentPath;
        Handler_rf_Helper::getParentPath(path, parentPath);
        err = Handler_rf_Helper::checkPathPermission(ds, parentPath, /*mustExist*/true, VPLFILE_CHECK_PERMISSION_READ, VPLFS_TYPE_DIR, /*checkType*/true, hs);
        if (err) {
            // Error message logged and HTTP response set by checkPathPermission()
            return 0;  // reset error
        }
    }
    err = Handler_rf_Helper::checkPathPermission(ds, path, /*mustExist*/true, VPLFILE_CHECK_PERMISSION_DELETE, VPLFS_TYPE_FILE, /*checkType*/true, hs);
    if (err) {
        // Error message logged and HTTP response set by checkPathPermission()
        return 0;  // reset error
    }

    err = ds->remove_iw(path);
    if (!err) {
        err = ds->commit_iw();
    }
    if (err) {
        LOG_ERROR("Handler_rf[%p]: Failed to delete file; dataset "FMTu64", path %s, err %d", this, ds->get_id().did, path.c_str(), err);
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Failed to delete file; dataset " << ds->get_id().did << ", path " << path << "\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 500, oss.str(), "application/json");
        return 0;  // reset error
    }

    HttpStream_Helper::SetCompleteResponse(hs, 200);

    return err;
}

int HttpSvc::Sn::Handler_rf::handle_file_GET()
{
    int err = 0;

    err = Handler_rf_Helper::pathValidatorCreateHelper.CreateIfNecessary_MediaRf_default();
    if (err) {
        // err msg logged by CreateIfNecessary_MediaRf_default()
        return err;
    }

    if (uri_tokens[0] == "media_rf" && !Handler_rf_Helper::mediaRfPathValidator_default->IsValidPath(path)) {
        LOG_INFO("Handler_rf[%p]: No access via media_rf; path %s", this, path.c_str());
        std::ostringstream oss;
        oss << "{\"errMsg\":\"No access via media_rf; path " << path << "\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return 0;
    }

    err = Handler_rf_Helper::checkPathPermission(ds, path, /*mustExist*/true, VPLFILE_CHECK_PERMISSION_READ, VPLFS_TYPE_FILE, /*checkType*/true, hs);
    if (err) {
        // Error message logged and HTTP response set by checkPathPermission()
        return 0;  // reset error
    }

    MediaFile *mf = new (std::nothrow) MediaDsFile(ds, path);
    if (!mf) {
        LOG_ERROR("Handler_rf[%p]: Not enough memory", this);
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Not enough memory\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 500, oss.str(), "application/json");
        return 0;  // reset error
    }
    ON_BLOCK_EXIT(deleteObj<MediaFile>, mf);

    MediaFileSender *mfs = new (std::nothrow) MediaFileSender(mf, "", hs);
    if (!mfs) {
        LOG_ERROR("Handler_rf[%p]: Not enough memory", this);
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Not enough memory\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 500, oss.str(), "application/json");
        return 0;  // reset error;
    }
    ON_BLOCK_EXIT(deleteObj<MediaFileSender>, mfs);

    err = mfs->Send();
    // mfs->Send() sets the HTTP response

    return err;
}

int HttpSvc::Sn::Handler_rf::handle_file_POST()
{
    int err = 0;

    std::string copyFrom;
    bool copyFrom_found = false;
    err = hs->GetQuery("copyFrom", copyFrom);
    if (err == CCD_ERROR_NOT_FOUND) {
        // acceptable outcome - silently ignore
        err = 0;  // reset error
    }
    else if (err) {
        LOG_WARN("Handler_rf[%p]: Unexpected error ignored: err %d", this, err);
        err = 0;  // reset error
    }
    else {
        copyFrom_found = true;
    }

    std::string moveFrom;
    bool moveFrom_found = false;
    err = hs->GetQuery("moveFrom", moveFrom);
    if (err == CCD_ERROR_NOT_FOUND) {
        // acceptable outcome - silently ignore
        err = 0;  // reset error
    }
    else if (err) {
        LOG_WARN("Handler_rf[%p]: Unexpected error ignored: err %d", this, err);
        err = 0;  // reset error
    }
    else {
        moveFrom_found = true;
    }

    if (copyFrom_found && moveFrom_found) {
        LOG_ERROR("Handler_rf[%p]: Both copyFrom and moveFrom are in query string", this);
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Both copyFrom and moveFrom are in query string\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return 0;
    }

    if (!copyFrom_found && !moveFrom_found) {
        err = handle_file_POST_upload();
    }
    else if (copyFrom_found) {
        assert(!moveFrom_found);
        Handler_rf_Helper::tryResolveAlias(copyFrom);
        err = handle_file_POST_copy(copyFrom);
    }
    else {
        assert(moveFrom_found && !copyFrom_found);
        Handler_rf_Helper::tryResolveAlias(moveFrom);
        err = handle_file_POST_move(moveFrom);
    }
    // HTTP response set by handle_file_POST_{upload,copy,move}()

    return err;
}

int HttpSvc::Sn::Handler_rf::handle_filemetadata_GET()
{
    int err = 0;

    err = Handler_rf_Helper::pathValidatorCreateHelper.CreateIfNecessary_MediaRf_default();
    if (err) {
        // err msg logged by CreateIfNecessary_MediaRf_default()
        return err;
    }

    if (uri_tokens[0] == "media_rf" && !Handler_rf_Helper::mediaRfPathValidator_default->IsValidPath(path)) {
        LOG_INFO("Handler_rf[%p]: No access via media_rf; path %s", this, path.c_str());
        std::ostringstream oss;
        oss << "{\"errMsg\":\"No access via media_rf; path " << path << "\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return 0;
    }

    err = Handler_rf_Helper::checkPathPermission(ds, path, /*mustExist*/true, VPLFILE_CHECK_PERMISSION_READ, /*doesn't matter*/VPLFS_TYPE_FILE, /*checkType*/false, hs);
    if (err) {
        // Error message logged and HTTP response set by checkPathPermission()
        return 0;  // reset error
    }

    VPLFS_stat_t stat;
    err = ds->stat_component(path, stat);
    if (err) {
        LOG_ERROR("Handler_rf[%p]: Failed to stat; dataset "FMTu64", path %s, err %d", this, ds->get_id().did, path.c_str(), err);
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Failed to stat; dataset " << ds->get_id().did << ", path " << path << "\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 404, oss.str(), "application/json");
        return 0;
    }

    std::string filename;
    Handler_rf_Helper::getFileNameFromPath(path, filename);

#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
    struct {
        std::string path, type, args;
    } lnkTarget;

    if (HttpSvc::Sn::Handler_rf_Helper::isShortcut(path, stat.type))
    {   // Bug 10824: get shortcut detail if it is a file with .lnk extension
        ds->stat_shortcut(path, lnkTarget.path, lnkTarget.type, lnkTarget.args);
        //Error msg is recorded in stat_shortcut function.
        //If any error occurs within stat_shortcut, lnkTarget.path, lnkTarget.type or lnkTarget.args could be empty.
        //They will be handled while generating the JSON response below.
    }
#endif

    std::ostringstream oss;
    oss << "{\"name\":\"" << filename << '"'
        << ",\"size\":" << stat.size
        << ",\"lastChanged\":" << stat.mtime
        << ",\"isReadOnly\":" << (stat.isReadOnly ? "true" : "false")
        << ",\"isHidden\":" << (stat.isHidden ? "true" : "false")
        << ",\"isSystem\":" << (stat.isSystem ? "true" : "false")
        << ",\"isArchive\":" << (stat.isArchive ? "true" : "false");
#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
    if (!lnkTarget.path.empty()) {
        oss << ",\"target_path\":\"" << lnkTarget.path << "\"";
    }
    if (!lnkTarget.type.empty()) {
        oss << ",\"target_type\":\"" << lnkTarget.type << "\"";
    }
    if (!lnkTarget.args.empty()) {
        oss << ",\"target_args\":\"" << lnkTarget.args << "\"";
    }
#endif
    oss << "}";

    HttpStream_Helper::SetCompleteResponse(hs, 200, oss.str(), "application/json");

    return err;
}

int HttpSvc::Sn::Handler_rf::handle_filemetadata_PUT()
{
    int err = 0;

    err = Handler_rf_Helper::pathValidatorCreateHelper.CreateIfNecessary_MediaRf_default();
    if (err) {
        // err msg logged by CreateIfNecessary_MediaRf_default()
        return err;
    }

    if (uri_tokens[0] == "media_rf" && !Handler_rf_Helper::mediaRfPathValidator_default->IsValidPath(path)) {
        LOG_INFO("Handler_rf[%p]: No access via media_rf; path %s", this, path.c_str());
        std::ostringstream oss;
        oss << "{\"errMsg\":\"No access via media_rf; path " << path << "\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return 0;
    }

    {
        std::string parentPath;
        Handler_rf_Helper::getParentPath(path, parentPath);
        err = Handler_rf_Helper::checkPathPermission(ds, parentPath, /*mustExist*/true, VPLFILE_CHECK_PERMISSION_READ, VPLFS_TYPE_DIR, /*checkType*/true, hs);
        if (err) {
            // Error message logged and HTTP response set by checkPathPermission()
            return 0;  // reset error
        }
    }
    err = Handler_rf_Helper::checkPathPermission(ds, path, /*mustExist*/true, VPLFILE_CHECK_PERMISSION_WRITE, VPLFS_TYPE_FILE, /*checkType*/false, hs);
    if (err) {
        // Error message logged and HTTP response set by checkPathPermission()
        return 0;  // reset error
    }

    char buf[1024];
    char *reqBody = NULL;
    {
        ssize_t bytes = hs->Read(buf, sizeof(buf) - 1);  // subtract 1 for EOL
        if (bytes < 0) {
            LOG_ERROR("Handler_rf[%p]: Failed to read from HttpStream[%p]: err "FMT_ssize_t, this, hs, bytes);
            std::ostringstream oss;
            oss << "{\"errMsg\":\"Failed to read from HttpStream\"}";
            HttpStream_Helper::SetCompleteResponse(hs, 500, oss.str(), "application/json");
            return 0;
        }
        buf[bytes] = '\0';
        char *boundary = strstr(buf, "\r\n\r\n");  // find header-body boundary
        if (!boundary) {
            // header-body boundary not found - bad request
            LOG_ERROR("Handler_rf[%p]: Failed to find header-body boundary in request", this);
            std::ostringstream oss;
            oss << "{\"errMsg\":\"Failed to find header-body boundary in request\"}";
            HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
            return 0;
        }
        reqBody = boundary + 4;
    }

    u32 attrs = 0;
    u32 mask = 0;

    cJSON2 *json = cJSON2_Parse(reqBody);
    if (!json) {
        LOG_ERROR("Handler_rf[%p]: Failed to parse JSON in request body \"%s\"", this, reqBody);
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Failed to parse JSON in request body\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return 0;
    }
    ON_BLOCK_EXIT(cJSON2_Delete, json);

    for (int i = 0; i < cJSON2_GetArraySize(json); i++) {
        cJSON2 *item = cJSON2_GetArrayItem(json, i);
        if (strcmp(item->string, "isReadOnly") == 0) {
            if (item->type == cJSON2_True) {
                attrs |= VSSI_ATTR_READONLY;
                mask |= VSSI_ATTR_READONLY;
            }
            else if (item->type == cJSON2_False) {
                attrs &= ~VSSI_ATTR_READONLY;
                mask |= VSSI_ATTR_READONLY;
            }
            else {
                LOG_WARN("Handler_rf[%p]: Ignored isReadOnly that had invalid value", this);
            }
        }
        else if (strcmp(item->string, "isHidden") == 0) {
            if (item->type == cJSON2_True) {
                attrs |= VSSI_ATTR_HIDDEN;
                mask |= VSSI_ATTR_HIDDEN;
            }
            else if (item->type == cJSON2_False) {
                attrs &= ~VSSI_ATTR_HIDDEN;
                mask |= VSSI_ATTR_HIDDEN;
            }
            else {
                LOG_WARN("Handler_rf[%p]: Ignored isHidden that had invalid value", this);
            }
        }
        else if (strcmp(item->string, "isSystem") == 0) {
            if (item->type == cJSON2_True) {
                attrs |= VSSI_ATTR_SYS;
                mask |= VSSI_ATTR_SYS;
            }
            else if (item->type == cJSON2_False) {
                attrs &= ~VSSI_ATTR_SYS;
                mask |= VSSI_ATTR_SYS;
            }
            else {
                LOG_WARN("Handler_rf[%p]: Ignored isSystem that had invalid value", this);
            }
        }
        else if (strcmp(item->string, "isArchive") == 0) {
            if (item->type == cJSON2_True) {
                attrs |= VSSI_ATTR_ARCHIVE;
                mask |= VSSI_ATTR_ARCHIVE;
            }
            else if (item->type == cJSON2_False) {
                attrs &= ~VSSI_ATTR_ARCHIVE;
                mask |= VSSI_ATTR_ARCHIVE;
            }
            else {
                LOG_WARN("Handler_rf[%p]: Ignored isArchive that had invalid value", this);
            }
        }
        else {
            LOG_WARN("Handler_rf[%p]: Ignored unexpected attribute %s", this, item->string);
        }
    }

    if (mask) {
        do {
            VPLFS_stat_t stat;
            err = ds->stat_component(path, stat);
            if (err) {
                LOG_ERROR("Handler_rf[%p]: stat_component failed; path %s, err %d", this, path.c_str(), err);
                break;
            }

            if (stat.type == VPLFS_TYPE_FILE) {
                vss_file *file = NULL;
                err = ds->open_file(path, /*version*/0, /*flags*/0, /*attrs*/0, file);
                if (err) {
                    LOG_ERROR("Handler_rf[%p]: open_file failed; path %s, err %d", this, path.c_str(), err);
                    break;
                }
                err = ds->chmod_file(file, /*vss_object*/NULL, /*origin*/0, attrs, mask);
                if (err) {
                    LOG_ERROR("Handler_rf[%p]: chmod_file failed; path %s, err %d", this, path.c_str(), err);
                    // fall through
                }
                err = ds->close_file(file, /*vss_object*/NULL, /*origin*/0);
                if (err) {
                    LOG_ERROR("Handler_rf[%p]: close_file failed; path %s, err %d", this, path.c_str(), err);
                    break;
                }
            }
            else if (stat.type == VPLFS_TYPE_DIR) {
                err = ds->chmod_iw(path, attrs, mask);
                if (err) {
                    LOG_ERROR("Handler_rf[%p]: chmod_iw failed; path %s, err %d", this, path.c_str(), err);
                    break;
                }
                err = ds->commit_iw();
                if (err) {
                    LOG_ERROR("Handler_rf[%p]: commit_iw failed; err %d", this, err);
                    break;
                }
            }
            else {  // unexpected/unknown component type
                LOG_ERROR("Handler_rf[%p]: Unexpected component type; path %s, type %d", this, path.c_str(), stat.type);
                err = VPL_ERR_INVALID;
            }
        } while (0);

        if (err) {
            LOG_ERROR("Handler_rf[%p]: Failed to set permission; dataset "FMTu64", path %s, attrs %u, mask %u, err %d", this, ds->get_id().did, path.c_str(), attrs, mask, err);
            std::ostringstream oss;
            oss << "{\"errMsg\":\"Failed to set permission; dataset " << ds->get_id().did << ", path " << path << ", attrs " << attrs << ", mask " << mask << "\"}";
            HttpStream_Helper::SetCompleteResponse(hs, 500, oss.str(), "application/json");
            return 0;
        }
    }

    HttpStream_Helper::SetCompleteResponse(hs, 200);

    return err;
}

int HttpSvc::Sn::Handler_rf::handle_whitelist_GET()
{
using namespace HttpSvc::Sn::Handler_rf_Helper;

    int err = 0;

    err = Handler_rf_Helper::pathValidatorCreateHelper.CreateIfNecessary_MediaRf_default();
    if (err) {
        // err msg logged by CreateIfNecessary_MediaRf_default()
        return err;
    }

    if (uri_tokens[0] == "media_rf" && !Handler_rf_Helper::mediaRfPathValidator_default->IsValidPath(path)) {
        LOG_INFO("Handler_rf[%p]: No access via media_rf; path %s", this, path.c_str());
        std::ostringstream oss;
        oss << "{\"errMsg\":\"No access via media_rf; path " << path << "\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return 0;
    }

    AccessControlLocker acLocker;

    std::ostringstream oss;
    oss << "{\"fileList\":[";
    std::multimap<std::string,ccd::RemoteFileAccessControlDirSpec>::const_iterator it;
    for(it=acDirs.begin(); it!=acDirs.end(); it++){
        if(it != acDirs.begin())
            oss << ","; 
        oss << "{";
        if((it->second).has_dir()){
            oss << "\"dir\":";
            oss << "\"" << (it->second).dir() << "\"";
        }
        if((it->second).has_name()){
            oss << "\"name\":";
            oss << "\"" << (it->second).name() << "\"";
        }
        oss << ",\"isUser\":";
        if((it->second).is_user()){
            oss << "true";
        }else{
            oss << "false";
        }
        oss << ",\"isAllowed\":";
        if((it->second).is_allowed()){
            oss << "true";
        }else{
            oss << "false";
        }
        oss << "}";
    }
    oss << "],";
    oss << "\"numOfFiles\":";
    oss << acDirs.size();
    oss << "}";

    HttpStream_Helper::SetCompleteResponse(hs, 200, oss.str(), "application/json");

    return err;
}

int HttpSvc::Sn::Handler_rf::handle_dir_POST_copy(const std::string &copyFrom)
{
    int err = 0;

    LOG_WARN("Handler_rf[%p]: Not yet implemented", this);
    std::ostringstream oss;
    oss << "{\"errMsg\":\"Not yet implemented\"}";
    HttpStream_Helper::SetCompleteResponse(hs, 501, oss.str(), "application/json");

    return err;
}

int HttpSvc::Sn::Handler_rf::handle_dir_POST_move(const std::string &moveFrom)
{
    int err = 0;

    std::string oldpath(moveFrom);
    std::string newpath(path);
#if SN_RF_REVERSE_COPY_MOVE_DIR
    oldpath.swap(newpath);
#endif

    err = Handler_rf_Helper::pathValidatorCreateHelper.CreateIfNecessary_MediaRf_default();
    if (err) {
        // err msg logged by CreateIfNecessary_MediaRf_default()
        return err;
    }

    if (uri_tokens[0] == "media_rf" && !Handler_rf_Helper::mediaRfPathValidator_default->IsValidPath(newpath)) {
        LOG_INFO("Handler_rf[%p]: No access via media_rf; path %s", this, newpath.c_str());
        std::ostringstream oss;
        oss << "{\"errMsg\":\"No access via media_rf; path " << newpath << "\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return 0;
    }
    if (uri_tokens[0] == "media_rf" && !Handler_rf_Helper::mediaRfPathValidator_default->IsValidPath(oldpath)) {
        LOG_INFO("Handler_rf[%p]: No access via media_rf; path %s", this, oldpath.c_str());
        std::ostringstream oss;
        oss << "{\"errMsg\":\"No access via media_rf; path " << oldpath << "\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return 0;
    }

    {
        std::string parentPath;
        Handler_rf_Helper::getParentPath(oldpath, parentPath);
        err = Handler_rf_Helper::checkPathPermission(ds, parentPath, /*mustExist*/true, VPLFILE_CHECK_PERMISSION_READ, VPLFS_TYPE_DIR, /*checkType*/true, hs);
        if (err) {
            // Error message logged and HTTP response set by checkPathPermission()
            return 0;  // reset error
        }
    }
    err = Handler_rf_Helper::checkPathPermission(ds, oldpath, /*mustExist*/true, VPLFILE_CHECK_PERMISSION_WRITE|VPLFILE_CHECK_PERMISSION_DELETE, VPLFS_TYPE_DIR, /*checkType*/true, hs);
    if (err) {
        // Error message logged and HTTP response set by checkPathPermission()
        return 0;  // reset error
    }

    {
        std::string parentPath;
        Handler_rf_Helper::getParentPath(newpath, parentPath);
        err = Handler_rf_Helper::checkPathPermission(ds, parentPath, /*mustExist*/true, VPLFILE_CHECK_PERMISSION_WRITE, VPLFS_TYPE_DIR, /*checkType*/true, hs);
        if (err) {
            // Error message logged and HTTP response set by checkPathPermission()
            return 0;  // reset error
        }
    }
    err = Handler_rf_Helper::checkPathPermission(ds, newpath, /*mustExist*/false, VPLFILE_CHECK_PERMISSION_WRITE, VPLFS_TYPE_DIR, /*checkType*/true, hs);
    if (err) {
        // Error message logged and HTTP response set by checkPathPermission()
        return 0;  // reset error
    }

    err = ds->rename_iw(oldpath, newpath);
    if (!err) {
        err = ds->commit_iw();
    }
    if (err) {
        LOG_ERROR("Handler_rf[%p]: Failed to move directory; dataset "FMTu64", oldpath %s, newpath%s, err %d", this, ds->get_id().did, oldpath.c_str(), newpath.c_str(), err);
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Failed to move directory; dataset " << ds->get_id().did << ", oldpath " << oldpath << ", newpath " << newpath << "\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 500, oss.str(), "application/json");
        return 0;
    }

    HttpStream_Helper::SetCompleteResponse(hs, 200);

    return err;
}

int HttpSvc::Sn::Handler_rf::handle_file_POST_copy(const std::string &copyFrom)
{
    int err = 0;

    std::string oldpath(copyFrom);
    std::string newpath(path);
#if SN_RF_REVERSE_COPY_MOVE_DIR
    oldpath.swap(newpath);
#endif

    err = Handler_rf_Helper::pathValidatorCreateHelper.CreateIfNecessary_MediaRf_default();
    if (err) {
        // err msg logged by CreateIfNecessary_MediaRf_default()
        return err;
    }

    if (uri_tokens[0] == "media_rf" && !Handler_rf_Helper::mediaRfPathValidator_default->IsValidPath(newpath)) {
        LOG_INFO("Handler_rf[%p]: No access via media_rf; path %s", this, newpath.c_str());
        std::ostringstream oss;
        oss << "{\"errMsg\":\"No access via media_rf; path " << newpath << "\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return 0;
    }
    if (uri_tokens[0] == "media_rf" && !Handler_rf_Helper::mediaRfPathValidator_default->IsValidPath(oldpath)) {
        LOG_INFO("Handler_rf[%p]: No access via media_rf; %s", this, oldpath.c_str());
        std::ostringstream oss;
        oss << "{\"errMsg\":\"No access via media_rf; path " << oldpath << "\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return 0;
    }

    err = Handler_rf_Helper::checkPathPermission(ds, oldpath, /*mustExist*/true, VPLFILE_CHECK_PERMISSION_READ, VPLFS_TYPE_FILE, /*checkType*/true, hs);
    if (err) {
        // Error message logged and HTTP response set by checkPathPermission()
        return 0;  // reset error
    }

    {
        std::string parentPath;
        Handler_rf_Helper::getParentPath(newpath, parentPath);
        err = Handler_rf_Helper::checkPathPermission(ds, parentPath, /*mustExist*/true, VPLFILE_CHECK_PERMISSION_WRITE, VPLFS_TYPE_DIR, /*checkType*/true, hs);
        if (err) {
            // Error message logged and HTTP response set by checkPathPermission()
            return 0;  // reset error
        }
    }
    err = Handler_rf_Helper::checkPathPermission(ds, newpath, /*mustExist*/false, VPLFILE_CHECK_PERMISSION_WRITE, VPLFS_TYPE_FILE, /*checkType*/true, hs);
    if (err) {
        // Error message logged and HTTP response set by checkPathPermission()
        return 0;  // reset error
    }

    err = ds->copy_file(oldpath, newpath);
    if (err) {
        LOG_ERROR("Handler_rf[%p]: Failed to copy file; dataset "FMTu64", oldpath %s, newpath %s, err %d", this, ds->get_id().did, oldpath.c_str(), newpath.c_str(), err);
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Failed to copy file; dataset " << ds->get_id().did << ", oldpath " << oldpath << ", newpath " << newpath << "\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 500, oss.str(), "application/json");
        return 0;
    }

    HttpStream_Helper::SetCompleteResponse(hs, 200);

    return err;
}

int HttpSvc::Sn::Handler_rf::handle_file_POST_move(const std::string &moveFrom)
{
    int err = 0;

    std::string oldpath(moveFrom);
    std::string newpath(path);
#if SN_RF_REVERSE_COPY_MOVE_DIR
    oldpath.swap(newpath);
#endif

    err = Handler_rf_Helper::pathValidatorCreateHelper.CreateIfNecessary_MediaRf_default();
    if (err) {
        // err msg logged by CreateIfNecessary_MediaRf_default()
        return err;
    }

    if (uri_tokens[0] == "media_rf" && !Handler_rf_Helper::mediaRfPathValidator_default->IsValidPath(newpath)) {
        LOG_INFO("Handler_rf[%p]: No access via media_rf; path %s", this, newpath.c_str());
        std::ostringstream oss;
        oss << "{\"errMsg\":\"No access via media_rf; path " << newpath << "\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return 0;
    }
    if (uri_tokens[0] == "media_rf" && !Handler_rf_Helper::mediaRfPathValidator_default->IsValidPath(oldpath)) {
        LOG_INFO("Handler_rf[%p]: No access via media_rf; path %s", this, oldpath.c_str());
        std::ostringstream oss;
        oss << "{\"errMsg\":\"No access via media_rf; path " << oldpath << "\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return 0;
    }

    {
        std::string parentPath;
        Handler_rf_Helper::getParentPath(oldpath, parentPath);
        err = Handler_rf_Helper::checkPathPermission(ds, parentPath, /*mustExist*/true, VPLFILE_CHECK_PERMISSION_READ, VPLFS_TYPE_DIR, /*checkType*/true, hs);
        if (err) {
            // Error message logged and HTTP response set by checkPathPermission()
            return 0;  // reset error
        }
    }
    err = Handler_rf_Helper::checkPathPermission(ds, oldpath, /*mustExist*/true, VPLFILE_CHECK_PERMISSION_WRITE|VPLFILE_CHECK_PERMISSION_DELETE, VPLFS_TYPE_FILE, /*checkType*/true, hs);
    if (err) {
        // Error message logged and HTTP response set by checkPathPermission()
        return 0;  // reset error
    }

    {
        std::string parentPath;
        Handler_rf_Helper::getParentPath(newpath, parentPath);
        err = Handler_rf_Helper::checkPathPermission(ds, parentPath, /*mustExist*/true, VPLFILE_CHECK_PERMISSION_WRITE, VPLFS_TYPE_DIR, /*checkType*/true, hs);
        if (err) {
            // Error message logged and HTTP response set by checkPathPermission()
            return 0;  // reset error
        }
    }
    err = Handler_rf_Helper::checkPathPermission(ds, newpath, /*mustExist*/false, VPLFILE_CHECK_PERMISSION_WRITE, VPLFS_TYPE_FILE, /*checkType*/true, hs);
    if (err) {
        // Error message logged and HTTP response set by checkPathPermission()
        return 0;  // reset error
    }

    err = ds->rename_iw(oldpath.c_str(), newpath.c_str());
    if (!err) {
        err = ds->commit_iw();
    }
    if (err) {
        LOG_ERROR("Handler_rf[%p]: Failed to rename file; dataset "FMTu64", oldpath %s, newpath %s, err %d", this, ds->get_id().did, oldpath.c_str(), newpath.c_str(), err);
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Failed to rename file; dataset " << ds->get_id().did << ", oldpath " << oldpath << ", newpath " << newpath << "\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 500, oss.str(), "application/json");
        return 0;
    }

    HttpStream_Helper::SetCompleteResponse(hs, 200);

    return err;
}

int HttpSvc::Sn::Handler_rf::handle_file_POST_upload()
{
    int err = 0;

    err = Handler_rf_Helper::pathValidatorCreateHelper.CreateIfNecessary_MediaRf_default();
    if (err) {
        // err msg logged by CreateIfNecessary_MediaRf_default()
        return err;
    }

    if (uri_tokens[0] == "media_rf" && !Handler_rf_Helper::mediaRfPathValidator_default->IsValidPath(path)) {
        LOG_INFO("Handler_rf[%p]: No access via media_rf; path %s", this, path.c_str());
        std::ostringstream oss;
        oss << "{\"errMsg\":\"No access via media_rf; path " << path << "\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return 0;
    }

    {
        std::string parentPath;
        Handler_rf_Helper::getParentPath(path, parentPath);
        err = Handler_rf_Helper::checkPathPermission(ds, parentPath, /*mustExist*/true, VPLFILE_CHECK_PERMISSION_WRITE, VPLFS_TYPE_DIR, /*checkType*/true, hs);
        if (err) {
            // Error message logged and HTTP response set by checkPathPermission()
            return 0;  // reset error
        }
    }
    err = Handler_rf_Helper::checkPathPermission(ds, path, /*mustExist*/false, VPLFILE_CHECK_PERMISSION_WRITE, VPLFS_TYPE_FILE, /*checkType*/true, hs);
    if (err) {
        // Error message logged and HTTP response set by checkPathPermission()
        return 0;  // reset error
    }

    vss_file *file = NULL;
    err = ds->open_file(path, /*version*/0, VSSI_FILE_OPEN_WRITE | VSSI_FILE_OPEN_CREATE, /*attrs*/0, file);
    if (err) {
        LOG_ERROR("Handler_rf[%p]: Failed to open file; dataset "FMTu64", path %s, err %d", this, ds->get_id().did, path.c_str(), err);
        std::ostringstream oss;
        oss << "{\"errMsg\":\"Failed to open file; dataset " << ds->get_id().did << ", path " << path << "\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 500, oss.str(), "application/json");
        return 0;  // reset error
    }

    int httpStatus = 0;  // invalid value - make sure to override
    std::string errMsg;
    std::string errMsg_MimeType;
    bool deleteTargetFile = false;

    bool skippedHeader = false;
    u64 offset = 0;
    while (1) {
        char buf[4096];

        ssize_t bytes = hs->Read(buf, sizeof(buf) - 1);
        if (bytes < 0) {
            LOG_ERROR("Handler_rf[%p]: Failed to read from HttpStream[%p]: err "FMT_ssize_t, this, hs, bytes);
            httpStatus = 500;
            errMsg.assign("{\"errMsg\":\"Failed to read from HttpStream\"}");
            errMsg_MimeType.assign("application/json");
            deleteTargetFile = true;
            // do not propagate error
            goto out;
        }
        if (bytes == 0) // EOF
            break;

        char *data = NULL;
        u32 datasize = 0;
        if (!skippedHeader) {
            buf[bytes] = '\0';
            char *boundary = strstr(buf, "\r\n\r\n");  // find header-body boundary
            if (!boundary) {
                // header-body boundary not found - bad request
                LOG_ERROR("Handler_rf[%p]: Failed to find header-body boundary in request", this);
                httpStatus = 400;
                errMsg.assign("{\"errMsg\":\"Failed to find header-body boundary in request\"}");
                errMsg_MimeType.assign("application/json");
                deleteTargetFile = true;
                // do not propagate error
                goto out;
            }
            data = boundary + 4;
            datasize = bytes - (boundary + 4 - buf);
            skippedHeader = true;
        }
        else {
            data = buf;
            datasize = bytes;
        }

        u32 bytes_written = datasize;
        err = ds->write_file(file, /*vss_object*/NULL, /*origin*/0, offset, bytes_written, data);
        if (err) {
            LOG_ERROR("Handler_rf[%p]: Failed to write to file; dataset "FMTu64", path %s, err %d", this, ds->get_id().did, path.c_str(), err);
            httpStatus = 500;
            std::ostringstream oss;
            oss << "{\"errMsg\":\"Failed to write to file; dataset " << ds->get_id().did << ", path " << path << "\"}";
            errMsg.assign(oss.str());
            errMsg_MimeType.assign("application/json");
            deleteTargetFile = true;
            err = 0;  // do not propagate error
            goto out;
        }
        if (bytes_written < datasize) {
            LOG_ERROR("Handler_rf[%p]: Failed to write to file; dataset "FMTu64", path %s, wrote %u bytes, expected "FMT_ssize_t,
                      this, ds->get_id().did, path.c_str(), bytes_written, datasize);
            httpStatus = 500;
            std::ostringstream oss;
            oss << "{\"errMsg\":\"Failed to write to file; dataset " << ds->get_id().did << ", path " << path << "\"}";
            errMsg.assign(oss.str());
            errMsg_MimeType.assign("application/json");
            deleteTargetFile = true;
            // do not propagate error
            goto out;
        }

        offset += bytes_written;
    }
    httpStatus = 200;

 out:
    {
        int _err = ds->close_file(file, /*vss_object*/NULL, /*origin*/0);
        if (_err) {
            LOG_ERROR("Handler_rf[%p]: Failed to close file; dataset "FMTu64", path %s, err %d", this, ds->get_id().did, path.c_str(), _err);
        }
    }

    if (deleteTargetFile) {
        int _err = ds->remove_iw(path);
        if (_err) {
            LOG_ERROR("Handler_rf[%p]: Failed to remove file; dataset "FMTu64", path %s, err %d", this, ds->get_id().did, path.c_str(), _err);
        }
        else {
            _err = ds->commit_iw();
            if (_err) {
                LOG_ERROR("Handler_rf[%p]: Failed to commit changes to dataset; dataset "FMTu64", err %d", this, ds->get_id().did, _err);
            }
        }
        deleteTargetFile = false;
    }

    if (!errMsg.empty() && !errMsg_MimeType.empty()) {
        HttpStream_Helper::SetCompleteResponse(hs, httpStatus, errMsg, errMsg_MimeType);
    }
    else {
        HttpStream_Helper::SetCompleteResponse(hs, httpStatus);
    }

    return err;
}

static int parseSearchBeginRequest(cJSON2 *json,
                                   std::string& searchScope_out,
                                   std::string& searchPattern_out,
                                   bool& disableIndex,
                                   bool& recursive)
{
    searchScope_out.clear();
    searchPattern_out.clear();
    bool requiredSearchScopePresent = false;
    bool requiredSearchPatternPresent = false;

    if (json == NULL) {
        LOG_ERROR("No response");
        return -1;
    }

    if (json->type == cJSON2_Object) {
        cJSON2* currObject = json->child;
        if (currObject == NULL) {
            LOG_ERROR("Object returned has no children.");
            return -2;
        }
        if (currObject->prev != NULL) {
            LOG_ERROR("Unexpected prev value. Continuing.");
        }

        while (currObject != NULL) {
            if (currObject->type == cJSON2_String) {
                if (strcmp("path", currObject->string)==0) {
                    requiredSearchScopePresent = true;
                    searchScope_out = currObject->valuestring;
                } else if (strcmp("searchString", currObject->string)==0) {
                    requiredSearchPatternPresent = true;
                    searchPattern_out = currObject->valuestring;
                } else {
                    LOG_WARN("Unrecognized cjson string(%s) Continuing.", currObject->string);
                }
            } else if (currObject->type == cJSON2_True ||
                       currObject->type == cJSON2_False)
            {
                if (strcmp("disableIndex", currObject->string)==0) {
                    disableIndex = false;
                    if (currObject->type==cJSON2_True) {
                        disableIndex = true;
                    }
                } else if (strcmp("recursive", currObject->string)==0) {
                    recursive = false;
                    if (currObject->type==cJSON2_True) {
                        recursive = true;
                    }
                }
            }
            currObject = currObject->next;
        }
    }

    if (!requiredSearchScopePresent || !requiredSearchPatternPresent) {
        LOG_ERROR("Required arguments (path, searchString) missing:(%d,%d)",
                  requiredSearchScopePresent, requiredSearchPatternPresent);
        return -3;
    }
    return 0;
}

int HttpSvc::Sn::Handler_rf::handle_search_POST_begin()
{
    if (Handler_rf_Helper::rfSearchManager == NULL) {
        LOG_ERROR("Handler_rf[%p]: remoteFileSearchManager not implemented", this);
        std::ostringstream oss;
        oss << "{\"err\":" << RF_SEARCH_ERR_NOT_IMPLEMENTED << "}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return 0;
    }

    std::string httpBody;
    {
        char buf[256];
        std::string httpContent;

        ssize_t bytes = 0;
        while((bytes = hs->Read(buf, sizeof(buf))) > 0) {
            httpContent.append(buf, bytes);
        }
        if (bytes < 0) {
            LOG_ERROR("Handler_rf[%p]: Failed to read from HttpStream[%p]: err "FMT_ssize_t, this, hs, bytes);
            std::ostringstream oss;
            oss << "{\"errMsg\":\"Failed to read from HttpStream\"}";
            HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
            return 0;
        }

        const std::string bodyBoundary("\r\n\r\n");  // find header-body boundary
        ssize_t bodyBoundaryIndex = httpContent.find(bodyBoundary);
        if (bodyBoundaryIndex == std::string::npos ||
            (bodyBoundaryIndex + bodyBoundary.size()) >= httpContent.size())
        {
            // header-body boundary not found - bad request
            LOG_ERROR("Handler_rf[%p]: Failed to find header-body boundary in request", this);
            std::ostringstream oss;
            oss << "{\"errMsg\":\"Failed to find header-body boundary in request\"}";
            HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
            return 0;
        }

        bodyBoundaryIndex += bodyBoundary.size(); // Skip over the body boundary to the actual body.
        httpBody = httpContent.substr(bodyBoundaryIndex, httpContent.size()-bodyBoundaryIndex);
    }

    // Arguments to the search.
    std::string searchScope;
    std::string searchPattern;
    bool disableIndex = false;
    bool recursive = true;
    {
        cJSON2 *json = cJSON2_Parse(httpBody.c_str());
        if (json==NULL) {
            LOG_ERROR("Handler_rf[%p]: Failed to parse JSON in request body \"%s\"", this, httpBody.c_str());
            std::ostringstream oss;
            oss << "{\"err\":" << RF_SEARCH_ERR_INVALID_REQUEST << "}";
            HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
            return 0;
        }
        ON_BLOCK_EXIT(cJSON2_Delete, json);

        int rc = parseSearchBeginRequest(json,
                                         /*OUT*/ searchScope,
                                         /*OUT*/ searchPattern,
                                         /*OUT*/ disableIndex,
                                         /*OUT*/ recursive);
        if (rc != 0) {
            LOG_ERROR("Handler_rf[%p]: parseSearchBeginRequest(%s):%d", this, httpBody.c_str(), rc);
            std::ostringstream oss;
            oss << "{\"err\":" << RF_SEARCH_ERR_INVALID_REQUEST << "}";
            HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
            return 0;
        }
    }
    Handler_rf_Helper::tryResolveAlias(/*IN_OUT*/ searchScope);

    u64 searchQueueId = 0;
    int rc = Handler_rf_Helper::rfSearchManager->RemoteFileSearch_Add(
                                                    searchPattern,
                                                    searchScope,
                                                    disableIndex,
                                                    recursive,
                                                    /*OUT*/ searchQueueId);
    if (rc != 0) {
        LOG_ERROR("Handler_rf[%p]: RemoteFileSearch_Add: searchScope(%s), "
                  "searchPattern(%s), disableIndex(%d), recursive(%d): %d ",
                  this, searchScope.c_str(), searchPattern.c_str(),
                  disableIndex, recursive, rc);
        std::ostringstream oss;
        oss << "{\"err\":" << rc << "}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return 0;
    }

    LOG_ALWAYS("Handler_rf[%p]:RemoteFileSearch_Add:searchScope(%s),"
               "searchPattern(%s),disableIndex(%d),recursive(%d), "
               "searchQueueId:"FMTu64,
               this, searchScope.c_str(), searchPattern.c_str(),
               disableIndex, recursive, searchQueueId);

    {  // success
        std::ostringstream oss;
        oss << "{\"searchQueueId\":" << searchQueueId << "}";
        HttpStream_Helper::SetCompleteResponse(hs, 200, oss.str(), "application/json");
    }
    return 0;
}
int HttpSvc::Sn::Handler_rf::handle_search_POST_get()
{
    if (Handler_rf_Helper::rfSearchManager == NULL) {
        LOG_ERROR("Handler_rf[%p]: remoteFileSearchManager not implemented", this);
        std::ostringstream oss;
        oss << "{\"err\":" << RF_SEARCH_ERR_NOT_IMPLEMENTED << "}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return 0;
    }

    // Verify that required queryArgs are present.
    bool searchQueueIdExists = false;
    u64 searchQueueId = 0;
    bool startIndexExists = false;
    u64 startIndex = 0;
    bool maxPageSizeExists = false;
    u64 maxPageSize = 0;

    {
        std::map<std::string, std::string>::iterator tempIter;

        tempIter = uriQueryArgs.find("searchQueueId");
        if (tempIter!=uriQueryArgs.end()) {
            searchQueueIdExists = true;
            searchQueueId = VPLConv_strToU64(tempIter->second.c_str(), NULL, 10);
        }

        tempIter = uriQueryArgs.find("startIndex");
        if (tempIter!=uriQueryArgs.end()) {
            startIndexExists = true;
            startIndex = VPLConv_strToU64(tempIter->second.c_str(), NULL, 10);
        }

        tempIter = uriQueryArgs.find("maxPageSize");
        if (tempIter!=uriQueryArgs.end()) {
            maxPageSizeExists = true;
            maxPageSize = VPLConv_strToU64(tempIter->second.c_str(), NULL, 10);
        }
    }

    if (!searchQueueIdExists || !startIndexExists || !maxPageSizeExists) {
        LOG_ERROR("Handler_rf[%p]: Missing required args. searchQueueId(%d,"FMTu64"), "
                  "startIndex(%d,"FMTu64"), maxPageSize(%d,"FMTu64")",
                  this, searchQueueIdExists, searchQueueId,
                  startIndexExists, startIndex,
                  maxPageSizeExists, maxPageSize);
        std::ostringstream oss;
        oss << "{\"err\":" << RF_SEARCH_ERR_INVALID_REQUEST << "}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return 0;
    }

    std::vector<RemoteFileSearchResult> searchResults;
    bool isSearchOngoing = false;
    int rc = Handler_rf_Helper::rfSearchManager->RemoteFileSearch_GetResults(
                                                        searchQueueId,
                                                        startIndex,
                                                        maxPageSize,
                                                        /*OUT*/ searchResults,
                                                        /*OUT*/ isSearchOngoing);
    if (rc != 0) {
        LOG_ERROR("Handler_rf[%p]: Missing required args. searchQueueId(%d,"FMTu64"), "
                  "startIndex(%d,"FMTu64"), maxPageSize(%d,"FMTu64")" ":%d",
                  this, searchQueueIdExists, searchQueueId,
                  startIndexExists, startIndex,
                  maxPageSizeExists, maxPageSize, rc);
        std::ostringstream oss;
        oss << "{\"err\":" << rc << "}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return 0;
    }

    { // success!
        std::ostringstream oss;

        oss << "{\"searchResults\":[";
        bool firstTime = true;
        for (std::vector<RemoteFileSearchResult>::iterator myIter=searchResults.begin();
             myIter != searchResults.end(); ++myIter)
        {
            bool isShortcut = false;
            std::string typeStr;
            if (myIter->isDir) {
                typeStr = "\"dir\"";
            } else {
                if (myIter->isShortcut) {
                    isShortcut = true;
                    typeStr = "\"shortcut\"";
                } else {
                    typeStr = "\"file\"";
                }
            }

            oss << (firstTime?"":",") << "{\"path\":\"" << myIter->path.c_str() << "\"";
            oss << ",\"displayName\":\"" << myIter->displayName.c_str() <<"\"";
            oss << ",\"type\":" << typeStr.c_str();
            oss << ",\"lastChanged\":" << VPLTime_ToMillisec(myIter->lastChanged);
            oss << ",\"size\":" << myIter->size;
            oss << ",\"isReadOnly\":" << (myIter->isReadOnly?"true":"false");
            oss << ",\"isHidden\":" << (myIter->isHidden?"true":"false");
            oss << ",\"isSystem\":" << (myIter->isSystem?"true":"false");
            oss << ",\"isArchive\":" << (myIter->isArchive?"true":"false");
            oss << ",\"isAllowed\":" << (myIter->isAllowed?"true":"false");
            if (isShortcut) {
                oss << ",\"target_path\":" << "\"" << myIter->shortcut.path.c_str() << "\"";
                oss << ",\"target_displayName\":" << "\"" << myIter->shortcut.displayName.c_str() << "\"";
                oss << ",\"target_type\":" << "\"" << myIter->shortcut.type.c_str() << "\"";
                oss << ",\"target_args\":" << "\"" << myIter->shortcut.args.c_str() << "\"";
            }
            oss << "}";
            firstTime = false;
        }
        oss << "]";

        oss << ",\"numberReturned\":" << searchResults.size();
        oss << ",\"searchInProgress\":" << (isSearchOngoing?"true":"false");
        oss << "}";
        HttpStream_Helper::SetCompleteResponse(hs, 200, oss.str(), "application/json");

        LOG_INFO("Handler_rf[%p]: Successful searchGet(searchQueueId:"FMTu64
                 ",startIndex:"FMTu64",maxPageSize:"FMTu64",numResults:%d,onGoing:%s)",
                 this, searchQueueId, startIndex, maxPageSize,
                 searchResults.size(), isSearchOngoing?"true":"false");
    }
    return 0;
}
int HttpSvc::Sn::Handler_rf::handle_search_POST_end()
{
    if (Handler_rf_Helper::rfSearchManager == NULL) {
        LOG_ERROR("Handler_rf[%p]: remoteFileSearchManager not implemented", this);
        std::ostringstream oss;
        oss << "{\"err\":" << RF_SEARCH_ERR_NOT_IMPLEMENTED << "}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return 0;
    }
    // Verify that required queryArgs are present.
    bool searchQueueIdExists = false;
    u64 searchQueueId = 0;

    {
        std::map<std::string, std::string>::iterator tempIter;

        tempIter = uriQueryArgs.find("searchQueueId");
        if (tempIter!=uriQueryArgs.end()) {
            searchQueueIdExists = true;
            searchQueueId = VPLConv_strToU64(tempIter->second.c_str(), NULL, 10);
        }
    }

    if (!searchQueueIdExists) {
        LOG_ERROR("Handler_rf[%p]: Missing required args. searchQueueId(%d,"FMTu64")",
                  this, searchQueueIdExists, searchQueueId);
        std::ostringstream oss;
        oss << "{\"err\":" << RF_SEARCH_ERR_INVALID_REQUEST << "}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return 0;
    }

    int rc = Handler_rf_Helper::rfSearchManager->RemoteFileSearch_RequestClose(searchQueueId);
    if (rc != 0) {
        LOG_ERROR("Handler_rf[%p]: searchQueueId("FMTu64") RemoteFileSearch_RequestClose:%d",
                  this, searchQueueId, rc);
        std::ostringstream oss;
        oss << "{\"err\":" << RF_SEARCH_ERR_SEARCH_QUEUE_ID_INVALID << "}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return 0;
    }

    rc = Handler_rf_Helper::rfSearchManager->RemoteFileSearch_Join(searchQueueId);
    if (rc != 0) {
        LOG_ERROR("Handler_rf[%p]: searchQueueId("FMTu64") RemoteFileSearch_Join:%d",
                  this, searchQueueId, rc);
        std::ostringstream oss;
        oss << "{\"err\":" << RF_SEARCH_ERR_INTERNAL << "}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return 0;
    }

    rc = Handler_rf_Helper::rfSearchManager->RemoteFileSearch_Destroy(searchQueueId);
    if (rc != 0) {
        LOG_ERROR("Handler_rf[%p]: searchQueueId("FMTu64") RemoteFileSearch_Destroy:%d",
                  this, searchQueueId, rc);
        std::ostringstream oss;
        oss << "{\"err\":" << RF_SEARCH_ERR_INTERNAL << "}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return 0;
    }

    { // success!
        LOG_INFO("Handler_rf[%p]: Successfully ended searchQueueId("FMTu64")", this, searchQueueId);
        HttpStream_Helper::SetCompleteResponse(hs, 200, "", "application/json");
    }
    return 0;
}

int HttpSvc::Sn::Handler_rf::handle_dirmetadata_GET()
{
    int err = 0;

    err = Handler_rf_Helper::pathValidatorCreateHelper.CreateIfNecessary_MediaRf_default();
    if (err) {
        // err msg logged by CreateIfNecessary_MediaRf_default()
        return err;
    }

    if (uri_tokens[0] == "media_rf" && !Handler_rf_Helper::mediaRfPathValidator_default->IsValidPath(path)) {
        LOG_INFO("Handler_rf[%p]: No access via media_rf; path %s", this, path.c_str());
        std::ostringstream oss;
        oss << "{\"errMsg\":\"No access via media_rf; path " << path << "\"}";
        HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
        return 0;
    }

    err = Handler_rf_Helper::checkPathPermission(ds, path, /*mustExist*/true, VPLFILE_CHECK_PERMISSION_READ, VPLFS_TYPE_DIR, /*checkType*/true, hs);
    if (err) {
        // Error message logged and HTTP response set by checkPathPermission()
        return 0;  // reset error
    }

    //get real path
    std::string abspath;
    {
        err = ds->get_component_path(path, abspath);
        if (err != VSSI_SUCCESS) {
            LOG_ERROR("Failed to map component to path: component {%s}, error %d.", path.c_str(), err);
            return err;
        }else{
            //convert to windows format
            LOG_INFO("abspath: %s", abspath.c_str());
            if(*(abspath.rbegin()) == '/'){
                abspath.erase(abspath.size()-1);
            }

            //get_component_path will return c://, need to erase one forward slash
            {
                int p = 0;
                if ((p = abspath.find("//", p)) != string::npos) {
                    abspath.erase(p, 1);
                }
            }

#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
            std::replace(abspath.begin(), abspath.end(), '/', '\\');
#endif
        }
    }

    //slash is escape in json
    {
        int p = 0;
        while ((p = abspath.find('\\', p)) != string::npos) {
            abspath.insert(p, "\\");
            p += 2;
        }
    }

    std::ostringstream oss;
    oss << "{\"absPath\":";
    oss << "\"";
    oss << abspath;
    oss << "\"";
    oss << "}";

    HttpStream_Helper::SetCompleteResponse(hs, 200, oss.str(), "application/json");

    return err;
}

void HttpSvc::Sn::Handler_rf::initJumpTableMap_byObj(std::map<std::string, subhandler_func> &m)
{
    m["dir"]          = &Handler_rf::handle_dir;
    m["file"]         = &Handler_rf::handle_file;
    m["filemetadata"] = &Handler_rf::handle_filemetadata;
    m["whitelist"]    = &Handler_rf::handle_whitelist;
    m["search"]       = &Handler_rf::handle_search;
    m["dirmetadata"]  = &Handler_rf::handle_dirmetadata;
}

void HttpSvc::Sn::Handler_rf::initJumpTableMap_dir_byMethod(std::map<std::string, subhandler_func> &m)
{
    m["DELETE"] = &Handler_rf::handle_dir_DELETE;
    m["GET"]    = &Handler_rf::handle_dir_GET;
    m["POST"]   = &Handler_rf::handle_dir_POST;
    m["PUT"]    = &Handler_rf::handle_dir_PUT;
}

void HttpSvc::Sn::Handler_rf::initJumpTableMap_file_byMethod(std::map<std::string, subhandler_func> &m)
{
    m["DELETE"] = &Handler_rf::handle_file_DELETE;
    m["GET"]    = &Handler_rf::handle_file_GET;
    m["POST"]   = &Handler_rf::handle_file_POST;
    m["PUT"]    = &Handler_rf::handle_file_POST; // intentionally using the same logic for PUT and POST.
}

void HttpSvc::Sn::Handler_rf::initJumpTableMap_filemetadata_byMethod(std::map<std::string, subhandler_func> &m)
{
    m["GET"] = &Handler_rf::handle_filemetadata_GET;
    m["PUT"] = &Handler_rf::handle_filemetadata_PUT;
}

void HttpSvc::Sn::Handler_rf::initJumpTableMap_whitelist_byMethod(std::map<std::string, subhandler_func> &m)
{
    m["GET"] = &Handler_rf::handle_whitelist_GET;
}

void HttpSvc::Sn::Handler_rf::initJumpTableMap_dirmetadata_byMethod(std::map<std::string, subhandler_func> &m)
{
    m["GET"] = &Handler_rf::handle_dirmetadata_GET;
}

const HttpSvc::Sn::Handler_rf::JumpTableMap HttpSvc::Sn::Handler_rf::jumpTableMap_byObj(initJumpTableMap_byObj);
const HttpSvc::Sn::Handler_rf::JumpTableMap HttpSvc::Sn::Handler_rf::jumpTableMap_dir_byMethod(initJumpTableMap_dir_byMethod);
const HttpSvc::Sn::Handler_rf::JumpTableMap HttpSvc::Sn::Handler_rf::jumpTableMap_file_byMethod(initJumpTableMap_file_byMethod);
const HttpSvc::Sn::Handler_rf::JumpTableMap HttpSvc::Sn::Handler_rf::jumpTableMap_filemetadata_byMethod(initJumpTableMap_filemetadata_byMethod);
const HttpSvc::Sn::Handler_rf::JumpTableMap HttpSvc::Sn::Handler_rf::jumpTableMap_whitelist_byMethod(initJumpTableMap_whitelist_byMethod);
const HttpSvc::Sn::Handler_rf::JumpTableMap HttpSvc::Sn::Handler_rf::jumpTableMap_dirmetadata_byMethod(initJumpTableMap_dirmetadata_byMethod);

HttpSvc::Sn::Handler_rf_Helper::AliasMap::AliasMap()
{
#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
    do {
        char *path = NULL;
        int err = _VPLFS__GetLocalAppDataPath(&path);
        if (err) {
            LOG_ERROR("Failed to get user's LocalAppData path: err %d", err);
            break;
        }
        if (!path) {
            LOG_ERROR("Failed to get user's LocalAppData path");
            break;
        }
        m["LOCALAPPDATA"] = normalizeWinPath(path);
        free(path);
    } while (0);
    do {
        char *path = NULL;
        int err = _VPLFS__GetProfilePath(&path);
        if (err) {
            LOG_ERROR("Failed to get user's Profile path: err %d", err);
            break;
        }
        if (!path) {
            LOG_ERROR("Failed to get user's Profile path");
            break;
        }
        m["USERPROFILE"] = normalizeWinPath(path);
        free(path);
    } while (0);
#elif defined(CLOUDNODE)
    m["LOCALAPPDATA"] = "/";
    m["USERPROFILE"] = "/";
#elif defined(LINUX) && defined(DEBUG)
    // FOR DEVELOPER USER ONLY
    m["LOCALAPPDATA"] = "/temp/localappdata";
    m["USERPROFILE"] = "/temp/userprofile";
#else
    // nothing to do
#endif
}

HttpSvc::Sn::Handler_rf_Helper::SyncboxAliasMap::SyncboxAliasMap()
{
    std::string stagingAreaAlias;
    std::string rfAppSyncFolderAlias;

    MutexAutoLock lock(VPLLazyInitMutex_GetMutex(&syncboxParamMutex));

    if (!isSyncboxArchiveStorage)
        return;

    stagingAreaAlias = getSyncboxArchiveStorageStagingAreaAlias(syncboxDatasetId);
    rfAppSyncFolderAlias = getSyncboxRFAppSyncFolderAlias(syncboxDatasetId);

#if defined(VPL_PLAT_IS_WIN_DESKTOP_MODE)
    m[stagingAreaAlias] = normalizeWinPath(syncboxStagingAreaPath.c_str());
    m[rfAppSyncFolderAlias] = normalizeWinPath(syncboxSyncFeaturePath.c_str());
#else
    m[stagingAreaAlias] = syncboxStagingAreaPath;
    m[rfAppSyncFolderAlias] = syncboxSyncFeaturePath;
#endif
}

void HttpSvc::Sn::Handler_rf_Helper::tryResolveAlias(std::string &path_in_out)
{
    if (path_in_out.size() == 0) return;
    assert(path_in_out.size() > 0);

    if (path_in_out[0] != '[') return;

    size_t pos = path_in_out.find(']');
    if (pos == path_in_out.npos) {
        LOG_WARN("Ignored unterminated alias: path %s", path_in_out.c_str());
        return;
    }
    if (pos == 1) {
        LOG_WARN("Ignored empty alias: path %s", path_in_out.c_str());
        return;
    }

    std::string alias(path_in_out, 1, pos - 1);
    AliasMap aliasMap;  // TODO: create obj only once for each user activation.  Bug 15737
    AliasMap::const_iterator it1 = aliasMap.find(alias);
    if (it1 == aliasMap.end()) {
        if (isSyncboxArchiveStorage) {
            SyncboxAliasMap syncboxAliasMap;
            SyncboxAliasMap::const_iterator it2 = syncboxAliasMap.find(alias);
            if (it2 == syncboxAliasMap.end()) {
                LOG_WARN("Ignored unknown alias %s", alias.c_str());
                return;
            } else {
                path_in_out.replace(0, pos + 1, it2->second);
            }
        }
    } else {
        path_in_out.replace(0, pos + 1, it1->second);
    }

    // clean up: replace any "//" with "/"
    pos = 0;
    while ((pos = path_in_out.find("//", pos)) != path_in_out.npos) {
        path_in_out.replace(pos, 2, "/");
    }
}

bool HttpSvc::Sn::Handler_rf_Helper::isSyncboxAlias(const std::string& path)
{
    if (path.size() == 0) return false;
    assert(path.size() > 0);

    if (path[0] != '[') return false;

    size_t pos = path.find(']');
    if (pos == path.npos) {
        LOG_WARN("Ignored unterminated alias: path %s", path.c_str());
        return false;
    }
    if (pos == 1) {
        LOG_WARN("Ignored empty alias: path %s", path.c_str());
        return false;
    }

    std::string alias(path, 1, pos - 1);
    if (isSyncboxArchiveStorage) {
        SyncboxAliasMap syncboxAliasMap;
        SyncboxAliasMap::const_iterator it = syncboxAliasMap.find(alias);
        if (it == syncboxAliasMap.end()) {
            return false;
        } else {
            return true;
        }
    } else 
        return false;
}

HttpSvc::Sn::Handler_rf_Helper::PathValidator::PathValidator(void (*initializer)(Patterns &patterns),
                                                             void (*refresher)(Patterns &patterns))
    : initializer(initializer), refresher(refresher)
{
    if (initializer) {
        (*initializer)(patterns);
    }
}

bool HttpSvc::Sn::Handler_rf_Helper::PathValidator::IsValidPath(const std::string &path)
{
    if (refresher) {
        (*refresher)(patterns);
    }

    if (patterns.exact_static.empty() && patterns.prefix_static.empty() && patterns.prefix_dynamic.empty())
        return true;

    return isValidPath_byExactMatch(path, patterns.exact_static) ||
        isValidPath_byPrefixMatch(path, patterns.prefix_static) ||
        isValidPath_byPrefixMatch(path, patterns.prefix_dynamic);
}

bool HttpSvc::Sn::Handler_rf_Helper::PathValidator::isValidPath_byExactMatch(const std::string &path, const std::set<std::string> &patterns)
{
    std::set<std::string>::const_iterator it;
    for (it = patterns.begin(); it != patterns.end(); it++) {
        if (path == *it)
            return true;
        // ELSE keep testing
    }
    return false;
}

bool HttpSvc::Sn::Handler_rf_Helper::PathValidator::isValidPath_byPrefixMatch(const std::string &path, const std::set<std::string> &patterns)
{
    std::set<std::string>::const_iterator it;
    for (it = patterns.begin(); it != patterns.end(); it++) {
        if (path.compare(0, it->size(), *it) == 0) {
            if (path.size() == it->size())
                return true;
            else if (path[it->size()] == '/')
                return true;
            // ELSE keep testing
        }
        // ELSE keep testing
    }
    return false;
}

void HttpSvc::Sn::Handler_rf_Helper::PathValidator_MediaRfPatternInitializer_Default(PathValidator::Patterns &patterns)
{
    patterns.exact_static.clear();
    patterns.prefix_static.clear();

#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
    // allowed folder lists are
    // 1. UserProfile/PicStream
    // 2. [LOCALAPPDATA]/clear.fi (both virtual and expanded path)
    // 3. All the music/video/photo library folders (both virtual and expanded path)
    {
        std::string path = "[USERPROFILE]/Picstream";
        tryResolveAlias(path);
        patterns.prefix_static.insert(path);
    }

    {
        std::string path = "[LOCALAPPDATA]/clear.fi";
        tryResolveAlias(path);
        patterns.prefix_static.insert(path);
    }
#elif defined(CLOUDNODE)
    // allowed folder lists are
    // 1. [LOCALAPPDATA]/clear.fi (both virtual and expanded path)
    {
        std::string path = "[LOCALAPPDATA]/clear.fi";
        tryResolveAlias(path);
        patterns.prefix_static.insert(path);
        patterns.prefix_static.insert("clear.fi");
    }
#endif

    PathValidator_MediaRfPatternRefresher_Default(patterns);
}

void HttpSvc::Sn::Handler_rf_Helper::PathValidator_MediaRfPatternInitializer_GetDir(PathValidator::Patterns &patterns)
{
    PathValidator_MediaRfPatternInitializer_Default(patterns);
#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
    patterns.exact_static.insert("Libraries");
#endif
}

void HttpSvc::Sn::Handler_rf_Helper::PathValidator_MediaRfPatternRefresher_Default(PathValidator::Patterns &patterns)
{
    patterns.prefix_dynamic.clear();

#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
    int err = 0;

    char *librariesPath = NULL;
    err = _VPLFS__GetLibrariesPath(&librariesPath);
    if (err) return;
    ON_BLOCK_EXIT(free, librariesPath);

    VPLFS_dir_t dir;
    VPLFS_dirent_t dirent;
    err = VPLFS_Opendir(librariesPath, &dir);
    if (err) return;
    ON_BLOCK_EXIT(VPLFS_Closedir, &dir);

    while (VPLFS_Readdir(&dir, &dirent) == VPL_OK) {
        char *p = strstr(dirent.filename, ".library-ms");
        if (p == NULL) continue;
        if (p[strlen(".library-ms")] != '\0') continue;

        // found library description file
        std::string libDescFilePath;
        libDescFilePath.assign(librariesPath).append("/").append(dirent.filename);

        // grab both localized and non-localized name of the library folders
        _VPLFS__LibInfo libinfo;
        _VPLFS__GetLibraryFolders(libDescFilePath.c_str(), &libinfo);
        if (libinfo.folder_type != "Music" &&
            libinfo.folder_type != "Video" &&
            libinfo.folder_type != "Photo") continue;

        std::map<std::string, _VPLFS__LibFolderInfo>::const_iterator it;
        for (it = libinfo.m.begin(); it != libinfo.m.end(); it++) {
            patterns.prefix_dynamic.insert(normalizeWinPath(it->second.path.c_str()));
        }

        patterns.prefix_dynamic.insert("Libraries/"+libinfo.n_name);
        patterns.prefix_dynamic.insert("Libraries/"+libinfo.l_name);
    }
#endif
}

std::string HttpSvc::Sn::Handler_rf_Helper::normalizeWinPath(const char *path)
{
    std::ostringstream oss;
    oss << "Computer/";
    if (strlen(path) >= 2 && path[1] == ':') {
        // drive letter present; e.g., C:/abc/def
        oss << path[0];
        if (strlen(path) > 3)
        {   // Bug 13322: When the target path is Computer/D/, CCD needs to
            //            remove the last '/;
            oss << &path[2];
        }
    }
    else {
        // unlikely
        oss << path;
    }
    return oss.str();
}


bool HttpSvc::Sn::Handler_rf_Helper::isShortcut(const std::string& path,
                                                VPLFS_file_type_t fileType)
{
    std::string endToMatch = ".lnk";
    if( fileType == VPLFS_TYPE_FILE &&
        path.size() >= endToMatch.size())
    {
        std::string ending = path.substr(path.size() - endToMatch.size());
        std::transform(ending.begin(), ending.end(), ending.begin(), ::tolower);
        if(ending == endToMatch) {
            return true;
        }
    }
    return false;
}

void HttpSvc::Sn::Handler_rf_Helper::winPathToRemoteFilePath(
                                        const std::string& inputWinPath,
                                        std::string& rfPath_out)
{
    rfPath_out.clear();
    // Fixes drive letter.  Example: C:\users\blah --> Computer/C\users\blah
    rfPath_out = normalizeWinPath(inputWinPath.c_str());

    // Replace all '\\' with '/'
    std::replace(rfPath_out.begin(), rfPath_out.end(), '\\', '/');
}

std::string HttpSvc::Sn::Handler_rf_Helper::getParentPath(const std::string &path)
{
    std::string parentDir;
    getParentPath(path, parentDir);
    return parentDir;
}

void HttpSvc::Sn::Handler_rf_Helper::getParentPath(const std::string &path, std::string &parentDir)
{
    size_t len = path.size();

    // skip slashes
    for (; len > 0 && path[len-1] == '/'; len--)
        ;
    // skip non-slashes
    for (; len > 0 && path[len-1] != '/'; len--)
        ;
    // skip slashes
    for (; len > 0 && path[len-1] == '/'; len--)
        ;

    if (len == 0) {
        if (path[0] == '/')
            parentDir.assign("/");
        else
            parentDir.clear();
    }
    else 
        parentDir.assign(path, 0, len);
}

#ifdef TEST_HTTPSVC_SN_HANDLER_RF_GETPARENTDIRECTORY
static void testSnHandlerRf_getParentPath(const std::string &teststr, const std::string &ans)
{
    std::string result;
    HttpSvc::Sn::Handler_rf_Helper::getParentPath(teststr, result);
    if (result == ans) {
        LOG_INFO("\"%s\" -> \"%s\" PASS", teststr.c_str(), result.c_str());
    }
    else {
        LOG_INFO("\"%s\" -> \"%s\" FAIL", teststr.c_str(), result.c_str());
    }
}

void testSnHandlerRf_getParentPath();
void testSnHandlerRf_getParentPath()
{
    testSnHandlerRf_getParentPath("", "");
    testSnHandlerRf_getParentPath("/", "/");
    testSnHandlerRf_getParentPath("///", "/");
    testSnHandlerRf_getParentPath("/abc", "/");
    testSnHandlerRf_getParentPath("/abc/", "/");
    testSnHandlerRf_getParentPath("/abc///", "/");
    testSnHandlerRf_getParentPath("/abc/def", "/abc");
    testSnHandlerRf_getParentPath("/abc/def/", "/abc");
    testSnHandlerRf_getParentPath("/abc/def///", "/abc");
    testSnHandlerRf_getParentPath("///abc///def///", "///abc");  // contiguous-slash elimination only works at end
    testSnHandlerRf_getParentPath("abc", "");
    testSnHandlerRf_getParentPath("abc/", "");
    testSnHandlerRf_getParentPath("abc///", "");
    testSnHandlerRf_getParentPath("abc/def", "abc");
    testSnHandlerRf_getParentPath("abc/def/", "abc");
    testSnHandlerRf_getParentPath("abc/def///", "abc");
}
#endif // TEST_HTTPSVC_SN_HANDLER_RF_GETPARENTDIRECTORY

std::string HttpSvc::Sn::Handler_rf_Helper::getFileNameFromPath(const std::string &path)
{
    std::string fileName;
    getFileNameFromPath(path, fileName);
    return fileName;
}

void HttpSvc::Sn::Handler_rf_Helper::getFileNameFromPath(const std::string &path, std::string &fileName)
{
    size_t len = path.size();

    // skip slashes
    for (; len > 0 && path[len-1] == '/'; len--)
        ;
    size_t pos_last = len;
    // skip non-slashes
    for (; len > 0 && path[len-1] != '/'; len--)
        ;
    size_t pos_first = len;

    if (pos_first < pos_last)
        fileName.assign(path, pos_first, pos_last - pos_first);
    else
        fileName.clear();
}

#ifdef TEST_HTTPSVC_SN_HANDLER_RF_FILENAMEFROMPATH
static void testSnHandlerRf_getFileNameFromPath(const std::string &teststr, const std::string &ans)
{
    std::string result;
    HttpSvc::Sn::Handler_rf_Helper::getFileNameFromPath(teststr, result);
    if (result == ans) {
        LOG_INFO("\"%s\" -> \"%s\" PASS", teststr.c_str(), result.c_str());
    }
    else {
        LOG_INFO("\"%s\" -> \"%s\" FAIL", teststr.c_str(), result.c_str());
    }
}

void testSnHandlerRf_getFileNameFromPath();
void testSnHandlerRf_getFileNameFromPath()
{
    testSnHandlerRf_getFileNameFromPath("", "");
    testSnHandlerRf_getFileNameFromPath("/", "");
    testSnHandlerRf_getFileNameFromPath("///", "");
    testSnHandlerRf_getFileNameFromPath("/abc", "abc");
    testSnHandlerRf_getFileNameFromPath("/abc/", "abc");
    testSnHandlerRf_getFileNameFromPath("/abc///", "abc");
    testSnHandlerRf_getFileNameFromPath("/abc/def", "def");
    testSnHandlerRf_getFileNameFromPath("/abc/def/", "def");
    testSnHandlerRf_getFileNameFromPath("/abc/def///", "def");
    testSnHandlerRf_getFileNameFromPath("///abc///def///", "def");
    testSnHandlerRf_getFileNameFromPath("abc", "abc");
    testSnHandlerRf_getFileNameFromPath("abc/", "abc");
    testSnHandlerRf_getFileNameFromPath("abc///", "abc");
    testSnHandlerRf_getFileNameFromPath("abc/def", "def");
    testSnHandlerRf_getFileNameFromPath("abc/def/", "def");
    testSnHandlerRf_getFileNameFromPath("abc/def///", "def");
}
#endif // TEST_HTTPSVC_SN_HANDLER_RF_GETFILENAMEFROMPATH

int HttpSvc::Sn::Handler_rf_Helper::getEnabledFeatures(u64 userId, u64 deviceId, EnabledFeatures &enabledFeatures)
{
    int err = 0;

    enabledFeatures.mediaServer = false;
    enabledFeatures.remoteFileAccess = false;

    ccd::ListUserStorageInput request;
    ccd::ListUserStorageOutput listSnOut;
    request.set_user_id(userId);
    request.set_only_use_cache(true);
    err = CCDIListUserStorage(request, listSnOut);
    if (err) {
        LOG_ERROR("CCDIListUserStorage failed: err %d", err);
        return err;
    }

    for (int i = 0; i < listSnOut.user_storage_size(); i++) {
        if (listSnOut.user_storage(i).storageclusterid() == deviceId) {
            enabledFeatures.mediaServer = listSnOut.user_storage(i).featuremediaserverenabled();
            enabledFeatures.remoteFileAccess = listSnOut.user_storage(i).featureremotefileaccessenabled();
            return 0;
        }
    }

    LOG_ERROR("Unknown storage device ID "FMTu64": userId "FMTu64, deviceId, userId);

    return err;
}

int HttpSvc::Sn::Handler_rf_Helper::checkAccessControlList(dataset *ds, const std::string &path)
{

    int rv = VPL_OK;

#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
    AccessControlLocker acLocker;
    std::string upath;

    //Just return VPL_OK if no blacklist exist
    if(blackList.empty())
        return VPL_OK;

    //expand alias to real path, similar to the logic of fs_dataset::check_access_right()
    {
        // return VPL_OK if path = "/", "Computer", "Libraries" & "Libraries/xxx"
        if (path == "" || path == "/") {
            return VPL_OK;
        } else if (path == "Computer" || path == "Computer/") {
            return VPL_OK;
        } else if (path == "Libraries" || path == "Libraries/") {
            return VPL_OK;
        }

        //convert to real path
        rv = ds->get_component_path(path, upath);
        if (rv != VSSI_SUCCESS) {
            LOG_WARN("Failed to map component to path: component {%s}, error %d.", path.c_str(), rv);
            VPLFS_stat_t stat;
            rv = ds->stat_component(path, stat);
            if (rv) {
                LOG_WARN("Failed to stat; dataset "FMTu64", path %s: err %d", ds->get_id().did, path.c_str(), rv);
                //TODO: need to check if the component must exist
                //goto out;
            } else {
                //We will keep name format in case the stat_component is pass but get_component_path is failed
                //ex. Libraries/Music
                //we will use path directly in this case
            }
        }
        //convert to windows format
        std::replace(upath.begin(), upath.end(), '/', '\\');
    }

    // copy from _VPLFS__GetExtendedPath
    // collapse consecutive slashes into one
    {
        size_t pos = upath.find_first_of("\\");
        while (pos != std::string::npos) {
            size_t pos2 = upath.find_first_not_of("\\", pos);
            // position interval [pos, pos2) contains slashes
            size_t len = pos2 != std::string::npos ? pos2 - pos : upath.length() - pos;
            upath.replace(pos, len, 1, '\\');
            if (pos + 1 < upath.length())
                pos = upath.find_first_of("\\", pos + 1);
            else
                pos = std::string::npos;
        }
    }


    //Check blacklist
    //TODO: if found, need to check next character is path separator
    {
        rv = VPL_OK;
        std::set<std::string>::const_iterator it;
        for(it=blackList.begin(); it!=blackList.end(); it++){
            std::string::size_type found;
            //Compare native format path
            found = upath.find((*it).c_str(), 0, (*it).size());
            if(found != std::string::npos){
                //Make sure there is slash right after matched acl
                //Ex: if acl is C:\test123, block c:\test123\xxx but don't block C:\test1234
                if(upath.size() == (*it).size() ||
                  (upath.size() >= (*it).size()+1 && upath[(*it).size()] == '\\') ||
                  ((*it).size() == 3 && (*it)[1] == ':' && (*it)[2] == '\\')){

                    rv = VPL_ERR_ACCESS;
                    break;
                }
            }
            //Compare name
            found = path.find((*it).c_str(), 0, (*it).size());
            if(found != std::string::npos){
                //Make sure there is slash right after matched acl
                //Ex: if acl is C:\test123, block c:\test123\xxx but don't block C:\test1234
                if(path.size() == (*it).size() ||
                  (path.size() >= (*it).size()+1 && path[(*it).size()] == '/')){
                    rv = VPL_ERR_ACCESS;
                    break;
                }
            }
        }
        if(rv == VPL_OK)
            goto out;

        //check whitelist
        for(it=whiteList.begin(); it!=whiteList.end(); it++){
            std::string::size_type found;
            found = upath.find((*it).c_str(), 0, (*it).size());
            if(found != std::string::npos){
                //Make sure there is slash right after matched acl
                //Ex: if acl is C:\test123, block c:\test123\xxx but don't block C:\test1234
                if(upath.size() == (*it).size() ||
                  (upath.size() >= (*it).size()+1 && upath[(*it).size()] == '\\')){ 
                    rv = VPL_OK;
                    break;
                }
            }
        }
        if(rv == VPL_OK)
            goto out;
        //check userWhitelist
        for(it=userWhiteList.begin(); it!=userWhiteList.end(); it++){
            std::string::size_type found;
            found = upath.find((*it).c_str(), 0, (*it).size());
            if(found != std::string::npos){
                //Make sure there is slash right after matched acl
                //Ex: if acl is C:\test123, block c:\test123\xxx but don't block C:\test1234
                if(upath.size() == (*it).size() ||
                  (upath.size() >= (*it).size()+1 && upath[(*it).size()] == '\\')){
                    rv = VPL_OK;
                    break;
                }
            }
        }
        if(rv == VPL_OK)
            goto out;
    }

out:
#endif
    return rv;
}

int HttpSvc::Sn::Handler_rf_Helper::checkPathPermission(dataset *ds,
                                                        const std::string &path,
                                                        bool mustExist,
                                                        u32 accessMask,
                                                        VPLFS_file_type_t expectedType,
                                                        bool checkType,
                                                        HttpStream *hs)
{
    int err = 0;

    // check whether the directory exist or not
    VPLFS_stat_t stat;
    if (mustExist) {
        // checking the parent directory first before we start checking the target path
        // this allow us to identify whether the parent directory is missing as well
        // for detail, check bug #8074
        err = ds->stat_component(Handler_rf_Helper::getParentPath(path), stat);
        if (err) {
            LOG_ERROR("Parent directory missing; dataset "FMTu64", path %s, err %d",
                      ds->get_id().did, path.c_str(), err);
            std::ostringstream oss;
            oss << "{\"errMsg\":\"" << RF_ERR_MSG_NODIR << "\"}";
            HttpStream_Helper::SetCompleteResponse(hs, 404, oss.str(), "application/json");
            return err;
        }

        err = ds->stat_component(path, stat);
        if (err) {
            LOG_ERROR("Failed to stat; dataset "FMTu64", path %s: err %d", ds->get_id().did, path.c_str(), err);
            std::ostringstream oss;
            oss << "{\"errMsg\":\"Failed to stat; dataset " << ds->get_id().did << ", path " << path << "\"}";
            HttpStream_Helper::SetCompleteResponse(hs, 404, oss.str(), "application/json");
            return err;
        }
        // check whether the file type is expected
        if (checkType && (stat.type != expectedType)) {
            LOG_ERROR("Unexpected type; dataset "FMTu64", path %s, expected %d, got %d, err %d",
                      ds->get_id().did, path.c_str(), expectedType, stat.type, err);
            std::ostringstream oss;
            oss << "{\"errMsg\":\"Unexpected type; dataset " << ds->get_id().did
                << ", path " << path << ", expected " << expectedType
                << ", got " << stat.type << "\"}";
            HttpStream_Helper::SetCompleteResponse(hs, 404, oss.str(), "application/json");
            return -1;
        }

        // check permission
        err = ds->check_access_right(path, accessMask);
        if (err == VPL_ERR_ACCESS) {
            LOG_ERROR("Access denied; dataset "FMTu64", path %s", ds->get_id().did, path.c_str());
            std::ostringstream oss;
            oss << "{\"errMsg\":\"Access denied; dataset " << ds->get_id().did << ", path " << path << "\"}";
            HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
            return -1;
        }
        if (err) {
            LOG_ERROR("Failed to determine access rights; dataset "FMTu64", path %s, err %d",
                      ds->get_id().did, path.c_str(), err);
            std::ostringstream oss;
            oss << "{\"errMsg\":\"Failed to determine access rights; dataset " << ds->get_id().did
                << ", path " << path << "\"}";
            HttpStream_Helper::SetCompleteResponse(hs, 500, oss.str(), "application/json");
            return err;
        }
    } else {
        err = ds->stat_component(path, stat);
        if (!err) {
            LOG_ERROR("Path exists; dataset "FMTu64", path %s", ds->get_id().did, path.c_str());
            std::ostringstream oss;
            oss << "{\"errMsg\":\"Path exists; dataset " << ds->get_id().did << ", path " << path << "\"}";
            HttpStream_Helper::SetCompleteResponse(hs, 409, oss.str(), "application/json");
            return -1;
        }
    }

    //Check Access Control List
    //check if it's fs_dataset first
    {
        err = Handler_rf_Helper::checkAccessControlList(ds, path);
        if (err == VPL_ERR_ACCESS) {
            LOG_ERROR("Access denied; dataset "FMTu64", path %s", ds->get_id().did, path.c_str());
            std::ostringstream oss;
            oss << "{\"errMsg\":\"Access denied by access control list; dataset " << ds->get_id().did << ", path " << path << "\"}";
            HttpStream_Helper::SetCompleteResponse(hs, 400, oss.str(), "application/json");
            return -1;
        }
        if (err) {
            LOG_ERROR("Failed to determine access rights; dataset "FMTu64", path %s, err %d", ds->get_id().did, path.c_str(), err);
            std::ostringstream oss;
            oss << "{\"errMsg\":\"Failed to determine access rights; dataset " << ds->get_id().did << ", path " << path << "\"}";
            HttpStream_Helper::SetCompleteResponse(hs, 500, oss.str(), "application/json");
            return err;
        }
    }

    return 0;
}

int HttpSvc::Sn::Handler_rf_Helper::updateRemoteFileAccessControlDir(
                            dataset *ds, 
                            const ccd::RemoteFileAccessControlDirSpec &dir, 
                            bool add)
{

    int rv = VPL_OK;
#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)

    AccessControlLocker acLocker;
    std::string upath;
    std::string path;

    if(dir.has_name()){
        path = dir.name();
    }else if(dir.has_dir()){
        path = dir.dir();
    }else{
        LOG_ERROR("name and dir are empty");
        goto out;
    }

    //The input path should exist
    if(dir.has_name()){
        VPLFS_stat_t stat;
        rv = ds->stat_component(path, stat);
        if (rv) {
            LOG_ERROR("Failed to stat; dataset "FMTu64", path %s: err %d", ds->get_id().did, path.c_str(), rv);
            goto out;
        }
    }

    //expand alias to real path, similar to the logic of fs_dataset::check_access_right()
    if(dir.has_name()){
        //convert to real path
        rv = ds->get_component_path(path, upath);
        if (rv != VSSI_SUCCESS) {
            LOG_WARN("Failed to map component to path: component {%s}, error %d.", path.c_str(), rv);
            //We will keep name format in case the stat_component is pass but get_component_path is failed
            //ex. Libraries/Music
            upath = path;
        }else{
            //convert to windows format
            std::replace(upath.begin(), upath.end(), '/', '\\');
        }
    }else{
        upath = path;
        //convert to windows format
        std::replace(upath.begin(), upath.end(), '/', '\\');
    }

    // copy from _VPLFS__GetExtendedPath
    // collapse consecutive slashes into one
    {
        size_t pos = upath.find_first_of("\\");
        while (pos != std::string::npos) {
            size_t pos2 = upath.find_first_not_of("\\", pos);
            // position interval [pos, pos2) contains slashes
            size_t len = pos2 != std::string::npos ? pos2 - pos : upath.length() - pos;
            upath.replace(pos, len, 1, '\\');
            if (pos + 1 < upath.length())
                pos = upath.find_first_of("\\", pos + 1);
            else
                pos = std::string::npos;
        }
    }

    //get_component_path and then insert to access control list
    if(add){
        if(dir.is_allowed()){
            whiteList.insert(upath);
            LOG_ALWAYS("Add %s into whiteList", upath.c_str());
        }else{
            blackList.insert(upath);
            LOG_ALWAYS("Add %s into blackList", upath.c_str());
        }
    }else{
        if(dir.is_allowed()){
            whiteList.erase(upath);
            LOG_ALWAYS("Remove %s from whiteList", upath.c_str());
        }else{
            blackList.erase(upath);
            LOG_ALWAYS("Remove %s from blackList", upath.c_str());
        }
    }

out:
#endif
    return rv;
}

