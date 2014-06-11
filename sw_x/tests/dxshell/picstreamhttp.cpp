//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.

#define _CRT_SECURE_NO_WARNINGS // for cross-platform getenv

#include <ccdi.hpp>
#include <vplex_file.h>
#include <vplex_strings.h>
#include <vpl_fs.h>
#include <vplu_types.h>
#include <vpl_conv.h>
#include "gvm_file_utils.hpp"
#include "gvm_misc_utils.h"

#include <iostream>
#include <sstream>
#include <string>
#include <deque>
#include <fstream>
#include <cstdio>

#include <log.h>
#include <vpl_plat.h>

#include "dx_common.h"
#include "ccd_utils.hpp"
#include "common_utils.hpp"
#include "picstreamhttp.hpp"
#include "EventQueue.hpp"
#include "HttpAgent.hpp"
#include "TargetLocalDevice.hpp"
#include "cJSON2.h"
#include "scopeguard.hpp"

#define RF_HEADER_USERID         "x-ac-userId"
#define RF_HEADER_SESSION_HANDLE "x-ac-sessionHandle"
#define RF_HEADER_SERVICE_TICKET "x-ac-serviceTicket"

int picstreamhttp_help(int argc, const char* argv[]);
int picstreamhttp_help(int argc, const char* argv[])
{
    bool printAll = strcmp(argv[0], "PicStreamHttp") == 0 || strcmp(argv[0], "Help") == 0;

    if (printAll || strcmp(argv[0], "Delete") == 0)
        std::cout << "PicStreamHttp Delete Title CompId" << std::endl;
    if (printAll || strcmp(argv[0], "Download") == 0)
        std::cout << "PicStreamHttp Download Title CompId Type Target_path" << std::endl;
    if (printAll || strcmp(argv[0], "GetFileInfo") == 0)
        std::cout << "PicStreamHttp GetFileInfo Album_Name Type [Title]" << std::endl;
   
    return 0;
}

static int picstreamhttp_help_with_response(int argc, const char* argv[], std::string &response) {
    return picstreamhttp_help(argc, argv);
}

static int picstreamhttp_get_fileinfo(int argc, const char* argv[], std::string& response)
{
    if (argc < 3) {
        picstreamhttp_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }

    if(argc == 3)
        return picstream_get_fileinfo(userId, "", argv[1], argv[2], response);
    else
        return picstream_get_fileinfo(userId, argv[3], argv[1], argv[2], response);
}

static int picstreamhttp_get_image(int argc, const char* argv[], std::string& response)
{
    if (argc < 5) {
        picstreamhttp_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }

    return picstream_get_image(userId, argv[1], argv[2], argv[3], argv[4], response);
}

static int picstreamhttp_delete(int argc, const char* argv[], std::string& response)
{

   if (argc < 3) {
        picstreamhttp_help(argc, argv);
        return -1;
    }

    u64 userId = 0;
    int rv = getUserIdBasic(&userId);
    if (rv != 0) {
        LOG_ERROR("Failed to get userId.");
        return -1;
    }

    return picstream_delete(userId, argv[1], argv[2], "", response);

    return 0;
}



///////////////////////////// End of Functions for Command Dispatcher /////////////////////////////////////
int picstream_get_image(u64 userId, const std::string &title, const std::string &component_id, const std::string type, const std::string outputFile, std::string& response)
{
    int rv = VPL_OK;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::string url;
    std::string uri_path;
    HttpAgent* agent = NULL;

    agent = getHttpAgent();
    if (!agent) {
        LOG_ERROR("Fail to get http agent");
        rv = -1;
        goto end;
    }

    rv = getLocalHttpInfo(userId, base_url, service_ticket, session_handle);
    if (rv < 0) {
        LOG_ERROR("getLocalHttpInfo returned %d.", rv);
        goto end;
    }

    {
        char *reqUserId = VPLString_MallocAndSprintf(RF_HEADER_USERID": "FMTu64, userId);
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

        url = base_url + "/picstream/file/" + title + "?compId=" + component_id + "&type=" + type;

        rv = agent->get_extended(url, 0, 0, headers, 3, outputFile.c_str());
        LOG_ALWAYS("download rv = %d", rv);
        
        // free header resources
        free(reqUserId);
        free(reqHandle);
        free(reqTicket);

        if (rv < 0) {
            LOG_ERROR("agent->get_extended(\"%s\") returned %d", url.c_str(), rv);
            response = "Download Failed!";
        } else {
            response = "Download Done!";
        }
    }

    LOG_ALWAYS("Status Code: %d", agent->getlastHttpStatusCode());
    LOG_ALWAYS("Result: %s", response.c_str());
end:
    return rv;
}

int picstream_get_fileinfo(u64 userId, const std::string &title, const std::string &albumname, const std::string type, std::string& response)
{
    int rv = VPL_OK;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::string url;
    std::string uri_path;
    HttpAgent* agent = NULL;

    agent = getHttpAgent();
    if (!agent) {
        LOG_ERROR("Fail to get http agent");
        rv = -1;
        goto end;
    }

    rv = getLocalHttpInfo(userId, base_url, service_ticket, session_handle);
    if (rv < 0) {
        LOG_ERROR("getLocalHttpInfo returned %d.", rv);
        goto end;
    }

    {
        char *reqUserId = VPLString_MallocAndSprintf(RF_HEADER_USERID": "FMTu64, userId);
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

        url = base_url + "/picstream/fileinfo/" + title + "?albumname=" + albumname + "&type=" + type;

        rv = agent->get_response_back(url, 0, 0, headers, 3, response);

        // free header resources
        free(reqUserId);
        free(reqHandle);
        free(reqTicket);

        if (rv < 0) {
            LOG_ERROR("agent->get_response_back(\"%s\") returned %d", url.c_str(), rv);
            goto end;
        }
    }

    LOG_ALWAYS("Status Code: %d", agent->getlastHttpStatusCode());
    LOG_ALWAYS("Result: %s", response.c_str());
end:
    return rv;
}

int picstream_delete(u64 userId,
    const std::string &title,
    const std::string &component_id,
    const std::string &revision,
    std::string& response)
{
    int rv = 0;
    std::string base_url;
    std::string service_ticket;
    std::string session_handle;
    std::string temp;
    std::map<std::string, std::string> payload;
    std::string url;
    HttpAgent *agent = NULL;

    agent = getHttpAgent();
    if (!agent) {
        LOG_ERROR("Fail to get http agent");
        rv = -1;
        goto end;
    }

    rv = getLocalHttpInfo(userId, base_url, service_ticket, session_handle);
    if (rv < 0) {
        LOG_ERROR("getLocalHttpInfo returned %d.", rv);
        goto end;
    }

    {
        // XXX Has to add the headers line by line instead of altogether.
        char *reqUserId = VPLString_MallocAndSprintf(RF_HEADER_USERID": "FMTu64, userId);
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

        url = base_url + "/picstream/file/" + title + "?compId="+component_id;

        if(revision != "")
            url += ("&revision="+revision);

        rv = agent->del(url, headers, 3, NULL, 0, NULL, response);

        // free header resources
        free(reqUserId);
        free(reqHandle);
        free(reqTicket);

        if (rv < 0) {
            LOG_ERROR("agent->get_response_back(\"%s\") returned %d", url.c_str(), rv);
            goto end;
        }

    }
    LOG_ALWAYS("Delete pass. response: %s", response.c_str());

    //not expect any json output
    //rv = check_json_format(response);

end:
    return rv;
}

class PicStreamHttpDispatchTable {
public:
    PicStreamHttpDispatchTable() {
        cmds["Delete"]        = picstreamhttp_delete;
        cmds["Download"]      = picstreamhttp_get_image;
        cmds["GetFileInfo"]   = picstreamhttp_get_fileinfo;
        cmds["Help"]          = picstreamhttp_help_with_response;
    }
    std::map<std::string, picstream_subcmd_fn> cmds;
};

static PicStreamHttpDispatchTable picstreamHttpDispatchTable;

static int picstream_dispatch(std::map<std::string, picstream_subcmd_fn> &cmdmap, int argc, const char* argv[], std::string &response)
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

int dispatch_picstreamhttp_cmd(int argc, const char* argv[])
{
    std::string response;

    if (argc <= 1)
        return picstreamhttp_help(argc, argv);
    else
        return picstream_dispatch(picstreamHttpDispatchTable.cmds, argc - 1, &argv[1], response);
}

int dispatch_picstreamhttp_cmd_with_response(int argc, const char* argv[], std::string& response)
{
    if (argc <= 1)
        return picstreamhttp_help(argc, argv);
    else
        return picstream_dispatch(picstreamHttpDispatchTable.cmds, argc - 1, &argv[1], response);
}

