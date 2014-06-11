/*
 *  Copyright 2014 Acer Cloud Technology Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF
 *  ACER CLOUD TECHNOLOGY INC.
 *
 */

#include "rexe_test.hpp"

#include <ccdi.hpp>
#include <log.h>
#include <vpl_fs.h>
#include <vplu_types.h>
#include <vplex_strings.h>
#include <vplex_http_util.hpp>

#include <string>
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#include "dx_common.h"
#include "ccd_utils.hpp"
#include "common_utils.hpp"
#include "HttpAgent.hpp"
#include "TargetLocalDevice.hpp"

#define RF_HEADER_USERID         "x-ac-userId"
#define RF_HEADER_SESSION_HANDLE "x-ac-sessionHandle"
#define RF_HEADER_SERVICE_TICKET "x-ac-serviceTicket"

static std::string url_encoding(std::string &str_in)
{
    std::string str_out;
    for(u16 ix = 0; ix < str_in.size(); ix++) {
        u8 buf[4];
        memset( buf, 0, 4 );
        if (isalnum( (u8)str_in[ix]) ) {
            buf[0] = str_in[ix];
        } else if ( isspace( (u8)str_in[ix] ) ) {
            buf[0] = '+';
        } else {
            buf[0] = '%';
            buf[1] = toHex( (u8)str_in[ix] >> 4 );
            buf[2] = toHex( (u8)str_in[ix] % 16);
        }
        str_out += (char *)buf;
    }
    return str_out;
}

int rexe_help(int argc, const char* argv[]);

int rexe_help(int argc, const char* argv[])
{
    bool printAll = strcmp(argv[0], "RemoteExecute") == 0 || strcmp(argv[0], "Help") == 0;
    if (printAll || strcmp(argv[0], "Register") == 0) {
        std::cout << "RemoteExecute Register <app_key> <app_name> <absolute_path> <version_number>" << std::endl;
    }
    if (printAll || strcmp(argv[0], "Unregister") == 0) {
        std::cout << "RemoteExecute Unregister <app_key> <app_name>" << std::endl;
    }
    if (printAll || strcmp(argv[0], "ListExecutable") == 0) {
        std::cout << "RemoteExecute ListExecutable <app_key>" << std::endl;
    }
    if (printAll || strcmp(argv[0], "Execute") == 0) {
        std::cout << "RemoteExecute Execute <device_id> <app_key> <app_name> <minimum_version_num> <request_body>" << std::endl;
    }
    return 0;
}

static int rexe_help_with_response(int argc, const char* argv[], std::string &response) {
    return rexe_help(argc, argv);
}

static int rexe_register_executable(int argc, const char* argv[], std::string& response)
{
    int rv = 0;
    u64 user_id = 0;
    u64 version_num = 0;

    if (argc < 5) {
        rexe_help(argc, argv);
        return -1;
    }

    rv = getUserIdBasic(&user_id);
    if (rv != 0) {
        LOG_ERROR("Failed to get user_id.");
        return -1;
    }

    version_num = atoi(argv[4]);
    return rexe_register_executable(user_id, argv[1], argv[2], argv[3], version_num, response);
}

int rexe_register_executable(u64 user_id, std::string app_key, std::string executable_name, std::string absolute_path, u64 version_num, std::string& response)
{
    int rv = 0;
    {
        ccd::RegisterRemoteExecutableInput input;
        input.set_user_id(user_id);
        input.set_app_key(app_key);
        input.mutable_remote_executable_info()->set_name(executable_name);
        input.mutable_remote_executable_info()->set_absolute_path(absolute_path);
        input.mutable_remote_executable_info()->set_version_num(version_num);
        rv = CCDIRegisterRemoteExecutable(input);
        if (rv < 0) {
            LOG_ERROR("Failed to register executable: %d", rv);
            goto end;
        }
        LOG_ALWAYS("Absolute Path: %s", absolute_path.c_str());
        LOG_ALWAYS("Executable Name: %s", executable_name.c_str());
        LOG_ALWAYS("App Key: %s", app_key.c_str());
        LOG_ALWAYS("Version Number: "FMTu64, version_num);
    }
end:
    return rv;
}

static int rexe_unregister_executable(int argc, const char* argv[], std::string& response)
{
    int rv = 0;
    u64 user_id = 0;

    if (argc < 3) {
        rexe_help(argc, argv);
        return -1;
    }

    rv = getUserIdBasic(&user_id);
    if (rv != 0) {
        LOG_ERROR("Failed to get user_id.");
        return -1;
    }

    return rexe_unregister_executable(user_id, argv[1], argv[2], response);
}

int rexe_unregister_executable(u64 user_id, std::string app_key, std::string executable_name, std::string& response)
{
    int rv = 0;
    {
        ccd::UnregisterRemoteExecutableInput input;
        input.set_user_id(user_id);
        input.set_app_key(app_key);
        input.set_remote_executable_name(executable_name);
        rv = CCDIUnregisterRemoteExecutable(input);
        if (rv < 0) {
            LOG_ERROR("Failed to unregister executable: %d", rv);
            goto end;
        }
        LOG_ALWAYS("Executable Name: %s", executable_name.c_str());
        LOG_ALWAYS("App Key: %s", app_key.c_str());
    }
end:
    return rv;
}

static int rexe_list_registered_executable(int argc, const char* argv[], std::string& response)
{
    int rv = 0;
    u64 user_id = 0;

    if (argc < 2) {
        rexe_help(argc, argv);
        return -1;
    }

    rv = getUserIdBasic(&user_id);
    if (rv != 0) {
        LOG_ERROR("Failed to get user_id.");
        return -1;
    }

    return rexe_list_registered_executable(user_id, argv[1], response);
}

int rexe_list_registered_executable(u64 user_id, std::string app_key, std::string& response)
{
    int rv = 0;
    {
        ccd::ListRegisteredRemoteExecutablesInput input;
        ccd::ListRegisteredRemoteExecutablesOutput output;
        input.set_user_id(user_id);
        input.set_app_key(app_key);
        rv = CCDIListRegisteredRemoteExecutables(input, output);
        if (rv < 0) {
            LOG_ERROR("Failed to list registered executables: %d", rv);
            goto end;
        }

        for (int i = 0; i < output.registered_remote_executables_size(); i++) {
            LOG_ALWAYS("[%d] Executable Name: %s", i, output.registered_remote_executables(i).name().c_str());
            LOG_ALWAYS("[%d] Absolute Path: %s", i, output.registered_remote_executables(i).absolute_path().c_str());
            LOG_ALWAYS("[%d] App Key: %s", i, app_key.c_str());
            LOG_ALWAYS("[%d] Version Number: "FMTu64, i, output.registered_remote_executables(i).version_num());
        }
    }
end:
    return rv;
}

static int rexe_execute(int argc, const char* argv[], std::string& response)
{
    int rv = 0;
    u64 user_id = 0;

    if (argc < 2) {
        rexe_help(argc, argv);
        return -1;
    }

    rv = getUserIdBasic(&user_id);
    if (rv != 0) {
        LOG_ERROR("Failed to get user_id.");
        return -1;
    }

    return rexe_execute(user_id, argv[1], argv[2], argv[3], argv[4], argv[5], response);
}

int rexe_execute(u64 user_id, std::string device_id, std::string app_key, std::string executable_name, std::string min_version_num, std::string request_body, std::string& response)
{
    int rv = 0;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::string url;
    HttpAgent* agent = NULL;

    agent = getHttpAgent();
    if (!agent) {
        LOG_ERROR("Fail to get http agent");
        rv = -1;
        goto end;
    }

    rv = getLocalHttpInfo(user_id, base_url, service_ticket, session_handle);
    if (rv < 0) {
        LOG_ERROR("getLocalHttpInfo returned %d.", rv);
        goto end;
    }

    {
        char *reqUserId = VPLString_MallocAndSprintf(RF_HEADER_USERID": "FMTu64, user_id);
        if (reqUserId == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            rv = -1;
            goto end;
        }
        char *reqHandle = VPLString_MallocAndSprintf(RF_HEADER_SESSION_HANDLE": %s", session_handle.c_str());
        if (reqHandle == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            free(reqUserId);
            rv = -1;
            goto end;
        }
        char *reqTicket = VPLString_MallocAndSprintf(RF_HEADER_SERVICE_TICKET": %s", service_ticket.c_str());
        if (reqTicket == NULL) {
            LOG_ERROR("unable to allocate memory for header");
            free(reqUserId);
            free(reqHandle);
            rv = -1;
            goto end;
        }

        const char* headers[] = {
            reqUserId,
            reqHandle,
            reqTicket,
        };

        std::ostringstream oss;
        oss << base_url << "/rexe/execute/"
                        << url_encoding(device_id) << "/"
                        << url_encoding(executable_name) << "/"
                        << url_encoding(app_key) << "/"
                        << url_encoding(min_version_num);

        url = oss.str();

        LOG_ALWAYS("URL: %s", url.c_str());
        LOG_ALWAYS("Request Body: %s", request_body.c_str());

        rv = agent->post(url, headers, 3, request_body.c_str(), request_body.length(), NULL, response);

        // free header resources
        free(reqUserId);
        free(reqHandle);
        free(reqTicket);



        if (rv < 0) {
            LOG_ERROR("agent->post(\"%s\") returned %d", url.c_str(), rv);
            LOG_ERROR("Status Code: %d", agent->getlastHttpStatusCode());
            LOG_ERROR("Response: %s", response.c_str());
            goto end;
        }
        LOG_ALWAYS("Status Code: %d", agent->getlastHttpStatusCode());
        LOG_ALWAYS("Response: %s", response.c_str());
    }

end:
    return rv;
}

class RemoteExecutableDispatchTable {
public:
    RemoteExecutableDispatchTable() {
        cmds["Register"]          = rexe_register_executable;
        cmds["Unregister"]        = rexe_unregister_executable;
        cmds["ListExecutable"]    = rexe_list_registered_executable;
        cmds["Execute"]           = rexe_execute;
        cmds["Help"]              = rexe_help_with_response;
    }
    std::map<std::string, rexe_subcmd_fn> cmds;
};

static RemoteExecutableDispatchTable rexeDispatchTable;

static int rexe_dispatch(std::map<std::string, rexe_subcmd_fn> &cmdmap, int argc, const char* argv[], std::string &response)
{
    int rv = 0;

    if (cmdmap.find(argv[0]) != cmdmap.end()) {
        rv = cmdmap[argv[0]](argc, &argv[0], response);
    } else {
        LOG_ERROR("Command %s not supported", argv[0]);
        rv = -1;
    }
    return rv;
}


int dispatch_rexe_test_cmd(int argc, const char* argv[])
{
    std::string response;
    if (argc <= 1) {
        return rexe_help(argc, argv);
    } else {
        return rexe_dispatch(rexeDispatchTable.cmds, argc - 1, &argv[1], response);
    }
}
