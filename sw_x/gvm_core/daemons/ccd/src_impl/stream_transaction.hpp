#ifndef __STREAM_TRANSACTION_HPP__
#define __STREAM_TRANSACTION_HPP__

#include <http_request.hpp>
#include <http_response.hpp>
#include <vpl_th.h>
#include <vpl_time.h>
#include <vplex_file.h>
#include <vplex_http2.hpp>
#include <string>

#include "AsyncUploadQueue.hpp"

class stream_client;
class stream_service;
class HttpStream;

class stream_transaction {
    public:
    // Transaction origin client
    stream_client* origin;

    stream_service *service;

    HttpStream *origHs; //The original http reqest from App 
    bool isSetOrigHsStatusCode;

    // states of a transaction
    bool receiving; // True when receiving any data. False when receive done.
    bool processing; // True when processing request, even if still receiving.
    bool sending; // True when response to send has been set.
    bool failed; // True when the transaction has failed to complete.
    bool populated; // True when this request triggered population of the connection pool.

    http_request* req;
    http_response resp;

    // Timestamp of transaction creation.
    VPLTime_t start;

    // Bytes received/sent for transaction
    u64 recv_cnt;
    u64 send_cnt;

    u64 id;
    u64 uid;
    u64 deviceid;
    u64 sent_so_far;
    u64 size;
    std::string request_content;
    UploadStatusType upload_status;

    // When set, transaction satisfied from local cache.
    VPLFile_handle_t cache_file;
    VPLFile_offset_t read_offset;
    VPLFile_offset_t read_length;
    VPLFile_offset_t read_end;

    VPLMutex_t mutex;
    VPLCond_t cond_resp;  // signaled when stream_client has read from this->resp.content.

    // BEGIN TEMPORARY CODE: DO NOT DEPEND ON THIS CODE TO BE HERE FOR LONG
    // temporarily added to support SyncBack
    VPLFile_handle_t out_content_file;  // if valid, write response body to this file
    // END TEMPORARY CODE

    // reference counter - defer destruction until it drops to 0
    int refcount;

    VPLHttp2 *http2;

    stream_transaction();
    stream_transaction(stream_client* origin);
    ~stream_transaction();

    int lock();
    int unlock();
};

#endif
