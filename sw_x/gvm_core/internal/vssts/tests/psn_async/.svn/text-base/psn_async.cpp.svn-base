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

#include <iostream>
#include <iomanip>
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


#include "vsTest_vscs_async.hpp"
#include "vsTest_infra.hpp"

#include "vssts.hpp"
#include "vssts_error.hpp"

using namespace std;

static int test_log_level = TRACE_INFO;

/// Read/write size for VSCS
static const int BLOCKSIZE = (32*1024);

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

std::string vsds_name = "www-c100.pc-int.igware.net";  /// VSDS server
u16 vsds_port = 443;

u16 vss_http_port = 17144;

std::string ias_name = "www-c100.pc-int.igware.net";
u16 ias_port = 443;

static u16 tsext_port = 0;

static int num_dirs = 1;
static int num_files = 1;
static int num_levels = 1;
static int file_size = 0;
static bool do_create = false;
static std::string root_dir = "/";
static std::string clnt_instance;
static u64 cluster_id = 0;

/// Session parameters picked up on successful login
VPLUser_Id_t uid = 0;
vplex::vsDirectory::SessionInfo session;

u64 testDeviceId = 0;  // to be obtained from the infra at runtime

/// localization to use for all commands.
vplex::vsDirectory::Localization l10n;

// VSDS query proxy
VPLVsDirectory_ProxyHandle_t proxy;

/// Contents and datasets collected for test.
std::vector<vplex::vsDirectory::Subscription> subscribedDatasets;
std::vector<vplex::vsDirectory::DatasetDetail> ownedDatasets;
std::vector<vplex::vsDirectory::UserStorage> userStorage;
bool found_VirtDrive = false;

/// Skip various parts of the test on-demand
bool vsds_only = false;
bool skip_direct = false;
bool skip_proxy = false;
bool skip_vssi = false;
bool one_only = false;
bool open_only = false;
bool run_case_insensitivity_test = false;

static int confirm_datasets(const vplex::vsDirectory::ListOwnedDataSetsOutput& listDatasetResp,
                            const vplex::vsDirectory::ListUserStorageOutput& listStorResp)
{
    // Datacenter storage (cluster ID 100) needs 5 datasets:
    // One each type: USER, CLEAR_FI, CR_UP, CR_DOWN, PIM_CONTACTS, PIM_EVENTS, PIM_TASKS, PIM_NOTES, and PIM_FAVORITES.
    // PSN storage (other clusters) need 3 datasets, all USER type.
    // Match dataset's storageClusterName against user storage to tell cluster
    // type.

    int rv = 0;

    for(int storage = 0; 
        storage < listStorResp.storageassignments_size();
        storage++) {
        const vplex::vsDirectory::UserStorage& storDetail = 
            listStorResp.storageassignments(storage);
        
        if(storDetail.storagetype() == 0) {
            continue;
        }
        if ( cluster_id && (cluster_id != storDetail.storageclusterid()) ) {
            VPLTRACE_LOG_INFO(TRACE_APP, 0,
                "cluster "FMTu64":"FMTu64" skipped.",
               cluster_id, storDetail.storageclusterid());
            continue;
        }

        // PSN storage
        for(int i = 0; i < listDatasetResp.datasets_size(); i++) {
            const vplex::vsDirectory::DatasetDetail& datasetDetail = 
                listDatasetResp.datasets(i);
                
            if(datasetDetail.clusterid() != storDetail.storageclusterid()) {
                continue;
            }

            VPLTRACE_LOG_INFO(TRACE_APP, 0,
                "Dataset "FMTu64" with type %d found in PSN storage.",
                datasetDetail.datasetid(), datasetDetail.datasettype());

            switch(datasetDetail.datasettype()) {
            case vplex::vsDirectory::VIRT_DRIVE:
                found_VirtDrive = true;
                ownedDatasets.push_back(listDatasetResp.datasets(i));
                break;
            case vplex::vsDirectory::USER:
            case vplex::vsDirectory::MEDIA:
            case vplex::vsDirectory::FS:
            case vplex::vsDirectory::CLEARFI_MEDIA:
                break;

            case vplex::vsDirectory::CAMERA:
            case vplex::vsDirectory::CLEAR_FI:
            case vplex::vsDirectory::CR_UP:
            case vplex::vsDirectory::CR_DOWN:
            case vplex::vsDirectory::PIM:
            case vplex::vsDirectory::CACHE:
            case vplex::vsDirectory::PIM_CONTACTS:
            case vplex::vsDirectory::PIM_EVENTS:
            case vplex::vsDirectory::PIM_NOTES:
            case vplex::vsDirectory::PIM_TASKS:
            case vplex::vsDirectory::PIM_FAVORITES:
            case vplex::vsDirectory::MEDIA_METADATA:
            case vplex::vsDirectory::USER_CONTENT_METADATA:
            default:
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                                 "PSN storage has unexpected dataset of type %d.",
                                 datasetDetail.datasettype());
                rv++;
                break;
            }
        }
    }
    if(!found_VirtDrive) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                         "PSN storage did not find VirtDrive dataset.");
        rv++;
    }

    return rv;
}

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

    sleep(5);

    // Get user storage clusters.
    req_session = listStorReq.mutable_session();
    *req_session = session;
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

    // Remember for later use.
    for(int i = 0; i < listStorResp.storageassignments_size(); i++) {
        userStorage.push_back(listStorResp.storageassignments(i));
    }

    // Get user's datasets.
    req_session = listDatasetReq.mutable_session();
    *req_session = session;
    listDatasetReq.set_userid(uid);
    listDatasetReq.set_version("1.0");
    rc = VPLVsDirectory_ListOwnedDataSets(proxy, VPLTIME_FROM_SEC(30),
                                          listDatasetReq, listDatasetResp);
    if(rc != 0) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "ListOwnedDatasets query returned %d, detail:%d:%s",
                         rc, listDatasetResp.error().errorcode(),
                         listDatasetResp.error().errordetail().c_str());
        rv++;
    }
    else {
        // Make sure all expected datasets are present.
        rc += confirm_datasets(listDatasetResp, listStorResp);
        if(rc > 0) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "Expected default datasets not confirmed: %d.",
                             rc);
            rv++;
            goto exit;
        }
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

    // TODO: Un-purchase all titles.

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
    printf(" -v --verbose               Raise verbosity one level (may repeat 3 times)");
    printf(" -t --terse                 Lower verbosity (make terse) one level (may repeat 2 times or cancel a -v flag)");
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
    printf(" -P --skip-proxy            Skip PSN proxy-path tests.\n");
    printf(" -Z --skip-direct           Skip direct-path tests (PSN and infra).\n");
    printf(" -W --skip-vssi             Skip VSSI-based data access tests\n");
    printf(" -o --one-only              Test only one dataset.\n");
    printf(" -V --vsds-only             Test only VSDS queries.\n");
    printf(" -D --num-dirs              Number of directories per level.\n");
    printf(" -F --num-files             Number of files per leaf dir.\n");
    printf(" -L --num-levels            Number of directory levels.\n");
    printf(" -C --do-create             Create the directory tree.\n");
    printf(" -R --root-dir              Root directory.\n");
    printf(" -O --open-only             Open object only\n");
    printf(" -I --instance              Client instance ID\n");
    printf(" -c --cluster               cluster id\n");
    printf(" -S --file-size             size of the file to use\n");
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
        {"skip-proxy", no_argument, 0, 'P'},
        {"skip-direct", no_argument, 0, 'Z'},
        {"skip-vssi", no_argument, 0, 'W'},
        {"one-only", no_argument, 0, 'o'},
        {"vsds-only", no_argument, 0, 'V'},
        {"num-dirs", required_argument, 0, 'D'},
        {"num-files", required_argument, 0, 'F'},
        {"num-levels", required_argument, 0, 'L'},
        {"do-create", no_argument, 0, 'C'},
        {"root-dir", required_argument, 0, 'R'},
        {"open-only", required_argument, 0, 'O'},
        {"instance", required_argument, 0, 'I'},
        {"cluster", required_argument, 0, 'c'},
        {"file-size", required_argument, 0, 'S'},
        {"ts-ext-port", required_argument, 0, 'T'},
        {0,0,0,0}
    };

    for(;;) {
        int option_index = 0;
        
        int c = getopt_long(argc, argv, "vtu:p:d:q:ZWPoVi:r:D:F:L:CR:OI:c:S:T:", 
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

        case 'T': 
            tsext_port = (u16)atoi(optarg);
            break;

        case 'i': 
            ias_name = optarg; 
            break;

        case 'r': 
            ias_port = atoi(optarg);
            break;

        case 'P':
            skip_proxy = true;
            break;

        case 'Z':
            skip_direct = true;
            break;

        case 'W':
            skip_vssi = true;
            break;

        case 'o':
            one_only = true;
            break;

        case 'V':
            vsds_only = true;
            break;

        case 'D': 
            num_dirs = atoi(optarg);
            break;

        case 'F': 
            num_files = atoi(optarg);
            break;

        case 'L': 
            num_levels = atoi(optarg);
            break;

        case 'C':
            do_create = true;
            break;

        case 'R': 
            root_dir = optarg;
            break;

        case 'O':
            open_only = true;
            break;

        case 'I':
            clnt_instance.assign(optarg);
            break;

        case 'c':
            cluster_id = atoll(optarg);
            break;

        case 'S':
            file_size = atoll(optarg);
            break;

        default:
            usage(argc, argv);
            rv = -1;
            break;
        }
    }

    return rv;
}

static const char vsTest_main[] = "VS Test Main";
int main(int argc, char* argv[])
{
    int rv = 0; // pass until failed.
    int rc;
    std::vector<vplex::vsDirectory::Subscription>::iterator subscription_it;
    std::vector<vplex::vsDirectory::DatasetDetail>::iterator dataset_it;
    std::vector<vplex::vsDirectory::UserStorage>::iterator storage_it;
    u64 user_id = 0;
    u64 dataset_id = 0;

    vsTest_curTestName = vsTest_main;

    VPLTrace_SetBaseLevel(test_log_level);
    VPLTrace_SetShowTimeAbs(true);
    VPLTrace_SetShowTimeRel(false);
    VPLTrace_SetShowThread(false);

    if(parse_args(argc, argv) != 0) {
        goto exit;
    }

{
    // XXX - Remove this when CCDI integration is complete
    extern int ts_ext_port;
    ts_ext_port = tsext_port;
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
                        username, "acer", password, uid, session);
        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                            "TC_RESULT = %s ;;; TC_NAME = User_Login",
                            rc ? "FAIL" : "PASS");
        if(rc != 0) {
            rv++;
            goto exit;
        }

        rc = registerAsDevice(ias_name, ias_port, username, password,
                              testDeviceId,
                              clnt_instance.size() ? clnt_instance.c_str() :
                                                   NULL);
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
        
        // Perform initial condition setup queries.
        // (cleanup as if failed to cleanup last time.)
        do_cleanup_queries();

        // Perform data collection and setup queries.
        do_data_collection_queries();

        if(vsds_only) {
            goto cleanup;
        }

        // Set-up VSSI library
        rc = VSSI_Init();
        // rc = do_vssi_setup(testDeviceId);
        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                            "TC_RESULT = %s ;;; TC_NAME = VSSI_Setup"
                            "device id"FMTu64,
                            rc ? "FAIL" : "PASS", testDeviceId);
        if(rc != 0) {
            rv++;
            goto exit;
        }

        if(skip_vssi) {
            goto vssi_skipped;
        }

        // Loop through the virt drives
        for(dataset_it = ownedDatasets.begin();
            dataset_it != ownedDatasets.end();
            dataset_it++) {
            string location = dataset_it->datasetlocation();
            int tests_done = 0;
            
            VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                "Check dataset %s, ID "FMTx64" with location \n\t%s.",
                                dataset_it->datasetname().c_str(),
                                dataset_it->datasetid(),
                                location.c_str());

            // Find storage where dataset belongs
            for(storage_it = userStorage.begin();
                storage_it != userStorage.end();
                storage_it++) {
                if(storage_it->storageclusterid() == dataset_it->clusterid()) {
                    break;
                }
            }
            if ( storage_it == userStorage.end() ) {
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                    "Failed finding user storage");
                rv++;
                goto exit;
            }

            if(skip_direct) {
                goto direct_skipped;
            }

            user_id = dataset_it->userid();
            dataset_id = dataset_it->datasetid();

            // perform the same test using the new non-xml routines.
            VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                "Test dataset access2 for dataset %s, ID "FMTx64" with location cluster_id "FMTu64"\n\t%s.",
                                dataset_it->datasetname().c_str(),
                                dataset_it->datasetid(),
                                dataset_it->clusterid(),
                                location.c_str());

            if (rc == 0) {
                rc = test_vssi_dataset_access(user_id, dataset_id,
                                              num_dirs, num_files,
                                              num_levels, file_size,
                                              do_create, root_dir,
                                              open_only);
            }
            VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                "TC_RESULT = %s ;;; TC_NAME = Dataset_VSSI_Access2_"FMTx64,
                                rc ? "FAIL" : "PASS",
                                dataset_it->datasetid());
            rv += rc;
            goto exit;
        direct_skipped:
#ifdef NOTDEF 
            strip_route_info(route_info);
            if(route_info.num_routes > 0) {
                VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                    "Test dataset access2 for dataset %s, ID "FMTx64" with location (proxy route forced) \n\t%s.",
                                    dataset_it->datasetname().c_str(),
                                    dataset_it->datasetid(),
                                    location.c_str());
                
                rc = test_vssi_dataset_access(user_id, dataset_id,
                                              route_info,
                                              num_dirs, num_files,
                                              num_levels, file_size,
                                              do_create, root_dir,
                                              open_only);
                VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                    "TC_RESULT = %s ;;; TC_NAME = Dataset_VSSI_Proxy_Access2_"FMTx64,
                                    rc ? "FAIL" : "PASS",
                                    dataset_it->datasetid());
                rv += rc;
                if(rv) goto exit;
                tests_done++;
            }
#endif // NOTDEF

            if(tests_done != 0 && one_only) {
                break;
            }
        }
        // Report indeterminate if no datasets owned.
        if(ownedDatasets.empty()) {
            VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                "TC_RESULT = INDETERMINATE ;;; TC_NAME = Dataset_VSSI_Access");
        }

    vssi_skipped:

    cleanup:
        // Perform cleanup queries.
        do_cleanup_queries();
        VSSI_Cleanup();
    }
    else {
        rv = 1;
    }

 exit:
    
    VPLVsDirectory_DestroyProxy(proxy);

    return (rv) ? 1 : 0;
}
