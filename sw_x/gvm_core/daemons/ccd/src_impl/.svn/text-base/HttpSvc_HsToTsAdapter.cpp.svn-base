#include "HttpSvc_HsToTsAdapter.hpp"
#include "HttpSvc_Utils.hpp"

#include "ccd_features.h"

#include <gvm_errors.h>
#include <gvm_thread_utils.h>
#include <log.h>
#include <HttpStream.hpp>

#include <scopeguard.hpp>
#include <vpl_conv.h>
#include <vplex_http_util.hpp>
#include <vplu_format.h>

// TODO for 2.7, Bug 13079:
// see if we can lower the stack requirement 16KB (UTIL_DEFAULT_THREAD_STACK_SIZE) for non-Android.
static const size_t helperThread_StackSize = 
#ifdef ANDROID
    UTIL_DEFAULT_THREAD_STACK_SIZE
#else
    128 * 1024
#endif
    ;

#if defined(WIN32) || defined(CLOUDNODE) || defined(LINUX)
static const size_t stream_ReadBufSize = 1024 * 1024;  // 1MB
static const size_t ts_ReadBufSize     = 1024 * 1024;  // 1MB
#else
static const size_t stream_ReadBufSize = 64 * 1024;  // 64KB
static const size_t ts_ReadBufSize     = 64 * 1024;  // 64KB
#endif

// Sequence diagram describing the behavior of HsToTsAdapter obj.
// http://www.ctbg.acer.com/wiki/index.php/File:Httpsvc_hs2ts_seq.png

HttpSvc::HsToTsAdapter::HsToTsAdapter(HttpStream *hs)
    : hs(hs), runCalled(false), bailout(0), sendDone(false)
{
    LOG_TRACE("HsToTsAdapter[%p]: Created for HttpStream[%p]", this, hs);
}

HttpSvc::HsToTsAdapter::~HsToTsAdapter()
{
    LOG_TRACE("HsToTsAdapter[%p]: Destroyed", this);
}

// static
const std::string HttpSvc::HsToTsAdapter::httpHeaderBodyBoundary = "\r\n\r\n";  // CR LF CR LF

int HttpSvc::HsToTsAdapter::Run()
{
    if (runCalled) {
        LOG_ERROR("HsToTsAdapter[%p]: Run called more than once", this);
        return -1;  // FIXME
    }
    runCalled = true;

    int err = 0;

    // The calling thread will handle data transfer from HttpStream obj to TS layer.
    // The helper thread will handle data transfer from TS layer to HttpStream obj.

    bool tsOpened = false;
    VPLDetachableThreadHandle_t helperThread;
    bool threadSpawned = false;

    {
        TSOpenParms_t params;
        params.user_id = hs->GetUserId();
        params.device_id = hs->GetDeviceId();
        params.instance_id = 0;  // FIXME: this is good enough for 2.7
        {
            std::vector<std::string> uri_parts;
            VPLHttp_SplitUri(hs->GetUri(), uri_parts);
            if (uri_parts.size() < 1) {
                LOG_ERROR("HsToTsAdapter[%p]: Malformed URI %s", this, hs->GetUri().c_str());
                Utils::SetCompleteResponse(hs, 400);
                goto end;
            }
            params.service_name.assign(uri_parts[0]);
        }
        params.flags = 0;
        params.timeout = VPLTime_FromSec(30);
        std::string errmsg;
        TSError_t tserr = TS_Open(params, ts, errmsg);
        if (tserr != TS_OK) {
            LOG_ERROR("HsToTsAdapter[%p]: TS_Open failed: err %d msg %s", this, tserr, errmsg.c_str());
            Utils::SetCompleteResponse(hs, 500);
            bailout = -tserr;
            goto end;
        }
        tsOpened = true;
    }

    {
        bool firsttime = true;
        char *buf = NULL;
        char *data = NULL;
        size_t datasize = 0;
        TSError_t tserr = TS_OK;
        std::string tserrmsg;

        buf = new (std::nothrow) char[stream_ReadBufSize];
        if (!buf) {
            LOG_ERROR("HsToTsAdapter[%p]: No memory for buffer", this);
            Utils::SetCompleteResponse(hs, 500);
            bailout = -CCD_ERROR_NOMEM;
            err = 0;  // reset error
            goto end;
        }
        ON_BLOCK_EXIT(deleteArray<char>, buf);

        err = Util_SpawnThread(helperThreadMain, this, helperThread_StackSize, /*isJoinable*/VPL_TRUE, &helperThread);
        if (err) {
            LOG_ERROR("HsToTsAdapter[%p]: Failed to spawn helper thread: err %d", this, err);
            Utils::SetCompleteResponse(hs, 500);
            bailout = -err;
            err = 0;  // reset error
            goto end;
        }
        threadSpawned = true;
        // After the helper thread is spawned, helper thread is the only thread allowed to set the response.

        while (1) {
            ssize_t bytes = hs->Read(buf, stream_ReadBufSize - 1);  // sub 1 for EOL
            if (bytes < 0) {  // error
                LOG_ERROR("HsToTsAdapter[%p]: Failed to read from Stream: err "FMT_ssize_t, this, bytes);
                bailout = (int)bytes;
                err = (int)bytes;
                goto failed_to_read_from_stream;
            }
            if (bytes == 0) {  // eof
                break;
            }

            // strm_http has a bug where it cannot correctly handle 
            // receiving a block that contains both the header and the body (whether whole or part).
            // Thus, we will make sure to split it up into two.
            // This section of code can be removed when backward compatibility with 2.5.x server
            // is no longer needed.
            if (firsttime) {
                buf[bytes] = '\0';
                char *boundary = strstr(buf, httpHeaderBodyBoundary.c_str());
                if (boundary) {
                    tserr = TS_Write(ts, buf, boundary - buf + httpHeaderBodyBoundary.size(), tserrmsg);
                    if (tserr != TS_OK) {
                        LOG_ERROR("HsToTsAdapter[%p]: TS_Write failed: err %d, msg %s", this, tserr, tserrmsg.c_str());
                        bailout = tserr;
                        err = tserr;
                        goto end;
                    }
                    data = boundary + httpHeaderBodyBoundary.size();
                    datasize = bytes - (boundary - buf + httpHeaderBodyBoundary.size());
                }
                else {
                    data = buf;
                    datasize = bytes;
                } 
                firsttime = false;
            }
            else {
                data = buf;
                datasize = bytes;
            }

            if (datasize > 0) {
                tserr = TS_Write(ts, data, datasize, tserrmsg);
                if (tserr != TS_OK) {
                    LOG_ERROR("HsToTsAdapter[%p]: TS_Write failed: err %d, msg %s", this, tserr, tserrmsg.c_str());
                    bailout = tserr;
                    err = tserr;
                    goto end;
                }
            }
        }
    }

 failed_to_read_from_stream:
    if (err == VPL_ERR_CANCELED && tsOpened) {
        std::string errmsg;
        TSError_t tserr = TS_Close(ts, errmsg);
        if (tserr != TS_OK) {
            LOG_ERROR("HsToTsAdapter[%p]: TS_Close failed: err %d, msg %s", this, tserr, errmsg.c_str());
            if (!err) err = tserr;
        }
        tsOpened = false;
    }

 end:
    sendDone = true;

    if (threadSpawned) {
        // wait for helper thread to exit
        int err2 = VPLDetachableThread_Join(&helperThread);
        if (err2) {
            LOG_ERROR("HsToTsAdapter[%p]: Failed to join helper thread: err %d", this, err2);
            if (!err) err = err2;
        }
        threadSpawned = false;
    }

    if (tsOpened) {
        std::string errmsg;
        TSError_t tserr = TS_Close(ts, errmsg);
        if (tserr != TS_OK) {
            LOG_ERROR("HsToTsAdapter[%p]: TS_Close failed: err %d, msg %s", this, tserr, errmsg.c_str());
            if (!err) err = tserr;
        }
        tsOpened = false;
    }

    if (!err && (bailout < 0)) {
        err = bailout;
    }

    return err;
}

VPLTHREAD_FN_DECL HttpSvc::HsToTsAdapter::helperThreadMain(void *param)
{
    HttpSvc::HsToTsAdapter *adapter = static_cast<HttpSvc::HsToTsAdapter*>(param);
    adapter->helperThreadMain();
    return VPLTHREAD_RETURN_VALUE;
}

void HttpSvc::HsToTsAdapter::helperThreadMain()
{
    // The code below assumes that Content-Length is returned in the first call to TS_Read().
    // TODO: support chunked encoding (bug 13078)

    if (bailout) return;

    char *buf = new (std::nothrow) char[ts_ReadBufSize];
    if (!buf) {
        LOG_ERROR("HsToTsAdapter[%p]: No memory for buffer", this);
        Utils::SetCompleteResponse(hs, 500);
        bailout = -CCD_ERROR_NOMEM;
        return;
    }
    ON_BLOCK_EXIT(deleteArray<char>, buf);

    size_t bufsize = ts_ReadBufSize;
    std::string errmsg;
    TSError_t tserr = TS_OK;
    
    do {
        tserr = TS_Read(ts, buf, bufsize, errmsg);
        if ((tserr == TS_ERR_TIMEOUT) && !sendDone) {
            // bug 15981: retry reading when nothing to read and times out
            LOG_DEBUG("HsToTsAdapter[%p]: TS_Read timeout: err %d, msg %s", this, tserr, errmsg.c_str());
        }
        else if (tserr != TS_OK) {
            LOG_ERROR("HsToTsAdapter[%p]: TS_Read failed: err %d, msg %s", this, tserr, errmsg.c_str());
            bailout = tserr;
            return;
        }
        else {
            break;
        }
    } while(1);
    
    if (bailout) return;

    int err = completeWrite(buf, bufsize);
    if (err) {
        if (err == VPL_ERR_CANCELED) {
            LOG_INFO("HsToTsAdapter[%p]: Bailout detected", this);
        }
        else {
            LOG_ERROR("HsToTsAdapter[%p]: Failed to write to stream: err %d", this, err);
            bailout = err;
        }
        return;
    }

    u64 bodysize = 0;
    if (hs->GetMethod() != "HEAD") {
        std::string contentLength;
        int err = hs->GetRespHeader("Content-Length", contentLength);
        if (err == CCD_ERROR_NOT_FOUND) {
            LOG_WARN("HsToTsAdapter[%p]: Content-Length not found in first "FMT_size_t" bytes", this, bufsize);
            bodysize = 0;
        }
        else if (err) {
            bailout = err;
            return;
        }
        else {
            bodysize = VPLConv_strToU64(contentLength.c_str(), NULL, 10);
        }
    }

    if (bodysize == 0) {
        hs->Flush();
        return;
    }

    while (hs->GetRespBodyBytesWritten() < bodysize) {
        if (bailout) return;

        bufsize = ts_ReadBufSize;
        if (bufsize > bodysize - hs->GetRespBodyBytesWritten())
            bufsize = static_cast<size_t>(bodysize - hs->GetRespBodyBytesWritten());  // this case is safe, as the value is less than bufsize, which is size_t
        do {
            tserr = TS_Read(ts, buf, bufsize, errmsg);
            if ((tserr == TS_ERR_TIMEOUT) && !sendDone) {
                // bug 15981: retry reading when nothing to read and times out
                LOG_DEBUG("HsToTsAdapter[%p]: TS_Read timeout: err %d, msg %s", this, tserr, errmsg.c_str());
            }
            else if (tserr != TS_OK) {
                LOG_ERROR("HsToTsAdapter[%p]: TS_Read failed: err %d, msg %s", this, tserr, errmsg.c_str());
                LOG_INFO("HsToTsAdapter[%p]: Bailing out after "FMTu64"/"FMTu64" of body", this, hs->GetRespBodyBytesWritten(), bodysize);
                bailout = tserr;
                return;
            }
            else {
                break;
            }
        } while(1);

        if (bailout) return;

        int err = completeWrite(buf, bufsize);
        if (err) {
            if (err == VPL_ERR_CANCELED) {
                LOG_INFO("HsToTsAdapter[%p]: Bailout detected", this);
            }
            else {
                LOG_ERROR("HsToTsAdapter[%p]: Failed to write to stream: err %d", this, err);
                bailout = err;
            }
            return;
        }
    }
}

int HttpSvc::HsToTsAdapter::completeWrite(const char *data, size_t datasize)
{
    size_t totalBytesWrote = 0;
    while (totalBytesWrote < datasize) {
        if (bailout) return VPL_ERR_CANCELED;

        ssize_t bytesWrote = hs->Write(data + totalBytesWrote, datasize - totalBytesWrote);
        if (bytesWrote < 0) {
            return bytesWrote;
        }
        totalBytesWrote += bytesWrote;
    }
    return 0;
}

