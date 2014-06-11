#include "HttpSvc_Utils.hpp"

#include "cache.h"
#include "config.h"
#include "MediaMetadata.hpp"
#include "stream_transaction.hpp"
#include "virtual_device.hpp"

#include <HttpStream.hpp>
#include <HttpStream_Helper.hpp>
#include <log.h>

#include <scopeguard.hpp>
#include <vpl_socket.h>
#include <vpl_thread.h>
#include <vplex_http_util.hpp>
#include <vplex_serialization.h>
#include <vplex_socket.h>

#include <sstream>
#include <string>

const std::string HttpSvc::Utils::HttpHeader_ContentLength  = "Content-Length";
const std::string HttpSvc::Utils::HttpHeader_ContentType    = "Content-Type";
const std::string HttpSvc::Utils::HttpHeader_ContentRange   = "Content-Range";
const std::string HttpSvc::Utils::HttpHeader_Date           = "Date";
const std::string HttpSvc::Utils::HttpHeader_Host           = "Host";
const std::string HttpSvc::Utils::HttpHeader_Range          = "Range";

const std::string HttpSvc::Utils::HttpHeader_ac_collectionId   = "X-ac-collectionId";
const std::string HttpSvc::Utils::HttpHeader_ac_origDeviceId   = "X-ac-origDeviceId";
const std::string HttpSvc::Utils::HttpHeader_ac_serviceTicket  = "X-ac-serviceTicket";
const std::string HttpSvc::Utils::HttpHeader_ac_sessionHandle  = "X-ac-sessionHandle";
const std::string HttpSvc::Utils::HttpHeader_ac_srcfile        = "X-ac-srcfile";
const std::string HttpSvc::Utils::HttpHeader_ac_userId         = "X-ac-userId";
const std::string HttpSvc::Utils::HttpHeader_ac_deviceId       = "X-ac-deviceId";
const std::string HttpSvc::Utils::HttpHeader_ac_datasetRelPath = "X-ac-datasetRelPath";
const std::string HttpSvc::Utils::HttpHeader_ac_tolerateFileModification = "X-ac-tolerateFileMod";
const std::string HttpSvc::Utils::HttpHeader_ac_version = "X-ac-version";

const std::string HttpSvc::Utils::Mime_ApplicationJson = "application/json";
const std::string HttpSvc::Utils::Mime_ApplicationOctetStream = "application/octet-stream";
const std::string HttpSvc::Utils::Mime_ImageUnknown    = "image/unknown";
const std::string HttpSvc::Utils::Mime_AudioUnknown    = "audio/unknown";
const std::string HttpSvc::Utils::Mime_VideoUnknown    = "video/unknown";
const std::string HttpSvc::Utils::Mime_TextPlain       = "text/plain";

const std::string &HttpSvc::Utils::GetThreadStateStr(ThreadState state)
{
    static const std::string strNoThread("NoThread");
    static const std::string strSpawning("Spawning");
    static const std::string strRunning("Running");
    static const std::string strUnknown("Unknown");

    switch (state) {
    case ThreadState_NoThread:
        return strNoThread;
    case ThreadState_Spawning:
        return strSpawning;
    case ThreadState_Running:
        return strRunning;
    default:
        return strUnknown;
    }
}

const std::string &HttpSvc::Utils::GetCancelStateStr(CancelState state)
{
    static const std::string strNoCancel("NoCancel");
    static const std::string strCanceling("Canceling");
    static const std::string strUnknown("Unknown");

    switch (state) {
    case CancelState_NoCancel:
        return strNoCancel;
    case CancelState_Canceling:
        return strCanceling;
    default:
        return strUnknown;
    }
}

int HttpSvc::Utils::CheckReqHeaders(HttpStream *hs)
{
    int err = 0;

    std::string errMsg;

    std::string x_ac_userId;
    err = hs->GetReqHeader(Utils::HttpHeader_ac_userId, x_ac_userId);
    if (err) {
        LOG_ERROR("Failed to find x-ac-userId: err %d", err);
        errMsg += "Failed to find x-ac-userId.  ";
    }

    std::string x_ac_sessionHandle;
    err = hs->GetReqHeader(Utils::HttpHeader_ac_sessionHandle, x_ac_sessionHandle);
    if (err) {
        LOG_ERROR("Failed to find x-ac-sessionHandle missing: err %d", err);
        errMsg += "Failed to find x-ac-sessionHandle.  ";
    }

    std::string x_ac_serviceTicket;
    err = hs->GetReqHeader(Utils::HttpHeader_ac_serviceTicket, x_ac_serviceTicket);
    if (err) {
        LOG_ERROR("Failed to find x-ac-serviceTicket: err %d", err);
        errMsg += "Failed to find x-ac-serviceTicket.  ";
    }

    if (!errMsg.empty()) {
        std::string content = "{\"errMsg\":\"" + errMsg + "\"}";
        SetCompleteResponse(hs, 400, content, Utils::Mime_ApplicationJson);
        return -1;  // FIXME
    }

    size_t decodedServiceTicketLen = VPL_BASE64_DECODED_MAX_BUF_LEN(x_ac_serviceTicket.size());
    char *decodedServiceTicket = (char*)malloc(decodedServiceTicketLen);
    if (decodedServiceTicket == NULL) {
        LOG_ERROR("Out of memory");
        SetCompleteResponse(hs, 500);
        return CCD_ERROR_NOMEM;
    }
    ON_BLOCK_EXIT(free, decodedServiceTicket);
    VPL_DecodeBase64(x_ac_serviceTicket.c_str(), x_ac_serviceTicket.size(),
                     decodedServiceTicket, &decodedServiceTicketLen);

    if (VPLConv_strToU64(x_ac_userId.c_str(), NULL, 10) != hs->GetUserId()) {
        LOG_ERROR("Invalid UserID");
        errMsg += "Invalid userId.  ";
        std::string content = "{\"errMsg\":\"" + errMsg + "\"}";
        SetCompleteResponse(hs, 401, content, Utils::Mime_ApplicationJson);
        return -1;  // FIXME
    }

    ServiceSessionInfo_t serviceSessionInfo;
    err = Cache_GetSessionForVsdsByUser(hs->GetUserId(), serviceSessionInfo);
    if (err) {
        LOG_ERROR("Cache_GetSessionForVsdsByUser failed: err %d", err);
        SetCompleteResponse(hs, 500);
        return err;
    }

    if (VPLConv_strToU64(x_ac_sessionHandle.c_str(), NULL, 10) != serviceSessionInfo.sessionHandle) {
        LOG_ERROR("Invalid SessionHandle");
        errMsg += "Invalid sessionHandle.  ";
    }

    if (memcmp(decodedServiceTicket, serviceSessionInfo.serviceTicket.data(), serviceSessionInfo.serviceTicket.size()) != 0) {
        LOG_ERROR("Invalid ServiceTicket");
        errMsg += "Invalid serviceTicket.  ";
    }

    if (!errMsg.empty()) {
        std::string content = "{\"errMsg\":\"" + errMsg + "\"}";
        SetCompleteResponse(hs, 401, content, Utils::Mime_ApplicationJson);
        return -1;  // FIXME
    }

    return err;
}

int HttpSvc::Utils::AddStdRespHeaders(HttpStream *hs)
{
    return HttpStream_Helper::AddStdRespHeaders(hs);
}

int HttpSvc::Utils::SetCompleteResponse(HttpStream *hs, int code)
{
    return HttpStream_Helper::SetCompleteResponse(hs, code);
}

int HttpSvc::Utils::SetCompleteResponse(HttpStream *hs, int code, const std::string &content, const std::string &contentType)
{
    return HttpStream_Helper::SetCompleteResponse(hs, code, content, contentType);
}

int HttpSvc::Utils::SetLocalDeviceIdInReq(HttpStream *hs)
{
    std::ostringstream oss;
    oss << VirtualDevice_GetDeviceId();
    hs->SetReqHeader(HttpHeader_ac_deviceId, oss.str());
    return 0;
}

int HttpSvc::Utils::GetOidCollectionId(u64 deviceId, const std::string &oid, std::string &collectionId)
{
    int err = 0;

    size_t decodedOidLen = VPL_BASE64_DECODED_MAX_BUF_LEN(oid.size());
    char *decodedOid = (char*)malloc(decodedOidLen);
    if (decodedOid == NULL) {
        LOG_ERROR("Out of memory");
        return CCD_ERROR_NOMEM;
    }
    ON_BLOCK_EXIT(free, decodedOid);
    VPL_DecodeBase64(oid.data(), oid.size(), decodedOid, &decodedOidLen);
        
    std::string searchQuery = "object_id = '";
    searchQuery.append(decodedOid, decodedOidLen);
    searchQuery.append("'");
    std::string sortOrder;  // empty string means no particular sort order
    google::protobuf::RepeatedPtrField<media_metadata::MCAMetadataQueryObject> output;
    MMError mmerr;
    bool found = false;

    if (!found) {
        mmerr = MCAQueryMetadataObjects(deviceId, media_metadata::MCA_MDQUERY_MUSICTRACK, searchQuery, sortOrder, output);
        if (!mmerr && output.size() > 0)
            found = true;
    }
    if (!found) {
        mmerr = MCAQueryMetadataObjects(deviceId, media_metadata::MCA_MDQUERY_MUSICALBUM, searchQuery, sortOrder, output);
        if (!mmerr && output.size() > 0)
            found = true;
    }
    if (!found) {
        mmerr = MCAQueryMetadataObjects(deviceId, media_metadata::MCA_MDQUERY_MUSICARTIST, searchQuery, sortOrder, output);
        if (!mmerr && output.size() > 0)
            found = true;
    }
    if (!found) {
        mmerr = MCAQueryMetadataObjects(deviceId, media_metadata::MCA_MDQUERY_MUSICGENRE, searchQuery, sortOrder, output);
        if (!mmerr && output.size() > 0)
            found = true;
    }
    if (!found) {
        mmerr = MCAQueryMetadataObjects(deviceId, media_metadata::MCA_MDQUERY_PHOTOITEM, searchQuery, sortOrder, output);
        if (!mmerr && output.size() > 0)
            found = true;
    }
    if (!found) {
        mmerr = MCAQueryMetadataObjects(deviceId, media_metadata::MCA_MDQUERY_PHOTOALBUM, searchQuery, sortOrder, output);
        if (!mmerr && output.size() > 0)
            found = true;
    }
    if (!found) {
        mmerr = MCAQueryMetadataObjects(deviceId, media_metadata::MCA_MDQUERY_VIDEOITEM, searchQuery, sortOrder, output);
        if (!mmerr && output.size() > 0)
            found = true;
    }
    if (!found) {
        mmerr = MCAQueryMetadataObjects(deviceId, media_metadata::MCA_MDQUERY_VIDEOALBUM, searchQuery, sortOrder, output);
        if (!mmerr && output.size() > 0)
            found = true;
    }
    if (!found) {
        LOG_ERROR("OID %s not found in MCA DB", oid.c_str());
        err = CCD_ERROR_NOT_FOUND;
    }
    else {
        collectionId = VPLHttp_UrlEncoding(output.Get(0).collection_id(), NULL);
    }

    return err;
}
