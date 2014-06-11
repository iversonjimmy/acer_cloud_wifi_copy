/*
 *               Copyright (C) 2011, iGware Inc.
 *
 *  These coded instructions, statements, and computer programs contain
 *  unpublished  proprietary information of BroadOn Communications Corp.,
 *  and  are protected by Federal copyright law. They may not be disclosed
 *  to  third  parties or copied or duplicated in any form, in whole or in
 *  part, without the prior written consent of BroadOn Communications Corp.
 */

// Put this first to fix build problems...
#include "cache.h"
#include "vplex_file.h"

#include <ccdi_rpc.pb.h>
#include <ccdi.hpp>
#include "gvm_rm_utils.hpp"
#include "sw_update.hpp"
#include "virtual_device.hpp"
#include "query.h"
#include "nus_query.hpp"
#include "ccd_storage.hpp"
#include "vpl_fs.h"
#include "vpl_th.h"
#include "vplex_http2.hpp"
#include "tmdviewer.h"
#include "escore.h"
#include "EventManagerPb.hpp"

#include <map>
#include <sstream>
#include <algorithm>
#include <iomanip>

#define ES_COOKIE	0xdeadbeef
#define ES_CERT_MAX 5

#define SWU_CONTENT_INDEX       0

#define SWU_EVENT_DLY           2   // wait 2 seconds between poll events

using namespace std;

#define MINS_TO_SECS(m)     ((m) * 60)
#define HOURS_TO_SECS(h)    MINS_TO_SECS((h) * 60)
#define NUS_CACHE_TIMEOUT   HOURS_TO_SECS(1)

#define _MIN(a,b)   (((a) <= (b)) ? (a) : (b))
#define _HTTP_CHUNK_SIZE    (256 * 1024)
#define _MAX_CHUNK_RETRY    3
#ifdef _MSC_VER
// We'll try twice to download the full content
#define _MAX_CONTENT_RETRY  2
#else // ! _MSC_VER
// We try once to download the full content because we retry the individual
// chunks making up the content.
#define _MAX_CONTENT_RETRY  1
#endif // ! _MSC_VER

// When restarting previously stopped fetches we need to back up a bit
// because we've found that error messages can be appended to the returned
// data. So, back up a good amount
#define FETCH_BOUNDARY      (32*1024)

// This class abstracts out some of the interactions with NUS.
struct swu_state_s {
    std::string app_guid;
    std::string app_version;
    std::string app_min_version;
    std::string ccd_min_version;
    std::string app_message;
    std::string title_id_str;
    u64         title_id;
    s32         version;
    s64         title_size;
    s32         ticket_size;
    s32         tmd_size;
    u64         fs_size;
    u32         content_id;
};
typedef struct swu_state_s swu_state_t;

// We'll map it by title id for now?
typedef map<std::string, swu_state_t> SWUStateMap;

class SWU_Download {
public:
    SWU_Download();
    ~SWU_Download();

    int init(const std::string& content_prefix_url, swu_state_t& state);
    
    int begin(u64& handle);
    int progress_check(void);
    int stop(void);
    int clean_up(void);
    void do_download(void);
    int decrypt(const std::string& out_file);
    void cancel(void);

    bool is_done(void) {return _is_done;}
    bool is_canceled(void) {return _is_canceled;}
    bool is_in_progress(void) {return _is_in_progress;}
    bool is_failed(void) {return _is_failed;}

    int get_progress(u64& tot_xfer_size, u64& tot_xferred, ccd::SWUpdateDownloadState_t& state);

    size_t write_callback(const void *buf, size_t size, size_t nmemb);
    const std::string& get_version(void) {return _state.app_version;}
    void worker_join(void);
    void _send_event(void);

private:
    int _get_tmd(const std::string& account_id);
    int _get_ticket(const std::string& account_id);
    int _get_content_id(void);
    int _get_content(u32 contentId);
    int _get_content_chunk(u32 contentId, u64 chunk_offset, u64 chunk_length);
    int _decrypt_content(u32 tmd_handle, const std::string& out_file);

    u64             _handle;
    u64             _content_size;
    u64             _tot_xfer_size;
    u64             _tot_xferred;
    u64             _chunk_offset;
    u64             _chunk_xferred;
    u64             _chunk_length;

    VPLTime_t       _last_event_time;

    bool            _is_init;

    std::string     _content_prefix_url;
    swu_state_t     _state;
    std::string     _download_path;
    std::string     _tmd;
    std::string     _ticket;
    vector<std::string> _ticket_certs;
    VPLFile_handle_t _download_fh;

    VPLThread_t     _worker;

    bool            _has_tmd;
    bool            _has_ticket;
    bool            _is_in_progress;
    bool            _is_canceled;
    bool            _is_done;
    bool            _is_failed;
    bool            _start_cancel;
    bool            _has_worker;
    bool            _download_fh_is_open;
};

typedef map <u64, SWU_Download*> SWUDownloadTitleMap;
typedef map <std::string, SWU_Download*> SWUDownloadGuidMap;

class SWU_Nus {
public:
    SWU_Nus();
    ~SWU_Nus() {};

    int init(void);
    int check_sw_update(const std::string& app_guid, const std::string& app_version, bool update_cache, u64& update_mask, u64& app_size, std::string& latest_app_version, std::string& change_log, std::string& latest_ccd_version, bool& isAutoUpdateDisabled, bool& isQA, bool& isInfraDownload, const char* userGroup);
    int set_ccd_version(const std::string& guid, const std::string& version);
    int guid_lookup(const std::string& guid, const std::string& version, swu_state_t& state);
    std::string& get_content_prefix_url(void) {return _content_prefix_url;}
    bool ccd_version_is_set(void) { return _ccd_version_is_set; }

private:
    int _get_sw_state(const char* userGroup);
    void _clear_stale_updates(void);

    SWUStateMap     _swu_states;
    std::string     _ccd_guid;
    std::string     _ccd_version;
    std::string     _ccd_version_latest;
    std::string     _content_prefix_url;
    time_t          _fetch_time;

    bool            _is_init;
    bool            _ccd_version_is_set;
    bool            _is_qa;
    bool            _is_auto_update_disabled;
    bool            _is_infra_download;
};

static SWU_Nus _swu_nus;
static VPLMutex_t _swu_lock;
SWUDownloadTitleMap _swu_downloads_handle;
SWUDownloadGuidMap _swu_downloads_guid;
#define _DEFAULT_DEV_ID 0x600000000ULL
static u64 _device_id = _DEFAULT_DEV_ID;
static bool _shutdown_now = false;
static std::string _update_path;
static std::string _to_delete_path;
static time_t _swu_start;

SWU_Download::SWU_Download()
{
    _is_init = false;
    _is_in_progress = false;
    _is_canceled = false;
    _start_cancel = false;
    _is_failed = false;
    _is_done = false;
    _has_tmd = false;
    _has_ticket = false;
    _has_worker = false;
    _download_fh_is_open = false;
    _tot_xferred = 0;
    _chunk_offset = 0;
    _chunk_length = 0;
    _chunk_xferred = 0;
}

SWU_Download::~SWU_Download()
{
    VPLThread_return_t retvalOut;
    SWUDownloadTitleMap::iterator t_it;
    SWUDownloadGuidMap::iterator g_it;

    if ( !_is_init ) {
        return;
    }

    // remove from lists
    t_it = _swu_downloads_handle.find(_handle);
    if ( t_it != _swu_downloads_handle.end() ) {
        _swu_downloads_handle.erase(t_it);
    }
    g_it = _swu_downloads_guid.find(_state.app_guid + _state.app_version);
    if ( g_it != _swu_downloads_guid.end() ) {
        _swu_downloads_guid.erase(g_it);
    }

    // Need to wait for this thing to finish...
    if ( _has_worker ) {
        VPLThread_Join(&_worker, &retvalOut);
    }

    if ( _download_fh_is_open ) {
        VPLFile_Close(_download_fh);
    }
}

void SWU_Download::worker_join(void)
{
    VPLThread_return_t retvalOut;

    // Need to wait for this thing to finish...
    if ( _has_worker ) {
        VPLThread_Join(&_worker, &retvalOut);
        _has_worker = false;
    }
}

int SWU_Download::get_progress(u64& tot_xfer_size, u64& tot_xferred,
    ccd::SWUpdateDownloadState_t& state)
{
    int rv;

    if ( !_is_init ) {
        rv = SWU_ERR_NOT_INIT;
        goto done;
    }

    tot_xfer_size = _tot_xfer_size;
    tot_xferred = _tot_xferred;

    if ( _is_in_progress ) {
        state = ccd::SWU_DLSTATE_IN_PROGRESS;
    }
    else if ( _is_canceled ) {
        state = ccd::SWU_DLSTATE_CANCELED;
    }
    else if ( _is_done ) {
        state = ccd::SWU_DLSTATE_DONE;
    }
    else if ( _is_failed ) {
        state = ccd::SWU_DLSTATE_FAILED;
    }
    else {
        state = ccd::SWU_DLSTATE_STOPPED;
    }

    rv = 0;
done:
    return rv;
}

static int _get_account_and_user_id(std::string& account_id, u64 &userId)
{
    int rv;
    ccd::GetSystemStateInput ccdiRequest;
    ccdiRequest.set_get_players(true);
    ccd::GetSystemStateOutput ccdiResponse;

    rv = CCDIGetSystemState(ccdiRequest, ccdiResponse);
    if (rv != 0) {
        LOG_ERROR("%s failed: %d", "CCDIGetSystemState", rv);
        goto done;
    }
    if (ccdiResponse.players().players_size() < 1) {
        LOG_ERROR("Unexpected response");
        rv = SWU_ERR_FAIL;
        goto done;
    }

    account_id = ccdiResponse.players().players(0).account_id();
    userId = ccdiResponse.players().players(0).user_id();

    rv = 0;
done:
    return rv;
}

int SWU_Download::_get_tmd(const std::string& account_id)
{
    int rv = 0;
    vplex::nus::TitleVersionType title;
    vplex::nus::GetSystemTMDResponseType res;
    vplex::nus::GetSystemTMDRequestType req;

    Query_SetNusAbstractRequestFields(req);
    req.mutable__inherited()->set_deviceid(_device_id);
    if ( account_id.size() ) {
        req.mutable__inherited()->set_accountid(account_id);
    }

    title.set_titleid(_state.title_id_str);
    title.set_version(_state.version);
    title.set_fssize(_state.title_size);
    title.set_ticketsize(_state.ticket_size);
    title.set_tmdsize(_state.tmd_size);

    req.add_titleversion()->CopyFrom(title);

    rv = QUERY_NUS(VPLNus_GetSystemTMD, req, res);
    if (rv != CCD_OK) {
        goto done;
    }
    if (res.tmd_size() != 1) {  // only 1 TMD expected
        LOG_ERROR("SWU: Unexpected number of TMD returned: %d", res.tmd_size());
        rv = CCD_ERROR_BAD_TMD;
        goto done;
    }

    _tmd = res.tmd(0);
    _has_tmd = true;

    rv = 0;
done:
    return rv;
}

int SWU_Download::_get_ticket(const std::string& account_id)
{
    int rv = 0;
    vplex::nus::GetSystemCommonETicketResponseType res;
    vplex::nus::GetSystemCommonETicketRequestType req;
    Query_SetNusAbstractRequestFields(req);

    req.mutable__inherited()->set_deviceid(_device_id);
    if ( account_id.size() ) {
        req.mutable__inherited()->set_accountid(account_id);
    }
    req.add_titleid(_state.title_id_str);

    rv = QUERY_NUS(VPLNus_GetSystemCommonETicket, req, res);
    if (rv != CCD_OK) {
        goto done;
    }
    if (res.commoneticket_size() != 1) {  // only 1 ticket expected
        LOG_ERROR("SWU: Unexpected number of ETickets returned: %d",
            res.commoneticket_size());
        goto done;
    }

    _ticket = res.commoneticket(0);
    for (int i = 0; i < res.certs_size(); i++) {
        _ticket_certs.push_back(res.certs(i));
    }
    _has_ticket = true;

    rv = 0;
done:
    return rv;
}

int SWU_Download::init(const std::string& content_prefix_url,
    swu_state_t& state)
{
    int rv;
    std::string account_id;
    u64 userId = 0;

    if ( _is_init ) {
        rv = SWU_ERR_ALREADY_INIT;
        goto done;
    }

    // set the account id. This is optional so if we fail obtaining one
    // perform the query without it.
    if ( (rv = _get_account_and_user_id(account_id, userId)) ) {
        LOG_ERROR("Failed pulling out the account id - %d", rv);
    }

    _content_prefix_url = content_prefix_url;
    _state = state;
    _handle = _state.title_id ^ (u64)_swu_start ^ (u64)_state.version;
    _last_event_time = 0;

    {
        std::ostringstream osPath; 
        osPath << _update_path << std::hex << _state.title_id << "_" << std::hex << std::setw(8) << std::setfill('0') << _state.version; 
        _download_path = osPath.str();
    }

    {
        VPLFS_stat_t stat;

        rv = VPLFS_Stat(_download_path.c_str(), &stat);
        if ( rv == VPL_OK ) {
            // remove anything that's in our way
            LOG_INFO("Removing thing found at %s", _download_path.c_str());
            if ( (rv = Util_rmRecursive(_download_path, _to_delete_path)) ) {
                LOG_ERROR("SWU: Failed removing file %s - %d",
                    _download_path.c_str(), rv);
                rv = SWU_ERR_FAIL;
                goto done;
            }
        }
        else if ( (rv != VPL_ERR_NOENT) ) {
            LOG_ERROR("SWU: Failed stating download file - %s - %d",
                _download_path.c_str(), rv);
            goto done;
        }
    }

    // We always pull down the TMD and tickets because it's not easy
    // to tell if what we have downloaded is good. TMD maybe. But the
    // tickets are messy because of the certs. They're small so for now
    // we always pull them down if a restart is needed.

    // Always pull down the TMD
    if ( (rv = _get_tmd(account_id)) ) {
        LOG_ERROR("SWU: Failed _get_tmd() - %d", rv);
        goto done;
    }

    // pull out the content id and title size
    rv = _get_content_id();
    if (rv < 0) {
        LOG_ERROR("SWU: Failed to determine contentId: %d", rv);
        goto done;
    }

    // Always pull down the Ticket
    if ( (rv = _get_ticket(account_id)) ) {
        LOG_ERROR("SWU: Failed _get_ticket() - %d", rv);
        goto done;
    }

    // We've now got everything we need to get this thing going.
    _content_size = _tot_xfer_size = _state.title_size;
    // figure in encryption overhead
    _tot_xfer_size += 0xf;
    _tot_xfer_size &= ~0xfUL;
    if ( _tot_xfer_size == 0 ) {
        LOG_ERROR("SWU: Content size is zero bytes!");
        rv = SWU_ERR_FAIL;
        goto done;
    }

    _swu_downloads_guid[_state.app_guid + _state.app_version] = this;

    _is_init = true;

    rv = 0;
done:
    return rv;
}

static void _download_helper(SWU_Download *ctxt)
{
    ctxt->do_download();
}

int SWU_Download::begin(u64& handle)
{
    int rv;

    if ( !_is_init ) {
        rv = SWU_ERR_NOT_INIT;
        goto done;
    }

    handle = _handle;

    if ( _is_in_progress || _is_done || _is_canceled ) {
        // Force an event to be sent to make sure
        // the app receives the event for done or canceled.
        // otherwise it might hang waiting for such an event
        _send_event();
        rv = 0;
        goto done;
    }

    _is_in_progress = true;

    // This will require a thread to continue at this point.
    if ( (rv = VPLThread_Create(&_worker, (VPLThread_fn_t)_download_helper,
            this, 0, "SWU_Download")) ) {
        _is_in_progress = false;
        LOG_ERROR("SWU: Failed starting download thread");
        goto done;
    }

    _swu_downloads_handle[handle] = this;

    _has_worker = true;

    rv = 0;
done:
    return rv;
}

int SWU_Download::_get_content_id(void)
{
    int rv = 0;

    ESContentIndex contentIndex = SWU_CONTENT_INDEX;  // we assume it's always content index 0
    ESContentInfo contentInfo;
    u32 numContentInfos = 1;

    rv = tmdv_findContentInfos(_tmd.c_str(), _tmd.size(), &contentIndex, 1,
        &contentInfo, &numContentInfos);
    if (rv < 0) {
        LOG_ERROR("SWU: Failed to parse tmd: %d", rv);
        goto end;
    }
    if (numContentInfos != 1) {
        LOG_ERROR("SWU: Failed to parse tmd");
        rv = CCD_ERROR_BAD_TMD;
        goto end;
    }

    // pull out the title size
    _state.content_id = contentInfo.id;
    _state.title_size = contentInfo.size;

 end:
    return rv;
}

void SWU_Download::_send_event(void)
{
    int rv;
    ccd::CcdiEvent *event;
    ccd::EventSWUpdateProgress *prog;
    VPLTime_t cur_time;
    u64 tot_xfer, tot_xferred;
    ccd::SWUpdateDownloadState_t state;

    // We get the progress first since we break the time check rule
    // when we reach the end of the transfer
    if ( (rv = get_progress(tot_xfer, tot_xferred, state)) ) {
        LOG_ERROR("SWU: Failed getting progress");
        goto done;
    }

    // Is it time for an update? Send at most one event every N seconds.
    cur_time = VPLTime_GetTime();
    if ( (state == ccd::SWU_DLSTATE_IN_PROGRESS) &&
            (cur_time - _last_event_time) < VPLTime_FromSec(SWU_EVENT_DLY) ) {
        goto done;
    }

    // Allocate an event
    event = new ccd::CcdiEvent();
    if ( event == NULL ) {
        LOG_ERROR("SWU: Failed allocating an event for progress reporting.");
        goto done;
    }

    // get the current state
    prog = event->mutable_sw_update_progress();
    prog->set_handle(_handle);
    prog->set_total_transfer_size(tot_xfer);
    prog->set_bytes_transferred_cnt(tot_xferred);
    prog->set_state(state);

    // fire it off
    EventManagerPb_AddEvent(event);
    _last_event_time = cur_time;

done:
    return;
}

size_t SWU_Download::write_callback(const void *buf, size_t size, size_t nmemb)
{
    size_t rv;
    size_t buf_size;
    u64 file_offset;

    // Returning zero will cancel out any further fetches.
    if ( _start_cancel ) {
        LOG_INFO("SWU: cancel handle "FMTu64, _handle);
        rv = 0;
        goto done;
    }

    buf_size = size * nmemb;
    LOG_TRACE("Received "FMTu64" bytes", (u64)buf_size);

    if ( (_chunk_xferred + buf_size) > _chunk_length ) {
        LOG_ERROR("Received too much data from server "FMTu64":"FMTu64":"FMTu64,
            _chunk_xferred, (u64)buf_size, _chunk_length);
        rv = 0;
        goto done;
    }
    file_offset = _chunk_offset;

    if ( (rv = VPLFile_WriteAt(_download_fh, buf, buf_size, file_offset)
            != buf_size) ) {
        LOG_INFO("SWU: write of data failed! - "FMTu_size_t, rv);
        rv = 0;
        goto done;
    }

    // update the download count
    _chunk_offset += buf_size;
    _chunk_xferred += buf_size;
    rv = nmemb;

    _send_event();

done:
    return rv;
}

static size_t _write_callback(const void *buf, size_t size, size_t nmemb,
    void *param)
{
    SWU_Download *download = (SWU_Download *)param;

    // If we're shutting down, kill any active fetches.
    if ( _shutdown_now ) {
        return 0;
    }

    return download->write_callback(buf, size, nmemb);
}

static s32 _swu_write_callback(VPLHttp2 *http, void *ctx, const char *buf, u32 size)
{
    return _write_callback(buf, size, 1, ctx) * size;
}

static void _swu_progress_callback(VPLHttp2 *http, void *ctx, u64 total, u64 sofar)
{
    LOG_DEBUG("SWU GET Progress CB: http("FMT0xPTR"), ctx("FMT0xPTR"), total("FMTu64"), sofar("FMTu64")", http, ctx, total, sofar);
    return ;
}


int SWU_Download::_get_content_chunk(u32 contentId, u64 chunk_offset,
    u64 chunk_length)
{
    int rv = 0;
    VPLHttp2 h;

    _chunk_offset = chunk_offset;
    _chunk_length = chunk_length;
    _chunk_xferred = 0;

    {
        
        std:: ostringstream ossurl;
        // Unlike the other manipulators, setw() only affects the very next output.
        ossurl << _content_prefix_url << "/" << _state.title_id_str << "/" <<  std::hex << std::setw(8) << std::setfill('0') << std::uppercase << contentId
               << std::dec << "?offset=" << _chunk_offset << "&chunksize=" << _chunk_length; 

        LOG_INFO("url %s", ossurl.str().c_str());

        int httpResponse = -1;
        h.SetUri(ossurl.str());
        h.SetTimeout(VPLTime_FromSec(30));
        rv = h.Get(_swu_write_callback, this, _swu_progress_callback, NULL);

        if (rv < 0) {
            LOG_ERROR("SWU: Failed http request: %d", rv);
            goto end;
        }

        httpResponse = h.GetStatusCode();
    }

    rv = 0;
 end:

    return rv;
}

int SWU_Download::_get_content(u32 contentId)
{
    int rv = 0;

    _download_fh = VPLFile_Open(_download_path.c_str(),
        VPLFILE_OPENFLAG_CREATE | VPLFILE_OPENFLAG_WRITEONLY, 
        VPLFILE_MODE_IRUSR | VPLFILE_MODE_IWUSR);
    if ( !VPLFile_IsValidHandle(_download_fh) ) {
        LOG_INFO("SWU: Failed open of %s", _download_path.c_str());
        rv = SWU_ERR_FAIL;
        goto end;
    }
    _download_fh_is_open = true;

    // break up the fetch into multiple requests
    _tot_xferred = 0;
    while (_tot_xferred != _tot_xfer_size) {
        u64 xfer_size;

        xfer_size = _MIN(_tot_xfer_size - _tot_xferred, _HTTP_CHUNK_SIZE);
        for( int i = 0 ; i < _MAX_CHUNK_RETRY ; i++ ) {
            if ( i ) {
                LOG_INFO("SWU: handle "FMTx64" retry %d @ "FMTx64,
                    _handle, i, _tot_xferred);
            }
            rv = _get_content_chunk(contentId, _tot_xferred, xfer_size);
            if ( _start_cancel || _shutdown_now) {
                LOG_INFO("SWU: Cancel handle "FMTx64, _handle);
                rv = SWU_ERR_CANCELED;
                goto end;
            }
            if ( rv == 0 ) {
                break;
            }
            LOG_ERROR("SWU: Failed fetch of chunk "FMTx64":"FMTx64":"FMTx64
                " - %d", _tot_xferred, xfer_size, _chunk_xferred, rv);
        }
        if ( rv || (_chunk_xferred != xfer_size) ) {
            rv = SWU_ERR_FAIL;
            goto end;
        }

        _tot_xferred += xfer_size;
    }

 end:
    if ( _download_fh_is_open ) {
        VPLFile_Close(_download_fh);
        _download_fh_is_open = false;
    }

    return rv;
}

void SWU_Download::do_download(void)
{
    int rv;

    LOG_INFO("_download_path = %s", _download_path.c_str());
    for( int i = 0 ; i < _MAX_CONTENT_RETRY ; i++ ) {
        if ( i ) {
            LOG_INFO("SWU: handle "FMTx64" retry %d", _handle, i);
        }
        _tot_xferred = 0;
        rv = _get_content(_state.content_id);
        if ( rv == 0 ) {
            break;
        }
        LOG_ERROR("SWU: Failed to fetch content: %d", rv);
    }
    if (rv < 0) {
        goto done;
    }

    LOG_INFO("SWU: Downloads complete for %s!", _state.app_guid.c_str());
    _is_done = true;

done:
    // Bug 14586 and 14541
    // It will cause deadlock when SWUpdate_Quit is called.
    // SWUpdate_Quit need to wait until this thread complete, while this thread is waiting for _swu_lock held by SWUpdate_Quit.
    // So, skip it to avoid deadlock.
    if (!_shutdown_now) {

        // This needs to be locked to prevent race conditions with sync
        VPLMutex_Lock(&_swu_lock);
        if ( rv ) {
            if ( _start_cancel ) {
                _is_canceled = true;
            }
            else {
                _is_failed = true;
            }
        }
        LOG_INFO("SWU: Download for %s done - rv %d", _state.app_guid.c_str(), rv);
        _is_in_progress = false;

        // We also don't want this structure getting reaped before
        // we're done using it.
        _send_event();
        VPLMutex_Unlock(&_swu_lock);

    }
}

void SWU_Download::cancel(void)
{
    _start_cancel = true;
    if ( !_is_in_progress ) {
        _is_canceled = true; 
    }
}

int SWU_Download::_decrypt_content(u32 tmd_handle, const std::string& out_file)
{
    int rv;
    ssize_t xfer_in;
    ssize_t xfer_out;
    VPLFile_handle_t _in_fh;
    VPLFile_handle_t _out_fh;
    bool in_is_open = false;
    bool out_is_open = false;
    const int pagesize = 4096;
    u8 *buf_in = NULL;
    u8 *buf_out = NULL;
    
    // open the input file
    _in_fh = VPLFile_Open(_download_path.c_str(), VPLFILE_OPENFLAG_READONLY, 0);
    if ( !VPLFile_IsValidHandle(_in_fh) ) {
        LOG_INFO("SWU: Failed open of %s", _download_path.c_str());
        rv = SWU_ERR_FAIL;
        goto done;
    }
    in_is_open = true;

    // open the output file
    _out_fh = VPLFile_Open(out_file.c_str(),
        VPLFILE_OPENFLAG_CREATE | VPLFILE_OPENFLAG_WRITEONLY, 
        VPLFILE_MODE_IRUSR | VPLFILE_MODE_IWUSR);
    if ( !VPLFile_IsValidHandle(_out_fh) ) {
        LOG_INFO("SWU: Failed open of %s", out_file.c_str());
        rv = SWU_ERR_FAIL;
        goto done;
    }
    out_is_open = true;

    if ( (buf_in = (u8 *) malloc(pagesize)) == NULL) {
        LOG_INFO("SWU: malloc failed");
        rv = SWU_ERR_NO_MEM;
        goto done;
    }

    if ( (buf_out = (u8 *) malloc(pagesize)) == NULL) {
        LOG_INFO("SWU: malloc failed");
        rv = SWU_ERR_NO_MEM;
        goto done;
    }

    // decrypt the file
    for( ;; ) {
        // read in data from the file
        xfer_in = VPLFile_Read(_in_fh, buf_in, pagesize);
        if ( xfer_in == 0 ) {
            break;
        }
        if ( xfer_in < 0 ) {
            LOG_ERROR("Failed reading input file %s", _download_path.c_str());
            rv = SWU_ERR_DECRYPT_FAIL;
            goto done;
        }

        // decrypt it
        if ( (rv = ESCore_DecryptContentByIdx(tmd_handle, SWU_CONTENT_INDEX,
                buf_in, buf_out, xfer_in)) ) {
            LOG_ERROR("Failed decrypt of file %s: %d", _download_path.c_str(),
                rv);
            rv = SWU_ERR_DECRYPT_FAIL;
            goto done;
        }

        // write it out
        xfer_out = VPLFile_Write(_out_fh, buf_out, xfer_in);
        if ( xfer_out != xfer_in ) {
            LOG_ERROR("SWU: Failed writing output file %s", out_file.c_str());
            rv = SWU_ERR_FAIL;
            goto done;
        }
    }

    // Truncate the output file
    LOG_INFO("SWU: Trunc size "FMTu64, _content_size);
    if ( (rv = VPLFile_TruncateAt(_out_fh, _content_size)) ) {
        LOG_ERROR("Failed trunc of content %s", _state.app_guid.c_str());
        rv = SWU_ERR_DECRYPT_FAIL;
        goto done;
    }

    rv = 0;
done:
    if ( in_is_open ) {
        VPLFile_Close(_in_fh);
    }
    if ( out_is_open ) {
        VPLFile_Close(_out_fh);
    }
    if ( rv ) {
        (void)VPLFile_Delete(out_file.c_str());
    }

    if ( buf_in ) {
        free(buf_in);
    }

    if ( buf_out ) {
        free(buf_out);
    }

    return rv;
}

int SWU_Download::decrypt(const std::string& out_file)
{
    int rv;
    u32 tkt_handle, tmd_handle;
    bool ticket_imported = false;;
    bool tmd_imported = false;
    const void *cert_data[ES_CERT_MAX];
    u32 cert_len[ES_CERT_MAX];

    // create the array of certs needed for import
    for (size_t i = 0; i < _ticket_certs.size(); i++) {
        cert_data[i] = _ticket_certs[i].c_str();
        cert_len[i] = _ticket_certs[i].size();
    }
    if ( (rv = ESCore_ImportTicketTitle(_state.title_id, ES_COOKIE,
            _ticket.c_str(), _ticket.size(), cert_data, cert_len,
            _ticket_certs.size(), &tkt_handle)) ) {
        LOG_ERROR("SWU: Failed to import ticket: %d", rv);
        goto done;

    }
    ticket_imported = true;

    if ( (rv = ESCore_ImportTitle(_state.title_id, ES_COOKIE, 0, tkt_handle,
            _tmd.c_str(), _tmd.size(), &tmd_handle)) ) {
        LOG_ERROR("SWU: Failed to import tmd: %d", rv);
        goto done;
    }
    LOG_INFO("SWU: Successfully imported tmd");
    tmd_imported = true;

    rv = _decrypt_content(tmd_handle, out_file);
    if (rv < 0) {
        LOG_ERROR("SWU: Failed to decrypt content: %d", rv);
        goto done;
    }

    LOG_INFO("SWU: Successfully decrypted content");

done:
    if ( tmd_imported ) {
        (void)ESCore_ReleaseTitle(tmd_handle);
    }
    if ( ticket_imported ) {
        (void)ESCore_ReleaseTicket(tkt_handle);
    }

    return rv;
}

int SWU_Download::clean_up(void)
{
    int rv;

    // now, remove the directory
    if ( (rv = Util_rmRecursive(_download_path, _to_delete_path)) ) {
        LOG_ERROR("SWU: Failed removing file %s: %d", _download_path.c_str(), rv);
        rv = SWU_ERR_FAIL;
        goto done;
    }
    LOG_INFO("SWU: Removed file %s", _download_path.c_str());

    rv = 0;
done:
    return rv;
}

SWU_Nus::SWU_Nus()
{
    _is_init = false;
    _ccd_version_is_set = false;
    _fetch_time = 0;
    _is_qa = false;
    _is_auto_update_disabled = false;
    _is_infra_download = false;
}

int SWU_Nus::init(void)
{
    // Nothing to really do at this point
    _is_init = true;

    return 0;
}

int SWU_Nus::_get_sw_state(const char* userGroup)
{
    int rv;
    vplex::nus::GetSystemUpdateResponseType res;
    vplex::nus::GetSystemUpdateRequestType req;
    std::string account_id;
    u64 userId = 0;
    std::string userGroupStr;

    userGroupStr.assign(userGroup);

    if ( !_is_init ) {
        LOG_ERROR("SWU: Not initialized!");
        rv = SWU_ERR_NOT_INIT;
        goto done;
    }

    if ( _device_id == _DEFAULT_DEV_ID ) {
        _device_id = VirtualDevice_GetDeviceId();
        if ( _device_id == 0 ) {
            _device_id = _DEFAULT_DEV_ID;
        }
        LOG_INFO("SWU: device id ="FMTx64, _device_id);
    }

    Query_SetNusAbstractRequestFields(req);
    req.set_runtimetypemask(0x0100);
    req.mutable__inherited()->set_deviceid(_device_id);

    // set the account id. This is optional so if we fail obtaining one
    // perform the query without it.
    if ( _device_id != _DEFAULT_DEV_ID ) {
        if ( (rv = _get_account_and_user_id(account_id, userId)) ) {
            LOG_ERROR("Failed pulling out the account and user id: %d", rv);
        }
    }
    if ( account_id.size() ) {
        req.mutable__inherited()->set_accountid(account_id);
    }
    // set the user id. This is optional so if we fail obtaining one
    // perform the query without it.
    if (userId != 0) {
        req.mutable__inherited()->set_userid(userId);
    }
    // set the user group. This is optional so if we fail obtaining one
    // perform the query without it.
    if (!userGroupStr.empty()) {
        req.set_group(userGroupStr);
    }

    rv = QUERY_NUS(VPLNus_GetSystemUpdate, req, res);
    if (rv != CCD_OK) {
        LOG_ERROR("SWU: Failed NUS query: %d", rv);
        goto done;
    }

    _content_prefix_url = res.contentprefixurl();

    _is_qa = res.isqa();
    _is_auto_update_disabled = res.isautoupdatedisabled();
    _is_infra_download = res.infradownload();

    _swu_states.clear();
    for( int i = 0 ; i < res.titleversion_size() ; i++ ) {
        swu_state_t *state;
        const vplex::nus::TitleVersionType &title = res.titleversion(i);
        istringstream ss(title.titleid());

        if ( !title.has_appguid() ) {
            LOG_INFO("SWU: Skipping %s - no app guid", title.titleid().c_str());
            continue;
        }
        if ( !title.has_appversion() ) {
            LOG_INFO("SWU: Skipping %s - no app version",
                title.titleid().c_str());
            continue;
        }
        if ( !title.has_appminversion() ) {
            LOG_INFO("SWU: Skipping %s - no app min version",
                title.titleid().c_str());
            continue;
        }
        if ( !title.has_ccdminversion() ) {
            LOG_INFO("SWU: Skipping %s - no ccd min version",
                title.titleid().c_str());
            continue;
        }
        if ( !title.has_appmessage() ) {
            LOG_INFO("SWU: Skipping %s - no app message",
                title.titleid().c_str());
            continue;
        }

        state = &_swu_states[title.appguid()];
        state->app_guid = title.appguid();
        state->app_version = title.appversion();
        state->app_min_version = title.appminversion();
        state->ccd_min_version = title.ccdminversion();
        state->app_message = title.appmessage();

        state->title_id_str = title.titleid();
        ss >> hex >> state->title_id;
        state->version = title.version();
        state->ticket_size = (title.has_ticketsize()) ? title.ticketsize() : 0;
        state->tmd_size = (title.has_tmdsize()) ? title.tmdsize() : 0;
        state->fs_size = title.fssize();
        LOG_INFO("SWU: GUID %s title "FMTx64": version %d, %s, %s, %s",
            state->app_guid.c_str(), state->title_id, state->version,
            state->app_version.c_str(), state->app_min_version.c_str(),
            state->ccd_min_version.c_str());

        if ( _ccd_version_is_set && (_ccd_guid.compare(state->app_guid) == 0)) {
            _ccd_version_latest = state->app_version;
            LOG_INFO("SWU: Latest CCD version = %s - %s = %s",
                _ccd_version_latest.c_str(), state->ccd_min_version.c_str(),
                title.ccdminversion().c_str());
        }
    }
    _fetch_time = time(NULL);

    rv = 0;
done:
    return rv;
}

// walk the updates directory and see if there are any files
// here that represent unknown titles
void SWU_Nus::_clear_stale_updates(void)
{
    int rv;
    VPLFS_dirent_t de;
    VPLFS_dir_t d;
    bool dir_is_open = false;

    LOG_INFO("SWU: Looking for stale updates");

    if ( (rv = VPLFS_Opendir(_update_path.c_str(), &d)) ) {
        LOG_ERROR("SWU: Failed opening update path %s: %d", _update_path.c_str(),
            rv);
        goto done;
    }
    dir_is_open = true;

    while ( (rv = VPLFS_Readdir(&d, &de)) == 0 ) {
        u64 title_id;
        int version;
        SWUStateMap::iterator it;
        SWUDownloadGuidMap::iterator g_it;
        std::string path;
        bool found;

        // We only care about files
        if ( de.type != VPLFS_TYPE_FILE ) {
            continue;
        }
        // Parse the filename
        if ( sscanf(de.filename, FMTx64"_%08x", &title_id, &version) != 2 ) {
            LOG_ERROR("SWU: Unexpected file %s", de.filename);
            continue;
        }

        found = false;
        for( it = _swu_states.begin() ; it != _swu_states.end() ; it++ ) {
            if ( it->second.title_id == title_id ) {
                if ( it->second.version == version ) {
                    found = true;
                }
                break;
            }
        }

        // This is a candidate to remove
        if ( !found ) {
            continue;
        }

        // Look for it in the active downloads
        g_it = _swu_downloads_guid.find(it->second.app_guid +
            it->second.app_version);
        if ( (g_it != _swu_downloads_guid.end()) ) {
            SWU_Download *download = NULL;
            if ( g_it->second->is_in_progress() ) {
                LOG_INFO("SWU: sparing active download %s",
                    it->second.app_guid.c_str());
                continue;
            }
        
            download = g_it->second;
            delete download;
        }

        path = _update_path + de.filename;
        LOG_INFO("SWU: Removing stale file %s", path.c_str());
        if ( (rv = VPLFile_Delete(path.c_str())) ) {
            LOG_ERROR("SWU: Failed removing stale file %s: %d",
                path.c_str(), rv);
        }
    }

done:
    if ( dir_is_open ) {
        VPLFS_Closedir(&d);
    }
}

// We don't completely control version strings so allow a
// ridiculous range of 1 to 5 components since we're already
// seing 2, 3 and 4.
#define _MIN_VER_COMP       1
#define _MAX_VER_COMP       5

static int _rel_compare(const std::string& local, const std::string& server,
    bool& needs_update, bool& server_is_downrev)
{
    int rv;
    int lcl[_MAX_VER_COMP];
    int svr[_MAX_VER_COMP];
    int ccl, ccs;
    int dmy;

    ccl = sscanf(local.c_str(), "%d.%d.%d.%d.%d.%d", &lcl[0], &lcl[1], &lcl[2],
        &lcl[3], &lcl[4], &dmy);
    ccs = sscanf(server.c_str(), "%d.%d.%d.%d.%d.%d", &svr[0], &svr[1], &svr[2],
        &svr[3], &svr[4], &dmy);
    if ( ccl != ccs ) {
        LOG_ERROR("SWU: Incompatible version strings %s:%s",
            local.c_str(), server.c_str());
        rv = SWU_ERR_FAIL;
        goto done;
    }

    if ( (ccl < _MIN_VER_COMP) || (ccs < _MIN_VER_COMP) ) {
        LOG_ERROR("SWU: Failed parsing version strings %s:%s", local.c_str(),
            server.c_str());
        rv = SWU_ERR_FAIL;
        goto done;
    }

    if ( (ccl > _MAX_VER_COMP) || (ccs > _MAX_VER_COMP) ) {
        LOG_ERROR("SWU: Too many comp's in version string %s:%s", local.c_str(),
            server.c_str());
        rv = SWU_ERR_FAIL;
        goto done;
    }

    needs_update = false;
    server_is_downrev = false;
    for( int i = 0 ; i < ccl ; i++ ) {
        if ( svr[i] > lcl[i] ) {
            needs_update = true;
            break;
        }
        // If we have a version greater than the server then the server is
        // downrev from us. We might need to downrev if there was a
        // publishing error
        if ( lcl[i] > svr[i] ) {
            server_is_downrev = true;
            break;
        }
    }

    rv = 0;
done:
    return rv;
}

int SWU_Nus::check_sw_update(const std::string& app_guid,
    const std::string& app_version, bool update_cache,
    u64& update_mask, u64& app_size, std::string& latest_app_version,
    std::string& change_log, std::string& latest_ccd_version, bool& isAutoUpdateDisabled, bool& isQA, bool& isInfraDownload, const char* userGroup)
{
    int rv;
    SWUStateMap::iterator it;
    bool needs_update;
    bool server_is_downrev_app;
    bool force_ccd_upgrade = false;
    bool server_is_downrev_ccd;

    // pull down the latest data if we can find it.
    if ( update_cache || ((time(NULL) - _fetch_time) > NUS_CACHE_TIMEOUT) ) {
        LOG_INFO("SWU: SW State for %s needs an update, update_cache = %s",
            app_guid.c_str(), update_cache ? "true" : "false");
        if ( (rv = _get_sw_state(userGroup)) ) {
            LOG_ERROR("SWU: Failed _get_sw_state(%s): %d", userGroup, rv);
            goto done;
        }
        // Clear away stale entries
        _clear_stale_updates();
    }

    // Can't update if we don't know anything about this GUID.
    it = _swu_states.find(app_guid);
    if ( it == _swu_states.end() ) {
        rv = SWU_ERR_NOT_FOUND;
        goto done;
    }

    // We now have enough information to fill in the mask
    update_mask = 0;

    isAutoUpdateDisabled = _is_auto_update_disabled;
    isQA = _is_qa;
    isInfraDownload = _is_infra_download;

    // downrev is OK here
    if ( (rv = _rel_compare(app_version, it->second.app_min_version,
            needs_update, server_is_downrev_app)) ) {
        LOG_WARN("SWU: Failed app version string min comparison = %d", rv);
        update_mask |= ccd::SW_UPDATE_BIT_APP_CRITICAL;
        force_ccd_upgrade = true;
    }
    else if ( needs_update ) {
        update_mask |= ccd::SW_UPDATE_BIT_APP_CRITICAL;
    }
    else {
        // Here's where downrev matters
        if ( (rv = _rel_compare(app_version, it->second.app_version,
                needs_update, server_is_downrev_app)) ) {
            LOG_WARN("SWU: Failed app version string comparison = %d", rv);
            update_mask |= ccd::SW_UPDATE_BIT_APP_CRITICAL;
            force_ccd_upgrade = true;
        }
        else if ( server_is_downrev_app ) {
            LOG_INFO("SWU: guid %s server(%s) is downrev from local(%s)",
                app_guid.c_str(), it->second.app_version.c_str(),
                app_version.c_str());
            update_mask |= ccd::SW_UPDATE_BIT_APP_CRITICAL;
            force_ccd_upgrade = true;
        }
        else if ( needs_update ) {
            update_mask |= ccd::SW_UPDATE_BIT_APP_OPTIONAL;
        }
    }

    // downrev is OK here
    if ( (rv = _rel_compare(_ccd_version, it->second.ccd_min_version,
            needs_update, server_is_downrev_ccd)) ) {
        LOG_WARN("SWU: Failed ccd version string comparison = %d", rv);
        update_mask |= ccd::SW_UPDATE_BIT_CCD_CRITICAL;
    }
    else if ( needs_update || force_ccd_upgrade ) {
        // If we're downrev'ing the application then we force an install
        // of CCD, as well, for safety sake.
        update_mask |= ccd::SW_UPDATE_BIT_CCD_CRITICAL;
    }
    else {
        if ( (rv = _rel_compare(_ccd_version, _ccd_version_latest,
            needs_update, server_is_downrev_ccd)) ) {
            LOG_WARN("SWU: Failed ccd vs latest version comparisons = %d", rv);
            update_mask |= ccd::SW_UPDATE_BIT_CCD_CRITICAL;
        }
        else if ( server_is_downrev_ccd ) {
            update_mask |= ccd::SW_UPDATE_BIT_CCD_CRITICAL;
        }
        else if ( needs_update ) {
            update_mask |= ccd::SW_UPDATE_BIT_CCD_NEEDED;
        }
    }

    latest_app_version = it->second.app_version;
    change_log = it->second.app_message;
    latest_ccd_version = _ccd_version_latest;
    app_size = it->second.fs_size;

    rv = 0;
done:
    return rv;
}

int SWU_Nus::guid_lookup(const std::string& guid, const std::string& version,
    swu_state_t& state)
{
    int rv;
    SWUStateMap::iterator it;

    if ( !_is_init ) {
        rv = SWU_ERR_NOT_INIT;
        goto done;
    }

    if ( (it = _swu_states.find(guid)) == _swu_states.end() ) {
        rv = SWU_ERR_NOT_FOUND;
        goto done;
    }

    if ( it->second.app_version.compare(version) != 0 ) {
        LOG_ERROR("SWU: Attempt to download invalid version %s",
            version.c_str());
        rv = SWU_ERR_NOT_FOUND;
        goto done;
    }

    state = it->second;

    rv = 0;
done:
    return rv;
}

int SWU_Nus::set_ccd_version(const std::string& guid,
    const std::string& version)
{
    int rv;

    if ( !_is_init ) {
        LOG_ERROR("SWU: Not initialized!");
        rv = SWU_ERR_NOT_INIT;
        goto done;
    }

    _ccd_guid = guid;
    _ccd_version = version;
    _ccd_version_is_set = true;

    rv = 0;
done:
    return rv;
}

int SWUpdate_Init(void)
{
    int rv;
    char tmpPathBuf[CCD_PATH_MAX_LENGTH];


    DiskCache::getPathForUpdate(sizeof(tmpPathBuf), /*OUT*/tmpPathBuf);
    _update_path = tmpPathBuf;

    DiskCache::getPathForDeletion(sizeof(tmpPathBuf), /*OUT*/tmpPathBuf);
    _to_delete_path = tmpPathBuf;

    // Pull out the time - used for creating unique handles
    // across CCD restarts
    _swu_start = time(NULL);

    if ( (rv = VPLFS_Mkdir(_update_path.c_str())) ) {
        if ( rv != VPL_ERR_EXIST ) {
            LOG_INFO("SWU: Failed creating update directory %s: %d",
                _update_path.c_str(), rv);
            rv = SWU_ERR_FAIL;
            goto done;
        }
    }

    if ( (rv = _swu_nus.init()) ) {
        LOG_ERROR("SWU: Failed init of SWU_Nus: %d", rv);
        goto done;
    }

    if ( VPLMutex_Init(&_swu_lock) ) {
        rv = -1;
        goto done;
    }

    LOG_INFO("SWU: tmp update dir %s", _update_path.c_str());
    LOG_INFO("SWU: SWUpdate initialized");
    rv = 0;
done:
    return rv;
}

int SWUpdateCheck(const std::string& app_guid, const std::string& app_version,
    bool update_cache, u64& update_mask, u64 &app_size,
    std::string& latest_app_version, std::string& change_log,
    std::string& latest_ccd_version, bool& isAutoUpdateDisabled, bool& isQA, bool &isInfraDownload, const char* userGroup)
{
    int rv;
    bool has_lock = false;

    LOG_INFO("SWU: check %s %s in %s group.", app_guid.c_str(), app_version.c_str(), userGroup);

    VPLMutex_Lock(&_swu_lock);
    has_lock = true;

    // Make sure that the CCD GUID/version is set otherwise the results
    // of the check may be bad. This also prevents the title cache
    // from being updated before the version is known and hence
    // not pulling out the current server version for CCD correctly.
    // (_ccd_Version_latest)
    if ( ! _swu_nus.ccd_version_is_set() ) {
        LOG_WARN("SWU: ccd version not set");
        rv = SWU_ERR_CCD_NOT_SET;
        goto done;
    }

    if ( (rv = _swu_nus.check_sw_update(app_guid, app_version, update_cache,
            update_mask, app_size, latest_app_version, change_log,
            latest_ccd_version, isAutoUpdateDisabled, isQA, isInfraDownload, userGroup)) ) {
        LOG_ERROR("SWU: Failed check_sw_update(): %d", rv);
        goto done;
    }

    LOG_INFO("SWU: %s mask "FMTx64" ccd %s log: %s", latest_app_version.c_str(),
        update_mask, latest_ccd_version.c_str(), change_log.c_str());

    rv = 0;
done:
    if ( (has_lock) ) {
        VPLMutex_Unlock(&_swu_lock);
    }
    return rv;
}

int SWUpdateBeginDownload(const std::string& app_guid,
    const std::string& app_version, u64& handle)
{
    int rv;
    bool has_lock = false;
    SWUDownloadGuidMap::iterator it;
    SWU_Download *download = NULL;

    LOG_INFO("SWU: Begin %s %s", app_guid.c_str(), app_version.c_str());

    // Force an update check to fill the title cache in case we've
    // restarted since the last SWUpdateCheck(). Otherwise we'd fail
    // the download as the title would be unknown.
    {
        u64 update_mask;
        u64 app_size;
        std::string latest_app_version;
        std::string change_log;
        std::string latest_ccd_version;
        bool isQA = false;
        bool isAutoUpdateDisabled = false;
        bool isInfraDownload = false;

        // NOTE: This check will catch whether the CCD GUID/Version
        // is set or not so we don't need an explicit call in this function.
        if ( (rv = SWUpdateCheck(app_guid, app_version, false, update_mask,
                app_size, latest_app_version, change_log,
                latest_ccd_version, isAutoUpdateDisabled, isQA, isInfraDownload, __ccdConfig.userGroup)) ) {
            LOG_INFO("SWU: Failed SWUpdateCheck(%s, %s): %d",
                app_guid.c_str(), app_version.c_str(), rv);
            goto done;
        }
    }

    VPLMutex_Lock(&_swu_lock);
    has_lock = true;

    // See if there's already a download in progress
    it = _swu_downloads_guid.find(app_guid + app_version);
    if ( it != _swu_downloads_guid.end() ) {
        LOG_INFO("SWU: Found existing download for %s", app_guid.c_str());
        download = it->second;
    }
    else {
        swu_state_t state;

        LOG_INFO("SWU: Creating new download for %s", app_guid.c_str());
        if ( (rv = _swu_nus.guid_lookup(app_guid, app_version, state)) ) {
            LOG_INFO("SWU: Unknown guid/version %s:%s", app_guid.c_str(),
                app_version.c_str());
            goto done;
        }
        download = new SWU_Download;
        if ( download == NULL ) {
            rv = SWU_ERR_NO_MEM;
            goto done;
        }
        if ( (rv = download->init(_swu_nus.get_content_prefix_url(), state)) ) {
            LOG_INFO("SWU: Failed init of download for %s", app_guid.c_str());
            goto done;
        }
    }

    // At this point we have have download entries for each.
    if ( (rv = download->begin(handle)) ) {
        LOG_INFO("SWU: Failed begin(%s)", app_guid.c_str());
        goto done;
    }

    LOG_INFO("SWU: begin handle "FMTu64, handle);

    rv = 0;
done:
    // clean up from failures
    if ( rv && download ) {
        delete download;
    }
    if ( (has_lock) ) {
        VPLMutex_Unlock(&_swu_lock);
    }
    return rv;
}

int SWUpdateGetDownloadProgress(u64 handle, u64& tot_xfer_size,
    u64& tot_xferred, ccd::SWUpdateDownloadState_t& state)
{
    int rv;
    bool has_lock = false;
    SWUDownloadTitleMap::iterator t_it;

    LOG_INFO("SWU: progress handle "FMTu64, handle);

    VPLMutex_Lock(&_swu_lock);
    has_lock = true;

    t_it = _swu_downloads_handle.find(handle);
    if ( t_it == _swu_downloads_handle.end() ) {
        rv = SWU_ERR_NOT_FOUND;
        goto done;
    }

    rv = t_it->second->get_progress(tot_xfer_size, tot_xferred, state);
    if ( rv ) {
        LOG_ERROR("SWU: Failed getting progress: %d", rv);
        goto done;
    }

    rv = 0;
done:
    if ( (has_lock) ) {
        VPLMutex_Unlock(&_swu_lock);
    }
    return rv;
}

int SWUpdateSetCcdVersion(const std::string& guid, const std::string& version)
{
    int rv;
    bool has_lock = false;

    LOG_INFO("SWU: CCD GUID %s version %s", guid.c_str(), version.c_str());

    VPLMutex_Lock(&_swu_lock);
    has_lock = true;

    if ( (rv = _swu_nus.set_ccd_version(guid, version)) ) {
        LOG_ERROR("SWU: Failed set_ccd_version(): %d", rv);
        goto done;
    }

    rv = 0;
done:
    if ( (has_lock) ) {
        VPLMutex_Unlock(&_swu_lock);
    }
    return rv;

}

int SWUpdateEndDownload(u64 handle, const std::string& file_loc)
{
    int rv;
    bool has_lock = false;
    std::string app_guid;
    SWU_Download *download;
    SWUDownloadTitleMap::iterator it;
    bool do_delete = false;

    LOG_INFO("SWU: end handle "FMTx64" loc %s", handle, file_loc.c_str());

    VPLMutex_Lock(&_swu_lock);
    has_lock = true;

    it = _swu_downloads_handle.find(handle);
    if ( it == _swu_downloads_handle.end() ) {
        rv = SWU_ERR_NOT_FOUND;
        goto done;
    }

    download = it->second;

    // There's a race condition here where in progress gets
    // cleared after done/canceled/failed is set but the worker thread 
    // is still accessing the data structure. So... We need to order this
    // properly to prevent deleting the data structure from the 
    // worker thread.
    if ( download->is_in_progress() ) {
        rv = SWU_ERR_IN_PROGRESS;
        goto done;
    }

    // If we were canceled, just delete everything and start over
    // later from scratch
    if ( download->is_canceled() ) {
        do_delete = true;
        rv = 0;
        goto done;
    }

    if ( ! download->is_done() ) {
        if ( download->is_failed() ) {
            rv = SWU_ERR_DOWNLOAD_FAILED;
        }
        else {
            rv = SWU_ERR_DOWNLOAD_STOPPED;
        }
        goto done;
    }

    if ( (rv = download->decrypt(file_loc)) ) {
        // If we failed a decrypt, blow away the content. It's probably
        // corrupted.
        if ( rv == SWU_ERR_DECRYPT_FAIL ) {
            do_delete = true;
        }
        LOG_ERROR("SWU: Failed exporting "FMTx64" to %s", handle,
            file_loc.c_str());
        goto done;
    }

    do_delete = true;
    rv = 0;

done:
    if ( do_delete ) {
        // Remove the tmp directory
        if ( download->clean_up() ) {
            // Note: This doesn't fail things
            LOG_ERROR("SWU: Failed removing the download temp files!");
        }

        delete download;
    }
    if ( (has_lock) ) {
        VPLMutex_Unlock(&_swu_lock);
    }
    return rv;
}

int SWUpdateCancelDownload(u64 handle)
{
    int rv;
    bool has_lock = false;
    std::string app_guid;
    SWU_Download *download;
    SWUDownloadTitleMap::iterator it;

    LOG_INFO("SWU: cancel handle "FMTx64, handle);

    VPLMutex_Lock(&_swu_lock);
    has_lock = true;

    it = _swu_downloads_handle.find(handle);
    if ( it == _swu_downloads_handle.end() ) {
        rv = SWU_ERR_NOT_FOUND;
        goto done;
    }

    download = it->second;

    download->cancel();

    rv = 0;

done:
    if ( (has_lock) ) {
        VPLMutex_Unlock(&_swu_lock);
    }
    return rv;
}

void SWUpdate_Quit(void)
{
    SWUDownloadTitleMap::iterator it;

    // trigger downloads to end
    _shutdown_now = true;

    // Assumption: User-level apps can no longer call into this module when
    // this function is called. The CCDI interface is disabled.

    // wait for everything to shut down.
    VPLMutex_Lock(&_swu_lock);

    for( it = _swu_downloads_handle.begin() ; it != _swu_downloads_handle.end() ;
            it++ ) {
        // wait for any running threads to complete
        if ( it->second->is_in_progress() ) {
            it->second->worker_join();
        }
    }

    VPLMutex_Unlock(&_swu_lock);

    (void)VPLMutex_Destroy(&_swu_lock);

    LOG_INFO("SWU: SWUpdate Quit");
}
