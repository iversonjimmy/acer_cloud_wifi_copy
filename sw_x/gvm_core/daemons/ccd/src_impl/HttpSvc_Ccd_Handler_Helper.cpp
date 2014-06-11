#include "HttpSvc_Ccd_Handler_Helper.hpp"

#include "cache.h"
#include "ccd_features.h"
#include "config.h"
#include "gvm_errors.h"
#include "HttpStream.hpp"
#include "HttpSvc_Utils.hpp"
#include "HttpSvc_HsToTsAdapter.hpp"
#if CCD_ENABLE_STORAGE_NODE
#include "HttpSvc_Sn_Dispatcher.hpp"
#endif // CCD_ENABLE_STORAGE_NODE
#include "vplu_sstr.hpp"
#include "virtual_device.hpp"
#include "util_mime.hpp"

#include <new>

#include "log.h"

#if defined(WIN32) || defined(CLOUDNODE) || defined(LINUX)
static const size_t mediaFile_ReadBufSize = 256 * 1024;  // 256 KB
#else
static const size_t mediaFile_ReadBufSize = 64 * 1024;  // 64KB
#endif

int HttpSvc::Ccd::Handler_Helper::ForwardToServerCcd(HttpStream *hs)
{
    int err = 0;

#if CCD_ENABLE_STORAGE_NODE
    if ((__ccdConfig.httpSvcOpt & CONFIG_HTTPSVC_OPT_TRY_FUNCALL_SN) &&
        (__ccdConfig.enableTs & CONFIG_ENABLE_TS_INIT_TS_IN_SN) &&
        (hs->GetDeviceId() == VirtualDevice_GetDeviceId())) {

        // TODO: bug 15621: this locking looks like a bottleneck.
        MutexAutoLock lock(LocalServers_GetMutex());

        vss_server* storageNode = LocalServers_getStorageNode();
        if (storageNode != NULL) {
            LOG_INFO("Contact Sn via funcall");
            err = HttpSvc::Sn::Dispatcher::Dispatch(hs);
            return err;
        }
    }
#endif // CCD_ENABLE_STORAGE_NODE

    HsToTsAdapter *adapter = new (std::nothrow) HsToTsAdapter(hs);
    if (!adapter) {
        LOG_ERROR("No memory to create HsToTsAdapter obj");
        HttpSvc::Utils::SetCompleteResponse(hs, 500);
        return CCD_ERROR_NOMEM;
    }
    err = adapter->Run();
    delete adapter;

    return err;
}

int HttpSvc::Ccd::Handler_Helper::utilForwardLocalFile(HttpStream *hs,
        const std::string& absPathToForward,
        bool includeMimeTypeHeader,
        const std::string& mimeTypeHeader)
{
    VPLFS_stat_t stat;
    {
        int temp_rc = VPLFS_Stat(absPathToForward.c_str(), &stat);
        if (temp_rc != VPL_OK) {
            LOG_ERROR("VPLFS_Stat(%s):%d", absPathToForward.c_str(), temp_rc);
            Utils::SetCompleteResponse(hs, 400);
            return 0;
        }
    }

    // Open file for read.
    VPLFile_handle_t file;
    file =  VPLFile_Open(absPathToForward.c_str(), VPLFILE_OPENFLAG_READONLY, 0);
    if (!VPLFile_IsValidHandle(file)) {
        LOG_ERROR("VPLFile_Open(%s):%d", absPathToForward.c_str(), (int)file);
        Utils::SetCompleteResponse(hs, 400);
        return 0;
    }
    ON_BLOCK_EXIT(VPLFile_Close, file);
    // Success, no errors in accessing the file for streaming, continue.
    LOG_INFO("Forwarding file(%s)", absPathToForward.c_str());

    // Check range request.
    u64 start, end;
    u64 total;

    {
        std::string reqRange;
        int temp_rc = hs->GetReqHeader(Utils::HttpHeader_Range, reqRange);
        if (temp_rc == 0) {
            LOG_INFO("Range request found: %s", reqRange.c_str());
            temp_rc = Util_ParseRange(&reqRange, stat.size, start, end);
            if(temp_rc != 0){
                if(!reqRange.empty()) {
                   LOG_ERROR("Range \"%s\" invalid for size("FMTu_VPLFS_file_size_t") file(%s).",
                             reqRange.c_str(), stat.size, absPathToForward.c_str());
                }
                std::string contentRangeValue = SSTR("bytes */" << stat.size);
                hs->SetRespHeader(Utils::HttpHeader_ContentRange, contentRangeValue);
                hs->SetStatusCode(416);
                hs->Flush();
                return 0;
            }
        }
        else {
            // Get whole entity.
            start = 0;
            end = stat.size - 1;
        }
    }
    total = end - start + 1;

    // Set-up response
    // Indicate if whole file or only part being returned.
    if(start == 0 && end == stat.size - 1) {
        hs->SetStatusCode(200);
    }
    else {
        hs->SetStatusCode(206);
        std::string contentRangeValue =
                SSTR("bytes " << start << "-" << end << "/" << stat.size);
        hs->SetRespHeader(Utils::HttpHeader_ContentRange, contentRangeValue);
    }

    {
        std::string contentLengthValue = SSTR(total);
        hs->SetRespHeader(Utils::HttpHeader_ContentLength, contentLengthValue);
    }
    if (includeMimeTypeHeader) {
        hs->SetRespHeader(Utils::HttpHeader_ContentType, mimeTypeHeader);
    }

    VPLFile_Seek(file, start, VPLFILE_SEEK_SET);
    char *buffer = (char*)malloc(mediaFile_ReadBufSize);
    if (buffer == NULL) {
        LOG_ERROR("Out of Memory (%s). Closing connection.", absPathToForward.c_str());
        return -1;
    }
    ON_BLOCK_EXIT(free, buffer);

    do {
        size_t chunksize = ((size_t)total > mediaFile_ReadBufSize) ?
                                             mediaFile_ReadBufSize : (size_t)total;
        ssize_t nbytes = VPLFile_Read(file, buffer, chunksize);
        if (nbytes < 0) {
            LOG_ERROR("Failed to read from file(%s):"FMTd_ssize_t". Closing connection.",
                      absPathToForward.c_str(), nbytes);
            // Too late to recover, header may be sent.
            // Return negative value to close connection
            return nbytes;
        }
        ssize_t chunkBytesWritten = 0;
        while (chunkBytesWritten != nbytes) {
            ssize_t bytesWritten = hs->Write(buffer+chunkBytesWritten,
                                             nbytes-chunkBytesWritten);
            if (bytesWritten < 0) {
                LOG_ERROR("Writing bytes from(%s):"FMTd_ssize_t". Closing connection.",
                          absPathToForward.c_str(), bytesWritten);
                // Too late to recover, header may be sent.
                // Return negative value to close connection
                return bytesWritten;
            } else {
                chunkBytesWritten += bytesWritten;
            }
        }
        total -= nbytes;
    } while (total > 0);

    hs->Flush();
    return 0;
}

HttpSvc::Ccd::Handler_Helper::PhotoMimeMap HttpSvc::Ccd::Handler_Helper::photoMimeMap;

HttpSvc::Ccd::Handler_Helper::PhotoMimeMap::PhotoMimeMap()
{
    Util_CreatePhotoMimeMap(photoMimeMap);
}

const std::string &HttpSvc::Ccd::Handler_Helper::PhotoMimeMap::GetMimeFromExt(
        const std::string &ext) const
{
    std::map<std::string, std::string, case_insensitive_less>::const_iterator it =
            photoMimeMap.find(ext);
    if (it != photoMimeMap.end()) {
        return it->second;
    }
    else {
        return Utils::Mime_ImageUnknown;
    }
}

HttpSvc::Ccd::Handler_Helper::FileMimeMap HttpSvc::Ccd::Handler_Helper::fileMimeMap;

HttpSvc::Ccd::Handler_Helper::FileMimeMap::FileMimeMap()
{
    std::map<std::string, std::string, case_insensitive_less> mime_type_map;

    Util_CreatePhotoMimeMap(mime_type_map);
    fileMimeMap.insert(mime_type_map.begin(), mime_type_map.end());

    Util_CreateAudioMimeMap(mime_type_map);
    fileMimeMap.insert(mime_type_map.begin(), mime_type_map.end());

    Util_CreateVideoMimeMap(mime_type_map);
    fileMimeMap.insert(mime_type_map.begin(), mime_type_map.end());
}

const std::string &HttpSvc::Ccd::Handler_Helper::FileMimeMap::GetMimeFromExt(
        const std::string &ext) const
{
    std::map<std::string, std::string, case_insensitive_less>::const_iterator it =
            fileMimeMap.find(ext);
    if (it != fileMimeMap.end()) {
        return it->second;
    }
    else {
        return Utils::Mime_ApplicationOctetStream;
    }
}

