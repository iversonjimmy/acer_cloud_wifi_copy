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

/// Local disk load generator.

#include "vpl_net.h"
#include "vplex_trace.h"
#include "vplex_assert.h"
#include "vplex_vs_directory.h"

#include <stdio.h>

/// These C++ STL classes may not be portable to all platforms.
#include <iostream>
#include <string>
#include <set>
#include <vector>
#include <sstream>
using namespace std;

/// These are Linux specific non-abstracted headers.
/// TODO: provide similar functionality in a platform-abstracted way.
#include <setjmp.h>
#include <signal.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <openssl/sha.h>

#include <assert.h>
#include "vsTest_personal_cloud_data.hpp"
#include "manifest.h"


#include "vsTest_vscs_common.hpp"
#include "vsTest_infra.hpp"
#include "vsTest_worker.h"

#define KiB(x)     (1024 *(x))
#define MiB(x)     (KiB(KiB(x)))
#define MAX_XFER_SIZE       (KiB(64))   // max xfer size in KiB

static int xfer_buf_size = 0;
// Common stuff
const char* vsTest_curTestName = NULL;
/// Abort error catching
static jmp_buf vsTestErrExit;
static time_t start = 0;
static time_t stop = 0;

/// Test parameters passed by command line
std::string username_prefix = "lgmss2us2";
std::string password = "password"; /// Default password
std::string ias_name = "www-c100.lab9.routefree.com";
u16 ias_port = 443;
std::string vsds_name = "www-c100.lab9.routefree.com";  /// VSDS server
u16 vsds_port = 443;
static string dataset = "clear.fi"; /// Dataset name
static std::string pc_namespace = "acer";

static int commit_interval = 1;
static u32 thread_cnt = 1;
static u32 file_size_k = KiB(5);   /// file size in KiB dflt [5 MiB]
static u32 file_cnt = 1;
static u32 rw_ratio = 0;      /// perc reads vs writes. 0 - no reads,
                              /// 100 - all reads
static u32 xfer_size_k = 32;  /// transfer size in KiB.
static bool rw_only = false;  /// threads are dedicated rd or wrt only.
static u32 rw_only_readers;   /// Number of dedicated reader threads
static u32 rw_only_writers;   /// Number of dedicated writer threads
static bool writer_wait = false;    /// readers keep going until writers
bool writers_are_done = false;
bool check_io = false;
static VPLTime_t xfer_delay_time = 0;
static u32 xfer_delay_size = 0;
static const char *manifest_file = NULL;
static u32 round_size_k = 4;
static bool manifest_randomize = false;
static manifest_t manifest;
static bool test_local = false; /// Read/write to local disk
static bool direct_xfer = false;/// Use for O_DIRECT for local IO
static bool unique_buffers = false ; /// per thread data buffer
static char *path_prefix = NULL; /// Directory for local file paths
static bool create_dir = false; /// Use to create directories from manifest on first run

static int baselevel = TRACE_ERROR; /// VPLTrace Base level

/// finish.

#define MAX_WRKR_THREAD     2000
#define MAX_FILE_SIZE_K     MiB(4)          // 4 GiB file is max
#define MAX_FILE_CNT        MiB(1)          // Allow 1 MiB files

int loadgen_round_size(u32 *size);
static void memfill(char *buf, int buflen, const char *pat, const int patlen);

bool pct_threads_stop = false;
bool use_metadata = false;
static int thread_id_start = 0;

static tpc_parms_t parms;
static tpc_thread_t threads[MAX_WRKR_THREAD];

static const char __version[]="$Id: loadgen_local.cpp,v 1.3 2012-03-15 19:40:13 david Exp $";
static void
print_version()
{
    printf("+++ Loadgen id:\n");
    printf("\t Version: %s\n", __version);
    printf("\t Build time: " __TIME__ " " __DATE__"\n");
    printf("\t Build type: Local\n");
}

static void 
usage(int argc, char* argv[])
{
    // Dump original command

    printf("*** This build of loadgen_local only supports reads/writes to local disk ***\n");

    // Print usage message
    printf( "Usage: %s [options]\n", argv[0]);
    printf("Options:\n");

    printf(" -D --direct-xfer           Use O_DIRECT for local IO.\n                  File, xfer sizes rounded up to be multiples of pagesize (%u bytes)\n", getpagesize());
    printf(" -x --xfer-size SIZE        Transfer Size in KiB (%d). Max size limited by memory.\n",
           xfer_size_k);
    printf(" -P --path-prefix           Path prefix to use for files created for local IO. Must exist and be readable and writable.\n");

    // Options common to all modes
    printf(" -t --thread-cnt COUNT      Number of threads (%d/%d)\n",
           thread_cnt, MAX_WRKR_THREAD);
    printf(" -C --check                  Check I/O (%s)\n",
            check_io ? "TRUE" : "FALSE");
    printf(" -f --file-size SIZE        File Size in KiB (%d/%d)\n",
           file_size_k, MAX_FILE_SIZE_K);
    printf(" -c --file-cnt COUNT        File Count (%d/%d)\n",
           file_cnt, MAX_FILE_CNT);
    printf(" -r --rw-ratio RATIO        Read/Write Ratio (%d)\n",
           rw_ratio);
    printf(" -y --rw-only               Threads are read or write only\n");
    printf(" -w --wrtr-wait             Keep reads going until writes complete\n");
    printf(" -U --unique-buffers        Unique per thread buffers\n");                
    printf(" -i --id-start  ID          Thread id start number [0]\n");
    printf(" -z --throttle  SIZE:SECS   Time to spend on each xfer\n");
    printf("                            take SECS to send SIZE KiB\n");
    printf(" -M --manifest  FILE        Load files to create from manifest\n");
    printf(" -R --round-size MULT       Round file size to X KiB [1]\n");
    printf(" -V --version               Print usage and version information\n");
    printf(" -Z                         Randomize manifest\n");
}

static int 
parse_args(int argc, char* argv[])
{
    int rv = 0;

    static struct option long_options[] = {
        // Options specific to local mode on Oracle Linux 6
        {"direct-xfer", no_argument, 0, 'D'},
        {"path-prefix", required_argument, 0, 'P'},
        // Options common to all modes
        {"id-start", required_argument, 0, 'i'},
        {"thread-cnt", required_argument, 0, 't'},
        {"file-size", required_argument, 0, 'f'},
        {"file-cnt", required_argument, 0, 'c'},
        {"xfer-time", required_argument, 0, 'z'},
        {"rw-ratio", required_argument, 0, 'r'},
        {"xfer-size", required_argument, 0, 'x'},
        {"rw-only", required_argument, 0, 'y'},
        {"unique-buffers", required_argument, 0, 'U'},
        {"wrtr-wait", required_argument, 0, 'w'},
        {"manifest", required_argument, 0, 'M'},
        {"round-size", required_argument, 0, 'R'},
        {"random-man", required_argument, 0, 'Z'},
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'V'},
        {0,0,0,0}
    };

    for(;;) {
        int option_index = 0;
        int c = getopt_long(argc, argv, "t:f:c:r:x:ywi:y:CM:R:Zz:LhDUVP:", long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
                // Options specific to loadgen_vss local
            case 'P':
                path_prefix = optarg;
                break;

            case 't':
                thread_cnt = atoi(optarg);
                if ( thread_cnt > MAX_WRKR_THREAD) {
                    printf("Thread cnt max exceeded - %d\n", thread_cnt);
                    rv = -1;
                }
                break;

            case 'f':
                file_size_k = atoi(optarg);
                if ( file_size_k > MAX_FILE_SIZE_K) {
                    printf("file_size max exceeded - %d\n", file_size_k);
                    rv = -1;
                }
                break;

            case 'c':
                file_cnt = atoi(optarg);
                if ( file_cnt > MAX_FILE_CNT) {
                    printf("file_cnt max exceeded - %d\n", file_cnt);
                    rv = -1;
                }
                break;

            case 'r':
                rw_ratio = atoi(optarg);
                if ( rw_ratio > 100) {
                    printf("rw_ratio max exceeded - %d\n", rw_ratio);
                    rv = -1;
                }
                break;

            case 'x':
                xfer_size_k = atoi(optarg);
                if ( xfer_size_k > MAX_XFER_SIZE ) {
                    printf("xfer_size max exceeded - %d\n", xfer_size_k);
                    rv = -1;
                }
                if ( xfer_size_k == 0 ) {
                    printf("xfer_size too small - %d\n", xfer_size_k);
                    rv = -1;
                }
                break;


            case 'y':
                rw_only = true;
                break;

            case 'w':
                writer_wait = true;
                break;

            case 'i':
                thread_id_start = atoi(optarg);
                break;

            case 'z':
                if ( sscanf(optarg, "%d:"FMT_VPLTime_t, &xfer_delay_size, &xfer_delay_time) != 2) {
                    printf("Invalid throttle value %s\n", optarg);
                    usage(argc, argv);
                }
                xfer_delay_size *= 1024;
                xfer_delay_time = VPLTime_FromSec(xfer_delay_time);
                break;

            case 'C':
                check_io = true;
                unique_buffers = true;
                break;

            case 'I':
                commit_interval = atoi(optarg);
                break;

            case 'M':
                manifest_file = optarg;
                break;

            case 'R':
                round_size_k = atoi(optarg);
                if (round_size_k < 4) {
                    printf("Round size atleast 4(K)\n");
                    round_size_k = 4;
                }
                break;

            case 'U':
                unique_buffers = true;
                break;

            case 'Z':
                manifest_randomize = true;
                break;

            case 'L': /* Local test */
                test_local = true;
                create_dir = true;
                VPLTRACE_LOG_INFO(TRACE_APP ,0, "*** Running local test!\n");
                break;

            case 'D': /* Local test */
                direct_xfer = true;
                VPLTRACE_LOG_INFO(TRACE_APP ,0, "*** Running direct transfer test\n");
                break;
            
            case 'V':
            case 'h':
                print_version();
                usage(argc, argv);
                exit(0);

            default:
                rv = -1;
                break;
        }
    }

    /// Make sure the xfer_size is sane.
    /// XXX - May want to force xfer_size to be a multiple of the file_size.
    if ( xfer_size_k > file_size_k ) {
        xfer_size_k = file_size_k;
        printf("Xfer_size cropped to %d\n", xfer_size_k);
    }

    // Compute the number of readers vs writers to set up.
    if ( rw_only ) {
        rw_only_readers = ((thread_cnt * rw_ratio) + 50 )/ 100;
        rw_only_writers = thread_cnt - rw_only_readers;
        if ( writer_wait && (rw_only_writers == 0) ) {
            writer_wait = false;
        }
    }
    else if ( writer_wait ) {
        printf("writer waiting only applies with read/write dedicated "
                "threads.\n");
        usage(argc, argv);
    }

    if ( rv != 0 ) {
        usage(argc, argv);
    }

    return rv;
}

static VPLCond_t pct_cond;
static VPLCond_t pct_start_cond;
static VPLMutex_t pct_mutex;

static bool thrds_are_ok;
static bool thrds_start;
static bool thrds_stop;
static u32 thrd_rdy_cnt;

static int 
thrd_sync_init(void)
{
    if (VPLCond_Init(&pct_cond)) {return -1;}
    if (VPLCond_Init(&pct_start_cond)) { return -1; }
    if (VPLMutex_Init(&pct_mutex)) { return -1; }

    thrds_are_ok = true;
    thrd_rdy_cnt = 0;
    thrds_start = false;
    thrds_stop = false;
    return 0;
}

bool
thrd_ready_wait(bool is_ok, void *handle, void (*idle)(void *))
{
    VPLMutex_Lock(&pct_mutex);
    if ( !is_ok ) {
        thrds_are_ok = false;
    }

    thrd_rdy_cnt++;
    VPLCond_Signal(&pct_cond);

    while (!thrds_start && !thrds_stop) {
        if (VPLCond_TimedWait(&pct_start_cond, &pct_mutex, VPLTime_FromSec(1)) == VPL_ERR_TIMEOUT) {
            if (idle) {
                (*idle)(handle);
            }
        }
    }
    VPLMutex_Unlock(&pct_mutex);

    return thrds_stop;
}

void thrd_writer_done(void)
{
    static u32 writers_done_count = 0;

    VPLMutex_Lock(&pct_mutex);
    writers_done_count++;
    if ( writers_done_count == rw_only_writers ) {
        writers_are_done = true;
    }
    VPLMutex_Unlock(&pct_mutex);
}

static void thrd_wake_signal(u32 thread_cnt, bool force_stop)
{
    VPLMutex_Lock(&pct_mutex);
    if ( force_stop ) {
        thrds_are_ok = false;
    }
    while (thrd_rdy_cnt != thread_cnt) {
        VPLCond_TimedWait(&pct_cond, &pct_mutex, VPLTime_FromSec(1));
    }
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "*** Thread wait done, thrds_are_ok: %d", thrds_are_ok);


    if ( !thrds_are_ok ) {
        thrds_stop = true;
    }
    else {
        thrds_start = true;
    }
    VPLMutex_Unlock(&pct_mutex);

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "*** Thread wait done, starting wakeup: %d", thrds_are_ok);
    VPLCond_Broadcast(&pct_start_cond);
    VPLTRACE_LOG_ALWAYS(TRACE_APP ,0, "*** Broadcast sent.");
}

static void
dump_rate(const char *dir, clock_t tot_time, u32 kibs, long clks_p_sec,
            bool do_nl)
{
    u32 tot_mibs;
    u32 mib_sec = 0;

    if ( tot_time) {
        tot_mibs = kibs / 1024;
        mib_sec = (((tot_mibs*(1000*clks_p_sec))+5)/tot_time)/10;
    }
    if ( do_nl ) {
        printf("+++ %s_time %lld rate %d.%02d MiB/sec\n",
                dir, (s64)tot_time, mib_sec/100, mib_sec % 100);
    }
    else {
        printf(" %s_time %lld rate %d.%02d MiB/sec",
                dir, (s64)tot_time, mib_sec/100, mib_sec % 100);
    }
}

static void
dump_oprate(const char *dir, clock_t tot_time, u32 ops,   long clks_p_sec,
    bool do_nl)
{
    u32 ops_sec = 0;

    if (tot_time != 0) {
        ops_sec = ops * 100 * clks_p_sec / tot_time;
    }
    if ( do_nl ) {
        printf("+++ %s_time %lld rate %d.%02d file ops/sec\n",
                dir, (s64)tot_time, ops_sec/100, ops_sec % 100);
    }
    else {
        printf(" %s_time %lld rate %d.%02d file ops/sec",
                dir, (s64)tot_time, ops_sec/100, ops_sec % 100);
    }
}

static void parms_dump(tpc_parms_t *parms)
{
    printf("           IAS Server: %s:%d\n", ias_name.c_str(),
        ias_port);
    printf("                VSDS Server: %s:%d\n", vsds_name.c_str(), vsds_port);
//    printf("                        User: %s\n", username.c_str());
//    printf("                    Password: %s\n", password.c_str());
//    printf("                    vss_host: %s\n", parms->vss_host);
    printf("                    Manifest: %s\n",
           manifest_file ? manifest_file : "none");
    printf("                    throttle: %d KiB/"FMT_VPLTime_t" seconds\n",
           parms->xfer_delay_size / 1024, VPLTime_ToSec(parms->xfer_delay_time));
    printf("                thread count: %d\n", thread_cnt);
    printf("                   file size: %d\n", parms->file_size);
    printf("                  file count: %d\n", parms->file_cnt);
    printf("            read/write ratio: %d%%\n", parms->rw_ratio);
    printf("                   xfer size: %d KiB\n", parms->xfer_size/1024);
    printf("                  Round_size: %d KiB\n", parms->round_size_k);
printf("Dedicated read/write threads: %s\n", parms->rw_only ? "true" : "false");
    printf("      Wait for writer thread: %s\n", parms->writer_wait ? "true" : "false");
    printf("              Run local test: %s\n", parms->test_local ? "true" : "false");
    printf("   Use O_DIRECT for local IO: %s\n", parms->direct_xfer ? "true" : "false");
    printf("    Path prefix for local IO: %s\n", parms->path_prefix ? "true" : "false");
    printf("                dataset name: %s\n", parms->dataset);
    printf("                   namespace: %s\n", parms->pc_namespace);
    printf("    files written per commit: %d\n", parms->commit_interval);
}

static void
__print_stats(time_t t0, time_t t1)
{
    u32 tot_kibs_read = 0;
    u32 tot_kibs_write = 0;
    u32 tot_reads = 0;
    u32 tot_writes = 0;
    clock_t tot_read_time = 0;
    clock_t tot_write_time = 0;
    u32 i;
    long clks_p_sec;


    if (0 == t0 && 0 == t1) {
        printf("+++ No data gathered\n");
        goto out;
    }
    clks_p_sec = sysconf(_SC_CLK_TCK);

    for( i = 0 ; i < thread_cnt ; i++ ) {
        s32 delta = (threads[i].stop > threads[i].start)? threads[i].stop - threads[i].start : 0;
        printf("+++ %5d: %5d reads %5d writes %6d time %d/%d KiB r/w",
                i,
                threads[i].reads, threads[i].writes,
                delta,
                threads[i].data_read_k, threads[i].data_write_k);
        dump_rate("read", threads[i].read_time, threads[i].data_read_k, clks_p_sec, false);
        dump_rate("write", threads[i].write_time, threads[i].data_write_k, clks_p_sec, false);
        printf("\n");
        tot_reads += threads[i].reads;
        tot_writes += threads[i].writes;
        tot_read_time += threads[i].read_time;
        tot_write_time += threads[i].write_time;
        tot_kibs_read += threads[i].data_read_k;
        tot_kibs_write += threads[i].data_write_k;
    }
    if (t1 > t0) {
    dump_rate("agg", (t1 - t0) * clks_p_sec,
            tot_kibs_read + tot_kibs_write, clks_p_sec, true);
    } else {
        printf("+++ No aggr data gathered\n");
    }
    dump_rate("avg read", tot_read_time, tot_kibs_read, clks_p_sec, true);
    dump_rate("avg write", tot_write_time, tot_kibs_write, clks_p_sec, true);
    if ( rw_only ) {
        dump_rate("read", tot_read_time/rw_only_readers, tot_kibs_read, clks_p_sec, true);
        dump_rate("write", tot_write_time/rw_only_writers, tot_kibs_write, clks_p_sec, true);
    }
    else {
        dump_rate("read", tot_read_time/thread_cnt, tot_kibs_read, clks_p_sec, true);
        dump_rate("write", tot_write_time/thread_cnt, tot_kibs_write, clks_p_sec, true);
    }
    dump_oprate("tot_read", (t1 - t0) * clks_p_sec, tot_reads, clks_p_sec, true);
    dump_oprate("tot_write", (t1 - t0) * clks_p_sec, tot_writes, clks_p_sec, true);
out:
    return;
}

static void signal_handler(int sig)
{
    // UNUSED(sig);  // expected to be SIGABRT
    longjmp(vsTestErrExit, 1);
}


// Round up to a multiple of page size
int
loadgen_round_size(u32 *size)
{
   int rv;
   int mask;

   if (size == NULL) {
       rv = -1;
       goto out;
   }
   mask = getpagesize() - 1;
   *size = (*size + mask) & ~mask;
   rv = 0;
out:
   return rv;
}

void
memfill(char *buf, int buflen, const char *pat, const int patlen)
{
    int i, l;
    for (i = 0; i < buflen; i += patlen) {
        l = min(buflen - i, patlen);
        memcpy(buf + i, pat, l);
    }
}

static const char vsTest_main[] = "VS Test Main";
int
main(int argc, char* argv[])
{
    int rv = 0; // pass until failed.
    u32 i;
    bool test_fail = false;
    void *shared_buffer = NULL;
    char patbuf[1024];
//    u64 user_id = 0;

    VPLTrace_Init(NULL);
    VPLTrace_SetShowTimeAbs(true);
    VPLTrace_SetShowTimeRel(false);
    VPLTrace_SetShowThread(true);
    VPLTrace_SetBaseLevel(baselevel);

    vsTest_curTestName = vsTest_main;

    if(parse_args(argc, argv) != 0) {
        goto exit;
    }

    print_version();

    printf("+++ threads %d files %d file_size %d KiB rw ratio %d xfer_size"
            " %d KiB\n",
            thread_cnt, file_cnt, file_size_k, rw_ratio, xfer_size_k);

    // Create directories for files for different threads (datasets)
    if (create_dir) {
        u32 i;
        char path[PATH_MAX + 1];
        extern int local_create_dirs(const char *path1, const char *path2);
        for (i = thread_id_start; i < thread_cnt; ++i) {
            generate_path_prefix(path, sizeof(path), path_prefix, i);
            if (local_create_dirs(path, NULL)) {
                if (errno != EEXIST) {
                    printf("Failed to create directory %s: %d Exiting\n", path, errno);
                    exit(1);
                }
                VPLTRACE_LOG_FINE(TRACE_APP, 0,
                                  "Creating thread directory: %s failed. Already exists.\n", path);
            }
            VPLTRACE_LOG_FINE(TRACE_APP, 0,
                              "Creating thread directory: %s\n", path);
        }
    }

    // If there's a manifest, load it.
    if ( manifest_file ) {
        if ( mani_load(&manifest, manifest_file, create_dir, thread_cnt, path_prefix, thread_id_start, manifest_randomize)) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                        "Loading manifest file");
                goto exit;
            }
            if ( file_cnt > manifest.file_cnt ) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                        "file_cnt %d manifest file_cnt %d\n",
                        file_cnt, manifest.file_cnt);
                goto exit;
            }
        }

        // catch SIGABRT 
        assert((signal(SIGABRT, signal_handler) != SIG_ERR));
        assert((signal(SIGINT, signal_handler) != SIG_ERR));

        if (setjmp(vsTestErrExit) == 0) { // initial invocation of setjmp
            // Run the tests. Any abort signal will hit the else block.

            if ( thrd_sync_init() ) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: thrd_sync_init");
                rv++;
                goto exit;
            }

            if (xfer_delay_size != 0 && xfer_delay_size < xfer_size_k) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "Delay size specified must be >= transfer size");
                rv++;
                goto exit;
            }

            parms.file_cnt = file_cnt;
            parms.rw_ratio = rw_ratio;
            parms.xfer_size = KiB(xfer_size_k);
            parms.rw_only = rw_only;
            parms.writer_wait = writer_wait;
//            parms.vss_host = vss_host;
            parms.xfer_delay_time = xfer_delay_time;
            parms.xfer_delay_size = xfer_delay_size;
            parms.manifest = (manifest_file) ? &manifest : NULL;
            parms.manifest_randomize = manifest_randomize;
            parms.round_size_k = round_size_k;
            parms.file_size = KiB(file_size_k);
            parms.test_local = test_local;
            parms.direct_xfer = direct_xfer;
            parms.path_prefix = path_prefix;
            parms.dataset = dataset.c_str();
            parms.pc_namespace = pc_namespace.c_str();
            parms.commit_interval = commit_interval;
            parms_dump(&parms);

            start = time(NULL);
            snprintf(patbuf, sizeof(patbuf), "loadgen: %s", ctime(&start));

            // Initialize transfer buffer with correct alignment, if necessary
            {

                xfer_buf_size = parms.xfer_size;

                if (parms.direct_xfer) {
                    // Need xfer buffer page aligned
                    loadgen_round_size((u32 *)&xfer_buf_size);
                    // Need file size to be multiple of page size
                    loadgen_round_size((u32 *)&parms.file_size);
                    // Need block size to be a multiple of page size
                    loadgen_round_size(&parms.xfer_size);
                    printf("Using pagesize= %u xfer_buf_size = %u xfer_size = %u file_size = %u\n", getpagesize(), xfer_buf_size, parms.xfer_size,
                            parms.file_size);
                }
                

                if (! unique_buffers) {
                    rv = posix_memalign(&shared_buffer, getpagesize(), xfer_buf_size);
                        if (rv) {
                            printf("Allocating aligned memory for shared buffer : %d\n", rv);
                            exit(1);
                        }
                }
                for (i = 0; i < thread_cnt; ++i) {
                    threads[i].xfer_buf_size = xfer_buf_size;
                    if (parms.direct_xfer) {
                        if (shared_buffer) {
                            threads[i].xfer_buf = (char *) shared_buffer;
                        } else {
                            rv = posix_memalign((void **)&threads[i].xfer_buf, getpagesize(), xfer_buf_size);
                            if (rv) {
                                printf("Allocating aligned memory for direct IO failed: %d\n", rv);
                                exit(1);
                            }
                        }
                    } else
                    {
                        if (shared_buffer) {
                            threads[i].xfer_buf = (char *) shared_buffer;
                        } else {
                            threads[i].xfer_buf = (char *) malloc(xfer_buf_size * sizeof(char));
                            if (NULL == threads[i].xfer_buf) {
                                printf("Allocating memory for transfer buffer failed\n");
                                exit(1);
                            }
                        }
                    }
                    memfill(threads[i].xfer_buf, xfer_buf_size, patbuf, strlen(patbuf));
                }
            }

            for( i = 0 ; i < thread_cnt ; i++ ) {
                threads[i].id = i + thread_id_start;


                threads[i].rw_reader = (i < rw_only_readers);
                threads[i].parms = &parms;
                // If this fails we want to force the other threads to
                // abandon ship.
                if(VPLThread_Create(&threads[i].wrkr,
                                     (VPLThread_fn_t)test_personal_cloud,
                                     &threads[i],
                                     0, // default VPLThread thread-attributes: priority, stack-size, etc.
                                     "wrkr thread") != VPL_OK) {
                    rv++;
                    VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                     "creating wrkr thread %d failed!", i);
                    test_fail = true;
                    thread_cnt = i;
                    break;
                } 
            }

            start = time(NULL);

            // signal all the threads to exit
            thrd_wake_signal(thread_cnt, test_fail);

            // wait for everyone to finish up.
            for( i = 0 ; i < thread_cnt ; i++ ) {
                VPLThread_Join(&(threads[i].wrkr), &threads[i].dontcare);
                if ( threads[i].failed ) {
                    test_fail = true;
                    rv++;
                }
            }
            stop = time(NULL);
            VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "*** All threads rejoined, rv: %d", rv);
            vsTest_curTestName = vsTest_main;
        } else {
            rv += 1;
        }

exit:

        if (shared_buffer) {
            free(shared_buffer);
        } else {
            for (i = 0; i < thread_cnt; ++i) {
                if (NULL != threads[i].xfer_buf) {
                    free(threads[i].xfer_buf);
                    threads[i].xfer_buf = NULL;
                }
            }
        }
        __print_stats(start, stop);

        printf("+++ Test %s\n", (rv == 0) ? "PASS" : "FAIL");

        return (rv) ? 1 : 0;
    }
