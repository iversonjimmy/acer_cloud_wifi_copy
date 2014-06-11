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

/// Virtual Storage Server Load generator.

#include "vpl_net.h"
#include "vplex_trace.h"
#include "vplex_assert.h"
#include "vplex_vs_directory.h"
#include "vplex_file.h"
#include "vpl_fs.h"

#include <stdio.h>

/// These C++ STL classes may not be portable to all platforms.
#include <iostream>
#include <string>
#include <set>
#include <vector>
#include <stack>
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

#include "vssi.h"
#include "vssi_error.h"

string infra_name = "www.lab18.routefree.com";
int infra_port = 443;
int log_level = TRACE_INFO; /// VPLTrace level
bool verbose_stats = false;
int worker_threads = 10;
bool all_one_connection = false;

// TODO: only support single user now
#if 0
u32 user_id_start = 0;
string username_prefix = "lgmss2us2";
static int get_username(int id, char *outbuf, int len);
static int init_usertable(std::string& prefix);
#endif
string username = "lgmss2us2";
string password = "password"; /// Default password
string pc_namespace = "acer";
u32 num_users = 1;
u32 num_readers = 0;
u32 num_writers = 0;
string read_dataset = "Virt Drive";
string write_dataset= "Virt Drive";
string manifest_file;
bool manifest_randomize = false;
bool reset_dataset = false;

bool use_metadata = false;

u32 xfer_size_k = 32;
u32 commit_interval = 200;
u32 commit_size_k = 1024; // 1 MiB
u32 file_size_k = (5 * 1024); // 5MiB
u32 file_cnt = 0;
bool verify_read = false;
bool verify_write = false;

// for doc save and go data profile.
u32 thumb_size = 10240;
bool write_thumb = false;
string base_dir = "documents";
string thumb_dir = "thumbs";

// for pic stream data profile.
string base_read_dir = ""; // dataset root
string photo_source_file; // default: no file

#define MAX_XFER_SIZE (1024) // max xfer size in KiB (1MiB)
#define MAX_FILE_SIZE_K (4 * 1024 * 1024) // 4 GiB

// Common stuff
const char* vsTest_curTestName = NULL;
/// Abort error catching
static jmp_buf vsTestErrExit;
static VPLTime_t start = 0;
static VPLTime_t stop = 0;

static load_params loadTaskParameters;
std::stack<VPLVsDirectory_ProxyHandle_t*> vsds_proxies;
VPLMutex_t vsdsMutex;

// localization to use for all commands.
static vplex::vsDirectory::Localization l10n;

VPLMutex_t loadgen_mutex;
VPLSem_t loadgen_sem;

load_task* loadTasks = NULL;
VSSI_Object load_task::handle = NULL;
VPLMutex_t load_task::obj_mutex;

int tasksStarted = 0;
int tasksDone = 0;
int writersStarted = 0;
int writersDone = 0;
int usersLaunched = 0;

static const char version[]="$Id: loadgen_vss.cpp,v 1.73 2012-04-30 23:28:18 david Exp $";
static void
print_version()
{
    printf("+++ Loadgen id:\n");
    printf("\t Version: %s\n", version);
    printf("\t Build time: " __TIME__ " " __DATE__"\n");
    printf("\t Build type: VSS/MSS\n");
}

static void
usage(int argc, char* argv[])
{
    // Dump original command

    // Print usage message
    printf( "Usage: %s [options]\n", argv[0]);
    printf("Options:\n");

    printf("Options specifying users to use when performing tasks.\n");
// TODO: only support single user now
#if 0
    printf(" -u --username USERNAME   VSDS user name template.\n"
           "                          User number is filled-in for \"%%d\"\n"
           "                          (or at end if no \"%%d\" in template) (%s)\n"
           "                          if USERNAME exists as a file, treat each line as a username\n",
           username_prefix.c_str());
#endif
    printf(" -u --username USERNAME   User name.\n");
    printf(" -p --password PASSWORD   User's password (same for all users) (%s)\n",
           password.c_str());
    printf(" -N --namespace           Personal Cloud Namespace (%s)\n",
           pc_namespace.c_str());
// TODO: only support one user now
#if 0
    printf(" -n --num-users COUNT     Number of users in test group (%d)\n",
           num_users);
    printf(" -i --id-start  ID        User ID start number [%d]\n",
           user_id_start);
#endif

    printf("Options specifying which tasks to perform.\n");
    printf(" -m --metadata        Use file metadata in tests (%s)\n",
           use_metadata ? "TRUE" : "FALSE");
    printf(" -x --xfer-size SIZE  Transfer Size in KiB (%d/%d)\n",
           xfer_size_k, MAX_XFER_SIZE);
    printf(" -r --readers NUMBER  Number of reader tasks per dataset (%d).\n",
           num_readers);
    printf(" -w --writers NUMBER  Number of writer tasks per dataset (%d).\n",
           num_writers);
    printf(" -R --reset-dataset   Reset dataset contents before starting test. (%s)\n",
           reset_dataset ? "TRUE" : "FALSE");

    printf("Options specifying files to write for writer tasks.\n");
    printf(" -I --commit-interval FILES   Commit after every N files (%d)\n",
           commit_interval);
    printf(" -S --commit-size SIZE        Commit after every K Kib of data written (on whole file boundary), with 0 meaning no limit (%d)\n",
           commit_size_k);
    printf(" -M --manifest  FILE        Write files according to manifest. Otherwise, write files by size specification (%s).\n",
           manifest_file.empty() ? "none" : manifest_file.c_str());
    printf(" -Z --randomize-manifest    Randomize file order of manifest (%s)\n",
           manifest_randomize ? "TRUE" : "FALSE");
    printf(" -f --file-size SIZE        When not using manifest, file Size in KiB (%d/%d)\n",
           file_size_k, MAX_FILE_SIZE_K);
    printf(" -c --file-cnt COUNT        When not using manifest, file Count (%d)\n",
           file_cnt);
    printf(" -V --verify-io             Verify I/O. Writers verify write is merged after each commit. Readers check file contents are as written. (%s)\n",
           verify_read ? "TRUE" : "FALSE");

    printf("General testing options.\n");
    printf(" -s --server NAME[:PORT] Infrastructure server name or IP address and port (%s:%d)\n",
           infra_name.c_str(), infra_port);
    printf(" -T --threads NUM   Number of worker threads to use to drive test (%d)\n",
           worker_threads);
    printf(" -a --all-one-connection    Make only one VSS connection for all worker threads (%s)\n", all_one_connection ? "TRUE" : "FALSE");
    printf(" --verbose-stats    Log verbose statistics at end of test. (%s)\n",
           verbose_stats ? "TRUE" : "FALSE");
    printf(" -v --verbose       Make logging more verbose (repeat up to 3 times)\n");
    printf(" -t --terse         Make logging more terse (repeat up to 2 times)\n");
}

static int
parse_args(int argc, char* argv[])
{
    int rv = 0;

    static struct option long_options[] = {
        // Options specific to general VSS/MSS mode
        {"username", required_argument, 0, 'u'},
        {"password", required_argument, 0, 'p'},
        {"namespace", required_argument, 0, 'N'},
// TODO: only support one user now
#if 0
        {"num-users", required_argument, 0, 'n'},
        {"id-start", required_argument, 0, 'i'},
#endif
        {"metdata", no_argument, 0, 'm'},
        {"xfer-size", required_argument, 0, 'x'},
        {"readers", required_argument, 0, 'r'},
        {"writers", required_argument, 0, 'w'},
        {"reset-dataset", no_argument, 0, 'R'},
        {"commit-interval", required_argument, 0, 'I'},
        {"commit-size", required_argument, 0, 'S'},
        {"manifest", required_argument, 0, 'M'},
        {"randomize-manifest", no_argument, 0, 'Z'},
        {"file-size", required_argument, 0, 'f'},
        {"file-cnt", required_argument, 0, 'c'},
        {"verify-io", no_argument, 0, 'V'},
        {"server", required_argument, 0, 's'},
        {"threads", required_argument, 0, 'T'},
        {"all-one-connection", no_argument, 0, 'a'},
        {"verbose", no_argument, 0, 'v'},
        {"terse", no_argument, 0, 't'},
        {"verbose-stats", no_argument, 0, '1'},
        {0,0,0,0}
    };

    for(;;) {
        int option_index = 0;
        int c = getopt_long(argc, argv,
// TODO: only support one user now
#if 0
                            "u:p:N:n:i:mx:r:w:RI:S:M:Zf:c:Vs:T:avt1",
#endif
                            "u:p:N:mx:r:w:RI:S:M:Zf:c:Vs:T:avt1",
                            long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
        case 'u':
// TODO: only support single user now
#if 0
            username_prefix = optarg;
#endif
            username = optarg;
            break;

        case 'p':
            password = optarg;
            break;

        case 'N':
            pc_namespace = optarg;
            break;

// TODO: only support one user now
#if 0
        case 'n':
            num_users = atoi(optarg);
            break;

        case 'i':
            user_id_start = atoi(optarg);
            break;
#endif
        case 'm':
            use_metadata = true;
            break;

        case 'x':
            xfer_size_k = atoi(optarg);
            if(xfer_size_k > MAX_XFER_SIZE ) {
                printf("xfer_size max exceeded - %d\n", xfer_size_k);
                rv = -1;
            }
            if(xfer_size_k == 0 ) {
                printf("xfer_size too small - %d\n", xfer_size_k);
                rv = -1;
            }
            break;

        case 'r':
            num_readers = atoi(optarg);
            break;

        case 'w':
            num_writers = atoi(optarg);
            break;

        case 'R':
            reset_dataset = true;
            break;

        case 'I':
            commit_interval = atoi(optarg);
            break;

        case 'S':
            commit_size_k = atoi(optarg);
            break;

        case 'M':
            manifest_file = optarg;
            break;

        case 'Z':
            manifest_randomize = true;
            break;

        case 'f':
            file_size_k = atoi(optarg);
            if(file_size_k > MAX_FILE_SIZE_K) {
                printf("file_size max exceeded - %d\n", file_size_k);
                rv = -1;
            }
            break;

        case 'c':
            file_cnt = atoi(optarg);
            break;

        case 'V':
            verify_read = verify_write = true;
            break;

        case 's':
            infra_name = optarg;
            if(infra_name.find(':') != string::npos ) {
                size_t colon = infra_name.find(':');
                string portStr = infra_name.substr(colon + 1);
                infra_name.erase(colon);
                infra_port = atoi(portStr.c_str());
            }
            break;

        case 'T':
            worker_threads = atoi(optarg);
            break;

        case 'a':
            all_one_connection = true;
            break;

        case 'v':
            if(log_level < TRACE_FINEST) {
                log_level++;
            }
            break;

        case 't':
            if(log_level > TRACE_ERROR) {
                log_level--;
            }
            break;

        case '1':
            verbose_stats = true;
            break;

        default:
            rv = -1;
            break;
        }
    }

    // Make sure there's work to be done.
    if(num_users == 0) {
        printf("Must have at least one user in test.\n");
        rv = -1;
    }
    if(num_readers == 0 && num_writers == 0 && !reset_dataset) {
        printf("Must have at least one task per user.\n");
        rv = -1;
    }
    if(num_writers > 0 && manifest_file.empty() && file_cnt == 0) {
        printf("Runs with writer tasks must have manifest or a number of files to write.\n");
        rv = -1;
    }
    if(!all_one_connection &&
       ((num_readers + num_writers) * num_users) > 1000) {
        printf("Can't start %d tasks without --all-one-connection option. Start 1000 tasks or less or use --all-one-connection.\n",
               (num_readers + num_writers) * num_users);
        rv = -1;
    }
    if(worker_threads == 0) {
        printf("Testing requires at least one worker thread.\n");
        rv = -1;
    }

    if(rv != 0) {
        print_version();
        usage(argc, argv);
        exit(255);
    }

    return rv;
}

static void dump_cmdline(int argc, char* argv[])
{
    printf("Command line:\n\t");
    for(int i = 0; i < argc; i++) {
        printf("%s ", argv[i]);
    }
    printf("\n");
}

static void dump_params()
{
    printf("                Infra Server: %s:%d\n",
           infra_name.c_str(), infra_port);
    printf("     User/namespace/password: %s/%s/%s\n",
           username.c_str(), pc_namespace.c_str(), password.c_str());
// TODO: only support single user now
#if 0
           username_prefix.c_str(), pc_namespace.c_str(), password.c_str());
    printf("            # Users/Start ID: %d/%d\n",
           num_users, user_id_start);
#endif
    printf("                dataset name: read:%s write:%s\n",
           read_dataset.c_str(), write_dataset.c_str());
    printf("                Use metadata: %s\n",
           use_metadata ? "YES" : "NO");
    printf("                   xfer size: %d KiB\n", xfer_size_k);
    printf("             Readers:Writers: %d:%d\n",
           num_readers, num_writers);
    printf("   Reset dataset before test: %s\n",
           reset_dataset ? "YES" : "NO");
    printf("        Commit interval/size: %d/%dKiB\n",
           commit_interval, commit_size_k);
    printf("               Manifest file: %s\n",
           manifest_file.empty() ? "none" : manifest_file.c_str());
    printf("        File size:File count: %dKiB:%d\n",
           file_size_k, file_cnt);
    printf("       Verify write complete: %s\n",
           verify_write ? "YES" : "NO");
    printf("     Verify contents on read: %s\n",
           verify_read ? "YES" : "NO");
    printf("All tasks use one connection: %s\n",
           all_one_connection ? "YES" : "NO");
}

static void
dump_rate(const char *dir, VPLTime_t tot_time, u64 bytes, bool do_nl)
{

    u64 mib_rate = 0; // m-MiB/s (milli-Mebibytes per second)

    if(tot_time) {
        //mib_rate = (bytes * 10000000000ull) / (tot_time * (1024 * 1024));
        if(bytes < (1<<30)) {
            mib_rate = (bytes * 9765625ull) / (tot_time * 1024);
        }
        else if( (bytes/1024) < (1<<30)) {
            mib_rate = ((bytes/1024) * 9765625ull) / tot_time; 
        }
        else {
            mib_rate = ((bytes/(1024 * 1024)) * 9765625ull) / tot_time; 
        }
        mib_rate +=5;
        mib_rate /= 10;
    }

    printf("%s%s %2llu.%03u MiB/s ("FMTu64"/"FMT_VPLTime_t"us)%s",
           do_nl ? "+++ " : " ",
           dir, mib_rate/1000, (u32)(mib_rate % 1000),
           bytes, tot_time,
           do_nl ? "\n" : "");
}

static void
dump_oprate(const char *dir, VPLTime_t tot_time, u32 ops, bool do_nl)
{
    u64 ops_rate = 0; // milli-operations per second

    if(tot_time != 0) {
        ops_rate = (ops * 10000000000ull) / tot_time;
        ops_rate += 5;
        ops_rate /= 10;
    }

    printf("%s%s %2llu.%03u op/s avg:"FMT_VPLTime_t"us (%u/"FMT_VPLTime_t"us) %s",
           do_nl ? "+++ " : " ",
           dir, 
           ops_rate/1000, (u32)(ops_rate % 1000),
           ops == 0 ? 0 : tot_time/ops,
           ops, tot_time,           
           do_nl ? "\n" : "");
}

static void
dump_time(const char *dir, VPLTime_t tot_time, bool do_nl)
{
    printf("%s%s "FMT_VPLTime_t" us %s",
           do_nl ? "+++ " : " ",
           dir, 
           tot_time,           
           do_nl ? "\n" : "");
}

static void
print_stats(VPLTime_t t0, VPLTime_t t1)
{
    u64 tot_bytes_read = 0;
    u64 tot_bytes_write = 0;
    u32 tot_reads = 0;
    u32 tot_dir_reads = 0;
    u32 tot_writes = 0;
    u32 tot_commits = 0;
    u32 read_tasks = 0;
    u32 write_tasks = 0;
    VPLTime_t tot_read_time = 0;
    VPLTime_t tot_dir_read_time = 0;
    VPLTime_t tot_write_time = 0;
    VPLTime_t tot_commit_time = 0;
    VPLTime_t tot_verify_time = 0;
    VPLTime_t earliest_start = VPLTIME_INVALID;
    VPLTime_t latest_stop = 0;
    VPLTime_t delta;
    u32 i;

    if(0 == t0 && 0 == t1) {
        printf("+++ No data gathered\n");
        goto out;
    }

    for(i = 0 ; i < tasksStarted ; i++ ) {
        if(earliest_start > loadTasks[i].start) {
            earliest_start = loadTasks[i].start;
        }
        if(latest_stop < loadTasks[i].start) {
            latest_stop = loadTasks[i].stop;
        }
        delta = loadTasks[i].stop - loadTasks[i].start;
        if(verbose_stats) {
            printf("+++ %s: elapsed:"FMT_VPLTime_t"us ("FMT_VPLTime_t"us to "FMT_VPLTime_t"us)",
                   loadTasks[i].taskName.c_str(), delta,
                   loadTasks[i].start, loadTasks[i].stop);
        }
        if(loadTasks[i].taskType == TASK_READER) {
            if(verbose_stats) {
                dump_rate("", loadTasks[i].read_time, loadTasks[i].data_read, false);
                dump_oprate("", loadTasks[i].read_time, loadTasks[i].reads, false);
                dump_oprate("dir", loadTasks[i].dir_read_time, loadTasks[i].dir_reads, false);
            }
            tot_reads += loadTasks[i].reads;
            tot_read_time += loadTasks[i].read_time;
            tot_bytes_read += loadTasks[i].data_read;
            tot_dir_reads += loadTasks[i].dir_reads;
            tot_dir_read_time += loadTasks[i].dir_read_time;
            read_tasks++;
        }
        else if(loadTasks[i].taskType == TASK_WRITER) {
            if(verbose_stats) {
                dump_rate("", loadTasks[i].write_time, loadTasks[i].data_write, false);
                dump_oprate("", loadTasks[i].write_time, loadTasks[i].writes, false);
                dump_oprate("commit", loadTasks[i].commit_time, loadTasks[i].commits, false);
                dump_time("merge", loadTasks[i].verify_time, false);
            }
            tot_writes += loadTasks[i].writes;
            tot_commits += loadTasks[i].commits;
            tot_write_time += loadTasks[i].write_time;
            tot_commit_time += loadTasks[i].commit_time;
            tot_verify_time += loadTasks[i].verify_time;
            tot_bytes_write += loadTasks[i].data_write;
            write_tasks++;
        }
        if(verbose_stats) {
            printf("\n");
        }
    }

    // Averages (task activity average / task runtime average)
    if(verbose_stats) {
        if(num_readers > 0) {
            dump_rate("Average read", tot_read_time, tot_bytes_read, true);
            dump_oprate("Average read", tot_read_time, tot_reads, true);
            dump_oprate("Average read dir", tot_dir_read_time, tot_dir_reads, true);
        }
        if(num_writers > 0) {
            dump_rate("Average write", tot_write_time, tot_bytes_write, true);
            dump_oprate("Average write", tot_write_time, tot_writes, true);
            dump_oprate("Average commit", tot_commit_time, tot_commits, true);
            dump_oprate("Average verify", tot_verify_time, tot_commits, true);
        }
    }

    // Overall (activity / task running time)
    delta = latest_stop - earliest_start;
    if(num_readers > 0) {
        dump_rate("Overall read", delta, tot_bytes_read, true);
        dump_oprate("Overall read", delta, tot_reads, true);
        dump_oprate("Overall read dir", delta, tot_dir_reads, true);
    }
    if(num_writers > 0) {
        dump_rate("Overall write", delta, tot_bytes_write, true);
        dump_oprate("Overall write", delta, tot_writes, true);
        dump_oprate("Overall commit", delta, tot_commits, true);
        dump_time("Overall merge", tot_verify_time, true);
    }
    if(num_readers > 0 && num_writers > 0) {
        dump_rate("Overall", delta, tot_bytes_read + tot_bytes_write, true);
    }

    // Runtime (activity / program running time)
    if(verbose_stats) {
        delta = t1 - t0;
        if(num_readers > 0) {
            dump_rate("Runtime read", delta, tot_bytes_read, true);
            dump_oprate("Runtime read", delta, tot_reads, true);
            dump_oprate("Runtime read dir", delta, tot_dir_reads, true);
        }
        if(num_writers > 0) {
            dump_rate("Runtime write", delta, tot_bytes_write, true);
            dump_oprate("Runtime write", delta, tot_writes, true);
            dump_oprate("Runtime commit", delta, tot_commits, true);
        }
        if(num_readers > 0 && num_writers > 0) {
            dump_rate("Runtime", delta, tot_bytes_read + tot_bytes_write, true);
        }
    }
    
 out:
    return;
}

static char* load_file(const std::string filename, 
                       size_t& len)
{
    struct stat stats;
    int fd = -1;
    char* rv = NULL;
    size_t so_far = 0;

    // If file can be found, allocate buffer and load data from disk.
    if(access(filename.c_str(), R_OK) != 0) {
        goto exit;
    }

    if(stat(filename.c_str(), &stats) != 0) {
        goto exit;
    }        

    fd = open(filename.c_str(), O_RDONLY);
    if(fd == -1) {
        goto exit;
    }

    rv = new (nothrow) char[stats.st_size];
    if(rv == NULL) {
        goto exit;
    }


    while(so_far < stats.st_size) {
        int rc;
        rc = read(fd, rv + so_far, stats.st_size - so_far);
        if(rc < 0) {
            delete rv;
            rv = NULL;
            goto exit;
        }
        else {
            so_far += rc;
        }
    }
    len = so_far;

 exit:
     if(fd != -1) {
         close(fd);
     }
    return rv;
}

static void signal_handler(int sig)
{
    // UNUSED(sig);  // expected to be SIGABRT
    longjmp(vsTestErrExit, 1);
}

static int setup_route_access_info(vplex::vsDirectory::UserStorage& storage,
                                   vplex::vsDirectory::DatasetDetail& dataset,
                                   u64& user_id,
                                   u64& dataset_id,
                                   VSSI_RouteInfo& route_info)
{
    int route_ind = 0;

    // Set up the route info first
    if ( storage.storageaccess_size() == 0 ) {
        VPLTRACE_LOG_INFO(TRACE_APP, 0, "No routes for this storage!");
        return -1;
    }
    route_info.routes = new VSSI_Route[storage.storageaccess_size()];
    route_info.num_routes = 0;
    for( int i = 0 ; i < storage.storageaccess_size() ; i++ ) {
        bool port_found = false;
        for( int j = 0 ; j < storage.storageaccess(i).ports_size() ; j++ ) {
            if ( storage.storageaccess(i).ports(j).porttype() != 
                    vplex::vsDirectory::PORT_VSSI ) {
                continue;
            }
            if ( storage.storageaccess(i).ports(j).port() == 0 ) {
                continue;
            }
            route_info.routes[route_ind].port =
                storage.storageaccess(i).ports(j).port();
            port_found = true;
            break;
        }
        if ( !port_found ) {
            continue;
        }
        route_info.routes[route_ind].server = strdup(storage.storageaccess(i).server().c_str());
        route_info.routes[route_ind].type = storage.storageaccess(i).routetype();
        route_info.routes[route_ind].proto = storage.storageaccess(i).protocol();
        route_info.routes[route_ind].cluster_id = storage.storageclusterid();
        route_ind++;
    }
    if ( route_ind == 0 ) {
        VPLTRACE_LOG_INFO(TRACE_APP, 0, "No VS routes found.");
        return -1;
    }
    route_info.num_routes = route_ind;

    // set up the access info
    if ( !dataset.has_userid() ) {
        VPLTRACE_LOG_INFO(TRACE_APP, 0, "No dataset access info");
        return -1;
    }

    user_id = dataset.userid();
    dataset_id = dataset.datasetid();

    return 0;
}

static int user_login(const string& username,
                      vplex::vsDirectory::SessionInfo& sessionInfo,
                      u64& userId)
{
    int rv = 0;
    
    rv = userLogin(infra_name, infra_port,
                   username, pc_namespace, password,
                   userId, sessionInfo);
    if(rv) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0,
                         "FAIL:" "Failed login: %d uid %s namespace %s password %s", rv,
                         username.c_str(), pc_namespace.c_str(), password.c_str());
        rv = -1;
        goto out;
    }
    VPLTRACE_LOG_INFO(TRACE_APP, 0,
                      "+++ Login for user %s "FMTu64" session "FMTu64" succeeded \n",
                      username.c_str(), userId, sessionInfo.sessionhandle());
    rv = 0;

 out:
    return rv;
}

static void loadgen_user_launch_done(void)
{
    VPLMutex_Lock(&loadgen_mutex);

    usersLaunched++;

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Users launched: %d/%d",
                        usersLaunched, num_users);

    if(usersLaunched == num_users &&
       tasksDone == tasksStarted) {
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                            "All tasks done.");
        VPLSem_Post(&loadgen_sem);
    }

    VPLMutex_Unlock(&loadgen_mutex);
}

static void loadgen_task_done(void* context)
{
    load_task* task = (load_task*)context;

    VPLMutex_Lock(&loadgen_mutex);

    if(task->taskType == TASK_WRITER) {
        writersDone++;

        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                            "Writer task[%s] done. %d/%d complete.",
                            task->taskName.c_str(),
                            writersDone, writersStarted);

        if(usersLaunched == num_users &&
           writersDone == writersStarted) {
            loadTaskParameters.wait_for_writers = false;
            VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                                "Writer tasks done.");
        }
    }
    tasksDone++;

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Task[%s] done. %d/%d complete.",
                        task->taskName.c_str(),
                        tasksDone, tasksStarted);

    if(usersLaunched == num_users &&
       tasksDone == tasksStarted) {
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                            "All tasks done.");
        VPLSem_Post(&loadgen_sem);
    }

    VPLMutex_Unlock(&loadgen_mutex);
}

// TODO: only support single login session now
#if 0
static void launch_task(void* ctx)
{
    load_task* task = (load_task*)ctx;
    int rv;

    // Login for this task
    rv = user_login(task->username, task->sessionInfo, task->userId);
    if(rv < 0) {
        goto exit;
    }
    
    task->session = VSSI_RegisterSession(task->accessHandle,
                                         task->deviceTicket.c_str());
    if(task->session == 0) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Registering VSS session failed: %d\n",
                         rv);
        goto exit;
    }

    // Launch load task.
    if(vsTest_worker_add_task(cloud_load_test, ctx) != 0) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Queuing load task %s failed!", 
                         task->taskName.c_str());
        goto exit;
    }

    return;
 exit:
    loadgen_task_done(ctx);
}
#endif

static void launch_user(void* ctx)
{
// TODO: only support single user now
#if 0
    int userIndex = (int)(ctx);
    string username;
    char uname[255];
#endif
    int i;
    vplex::vsDirectory::SessionInfo sessionInfo;
    u64 userId;
    int rv;
    VSSI_Session session;
    int retries = 0;
    u64 test_device_id = 0;
    vplex::vsDirectory::SessionInfo *req_session;
    vplex::vsDirectory::ListOwnedDataSetsInput listDatasetReq;
    vplex::vsDirectory::ListOwnedDataSetsOutput listDatasetResp;
    vplex::vsDirectory::ListUserStorageInput listStorReq;
    vplex::vsDirectory::ListUserStorageOutput listStorResp;
    vplex::vsDirectory::LinkDeviceInput linkReq;
    vplex::vsDirectory::LinkDeviceOutput linkResp;

    string datasetLocation;
    u64 datasetId = 0;
    u64 accessHandle = 0;
    std::string ticket;
    vplex::vsDirectory::DatasetDetail dataset;
    vplex::vsDirectory::UserStorage storage;
    VSSI_RouteInfo routeInfo;
    VPLVsDirectory_ProxyHandle_t* vsds_proxy = NULL;

// TODO: only support single user now
#if 0
    if(get_username(userIndex + user_id_start, uname, sizeof(uname))) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: creating user for %d", 
                         userIndex);
        rv = -1;
        goto exit;
    }
    username.assign(uname);
#endif
    rv = user_login(username, sessionInfo, userId);
    if(rv < 0) {
        goto exit;
    }

    rv = registerAsDevice(infra_name, infra_port, username, password,
                          test_device_id);
    if (rv != 0) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Register as devie.");
        rv = -1;
        goto exit;
    }

    // Init VSSI library
    // TODO: Currently since a single vssi library could only be used for single device, 
    // so mutilple user login is not possible now.
    rv = do_vssi_setup(test_device_id);
    if(rv != 0) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                     "Initialization of VSSI.");
        rv = -1;
        goto exit;
    }
    if(!all_one_connection) {
        // Set no connection reuse mode.
        // Each login session will use separate TCP connections.        
        VSSI_SetParameter(VSSI_PARAM_REUSE_CONNECTIONS, 0);
    }

    req_session = linkReq.mutable_session();
    *req_session = sessionInfo;
    linkReq.set_userid(userId);
    linkReq.set_deviceid(test_device_id);
    linkReq.set_hascamera(false);
    linkReq.set_isacer(true);
    linkReq.set_deviceclass("AndroidPhone");
    linkReq.set_devicename("vstest device");
    
    // Get user's datasets.
    req_session = listDatasetReq.mutable_session();
    *req_session = sessionInfo;
    listDatasetReq.set_userid(userId);
    listDatasetReq.set_version("1.0");

    // Get user storage clusters.
    req_session = listStorReq.mutable_session();
    *req_session = sessionInfo;
    listStorReq.set_userid(userId);

    listStorReq.set_deviceid(test_device_id);

    // XXX bug 9624 VSDS proxy sometimes messes up.  Retry
    while(retries++ < 4) {
        VPLTRACE_LOG_FINE(TRACE_APP, 0,
                          "ListOwnedDatasets attempt with session "FMTu64" user "FMTu64".",
                          listDatasetReq.session().sessionhandle(),
                          listDatasetReq.userid());
        
        VPLMutex_Lock(&vsdsMutex);
        // Grab a proxy. Create one if needed.
        if(vsds_proxies.empty()) {
            vsds_proxy = new VPLVsDirectory_ProxyHandle_t;
            rv = VPLVsDirectory_CreateProxy(infra_name.c_str(), infra_port,
                                            vsds_proxy);
            if (rv) {
                printf("+++ Creating directory proxy failed: %d\n", rv);
                rv = -1;
                goto exit;
            }
        }
        else {
            vsds_proxy = vsds_proxies.top();
            vsds_proxies.pop();
        }
        VPLMutex_Unlock(&vsdsMutex);

        rv = VPLVsDirectory_LinkDevice(*vsds_proxy, VPLTIME_FROM_SEC(30),
                                       linkReq, linkResp);
        if(rv) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "LinkDevice query with session "FMTu64" user "FMTu64" returned %d, detail:%d: %s attempt %d",
                             linkReq.session().sessionhandle(),
                             linkReq.userid(),
                             rv, linkResp.error().errorcode(),
                             linkResp.error().errordetail().c_str(),
                             retries);
        } else {

            rv = VPLVsDirectory_ListOwnedDataSets(*vsds_proxy, VPLTIME_FROM_SEC(30),
                                                  listDatasetReq, listDatasetResp);
            if(rv) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "ListOwnedDatasets query with session "FMTu64" user "FMTu64" returned %d, detail:%d: %s attempt %d",
                                 listDatasetReq.session().sessionhandle(),
                                 listDatasetReq.userid(),
                                 rv, listDatasetResp.error().errorcode(),
                                 listDatasetResp.error().errordetail().c_str(),
                                 retries);
            } else {
                rv = VPLVsDirectory_ListUserStorage(*vsds_proxy, VPLTIME_FROM_SEC(30),
                                                    listStorReq, listStorResp);
                if(rv) {
                    VPLTRACE_LOG_ERR(TRACE_APP, 0, "ListUserStorage query with session "FMTu64" user "FMTu64" returned %d, detail:%d: %s attempt %d",
                                     listStorReq.session().sessionhandle(),
                                     listStorReq.userid(),
                                     rv, listStorResp.error().errorcode(),
                                     listStorResp.error().errordetail().c_str(),
                                     retries);
                }
            }
        }

        VPLMutex_Lock(&vsdsMutex);
        // Put proxy on stack for re-use
        vsds_proxies.push(vsds_proxy);
        VPLMutex_Unlock(&vsdsMutex);

        if(rv) {
            // Try again  insert a pseudo-random delay
            sleep(userId % 7);
        } else {
            break;
        }
    }
    if(rv != 0) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                         "Too many retries on ListOwnedDatasets");
        goto exit;
    }

    // Set dataset to use for each load task
    for(i = 0; i < listDatasetResp.datasets_size(); i++) {
        if(listDatasetResp.datasets(i).datasetname().compare(read_dataset) == 0) {
            datasetLocation = listDatasetResp.datasets(i).datasetlocation();
            datasetId = listDatasetResp.datasets(i).datasetid();
            dataset = listDatasetResp.datasets(i);
            for(int i = 0; i < listStorResp.storageassignments_size(); i++) {
                if(listStorResp.storageassignments(i).storageclusterid() ==
                   dataset.clusterid()) {
                    storage = listStorResp.storageassignments(i);
                    accessHandle = storage.accesshandle();
                    ticket = storage.devspecaccessticket();
                }
            }
            break;
        }
    }
    if(datasetLocation.empty()) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Dataset %s not found.",
                         read_dataset.c_str());
        goto exit;
    }

    // Register session
    session = VSSI_RegisterSession(accessHandle,
                                   ticket.c_str());
    if(session == 0) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Registering VSS session failed: %d\n",
                         rv);
        goto exit;
    }

    setup_route_access_info(storage,
                                dataset,
                                userId,
                                datasetId,
                                routeInfo);
    // Reset dataset if needed.
    if(reset_dataset) {
        rv = cloud_clear_dataset(session, userId,
                                 datasetId,
                                 &routeInfo);
        if(rv != 0) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Clear dataset contents for user %s failed: %d\n",
                             username.c_str(), rv);
// TODO: only support single user now
#if 0
                             "Clear dataset contents for user %d failed: %d\n",
                             userIndex, rv);
#endif
            goto exit;
        }
        VPLTRACE_LOG_INFO(TRACE_APP, 0,
                          "Cleared dataset contents for user %s.",
                          username.c_str());
// TODO: only support single user now
#if 0
                          "Cleared dataset contents for user %d.",
                          userIndex);
#endif
    }
    
    // TODO: Currently since a single vssi library could only be used for single device, 
    // device_id and app_id doesn't support mutiple login session.
    // Just keep the interface here for future using.

    // Configure and launch all tasks.
    // First task launched takes this login and VSSI session.
    for(i = 0; i < (num_readers + num_writers); i++) {
// TODO: only support single user now
#if 0
        int taskId = (userIndex * (num_readers + num_writers)) + i;
#endif
        int taskId = i;
        loadTasks[taskId].datasetLocation = datasetLocation;
        loadTasks[taskId].deviceTicket = ticket;
        loadTasks[taskId].accessHandle = accessHandle;
        loadTasks[taskId].datasetId = datasetId;
        loadTasks[taskId].username = username;
        loadTasks[taskId].session = session;
        loadTasks[taskId].routeInfo = routeInfo;
        loadTasks[taskId].userId = userId;
        loadTasks[taskId].sessionInfo = sessionInfo;
        if(vsTest_worker_add_task(cloud_load_test,
                                  (void*)&(loadTasks[taskId]))
           != 0) {
            rv++;
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Queuing load task %d failed!", i);
            goto exit;
        }

// TODO: only support single login session now
#if 0
        if(vsTest_worker_add_task(launch_task,
                                  (void*)&(loadTasks[taskId]))
           != 0) {
            rv++;
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Queuing load task %d failed!", i);
            goto exit;
        }
#endif
        VPLMutex_Lock(&loadgen_mutex);
        tasksStarted++;
        if(loadTasks[taskId].taskType == TASK_WRITER) {
            writersStarted++;
        }
        VPLMutex_Unlock(&loadgen_mutex);
    }

 exit:
    loadgen_user_launch_done();
}

static const char vsTest_main[] = "Loadgen Main";
int
main(int argc, char* argv[])
{
    int rv = 0; // pass until failed.
    int rc;
    bool test_fail = false;
    int taskCount = 0;

    // Localization for all VS queries.
    l10n.set_language("en");
    l10n.set_country("US");
    l10n.set_region("USA");

    VPLTrace_Init(NULL);
    VPLTrace_SetShowTimeAbs(true);
    VPLTrace_SetShowTimeRel(false);
    VPLTrace_SetShowThread(true);

    VPLMutex_Init(&loadgen_mutex);
    VPLMutex_Init(&vsdsMutex);
    // Init obj mutex.
    VPLMutex_Init(&(load_task::obj_mutex));
    VPLSem_Init(&loadgen_sem, 1, 0);
    vsTest_infra_init();

    vsTest_curTestName = vsTest_main;

    if(parse_args(argc, argv) != 0) {
        goto exit;
    }

    VPLTrace_SetBaseLevel(log_level);

    print_version();
    dump_cmdline(argc, argv);
    dump_params();

    // catch SIGABRT/SIGINT.
    assert((signal(SIGABRT, signal_handler) != SIG_ERR));
    assert((signal(SIGINT, signal_handler) != SIG_ERR));

    if(setjmp(vsTestErrExit) == 0) { // initial invocation of setjmp
        // Run the tests. Any abort signal will hit the else block.

        // Determine number of total tasks to start.
        taskCount = (num_readers + num_writers) * num_users;
        if(taskCount > 0) { // might just be reset
            loadTasks = new (nothrow) load_task[taskCount]();
            if(loadTasks == NULL) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "Allocating task structures.");
                rv = -1;
                goto exit;
            }
        }

// TODO: only support single user now
#if 0
        // TODO: cleanup on exit
        init_usertable(username_prefix);
#endif
        // Set number of worker threads to process tasks.
        if(vsTest_worker_init(worker_threads) != 0) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: Task queue init.");
            rv++;
            goto exit;
        }

        // Set load task parameters
        // If there's a manifest, load it.
        if(!manifest_file.empty() ) {
            if(mani_load(&(loadTaskParameters.manifest),
                         manifest_file.c_str(), manifest_randomize)) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "Loading manifest file");
                rv = -1;
                goto exit;
            }
            loadTaskParameters.file_cnt = loadTaskParameters.manifest.file_cnt;
            loadTaskParameters.use_manifest = true;
        }
        else {
            // Constant-sized files
            loadTaskParameters.file_size = file_size_k * 1024;
            loadTaskParameters.file_cnt = file_cnt;
            loadTaskParameters.use_manifest = false;
        }
        loadTaskParameters.use_metadata = use_metadata;
        loadTaskParameters.xfer_size = xfer_size_k * 1024;
        loadTaskParameters.wait_for_writers = true;
        loadTaskParameters.commit_interval = commit_interval;
        loadTaskParameters.commit_size = commit_size_k * 1024;
        loadTaskParameters.verify_read = verify_read;
        loadTaskParameters.verify_write = verify_write;
        loadTaskParameters.callback = loadgen_task_done;
        loadTaskParameters.thumb_size = thumb_size;
        loadTaskParameters.write_thumb = write_thumb;
        loadTaskParameters.base_dir = base_dir;
        loadTaskParameters.thumb_dir = thumb_dir;
        loadTaskParameters.read_dir = base_read_dir;
        if(!photo_source_file.empty()) {
            loadTaskParameters.source_data =
                load_file(photo_source_file, 
                          loadTaskParameters.file_size);
        }
        else {
            loadTaskParameters.source_data = NULL;
        }

        // Initialize tasks.
        for(u32 i = 0; i < taskCount; i++) {
            stringstream taskName;
            int userId = i / (num_readers + num_writers);
            int taskCopy = (i % (num_readers + num_writers));
            bool writer = (taskCopy < num_writers) ? true : false;
            if(writer) {
                taskName << "W" << userId << "." << taskCopy;
                loadTasks[i].taskType = TASK_WRITER;
            }
            else {
                taskCopy -= num_writers;
                taskName << "R" << userId << "." << taskCopy;
                loadTasks[i].taskType = TASK_READER;
            }
            loadTasks[i].taskName = taskName.str();
            loadTasks[i].context = (void*)(&loadTasks[i]);
            loadTasks[i].parameters = &loadTaskParameters;
        }

        // If no writers, readers only scan once.
        if(num_writers == 0) {
            loadTaskParameters.wait_for_writers = false;
        }

        // Set-up user-launch task per user.
        // TODO: Currently since a single vssi library could only be used for single device, 
        // so mutilple user login is not possible now.
        // Just keep the interface here for future using.
        for(u32 i = 0; i < num_users; i++) {
            if(vsTest_worker_add_task(launch_user, (void*)(i)) != 0) {
                rv++;
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "Queuing user-launch task %d failed!", i);
                goto exit;
            }
        }

        // Start workers.
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                            "Starting %d tasks at "FMT_VPLTime_t".",
                            taskCount, start);
        start = VPLTime_GetTimeStamp();
        vsTest_worker_start();

        // wait for everyone to finish up.
        VPLSem_Wait(&loadgen_sem);

        for(u32 i = 0 ; i < tasksStarted ; i++) {
            if(loadTasks[i].failed) {
                test_fail = true;
                rv++;
            }
        }
        stop = VPLTime_GetTimeStamp();

        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "*** All tasks complete, rv: %d", rv);
        for(u32 i = 0 ; i < tasksStarted ; i++ ) {
            rc = VSSI_EndSession(loadTasks[i].session);
            if(rc != VSSI_SUCCESS) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "Task[%s]: EndSession failed %d",
                                 loadTasks[i].taskName.c_str(), (int) rc);
            }
        }

        do_vssi_cleanup();
    }
    else {
        VPLTRACE_LOG_ERR(TRACE_APP, 0,
                         "*** Test Aborted.");
        exit(1);
    }

    dump_params();
    print_stats(start, stop);
    dump_cmdline(argc, argv);
    
    if(loadTasks) {
        delete[] loadTasks;
    }

 exit:
    printf("+++ Test %s\n", (rv == 0) ? "PASS" : "FAIL");

    return (rv) ? 1 : 0;
}


// TODO: only support single user now
#if 0
static char *usernames_data = NULL;
static char **usernames_list;
static int usernames_count;

/*
 * If userprefix  is a file, read it to provide a list of usernames.
 * Otherwise see if we need to tack on a format element.
 */
static int
init_usertable(std::string& userprefix)
{
    VPLFS_stat_t sb;
    VPLFile_handle_t fp = VPLFILE_INVALID_HANDLE;
    int i, j, nr, n, rv;

    VPLTRACE_LOG_FINE(TRACE_APP, 0, "user file %s", userprefix.c_str());
    if (access(userprefix.c_str(), R_OK) != 0) {
        if (strstr(username_prefix.c_str(), "%d") == NULL) {
            username_prefix += "%d";
        }
        rv = 0;
        goto done;
    }

    if (VPLFS_Stat(userprefix.c_str(), &sb) != 0) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "bad user file %s", userprefix.c_str());
        rv = -1 ;
        goto done;
    }
    VPLTRACE_LOG_FINE(TRACE_APP, 0, "found user file %s", userprefix.c_str());
    usernames_data = (char *) malloc(sb.size);
    if (usernames_data == NULL) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "Malloc for user data");
        rv = -1;
        goto done;
    }
    if ( (fp = VPLFile_Open(userprefix.c_str(), VPLFILE_OPENFLAG_READONLY, 0)) == VPLFILE_INVALID_HANDLE) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "Failed opening %s", userprefix.c_str());
        rv = -1;
    }
    nr = 0;
    while ((n  = VPLFile_Read(fp, usernames_data + nr, sb.size - nr)) > 0) {
        nr += n;
    }
    if (nr != sb.size) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "reading usernames nr %d sb.size %d", nr, (int) sb.size);
        rv = -1;
        goto done;
    }
    VPLFile_Close(fp);
    usernames_count = 0;
    for (i =0 ; i < nr; i++) {
        if (usernames_data[i]  == '\n') {
            usernames_data[i] = '\0';
            usernames_count++;
        }
    }
    usernames_list = (char **) malloc(sizeof(usernames_list[0]) * usernames_count);
    j = 0;
    i = 0;
    while (i < nr && j < usernames_count) {
        usernames_list[j] = &usernames_data[i];
        n = strlen(usernames_list[j]);
        if (n > 64) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "Bad user %s too long", usernames_list[j]);
            rv = -1;
            goto done;
        }
        i += strlen(usernames_list[j]) + 1;
        j++;
    }
    VPLTRACE_LOG_FINE(TRACE_APP, 0, "reading usernames count %d, %s", usernames_count, usernames_list[0]);
    rv = 0;
done:
    if (rv != 0) {
        if (usernames_list) {
            free(usernames_list);
            usernames_list = NULL;
        }
        if (usernames_data) {
            free(usernames_data);
            usernames_data = NULL;
        }
        usernames_count = 0;
    }
    return (rv);
}

/*
 * Fill outbuf with a username based on 'id'.
 */
static int
get_username(int id, char *outbuf, int len)
{
    int rv = 0;
    memset(outbuf, 0, len);

    if (!usernames_list) {
        snprintf(outbuf, len, username_prefix.c_str(), id);
    }
    else {
        if (id < 0 || id > usernames_count) {
            rv = -1;
        }
        else {
            strncpy(outbuf, usernames_list[id], len);
        }
    }
    outbuf[len - 1] = '\0';
    return rv;
}
#endif
