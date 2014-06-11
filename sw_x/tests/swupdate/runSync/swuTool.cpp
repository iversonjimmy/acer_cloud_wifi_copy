// vim: ts=4:sw=4:expandtab

#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <ccdi.hpp>
#include <gvm_errors.h>
#include <log.h>
#include <sys/types.h>
#include <sys/file.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/errno.h>

#include <string>

#include <ftw.h>
#include <vplex_file.h>
#include <vpl_fs.h>

using namespace std;
static string _ccd_guid = "sw-test-ccd";
static string _ccd_ver = "0.0.1";
static bool _event_driven = false;
static bool _cancel_download = false;
static bool _just_check = false;

static char *progNm;

#define ARRAY_LEN(x)    ((sizeof(x))/(sizeof(x[0])))

static void
usage(const char *msg)
{
    LOG_ERROR("%s", msg);
    LOG_ERROR("usage: %s <flags>", progNm);
    LOG_ERROR("  -h             # usage info");
    LOG_ERROR("  -g <app_guid>  # app guid");
    LOG_ERROR("  -a <app_ver>   # app version");
    LOG_ERROR("  -G <ccd_guid>  # ccd guid");
    LOG_ERROR("  -A <ccd_ver>   # app version");
    LOG_ERROR("  -e             # event driven [polled]");
    LOG_ERROR("  -o <file>      # output file");
    LOG_ERROR("  -c             # cancel download [no cancel]");
    LOG_ERROR("  -C             # Just perform update check [do download]");

    exit(1);
}

static int _set_ccd_version(const string& guid, const string& ver)
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

static int _sw_update_check(const string& app_guid, const string& app_ver,
    string& ret_app_ver, string& ret_ccd_ver, u64& ret_mask, u64& ret_size)
{
    int rv;
    ccd::SWUpdateCheckInput req;
    ccd::SWUpdateCheckOutput resp;

    req.set_app_guid(app_guid);
    req.set_app_version(app_ver);

    if ( (rv = CCDISWUpdateCheck(req, resp)) ) {
        LOG_ERROR("Failed SWUpdateCheck() - %d", rv);
        goto done;
    }

    ret_app_ver = resp.latest_app_version();
    ret_ccd_ver = resp.latest_ccd_version();
    ret_mask = resp.update_mask();
    ret_size = resp.app_size();

    rv = 0;
done:
    return rv;
}

static int _sw_download_begin(const string& guid, const string& ver,
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

static int _sw_download_end(u64 handle, const string& file_loc)
{
    int rv;
    ccd::SWUpdateEndDownloadInput req;

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

static int _download_app_poll(const string& guid, const string& ver,
    const string& file_loc)
{
    int rv;
    u64 handle = 0;

    if ( (rv = unlink(file_loc.c_str())) && errno != ENOENT) {
        LOG_ERROR("cleaning up %s %d errno %d", file_loc.c_str(), rv, errno);
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

    // Verify that the handle is no good at this point
    if ( (rv = _sw_progress_poll(handle, false)) == 0 ) {
        LOG_ERROR("_sw_progress_poll worked on a stale handle!");
        rv = -1;
        goto done;
    } else {
        LOG_INFO("expected failure on sw_progress_poll seen");
    }


    rv = 0;
done:
    return rv;
}

static int _download_app_poll_cancel(const string& guid, const string& ver,
    const string& file_loc)
{
    int rv;
    u64 handle = 0;

    if ( (rv = unlink(file_loc.c_str())) && errno != ENOENT) {
        LOG_ERROR("cleaning up %s %d errno %d", file_loc.c_str(), rv, errno);
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


    if ( (rv = _sw_progress_poll(handle, true)) ) {
        LOG_ERROR("_sw_progress_poll() failed - %d", rv);
        goto done;
    }

    if ( (rv = _sw_download_end(handle, file_loc)) ) {
        LOG_ERROR("_sw_download_end() failed - %d", rv);
        goto done;
    }

    // Verify that the handle is no good at this point
    if ( (rv = _sw_progress_poll(handle, false)) == 0 ) {
        LOG_ERROR("_sw_progress_poll worked on a stale handle!");
        rv = -1;
        goto done;
    } else {
        LOG_INFO("expected failure on sw_progress_poll seen");
    }


    rv = 0;
done:
    return rv;
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

static int _download_app_event(u64 ev_handle, const string& guid,
    const string& ver, const string& file_loc)
{
    int rv;
    u64 handle;

    if ( (rv = unlink(file_loc.c_str())) && errno != ENOENT) {
        LOG_ERROR("Failed cleaning up pre-existing file %s %d errno %d",
            file_loc.c_str(), rv, errno);
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
    return rv;
}

static int _download_app_event_cancel(u64 ev_handle, const string& guid,
    const string& ver, const string& file_loc)
{
    int rv;
    u64 handle;
    u64 new_handle;

    if ( (rv = unlink(file_loc.c_str())) && errno != ENOENT) {
        LOG_ERROR("Failed cleaning up pre-existing file %s %d errno %d",
            file_loc.c_str(), rv, errno);
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

int
main(int ac, char *av[])
{
    // extern char *optarg;
    int c, errflg;
    int rv;
    u64 ev_handle = 0;
    string cur_app_ver;
    string cur_ccd_ver;
    u64 update_mask = 0;
    u64 app_size = 0;
    string app_guid;
    string app_ver;
    string out_file = "/tmp/foo";

    progNm = av[0];

    errflg = 0;
    while ((c = getopt(ac, av, "hg:a:G:A:eo:cC")) != EOF) {
        switch(c) {
        case 'h': {
            usage("This help message");
            break;
        }
        case 'g': {
            app_guid = optarg;
            break;
        }
        case 'G': {
            _ccd_guid = optarg;
            break;
        }
        case 'a': {
            app_ver = optarg;
            break;
        }
        case 'A': {
            _ccd_ver = optarg;
            break;
        }
        case 'e': {
            _event_driven = true;
            break;
        }
        case 'o': {
            out_file = optarg;
            break;
        }
        case 'c': {
            _cancel_download = true;
            break;
        }
        case 'C': {
            _just_check = true;
            break;
        }
        case '?':
        default: {
            errflg++;
            break;
        }
        }
    }

    LOGInit("swu_tool", NULL);
    LOGSetMax(0); // No limit
    LOG_ENABLE_LEVEL(LOG_LEVEL_TRACE);

    if (errflg) {
        usage("invalid args");
    }

    // 1st: tell CCD who it is
    LOG_INFO("_set_ccd_version(%s, %s)", _ccd_guid.c_str(), _ccd_ver.c_str());
     if ( (rv = _set_ccd_version(_ccd_guid, _ccd_ver)) ) {
         LOG_ERROR("_set_ccd_version() failed - %d", rv);
         goto done;
     }

    // Perform the update check
    if ( (rv = _sw_update_check(app_guid, app_ver, cur_app_ver, cur_ccd_ver,
            update_mask, app_size)) ) {
        LOG_ERROR("_sw_update_check() failed - %d", rv);
        goto done;
    }

    LOG_INFO(" my ccd ver: %s", _ccd_ver.c_str());
    LOG_INFO("swu ccd ver: %s", cur_ccd_ver.c_str());
    LOG_INFO(" my app ver: %s", app_ver.c_str());
    LOG_INFO("swu app ver: %s", cur_app_ver.c_str());
    LOG_INFO("update mask: "FMTx64, update_mask);
    LOG_INFO("   app size: "FMTx64, app_size);

    if ( _just_check ) {
        LOG_INFO("No download performed");
        goto pass;
    }

    if ( _event_driven ) {
        // Do it again, and this time event driven
        if ( (rv = _event_queue_create(ev_handle)) ) {
            LOG_ERROR("_event_queue_create() failed - %d", rv);
            goto done;
        }

        if ( _cancel_download ) {
            // Pull down app event driven and cancel it.
            if ( (rv = _download_app_event_cancel(ev_handle, app_guid, 
                    cur_app_ver, out_file)) ) {
                LOG_ERROR("_download_app_event(%s, %s) failed - %d",
                    app_guid.c_str(), cur_app_ver.c_str(), rv);
                goto done;
            }

        }
        else {
            // Pull down app event driven
            if ( (rv = _download_app_event(ev_handle, app_guid, cur_app_ver,
                    out_file)) ) {
                LOG_ERROR("_download_app_event(%s, %s) failed - %d",
                    app_guid.c_str(), cur_app_ver.c_str(), rv);
                goto done;
            }
        }

        goto pass;
    }


    // Handled the polled approach
    if ( _cancel_download ) {
        if ( (rv = _download_app_poll_cancel(app_guid, cur_app_ver,
                out_file)) ) {
            LOG_ERROR("_download_app_poll_cancel(%s, %s) failed - %d",
                app_guid.c_str(), cur_app_ver.c_str(), rv);
            goto done;
        }
        goto pass;
    }

    if ( (rv = _download_app_poll(app_guid, cur_app_ver, out_file)) ) {
        LOG_ERROR("_download_app_poll(%s, %s) failed - %d",
            app_guid.c_str(), cur_app_ver.c_str(), rv);
        goto done;
    }

pass:
    LOG_INFO("Test PASSED");

    rv = 0;

done:
    return rv;
}
