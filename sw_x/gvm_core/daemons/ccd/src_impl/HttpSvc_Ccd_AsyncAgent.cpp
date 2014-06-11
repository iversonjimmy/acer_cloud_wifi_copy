#include "HttpSvc_Ccd_AsyncAgent.hpp"

#include "HttpSvc_Ccd_Dispatcher.hpp"
#include "HttpSvc_Ccd_Server.hpp"
#include "HttpSvc_Utils.hpp"

#include "AsyncUploadQueue.hpp"
#include "cache.h"
#include "ccd_storage.hpp"
#include "stream_transaction.hpp"
#include "virtual_device.hpp"

#include <gvm_errors.h>
#include <gvm_thread_utils.h>
#include <log.h>
#include <HttpFileUploadStream.hpp>
#include <HttpStringStream.hpp>
#include <InStringStream.hpp>
#include <OutStringStream.hpp>

#include <scopeguard.hpp>
#include <vpl_fs.h>
#include <vpl_time.h>
#include <vplex_file.h>
#include <vplex_serialization.h>
#include <vplu_format.h>
#include <vplu_mutex_autolock.hpp>

#include <new>
#include <sstream>
#include <string>

// TODOs
// Stop using stream_transaction as container for async requests.
//  This will require changes to AsyncUploadQueue.* too.

#ifdef DEBUG
#define LOG_STATE LOG_INFO("AsyncAgent[%p]: State %s", this, getStateStr().c_str())
#else
#define LOG_STATE LOG_INFO("AsyncAgent[%p]: State %x", this, getStateNum())
#endif

#define FILE_READ_BUFFER_SIZE (32*1024)

// TODO for 2.7, Bug 13079:
// see if we can lower the stack requirement 16KB (UTIL_DEFAULT_THREAD_STACK_SIZE) for non-Android.
static const size_t AsyncAgent_StackSize = 
#ifdef ANDROID
    UTIL_DEFAULT_THREAD_STACK_SIZE
#else
    128 * 1024
#endif
    ;

HttpSvc::Ccd::AsyncAgent::AsyncAgent(Server *server, u64 userId)
    : Agent(server, userId), 
      threadState(Utils::ThreadState_NoThread), cancelState(Utils::CancelState_NoCancel),
      uploadQueueReady(false), emptyDbOnDestroy(false), curReqId(0), curHs(NULL), curSentSoFar(0), lastUpdateTimestamp(0)
{
    VPLCond_Init(&has_req_cond);

    do {
        upload_queue = new (std::nothrow) async_upload_queue();
        if (!upload_queue) {
            LOG_ERROR("AsyncAgent[%p]: No memory to create async_upload_queue obj", this);
            break;
        }
        char path[CCD_PATH_MAX_LENGTH];
        DiskCache::getPathForAsyncUpload(sizeof(path), path);
        int err = upload_queue->init(path, userId);
        if (err) {
            LOG_ERROR("AsyncAgent[%p]: Failed to initialize async_upload_queue: err %d", this, err);            
            break;
        }
        err = upload_queue->startProcessing(userId);
        if (err) {
            LOG_ERROR("AsyncAgent[%p]: Failed to start async_upload_queue: err %d", this, err);
            break;
        }
        uploadQueueReady = true;
    } while (0);

    LOG_INFO("AsyncAgent[%p]: Created for Server[%p] User["FMTu64"]", this, server, userId);
}

HttpSvc::Ccd::AsyncAgent::~AsyncAgent()
{
    VPLCond_Destroy(&has_req_cond);

    if (upload_queue) {
        if (emptyDbOnDestroy) {
            upload_queue->deleteAllTrans();
        }
        delete upload_queue;
    }

    LOG_INFO("AsyncAgent[%p]: Destroyed", this);
}

int HttpSvc::Ccd::AsyncAgent::Start()
{
    int err = 0;

    if (!uploadQueueReady) {
        LOG_ERROR("AsyncAgent[%p]: Failed to create queue", this);
        return CCD_ERROR_NOT_INIT;
    }

    {
        MutexAutoLock lock(&mutex);
        threadState = Utils::ThreadState_Spawning;
        LOG_STATE;
    }

    AsyncAgent *copyOf_this = Utils::CopyPtr(this);  // REFCOUNT(AsyncAgent,ForThread)
    err = Util_SpawnThread(thread_main, (void*)copyOf_this, AsyncAgent_StackSize, /*isJoinable*/VPL_FALSE, &thread);
    if (err != VPL_OK) {
        LOG_ERROR("AsyncAgent[%p]: Failed to spawn thread: %d", this, err);
        Utils::DestroyPtr(copyOf_this);  // REFCOUNT(AsyncAgent,ForThread)
        goto end;
    }

 end:
    return err;
}

int HttpSvc::Ccd::AsyncAgent::AsyncStop(bool isUserLogout)
{
    int err = 0;

    {
        MutexAutoLock lock(&mutex);

        if (cancelState != Utils::CancelState_Canceling) {
            cancelState = Utils::CancelState_Canceling;
            LOG_STATE;
        }

        VPLCond_Signal(&has_req_cond);

        if (curHs) {
            curHs->StopIo();
        }

        emptyDbOnDestroy = isUserLogout;
    }

    return err;
}

int HttpSvc::Ccd::AsyncAgent::GetJsonTaskStatus(std::string &response)
{
    MutexAutoLock lock(&mutex);
    if (!upload_queue)
        return CCD_ERROR_NOT_INIT;
    return upload_queue->getJsonTaskStatus(response);
}

int HttpSvc::Ccd::AsyncAgent::QueryTransaction(stream_transaction &st, u64 id, u64 userId, bool &found)
{
    MutexAutoLock lock(&mutex);
    if (!upload_queue)
        return CCD_ERROR_NOT_INIT;
    return upload_queue->queryTransaction(st, id, userId, found);
}

int HttpSvc::Ccd::AsyncAgent::Dequeue(stream_transaction &st)
{
    MutexAutoLock lock(&mutex);
    if (!upload_queue)
        return CCD_ERROR_NOT_INIT;
    return upload_queue->dequeue(st);
}

int HttpSvc::Ccd::AsyncAgent::Enqueue(u64 uid, u64 deviceid, u64 sent_so_far, u64 size, const char *request_content, int status, u64 &handle)
{
    MutexAutoLock lock(&mutex);
    if (!upload_queue)
        return CCD_ERROR_NOT_INIT;
    int err = upload_queue->enqueue(uid, deviceid, sent_so_far, size, request_content, status, handle);

    VPLCond_Signal(&has_req_cond);

    return err;
}

int HttpSvc::Ccd::AsyncAgent::AddRequest(u64 uid, u64 deviceid, u64 size, const std::string &request_content, u64 &requestId)
{
    return Enqueue(uid, deviceid, 0, size, request_content.c_str(), 0, requestId);
}

int HttpSvc::Ccd::AsyncAgent::CancelRequest(u64 requestId)
{
    int err = 0;

    // Some error codes returned from this function have special meanings:
    // CCD_ERROR_NOT_FOUND - mapped to 404
    // All other errors are mapped to 400.

    stream_transaction st;
    bool found;
    err = QueryTransaction(st, requestId, userId, found);
    if (err) {
        LOG_ERROR("Failed to query status of AsyncTransferRequest["FMTu64"]: err %d", requestId, err);
        // Make sure it's not one of the special error codes.
        if (err == CCD_ERROR_NOT_FOUND || err == CCD_ERROR_WRONG_STATE)
            err = CCD_ERROR_PARAMETER;
        return err;
    }

    if (!found) {
        LOG_ERROR("AsyncTransferRequest["FMTu64"] not found", requestId);
        return CCD_ERROR_NOT_FOUND;
    }

    if (st.upload_status == pending) {
        err = Dequeue(st);
        if (err) {
            LOG_ERROR("Failed to dequeue AsyncTransferRequest["FMTx64"]: err %d", requestId, err);
            // Make sure it's not one of the special error codes.
            if (err == CCD_ERROR_NOT_FOUND || err == CCD_ERROR_WRONG_STATE)
                err = CCD_ERROR_PARAMETER;
        }
    } else if (st.upload_status == querying ||
               st.upload_status == processing) {
        st.upload_status = cancelling;
        err = UpdateUploadStatus(st.id, cancelling, userId);
        if (err) {
            LOG_ERROR("Failed to update status of AsyncTransferRequest["FMTu64"]: err %d", requestId, err);
            // Make sure it's not one of the special error codes.
            if (err == CCD_ERROR_NOT_FOUND || err == CCD_ERROR_WRONG_STATE)
                err = CCD_ERROR_PARAMETER;
        }

        // If the current HttpStream obj is for this async request, cancel it.
        {
            MutexAutoLock lock(&mutex);
            if (curHs && curReqId == requestId) {
                curHs->StopIo();
            }
        }
    } else {
        // The request is already finished or canceling - wrong state to cancel.
        LOG_ERROR("AsyncTransferRequest["FMTu64"] in wrong state(%d) to cancel", requestId, st.upload_status);
        err = CCD_ERROR_WRONG_STATE;
    }

    return err;
}

int HttpSvc::Ccd::AsyncAgent::GetRequestStatus(u64 requestId, RequestStatus &status)
{
    int err = 0;

    // Some error codes returned from this function have special meanings:
    // CCD_ERROR_NOT_FOUND - mapped to 404
    // All other errors are mapped to 400.

    stream_transaction st;
    bool found;
    err = QueryTransaction(st, requestId, userId, found);
    if (err) {
        LOG_ERROR("Failed to query status of AsyncTransferRequest["FMTu64"]: err %d", requestId, err);
        // Make sure it's not one of the special error codes.
        if (err == CCD_ERROR_NOT_FOUND)
            err = CCD_ERROR_PARAMETER;
        return err;
    }

    if (!found) {
        LOG_ERROR("AsyncTransferRequest["FMTu64"] not found", requestId);
        return CCD_ERROR_NOT_FOUND;
    }

    // Copying undocumented behavior from previous implementation.
    // Dequeue finished request on first query.
    if ((st.sent_so_far == st.size && st.upload_status == complete) ||
        st.upload_status == cancelled ||
        st.upload_status == failed) {
        err = Dequeue(st);
        if (err) {
            LOG_ERROR("Failed to dequeue AsyncTransferRequest["FMTx64"]: err %d", requestId, err);
            // Make sure it's not one of the special error codes.
            if (err == CCD_ERROR_NOT_FOUND)
                err = CCD_ERROR_PARAMETER;
        }
    }

    status.requestId   = requestId;
    status.status      = st.upload_status;
    status.size        = st.size;
    status.sent_so_far = st.sent_so_far;

    return err;
}

int HttpSvc::Ccd::AsyncAgent::UpdateStatus(stream_transaction &st)
{
    MutexAutoLock lock(&mutex);
    if (!upload_queue)
        return CCD_ERROR_NOT_INIT;
    return upload_queue->updateStatus(st);
}

int HttpSvc::Ccd::AsyncAgent::UpdateUploadStatus(u64 requestId, UploadStatusType upload_status, u64 userId)
{
    MutexAutoLock lock(&mutex);
    if (!upload_queue)
        return CCD_ERROR_NOT_INIT;
    return upload_queue->updateUploadStatus(requestId, upload_status, userId);
}

int HttpSvc::Ccd::AsyncAgent::UpdateSentSoFar(u64 requestId, u64 sent_so_far)
{
    MutexAutoLock lock(&mutex);
    if (!upload_queue)
        return CCD_ERROR_NOT_INIT;
    return upload_queue->updateSentSoFar(requestId, sent_so_far);
}

VPLTHREAD_FN_DECL HttpSvc::Ccd::AsyncAgent::thread_main(void* param)
{
    AsyncAgent *aagent = static_cast<AsyncAgent*>(param);

    aagent->thread_main();

    Utils::DestroyPtr(aagent);  // REFCOUNT(AsyncAgent,ForThread)

    return VPLTHREAD_RETURN_VALUE;
}

stream_transaction *HttpSvc::Ccd::AsyncAgent::tryGetRequest()
{
    stream_transaction *st = NULL;

    MutexAutoLock lock(&mutex);

    if (cancelState == Utils::CancelState_Canceling) {
        LOG_INFO("AsyncAgent[%p]: Cancel detected", this);
        return NULL;
    }
    // assert: upload_queue != NULL

    upload_queue->refreshTransNum();  // FUMI: this sets upload_queue->trans_num to num of reqs in pending|processing|querying
    if(upload_queue->isReadyToUse() && upload_queue->getTransNum() > 0) {
        st = new (std::nothrow) stream_transaction(/*async_client*/NULL);
        if (!st) {
            LOG_ERROR("AsyncAgent[%p]: No memory to create stream_transaction obj", this);
            goto end;
        }
        int err = upload_queue->activateNextTransaction(*st);
        // FUMI: gets oldest request that is pending|processing|querying
        if (err != 0) {
            LOG_ERROR("activateNextTransaction failed. error:%d", err);
            delete st;
            st = NULL;
        }
    }

 end:
    return st;
}

int HttpSvc::Ccd::AsyncAgent::checkNoExistence(stream_transaction *async_req)
{
    int err = 0;

    // Test file presence by making GET /rf/filemetadata/<path>.

    // we are going to create a GET /rf/filemetadata/<file_path> request to server
    // before we actually start transaction
    // clone the request_content and change http method to GET to avoid
    // http_request_async from parsing the file content

    // Fumi: original request header is saved in async_req->request_content
#if 0
    Sample request_content:

POST /rf/file/5305291/aaa HTTP/1.1
x-ac-srcfile: /temp/a9
Content-Type: application/x-www-form-urlencoded
Host: 127.0.0.1:41740
Accept: * / *
Content-Length: 70
x-ac-userId: 548955
x-ac-sessionHandle: -7123493070466516414
x-ac-serviceTicket: qzkb+ZvbhRsCPvlvNQRHt0VUtn8=


#endif

    do {
        std::string uri;
        {
            // the URI is between the first two space chars
            size_t space1 = async_req->request_content.find_first_of(" \r\n");
            if (space1 == std::string::npos) {
                LOG_ERROR("AsyncAgent[%p]: Malformed request: %s", this, async_req->request_content.c_str());
                err = CCD_ERROR_PARSE_CONTENT;
                goto end;
            }
            size_t space2 = async_req->request_content.find_first_of(" \r\n", space1 + 1);
            if (space2 == std::string::npos) {
                LOG_ERROR("AsyncAgent[%p]: Malformed request: %s", this, async_req->request_content.c_str());
                err = CCD_ERROR_PARSE_CONTENT;
                goto end;
            }
            uri.assign(async_req->request_content, space1 + 1, space2 - space1 - 1);
        }

        std::string prefixPattern;
        bool isMedia_rf = false;
        {
            const std::string prefixPattern_rf_file = "/rf/file/";
            const std::string prefixPattern_mediarf_file = "/media_rf/file/";
            if (uri.compare(0, prefixPattern_rf_file.size(), prefixPattern_rf_file) == 0) {
                prefixPattern = prefixPattern_rf_file;
                isMedia_rf = false;
            }
            else if (uri.compare(0, prefixPattern_mediarf_file.size(), prefixPattern_mediarf_file) == 0) {
                prefixPattern = prefixPattern_mediarf_file;
                isMedia_rf = true;
            }
            else {
                LOG_ERROR("AsyncAgent[%p]: Unexpected URI: uri %s", this, uri.c_str());
                err = CCD_ERROR_PARSE_CONTENT;
                goto end;
            }
        }

        ServiceSessionInfo_t serviceSessionInfo;
        err = Cache_GetSessionForVsdsByUser(userId, serviceSessionInfo);
        if (err) {
            LOG_ERROR("AsyncAgent[%p]: Cache_GetSessionForVsdsByUser failed: err %d", this, err);
            goto end;
        }

        char *serviceTicket = NULL;
        {
            const std::string &curServiceTicket = serviceSessionInfo.serviceTicket;
            size_t buffersize = VPL_BASE64_ENCODED_SINGLE_LINE_BUF_LEN(curServiceTicket.size());
            serviceTicket = (char*)malloc(buffersize);
            if (serviceTicket == NULL) {
                err = CCD_ERROR_NOMEM;
                goto end;
            }
            VPL_EncodeBase64(curServiceTicket.data(), curServiceTicket.size(),
                             serviceTicket, &buffersize, /*addNewLines*/VPL_FALSE, /*urlSafe*/VPL_FALSE);
        }
        ON_BLOCK_EXIT(free, serviceTicket);

        std::string reqhdr;
        {
            std::ostringstream oss;
            if (isMedia_rf) {
                oss << "GET /media_rf/filemetadata/";
            } else {
                oss << "GET /rf/filemetadata/";
            }
            oss.write(uri.data() + prefixPattern.size(), uri.size() - prefixPattern.size());
            oss << " HTTP/1.1\r\n";
            oss << Utils::HttpHeader_ac_userId        << ": " << userId << "\r\n";
            oss << Utils::HttpHeader_ac_sessionHandle << ": " << (s64)serviceSessionInfo.sessionHandle << "\r\n";
            oss << Utils::HttpHeader_ac_serviceTicket << ": " << serviceTicket << "\r\n";
            oss << Utils::HttpHeader_ac_deviceId      << ": " << VirtualDevice_GetDeviceId() << "\r\n";
            oss << "\r\n";
            reqhdr.assign(oss.str());
        }

        InStringStream iss(reqhdr);
        OutStringStream oss;

        {
            MutexAutoLock lock(&mutex);

            curHs = new (std::nothrow) HttpStringStream(&iss, &oss);
            if (!curHs) {
                LOG_ERROR("AsyncAgent[%p]: No memory to create HttpStringStream obj", this);
                err = CCD_ERROR_NOMEM;
                goto end;
            }
            curReqId = async_req->id;
            LOG_INFO("AsyncAgent[%p]: Created HttpStringStream[%p]", this, curHs);
            curHs->SetUserId(userId);
        }

        err = HttpSvc::Ccd::Dispatcher::Dispatch(curHs);
        if (!err) {
            MutexAutoLock lock(&mutex);

            int statuscode = curHs->GetStatusCode();
            if (statuscode != 404) {
                err = CCD_ERROR_HTTP_STATUS;
                goto end;
            }
        }

        // if it's parent directory that's missing, stop uploading and claim an error
        // for detail check bug 8074
        // NOTE: the error message should be matched with storageNode/src/strm_http.cpp::check_path_permission()
        {
            std::string output = oss.GetOutput();
            if (output.find(RF_ERR_MSG_NODIR) != output.npos) {
                // something exists
                err = CCD_ERROR_HTTP_STATUS;
                goto end;
            }
        }
    } while (0);

 end:
    if (err) {
        int err2 = UpdateUploadStatus(async_req->id, failed, userId);
        if (err2) {
            LOG_ERROR("AsyncAgent[%p]: Failed to update request (id "FMTu64") status: err %d", this, async_req->id, err2);
        }
    }

    {
        MutexAutoLock lock(&mutex);
        if (curHs) {
            delete curHs;
            curHs = NULL;
            curReqId = 0;
        }
    }

    return err;
}

// class method
void HttpSvc::Ccd::AsyncAgent::FileReadCb(void *ctx, size_t bytes, const char* buf)
{
    if (!ctx) {
        LOG_ERROR("AsyncAgent: FileReadCb called with null context");
        return;
    }

    HttpSvc::Ccd::AsyncAgent *agent = (HttpSvc::Ccd::AsyncAgent*)ctx;
    agent->FileReadCb(bytes);
}

void HttpSvc::Ccd::AsyncAgent::FileReadCb(size_t bytes)
{
    if (!curReqId) {
        LOG_ERROR("AsyncAgent[%p]: FileReadCb called when no request in progress", this);
        return;
    }

    curSentSoFar += bytes;

    // update the sent-so-far column in the db, but no more than once per second
    // this is done for performance reason
    VPLTime_t now = VPLTime_GetTimeStamp();
    if ((lastUpdateTimestamp == 0) || (now >= lastUpdateTimestamp + VPLTime_FromSec(1))) {
        int err = UpdateSentSoFar(curReqId, curSentSoFar);
        if (err) {
            LOG_ERROR("AsyncAgent[%p]: Failed to update request["FMTu64"] status: err %d", this, curReqId, err);
        }
        lastUpdateTimestamp = now;
    }
}

int HttpSvc::Ccd::AsyncAgent::uploadFile(stream_transaction *async_req)
{
    int err = 0;

    LOG_INFO("uploadFile");

    do {
        // prepare http request header
        std::string request_header;
        std::string filepath;
        u64 filesize = 0;
        {
            http_request hr;
            hr.receive(async_req->request_content.data(), async_req->request_content.size());

            const std::string *p = hr.find_header(Utils::HttpHeader_ac_srcfile);
            if (p) {
                filepath = *p;
            }
            if (filepath.empty()) {
                LOG_ERROR("AsyncAgent[%p]: Failed to find source file path in request", this);
                err = -1;  // FIXME
                break;
            }

            VPLFS_stat_t stat;
            err = VPLFS_Stat(filepath.c_str(), &stat);
            if (err) {
                LOG_ERROR("AsyncAgent[%p]: VPLStat() failed: err %d", this, err);
                break;
            }
            filesize = stat.size;
            {
                std::ostringstream oss;
                oss << filesize;
                hr.headers[Utils::HttpHeader_ContentLength] = oss.str();
            }

            // setup user id
            {
                std::ostringstream oss;
                oss << userId;
                hr.headers[Utils::HttpHeader_ac_userId] = oss.str();
            }

            // setup device id
            {
                std::ostringstream oss;
                oss << VirtualDevice_GetDeviceId();
                hr.headers[Utils::HttpHeader_ac_origDeviceId] = oss.str();
            }

            // add/update session info
            {
                ServiceSessionInfo_t serviceSessionInfo;
                err = Cache_GetSessionForVsdsByUser(userId, serviceSessionInfo);
                if (err) {
                    LOG_ERROR("AsyncAgent[%p]: Cache_GetSessionForVsdsByUser failed: err %d", this, err);
                    break;
                }

                {
                    std::ostringstream oss;
                    oss << (s64)serviceSessionInfo.sessionHandle;
                    hr.headers[Utils::HttpHeader_ac_sessionHandle] = oss.str();
                }
                {
                    const std::string &curServiceTicket = serviceSessionInfo.serviceTicket;
                    size_t buffersize = VPL_BASE64_ENCODED_SINGLE_LINE_BUF_LEN(curServiceTicket.size());
                    char *buffer = (char*)malloc(buffersize);
                    if (buffer == NULL) {
                        err = CCD_ERROR_NOMEM;
                        break;
                    }
                    ON_BLOCK_EXIT(free, buffer);
                    VPL_EncodeBase64(curServiceTicket.data(), curServiceTicket.size(),
                                     buffer, &buffersize, /*addNewLines*/VPL_FALSE, /*urlSafe*/VPL_FALSE);
                    hr.headers[Utils::HttpHeader_ac_serviceTicket].assign(buffer);
                }
            }

            hr.dump(request_header);
            LOG_INFO("fixed_header %s<<", request_header.c_str());
        }

        VPLFile_handle_t file = VPLFILE_INVALID_HANDLE;
        file = VPLFile_Open(filepath.c_str(), VPLFILE_OPENFLAG_READONLY, 0);
        if (!VPLFile_IsValidHandle(file)) {
            LOG_ERROR("AsyncAgent[%p]: Failed to open file %s", this, filepath.c_str());
            err = file;
            break;
        }
        ON_BLOCK_EXIT(VPLFile_Close, file);

        InStringStream iss_reqhdr(request_header);
        OutStringStream oss_resp;

        HttpFileUploadStream *hfus = new (std::nothrow) HttpFileUploadStream(&iss_reqhdr, file, &oss_resp);
        if (!hfus) {
            LOG_ERROR("AsyncAgent[%p]: No memory to create HttpFileUploadStream obj", this);
            err = CCD_ERROR_NOMEM;
            break;
        }
        hfus->SetUserId(userId);
        hfus->SetBodyReadCb(FileReadCb, this);

        {
            MutexAutoLock lock(&mutex);
            curHs = hfus;
            curReqId = async_req->id;
            curSentSoFar = 0;
            LOG_INFO("AsyncAgent[%p]: Created HttpFileUploadStream[%p]", this, curHs);
        }

        async_req->upload_status = processing;
        err = UpdateUploadStatus(async_req->id, processing, userId);
        if (err) {
            LOG_ERROR("AsyncAgent[%p]: Failed to update request (id "FMTu64") status", this, async_req->id);
            break;
        }

        // Set last update timestamp to 0, so the first call to FileReadCb() will write to DB.
        lastUpdateTimestamp = 0;

        err = HttpSvc::Ccd::Dispatcher::Dispatch(curHs);
        if (!err) {
            MutexAutoLock lock(&mutex);
            
            int statuscode = curHs->GetStatusCode();
            LOG_INFO("status code is %d", statuscode);
            if (statuscode != 200)
                err = CCD_ERROR_HTTP_STATUS;
        }
        if (err) {
            LOG_ERROR("AsyncAgent[%p]: Failed to dispatch request["FMTu64"], err: %d", this, curReqId, err);
            break;
        }

        err = UpdateSentSoFar(curReqId, curSentSoFar);
        if (err) {
            LOG_ERROR("AsyncAgent[%p]: Failed to update request["FMTu64"] status: err %d", this, curReqId, err);
        }
        
    } while (0);

    {
        int err2 = UpdateUploadStatus(async_req->id, err ? (err == VPL_ERR_CANCELED ? cancelled : failed) : complete, userId);
        if (err2) {
            LOG_ERROR("AsyncAgent[%p]: Failed to update request["FMTu64"] status: err %d", this, async_req->id, err2);
            if (!err) err = err2;
        }
    }

    {
        MutexAutoLock lock(&mutex);
        if (curHs) {
            delete curHs;
            curHs = NULL;
            curReqId = 0;
        }
    }

    return err;
}

void HttpSvc::Ccd::AsyncAgent::thread_main()
{
    int err = 0;

    {
        MutexAutoLock lock(&mutex);
        threadState = Utils::ThreadState_Running;
        LOG_STATE;
    }

    VPLMutex_Lock(&mutex);
    while (cancelState != Utils::CancelState_Canceling) {
        upload_queue->refreshTransNum();
        while (cancelState != Utils::CancelState_Canceling && upload_queue->getTransNum() == 0) {
            VPLCond_TimedWait(&has_req_cond, &mutex, VPL_TIMEOUT_NONE);
            if (cancelState == Utils::CancelState_Canceling) {
                LOG_INFO("AsyncAgent[%p]: Cancel detected", this);
                break;
            }
            upload_queue->refreshTransNum();
        }
        if (cancelState == Utils::CancelState_Canceling) {
            LOG_INFO("AsyncAgent[%p]: Cancel detected", this);
            break;
        }
        VPLMutex_Unlock(&mutex);

        stream_transaction *async_req = tryGetRequest();
        if (async_req) {
            err = checkNoExistence(async_req);
            if (!err) {
                err = uploadFile(async_req);
                if (err) {
                    LOG_ERROR("AsyncAgent[%p]: Upload failed: err %d", this, err);
                }
            }

            delete async_req;
        }

        VPLMutex_Lock(&mutex);
    }
    VPLMutex_Unlock(&mutex);

    {
        MutexAutoLock lock(&mutex);
        threadState = Utils::ThreadState_NoThread;
        cancelState = Utils::CancelState_NoCancel;
        LOG_STATE;
    }
}

std::string HttpSvc::Ccd::AsyncAgent::getStateStr() const
{
    MutexAutoLock lock(&mutex);
    std::ostringstream oss;

    oss << "<"
        << Utils::GetThreadStateStr(threadState)
        << ","
        << Utils::GetCancelStateStr(cancelState)
        << ">";

    return oss.str();
}

int HttpSvc::Ccd::AsyncAgent::getStateNum() const
{
    MutexAutoLock lock(&mutex);
    return (threadState << 4) | cancelState;
}

