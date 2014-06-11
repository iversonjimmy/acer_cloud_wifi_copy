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
#include "vsTest_vscs_async.hpp"
#include "vsTest_personal_cloud_http.hpp"

#include "vssi.h"
#include "vssi_error.h"

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

std::string lab_name = "www-c100.pc-int.igware.net";
u16 lab_port = 443;

u16 vss_http_port = 17144;

/// Session parameters picked up on successful login
VPLUser_Id_t uid = 0;
vplex::vsDirectory::SessionInfo session;
extern VSSI_Session vssi_session;

u64 testDeviceId = 0;  // to be obtained from the infra at runtime

/// localization to use for all commands.
vplex::vsDirectory::Localization l10n;

// VSDS query proxy
VPLVsDirectory_ProxyHandle_t proxy;

/// Contents and datasets collected for test.
std::vector<vplex::vsDirectory::TitleDetail> ownedTitles;
std::vector<vplex::vsDirectory::Subscription> subscribedDatasets;
std::vector<vplex::vsDirectory::DatasetDetail> ownedDatasets;
std::vector<vplex::vsDirectory::UserStorage> userStorage;

/// Skip various parts of the test on-demand
bool vsds_only = false;
bool skip_content = false;
bool skip_subscriptions = false;
bool skip_vssi = false;
bool skip_xml = false;
bool one_only = false;
bool http_verbose = false;
bool camera_roll_only = false;

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
            // Datacenter storage
            bool userFound = false;
            bool clearfiFound = false;
            bool crUpFound = false;
            bool crDownFound = false;
            bool pimContactsFound = false;
            bool pimEventsFound = false;
            bool pimNotesFound = false;
            bool pimTasksFound = false;
            bool pimFavoritesFound = false;

            for(int i = 0; i < listDatasetResp.datasets_size(); i++) {
                const vplex::vsDirectory::DatasetDetail& datasetDetail = 
                    listDatasetResp.datasets(i);
                if(datasetDetail.storageclustername().
                   compare(storDetail.storagename()) == 0) {
                    VPLTRACE_LOG_INFO(TRACE_APP, 0,
                                      "Dataset "FMTu64" with type %s found in datacenter storage.",
                                      datasetDetail.datasetid(), DatasetType_Name(datasetDetail.datasettype()).c_str());
                    switch(datasetDetail.datasettype()) {
                    case vplex::vsDirectory::USER:
                        if(userFound) {
                            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                                             "Datacenter storage has more than one USER dataset.");
                            rv++;
                        }
                        userFound = true;
                        break;
                    case vplex::vsDirectory::CLEAR_FI:
                        if(clearfiFound) {
                            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                                             "Datacenter storage has more than one CLEAR_FI dataset.");
                            rv++;
                        }
                        clearfiFound = true;
                        break;
                    case vplex::vsDirectory::CR_UP:
                        if(crUpFound) {
                            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                                             "Datacenter storage has more than one CR_UP dataset.");
                            rv++;
                        }
                        crUpFound = true;
                        break;
                    case vplex::vsDirectory::CR_DOWN:
                        if(crDownFound) {
                            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                                             "Datacenter storage has more than one CR_DOWN dataset.");
                            rv++;
                        }
                        crDownFound = true;
                        break;
                    case vplex::vsDirectory::PIM_CONTACTS:
                        if(pimContactsFound) {
                            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                                             "Datacenter storage has more than one PIM_CONTACTS dataset.");
                            rv++;
                        }
                        pimContactsFound = true;
                        break;

                    case vplex::vsDirectory::PIM_EVENTS:
                        if(pimEventsFound) {
                            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                                             "Datacenter storage has more than one PIM_EVENTS dataset.");
                            rv++;
                        }
                        pimEventsFound = true;
                        break;
                    case vplex::vsDirectory::PIM_NOTES:
                        if(pimNotesFound) {
                            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                                             "Datacenter storage has more than one PIM_NOTES dataset.");
                            rv++;
                        }
                        pimNotesFound = true;
                        break;
                    case vplex::vsDirectory::PIM_TASKS:
                        if(pimTasksFound) {
                            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                                             "Datacenter storage has more than one PIM_TASKS dataset.");
                            rv++;
                        }
                        pimTasksFound = true;
                        break;
                    case vplex::vsDirectory::PIM_FAVORITES:
                        if(pimFavoritesFound) {
                            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                                             "Datacenter storage has more than one PIM_FAVORITES dataset.");
                            rv++;
                        }
                        pimFavoritesFound = true;
                        break;
                    case vplex::vsDirectory::CACHE:
                        VPLTRACE_LOG_INFO(TRACE_APP, 0, 
                                          "Datacenter storage has dataset of type CACHE - ignored.");
                        break;
                    case vplex::vsDirectory::USER_CONTENT_METADATA:
                        VPLTRACE_LOG_INFO(TRACE_APP, 0, 
                                          "Datacenter storage has dataset of type USER_CONTENT_METADATA - ignored.");
                        break;
                    case vplex::vsDirectory::CAMERA:
                    case vplex::vsDirectory::PIM:
                    case vplex::vsDirectory::MEDIA:
                    case vplex::vsDirectory::MEDIA_METADATA:
                    case vplex::vsDirectory::FS:
                    case vplex::vsDirectory::VIRT_DRIVE:
                    case vplex::vsDirectory::CLEARFI_MEDIA:
                    case vplex::vsDirectory::SYNCBOX:
                    case vplex::vsDirectory::SBM:
                    case vplex::vsDirectory::SWM:
                    default:
                        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                                         "Datacenter storage has unexpected dataset of type %d.",
                                         datasetDetail.datasettype());
                        rv++;
                        break;
                    }
                }
            }

            if(!userFound) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                                 "Datacenter storage did not find USERDATA datasets.");
                rv++;
            }
            if(!clearfiFound) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                                 "Datacenter storage did not find clear.fi datasets.");
                rv++;
            }
            if(!crUpFound) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                                 "Datacenter storage did not find CameraRoll Upload datasets.");
                rv++;
            }
            if(!crDownFound) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                                 "Datacenter storage did not find CameraRoll Download datasets.");
                rv++;
            }
#if 0
            if(!pimContactsFound) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                                 "Datacenter storage did not find PIM_CONTACTS datasets.");
                rv++;
            }
            if(!pimEventsFound) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                                 "Datacenter storage did not find PIM_EVENTS datasets.");
                rv++;
            }
            if(!pimNotesFound) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                                 "Datacenter storage did not find PIM_NOTES datasets.");
                rv++;
            }
            if(!pimTasksFound) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                                 "Datacenter storage did not find PIM_TASKS datasets.");
                rv++;
            }
            if(!pimFavoritesFound) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                                 "Datacenter storage did not find PIM_FAVORITES datasets.");
                rv++;
            }
#endif
        }
#ifdef NOTDEF
        else {
            // PSN storage
            bool found_Music = false;
            bool found_Pictures = false;
            bool found_Videos = false;

            for(int i = 0; i < listDatasetResp.datasets_size(); i++) {
                const vplex::vsDirectory::DatasetDetail& datasetDetail = 
                    listDatasetResp.datasets(i);
                if(datasetDetail.storageclustername().
                   compare(storDetail.storagename()) == 0) {
                    VPLTRACE_LOG_INFO(TRACE_APP, 0,
                                      "Dataset "FMTu64" with type %d found in PSN storage.",
                                      datasetDetail.datasetid(), datasetDetail.datasettype());

                    switch(datasetDetail.datasettype()) {
                    case vplex::vsDirectory::USER:
                        if (datasetDetail.datasetname() == "Music") {
                            found_Music = true;
                        }
                        if (datasetDetail.datasetname() == "Pictures") {
                            found_Pictures = true;
                        }
                        if (datasetDetail.datasetname() == "Videos") {
                            found_Videos = true;
                        }
                        break;
                    case vplex::vsDirectory::MEDIA:
                    case vplex::vsDirectory::FS:
                    case vplex::vsDirectory::VIRT_DRIVE:
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

            if(!found_Music) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                                 "PSN storage did not find Music dataset.");
                rv++;
            }
#if 0
            if(!found_Pictures) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                                 "PSN storage did not find Pictures dataset.",
                rv++;
            }
            if(!found_Videos) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                                 "PSN storage did not find Videos dataset.");
                rv++;
            }
#endif
        }
#endif // NOTDEF
    }

    return rv;
}

static int test_dataset_subscriptions(const vplex::vsDirectory::ListOwnedDataSetsOutput& listDatasetResp)
{
    int rv = 0;
    int rc;
    vplex::vsDirectory::SessionInfo* req_session;
    vplex::vsDirectory::AddDatasetSubscriptionInput subReq;
    vplex::vsDirectory::AddDatasetSubscriptionOutput subResp;
    bool pass = false;

    req_session = subReq.mutable_session();
    *req_session = session;
    subReq.set_userid(uid);
    subReq.set_deviceid(testDeviceId);
    
    for(int i = 0; i < listDatasetResp.datasets_size(); i++) {
        subReq.set_datasetid(listDatasetResp.datasets(i).datasetid());
        subReq.set_datasettype(listDatasetResp.datasets(i).datasettype());

        // Unsubscribe the dataset, just in case subscription is left over.
        unsubscribeDataset(proxy, session, uid, testDeviceId, subReq.datasetid());
        
        // Subscribe each dataset for its testing role.        
        for(vplex::vsDirectory::SubscriptionRole role = vplex::vsDirectory::SubscriptionRole_MIN;
            role <= vplex::vsDirectory::SubscriptionRole_MAX;
            role = (vplex::vsDirectory::SubscriptionRole)(((int)role) + 1)) {
            subReq.set_role(role);
            
            rc = VPLVsDirectory_AddDatasetSubscription(proxy,
                                                       VPLTIME_FROM_SEC(30),
                                                       subReq, subResp);
            VPLTRACE_LOG_INFO(TRACE_APP, 0,
                              "AddDatasetSubscription of dataset type %d for role %d returned %d, detail:%d:%s",
                              subReq.datasettype(), subReq.role(),
                              rc, subResp.error().errorcode(),
                              subResp.error().errordetail().c_str());
            switch(subReq.datasettype()) {
            case vplex::vsDirectory::USER:
                switch(subReq.role()) {
                case vplex::vsDirectory::GENERAL:
                case vplex::vsDirectory::CLEARFI_SERVER:
                case vplex::vsDirectory::CLEARFI_CLIENT:
                case vplex::vsDirectory::READER:
                case vplex::vsDirectory::WRITER:
                case vplex::vsDirectory::PRODUCER:
                case vplex::vsDirectory::CONSUMER:
                    pass = (rc == 0);
                    break;
                default:
                    pass = false;
                    break;
                }
                break;
            case vplex::vsDirectory::PIM:
            case vplex::vsDirectory::PIM_CONTACTS:
            case vplex::vsDirectory::PIM_EVENTS:
            case vplex::vsDirectory::PIM_NOTES:
            case vplex::vsDirectory::PIM_TASKS:
            case vplex::vsDirectory::PIM_FAVORITES:
                switch(subReq.role()) {
                case vplex::vsDirectory::GENERAL:
                    pass = (rc == 0);
                    break;
                case vplex::vsDirectory::CLEARFI_SERVER:
                case vplex::vsDirectory::CLEARFI_CLIENT:
                case vplex::vsDirectory::READER:
                case vplex::vsDirectory::WRITER:
                case vplex::vsDirectory::PRODUCER:
                case vplex::vsDirectory::CONSUMER:
                    pass = (rc != 0);
                    break;
                default:
                    pass = false;
                    break;
                }
                break;
            case vplex::vsDirectory::CLEAR_FI:
                switch(subReq.role()) {
                case vplex::vsDirectory::GENERAL:
                case vplex::vsDirectory::CLEARFI_SERVER:
                case vplex::vsDirectory::CLEARFI_CLIENT:
                case vplex::vsDirectory::READER:
                case vplex::vsDirectory::WRITER:
                    pass = (rc == 0);
                    break;
                case vplex::vsDirectory::PRODUCER:
                case vplex::vsDirectory::CONSUMER:
                    pass = (rc != 0);
                    break;
                default:
                    pass = false;
                    break;
                }
                break;
            case vplex::vsDirectory::CR_UP:
                switch(subReq.role()) {
                case vplex::vsDirectory::PRODUCER:
                    pass = (rc == 0);
                    break;
                case vplex::vsDirectory::GENERAL:
                case vplex::vsDirectory::CLEARFI_SERVER:
                case vplex::vsDirectory::CLEARFI_CLIENT:
                case vplex::vsDirectory::READER:
                case vplex::vsDirectory::WRITER:
                case vplex::vsDirectory::CONSUMER:
                    pass = (rc != 0);
                    break;
                default:
                    pass = false;
                    break;
                }
                break;
            case vplex::vsDirectory::CR_DOWN:
            case vplex::vsDirectory::MEDIA:
            case vplex::vsDirectory::MEDIA_METADATA:
                switch(subReq.role()) {
                case vplex::vsDirectory::CONSUMER:
                    pass = (rc == 0);
                    break;
                case vplex::vsDirectory::GENERAL:
                case vplex::vsDirectory::PRODUCER:
                case vplex::vsDirectory::CLEARFI_SERVER:
                case vplex::vsDirectory::CLEARFI_CLIENT:
                case vplex::vsDirectory::READER:
                case vplex::vsDirectory::WRITER:
                    pass = (rc != 0);
                    break;
                default:
                    pass = false;
                    break;
                }
                break;
            case vplex::vsDirectory::CACHE:
                switch(subReq.role()) {
                case vplex::vsDirectory::GENERAL:
                case vplex::vsDirectory::CLEARFI_SERVER:
                case vplex::vsDirectory::CLEARFI_CLIENT:
                case vplex::vsDirectory::READER:
                case vplex::vsDirectory::WRITER:
                case vplex::vsDirectory::PRODUCER:
                case vplex::vsDirectory::CONSUMER:
                    pass = (rc != 0);
                    break;
                default:
                    pass = false;
                    break;
                }
                break;
            default:
            case vplex::vsDirectory::CAMERA:
            case vplex::vsDirectory::FS:
            case vplex::vsDirectory::VIRT_DRIVE:
            case vplex::vsDirectory::CLEARFI_MEDIA:
            case vplex::vsDirectory::USER_CONTENT_METADATA:
            case vplex::vsDirectory::SYNCBOX:
            case vplex::vsDirectory::SBM:
            case vplex::vsDirectory::SWM:
                // Don't subscribe to unexpected datasets.
                pass = true;
                break;
            }
            if(!pass) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: "
                                 "%s dataset type %d %s with type %d.",
                                 listDatasetResp.datasets(i).datasetname().c_str(),
                                 listDatasetResp.datasets(i).datasettype(),
                                 (rc == 0) ? "incorrectly subscribed" :
                                 "failed to subscribe",
                                 subReq.role());
                rv++;
            }

            // Have to unsubscribe for next role if subscription succeeded.
            if(rc == 0) {
                rc = unsubscribeDataset(proxy, session, uid, testDeviceId, subReq.datasetid());
                if(rc != 0) {
                    rv++;
                }
            }
        }
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
    vplex::vsDirectory::GetSubscriptionDetailsForDeviceInput listSubReq;
    vplex::vsDirectory::GetSubscriptionDetailsForDeviceOutput listSubResp;
    vplex::vsDirectory::AddDatasetSubscriptionInput subReq;
    vplex::vsDirectory::AddDatasetSubscriptionOutput subResp;

    // TODO: Purchase a test title.


    // Get titles for the test user.
    if(!skip_content) {
        rv += getOwnedTitles(proxy, session, l10n, ownedTitles);
    }

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

        if(!skip_subscriptions) {
            // Try each role. Make sure invalid roles for dataset type fail.
            rc += test_dataset_subscriptions(listDatasetResp);
            if(rc > 0) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "Dataset subscribe tests failed: %d.",
                                 rc);
                rv++;
                goto exit;
            }
        }

        req_session = subReq.mutable_session();
        *req_session = session;
        subReq.set_userid(uid);
        subReq.set_deviceid(testDeviceId);

        // Subscribe this device to each dataset.
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                            "User "FMTu64" has %d datasets.",
                            uid, listDatasetResp.datasets_size());
        for(int i = 0; i < listDatasetResp.datasets_size(); i++) {
            subReq.set_datasetid(listDatasetResp.datasets(i).datasetid());
            subReq.set_datasettype(listDatasetResp.datasets(i).datasettype());
            bool skip = false;

            // Use appropriate subscription role per subscription.
            switch(subReq.datasettype()) {
            case vplex::vsDirectory::USER:
            case vplex::vsDirectory::PIM: // deprecated
            case vplex::vsDirectory::CAMERA: // deprecated
            case vplex::vsDirectory::PIM_CONTACTS:
            case vplex::vsDirectory::PIM_EVENTS:
            case vplex::vsDirectory::PIM_NOTES:
            case vplex::vsDirectory::PIM_TASKS:
            case vplex::vsDirectory::PIM_FAVORITES:
                subReq.set_role(vplex::vsDirectory::GENERAL);
                break;
            case vplex::vsDirectory::CLEAR_FI:
                subReq.set_role(vplex::vsDirectory::CLEARFI_SERVER);
                break;
            case vplex::vsDirectory::CR_UP:
                subReq.set_role(vplex::vsDirectory::PRODUCER);
                break;
            case vplex::vsDirectory::CR_DOWN:
            case vplex::vsDirectory::MEDIA:
            case vplex::vsDirectory::MEDIA_METADATA:
                subReq.set_role(vplex::vsDirectory::CONSUMER);
                break;
            case vplex::vsDirectory::CACHE:  // FIXME?
            case vplex::vsDirectory::FS:
            case vplex::vsDirectory::VIRT_DRIVE:
            case vplex::vsDirectory::CLEARFI_MEDIA:
            case vplex::vsDirectory::USER_CONTENT_METADATA:
            case vplex::vsDirectory::SYNCBOX:
            case vplex::vsDirectory::SBM:
            case vplex::vsDirectory::SWM:
                skip = true;
                break;
            }

            // Remember the dataset for HTTP testing.
            ownedDatasets.push_back(listDatasetResp.datasets(i));
            VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                                "Found dataset %s, ID:"FMTu64" stored at cluster %s, reachable at %s:%u.",
                                listDatasetResp.datasets(i).datasetname().c_str(),
                                listDatasetResp.datasets(i).datasetid(),
                                listDatasetResp.datasets(i).storageclustername().c_str(),
                                listDatasetResp.datasets(i).storageclusterhostname().c_str(),
                                listDatasetResp.datasets(i).storageclusterport());

            if(skip) {
                continue;
            }
            
            rc = VPLVsDirectory_AddDatasetSubscription(proxy,
                                                       VPLTIME_FROM_SEC(30),
                                                       subReq, subResp);
            if(rc != 0) {
                VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                                 "AddDatasetSubscription query returned %d, detail:%d:%s",
                                 rc, listStorResp.error().errorcode(),
                                 listStorResp.error().errordetail().c_str());
            }
        }
    }

    // Get subscriptions for this device.
    req_session = listSubReq.mutable_session();
    *req_session = session;
    listSubReq.set_userid(uid);
    listSubReq.set_deviceid(testDeviceId);
    rc = VPLVsDirectory_GetSubscriptionDetailsForDevice(proxy, VPLTIME_FROM_SEC(30),
                                                        listSubReq, listSubResp);
    if(rc != 0) {
        VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                         "GetSubscriptionDetailsForDevice query returned %d, detail:%d:%s",
                         rc, listStorResp.error().errorcode(),
                         listStorResp.error().errordetail().c_str());
        rv++;
    }
    else {
        VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                            "User "FMTu64" has %d subscriptions for device "FMTx64".",
                            uid, listSubResp.subscriptions_size(), testDeviceId);
        for(int i = 0; i < listSubResp.subscriptions_size(); i++) {
            subscribedDatasets.push_back(listSubResp.subscriptions(i));
            VPLTRACE_LOG_ALWAYS(TRACE_APP, 0,
                                "Found subscription to dataset %s, ID:"FMTu64" found at location %s, datasetroot:%s.",
                                listSubResp.subscriptions(i).datasetname().c_str(),
                                listSubResp.subscriptions(i).datasetid(),
                                listSubResp.subscriptions(i).datasetlocation().c_str(),
                                listSubResp.subscriptions(i).datasetroot().c_str());
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
    printf(" -l --lab-name NAME         Lab server name or IP address (%s)\n",
           lab_name.c_str());
    printf(" -L --lab-port PORT         Lab server port (%d)\n",
           lab_port);
    printf(" -u --username USERNAME     User name (%s)\n",
           username.c_str());
    printf(" -p --password PASSWORD     User password (%s)\n",
           password.c_str());
    printf(" -C --skip-content          Skip content access tests.\n");
    printf(" -S --skip-subscriptions    Skip subscription testing.\n");
    printf(" -W --skip-vssi             Skip VSSI-based data access tests\n");
    printf(" -O --one-only              Test only one dataset.\n");
    printf(" -R --camera-roll-only      Test only the CameraRoll feature datasets.\n");
    printf(" -V --vsds-only             Test only VSDS queries.\n");
    printf(" -H --http-verbose          Verbose logging of HTTP webservice queries.\n");
    printf(" -X --skip-XML              Skip XML-based open object tests\n");
    printf(" -N --run-cloudnode-tests   Run tests that are specific to the cloud node\n");
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
        {"skip-content", no_argument, 0, 'C'},
        {"skip-proxy", no_argument, 0, 'P'},
        {"skip-direct", no_argument, 0, 'D'},
        {"skip-subscriptions", no_argument, 0, 'S'},
        {"skip-vssi", no_argument, 0, 'W'},
        {"one-only", no_argument, 0, 'O'},
        {"vsds-only", no_argument, 0, 'V'},
        {"camera-roll-only", no_argument, 0, 'R'},
        {"http-verbose", no_argument, 0, 'H'},
        {"skip-XML", no_argument, 0, 'X'},
        {"run-cloudnode-tests", no_argument, 0, 'N'},
        {0,0,0,0}
    };

    for(;;) {
        int option_index = 0;
        
        int c = getopt_long(argc, argv, "vtu:p:l:L:CIDSWPOVHRX", 
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

        case 'C':
            skip_content = true;
            break;

        case 'S':
            skip_subscriptions = true;
            break;

        case 'W':
            skip_vssi = true;
            break;

        case 'O':
            one_only = true;
            break;

        case 'R':
            camera_roll_only = true;
            break;

        case 'V':
            vsds_only = true;
            break;

        case 'H':
            http_verbose = true;
            break;

        case 'X':
            skip_xml = true;
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

static const char vsTest_main[] = "VS Test Main";
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
                        "     Lab address: %s:%d", 
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

        if(vsds_only) {
            goto cleanup;
        }

        // Set-up VSSI library
        rc = do_vssi_setup(testDeviceId, session, vssi_session);
        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                            "TC_RESULT = %s ;;; TC_NAME = VSSI_Setup",
                            rc ? "FAIL" : "PASS");
        if(rc != 0) {
            rv++;
            goto exit;
        }

        if(skip_vssi) {
            goto vssi_skipped;
        }

        if(skip_content) {
            goto content_skipped;
        }

        if(camera_roll_only) {
            goto camera_roll_test;
        }

        // Do content access tests.
        for(title_it = ownedTitles.begin();
            title_it != ownedTitles.end();
            title_it++) {
            for(int content = 0;
                content < title_it->contents_size(); 
                content++) {                
                VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                    "Test content access for title %s content %s with location \n\t%s.",
                                    title_it->titleid().c_str(),
                                    title_it->contents(content).contentid().c_str(),
                                    title_it->contents(content).contentlocation().c_str());
                                    
                // Perform large-volume test for first title/content only.
                if(title_it == ownedTitles.begin() &&
                   content == 0) {
                    rc = test_vssi_content_access(title_it->contents(content).contentlocation(),
                                                  true);
                }
                else {
                    rc = test_vssi_content_access(title_it->contents(content).contentlocation(),
                                                   false);
                }

                VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                    "TC_RESULT = %s ;;; TC_NAME = Content_Access_%s_%s",
                                    rc ? "FAIL" : "PASS",
                                    title_it->titleid().c_str(),
                                    title_it->contents(content).contentid().c_str());
                rv += rc;
                if(rv) goto exit;
            }
        }
    content_skipped:

        // Do dataset VSS-access tests.
        for(dataset_it = ownedDatasets.begin();
            dataset_it != ownedDatasets.end();
            dataset_it++) {
            string location = dataset_it->datasetlocation();
            int tests_done = 0;
            bool skip;
            
            VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                "Check dataset %s, ID "FMTx64" with location \n\t%s.",
                                dataset_it->datasetname().c_str(),
                                dataset_it->datasetid(),
                                location.c_str());

            // Only test USER type datasets.
            skip = false;
            switch(dataset_it->datasettype()) {
            case vplex::vsDirectory::USER:
            case vplex::vsDirectory::CACHE:
                break;
            case vplex::vsDirectory::CAMERA:
            case vplex::vsDirectory::CLEAR_FI:
            case vplex::vsDirectory::PIM:
            case vplex::vsDirectory::PIM_CONTACTS:
            case vplex::vsDirectory::PIM_EVENTS:
            case vplex::vsDirectory::PIM_NOTES:
            case vplex::vsDirectory::PIM_TASKS:
            case vplex::vsDirectory::PIM_FAVORITES:
            case vplex::vsDirectory::MEDIA:
            case vplex::vsDirectory::MEDIA_METADATA:
            case vplex::vsDirectory::CR_UP:
            case vplex::vsDirectory::CR_DOWN:
            case vplex::vsDirectory::FS:
            case vplex::vsDirectory::VIRT_DRIVE:
            case vplex::vsDirectory::CLEARFI_MEDIA:
            case vplex::vsDirectory::USER_CONTENT_METADATA:
            case vplex::vsDirectory::SYNCBOX:
            case vplex::vsDirectory::SBM:
            case vplex::vsDirectory::SWM:
            default:
                skip = true;
                break;
            }
            if(skip) {
                continue;
            }

            // Find storage where dataset belongs
            for(storage_it = userStorage.begin();
                storage_it != userStorage.end();
                storage_it++) {
                if(dataset_it->clusterid() == storage_it->storageclusterid()) {
                    break;
                }
            }

            // Skip datasets on PSNs as they no longer support
            // transactions
            if ( storage_it->storagetype() != 0 ) {
                continue;
            }

            if((rc = setup_route_access_info(*storage_it, *dataset_it,
                                             user_id, dataset_id, route_info))) {
                VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                    "setup_route_access_info() - %d", rc);
            }

            if ( skip_xml ) {
                goto xml_skipped;
            }
            VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                "Test dataset access for dataset %s, ID "FMTx64" with location \n\t%s.",
                                dataset_it->datasetname().c_str(),
                                dataset_it->datasetid(),
                                location.c_str());

            rc = test_vssi_dataset_access(location, 0, 0, route_info, true,
                                          dataset_it->datasettype());
            VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                "TC_RESULT = %s ;;; TC_NAME = Dataset_VSSI_Access_"FMTx64,
                                rc ? "FAIL" : "PASS",
                                dataset_it->datasetid());
            rv += rc;
            if(rv) goto exit;

        xml_skipped:
            // perform the same test using the new non-xml routines.
            VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                "Test dataset access2 for dataset %s, ID "FMTx64" with location \n\t%s.",
                                dataset_it->datasetname().c_str(),
                                dataset_it->datasetid(),
                                location.c_str());

            if (rc == 0) {
                rc = test_vssi_dataset_access(location, user_id, dataset_id,
                                              route_info, false,
                                              dataset_it->datasettype());
            }
            VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                "TC_RESULT = %s ;;; TC_NAME = Dataset_VSSI_Access2_"FMTx64,
                                rc ? "FAIL" : "PASS",
                                dataset_it->datasetid());
            rv += rc;
            if(rv) goto exit;
            VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                "Test session recognition.");
            rc = test_vss_session_recognition(session, location.c_str(),
                                              user_id, dataset_id, route_info, false);
            VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                "TC_RESULT = %s ;;; TC_NAME = Session_Recognition_"FMTx64,
                                rc ? "FAIL" : "PASS",
                                dataset_it->datasetid());
            rv += rc;
            if(rv) goto exit;
            tests_done++;
            
            delete_route_info(route_info);

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
        // Do datase HTTP-access tests.
        for(dataset_it = ownedDatasets.begin();
            dataset_it != ownedDatasets.end();
            dataset_it++) {
            bool skip;

            // Only test USER type datasets.
            skip = false;
            switch(dataset_it->datasettype()) {
            case vplex::vsDirectory::USER:
                break;
            case vplex::vsDirectory::CAMERA:
            case vplex::vsDirectory::CLEAR_FI:
            case vplex::vsDirectory::PIM:
            case vplex::vsDirectory::PIM_CONTACTS:
            case vplex::vsDirectory::PIM_EVENTS:
            case vplex::vsDirectory::PIM_NOTES:
            case vplex::vsDirectory::PIM_TASKS:
            case vplex::vsDirectory::PIM_FAVORITES:
            case vplex::vsDirectory::MEDIA:
            case vplex::vsDirectory::MEDIA_METADATA:
            case vplex::vsDirectory::CR_UP:
            case vplex::vsDirectory::CR_DOWN:
            case vplex::vsDirectory::CACHE:
            case vplex::vsDirectory::FS:
            case vplex::vsDirectory::VIRT_DRIVE:
            case vplex::vsDirectory::CLEARFI_MEDIA:
            case vplex::vsDirectory::USER_CONTENT_METADATA:
            case vplex::vsDirectory::SYNCBOX:
            case vplex::vsDirectory::SBM:
            case vplex::vsDirectory::SWM:
            default:
                skip = true;
                break;
            }
            if(skip) {
                continue;
            }


            // Find storage for dataset
            for(storage_it = userStorage.begin();
                storage_it != userStorage.end();
                storage_it++) {
                if(storage_it->storageclusterid() == dataset_it->clusterid()) {
                    break;
                }
            }

            if ( storage_it->storagetype() != 0 ) {
                continue;
            }

            VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                "Test dataset HTTP access for dataset %s, ID "FMTx64" at server %s:%u.",
                                dataset_it->datasetname().c_str(),
                                dataset_it->datasetid(),
                                dataset_it->storageclusterhostname().c_str(),
                                dataset_it->storageclusterport());
            
            rc = test_http_dataset_access(*dataset_it, uid,
                                          session.sessionhandle(),
                                          session.serviceticket());
            VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                "TC_RESULT = %s ;;; TC_NAME = Dataset_HTTP_Access_"FMTx64,
                                rc ? "FAIL" : "PASS",
                                dataset_it->datasetid());
            rv += rc;
            if(rv) goto exit;

            if(one_only) {
                break;
            }
        }
        // Report indeterminate if no datasets exist.
        if(ownedDatasets.empty()) {
            VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                "TC_RESULT = INDETERMINATE ;;; TC_NAME = Dataset_HTTP_Access");
        }
        if(skip_vssi) {
            goto cleanup;
        }

    camera_roll_test:
        // Do CameraRoll feature test
        // Find the CameraRoll datasets.
        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                            "Test CameraRoll feature.");
        {
            string upload_location;
            string download_location;
            u64 upload_dataset_id = 0;
            u64 download_dataset_id = 0;
            
            // Find CameraRoll dataset locations
            for(dataset_it = ownedDatasets.begin();
                dataset_it != ownedDatasets.end();
                dataset_it++) {
                if(dataset_it->datasettype() == vplex::vsDirectory::CR_DOWN) {
                    download_location = dataset_it->datasetlocation();
                    download_dataset_id = dataset_it->datasetid();
                    break;
                }
            }
            for(dataset_it = ownedDatasets.begin();
                dataset_it != ownedDatasets.end();
                dataset_it++) {
                if(dataset_it->datasettype() == vplex::vsDirectory::CR_UP) {
                    upload_location = dataset_it->datasetlocation();
                    upload_dataset_id = dataset_it->datasetid();
                    
                    // Find storage where dataset belongs
                    for(storage_it = userStorage.begin();
                        storage_it != userStorage.end();
                        storage_it++) {
                        if(dataset_it->clusterid() == storage_it->storageclusterid()) {
                            break;
                        }
                    }
                    break;
                }
            }
            
            if(upload_location.empty() || download_location.empty()) {
                VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0, "FAIL: "
                                    "CameraRoll dataset missing. Upload location: {%s}, download location: {%s}.",
                                    upload_location.c_str(), download_location.c_str());
                rc = 1;
            }
            else {
                rc = test_camera_roll(upload_location, download_location,
                                      user_id, upload_dataset_id, download_dataset_id,
                                      route_info, true);
                VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                    "TC_RESULT = %s ;;; TC_NAME = CameraRoll_Feature_xml",
                                    rc ? "FAIL" : "PASS");
                
                
                if((rc = setup_route_access_info(*storage_it, *dataset_it,
                                                 user_id, upload_dataset_id, route_info))) {
                    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                        "setup_route_access_info() - %d", rc);
                    
                    rc = test_camera_roll(upload_location, download_location,
                                          user_id, upload_dataset_id, download_dataset_id,
                                          route_info, false);
                    VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                                        "TC_RESULT = %s ;;; TC_NAME = CameraRoll_Feature_routes",
                                        rc ? "FAIL" : "PASS");
                }
            }
        }
        VPLTRACE_LOG_ALWAYS(TRACE_BVS, 0,
                            "TC_RESULT = %s ;;; TC_NAME = CameraRoll_Feature",
                            rc ? "FAIL" : "PASS");
        rv += rc;

    cleanup:
        // Perform cleanup queries.
        do_cleanup_queries();
        do_vssi_cleanup(vssi_session);
    }
    else {
        rv = 1;
    }

 exit:
    delete_route_info(route_info);

    vsTest_infra_destroy();

    VPLVsDirectory_DestroyProxy(proxy);

    return (rv) ? 1 : 0;
}
