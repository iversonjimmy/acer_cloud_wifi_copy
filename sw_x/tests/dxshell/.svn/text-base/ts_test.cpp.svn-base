/*
 *  Copyright 2013 Acer Cloud Technology Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF
 *  ACER CLOUD TECHNOLOGY INC.
 *
 */


#include <vpl_net.h>
#include "vplex_trace.h"
#include "vplex_assert.h"

#include "vpl_conv.h"
#include "vpl_thread.h"
#include "vpl_th.h"
#include "vplex_math.h"
#include "ccd_utils.hpp"
#include "common_utils.hpp"
#include "dx_common.h"
#include "ccdconfig.hpp"
#include "ts_test.hpp"
#include "ts_test_network_test.hpp"
#include "ts_test_configure_router.hpp"
#include "echoSvc.hpp"
#include "ts_ext_client.hpp"
#include "gvm_errors.h"

#if defined(WIN32)
#include <getopt_win.h>
#else
#include <getopt.h>
#endif

#include <list>
#include <string>
#include <sstream>

#include "log.h"

using namespace std;
using namespace EchoSvc;


// Logs with LOGLevel enum value less than logEnableLevel will be disabled in the test application.
// i.e.; Decrementing logEnableLevel increases verbosity of test output.
//       Incrementing logEnableLevel decreases verbosity of test output.
// Examples: logEnableLevel == LOG_LEVEL_INFO
//                             enabled:  INFO, WARN, ERROR, CRITICAL, ALWAYS
//                             disabled: DEBUG, TRACE
//           logEnableLevel == LOG_LEVEL_ERROR
//                             enabled:  ERROR, CRITICAL, ALWAYS
//                             disabled: DEBUG, TRACE, INFO, WARN
// Multiple -v options can decrease logEnableLevel until it reaches 0 (i.e. LOG_LEVEL_TRACE).
// Multiple -q options can increase logEnableLevel until it reaches minVerbosityLevel.
// Example: minVerbosityLevel = LOG_LEVEL_ERROR means TRACE, DEBUG, INFO, AND WARN logs can
//          be enabled/disabled, but ERROR, CRITICAL, and ALWAYS logs can not be disabled.
static int logEnableLevel = (int) LOG_LEVEL_INFO;
static int minVerbosityLevel = (int) LOG_LEVEL_ERROR; 

#define CHECK_AND_PRINT_EXPECTED_TO_FAIL(testsuite, subtestname, result, bug) { \
        if (result < 0) {                                       \
            LOG_ERROR("%s fail rv (%d)", subtestname, result);     \
            LOG_ALWAYS("TC_RESULT=%s ;;; TC_NAME=TsTest_%s_%s(BUG %s)", \
                       (result == 0)? "PASS":"EXPECTED_TO_FAIL", testsuite, subtestname, bug); \
        } else {                                                \
            LOG_ALWAYS("TC_RESULT=%s ;;; TC_NAME=TsTest_%s_%s(BUG %s)", \
                       (result == 0)? "PASS":"EXPECTED_TO_FAIL", testsuite, subtestname, bug); \
        }                                                       \
}

#define CHECK_AND_PRINT_RESULT(testsuite, subtestname, result) {              \
        if (result < 0) {                                       \
            LOG_ERROR("%s fail rv (%d)", subtestname, result);     \
            LOG_ALWAYS("TC_RESULT=%s ;;; TC_NAME=TsTest_%s_%s", (result == 0)? "PASS":"FAIL", testsuite, subtestname); \
            goto exit;                                          \
        } else {                                                \
            LOG_ALWAYS("TC_RESULT=%s ;;; TC_NAME=TsTest_%s_%s", (result == 0)? "PASS":"FAIL", testsuite, subtestname); \
        }                                                       \
}

#define START_CCD(tc_name, rc) \
    do { \
        const char *testStr = "StartCCD"; \
        const char *testArg[] = { testStr }; \
        rv = start_ccd(1, testArg); \
        CHECK_AND_PRINT_RESULT(tc_name, testStr, rc); \
    } while (0)

#define SET_TARGET_MACHINE(tc_name, alias, rc) \
    do { \
        const char *testStr = "SetTargetMachine"; \
        rc = set_target_machine(alias); \
        if (rc == VPL_ERR_FAIL) { \
            CHECK_AND_PRINT_RESULT(tc_name, testStr, rc); \
        } \
    } while (0)

#define SET_DOMAIN(domain, tc_name, rc) \
    do { \
        const char *testStr = "SetDomain"; \
        const char *testArg[] = { testStr, domain }; \
        rc = set_domain(2, testArg); \
        CHECK_AND_PRINT_RESULT(tc_name, testStr, rc); \
    } while (0)

#define SET_CCDCONFIG(method, config, value, tc_name, rc) \
    BEGIN_MULTI_STATEMENT_MACRO \
    { \
        const char *testStr = "CCDConfig"; \
        const char *testArg[] = { testStr, method, config, value }; \
        LOG_INFO("%s ccdconfig %s to %s", method, config, value); \
        rc = dispatch_ccdconfig_cmd(4, testArg); \
        CHECK_AND_PRINT_RESULT(tc_name, "Set"config, rc); \
    } \
    END_MULTI_STATEMENT_MACRO

#define START_CLOUDPC(user_name, pwd, tc_name, retry, rc) \
    do { \
        const char *testArgs[] = { "StartCloudPC", user_name, pwd }; \
        rc = start_cloudpc(3, testArgs); \
        if (rc < 0 && retry) { \
            /**/ \
            rc = start_cloudpc(3, testArgs); \
        } \
        CHECK_AND_PRINT_RESULT(tc_name, "StartCloudPC", rc); \
    } while (0)

#define START_CLIENT(user_name, pwd, tc_name, retry, rc) \
    do { \
        const char *testArgs[] = { "StartClient", user_name, pwd }; \
        rc = start_client(3, testArgs); \
        if (rc < 0 && retry) { \
            /**/ \
            rc = start_client(3, testArgs); \
        } \
        CHECK_AND_PRINT_RESULT(tc_name, "StartClient", rc); \
    } while (0)

#define QUERY_TARGET_OSVERSION(osversion, tc_name, rc) \
    do { \
        rc = get_target_osversion(osversion); \
        CHECK_AND_PRINT_RESULT(tc_name, "Get_Target_OSVersion", rc); \
    } while (0)

#define CHECK_LINK_REMOTE_AGENT(alias, tc_name, rc) \
    do { \
        rc = check_link_dx_remote_agent(alias); \
        CHECK_AND_PRINT_RESULT(tc_name, "CheckDxRemoteAgent", rc); \
    } while (0)

#define SET_POWER_FGMODE(app_id, foreground) \
    BEGIN_MULTI_STATEMENT_MACRO \
    { \
        const char *testStr1 = "Power"; \
        const char *testStr2 = "FgMode"; \
        const char *testStr3 = app_id; \
        const char *testStr4 = "5"; \
        const char *testStr5 = foreground; \
        const char *testArg[] = { testStr1, testStr2, testStr3, testStr4, testStr5 }; \
        LOG_INFO("Set %s to foreground(%s)", app_id, foreground); \
        rv = power_dispatch(5, testArg); \
        const char *testStr = "SetPowerFgMode"; \
        CHECK_AND_PRINT_RESULT(ECHOTEST_STR, testStr, rv); \
    } \
    END_MULTI_STATEMENT_MACRO

const char* TSTEST_STR = "TsTest";
const char* TSTEST_MAX_TOTAL_LOG_SIZE = "0"; // "1073741824";  // 1024*1024*1024

static const u8 echoSvcTestCmdMsgId[] = ECHO_SVC_TEST_CMD_MSG_ID_STR;
static const char* ECHOTEST_STR = "EchoTest";

struct DataToEcho
{
    char *data;
    size_t len;
    size_t lenCompared;

    DataToEcho () {
        data = 0;
        len = 0;
        lenCompared = 0;
    }

    ~DataToEcho () {
        if (data) {
            delete[] data;
        }
    }
};

// struct to pass to ts_read and ts_write threads
struct EchoClientRWThreadArgs
{
    TSIOHandle_t io_handle;
    VPLMutex_t *mutex;
    VPLCond_t *cond;
    list<DataToEcho> sent_bufs;
    u64 numQueuedBytes;
    u32 clientInstanceId;
    bool isdone;
    bool isReadActive;
    bool isWriterWaiting;
    bool isTunnelClosed;
};

struct EchoClientThreadArgs
{
    TSOpenParms_t tsOpenParms;
    u32 clientInstanceId;
};



static std::map<std::string, subcmd_fn> g_tstest_cmds;

static const u32 sentQueueWaitLimit  = 2*1024*1024;
static const u32 sentQueueContLimit  = 1*1024*1024;

static const size_t chunkSize_default = 1024;
static const u32 numChunks_default = 1;
static const u32 nTestIterations_default = 1;
static const u32 nClients_default = 1;
static const int client_write_delay_default = 0; // milliseconds
static const int server_read_delay_default = 0; // milliseconds

static size_t chunkSize = chunkSize_default;
static u32 numChunks = numChunks_default;
static u32 nTestIterations = nTestIterations_default; // number of times to run the test
static u32 nClients = nClients_default; // number of echo service client instance threads
static const u32 maxClients = 10;
static int client_write_delay = client_write_delay_default; // milliseconds
static int server_read_delay = server_read_delay_default; // milliseconds

static bool chunkSizeSetByArg = false;
static bool numChunksSetByArg = false;
static bool clientWriteDelaySetByArg = false;

static bool findStorageNode = false;

static bool setTestNetworkEnvArg = false;
static bool initDone = false;

static u64 clientDeviceId = 0;
static u64 serverDeviceId = 0;
static u64 requestedServerDeviceId = 0;

static const char* domain_default = "pc-int.igware.net";
static const char* domain = domain_default;
static const char* username = 0;
static const char* password_default = "password";
static const char* password = password_default;

static s32 testId;

static void usage(int argc, const char *argv[]) 
{
    printf("\n");
    printf ("Usage: dxshell TsTest %s [options] -u <username> [options]\n", argv[0]);
    printf("\n");
    printf("    Client and cloudPC ccds are started and the cloudPC is used as\n");
    printf("    the echo service server unless -d or -r is specified.\n");
    printf("\n");
    printf("    The only required argument is username.  Other arguments can be specified\n");
    printf("    to override defaults.\n");
    printf("\n");
    printf("    Some tests other than the default TunnelAndConnectionWorking test\n");
    printf("    have specific message size and count requirements.\n");
    printf("    Incompatible xfer_cnt or xfer_size arguments will be overridden and logged.\n");
    printf("\n");
    printf("Options:\n");
    printf(" -v --verbose                 Raise verbosity one level. Repeat up to %d times (more has no effect).\n", logEnableLevel);
    printf(" -q --quiet                   Lower verbosity one level. Repeat up to %d times (more has no effect).\n",
                                                                                minVerbosityLevel - logEnableLevel);
    printf(" -c --xfer_cnt <#>            Number of data blocks to write, read, and check. Default %u.\n", numChunks_default);
    printf(" -s --xfer_size  <#>          Size of data blocks. Default %d.\n", chunkSize_default);
    printf(" -w --client_write_delay <#>  Extra msecs of delay between client writes. Default %d\n", client_write_delay_default);
    printf(" -y --server_read_delay <#>   Extra msecs of delay between server reads. Default %d\n", server_read_delay_default);
    printf(" -d --device <#>              Starts client ccd and opens echo service on already running ccd\n");
    printf("                              specified by deviceId. Default is to find an active storage node.\n");
    printf(" -r --running                 Starts client ccd and finds an already running storage node\n");
    printf("                              to use as the echo service server.\n");
    printf(" -o --domain                  default is \"%s\".\n", domain_default);
    printf(" -u --username                Required argument.\n");
    printf(" -p --password                default is \"password\".\n");
    printf(" -i --iterations <#>          The number of times to run the test. Default %u\n", nTestIterations_default);
    printf(" -n --num_clients <#>         Number of echo service virtual tunnel client threads simultaneously\n");
    printf("                              running the same test. Default %u, max %u.\n", nClients_default, maxClients);
    printf(" -t --testId <EchoServiceTestId>\n");
    printf("       EchoServiceTestIds:\n");
    printf("           TunnelAndConnectionWorking = 0 (this is the default)\n");
    printf("           ClientCloseAfterWrite = 1\n");
    printf("           ServiceCloseAfterWrite = 2\n");
    printf("           ClientCloseWhileServiceWriting = 3\n");
    printf("           ServiceCloseWhileClientWritng = 4\n");
    printf("           ServiceCloseWith_TS_CLOSE = 5\n");
    printf("           LostConnection = 6\n");
    printf("           ServiceIgnoreIncoming = 7\n");
    printf("           ServiceDoNothing = 8\n");
    printf("           ServiceDontEcho = 9\n");
    printf(" -e --set-ts2testNetworkEnv   Set ts2TestNetworkEnv in ccd.conf.\n");
    printf("\n");
}

static int parse_args(int argc, const char* argv[]) 
{
    int rv = 0;

    static struct option long_options[] = {
        {"verbose", no_argument, 0, 'v'},
        {"quiet", no_argument, 0, 'q'},
        {"xfer_cnt", required_argument, 0, 'c'},
        {"xfer_size", required_argument, 0, 's'},
        {"client_write_delay", required_argument, 0, 'w'},
        {"server_read_delay", required_argument, 0, 'y'},
        {"device", required_argument, 0, 'd'},
        {"running", no_argument, 0, 'r'},
        {"domain", required_argument, 0, 'o'},
        {"username", required_argument, 0, 'u'},
        {"password", required_argument, 0, 'p'},
        {"iterations", required_argument, 0, 'i'},
        {"num_clients", required_argument, 0, 'n'},
        {"testId", required_argument, 0, 't'},
        {"set-ts2testNetworkEnv", no_argument, 0, 'e'},
        {0,0,0,0}
    };

    while (true) {
        int option_index = 0;

        int option = getopt_long(argc, (char * const *)argv, "vqc:s:w:y:d:ro:u:p:i:n:t:e", long_options, &option_index);

        if (option == -1)
            break;

        switch(option) {
        case 'v':
            if (logEnableLevel > 0) {
                --logEnableLevel;
            }
            break;
        case 'q':
            if (logEnableLevel < minVerbosityLevel) {
                ++logEnableLevel;
            }
            break;
        case 'c':   
            numChunks = strtoul(optarg, NULL, 0);
            numChunksSetByArg = true;
            break;        
        case 's':   
            chunkSize = strtoul(optarg, NULL, 0);
            chunkSizeSetByArg = true;
            break;        
        case 'w':
            client_write_delay = atoi(optarg);
            clientWriteDelaySetByArg = true;
            break;
        case 'y':
            server_read_delay = atoi(optarg);
            break;
        case 'd':
            requestedServerDeviceId = strtoull(optarg, NULL, 0);
            break;
        case 'r':
            findStorageNode = true;;
            break;
        case 'o':
            domain = optarg;
            break;
        case 'u':
            username = optarg;
            break;
        case 'p':
            password = optarg;
            break;
        case 'i': 
            nTestIterations = strtoul(optarg, NULL, 0);
            break;        
        case 'n': 
            nClients = strtoul(optarg, NULL, 0);
            if ( nClients > maxClients ) {
                rv = -1;
            } 
            break;        
        case 't':
            {
                s32 testIdArg = atoi(optarg);
                if (testIdArg < 0 || testIdArg > MaxEchoServiceTestId) {
                    rv = -1;
                } else {
                    testId = (s32)testIdArg;
                }
            }
            break;
        case 'e':
            setTestNetworkEnvArg = true;
            break;
        default:
            rv = -1;
            break;
        }
    }

    if (rv == 0) {

        if (!username) {
            rv = -1;
        }
        else if (findStorageNode && requestedServerDeviceId) {
            rv = -1;
        }
    }

    if (rv != 0) {
        usage (argc, argv);
    }

    return rv;
}

bool isInitDone()
{
    return initDone;
}


static void setCcdTestInstanceNum(int id)
{
    testInstanceNum = id;    // DXShell global variable need to set to the testInstanceNum desired.
    CCDIClient_SetTestInstanceNum(id);
}


// if !specifiedDeviceId, returns 1st found storage node deviceId or 0 at *deviceId
// else returns specifiedDeviceId or 0 at *deviceId
// switches to CcdTestInstanceNum if not 0
static int find_linked_storage_node(int  CcdTestInstanceNum,
                                    u64  userId,
                                    u64  specifiedDeviceId,
                                    bool verbose,
                                    u64 *deviceId)
{
    int rv = VPL_OK;

    *deviceId = 0;

    ccd::ListLinkedDevicesOutput listSnOut;
    ccd::ListLinkedDevicesInput request;
    request.set_user_id(userId);
    request.set_storage_nodes_only(true);

    if(CcdTestInstanceNum > 0)
         setCcdTestInstanceNum(CcdTestInstanceNum);

    // Try contacting infra for the most updated information first
    rv = CCDIListLinkedDevices(request, listSnOut);
    if (rv != 0) {
        LOG_ERROR("CCDIListLinkedDevice for user("FMTu64") failed %d", userId, rv);

        // Fall back to cache if cannot reach server
        LOG_ALWAYS("Retry with only_use_cache option");
        request.set_only_use_cache(true);
        rv = CCDIListLinkedDevices(request, listSnOut);
        if (rv != 0) {
            LOG_ERROR("CCDIListLinkedDevice for user("FMTu64") failed %d", userId, rv);
        }
    } 
   
    if (rv == 0) {
        for (int i = 0; i < listSnOut.devices_size(); i++) {
            const ccd::LinkedDeviceInfo& curr = listSnOut.devices(i);
            u64 snid = curr.device_id();
            if (curr.is_storage_node()) {
                if (specifiedDeviceId) {
                    if (specifiedDeviceId == snid) {
                        *deviceId = snid;
                    }
                }
                else if (!*deviceId && !curr.connection_status().updating() &&
                         ((int)curr.connection_status().state() == ccd::DEVICE_CONNECTION_ONLINE)) {
                    *deviceId = snid;
                }
                if (verbose) {
                    LOG_ALWAYS("user("FMTu64") device("FMTu64") is storage node", userId, snid);
                }
            } else if (verbose) {
                LOG_ALWAYS("user("FMTu64") device("FMTu64") is not storage node", userId, snid);
            }
        }
    }

    return rv;
}



// switch to the CcdTestInstanceNum and call the ListLinkedDevices to make sure all
// the devices listed in the deviceIDs are ONLINE
static int wait_for_devices_to_be_online(int CcdTestInstanceNum,
                                   const u64 &userId,
                                   const std::vector<u64> &deviceIDs,
                                   const int max_retry)
{
    int rv = VPL_OK;
    int ans_retry = 0;
    bool all_linked = false;

    ccd::ListLinkedDevicesOutput listSnOut;
    ccd::ListLinkedDevicesInput request;

    request.set_user_id(userId);
    request.set_only_use_cache(true);

    if(CcdTestInstanceNum > 0)
         setCcdTestInstanceNum(CcdTestInstanceNum);

    while (!all_linked && ans_retry++ < max_retry) {
        LOG_ALWAYS("Wait the current device status to be ONLINE."
                   " ccd ID = %d, ans_retry = %d",
                   CcdTestInstanceNum, ans_retry);

        rv = CCDIListLinkedDevices(request, listSnOut);
        if (rv != 0) {
            LOG_ERROR("Fail to list devices:%d", rv);
            return rv;
        }
        // scan all devices and make sure it's updated
        all_linked = true;
        for (unsigned int deviceIdx = 0; deviceIdx < deviceIDs.size(); deviceIdx++) {
            // Compare device with the devices listed in ListSnOut and make sure it's online
            bool match = false;
            for (int i = 0; i < listSnOut.devices_size(); i++) {
                const ccd::LinkedDeviceInfo& curr = listSnOut.devices(i);
                LOG_ALWAYS("Device Id "FMTu64" Status %d, updating %s",
                           curr.device_id(),
                           (int)curr.connection_status().state(),
                           curr.connection_status().updating()? "TRUE" : "FALSE");

                if (curr.device_id() == deviceIDs[deviceIdx] &&
                    !curr.connection_status().updating() &&
                    ((int)curr.connection_status().state() == ccd::DEVICE_CONNECTION_ONLINE)) {
                    match = true;
                    break;
                }
            }
            // Device not found or online. Break and wait for next round
            if (match == false) {
                all_linked = false;
                break;
            }
        }
        VPLThread_Sleep(VPLTime_FromMillisec(1000));
    }
    return all_linked? VPL_OK : VPL_ERR_FAIL;
}


static int check_link_dx_remote_agent(std::string &alias)
{
    int rc = VPL_OK;
    int retry = 0;
    TargetDevice *target = NULL;

    target = getTargetDevice();
    if (target == NULL) {
        LOG_ERROR("target is NULL, return!");
        return rc;
    }

    do {
        rc = target->checkRemoteAgent();
        if (rc != VPL_OK) {
            LOG_ERROR("checkRemoteAgent[%s] failed! %d. Check communication with dx_remote_agent!", alias.c_str(), rc);
        }
        else {
            LOG_ALWAYS("DxRemoteAgent[%s] is connected!", alias.c_str());
            break;
        }
        VPLThread_Sleep(VPLTIME_FROM_MILLISEC(1000));
    } while (retry++ < 15);

    if (target != NULL)
        delete target;
    return rc;
}

static int get_target_osversion(std::string &osversion)
{
    int rc = VPL_OK;
    bool retry = true;
    TargetDevice *target = getTargetDevice();

    osversion = target->getOsVersion();
    if(osversion.empty() && retry) {
        VPLThread_Sleep(VPLTIME_FROM_SEC(2));
        osversion = target->getOsVersion();
    }

    if (target != NULL) {
        delete target;
        target = NULL;
    }

    if(osversion.empty()) {
        rc = -1;
        LOG_ERROR("could not get osversion info from target device");
    }

    return rc;
}




// echoTestSetup_local() does setCcdInstanceId(clientPCId) before successful exit

static int echoTestSetup_local(TSOpenParms_t& tsOpenParms)
{
    int rv = VPL_OK;
    int cloudPCId = 1;
    int clientPCId = 2;
    bool testLaunchedCloudPC = false;
    u64 userId = 0;
    u32 i;

    LOG_ALWAYS("Starting execution of echoTestSetup_local");

    if (!findStorageNode && !requestedServerDeviceId) {

        // start cloudpc ccd and get deviceId for TS_Open

        LOG_ALWAYS("\n\n==== Launching Cloud PC CCD ====");
        setCcdTestInstanceNum(cloudPCId);

        // hard stop for all ccds
        const char *testArg[] = { "StopCCD" };
        stop_ccd_hard(1, testArg);

        SET_DOMAIN(domain, ECHOTEST_STR, rv);
        SET_CCDCONFIG("Set", "ts2TestNetworkEnv", setTestNetworkEnvArg ? "1" : "0", ECHOTEST_STR, rv);
        START_CCD(ECHOTEST_STR, rv);
        START_CLOUDPC(username, password, ECHOTEST_STR, true, rv);
        if (rv) {
            goto exit;
        }

        rv = getDeviceId(&serverDeviceId);
        if (rv != 0) {
            LOG_ERROR("Fail to get device id: %d", rv);
            goto exit;
        }
        testLaunchedCloudPC = true;
    }

    // start client ccd and get userId for TS_Open

    LOG_ALWAYS("\n\n==== Launching Client CCD ====");
    setCcdTestInstanceNum(clientPCId);

    // hard stop for all ccds
    {
        const char *testArg[] = { "StopCCD" };
        stop_ccd_hard(1, testArg);
    }

    SET_DOMAIN(domain, ECHOTEST_STR, rv);
    SET_CCDCONFIG("Set", "ts2TestNetworkEnv", setTestNetworkEnvArg ? "1" : "0", ECHOTEST_STR, rv);
    START_CCD(ECHOTEST_STR, rv);
    START_CLIENT(username, password, ECHOTEST_STR, true, rv);
    if (rv) {
        goto exit;
    }

    rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Fail to get user id:%d", rv);
        goto exit;
    }

    rv = getDeviceId(&clientDeviceId);
    if (rv != 0) {
        LOG_ERROR("Fail to get device id: %d", rv);
        goto exit;
    }

    // make sure both cloudpc/client has the device linked info updated
    LOG_ALWAYS("\n\n== Checking CloudPC and Client device link status ==");
    {
        std::vector<u64> deviceIds;

        deviceIds.push_back(clientDeviceId);

        if (findStorageNode || requestedServerDeviceId) {
            u32 maxTrys = 120;
            for (i = 0; i < maxTrys; ++i) {
                rv = find_linked_storage_node(clientPCId, userId,
                                              requestedServerDeviceId,
                                              ((i%10)==0), &serverDeviceId);
                if (rv) {
                    goto exit;
                }
                if (serverDeviceId) {
                    break;
                }
                VPLThread_Sleep(VPLTime_FromMillisec(1000));
            }
            if (!serverDeviceId) {
                LOG_ERROR("Did not find storage node for user "FMTu64, userId);
                rv = -1;
                goto exit;
            }
        }
        deviceIds.push_back(serverDeviceId);
        rv = wait_for_devices_to_be_online(clientPCId, userId, deviceIds, 20);
        CHECK_AND_PRINT_RESULT(ECHOTEST_STR, "CheckClientDeviceLinkStatus", rv);
        if (testLaunchedCloudPC) { // already checked if did find_linked_storage_node()
            rv = wait_for_devices_to_be_online(cloudPCId, userId, deviceIds, 20);
            CHECK_AND_PRINT_RESULT(ECHOTEST_STR, "CheckCloudPCDeviceLinkStatus", rv);
        }
    }

    LOG_ALWAYS ("\n\n== Connecting to echo service on storage node with deviceId "FMTu64" ==\n\n", serverDeviceId);

    // set parameters to be passed to TS_Open
    tsOpenParms.user_id = userId;
    tsOpenParms.device_id = serverDeviceId;
    tsOpenParms.service_name = "echo";
    tsOpenParms.flags = 0;
    tsOpenParms.timeout = 0;

    // caller expects this was done before successful exit
    setCcdTestInstanceNum(clientPCId);

exit:
    return rv;
}


// echoTestSetup_useRemoteAgent() does SET_TARGET_MACHINE("MD") before successful exit

static int echoTestSetup_useRemoteAgent(TSOpenParms_t& tsOpenParms)
{
    int rv = VPL_OK;
    int cloudPCId = 1;
    int clientPCId = 2;
    bool testLaunchedCloudPC = false;
    u64 userId = 0;
    u32 i;
    std::string clientPCOSVersion;

    LOG_ALWAYS("Starting execution of echoTestSetup_useRemoteAgent");

    if (!findStorageNode && !requestedServerDeviceId) {

        // start cloudpc ccd and get deviceId for TS_Open

        LOG_ALWAYS("\n\n==== Launching Cloud PC CCD ====");
        SET_TARGET_MACHINE(ECHOTEST_STR, "CloudPC", rv);
        if (rv < 0) {
            setCcdTestInstanceNum(cloudPCId);
            SET_DOMAIN(domain, ECHOTEST_STR, rv);
        }

        {
            std::string alias = "CloudPC";
            CHECK_LINK_REMOTE_AGENT(alias, ECHOTEST_STR, rv);
        }

        SET_CCDCONFIG("Set", "maxTotalLogSize", TSTEST_MAX_TOTAL_LOG_SIZE, ECHOTEST_STR, rv);
        SET_CCDCONFIG("Set", "ts2TestNetworkEnv", setTestNetworkEnvArg ? "1" : "0", ECHOTEST_STR, rv);

        START_CCD(ECHOTEST_STR, rv);
        START_CLOUDPC(username, password, ECHOTEST_STR, true, rv);
        if (rv) {
            goto exit;
        }

        SET_POWER_FGMODE("dx_remote_agent_CloudPC", "1");

        rv = getDeviceId(&serverDeviceId);
        if (rv != 0) {
            LOG_ERROR("Fail to get device id: %d", rv);
            goto exit;
        }
        testLaunchedCloudPC = true;
    }

    // start client ccd and get userId for TS_Open

    LOG_ALWAYS("\n\n==== Launching Client CCD ====");
    SET_TARGET_MACHINE(ECHOTEST_STR, "MD", rv);
    if (rv < 0) {
        setCcdTestInstanceNum(clientPCId);
        SET_DOMAIN(domain, ECHOTEST_STR, rv);
    }

    {
        std::string alias = "MD";
        CHECK_LINK_REMOTE_AGENT(alias, ECHOTEST_STR, rv);
    }

    {
        QUERY_TARGET_OSVERSION(clientPCOSVersion, ECHOTEST_STR, rv);
    }

    SET_CCDCONFIG("Set", "maxTotalLogSize", TSTEST_MAX_TOTAL_LOG_SIZE, ECHOTEST_STR, rv);
    SET_CCDCONFIG("Set", "ts2TestNetworkEnv", setTestNetworkEnvArg ? "1" : "0", ECHOTEST_STR, rv);

    START_CCD(ECHOTEST_STR, rv);

    {
        const char *testStr1 = "Power";
        const char *testStr2 = "FgMode";
        const char *testStr3 = "dx_remote_agent";
        const char *testStr4 = "5";
        const char *testStr5 = "1";
        const char *testArg[] = { testStr1, testStr2, testStr3, testStr4, testStr5 };
        rv = power_dispatch(5, testArg);
        const char *testStr = "UpdateAppState";
        CHECK_AND_PRINT_RESULT(ECHOTEST_STR, testStr, rv);
    }

    START_CLIENT(username, password, ECHOTEST_STR, true, rv);
    if (rv) {
        goto exit;
    }

    SET_POWER_FGMODE("dx_remote_agent_MD", "1");

    rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Fail to get user id:%d", rv);
        goto exit;
    }

    rv = getDeviceId(&clientDeviceId);
    if (rv != 0) {
        LOG_ERROR("Fail to get device id: %d", rv);
        goto exit;
    }

    // make sure both cloudpc/client has the device linked info updated
    LOG_ALWAYS("\n\n== Checking cloudpc and Client device link status ==");
    {
        std::vector<u64> deviceIds;

        deviceIds.push_back(clientDeviceId);

        if (findStorageNode || requestedServerDeviceId) {
            u32 maxTrys = 120;
            for (i = 0; i < maxTrys; ++i) {
                rv = find_linked_storage_node(-1, userId,
                                              requestedServerDeviceId,
                                              ((i%10)==0), &serverDeviceId);
                if (rv) {
                    goto exit;
                }
                if (serverDeviceId) {
                    break;
                }
                VPLThread_Sleep(VPLTime_FromMillisec(1000));
            }
            if (!serverDeviceId) {
                LOG_ERROR("Did not fine storage node for user "FMTu64, userId);
                rv = -1;
                goto exit;
            }
        }
        deviceIds.push_back(serverDeviceId);

        rv = wait_for_devices_to_be_online(-1, userId, deviceIds, 20);
        CHECK_AND_PRINT_RESULT(ECHOTEST_STR, "CheckClientDeviceLinkStatus", rv);

        if (testLaunchedCloudPC) { // already checked if did find_linked_storage_node()
            SET_TARGET_MACHINE(ECHOTEST_STR, "CloudPC", rv);
            if (rv < 0) {
                setCcdTestInstanceNum(cloudPCId);
            }
            rv = wait_for_devices_to_be_online(-1, userId, deviceIds, 20);
            CHECK_AND_PRINT_RESULT(ECHOTEST_STR, "CheckCloudPCDeviceLinkStatus", rv);
        }
    }

    LOG_ALWAYS ("\n\n== Connecting to echo service on storage node with deviceId "FMTu64" ==\n\n", serverDeviceId);

    // set parameters to be passed to TS_Open
    tsOpenParms.user_id = userId;
    tsOpenParms.device_id = serverDeviceId;
    tsOpenParms.service_name = "echo";
    tsOpenParms.flags = 0;
    tsOpenParms.timeout = 0;

    // caller expects this was done before successful exit
    SET_TARGET_MACHINE(ECHOTEST_STR, "MD", rv);
    if (rv < 0) {
        setCcdTestInstanceNum(clientPCId);
    }

exit:
    return rv;
}


static int echoTestMain(int argc, const char* argv[], bool useRemoteAgent)
{
    int err = 0;
    string error_msg;
    TSOpenParms_t tsOpenParms;
    TargetDevice *target = NULL;

    // Command line arguments
    if (parse_args(argc, argv) != 0) {
        err = 1;
        goto exit;
    }

    // On entry log level set by setDebugLevel is g_defaultLogLevel which is LOG_LEVEL_ERROR.
    // At least SET_DOMAIN and possibly other utilities also set log level to g_defaultLogLevel.
    // So it doesn't do any good to call setDebugLevel()before those utilites.
    // Could change g_defaultLogLevel, but will attempt not to do that.

    switch (testId) {
        case ServiceCloseAfterWrite:
            if (numChunks != 2) {
                numChunks = 2;
                LOG_ALWAYS("Setting xfer_cnt to 2 for this test.");
            }
            break;
        case ClientCloseWhileServiceWriting:
            numChunks = 1024;
            chunkSize = 1024;
            if (numChunks * chunkSize < EchoMinBytesBeforeClientCloseWhileSvcWriting) {
                numChunks = (EchoMinBytesBeforeClientCloseWhileSvcWriting/chunkSize) * 16;
            }
            LOG_ALWAYS("Setting xfer_cnt to %u and xfer_size to 1024 for this test.", numChunks);
            break;
        case ServiceCloseWhileClientWritng:
            if (numChunks < 1000) { 
                numChunks = 1000;
                LOG_ALWAYS("Setting xfer_cnt to 3 for this test.");
            }
            break;
        case LostConnection:
            if (!numChunksSetByArg) {
                numChunks = 100;
                LOG_ALWAYS("Setting xfer_cnt to %u for this test.", numChunks);
            }
            if (!clientWriteDelaySetByArg) {
                client_write_delay = 50;
                LOG_ALWAYS("Setting client_write_delay to %u millisec for this test.", client_write_delay);
            }
            break;
        case ServiceIgnoreIncoming:
            if (numChunks < 10000) {
                numChunks = 10000;
                LOG_ALWAYS("Setting xfer_cnt to minimum of %u for this test.", numChunks);
            }
            break;
        default:
            break;
    };


    LOG_ALWAYS("Size per data block is  %d", chunkSize);
    LOG_ALWAYS("Number of data blocks to write is  %u", numChunks);
    LOG_ALWAYS("Number of times to run the test is  %u", nTestIterations);
    LOG_ALWAYS("Number of clients is  %u", nClients);
    LOG_ALWAYS("logEnableLevel is  %u", logEnableLevel);
    LOG_ALWAYS("client_write_delay is %u ms", client_write_delay);
    LOG_ALWAYS("server_read_delay is %u ms", server_read_delay);
    LOG_ALWAYS("TestId is  %d", testId);


    // hard stop for all ccds
    {
        const char *testArg[] = { "StopCCD" };
        stop_ccd_hard(1, testArg);
    }

    if (useRemoteAgent) {
        err = echoTestSetup_useRemoteAgent(tsOpenParms);
    }
    else {
        err = echoTestSetup_local(tsOpenParms);
    }

    if (err) {
        LOG_ERROR("%s returned %d", (useRemoteAgent ? 
                  "echoTestSetup_useRemoteAgent" : "echoTestSetup_local"), err);
        goto exit;
    }

    initDone = true;

    // On entry, log level is set by setDebugLevel(g_defaultLogLevel) i.e. LOG_LEVEL_ERROR.
    // At least SET_DOMAIN and possibly other utilities also set log level to g_defaultLogLevel.
    // So it doesn't do any good to call setDebugLevel() before those utilities.

    setDebugLevel(logEnableLevel);
    LOG_ALWAYS("setDebugLevel(%d)", logEnableLevel);

    target = getTargetDevice();

    {
        TSTestParameters test(logEnableLevel);
        TSTestResult    result;

        test.tsOpenParms.user_id = tsOpenParms.user_id;
        test.tsOpenParms.device_id = tsOpenParms.device_id;
        test.tsOpenParms.service_name = tsOpenParms.service_name;
        test.tsOpenParms.credentials = tsOpenParms.credentials;
        test.tsOpenParms.flags = tsOpenParms.flags;
        test.tsOpenParms.timeout = tsOpenParms.timeout;

        test.testId = testId;
        test.xfer_cnt = numChunks;
        test.xfer_size = chunkSize;
        test.nTestIterations = nTestIterations;
        test.nClients = nClients;
        test.client_write_delay = client_write_delay;
        test.server_read_delay = server_read_delay;

        err = target->tsTest(test, result);
        if (err) {
            LOG_ALWAYS("target->testTS failed: %d.", err);
            goto exit;
        }
        err = result.return_value;
        if (err) {
            LOG_ERROR("ts test failed: %d:%s", err, result.error_msg.c_str());
            goto exit;
        }
        LOG_ALWAYS("ts test passed.");
    }

exit:

    delete target;
    resetDebugLevel();

    return err;
}


static int echoTest(int argc, const char* argv[])
{
    return echoTestMain(argc, argv, false);
}

static int echoTest2(int argc, const char* argv[])
{
    return echoTestMain(argc, argv, true);
}



static void tstest_init_commands()
{
    g_tstest_cmds["echo"] = echoTest;
    g_tstest_cmds["echo2"] = echoTest2;
    g_tstest_cmds["NetworkTest"] = network_test_main;
    g_tstest_cmds["ConfigureRouter"] = configure_router_main;
}


static int print_tstest_help()
{
    int rv = 0;
    std::map<std::string, subcmd_fn>::iterator it;

    for (it = g_tstest_cmds.begin(); it != g_tstest_cmds.end(); ++it) {
        const char *argv[2];
        argv[0] = (const char *)it->first.c_str();
        argv[1] = "Help";
        it->second(2, argv);
    }

    return rv;
}


int tstest_commands(int argc, const char* argv[])
{
    int rv = 0;

    tstest_init_commands();

    if (argc == 1 || checkHelp(argc, argv)) {
        printf("P2P tunnel test\n");
        print_tstest_help();
        return rv;   
    }

    if (g_tstest_cmds.find(argv[1]) != g_tstest_cmds.end()) {
        rv = g_tstest_cmds[argv[1]](argc-1, &argv[1]);
    } else {
        LOG_ERROR("Command %s %s not supported", argv[0], argv[1]);
        rv = -1;
    }

    return rv;
}


