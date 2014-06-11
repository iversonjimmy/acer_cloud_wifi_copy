#define TS_DEV_TEST
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
#include "strm_http.hpp"

#include "strm_dlna.h"
#include "vplex_trace.h"
#include "vplex_file.h"
#include "vpl_fs.h"
#include "vss_comm.h"
#include "vss_util.hpp"
#include "vssi_types.h"
#include "util_mime.hpp"
#include "dataset.hpp"
#include "config.h"
#include "ccdi.hpp"
#include "ccdi_orig_types.h"

#ifdef ENABLE_PHOTO_TRANSCODE
#include "image_transcode.h"
#endif

#include "cslsha.h"
#include <time.h>

#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <stack>

#include <algorithm>  // for trim
#include <functional> // for trim
#include <cctype>     // for trim
#include <locale>     // for trim

#include "vplex_powerman.h"
#include "vplex_http_util.hpp"

#include <cJSON2.h>
#include "utf8.hpp"

#define URL_CMD_KEEPAWAKE_TOKEN         "keepAwake"

using namespace std;

static const std::string CONTENT_TYPE = "Content-Type";
static const std::string CONTENT_LENGTH = "Content-Length";
static const std::string CONTENT_RANGE = "Content-Range";

static const std::string RF_HEADER_USERID = "x-ac-userId";
static const std::string RF_HEADER_SESSION_HANDLE = "x-ac-sessionHandle";
static const std::string RF_HEADER_SERVICE_TICKET = "x-ac-serviceTicket";
static const std::string RF_HEADER_DEVICE_ID = "x-ac-origDeviceId";

static const std::string HEADER_ACT_XCODE_DIMENSION = "act_xcode_dimension";
static const std::string HEADER_ACT_XCODE_FMT = "act_xcode_fmt";

#define LOG_HTTP_TRANSACTIONS 0
#if LOG_HTTP_TRANSACTIONS
#define LOG_REQUEST(transaction) (transaction)->req.dump()
#define LOG_RESPONSE(transaction) (transaction)->resp.dump()
#else
#define LOG_REQUEST(transaction) 
#define LOG_RESPONSE(transaction)
#endif

static void apply_prefix_rewrite_rules(std::string &component);

// trim whitespace from start
static inline std::string &ltrim(std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
        return s;
}
// trim whitespace from end
static inline std::string &rtrim(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
        return s;
}
// trim whitespace from both ends
static inline void trim(std::string &s) {
        ltrim(rtrim(s));
}

#ifdef ENABLE_PHOTO_TRANSCODE
static char uppercase(char c){
    if (c >= 'a' && c <= 'z') {
        return c + ('A'-'a');
    }
    return c;
}
#endif

static int open_vss_write_handle(strm_http_transaction* transaction,
                                 const std::string& path)
{
    int rv = 0;
    if(transaction->vss_file_handle != NULL) {
        // Verify this is the handle we want.

        // FIXME: this does not handle the case sensitive case correctly
        if((transaction->vss_file_handle->get_access_mode() & VSSI_FILE_OPEN_WRITE) &&
           ((!transaction->ds->is_case_insensitive() &&
             transaction->vss_file_handle->component == path) ||
             !utf8_casencmp(transaction->vss_file_handle->component.size(),
                            transaction->vss_file_handle->component.c_str(),
                            path.size(),
                            path.c_str()))) {
            // Nothing needs to be done, good vss_file_handle
        }else{
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Different vss_file handle needed for transaction,"
                             "Should not occur: %s, %s, access:%d",
                             transaction->vss_file_handle->component.c_str(),
                             path.c_str(),
                             transaction->vss_file_handle->get_access_mode());
            transaction->ds->close_file(transaction->vss_file_handle, NULL, 0);
            transaction->vss_file_handle = NULL;
        }
    }
    if(transaction->vss_file_handle == NULL) {
        // Create vss_file_handle
        int rc = transaction->ds->open_file(path,
                                            0,
                                            VSSI_FILE_OPEN_WRITE|VSSI_FILE_OPEN_CREATE,
                                            0,
                                            transaction->vss_file_handle);
        if(rc != 0) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "cannot open file:%d, %s",
                             rc, path.c_str());
            rv = rc;
        }
    }
    return rv;
}

static void close_vss_handle(strm_http_transaction* transaction)
{
    if(transaction->ds && transaction->vss_file_handle) {
        transaction->ds->close_file(transaction->vss_file_handle, NULL, 0);
        transaction->vss_file_handle = NULL;
    }
}

#ifdef ENABLE_PHOTO_TRANSCODE
static void populate_response(http_response &resp,
                              int status,
                              const std::string &content,
                              const std::string &content_type);

#define GENERATE_TEMPFILE_BUF_SIZE 4096

typedef struct struct_GenerateTempFileCallbackArgs {
    strm_http* m_strm_http;
    strm_http_transaction* m_transaction;
    std::string m_media_file;
} GenerateTempFileArgs_t;

typedef struct struct_AsyncTranscodingCallbackArgs {
    strm_http* m_strm_http;
    strm_http_transaction* m_transaction;
    std::string m_source_file_extension;
} AsyncTranscodingCallbackArgs_t;

static int generate_temp_file_callback(void* ctx, const char* path) {
    int rv = VPL_OK;
    int rc;
    GenerateTempFileArgs_t* callback_args;
    strm_http* http;
    strm_http_transaction* transaction;
    std::string media_file;
    VPLFS_stat_t stat;
    VPLFile_handle_t handle;
    u32 photo_size;
    u32 offset;
    vss_file* vss_file_handle = NULL;

    if (ctx == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Parameter is NULL.");
        return VPL_ERR_FAIL;
    }

    callback_args = (GenerateTempFileArgs_t*) ctx;
    http = callback_args->m_strm_http;
    transaction = callback_args->m_transaction;
    media_file = callback_args->m_media_file;

    handle = VPLFile_Open(path, VPLFILE_OPENFLAG_CREATE | VPLFILE_OPENFLAG_TRUNCATE | VPLFILE_OPENFLAG_WRITEONLY, 0600);
    if ( !VPLFile_IsValidHandle(handle) ) {
        rv = VPL_ERR_FAIL;
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Stream("FMT_VPLSocket_t"): Can not open the file: \"%s\"",
                         VAL_VPLSocket_t(http->sockfd),
                         path);
        goto failed_to_open_file;
    }

    rc = transaction->ds->stat_component(media_file.c_str(), stat);
    if (rc != VPL_OK) {
        rv = rc;
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Stream("FMT_VPLSocket_t"): Failed to get file state \"%s\", rc = %d",
                         VAL_VPLSocket_t(http->sockfd),
                         media_file.c_str(),
                         rc);
        goto failed_to_state_ds_file;
    }

    photo_size = stat.size;

    rc = transaction->ds->open_file(media_file,
                                    0,
                                    VSSI_FILE_OPEN_READ | VSSI_FILE_SHARE_READ,
                                    0,
                                    vss_file_handle);
    if (rc != 0) {
        rv = rc;
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                        "Stream("FMT_VPLSocket_t"): Failed to open file: %s, rc = %d",
                        VAL_VPLSocket_t(http->sockfd),
                        media_file.c_str(),
                        rc);
        goto failed_to_open_ds_file;
    }

    offset = 0;
    while (offset < photo_size) {
        u32 read_bytes = 0;
        u32 write_bytes = 0;
        char buf[GENERATE_TEMPFILE_BUF_SIZE];
        u32 read_size = ((photo_size - offset) >= GENERATE_TEMPFILE_BUF_SIZE) ? GENERATE_TEMPFILE_BUF_SIZE : (photo_size - offset);
        rc = transaction->ds->read_file(vss_file_handle,
                                        NULL,
                                        0,
                                        offset,
                                        read_size,
                                        buf);
        if (rc != VSSI_SUCCESS) {
            rv = rc;
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Stream("FMT_VPLSocket_t"): Failed to read image \"%s\", rc = %d",
                             VAL_VPLSocket_t(http->sockfd),
                             media_file.c_str(),
                             rc);
            goto failed_to_generate_tempfile;
        }

        read_bytes = read_size;
        write_bytes = VPLFile_Write(handle, buf, read_bytes);
        if (write_bytes != read_bytes) {
            rv = VPL_ERR_FAIL;
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Stream("FMT_VPLSocket_t"): Failed to write buffer into file: \"%s\"",
                             VAL_VPLSocket_t(http->sockfd),
                             path);
            goto failed_to_generate_tempfile;
        }
        offset += write_bytes;
    }

    VPLFile_Sync(handle);

 failed_to_generate_tempfile:
    transaction->ds->close_file(vss_file_handle, NULL, 0);
 failed_to_open_ds_file:
 failed_to_state_ds_file:
    VPLFile_Close(handle);
 failed_to_open_file:
    delete callback_args;
    return rv;
}

static void process_async_transcoding_callback(ImageTranscode_handle_t handle, int return_code, void* args)
{
    int rv;
    AsyncTranscodingCallbackArgs_t* callback_args;
    strm_http* self;
    strm_http_transaction* transaction;
    size_t image_len;
    const char* buf = NULL;
    size_t offset;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "process_async_transcoding_callback, handle["FMTu32"] rc = %d", handle, return_code);

    if (args == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Unexpected number of arguments.");
        return;
    }

    callback_args = (AsyncTranscodingCallbackArgs_t*) args;
    self = callback_args->m_strm_http;
    transaction = callback_args->m_transaction;

    if (return_code != IMAGE_TRANSCODING_OK) {
        // Print warning and return original image.
        VPLTRACE_LOG_WARN(TRACE_BVS, 0, "Failed to transcode image");
        ::populate_response(transaction->resp, 415, "{\"errMsg\":\"Failed to transcode image.\"}", "application/json");
        self->put_response(transaction);
        goto end;
    }

    rv = ImageTranscode_GetContentLength(handle, image_len);
    if (rv != IMAGE_TRANSCODING_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to get content length");
        ::populate_response(transaction->resp, 500, "{\"errMsg\":\"Failed to get content length of transcode image.\"}", "application/json");
        self->put_response(transaction);
        goto end;
    }

    // Setup the response headers
    {
        int snprintfBytes;
        char contentLengthStr[128];
        char date[30];
        time_t curTime;
        map<string, string, case_insensitive_less>::iterator mime_it;
        std::string mimeType;
        std::string extension;

        curTime = time(NULL);
        strftime(date, 30,"%a, %d %b %Y %H:%M:%S GMT" , gmtime(&curTime));
        snprintfBytes = snprintf(contentLengthStr, 128, FMTu_size_t, image_len);

        // Use the mime-type after transcoding as Content-Type
        if (extension.empty()) {
            const std::string* header_act_xcode_fmt;
            header_act_xcode_fmt = transaction->req.find_header(HEADER_ACT_XCODE_FMT);
            if (header_act_xcode_fmt) {
                extension = *header_act_xcode_fmt;
            }
        }

        // Use the original file extension to find the mapped mime-type as Content-Type
        if (extension.empty() && !callback_args->m_source_file_extension.empty()) {
            extension = callback_args->m_source_file_extension;
        }

        if (extension.empty()) {
            mimeType = "image/unknown";
        } else {
            mime_it = transaction->http_handle->photo_mime_map.find(extension);
            if (mime_it != transaction->http_handle->photo_mime_map.end()) {
                mimeType = mime_it->second;
            }
        }

        transaction->resp.add_response(200);
        transaction->resp.add_header("Accept-Ranges", "bytes");
        transaction->resp.add_header("Connection", "Keep-Alive");
        transaction->resp.add_header("Date", date);
        transaction->resp.add_header("Ext", "");
        transaction->resp.add_header("realTimeInfo.dlna.org", "DLNA.ORG_TLAG=*");
        transaction->resp.add_header("Server", IGWARE_SERVER_STRING);
        transaction->resp.add_header(CONTENT_LENGTH, contentLengthStr);
        transaction->resp.add_header(CONTENT_TYPE, mimeType);
        transaction->resp.add_header("transferMode.dlna.org", "Interactive");
        if (transaction->req.find_header("getcontentFeatures.dlna.org")) {
            transaction->resp.add_header("contentFeatures.dlna.org", "DLNA.ORG_OP=01;DLNA.ORG_FLAGS=00100000000000000000000000000000");
        }
    }

    if (transaction->req.http_method == "GET") {
        transaction->resp.add_content(buf, image_len);
        buf = new char[image_len];
        offset = 0;
        do {
            rv = ImageTranscode_Read(handle, (void*)(buf), image_len, offset);
            if (rv < 0) {
                break;
            }
            offset += rv;
            if (offset > image_len) {
                // Should not happen.
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "ImageTranscode_Read() Read exceeded expected size of data.");
            }
        } while (offset < image_len);
    }

    self->put_response(transaction);

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "process_async_transcoding_callback(), done.");

 end:
    if (buf != NULL) {
        delete [] buf;
    }
    delete callback_args;
    ImageTranscode_DestroyHandle(handle);
}
#endif

void strm_http::transaction_cleanup(strm_http_transaction* transaction, bool aborted)
{
    log_completion(transaction, aborted);
    if(transaction) {
        close_vss_handle(transaction);

        // close the VPL file handle for the tmp file which we used back in the write_upload_file_req()
        if (VPLFile_IsValidHandle(transaction->fh)) {
            VPLFile_Close(transaction->fh);
            transaction->fh = VPLFILE_INVALID_HANDLE;
            // not dealing w/ the tmp file clean-up. tmp files will be removed
            // during the strm_htp construction/destruction
        }
    }
    delete transaction;
}

static void populate_response(http_response &resp,
                              int status,
                              const std::string &content,
                              const std::string &content_type)
{
    char dateStr[30];
    time_t curTime = time(NULL);
    strftime(dateStr, sizeof(dateStr), "%a, %d %b %Y %H:%M:%S GMT" , gmtime(&curTime));
    resp.add_header("Date", dateStr);

    resp.add_header("Ext", "");
    resp.add_header("Server", IGWARE_SERVER_STRING);
    resp.add_response(status);
    char contentLengthValue[128];
    sprintf(contentLengthValue, "%d", content.length());
    resp.add_header("Content-Length", contentLengthValue);
    if (!content_type.empty()) {
        resp.add_header("Content-Type", content_type);
    }
    resp.add_content(content);
}

static void isRemoteFileOrMediaServerEnable(u64 userId, u64 psnDeviceId, bool &rfEnabled, bool &msEnabled) {
    s32 rv = -1;
    ccd::ListUserStorageInput request;
    ccd::ListUserStorageOutput listSnOut;
    request.set_user_id(userId);
    request.set_only_use_cache(true);
    rv = CCDIListUserStorage(request, listSnOut);
    if (rv != 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,"CCDIListUserStorage for user("FMTu64") failed %d", userId, rv);
    } else {
        VPLTRACE_LOG_FINE(TRACE_BVS, 0, "CCDIListUserStorage OK - %d", listSnOut.user_storage_size());
        for (int i = 0; i < listSnOut.user_storage_size(); i++) {
            if (listSnOut.user_storage(i).storageclusterid() == psnDeviceId) {
                rfEnabled = listSnOut.user_storage(i).featureremotefileaccessenabled();
                msEnabled = listSnOut.user_storage(i).featuremediaserverenabled();
                return;
            }
        }
    }
    // default return false if cannot match any
    VPLTRACE_LOG_ERR(TRACE_BVS, 0,"No matched record for user["FMTu64"] / device ["FMTu64
                    "], set remotefile and mediaserver as disabled", userId, psnDeviceId);
    rfEnabled = false;
    msEnabled = false;
    return;
}

static void remove_dir_recursive(const std::string& target)
{
    stack<string> to_empty; // directories to empty of all contents
    stack<string> to_remove; // directories to remove when empty
    VPLFS_stat_t stats;
    VPLFS_dir_t directory;
    VPLFS_dirent_t direntry;
    string dir;
    string entry;
    int rc;

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      "Forcibly removing file/directory {%s}.",
                      target.c_str());

    rc = VPLFS_Stat(target.c_str(), &stats);
    if(rc != 0) {
        VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                          "Cannot stat %s for removal: %d",
                          target.c_str(), rc);
        return;
    }

    // If the target element is a file, just unlink it.
    if(stats.type != VPLFS_TYPE_DIR) {
        VPLFile_Delete(target.c_str());
        return;
    }

    to_empty.push(target);

    // Recursively delete all files and directories within this directory,
    // and the directory itself.
    while (!to_empty.empty()) {
        dir = to_empty.top();
        to_empty.pop();
        to_remove.push(dir);

        rc = VPLFS_Opendir(dir.c_str(), &directory);
        if(rc != VPL_OK) {
            VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                              "Failed to open directory %s: %d.",
                              dir.c_str(), rc);
            continue;
        }

        while((VPLFS_Readdir(&directory, &direntry) == VPL_OK)) {
            if((direntry.filename[0] == '.' && direntry.filename[1] == '\0') ||
               (direntry.filename[0] == '.' && direntry.filename[1] == '.' && direntry.filename[2] == '\0')) {
                // Special file. Leave it be.
                continue;
            }

            entry = dir + "/" + direntry.filename;

            rc = VPLFS_Stat(entry.c_str(), &stats);
            if(rc != VPL_OK) {
                VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                                  "Cannot stat %s: %d. Skipping entry.",
                                  entry.c_str(), rc);
                continue;
            }

            // Delete files. Stack directories to be emptied.
            if(stats.type != VPLFS_TYPE_DIR) {
                VPLFile_Delete(entry.c_str());
            } else {
                to_empty.push(entry);
            }
        }

        VPLFS_Closedir(&directory);
    }

    // Remove the emptied directories.
    while (!to_remove.empty()) {
        dir = to_remove.top();
        to_remove.pop();
        VPLDir_Delete(dir.c_str());
    }
}

// Input: full file/dir path
// Output: filename or directory name
static std::string getFilenameFromPath(const std::string path) {

    size_t last;

    // possible file/dir path looks like /A/B/C/filename.ext or /A/B/C////
    // strip the trailing slashes and make it looks like : /A/B/C
    last = path.find_last_not_of("/");
    if (last != std::string::npos) {
        size_t start;
        // search for the last "/" which should now precede the filename
        start = path.find_last_of("/", last);
        if (start != std::string::npos) {
            return path.substr(start+1, last-start);
        } else {
            // cannot find any "/", return the whole string w/o trailing "/"
            return path.substr(0, last+1);
        }
    } else {
        // we cannot found any non "/". it can only be "////"
        if (path.size() > 0) {
            // since we don't find last "/", and the first character is "/"
            // then it should be string looks like "//////". Reduce into "/"
            // and return directly
            return "/";
        } else {
            // can only be empty string
            return "";
        }
    }
}

// Input: full file/dir path
// Output: parent directory name (if has more than 2 levels) or root (if 1 level)
static std::string getParentDirectory(const std::string path) 
{
    size_t last_non_slash;

    // possible file/dir path looks like /A/B/C/filename.ext or /A/B/C////
    // strip the trailing slashes and make it looks like : /A/B/C
    last_non_slash = path.find_last_not_of("/");
    if (last_non_slash != std::string::npos) {
        size_t start;
        size_t first_non_slash;

        // search first non-slash position
        first_non_slash = path.find_first_not_of("/");
        if (first_non_slash == std::string::npos) {
            // This can't happen, since last_non_slash != string::npos.
            first_non_slash = 0;
        }

        // search for the last "/" which should now precede the filename
        // unless the name is of the form /A/B/C/
        start = path.find_last_of("/", last_non_slash);
        // if we found any and it's not between the preceding slashes
        if (start != std::string::npos && (start >= first_non_slash)){
            return path.substr(0, start);
        }
        return "/";
    } else {
        // we cannot find any non "/". it can only be "////"
        if (path.size() > 0) {
            // since we don't find last non-"/", and the first character is "/"
            // then it must be string of "//////". Reduce into "/"
            // and return directly
            return "/";
        } else {
            // can only be empty string
            return "";
        }
    }
}

strm_http_transaction::strm_http_transaction(strm_http* handle)
:   receiving(true),
    processing(false),
    sending(false),
    failed(false),
    recv_cnt(0),
    send_cnt(0),
    http_handle(handle),
    vss_file_handle(NULL),
#ifdef ENABLE_PHOTO_TRANSCODE
    is_image_transcoding(false),
    image_transcoding_handle(INVALID_IMAGE_TRANSCODE_HANDLE),
#endif
    fh(VPLFILE_INVALID_HANDLE),
    ds(NULL),
    cur_offset(0),
    end_offset(0),
    lock_is_wrlock(false),
    rangeIndex(0),
    rangeIndexHeaderSent(false),
    write_file(false)
{
    start = VPLTime_GetTimeStamp();
}

strm_http_transaction::~strm_http_transaction()
{
#ifdef ENABLE_PHOTO_TRANSCODE
    if (is_image_transcoding && ImageTranscode_IsValidHandle(image_transcoding_handle)) {
        ImageTranscode_DestroyHandle(image_transcoding_handle);
    }
#endif
    if(VPLFile_IsValidHandle(fh)) {
        VPLFile_Close(fh);
        fh = VPLFILE_INVALID_HANDLE;
    }
    if (vss_file_handle) {
        if (ds) {
            ds->close_file(vss_file_handle, /*vss_object*/NULL, /*origin*/0);
        }
        else {
            VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                              "vss file handle is not NULL but ds is NULL");
        }
    }
    if(ds != NULL) {
        ds->release();
        ds = NULL;
    }
}

strm_http::strm_http(vss_server& server, VPLSocket_t sockfd, int conn_type,
                     vss_session* session, u64 device_id,  bool need_auth)
    : sockfd(sockfd),
      receiving(false),
      sending(false),
      disconnected(false),
      conn_type(conn_type),
      server(server),
      session(session),
      need_auth(need_auth),
      device_id(device_id),
      next_xid(1),
      sent_so_far(0),
      signing_mode(VSS_NEGOTIATE_SIGNING_MODE_FULL),
      sign_type(VSS_NEGOTIATE_SIGN_TYPE_SHA1),
      encrypt_type(VSS_NEGOTIATE_ENCRYPT_TYPE_AES128),
      proto_version(2),
      reqlen(0),
      req_so_far(0),
      incoming_body(NULL),
      recv_error(false)
{
    last_active = VPLTime_GetTimeStamp();

    string method = "GET";
    register_method_handler(method, handle_get_or_head);
    method = "HEAD";
    register_method_handler(method, handle_get_or_head);
    method = "POST";
    register_method_handler(method, handle_put_post_n_del);
    method = "PUT";
    register_method_handler(method, handle_put_post_n_del);
    method = "DELETE";
    register_method_handler(method, handle_put_post_n_del);

    Util_CreatePhotoMimeMap(photo_mime_map);
    Util_CreateAudioMimeMap(audio_mime_map);
    Util_CreateVideoMimeMap(video_mime_map);

    {
        int rc = 0;

        // Set TCP keep-alive on, probe after 30 seconds inactive, 
        // repeat every 3 seconds, fail if inactive for 60 seconds.
        rc = VPLSocket_SetKeepAlive(sockfd, true, 30, 3, 10);
        if (rc != VPL_OK) {
            VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                              "Stream("FMT_VPLSocket_t"): Failed to set TCP_KEEPALIVE: %d.",
                              VAL_VPLSocket_t(sockfd), rc);
        }
    }
    {
        // TODO: include virtual_device.hpp instead
        extern u64 VirtualDevice_GetDeviceId();
        this->my_device_id = VirtualDevice_GetDeviceId();
    }

    {
        int rc = 0;

        std::ostringstream oss;
        std::string rootTmpFolder = this->server.getRemoteFileTempFolder();
        oss << rootTmpFolder << "/" << VAL_VPLSocket_t(sockfd);
        this->remotefile_tmp_folder = oss.str(); // separate namespace by socket fd

        VPLTRACE_LOG_FINE(TRACE_BVS, 0, "Stream("FMT_VPLSocket_t"): Creating temp remotefile root folder: %s",
                          VAL_VPLSocket_t(sockfd), rootTmpFolder.c_str());

        VPLTRACE_LOG_FINE(TRACE_BVS, 0, "Stream("FMT_VPLSocket_t"): Creating temp remotefile folder: %s",
                          VAL_VPLSocket_t(sockfd), remotefile_tmp_folder.c_str());

        // chek and create root tmp folder
        rc = VPLDir_Create(rootTmpFolder.c_str(), 0755);
        if (rc != VPL_OK && rc != VPL_ERR_EXIST) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Stream("FMT_VPLSocket_t"): Unable to create temporary folder : %s. %d",
                             VAL_VPLSocket_t(sockfd), rootTmpFolder.c_str(), rc);
        }
        // check and clean-up and create folder
        remove_dir_recursive(this->remotefile_tmp_folder);
        rc = VPLDir_Create(this->remotefile_tmp_folder.c_str(), 0755);
        if (rc != VPL_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Stream("FMT_VPLSocket_t"): Unable to create temporary folder : %s. %d",
                             VAL_VPLSocket_t(sockfd), rootTmpFolder.c_str(), rc);
        }
    }
}

strm_http::~strm_http()
{
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Stream("FMT_VPLSocket_t"): closed.",
                      VAL_VPLSocket_t(sockfd));

    // clean-up temporarily folder for remotefile upload
    VPLTRACE_LOG_FINE(TRACE_BVS, 0, "Stream("FMT_VPLSocket_t"): clean-up tmp folder: %s",
                      VAL_VPLSocket_t(sockfd), remotefile_tmp_folder.c_str());
    remove_dir_recursive(this->remotefile_tmp_folder);

    if(!VPLSocket_Equal(sockfd, VPLSOCKET_INVALID)) {
        VPLSocket_Close(sockfd);
    }

    if(session) {
        server.release_session(session);
    }
    
    if(incoming_body) {
        free(incoming_body);
    }

    while(!transaction_queue.empty()) {
        strm_http_transaction* transaction = transaction_queue.front();
        transaction_queue.pop();
        delete transaction;
    }

    while(!send_queue.empty()) {
        pair<size_t, const char*> response = send_queue.front();
        free((void*)(response.second));
        send_queue.pop();
    }
}

u64 strm_http::get_device_id() {
    return this->my_device_id;
}

void strm_http::register_method_handler(std::string& method,
                                        void (*handler)(strm_http*, strm_http_transaction*))
{
    handlers[method] = handler;
}

int strm_http::start(VPLTime_t inactive_timeout)
{
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, 
                      "Stream("FMT_VPLSocket_t"): %s start.",
                      VAL_VPLSocket_t(sockfd),
                      need_auth ? "Secure, authentication needed" 
                      : session ? "Secure, authenticated" : "Non-secure");
    receiving = true;
    this->inactive_timeout = inactive_timeout;
    last_active = VPLTime_GetTimeStamp();
    return 0;
}

void strm_http::do_send()
{
    if (disconnected) return;

    if(send_queue.empty()) {
        get_chunk_to_send();
        if(send_queue.empty()) {
            // Nothing ready to send yet.
            return;
        }
        sent_so_far = 0;
    }

    pair<size_t, const char*> response = send_queue.front();

    int rc = VPLSocket_Send(sockfd,
                            response.second + sent_so_far,
                            response.first - sent_so_far);

    // handle socket errors with read (socket closed).
    if(rc > 0) {
        sent_so_far += rc;
    }
    else if(rc == 0) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Stream("FMT_VPLSocket_t") disconnected.",
                          VAL_VPLSocket_t(sockfd));
        
        disconnected = true;
        receiving = false;
        sending = false;
    }
    else {
        if(rc != VPL_ERR_AGAIN) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Stream("FMT_VPLSocket_t"): Send %d-%d failed:%d", 
                             VAL_VPLSocket_t(sockfd), 
                             response.first, sent_so_far, rc);
            disconnected = true;
            receiving = false;
            sending = false;
        }
    }
        
    if(sent_so_far == response.first) {
        free((void*)(response.second));
        send_queue.pop();
        sent_so_far = 0;
    }

    // When out of replies to send, say so.
    if(transaction_queue.empty() &&
       send_queue.empty()) {
        VPLTRACE_LOG_FINE(TRACE_BVS, 0, "Stream("FMT_VPLSocket_t"): Done.",
                          VAL_VPLSocket_t(sockfd));
        sending = false;
    }

    last_active = VPLTime_GetTimeStamp();
}


void strm_http::get_chunk_to_send()
{
    strm_http_transaction* transaction = NULL;
    char* send_buf = NULL;
    size_t send_len;

    if (transaction_queue.empty()) {
        return;
    }

    transaction = transaction_queue.front();

    if(transaction->receiving || !transaction->sending) {
        // Not ready to send this yet.
        return;
    }

    // Pull chunk of leading transaction to send and put into send queue.
    if(session) {
        // Must secure data before putting in send queue.
        char* data;
        send_buf = (char*)malloc(VSS_HEADER_SIZE +
                                 VSS_TUNNEL_DATA_BASE_SIZE +
                                 SEND_BUFSIZE);
        if(send_buf == NULL) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Stream("FMT_VPLSocket_t"): Failed to alloc data!",
                             VAL_VPLSocket_t(sockfd));
            goto exit;
        }
        data = send_buf + VSS_HEADER_SIZE;
        vss_set_version(send_buf, proto_version); // same version as authenticated
        vss_set_command(send_buf, VSS_TUNNEL_DATA);
        vss_set_status(send_buf, 0);
        vss_set_xid(send_buf, next_xid++);
        vss_set_device_id(send_buf, device_id);
        
        // Get chunk. Maintain block alignment on read form disk.
        send_len = transaction->resp.get_data(vss_tunnel_data_get_data(data), SEND_BUFSIZE);

        vss_set_data_length(send_buf, VSS_TUNNEL_DATA_BASE_SIZE + send_len);
        vss_tunnel_data_set_length(data, send_len);

        session->sign_reply(send_buf, signing_mode, sign_type, encrypt_type);
        // length may change after signing
        send_len = VSS_HEADER_SIZE + vss_get_data_length(send_buf);
    }
    else {
        send_buf = (char*)malloc(SEND_BUFSIZE);
        if(send_buf == NULL) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Stream("FMT_VPLSocket_t"):Failed to alloc data!",
                             VAL_VPLSocket_t(sockfd));
            goto exit;
        }

        send_len = transaction->resp.get_data(send_buf, SEND_BUFSIZE);
    }

    send_queue.push(make_pair(send_len, send_buf));
    transaction->send_cnt += send_len;

    if(transaction->resp.send_done()) {
        transaction_queue.pop();
        transaction_cleanup(transaction, false);
    }

 exit:
    return;
}

void strm_http::do_receive()
{
    int rc;
    char* buf;
    size_t bufsize;

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      "Stream("FMT_VPLSocket_t"): Handling Recv.",
                      VAL_VPLSocket_t(sockfd));

    if(recv_error) {
        goto recv_error;
    }

    // Determine how many bytes to receive.
    bufsize = receive_buffer(&buf);
    if(buf == NULL || bufsize <= 0) {
        // nothing to receive now
        return;
    }

    // Receive them. Handle any socket errors.
    rc = VPLSocket_Recv(sockfd, buf, bufsize);

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      "Stream("FMT_VPLSocket_t"): Recv %u/%u bytes requested.",
                      VAL_VPLSocket_t(sockfd), rc, bufsize);

    if(rc < 0) {
        if(rc == VPL_ERR_AGAIN) {
            // Temporary setback.
            return;
        }
        else {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Stream("FMT_VPLSocket_t"): recv error %d, bufaddr %p, length %u",
                             VAL_VPLSocket_t(sockfd),
                             rc, buf, bufsize);
            goto recv_error;
        }
    }
    else if(rc == 0) { // Socket shutdown. Treat as "error".
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Stream("FMT_VPLSocket_t"): disconnected.",
                          VAL_VPLSocket_t(sockfd));
        goto recv_error;
    }
    else {
        // Handle received bytes.
        handle_received(rc);
    }

    last_active = VPLTime_GetTimeStamp();

    return;
 recv_error:
    // Failed to receive data. Consider connection lost.
    disconnected = true;
    receiving = false;
    sending = false;
}

size_t strm_http::receive_buffer(char** buf_out)
{
    // Determine how much data to receive.
    // Allocate space to receive it as needed.
    size_t rv = 0;
    *buf_out = NULL;

    if(session || need_auth) {
        // Secure channel. Receive a VSSI packet.
        if(reqlen == 0) {
            // If no packet yet in progress, get packet header.
            *buf_out = incoming_hdr + req_so_far;
            rv = VSS_HEADER_SIZE - req_so_far;
        }
        else {
            *buf_out = incoming_body + (req_so_far - VSS_HEADER_SIZE);
            rv = reqlen - req_so_far;
        }
    }
    else {
        // Non-secure channel. Receive fixed block of data.
        if(incoming_body == NULL) {
            incoming_body = (char*)malloc(RECV_BUFSIZE);
            if(incoming_body == NULL) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "Stream("FMT_VPLSocket_t"):Failed to allocate receive buffer.",
                                 VAL_VPLSocket_t(sockfd));
                goto exit;
            }
        }
        rv = RECV_BUFSIZE;
        *buf_out = incoming_body;
    }

 exit:
    return rv;
}

void strm_http::handle_received(int bufsize)
{
    int rc;
    bool verify_device = false;

    if(session || need_auth) {
        // Secure channel. Receive a VSSI packet.
        
        // Accept the bytes received.
        req_so_far += bufsize;
        
        if(req_so_far < VSS_HEADER_SIZE) {
            // Do nothing yet.
            goto exit;
        }
        
        if(req_so_far == VSS_HEADER_SIZE) {
            // Verify the header as good. Must be client session.
            // If auth needed, must get client session first.
            if(need_auth) {
                session = server.get_session(vss_get_handle(incoming_hdr));
                if(session == NULL) {
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                     "Stream("FMT_VPLSocket_t"):Failed to find user session by handle "FMTx64" to authenticate stream.",
                                     VAL_VPLSocket_t(sockfd),
                                     vss_get_handle(incoming_hdr));       
                    recv_error = true;
                    goto exit;
                }
            }

            switch(vss_get_command(incoming_hdr)) {
            case VSS_TUNNEL_DATA:
            case VSS_TUNNEL_DATA_REPLY:
                rc = session->verify_header(incoming_hdr, verify_device,
                                            signing_mode, sign_type);
                break;
            default:
                rc = session->verify_header(incoming_hdr, verify_device,
                                            VSS_NEGOTIATE_SIGNING_MODE_FULL,
                                            VSS_NEGOTIATE_SIGN_TYPE_SHA1);
                break;
            }
            if(rc != 0) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "Stream("FMT_VPLSocket_t"):Failed to authenticate incoming packet header.",
                                 VAL_VPLSocket_t(sockfd));
                recv_error = true;
                goto exit;
            }

            // If device-specific tickets are used we have to verify that
            // the device is linked.
            if ( verify_device ) {
                u64 user_id = session->get_uid();
                u64 device_id = vss_get_device_id(incoming_hdr);
                if ( !server.isDeviceLinked(user_id, device_id) ) {
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                     "Stream("FMT_VPLSocket_t"): Unlinked "
                                     "device"FMTu64".",
                                     VAL_VPLSocket_t(sockfd), device_id);
                    recv_error = true;
                    goto exit;
                }
            }
            
            // Allocate body as needed.
            if(vss_get_data_length(incoming_hdr) > 0) {
                incoming_body = (char*)malloc(vss_get_data_length(incoming_hdr));
                if(incoming_body == NULL) {
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                     "Stream("FMT_VPLSocket_t"):Failed to alloc req body of size %u for req.",
                                     VAL_VPLSocket_t(sockfd),
                                     vss_get_data_length(incoming_hdr));
                    goto exit;
                }
            }

            reqlen = vss_get_data_length(incoming_hdr) + VSS_HEADER_SIZE;
        }
        
        if(reqlen > 0 && reqlen == req_so_far) {
            // Verify the data as good, if there's data.
            int rc;

            switch(vss_get_command(incoming_hdr)) {
            case VSS_TUNNEL_DATA:
            case VSS_TUNNEL_DATA_REPLY:
                rc = session->validate_request_data(incoming_hdr, incoming_body, 
                                                    signing_mode, sign_type, encrypt_type);
                break;
            default:
                rc = session->validate_request_data(incoming_hdr, incoming_body,
                                                    VSS_NEGOTIATE_SIGNING_MODE_FULL,
                                                    VSS_NEGOTIATE_SIGN_TYPE_SHA1,
                                                    VSS_NEGOTIATE_ENCRYPT_TYPE_AES128);
                break;
            }
            if(rc != 0) {
                recv_error = true;
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "Stream("FMT_VPLSocket_t"):Failed to authenticate incoming packet (%x) data.",
                                 VAL_VPLSocket_t(sockfd), vss_get_command(incoming_hdr));
                goto exit;
            }
            
            // Handle received request/data
            if(need_auth) {
                authenticate_connection();
            }
            else {
                switch(vss_get_command(incoming_hdr)) {
                case VSS_TUNNEL_DATA:
                case VSS_TUNNEL_DATA_REPLY: // same thing
                    if(incoming_body != NULL) {
                        digest_input(vss_tunnel_data_get_data(incoming_body),
                                     vss_tunnel_data_get_length(incoming_body));
                    }
                    break;
                case VSS_TUNNEL_RESET:
                case VSS_TUNNEL_RESET_REPLY: // same thing
                    // Reset the tunnel state now.
                    reset_stream();
                    break;
                default:
                    // Send back an error.
                    char* resp = (char*)calloc(VSS_HEADER_SIZE, 1);
                    vss_set_version(resp, vss_get_version(incoming_hdr));
                    vss_set_command(resp, VSS_ERROR);
                    vss_set_status(resp, VSSI_BADCMD);
                    vss_set_xid(resp, vss_get_xid(incoming_hdr));
                    vss_set_device_id(resp, device_id);
                    vss_set_handle(resp, vss_get_handle(incoming_hdr));
                    vss_set_data_length(resp, 0);
                    session->sign_reply(resp);
                    put_proxy_response(resp, VSS_HEADER_SIZE);
                    break;
                }
            }
            free(incoming_body);
            incoming_body = NULL; 
            reqlen = 0;
            req_so_far = 0;
        }

    }
    else {
        // Digest raw buffer.
        digest_input(incoming_body, bufsize);
    }

 exit:
    return;
}

void strm_http::authenticate_connection()
{
    s16 rc = VSSI_SUCCESS;
    char* resp;

    // Expect a VSS_AUTHENTICATE. Anything else is an error.
    if(vss_get_command(incoming_hdr) != VSS_AUTHENTICATE) {
        rc = VSSI_INVALID;
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Stream("FMT_VPLSocket_t"):Received unexpected command code %d when authenticating proxy stream.",
                         VAL_VPLSocket_t(sockfd),
                         vss_get_command(incoming_hdr));     
        recv_error = true;
    }
    // Check cluster ID is the local device ID.
    else if(vss_authenticate_get_cluster_id(incoming_body) !=
            server.clusterId) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Stream("FMT_VPLSocket_t"): intended for device "FMTx64" arrived here at device "FMTx64". Rejecting auth.",
                         VAL_VPLSocket_t(sockfd),
                         vss_authenticate_get_cluster_id(incoming_body),
                         server.clusterId);
        rc = VSSI_WRONG_CLUSTER;
        recv_error = true;
    }
    // Successful authentication.
    else {
        VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                          "Stream("FMT_VPLSocket_t"): Successful authentication.",
                          VAL_VPLSocket_t(sockfd));
        need_auth = false;
        device_id = vss_get_device_id(incoming_hdr);

        if(server.isTrustedNetwork()) {
            if(vss_get_data_length(incoming_hdr) >= VSS_AUTHENTICATE_SIZE_EXT_VER_1) {
                // Authenticate has additional details.
                // Assuming v1 message. Future versions must match v1 layout through v1 size.
                signing_mode = vss_authenticate_get_signing_mode(incoming_body);
                sign_type = vss_authenticate_get_sign_type(incoming_body);
                encrypt_type = vss_authenticate_get_encrypt_type(incoming_body);
            }      
        }
        // else continue using high security settings.
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Stream("FMT_VPLSocket_t") security settings: %d:%d:%d.",
                          VAL_VPLSocket_t(sockfd), signing_mode, sign_type, encrypt_type);
    }
    proto_version = vss_get_version(incoming_hdr);

    // Prepare and send response with result code.
    resp = (char*)calloc(VSS_HEADER_SIZE + VSS_AUTHENTICATER_SIZE_EXT_VER_1, 1);
    vss_set_version(resp, vss_get_version(incoming_hdr));
    vss_set_command(resp, VSS_AUTHENTICATE_REPLY);
    vss_set_status(resp, rc);
    vss_set_xid(resp, vss_get_xid(incoming_hdr));
    vss_set_device_id(resp, device_id);
    vss_set_handle(resp, vss_get_handle(incoming_hdr));

    if(vss_get_data_length(incoming_hdr) == VSS_AUTHENTICATE_SIZE_EXT_VER_0) {
        vss_set_data_length(resp, VSS_AUTHENTICATER_SIZE_EXT_VER_0);
    }
    else {
        // assume version 1
        vss_set_data_length(resp, VSS_AUTHENTICATER_SIZE_EXT_VER_1);
        vss_authenticate_reply_set_ext_version(resp + VSS_HEADER_SIZE, 1);
        vss_authenticate_reply_set_signing_mode(resp + VSS_HEADER_SIZE, signing_mode);
        vss_authenticate_reply_set_sign_type(resp + VSS_HEADER_SIZE, sign_type);
        vss_authenticate_reply_set_encrypt_type(resp + VSS_HEADER_SIZE, encrypt_type);
    }

    session->sign_reply(resp, 
                        VSS_NEGOTIATE_SIGNING_MODE_FULL,
                        VSS_NEGOTIATE_SIGN_TYPE_SHA1,
                        VSS_NEGOTIATE_ENCRYPT_TYPE_AES128);
    put_proxy_response(resp, VSS_HEADER_SIZE + vss_get_data_length(resp));
}

void strm_http::reset_stream()
{
    s16 rc = VSSI_SUCCESS;
    char* resp;

    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      "Stream("FMT_VPLSocket_t"): Reset.",
                      VAL_VPLSocket_t(sockfd));

    // Delete/terminate all transactions.
    while(!transaction_queue.empty()) {
        strm_http_transaction* transaction = transaction_queue.front();
        transaction_queue.pop();
        transaction_cleanup(transaction, true);
    }
    
    // Delete any whole responses where send has not started.
    if(!send_queue.empty()) {
        pair<size_t, const char*> in_progress;

        if(sent_so_far != 0) {
            in_progress = send_queue.front();
            send_queue.pop();
        }

        while(!send_queue.empty()) {
            pair<size_t, const char*> response = send_queue.front();
            free((void*)(response.second));
            send_queue.pop();
        }

        if(sent_so_far != 0) {
            send_queue.push(in_progress);
        }
    }

    // Queue reset reply for send.
    resp = (char*)calloc(VSS_HEADER_SIZE, 1);
    vss_set_version(resp, vss_get_version(incoming_hdr));
    vss_set_command(resp, VSS_TUNNEL_RESET_REPLY);
    vss_set_status(resp, rc);
    vss_set_xid(resp, vss_get_xid(incoming_hdr));
    vss_set_device_id(resp, device_id);
    vss_set_handle(resp, vss_get_handle(incoming_hdr));
    vss_set_data_length(resp, 0);
    session->sign_reply(resp,
                        VSS_NEGOTIATE_SIGNING_MODE_FULL,
                        VSS_NEGOTIATE_SIGN_TYPE_SHA1,
                        VSS_NEGOTIATE_ENCRYPT_TYPE_AES128);
    put_proxy_response(resp, VSS_HEADER_SIZE);
}

void strm_http::digest_input(const char* data, size_t length)
{
    int used = 0;
    s16 rv = VSSI_SUCCESS;

    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      "Stream("FMT_VPLSocket_t"): Digesting %u bytes.",
                      VAL_VPLSocket_t(sockfd), length);
    VPLTRACE_DUMP_BUF_FINE(TRACE_BVS, 0,
                            data, length);

    do {
        if(transaction_queue.empty() ||
           !(transaction_queue.back()->receiving)) {
            VPLTRACE_LOG_INFO(TRACE_BVS, 0, 
                              "Stream("FMT_VPLSocket_t"): Receive new request.",
                              VAL_VPLSocket_t(sockfd));
            transaction_queue.push(new strm_http_transaction(this));
        }
        strm_http_transaction* transaction = transaction_queue.back();

        used += transaction->req.receive(data + used, length - used);
        transaction->recv_cnt += used;

        // Handle transaction if least headers completed and not yet processing.
        if(transaction->req.headers_complete() &&
           !transaction->processing) {
            // Skip handling the transaction if
            // 1. http content length <= 16KB and
            // 2. request body is not yet fully fetched
            // This will help on the new RESTful API with small request body
            if(transaction->req.content_length() <= 16*1024 && !transaction->req.receive_complete()) {
                VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                                  "Stream("FMT_VPLSocket_t"): len "FMTu64" <= 16KB, wait for content",
                                  VAL_VPLSocket_t(sockfd), transaction->req.content_length());
            } else {
                transaction->processing = true;
                handle_transaction(transaction); // may delete transaction
            }
        }
        else if(transaction->processing && transaction->receiving && transaction->write_file) {
            rv = write_upload_file_req(transaction);
            if(rv != 0) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, 
                                "Write upload file failed.");
            }
        }

        // Stop receiving when whole request has been received.
        if(transaction->req.receive_complete()) {
            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                              "Stream("FMT_VPLSocket_t"): Request receive complete.",
                              VAL_VPLSocket_t(sockfd));
            transaction->receiving = false;
            // ...and enable send if needed.
            if(!sending && transaction->sending) {
                sending = true;
            }
        }
    } while (used < length);
}

bool strm_http::active()
{
    bool rv = false;

    // Consider active if connected and there are pending commands,
    // request being received, or response data to send.
    if(!disconnected && 
       (req_so_far > 0 || !transaction_queue.empty() || !send_queue.empty())) {
        rv = true;
    }

    return rv;
}

bool strm_http::inactive()
{
    bool rv = false;
    
    // Consider inactive if disconnected or idle for longer than timeout.
    if(disconnected || recv_error) {
        // If disconnected, definitely inactive, but need all transactions done.
        purge_transactions();
        if(transaction_queue.empty()) {
            rv = true;
            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                              "Stream("FMT_VPLSocket_t"): inactive.",
                              VAL_VPLSocket_t(sockfd));
        }
        // Else must wait for transactions to be terminated.
    }
    else if(transaction_queue.empty() && send_queue.empty()) {
        if(inactive_timeout != VPLTIME_INVALID &&
           VPLTime_GetTimeStamp() > last_active + inactive_timeout) {
            rv = true;
            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                              "Stream("FMT_VPLSocket_t"): timed out after "FMT_VPLTime_t"us inactivity.",
                              VAL_VPLSocket_t(sockfd), VPLTime_GetTimeStamp() - last_active);
        }
    }

    return rv;
}

void strm_http::disconnect()
{
    receiving = false;
    sending = false;
    disconnected = true;
}

void strm_http::purge_transactions()
{
    // Remove any non-processing transaction.
    while(!transaction_queue.empty()) {
        strm_http_transaction* transaction = transaction_queue.front();
        if(!transaction->processing) {
            transaction_queue.pop();
            VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                              "Stream("FMT_VPLSocket_t"): Deleting non-processing transaction %p.",
                              VAL_VPLSocket_t(sockfd), transaction);
            transaction_cleanup(transaction, true);
        }
        else {
            VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                              "Stream("FMT_VPLSocket_t"): Purging request content for processing transaction %p.",
                              VAL_VPLSocket_t(sockfd),
                              transaction);
            transaction->req.purge_content();
            break;
        }
    } 
}

void strm_http::handle_transaction(strm_http_transaction* transaction)
{
    map<string, void (*)(strm_http*, strm_http_transaction*)>::iterator it;
    VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                      "Stream("FMT_VPLSocket_t"): Handling transaction %p.",
                      VAL_VPLSocket_t(sockfd), transaction);
    LOG_REQUEST(transaction);

    // Secure tunnel has already verified the authentication
    // So this check for verifyClientAccess is not necessary

    it = handlers.find(transaction->req.http_method);
    if(it != handlers.end()) {
        it->second(this, transaction);
        return;
    }

    // no handler found. Terminate transaction with default error.
    put_response(transaction);
}

static int check_path_permission(strm_http_transaction *transaction,
                                 const std::string &path,
                                 bool expect_exist,
                                 u32 access_mask,
                                 VPLFS_file_type_t type,
                                 int &status_code,
                                 std::string &response,
                                 bool skip_type = false)
{
    VPLFS_stat_t stat;
    int rc = VPL_OK;
    std::string file_type;

    switch (type) {
    case VPLFS_TYPE_DIR:
        file_type = "directory";
        break;
    case VPLFS_TYPE_FILE:
        file_type = "file";
        break;
    case VPLFS_TYPE_OTHER:
        file_type = "other";
        break;
    }

    // check whether the directory exist or not
    if (expect_exist == true) {
        // checking the parent directory first before we start checking the target path
        // this allow us to identify whether the parent directory is missing as well
        // for detail, check bug #8074
        if (transaction->ds->stat_component(getParentDirectory(path), stat) != VPL_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Parent dir %s %s is missing; responding ERROR 404",
                             file_type.c_str(), path.c_str());
            response = "{\"errMsg\":\""RF_ERR_MSG_NODIR"\"}";
            status_code = 404;
            return -1;
        }
        if (transaction->ds->stat_component(path, stat) != VPL_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "%s %s is missing; responding ERROR 404",
                             file_type.c_str(), path.c_str());
            response = "{\"errMsg\":\""+file_type+" doesn't exist\"}";
            status_code = 404;
            return -1;
        }
        // check whether the file type is expected
        if (!skip_type && (stat.type != type)) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "%s is not %s; responding ERROR 404. type = %d, expected = %d",
                             path.c_str(), file_type.c_str(), stat.type, type);
            response = "{\"errMsg\":\"Not a "+file_type+"\"}";
            status_code = 404;
            return -1;
        }
    } else {
        if (transaction->ds->stat_component(path, stat) == VPL_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "%s %s existed; responding ERROR 409",
                             file_type.c_str(), path.c_str());
            response = "{\"errMsg\":\""+file_type+" existed\"}";
            status_code = 409;
            return -1;
        }
    }

    // check permission
    rc = transaction->ds->check_access_right(path, access_mask);
    if (rc == VPL_ERR_ACCESS) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "%s access denied; responding ERROR 400",
                         path.c_str());
        response = "{\"errMsg\":\"Access denied\"}";
        status_code = 400;
        return -1;
    }
    // only perform the request when check_access_right return VPL_OK
    // or expect_exist = false and error = VPL_ERR_NOENT
    if (rc != VPL_OK && (expect_exist == true || rc != VPL_ERR_NOENT)) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "fail to lookup permission: %s, %d",
                         path.c_str(), rc);
        response = "{\"errMsg\":\"Failed to lookup permission\"}";
        status_code = 500;
        return -1;
    }
    return 0;
}

s16 strm_http::write_upload_file_req(strm_http_transaction* transaction)
{
    int status_code = 200;

    s16 rv = VSSI_SUCCESS;

    std::string response;
    std::string requestPath;
    std::string tempPath;
    std::string* path = NULL;
    std::vector<std::string> uri_tokens;

    u64 length = 0;
    u64 offset = 0;
    std::string content;

    const int BUF_SIZE = 16 * 1024;
    char *data_in = NULL;

    if (transaction->ds == NULL) {
        response = "{\"errMsg\":\"Null dataset\"}";
        status_code = 500;
        rv = -1;
        goto fail;
    }

    // extract from URI and need to be decoded
    VPLHttp_SplitUri(transaction->req.uri, uri_tokens);
    if (uri_tokens.size() < 3) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "path missing in request URI");
        rv = -1;
        response = "{\"errMsg\":\"Null path\"}";
        status_code = 400;
        goto fail;
    }

    // prepare the filepath for later use
    {
        rv = VPLHttp_DecodeUri(uri_tokens[3], requestPath);
        if (rv) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "failed to decode path %s", uri_tokens[3].c_str());
            response = "{\"errMsg\":\"Failed to decode path " + uri_tokens[3] + "\"}";
            status_code = 400;
            goto fail;
        }
    }
    path = &requestPath;

    // expands the aliases. folder access restriction is enforced at handle_rf_file_op()
    // while digesting the file upload request. just do the prefix re-write here.
    apply_prefix_rewrite_rules(*path);

    // one transaction per strm_http. should be safe
    tempPath = this->remotefile_tmp_folder + "/" + getFilenameFromPath(*path);

    // XXX is it possible that the file handle is closed and entry here again?
    // only check if file exist when file is not yet open (not create by us)
    if (!VPLFile_IsValidHandle(transaction->fh)) {
        std::string parent_dir = ::getParentDirectory(*path);

        // check parent directory
        if (check_path_permission(transaction, parent_dir, true,
                                  VPLFILE_CHECK_PERMISSION_WRITE,
                                  VPLFS_TYPE_DIR, status_code, response) != 0) {
            goto fail;
        }
        // check requested file
        if (check_path_permission(transaction, *path, false,
                                  VPLFILE_CHECK_PERMISSION_WRITE,
                                  VPLFS_TYPE_FILE, status_code, response) != 0) {
            goto fail;
        }

        // open tmp file handle in the strm_transaction
        transaction->fh =
            VPLFile_Open(tempPath.c_str(),
                         VPLFILE_OPENFLAG_CREATE | VPLFILE_OPENFLAG_TRUNCATE | VPLFILE_OPENFLAG_READWRITE,
                         0777);

        // check file handle is valid or not
        if (!VPLFile_IsValidHandle(transaction->fh)) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "cannot create temp upload file handle: %s, rv = %d",
                    tempPath.c_str(), transaction->fh);
            response = "{\"errMsg\":\"Unable to open temporarily file handle\"}";
            status_code = 500;
            goto fail;
        }
    }


    // if we get here, it means the file handle is already opened and we are supposely
    // in the process of temporarily file write


    // read the data into data_in
    transaction->req.read_content(content);
    length = content.size();
    data_in = new char[length];
    memcpy(data_in, content.c_str(), length);

    VPLTRACE_LOG_FINE(TRACE_BVS, 0, "write tmp file, length to write: "FMTu64, length);

    do {
        ssize_t bytes_write = VPLFile_Write(transaction->fh, data_in, length);
        if (bytes_write < 0) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "write tmp file failed, rv: %d, cur_offset = "FMTu64,
                             bytes_write, transaction->cur_offset);
            response = "{\"errMsg\":\"Failed to write to temporarily file\"}";
            status_code = 500;
            rv = -1;
            goto fail_tmp_write;
        }
        length -= bytes_write;
        transaction->cur_offset += bytes_write;
    } while (length > 0);

    // clean-up data alocated
    delete [] data_in;
    data_in = NULL;

    // If we have not received all the contents and there are no errors, return here.
    if ((rv == 0) && (transaction->cur_offset < transaction->req.content_length())) {
        return rv;
    }


    // if we get here, it means we've already received all the content and have already written to the
    // temporarily file. Let's copy the content from tmp file to the dataset file

    VPLTRACE_LOG_FINE(TRACE_BVS, 0, "open_vss_write_handle for %s", path->c_str());

    // open vss file handle
    rv = open_vss_write_handle(transaction, *path);
    if(rv != 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "open_vss_write_handle:%d, %s", rv, path->c_str());
        response = "{\"errMsg\":\"Failed to open VSS file handle\"}";
        status_code = 500;
        goto fail_tmp_write;
    }

    // reset the tmp file cursor
    VPLFile_Seek(transaction->fh, 0, VPLFILE_SEEK_SET);

    data_in = new char[BUF_SIZE];
    length = transaction->req.content_length();// content length;
    offset = 0;
    do {
        ssize_t src_read = VPLFile_Read(transaction->fh, data_in, BUF_SIZE);
        if (src_read < 0) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "error while reading tmp file, rv = %d, total = "FMTu64,
                             src_read, length);
            response = "{\"errMsg\":\"Failed to read data from temporarily file\"}";
            status_code = 500;
            rv = -1;
            goto fail_vss_write;
        }
        u32 dst_write = src_read;
        rv = transaction->ds->write_file(transaction->vss_file_handle,
                NULL,
                0,
                offset,
                dst_write,
                data_in);
        if ((rv != 0) || (dst_write != src_read) ){
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "write data failed:%d or length un-equalled, return bad response.", rv);
            response = "{\"errMsg\":\"Failed to write file\"}";
            status_code = 500;
            rv = -1;
            goto fail_vss_write;
        }
        offset += src_read;
    } while (offset < length);

    // If there are no errors, set the size.
    rv = transaction->ds->truncate_file(transaction->vss_file_handle,
                                        NULL,
                                        0,
                                        transaction->cur_offset);
    if(rv != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "write data set_size fail");
        response = "{\"errMsg\":\"Failed to write file due to setting file size failed\"}";
        status_code = 500;
    }

fail_vss_write:
    // close vss file handle and remove the file is error happens
    close_vss_handle(transaction);

    // delete file if failed before closing file handle
    if(rv != VSSI_SUCCESS && path != NULL) {
        int rc = transaction->ds->remove_iw(*path);
        if (rc == VSSI_SUCCESS) {
            rc = transaction->ds->commit_iw();
        } else if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "failed to clean-up for upload file failed: %s, %d",
                             path->c_str(), rc);
        }
    }

fail_tmp_write:
    if (data_in != NULL) {
        delete [] data_in;
        data_in = NULL;
    }

    if (VPLFile_IsValidHandle(transaction->fh)) {
        int rc = 0;
        VPLFile_Close(transaction->fh);
        transaction->fh = VPLFILE_INVALID_HANDLE;
        // delete tmp file
        rc = VPLFile_Delete(tempPath.c_str());
        if (rc) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "failed to clean-up tmp file: %s, %d",
                             tempPath.c_str(), rc);
        }
    }

fail:
    ::populate_response(transaction->resp, status_code, response, "application/json");
    transaction->receiving = false;
    transaction->processing = false;
    transaction->sending = true;
    put_response(transaction);

    return rv;
}

void strm_http::put_response(strm_http_transaction* transaction)
{
    transaction->processing = false;

    // Don't queue to send unless connected.
    if(!disconnected) {
        VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                          "Stream("FMT_VPLSocket_t"): Have response for transaction %p.",
                          VAL_VPLSocket_t(sockfd), transaction);
        LOG_RESPONSE(transaction);

        transaction->sending = true;
        
        // If this was a HEAD request, drop the response content.
        if(transaction->req.http_method == "HEAD") {
            transaction->resp.content.clear();
            transaction->resp.set_data_fetch_callback(NULL, NULL);
        } else if (transaction->resp.headers.size() == 0 && transaction->resp.response == 500) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                    "Stream("FMT_VPLSocket_t"): response is not touched (500).",
                    VAL_VPLSocket_t(sockfd));
            ::populate_response(transaction->resp, 500, "", "");
        }

        // If still receiving, purge all content.
        if(transaction->receiving) {
            transaction->req.purge_content();
        }
    
        if(!sending &&
           transaction_queue.front() == transaction &&
           !transaction->receiving) {
            VPLTRACE_LOG_FINE(TRACE_BVS, 0, 
                              "Stream("FMT_VPLSocket_t"): Start sending.",
                              VAL_VPLSocket_t(sockfd));
            sending = true;
            server.changeNotice();
        }
    }

    last_active = VPLTime_GetTimeStamp();
}

void strm_http::put_proxy_response(const char* msg, size_t length)
{
    if(!disconnected) {
        send_queue.push(make_pair(length, msg));

        if(!sending) {
            VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                              "Stream("FMT_VPLSocket_t"): Start sending.",
                              VAL_VPLSocket_t(sockfd));
            sending = true;
            server.changeNotice();
        }

        last_active = VPLTime_GetTimeStamp();
    }
}

static int parse_uri(const std::string& uri,
                     std::string& content) {

    int rv = 0;

    content = uri;
    
    // chop leading slashes
    while (content[0] == '/') {
        content.erase(0, 1);
    }

    return rv;
}

static size_t strm_http_fetch_data(char* buf, size_t len, void* ctx, bool& done)
{
    strm_http_transaction* transaction = (strm_http_transaction*)(ctx);
    size_t rv = 0;
    int rc;

    // Attempt to read the file's contents for requested length.
    if(len > (transaction->end_offset - transaction->cur_offset)) {
        // Fetch no further than the last byte of request range.
        len = (transaction->end_offset - transaction->cur_offset);
    }
    // Make sure fetches are aligned nicely for disk.
    // 4k alignment should be good.
    // If more than 4k, stop at last 4k boundary within range.
    if(len > 4096) {
        len = (((transaction->cur_offset + len) & ~0xFFF) - 
                   transaction->cur_offset);
    }

    if (VPLFile_IsValidHandle(transaction->fh)) {
        rc = VPLFile_ReadAt(transaction->fh, (void*)(buf), len, 
                            transaction->cur_offset);
    }
    else {
        rc = transaction->ds->read_file(transaction->vss_file_handle,
                                        NULL,
                                        0,
                                        transaction->cur_offset,
                                        len,
                                        buf);
        if(rc == VSSI_SUCCESS) {
            rc = len;
        }
    }
    if(rc > 0) {
        transaction->cur_offset += rc;
        rv = rc;
    }
    else {
        // On read error, consider file done.
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Stream("FMT_VPLSocket_t"): Failed reading file %s for %u bytes from offset "FMTu64" with error %d",
                         VAL_VPLSocket_t(transaction->http_handle->sockfd),
                         transaction->content_file.c_str(),
                         len, transaction->cur_offset, rc);
        transaction->cur_offset = transaction->end_offset;
    }

    if(transaction->cur_offset == transaction->end_offset) {
        done = true;
    }
    else {
        done = false;
    }

    return rv;
}

#define MULTI_RANGE_BOUNDARY_SEPARATOR "\r\n--%s\r\n"                     \
                                       "Content-Type: %s\r\n"             \
                                       "Content-Range: bytes "            \
                                       FMTu_VPLFile_offset_t"-"           \
                                       FMTu_VPLFile_offset_t              \
                                       "/"FMTu_VPLFile_offset_t"\r\n\r\n"
#define MULTI_RANGE_HEADER_SIZE_MAX 256

#define MULTI_RANGE_BOUNDARY_END       "\r\n--%s--\r\n"

static size_t strm_http_fetch_multirange_data(char* buf, size_t len, void* ctx, bool& done)
{
    strm_http_transaction* transaction = (strm_http_transaction*)(ctx);
    size_t rv = 0;
    done = false;

    // See http://www.ietf.org/rfc/rfc2616.txt
    // Section 19.2 for multi-range body format.

    while(transaction->rangeIndex < transaction->ranges.size())
    {
        RangeHeader & range = transaction->ranges[transaction->rangeIndex];
        if(len-rv < range.header.size()) {
            break;
        }
        if(!transaction->rangeIndexHeaderSent) {
            memcpy(buf+rv, range.header.c_str(), range.header.size());
            rv += range.header.size();
            transaction->rangeIndexHeaderSent = true;
            transaction->cur_offset = range.start;
            transaction->end_offset = range.end+1;// +1 because http spec is inclusive.  (ie. range "5-5" is 1 byte)
        }
        size_t bytesSent;
        bool subDone = false;
        bytesSent = strm_http_fetch_data(buf+rv, len-rv, transaction, subDone);
        rv += bytesSent;
        if(!subDone){
            break;  // No more room
        }

        // move to next range
        transaction->rangeIndex++;
        transaction->rangeIndexHeaderSent = false;
        transaction->cur_offset = 0;
        transaction->end_offset = 0;
    }

    if(transaction->rangeIndex >= transaction->ranges.size())
    {  // Send the end delimiter
        if(!transaction->rangeIndexHeaderSent) {
            if(len-rv >= transaction->footer.size()) {
                memcpy(buf+rv, transaction->footer.c_str(), transaction->footer.size());
                rv += transaction->footer.size();
                transaction->rangeIndexHeaderSent = true;
                done = true;
            }
        }else{  // footer already sent.
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,"Callback called extra time.");
            done = true;
        }
    }

    return rv;
}

static void toHashString(const u8* hash, std::string& hashString)
{
    // Write recorded hash
    hashString.clear();
    for(int hashIndex = 0; hashIndex<CSL_SHA1_DIGESTSIZE; hashIndex++) {
        char byteStr[4];
        sprintf(byteStr, "%02"PRIx8, hash[hashIndex]);
        hashString.append(byteStr);
    }
}

// Preconditions:
//  a) transaction is valid
//  b) transaction->ds is NULL
static void get_mediafile_resp(const std::string& filepath, int mediaType, 
                               strm_http_transaction* transaction)
{
    int rc;
    VPLFile_offset_t total;
    char             date[30];
    string           extension;
    const string*    reqRange = NULL;
    time_t           curTime;
    map<string, string, case_insensitive_less>::iterator mime_it;
    bool             invalidRange = false;
    VPLFS_stat_t     stat;

    std::string rangeBoundarySep; // the range boundary separator string.
    std::string mimeType;
    VPLFile_offset_t fileSize;

    transaction->resp.content.clear();
    transaction->ranges.clear();
    transaction->footer.clear();

    curTime = time(NULL);
    strftime(date, 30,"%a, %d %b %Y %H:%M:%S GMT" , gmtime(&curTime));

    // Determine MIME type.
    // The caller may have given some hints via mediaType param.
    // In particular, /mm/ requests are always with a good hint (MEDIA_TYPE_PHOTO or MEDIA_TYPE_AUDIO or MEDIA_TYPE_VIDEO)
    // /rf/json/download comes with no hints (MEDIA_TYPE_UNKNOWN).

    // Try to determine MIME type from file name extension.
    size_t last_dot = filepath.find_last_of("./");
    if (last_dot != std::string::npos && filepath[last_dot] == '.') {
        extension = filepath.substr(last_dot + 1);
        VPLTRACE_LOG_FINE(TRACE_BVS, 0, "Filename extension: %s", extension.c_str());

        // Search the list of known extensions.
        if (mediaType == MEDIA_TYPE_PHOTO ||
            (mediaType == MEDIA_TYPE_UNKNOWN && mimeType.empty())) {
            mime_it = transaction->http_handle->photo_mime_map.find(extension);
            if (mime_it != transaction->http_handle->photo_mime_map.end()) {
                mimeType = mime_it->second;
            }
        }
        if (mediaType == MEDIA_TYPE_AUDIO ||
            (mediaType == MEDIA_TYPE_UNKNOWN && mimeType.empty())) {
            mime_it = transaction->http_handle->audio_mime_map.find(extension);
            if (mime_it != transaction->http_handle->audio_mime_map.end()) {
                mimeType = mime_it->second;
            }
        }
        if (mediaType == MEDIA_TYPE_VIDEO ||
            (mediaType == MEDIA_TYPE_UNKNOWN && mimeType.empty())) {
            mime_it = transaction->http_handle->video_mime_map.find(extension);
            if (mime_it != transaction->http_handle->video_mime_map.end()) {
                mimeType = mime_it->second;
            }
        }
    }

    // If MIME type is still undetermined, ...
    if (mimeType.empty()) {
        switch (mediaType) {
        case MEDIA_TYPE_PHOTO:
            mimeType = "image/unknown";
            break;
        case MEDIA_TYPE_AUDIO:
            mimeType = "audio/unknown";
            break;
        case MEDIA_TYPE_VIDEO:
            mimeType = "video/unknown";
        default:
            ; // leave mimeType empty
        }
    }

    VPLTRACE_LOG_FINE(TRACE_BVS, 0, "MIME: %s.", mimeType.c_str());

    if (transaction->ds == NULL) {
        if ((rc = VPLFS_Stat(filepath.c_str(), &stat)) != VPL_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "%d, %s missing; responding ERROR 404", rc, filepath.c_str());
        }
        if (rc != VPL_OK) {
            ::populate_response(transaction->resp, 404, "{\"errMsg\":\"Content not found.\"}", "application/json");
            goto out;
        }
    }
    else {
        if ((rc = transaction->ds->stat_component(filepath, stat)) != VPL_OK) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "%d, %s missing; responding ERROR 404", rc, filepath.c_str());
            ::populate_response(transaction->resp, 404, "{\"errMsg\":\"Content not found.\"}", "application/json");
            goto out;
        }
    }
    if (stat.type != VPLFS_TYPE_FILE) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "%d, %s not a file; responding ERROR 404", (int)stat.type, filepath.c_str());
        ::populate_response(transaction->resp, 404, "{\"errMsg\":\"Content not found.\"}", "application/json");
        goto out;
    }
    fileSize = stat.size;

    reqRange = transaction->req.find_header("Range");
    if(reqRange) {
        // http://www.ietf.org/rfc/rfc2616.txt,
        // 14.35 - Range header
        // See http://bugs.ctbg.acer.com/show_bug.cgi?id=1020

        VPLFile_offset_t startToken = 0;
        VPLFile_offset_t endToken = 0;
        bool isBytesUnit = false;

        for(;startToken < reqRange->size(); startToken=endToken+1) {
            string currentToken;

            endToken = reqRange->find(',', startToken);
            if(endToken != string::npos) {
                currentToken = reqRange->substr(startToken, endToken-startToken);
            }else{
                currentToken = reqRange->substr(startToken);
                endToken = reqRange->size();
            }

            // check for "bytes" in the range (example)
            //     "Range: bytes=100-199,1000001000-1000001008"
            size_t optionalBytes = currentToken.find("=");
            if(optionalBytes != string::npos) {
                string str_bytes_unit = currentToken.substr(0, optionalBytes);
                trim(str_bytes_unit);
                if(str_bytes_unit.compare("bytes") != 0) {
                    // Invalid range unit
                    isBytesUnit = false;
                    continue;
                }
                isBytesUnit = true;
                // Skip past "="
                currentToken = currentToken.substr(optionalBytes+1);
            }
            if(!isBytesUnit)
            {   // 3.12: The only range unit defined by HTTP/1.1 is "bytes". HTTP/1.1
                // implementations MAY ignore ranges specified using other units.
                continue;
            }

            // currentToken should be in format "( byte-range-spec | suffix-byte-range-spec )"
            // Referring to 14.35 of http://www.ietf.org/rfc/rfc2616.txt

            // Finding dash in: byte-range-spec = first-byte-pos "-" [last-byte-pos]
            size_t dashPos = currentToken.find('-');
            if(dashPos == string::npos) {
                continue;
            }

            // http://www.ietf.org/rfc/rfc2616.txt  Secion 14.35
            // first-byte-pos  = 1*DIGIT
            // last-byte-pos   = 1*DIGIT
            string str_first_byte_pos;
            string str_last_byte_pos;
            str_first_byte_pos = currentToken.substr(0, dashPos);
            str_last_byte_pos = currentToken.substr(dashPos + 1);
            trim(str_first_byte_pos);
            trim(str_last_byte_pos);

            // Eliminate range definitions with invalid characters
            if(str_first_byte_pos.find_first_not_of("0123456789") != string::npos) {
                continue;
            }
            if(str_last_byte_pos.find_first_not_of("0123456789") != string::npos) {
                continue;
            }

            // Must accept ranges only where end >= start,
            // except for suffix-range where start is not defined.
            // Also check range against file size.
            RangeHeader range;
            if(str_first_byte_pos.empty())
            {   // case suffix-byte-range-spec = "-" suffix-length
                if(str_last_byte_pos.empty()) {
                    // In suffix-byte-range-spec, suffix-length cannot be empty
                    continue;
                }
                else {
                    // Suffix range. Get last N bytes of file.
                    range.end = strtoull(str_last_byte_pos.c_str(), 0, 10);
                    if(fileSize > range.end) {
                        range.start = (fileSize - range.end);
                    }
                    else {
                        // Get whole file if file too small.
                        range.start = 0;
                    }
                    range.end = fileSize - 1;

                }
            }
            else
            {   // byte-range-spec = first-byte-pos "-" [last-byte-pos]
                range.start = strtoull(str_first_byte_pos.c_str(), 0, 10);
                if(range.start > fileSize - 1) {
                    // Invalid range - start past EOF
                    continue;
                }
                if(str_last_byte_pos.empty()) {
                    // Get from start through EOF
                    range.end = fileSize - 1;
                }
                else {
                    // Get start-end inclusive
                    range.end = strtoull(str_last_byte_pos.c_str(), 0, 10);
                    if(range.end < range.start) {
                        // Invalid. Must ignore.
                        continue;
                    }
                    if(range.end > fileSize - 1) {
                        // End past EOF: stop at EOF.
                        range.end = fileSize - 1;
                    }
                }
            }
            transaction->ranges.push_back(range);
        }
    }
    else {
        // Get whole entity.
        RangeHeader range;
        range.start = 0;
        range.end = fileSize - 1;
        transaction->ranges.push_back(range);
    }

    if(transaction->ranges.size()==0) {
        invalidRange = true;
        goto out;
    }

    // Add the common headers first.
    transaction->resp.add_header(CONTENT_TYPE, mimeType.data());
    if ((strncmp(mimeType.data(), "video", 5) == 0) ||
        (strncmp(mimeType.data(), "audio", 5) == 0)) {
        transaction->resp.add_header("transferMode.dlna.org", "Streaming");
    } else {
        transaction->resp.add_header("transferMode.dlna.org", "Interactive");
    }
    transaction->resp.add_header("Accept-Ranges", "bytes");
    transaction->resp.add_header("Connection", "Keep-Alive");
    transaction->resp.add_header("Date", date);
    transaction->resp.add_header("Ext", "");
    transaction->resp.add_header("realTimeInfo.dlna.org", "DLNA.ORG_TLAG=*");
    transaction->resp.add_header("Server", IGWARE_SERVER_STRING);

    if (transaction->req.find_header("getcontentFeatures.dlna.org")) {
        transaction->resp.add_header("contentFeatures.dlna.org", "DLNA.ORG_OP=01;DLNA.ORG_FLAGS=00100000000000000000000000000000");
    }

    // Content to be read from file on-demand until done.
    transaction->content_file = filepath;
    if (transaction->ds == NULL) {
        transaction->fh = VPLFile_Open(filepath.c_str(), VPLFILE_OPENFLAG_READONLY, 0);
        if (!VPLFile_IsValidHandle(transaction->fh)) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Error opening %s responding ERROR 404", filepath.c_str());
            ::populate_response(transaction->resp, 404, "{\"errMsg\":\"Content not found.\"}", "application/json");
            goto out;
        }
    }else{
        int rc = transaction->ds->open_file(filepath,
                                            0,
                                            VSSI_FILE_OPEN_READ | VSSI_FILE_SHARE_READ,
                                            0,
                                            transaction->vss_file_handle);
        if(rc != 0) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "open_file:%d, for %s",
                             rc, filepath.c_str());
            ::populate_response(transaction->resp, 404, "{\"errMsg\":\"Content not found.\"}", "application/json");
            goto out;
        }
    }

    if(transaction->ranges.size()==1) {
        VPLFile_offset_t start = transaction->ranges[0].start;
        VPLFile_offset_t end = transaction->ranges[0].end;
        VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Stream mediafile %s. "
                          FMTu_VPLFile_offset_t"-"FMTu_VPLFile_offset_t".",
                          filepath.c_str(), start, end);
        total = end - start + 1;  // +1 because http spec is inclusive.  (ie. range "5-5" is 1 byte)

        char contentRangeValue[128];
        char contentLengthValue[128];
        sprintf(contentLengthValue, FMTu_VPLFile_offset_t, total);
        sprintf(contentRangeValue, "bytes "FMTu_VPLFile_offset_t"-"
                                           FMTu_VPLFile_offset_t"/"
                                           FMTu_VPLFile_offset_t,
                start, end, fileSize);

        // Indicate if whole file or only part being returned.
        if(start == 0 && end == fileSize - 1) {
            transaction->resp.add_response(200);
        }
        else {
            transaction->resp.add_response(206);
            transaction->resp.add_header(CONTENT_RANGE, contentRangeValue);
        }

        transaction->resp.add_header(CONTENT_LENGTH, contentLengthValue);

        // Set parameters for streaming content in response.
        transaction->cur_offset = start;
        transaction->end_offset = end + 1; // +1 because http spec is inclusive.  (ie. range "5-5" is 1 byte)
        if (transaction->req.http_method == "HEAD") {
            transaction->http_handle->put_response(transaction);
        } else if (fileSize > 0) {
            transaction->resp.set_data_fetch_callback(strm_http_fetch_data,
                                                      transaction);
        }
    }else{  // Multi-range request
        {
            // If a file wants to forge http multi-range separators, no big deal, client
            // should be using "prepended length" in Content-Range header.
            // Just need to be obscure/long enough that accidental collision doesn't happen
            // for bad client implementations
            CSL_ShaContext context;
            unsigned char boundary[CSL_SHA1_DIGESTSIZE]; //file signature.
            string salt("strm_http_salt--for http multirange separator boundary");
            CSL_ResetSha(&context);
            CSL_InputSha(&context, transaction->content_file.c_str(),
                                   transaction->content_file.size());
            CSL_InputSha(&context, salt.c_str(), salt.size());
            CSL_ResultSha(&context, (unsigned char*)boundary);
            toHashString(boundary, rangeBoundarySep);
        }

        // Calculate the size of all the ranges in the body
        VPLFile_offset_t total_body_len = 0;
        int snprintfBytes;
        for(int i=0; i<transaction->ranges.size();i++) {
            char header[MULTI_RANGE_HEADER_SIZE_MAX];
            snprintfBytes = snprintf(header, MULTI_RANGE_HEADER_SIZE_MAX,
                                     MULTI_RANGE_BOUNDARY_SEPARATOR,
                                     rangeBoundarySep.c_str(),
                                     mimeType.c_str(),
                                     transaction->ranges[i].start,
                                     transaction->ranges[i].end,
                                     fileSize);
            if(snprintfBytes >= MULTI_RANGE_HEADER_SIZE_MAX) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,"Overflow:%d", snprintfBytes);
                snprintfBytes = MULTI_RANGE_HEADER_SIZE_MAX;
            }
            transaction->ranges[i].header = header;
            total_body_len += transaction->ranges[i].header.size();
            total_body_len += transaction->ranges[i].end -
                              transaction->ranges[i].start +
                              1;  // +1 because http range spec is inclusive.  (ie. range "5-5" is 1 byte)
        }
        {
            char footer[MULTI_RANGE_HEADER_SIZE_MAX];
            snprintfBytes = snprintf(footer, MULTI_RANGE_HEADER_SIZE_MAX,
                                     MULTI_RANGE_BOUNDARY_END,
                                     rangeBoundarySep.c_str());
            if(snprintfBytes >= MULTI_RANGE_HEADER_SIZE_MAX) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,"Overflow:%d", snprintfBytes);
                snprintfBytes = MULTI_RANGE_HEADER_SIZE_MAX;
            }
            transaction->footer = footer;
            total_body_len += transaction->footer.size();
        }
        transaction->resp.add_response(206);
        char contentTypeStr[256];
        snprintfBytes = snprintf(contentTypeStr, 256,
                                 "multipart/byteranges; boundary=%s",
                                 rangeBoundarySep.c_str());
        if(snprintfBytes >= 256) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,"Overflow:%d", snprintfBytes);
            snprintfBytes = 256;
        }
        transaction->resp.add_header(CONTENT_TYPE, contentTypeStr);
        char contentLengthStr[128];
        snprintfBytes = snprintf(contentLengthStr, 128,
                                 FMTu_VPLFile_offset_t,
                                 total_body_len);
        if(snprintfBytes >= 128) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,"Overflow:%d", snprintfBytes);
            snprintfBytes = 128;
        }

        if (transaction->req.http_method != "HEAD") {
            transaction->resp.add_header(CONTENT_LENGTH, contentLengthStr);
        }

        transaction->rangeIndex = 0;
        transaction->rangeIndexHeaderSent = false;

        if (transaction->req.http_method == "HEAD") {
            transaction->http_handle->put_response(transaction);
        } else {
            transaction->resp.set_data_fetch_callback(strm_http_fetch_multirange_data,
                                                      transaction);
        }
    }

out:
    if(invalidRange)
    {
        if(reqRange) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, 
                             "Range \"%s\" invalid for size "FMTu_VPLFile_offset_t".",
                             reqRange->c_str(), fileSize);
        }
        char contentRangeValue[128];
        sprintf(contentRangeValue, "bytes */"FMTu_VPLFile_offset_t, 
                fileSize);
        transaction->resp.add_header(CONTENT_RANGE, contentRangeValue); 
        transaction->resp.add_response(416);
    }
    return;
}

struct GetMediaFileResp_ctx {
    const std::string filepath;
    int mediaType;
    strm_http_transaction* transaction;
    strm_http* http;
    GetMediaFileResp_ctx(const std::string& myFilepath,
                         int myMediaType,
                         strm_http_transaction* myTransaction,
                         strm_http* myHttp)
    :  filepath(myFilepath),
       mediaType(myMediaType),
       transaction(myTransaction),
       http(myHttp)
    {}
};

static void get_mediafile_resp_helper(void* ctx)
{
    GetMediaFileResp_ctx* getRespCtx = (GetMediaFileResp_ctx*) ctx;
    get_mediafile_resp(getRespCtx->filepath,
                       getRespCtx->mediaType,
                       getRespCtx->transaction);

    getRespCtx->http->put_response(getRespCtx->transaction);

    delete getRespCtx;
}

// return the namespace part from uri
static std::string get_uri_namespace(const std::string &uri)
{
    // If uri begins with '/', namespace is between first and second '/'.
    // Otherwise, it's before the first '/'.
    // Examples: "/namespace/foo/bar", "namespace/foo/bar"
    if (uri.empty())
        return "badns";
    size_t ns_begin = uri[0] == '/' ? 1 : 0;
    if (ns_begin >= uri.length())
        return "badns";
    size_t ns_end = uri.find_first_of('/', ns_begin);
    if (ns_end == uri.npos)
        return "badns";
    return uri.substr(ns_begin, ns_end - ns_begin);

}

static void handle_ns_test(strm_http* http, strm_http_transaction* transaction,
                           const std::string& content,
                           bool& goto_respond, bool& is_deferred, bool& doRead,
                           std::string& mediaFile, int& mediaType)
{
    // Read default test file
    transaction->http_handle->server.get_test_media_file(mediaFile, &mediaType);
    goto_respond = false;
    doRead = true;
}

#ifdef TS_DEV_TEST
// Temporary test code added to TS development.
// FIXME: Remove this code before public release.
static void handle_ns_tstest(strm_http* http, strm_http_transaction* transaction,
                             const std::string& content,
                             bool& goto_respond, bool& is_deferred, bool& doRead,
                             std::string& mediaFile, int& mediaType)
{
    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "handle_ns_tstest");
    std::string response;
    response.resize(1024*1024, 'a');
    ::populate_response(transaction->resp, 200, response, "text/plain");
    goto_respond = true;
}
#endif // TS_DEV_TEST

static void handle_ns_mm(strm_http* http, strm_http_transaction* transaction,
                         const std::string& content,
                         bool& goto_respond, bool& is_deferred, bool& doRead,
                         std::string& mediaFile, int& mediaType)
{
    const string token ("/c/");
    bool isThumbnail = ( string::npos == transaction->req.uri.find( token ) ) ? true : false;
#ifdef ENABLE_PHOTO_TRANSCODE
    ImageTranscode_ImageType image_type = ImageType_Original;
    size_t width = -1;
    size_t height = -1;
#endif

    // Find optional header 'X-ac-collectionId' - if this exists, we will only load this collection.
    const std::string *ac_collectionId = transaction->req.find_header("X-ac-collectionId");
    std::string collectionId;
    if (ac_collectionId) {
        int err = VPLHttp_DecodeUri(*ac_collectionId, collectionId);
        if (err) collectionId.clear();
    }

    vss_server::msaGetObjectMetadataFunc_t getObjectMetadataCb = http->server.getMsaGetObjectMetadataCb();
    if (getObjectMetadataCb == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, 
                         "Stream("FMT_VPLSocket_t"): Not configured to stream media, responding ERROR 500",
                         VAL_VPLSocket_t(http->sockfd));
        ::populate_response(transaction->resp, 500, "{\"errMsg\":\"Not configured to stream media.\"}", "application/json");
        goto_respond = true;
        return;
    }
    media_metadata::GetObjectMetadataInput request;
    request.set_url(transaction->req.uri);
    media_metadata::GetObjectMetadataOutput response;
    int rc = getObjectMetadataCb(request, collectionId, response, http->server.getMsaCallbackContext());
    if (rc != 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Stream("FMT_VPLSocket_t"): Content not found, responding ERROR 404 [url %s], %d",
                         VAL_VPLSocket_t(http->sockfd),
                         transaction->req.uri.c_str(), rc);
        ::populate_response(transaction->resp, 404, "{\"errMsg\":\"Content not found.\"}", "application/json");
        goto_respond = true;
        return;
    }
    doRead = true;
    switch (response.media_type()) {
    case media_metadata::MEDIA_MUSIC_TRACK:
        if (response.has_absolute_path()) {
            mediaFile = response.absolute_path();
            VPLTRACE_LOG_INFO(TRACE_BVS, 0, 
                              "Stream("FMT_VPLSocket_t"): music_track \"%s\"",
                              VAL_VPLSocket_t(http->sockfd),
                              mediaFile.c_str());
        } else {
            VPLTRACE_LOG_INFO(TRACE_BVS, 0, 
                              "Stream("FMT_VPLSocket_t"): Cannot find music_track, responding ERROR 404 [url %s]",
                              VAL_VPLSocket_t(http->sockfd),
                              transaction->req.uri.c_str());
            ::populate_response(transaction->resp, 404, "{\"errMsg\":\"Content not found.\"}", "application/json");
            goto_respond = true;
            return;
        }
        mediaType = MEDIA_TYPE_AUDIO;
        break;
    case media_metadata::MEDIA_PHOTO:

        if ( false == isThumbnail ) {
            if (response.has_absolute_path()) {
                mediaFile = response.absolute_path();
                VPLTRACE_LOG_INFO(TRACE_BVS, 0, 
                                  "Stream("FMT_VPLSocket_t"): photo_item \"%s\"",
                                  VAL_VPLSocket_t(http->sockfd),
                                  mediaFile.c_str());
            } else {
                VPLTRACE_LOG_INFO(TRACE_BVS, 0, 
                                  "Stream("FMT_VPLSocket_t"): Cannot find photo_item, responding ERROR 404 [url %s]",
                                  VAL_VPLSocket_t(http->sockfd),
                                  transaction->req.uri.c_str());
                ::populate_response(transaction->resp, 404, "{\"errMsg\":\"Content not found.\"}", "application/json");
                goto_respond = true;
                return;
            }
#ifdef ENABLE_PHOTO_TRANSCODE
            // Check Header - act_xcode_dimension
            const std::string* header_act_xcode_dimension = transaction->req.find_header(HEADER_ACT_XCODE_DIMENSION);
            if (header_act_xcode_dimension && header_act_xcode_dimension->find(",") != std::string::npos) {
                std::string value_header_act_xcode_dimension = *header_act_xcode_dimension;
                std::string str_width;
                std::string str_height;
                const std::string* header_act_xcode_fmt;

                trim(value_header_act_xcode_dimension);

                str_width = header_act_xcode_dimension->substr(0, value_header_act_xcode_dimension.find(","));
                str_height = header_act_xcode_dimension->substr(value_header_act_xcode_dimension.find(",") + 1,
                                                                value_header_act_xcode_dimension.size());
                trim(str_width);
                trim(str_height);

                width = atoi(str_width.c_str());
                height = atoi(str_height.c_str());
                VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                                  "Transcode image size "FMTu_size_t" x "FMTu_size_t" (%s)",
                                  width,
                                  height,
                                  value_header_act_xcode_dimension.c_str());

                // Set the transcoding flag.
                transaction->is_image_transcoding = true;

                // Check Header - act_xcode_fmt
                header_act_xcode_fmt = transaction->req.find_header(HEADER_ACT_XCODE_FMT);
                if (header_act_xcode_fmt) {
                    std::string value_act_xcode_fmt = *header_act_xcode_fmt;
                    trim(value_act_xcode_fmt);
                    // To uppercase
                    for (int i = 0; i < value_act_xcode_fmt.size(); i++) {
                        value_act_xcode_fmt[i] = uppercase(value_act_xcode_fmt[i]);
                    }
                    if (value_act_xcode_fmt == "JPG") {
                        image_type = ImageType_JPG;
                    } else if (value_act_xcode_fmt == "PNG") {
                        image_type = ImageType_PNG;
                    } else if (value_act_xcode_fmt == "TIFF") {
                        image_type = ImageType_TIFF;
                    } else if (value_act_xcode_fmt == "BMP") {
                        image_type = ImageType_BMP;
                    } else {
                        VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                                          "Unsupported image type - %s",
                                          value_act_xcode_fmt.c_str());
                        ::populate_response(transaction->resp, 415, "{\"errMsg\":\"Unsupported image type.\"}", "application/json");
                        goto_respond = true;
                        return;
                    }
                    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                                      "Transcode image to type %s",
                                      value_act_xcode_fmt.c_str());
                }
            }
#endif
        }
        else {
            if (response.has_thumbnail()) {
                mediaFile = response.thumbnail();
                VPLTRACE_LOG_INFO(TRACE_BVS, 0, 
                                  "Stream("FMT_VPLSocket_t"): photo_item's thumbnail \"%s\"",
                                  VAL_VPLSocket_t(http->sockfd),
                                  mediaFile.c_str());
            } else {
                VPLTRACE_LOG_INFO(TRACE_BVS, 0, 
                                  "Stream("FMT_VPLSocket_t"): Cannot find photo_item's thumbnail, responding ERROR 404 [url %s]",
                                  VAL_VPLSocket_t(http->sockfd),
                                  transaction->req.uri.c_str());
                ::populate_response(transaction->resp, 404, "{\"errMsg\":\"Content not found.\"}", "application/json");
                goto_respond = true;
                return;
            }
        }

        mediaType = MEDIA_TYPE_PHOTO;
        break;
    case media_metadata::MEDIA_VIDEO:
        if ( false == isThumbnail ) {
            if (response.has_absolute_path()) {
                mediaFile = response.absolute_path();
                VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                                  "Stream("FMT_VPLSocket_t"): video_item \"%s\"",
                                  VAL_VPLSocket_t(http->sockfd),
                                  mediaFile.c_str());
            } else {
                VPLTRACE_LOG_INFO(TRACE_BVS, 0, 
                                  "Stream("FMT_VPLSocket_t"): Cannot find video_item, responding ERROR 404 [url %s]",
                                  VAL_VPLSocket_t(http->sockfd),
                                  transaction->req.uri.c_str());
                ::populate_response(transaction->resp, 404, "{\"errMsg\":\"Content not found.\"}", "application/json");
                goto_respond = true;
                return;
            }
        }
        else {
            if (response.has_thumbnail()) {
                mediaFile = response.thumbnail();
                VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                                  "Stream("FMT_VPLSocket_t"): video_item's thumbnail \"%s\"",
                                  VAL_VPLSocket_t(http->sockfd),
                                  mediaFile.c_str());
            } else {
                VPLTRACE_LOG_INFO(TRACE_BVS, 0, 
                                  "Stream("FMT_VPLSocket_t"): Cannot find video_item's thumbnail, responding ERROR 404 [url %s]",
                                  VAL_VPLSocket_t(http->sockfd),
                                  transaction->req.uri.c_str());
                ::populate_response(transaction->resp, 404, "{\"errMsg\":\"Content not found.\"}", "application/json");
                goto_respond = true;
                return;
            }
        }
        mediaType = MEDIA_TYPE_VIDEO;
        break;
    case media_metadata::MEDIA_MUSIC_ALBUM:
        if (response.has_thumbnail()) {
            mediaFile = response.thumbnail();
            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                              "Stream("FMT_VPLSocket_t"): music_album's thumbnail \"%s\"",
                              VAL_VPLSocket_t(http->sockfd),
                              mediaFile.c_str());
        } else {
            VPLTRACE_LOG_INFO(TRACE_BVS, 0, 
                                "Stream("FMT_VPLSocket_t"): Cannot find music_album's thumbnail, responding ERROR 404 [url %s]",
                                VAL_VPLSocket_t(http->sockfd),
                                transaction->req.uri.c_str());
            ::populate_response(transaction->resp, 404, "{\"errMsg\":\"Content not found.\"}", "application/json");
            goto_respond = true;
            return;
        }
        mediaType = MEDIA_TYPE_PHOTO;
        break;
    case media_metadata::MEDIA_PHOTO_ALBUM:
        if (response.has_thumbnail()) {
            mediaFile = response.thumbnail();
            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                              "Stream("FMT_VPLSocket_t"): photo_album's thumbnail \"%s\"",
                              VAL_VPLSocket_t(http->sockfd),
                              mediaFile.c_str());
        } else {
            VPLTRACE_LOG_INFO(TRACE_BVS, 0, 
                                "Stream("FMT_VPLSocket_t"): Cannot find photo_album's thumbnail, responding ERROR 404 [url %s]",
                                VAL_VPLSocket_t(http->sockfd),
                                transaction->req.uri.c_str());
            ::populate_response(transaction->resp, 404, "{\"errMsg\":\"Content not found.\"}", "application/json");
            goto_respond = true;
            return;
        }
        mediaType = MEDIA_TYPE_PHOTO;
        break;
    case media_metadata::MEDIA_NONE:
    default:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Stream("FMT_VPLSocket_t"): Unhandled type",
                         VAL_VPLSocket_t(http->sockfd));
        doRead = false;
        break;
    }

#if defined(CLOUDNODE)
    // For the cloud node, files could be located in a dataset or directly
    // on the native file system (e.g. thumbnails).  The paths are prefixed
    // with either "/dataset/{userId}/{datasetId}" or "/native" to 
    // distinguish between them

    std:: string namespaceStr;
    u32 pos;

    pos = mediaFile.find('/');
    if (pos != 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "mediaFile name not in expected format %s", mediaFile.c_str());
        ::populate_response(transaction->resp, 500, "{\"errMsg\":\"mediaFile name not in expected format.\"}", "application/json");
        goto_respond = true;
        return;
    }

    pos = mediaFile.find('/', pos + 1);
    if (pos == string::npos) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "mediaFile name not in expected format %s", mediaFile.c_str());
        ::populate_response(transaction->resp, 500, "{\"errMsg\":\"mediaFile name not in expected format.\"}", "application/json");
        goto_respond = true;
        return;
    }
    namespaceStr = mediaFile.substr(1, pos - 1);
    mediaFile.erase(0, pos);

    if (namespaceStr.compare("dataset") == 0) {
        std::string userIdStr, datasetIdStr;
        u64 userId, datasetId;

        pos = mediaFile.find('/');
        if (pos != 0) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "mediaFile name not in expected format %s", mediaFile.c_str());
            ::populate_response(transaction->resp, 500, "{\"errMsg\":\"mediaFile name not in expected format.\"}", "application/json");
            goto_respond = true;
            return;
        }

        pos = mediaFile.find('/', pos + 1);
        if (pos == string::npos) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "mediaFile name not in expected format %s", mediaFile.c_str());
            ::populate_response(transaction->resp, 500, "{\"errMsg\":\"mediaFile name not in expected format.\"}", "application/json");
            goto_respond = true;
            return;
        }
        userIdStr = mediaFile.substr(1, pos - 1);
        userId = VPLConv_strToU64(userIdStr.c_str(), NULL, 10);

        mediaFile.erase(0, pos + 1);

        pos = mediaFile.find('/');
        if (pos == 0 || pos == string::npos) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "mediaFile name not in expected format %s", mediaFile.c_str());
            ::populate_response(transaction->resp, 500, "{\"errMsg\":\"mediaFile name not in expected format.\"}", "application/json");
            goto_respond = true;
            return;
        }
        datasetIdStr = mediaFile.substr(0, pos);
        mediaFile.erase(0, pos + 1);
        datasetId = VPLConv_strToU64(datasetIdStr.c_str(), NULL, 10);

        VPLTRACE_LOG_INFO(TRACE_BVS, 0, 
                          "Stream("FMT_VPLSocket_t"): \"/%s/%s/%s\"",
                          VAL_VPLSocket_t(http->sockfd),
                          userIdStr.c_str(),
                          datasetIdStr.c_str(),
                          mediaFile.c_str());

        http->server.getDataset(userId, datasetId, transaction->ds);
        if(transaction->ds == NULL) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Failed to retrieve dataset");
            ::populate_response(transaction->resp, 500, "{\"errMsg\":\"Failed to retrieve dataset.\"}", "application/json");
            goto_respond = true;
            return;
        }
    } else if (namespaceStr.compare("native") != 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "mediaFile name not in expected format %s", mediaFile.c_str());
        ::populate_response(transaction->resp, 500, "{\"errMsg\":\"mediaFile name not in expected format.\"}", "application/json");
        goto_respond = true;
        return;
    }
#endif  // CLOUDNODE

#ifdef ENABLE_PHOTO_TRANSCODE
    if (transaction->is_image_transcoding) {
        VPLFS_stat_t stat;
        AsyncTranscodingCallbackArgs_t* callback_args = NULL;
        GenerateTempFileArgs_t* generate_tempfile_callback_args = NULL;
        if (transaction->ds) {
            u32 pos;
            std::string source_file_extension;
            pos = mediaFile.find_last_of(".");

            if (pos != std::string::npos) {
                source_file_extension = mediaFile.substr(pos + 1);
                VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                                  "Stream("FMT_VPLSocket_t"): Get the file extension: \"%s\" - \"%s\"",
                                  VAL_VPLSocket_t(http->sockfd),
                                  mediaFile.c_str(),
                                  source_file_extension.c_str());

            } else {
                // No file extension, ffmpeg can not scale this photo. We should return the original photo.
                VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                                  "Stream("FMT_VPLSocket_t"): Can not get the file extension: \"%s\", return original photo.",
                                  VAL_VPLSocket_t(http->sockfd),
                                  mediaFile.c_str());
                transaction->is_image_transcoding = false;
                return;
            }

            callback_args = new AsyncTranscodingCallbackArgs_t();
            callback_args->m_strm_http = http;
            callback_args->m_transaction = transaction;
            callback_args->m_source_file_extension = source_file_extension;

            generate_tempfile_callback_args = new GenerateTempFileArgs_t();
            generate_tempfile_callback_args->m_strm_http = http;
            generate_tempfile_callback_args->m_transaction = transaction;
            generate_tempfile_callback_args->m_media_file = mediaFile;

            rc = ImageTranscode_AsyncTranscode_cloudnode(generate_temp_file_callback,
                                                         generate_tempfile_callback_args,
                                                         source_file_extension.c_str(),
                                                         source_file_extension.size(),
                                                         image_type,
                                                         width,
                                                         height,
                                                         process_async_transcoding_callback,
                                                         callback_args,
                                                         &transaction->image_transcoding_handle);
            if (rc == VPL_OK) {
                VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                                  "Stream("FMT_VPLSocket_t"): Got image transcoding handle: "FMTu32,
                                  VAL_VPLSocket_t(http->sockfd),
                                  transaction->image_transcoding_handle);

            } else {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "Stream("FMT_VPLSocket_t"): Failed to transcode image \"%s\"",
                                 VAL_VPLSocket_t(http->sockfd),
                                 mediaFile.c_str());
                ::populate_response(transaction->resp, 415, "{\"errMsg\":\"Failed to transcode image.\"}", "application/json");
                goto_respond = true;
                delete callback_args;
                delete generate_tempfile_callback_args;
            }

        } else {
            rc = VPLFS_Stat(mediaFile.c_str(), &stat);
            if (rc == VPL_OK) {
                u32 pos;
                std::string source_file_extension;
                pos = mediaFile.find_last_of(".");

                if (pos != std::string::npos) {
                    source_file_extension = mediaFile.substr(pos + 1);
                    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                                      "Stream("FMT_VPLSocket_t"): Get the file extension: \"%s\" - \"%s\"",
                                      VAL_VPLSocket_t(http->sockfd),
                                      mediaFile.c_str(),
                                      source_file_extension.c_str());
                }

                callback_args = new AsyncTranscodingCallbackArgs_t();
                callback_args->m_strm_http = http;
                callback_args->m_transaction = transaction;
                callback_args->m_source_file_extension = source_file_extension;

                rc = ImageTranscode_AsyncTranscode(mediaFile.c_str(),
                                                   mediaFile.size(),
                                                   NULL,
                                                   0,
                                                   image_type,
                                                   width,
                                                   height,
                                                   process_async_transcoding_callback,
                                                   callback_args,
                                                   &transaction->image_transcoding_handle);
                if (rc == VPL_OK) {
                    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                                      "Stream("FMT_VPLSocket_t"): Got image transcoding handle: "FMTu32,
                                      VAL_VPLSocket_t(http->sockfd),
                                      transaction->image_transcoding_handle);

                } else {
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                     "Stream("FMT_VPLSocket_t"): Failed to transcode image \"%s\"",
                                     VAL_VPLSocket_t(http->sockfd),
                                     mediaFile.c_str());
                    ::populate_response(transaction->resp, 415, "{\"errMsg\":\"Failed to transcode image.\"}", "application/json");
                    goto_respond = true;
                    delete callback_args;
                }

            } else {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "Stream("FMT_VPLSocket_t"): Can\'t find the photo \"%s\"",
                                 VAL_VPLSocket_t(http->sockfd),
                                 mediaFile.c_str());
                ::populate_response(transaction->resp, 404, "{\"errMsg\":\"Content not found.\"}", "application/json");
                goto_respond = true;
            }
        }
    }
#endif  // ENABLE_PHOTO_TRANSCODE
}

static void handle_ns_minidms(strm_http* http, strm_http_transaction* transaction,
                              const std::string& content,
                              bool& goto_respond, bool& is_deferred, bool& doRead,
                              std::string& mediaFile, int& mediaType)
{
    int rv = 0;
    std::vector<std::string> uri_tokens;
    VPLNet_port_t proxy_agent_port;
    VPLNet_addr_t local_addrs[VPLNET_MAX_INTERFACES];
    VPLNet_addr_t local_netmasks[VPLNET_MAX_INTERFACES];
    std::ostringstream oss;
    std::string json_response;

    VPLHttp_SplitUri(transaction->req.uri, uri_tokens);
    if (uri_tokens.size() < 2 || uri_tokens[1] != "deviceinfo") {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Stream("FMT_VPLSocket_t"): Invalid request (%s).",
                         VAL_VPLSocket_t(http->sockfd),
                         transaction->req.uri.c_str());
        ::populate_response(transaction->resp, 400, "{\"errMsg\":\"Invalid request.\"}", "application/json");
        goto out;
    }

    rv = VPLNet_GetLocalAddrList(local_addrs, local_netmasks);
    if (rv < 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Stream("FMT_VPLSocket_t"): VPLNet_GetLocalAddrList() failed: %d",
                         VAL_VPLSocket_t(http->sockfd), rv);
        ::populate_response(transaction->resp, 500, "{\"errMsg\":\"Failed to get local addresses.\"}", "application/json");
        goto out;
    }

    oss << "{\"network_info_list\":[";
    for (int i = 0; i < rv; i++) {
        char c_addr[20];
        char c_mask[20];
        snprintf(c_addr, 20, FMT_VPLNet_addr_t, VAL_VPLNet_addr_t(local_addrs[i]));
        snprintf(c_mask, 20, FMT_VPLNet_addr_t, VAL_VPLNet_addr_t(local_netmasks[i]));
        if (i > 0) {
            oss << ",";
        }
        oss << "{";
        oss << "\"ip\":\"" << c_addr << "\",";
        oss << "\"mask\":\"" << c_mask << "\"";
        oss << "}";
    }
    oss << "],";

    {
        ccd::GetSystemStateInput request;
        ccd::GetSystemStateOutput response;
        request.set_get_network_info(true);
        rv = CCDIGetSystemState(request, response);
        if (rv != 0) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Stream("FMT_VPLSocket_t"): CCDIGetSystemState()() failed: %d",
                             VAL_VPLSocket_t(http->sockfd), rv);
            ::populate_response(transaction->resp, 500, "{\"errMsg\":\"Internal Server Error.\"}", "application/json");
            goto out;
        }
        proxy_agent_port = response.network_info().proxy_agent_port();
    }

    oss << "\"port\":" << proxy_agent_port << "}";

    json_response = oss.str();
    ::populate_response(transaction->resp, 200, json_response.c_str(), "application/json");

out:
    goto_respond = true;
}

static void handle_ns_cmd(strm_http* http, strm_http_transaction* transaction,
                          const std::string& content,
                          bool& goto_respond, bool& is_deferred, bool& doRead,
                          std::string& mediaFile, int& mediaType)
{
    if (content.find(URL_CMD_KEEPAWAKE_TOKEN) != string::npos) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Stream("FMT_VPLSocket_t"): Receives keep awake request", VAL_VPLSocket_t(http->sockfd));
        VPLPowerMan_PostponeSleep(VPL_POWERMAN_ACTIVITY_SERVING_DATA, NULL);
    }
    goto_respond = false;
    doRead = false;
}

struct handle_rf_upload_context {
    strm_http_transaction *transaction;
    const std::string path;
    const std::string data;

    handle_rf_upload_context(strm_http_transaction *transaction,
                             std::string& path, const std::string& content)
        : transaction(transaction), path(path), data(content) {
    }
};

struct handle_rf_metadata_context {
    strm_http_transaction *transaction;
    const std::string path;
    cJSON2 *json;

    handle_rf_metadata_context(strm_http_transaction *transaction,
                               std::string& path, cJSON2 *json)
        : transaction(transaction), path(path), json(json) {
    }

    ~handle_rf_metadata_context() {
        if (json != NULL) {
            cJSON2_Delete(json);
        }
    }
};

struct handle_rf_context {
    strm_http_transaction *transaction;
    const std::string path;
    const std::string newpath;

    handle_rf_context(strm_http_transaction *transaction,
                      std::string& path, std::string& newpath)
        : transaction(transaction), path(path), newpath(newpath) {
    }
};

typedef void (*rf_callback_fn) (void *arg);

#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
static std::string convert_to_win32_virt_folder(const char *path) {
    std::string tmp;
    size_t idx;

    // adjust C:/ to Computer/C
    tmp.assign("Computer/").append(path);
    idx = tmp.find_first_of(":");
    if (idx != std::string::npos) {
        tmp.erase(idx, 1);
    }
    return tmp;
}
#endif //defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)

// Apply the prefix rewrite rules at RemoteFile layer
// for ex: [LOCALAPPDATA] -> "/" for cloudnode
static void apply_prefix_rewrite_rules(std::string &component)
{
    bool modified = false;
    std::string path;

    std::map<std::string, std::string> prefixRewriteRules;
#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
    {
        char *path = NULL;
        _VPLFS__GetLocalAppDataPath(&path);
        if (path) {
            prefixRewriteRules["[LOCALAPPDATA]"] = convert_to_win32_virt_folder(path);
            free(path);
            path = NULL;
        } else {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "unable to get user's localapp path");
        }
        _VPLFS__GetProfilePath(&path);
        if (path) {
            prefixRewriteRules["[USERPROFILE]"] = convert_to_win32_virt_folder(path);
            free(path);
            path = NULL;
        } else {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "unable to get user's profile path");
        }
    }
#elif defined(CLOUDNODE)
    prefixRewriteRules["[LOCALAPPDATA]"] = "/";
    prefixRewriteRules["[USERPROFILE]"] = "/";
#elif defined(LINUX) && defined(DEBUG)
    // FOR DEVELOPER USE ONLY
    prefixRewriteRules["[LOCALAPPDATA]"] = "/temp/localappdata";
    prefixRewriteRules["[USERPROFILE]"] = "/temp/userprofile";
#endif //defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)

    std::map<std::string, std::string>::const_iterator it;
    for (it = prefixRewriteRules.begin(); it != prefixRewriteRules.end(); it++) {
        // it->first is the prefix pattern
        // it->second is the replacement prefix

        // really obvious case: prefix pattern is empty
        // => simply prepend replacement prefix to component
        if (it->first.empty()) {
            path.assign(it->second);
            if (!component.empty()) {
                path.append("/");
                path.append(component);
            }
            modified = true;
            break;
        }

        // general case
        if (component.compare(0, it->first.length(), it->first) == 0) {
            path.assign(it->second);
            // append the rest only if it's starting with "/"
            if (component.length() > it->first.length() &&
                (component[it->first.length()] == '/')) {
                // avoid creating path like "//xxx"
                if (it->second != "/") {
                    path.append(component, it->first.length(), component.length() - it->first.length());
                } else {
                    path.append(component, it->first.length() + 1, component.length() - it->first.length() - 1);
                }
            }
            modified = true;
            break;
        }
    }
    if (modified) {
        component = path;
    }
}

#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
static void get_media_libraries(std::map<std::string, bool> &folder) {

    char *librariesPath = NULL;

    if (_VPLFS__GetLibrariesPath(&librariesPath) == VPL_OK) {
        VPLFS_dir_t dir;
        VPLFS_dirent_t dirent;
        if (VPLFS_Opendir(librariesPath, &dir) == VPL_OK) {
            while (VPLFS_Readdir(&dir, &dirent) == VPL_OK) {

                char *p = strstr(dirent.filename, ".library-ms");
                if (p != NULL && p[strlen(".library-ms")] == '\0') {
                    // found library description file
                    std::string libDescFilePath;
                    libDescFilePath.assign(librariesPath).append("/").append(dirent.filename);

                    // grab both localized and non-localized name of the library folders
                    _VPLFS__LibInfo libinfo;
                    _VPLFS__GetLibraryFolders(libDescFilePath.c_str(), &libinfo);

                    if (libinfo.folder_type == "Music" ||
                        libinfo.folder_type == "Video" ||
                        libinfo.folder_type == "Photo") {

                        std::map<std::string, _VPLFS__LibFolderInfo>::const_iterator it;
                        for (it = libinfo.m.begin(); it != libinfo.m.end(); it++) {
                            folder[convert_to_win32_virt_folder(it->second.path.c_str())] = false;
                        }

                        folder["Libraries/"+libinfo.n_name] = false;
                        folder["Libraries/"+libinfo.l_name] = false;
                    }
                }
            }
            VPLFS_Closedir(&dir);
        }
        if (librariesPath) {
            free(librariesPath);
            librariesPath = NULL;
        }
    }
}
#endif // defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)

static void refresh_restricted_folders(std::map<std::string, bool> &restricted_folders)
{
    // NOTE: The filepath of the restriction rules are all alias expanded

    restricted_folders.clear();
#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
    // allowed folder lists are
    // 1. UserProfile/PicStream
    // 2. [LOCALAPPDATA]/clear.fi (both virtual and expanded path)
    // 3. All the music/video/photo library folders (both virtual and expanded path)
    std::string picstream = "[USERPROFILE]/Picstream";
    std::string clear_fi = "[LOCALAPPDATA]/clear.fi";

    // replace the [USERPROFILE] w/ the real one
    apply_prefix_rewrite_rules(picstream);
    restricted_folders[picstream] = false;

    // replace the [LOCALAPPDATA] w/ the real one
    apply_prefix_rewrite_rules(clear_fi);
    restricted_folders[clear_fi] = false;

    get_media_libraries(restricted_folders);
#elif defined(CLOUDNODE)
    // allowed folder lists are
    // 1. [LOCALAPPDATA]/clear.fi (both virtual and expanded path)
    std::string clear_fi = "[LOCALAPPDATA]/clear.fi";

    // replace the [LOCALAPPDATA] w/ the real one
    apply_prefix_rewrite_rules(clear_fi);
    restricted_folders[clear_fi] = false;
    restricted_folders["clear.fi"] = false;
#endif //defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)

    // debugging purpose only
#if 0
    {
        std::map<std::string, bool>::iterator it;
        for (it = restricted_folders.begin(); it != restricted_folders.end(); it++) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "f[%s] = %d", it->first.c_str(), it->second);
        }
    }
#endif
}

static int path_checking_n_rewrite(std::string &path, std::map<std::string, bool> &restricted_folders)
{
    // don't check if no restricted folder provided
    bool match = restricted_folders.size() > 0? false : true;

    // NOTE: expands the prefix before we do the restriction folder comparison
    //       for they are all saved in expanded form
    // path prefix write for expanding the alias like [LOCALAPPDATA]
    apply_prefix_rewrite_rules(path);

    // debugging purpose only
#if 0
    // nothing alias beyond this point
    VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Checking path : %s", path.c_str());
#endif

    // [security] prefix checking
    std::map<std::string, bool>::iterator it;
    for (it = restricted_folders.begin(); it != restricted_folders.end(); it++) {
        if (path.length() < it->first.length()) {
            // doesn't match if path is shorter then the current rule
            continue;
        }
        if (it->second == true) {
            // request exact match
            if (path == it->first) {
                match = true;
                break;
            }
        } else {
            // request prefix match
            int rc = path.compare(0, it->first.length(), it->first);
            if (rc == 0 && path.length() > it->first.length()) {
                // if match the prefix of the rules and the "path" is longer then the rule
                // check whether it's ended w/ the '/'
                rc = path[it->first.length()] == '/'? 0 : -1;
            }
            if (rc == 0) {
                match = true;
                break;
            }
        }
    }

    if (match == false) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0, "path access denied: %s", path.c_str());
        return -1;
    }

    return 0;
}

static int prepare_rf_info(strm_http* http,
                           strm_http_transaction* transaction,
                           u64& userId,
                           u64& datasetId,
                           std::string& path,
                           std::string& response,
                           int &status_code,
                           bool &is_media_rf)
{
    bool rfEnabled, msEnabled;

    const std::string* userIdStr = transaction->req.find_header(RF_HEADER_USERID);
    if (userIdStr == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "unable to get user id from header, ERROR 400. http_method=%s, URI=%s",
                         transaction->req.http_method.c_str(),
                         transaction->req.uri.c_str());
        response = "{\"errMsg\":\"Unable to get user ID\"}";
        status_code = 400;
        return -1;
    }
    userId = VPLConv_strToS64(userIdStr->c_str(), NULL, 10);

    // parsing the URI namespaces for the parameters
    std::vector<std::string> uri_tokens;
    VPLHttp_SplitUri(transaction->req.uri, uri_tokens);

    // if we get here, we are sure about that it's starting with
    // /rf/<dir/file/filemetadata/...
    // bcz it how the dispatcher works.
    if (uri_tokens.size() < 2) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "unable to get dataset id from URI, ERROR 400. http_method=%s, URI=%s",
                         transaction->req.http_method.c_str(),
                         transaction->req.uri.c_str());
        response = "{\"errMsg\":\"Unable to get dataset ID\"}";
        status_code = 400;
        return -1;
    }
    datasetId = VPLConv_strToS64(uri_tokens[2].c_str(), NULL, 10);

    isRemoteFileOrMediaServerEnable(userId,http->get_device_id(), rfEnabled, msEnabled);
    VPLTRACE_LOG_FINE(TRACE_BVS, 0, "Feature enabled: RemoteFile = %d, MediaServer = %d", rfEnabled, msEnabled);

    // block the request if it's /rf and the remote file is disabled
    if ((uri_tokens[0] == "rf") && rfEnabled == false) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Remote file access is disabled. user["FMTu64"] / device ["FMTu64"]",
                         userId, http->get_device_id());
        response = "{\"errMsg\":\"Remote File Access Disabled\"}";
        status_code = 400;
        return -1;
    }
    // block the request if it's /media_rf and the media server is disabled
    if (uri_tokens[0] == "media_rf") {
        is_media_rf = true;
        if (msEnabled == false) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "MediaServer is disabled. user["FMTu64"] / device ["FMTu64"]",
                             userId, http->get_device_id());
            response = "{\"errMsg\":\"Media Server Disabled\"}";
            status_code = 400;
            return -1;
        }
    }

    http->server.getDataset(userId, datasetId, transaction->ds);
    if (transaction->ds == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "cannot access dataset");
        response = "{\"errMsg\":\"Cannot access dataset\"}";
        status_code = 404;
        return -1;
    }

    // extract from URI, and need to be decoded
    if (uri_tokens.size() < 3) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "file path is missing in the URI, ERROR 400. http_method=%s, URI=%s",
                         transaction->req.http_method.c_str(),
                         transaction->req.uri.c_str());
        response = "{\"errMsg\":\"File path is missing in the URI\"}";
        status_code = 400;
        return -1;
    }
    int err = VPLHttp_DecodeUri(uri_tokens[3], path);
    if (err) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "failed to decode path %s", uri_tokens[3].c_str());
        response = "{\"errMsg\":\"Failed to decode path " + uri_tokens[3] + "\"}";
        status_code = 400;
    }

    return err;
}

static void handle_rf_directory_op__deletedir(void *arg) {
    handle_rf_context *ctx = (handle_rf_context*)arg;
    strm_http_transaction *transaction = ctx->transaction;

    int rc = VSSI_SUCCESS;
    int status_code = 200;
    std::string response;
    std::string parent_dir = ::getParentDirectory(ctx->path);

    // check parent directory
    if (check_path_permission(transaction, parent_dir, true,
                              VPLFILE_CHECK_PERMISSION_READ,
                              VPLFS_TYPE_DIR, status_code, response) != 0) {
        goto out;
    }
    // check requested directory
    if (check_path_permission(transaction, ctx->path, true,
                              VPLFILE_CHECK_PERMISSION_DELETE,
                              VPLFS_TYPE_DIR, status_code, response) != 0) {
        goto out;
    }

    rc = transaction->ds->remove_iw(ctx->path);
    if (rc == VSSI_SUCCESS) {
        rc = transaction->ds->commit_iw();
    }
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "delete_dir failed: %s, rv = %d", ctx->path.c_str(), rc);
        response = "{\"errMsg\":\"Failed to delete directory\"}";
        status_code = 500;
    }

out:
    ::populate_response(transaction->resp, status_code, response, "application/json");
    transaction->processing = false;
    transaction->sending = true;

    transaction->http_handle->put_response(transaction);

    delete ctx;
}

static VPLTHREAD_FN_DECL readdir_thread_fn(void* param)
{
    handle_rf_context *ctx = (handle_rf_context*)param;
    strm_http_transaction *transaction = ctx->transaction;

    int status_code = 200;
    std::string response;
    const std::string *sortBy;
    const std::string *tmpStr;
    u32 max = UINT_MAX;
    u32 index = 1;

    sortBy = transaction->req.find_query("sortBy");
    if ((tmpStr = transaction->req.find_query("max")) != NULL) {
        max = strtol(tmpStr->c_str(), NULL, 10);
    }
    if ((tmpStr = transaction->req.find_query("index")) != NULL) {
        index = strtol(tmpStr->c_str(), NULL, 10);
    }

    transaction->ds->read_dir2(ctx->path, response, true, true,
                               sortBy == NULL? "time" : *sortBy, index, max);

    ::populate_response(transaction->resp, status_code, response, "application/json");
    transaction->processing = false;
    transaction->sending = true;

    transaction->http_handle->put_response(transaction);

    delete ctx;

    return VPLTHREAD_RETURN_VALUE;
}

static void handle_rf_directory_op__readdir(void *arg) {
    handle_rf_context *ctx = (handle_rf_context*)arg;
    strm_http_transaction *transaction = ctx->transaction;

    int status_code = 200;
    std::string response;

    if (check_path_permission(transaction, ctx->path, true,
                              VPLFILE_CHECK_PERMISSION_READ,
                              VPLFS_TYPE_DIR, status_code, response) != 0) {
        goto out;
    }

    {
        VPLThread_attr_t thread_attributes;
        VPLDetachableThreadHandle_t thread_handle;

        // create thread
        int rc = VPLThread_AttrInit(&thread_attributes);
        if (rc < 0) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLThread_AttrInit returned %d", rc);
            goto thread_out;
        }

        rc = VPLThread_AttrSetDetachState(&thread_attributes, VPL_TRUE);
        if (rc < 0) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLThread_AttrSetDetachState returned %d", rc);
            goto thread_out;
        }

        rc = VPLDetachableThread_Create(&thread_handle, readdir_thread_fn, (void *)ctx, &thread_attributes, NULL);
        if (rc < 0) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLDetachableThread_Create returned %d", rc);
        }

thread_out:
        VPLThread_AttrDestroy(&thread_attributes);

        // response w/ error
        if (rc < 0) {
            response = "{\"errMsg\":\"Failed to spawn thread for directory reading\"}";
            status_code = 500;
            goto out;
        }
    }
    return;

out:
    ::populate_response(transaction->resp, status_code, response, "application/json");
    transaction->processing = false;
    transaction->sending = true;

    transaction->http_handle->put_response(transaction);

    delete ctx;
}

static void handle_rf_directory_op__makedir(void *arg) {
    handle_rf_context *ctx = (handle_rf_context*)arg;
    strm_http_transaction *transaction = ctx->transaction;

    int status_code = 200;
    int rv = VSSI_SUCCESS;
    std::string response;
    std::string parent_dir = ::getParentDirectory(ctx->path);

    // check parent directory
    if (check_path_permission(transaction, parent_dir, true,
                              VPLFILE_CHECK_PERMISSION_WRITE,
                              VPLFS_TYPE_DIR, status_code, response) != 0) {
        goto out;
    }
    // check requested directory
    if (check_path_permission(transaction, ctx->path, false,
                              VPLFILE_CHECK_PERMISSION_WRITE,
                              VPLFS_TYPE_DIR, status_code, response) != 0) {
        goto out;
    }

    rv = transaction->ds->make_directory_iw(ctx->path, 0);
    if (rv == VSSI_SUCCESS) {
        rv = transaction->ds->commit_iw();
    }
    if (rv == VSSI_NOTFOUND) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "make_dir failed. parent directory doesn't exist: %s",
                         ctx->path.c_str());
        status_code = 404;
        response = "{\"errMsg\":\"Make directory failed due to parent directory doesn't exist\"}";
    } else if (rv != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "fail to make directory: %s, rv = %d",
                         ctx->path.c_str(), rv);
        status_code = 500;
        response = "{\"errMsg\":\"Make directory failed\"}";
    }

out:
    ::populate_response(transaction->resp, status_code, response, "application/json");
    transaction->processing = false;
    transaction->sending = true;

    transaction->http_handle->put_response(transaction);

    delete ctx;
}

static void handle_rf_directory_op__copydir(void *arg) {
    handle_rf_context *ctx = (handle_rf_context*)arg;
    strm_http_transaction *transaction = ctx->transaction;

    // For now, we won't support copy directory.
    // So we'll return 501.
    // cf. bug 2988

    // TODO put security checking here

    int status_code = 501;
    std::string response = "{\"errMsg\":\"Copy directory function is not yet implemented\"}";

    VPLTRACE_LOG_ERR(TRACE_BVS, 0, "copy_dir is not yet implemented!");

    ::populate_response(transaction->resp, status_code, response, "application/json");
    transaction->processing = false;
    transaction->sending = true;

    transaction->http_handle->put_response(transaction);

    delete ctx;
}

static void handle_rf_directory_op__movedir(void *arg) {
    handle_rf_context *ctx = (handle_rf_context*)arg;
    strm_http_transaction *transaction = ctx->transaction;

    int status_code = 200;
    std::string response;
    int rv = VSSI_SUCCESS;
    std::string parent_dir = ::getParentDirectory(ctx->path);

    // check parent directory
    if (check_path_permission(transaction, parent_dir, true,
                              VPLFILE_CHECK_PERMISSION_READ,
                              VPLFS_TYPE_DIR, status_code, response) != 0) {
        goto out;
    }
    // check requested directory
    if (check_path_permission(transaction, ctx->path, true,
                              VPLFILE_CHECK_PERMISSION_WRITE|VPLFILE_CHECK_PERMISSION_DELETE,
                              VPLFS_TYPE_DIR, status_code, response) != 0) {
        goto out;
    }

    parent_dir = ::getParentDirectory(ctx->newpath);
    // check parent directory
    if (check_path_permission(transaction, parent_dir, true,
                              VPLFILE_CHECK_PERMISSION_WRITE,
                              VPLFS_TYPE_DIR, status_code, response) != 0) {
        goto out;
    }
    // check requested directory
    if (check_path_permission(transaction, ctx->newpath, false,
                              VPLFILE_CHECK_PERMISSION_WRITE,
                              VPLFS_TYPE_DIR, status_code, response) != 0) {
        goto out;
    }

    rv = transaction->ds->rename_iw(ctx->path, ctx->newpath);
    if (rv == VSSI_SUCCESS) {
        rv = transaction->ds->commit_iw();
    }
    if (rv != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "move dir failed: %s. rv = %d",
                         ctx->path.c_str(), rv);
        status_code = 500;
        response = "{\"errMsg\":\"Move directory failed\"}";
    }

out:
    ::populate_response(transaction->resp, status_code, response, "application/json");
    transaction->processing = false;
    transaction->sending = true;

    transaction->http_handle->put_response(transaction);

    delete ctx;
}

static void handle_rf_directory_op(strm_http* http, strm_http_transaction* transaction,
                                   const std::string& content,
                                   bool& goto_respond, bool& is_deferred, bool& doRead,
                                   std::string& mediaFile, int& mediaType)
{
    std::string response;
    const std::string* copy_path_p = NULL;
    std::string copy_path;
    const std::string* move_path_p = NULL;
    std::string move_path;

    u64 userId;
    u64 datasetId;
    std::string path;

    int rv = 0;
    int status_code = 200;
    bool is_media_rf = false;

    rf_callback_fn callback = NULL;
    handle_rf_context *ctx = NULL;
    std::string empty = "";

    if (transaction->req.http_method != "GET" &&
        transaction->req.http_method != "PUT" &&
        transaction->req.http_method != "POST" &&
        transaction->req.http_method != "DELETE") {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "invalid request. /rf/dir/ is not handling request"
                         " with http_method = %s, ERROR 400",
                         transaction->req.http_method.c_str());
        status_code = 400;
        response = "{\"errMsg\":\"Invalid request. Unhandled HTTP method\"}";
        goto error;
    }

    rv = prepare_rf_info(http, transaction, userId, datasetId, path, response, status_code, is_media_rf);
    if (rv) {
        goto error;
    }

    // check the query parameter to see whether there's copy_from or move_from.
    // if they both exist, response w/ bad request error code 400
    copy_path_p = transaction->req.find_query("copyFrom");
    if (copy_path_p) copy_path.assign(*copy_path_p);
    move_path_p = transaction->req.find_query("moveFrom");
    if (move_path_p) move_path.assign(*move_path_p);
    if (copy_path_p != NULL && move_path_p != NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "invalid request where both copyFrom and moveFrom are"
                         " shown at the query parameters, ERROR 400. http_method=%s, URI=%s",
                         transaction->req.http_method.c_str(),
                         transaction->req.uri.c_str());
        status_code = 400;
        response = "{\"errMsg\":\"Invalid request. Both copyFrom and moveFrom are provided in URI\"}";
        goto error;
    }

    {
        int rc = 0;
        std::map<std::string, bool> restricted_folders;

        if (is_media_rf) {
            refresh_restricted_folders(restricted_folders);
#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
            // allow reading the "Libraries" virtual folder
            if (transaction->req.http_method == "GET") {
                restricted_folders["Libraries"] = true;
            }
#endif //defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
        }
        // path prefix write for expanding the alias like [LOCALAPPDATA]
        rc = path_checking_n_rewrite(path, restricted_folders);
        if (rc == 0 && copy_path_p != NULL) {
            rc = path_checking_n_rewrite(copy_path, restricted_folders);
        }
        if (rc == 0 && move_path_p != NULL) {
            rc = path_checking_n_rewrite(move_path, restricted_folders);
        }
        if (rc) {
            status_code = 400;
            response = "{\"errMsg\":\"Folder Access Denied\"}";
            goto error;
        }
    }

    if (transaction->req.http_method == "GET") {
        ctx = new handle_rf_context(transaction, path, empty);
        callback = handle_rf_directory_op__readdir;
    } else if (transaction->req.http_method == "PUT") {
        ctx = new handle_rf_context(transaction, path, empty);
        callback = handle_rf_directory_op__makedir;
    } else if (transaction->req.http_method == "DELETE") {
        ctx = new handle_rf_context(transaction, path, empty);
        callback = handle_rf_directory_op__deletedir;
    } else if (transaction->req.http_method == "POST") {
        if (copy_path_p) {
            callback = handle_rf_directory_op__copydir;
            ctx = new handle_rf_context(transaction, path, copy_path);
        } else if (move_path_p) {
            ctx = new handle_rf_context(transaction, path, move_path);
            callback = handle_rf_directory_op__movedir;
        }
    }

    if (callback == NULL || ctx == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "unhandled case, ERROR 500. http_method=%s, URI=%s",
                         transaction->req.http_method.c_str(),
                         transaction->req.uri.c_str());
        status_code = 500;
        response = "{\"errMsg\":\"Unhandled request\"}";
        goto error;
    }

    callback(ctx);

    goto_respond = false;
    doRead = false;
    is_deferred = true;
    return;

error:
    ::populate_response(transaction->resp, status_code, response, "application/json");
    transaction->processing = false;
    transaction->sending = true;
    goto_respond = true;
    return;
}

static void handle_rf_file_op__delete(void *arg) {
    handle_rf_context *ctx = (handle_rf_context*)arg;
    strm_http_transaction *transaction = ctx->transaction;

    int rc = VSSI_SUCCESS;
    int status_code = 200;
    std::string response;
    std::string parent_dir = ::getParentDirectory(ctx->path);

    // check parent directory
    if (check_path_permission(transaction, parent_dir, true,
                              VPLFILE_CHECK_PERMISSION_READ,
                              VPLFS_TYPE_DIR, status_code, response) != 0) {
        goto out;
    }
    // check requested file
    if (check_path_permission(transaction, ctx->path, true,
                              VPLFILE_CHECK_PERMISSION_DELETE,
                              VPLFS_TYPE_FILE, status_code, response) != 0) {
        goto out;
    }

    rc = transaction->ds->remove_iw(ctx->path);
    if (rc == VSSI_SUCCESS) {
        rc = transaction->ds->commit_iw();
    }
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "delete_file failed: %s", ctx->path.c_str());
        response = "{\"errMsg\":\"Failed to delete file\"}";
        status_code = 500;
    }

out:
    ::populate_response(transaction->resp, status_code, response, "application/json");
    transaction->processing = false;
    transaction->sending = true;

    transaction->http_handle->put_response(transaction);

    delete ctx;
}

static void handle_rf_file_op__download(void *arg) {
    handle_rf_context *ctx = (handle_rf_context*)arg;
    strm_http_transaction *transaction = ctx->transaction;
    std::string response;
    int status_code = 200;

    // check requested directory
    if (check_path_permission(transaction, ctx->path, true,
                              VPLFILE_CHECK_PERMISSION_READ,
                              VPLFS_TYPE_FILE, status_code, response) != 0) {
        ::populate_response(transaction->resp, status_code, response, "application/json");
        goto out;
    }

    get_mediafile_resp(ctx->path, MEDIA_TYPE_UNKNOWN, transaction);

out:
    transaction->http_handle->put_response(transaction);

    delete ctx;
}

static void handle_rf_file_op__upload(void *arg) {
    handle_rf_upload_context *ctx = (handle_rf_upload_context*)arg;
    strm_http_transaction *transaction = ctx->transaction;

    int rc = VSSI_SUCCESS;
    u32 length = 0;
    int status_code = 200;
    std::string response;
    std::string parent_dir = ::getParentDirectory(ctx->path);

    // check parent directory
    if (check_path_permission(transaction, parent_dir, true,
                              VPLFILE_CHECK_PERMISSION_WRITE,
                              VPLFS_TYPE_DIR, status_code, response) != 0) {
        goto out;
    }
    // check requested file
    if (check_path_permission(transaction, ctx->path, false,
                              VPLFILE_CHECK_PERMISSION_WRITE,
                              VPLFS_TYPE_FILE, status_code, response) != 0) {
        goto out;
    }

    rc = open_vss_write_handle(transaction, ctx->path);
    if(rc != 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "open_vss_write_handle:%d, %s",
                         rc,
                         ctx->path.c_str());
        response = "{\"errMsg\":\"Failed to open VSS file handle\"}";
        status_code = 500;
        goto out;
    }

    length = ctx->data.size();
    rc = transaction->ds->write_file(transaction->vss_file_handle,
                                     NULL,
                                     0,
                                     transaction->cur_offset,
                                     length,
                                     ctx->data.c_str());
    if(rc != 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "write data failed, return bad response.");
        response = "{\"errMsg\":\"Failed to write file\"}";
        status_code = 500;
        goto fail_write;
    } else {
        transaction->cur_offset += length;
        VPLTRACE_LOG_INFO(TRACE_BVS, 0, "write_data %d bytes.", length);
    }

    if(transaction->req.content_length() != transaction->cur_offset) {
        status_code = 500;
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "content_length("FMTu64") != cur_offset("FMTu64")",
                         transaction->req.content_length(), transaction->cur_offset);
        response = "{\"errMsg\":\"Failed to write file due to content length doesn't match offset\"}";
        goto fail_write;
    }

    rc = transaction->ds->truncate_file(transaction->vss_file_handle,
                                        NULL,
                                        0,
                                        transaction->cur_offset);
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "write data set_size fail");
        status_code = 500;
        response = "{\"errMsg\":\"Failed to write file due to setting file size failed\"}";
        goto fail_write;
    }

fail_write:
    close_vss_handle(transaction);

    if (rc) {
        rc = transaction->ds->remove_iw(ctx->path);
        if (rc == VSSI_SUCCESS) {
            rc = transaction->ds->commit_iw();
        } else if (rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "failed to clean-up upload file: %s, %d",
                             ctx->path.c_str(), rc);
        }
    }

out:
    ::populate_response(transaction->resp, status_code, response, "application/json");
    transaction->processing = false;
    transaction->sending = true;

    transaction->http_handle->put_response(transaction);

    delete ctx;
}

static void handle_rf_file_op__copy(void *arg) {
    handle_rf_context *ctx = (handle_rf_context*)arg;
    strm_http_transaction *transaction = ctx->transaction;

    int rc = VSSI_SUCCESS;
    int status_code = 200;
    std::string response;
    std::string parent_dir = ::getParentDirectory(ctx->path);

    // check requested file
    if (check_path_permission(transaction, ctx->path, true,
                              VPLFILE_CHECK_PERMISSION_READ,
                              VPLFS_TYPE_FILE, status_code, response) != 0) {
        goto out;
    }

    parent_dir = ::getParentDirectory(ctx->newpath);
    // check parent directory
    if (check_path_permission(transaction, parent_dir, true,
                              VPLFILE_CHECK_PERMISSION_WRITE,
                              VPLFS_TYPE_DIR, status_code, response) != 0) {
        goto out;
    }
    // check requested file
    if (check_path_permission(transaction, ctx->newpath, false,
                              VPLFILE_CHECK_PERMISSION_WRITE,
                              VPLFS_TYPE_FILE, status_code, response) != 0) {
        goto out;
    }

    rc = transaction->ds->copy_file(ctx->path, ctx->newpath);
    if (rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "failed to copy file. rc = %d", rc);
        response = "{\"errMsg\":\"Failed to copy file\"}";
        status_code = 500;
    }

out:
    ::populate_response(transaction->resp, status_code, response, "application/json");
    transaction->processing = false;
    transaction->sending = true;

    transaction->http_handle->put_response(transaction);

    delete ctx;
}

static void handle_rf_file_op__move(void *arg) {
    handle_rf_context *ctx = (handle_rf_context*)arg;
    strm_http_transaction *transaction = ctx->transaction;

    int rc = VSSI_SUCCESS;
    int status_code = 200;
    std::string response;
    std::string parent_dir = ::getParentDirectory(ctx->path);

    // check parent directory
    if (check_path_permission(transaction, parent_dir, true,
                              VPLFILE_CHECK_PERMISSION_READ,
                              VPLFS_TYPE_DIR, status_code, response) != 0) {
        goto out;
    }
    // check requested file
    if (check_path_permission(transaction, ctx->path, true,
                              VPLFILE_CHECK_PERMISSION_WRITE|VPLFILE_CHECK_PERMISSION_DELETE,
                              VPLFS_TYPE_FILE, status_code, response) != 0) {
        goto out;
    }

    parent_dir = ::getParentDirectory(ctx->newpath);
    // check parent directory
    if (check_path_permission(transaction, parent_dir, true,
                              VPLFILE_CHECK_PERMISSION_WRITE,
                              VPLFS_TYPE_DIR, status_code, response) != 0) {
        goto out;
    }
    // check requested file
    if (check_path_permission(transaction, ctx->newpath, false,
                              VPLFILE_CHECK_PERMISSION_WRITE,
                              VPLFS_TYPE_FILE, status_code, response) != 0) {
        goto out;
    }

    rc = transaction->ds->rename_iw(ctx->path, ctx->newpath);
    if (rc == VSSI_SUCCESS) {
        rc = transaction->ds->commit_iw();
    }
    if (rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "move_file failed: %s -> %s. rc = %d"
                         , ctx->path.c_str(), ctx->newpath.c_str(), rc);
        status_code = 500;
        response = "{\"errMsg\":\"Failed to move file\"}";
    }

out:
    ::populate_response(transaction->resp, status_code, response, "application/json");
    transaction->processing = false;
    transaction->sending = true;

    transaction->http_handle->put_response(transaction);

    delete ctx;
}

static void handle_rf_file_op(strm_http* http, strm_http_transaction* transaction,
                              const std::string& content,
                              bool& goto_respond, bool& is_deferred, bool& doRead,
                              std::string& mediaFile, int& mediaType)
{
    std::string response;
    const std::string* copy_path_p = NULL;
    std::string copy_path;
    const std::string* move_path_p = NULL;
    std::string move_path;

    u64 userId;
    u64 datasetId;
    std::string path;

    int rv = 0;
    int status_code = 200;
    bool is_media_rf = false;

    rf_callback_fn callback = NULL;
    void *ctx = NULL;
    std::string empty = "";

    if (transaction->req.http_method != "GET" &&
        transaction->req.http_method != "POST" &&
        transaction->req.http_method != "DELETE") {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "invalid request. /rf/file/ is not handling request"
                         " with http_method = %s, ERROR 400",
                         transaction->req.http_method.c_str());
        status_code = 400;
        response = "{\"errMsg\":\"Invalid request. Unhandled HTTP method\"}";
        goto error;
    }

    rv = prepare_rf_info(http, transaction, userId, datasetId, path, response, status_code, is_media_rf);
    if (rv) {
        goto error;
    }

    // check the query parameter to see whether there's copy_from or move_from.
    // if they both exist, response w/ bad request error code 400
    copy_path_p = transaction->req.find_query("copyFrom");
    if (copy_path_p) copy_path.assign(*copy_path_p);
    move_path_p = transaction->req.find_query("moveFrom");
    if (move_path_p) move_path.assign(*move_path_p);
    if (copy_path_p != NULL && move_path_p != NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "invalid request where both copyFrom and moveFrom are"
                         " shown at the query parameters, ERROR 400. http_method=%s, URI=%s",
                         transaction->req.http_method.c_str(),
                         transaction->req.uri.c_str());
        status_code = 400;
        response = "{\"errMsg\":\"Invalid request. Both copyFrom and moveFrom are provided in URI\"}";
        goto error;
    }

    {
        int rc = 0;
        std::map<std::string, bool> restricted_folders;

        if (is_media_rf) {
            refresh_restricted_folders(restricted_folders);
        }

        // path prefix write for expanding the alias like [LOCALAPPDATA]
        rc = path_checking_n_rewrite(path, restricted_folders);
        if (rc == 0 && copy_path_p != NULL) {
            rc = path_checking_n_rewrite(copy_path, restricted_folders);
        }
        if (rc == 0 && move_path_p != NULL) {
            rc = path_checking_n_rewrite(move_path, restricted_folders);
        }
        if (rc) {
            status_code = 400;
            response = "{\"errMsg\":\"Folder Access Denied\"}";
            goto error;
        }
    }

    if (transaction->req.http_method == "GET") {
        ctx = new handle_rf_context(transaction, path, empty);
        // it's used by the get_mediafile_resp
        callback = handle_rf_file_op__download;
    } else if (transaction->req.http_method == "DELETE") {
        ctx = new handle_rf_context(transaction, path, empty);
        callback = handle_rf_file_op__delete;
    } else if (transaction->req.http_method == "POST") {
        if (copy_path_p) {
            ctx = new handle_rf_context(transaction, path, copy_path);
            callback = handle_rf_file_op__copy;
        } else if (move_path_p) {
            ctx = new handle_rf_context(transaction, path, move_path);
            callback = handle_rf_file_op__move;
        } else {
            // Check whether the content is fetched already. If so, then write file directly
            if (!transaction->req.receive_complete()) {
                transaction->write_file = true;
                goto out;
            } else {
                VPLTRACE_LOG_INFO(TRACE_BVS, 0, "file is already received. write this by ourselves");
                ctx = new handle_rf_upload_context(transaction, path, content);
                callback = handle_rf_file_op__upload;
            }
        }
    }

    if (callback == NULL || ctx == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "unhandled case, ERROR 500. http_method=%s, URI=%s",
                         transaction->req.http_method.c_str(),
                         transaction->req.uri.c_str());
        status_code = 500;
        response = "{\"errMsg\":\"Unhandled request\"}";
        goto error;
    }

    callback(ctx);

out:
    goto_respond = false;
    doRead = false;
    is_deferred = true;
    return;

error:
    ::populate_response(transaction->resp, status_code, response, "application/json");
    transaction->processing = false;
    transaction->sending = true;
    goto_respond = true;
    return;
}

static void handle_rf_metadata__read(void *arg)
{
    handle_rf_metadata_context *ctx = (handle_rf_metadata_context*)arg;
    strm_http_transaction *transaction = ctx->transaction;

    VPLFS_stat_t stat;
    std::ostringstream oss;
    std::string response;
    std::string filename;
    int status_code = 200;
#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
    std::string target_path;
    std::string target_type;
    std::string target_args;
#endif
    // check requested file
    if (check_path_permission(transaction, ctx->path, true,
                              VPLFILE_CHECK_PERMISSION_READ,
                              VPLFS_TYPE_FILE, status_code, response,
                              true) != 0) {
        goto out;
    }
    // stat the dir to see whether it's existed or not
    if (transaction->ds->stat_component(ctx->path, stat) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "%s missing; responding ERROR 404", ctx->path.c_str());
        response = "{\"errMsg\":\"File not found\"}";
        status_code = 404;
        goto out;
    }

    filename = getFilenameFromPath(ctx->path);

#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
    // Bug 10824: get shortcut detail if it is a file with .lnk extension
    if (stat.type == VPLFS_TYPE_FILE 
        && ctx->path.find_last_of(".") != string::npos
        && ctx->path.substr(ctx->path.find_last_of(".")+1) == "lnk") {

        transaction->ds->stat_shortcut(ctx->path, target_path, target_type, target_args);
        //Error msg is recorded in stat_shortcut function.
        //If any error occurs within stat_shortcut, target_path, target_type or target_args could be empty.
        //They will be handled while generating the JSON response below.
    }
#endif

    oss << "{\"name\":\"" << filename << '"'
        << ",\"size\":" << stat.size
        << ",\"lastChanged\":" << stat.mtime
        << ",\"isReadOnly\":" << (stat.isReadOnly ? "true" : "false")
        << ",\"isHidden\":" << (stat.isHidden ? "true" : "false")
        << ",\"isSystem\":" << (stat.isSystem ? "true" : "false")
        << ",\"isArchive\":" << (stat.isArchive ? "true" : "false");
#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
    if (!target_path.empty()) {
        oss << ",\"target_path\":\"" << target_path << "\"";
    }
    if (!target_type.empty()) {
        oss << ",\"target_type\":\"" << target_type << "\"";
    }
    if (!target_args.empty()) {
        oss << ",\"target_args\":\"" << target_args << "\"";
    }
#endif
    oss << "}";
    response = oss.str();

out:
    ::populate_response(transaction->resp, status_code, response, "application/json");
    transaction->processing = false;
    transaction->sending = true;
    transaction->http_handle->put_response(transaction);

    delete ctx;
}

static void handle_rf_metadata__setpermission(void *arg)
{
    handle_rf_metadata_context *ctx = (handle_rf_metadata_context*)arg;
    strm_http_transaction *transaction = ctx->transaction;

    std::string response;
    int status_code = 200;
    int rc = VPL_OK;

    u32 attrs = 0;
    u32 mask = 0;
    std::string parent_dir = ::getParentDirectory(ctx->path);

    // check parent directory
    if (check_path_permission(transaction, parent_dir, true,
                              VPLFILE_CHECK_PERMISSION_READ,
                              VPLFS_TYPE_DIR, status_code, response) != 0) {
        goto out;
    }
    // check requested file
    if (check_path_permission(transaction, ctx->path, true,
                              VPLFILE_CHECK_PERMISSION_WRITE,
                              VPLFS_TYPE_FILE, status_code, response, true) != 0) {
        goto out;
    }

    for (int i = 0 ; i < cJSON2_GetArraySize(ctx->json) ; i++) {
        cJSON2 * subitem = cJSON2_GetArrayItem(ctx->json, i);
        // only take cares of first layer boolean objects
        if (subitem->type == cJSON2_True || subitem->type == cJSON2_False) {
            if (strcmp(subitem->string, "isReadOnly") == 0) {
                attrs |= (subitem->type == cJSON2_True)? VSSI_ATTR_READONLY : 0;
                mask |= VSSI_ATTR_READONLY;
            } else if (strcmp(subitem->string, "isHidden") == 0) {
                attrs |= (subitem->type == cJSON2_True)? VSSI_ATTR_HIDDEN : 0;
                mask |= VSSI_ATTR_HIDDEN;
            } else if (strcmp(subitem->string, "isSystem") == 0) {
                attrs |= (subitem->type == cJSON2_True)? VSSI_ATTR_SYS : 0;
                mask |= VSSI_ATTR_SYS;
            } else if (strcmp(subitem->string, "isArchive") == 0) {
                attrs |= (subitem->type == cJSON2_True)? VSSI_ATTR_ARCHIVE : 0;
                mask |= VSSI_ATTR_ARCHIVE;
            }
        }
    }

    if (mask == 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "setpermission w/o valid attribute given; responding ERROR 400");
        response = "{\"errMsg\":\"attributes missing\"}";
        status_code = 400;
        goto out;
    }

    do {
        VPLFS_stat_t stat;
        rc = transaction->ds->stat_component(ctx->path, stat);
        if (rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "stat_component failed; path %s, err %d", ctx->path.c_str(), rc);
            break;
        }

        if (stat.type == VPLFS_TYPE_FILE) {
            vss_file *file = NULL;
            rc = transaction->ds->open_file(ctx->path, /*version*/0, /*flags*/0, /*attrs*/0, file);
            if (rc != VSSI_SUCCESS) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "open_file failed; path %s, err %d", ctx->path.c_str(), rc);
                break;
            }
            rc = transaction->ds->chmod_file(file, /*vss_object*/NULL, /*origin*/0, attrs, mask);
            if (rc != VSSI_SUCCESS) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "chmod_file failed; path %s, err %d", ctx->path.c_str(), rc);
                // fall through, to close file handle
            }
            rc = transaction->ds->close_file(file, /*vss_object*/NULL, /*origin*/0);
            if (rc != VSSI_SUCCESS) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "close_file failed; path %s, err %d", ctx->path.c_str(), rc);
                break;
            }
        }
        else if (stat.type == VPLFS_TYPE_DIR) {
            rc = transaction->ds->chmod_iw(ctx->path, attrs, mask);
            if (rc != VSSI_SUCCESS) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "chmod_iw failed; path %s, err %d", ctx->path.c_str(), rc);
                break;
            }
            rc = transaction->ds->commit_iw();
            if (rc != VSSI_SUCCESS) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "commit_iw failed; err %d", rc);
                break;
            }
        }
        else {  // unexpected/unknown component type
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Unexpected component type; path %s, type %d", ctx->path.c_str(), stat.type);
            rc = VSSI_BADOBJ;
            break;
        }
    } while (0);

    if (rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "fail to set permissions, rv = %d", rc);
        response = "{\"errMsg\":\"fail to set permissions\"}";
        status_code = 500;
    }

out:
    ::populate_response(transaction->resp, status_code, response, "application/json");
    transaction->processing = false;
    transaction->sending = true;
    transaction->http_handle->put_response(transaction);

    delete ctx;
}

static void handle_rf_metadata_op(strm_http* http, strm_http_transaction* transaction,
                                  const std::string& content,
                                  bool& goto_respond, bool& is_deferred, bool& doRead,
                                  std::string& mediaFile, int& mediaType)
{
    std::string response;

    u64 userId;
    u64 datasetId;
    std::string path;

    int rv = 0;
    int status_code = 200;
    bool is_media_rf = false;

    rf_callback_fn callback = NULL;
    handle_rf_metadata_context *ctx = NULL;

    if (transaction->req.http_method != "GET" &&
        transaction->req.http_method != "PUT") {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "invalid request. /rf/filemetadata/ is not handling request"
                         " with http_method = %s, ERROR 400",
                         transaction->req.http_method.c_str());
        status_code = 400;
        response = "{\"errMsg\":\"Invalid request. Unhandled HTTP method\"}";
        goto error;
    }

    rv = prepare_rf_info(http, transaction, userId, datasetId, path, response, status_code, is_media_rf);
    if (rv) {
        goto error;
    }

    {
        int rc = 0;
        std::map<std::string, bool> restricted_folders;

        if (is_media_rf) {
            refresh_restricted_folders(restricted_folders);
        }
        // path prefix write for expanding the alias like [LOCALAPPDATA]
        rc = path_checking_n_rewrite(path, restricted_folders);
        if (rc) {
            status_code = 400;
            response = "{\"errMsg\":\"Folder Access Denied\"}";
            goto error;
        }
    }

    if (transaction->req.http_method == "GET") {
        ctx = new handle_rf_metadata_context(transaction, path, NULL);
        callback = handle_rf_metadata__read;
    } else if (transaction->req.http_method == "PUT") {
        // this will be freed inside the callback function
        cJSON2 *json_root = cJSON2_Parse(content.c_str());
        if (json_root == NULL) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "unable to parse request body (corrupted json format)"
                             ", ERROR 400. method = %s, URI = %s",
                             transaction->req.http_method.c_str(),
                             transaction->req.uri.c_str());
            status_code = 400;
            response = "{\"errMsg\":\"Invalid request. Invalid JSON format in the request\"}";
            goto error;
        }
        ctx = new handle_rf_metadata_context(transaction, path, json_root);
        callback = handle_rf_metadata__setpermission;
    }

    if (callback == NULL || ctx == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "unhandled case, ERROR 500. http_method=%s, URI=%s",
                         transaction->req.http_method.c_str(),
                         transaction->req.uri.c_str());
        status_code = 500;
        response = "{\"errMsg\":\"Unhandled request\"}";
        goto error;
    }

    callback(ctx);

    goto_respond = false;
    doRead = false;
    is_deferred = true;
    return;

error:
    ::populate_response(transaction->resp, status_code, response, "application/json");
    transaction->processing = false;
    transaction->sending = true;
    goto_respond = true;
    return;

}

typedef void (*ns_handler_fn)(strm_http* http, strm_http_transaction* transaction,
                              const std::string& content,
                              bool& goto_respond, bool& is_deferred, bool& doRead,
                              std::string& mediaFile, int& mediaType);

class RfDispatchTable {
private:
    map<std::string, ns_handler_fn> handler;
public:
    RfDispatchTable() {
        handler["dir"]          = handle_rf_directory_op;
        handler["file"]         = handle_rf_file_op;
        handler["filemetadata"] = handle_rf_metadata_op;
    }
    int dispatch(const std::string& op,
                 strm_http* http, strm_http_transaction* transaction,
                 const std::string& content,
                 bool& goto_respond, bool& is_deferred, bool& doRead,
                 std::string& mediaFile, int& mediaType) {
        if (handler.find(op) != handler.end()) {
            handler[op](http, transaction, content,
                        goto_respond, is_deferred, doRead, mediaFile, mediaType);
            return 0;  // successfully dipatched
        }
        else {
            return -1;  // failed to dispatch
        }
    }
};
static RfDispatchTable rfDispatchTable;

static void handle_ns_rf(strm_http* http, strm_http_transaction* transaction,
                         const std::string& content,
                         bool& goto_respond, bool& is_deferred, bool& doRead,
                         std::string& mediaFile, int& mediaType)
{
    std::vector<std::string> uri_tokens;
    VPLHttp_SplitUri(transaction->req.uri, uri_tokens);

    // not checking the /rf or /media_rf for it's handled by the nsDispatchTable
    // expecting /rf/<op> or /media_rf/<op> which should at least has 2 tokens
    if (uri_tokens.size() < 2) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "no remotefile operation provided in URI");
        ::populate_response(transaction->resp, 400, "{\"errMsg\":\"Invalid request.\"}", "application/json");
        goto_respond = true;
        return;
    }
    int rc = rfDispatchTable.dispatch(uri_tokens[1], http, transaction, content,
                                      goto_respond, is_deferred, doRead, mediaFile, mediaType);
    if (rc != 0) {  // unknown rf_func
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Unknown rf_op %s", uri_tokens[1].c_str());
        ::populate_response(transaction->resp, 400, "{\"errMsg\":\"Invalid request.\"}", "application/json");
        goto_respond = true;
        return;
    }
}

struct handle_mediafile_context {
    strm_http_transaction *transaction;
    const std::string command;

    handle_mediafile_context(strm_http_transaction *transaction,
                             const std::string& command)
        : transaction(transaction), command(command) {
    }
};

static VPLTHREAD_FN_DECL tagedit_thread_fn(void* param)
{
    handle_mediafile_context* ctx = (handle_mediafile_context *)param;

    int rc = 0;
    int status_code = 200;
    std::string response;
    strm_http_transaction *transaction = ctx->transaction;

    if (ctx == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Calling thread function w/o valid context");
        return VPLTHREAD_RETURN_VALUE;
    }
    if (transaction == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Calling thread function w/o valid transaction");
        goto no_trans_out;
    }

    VPLTRACE_LOG_FINE(TRACE_BVS, 0, "TagEdit command: %s", ctx->command.c_str());

#if defined(WIN32)
    wchar_t *wcommand = NULL;

    rc = _VPL__utf8_to_wstring(ctx->command.c_str(), &wcommand);
    if (rc != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "unable to convert command to wchar_t: %s, rc=%d", ctx->command.c_str(), rc);
        response = "{\"errMsg\":\"Unable to convert string to wstring for _wsystem() call\"}";
        status_code = 500;
        goto out;
    } else {
        rc = _wsystem(wcommand);
    }
    if (wcommand != NULL) {
        free(wcommand);
        wcommand = NULL;
    }
#else
    rc = system(ctx->command.c_str());
#endif //defined(WIN32)

    // if error, put the return value at the response body
    if (rc) {
        ostringstream oss;
        oss << rc;
        response = "{\"errMsg\":\"ERR_TAGEDITOR("+oss.str()+")\"}";
        status_code = 400;
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "error returned by tag edit program: %s", response.c_str());
        goto out;
    }

out:
    ::populate_response(transaction->resp, status_code, response, "application/json");
    transaction->processing = false;
    transaction->sending = true;
    transaction->http_handle->put_response(transaction);

no_trans_out:
    delete ctx;

    return VPLTHREAD_RETURN_VALUE;
}

static void handle_mediafile_edittag(strm_http* http, strm_http_transaction* transaction,
                                     const std::string& content,
                                     bool& goto_respond, bool& is_deferred, bool& doRead,
                                     std::string& mediaFile, int& mediaType,
                                     std::string object_id)
{
    int rc = 0;
    int status_code = 200;
    char* vplOsUserId = NULL;
    std::string tagEditPath;
    std::string command;
    std::string response;
    std::map<std::string, std::string>::iterator it;
    const char *type = NULL;
    media_metadata::GetObjectMetadataInput media_request;
    media_metadata::GetObjectMetadataOutput media_response;
    vss_server::msaGetObjectMetadataFunc_t getObjectMetadataCb;
    std::map<std::string, std::string> content_list;
    const std::string *ac_collectionId;
    std::string collectionId;

    // parse request body
    cJSON2 *json_root = cJSON2_Parse(content.c_str());
    if (json_root == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "unable to parse request body (corrupted json format)");
        status_code = 400;
        response = "{\"errMsg\":\"Invalid request. Invalid JSON format in the request\"}";
        goto error;
    } else {
        for (int i = 0 ; i < cJSON2_GetArraySize(json_root) ; i++) {
            cJSON2 * subitem = cJSON2_GetArrayItem(json_root, i);
            // only take cares of first layer string objects
            if (subitem->type == cJSON2_String) {
                content_list[subitem->string] = subitem->valuestring;
            }
        }
        cJSON2_Delete(json_root);
    }

    // check if parameters provided
    if (content_list.empty()) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Stream("FMT_VPLSocket_t"): mediafile editing w/o parameters",
                         VAL_VPLSocket_t(http->sockfd));
        status_code = 400;
        response = "{\"errMsg\":\"Invalid request. No tag editing parameters provided\"}";
        goto error;
    }

    // get the tag editor program path
    tagEditPath = http->server.getTagEditProgramPath();

    if (tagEditPath.empty()) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Stream("FMT_VPLSocket_t"): mediafile editing w/o tagEditPath specified",
                         VAL_VPLSocket_t(http->sockfd));
        status_code = 500;
        response = "{\"errMsg\":\"No tag editing filepath is defined at server side\"}";
        goto error;
    }

#if defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)
    rc = _VPLFS_CheckTrustedExecutable(tagEditPath);
    if (rc == VPL_ERR_NOENT) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Stream("FMT_VPLSocket_t"): mediafile tagedit program doesn't exist: %s",
                         VAL_VPLSocket_t(http->sockfd), tagEditPath.c_str());
        status_code = 500;
        response = "{\"errMsg\":\"No tag editing program found\"}";
        goto error;
    } else if (rc != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Stream("FMT_VPLSocket_t"): mediafile tagedit program is not trusted: %s",
                         VAL_VPLSocket_t(http->sockfd), tagEditPath.c_str());
        status_code = 500;
        response = "{\"errMsg\":\"Tag editing program is not trusted\"}";
        goto error;
    }
#endif //defined(WIN32) && !defined(VPL_PLAT_IS_WINRT)

    // Find optional header 'X-ac-collectionId' - if this exists, we will only load this collection.
    ac_collectionId = transaction->req.find_header("X-ac-collectionId");
    if (ac_collectionId) {
        int err = VPLHttp_DecodeUri(*ac_collectionId, collectionId);
        if (err) collectionId.clear();
    }

    getObjectMetadataCb = http->server.getMsaGetObjectMetadataCb();
    if (getObjectMetadataCb == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Stream("FMT_VPLSocket_t"): Not configured to stream media, responding ERROR 500",
                         VAL_VPLSocket_t(http->sockfd));
        status_code = 500;
        response = "{\"errMsg\":\"Not configured to stream media\"}";
        goto error;
    }

    // get the metadata of the given file (object_id) from server
    media_request.set_object_id(object_id.c_str());
    rc = getObjectMetadataCb(media_request, collectionId, media_response, http->server.getMsaCallbackContext());
    if (rc != 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Stream("FMT_VPLSocket_t"): Content not found, responding ERROR 404 [url %s], %d",
                         VAL_VPLSocket_t(http->sockfd),
                         transaction->req.uri.c_str(), rc);
        status_code = 404;
        response = "{\"errMsg\":\"Content not found\"}";
        goto error;
    }

    // get the file path, media type and format from response
    switch (media_response.media_type()) {
    case media_metadata::MEDIA_MUSIC_TRACK:
        if (media_response.has_file_format()) {
            type = media_response.file_format().c_str();
        }
        if (media_response.has_absolute_path()) {
            mediaFile = media_response.absolute_path();
            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                              "Stream("FMT_VPLSocket_t"): music_track \"%s\"",
                              VAL_VPLSocket_t(http->sockfd),
                              mediaFile.c_str());
        } else {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Stream("FMT_VPLSocket_t"): cannot find absolute path for music_track, object id \"%s\"",
                             VAL_VPLSocket_t(http->sockfd),
                             object_id.c_str());
            status_code = 404;
            response = "{\"errMsg\":\"Content not found\"}";
            goto error;
        }
        mediaType = MEDIA_TYPE_AUDIO;
        break;
    case media_metadata::MEDIA_PHOTO:
        if (media_response.has_file_format()) {
            type = media_response.file_format().c_str();
        }
        if (media_response.has_absolute_path()) {
            mediaFile = media_response.absolute_path();
            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                              "Stream("FMT_VPLSocket_t"): photo_item \"%s\"",
                              VAL_VPLSocket_t(http->sockfd),
                              mediaFile.c_str());
        } else {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Stream("FMT_VPLSocket_t"): cannot find absolute path for photo_item, object id \"%s\"",
                             VAL_VPLSocket_t(http->sockfd),
                             object_id.c_str());
            status_code = 404;
            response = "{\"errMsg\":\"Content not found\"}";
            goto error;
        }
        mediaType = MEDIA_TYPE_PHOTO;
        break;
    case media_metadata::MEDIA_VIDEO:
        if (media_response.has_file_format()) {
            type = media_response.file_format().c_str();
        }
        if (media_response.has_absolute_path()) {
            mediaFile = media_response.absolute_path();
            VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                              "Stream("FMT_VPLSocket_t"): video_item \"%s\"",
                              VAL_VPLSocket_t(http->sockfd),
                              mediaFile.c_str());
        } else {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Stream("FMT_VPLSocket_t"): cannot find absolute path for video_item, object id \"%s\"",
                             VAL_VPLSocket_t(http->sockfd),
                             object_id.c_str());
            status_code = 404;
            response = "{\"errMsg\":\"Content not found\"}";
            goto error;
        }
        mediaType = MEDIA_TYPE_VIDEO;
        break;
    case media_metadata::MEDIA_MUSIC_ALBUM:
    case media_metadata::MEDIA_PHOTO_ALBUM:
    case media_metadata::MEDIA_NONE:
    default:
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Stream("FMT_VPLSocket_t"): Unhandled type",
                         VAL_VPLSocket_t(http->sockfd));
        doRead = false;
        break;
    }

    // file format shall not be empty, required by tag edit program
    if (type == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "unhandled file type, ERROR 500");
        status_code = 500;
        response = "{\"errMsg\":\"Unhandled file type\"}";
        goto error;
    }
#ifdef WIN32
    _VPL__GetUserSidStr(&vplOsUserId);
#endif
    if (vplOsUserId == NULL) {
        rc = VPL_GetOSUserId(&vplOsUserId);
        if (rc < 0) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "%s failed: %d", "VPL_GetOSUserId", rc);
            status_code = 500;
            response = "{\"errMsg\":\"Unable to get user ID\"}";
            goto error;
        }
    }

    command.append(tagEditPath);
    command.append(" --osuserid ");
    command.append(vplOsUserId);
    command.append(" --type ");
    command.append(type);
    command.append(" --file ");
    command.append("\"");
    command.append(mediaFile.c_str());
    command.append("\"");

    // free the memory
    VPL_ReleaseOSUserId(vplOsUserId);

    // append the tag editing parameters
    for(it=content_list.begin(); it != content_list.end(); it++) {
        command.append(" --");
        command.append(it->first);
        command.append(" ");
        command.append(it->second);
    }

    VPLTRACE_LOG_INFO(TRACE_BVS, 0, "Edit object[%s] tag; command: %s",
                      object_id.c_str(), command.c_str());

    {
        VPLThread_attr_t thread_attributes;
        VPLDetachableThreadHandle_t thread_handle;

        handle_mediafile_context *ctx = new handle_mediafile_context(transaction, command);

        // create thread
        rc = VPLThread_AttrInit(&thread_attributes);
        if (rc < 0) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLThread_AttrInit returned %d", rc);
            goto thread_out;
        }

        rc = VPLThread_AttrSetDetachState(&thread_attributes, VPL_TRUE);
        if (rc < 0) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLThread_AttrSetDetachState returned %d", rc);
            goto thread_out;
        }

        rc = VPLDetachableThread_Create(&thread_handle, tagedit_thread_fn, (void *)ctx, &thread_attributes, NULL);
        if (rc < 0) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, "VPLDetachableThread_Create returned %d", rc);
        }
        // defered the response until we finished it at the tagedit_thread_fn
        is_deferred = true;
thread_out:
        VPLThread_AttrDestroy(&thread_attributes);

        // response w/ error
        if (rc < 0) {
            status_code = 500;
            response = "{\"errMsg\":\"Unable to spawn tag editing thread\"}";
            goto error;
        }
    }
    return;

error:
    ::populate_response(transaction->resp, status_code, response, "application/json");
    transaction->processing = false;
    transaction->sending = true;
    goto_respond = true;

    return;
}

static void handle_ns_mediafile(strm_http* http, strm_http_transaction* transaction,
                         const std::string& content,
                         bool& goto_respond, bool& is_deferred, bool& doRead,
                         std::string& mediaFile, int& mediaType)
{
    std::vector<std::string> uri_tokens;

    // Get device ID & UUID "/mediafile/tag/<deviceid>/<uuid>
    VPLHttp_SplitUri(transaction->req.uri, uri_tokens);
    if (uri_tokens.size() < 4 || uri_tokens[1] != "tag") {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Invalid request, http_method=%s, uri=%s",
                         transaction->req.http_method.c_str(),
                         transaction->req.uri.c_str());
        ::populate_response(transaction->resp, 400, "{\"errMsg\":\"Invalid request.\"}", "application/json");
        goto_respond = true;
        return;
    }

    handle_mediafile_edittag(http, transaction, content, goto_respond,
                             is_deferred, doRead, mediaFile, mediaType,
                             uri_tokens[3]);

}

class NsDispatchTable {
private:
    map<std::string, ns_handler_fn> handler;
public:
    NsDispatchTable() {
        handler["test"] = handle_ns_test;
#ifdef TS_DEV_TEST
        handler["tstest"] = handle_ns_tstest;
#endif
        handler["mm"]   = handle_ns_mm;
        handler["minidms"] = handle_ns_minidms;
        handler["cmd"]  = handle_ns_cmd;
        handler["rf"]   = handle_ns_rf;
        handler["media_rf"]  = handle_ns_rf;
        handler["mediafile"] = handle_ns_mediafile;
    }
    int dispatch(const std::string& ns,
                 strm_http* http, strm_http_transaction* transaction,
                 const std::string& content,
                 bool& goto_respond, bool& is_deferred, bool& doRead,
                 std::string& mediaFile, int& mediaType) {
        if (handler.find(ns) != handler.end()) {
            handler[ns](http, transaction, content,
                        goto_respond, is_deferred, doRead, mediaFile, mediaType);
            return 0;  // successfully dipatched
        }
        else {
            return -1;  // failed to dispatch
        }
    }
};
static NsDispatchTable nsDispatchTable;

void handle_get_or_head(strm_http* http, strm_http_transaction* transaction)
{
    if (transaction->req.http_method == "GET") {
        VPLTRACE_LOG_FINE(TRACE_BVS, 0, 
                          "Stream("FMT_VPLSocket_t"): GET request for URI %s",
                          VAL_VPLSocket_t(http->sockfd),
                          transaction->req.uri.c_str());
    } else {
        VPLTRACE_LOG_FINE(TRACE_BVS, 0, 
                          "Stream("FMT_VPLSocket_t"): HEAD request for URI %s",
                          VAL_VPLSocket_t(http->sockfd),
                          transaction->req.uri.c_str());
    }

    int         rc = 0;
    string      content;
    bool        goto_respond = false;
    bool        is_deferred = false;
    bool        doRead = false;
    string      mediaFile;
    int         mediaType;  
    string      cmdToken;
    string      uri_ns;

    // Check headers
    if(transaction->req.version != "HTTP/1.1" && 
       transaction->req.find_header("Host")  == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, 
                         "Stream("FMT_VPLSocket_t"): Invalid request , responding ERROR 400",
                         VAL_VPLSocket_t(http->sockfd));
        transaction->resp.add_response(400);
        goto respond;
    }

    (IGNORE_RESULT) parse_uri(transaction->req.uri, content);
    uri_ns = get_uri_namespace(transaction->req.uri);

    rc = nsDispatchTable.dispatch(uri_ns, http, transaction, content, 
                                  goto_respond, is_deferred, doRead, mediaFile, mediaType);
    if (rc != 0) {  // unknown namespace
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Unknown namespace %s", uri_ns.c_str());
        ::populate_response(transaction->resp, 400, "{\"errMsg\":\"Invalid request.\"}", "application/json");
        goto respond;
    }
    if (is_deferred) {
        // don't send any response yet, just exit the function
        return;
    }
    if (goto_respond) {
        goto respond;
    }

#ifdef ENABLE_PHOTO_TRANSCODE
    if (transaction->is_image_transcoding) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Pending the response until transcoding is done.");
        goto pending_response;
    }
#endif
    if (doRead) {
        GetMediaFileResp_ctx* gmfr_ctx =
                new GetMediaFileResp_ctx(mediaFile, mediaType, transaction, http);
        // call the callback function directly.
        get_mediafile_resp_helper(gmfr_ctx);  // will call put_response
        goto pending_response;
    } else {
        if ((content.size() == 0) || (uri_ns == "cmd")) {
            transaction->resp.add_response(204);
        } else {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0, 
                             "Stream("FMT_VPLSocket_t"): Invalid request [No valid content specified] , responding ERROR 400",
                             VAL_VPLSocket_t(http->sockfd));
            ::populate_response(transaction->resp, 400, "{\"errMsg\":\"Invalid request.\"}", "application/json");
        }
    }

 respond:
    http->put_response(transaction);
 pending_response:
    ;
}

// TODO:  Post command is for connection manager service or av transport service. 
//        Not handling this now.
void handle_put_post_n_del(strm_http* http, strm_http_transaction* transaction)
{
    VPLTRACE_LOG_FINE(TRACE_BVS, 0, 
                      "Stream("FMT_VPLSocket_t"): Got a %s request for URI %s",
                      VAL_VPLSocket_t(http->sockfd),
                      transaction->req.http_method.c_str(),
                      transaction->req.uri.c_str());

    int         rc = 0;
    string      content;
    bool        goto_respond = false;
    bool        is_deferred = false;
    bool        doRead = false;
    string      mediaFile;
    int         mediaType;  
    string      cmdToken;
    string      uri_ns;

    // Check headers
    if(transaction->req.version != "HTTP/1.1" && 
       transaction->req.find_header("Host")  == NULL) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, 
                         "Stream("FMT_VPLSocket_t"): Invalid request , responding ERROR 400",
                         VAL_VPLSocket_t(http->sockfd));
        transaction->resp.add_response(400);
        goto respond;
    }

    (IGNORE_RESULT) parse_uri(transaction->req.uri, content);
    uri_ns = get_uri_namespace(transaction->req.uri);
    content.clear();
    rc = transaction->req.read_content(content);

    rc = nsDispatchTable.dispatch(uri_ns, http, transaction, content, 
                                  goto_respond, is_deferred, doRead, mediaFile, mediaType);
    if (rc != 0) {  // unknown namespace
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Unknown namespace %s", uri_ns.c_str());
        ::populate_response(transaction->resp, 400, "{\"errMsg\":\"Invalid request.\"}", "application/json");
        goto respond;
    }
    if (goto_respond) {
        goto respond;
    }
    if (is_deferred) {
        goto end;
    }
    if(!transaction->req.read_complete()) {
        return;
    }

 respond:
    http->put_response(transaction);
 end:
    ;
}

void strm_http::log_completion(strm_http_transaction* transaction, bool aborted)
{
    const string* rangestr = transaction->req.find_header("Range");

    VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                      "Stream("FMT_VPLSocket_t"): Done %s "FMT_VPLTime_t"us in:"FMTu64" out:"FMTu64" \"%s %s %s\" File(%s) Range(%s) %d%s",
                      VAL_VPLSocket_t(sockfd),
                      (conn_type == STRM_DIRECT) ? "direct" :
                      (conn_type == STRM_PROXY) ? "proxy" : "p2p",
                      VPLTime_GetTimeStamp() - transaction->start,
                      transaction->recv_cnt, transaction->send_cnt,
                      transaction->req.http_method.c_str(),
                      transaction->req.uri.c_str(),
                      transaction->req.version.c_str(),
                      transaction->content_file.c_str(),
                      rangestr ? rangestr->c_str() : "none",
                      transaction->resp.response,
                      aborted ? " incomplete" : "");
}
