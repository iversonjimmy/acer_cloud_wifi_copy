/*
 *  Copyright 2013 Acer Cloud Technology Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER CLOUD TECHNOLOGY INC.
 *
 */

/// Proxy load generator
///
/// This unit test exercises the proxy server, issuing raw proxy requests
/// and then driving traffic over the request as requested.
///
/// At the end of each exchange, a report is generated with the raw timestamps
/// needed to tabulate bandwidth used and latency added. 
///
/// Each time a report is generated, the bandwidth measured is compared to the target bandwidth.
/// * If measurement > target by 10%, another thread is spawned to increase load.
/// * If measurement < target by 10%, the Nth thread to report below target is terminated,
///   where N is the number of running threads.
/// * If measurement = target +/- 10%, after N reports == # threads running, the test ends.
///
/// Multiple concurrent runs may be performed between different test/target pairs to spread the load.
/// The results of all runs  may then be combined for a final result.

#include "loadgen_proxy.hpp"

#include "vpl_net.h"
#include "vplex_trace.h"
#include "vplex_assert.h"
#include "vplex_vs_directory.h"
#include "vplex_file.h"
#include "vpl_conv.h"
#include "vplex_serialization.h"
#include "vplex_math.h"

#include <iostream>
#include <sstream>
#include <string>
#include <set>
#include <vector>

/// These are Linux specific non-abstracted headers.
/// TODO: provide similar functionality in a platform-abstracted way.
#include <setjmp.h>
#include <signal.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "vsTest_infra.hpp"
#include "vsTest_vscs_common.hpp"
#include "test_server.hpp"

#include "vssi.h"
#include "vssi_error.h"
#include "vss_comm.h"

#define ABS(x) (((x) < 0) ? (-(x)) : (x))

using namespace std;

static int log_level = TRACE_WARN;

/// Abort error catching
static jmp_buf errorExit;
static void signal_handler(int sig)
{
    UNUSED(sig);  // expected to be SIGABRT
    longjmp(errorExit, 1);
}

string infra_name = "www-c100.pc-int.igware.net";  /// VSDS server
string infra_domain = "pc-int.igware.net";
u16 infra_port = 443;

/// Test parameters passed by command line
string username = "load_test_user";   /// Default username
string password = "password"; /// Default password
/// Session parameters picked up on successful login
VPLUser_Id_t uid = 0;
vplex::vsDirectory::SessionInfo session;
string iasTicket;

u64 testDeviceId = 0;  // to be obtained from the infra at runtime

/// localization to use for all commands.
vplex::vsDirectory::Localization l10n;

/// Ways to reach the PSN
u64 psnDeviceId = 0;
u64 psnSessionHandle;
string psnServiceTicket;
string psnProxyAddress;
u16 psnProxyPort = 0;

// VSSI session for making VSSI calls.
VSSI_Session vssi_session;

bool server_mode = false;

u64 upcnt = 0;
u64 downcnt = 0;
u64 target_bandwidth = 300;
int target_tolerance = 5; // percent
int target_clients = 0;// bandwidth target by default
int p2p_ratio = 0; // no P2P clients by default (percent)

VPLMutex_t mutex;
VPLSem_t done_sem;
VPLTime_t connect_accumulator = 0; // Total time spent waiting for proxy connections
int connect_attempts = 0; // Number of connection attempts made
int connect_failures = 0; // Number of attempts that ended in failure
int connect_in_progress = 0; // Number of connect attempts in progress
#define MAX_CONNECT_IN_PROGRESS 50 // maximum in-progress at a time, to keep from flooding ANS, client.
VPLTime_t p2p_connect_accumulator = 0; // Total time spent waiting for P2P connections
int p2p_connect_attempts = 0; // Number of P2P connection attempts made
int p2p_connect_failures = 0; // Number of P2P attempts that ended in failure
u64 rate_accumulator = 0; // accumulation of rates when measuring results
VPLTime_t latency_accumulator = 0; // accumulation of latency when measuring results
int final_threads = 0; // number of threads at equilibrium; final result
int measured_threads = 0; // number of threads measured
bool measurements_done = false; // measurements done, winding down test
VPLTime_t last_threadcount_change_time = 0;
int num_threads = 0;  // number of threads currently running
int max_threads = 0; // 0 means max not determined.
int min_threads = 0; // 0 means min not determined (also actual minimum)
int adjust_threads = 0; // number of threads to add/remove

VPLThread_attr_t thread_attrs;
VPLTHREAD_FN_DECL proxy_client_main(void*);

static void usage(int argc, char* argv[])
{
    // Dump original command

    // Print usage message
    printf("Usage: %s [options]\n", argv[0]);
    printf("This utility may be used in several ways.\n"
           "* Determine number of proxy clients supportable at a target bandwidth within prcentage tolerance.\n"
           "* Start set number of clients that attempt/use proxy connection."
           "* Mix-in a percentage of P2P clients along with the proxy clients.\n");
    printf("Options:\n");
    printf(" -v --verbose               Raise verbosity one level (may repeat 3 times)\n");
    printf(" -t --terse                 Lower verbosity (make terse) one level (may repeat 2 times or cancel a -v flag)\n");
    printf(" -i --infra-name NAME       Infrastructure server name or IP address (%s)\n",
           infra_name.c_str());
    printf(" -I --infra-port PORT       Infrastructure server port (%d)\n",
           infra_port);
    printf(" -u --username USERNAME     User name (%s)\n",
           username.c_str());
    printf(" -p --password PASSWORD     User password (%s)\n",
           password.c_str());
    printf(" -S --server-target         Act as a target server (storage node) for test by another instance. (%s).\n",
           server_mode ? "yes" : "no");
    printf(" -U --upload-bytes          Bytes to send per upload action ("FMTu64")\n", 
           upcnt);
    printf(" -D --download-bytes        Bytes to receive per download action ("FMTu64")\n",
           downcnt);
    printf(" -B --bandwidth-target      Target bandwidth to reach per task, in KiB/s.\n"
           "                            Zero means generate clients until exhaustion. ("FMTu64")\n", 
           target_bandwidth);
    printf(" -T --tolerance             Measurement tolerance (%d%%)\n",
           target_tolerance);
    printf(" -Q --quantity              Start a set quantity of clients. When nonzero, -B and -T are ignored (%d).\n",
           target_clients);
    printf(" -P --p2p-ratio             Make a percentage of started clients P2P clients. \n"
           "                            NOTE: P2P clients disconnect after completing P2P connection. No traffic driven.(%d%%)\n",
           p2p_ratio);

    printf("Command entered:");
    for(int i = 0; i < argc; i++) {
        printf(" %s", argv[i]);
    }
    printf("\n");
}

static int parse_args(int argc, char* argv[])
{
    int rv = 0;

    static struct option long_options[] = {
        {"verbose", no_argument, 0, 'v'},
        {"terse", no_argument, 0, 't'},
        {"username", required_argument, 0, 'u'},
        {"password", required_argument, 0, 'p'},
        {"infra-name", required_argument, 0, 'i'},
        {"infra-port", required_argument, 0, 'I'},
        {"upload-bytes", required_argument, 0, 'U'},
        {"download-bytes", required_argument, 0, 'D'},
        {"target-bandwidth", required_argument, 0, 'B'},
        {"target-tolerance", required_argument, 0, 'T'},
        {"quantity", required_argument, 0, 'Q'},
        {"p2p-ratio", required_argument, 0, 'P'},
        {"server-target", no_argument, 0, 'S'},
        {0,0,0,0}
    };

    for(;;) {
        int option_index = 0;

        int c = getopt_long(argc, argv, "vtu:p:i:I:U:D:B:T:Q:P:S",
                            long_options, &option_index);

        if (c == -1)
            break;

        switch (c) {
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
        case 'u':
            username = optarg;
            break;

        case 'p':
            password = optarg;
            break;

        case 'i':
            infra_name = optarg;
            infra_domain = infra_name.substr(infra_name.find_first_of(".") + 1);
            break;

        case 'I':
            infra_port = atoi(optarg);
            break;

        case 'U':
            upcnt = strtoull(optarg, 0, 10);
            break;

        case 'D':
            downcnt = strtoull(optarg, 0, 10);
            break;

        case 'B':
            target_bandwidth = strtoull(optarg, 0, 10);
            break;

        case 'T':
            target_tolerance = strtoul(optarg, 0, 10);
            break;

        case 'Q':
            target_clients = strtoul(optarg, 0, 10);
            break;

        case 'P':
            p2p_ratio = strtoul(optarg, 0, 10);
            break;

        case 'S':
            server_mode = true;
            break;

        default:
            usage(argc, argv);
            rv++;
            break;
        }
    }

    return rv;
}

static bool report_rate(u64 throughput, VPLTime_t latency, VPLTime_t pass_start_time)
{
    bool rv = true;

    VPLMutex_Lock(&mutex);

    VPLTRACE_LOG_INFO(TRACE_APP, 0,
                        "threads:%d/%d rate:%llu.%03llukiB/s latency:"FMT_VPLTime_t".%03llums min:%d max:%d adjust: %d done:%s",
                        measured_threads, num_threads, 
                        throughput / 1000, throughput % 1000,
                        latency / 1000, latency % 1000,
                        min_threads, max_threads, adjust_threads,
                      measurements_done ? "true" : "false");

    if(measurements_done) {
        // Already met goal. End threads.
        rv = false;
    }
    else if(target_clients > 0) {
        if(num_threads < target_clients) {
            if(connect_in_progress < MAX_CONNECT_IN_PROGRESS) {
                num_threads++;
                connect_in_progress++;
                VPLDetachableThread_Create(NULL, 
                                           proxy_client_main, 
                                           NULL,
                                           &thread_attrs,
                                           "client");
                VPLTRACE_LOG_INFO(TRACE_APP, 0,
                                  "%d/%d threads launched. Throughput:"FMTs64".%03llu/"FMTu64"kb/s",
                                  num_threads, target_clients,
                                  throughput / 1000, throughput % 1000, target_bandwidth);
            }
        }
        else if(pass_start_time < last_threadcount_change_time) {
            VPLTRACE_LOG_INFO(TRACE_APP, 0, "Skipped a measurement. "FMT_VPLTime_t" before "FMT_VPLTime_t".",
                              pass_start_time, last_threadcount_change_time);
        }
        else {
            rate_accumulator += throughput;
            latency_accumulator += latency;
            measured_threads++;

            if(measured_threads == num_threads) {
                final_threads = measured_threads;
                measurements_done = true;
            }
        }
    }
    else { // throughput-seeking mode
        if(adjust_threads) {
            VPLTRACE_LOG_INFO(TRACE_APP, 0, "Making adjustment.");

            if(adjust_threads < 0) {
                // adjust workers down
                rv = false;
                adjust_threads++;
            }
            else if(connect_in_progress < MAX_CONNECT_IN_PROGRESS) {
                // spawn additional worker
                num_threads++;
                connect_in_progress++;
                adjust_threads--;
                VPLDetachableThread_Create(NULL, 
                                           proxy_client_main, 
                                           NULL,
                                           &thread_attrs,
                                           "client");
                VPLTRACE_LOG_FINE(TRACE_APP, 0,
                                  "Launched thread %d.",
                                  num_threads);
            }
        }
        else if(pass_start_time < last_threadcount_change_time) {
            VPLTRACE_LOG_INFO(TRACE_APP, 0, "Skipped a measurement. "FMT_VPLTime_t" before "FMT_VPLTime_t".",
                              pass_start_time, last_threadcount_change_time);
            rate_accumulator = 0;
            latency_accumulator = 0;
            measured_threads = 0;
        }
        else {
            rate_accumulator += throughput;
            latency_accumulator += latency;
            measured_threads++;

            if(measured_threads == num_threads) {
                // evaluate current results
                u64 avg_rate = rate_accumulator / measured_threads;
                u64 avg_latency = latency_accumulator / measured_threads;
                int target_threads = rate_accumulator / (target_bandwidth * 1000);
                if(ABS(((s64)avg_rate) - ((s64)target_bandwidth * 1000)) * 100 <
                   (target_bandwidth * 1000 * target_tolerance)) {
                    // Within allowed tolerance. Test over.
                    final_threads = measured_threads;
                    measurements_done = true;
                }
                else if(max_threads != 0 && ABS(max_threads - min_threads) <= 1) {
                    // No room to adjust. Consider test done.
                    final_threads = measured_threads;
                    measurements_done = true;                    
                }
                else {
                    if(avg_rate > (target_bandwidth * 1000)) {
                        min_threads = measured_threads;
                    }
                    else {
                        max_threads = measured_threads;
                    }

                    if(max_threads != 0 && target_threads > max_threads) {
                        // Target is over max. Stop.
                        final_threads = measured_threads;
                        measurements_done = true;
                    }
                    else if(min_threads != 0 && target_threads < min_threads) {
                        // Target is under min. Stop.
                        final_threads = measured_threads;
                        measurements_done = true;
                    }
                    else {
                        adjust_threads = target_threads - measured_threads;
                    }

                    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                                        "threads:%d rate:%llu.%03llukiB/s vs. %lluKiB/s latency:"FMT_VPLTime_t".%03llums min:%d max:%d adjust: %d",
                                        measured_threads, 
                                        avg_rate / 1000, avg_rate % 1000, target_bandwidth,
                                        avg_latency / 1000, avg_latency % 1000,
                                        min_threads, max_threads, adjust_threads);

                    rate_accumulator = 0;
                    latency_accumulator = 0;
                    measured_threads = 0;
                }
            }
        }
    }

    VPLMutex_Unlock(&mutex);

    return rv;
}

static void report_connect(bool fail, bool p2p, VPLTime_t time)
{
    VPLMutex_Lock(&mutex);
    
    connect_in_progress--;
    if(p2p) {
        p2p_connect_attempts++;
        if(fail) {
            p2p_connect_failures++;
        }        
        else {
            p2p_connect_accumulator += time;
        }
    }
    else {
        connect_attempts++;
        if(fail) {
            connect_failures++;
        }
        else {
            connect_accumulator += time;
        }
    }
    last_threadcount_change_time = VPLTime_GetTimeStamp();
    measured_threads = 0;
    rate_accumulator = 0;
    latency_accumulator = 0;
    VPLMutex_Unlock(&mutex);
}

static void report_done(bool failed)
{
    bool test_complete = false;

    VPLMutex_Lock(&mutex);
    
    VPLTRACE_LOG_INFO(TRACE_APP, 0,
                        "Terminate thread %d%s.",
                        num_threads, failed ? " with failure" : "");
    
    num_threads--;

    if(num_threads == 0) {
        test_complete = true;
    }

    if(!measurements_done) {
        last_threadcount_change_time = VPLTime_GetTimeStamp();
        // Forced to start over after thread failure.
        measured_threads = 0;
        rate_accumulator = 0;
        latency_accumulator = 0;
    }

    VPLMutex_Unlock(&mutex);
    if(test_complete) {
        VPLSem_Post(&done_sem);
    }
}

typedef struct  {
    VPLSem_t sem;
    int rv;
} test_context_t;

static void test_callback(void* ctx, VSSI_Result rv)
{
    test_context_t* context = (test_context_t*)ctx;

    context->rv = rv;
    VPLSem_Post(&(context->sem));
}

VPLTHREAD_FN_DECL proxy_client_main(void*)
{
    VPLSocket_t socket = VPLSOCKET_INVALID;
    test_context_t context;
    bool failure = true;
    bool use_p2p = ((VPLMath_Rand() % 100) < p2p_ratio) ? true : false;
    
    enum {
        SEND_START,
        SEND_END,
        SEND_WAIT_TOTAL,
        RECV_START,
        RECV_END,
        RECV_WAIT_TOTAL,        
        NUM_TIMESTAMPS
    };
    VPLTime_t query_times[NUM_TIMESTAMPS] = {0};
    VPLTime_t response_times[NUM_TIMESTAMPS] = {0};
    VPLTime_t pass_start_time;
    const int buflen = 16 * 1024;
    char* buffer = new char[buflen];
    u64 progress = 0;
    u64 value;
    VPLSocket_poll_t pollInfo[1];
    int rc;
    int iteration = 0;
    VPLTime_t connect_start;
    VPLTime_t connect_end;
    u64 throughput;
    VPLTime_t latency;
    int is_direct;
    VPLSem_Init(&(context.sem), 1, 0);
    memset(buffer, 0, buflen);

    // Request proxy connection
    connect_start = VPLTime_GetTime();
    VSSI_OpenProxiedConnection(vssi_session, 
                               psnProxyAddress.c_str(),
                               psnProxyPort,
                               psnDeviceId,
                               0xff, // test traffic
                               use_p2p,
                               &socket,
                               &is_direct,
                               &context,
                               test_callback);

    VPLSem_Wait(&context.sem);
    connect_end = VPLTime_GetTime();
    VPLSem_Destroy(&context.sem);
    
    report_connect(VPLSocket_Equal(socket, VPLSOCKET_INVALID) ||
                   (use_p2p && !is_direct), 
                   use_p2p, connect_end - connect_start);

    if(VPLSocket_Equal(socket, VPLSOCKET_INVALID)) {
        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                            "FAIL: Open proxy connection.");
        goto exit;
    }
    
    if(use_p2p && !is_direct) {
        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                            "FAIL: P2P attempt ended with proxy connection.");
        goto exit;
    }

    VPLTRACE_LOG_FINE(TRACE_APP, 0,
                        "Starting proxy worker on socket "FMT_VPLSocket_t" for up/down "FMTu64"/"FMTu64".",
                        VAL_VPLSocket_t(socket), upcnt, downcnt);
    VPLTRACE_LOG_FINE(TRACE_APP, 0,
                        "\n\tConnect: S="FMT_VPLTime_t" E="FMT_VPLTime_t" D="FMT_VPLTime_t,
                        connect_start, connect_end, connect_end - connect_start);

    do {
        pass_start_time = VPLTime_GetTimeStamp();
        iteration++;
        VPLTRACE_LOG_FINE(TRACE_APP, 0,
                          "Proxy worker iteration %d socket "FMT_VPLSocket_t".",
                          iteration, VAL_VPLSocket_t(socket));

        pollInfo[0].socket = socket;
        pollInfo[0].events = VPLSOCKET_POLL_OUT;

        // Send activity query
        value = VPLConv_hton_u64(upcnt);
        if(VPLSocket_Poll(pollInfo, 1, VPL_TIMEOUT_NONE) != 1) { goto exit; }
        if(sizeof(value) != VPLSocket_Send(socket, &value, sizeof(value))) { goto exit; }
        value = VPLConv_hton_u64(downcnt);
        if(VPLSocket_Poll(pollInfo, 1, VPL_TIMEOUT_NONE) != 1) { goto exit; }
        if(sizeof(value) != VPLSocket_Send(socket, &value, sizeof(value))) { goto exit; }

        // Send upload data        
        query_times[SEND_START] = VPLTime_GetTime();
        query_times[SEND_WAIT_TOTAL] = 0;
        progress = 0;
        while(progress < upcnt) {
            VPLTime_t wait_start = VPLTime_GetTime();
            rc = VPLSocket_Poll(pollInfo, 1, VPL_TIMEOUT_NONE);
            query_times[SEND_WAIT_TOTAL] += (VPLTime_GetTime() - wait_start);

            switch(rc) {
            case 1:
                if(pollInfo[0].revents & (VPLSOCKET_POLL_ERR | VPLSOCKET_POLL_HUP)) {
                    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                        "FAIL: Socket error: %d.", pollInfo[0].revents);
                    goto exit;
                }

                rc = VPLSocket_Send(socket, buffer, 
                                    (buflen < (upcnt - progress)) ? 
                                    buflen : upcnt - progress);
                if(rc > 0) {
                    progress += rc;
                }
                else if(rc == VPL_ERR_AGAIN || rc == VPL_ERR_INTR) {
                    // Just try again.
                }
                else {
                    // Broken.
                    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                        "FAIL: Socket error on send: %d.", rc);
                                        
                    goto exit;
                }
                break;
            case 0: // Timeout. Huh?
            case VPL_ERR_AGAIN:
            case VPL_ERR_INTR:
                // Just try again.
                break;
            case VPL_ERR_INVALID:
            default:
                // Broken.
                VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                    "FAIL: Socket error on poll: %d.", rc);
                goto exit;
                break;
            }
        }
        query_times[SEND_END] = VPLTime_GetTime();

        // Receive activity report
        pollInfo[0].socket = socket;
        pollInfo[0].events = VPLSOCKET_POLL_RDNORM;

        if(VPLSocket_Poll(pollInfo, 1, VPL_TIMEOUT_NONE) != 1) { goto exit; }
        if(sizeof(value) != VPLSocket_Recv(socket, &value, sizeof(value))) { goto exit; }
        query_times[RECV_START] = VPLConv_ntoh_u64(value);
        if(VPLSocket_Poll(pollInfo, 1, VPL_TIMEOUT_NONE) != 1) { goto exit; }
        if(sizeof(value) != VPLSocket_Recv(socket, &value, sizeof(value))) { goto exit; }
        query_times[RECV_END] = VPLConv_ntoh_u64(value);
        if(VPLSocket_Poll(pollInfo, 1, VPL_TIMEOUT_NONE) != 1) { goto exit; }
        if(sizeof(value) != VPLSocket_Recv(socket, &value, sizeof(value))) { goto exit; }
        query_times[RECV_WAIT_TOTAL] = VPLConv_ntoh_u64(value);

        // Receive response data
        response_times[RECV_START] = VPLTime_GetTime();
        response_times[RECV_WAIT_TOTAL] = 0;
        progress = 0;
        while(progress < downcnt) {
            VPLTime_t wait_start = VPLTime_GetTime();
            rc = VPLSocket_Poll(pollInfo, 1, VPL_TIMEOUT_NONE);
            response_times[RECV_WAIT_TOTAL] += (VPLTime_GetTime() - wait_start);

            switch(rc) {
            case 1:
                if(pollInfo[0].revents & (VPLSOCKET_POLL_ERR | VPLSOCKET_POLL_HUP)) {
                    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                        "FAIL: Socket error: %d.", pollInfo[0].revents);
                    goto exit;
                }

                rc = VPLSocket_Recv(socket, buffer, 
                                    (buflen < (downcnt - progress)) ? 
                                    buflen : downcnt - progress);
                if(rc > 0) {
                    progress += rc;
                }
                else if(rc == VPL_ERR_AGAIN || rc == VPL_ERR_INTR) {
                    // Just try again.
                }
                else {
                    // Broken.
                    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                        "FAIL: Socket error on recv: %d.", rc);
                                        
                    goto exit;
                }
                break;
            case 0: // Timeout. Huh?
            case VPL_ERR_AGAIN:
            case VPL_ERR_INTR:
                // Just try again.
                break;
            case VPL_ERR_INVALID:
            default:
                // Broken.
                VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                    "FAIL: Socket error on poll: %d.", rc);
                goto exit;
                break;
            }
        }
        response_times[RECV_END] = VPLTime_GetTime();

        // Receive transmission report
        if(VPLSocket_Poll(pollInfo, 1, VPL_TIMEOUT_NONE) != 1) { goto exit; }
        if(sizeof(value) != VPLSocket_Recv(socket, &value, sizeof(value))) { goto exit; }
        response_times[SEND_START] = VPLConv_ntoh_u64(value);
        if(VPLSocket_Poll(pollInfo, 1, VPL_TIMEOUT_NONE) != 1) { goto exit; }
        if(sizeof(value) != VPLSocket_Recv(socket, &value, sizeof(value))) { goto exit; }
        response_times[SEND_END] = VPLConv_ntoh_u64(value);
        if(VPLSocket_Poll(pollInfo, 1, VPL_TIMEOUT_NONE) != 1) { goto exit; }
        if(sizeof(value) != VPLSocket_Recv(socket, &value, sizeof(value))) { goto exit; }
        response_times[SEND_WAIT_TOTAL] = VPLConv_ntoh_u64(value);

        // Log results
        VPLTRACE_LOG_FINER(TRACE_APP, 0,
                           "\n\tQuery send: S="FMT_VPLTime_t" E="FMT_VPLTime_t" W="FMT_VPLTime_t
                           "\n\tQuery recv: S="FMT_VPLTime_t" E="FMT_VPLTime_t" W="FMT_VPLTime_t
                           "\n\tRspns send: S="FMT_VPLTime_t" E="FMT_VPLTime_t" W="FMT_VPLTime_t
                           "\n\tRspns recv: S="FMT_VPLTime_t" E="FMT_VPLTime_t" W="FMT_VPLTime_t,
                           query_times[SEND_START], query_times[SEND_END], query_times[SEND_WAIT_TOTAL],
                           query_times[RECV_START], query_times[RECV_END], query_times[RECV_WAIT_TOTAL],
                           response_times[SEND_START], response_times[SEND_END], response_times[SEND_WAIT_TOTAL],
                           response_times[RECV_START], response_times[RECV_END], response_times[RECV_WAIT_TOTAL]);
        {
            // Rates computed are kb/s with three-digit fixed-precision.
            u64 uprate = 999999999999ull; // In case of divide by zero, preposterously high rate.
            u64 downrate = 99999999999ull;
            VPLTime_t uplatency = query_times[RECV_START] - query_times[SEND_START];
            VPLTime_t downlatency = response_times[RECV_START] - response_times[SEND_START];
            latency = (uplatency + downlatency) / 2;

            if(query_times[RECV_END] != query_times[RECV_START]) {
                // uprate = (upcnt * 1000 * 1000 * 1000) / (1024 * (query_times[RECV_END] - query_times[RECV_START]));
                uprate = (upcnt * 125 * 125 * 125) / ( 2 * (query_times[RECV_END] - query_times[RECV_START]));
            }

            if(response_times[RECV_END] != response_times[RECV_START]) {
                // downrate = (downcnt * 1000 * 1000 * 1000) / (1024 * (response_times[RECV_END] - response_times[RECV_START]));
                downrate = (downcnt * 125 * 125 * 125) / ( 2 * (response_times[RECV_END] - response_times[RECV_START]));
            }
            throughput = (((upcnt + downcnt) * 125 * 125 * 125) / 
                          (2 * (query_times[RECV_END] - query_times[RECV_START]) +
                           (response_times[RECV_END] - response_times[RECV_START])));

            VPLTRACE_LOG_FINE(TRACE_APP, 0,
                              "\n\tUpload:   %7llu.%03lluKiB/s Delay:"FMT_VPLTime_t"us."
                              "\n\tDownload: %7llu.%03lluKiB/s Delay:"FMT_VPLTime_t"us."
                              "\n\tOverall:  %7llu.%03lluKiB/s Delay:"FMT_VPLTime_t"us.",
                              uprate / 1000, uprate % 1000, uplatency,
                              downrate / 1000, downrate % 1000, downlatency,
                              throughput / 1000, throughput % 1000, latency);
        }
    } while(report_rate(throughput, latency, pass_start_time) && !use_p2p);
    failure = false;

 exit:
    if(!VPLSocket_Equal(socket, VPLSOCKET_INVALID)) {
        VPLSocket_Close(socket);
    }
    delete buffer;

    report_done(failure);

    return VPL_NULL;
}

int main(int argc, char* argv[])
{
    int rv = 0; // pass until failed.
    int rc;

    VPLVsDirectory_ProxyHandle_t vsds_proxy;
    test_server* server = NULL;
    
    VPLTrace_SetBaseLevel(log_level);
    VPLTrace_SetShowTimeAbs(true);
    VPLTrace_SetShowTimeRel(false);
    VPLTrace_SetShowThread(true);

    if(parse_args(argc, argv) != 0) {
        goto exit;
    }

    VPLTrace_SetBaseLevel(log_level);
    VPLMath_InitRand();

    // Print out configuration used for recordkeeping.
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Running %s with the following options:", argv[0]);
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "     Infra Server: %s:%d",
                        infra_name.c_str(), infra_port);
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "     User: %s", username.c_str());
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "     Password: %s", password.c_str());
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "     Up/Down/target: "FMTu64"/"FMTu64"/"FMTu64,
                        upcnt, downcnt, target_bandwidth);
    
    // Localization for all VS queries.
    l10n.set_language("en");
    l10n.set_country("US");
    l10n.set_region("USA");

    // catch SIGABRT
    ASSERT((signal(SIGABRT, signal_handler) == 0));

    if (setjmp(errorExit) == 0) { // initial invocation of setjmp
        // Run the tests. Any abort signal will hit the else block.

        // Login the user. Required for all further operations.
        vsTest_infra_init();
        rc = userLogin(infra_name, infra_port,
                       username, "acer", password, uid, session, &iasTicket);
        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                            "TC_RESULT = %s ;;; TC_NAME = User_Login",
                            rc ? "FAIL" : "PASS");
        if(rc != 0) {
            rv++;
            goto exit;
        }

        rc = registerAsDevice(infra_name, infra_port, username, password,
                              testDeviceId);
        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                            "TC_RESULT = %s ;;; TC_NAME = Register_As_Device",
                            rc ? "FAIL" : "PASS");
        if (rc != 0) {
            rv++;
            goto exit;
        }

        // Create VSDS proxy.
        rc = VPLVsDirectory_CreateProxy(infra_name.c_str(), infra_port,
                                        &vsds_proxy);
        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                            "TC_RESULT = %s ;;; TC_NAME = Create_VSDS_Proxy",
                            rc ? "FAIL" : "PASS");
        if(rc != 0) {
            rv++;
            goto exit;
        }

        {
            // Link device to the user.
            vplex::vsDirectory::SessionInfo* req_session;
            vplex::vsDirectory::LinkDeviceInput linkReq;
            vplex::vsDirectory::LinkDeviceOutput linkResp;

            req_session = linkReq.mutable_session();
            *req_session = session;
            linkReq.set_userid(uid);
            linkReq.set_deviceid(testDeviceId);
            linkReq.set_hascamera(false);
            linkReq.set_isacer(true);
            linkReq.set_deviceclass("DefaultCloudNode");
            linkReq.set_devicename("proxy-load server");
            rc = VPLVsDirectory_LinkDevice(vsds_proxy, VPLTIME_FROM_SEC(30),
                                           linkReq, linkResp);
            if(rc != 0) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "LinkDevice query returned %d, detail:%d:%s",
                                 rc, linkResp.error().errorcode(),
                                 linkResp.error().errordetail().c_str());
                rv++;
                goto fail;
            }

            if(server_mode) {
                // Add this device as a storage node for server use.
                vplex::vsDirectory::CreatePersonalStorageNodeInput storReq;
                vplex::vsDirectory::CreatePersonalStorageNodeOutput storResp;
                req_session = storReq.mutable_session();
                *req_session = session;
                storReq.set_userid(uid);
                storReq.set_clusterid(testDeviceId);
                storReq.set_clustername("Test-device");
                rc = VPLVsDirectory_CreatePersonalStorageNode(vsds_proxy, VPLTIME_FROM_SEC(30),
                                                              storReq, storResp);
                if(rc == -32230) { // already created this PSN
                    VPLTRACE_LOG_WARN(TRACE_APP, 0, "FAIL:"
                                     "CreatePersonalStorageNode query returned %d, detail:%d:%s, continuing.",
                                     rc, storResp.error().errorcode(),
                                     storResp.error().errordetail().c_str());
                }
                else if(rc != 0) {
                    VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                     "CreatePersonalStorageNode query returned %d, detail:%d:%s",
                                     rc, storResp.error().errorcode(),
                                     storResp.error().errordetail().c_str());
                    rv++;
                    goto fail;
                }

                vplex::vsDirectory::AddUserStorageInput query;
                vplex::vsDirectory::AddUserStorageOutput response;
                req_session = query.mutable_session();
                *req_session = session;
                stringstream namestream;
                string name;
                
                namestream << "Test-device-" << hex << uppercase << testDeviceId;
                name = namestream.str();
                
                query.set_userid(uid);
                query.set_storageclusterid(testDeviceId);
                query.set_storagename(name);
                query.set_usagelimit(0);
                
                rv = VPLVsDirectory_AddUserStorage(vsds_proxy,
                                                   VPLTIME_FROM_SEC(30),
                                                   query, response);
                if(rv != 0) {
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                     "VPLVsDirectory_AddUserStorage() failed with code %d.", rv);
                    rv++;
                    goto fail;
                }

                // Inform VSDS of session to use.
                rv = updatePSNConnection(vsds_proxy, uid, testDeviceId, "unknown", 0, 0, session);
                if(rv != 0) {
                    goto fail;
                }
            }
        }

        // Give some time for the device to link.
        // May want to poll VSDS before continuing on?
        sleep(5);

        {
            // Find storage node target, proxy route.
            vplex::vsDirectory::SessionInfo* req_session;
            vplex::vsDirectory::ListUserStorageInput listStorReq;
            vplex::vsDirectory::ListUserStorageOutput listStorResp;
            req_session = listStorReq.mutable_session();
            *req_session = session;

            listStorReq.set_userid(uid);
            listStorReq.set_deviceid(testDeviceId);
            rc = VPLVsDirectory_ListUserStorage(vsds_proxy, VPLTIME_FROM_SEC(30),
                                                listStorReq, listStorResp);
            if(rc != 0) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "ListUserStorage query returned %d, detail:%d:%s",
                                 rc, listStorResp.error().errorcode(),
                                 listStorResp.error().errordetail().c_str());
                rv++;
                goto fail;
            }
            
            VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                                "User "FMTu64" has %d storage assignments.",
                                uid, listStorResp.storageassignments_size());
            
            // Using the first PSN storage cluster, get the PSN's address info.
            for(int i = 0; i < listStorResp.storageassignments_size(); i++) {
                if(listStorResp.storageassignments(i).storagetype() == 1) {
                    psnDeviceId = listStorResp.storageassignments(i).storageclusterid();
                    psnSessionHandle = listStorResp.storageassignments(i).accesshandle();
                    psnServiceTicket = listStorResp.storageassignments(i).devspecaccessticket();
                    for(int j = 0; j < listStorResp.storageassignments(i).storageaccess_size(); j++) {
                        if(listStorResp.storageassignments(i).storageaccess(j).routetype() == vplex::vsDirectory::PROXY) {
                            psnProxyAddress = listStorResp.storageassignments(i).storageaccess(j).server();
                            for(int k = 0; k < listStorResp.storageassignments(i).storageaccess(j).ports_size(); k++) {
                                if(listStorResp.storageassignments(i).storageaccess(j).ports(k).porttype() == vplex::vsDirectory::PORT_VSSI) {
                                    psnProxyPort = listStorResp.storageassignments(i).storageaccess(j).ports(k).port();
                                    break;
                                }
                            }
                            break;
                        }
                    }
                    break;
                }
            }
        }

        if(server_mode) {
            // Launch a server now.
            server = new test_server();
            if(server && server->start()) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "Launch server process.");
                goto exit;
            }
            // Allow ANS conection to stabilize.
            sleep(5);
        }

        // Set-up VSSI library
        {
            vplex::vsDirectory::SessionInfo activeSession;
            activeSession.set_sessionhandle(psnSessionHandle);
            activeSession.set_serviceticket(psnServiceTicket);
            rc = do_vssi_setup(testDeviceId, activeSession, vssi_session);
            VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                "TC_RESULT = %s ;;; TC_NAME = VSSI_Setup",
                                rc ? "FAIL" : "PASS");
            if(rc != 0) {
                rv++;
                goto fail_vssi;
            }
        }

        // Initialize synchronization
        VPLSem_Init(&done_sem, 1, 0);
        VPLMutex_Init(&mutex);

        // Spawn first thread.
        VPLThread_AttrInit(&thread_attrs);
        VPLThread_AttrSetDetachState(&thread_attrs, true);
        VPLThread_AttrSetStackSize(&thread_attrs, 1024*64);
        num_threads = 1;
        VPLDetachableThread_Create(NULL, 
                                   proxy_client_main, 
                                   NULL,
                                   &thread_attrs,
                                   "client");

        // Wait for test completion
        VPLSem_Wait(&done_sem);

        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                            "\n\t%d threads avg bandwidth %llu.%03lluKiB/s average latency "FMT_VPLTime_t".%03llums."
                            "\n\t%d connect attempts %d failures "FMT_VPLTime_t".%03llums average connect time."
                            "\n\t%d P2P connect attempts %d failures "FMT_VPLTime_t".%03llums average connect time.",
                            final_threads, 
                            final_threads == 0 ? 0 : ((rate_accumulator) / final_threads) / 1000,
                            final_threads == 0 ? 0 : ((rate_accumulator) / final_threads) % 1000,
                            final_threads == 0 ? 0 : (latency_accumulator / final_threads) / 1000,
                            final_threads == 0 ? 0 : (latency_accumulator / final_threads) % 1000,
                            connect_attempts, connect_failures, 
                            connect_attempts == 0 ? 0 : (connect_accumulator / connect_attempts) / 1000,
                            connect_attempts == 0 ? 0 : (connect_accumulator / connect_attempts) % 1000,
                            p2p_connect_attempts, p2p_connect_failures, 
                            p2p_connect_attempts == 0 ? 0 : (p2p_connect_accumulator / p2p_connect_attempts) / 1000,
                            p2p_connect_attempts == 0 ? 0 : (p2p_connect_accumulator / p2p_connect_attempts) % 1000);

    fail_vssi:
        //cleanup:
        // Perform cleanup queries.
        do_vssi_cleanup(vssi_session);
    fail:
        {
            // Remove this device.
            vplex::vsDirectory::SessionInfo* req_session;
            vplex::vsDirectory::UnlinkDeviceInput req;
            vplex::vsDirectory::UnlinkDeviceOutput resp;
            req_session = req.mutable_session();
            *req_session = session;
            req.set_userid(uid);
            req.set_deviceid(testDeviceId);
            rc = VPLVsDirectory_UnlinkDevice(vsds_proxy, VPLTIME_FROM_SEC(30),
                                             req, resp);
        }
        
        VPLVsDirectory_DestroyProxy(vsds_proxy);

        if(server) {
            server->stop();
            delete server;
        }
    }
    else {
        rv = 1;
    }

 exit:
    if(final_threads == 0) {
        rv = 1;
    }

    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                        "TC_RESULT = %s ;;; TC_NAME = ProxyLoader",
                        rv ? "FAIL" : "PASS");
    
    return (rv) ? 1 : 0;
}

#include "cslsha.h" // SHA1 HMAC
#include "aes.h"    // AES128 encrypt/decrypt

void compute_hmac(const char* buf, size_t len,
                  const void* key, char* hmac, int hmac_size)
{
    CSL_HmacContext context;
    char calc_hmac[CSL_SHA1_DIGESTSIZE] = {0};

    if(len <= 0) {
        // No signature for empty data. Return 0.
        memset(hmac, 0, hmac_size);
        return;
    }

    CSL_ResetHmac(&context, (u8*)key);
    CSL_InputHmac(&context, buf, len);
    CSL_ResultHmac(&context, (u8*)calc_hmac);

    if(hmac_size > CSL_SHA1_DIGESTSIZE) {
        hmac_size = CSL_SHA1_DIGESTSIZE;
    }
    memcpy(hmac, calc_hmac, hmac_size);
}

void encrypt_data(char* dest, const char* src, size_t len, 
                  const char* iv_seed, const char* key)
{
    int rc;

    char iv[CSL_AES_IVSIZE_BYTES];
    memcpy(iv, iv_seed, sizeof(iv));

    rc = aes_SwEncrypt((u8*)key, (u8*)iv, (u8*)src, len, (u8*)dest);
    if(rc != 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to encrypt data.");
    }
}

void decrypt_data(char* dest, const char* src, size_t len, 
                  const char* iv_seed, const char* key)
{
    int rc;

    char iv[CSL_AES_IVSIZE_BYTES];
    memcpy(iv, iv_seed, sizeof(iv));

    rc = aes_SwDecrypt((u8*)key, (u8*)iv, (u8*)src, len, (u8*)dest);
    if(rc != 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to decrypt data.");
    }
}

// assumed msg is large enough for encryption padding, if called for, and security is max.
void sign_vss_msg(char* msg, const char* enc_key, const char* sign_key)
{
    // Compute signature for data and header.
    size_t data_len = vss_get_data_length(msg);

    if(data_len > 0) {
        // Pad data.
        size_t pad_len = ((CSL_AES_BLOCKSIZE_BYTES - 
                           (data_len % CSL_AES_BLOCKSIZE_BYTES)) %
                          CSL_AES_BLOCKSIZE_BYTES);
        data_len += pad_len;
        // Extend buffer for pad.
        vss_set_data_length(msg, data_len);
        
        // Add random pad bytes.
        while(pad_len > 0) {
            // Random number generator good for 16 bits of randomness.
            u32 random = VPLMath_Rand();
            msg[VSS_HEADER_SIZE + data_len - pad_len] = random & 0xff;
            pad_len--;
            if(pad_len > 0) {
                msg[VSS_HEADER_SIZE + data_len - pad_len] = (random >> 8) & 0xff;
                pad_len--;
            }
        }
        
        // Sign data
        compute_hmac(msg + VSS_HEADER_SIZE, data_len,
                     sign_key, vss_get_data_hmac(msg), VSS_HMAC_SIZE);
        
        // Encrypt data. Use temp buffer.
        char tmp[data_len];
        
        encrypt_data(tmp, msg + VSS_HEADER_SIZE, data_len, 
                     msg, enc_key);
        memcpy(msg + VSS_HEADER_SIZE, tmp, data_len);
    }
    else {
        memset(vss_get_data_hmac(msg), 0, VSS_HMAC_SIZE);
    }

    memset(vss_get_header_hmac(msg), 0, VSS_HMAC_SIZE);
    compute_hmac(msg, VSS_HEADER_SIZE,
                 sign_key, vss_get_header_hmac(msg), VSS_HMAC_SIZE);
}
