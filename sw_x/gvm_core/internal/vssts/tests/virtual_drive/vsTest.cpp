/*
 *  Copyright 2012 Acer Cloud Technology Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF IGWARE INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
 *
 */

///Virtual Storage Offline Test
///
/// This test exercises the Virtual Storage offline usage.

//#include "vpl_net.h"
#include "vplex_trace.h"
#include "vplex_assert.h"
#include "vplex_vs_directory.h"
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

#include "vsTest_virtual_drive_test.hpp"

#include "vssts.hpp"
#include "vssts_error.hpp"
#include "ccdi.hpp"
#include "ccdi_client.hpp"

using namespace std;

static int test_log_level = TRACE_INFO;
static bool skip_direct = false;
static bool skip_proxy = false;
static bool wait_sync = false;
static string ccd_instance = "0";
/// Abort error catching
static jmp_buf vsTestErrExit;
string vsTest_curTestName;

static void signal_handler(int sig)
{
    UNUSED(sig);  // expected to be SIGABRT
    longjmp(vsTestErrExit, 1);
}

/// localization to use for all commands.
vplex::vsDirectory::Localization l10n;

static void usage(int argc, char* argv[])
{
    // Dump original command

    // Print usage message
    printf("Usage: %s [options]\n", argv[0]);
    printf("Options:\n");
    printf(" -v --verbose               Raise verbosity one level (may repeat 3 times)\n");
    printf(" -t --terse                 Lower verbosity (make terse) one level (may repeat 2 times or cancel a -v flag)\n");
    printf(" -P --skip-proxy            Skip PSN proxy-path tests.\n");
    printf(" -D --skip-direct           Skip direct-path tests (PSN and infra).\n");
    printf(" -i --ccd-instance          Number of ccd instance.\n");
    printf(" -w --wait-sync             Wait the orbe to sync device status.\n");
}

static int parse_args(int argc, char* argv[])
{
    int rv = 0;

    static struct option long_options[] = {
        {"verbose", no_argument, 0, 'v'},
        {"terse", no_argument, 0, 't'},
        {"skip-direct", no_argument, 0, 'D'},
        {"skip-proxy", no_argument, 0, 'P'},
        {"ccd-instance", required_argument, 0, 'i'},
        {"wait-sync", no_argument, 0, 'w'},
        {0,0,0,0}
    };

    for(;;) {
        int option_index = 0;

        int c = getopt_long(argc, argv, "vtDPi:w",
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
        case 'D':
            skip_direct = true;
            break;
        case 'P':
            skip_proxy = true;
            break;
        case 'i':
            ccd_instance = optarg;
            break;
        case 'w':
            wait_sync = true;
            break;
        default:
            usage(argc, argv);
            rv++;
            break;
        }
    }

    return rv;
}



static int getUserId(u64& userId)
{
    int rv = 0;
    {
        ccd::GetSystemStateInput ccdiRequest;
        ccdiRequest.set_get_players(true);
        ccd::GetSystemStateOutput ccdiResponse;
        rv = CCDIGetSystemState(ccdiRequest, ccdiResponse);
        if (rv != 0) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "%s failed: %d",
                             "CCDIGetSystemState",
                             rv);
            goto out;
        }
        userId = ccdiResponse.players().players(0).user_id();
        if (userId == 0) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Not signed-in!");
            rv = -1;
            goto out;
        }
    }
    {
        ccd::GetSyncStateInput ccdiRequest;
        ccdiRequest.set_user_id(userId);
        ccdiRequest.set_only_use_cache(true);
        ccd::GetSyncStateOutput ccdiResponse;
        rv = CCDIGetSyncState(ccdiRequest, ccdiResponse);
        if (rv != 0) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "%s failed: %d",
                             "CCDIGetSyncState",
                             rv);
            goto out;
        }
        if (!ccdiResponse.is_device_linked()) {
            // TODO: autolink?
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Device is not linked");
            rv = -2;
            goto out;
        }
        if (!ccdiResponse.is_sync_agent_enabled()) {
            // TODO: enable it?
            VPLTRACE_LOG_WARN(TRACE_BVS, 0,
                              "Sync agent is not enabled");
        }
    }
 out:
    return rv;
}

static int getDeviceId(u64& deviceId)
{
    int rv = 0;
    ccd::GetSystemStateInput ccdiRequest;
    ccdiRequest.set_get_device_id(true);
    ccd::GetSystemStateOutput ccdiResponse;
    rv = CCDIGetSystemState(ccdiRequest, ccdiResponse);
    if (rv != 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                         "%s failed: %d",
                         "CCDIGetSystemState",
                         rv);
        goto exit;
    }
    deviceId = ccdiResponse.device_id();
 exit:
    return rv;
}

static const string vsTest_main = "VS Test Main";
int main(int argc, char* argv[])
{
    int rv = 0; // pass until failed.
    int rc = -1;
    vsTest_curTestName = vsTest_main;

    VPLTrace_SetBaseLevel(test_log_level);
    VPLTrace_SetShowTimeAbs(true);
    VPLTrace_SetShowTimeRel(false);
    VPLTrace_SetShowThread(false);

    if(parse_args(argc, argv) != 0) {
        goto exit;
    }

    CCDIClient_SetTestInstanceNum(atoi(ccd_instance.c_str()));

    // Localization for all VS queries.
    l10n.set_language("en");
    l10n.set_country("US");
    l10n.set_region("USA");

    // catch SIGABRT
    ASSERT((signal(SIGABRT, signal_handler) == 0));

    if (setjmp(vsTestErrExit) == 0) { // initial invocation of setjmp
        // Run the tests. Any abort signal will hit the else block.
        u64 userId = 0;
        u64 deviceId = 0;
        u64 virtualDriveId = 0;
        u64 handle = 0;
//        string ticket;
        u64 datasetId = 0;

        //get user id
        rc = getDeviceId(deviceId);
        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                            "TC_RESULT = %s ;;; TC_NAME = Get_Device_Id",
                            rc ? "FAIL" : "PASS");
        if(rc != 0) {
            rv++;
            goto exit;
        }

        //get device id
        rc = getUserId(userId);
        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                            "TC_RESULT = %s ;;; TC_NAME = Get_User_Id",
                            rc ? "FAIL" : "PASS");
        if(rc != 0) {
            rv++;
            goto exit;
        }

        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                            "VALUE = "FMTu64" "FMTu64" ", 
                            deviceId,userId);

        //discover virtual drive
        //use the first one virtual drive?
        ccd::ListUserStorageOutput listSnOut;
        ccd::ListUserStorageInput listSnIn;
        listSnIn.set_user_id(userId);
        listSnIn.set_only_use_cache(true);
        rc = CCDIListUserStorage(listSnIn, listSnOut);
        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                         "TC_RESULT = %s ;;; TC_NAME = List User Storage",
                         rc ? "FAIL" : "PASS");
        if(rc != 0) {
            rv++;
            goto exit;
        }
 
        rc = -1;
        for (int i = 0; i < listSnOut.user_storage_size(); i++) {

            if(listSnOut.user_storage(i).featurevirtdriveenabled() &&
               virtualDriveId == 0) {
                handle = listSnOut.user_storage(i).accesshandle();
                //TODO other ticket
                virtualDriveId = listSnOut.user_storage(i).storageclusterid();
#ifdef  NDEF
                //ticket = listSnOut.user_storage(i).devspecaccessticket();
                {
                    ostringstream tohex;
                    for (int buf_ind = 0; buf_ind < ticket.length(); buf_ind++) {
                        tohex << std::hex << std::setfill('0') << 
                            std::setw(2) << 
                            std::nouppercase << 
                            (int)ticket[buf_ind];
                    }
                    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                        "Ticket value %s", tohex.str().c_str());
                }
#endif // NDEF
                //local route info
                ccd::ListLanDevicesInput listLanDevIn;
                ccd::ListLanDevicesOutput listLanDevOut;
                listLanDevIn.set_user_id(userId);
                listLanDevIn.set_include_unregistered(true);
                listLanDevIn.set_include_registered_but_not_linked(true);
                listLanDevIn.set_include_linked(true);
                rc = CCDIListLanDevices(listLanDevIn, listLanDevOut);
                if(rc != 0) {
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                     "List Lan Devices result %d", rc);
                    break;
                }

#ifdef NDEF
                int route_ind = 0;
                vssRouteInfo.num_routes = 0;
                vssRouteInfo.routes = new VSSI_Route[listSnOut.user_storage(i).storageaccess_size()+
                                                     listLanDevOut.infos_size()];
                memset(vssRouteInfo.routes, 0, sizeof(VSSI_Route)*(listSnOut.user_storage(i).storageaccess_size()+listLanDevOut.infos_size()));
                for(int j = 0; j < listLanDevOut.infos_size(); j++) {
                    if(listLanDevOut.infos(j).device_id() == virtualDriveId) {
                        vssRouteInfo.routes[route_ind].server = strdup(listLanDevOut.infos(j).route_info().ip_v4_address().c_str());
                        vssRouteInfo.routes[route_ind].port = listLanDevOut.infos(j).route_info().virtual_drive_port();
                        vssRouteInfo.routes[route_ind].type = vplex::vsDirectory::DIRECT_INTERNAL;
                        vssRouteInfo.routes[route_ind].proto =  vplex::vsDirectory::VS;
                        vssRouteInfo.routes[route_ind].cluster_id = virtualDriveId;
                        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,"server : %s.", vssRouteInfo.routes[route_ind].server);
                        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,"port : %u.", vssRouteInfo.routes[route_ind].port);
                        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,"type : %d.", vssRouteInfo.routes[route_ind].type);
                        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,"proto : %d.", vssRouteInfo.routes[route_ind].proto);
                        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,"cluster_id : "FMTu64".", vssRouteInfo.routes[route_ind].cluster_id);
                        route_ind++;
                    }
                }              
                for( int j = 0 ; j < listSnOut.user_storage(i).storageaccess_size() ; j++ ) {
                    bool port_found = false;
                    for( int k = 0 ; k < listSnOut.user_storage(i).storageaccess(j).ports_size() ; k++ ) {
                        if ( listSnOut.user_storage(i).storageaccess(j).ports(k).porttype() != 
                                vplex::vsDirectory::PORT_VSSI ) {
                            continue;
                        }
                        if ( listSnOut.user_storage(i).storageaccess(j).ports(k).port() == 0 ) {
                            continue;
                        }
                        vssRouteInfo.routes[route_ind].port =
                            listSnOut.user_storage(i).storageaccess(j).ports(k).port();
                        port_found = true;
                        break;
                    }
                    if ( !port_found ) {
                        continue;
                    }
                    vssRouteInfo.routes[route_ind].server = strdup(listSnOut.user_storage(i).storageaccess(j).server().c_str());
                    vssRouteInfo.routes[route_ind].type = listSnOut.user_storage(i).storageaccess(j).routetype();
                    vssRouteInfo.routes[route_ind].proto = listSnOut.user_storage(i).storageaccess(j).protocol();
                    vssRouteInfo.routes[route_ind].cluster_id = listSnOut.user_storage(i).storageclusterid();
                    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,"server : %s.", vssRouteInfo.routes[route_ind].server);
                    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,"port : %u.", vssRouteInfo.routes[route_ind].port);
                    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,"type : %d.", vssRouteInfo.routes[route_ind].type);
                    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,"proto : %d.", vssRouteInfo.routes[route_ind].proto);
                    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,"cluster_id : "FMTu64".", vssRouteInfo.routes[route_ind].cluster_id);
                    route_ind++;
                }
                if ( route_ind == 0 ) {
                    VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                     "No VS routes found.");
                    rc = -1;
                } else {
                    vssRouteInfo.num_routes = route_ind;
                }
                break;
#endif // NDEF
            }
        }
        if(virtualDriveId == 0) {
            VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                             "Can't find virtual drive");
            rc = -1;
        }

        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                            "TC_RESULT = %s ;;; TC_NAME = Find virtual drive and route info.",
                            rc ? "FAIL" : "PASS");
        if(rc != 0) {
            rv++;
            goto exit;
        }

        //find associated datasets*
        {
            ccd::ListOwnedDatasetsInput listDstIn;
            ccd::ListOwnedDatasetsOutput listDstOut;
            listDstIn.set_user_id(userId);
            listDstIn.set_only_use_cache(true);
            rc = CCDIListOwnedDatasets(listDstIn, listDstOut);

            if(rc != CCD_OK) { 
                VPLTRACE_LOG_ERR(TRACE_BVS, 0,
                                 "%s failed: %d",
                                 "CCDIListOwnedDatasets",
                                 rc);
            } else {
                rc = -1;
                for(int i = 0; i < listDstOut.dataset_details_size(); i++) {
                    if(listDstOut.dataset_details(i).clusterid() ==  virtualDriveId
                       //&& listDstOut.created_by_this_device(i)){
                       && listDstOut.dataset_details(i).datasettype() == vplex::vsDirectory::VIRT_DRIVE) {
                        datasetId = listDstOut.dataset_details(i).datasetid();
                        rc = 0; 
                        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,"datasetId : "FMTu64".", datasetId);
                        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,"contentType : %s.", listDstOut.dataset_details(i).contenttype().c_str());
                        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,"datasetName : %s.", listDstOut.dataset_details(i).datasetname().c_str());
                        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,"datasetLocation : %s.", listDstOut.dataset_details(i).datasetlocation().c_str());
                        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,"storageclustername : %s.", listDstOut.dataset_details(i).storageclustername().c_str());
                        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,"storageclusterhostname : %s.", listDstOut.dataset_details(i).storageclusterhostname().c_str());
                        break;
                    }
                }
            }
        }
        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                            "TC_RESULT = %s ;;; TC_NAME = Find Owned Datasets",
                            rc ? "FAIL" : "PASS");
        if(rc != 0) {
            rv++;
            goto exit;
        }


        if(wait_sync) {
            VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                "Wait the orbe to sync device status for 10 minutes,"\
                                " you can unlink the client via OPS now.");
            
            sleep(600);
        }

        //vssi setup
        rc = VSSI_Init(13 /* test value */);
        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                            "TC_RESULT = %s ;;; TC_NAME = VSSI_Setup.",
                            rc ? "FAIL" : "PASS");
        if(rc != 0) {
            rv++;
            goto exit;
        }

       goto direct;

 direct:
//        if(skip_direct)
//            goto proxy;
 
        rc = test_virtual_drive_access(userId,
                                       datasetId,
                                       deviceId,
                                       handle
                                       );
        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                            "TC_RESULT = %s ;;; TC_NAME = Direct Virtual Drive Access",
                            rc ? "FAIL" : "PASS");
        if(rc != 0) {
            rv++;
            goto fail;
        }

#ifdef NDEF
        //negative test - vssi session registration
        rc = test_virtual_drive_access(userId,
                                       datasetId,
                                       deviceId,
                                       handle
                                       );
        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                            "TC_RESULT = %s ;;; TC_NAME = Direct Virtual Drive Access, Negative Test",
                            !rc ? "FAIL" : "PASS");
        if(rc == 0) {
            rv++;
            goto fail;
        }

        //vssi setup
        rc = do_vssi_setup(deviceId);
        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                            "TC_RESULT = %s ;;; TC_NAME = VSSI_Setup.",
                            rc ? "FAIL" : "PASS");
        if(rc != 0) {
            rv++;
            goto fail;
        }

        goto proxy;
 proxy:
        if(skip_proxy)
            goto fail;
        strip_route_info(vssRouteInfo);

        for(int route_ind = 0 ; route_ind < vssRouteInfo.num_routes ; route_ind++) {                            
            VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,"server : %s.", vssRouteInfo.routes[route_ind].server);
            VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,"port : %u.", vssRouteInfo.routes[route_ind].port);
            VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,"type : %d.", vssRouteInfo.routes[route_ind].type);
            VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,"proto : %d.", vssRouteInfo.routes[route_ind].proto);
            VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,"cluster_id : "FMTu64".", vssRouteInfo.routes[route_ind].cluster_id);

        }

        rc = test_virtual_drive_access(userId,
                                       datasetId,
                                       deviceId,
                                       handle,
                                       );
        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                            "TC_RESULT = %s ;;; TC_NAME = Proxy Virtual Drive Access",
                            rc ? "FAIL" : "PASS");
        if(rc != 0) {
            rv++;
            goto fail;
        }

        //negative test - vssi session registration
        rc = test_virtual_drive_access(userId,
                                       datasetId,
                                       deviceId,
                                       handle
                                       );
        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                            "TC_RESULT = %s ;;; TC_NAME = Proxy Virtual Drive Access, Negative Test",
                            !rc ? "FAIL" : "PASS");
        if(rc == 0) {
            rv++;
            goto fail;
        }
#endif // NDEF
 fail:
//        do_vssi_cleanup();
        VSSI_Cleanup();
    }
    else {
        rv = 1;
    }
 exit:
    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                        "TC_RESULT = %s ;;; TC_NAME = Virtual Drive Test",
                        rv ? "FAIL" : "PASS");
    return (rv) ? 1 : 0;
}
