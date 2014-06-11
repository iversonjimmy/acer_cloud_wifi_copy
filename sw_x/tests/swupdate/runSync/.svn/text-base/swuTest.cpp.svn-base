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
int _fileCp(const std::string& src, const std::string& dst);

// TODO: Use a more appropriate structure for representing GUIDs
#define WIN_ACERCLOUD_GUID      "a5ad0b17-f34d-49be-a157-c8b3d52acd13"
#define WIN_CLEARFIPHOTO_GUID   "b5ad89f2-03d3-4206-8487-018298007dd0"
#define WIN_CLEARFIMEDIA_GUID   "e9af1707-3f3a-49e2-8345-4f2d629d0876"
#define WIN_ACERCLOUDDOCS_GUID  "ca4fe8b0-298c-4e5d-a486-f33b126d6a0a"

#define AND_ACERCLOUD_GUID      "951D885F-BD9F-4F74-A013-E296B194F46B"
#define AND_CLEARFIPHOTO_GUID   "765C60BE-95A4-46C7-AB56-38104B4D438A"
#define AND_CLEARFIVIDEO_GUID   "D81B5307-CCFE-404F-BEA5-2D9C7B3894AA"
#define AND_CLEARFIMUSIC_GUID   "0CA4401A-BAC6-4280-932D-EBDCC89C5BB0"
#define AND_ACERCLOUDDOCS_GUID  "74E844FA-2038-48B7-A9A4-61936AF0149D"

static std::string swu_default_guids = WIN_ACERCLOUD_GUID ","
	WIN_CLEARFIPHOTO_GUID ","
	WIN_CLEARFIMEDIA_GUID ","
	WIN_ACERCLOUDDOCS_GUID ;

static std::string swu_android_guids = AND_ACERCLOUD_GUID ","
    AND_CLEARFIPHOTO_GUID ","
    AND_CLEARFIVIDEO_GUID ","
    AND_CLEARFIMUSIC_GUID ","
    AND_ACERCLOUDDOCS_GUID;


static struct swu_testdata {
     std::string    ccd_guid;
     std::string    ccd_ver;
     std::string    app_guid;
     std::string    app_ver;
     std::string    rel_app_ver;

     std::string    app_longver;
     std::string    app_shortver;
     std::string    app_overver;
     std::string    ccd_needupver;
     std::string    ccd_noupver;
     std::string    ccd_opupver;
     std::string    ccd_overver;
     std::string    test_guids;
} swu_testcases[]= {
    { 
    // Implicit here are the following versions in 
    // ccd version = 1.1.2, min version = 1.1.1
    // app version = 1.1.3, min version = 1.1.3 , ccdver 
    "test-dxwin-swu-ccd", "0.0.1", "test-dxwin-swu-01", "1.1.0", 
    "1.1.3", "1.1",
    "1.1.1.1", "2.0.0", "1.1.0", "1.1.2", "1.1.1", "1.1.3", swu_default_guids
    },
    { 
    // Implicit here are the following versions in 
    // ccd version = 1.1.2.1, min version = 1.1.1.1, CCDMinVersion 1.1.1.1
    // app version = 1.1.3.1, min version = 1.1.3.1 , ccdMinVer 1.1.1.1
    "test-dxand-swu-ccd", "0.0.0.1", "test-dxand-swu-001", "1.1.0.0", 
    "1.1.3.1", "1.1",
    "1.1.1", "2.0.0.0", "1.1.1.0", "1.1.2.1", "1.1.1.1", "1.1.3.1",
    swu_android_guids
    }
};

struct swu_versSizes_s {
    const char *app_guid;
    const char *app_ver;
    const char *app_low;
    const char *app_high;
    const char *app_inv;
};
typedef struct swu_versSizes_s swu_versSizes_t;

swu_versSizes_t _verSizes[] = {
    {"test-swu-app-v1",         "1",         "0",         "2",         "1.1"},
    {"test-swu-app-v2",       "2.2",       "1.1",       "3.3",       "2.2.2"},
    {"test-swu-app-v3",     "3.3.3",     "2.2.2",     "4.4.4",     "3.3.3.3"},
    {"test-swu-app-v4",   "4.4.4.4",   "3.3.3.3",   "5.5.5.5",   "4.4.4.4.4"},
    {"test-swu-app-v5", "5.5.5.5.5", "4.4.4.4.4", "6.6.6.6.6", "5.5.5.5.5.5"},
};


// app is 1.1.3
struct version_walk_s {
    const char* ver;
    u64         mask;
};
typedef struct version_walk_s version_walk_t;

// windows app ver/min 1.1.3
static version_walk_t _win_vwalk[] = {
    {"1.1.2", ccd::SW_UPDATE_BIT_APP_CRITICAL},
    {"1.1.003", 0},
    {"1.1.004", ccd::SW_UPDATE_BIT_APP_CRITICAL},
    {"1.0.3", ccd::SW_UPDATE_BIT_APP_CRITICAL},
    {"1.01.3", 0},
    {"1.002.3", ccd::SW_UPDATE_BIT_APP_CRITICAL},
    {"0.1.3", ccd::SW_UPDATE_BIT_APP_CRITICAL},
    {"01.01.03", 0},
    {"2.1.3", ccd::SW_UPDATE_BIT_APP_CRITICAL},
};

// windows app ver/min 1.1.3.1
static version_walk_t _and_vwalk[] = {
    {"1.1.3.0", ccd::SW_UPDATE_BIT_APP_CRITICAL},
    {"1.1.3.001", 0},
    {"1.1.3.002", ccd::SW_UPDATE_BIT_APP_CRITICAL},
    {"1.1.2.1", ccd::SW_UPDATE_BIT_APP_CRITICAL},
    {"1.1.003.1", 0},
    {"1.1.004.1", ccd::SW_UPDATE_BIT_APP_CRITICAL},
    {"1.0.3.1", ccd::SW_UPDATE_BIT_APP_CRITICAL},
    {"1.01.3.1", 0},
    {"1.002.3.1", ccd::SW_UPDATE_BIT_APP_CRITICAL},
    {"0.1.3.1", ccd::SW_UPDATE_BIT_APP_CRITICAL},
    {"01.01.03.01", 0},
    {"2.1.3.1", ccd::SW_UPDATE_BIT_APP_CRITICAL},
};

static struct swu_testdata *_td;

// Location where swupdate caches the download
static string _cache_loc;

static string _ccd_root;
static bool _swu_canceled;

static char *progNm;

#define ARRAY_LEN(x)    ((sizeof(x))/(sizeof(x[0])))

static void
usage(const char *msg)
{
    LOG_ERROR("%s", msg);
    LOG_ERROR("usage: %s <flags>", progNm);
    LOG_ERROR("  -h           # usage info");
    LOG_ERROR("  -r <ccdroot>");

    exit(1);
}

static void
log_result(bool passfail, const char *tcname)
{
    const char *pfstring = passfail ? "PASS" : "FAIL";
    printf("TC_RESULT=%s ;;; TC_NAME=%s\n", pfstring, tcname);
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

    _swu_canceled = false;

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
                _swu_canceled = true;
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


static vector<string> _find_file_matches_vec;
static string _find_file_matches_fpat;
static off_t  _find_file_matches_fsize;
extern "C" int _find_file_matches_walker(const char *fpath,
    const struct stat *sb, int typeflag, struct FTW *ftwbuf);

/*
 * We look for a files which are within a padding size of
 * a template file
 */
int _find_file_matches_walker(const char *fpath,
    const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    if (typeflag == FTW_F) {
        off_t delta = sb->st_size - _find_file_matches_fsize;

        if ( delta >= 0 && delta < 64 ) {
            LOG_INFO("Found %s", fpath);
            _find_file_matches_vec.push_back(string(fpath));
        }
    }
    return 0;
}

static int _find_file_matches(const string& rootpath, const string& sample,
    string& result)
{
    struct stat sb;
    int rv;
    if (stat(sample.c_str(), &sb) != 0) {
        LOG_ERROR("Failed stat of pattern file %s", sample.c_str());
    }
    _find_file_matches_fpat = sample;
    _find_file_matches_fsize = sb.st_size;

    _find_file_matches_vec.clear();

    rv =  nftw(rootpath.c_str(), _find_file_matches_walker, 10, 0);
    if (rv != 0) {
        LOG_ERROR("got error from nftw, %d", rv);
        goto done;
    }

    if (_find_file_matches_vec.size() != 1) {
        LOG_ERROR("bad count of match files %d for path '%s' size %d",
            _find_file_matches_vec.size(), rootpath.c_str(), (int) sb.st_size);
    }
    result = _find_file_matches_vec[0];
done:
    return rv;
}

static void _log_cmd(const char *msg, std::string& cmd)
{
    FILE *fp = NULL;
    char buf[256];

    if ( (fp = popen(cmd.c_str(), "r")) == NULL ) {
        LOG_ERROR("popen(%s) - %d", cmd.c_str(), errno);
        goto done;
    }

    LOG_INFO("%s", msg);
    LOG_INFO("output from %s:", cmd.c_str());
    while ( fgets(buf, sizeof(buf), fp) ) {
        if ( strlen(buf) ) {
            buf[strlen(buf) - 1] = '\0';
        }
        LOG_INFO("%s", buf);
    }

done:
    if ( fp != NULL ) {
        pclose(fp);
    }
}

// Note: This function is really retarded... However, I'm not going to 
// rewrite it until I understand why it's not working.
// cut file to percentage and then add badpad bytes of stuff at the end 
static int _truncate_pad_file(const string& rootpath, int truncpct, int badpad)
{
    int fd = open(rootpath.c_str(), O_RDWR);
    char *buf = NULL;
    int i;
    struct stat sb;
    struct stat post_sb;
    off_t new_len;
    off_t new_off;
    int rv;
    string md5_cmd;
    int ret;

    if ( fd < 0 ) {
        LOG_ERROR("Failed to open file %s", rootpath.c_str());
        rv = errno;
        goto done;
    }

    if ( (rv = fstat(fd, &sb)) ) {
        LOG_ERROR("stat failed");
        goto done;
    }

    // md5_cmd = "/usr/bin/md5sum " + rootpath + " >> /tmp/foo_md5";
    md5_cmd = "/usr/bin/md5sum " + rootpath;
    _log_cmd("before corruption", md5_cmd);

    new_len = sb.st_size*truncpct/100;
    LOG_INFO(" file: %s size: "FMTu64" trunc: "FMTu64" pad: "FMTu64,
        rootpath.c_str(), sb.st_size, new_len, (u64)badpad);

    // Bug 801: I can see this working if the ftruncate didn't really
    // happen and we somehow appended the bad pad to the end of the file.
    if ( (rv = ftruncate(fd, new_len)) ) {
        LOG_ERROR("ftruncate failed");
        goto done;
    }

    if ( (rv = fstat(fd, &post_sb)) ) {
        LOG_ERROR("post trunc stat failed");
        goto done;
    }
    LOG_INFO(" after trunc: file: %s size: "FMTu64, rootpath.c_str(),
        post_sb.st_size);

    if ( NULL == (buf = (char *) malloc(badpad)) ) {
        LOG_ERROR("malloc failed");
        rv = ENOMEM;
        goto done;
    }

    for (i = 0; i < badpad; i++) {
        buf[i] =  random();
    }
    new_off = lseek(fd, 0, SEEK_END);
    LOG_INFO("new offset = "FMTu64, new_off);
    if ( new_off != new_len ) {
        LOG_ERROR("new offset not at new_len!");
    }
    if ( (rv = (int) write(fd, buf, badpad)) < 0 ) {
        LOG_ERROR("corrupt failed %d", rv);
        goto done;
    }

    if (stat(rootpath.c_str(), &post_sb) != 0) {
        LOG_ERROR("Failed post stat of file\n");
        rv = -1;
        goto done;
    }

    LOG_INFO("new size: "FMTu64" old size: "FMTu64, post_sb.st_size,
        sb.st_size);
    if ( post_sb.st_size > sb.st_size ) {
        // This could potentially happen 
        LOG_ERROR("File has grown.");
    }
    close(fd);
    fd = -1;

    _log_cmd("after corruption", md5_cmd);

    ret = system(md5_cmd.c_str());
    if ( ret < 0 ) {
        LOG_ERROR("md5 failed - %d", errno);
    }
    if ( WEXITSTATUS(ret) ) {
        LOG_ERROR("Failed performing md5sum on %s", rootpath.c_str());
    }

    rv = 0;

done:
    if (fd >= 0) {
        close(fd);
    }
    if (buf) {
        free(buf);
    }
    return rv;
}

static int _sw_download_end(u64 handle, const string& file_loc)
{
    int rv;
    VPLFS_stat_t sb;
    ccd::SWUpdateEndDownloadInput req;

    req.set_handle(handle);
    req.set_file_location(file_loc);
    if ( (rv = CCDISWUpdateEndDownload(req)) ) {
        LOG_ERROR("Failed SWUpdateEndDownload() - %d", rv);
        goto done;
    }
    LOG_INFO("Download ended!");

    // The file returned should have a non-zero size if the 
    // download had not been canceled.
    rv = VPLFS_Stat(file_loc.c_str(), &sb);
        if (!_swu_canceled) {
        if (rv != 0) {
            LOG_ERROR("Failed stat of target file %s - %d",
                file_loc.c_str(), rv);
            goto done;
        }

        if (sb.size <= 0) {
            LOG_ERROR("Illegal zero length file returned for %s",
                file_loc.c_str());
            rv = 1;
            goto done;
        }
    }

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

    _swu_canceled = true;

    rv = 0;
done:
    return rv;
}

static int _download_app_poll(const string& guid, const string& ver,
    const string& file_loc)
{
    int rv;
    u64 handle;

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
                _swu_canceled = true;
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

int _fileCp(const std::string& src, const std::string& dst)
{
    VPLFile_handle_t ifd = VPLFILE_INVALID_HANDLE, ofd = VPLFILE_INVALID_HANDLE;
    ssize_t inchar, outchar;
    const ssize_t blksize = 1024 * 1024;
    char buffer[blksize];
    int rv = 0;

    ifd = VPLFile_Open(src.c_str(), VPLFILE_OPENFLAG_READONLY,
        VPLFILE_MODE_IRUSR);
    if (!VPLFile_IsValidHandle(ifd)) {
        rv = ifd;
        goto done;
    }
    ofd = VPLFile_Open(dst.c_str(),
        VPLFILE_OPENFLAG_CREATE | VPLFILE_OPENFLAG_WRITEONLY |
            VPLFILE_OPENFLAG_TRUNCATE, VPLFILE_MODE_IRUSR | VPLFILE_MODE_IWUSR);
    if (!VPLFile_IsValidHandle(ofd)) {
        rv = ofd;
        goto done;
    }

    while( (inchar = VPLFile_Read(ifd, buffer, blksize)) > 0) {
        outchar = VPLFile_Write(ofd, buffer, inchar);
        if (outchar != inchar) {
            rv = VPL_ERR_IO;
            goto done;
        }
    }

    if ( inchar < 0 ) {
        rv = inchar;
    }

done:
    if ( VPLFile_IsValidHandle(ifd) ) {
        VPLFile_Close(ifd);
    }

    if ( VPLFile_IsValidHandle(ofd) ) {
        VPLFile_Close(ofd);
    }

    return rv;
}

// Test downloading 
static int _download_app_partial(u64 ev_handle, const string& guid,
    const string& ver, const string& file_loc, const string& good_loc)
{
    int rv;
    u64 handle;
    struct {
        int len;
        bool passval;
    } padlengths[] = { {0, true}, {8192, true}}; // , {65536, false} };
    int len;
    bool passval;
    int nfails = 0, i;
    const string save_loc = good_loc + ".crypt";


    // Do a complete download to get a baseline.
    if ( (rv = _sw_download_begin(guid, ver, handle)) ) {
        LOG_ERROR("_sw_download_begin(%s, %s) failed - %d", guid.c_str(),
            ver.c_str(), rv);
        goto done;
    }

    if ( (rv = _sw_progress_event(ev_handle, handle, false))
            != 0 ) {
        LOG_ERROR("_sw_progress_event() failed - %d", rv);
        goto done;
    }

    if ( (rv = _find_file_matches(_ccd_root, good_loc, _cache_loc)) != 0) {
        LOG_ERROR("could not find cache file %d", rv);
        goto done;
    }

    _fileCp(_cache_loc, save_loc);

    // Let the update terminate
    if ( (rv = _sw_download_end(handle, file_loc)) ) {
        LOG_ERROR("_sw_download_end() failed - %d", rv);
        goto done;
    }

    LOG_INFO("cache location: %s", _cache_loc.c_str());
    // truncate it and see how the restart works.
    for (i = 0; i < sizeof(padlengths)/sizeof(padlengths[0]); i++) {
        len = padlengths[i].len;
        passval = padlengths[i].passval;
        LOG_INFO("Testing truncation and pad to %d", len);
        _fileCp(save_loc, _cache_loc);
        _truncate_pad_file(_cache_loc, 50, len);

        if ( (rv = _sw_download_begin(guid, ver, handle)) ) {
            LOG_ERROR("_sw_download_begin(%s, %s) failed - %d", guid.c_str(),
                ver.c_str(), rv);
            goto doneiter;
        }
        
        if ( (rv = _sw_progress_event(ev_handle, handle, false))
                != 0 ) {
            LOG_ERROR("_sw_progress_event() failed - %d", rv);
            goto doneiter;
        }

        if ( (rv = _sw_download_end(handle, file_loc))) {
            LOG_ERROR("_sw_download_end() failed - %d", rv);
            goto doneiter;
        }
        rv = 0;
doneiter:
        if ((passval && rv == 0) || (!passval && rv != 0)) {
            LOG_INFO("Transfer test with padding of %d passed: rv %d want  %d",
                len, rv, passval == false);
        } else {
            LOG_ERROR("Transfer test with padding of %d failed: rv %d want %d",
                len, rv, passval == false);
            nfails++;
        }
    }
    rv = 0;
done:
    log_result( ((nfails == 0) && (rv == 0)), "swupdate_partialdload");
    return nfails + rv;
    
}

static int _download_app_corrupt(u64 ev_handle, const string& guid,
    const string& ver, const string& file_loc, const string& good_loc)
{
    int rv;
    u64 handle;
    struct stat sb;

    stat(good_loc.c_str(), &sb);


    LOG_INFO(" Corrupt end of file -- should not hurt subsequent download");
    LOG_INFO(" - %s size "FMTu64, good_loc.c_str(), sb.st_size);
    if ( (rv = unlink(file_loc.c_str())) && errno != ENOENT) {
        LOG_ERROR("Failed rm %s %d errno %d", file_loc.c_str(), rv, errno);
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

    LOG_INFO(" cache_loc - %s", _cache_loc.c_str());
    _truncate_pad_file(_cache_loc, 99, sb.st_size - sb.st_size*99/100);

    rv = _sw_download_end(handle, file_loc);
    if (rv != SWU_ERR_DECRYPT_FAIL) {
        LOG_ERROR("_sw_download_end() received unexpected result - %d", rv);
        rv = EINVAL;
        goto done;
    }
    LOG_INFO("_sw_download_end received expected error ");

    // Try it again. It should work.
    if ( (rv = _download_app_poll(guid, ver, file_loc)) ) {
        LOG_ERROR("retry after corrupt file of (%s, %s) failed - %d",
            guid.c_str(), ver.c_str(), rv);
        goto done;
    }

    rv = 0;
done:
    log_result( (rv == 0), "swupdate_corruptdload");
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

static int _neg_tests()
{
    int rv;
    u64 handle;
    std::string app_ver;
    std::string ccd_ver;
    u64 update_mask;
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
    if ( (rv = _sw_update_check(_td->app_guid, _td->app_longver, app_ver,
            ccd_ver, update_mask, ret_size)) != 0 ) {
        LOG_ERROR("Unsuccessfully performed update check on invalid version 1");
        rv = -3;
        goto done;
    }

    // XXX - need to verify a force update of app
    if ( (update_mask & ccd::SW_UPDATE_BIT_APP_CRITICAL) == 0 ) {
        LOG_ERROR("APP Critical update bit not set 1");
        rv = -4;
        goto done;
    }
    if ( (update_mask & ccd::SW_UPDATE_BIT_CCD_CRITICAL) == 0 ) {
        LOG_ERROR("CCD Critical update bit not set 1.5");
        rv = -4;
        goto done;
    }

    // part two of invalid version string of app
    if ( (rv = _sw_update_check(_td->app_guid, _td->app_shortver, app_ver,
            ccd_ver, update_mask, ret_size)) != 0 ) {
        LOG_ERROR("Unsuccessfully performed update check on invalid version 2");
        rv = -5;
        goto done;
    }

    // XXX - need to verify a force update
    if ( (update_mask & ccd::SW_UPDATE_BIT_APP_CRITICAL) == 0 ) {
        LOG_ERROR("APP Critical update bit not set 2");
        rv = -6;
        goto done;
    }
    if ( (update_mask & ccd::SW_UPDATE_BIT_CCD_CRITICAL) == 0 ) {
        LOG_ERROR("CCD Critical update bit not set 2.5");
        rv = -6;
        goto done;
    }

    // part three of invalid version string of app
    {
        std::string bad_app_ver = "asbasdfasdfasdf";
        if ( (rv = _sw_update_check(_td->app_guid, bad_app_ver, app_ver,
                ccd_ver, update_mask, ret_size)) != 0 ) {
            LOG_ERROR("Unsuccessfully performed update check on invalid "
                "version 3");
            rv = -7;
            goto done;
        }
    }

    // XXX - need to verify a force update
    if ( (update_mask & ccd::SW_UPDATE_BIT_APP_CRITICAL) == 0 ) {
        LOG_ERROR("APP Critical update bit not set 3");
        rv = -8;
        goto done;
    }
    if ( (update_mask & ccd::SW_UPDATE_BIT_CCD_CRITICAL) == 0 ) {
        LOG_ERROR("CCD Critical update bit not set 3.5");
        rv = -8;
        goto done;
    }

    // XXX - Need to check handling of CCD's bit - 
    // force a bad CCD version and good app versions.

    LOG_INFO("negative tests done");

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

static int _sw_update_mask_tests(void)
{
    int rv;
    std::string new_app_ver;
    std::string app_ver;
    std::string ccd_ver;
    std::string up_mask_str, exp_mask_str;
    u64 update_mask;
    u64 exp_mask;
    u64 ret_size;
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
    if ( (rv = _set_ccd_version(_td->ccd_guid, _td->ccd_noupver)) ) {
        LOG_ERROR("3: Failed setting the ccd version = %d", rv);
        goto done;
    }
    if ( (rv = _sw_update_check(_td->ccd_guid, _td->ccd_opupver, app_ver,
            ccd_ver, update_mask, ret_size)) ) {
        LOG_ERROR("3: Failed sw update check - %d", rv);
        goto done;
    }
    exp_mask = ccd::SW_UPDATE_BIT_APP_OPTIONAL;
    if ( update_mask != exp_mask ) {
        _sw_mask2str(update_mask, up_mask_str);
        _sw_mask2str(exp_mask, exp_mask_str);
        LOG_ERROR("3: Unexpected mask got "FMTx64" %s want "FMTx64 "%s",
            update_mask, up_mask_str.c_str(), exp_mask, exp_mask_str.c_str());
        rv = -2;
        goto done;
    }

    // force app and ccd to no update needed
    if ( (rv = _set_ccd_version(_td->ccd_guid, _td->ccd_noupver)) ) {
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
        _sw_mask2str(update_mask, up_mask_str);
        _sw_mask2str(exp_mask, exp_mask_str);
        LOG_ERROR("4: Unexpected mask got "FMTx64" %s want "FMTx64 " %s",
            update_mask, up_mask_str.c_str(), exp_mask, exp_mask_str.c_str());
        rv = -2;
        goto done;
    }

    // If Application is ahead of the current version, something has gone
    // wrong. This should be a critical update for both 
    exp_mask = ccd::SW_UPDATE_BIT_APP_CRITICAL 
        | ccd::SW_UPDATE_BIT_CCD_CRITICAL;
    if ( (rv = _sw_update_check(_td->app_guid, _td->app_overver, app_ver,
            ccd_ver, update_mask, ret_size)) ) {
        LOG_ERROR("5: Failed sw update check - %d", rv);
        goto done;
    }
    if ( update_mask != exp_mask ) {
        _sw_mask2str(update_mask, up_mask_str);
        _sw_mask2str(exp_mask, exp_mask_str);
        LOG_ERROR("5: Unexpected mask got "FMTx64" %s want "FMTx64 " %s",
            update_mask, up_mask_str.c_str(), exp_mask, exp_mask_str.c_str());
        rv = -2;
        goto done;
    }

    if ( (rv = _set_ccd_version(_td->ccd_guid, _td->ccd_overver)) ) {
        LOG_ERROR("5: Failed setting the CCD version = %d", rv);
    }
    exp_mask = ccd::SW_UPDATE_BIT_CCD_CRITICAL;
    if ( (rv = _sw_update_check(_td->app_guid, new_app_ver, app_ver, ccd_ver,
            update_mask, ret_size)) ) {
        LOG_ERROR("5: Failed sw update check - %d", rv);
        goto done;
    }
    if ( update_mask != exp_mask ) {
        _sw_mask2str(update_mask, up_mask_str);
        _sw_mask2str(exp_mask, exp_mask_str);
        LOG_ERROR("5: Unexpected mask got "FMTx64" %s want "FMTx64 " %s",
            update_mask, up_mask_str.c_str(), exp_mask, exp_mask_str.c_str());
        rv = -2;
        goto done;
    }

    // Test the SW_UPDATE_BIT_CCD_NEEDED case
    // 
    if ( (rv = _set_ccd_version(_td->ccd_guid, _td->ccd_opupver)) ) {
        LOG_ERROR("5: Failed setting the CCD version = %d", rv);
    }
    exp_mask = ccd::SW_UPDATE_BIT_CCD_NEEDED;
    if ( (rv = _sw_update_check(_td->app_guid, new_app_ver, app_ver, ccd_ver,
            update_mask, ret_size)) ) {
        LOG_ERROR("5: Failed sw update check - %d", rv);
        goto done;
    }
    if ( update_mask != exp_mask ) {
        _sw_mask2str(update_mask, up_mask_str);
        _sw_mask2str(exp_mask, exp_mask_str);
        LOG_ERROR("5: Unexpected mask got "FMTx64" %s want "FMTx64 " %s",
            update_mask, up_mask_str.c_str(), exp_mask, exp_mask_str.c_str());
        rv = -2;
        goto done;
    }

    LOG_INFO("update mask tests done");

    rv = 0;
done:
    return rv;
}

static int _sw_update_ver_comp_test1(void)
{
    int rv;
    version_walk_t* list;
    int list_len;
    std::string app_ver;
    std::string ccd_ver;
    std::string up_mask_str;
    u64 update_mask;
    u64 ret_size;

    LOG_INFO("Now performing ver comp test1");
    
    if ( _td == &swu_testcases[0] ) {
        list = _win_vwalk;
        list_len = sizeof(_win_vwalk)/sizeof(_win_vwalk[0]);
    }
    else {
        list = _and_vwalk;
        list_len = sizeof(_and_vwalk)/sizeof(_and_vwalk[0]);
    }

    // walk the lists
    for( int i = 0 ; i < list_len ; i++ ) {
        if ( (rv = _sw_update_check(_td->app_guid, list->ver, app_ver, 
            ccd_ver, update_mask, ret_size)) ) {
            LOG_ERROR("%d: Failed sw update check - %d", i, rv);
            goto done;
        }
        update_mask &= (ccd::SW_UPDATE_BIT_APP_CRITICAL | 
            ccd::SW_UPDATE_BIT_APP_OPTIONAL);
        if ( update_mask != list->mask ) {
            LOG_ERROR("%d: Unexpected mask got "FMTx64" want "FMTx64, i,
                update_mask, list->mask);
            rv = -1;
            goto done;
        }
        list++;
    }

    LOG_INFO("update ver comp test1 done");

    rv = 0;
done:
    return rv;
}

static int _sw_update_ver_comp_test2(void)
{
    int rv;
    int list_len;
    std::string app_ver;
    std::string ccd_ver;
    std::string up_mask_str;
    u64 update_mask;
    u64 exp_mask;
    u64 ret_size;

    LOG_INFO("Now performing ver comp test2");
    
    if ( (rv = _set_ccd_version(_td->ccd_guid, _td->ccd_noupver)) ) {
        LOG_ERROR("Failed setting the ccd version = %d", rv);
        goto done;
    }

    // walk the lists
    list_len = sizeof(_verSizes)/sizeof(_verSizes[0]);
    for( int i = 0 ; i < list_len ; i++ ) {
        if ( (rv = _sw_update_check(_verSizes[i].app_guid, _verSizes[i].app_ver,
                                    app_ver, ccd_ver, update_mask,
                                    ret_size)) ) {
            LOG_ERROR("%d: Failed sw update equal check - %d", i, rv);
            goto done;
        }
        exp_mask = 0;
        if ( update_mask != exp_mask ) {
            LOG_ERROR("%d: Unexpected equal mask got "FMTx64" want "FMTx64, i,
                update_mask, exp_mask);
            rv = -1;
            goto done;
        }
        if ( (rv = _sw_update_check(_verSizes[i].app_guid, _verSizes[i].app_low,
                                    app_ver, ccd_ver, update_mask,
                                    ret_size)) ) {
            LOG_ERROR("%d: Failed sw update low check - %d", i, rv);
            goto done;
        }
        exp_mask = ccd::SW_UPDATE_BIT_APP_CRITICAL;
        if ( update_mask != exp_mask ) {
            LOG_ERROR("%d: Unexpected low mask got "FMTx64" want "FMTx64, i,
                update_mask, exp_mask);
            rv = -1;
            goto done;
        }
        if ( (rv = _sw_update_check(_verSizes[i].app_guid,
                                    _verSizes[i].app_high,
                                    app_ver, ccd_ver, update_mask,
                                    ret_size)) ) {
            LOG_ERROR("%d: Failed sw update high check - %d", i, rv);
            goto done;
        }
        exp_mask = ccd::SW_UPDATE_BIT_APP_CRITICAL |
            ccd::SW_UPDATE_BIT_CCD_CRITICAL;
        if ( update_mask != exp_mask ) {
            LOG_ERROR("%d: Unexpected high mask got "FMTx64" want "FMTx64, i,
                update_mask, exp_mask);
            rv = -1;
            goto done;
        }
        if ( (rv = _sw_update_check(_verSizes[i].app_guid,
                                    _verSizes[i].app_inv,
                                    app_ver, ccd_ver, update_mask,
                                    ret_size)) ) {
            LOG_ERROR("%d: Failed sw update inv check - %d", i, rv);
            goto done;
        }
        exp_mask = ccd::SW_UPDATE_BIT_APP_CRITICAL |
            ccd::SW_UPDATE_BIT_CCD_CRITICAL;
        if ( update_mask != exp_mask ) {
            LOG_ERROR("%d: Unexpected inv mask got "FMTx64" want "FMTx64, i,
                update_mask, exp_mask);
            rv = -1;
            goto done;
        }
    }

    LOG_INFO("update ver comp test2 done");

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
    u64 ev_handle = -1;
    string app_ver;
    string ccd_ver;
    u64 update_mask = 0;
    u64 app_size = 0;

    progNm = av[0];

    errflg = 0;
    while ((c = getopt(ac, av, "hr:")) != EOF) {
        switch(c) {
        case 'h': {
            usage("This help message");
            break;
        }
        case 'r': {
            _ccd_root = optarg;
            break;
        }
        case '?':
        default: {
            errflg++;
            break;
        }

        }
    }

    LOGInit("swu_test", NULL);
    LOGSetMax(0);       // No limit
    LOG_ENABLE_LEVEL(LOG_LEVEL_TRACE);

    // note: Fix the ver comp test if we ever change this.
    _td = &swu_testcases[1];    // Use android set of values

    LOG_INFO("ccd root %s", _ccd_root.c_str());

    if (errflg) {
        usage("invalid args");
    }

    if (_ccd_root.empty()) {
        LOG_ERROR("ccd root not specified");
        rv = 1;
        goto done;
    }

    // Verify that we get SWU_ERR_CCD_NOT_SET errors if we try to
    // updatecheck or begin downloads before setting the CCD version
    if ( (rv = _sw_update_check(_td->app_guid, _td->app_ver, app_ver, ccd_ver,
            update_mask, app_size)) != SWU_ERR_CCD_NOT_SET ) {
        LOG_ERROR("_sw_update_check() err - %d, wanted %d", rv,
            SWU_ERR_CCD_NOT_SET);
        goto done;
    }
    LOG_INFO("Received expected update_check error");
    if ( (rv = _download_app_poll(_td->app_guid, _td->rel_app_ver,
            "/tmp/foo")) != SWU_ERR_CCD_NOT_SET ) {
        LOG_ERROR("_download_app_poll(%s, %s) err - %d, wanted %d",
            _td->app_guid.c_str(), _td->rel_app_ver.c_str(), rv,
            SWU_ERR_CCD_NOT_SET);
        goto done;
    }
    LOG_INFO("Received expected _download_app_poll error");

    // 1st: tell CCD who it is
     if ( (rv = _set_ccd_version(_td->ccd_guid,  _td->ccd_ver)) ) {
         LOG_ERROR("_set_ccd_version() failed - %d", rv);
         goto done;
     }

    // simulate CCD restarting between a swupdate check and begin
    // download by downloading prior to the update_check.
    if ( (rv = _download_app_poll(_td->app_guid, _td->rel_app_ver,
            "/tmp/foo")) ) {
        LOG_ERROR("_download_app_poll(%s, %s) failed - %d",
            _td->app_guid.c_str(), _td->rel_app_ver.c_str(), rv);
        goto done;
    }

    if ( (rv = _sw_update_check(_td->app_guid, _td->app_ver, app_ver, ccd_ver,
            update_mask, app_size)) ) {
        LOG_ERROR("_sw_update_check() failed - %d", rv);
        goto done;
    }

    LOG_INFO(" my ccd ver: %s", _td->ccd_ver.c_str());
    LOG_INFO("swu ccd ver: %s", ccd_ver.c_str());
    LOG_INFO(" my app ver: %s", _td->app_ver.c_str());
    LOG_INFO("swu app ver: %s", app_ver.c_str());
    LOG_INFO("update mask: "FMTx64, update_mask);
    LOG_INFO("   app size: "FMTx64, app_size);

    if ( (rv = _download_app_poll(_td->app_guid, app_ver, "/tmp/foo")) ) {
        LOG_ERROR("_download_app_poll(%s, %s) failed - %d",
            _td->app_guid.c_str(), app_ver.c_str(), rv);
        goto done;
    }

    log_result(true, "download_poll");

    // Do it again, and this time event driven
    if ( (rv = _event_queue_create(ev_handle)) ) {
        LOG_ERROR("_event_queue_create() failed - %d", rv);
        goto done;
    }

    // Pull down app event driven
    if ( (rv = _download_app_event(ev_handle, _td->app_guid, app_ver,
            "/tmp/foo")) ) {
        LOG_ERROR("_download_app_event(%s, %s) failed - %d",
            _td->app_guid.c_str(), app_ver.c_str(), rv);
        goto done;
    }

    // Pull down app event driven and cancel it.
    if ( (rv = _download_app_event_cancel(ev_handle, _td->app_guid, app_ver,
            "/tmp/foo")) ) {
        LOG_ERROR("_download_app_event(%s, %s) failed - %d",
            _td->app_guid.c_str(), app_ver.c_str(), rv);
        goto done;
    }

    // Pull down CCD this way
    if ( (rv = _download_app_event(ev_handle, _td->ccd_guid, ccd_ver,
            "/tmp/foo")) ) {
        LOG_ERROR("_download_app_event(%s, %s) failed - %d",
            _td->ccd_guid.c_str(), ccd_ver.c_str(), rv);
        goto done;
    }

    // Pull down CCD partial and retry
    if ( (rv = _download_app_partial(ev_handle, _td->ccd_guid, ccd_ver,
            "/tmp/foo2", "/tmp/foo")) ) {
        LOG_ERROR("_download_app_partial(%s, %s) failed - %d",
            _td->ccd_guid.c_str(), ccd_ver.c_str(), rv);
        goto done;
    }

    if ( (rv = _download_app_corrupt(ev_handle, _td->ccd_guid, ccd_ver,
            "/tmp/foo2", "/tmp/foo")) ) {
        LOG_ERROR("_download_app_corrupt(%s, %s) failed - %d",
            _td->ccd_guid.c_str(), ccd_ver.c_str(), rv);
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

    // Now perform a walk of the mask to verify all components of the version
    // are checked independently and correctly
    if ( (rv = _sw_update_ver_comp_test1()) ) {
        LOG_ERROR("_sw_update_ver_comp_test1() failed - %d", rv);
        goto done;
    }

    // Now verify we support all sizes of version string, 1-5
    // components
    if ( (rv = _sw_update_ver_comp_test2()) ) {
        LOG_ERROR("_sw_update_ver_comp_test2() failed - %d", rv);
        goto done;
    }

    LOG_INFO("Test PASSED");

    // Cleanup on success
    unlink("/tmp/foo");

    rv = 0;

done:
    return rv;
}
