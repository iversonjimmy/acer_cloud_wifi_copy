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
/// This unit test checks the auto-expiration feature for CloudDocs and CameraRoll datasets.
/// Only the CloudDocs dataset is tested since this feature is common to all datasets with expiration.


#include "vpl_net.h"
#include "vplex_trace.h"
#include "vplex_assert.h"
#include "vplex_vs_directory.h"
#include "vplex_file.h"
#include "vpl_conv.h"

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
#include "vsTest_expire_data.hpp"

#include "vssi.h"
#include "vssi_error.h"

using namespace std;

static int test_log_level = TRACE_INFO;

/// Abort error catching
static jmp_buf vsTestErrExit;
const char* vsTest_curTestName = NULL;

static void signal_handler(int sig)
{
    UNUSED(sig);  // expected to be SIGABRT
    longjmp(vsTestErrExit, 1);
}

/// Test parameters passed by command line
std::string username = "hybrid";   /// Default username
std::string password = "password"; /// Default password

std::string lab_name = "www-c100.pc-int.igware.net";  /// VSDS server
u16 lab_port = 443;

/// Session parameters picked up on successful login
VPLUser_Id_t uid = 0;
vplex::vsDirectory::SessionInfo session;
VSSI_Session vssi_session;

u64 testDeviceId = 0;  // to be obtained from the infra at runtime

/// localization to use for all commands.
vplex::vsDirectory::Localization l10n;

// VSDS query proxy
VPLVsDirectory_ProxyHandle_t proxy;

/// Contents and datasets collected for test.
std::vector<vplex::vsDirectory::DatasetDetail> ownedDatasets;
std::vector<vplex::vsDirectory::UserStorage> userStorage;

bool http_verbose = false;

static int do_data_collection_queries(void)
{
    int rv = 0;
    int rc;
    vplex::vsDirectory::SessionInfo* req_session;
    vplex::vsDirectory::LinkDeviceInput linkReq;
    vplex::vsDirectory::LinkDeviceOutput linkResp;
    vplex::vsDirectory::ListUserStorageInput listStorReq;
    vplex::vsDirectory::ListUserStorageOutput listStorResp;
    vplex::vsDirectory::ListOwnedDataSetsInput listDatasetReq;
    vplex::vsDirectory::ListOwnedDataSetsOutput listDatasetResp;

    // Link device to the user.
    req_session = linkReq.mutable_session();
    *req_session = session;
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

    // Get user storage clusters.
    req_session = listStorReq.mutable_session();
    *req_session = session;
    listStorReq.set_userid(uid);
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

    // Remember for later use.
    for(int i = 0; i < listStorResp.storageassignments_size(); i++) {
        userStorage.push_back(listStorResp.storageassignments(i));
    }

    // Get user's datasets.
    req_session = listDatasetReq.mutable_session();
    *req_session = session;
    listDatasetReq.set_userid(uid);
    rc = VPLVsDirectory_ListOwnedDataSets(proxy, VPLTIME_FROM_SEC(30),
                                          listDatasetReq, listDatasetResp);
    if(rc != 0) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "ListOwnedDatasets query returned %d, detail:%d:%s",
                         rc, listDatasetResp.error().errorcode(),
                         listDatasetResp.error().errordetail().c_str());
        rv++;
    }

    for(int i = 0; i < listDatasetResp.datasets_size(); i++) {
        ownedDatasets.push_back(listDatasetResp.datasets(i));
    }

 exit:
    return rv;
}

static int do_cleanup_queries(void)
{
    int rv = 0;
    int rc;
    vplex::vsDirectory::SessionInfo* req_session;
    vplex::vsDirectory::UnlinkDeviceInput unlinkReq;
    vplex::vsDirectory::UnlinkDeviceOutput unlinkResp;
    std::vector<vplex::vsDirectory::DatasetDetail>::iterator dataset_it;

    // Unlink this device.
    req_session = unlinkReq.mutable_session();
    *req_session = session;
    unlinkReq.set_userid(uid);
    unlinkReq.set_deviceid(testDeviceId);
    rc = VPLVsDirectory_UnlinkDevice(proxy, VPLTIME_FROM_SEC(30),
                                     unlinkReq, unlinkResp);
    if(rc != 0) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "UnlinkDevice query returned %d, detail:%d:%s",
                         rc, unlinkResp.error().errorcode(),
                         unlinkResp.error().errordetail().c_str());
        rv++;
    }

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
    printf(" -l --lab-name NAME         Lab name  (%s)\n",
           lab_name.c_str());
    printf(" -L --lab-port PORT         Lab server port (%d)\n",
           lab_port);
    printf(" -u --username USERNAME     User name (%s)\n",
           username.c_str());
    printf(" -p --password PASSWORD     User password (%s)\n",
           password.c_str());
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
        {"lab-name", required_argument, 0, 'l'},
        {"lab-port", required_argument, 0, 'L'},
        {"http-verbose", no_argument, 0, 'H'},
        {0,0,0,0}
    };

    for(;;) {
        int option_index = 0;
        
        int c = getopt_long(argc, argv, "vtu:p:l:L:H", 
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

        case 'l': 
            lab_name = optarg; 
            break;

        case 'L': 
            lab_port = atoi(optarg);
            break;

        case 'H':
            http_verbose = true;
            break;

        default:
            usage(argc, argv);
            rv = -1;
            break;
        }
    }

    return rv;
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

static void delete_route_info(VSSI_RouteInfo& route_info)
{
    // release the route info
    if ( route_info.routes ) {
        for( int i = 0 ; i < route_info.num_routes ; i++ ) {
            if ( route_info.routes[i].server ) {
                free(route_info.routes[i].server);
                route_info.routes[i].server = NULL;
            }
        }
        route_info.num_routes = 0;
        delete [] route_info.routes;
        route_info.routes = NULL;
    }
}

static const char vsTest_main[] = "VS Expiration Test Main";
int main(int argc, char* argv[])
{
    int rv = 0; // pass until failed.
    int rc;
    std::vector<vplex::vsDirectory::TitleDetail>::iterator title_it;
    std::vector<vplex::vsDirectory::Subscription>::iterator subscription_it;
    std::vector<vplex::vsDirectory::DatasetDetail>::iterator dataset_it;
    std::vector<vplex::vsDirectory::UserStorage>::iterator storage_it;
    VSSI_RouteInfo route_info;
    u64 user_id = 0;
    u64 dataset_id = 0;

    vsTest_curTestName = vsTest_main;

    VPLTrace_SetBaseLevel(test_log_level);
    VPLTrace_SetShowTimeAbs(true);
    VPLTrace_SetShowTimeRel(false);
    VPLTrace_SetShowThread(false);

    memset(&route_info, 0, sizeof(route_info));

    if(parse_args(argc, argv) != 0) {
        goto exit;
    }

    // Print out configuration used for recordkeeping.
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "Running vsTest with the following options:");
    VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                        "     Lab: %s:%d", 
                        lab_name.c_str(), lab_port);
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
        rc = userLogin(lab_name, lab_port, 
                        username, "acer", password, uid, session);
        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                            "TC_RESULT = %s ;;; TC_NAME = User_Login",
                            rc ? "FAIL" : "PASS");
        if(rc != 0) {
            rv++;
            goto exit;
        }

        rc = registerAsDevice(lab_name, lab_port, username, password,
                              testDeviceId);
        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                            "TC_RESULT = %s ;;; TC_NAME = Register_As_Device",
                            rc ? "FAIL" : "PASS");
        if (rc != 0) {
            rv++;
            goto exit;
        }

        // Create VSDS proxy.
        rc = VPLVsDirectory_CreateProxy(lab_name.c_str(), lab_port,
                                        &proxy);
        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                            "TC_RESULT = %s ;;; TC_NAME = Create_Proxy",
                            rc ? "FAIL" : "PASS");
        if(rc != 0) {
            rv++;
            goto exit;
        }
        

        // Perform initial condition setup queries.
        // (cleanup as if failed to cleanup last time.)
        do_cleanup_queries();

        // Perform data collection and setup queries.
        do_data_collection_queries();

        // Set-up VSSI library
        rc = do_vssi_setup(testDeviceId, session, vssi_session);
        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                            "TC_RESULT = %s ;;; TC_NAME = VSSI_Setup",
                            rc ? "FAIL" : "PASS");
        if(rc != 0) {
            rv++;
            goto exit;
        }

        // Perform data expiration testing. Use Docs dataset (type:CACHE).
        for(dataset_it = ownedDatasets.begin();
            dataset_it != ownedDatasets.end();
            dataset_it++) {
            if(dataset_it->datasettype() == vplex::vsDirectory::CACHE) {
                break;
            }
        }
        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                            "TC_RESULT = %s ;;; TC_NAME = VSSI_FindDocsDataset",
                            (dataset_it == ownedDatasets.end()) ? "FAIL" : "PASS");
        if(dataset_it == ownedDatasets.end()) {
            rv++;
            goto exit;
        }

        // Find storage where dataset belongs
        for(storage_it = userStorage.begin();
            storage_it != userStorage.end();
            storage_it++) {
            if(dataset_it-> clusterid() == storage_it->storageclusterid()) {
                break;
            }
        }
        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                            "TC_RESULT = %s ;;; TC_NAME = VSSI_FindDocsDatasetStorage",
                            (storage_it == userStorage.end()) ? "FAIL" : "PASS");
        if(storage_it == userStorage.end()) {
            rv++;
            goto exit;
        }

        if((rc = setup_route_access_info(*storage_it, *dataset_it,
                                         user_id, dataset_id, route_info))) {
            VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                "setup_route_access_info() - %d", rc);
            goto exit;
        }

        rc = test_data_expiration(vssi_session, user_id, dataset_id,
                                  route_info);
        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                            "TC_RESULT = %s ;;; TC_NAME = Dataset_Expiration_"FMTu64,
                            rc ? "FAIL" : "PASS",
                            dataset_it->datasetid());
        rv += rc;

        // Perform cleanup queries.
        do_cleanup_queries();
        do_vssi_cleanup(vssi_session);
    }
    else {
        rv = 1;
    }

 exit:
    delete_route_info(route_info);
    
    VPLVsDirectory_DestroyProxy(proxy);

    return (rv) ? 1 : 0;
}
