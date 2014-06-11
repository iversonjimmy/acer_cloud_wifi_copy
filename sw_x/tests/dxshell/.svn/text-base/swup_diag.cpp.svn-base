//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
// vim: ts=4:sw=4:expandtab

// Software Update API test for dxshell

#include <ccdi.hpp>
#include <ccdi_client.hpp>
#include <vplex_file.h>
#include <vplex_time.h>
#include <vpl_fs.h>
#include <vplu_types.h>
#include "vpl_th.h"
#include <vpl_plat.h>


#include <string>
#include <map>
#include <log.h>

#include "dx_common.h"
#include "ccd_utils.hpp"
#include "common_utils.hpp"
#include "gvm_file_utils.hpp"

#define ARRAY_LEN(x)    ((sizeof(x))/(sizeof(x[0])))

// TODO: Use a more appropriate structure for representing GUIDs
#define WIN_ACERCLOUD_GUID      "a5ad0b17-f34d-49be-a157-c8b3d52acd13"
#define WIN_CLEARFIPHOTO_GUID   "b5ad89f2-03d3-4206-8487-018298007dd0"
#define WIN_CLEARFIMEDIA_GUID   "e9af1707-3f3a-49e2-8345-4f2d629d0876"
#define WIN_ACERCLOUDDOCS_GUID  "ca4fe8b0-298c-4e5d-a486-f33b126d6a0a"
#ifdef _FUTURE
#define WIN_IOAC_WIFI_WIN7_GUID "28006915-2739-4EBE-B5E8-49B25D32EB33 "
#endif // _FUTURE

#define AND_ACERCLOUD_GUID      "951D885F-BD9F-4F74-A013-E296B194F46B"
#define AND_CLEARFIPHOTO_GUID   "765C60BE-95A4-46C7-AB56-38104B4D438A"
#define AND_CLEARFIVIDEO_GUID   "D81B5307-CCFE-404F-BEA5-2D9C7B3894AA"
#define AND_CLEARFIMUSIC_GUID   "0CA4401A-BAC6-4280-932D-EBDCC89C5BB0"
#define AND_ACERCLOUDDOCS_GUID  "74E844FA-2038-48B7-A9A4-61936AF0149D"

// These strings are used as GUIDs even though they are not real GUIDs
#define TEST_DXWIN_SWU_CCD      "test-dxwin-swu-ccd"
#define TEST_DXWIN_SWU_APP      "test-dxwin-swu-01"
#define TEST_DXAND_SWU_CCD      "test-dxand-swu-ccd"
#define TEST_DXAND_SWU_APP      "test-dxand-swu-001"

static std::string swu_default_guids = WIN_ACERCLOUD_GUID ","
    WIN_CLEARFIPHOTO_GUID ","
    WIN_CLEARFIMEDIA_GUID ","
    WIN_ACERCLOUDDOCS_GUID
#ifdef _FUTURE
    "," WIN_IOAC_WIFI_WIN7_GUID
#endif // _FUTURE
    ;

static std::string swu_android_guids = AND_ACERCLOUD_GUID ","
    AND_CLEARFIPHOTO_GUID ","
    AND_CLEARFIVIDEO_GUID ","
    AND_CLEARFIMUSIC_GUID ","
    AND_ACERCLOUDDOCS_GUID;

static std::map<std::string,std::string> mapGuidFile;
static std::map<std::string,int> mapGuidVers;
static void init_mapGuidFile(void)
{
    mapGuidFile[WIN_ACERCLOUD_GUID] = "AcerCloudSetup";
    mapGuidFile[WIN_CLEARFIPHOTO_GUID] = "clear.fiPhotoSetup";
    mapGuidFile[WIN_CLEARFIMEDIA_GUID] = "clear.fiMediaSetup";
    mapGuidFile[WIN_ACERCLOUDDOCS_GUID] = "AcerCloudDocsSetup";
#ifdef _FUTURE
    mapGuidFile[WIN_IOAC_WIFI_WIN7_GUID ] = "Win7IOACWifiDriver";
#endif // _FUTURE
    mapGuidFile[AND_ACERCLOUD_GUID] = "AcerCloud";
    mapGuidFile[AND_CLEARFIPHOTO_GUID] = "clear.fiPhoto";
    mapGuidFile[AND_CLEARFIVIDEO_GUID] = "clear.fiVideo";
    mapGuidFile[AND_CLEARFIMUSIC_GUID] = "clear.fiMusic";
    mapGuidFile[AND_ACERCLOUDDOCS_GUID] = "AcerCloudDocs";
//    mapGuidFile["windows-cf-explorer-app"] = "cf_explorer";

    mapGuidVers[WIN_ACERCLOUD_GUID] = 3;
    mapGuidVers[WIN_CLEARFIPHOTO_GUID] = 3;
    mapGuidVers[WIN_CLEARFIMEDIA_GUID] = 3;
    mapGuidVers[WIN_ACERCLOUDDOCS_GUID] = 3;
#ifdef _FUTURE
    mapGuidVers[WIN_IOAC_WIFI_WIN7_GUID] = 2;
#endif // _FUTURE
    mapGuidVers[AND_ACERCLOUD_GUID] = 4;
    mapGuidVers[AND_CLEARFIPHOTO_GUID] = 4;
    mapGuidVers[AND_CLEARFIVIDEO_GUID] = 4;
    mapGuidVers[AND_CLEARFIMUSIC_GUID] = 4;
    mapGuidVers[AND_ACERCLOUDDOCS_GUID] = 3;
}

typedef std::vector<std::string> StrVec;

struct swu_testdata {
     std::string    ccd_guid;
     std::string    ccd_ver;
     std::string    app_guid;
     std::string    app_ver;
     std::string    app_longver;

     std::string    app_shortver;
     std::string    app_overver;
     std::string    app_noupver;
     std::string    ccd_needupver;
     std::string    ccd_noupver;
     std::string    ccd_opupver;
     std::string    ccd_overver;
     std::string    test_guids;
};

static const struct swu_testdata swu_testcases[]= {
    {
    // Implicit here are the following versions in
    // ccd version = 1.1.2, min version = 1.1.1
    // app version = 1.1.3, min version = 1.1.3 , ccdver
    TEST_DXWIN_SWU_CCD, "0.0.1", TEST_DXWIN_SWU_APP, "1.1.0", "1.1",
    "1.1.1.1", "2.0.0", "1.1.3", "1.1.0", "1.1.2", "1.1.1", "1.1.3", swu_default_guids
    },
    {
    // Implicit here are the following versions in
    // ccd version = 1.1.2.1, min version = 1.1.1.1, CCDMinVersion 1.1.1.1
    // app version = 1.1.3.1, min version = 1.1.3.1 , ccdMinVer 1.1.1.1
    "test-dxand-swu-ccd", "0.0.0.1", "test-dxand-swu-001", "1.1.0.0", "1.1",
    "1.1.1", "2.0.0.0", "1.1.3.1", "1.1.1.0", "1.1.2.1", "1.1.1.1", "1.1.3.1",
    swu_android_guids
    }
}, *_td;

static struct swu_vertest {
        std::string ccd_guid, ccd_ver, app_guid, app_ver;
        int exp_rv;
        u64 exp_mask;
        std::string test_desc;
} swu_vertests[] = {
    { TEST_DXWIN_SWU_CCD, "1.1.2", TEST_DXWIN_SWU_APP, "1.1.3",
        0, 0, "Matching version"},

    { TEST_DXWIN_SWU_CCD, "1.1.02", TEST_DXWIN_SWU_APP, "1.1.3",
        0, 0, "Leading 0 CCD 3rd digit"},
    
    { TEST_DXWIN_SWU_CCD, "1.1.2", TEST_DXWIN_SWU_APP, "1.1.03",
        0, 0, "Leading 0 App 3rd digit"},
    
    { TEST_DXWIN_SWU_CCD, "1.1.2", TEST_DXWIN_SWU_APP, "1.1.1.4",
    0, ccd::SW_UPDATE_BIT_CCD_CRITICAL | ccd::SW_UPDATE_BIT_APP_CRITICAL,
    "Invalid long app version string"},
    
    { TEST_DXWIN_SWU_CCD, "1.1.2", TEST_DXWIN_SWU_APP, "1.1",
    0, ccd::SW_UPDATE_BIT_CCD_CRITICAL | ccd::SW_UPDATE_BIT_APP_CRITICAL,
    "Invalid short app version string"},
    
    { TEST_DXWIN_SWU_CCD, "1.1.2", TEST_DXWIN_SWU_APP, "notaversion",
    0, ccd::SW_UPDATE_BIT_CCD_CRITICAL | ccd::SW_UPDATE_BIT_APP_CRITICAL,
    "Invalid non-numeric string"},

    { swu_testcases[0].ccd_guid, swu_testcases[0].ccd_noupver,
        swu_testcases[0].app_guid, swu_testcases[0].app_ver,
        0, ccd::SW_UPDATE_BIT_APP_CRITICAL,
        "Testing critical app upgrade"},

    { swu_testcases[0].ccd_guid, swu_testcases[0].ccd_overver,
        swu_testcases[0].app_guid, swu_testcases[0].app_noupver,
        0, ccd::SW_UPDATE_BIT_CCD_CRITICAL,
        "Testing required CCD downgrade"},
};


static std::string getNameFromGuid(std::string strGuid)
{
    std::map<std::string,std::string>::iterator it;
    std::string s;

    it = mapGuidFile.find(strGuid);
    if (it == mapGuidFile.end()) {
        s = strGuid;
    } else {
        s = it->second;
    }
    return s;
}
static std::string getFileFromGuid(std::string strGuid, std::string strVer)
{
    std::string s = getNameFromGuid(strGuid);

    if (s.find("Setup") != std::string::npos) {
        return (s + "_v" + strVer + ".zip");
    } else {
        return (s + "_v" + strVer + ".apk");
    }
}

static int _set_ccd_version(const std::string& guid, const std::string& ver)
{
    int rv;
    ccd::SWUpdateSetCcdVersionInput req;

    req.set_ccd_guid(guid);
    req.set_ccd_version(ver);
    if ( (rv = CCDISWUpdateSetCcdVersion(req)) ) {
        LOG_ERROR("Failed SWUpdateSetCcdVersion() - %d", rv);
        goto done;
    }

    rv = 0;
done:
    return rv;
}

static int _sw_download_begin(const std::string& guid, const std::string& ver,
    u64& handle)
{
    int rv;
    ccd::SWUpdateBeginDownloadInput req;
    ccd::SWUpdateBeginDownloadOutput resp;

    req.set_app_guid(guid);
    req.set_app_version(ver);

    if ( (rv = CCDISWUpdateBeginDownload(req, resp)) ) {
        LOG_ERROR("Failed SWUpdateBeginDownload - %d", rv);
        goto done;
    }

    handle = resp.handle();
    LOG_INFO("Handle = "FMTx64, handle);

    rv = 0;
done:
    return rv;
}

static int _sw_progress_poll(u64 handle, bool is_canceled)
{
    int rv;

    for ( ;; ) {
        ccd::SWUpdateGetDownloadProgressInput req;
        ccd::SWUpdateGetDownloadProgressOutput resp;
        req.set_handle(handle);

        if ( (rv = CCDISWUpdateGetDownloadProgress(req, resp)) ) {
            LOG_ERROR("Failed getting download progress - %d", rv);
            goto done;
        }
        switch (resp.state()) {
        case ccd::SWU_DLSTATE_DONE:
            LOG_INFO("Download complete");
            if (!is_canceled) {
                goto download_done;
            }
            rv = -1;
            goto done;
        case ccd::SWU_DLSTATE_IN_PROGRESS:
            LOG_INFO(FMTu64 " of " FMTu64 " bytes downloaded",
                resp.bytes_transferred_cnt(), resp.total_transfer_size());
            break;
        case ccd::SWU_DLSTATE_FAILED:
            LOG_INFO("Download failed");
            rv = -1;
            goto done;
        case ccd::SWU_DLSTATE_STOPPED:
            LOG_INFO("Download stopped");
            rv = -1;
            goto done;
        case ccd::SWU_DLSTATE_CANCELED:
            LOG_INFO("Download Canceled");
            if (is_canceled) {
                goto download_done;
            }
            rv = -1;
            goto done;
        default:
            LOG_ERROR("Failed getDownloadProgress() - %d", rv);
            rv = -1;
            goto done;
        }
        VPLThread_Sleep(VPLTIME_FROM_MILLISEC(500));
    }

download_done:
    rv = 0;
done:
    return rv;
}

static int _sw_update_check(const std::string& app_guid,
    const std::string& app_ver,
    bool has_update_cache, bool update_cache,
    std::string& ret_app_ver, 
    std::string& ret_ccd_ver, u64& ret_mask, u64& ret_size)
{
    int rv;
    ccd::SWUpdateCheckInput req;
    ccd::SWUpdateCheckOutput resp;

    req.set_app_guid(app_guid);
    req.set_app_version(app_ver);
    if (has_update_cache) {
        req.set_update_cache(update_cache);
    }

    if ( (rv = CCDISWUpdateCheck(req, resp)) ) {
        LOG_ERROR("Failed SWUpdateCheck() - %d", rv);
        goto done;
    }

    ret_app_ver = resp.latest_app_version();
    ret_ccd_ver = resp.latest_ccd_version();
    ret_mask = resp.update_mask();
    ret_size = resp.app_size();
    LOG_ALWAYS("isQA = %d", resp.is_qa());
    LOG_ALWAYS("isAutoUpdateDisabled = %d", resp.is_auto_update_disabled());
    // controlled release was rolled back.
    // see https://bugs.ctbg.acer.com/show_bug.cgi?id=18192
    // LOG_ALWAYS("isInfraDownload = %d", resp.is_infra_download());

    if (ret_size == (u64) 0) {
        LOG_ERROR("Bad size "FMTu64, ret_size);
        rv = -1;
    }

    rv = 0;
done:
    return rv;
}

static int _sw_update_check(const std::string& app_guid,
    const std::string& app_ver, std::string& ret_app_ver, 
    std::string& ret_ccd_ver, u64& ret_mask, u64& ret_size)
{
    return (_sw_update_check(app_guid, app_ver, false, false,
        ret_app_ver, ret_ccd_ver, ret_mask, ret_size));
}


static int _sw_download_end(u64 handle, const std::string& file_loc)
{
    int rv;
    ccd::SWUpdateEndDownloadInput req;

    // Ensure the string is clean
    req.set_handle(handle);
    req.set_file_location(file_loc);
    if ( (rv = CCDISWUpdateEndDownload(req)) ) {
        LOG_ERROR("Failed SWUpdateEndDownload() - %d", rv);
        goto done;
    }
    LOG_INFO("Download ended!");

    rv = 0;
done:
    return rv;
}

static int _sw_download_cancel(u64 handle)
{
    int rv;
    ccd::SWUpdateCancelDownloadInput req;

    req.set_handle(handle);
    if ( (rv = CCDISWUpdateCancelDownload(req)) ) {
        LOG_ERROR("Failed SWUpdateCancelDownload("FMTx64") - %d", handle, rv);
        goto done;
    }
    LOG_INFO("Download canceled!");

    rv = 0;
done:
    return rv;
}


static int _download_app_poll_int(const std::string& guid, const std::string& ver,
    const std::string& file_loc, bool test_handle)
{
    int rv;
    u64 handle;
    TargetDevice *target = NULL;

    target = getTargetDevice();
    if (target == NULL) {
        LOG_ERROR("getTargetDevice failed!");
        return -1;
    }
    rv = target->deleteFile(file_loc);
    if (rv != 0 && rv != VPL_ERR_NOT_EXIST && rv != VPL_ERR_NOENT) {
        LOG_ERROR("Failed cleaning up %s %d", file_loc.c_str(), rv);
        goto done;
    }

    if ( (rv = _sw_download_begin(guid, ver, handle)) ) {
        LOG_ERROR("_sw_download_begin(%s, %s) failed - %d", guid.c_str(),
            ver.c_str(), rv);
        goto done;
    }

    if ( (rv = _sw_progress_poll(handle, false)) ) {
        LOG_ERROR("_sw_progress_poll() failed - %d", rv);
        goto done;
    }

    if ( (rv = _sw_download_end(handle, file_loc)) ) {
        LOG_ERROR("_sw_download_end() failed - %d", rv);
        goto done;
    }

    if (test_handle) {
        // Verify that the handle is no good at this point
        if ( (rv = _sw_progress_poll(handle, false)) == 0 ) {
            LOG_ERROR("_sw_progress_poll worked on a stale handle!");
            rv = -1;
            goto done;
        }
    }

    rv = 0;
done:
    if (target != NULL)
        delete target;
    return rv;
}

static int _download_app_poll(const std::string& guid, const std::string& ver,
    const std::string& file_loc)
{
    return(_download_app_poll_int(guid, ver, file_loc, false));
}

static int _download_app_poll_test(const std::string& guid, const std::string& ver,
    const std::string& file_loc)
{
    return(_download_app_poll_int(guid, ver, file_loc, true));
}

static int _sw_progress_event(u64 ev_handle, u64 handle, bool is_canceled)
{
    int rv;
    u64 cnt, size;
    const int timeout_msec = 10000;

    for ( ;; ) {
        ccd::EventsDequeueInput req;
        ccd::EventsDequeueOutput resp;
        ccd::EventSWUpdateProgress progress;

        // This thing has to set a timeout or it won't block.
        req.set_queue_handle(ev_handle);
        req.set_timeout(timeout_msec);

        if ( (rv = CCDIEventsDequeue(req, resp)) ) {
            LOG_ERROR("Failed Event Dequeue- %d", rv);
            goto done;
        }

        if ( resp.events().size() == 0 ) {
            LOG_ERROR("Returned with no events?");
            continue;
        }

        for( int i = 0 ; i < resp.events().size() ; i++ ) {
            if ( ! resp.events(i).has_sw_update_progress() ) {
                LOG_INFO("Skipping event %d", i);
                continue;
            }
            progress = resp.events(i).sw_update_progress();
            switch (progress.state()) {
            case ccd::SWU_DLSTATE_DONE:
                size = progress.total_transfer_size();
                LOG_INFO("Download complete %lld bytes.", size);
                if ( is_canceled ) {
                    rv = -1;
                    goto done;
                }
                goto download_done;
            case ccd::SWU_DLSTATE_IN_PROGRESS: {
                cnt = progress.bytes_transferred_cnt();
                size = progress.total_transfer_size();
                        
                LOG_INFO(FMTu64 " of " FMTu64 " bytes downloaded.",
                    cnt, size);
                break;
                }
            case ccd::SWU_DLSTATE_FAILED:
                LOG_INFO("Download failed");
                rv = -1;
                goto done;
            case ccd::SWU_DLSTATE_STOPPED:
                LOG_INFO("Download stopped");
                rv = -1;
                goto done;
            case ccd::SWU_DLSTATE_CANCELED:
                LOG_INFO("Download canceled");
                if ( !is_canceled ) {
                    rv = -1;
                    goto done;
                }
                goto download_done;
            default:
                LOG_ERROR("Unknown state - %d",
                    progress.state());
                rv = -1;
                goto done;
            }
        }
    }

download_done:
    rv = 0;
done:
    return rv;
}


static int _download_app_event(u64 ev_handle, const std::string& guid,
    const std::string& ver, const std::string& file_loc)
{
    int rv;
    u64 handle;
    TargetDevice *target = NULL;

    target = getTargetDevice();
    if (target == NULL) {
        LOG_ERROR("getTargetDevice failed!");
        return -1;
    }

    rv = target->deleteFile(file_loc);
    if (rv != 0 && rv != VPL_ERR_NOT_EXIST && rv != VPL_ERR_NOENT) {
        LOG_ERROR("Failed cleaning up pre-existing file %s %d",
            file_loc.c_str(), rv);
        goto done;
    }

    if ( (rv = _sw_download_begin(guid, ver, handle)) ) {
        LOG_ERROR("_sw_download_begin(%s, %s) failed - %d", guid.c_str(),
            ver.c_str(), rv);
        goto done;
    }

    if ( (rv = _sw_progress_event(ev_handle, handle, false)) ) {
        LOG_ERROR("_sw_progress_event() failed - %d", rv);
        goto done;
    }

    if ( (rv = _sw_download_end(handle, file_loc)) ) {
        LOG_ERROR("_sw_download_end() failed - %d", rv);
        goto done;
    }

    rv = 0;
done:
    if (target != NULL)
        delete target;
    return rv;
}

static int _download_app_event_cancel(u64 ev_handle, const std::string& guid,
    const std::string& ver, const std::string& file_loc)
{
    int rv;
    u64 handle;
    u64 new_handle;
    TargetDevice *target = NULL;

    target = getTargetDevice();
    if (target == NULL) {
        LOG_ERROR("getTargetDevice failed!");
        return -1;
    }

    rv = target->deleteFile(file_loc);
    if (rv != 0 && rv != VPL_ERR_NOT_EXIST && rv != VPL_ERR_NOENT) {
        LOG_ERROR("Failed cleaning up pre-existing file %s %d",
            file_loc.c_str(), rv);
        goto done;
    }

    if ( (rv = _sw_download_begin(guid, ver, handle)) ) {
        LOG_ERROR("_sw_download_begin(%s, %s) failed - %d", guid.c_str(),
            ver.c_str(), rv);
        goto done;
    }

    if ( (rv = _sw_download_cancel(handle)) ) {
        LOG_ERROR("_sw_download_cancel("FMTx64") - %d", handle, rv);
        goto done;
    }

    if ( (rv = _sw_progress_event(ev_handle, handle, true)) ) {
        LOG_ERROR("_sw_progress_event() failed - %d", rv);
        goto done;
    }

    if ( (rv = _sw_download_begin(guid, ver, new_handle)) ) {
        LOG_ERROR("_sw_download_begin(%s, %s) failed - %d", guid.c_str(),
            ver.c_str(), rv);
        goto done;
    }

    if ( new_handle != handle ) {
        LOG_ERROR("Received different handle for canceled, unreaped download");
        rv = -1;
        goto done;
    }

    // Make sure we get an event from the previous open telling us
    // this has been canceled.
    if ( (rv = _sw_progress_event(ev_handle, handle, true)) ) {
        LOG_ERROR("_sw_progress_event() failed - %d", rv);
        goto done;
    }

    if ( (rv = _sw_download_end(handle, file_loc)) ) {
        LOG_ERROR("_sw_download_end() failed - %d", rv);
        goto done;
    }

    rv = 0;
done:
    if (target != NULL)
        delete target;
    return rv;
}

static int _event_queue_create(u64& handle)
{
    int rv;
    ccd::EventsCreateQueueInput req;
    ccd::EventsCreateQueueOutput resp;

    if ( (rv = CCDIEventsCreateQueue(req, resp)) ) {
        LOG_ERROR("Failed CCDIEventsQueueCreate - %d", rv);
        goto done;
    }

    handle = resp.queue_handle();

    rv = 0;
done:
    return rv;
}

static void _sw_mask2str(u64 mask, std::string &rv)
{
    rv = "";
    if (mask & ccd::SW_UPDATE_BIT_APP_CRITICAL) {
        rv.append(" APP_CRIT");
    }
    if (mask & ccd::SW_UPDATE_BIT_APP_OPTIONAL) {
        rv.append(" APP_OPT");
    }
    if (mask & ccd::SW_UPDATE_BIT_CCD_CRITICAL) {
        rv.append(" CCD_CRIT");
    }
    if (mask & ccd::SW_UPDATE_BIT_CCD_NEEDED) {
        rv.append(" CCD_NEEDED");
    }
}

static int _neg_tests()
{
    int rv;
    u64 handle;
    std::string app_ver;
    std::string ccd_ver;
    u64 update_mask, exp_mask;
    u64 ret_size;

    LOG_INFO("Try to download invalid apps");
    if ( (rv = _sw_download_begin(_td->app_guid, _td->app_ver, handle)) == 0 ) {
        LOG_ERROR("Successfully began download of bad version (%s,%s)",
            _td->app_guid.c_str(), _td->app_ver.c_str());
        rv = -1;
        goto done;
    }

    // Try to pull down something unknown
    if ( (rv = _sw_download_begin("foo bar", _td->app_ver, handle)) == 0 ) {
        LOG_ERROR("Successfully began download of invalid app");
        rv = -2;
        goto done;
    }

    LOG_INFO("Try to perform checks on invalid version strings");
    // Pass in an invalid version string
    if ( (rv = _sw_update_check(_td->app_guid, "1.1", app_ver, ccd_ver,
            update_mask, ret_size)) != 0 ) {
        LOG_ERROR("failed performed update check on invalid version 1");
        rv = -3;
        goto done;
    }
    exp_mask = ccd::SW_UPDATE_BIT_APP_CRITICAL |
        ccd::SW_UPDATE_BIT_CCD_CRITICAL;
    if (update_mask != exp_mask) {
        LOG_ERROR("bad mask " FMTx64 " not " FMTx64,  update_mask, exp_mask);
        rv = -4;
        goto done;

    }

    // part two of invalid version string
    if ( (rv = _sw_update_check(_td->app_guid, "1.1.1.1", app_ver, ccd_ver,
            update_mask, ret_size)) != 0 ) {
        LOG_ERROR("Successfully performed update check on invalid version 2");
        rv = -4;
        goto done;
    }
    if (update_mask != exp_mask) {
        LOG_ERROR("bad mask " FMTx64 " not " FMTx64, update_mask, exp_mask);
        rv = -5;
        goto done;
    }
    LOG_INFO("negative tests done");

    rv = 0;
done:
    return rv;
}


static int _sw_update_mask_tests(void)
{
    int rv, i;
    std::string new_app_ver;
    std::string app_ver;
    std::string ccd_ver;
    u64 ret_size, update_mask, exp_mask;

    LOG_INFO("Now performing update mask tests");

    // Pass in an invalid version string
    // force critical ccd and critical app update
    if ( (rv = _sw_update_check(_td->app_guid, _td->app_ver, app_ver, 
        ccd_ver, update_mask, ret_size)) ) {
        LOG_ERROR("1: Failed sw update check - %d", rv);
        goto done;
    }
    exp_mask = ccd::SW_UPDATE_BIT_APP_CRITICAL |
        ccd::SW_UPDATE_BIT_CCD_CRITICAL;
    if ( update_mask != exp_mask ) {
        LOG_ERROR("1: Unexpected mask got "FMTx64" want "FMTx64, update_mask,
            exp_mask);
        rv = -1;
        goto done;
    }

    // The app versions limit our testing ability
    // force app to no update and ccd critical
    new_app_ver = app_ver;
    if ( (rv = _sw_update_check(_td->app_guid, new_app_ver, app_ver, ccd_ver,
            update_mask, ret_size)) ) {
        LOG_ERROR("2: Failed sw update check - %d", rv);
        goto done;
    }
    exp_mask = ccd::SW_UPDATE_BIT_CCD_CRITICAL;
    if ( update_mask != exp_mask ) {
        LOG_ERROR("2: Unexpected mask got "FMTx64" want "FMTx64, update_mask,
            exp_mask);
        rv = -2;
        goto done;
    }

    // force app to no update and ccd needed
    if ( (rv = _set_ccd_version(_td->ccd_guid, "1.1.2")) ) {
        LOG_ERROR("3: Failed setting the ccd version = %d", rv);
        goto done;
    }
    if ( (rv = _sw_update_check(_td->ccd_guid, "1.1.1", app_ver, ccd_ver,
            update_mask, ret_size)) ) {
        LOG_ERROR("3: Failed sw update check - %d", rv);
        goto done;
    }
    exp_mask = ccd::SW_UPDATE_BIT_APP_OPTIONAL;
    if ( update_mask != exp_mask ) {
        LOG_ERROR("3: Unexpected mask got "FMTx64" want "FMTx64, update_mask,
            exp_mask);
        rv = -2;
        goto done;
    }

    // force app and ccd to no update needed
    if ( (rv = _set_ccd_version(_td->ccd_guid, "1.1.2")) ) {
        LOG_ERROR("4: Failed setting the ccd version = %d", rv);
        goto done;
    }
    if ( (rv = _sw_update_check(_td->app_guid, new_app_ver, app_ver, ccd_ver,
            update_mask, ret_size)) ) {
        LOG_ERROR("4: Failed sw update check - %d", rv);
        goto done;
    }
    exp_mask = 0;
    if ( update_mask != exp_mask ) {
        LOG_ERROR("4: Unexpected mask got "FMTx64" want "FMTx64, update_mask,
            exp_mask);
        rv = -2;
        goto done;
    }

    for (i = 0; i < ARRAY_LEN(swu_vertests); i++) {
        struct swu_vertest *p = swu_vertests + i;
        std::string maskstr, exp_maskstr;
        LOG_INFO("%d: version test: %s", i, p->test_desc.c_str());
        rv = _set_ccd_version(p->ccd_guid, p->ccd_ver);
        if (rv != 0) {
            LOG_ERROR("5: Failed setting the ccd version (%s,%s) -> %d, ", 
                p->ccd_guid.c_str(), p->ccd_ver.c_str(), rv);
            goto done;
        }
        if ( (rv = _sw_update_check(p->app_guid, p->app_ver, app_ver, 
            ccd_ver, update_mask, ret_size)) != p->exp_rv) {
            LOG_ERROR("6: sw_update_check %d not %d", rv, p->exp_rv);
            rv = -7;
            goto done;
        }
        if (p->exp_mask != update_mask) {
            _sw_mask2str(p->exp_mask, exp_maskstr);
            _sw_mask2str(update_mask, maskstr);
            LOG_ERROR("7: version test %s: mask '%s' not '%s' for app (%s,%s) ccd(%s,%s) current (%s,%s)",
                p->test_desc.c_str(), maskstr.c_str(), exp_maskstr.c_str(),
                p->app_guid.c_str(), p->app_ver.c_str(), 
                p->ccd_guid.c_str(), p->ccd_ver.c_str(),
                ccd_ver.c_str(), app_ver.c_str());
            rv = -8;
            goto done;

        }
    }
    LOG_INFO("update mask tests done");

    rv = 0;
done:
    return rv;
}

int swup_diag(int guid_set, std::string strDestdir)
{
    int rv;
    u64 update_mask;
    u64 ev_handle = -1;
    u64 ret_size;
    std::string app_ver, ccd_ver;
    std::string spacedir;
    TargetDevice *target = NULL;
    std::string separator;

    target = getTargetDevice();
    if (target == NULL) {
        LOG_ERROR("getTargetDevice failed!");
        return -1;
    }

    if (mapGuidFile.size() == 0) {
        init_mapGuidFile();
    }

    if (guid_set < 0 || guid_set >= ARRAY_LEN(swu_testcases)) {
        LOG_ERROR("guid_set out of range %d", guid_set);
        rv = -1;
        goto done;
    }
    _td = &swu_testcases[guid_set];

    setDebugLevel(LOG_LEVEL_INFO);
    rv = target->getDirectorySeparator(separator);
    if (rv != 0) {
        LOG_ERROR("getDirectorySeparator failed");
        goto done;
    }
    spacedir = strDestdir.append("a dir").append(separator);

    rv = target->createDir(spacedir, 0777);
    if (rv != VPL_OK) {
        LOG_ERROR("Failed to create directory %s, rv = %d", spacedir.c_str(), rv);
        goto done;
    }
    LOG_ALWAYS("swup_diag %d destination %s", guid_set, strDestdir.c_str());

    // 1st: tell CCD who it is
    if ( (rv = _set_ccd_version(_td->ccd_guid, _td->ccd_ver)) ) {
        LOG_ERROR("_set_ccd_version() failed - %d", rv);
        goto done;
    }

    if ( (rv = _sw_update_check(_td->app_guid, _td->app_ver, app_ver, ccd_ver,
            update_mask, ret_size)) ) {
        LOG_ERROR("_sw_update_check() failed - %d", rv);
        goto done;
    }

    LOG_INFO(" my ccd ver: %s", _td->ccd_ver.c_str());
    LOG_INFO("swu ccd ver: %s", ccd_ver.c_str());
    LOG_INFO(" my app ver: %s", _td->app_ver.c_str());
    LOG_INFO("swu app ver: %s", app_ver.c_str());
    LOG_INFO("update mask: "FMTx64, update_mask);

    if ( (rv = _download_app_poll_test(_td->app_guid, app_ver,
        strDestdir + "foo")) ) {
        LOG_ERROR("_download_app_poll(%s, %s) failed - %d",
            _td->app_guid.c_str(), _td->app_ver.c_str(), rv);
        goto done;
    }

    // Do it again, and this time event driven
    if ( (rv = _event_queue_create(ev_handle)) ) {
        LOG_ERROR("_event_queue_create() failed - %d", rv);
        goto done;
    }

    // Pull down app this way
    if ( (rv = _download_app_event(ev_handle, _td->app_guid, app_ver,
            spacedir + "foo-ev")) ) {
        LOG_ERROR("_download_app_event(%s, %s) failed - %d",
            _td->app_guid.c_str(), _td->app_ver.c_str(), rv);
        goto done;
    }

       // Pull down app event driven and cancel it.
    if ( (rv = _download_app_event_cancel(ev_handle, _td->app_guid, app_ver,
            strDestdir + "foo-ev")) ) {
        LOG_ERROR("_download_app_event(%s, %s) failed - %d",
            _td->app_guid.c_str(), app_ver.c_str(), rv);
        goto done;
    }

    //Download again after previous cancel
    if ( (rv = _download_app_poll_test(_td->app_guid, app_ver,
        strDestdir + "foo")) ) {
        LOG_ERROR("_download_app_poll(%s, %s) failed - %d",
            _td->app_guid.c_str(), _td->app_ver.c_str(), rv);
        goto done;
    }

    // Now, peform some negative tests
    if ( (rv = _neg_tests()) ) {
        LOG_ERROR("_neg_tests() failed - %d", rv);
        goto done;
    }

    // Now perform some tests on the update mask
    if ( (rv = _sw_update_mask_tests()) ) {
        LOG_ERROR("_sw_update_mask_tests() failed - %d", rv);
        goto done;
    }

done:
    if (target != NULL)
        delete target;
    resetDebugLevel();
    return rv;
}

static void
_line_split(const std::string& line, char c, StrVec& split)
{
    size_t s_pos, t_pos;
    int i;

    split.clear();
    s_pos = 0;
    for( i = 0 ; ; i++ ) {
        t_pos = line.find(c, s_pos);
        split.push_back(line.substr(s_pos, (t_pos - s_pos)));
        if ( t_pos == std::string::npos ) {
            break;
        }
        s_pos = t_pos + 1;
    }
}


// Returns the number of download failures encountered.
// 
// guids is a comma separated list of GUIDs to attempt to download to
// the directory in destdir which include a trailing directory separator.
//
// A default list is available as swu_default_guids.
//
// The downloaded filenames are of the form <GUID>_v<ver>.zip though
// mapGuidFile can be used to override this.
int swup_fetch_all(const std::string& strGuids, const std::string& destdir,
    const std::string& app_ver_in, const bool check_only,
    const bool fetch_polled, const bool android_guids, const bool refresh)
{
    StrVec strvecGuids;
    unsigned int i;
    int rv;
    std::string ccd_ver, app_ver;
    u64 update_mask;
    StrVec failures, successes;
    u64 ret_size, ev_handle;
    VPLTime_t time_begin, time_spent;
    std::string app_ver_cur;
    std::string separator;

    TargetDevice *target = getTargetDevice();
    if (target == NULL) {
        LOG_ERROR("getTargetDevice failed!");
        return -1;
    }

    if (mapGuidFile.size() == 0) {
        init_mapGuidFile();
    }

    time_begin = VPLTime_GetTimeStamp();
    
    _td = &swu_testcases[android_guids ? 1 : 0];

    std::string guids;
    if (strGuids.size() > 0) {
        guids = strGuids;
    } else {
        guids = _td->test_guids;
    }
    _line_split(guids, ',', strvecGuids);

    rv = target->createDir(destdir, 0777);
    if (rv != VPL_OK) {
        goto done;
    }

    rv = target->getDirectorySeparator(separator);
    if (rv != 0) {
        LOG_ERROR("getDirectorySeparator failed");
        goto done;
    }

    if ( !fetch_polled && !check_only) {
        if ( (rv = _event_queue_create(ev_handle)) ) {
            LOG_ERROR("_event_queue_create() failed - %d", rv);
            goto done;
        }
    }

    _td = &swu_testcases[android_guids ? 1 : 0];
    LOG_INFO("Fetching Apps %s to %s", guids.c_str(), destdir.c_str());
    _set_ccd_version(_td->ccd_guid, _td->ccd_ver);
    for (i = 0; i < strvecGuids.size(); i++) {
        std::string guid = strvecGuids[i];
        // There's no rhyme or reason to version strings. Try to pull it
        // from the user, the mapGuidVers array or try to default it.
        // If the components don't match you'll just be given a forced
        // update mask.
        if (app_ver_in.size() ) {
            app_ver_cur = app_ver_in;
        }
        else {
            std::map<std::string, int>::iterator it;
            if ( (it = mapGuidVers.find(guid)) == mapGuidVers.end() ) {
                app_ver_cur = _td->ccd_ver;
            }
            // XXX - replace this with an array for 1-5 version components
            // or perhaps index into a single string of 5 components.
            else if (it->second == 3) {
                app_ver_cur = "0.0.0"; 
            }
            else {
                app_ver_cur = "0.0.0.0"; 
            }
        }

        if ( (rv = _sw_update_check(guid, app_ver_cur, true, refresh,
            /*out*/app_ver, /*out*/ccd_ver, /*out*/update_mask, /*out*/ret_size)) ) {
            LOG_ERROR("_sw_update_check(%s) failed - %d", guid.c_str(), rv);
            failures.push_back(guid);
            continue;
        }
        LOG_ALWAYS("GUID %s (%s) current version %s download size %d", guid.c_str(), 
            getNameFromGuid(guid).c_str(), app_ver.c_str(), (int)ret_size);
        if ( check_only ) {
            successes.push_back(guid + " version " + app_ver);
        } else {
            // Note that destdir will include the directory separator at the end.
            std::string destfilepath = destdir + getFileFromGuid(guid, app_ver);
            if ( fetch_polled ) {
                rv = _download_app_poll(guid, app_ver, destfilepath);
            } else {
                rv = _download_app_event(ev_handle, guid, app_ver, destfilepath);
            }

            if ( rv ) {
                LOG_ERROR("_download_app(%s, %s, %d) failed - %d", guid.c_str(), app_ver.c_str(), fetch_polled, rv);
                failures.push_back(guid);
                continue;
            } else {
                    LOG_INFO("%s %s downloaded version %s to %s ("FMTu64 " bytes)", 
                    guid.c_str(), fetch_polled ? "polled" : "event", app_ver.c_str(),
                    destfilepath.c_str(), ret_size);
                successes.push_back("Downloaded " + destfilepath);
            }
        }
    }

done:
    if (target != NULL)
        delete target;
    time_spent = VPLTime_GetTimeStamp() - time_begin;
    if (failures.size() > 0) {
        LOG_ERROR("*** Downloaded %d files %d failures*** %f seconds", 
            successes.size(), failures.size(), time_spent / 1000000.0);
        for (i = 0; i < failures.size(); i++) {
            LOG_ERROR("Failed file %s", failures[i].c_str());
        }
    } else {
        LOG_ALWAYS("*** Download summary: %d successes *** %f seconds",
            successes.size(), time_spent / 1000000.0);
    }
    for (i = 0; i < successes.size(); i++) {
        LOG_ALWAYS("%s", successes[i].c_str());
    }
    return failures.size();
}

