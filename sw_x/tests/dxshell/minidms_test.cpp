/*
 *  Copyright 2013 Acer Cloud Technology Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF
 *  ACER CLOUD TECHNOLOGY INC.
 *
 */

#include "minidms_test.hpp"

#include <ccdi.hpp>
#include <log.h>
#include <vpl_fs.h>
#include <vplu_types.h>
#include <vplex_strings.h>
#include <vplex_http_util.hpp>

#include <string>
#include <sstream>
#include <iostream>
#include <map>

#include "dx_common.h"
#include "ccd_utils.hpp"
#include "HttpAgent.hpp"
#include "cJSON2.h"
#include "media_metadata_types.pb.h"

#define RF_HEADER_USERID         "x-ac-userId"
#define RF_HEADER_SESSION_HANDLE "x-ac-sessionHandle"
#define RF_HEADER_SERVICE_TICKET "x-ac-serviceTicket"

#define OS_WINDOWS_RT "WindowsRT"
#define OS_WINDOWS    "Windows"

static int JSON_getString(cJSON2* node, const char* attributeName, std::string& value);
static int JSON_getJSONObject(cJSON2* node, const char* attributeName, cJSON2** value);
static int check_json_format(std::string& json, const std::string& attributeName = std::string());
#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
static void replace_all(std::string &data, const std::string &oldsub, const std::string &newsub);
#endif
static int to_uri_path(std::string filepath, std::string& uri_path);

int minidms_help(int argc, const char* argv[]);

int minidms_help(int argc, const char* argv[])
{
    bool printAll = strcmp(argv[0], "MiniDMS") == 0 || strcmp(argv[0], "Help") == 0;
    if (printAll || strcmp(argv[0], "ProtocolInfo") == 0) {
        std::cout << "MiniDMS ProtocolInfo <MIME-TYPE>" << std::endl;
    }
    if (printAll || strcmp(argv[0], "SampleURLMetadata") == 0) {
        std::cout << "MiniDMS SampleURLMetadata <ProtocolInfo> <URL> [<AlbumArtURL>]" << std::endl;
    }
    if (printAll || strcmp(argv[0], "MSAContentURL") == 0) {
        std::cout << "MiniDMS MSAContentURL <CollectionID> <ObjectID> <MediaType>" << std::endl;
        std::cout << "  MediaType = 1 - music | 2 - photo | 3 - video" << std::endl;
    }
    if (printAll || strcmp(argv[0], "AddPINItem") == 0) {
        std::cout << "MiniDMS AddPINItem <Path> <Source Device ID> [<Source Object ID>]" << std::endl;
    }
    if (printAll || strcmp(argv[0], "RemovePINItem") == 0) {
        std::cout << "MiniDMS RemovePINItem <Path>" << std::endl;
    }
    if (printAll || strcmp(argv[0], "GetPINItem") == 0) {
        std::cout << "MiniDMS GetPINItem <Path>" << std::endl;
    }
    if (printAll || strcmp(argv[0], "MediaServerInfo") == 0) {
        std::cout << "MiniDMS MediaServerInfo <cloud_pc_device_id>" << std::endl;
    }
    return 0;
}

static int minidms_help_with_response(int argc, const char* argv[], std::string &response) {
    return minidms_help(argc, argv);
}

static bool inline is_windows(const std::string &os)
{
    if (os.find(OS_WINDOWS) != std::string::npos ||
        os.find(OS_WINDOWS_RT) != std::string::npos) {
        return true;
    }
    return false;
}

static int JSON_getString(cJSON2* node, const char* attributeName, std::string& value)
{
    int rv = 0;

    if (node==NULL) {
        LOG_ERROR("JSON_getString, Node is null");
        return -1;
    }

    cJSON2* json_value;
    if (JSON_getJSONObject(node, attributeName, &json_value)) {
        LOG_INFO("JSON_getString, Can\' find - %s", attributeName);
        return -1;
    }

    value = json_value->valuestring;
    return rv;
}

static int JSON_getJSONObject(cJSON2* node, const char* attributeName, cJSON2** value)
{
    int rv = 0;
    if (node==NULL) {
        LOG_ERROR("JSON_getJSONObject: Node is null");
        return -1;
    }

    cJSON2* json_value;
    json_value = cJSON2_GetObjectItem(node, attributeName);
    if (json_value==NULL) {
        return -1;
    }
    *value = json_value;
    return rv;
}

static int check_json_format(std::string& json, const std::string& attributeName)
{
    cJSON2 *jsonResponse = cJSON2_Parse(json.c_str());
    if (!jsonResponse) {
        LOG_ERROR("Invalid root json data.");
        return -1;
    }

    cJSON2 *jsonAttribute = NULL;

    if (attributeName != "") {
        int ret = JSON_getJSONObject(jsonResponse, attributeName.c_str(), &jsonAttribute);

        if(ret != 0){
            LOG_ERROR("Can not find %s!", attributeName.c_str());
            cJSON2_Delete(jsonResponse);
            return -1;
        }
    }


    cJSON2_Delete(jsonResponse);
    jsonResponse = NULL;
    return 0;
}

static int minidms_protocolinfo(int argc, const char* argv[], std::string& response)
{
    int rv = 0;
    u64 user_id = 0;
    if (argc < 2) {
        minidms_help(argc, argv);
        return -1;
    }

    rv = getUserIdBasic(&user_id);
    if (rv != 0) {
        LOG_ERROR("Failed to get user_id.");
        return -1;
    }

    return minidms_protocolinfo(user_id, argv[1], response);
}

static void replace_all(std::string &data, const std::string &oldsub, const std::string &newsub)
{
    u32 r_index = 0;
    while (r_index < data.size() && (r_index=data.find(oldsub, r_index)) != std::string::npos) {
        data.replace(r_index, oldsub.size(), newsub);
        r_index += newsub.size();
    }
}

static int to_uri_path(std::string filepath, std::string& uri_path)
{
    TargetDevice* target = NULL;
    target = getTargetDevice();

    uri_path = filepath;
    replace_all(uri_path, "\\", "/");

    if (is_windows(target->getOsVersion())) {
        if (uri_path[1] == ':') {
            uri_path.erase(1, 1);
        } else {
            LOG_ERROR("Bad filepath: %s", filepath.c_str());
            return -1;
        }
    } else {
        if (uri_path[0] == '/') {
            uri_path = uri_path.substr(1);
        } else {
            LOG_ERROR("Bad filepath: %s", filepath.c_str());
            return -1;
        }
    }

    VPLHttp_UrlEncoding(uri_path, "/");

    return 0;
}

int minidms_protocolinfo(u64 userId, std::string mimetype, std::string& response)
{
    int rv = 0;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::string url;
    HttpAgent *agent = NULL;

    agent = getHttpAgent();
    if (!agent) {
        LOG_ERROR("Fail to get http agent");
        rv = -1;
        goto end;
    }

    rv = getLocalHttpInfo(userId, base_url, service_ticket, session_handle);
    if (rv < 0) {
        LOG_ERROR("getLocalHttpInfo returned %d.", rv);
        goto end;
    }

    {
        char *reqUserId = VPLString_MallocAndSprintf(RF_HEADER_USERID": "FMTu64, userId);
        if (reqUserId == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            free(reqUserId);
            rv = -1;
            goto end;
        }
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            free(reqUserId);
            free(reqHandle);
            rv = -1;
            goto end;
        }

        const char* headers[] = {
            reqUserId,
            reqHandle,
            reqTicket,
        };

        url = base_url + "/minidms/protocolinfo";

        rv = agent->get_response_back(url, 0, 0, headers, 3, response);

        // free header resources
        free(reqUserId);
        free(reqHandle);
        free(reqTicket);

        if (rv < 0) {
            LOG_ERROR("agent->get_response_back(\"%s\") returned %d", url.c_str(), rv);
            goto end;
        }
    }
    {
        rv = check_json_format(response);
        if (rv < 0) {
            LOG_ERROR("Not valid JSON response: %s", response.c_str());
            goto end;
        }
    }
    {
        std::string str_protocol;
        std::string str_network;
        std::string str_paramOp;
        std::string str_paramFlags;
        std::ostringstream oss;
        std::string protocolInfo;

        cJSON2* json_response = cJSON2_Parse(response.c_str());
        if (json_response == NULL) {
            LOG_ERROR("cJSON2_Parse error!");
            return -1;
        }

        if (JSON_getString(json_response, "protocol", str_protocol) ){
            LOG_ERROR("Can not find \"protocol\" in JSON response: %s", response.c_str());
            return -1;
        }

        if (JSON_getString(json_response, "network", str_network) ){
            LOG_ERROR("Can not find \"network\" in JSON response: %s", response.c_str());
            return -1;
        }

        if (JSON_getString(json_response, "paramOp", str_paramOp) ){
            LOG_ERROR("Can not find \"paramOp\" in JSON response: %s", response.c_str());
            return -1;
        }

        if (JSON_getString(json_response, "paramFlags", str_paramFlags) ){
            LOG_ERROR("Can not find \"paramFlags\" in JSON response: %s", response.c_str());
            return -1;
        }

        oss << str_protocol << ":"
            << str_network << ":"
            << mimetype << ":"
            << str_paramOp << ";" << str_paramFlags;

        protocolInfo = oss.str();
        oss.clear();

        LOG_ALWAYS("ProtocolInfo: %s", protocolInfo.c_str());
    }
end:
    return rv;
}

static int minidms_samplemetadata(int argc, const char* argv[], std::string& response)
{
    int rv = 0;
    u64 user_id = 0;
    if (argc < 3 || argc > 4) {
        minidms_help(argc, argv);
        return -1;
    }

    rv = getUserIdBasic(&user_id);
    if (rv != 0) {
        LOG_ERROR("Failed to get user_id.");
        return -1;
    }

    if (argc == 3) {
        return minidms_samplemetadata(user_id, argv[1], argv[2], "", response);
    }

    return minidms_samplemetadata(user_id, argv[1], argv[2], argv[3], response);
}

int minidms_samplemetadata(u64 userId, std::string protocolInfo, std::string contentUrl, std::string albumartUrl, std::string& response)
{
    std::ostringstream oss;
    std::string url_metadata;

    oss << "<DIDL-Lite xmlns:dc=\"http://purl.org/dc/elements/1.1/\" xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\" xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\">"
        << "<item>"
        << "<res protocolInfo=\"" << protocolInfo << "\">" << contentUrl << "</res>"
        << "<upnp:albumArtURI>" << albumartUrl << "</upnp:albumArtURI>"
        << "</item>"
        << "</DIDL-Lite>";

    url_metadata = oss.str();

    LOG_ALWAYS("CurrentURLMetadata: %s", url_metadata.c_str());

    oss.clear();
    return 0;
}

static int minidms_msa_contenturl(int argc, const char* argv[], std::string& response)
{
    int rv = 0;
    u64 user_id = 0;
    media_metadata::CatalogType_t catalog_type;

    if (argc != 4) {
        minidms_help(argc, argv);
        return -1;
    }

    rv = getUserIdBasic(&user_id);
    if (rv != 0) {
        LOG_ERROR("Failed to get user_id.");
        return -1;
    }

    if (atoi(argv[3]) == 1) {
        catalog_type = media_metadata::MM_CATALOG_MUSIC;
    } else if (atoi(argv[3]) == 2) {
        catalog_type = media_metadata::MM_CATALOG_PHOTO;
    } else if (atoi(argv[3]) == 3) {
        catalog_type = media_metadata::MM_CATALOG_VIDEO;
    } else {
        minidms_help(argc, argv);
        return -1;
    }

    return minidms_msa_contenturl(user_id, argv[1], argv[2], catalog_type, response);
}

int minidms_msa_contenturl(u64 userId, std::string collectionId, std::string objectId, media_metadata::CatalogType_t type, std::string& response)
{
    int rv = 0;
    {
        ccd::MSAGetContentURLInput input;
        ccd::MSAGetContentURLOutput output;
        input.set_user_id(userId);
        input.set_collection_id(collectionId);
        input.set_object_id(objectId);
        input.set_catalog_type(type);
        rv = CCDIMSAGetContentURL(input, output);
        if (rv < 0) {
            LOG_ERROR("Failed to get content_url: %d", rv);
            goto end;
        }
        LOG_ALWAYS("Content URL: %s", output.url().c_str());
    }
    {
        ccd::MSAGetContentURLInput input;
        ccd::MSAGetContentURLOutput output;
        input.set_user_id(userId);
        input.set_collection_id(collectionId);
        input.set_object_id(objectId);
        input.set_catalog_type(type);
        input.set_is_thumb(true);
        rv = CCDIMSAGetContentURL(input, output);
        if (rv < 0) {
            LOG_ERROR("Failed to get thumbnail_url: %d", rv);
            goto end;
        }
        LOG_ALWAYS("Thumbnail URL: %s", output.url().c_str());
    }

end:
    return rv;
}

static int minidms_add_pinitem(int argc, const char* argv[], std::string& response)
{
    int rv = 0;
    u64 user_id = 0;

    if (argc < 3) {
        minidms_help(argc, argv);
        return -1;
    }

    rv = getUserIdBasic(&user_id);
    if (rv != 0) {
        LOG_ERROR("Failed to get user_id.");
        return -1;
    }

    if (argc == 3) {
        return minidms_add_pinitem(user_id, argv[1], argv[2], "", response);
    }

    return minidms_add_pinitem(user_id, argv[1], argv[2], argv[3], response);
}

int minidms_add_pinitem(u64 userId, std::string filepath, std::string source_device_id, std::string object_id, std::string& response)
{
    int rv = 0;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::string url;
    std::string uri_path;
    HttpAgent* agent = NULL;

    agent = getHttpAgent();
    if (!agent) {
        LOG_ERROR("Fail to get http agent");
        rv = -1;
        goto end;
    }

    rv = getLocalHttpInfo(userId, base_url, service_ticket, session_handle);
    if (rv < 0) {
        LOG_ERROR("getLocalHttpInfo returned %d.", rv);
        goto end;
    }

    {
        char *reqUserId = VPLString_MallocAndSprintf(RF_HEADER_USERID": "FMTu64, userId);
        if (reqUserId == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            free(reqUserId);
            rv = -1;
            goto end;
        }
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            free(reqUserId);
            free(reqHandle);
            rv = -1;
            goto end;
        }

        const char* headers[] = {
            reqUserId,
            reqHandle,
            reqTicket,
        };

        std::ostringstream oss;
        std::string payload;

        if (!object_id.empty()) {
            oss << "{ \"sourceDeviceId\" : " << source_device_id << ", "
                << "\"sourceObjectId\" : \"" << object_id << "\" }";
        } else {
            oss << "{ \"sourceDeviceId\" : " << source_device_id << " }";
        }

        payload = oss.str();

        rv = to_uri_path(filepath, uri_path);

        if (rv < 0) {
            LOG_ERROR("to_uri_path() failed, filepath = %s, rv = %d", filepath.c_str(), rv);
            goto end;
        }

        url = base_url + "/minidms/pin/" + uri_path;

        rv = agent->put(url, headers, 3, payload.c_str(), payload.length(), 0, response);

        // free header resources
        free(reqUserId);
        free(reqHandle);
        free(reqTicket);

        if (rv < 0) {
            LOG_ERROR("agent->get_response_back(\"%s\") returned %d", url.c_str(), rv);
            goto end;
        }
    }

    LOG_ALWAYS("Status Code: %d", agent->getlastHttpStatusCode());
    LOG_ALWAYS("Result: %s", response.c_str());

end:
    return rv;
}

static int minidms_get_pinitem(int argc, const char* argv[], std::string& response)
{
    int rv = 0;
    u64 user_id = 0;

    if (argc < 2) {
        minidms_help(argc, argv);
        return -1;
    }

    rv = getUserIdBasic(&user_id);
    if (rv != 0) {
        LOG_ERROR("Failed to get user_id.");
        return -1;
    }

    return minidms_get_pinitem(user_id, argv[1], response);
}

int minidms_get_pinitem(u64 userId, std::string filepath, std::string& response)
{
    int rv = 0;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::string url;
    std::string uri_path;
    HttpAgent* agent = NULL;

    agent = getHttpAgent();
    if (!agent) {
        LOG_ERROR("Fail to get http agent");
        rv = -1;
        goto end;
    }

    rv = getLocalHttpInfo(userId, base_url, service_ticket, session_handle);
    if (rv < 0) {
        LOG_ERROR("getLocalHttpInfo returned %d.", rv);
        goto end;
    }

    {
        char *reqUserId = VPLString_MallocAndSprintf(RF_HEADER_USERID": "FMTu64, userId);
        if (reqUserId == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            free(reqUserId);
            rv = -1;
            goto end;
        }
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            free(reqUserId);
            free(reqHandle);
            rv = -1;
            goto end;
        }

        const char* headers[] = {
            reqUserId,
            reqHandle,
            reqTicket,
        };

        rv = to_uri_path(filepath, uri_path);
        if (rv < 0) {
            LOG_ERROR("to_uri_path() failed, filepath = %s, rv = %d", filepath.c_str(), rv);
            goto end;
        }

        url = base_url + "/minidms/pin/" + uri_path;

        rv = agent->get_response_back(url, 0, 0, headers, 3, response);

        // free header resources
        free(reqUserId);
        free(reqHandle);
        free(reqTicket);

        if (rv < 0) {
            LOG_ERROR("agent->get_response_back(\"%s\") returned %d", url.c_str(), rv);
            goto end;
        }
    }

    LOG_ALWAYS("Status Code: %d", agent->getlastHttpStatusCode());
    LOG_ALWAYS("Result: %s", response.c_str());
end:
    return rv;
}

static int minidms_remove_pinitem(int argc, const char* argv[], std::string& response)
{
    int rv = 0;
    u64 user_id = 0;

    if (argc < 2) {
        minidms_help(argc, argv);
        return -1;
    }

    rv = getUserIdBasic(&user_id);
    if (rv != 0) {
        LOG_ERROR("Failed to get user_id.");
        return -1;
    }

    return minidms_remove_pinitem(user_id, argv[1], response);
}

int minidms_remove_pinitem(u64 userId, std::string filepath, std::string& response)
{
    int rv = 0;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::string url;
    std::string uri_path;
    HttpAgent* agent = NULL;

    agent = getHttpAgent();
    if (!agent) {
        LOG_ERROR("Fail to get http agent");
        rv = -1;
        goto end;
    }

    rv = getLocalHttpInfo(userId, base_url, service_ticket, session_handle);
    if (rv < 0) {
        LOG_ERROR("getLocalHttpInfo returned %d.", rv);
        goto end;
    }

    {
        char *reqUserId = VPLString_MallocAndSprintf(RF_HEADER_USERID": "FMTu64, userId);
        if (reqUserId == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            free(reqUserId);
            rv = -1;
            goto end;
        }
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            free(reqUserId);
            free(reqHandle);
            rv = -1;
            goto end;
        }

        const char* headers[] = {
            reqUserId,
            reqHandle,
            reqTicket,
        };

        rv = to_uri_path(filepath, uri_path);
        if (rv < 0) {
            LOG_ERROR("to_uri_path() failed, filepath = %s, rv = %d", filepath.c_str(), rv);
            goto end;
        }

        url = base_url + "/minidms/pin/" + uri_path;

        rv = agent->del(url, headers, 3, NULL, 0, NULL, response);

        // free header resources
        free(reqUserId);
        free(reqHandle);
        free(reqTicket);

        if (rv < 0) {
            LOG_ERROR("agent->get_response_back(\"%s\") returned %d", url.c_str(), rv);
            goto end;
        }
    }

    LOG_ALWAYS("Status Code: %d", agent->getlastHttpStatusCode());
    LOG_ALWAYS("Result: %s", response.c_str());
end:
    return rv;
}

static int minidms_deviceinfo(int argc, const char* argv[], std::string& response)
{
    int rv = 0;
    u64 user_id = 0;

    if (argc < 2) {
        minidms_help(argc, argv);
        return -1;
    }

    rv = getUserIdBasic(&user_id);
    if (rv != 0) {
        LOG_ERROR("Failed to get user_id.");
        return -1;
    }

    return minidms_deviceinfo(user_id, argv[1], response);
}

int minidms_deviceinfo(u64 userId, std::string device_id, std::string& response)
{
    int rv = 0;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::string url;
    HttpAgent* agent = NULL;

    agent = getHttpAgent();
    if (!agent) {
        LOG_ERROR("Fail to get http agent");
        rv = -1;
        goto end;
    }

    rv = getLocalHttpInfo(userId, base_url, service_ticket, session_handle);
    if (rv < 0) {
        LOG_ERROR("getLocalHttpInfo returned %d.", rv);
        goto end;
    }

    {
        char *reqUserId = VPLString_MallocAndSprintf(RF_HEADER_USERID": "FMTu64, userId);
        if (reqUserId == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            free(reqUserId);
            rv = -1;
            goto end;
        }
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            free(reqUserId);
            free(reqHandle);
            rv = -1;
            goto end;
        }

        const char* headers[] = {
            reqUserId,
            reqHandle,
            reqTicket,
        };

        url = base_url + "/minidms/deviceinfo/" + device_id;

        rv = agent->get_response_back(url, 0, 0, headers, 3, response);

        // free header resources
        free(reqUserId);
        free(reqHandle);
        free(reqTicket);

        if (rv < 0) {
            LOG_ERROR("agent->get_response_back(\"%s\") returned %d", url.c_str(), rv);
            goto end;
        }
    }

    LOG_ALWAYS("Status Code: %d", agent->getlastHttpStatusCode());
    LOG_ALWAYS("Result: %s", response.c_str());
end:
    return rv;
}

class MiniDMSDispatchTable {
public:
    MiniDMSDispatchTable() {
        cmds["ProtocolInfo"]       = minidms_protocolinfo;
        cmds["SampleURLMetadata"]  = minidms_samplemetadata;
        cmds["MSAContentURL"]      = minidms_msa_contenturl;
        cmds["AddPINItem"]         = minidms_add_pinitem;
        cmds["RemovePINItem"]      = minidms_remove_pinitem;
        cmds["GetPINItem"]         = minidms_get_pinitem;
        cmds["MediaServerInfo"]    = minidms_deviceinfo;
        cmds["Help"]               = minidms_help_with_response;
    }
    std::map<std::string, minidms_subcmd_fn> cmds;
};

static MiniDMSDispatchTable miniDMSDispatchTable;

static int minidms_dispatch(std::map<std::string, minidms_subcmd_fn> &cmdmap, int argc, const char* argv[], std::string &response)
{
    int rv = 0;

    if (cmdmap.find(argv[0]) != cmdmap.end()) {
        rv = cmdmap[argv[0]](argc, &argv[0], response);
    } else {
        LOG_ERROR("Command %s not supported", argv[0]);
        rv = -1;
    }
    return rv;
}


int dispatch_minidms_test_cmd(int argc, const char* argv[])
{
    std::string response;
    if (argc <= 1) {
        return minidms_help(argc, argv);
    } else {
        return minidms_dispatch(miniDMSDispatchTable.cmds, argc - 1, &argv[1], response);
    }
}
