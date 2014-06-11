#include "virtdrive_cmds.hpp"
#include "common_utils.hpp"
#include "dx_common.h"
#include <ccdi.hpp>

#include <log.h>
#include <cerrno>

#include <iostream>
#include <string>
#include <map>
#include <cstring>


//--------------------------------------------------
// dispatcher

static int virtdrive_help(int argc, const char* argv[])
{
    // argv[0] == "VirtDrive" if called from dispatch_virtdrive_cmd()
    // Otherwise, called from virtdrive_*() handler function

    bool printAll = strcmp(argv[0], "VirtDrive") == 0 || strcmp(argv[0], "Help") == 0;

    if (printAll || strcmp(argv[0], "AccessInfo") == 0)
        std::cout << "VirtDrive AccessInfo <device_id>" << std::endl;

    return 0;
}

static int virtdrive_getAccessInfo(int argc, const char* argv[])
{
    int rv = 0;
    ccd::VirtDriveGetAccessInfoInput req;
    ccd::VirtDriveGetAccessInfoOutput resp;
    u64 device_id;

    if (argc < 2) {
        virtdrive_help(argc, argv);
        rv = -1;
        goto end;
    }

    device_id = strtoull(argv[1], NULL, 0);
    if ( (device_id == ULONG_MAX) && (errno != 0) ) {
        std::cout << "Invalid device id: " << argv[1] << std::endl;
        rv = -1;
        goto end;
    }
    req.set_device_id(device_id);

    rv = CCDIVirtDriveGetAccessInfo(req, resp);
    if ( rv != CCD_OK ) {
        LOG_ERROR("CCDIVirtDriveGetAccessInfo() - %d", rv);
        goto end;
    }

    // convert the ticket to a printable value
    {
        std::string ticket;
        const char* data;
        char tmp[3];

        data = resp.access_ticket().data();
        for( unsigned int i = 0 ; i < resp.access_ticket().size() ; i++ ) {
            snprintf(tmp, sizeof(tmp), "%02x", (u8)data[i]);
            ticket += tmp;
        }

        printf("VirtDrive Access Info: handle "FMTx64" ticket %s\n",
            resp.access_handle(), ticket.c_str());
    }

 end:
    return rv;
}

class VirtdriveCmdDispatchTable {
public:
    VirtdriveCmdDispatchTable() {
        cmds["AccessInfo"] = virtdrive_getAccessInfo;
        cmds["Help"]       = virtdrive_help;
    }
    std::map<std::string, subcmd_fn> cmds;
};

static VirtdriveCmdDispatchTable virtdriveCmdDispatchTable;

int dispatch_virtdrive_cmd(int argc, const char* argv[])
{
    if (argc <= 1)
        return virtdrive_help(argc, argv);
    else
        return dispatch(virtdriveCmdDispatchTable.cmds, argc - 1, &argv[1]);
}

