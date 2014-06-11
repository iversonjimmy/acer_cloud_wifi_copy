#include "HttpSvc_Ccd_Handler_picstream.hpp"

#include "HttpSvc_HsToHttpAdapter.hpp"
#include "HttpSvc_Utils.hpp"
#include "util_mime.hpp"
#include "SyncDown.hpp"

#include "cache.h"
#include "JsonHelper.hpp"
#include "vcs_proxy.hpp"
#include "vcs_utils.hpp"
#include "virtual_device.hpp"

#include <cJSON2.h>
#include <HttpStream.hpp>
#include <HttpStringStream.hpp>
#include <InStringStream.hpp>
#include <OutStringStream.hpp>
#include <log.h>

#include <scopeguard.hpp>
#include <vpl_conv.h>
#include <vpl_fs.h>
#include <vplex_http_util.hpp>

#include <cassert>
#include <new>
#include <sstream>
#include <string>

// https://www.ctbg.acer.com/wiki/index.php/PicStream_Enhancements_3.0_Design

static const std::string picstream_DatasetName = "PicStream";

HttpSvc::Ccd::Handler_picstream::Handler_picstream(HttpStream *hs)
    : Handler(hs)
{
    LOG_INFO("Handler_picstream[%p]: Created for HttpStream[%p]", this, hs);
}

HttpSvc::Ccd::Handler_picstream::~Handler_picstream()
{
    LOG_INFO("Handler_picstream[%p]: Destroyed", this);
}

int HttpSvc::Ccd::Handler_picstream::_Run()
{
    int err = 0;

    LOG_INFO("Handler_picstream[%p]: Run", this);

    err = Utils::CheckReqHeaders(hs);
    if (err) {
        // errmsg logged and response set by CheckReqHeaders()
        return 0;  // reset error
    }

    const std::string &uri = hs->GetUri();
    VPLHttp_SplitUri(uri, uri_tokens);
    if (uri_tokens.size() < 2) {
        LOG_ERROR("Handler_picstream[%p]: Unexpected URI: uri %s", this, uri.c_str());
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }
    assert(uri_tokens[0].compare("picstream") == 0);
    const std::string &uri_objtype = uri_tokens[1];

    if (objJumpTable.handlers.find(uri_objtype) == objJumpTable.handlers.end()) {
        LOG_ERROR("Handler_picstream[%p]: No handler: objtype %s", this, uri_objtype.c_str());
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }

    return err = (this->*objJumpTable.handlers[uri_objtype])();
    // response set by obj handler
}

int HttpSvc::Ccd::Handler_picstream::run_file()
{
    assert(uri_tokens.size() >= 2);
    assert(uri_tokens[0].compare("picstream") == 0);
    assert(uri_tokens[1].compare("file") == 0);

    const std::string &method = hs->GetMethod();
    if (fileMethodJumpTable.handlers.find(method) == fileMethodJumpTable.handlers.end()) {
        LOG_ERROR("Handler_picstream[%p]: No handler: method %s", this, method.c_str());
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }

    return (this->*fileMethodJumpTable.handlers[method])();
    // response set by method handler
}

int HttpSvc::Ccd::Handler_picstream::run_fileinfo()
{
    int err = 0;

    assert(uri_tokens.size() >= 2);
    assert(uri_tokens[0].compare("picstream") == 0);
    assert(uri_tokens[1].compare("fileinfo") == 0);

    // only acceptable method is GET
    if (hs->GetMethod().compare("GET") != 0) {
        LOG_ERROR("Handler_picstream[%p]: Unexpected method %s", this, hs->GetMethod().c_str());
        Utils::SetCompleteResponse(hs, 405);
        return 0;
    }

    // albumname must be present in the query string
    err = checkQueryPresent("albumname");
    if (err < 0) {
        // errmsg logged and response set by checkQueryPresent()
        LOG_ERROR("Handler_picstream[%p]: Invalid request, http_method=%s, uri=%s",
                    this,
                  hs->GetMethod().c_str(),
                  hs->GetUri().c_str());
        std::string response = "{\"errMsg\": \"query parameter: albumname is required.\"}";
        Utils::SetCompleteResponse(hs, 400, response, Utils::Mime_ApplicationJson);
        return 0;
    }

    // type must be present in the query string
    err = checkQueryPresent("type");
    if (err < 0) {
        // errmsg logged and response set by checkQueryPresent()
        LOG_ERROR("Handler_picstream[%p]: Invalid request, http_method=%s, uri=%s",
                  this,
                  hs->GetMethod().c_str(),
                  hs->GetUri().c_str());
        std::string response = "{\"errMsg\": \"query parameter: type is required.\"}";
        Utils::SetCompleteResponse(hs, 400, response, Utils::Mime_ApplicationJson);
        return 0;
    }

    std::string albumName, type, title;
    // Process parameters
    {
        err = hs->GetQuery("albumname", albumName);
        if (err) {
            LOG_ERROR("Handler_picstream[%p]: Error when obtaining album name: err %d", this, err);
            Utils::SetCompleteResponse(hs, 400);
            return 0;
        }

        err = hs->GetQuery("type", type);
        if (err) {
            LOG_ERROR("Handler_picstream[%p]: Error when obtaining type: err %d", this, err);
            Utils::SetCompleteResponse(hs, 400);
            return 0;
        }

        std::string uri = hs->GetUri();
        size_t pos_slash, pos;
        pos_slash = uri.find_last_of('/', string::npos);
        pos = uri.find_last_of('?',string::npos);
        if(pos - pos_slash > 1) {
            title = uri.substr(pos_slash + 1, pos - pos_slash - 1);
        }

    }

    std::string criteria;
    int i=0;
    std::queue< PicStreamPhotoSet > infos;

    criteria = "file_type=" + type + " AND album_name = \"" + albumName + "\"";
    if(title.length() > 0) {
       criteria.append(" AND title=\"");
       criteria.append(title);
       criteria.append("\"");
    }

    LOG_DEBUG("criteria: %s", criteria.c_str());

    err = SyncDown_QueryPicStreamItems(criteria, "", infos);
    if (err != 0) {
        LOG_ERROR("Handler_picstream[%p]: Error when Query: err %d", this, err);
        Utils::SetCompleteResponse(hs, 500);
        return 0;
    }

    int num = infos.size();
    std::ostringstream oss;

    oss << "{\"PhotoList\":[";
    while(infos.size() > 0) {

        PicStreamPhotoSet cur = infos.front();

        oss << "{";

        oss << "\"name\":\"" <<  cur.name <<  "\", ";
        oss << "\"originDevice\":" << cur.ori_deviceid << ",";
        oss << "\"identifier\":\"" << cur.identifier << "\",";
        oss << "\"title\":\"" << cur.title << "\",";
        oss << "\"albumname\":\"" << cur.albumName << "\",";
        oss << "\"date_time\":" << cur.dateTime << ",";
        oss << "\"file_size\":" << cur.full_size << ",";

        //query must choose from one of full resolution, low resolution and thumbnail.
        if (cur.full_url.length()) {
            oss << "\"full_res_url\":\"" << cur.full_url << "\"";
        } else if (cur.low_url.length()) {
            oss << "\"low_res_url\":\"" << cur.low_url << "\"";
        } else {
            oss << "\"thumb_url\":\"" << cur.thumb_url << "\"";
        }

        oss << "}";
        if(i++ < num - 1 ) oss << ", ";

        infos.pop();

    }

    oss << "],";
    oss << " \"numOfPhotos\":"<< num << "}";

    Utils::SetCompleteResponse(hs, 200, oss.str(), Utils::Mime_ApplicationJson);
    return 0;
}

int HttpSvc::Ccd::Handler_picstream::run_file_delete()
{
    assert(hs->GetMethod().compare("DELETE") == 0);

    int err = 0;

    // compId must be present in the query string
    err = checkQueryPresent("compId");
    if (err < 0) {
        // errmsg logged and response set by checkQueryPresent()
        return 0;
    }

    std::string compId, query_title;
    err = hs->GetQuery("compId", compId);
    if (err) {
        LOG_ERROR("Handler_picstream[%p]: Error when obtaining compId: err %d", this, err);
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }

    std::string uri = hs->GetUri();
    size_t pos_slash, pos;
    pos_slash = uri.find_last_of('/', string::npos);
    pos = uri.find_last_of('?',string::npos);
    if(pos - pos_slash > 1) {
        query_title = uri.substr(pos_slash + 1, pos - pos_slash - 1);
    }

    if(query_title.length() <= 0) {
        LOG_ERROR("Handler_picstream[%p]: Error when obtaining title: %s ", this, uri.c_str());
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }

    //check if it's full-res item
    std::queue< PicStreamPhotoSet > infos;
    std::string criteria = "file_type=0 AND comp_id=" + compId + " AND title=\""+query_title+"\"";

    LOG_DEBUG("criteria: %s", criteria.c_str());

    err = SyncDown_QueryPicStreamItems(criteria, "", infos);
    if (err != 0) {
        LOG_ERROR("Handler_picstream[%p]: Error when Query: err %d", this, err);
        Utils::SetCompleteResponse(hs, 500);
        return 0;
    }

    LOG_DEBUG("Item count: "FMTu_size_t, infos.size());

    if(infos.size() != 1) {
        LOG_ERROR("Handler_picstream[%p]: comp_id(%s) is not a full-res item.", this, compId.c_str());
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }

    std::string title, albumName;
    PicStreamPhotoSet cur = infos.front();
    title= cur.title;
    albumName= cur.albumName;

    u64 datasetId = 0;
    err = getDatasetId(datasetId);
    if (err < 0) {
        // errmsg logged and response set by getDatasetId()
        return 0;
    }

    std::string host_port;
    err = getServer(host_port);
    if (err < 0) {
        // errmsg logged and response set by getServer()
        return 0;
    }

    // hs->GetUri() looks like /picstream/file/title?compId=123&revision=45
    // we want to construct a url that looks like
    // https://host:port/vcs/file/<datasetid>/title?compId=123&revision=45

    std::string newuri;
    {
        std::ostringstream oss;
        oss << "https://" << host_port << "/vcs/file/" << datasetId << "/photos/" << albumName <<"/" << title << "?compId=" << compId;
        newuri.assign(oss.str());
        newuri.append( "&revision=1");//photo only has revision 1
    }

    LOG_DEBUG("New URI: %s", newuri.c_str());

    hs->SetUri(newuri);
    Utils::SetLocalDeviceIdInReq(hs);
    hs->RemoveReqHeader("Host");

    {
        HsToHttpAdapter *adapter = new (std::nothrow) HsToHttpAdapter(hs);
        if (!adapter) {
            LOG_ERROR("Handler_picstream[%p]: No memory to create HsToHttpAdapter obj", this);
            Utils::SetCompleteResponse(hs, 500);
            return 0;
        }
        err = adapter->Run();
        delete adapter;
    }

    return err;
}

int HttpSvc::Ccd::Handler_picstream::run_file_get()
{
    assert(hs->GetMethod().compare("GET") == 0);

    int err = 0;

    // compId must be present in the query string
    err = checkQueryPresent("compId");
    if (err < 0) {
        // errmsg logged and response set by checkQueryPresent()
        return 0;
    }
    // type must be present in the query string
    err = checkQueryPresent("type");
    if (err < 0) {
        // errmsg logged and response set by checkQueryPresent()
        return 0;
    }

    // query database for lpath and name
    std::string compId, type, title;
    {
        err = hs->GetQuery("compId", compId);
        if (err) {
            LOG_ERROR("Handler_picstream[%p]: Error when obtaining compId: err %d", this, err);
            Utils::SetCompleteResponse(hs, 400);
            return 0;
        }

        err = hs->GetQuery("type", type);
        if (err) {
            LOG_ERROR("Handler_picstream[%p]: Error when obtaining type: err %d", this, err);
            Utils::SetCompleteResponse(hs, 400);
            return 0;
        }

        std::string uri = hs->GetUri();
        size_t pos_slash, pos;
        pos_slash = uri.find_last_of('/', string::npos);
        pos = uri.find_last_of('?',string::npos);
        if(pos - pos_slash > 1) {
            title = uri.substr(pos_slash + 1, pos - pos_slash - 1);
        }

        if(title.length() <= 0) {
            LOG_ERROR("Handler_picstream[%p]: Error when obtaining title: %s ", this, uri.c_str());
            Utils::SetCompleteResponse(hs, 400);
            return 0;
        }

    }

    std::string dlPath;
    std::string criteria, response;
    std::queue< PicStreamPhotoSet > infos;

    u64 datasetId = 0;
    err = getDatasetId(datasetId);
    if (err < 0) {
        LOG_ERROR("Handler_picstream[%p]: Error when obtaining DatasetId: err %d", this, err);
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }

    criteria = " title=\"" + title + "\" AND file_type=" + type + " AND (comp_id=" + compId + " OR orig_comp_id="+ compId +")";

    LOG_DEBUG("criteria: %s", criteria.c_str());

    err = SyncDown_QueryPicStreamItems(criteria, "", infos);
    if (err != 0) {
        LOG_ERROR("Handler_picstream[%p]: Error when Query: err %d", this, err);
        Utils::SetCompleteResponse(hs, 500);
        return 0;
    }

    LOG_DEBUG("Item counts: "FMTu_size_t, infos.size());

    if(infos.size() == 0) {
        LOG_ERROR("Handler_picstream[%p]: No query result comp_id=%s file_type=%s", this, compId.c_str(), type.c_str());
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    } else if(infos.size() > 1) {
        LOG_WARN("Handler_picstream[%p]: query should has only one result comp_id=%s file_type=%s", this, compId.c_str(), type.c_str());
    }

    std::string compPath;
    std::string lpath;
    PicStreamPhotoSet cur = infos.front();

    compPath= cur.name;
    lpath = cur.lpath;
    u64 fileType = VPLConv_strToU64(type.c_str(), NULL, 10);
    u64 fileSize = cur.full_size;
    u64 comp_id = VPLConv_strToU64( cur.compId.c_str() , NULL, 10);

    //
    // Local file might crash or disappear, so check cached file first.
    //
    {
        std::string cacheFilePath;
        SyncDown_GetCacheFilePath(hs->GetUserId(), datasetId, comp_id, fileType, compPath, cacheFilePath);
        if (cacheFilePath.length() > 0) {
            LOG_INFO("Handler_picstream[%p]: read cached file: %s", this, cacheFilePath.c_str());
            err = file2httpstream(cacheFilePath, hs);
            if(0 == err) {
                return 0;
            }
            LOG_WARN("Handler_picstream[%p]: Using cached file failed.", this);
        }

        if (cur.identifier.length() > 0) {
            LOG_INFO("Handler_picstream[%p]: read identifier file: %s", this, cur.identifier.c_str());
            err = file2httpstream(cur.identifier, hs);
            if(0 == err) {
                return 0;
            }
            LOG_WARN("Handler_picstream[%p]: Using identifier file failed.", this);
        }

        if (lpath.length() > 0) {
            LOG_INFO("Handler_picstream[%p]: read lpath file: %s", this, lpath.c_str());
            err = file2httpstream(lpath, hs);
            if(0 == err) {
                return 0;
            }
            LOG_WARN("Handler_picstream[%p]: Using lpath file failed.", this);
        }

        LOG_WARN("Handler_picstream[%p]: No local or cached file available! Try to download from server.", this);
    }

    //
    // Note: We don't have thumbnail's file size, so thumbnail need to be donwloaded first and use file2httpstream
    //
    {//download_from_internet
        std::map<std::string, std::string, case_insensitive_less> mime_type_map;
        size_t pos;
        std::string ext = "";
        pos = compPath.rfind(".");
        if(pos > 0) {
            ext = compPath.substr(pos+1, string::npos);
        }

        // For thumbnail, file size is obtained after download.
        // For full/low-res, file size is obtained from database already.
        if(fileType != FileType_Thumbnail) {
            char content_length_value[128];
            sprintf(content_length_value, FMTu64, fileSize);
            hs->SetRespHeader(Utils::HttpHeader_ContentLength, content_length_value);

            Util_CreatePhotoMimeMap(mime_type_map);
            std::map<std::string, std::string, case_insensitive_less>::iterator it;
            it = mime_type_map.find(ext);
            if(it != mime_type_map.end()) {
                hs->SetRespHeader(Utils::HttpHeader_ContentType, it->second);
            } else {
                hs->SetRespHeader(Utils::HttpHeader_ContentType, Utils::Mime_ImageUnknown);
            }
        }

        //Raw data will be fed to hs in this function, except thumbnail.
        err = SyncDown_DownloadOnDemand(datasetId, compPath, comp_id, fileType, hs, /*OUT*/dlPath);

        LOG_DEBUG("download Path:%s", dlPath.c_str());

        if(err != 0) {
            LOG_ERROR("Handler_picstream[%p]: Error when SyncDown_DownloadOnDemand: err %d", this, err);
            Utils::SetCompleteResponse(hs, 400);
            return 0;
        }

        if (FileType_Thumbnail == fileType) { //thumbnail
            err = file2httpstream(dlPath, hs);
            if (err == -1) {
                LOG_ERROR("Handler_picstream[%p]: Error when load local file: err %d", this, err);
                Utils::SetCompleteResponse(hs, 404);
                return 0;
            }
        }
    }//download_from_internet

    return 0;
}

HttpSvc::Ccd::Handler_picstream::ObjJumpTable HttpSvc::Ccd::Handler_picstream::objJumpTable;

HttpSvc::Ccd::Handler_picstream::ObjJumpTable::ObjJumpTable()
{
    handlers["file"]         = &Handler_picstream::run_file;
    handlers["fileinfo"] = &Handler_picstream::run_fileinfo;
}

HttpSvc::Ccd::Handler_picstream::FileMethodJumpTable HttpSvc::Ccd::Handler_picstream::fileMethodJumpTable;

HttpSvc::Ccd::Handler_picstream::FileMethodJumpTable::FileMethodJumpTable()
{
    handlers["DELETE"] = &Handler_picstream::run_file_delete;
    handlers["GET"]    = &Handler_picstream::run_file_get;
}

int HttpSvc::Ccd::Handler_picstream::getDatasetId(u64 &datasetId)
{
    int err = VCS_getDatasetID(hs->GetUserId(), picstream_DatasetName, datasetId);
    if (err < 0) {
        LOG_ERROR("Handler_picstream[%p]: Failed to find Cloud Doc dataset: err %d", this, err);
        Utils::SetCompleteResponse(hs, 500);
    }
    return err;
}

int HttpSvc::Ccd::Handler_picstream::getServer(std::string &host_port)
{
    host_port = VCS_getServer(hs->GetUserId(), picstream_DatasetName);
    if (host_port.empty()) {
        LOG_ERROR("Handler_picstream[%p]: Failed to determine server host:port", this);
        Utils::SetCompleteResponse(hs, 500);
        return -1;
    }
    return 0;
}

int HttpSvc::Ccd::Handler_picstream::checkQueryPresent(const std::string &name)
{
    std::string value;
    int err = hs->GetQuery(name, value);
    if (err) {
        LOG_ERROR("Handler_picstream[%p]: Query param %s missing: err %d", this, name.c_str(), err);
        Utils::SetCompleteResponse(hs, 400);
    }
    return err;
}

int HttpSvc::Ccd::Handler_picstream::file2httpstream(std::string filePath, HttpStream *hs)
{
    int err = 0;
    VPLFS_stat_t stat;
    char content_length_value[128];
    std::map<std::string, std::string, case_insensitive_less> mime_type_map;
    size_t pos;
    std::string ext = "";
    pos = filePath.rfind(".");
    if(pos > 0) {
        ext = filePath.substr(pos+1, string::npos);
    }

    err = VPLFS_Stat(filePath.c_str(), &stat);
    if (err) {
        LOG_ERROR("Handler_picstream[%p]: VPLFS_state error path = %s, err = %d", this, filePath.c_str(), err);
        return -1;
    }

    err = VPLFile_CheckAccess(filePath.c_str(), VPLFILE_CHECKACCESS_READ);
    if (err) {
        LOG_ERROR("Handler_picstream[%p]: VPLFile_CheckAccess path = %s, err = %d", this, filePath.c_str(), err);
        return -1;
    }

    sprintf(content_length_value, FMTu_VPLFS_file_size__t, stat.size);

    VPLFile_handle_t file = VPLFILE_INVALID_HANDLE;
    file =  VPLFile_Open(filePath.c_str(), VPLFILE_OPENFLAG_READONLY, 0);
    if (!VPLFile_IsValidHandle(file)) {
        LOG_ERROR("Handler_picstream[%p]: Failed to open file file %s", this, filePath.c_str());
        return -1;
    } else {
        LOG_INFO("Handler_picstream[%p]: Open %s", this, filePath.c_str());
    }
    ON_BLOCK_EXIT(VPLFile_Close, file);

    hs->SetRespHeader(HttpSvc::Utils::HttpHeader_ContentLength, content_length_value);
    {
        Util_CreatePhotoMimeMap(mime_type_map);
        std::map<std::string, std::string, case_insensitive_less>::iterator it;
        it = mime_type_map.find(ext);
        if(it != mime_type_map.end()) {
            hs->SetRespHeader(HttpSvc::Utils::HttpHeader_ContentType, it->second);
        } else {
            hs->SetRespHeader(HttpSvc::Utils::HttpHeader_ContentType, HttpSvc::Utils::Mime_ImageUnknown);
        }
    }

    hs->SetStatusCode(200);
    u64 total = stat.size;
    do {
        char buffer[32 * 1024];
        ssize_t bytesWritten;
        size_t chunksize = (size_t)total > ARRAY_SIZE_IN_BYTES(buffer) ? ARRAY_SIZE_IN_BYTES(buffer) : (size_t)total;
        ssize_t nbytes = VPLFile_Read(file, buffer, chunksize);
        if (nbytes < 0) {
            LOG_ERROR("Handler_picstream[%p]: Failed to read from file: err "FMTd_ssize_t, this, nbytes);
            hs->SetStatusCode(500);
            break;
        }
        bytesWritten = hs->Write(buffer, nbytes);
        if (bytesWritten != nbytes) {
            LOG_ERROR("Handler_picstream[%p]: Write(%d):%d", this, (int)nbytes, (int)bytesWritten);
            hs->SetStatusCode(500);
            break;
        }
        total -= chunksize;
    } while (total > 0);

    return 0;
}

