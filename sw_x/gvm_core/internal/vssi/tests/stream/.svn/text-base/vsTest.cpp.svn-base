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

///Virtual Storage Unit Test
///
/// This unit test exercises the Virtual Storage servers and client libraries.

#include "vpl_net.h"
#include "vplex_trace.h"
#include "vplex_assert.h"
#include "vplex_vs_directory.h"
#include "vplex_file.h"
#include "vpl_conv.h"
#include "vplex_serialization.h"

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

#include "vssi.h"
#include "vssi_error.h"

using namespace std;

static int test_log_level = TRACE_INFO;
static bool http_verbose = false;

/// Abort error catching
static jmp_buf vsTestErrExit;
static string vsTest_curTestName;

static void signal_handler(int sig)
{
    UNUSED(sig);  // expected to be SIGABRT
    longjmp(vsTestErrExit, 1);
}

/// Test parameters passed by command line
string username = "hybrid";   /// Default username
string password = "password"; /// Default password

string vsds_name = "www-c100.lab1.routefree.com";  /// VSDS server
u16 vsds_port = 443;

u16 vss_http_port = 17144;

string ias_name = "www-c100.lab1.routefree.com"; /// IAS server
u16 ias_port = 443;

u64 testDeviceId = 0;  // to be obtained from the infra at runtime

/// localization to use for all commands.
vplex::vsDirectory::Localization l10n;

/// VSDS query proxy
VPLVsDirectory_ProxyHandle_t proxy;

/// Session parameters picked up on successful login
VPLUser_Id_t uid = 0;
enum {
    LOGIN_SESSION, // Generated when user logs in
    PSN_SESSION,   // Fetched from first PSN for user
    BAD_HANDLE_SESSION, // Manged user login session (handle changed)
    NUM_SESSIONS
};
vplex::vsDirectory::SessionInfo session[NUM_SESSIONS];
VSSI_Session vssi_session[NUM_SESSIONS] = {0};

/// Ways to reach the PSN
u64 psnDeviceId = 0;
string psnInternalDirectAddress;
string psnDirectAddress;
string psnProxyAddress;
string psnHttpAddress; // from metadata index dataset
u16 psnDirectSecurePort = 0;
u16 psnProxyPort = 0;
u16 psnHttpPort = 0; // from metadata index dataset

// Number of repeats for interrupted streaming test
int num_repeats = 2;

// Set of target files to download. Must be populated at PSN appropriately.
// Empty string terminates list.
string targetFiles[] =
    {"test",
     ""
    };

// Set of range header values for download tests.
pair<string, int> rangeValues[] = 
    {
        make_pair("", 200), // No range header
        make_pair("bytes=0-", 200), // whole file
        make_pair("bytes=100-199", 206), // second 100 bytes
        make_pair("bytes=199-100", 416), // backwards range, no range valid (invalid)
        make_pair("100-199", 416), // missing bytes=.  no range valid (invalid)
        make_pair("bytes=100-199,500-749", 206), // multi range request
        make_pair("bytes=100-199,bytes=500-749", 206),  // technically illegal (compatibility)
        make_pair("bytes=999999999-999999999,100-199,500-749", 206),  // first range illegal, should satisfy other ranges though
        make_pair("bytes=999999999-999999999,888888888-888888888", 416), // all ranges unsatisfiable
        make_pair("bytes=-100", 206), // last 100 bytes
        make_pair("bytes=0-9999999999", 200), // end past EOF (should work)
        make_pair("bytes=999999999-9999999999", 416), // range past EOF
        make_pair("abc", 416), //invalid request
        make_pair("bytes=abc", 416), //invalid request        
        make_pair("bytes=abc-def", 416), //invalid request        
        make_pair("bytes=-abc", 416), //invalid request        
        make_pair("bytes=abc-", 416), //invalid request        
        make_pair("kbytes=100-199", 416), //invalid request
        make_pair("bitts=0-", 416), //invalid request
        make_pair("", 0) // terminator
    };

// Routes to stream data
enum {
    DIRECT_INTERNAL,
    DIRECT,
    PROXY,
    PROXY_P2P,
    NUM_ROUTES
};

static int do_data_collection_queries(void)
{
    int rv = 0;
    int rc;
    vplex::vsDirectory::SessionInfo* req_session;
    vplex::vsDirectory::ListOwnedDataSetsInput listDatasetReq;
    vplex::vsDirectory::ListOwnedDataSetsOutput listDatasetResp;
    vplex::vsDirectory::ListUserStorageInput listStorReq;
    vplex::vsDirectory::ListUserStorageOutput listStorResp;
    vplex::vsDirectory::GetUserStorageAddressInput storAddrReq;
    vplex::vsDirectory::GetUserStorageAddressOutput storAddrResp;
    int i;

    // Get owned datasets. Need HTTP route info for dataset type CLEAR_FI
    req_session = listDatasetReq.mutable_session();
    *req_session = session[0];
    listDatasetReq.set_userid(uid);
    rc = VPLVsDirectory_ListOwnedDataSets(proxy, VPLTIME_FROM_SEC(30),
                                          listDatasetReq, listDatasetResp);
    if(rc != 0) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "ListOwnedDatasets query returned %d, detail:%d:%s",
                         rc, listDatasetResp.error().errorcode(),
                         listDatasetResp.error().errordetail().c_str());
        rv++;
        goto exit;
    }
    else {
        // Find CLEAR_FI dataset to extract HTTP route info
        for(i = 0; i < listDatasetResp.datasets_size(); i++) {
            const vplex::vsDirectory::DatasetDetail& datasetDetail = 
                listDatasetResp.datasets(i);
            if(datasetDetail.datasettype() == 
               vplex::vsDirectory::CLEAR_FI) {
                psnHttpAddress = datasetDetail.storageclusterhostname();
                psnHttpPort =  datasetDetail.storageclusterport();
                break;
            }
        }
    }

    // Get user storage clusters.
    req_session = listStorReq.mutable_session();
    *req_session = session[0];
    listStorReq.set_userid(uid);
    listStorReq.set_deviceid(testDeviceId);
    rc = VPLVsDirectory_ListUserStorage(proxy, VPLTIME_FROM_SEC(30),
                                        listStorReq, listStorResp);
    if(rc != 0) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "ListUserStorage query returned %d, detail:%d:%s",
                         rc, listStorResp.error().errorcode(),
                         listStorResp.error().errordetail().c_str());
        rv++;
        goto exit;
    }

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "User "FMTu64" has %d storage assignments.",
                        uid, listStorResp.storageassignments_size());

    // Using the first PSN storage cluster, get the PSN's address info.
    for(i = 0; i < listStorResp.storageassignments_size(); i++) {
        if(listStorResp.storageassignments(i).storagetype() != 0) {
            req_session = storAddrReq.mutable_session();
            *req_session = session[0];
            storAddrReq.set_userid(uid);
            psnDeviceId = listStorResp.storageassignments(i).storageclusterid();
            storAddrReq.set_storageclusterid(psnDeviceId);
            if (listStorResp.storageassignments(i).has_accessticket() &
                listStorResp.storageassignments(i).has_devspecaccessticket() ) {
                session[PSN_SESSION].set_sessionhandle(listStorResp.storageassignments(i).accesshandle());
                session[PSN_SESSION].set_serviceticket(listStorResp.storageassignments(i).devspecaccessticket());
            }
            else {
                VPLTRACE_LOG_ERR(TRACE_APP, 0,
                                 "PSN "FMTu64" did not provide access session info.",
                                 psnDeviceId);
                rv++;
                goto exit;
            }

            rc = VPLVsDirectory_GetUserStorageAddress(proxy, VPLTIME_FROM_SEC(30),
                                                storAddrReq, storAddrResp);
            if(rc != 0) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "GetUserStorageAddress query returned %d, detail:%d:%s",
                                 rc, storAddrResp.error().errorcode(),
                                 storAddrResp.error().errordetail().c_str());
                rv++;
                goto exit;
            }
            break;
        }
    }
    if(i == listStorResp.storageassignments_size()) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "No PSN found. Test cannot run.");
        rv++;
        goto exit;
    }

    if(storAddrResp.has_internaldirectaddress()) {
        psnInternalDirectAddress = storAddrResp.internaldirectaddress();
    }
    else {
        VPLTRACE_LOG_ERR(TRACE_APP, 0,
                         "PSN "FMTu64" did not provide internal direct address.",
                         psnDeviceId);
        rv++;
    }

    if(storAddrResp.has_directaddress()) {
        psnDirectAddress = storAddrResp.directaddress();
    }
    else {
        VPLTRACE_LOG_ERR(TRACE_APP, 0,
                         "PSN "FMTu64" did not provide direct address.",
                         psnDeviceId);
        rv++;
    }

    if(storAddrResp.has_proxyaddress()) {
        psnProxyAddress = storAddrResp.proxyaddress();
    }
    else {
        VPLTRACE_LOG_ERR(TRACE_APP, 0,
                         "PSN "FMTu64" did not provide proxy address.",
                         psnDeviceId);
        rv++;
    }

    if(storAddrResp.has_directsecureport()) {
        psnDirectSecurePort = storAddrResp.directsecureport();
    }
    else {
        VPLTRACE_LOG_ERR(TRACE_APP, 0,
                         "PSN "FMTu64" did not provide direct secure port.",
                         psnDeviceId);
        rv++;
    }

    if(storAddrResp.has_proxyport()) {
        psnProxyPort = storAddrResp.proxyport();
    }
    else {
        VPLTRACE_LOG_ERR(TRACE_APP, 0,
                         "PSN "FMTu64" did not provide direct port.",
                         psnDeviceId);
        rv++;
    }

    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Direct access: Internal(%s) External(%s) Port(%u)",
                        psnInternalDirectAddress.c_str(),
                        psnDirectAddress.c_str(),
                        psnDirectSecurePort);
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Proxy access: %s:%u",
                        psnProxyAddress.c_str(), psnProxyPort);
 exit:
    return rv;
}

static void usage(int argc, char* argv[])
{
    // Dump original command

    // Print usage message
    printf("Usage: %s [options]\n", argv[0]);
    printf("Options:\n");
    printf(" -v --verbose               Raise verbosity one level (may repeat 3 times)\n");
    printf(" -t --terse                 Lower verbosity (make terse) one level (may repeat 2 times or cancel a -v flag)\n");
    printf(" -d --vsds-name NAME        VSDS server name or IP address (%s)\n",
           vsds_name.c_str());
    printf(" -q --vsds-port PORT        VSDS server port (%d)\n",
           vsds_port);
    printf(" -i --ias-name NAME         IAS server name or IP address (%s)\n",
           vsds_name.c_str());
    printf(" -r --ias-port PORT         IAS server port (%d)\n",
           vsds_port);
    printf(" -u --username USERNAME     User name (%s)\n",
           username.c_str());
    printf(" -p --password PASSWORD     User password (%s)\n",
           password.c_str());
    printf(" -n --num-repeats COUNT     Number of repeats for interrupted streaming test (%d)\n",
           num_repeats);
    printf(" -H --http-verbose          Verbose logging of HTTP webservice queries.\n");
}

static int parse_args(int argc, char* argv[])
{
    int rv = 0;

    static struct option long_options[] = {
        {"verbose", no_argument, 0, 'v'},
        {"terse", no_argument, 0, 't'},
        {"username", required_argument, 0, 'u'},
        {"password", required_argument, 0, 'p'},
        {"vsds-name", required_argument, 0, 'd'},
        {"vsds-port", required_argument, 0, 'q'},
        {"ias-name", required_argument, 0, 'i'},
        {"ias-port", required_argument, 0, 'r'},
        {"num-repeats", required_argument, 0, 'n'},
        {"http-verbose", no_argument, 0, 'H'},
        {0,0,0,0}
    };

    for(;;) {
        int option_index = 0;

        int c = getopt_long(argc, argv, "vtu:p:d:q:i:r:n:H",
                            long_options, &option_index);

        if (c == -1)
            break;

        switch (c) {
        case 'v':
            if(test_log_level < TRACE_FINEST) {
                test_log_level++;
            }
            break;
        case 't':
            if(test_log_level > TRACE_ERROR) {
                test_log_level--;
            }
            break;
        case 'u':
            username = optarg;
            break;

        case 'p':
            password = optarg;
            break;

        case 'd':
            vsds_name = optarg;
            break;

        case 'q':
            vsds_port = atoi(optarg);
            break;

        case 'i':
            ias_name = optarg;
            break;

        case 'r':
            ias_port = atoi(optarg);
            break;

        case 'H':
            http_verbose = true;
            break;

        case 'n':
            num_repeats = atoi(optarg);
            break;

        default:
            usage(argc, argv);
            rv++;
            break;
        }
    }

    return rv;
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

static int receive_stream(VSSI_SecureTunnel tunnel_handle,
                          u64& received,
                          int expectResponse,
                          bool end_early = false)
{
    int rv = 0;
    size_t buflen = 4096;
    char buf[buflen];
    size_t beginline = 0;
    size_t linelen = 0;
    int rc = 0;
    int i;

    string line;
    string response;
    int respcode = 0;
    vector<string> headers;
    vector<string>::iterator it;
    bool done_headers = false; // true when last header received.
    bool processed_headers = false;
    u64 bodylen = 0;

    test_context_t test_ctx;
    VPL_SET_UNINITIALIZED(&(test_ctx.sem));
    if(VPLSem_Init(&(test_ctx.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        rv++;
        goto exit;
    }

    received = 0;

    // Receive data as available.
    do {
        VSSI_SecureTunnelWaitToReceive(tunnel_handle,
                                       &test_ctx, test_callback);
        VPLSem_Wait(&(test_ctx.sem));
        rc = test_ctx.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Unexpected tunnel error %d.",
                             rc);
            rv++;
            goto exit;
        }

        rc = VSSI_SecureTunnelReceive(tunnel_handle, buf, 
                                      (bodylen == 0) ? buflen :
                                      (bodylen - received > buflen) ? buflen :
                                      (bodylen - received));
        if(rc == VSSI_AGAIN) {
            // Nothing there now.
            VPLTRACE_LOG_WARN(TRACE_APP, 0,
                              "Unexpected VSSI_AGAIN response after waiting for data.");
            continue;
        }
        if(rc < 0) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Unexpected tunnel error %d.",
                             rc);
            rv++;
            goto exit;
        }

        // Receive response header until done
        beginline = 0;
        while(!done_headers && beginline < rc) {
            linelen = 0;
            for(i = beginline; i < rc; i++) {
                linelen++;
                if(buf[i] == '\n') {
                    break;
                }
            }

            // Add line fragment to partial line.
            line.append(buf + beginline, linelen);
            beginline += linelen;

            if(line[line.size() - 1] != '\n') {
                // Not a complete line. Get more data.
                break;
            }

            // Strip newline characters from the end.
            line.erase(line.find_first_of("\r\n"));

            if(response.empty()) {
                // Collect response line
                VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                    "Response: [%s]", line.c_str());
                response = line;
                line.erase();
            }
            else {
                if(line.empty()) {
                    // Blank line after headers indicates end of headers.
                    done_headers = true;
                    break;
                }
                else {
                    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                        "Header: [%s]", line.c_str());
                    headers.push_back(line);
                    line.erase();
                }
            }
        }

        if(done_headers && !processed_headers) {
            // What did the response header say?
            linelen = response.find_first_of(' ');
            if (linelen == string::npos) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Unexpected pattern in response \"%s\"", response.c_str());
                rv++;
                goto exit;
            }
            respcode = atoi(&(response[linelen + 1]));
            if(respcode != expectResponse) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "Response had code %d. Expected %d.",
                                 respcode, expectResponse);
                rv++;
                goto exit;
            }

            // Get the body length from the headers.
            for(it = headers.begin(); it != headers.end(); it++) {
                if(it->find("Content-Length:") != string::npos) {
                    bodylen = strtoull(&((*it)[it->find_first_not_of(' ', it->find_first_of(':') + 1)]), 0, 10);

                    if(end_early) {
                        // Only receive 10k or half the file, whichever is less.
                        if((bodylen / 2) < 10240) {
                            bodylen /= 2;
                        }
                        else {
                            bodylen = 10240;
                        }
                    }
                }
            }
            processed_headers = true;
        }

        if(processed_headers) {
            // Collect body data.
            received += (rc - beginline);
        }

    } while(!processed_headers || received < bodylen);

 exit:
    return rv;
}

#ifdef NOTDEF
static int receive_http_stream(VPLSocket_t socket,
                               int expected,
                               u64& received)
{
    int rv = 0;
    size_t buflen = 4096;
    char buf[buflen];
    size_t beginline = 0;
    size_t linelen = 0;
    int rc = 0;
    int i;

    string line;
    string response;
    int respcode = 0;
    vector<string> headers;
    vector<string>::iterator it;
    bool done_headers = false; // true when last header received.
    bool processed_headers = false;
    u64 bodylen = 0;

    received = 0;

    // Receive data as available.
    do {
        // Wait for ready to receive.
        VPLSocket_poll_t psock;
        psock.socket = socket;
        psock.events = VPLSOCKET_POLL_RDNORM;
        rc = VPLSocket_Poll(&psock, 1, VPLTIME_FROM_SEC(30000));
        if(rc < 0) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Encountered error %d waiting for socket ready to recv.",
                             rc);
            rv++;
            goto exit;
        }
        if(rc != 1) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Receive timed out.");
            rv++;
            goto exit;
        }
        if(psock.revents != VPLSOCKET_POLL_RDNORM) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Encountered unexpected event %x waiting for socket ready to recv.",
                             psock.events);
            rv++;
            goto exit;
        }

        rc = VPLSocket_Recv(socket, buf, buflen);
        if(rc == VPL_ERR_AGAIN || rc == VPL_ERR_INTR) {
            
            // trying again.
            rc = 0;
            continue;
        }
        if(rc <= 0) {
            // Socket closed or had some other error.
            if(rc == 0) {
                VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                                  "Got socket closed for connection "FMT_VPLSocket_t".",
                                  VAL_VPLSocket_t(socket));
            }
            else {
                VPLTRACE_LOG_FINE(TRACE_BVS, 0,
                                  "Got socket error %d for connection "FMT_VPLSocket_t".",
                                  rc, VAL_VPLSocket_t(socket));
            }
            rv++;
            goto exit;
        }

        beginline = 0;
        VPLTRACE_LOG_FINEST(TRACE_BVS, 0,
                            "Received %d bytes.", rc);
        while(!done_headers && beginline < rc) {
            linelen = 0;
            for(i = beginline; i < rc; i++) {
                linelen++;
                if(buf[i] == '\n') {
                    break;
                }
            }
            
            // Add line fragment to partial line.
            line.append(buf + beginline, linelen);
            beginline += linelen;
            
            if(line[line.size() - 1] != '\n') {
                // Not a complete line. Get more data.
                break;
            }
            
            // Strip newline characters from the end.
            line.erase(line.find_first_of("\r\n"));
            
            if(response.empty()) {
                // Collect response line
                VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                    "Response: [%s]", line.c_str());
                response = line;
                line.erase();
            }
            else {
                if(line.empty()) {
                    // Blank line after headers indicates end of headers.
                    done_headers = true;
                    break;
                }
                else {
                    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                        "Header: [%s]", line.c_str());
                    headers.push_back(line);
                    line.erase();
                }
            }
        }
        
        if(done_headers && !processed_headers) {
            // What did the response header say?
            linelen = response.find_first_of(' ');
            if (linelen == string::npos) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Unexpected pattern in response \"%s\"", response.c_str());
                rv++;
                goto exit;
            }
            respcode = atoi(&(response[linelen + 1]));
            if(respcode != expected) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "Response had code %d. Expected %d.",
                                 respcode, expected);
                rv++;
                goto exit;
            }
            
            // Get the body length from the headers.
            for(it = headers.begin(); it != headers.end(); it++) {
                if(it->find("Content-Length:") != string::npos) {
                    bodylen = strtoull(&((*it)[it->find_first_not_of(' ', it->find_first_of(':') + 1)]), 0, 10);
                }
            }
            processed_headers = true;
        }
        
        if(processed_headers) {
            // Collect body data.
            received += (rc - beginline);
        }
        
    } while(!processed_headers || received < bodylen);

 exit:
    return rv;
}
#endif // NOTDEF

static int test_streaming_download(int sessionIdx,
                                   int routeType,
                                   int fileIdx,
                                   int rangeIdx)
{
    int rv = 0;
    int rc;
    string case_name = "Stream_";
    string address;
    u16 port = 0;
    VSSI_SecureTunnelConnectType connectionType = VSSI_SECURE_TUNNEL_INVALID;
    VSSI_SecureTunnel tunnel_handle = NULL;
    string streamFileQuery;
    size_t querySent = 0;
    VPLTime_t start, end;
    u64 received;
    test_context_t test_ctx;
    bool p2p_path_fail = false;
    bool direct_external_path_fail = false;

    VPL_SET_UNINITIALIZED(&(test_ctx.sem));

    switch(sessionIdx) {
    case LOGIN_SESSION:
        case_name += "UserSession_";
        break;
    case PSN_SESSION:
        case_name += "PSNSession_";
        break;
    case BAD_HANDLE_SESSION:
        case_name += "BadSession_";
        break;
    default:
    case NUM_SESSIONS:
        break;
    }

    switch(routeType) {
    case DIRECT_INTERNAL:
        case_name += "DirectInternal_";
        address = psnInternalDirectAddress;
        port = psnDirectSecurePort;
        connectionType = VSSI_SECURE_TUNNEL_DIRECT_INTERNAL;
        break;
    case DIRECT:
        case_name += "Direct_";
        address = psnDirectAddress;
        port = psnDirectSecurePort;
        connectionType = VSSI_SECURE_TUNNEL_DIRECT;
        break;
    case PROXY:
        case_name += "Proxy_";
        address = psnProxyAddress;
        port = psnProxyPort;
        connectionType = VSSI_SECURE_TUNNEL_PROXY;
        break;
    case PROXY_P2P:
        case_name += "Proxy_P2P_";
        address = psnProxyAddress;
        port = psnProxyPort;
        connectionType = VSSI_SECURE_TUNNEL_PROXY_P2P;
        break;
    case NUM_ROUTES:
    default:
        break;
    }

    case_name += targetFiles[fileIdx];
    if(!rangeValues[rangeIdx].first.empty()) {
        case_name += ":";
        case_name += rangeValues[rangeIdx].first;
    }
    vsTest_curTestName = case_name;
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Starting test: %s.",
                        vsTest_curTestName.c_str());

    if(VPLSem_Init(&(test_ctx.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        rv++;
        goto exit;
    }

    // Try connection with wrong device ID. Should fail.
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Testing authentication...");
    VSSI_SecureTunnelConnect(vssi_session[sessionIdx],
                             address.c_str(), port,
                             connectionType,
                             psnDeviceId ^ 0xff,
                             &tunnel_handle,
                             &test_ctx, test_callback);
    VPLSem_Wait(&(test_ctx.sem));
    rc = test_ctx.rv;
    if(rc == VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Secure tunnel connect succeeded for bad device ID. Must fail.");
        rv++;
    }
    else {
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                            "Authentication check passed.");
    }
    // Set-up connection
    VSSI_SecureTunnelConnect(vssi_session[sessionIdx],
                             address.c_str(), port,
                             connectionType, psnDeviceId,
                             &tunnel_handle,
                             &test_ctx, test_callback);
    VPLSem_Wait(&(test_ctx.sem));
    rc = test_ctx.rv;
    if(rc != VSSI_SUCCESS) {
        // Failure is expected for the bad session.
        if(sessionIdx != BAD_HANDLE_SESSION) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Secure tunnel connect failed with code %d.",
                             rc);
            if(routeType == DIRECT) {
                direct_external_path_fail = true;
                goto exit;
            }
            rv++;
        }
        goto exit;
    }
    else {
        // Failure is expected for the bad session.
        if(sessionIdx == BAD_HANDLE_SESSION) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Secure tunnel connect succeeded for bad session Must fail.");
            rv++;
            goto exit;
        }
        else {
            VPLTRACE_LOG_INFO(TRACE_APP, 0,
                              "Secure tunnel connection made.");
        }
    }

    // Check if tunnel is direct.
    if(VSSI_SecureTunnelIsDirect(tunnel_handle)) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Made direct connection for %s route.",
                          case_name.c_str());
        if(routeType == PROXY) {
            rv++;
            goto exit;
        }
    }
    else {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Made proxy connection for %s route.",
                         case_name.c_str());
        switch(routeType) {
        case DIRECT_INTERNAL:
        case DIRECT:
            rv++;
            goto exit;
            break;
        case PROXY:
            break;
        case PROXY_P2P:
            p2p_path_fail = true;
            break;
        }
    }

    // Send download query
    streamFileQuery =
        "GET /test/" + targetFiles[fileIdx] + " HTTP/1.1\r\n"
        "Host: " + address  + "\r\n";
    if(!rangeValues[rangeIdx].first.empty()) {
        streamFileQuery +=
            "Range: " + rangeValues[rangeIdx].first + "\r\n";
    }
    streamFileQuery += "\r\n";

    do {
        VSSI_SecureTunnelWaitToSend(tunnel_handle,
                                    &test_ctx, test_callback);
        VPLSem_Wait(&(test_ctx.sem));
        rc = test_ctx.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Unexpected tunnel error %d.",
                             rc);
            rv++;
            goto exit;
        }

        rc += VSSI_SecureTunnelSend(tunnel_handle,
                                    streamFileQuery.data() + querySent,
                                    streamFileQuery.size() - querySent);
        if(rc < 0) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Unexpected tunnel error %d.",
                             rc);
            rv++;
            goto exit;
        }
        querySent += rc;
    } while(querySent < streamFileQuery.size());

    // Receive response data
    start = VPLTime_GetTimeStamp();
    rc = receive_stream(tunnel_handle, received, rangeValues[rangeIdx].second);
    end = VPLTime_GetTimeStamp();
    if(rc == 0) {
        u64 mbps = (received * 8)/ (end - start);
        u64 kbps = (received * 8192)/ (end - start);
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                            "Took "FMT_VPLTime_t"us to download "FMTu64" bytes ("FMTu64"Mb/s or "FMTu64"kb/s).",
                            end - start, received, mbps, kbps);
    }
    else {
        rv = rc;
    }

 exit:
    if(tunnel_handle) {
        VSSI_SecureTunnelDisconnect(tunnel_handle);
    }

    // P2P connections intermittently fail.
    // Direct external routes broken by network topology.
    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                        "TC_RESULT = %s ;;; TC_NAME = %s",
                        rv ? "FAIL" :
                        (p2p_path_fail || direct_external_path_fail) ? "EXPECTED_TO_FAIL" : "PASS",
                        case_name.c_str());

    return rv;
}

static int test_streaming_partial_download(int sessionIdx,
                                           int routeType,
                                           int fileIdx,
                                           int rangeIdx)
{
    int rv = 0;
    int rc;
    string case_name = "Interrupt_Stream_";
    string address;
    u16 port = 0;
    VSSI_SecureTunnelConnectType connectionType = VSSI_SECURE_TUNNEL_INVALID;
    VSSI_SecureTunnel tunnel_handle = NULL;
    string streamFileQuery;
    VPLTime_t start, end;
    u64 received;
    test_context_t test_ctx;
    bool p2p_path_fail = false;
    bool direct_external_path_fail = false;

    VPL_SET_UNINITIALIZED(&(test_ctx.sem));

    if(sessionIdx == BAD_HANDLE_SESSION) {
        // Skip pointless tests.
        return 0;
    }

    switch(sessionIdx) {
    case LOGIN_SESSION:
        case_name += "UserSession_";
        break;
    case PSN_SESSION:
        case_name += "PSNSession_";
        break;
    case BAD_HANDLE_SESSION:
        case_name += "BadSession_";
        break;
    default:
    case NUM_SESSIONS:
        break;
    }

    switch(routeType) {
    case DIRECT_INTERNAL:
        case_name += "DirectInternal_";
        address = psnInternalDirectAddress;
        port = psnDirectSecurePort;
        connectionType = VSSI_SECURE_TUNNEL_DIRECT_INTERNAL;
        break;
    case DIRECT:
        case_name += "Direct_";
        address = psnDirectAddress;
        port = psnDirectSecurePort;
        connectionType = VSSI_SECURE_TUNNEL_DIRECT;
        break;
    case PROXY:
        case_name += "Proxy_";
        address = psnProxyAddress;
        port = psnProxyPort;
        connectionType = VSSI_SECURE_TUNNEL_PROXY;
        break;
    case PROXY_P2P:
        case_name += "Proxy_P2P_";
        address = psnProxyAddress;
        port = psnProxyPort;
        connectionType = VSSI_SECURE_TUNNEL_PROXY_P2P;
        break;
    case NUM_ROUTES:
    default:
        break;
    }

    case_name += targetFiles[fileIdx];
    if(!rangeValues[rangeIdx].first.empty()) {
        case_name += ":";
        case_name += rangeValues[rangeIdx].first;
    }
    vsTest_curTestName = case_name;
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Starting test: %s.",
                        vsTest_curTestName.c_str());

    if(VPLSem_Init(&(test_ctx.sem), 1, 0) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to create semaphore.");
        rv++;
        goto exit;
    }

    // Set-up connection
    VSSI_SecureTunnelConnect(vssi_session[sessionIdx],
                             address.c_str(), port,
                             connectionType, psnDeviceId,
                             &tunnel_handle,
                             &test_ctx, test_callback);
    VPLSem_Wait(&(test_ctx.sem));
    rc = test_ctx.rv;
    if(rc != VSSI_SUCCESS) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "Secure tunnel connect failed with code %d.",
                         rc);
        if(routeType == DIRECT) {
            direct_external_path_fail = true;
            goto exit;
        }
        rv++;
        goto exit;
    }
    else {
        VPLTRACE_LOG_INFO(TRACE_APP, 0,
                          "Secure tunnel connection made.");
    }
    
    // Check if tunnel is direct.
    if(VSSI_SecureTunnelIsDirect(tunnel_handle)) {
        VPLTRACE_LOG_INFO(TRACE_BVS, 0,
                          "Made direct connection for %s route.",
                          case_name.c_str());
        if(routeType == PROXY) {
            rv++;
            goto exit;
        }
    }
    else {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Made proxy connection for %s route.",
                         case_name.c_str());
        switch(routeType) {
        case DIRECT_INTERNAL:
        case DIRECT:
            rv++;
            goto exit;
            break;
        case PROXY:
            break;
        case PROXY_P2P:
            p2p_path_fail = true;
            break;
        }
    }
    
    // Send download query
    streamFileQuery =
        "GET /test/" + targetFiles[fileIdx] + " HTTP/1.1\r\n"
        "Host: " + address  + "\r\n";
    if(!rangeValues[rangeIdx].first.empty()) {
        streamFileQuery +=
            "Range: " + rangeValues[rangeIdx].first + "\r\n";
    }
    streamFileQuery += "\r\n";

    // Make and break tunnel streaming num_repeasts times.
    for(int i = 0; i < num_repeats; i++) {
        size_t querySent = 0;
        VPLTRACE_LOG_INFO(TRACE_APP, 0,
                          "Secure tunnel interrupted %d/%d",
                          i + 1, num_repeats);
        
        do {
            VSSI_SecureTunnelWaitToSend(tunnel_handle,
                                        &test_ctx, test_callback);
            VPLSem_Wait(&(test_ctx.sem));
            rc = test_ctx.rv;
            if(rc != VSSI_SUCCESS) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "Unexpected tunnel error %d.",
                                 rc);
                rv++;
                goto exit;
            }
            
            rc += VSSI_SecureTunnelSend(tunnel_handle,
                                        streamFileQuery.data() + querySent,
                                        streamFileQuery.size() - querySent);
            if(rc < 0) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "Unexpected tunnel error %d.",
                                 rc);
                rv++;
                goto exit;
            }
            querySent += rc;
        } while(querySent < streamFileQuery.size());
        
        // Receive response data
        start = VPLTime_GetTimeStamp();
        rc = receive_stream(tunnel_handle, received, rangeValues[rangeIdx].second, true);
        end = VPLTime_GetTimeStamp();
        if(rc == 0) {
            u64 mbps = (received * 8)/ (end - start);
            u64 kbps = (received * 8192)/ (end - start);
            VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                                "Took "FMT_VPLTime_t"us to download "FMTu64" bytes ("FMTu64"Mb/s or "FMTu64"kb/s).",
                                end - start, received, mbps, kbps);
        }
        else {
            rv = rc;
        }

        // Reset the secure tunnel.
        VSSI_SecureTunnelReset(tunnel_handle,
                               &test_ctx, test_callback);
        VPLSem_Wait(&(test_ctx.sem));
        rc = test_ctx.rv;
        if(rc != VSSI_SUCCESS) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Tunnel reset error:%d.",
                             rc);
            rv++;
            goto exit;
        }
    }

 exit:
    if(tunnel_handle) {
        VSSI_SecureTunnelDisconnect(tunnel_handle);
    }

    // P2P connections intermittently fail.
    // Direct external routes broken by network topology.
    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                        "TC_RESULT = %s ;;; TC_NAME = %s",
                        rv ? "FAIL" :
                        (p2p_path_fail || direct_external_path_fail) ? "EXPECTED_TO_FAIL" : "PASS",
                        case_name.c_str());

    return rv;
}

#ifdef NOTDEF
static int test_http_streaming_download(int sessionIdx,
                                        int fileIdx,
                                        int rangeIdx)
{
    int rv = 0;
    int rc;
    string case_name = "HTTP_Stream_";
    VPLSocket_t socket;
    VPLSocket_addr_t address;
    stringstream qStream;
    string streamFileQuery;
    size_t querySent = 0;
    VPLTime_t start, end;
    u64 received;

    switch(sessionIdx) {
    case LOGIN_SESSION:
        case_name += "UserSession_";
        break;
    case PSN_SESSION:
        case_name += "PSNSession_";
        break;
    case BAD_HANDLE_SESSION:
        case_name += "BadSession_";
        break;
    default:
    case NUM_SESSIONS:
        break;
    }

    case_name += targetFiles[fileIdx];
    vsTest_curTestName = case_name;
    if(!rangeValues[rangeIdx].first.empty()) {
        case_name += ":";
        case_name += rangeValues[rangeIdx].first;
    }
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Starting test: %s.",
                        vsTest_curTestName.c_str());

    // Set-up connection
    address.addr = VPLNet_GetAddr(psnHttpAddress.c_str());
    address.port = VPLNet_port_hton(psnHttpPort);
    socket = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_STREAM, false);
    if(VPLSocket_Equal(socket, VPLSOCKET_INVALID)) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to open socket to a server.");
        rv++;
        goto exit;
    }
    if(VPLSocket_Connect(socket, &address, sizeof(address)) != VPL_OK) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "Failed to connect to server "FMT_VPLNet_addr_t":%u on socket "FMT_VPLSocket_t".",
                         VAL_VPLNet_addr_t(address.addr), VPLNet_port_ntoh(address.port),
                         VAL_VPLSocket_t(socket));
        VPLSocket_Close(socket);
        socket = VPLSOCKET_INVALID;
        rv++;
        goto exit;
    }

    // Send download query
    {
        qStream << "GET MEDIA/" << hex << uppercase << psnDeviceId << "/test/"
                << targetFiles[fileIdx] << " HTTP/1.1\r\n"
                << "Host: " << psnHttpAddress  << "\r\n";
        if(!rangeValues[rangeIdx].first.empty()) {
            qStream << "Range: " << rangeValues[rangeIdx].first << "\r\n";
        }
        qStream << "x-session-handle: "<< hex << uppercase << session[sessionIdx].sessionhandle() << "\r\n"
                << "x-service-ticket: ";    
        size_t encoded_len = VPL_BASE64_ENCODED_SINGLE_LINE_BUF_LEN(session[sessionIdx].serviceticket().length());
        char encoded[encoded_len];
        VPL_EncodeBase64(session[sessionIdx].serviceticket().c_str(), session[sessionIdx].serviceticket().length(),
                         encoded, &encoded_len, false, false);
        streamFileQuery = qStream.str();
        streamFileQuery.append(encoded, encoded_len);
        streamFileQuery += "\r\n\r\n";
    }

    do {
        rc = VPLSocket_Send(socket, streamFileQuery.data() + querySent,
                            streamFileQuery.size() - querySent);
        if(rc > 0) {
            querySent += rc;
        }
        else if(rc == VPL_ERR_AGAIN) {
            // Wait up to 5 minutes for ready to send, then give up.
            VPLSocket_poll_t psock;
            psock.socket = socket;
            psock.events = VPLSOCKET_POLL_OUT;
            rc = VPLSocket_Poll(&psock, 1, VPLTIME_FROM_SEC(300));
            if(rc < 0) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "Encountered error %d waiting for socket ready to send.",
                                 rc);
                break;
            }
            if(psock.revents != VPLSOCKET_POLL_OUT) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "Encountered unexpected event %x waiting for socket ready to send.",
                                 psock.revents);
                break;
            }
        }
        else {
            break;
        }
    } while(querySent < streamFileQuery.size());

    // Receive response data
    start = VPLTime_GetTimeStamp();
    rc = receive_http_stream(socket, 
                             (sessionIdx == BAD_HANDLE_SESSION) ? 400 : 
                             rangeValues[rangeIdx].second,
                             received);
    end = VPLTime_GetTimeStamp();
    if(rc == 0) {
        u64 mbps = (received * 8)/ (end - start);
        u64 kbps = (received * 8192)/ (end - start);
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                            "Took "FMT_VPLTime_t"us to download "FMTu64" bytes ("FMTu64"Mb/s or "FMTu64"kb/s).",
                            end - start, received, mbps, kbps);
    }
    else {
        rv = rc;
    }

 exit:
    if(!VPLSocket_Equal(socket, VPLSOCKET_INVALID)) {
        VPLSocket_Close(socket);
    }

    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                        "TC_RESULT = %s ;;; TC_NAME = %s",
                        rv ? "FAIL" : "PASS",
                        case_name.c_str());
    return rv;
}
#endif // NOTDEF

static const string vsTest_main = "VS Test Main";
int main(int argc, char* argv[])
{
    int rv = 0; // pass until failed.
    int rc;

    vsTest_curTestName = vsTest_main;

    VPLTrace_SetBaseLevel(test_log_level);
    VPLTrace_SetShowTimeAbs(true);
    VPLTrace_SetShowTimeRel(false);
    VPLTrace_SetShowThread(false);

    if(parse_args(argc, argv) != 0) {
        goto exit;
    }

    // Print out configuration used for recordkeeping.
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Running vsTest with the following options:");
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "     IAS Server: %s:%d",
                        ias_name.c_str(), ias_port);
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "     VSDS Server: %s:%d",
                        vsds_name.c_str(), vsds_port);
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "     User: %s", username.c_str());
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "     Password: %s", password.c_str());

    // Localization for all VS queries.
    l10n.set_language("en");
    l10n.set_country("US");
    l10n.set_region("USA");

    // catch SIGABRT
    ASSERT((signal(SIGABRT, signal_handler) == 0));

    if (setjmp(vsTestErrExit) == 0) { // initial invocation of setjmp
        // Run the tests. Any abort signal will hit the else block.

        // Login the user. Required for all further operations.
        vsTest_curTestName = "VSDS Login Tests";
        vsTest_infra_init();
        rc = userLogin(ias_name, ias_port,
                        username, "acer", password, uid, session[0]);
        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                            "TC_RESULT = %s ;;; TC_NAME = User_Login",
                            rc ? "FAIL" : "PASS");
        if(rc != 0) {
            rv++;
            goto exit;
        }

        rc = registerAsDevice(ias_name, ias_port, username, password,
                              testDeviceId);
        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                            "TC_RESULT = %s ;;; TC_NAME = Register_As_Device",
                            rc ? "FAIL" : "PASS");
        if (rc != 0) {
            rv++;
            goto exit;
        }

        // Create VSDS proxy.
        rc = VPLVsDirectory_CreateProxy(vsds_name.c_str(), vsds_port,
                                        &proxy);
        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                            "TC_RESULT = %s ;;; TC_NAME = Create_Proxy",
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
            *req_session = session[0];
            linkReq.set_userid(uid);
            linkReq.set_deviceid(testDeviceId);
            linkReq.set_hascamera(false);
            linkReq.set_isacer(true);
            linkReq.set_deviceclass("AndroidPhone");
            linkReq.set_devicename("vstest device");
            rc = VPLVsDirectory_LinkDevice(proxy, VPLTIME_FROM_SEC(30),
                                           linkReq, linkResp);
            if(rc != 0) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "LinkDevice query returned %d, detail:%d:%s",
                                 rc, linkResp.error().errorcode(),
                                 linkResp.error().errordetail().c_str());
                rv++;
                goto exit;
            }

            // Give some time for the device to link.
            // May want to poll VSDS before continuing on?
            sleep(5);
        }

        // Perform data collection and setup queries.
        rc = do_data_collection_queries();
        if(rc != 0) {
            rv++;
            goto exit;
        }

        // Set-up VSSI library
        rc = do_vssi_setup(testDeviceId, session[0], vssi_session[0]);
        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                            "TC_RESULT = %s ;;; TC_NAME = VSSI_Setup",
                            rc ? "FAIL" : "PASS");
        if(rc != 0) {
            rv++;
            goto fail;
        }

        // Set-up the PSN and bogus sessions.
        vssi_session[PSN_SESSION] =
            VSSI_RegisterSession(session[PSN_SESSION].sessionhandle(),
                                 session[PSN_SESSION].serviceticket().c_str());
        session[BAD_HANDLE_SESSION] = session[LOGIN_SESSION];
        session[BAD_HANDLE_SESSION].set_sessionhandle(session[LOGIN_SESSION].sessionhandle() + 1);
        vssi_session[BAD_HANDLE_SESSION] =
            VSSI_RegisterSession(session[BAD_HANDLE_SESSION].sessionhandle(),
                                 session[BAD_HANDLE_SESSION].serviceticket().c_str());

        // Perform streaming test with each parameter combination:
        // * Session: Login session, PSN session, bogus sessions
        // * Route: Internal, External, Proxy
        // * Security: Secure (Skip insecure)
        // * Download target: "test"
        // * Range value
        for(int ses = PSN_SESSION; ses < NUM_SESSIONS; ses++) {
            int tgt;
            for(int rte = 0; rte < NUM_ROUTES; rte++) {
                tgt = 0;                
                while(!targetFiles[tgt].empty()) {                    
                    int rng = 0;
                    while(rangeValues[rng].second != 0) {
                        rv += test_streaming_download(ses, rte, tgt, rng);
                        rv += test_streaming_partial_download(ses, rte, tgt, rng);
                        rng++;
                    }
                    tgt++;
                }
            }
            
#ifdef NOTDEF
            // Test HTTP streaming access
            tgt = 0;
            while(!targetFiles[tgt].empty()) {
                int rng = 0;
                while(rangeValues[rng].second != 0) {
                    rv += test_http_streaming_download(ses, tgt, rng);
                    rng++;
                }
                tgt++;
            }       
#endif // NOTDEF
        }

    fail:
        //cleanup:
        // Perform cleanup queries.
        do_vssi_cleanup(vssi_session[0]);
    }
    else {
        rv = 1;
    }

 exit:
    vsTest_infra_destroy();
    VPLVsDirectory_DestroyProxy(proxy);

    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                        "TC_RESULT = %s ;;; TC_NAME = StreamingMedia",
                        rv ? "FAIL" : "PASS");

    return (rv) ? 1 : 0;
}
