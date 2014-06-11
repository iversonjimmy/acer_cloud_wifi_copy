#include "HttpSvc_Ccd_Handler_minidms.hpp"

#include "HttpSvc_Utils.hpp"
#include "HttpSvc_Ccd_Handler_Helper.hpp"

#include <HttpStream.hpp>
#include <log.h>

#include <vplu_types.h>
#include <vplex_http_util.hpp>
#include <vpl_net.h>
#include <vpl_fs.h>
#include <vpl_string.h>
#include <gvm_misc_utils.h>

#include <string>
#include <sstream>
#include <iomanip>
#include <vector>

#include "cslsha.h"
#include "pin_manager.hpp"
#include "ccdi.hpp"
#include "cache.h"

#if CCD_ENABLE_MEDIA_SERVER_AGENT
#include "MediaMetadataServer.hpp"
#endif

static void replaceAll(std::string &data, const std::string &A, const std::string &B)
{
    size_t r_index = 0;
    while (r_index < data.size() && (r_index=data.find(A, r_index)) != std::string::npos) {
        data.replace(r_index, A.size(), B);
        r_index += B.size();
    }
}

// ex. C/test/abc/def/xyz.mp3 >> C:\test\abc\def\xyz.mp3
static int to_local_filepath(std::string source_path, std::string& filepath)
{
    int rv = 0;
#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
    std::map<std::string, _VPLFS__DriveType> drive_map;
    bool is_valid = false;

    rv = _VPLFS__GetComputerDrives(drive_map);
    if (rv) {
        LOG_WARN("Failed to get computer drives, rv = %d", rv);
        goto end;
    }
    {
        std::map<std::string, _VPLFS__DriveType>::const_iterator it;
        for(it = drive_map.begin(); it != drive_map.end(); it++) {
            std::string prefix;
            prefix.assign(it->first.substr(0, 1));
            prefix.append("/");
            if (source_path.length() > 2 && source_path.substr(0, 2) == prefix) {
                filepath = it->first + source_path.substr(1);
                is_valid = true;
                break;
            }
        }
    }

    if (is_valid) {
        replaceAll(filepath, "/", "\\");
    } else {
        rv = -1;
    }

end:
#else
    filepath = source_path;
    replaceAll(filepath, "\\", "/");
    if (filepath.at(0) != '/') {
        filepath = "/" + filepath;
    }
#endif
    return rv;
}

static inline std::string convert_device_id_to_string(u64 value)
{
    char buf[17];
    VPL_snprintf(buf, ARRAY_SIZE_IN_BYTES(buf), "%016"PRIx64, value);
    return std::string(buf);
}

static int generate_url(u64 device_id, const std::string& object_id, VPLNet_port_t http_port, const std::string& options, std::string& url)
{
    int rv = 0;

    {
        char* base64_encoded_obj_id = NULL;
        rv = Util_EncodeBase64(object_id.c_str(), object_id.size(), &base64_encoded_obj_id, NULL, VPL_FALSE, VPL_TRUE);
        if (rv != 0) {
            LOG_ERROR("%s failed: %d", "Util_EncodeBase64", rv);
            goto out;
        }

        std::ostringstream oss;
        oss << "http://127.0.0.1:" << http_port << "/mm/"
            << convert_device_id_to_string(device_id)
            << "/c/" << base64_encoded_obj_id;
        if (!options.empty()) {
            oss << "/" << options;
        }
        url = oss.str();
        free(base64_encoded_obj_id);
    }

out:
    return rv;
}

static int generate_content_url(u64 device_id, const std::string& object_id, VPLNet_port_t http_port, const std::string& ext, std::string& url)
{
    int rv = 0;
    std::string options = "";

    if (!ext.empty()) {

        char* base64_encoded_obj_id = NULL;
        rv = Util_EncodeBase64(object_id.c_str(), object_id.size(), &base64_encoded_obj_id, NULL, VPL_FALSE, VPL_TRUE);
        if (rv != 0) {
            LOG_ERROR("%s failed: %d", "Util_EncodeBase64", rv);
            goto out;
        }
        ON_BLOCK_EXIT(free, base64_encoded_obj_id);

        std::ostringstream oss;
        oss << base64_encoded_obj_id << "." << ext;
        options = oss.str();
    }

out:
    return generate_url(device_id, object_id, http_port, options, url);
}

static const std::string get_extension(const std::string& path)
{
    std::string ext;
    size_t pos = path.find_last_of("/.");
    if ((pos != path.npos) && (path[pos] == '.')) {
        ext.assign(path, pos + 1, path.size());
    }
    return ext;
}

static void to_hash_string(const u8* hash, std::string& hash_string)
{
    std::ostringstream oss;
    // Write recorded hash
    hash_string.clear();
    oss << std::hex;
    for(int hashIndex = 0; hashIndex<CSL_SHA1_DIGESTSIZE; hashIndex++) {
        oss << std::setfill('0')
            << std::setw(2)
            << static_cast<unsigned>(hash[hashIndex]);
    }
    hash_string.assign(oss.str());
}

static int calculate_hash(const std::string str_in, std::string& hash_out)
{
    int rv;
    CSL_ShaContext hashState;
    u8 sha1digest[CSL_SHA1_DIGESTSIZE];

    rv = CSL_ResetSha(&hashState);
    if(rv != 0) {
        LOG_ERROR("CSL_ResetSha should never fail:%d", rv);
        return rv;
    }

    rv = CSL_InputSha(&hashState,
                      str_in.c_str(),
                      str_in.size());
    if(rv != 0) {
        LOG_ERROR("CSL_InputSha should never fail:%d", rv);
        return rv;
    }

    rv = CSL_ResultSha(&hashState, sha1digest);
    if(rv != 0) {
        LOG_ERROR("CSL_ResultSha should never fail:%d", rv);
        return rv;
    }

    to_hash_string(sha1digest, hash_out);
    return 0;
}

static int generate_object_id_from_path(std::string& path, std::string& object_id)
{
    return calculate_hash(path, object_id);
}

HttpSvc::Ccd::Handler_minidms::Handler_minidms(HttpStream *hs)
    : Handler(hs)
{
    LOG_TRACE("Handler_minidms[%p]: Created for HttpStream[%p]", this, hs);
}

HttpSvc::Ccd::Handler_minidms::~Handler_minidms()
{
    LOG_TRACE("Handler_minidms[%p]: Destroyed", this);
}

int HttpSvc::Ccd::Handler_minidms::_Run()
{
    int err = 0;

    LOG_TRACE("Handler_minidms[%p]: Run", this);

    err = Utils::CheckReqHeaders(hs);
    if (err) {
        // errmsg logged and response set by CheckReqHeaders()
        return 0;
    }

    const std::string&uri = hs->GetUri();

    VPLHttp_SplitUri(uri, uri_tokens);

    if (uri_tokens.size() < 2) {
        LOG_ERROR("Handler_minidms[%p]: Unexpected URI: uri %s", this, uri.c_str());
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }

    const std::string &uri_objtype = uri_tokens[1];

    if (objJumpTable.handlers.find(uri_objtype) == objJumpTable.handlers.end()) {
        LOG_ERROR("Handler_minidms[%p]: No handler: objtype %s", this, uri_objtype.c_str());
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }

    err = (this->*objJumpTable.handlers[uri_objtype])();

    return err;
}

int HttpSvc::Ccd::Handler_minidms::process_protocolinfo()
{
    int err = 0;
    std::string response;
    std::ostringstream ostream;

    /*
     *  About the definition of following please refers to:
     *  - http://www.ctbg.acer.com/wiki/index.php/CCD_DLNA_DMS_Support#get_protocolInfo_data
     *  - http://www.ctbg.acer.com/wiki/index.php/CCD_DLNA_DMS_Support#.40protocolInfo
     */
    ostream << "{"
            << "\"protocol\" : \"http-get\","
            << "\"network\" : \"*\","
            << "\"paramOp\" : \"DLNA.ORG_OP=01\","
            << "\"paramFlags\" : \"DLNA.ORG_FLAGS=00100000000000000000000000000000\""
            << "}";

    response = ostream.str();
    Utils::SetCompleteResponse(hs, 200, response, Utils::Mime_ApplicationJson);

    return err;
}

int HttpSvc::Ccd::Handler_minidms::process_pin()
{
    int err = 0;
    const std::string &method = hs->GetMethod();

    if (pinMethodJumpTable.handlers.find(method) != pinMethodJumpTable.handlers.end()) {
        err = (this->*pinMethodJumpTable.handlers[method])();

    } else {
        LOG_ERROR("Handler_minidms[%p]: Unsupported method %s", this, method.c_str());
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }
    return err;
}

int HttpSvc::Ccd::Handler_minidms::process_pin_get()
{
    int err = 0;
    static const std::string prefix = "/minidms/pin/";
    if (uri_tokens.size() < 3 || uri_tokens[2].empty()) {
        LOG_ERROR("Handler_minidms[%p]: Invalid http request. (URI: %s)", this, hs->GetUri().c_str());
        Utils::SetCompleteResponse(hs, 400);
        goto end;
    }

    {
        const std::string path_query = hs->GetUri().substr(prefix.size());
        std::string path;
        std::string filepath;
        std::string file_ext;
        std::string url;
        PinnedMediaItem pinned_media_item;
        VPLNet_port_t proxy_agent_http_port;

        err = VPLHttp_DecodeUri(path_query, path);
        if (err) {
            LOG_ERROR("Handler_minidms[%p]: VPLHttp_DecodeUri() failed, rv = %d", this, err);
            Utils::SetCompleteResponse(hs, 400);
            err = 0;
            goto end;
        }

        err = to_local_filepath(path, filepath);
        if (err) {
            LOG_ERROR("Handler_minidms[%p]: Failed to translate to filepath, path = %s, rv = %d", this, path.c_str(), err);
            Utils::SetCompleteResponse(hs, 400);
            err = 0;
            goto end;
        }

        err = PinnedMediaManager_GetPinnedMediaItem(filepath, /*OUT*/pinned_media_item);
        if (err == CCD_ERROR_NOT_FOUND) {
            LOG_ERROR("Handler_minidms[%p]: No such pinned media item in database, path = %s.", this, path.c_str());
            Utils::SetCompleteResponse(hs, 404, "{\"errMsg\":\"Media not found.\"}", Utils::Mime_ApplicationJson);
            err = 0;
            goto end;
        } else if (err) {
            LOG_ERROR("Handler_minidms[%p]: PinnedMediaManager_GetPinnedMediaItem() failed, path = %s, rv = %d", this, path.c_str(), err);
            Utils::SetCompleteResponse(hs, 500);
            err = 0;
            goto end;
        }
        {
            ccd::GetSystemStateInput request;
            ccd::GetSystemStateOutput response;
            request.set_get_network_info(true);
            err = CCDIGetSystemState(request, response);
            if (err != 0) {
                LOG_ERROR("CCDIGetSystemState failed: %d", err);
                Utils::SetCompleteResponse(hs, 500);
                err = 0;
                goto end;
            }
            proxy_agent_http_port = response.network_info().proxy_agent_port();
        }

        file_ext = get_extension(filepath);
        err = generate_content_url(pinned_media_item.device_id, pinned_media_item.object_id, proxy_agent_http_port, file_ext, url);
        if (err) {
            LOG_ERROR("Handler_minidms[%p]: Failed to generate url, rv = %d", this, err);
            Utils::SetCompleteResponse(hs, 500);
            err = 0;
            goto end;
        }
        {
            std::ostringstream oss;
            oss << "{\"url\":\"" << url << "\"}";
            Utils::SetCompleteResponse(hs, 200, oss.str(), Utils::Mime_ApplicationJson);
        }
    }

end:
    return err;
}

int HttpSvc::Ccd::Handler_minidms::process_pin_put()
{
    int rv = 0;
    int err = 0;
    static const std::string prefix = "/minidms/pin/";
    if (uri_tokens.size() < 3 || uri_tokens[2].empty()) {
        LOG_ERROR("Handler_minidms[%p]: Invalid http request. (URI: %s)", this, hs->GetUri().c_str());
        Utils::SetCompleteResponse(hs, 400);
        goto end;
    }

    {
        const std::string path_query = hs->GetUri().substr(prefix.size());
        std::string path;
        std::string filepath;
        std::string object_id;
        u64 device_id;
        u64 local_device_id;
        bool is_cloud_pc = false;
        VPLFS_stat_t file_stat;

        err = VPLHttp_DecodeUri(path_query, path);
        if (err) {
            LOG_ERROR("Handler_minidms[%p]: VPLHttp_DecodeUri() failed, rv = %d", this, err);
            Utils::SetCompleteResponse(hs, 400);
            goto end;
        }

        err = to_local_filepath(path, filepath);
        if (err) {
            LOG_ERROR("Handler_minidms[%p]: Failed to translate to filepath, path = %s, rv = %d", this, path.c_str(), err);
            Utils::SetCompleteResponse(hs, 400);
            goto end;
        }

        err = VPLFS_Stat(filepath.c_str(), &file_stat);
        if (err) {
            LOG_ERROR("Handler_minidms[%p]: The file is not available. rv = %d, filepath = %s", this, err, filepath.c_str());
            Utils::SetCompleteResponse(hs, 404);
            goto end;
        }

        // FIXME: the code below assumes a single wait is sufficient to get the whole content.
        //        this is generally not the case but usually sufficient.
        {
            char buf[4096];
            char *reqBody = NULL;
            {
                ssize_t bytes = hs->Read(buf, sizeof(buf) - 1);  // subtract 1 for EOL
                if (bytes < 0) {
                    LOG_ERROR("Handler_minidms[%p]: Failed to read from HttpStream[%p]: err "FMT_ssize_t, this, hs, bytes);
                    Utils::SetCompleteResponse(hs, 500, "{\"errMsg\":\"Failed to read from HttpStream\"}", Utils::Mime_ApplicationJson);
                    goto end;
                }
                buf[bytes] = '\0';
                char *boundary = strstr(buf, "\r\n\r\n");  // find header-body boundary
                if (!boundary) {
                    // header-body boundary not found - bad request
                    LOG_ERROR("Handler_minidms[%p]: Failed to find header-body boundary in request", this);
                    Utils::SetCompleteResponse(hs, 400, "{\"errMsg\":\"Failed to find header-body boundary in request\"}", Utils::Mime_ApplicationJson);
                    goto end;
                }
                reqBody = boundary + 4;
            }

            LOG_DEBUG("Handler_minidms[%p]: Http Request Body: %s", this, reqBody);

            cJSON2 *json_async = cJSON2_Parse(reqBody);
            if (JSON_getInt64(json_async, "sourceDeviceId", device_id)) {
                LOG_ERROR("Handler_minidms[%p]: No sourceDeviceId in http request.", this);
                Utils::SetCompleteResponse(hs, 400);
                goto end;
            }

            if (JSON_getString(json_async, "sourceObjectId", object_id)) {
                LOG_INFO("Handler_minidms[%p]: No sourceObjectId in http request.", this);
            }
        }
        {
            bool is_valid = false;
            ccd::ListLinkedDevicesInput input;
            ccd::ListLinkedDevicesOutput output;
            input.set_only_use_cache(true);
            input.set_user_id(hs->GetUserId());
            err = CCDIListLinkedDevices(input, output);
            if (err) {
                LOG_ERROR("Handler_minidms[%p]: Failed to list devices, rv = %d", this, err);
                Utils::SetCompleteResponse(hs, 500);
                goto end;
            }

            for (int i = 0; i < output.linked_devices_size(); i++) {
                if (device_id == output.linked_devices(i).deviceid()) {
                    is_valid = true;
                    LOG_DEBUG("Handler_minidms[%p]: Found device in linked devices. (device_id = "FMTu64")", this, device_id);
                    break;
                }
            }

            if (!is_valid) {
                LOG_ERROR("Handler_minidms[%p]: The input device_id is not in linked devices, device_id = "FMTu64, this, device_id);
                Utils::SetCompleteResponse(hs, 400);
                goto end;
            }
        }
        {
            ccd::GetSystemStateInput request;
            ccd::GetSystemStateOutput response;
            request.set_get_device_id(true);
            err = CCDIGetSystemState(request, response);
            if (err != 0) {
                LOG_ERROR("CCDIGetSystemState failed: %d", err);
                Utils::SetCompleteResponse(hs, 500);
                goto end;
            }
            local_device_id = response.device_id();
        }
        {
            ccd::ListUserStorageInput input;
            ccd::ListUserStorageOutput output;
            input.set_only_use_cache(true);
            input.set_user_id(hs->GetUserId());
            err = CCDIListUserStorage(input, output);
            if (err) {
                LOG_ERROR("Handler_minidms[%p]: CCDIListUserStorage() failed, rv = %d", this, err);
                Utils::SetCompleteResponse(hs, 500);
                goto end;
            }

            for (int i = 0; i < output.user_storage_size(); i++) {
                if (output.user_storage(i).storageclusterid() == local_device_id &&
                    output.user_storage(i).featuremediaserverenabled()) {
                    LOG_WARN("Handler_minidms[%p]: Is cloud pc.", this);
                    is_cloud_pc = true;
                    break;
                }
            }
        }

        if (object_id.empty()) {
            err = generate_object_id_from_path(filepath, object_id);
            if (err) {
                LOG_ERROR("Handler_minidms[%p]: Failed to generate object_id, rv = %d", this, err);
                Utils::SetCompleteResponse(hs, 500);
                goto end;
            }
            LOG_INFO("Handler_minidms[%p]: Generate object_id: %s", this, object_id.c_str());

        } else if (is_cloud_pc) { // Look up MSA database
            LOG_WARN("Handler_minidms[%p]: App should use CCDIMSAGetContentURL() to retrieve content URL.", this);
#if CCD_ENABLE_MEDIA_SERVER_AGENT
            bool is_in_msa_db = false;
            media_metadata::CatalogType_t catalog_types[] = {
                    media_metadata::MM_CATALOG_MUSIC,
                    media_metadata::MM_CATALOG_PHOTO,
                    media_metadata::MM_CATALOG_VIDEO};
            const int size_of_catalog_types = ARRAY_ELEMENT_COUNT(catalog_types);
            for (int i = 0; i < size_of_catalog_types; i++) {
                ccd::BeginCatalogInput begin_catalog_input;
                begin_catalog_input.set_catalog_type(catalog_types[i]);
                err = MSABeginCatalog(begin_catalog_input);
                if (err) {
                    LOG_ERROR("Handler_minidms[%p]: Failed to begin catalog, catalog_types:%d, rv = %d", this, catalog_types[i], err);
                    break;
                }

                media_metadata::ListCollectionsOutput list_collections_output;
                err = MSAListCollections(list_collections_output);
                if (err) {
                    LOG_ERROR("Handler_minidms[%p]: Failed to get collections from MSA db, rv = %d", this, err);
                    break;
                } else {
                    for (int j = 0; j < list_collections_output.collection_id_size(); j++) {
                        media_metadata::GetObjectMetadataOutput get_obj_metadata_output;

                        err = MSAGetContentObjectMetadata(object_id,
                                                          list_collections_output.collection_id(j),
                                                          catalog_types[i],
                                                          get_obj_metadata_output);
                        if (err == MM_ERR_NOT_FOUND || !get_obj_metadata_output.has_absolute_path()) {
                            LOG_DEBUG("Handler_minidms[%p]: Not in this collection. collection_id: %s",
                                      this,
                                      list_collections_output.collection_id(j).c_str());
                        } else if(err) {
                            LOG_ERROR("Handler_minidms[%p]: Failed to get content object, rv = %d, object_id = %s, collection_id = %s",
                                      this,
                                      err,
                                      object_id.c_str(),
                                      list_collections_output.collection_id(j).c_str());
                            break;
                        } else {
                            LOG_DEBUG("Handler_minidms[%p]: Found in MSA db. object_id: %s",
                                      this,
                                      object_id.c_str());
                            is_in_msa_db = true;
                            break;
                        }
                    }
                }

                ccd::EndCatalogInput end_catalog_input;
                end_catalog_input.set_catalog_type(catalog_types[i]);
                int tmp_err = MSAEndCatalog(end_catalog_input);
                if (tmp_err) {
                    LOG_ERROR("Handler_minidms[%p]: Failed to end catalog, catalog_types:%d, rv = %d", this, catalog_types[i], tmp_err);
                    if (err == 0) {
                        err = tmp_err;
                    }
                    break;
                }

                if (is_in_msa_db) {
                    break;
                }
            }

            if (err) {
                LOG_ERROR("Handler_minidms[%p]: Got error while looking up MSA db, object_id: %s, rv = %d", this, object_id.c_str(), err);
                Utils::SetCompleteResponse(hs, 500);
                goto end;
            }

            if (!is_in_msa_db) {
                LOG_ERROR("Handler_minidms[%p]: Invalid object_id: %s", this, object_id.c_str());
                Utils::SetCompleteResponse(hs, 404);
                goto end;
            }
#endif
        } else if (!is_cloud_pc) { // Look up MCA database
            bool is_in_mca_db = false;

            if (!is_in_mca_db) {
                ccd::MCAQueryMetadataObjectsInput input;
                ccd::MCAQueryMetadataObjectsOutput output;
                std::string query_string;
                std::stringstream oss;
                oss << "photos_albums_relation.object_id='" << object_id << "'";
                query_string = oss.str();
                input.set_cloud_device_id(device_id);
                input.set_filter_field(media_metadata::MCA_MDQUERY_PHOTOITEM);
                input.set_search_field(query_string.c_str());
                err = CCDIMCAQueryMetadataObjects(input, output);
                if (err) {
                    LOG_ERROR("Handler_minidms[%p]: CCDIMCAQueryMetadataObjects() failed, rv = %d", this, err);
                    Utils::SetCompleteResponse(hs, 500);
                    goto end;
                }

                if (output.content_objects_size() > 0) {
                    LOG_INFO("Handler_minidms[%p]: Found photo item with object_id: %s", this, object_id.c_str());
                    is_in_mca_db = true;
                }
            }

            if (!is_in_mca_db) {
                ccd::MCAQueryMetadataObjectsInput input;
                ccd::MCAQueryMetadataObjectsOutput output;
                std::string query_string;
                std::stringstream oss;
                oss << "object_id='" << object_id << "'";
                query_string = oss.str();
                input.set_cloud_device_id(device_id);
                input.set_filter_field(media_metadata::MCA_MDQUERY_MUSICTRACK);
                input.set_search_field(query_string.c_str());
                err = CCDIMCAQueryMetadataObjects(input, output);
                if (err) {
                    LOG_ERROR("Handler_minidms[%p]: CCDIMCAQueryMetadataObjects() failed, rv = %d", this, err);
                    Utils::SetCompleteResponse(hs, 500);
                    goto end;
                }

                if (output.content_objects_size() > 0) {
                    LOG_INFO("Handler_minidms[%p]: Found music item with object_id: %s", this, object_id.c_str());
                    is_in_mca_db = true;
                }
            }

            if (!is_in_mca_db) {
                ccd::MCAQueryMetadataObjectsInput input;
                ccd::MCAQueryMetadataObjectsOutput output;
                std::string query_string;
                std::stringstream oss;
                oss << "object_id='" << object_id << "'";
                query_string = oss.str();
                input.set_cloud_device_id(device_id);
                input.set_filter_field(media_metadata::MCA_MDQUERY_VIDEOITEM);
                input.set_search_field(query_string.c_str());
                err = CCDIMCAQueryMetadataObjects(input, output);
                if (err) {
                    LOG_ERROR("Handler_minidms[%p]: CCDIMCAQueryMetadataObjects() failed, rv = %d", this, err);
                    Utils::SetCompleteResponse(hs, 500);
                    goto end;
                }

                if (output.content_objects_size() > 0) {
                    LOG_INFO("Handler_minidms[%p]: Found video item with object_id: %s", this, object_id.c_str());
                    is_in_mca_db = true;
                }
            }

            if (!is_in_mca_db) {
                LOG_ERROR("Handler_minidms[%p]: Invalid object_id: %s", this, object_id.c_str());
                Utils::SetCompleteResponse(hs, 404);
                goto end;
            }
        }

        err = PinnedMediaManager_InsertOrUpdatePinnedMediaItem(filepath, object_id, device_id);
        if (err) {
            LOG_ERROR("Handler_minidms[%p]: Failed to insert/update, rv = %d", this, err);
            Utils::SetCompleteResponse(hs, 500);
            goto end;
        }

        Utils::SetCompleteResponse(hs, 200, "{ \"result\":\"success\"}", Utils::Mime_ApplicationJson);
    }

end:
    return rv;
}

int HttpSvc::Ccd::Handler_minidms::process_pin_delete()
{
    int err = 0;
    static const std::string prefix = "/minidms/pin/";
    if (uri_tokens.size() < 3 || uri_tokens[2].empty()) {
        LOG_ERROR("Handler_minidms[%p]: Invalid http request. (URI: %s)", this, hs->GetUri().c_str());
        Utils::SetCompleteResponse(hs, 400);
        goto end;
    }

    {
        const std::string path_query = hs->GetUri().substr(prefix.size());
        std::string path;
        std::string filepath;

        err = VPLHttp_DecodeUri(path_query, path);
        if (err) {
            LOG_ERROR("Handler_minidms[%p]: VPLHttp_DecodeUri() failed, rv = %d", this, err);
            Utils::SetCompleteResponse(hs, 400);
            err = 0;
            goto end;
        }

        err = to_local_filepath(path, filepath);
        if (err) {
            LOG_ERROR("Handler_minidms[%p]: Failed to translate to filepath, path = %s, rv = %d", this, path.c_str(), err);
            Utils::SetCompleteResponse(hs, 400);
            err = 0;
            goto end;
        }

        err = PinnedMediaManager_RemovePinnedMediaItem(filepath);
        if (err) {
            LOG_ERROR("Handler_minidms[%p]: Failed to remove pinned media item by path, path = %s, rv = %d", this, path.c_str(), err);
            Utils::SetCompleteResponse(hs, 404);
            err = 0;
            goto end;
        }

        Utils::SetCompleteResponse(hs, 200, "{ \"result\":\"success\"}", Utils::Mime_ApplicationJson);
    }

end:
    return err;
}


int HttpSvc::Ccd::Handler_minidms::process_deviceinfo()
{
    int err = 0;
    u64 device_id;
    if (uri_tokens.size() < 3 || uri_tokens[2].empty()) {
        LOG_ERROR("Handler_minidms[%p]: Invalid http request. (URI: %s)", this, hs->GetUri().c_str());
        Utils::SetCompleteResponse(hs, 400);
        goto end;
    }

    {
        errno = 0;
        device_id = VPLConv_strToU64(uri_tokens[2].c_str(), NULL, 10);
        if (errno != 0) {
            LOG_ERROR("Failed to parse content id \"%s\"", uri_tokens[2].c_str());
            Utils::SetCompleteResponse(hs, 400);
            err = 0;
            goto end;
        }
    }
    {
        CacheAutoLock lock;
        err = lock.LockForRead();
        if(err) {
            LOG_WARN("Unable to obtain cache lock:%d", err);
            Utils::SetCompleteResponse(hs, 500);
            err = 0;
            goto end;
        }
        {
            bool is_valid = false;
            ccd::ListLinkedDevicesInput input;
            ccd::ListLinkedDevicesOutput output;
            input.set_user_id(hs->GetUserId());
            input.set_storage_nodes_only(true);
            input.set_only_use_cache(true);
            err = CCDIListLinkedDevices(input, output);
            if (err) {
                LOG_ERROR("CCDIListLinkedDevices for user("FMTu64") failed %d", hs->GetUserId(), err);
                Utils::SetCompleteResponse(hs, 400);
                err = 0;
                goto end;
            }

            for (int i = 0; i < output.linked_devices_size(); i++) {
                if (output.linked_devices(i).deviceid() == device_id) {
                    LOG_DEBUG("CloudPC deviceId is "FMTu64, device_id);
                    is_valid = true;
                }
            }

            if (!is_valid) {
                LOG_ERROR("Can not find CloudPC with device_id: "FMTu64, device_id);
                Utils::SetCompleteResponse(hs, 404);
                err = 0;
                goto end;
            }
        }
    }

    // Prepare for forwarding to storage-node-side ccd.
    {
        hs->SetDeviceId(device_id);
        hs->RemoveReqHeader(Utils::HttpHeader_ac_serviceTicket);
        hs->RemoveReqHeader(Utils::HttpHeader_ac_sessionHandle);
    }

    err = Handler_Helper::ForwardToServerCcd(hs);
    if (err) {
        LOG_ERROR("Handler_minidms[%p]: ForwardToServerCcd() failed, rv = %d", this, err);
        Utils::SetCompleteResponse(hs, 500);
        err = 0;
        goto end;
    }

end:
    return err;
}


HttpSvc::Ccd::Handler_minidms::ObjJumpTable HttpSvc::Ccd::Handler_minidms::objJumpTable;

HttpSvc::Ccd::Handler_minidms::ObjJumpTable::ObjJumpTable()
{
    handlers["protocolinfo"] = &Handler_minidms::process_protocolinfo;
    handlers["pin"] = &Handler_minidms::process_pin;
    handlers["deviceinfo"] = &Handler_minidms::process_deviceinfo;
}

HttpSvc::Ccd::Handler_minidms::PINMethodJumpTable HttpSvc::Ccd::Handler_minidms::pinMethodJumpTable;

HttpSvc::Ccd::Handler_minidms::PINMethodJumpTable::PINMethodJumpTable()
{
    handlers["GET"] = &Handler_minidms::process_pin_get;
    handlers["PUT"] = &Handler_minidms::process_pin_put;
    handlers["DELETE"] = &Handler_minidms::process_pin_delete;
}
