#include "stream_transaction.hpp"

#include "ccd_features.h"
#include "config.h"
#include <vpl_th.h>
#include <vpl_time.h>
#include <vplex_file.h>

stream_transaction::stream_transaction() :
    origin(NULL), service(NULL),origHs(NULL),isSetOrigHsStatusCode(false),
    receiving(true), processing(false), sending(false), failed(false), populated(false), req(NULL),
    recv_cnt(0), send_cnt(0), id(0), uid(0), sent_so_far(0), size(0), upload_status(pending),
    cache_file(VPLFILE_INVALID_HANDLE),
    out_content_file(VPLFILE_INVALID_HANDLE),
    refcount(1),  // 1 for requesting thread (which should be stream_service)
    http2(NULL)
{
    start = VPLTime_GetTimeStamp();
    VPLMutex_Init(&mutex);
    VPLCond_Init(&cond_resp);
}

stream_transaction::stream_transaction(stream_client* origin) :
    origin(origin),
    service(NULL),origHs(NULL),isSetOrigHsStatusCode(false),
    receiving(true), processing(false), sending(false), failed(false), populated(false), req(NULL),
    recv_cnt(0), send_cnt(0), id(0), uid(0), sent_so_far(0), size(0), upload_status(pending),
    cache_file(VPLFILE_INVALID_HANDLE),
    out_content_file(VPLFILE_INVALID_HANDLE),
    refcount(1),  // 1 for requesting thread (which should be stream_service)
    http2(NULL)
{
    start = VPLTime_GetTimeStamp();
    VPLMutex_Init(&mutex);
    VPLCond_Init(&cond_resp);
    req = new http_request();
}

stream_transaction::~stream_transaction() 
{
    if(cache_file != VPLFILE_INVALID_HANDLE) {
        VPLFile_Close(cache_file);
    }
    if(req != NULL) {
        delete req;
        req = NULL;
    }
    if (http2) {
        delete http2;
        http2 = NULL;
    }
    VPLMutex_Destroy(&mutex);
    VPLCond_Destroy(&cond_resp);
}

int stream_transaction::lock()
{
    return VPLMutex_Lock(&mutex);
}

int stream_transaction::unlock()
{
    return VPLMutex_Unlock(&mutex);
}

