//============================================================================
/// @file
/// Tool to call CCDIGetLocalHttpInfo.
//============================================================================

#include <vplu.h>
#include <ccdi.hpp>
#include <ccdi_client.hpp>
#include <log.h>

#include "LoginCCD.hpp"

#include <iostream>

using namespace std;

class CallGetLocalHttpInfo : public LoginCCD {
public:
    int getLocalHttpInfo();
};

int CallGetLocalHttpInfo::getLocalHttpInfo()
{
    int rv = 0;

    ccd::GetLocalHttpInfoInput req;
    ccd::GetLocalHttpInfoOutput res;
    req.set_user_id(userId);
    req.set_service(ccd::LOCAL_HTTP_SERVICE_REMOTE_FILES);
    rv = CCDIGetLocalHttpInfo(req, res);
    if (rv != CCD_OK) {
        LOG_ERROR("CCDIGetLocalHttpInfo failed: %d", rv);
        goto end;
    }

    cout << res.DebugString() << endl;

 end:
    return rv;
}

static void print_usage_and_exit(const char *progname)
{
    cerr << "Usage: " << progname << " testInstanceNum username password" << endl;
    exit(0);
}

class Args {
public:
    Args(int argc, char *argv[]);
    int testInstanceNum;
    string username;
    string password;
};

Args::Args(int argc, char *argv[])
{
    if (argc < 4) {
        print_usage_and_exit(argv[0]);
    }

    testInstanceNum = atoi(argv[1]);
    username = argv[2];
    password = argv[3];
}

int main(int argc, char *argv[])
{
    LOGInit("CallGetLocalHttpInfo", NULL);
    LOGSetMax(0);
    LOG_DISABLE_LEVEL(LOG_LEVEL_DEBUG);

    VPL_Init();

    Args args(argc, argv);

    LOG_INFO("testInstanceNum %d", args.testInstanceNum);
    CCDIClient_SetTestInstanceNum(args.testInstanceNum);

    CallGetLocalHttpInfo ccd;
    int rv;
    ccd.dontLogoutOnExit();
    rv = ccd.login(args.username, args.password);
    if (rv) goto end;
    rv = ccd.getLocalHttpInfo();
    if (rv) goto end;

 end:
    return rv;
}
