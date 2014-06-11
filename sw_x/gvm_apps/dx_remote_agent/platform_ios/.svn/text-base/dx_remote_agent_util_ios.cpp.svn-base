#include <vplex_file.h>
#include <vpl_fs.h>
#include "dx_remote_agent_util.h"
#include "dx_remote_agent_util_ios.h"
#include <string>
#include "ccd_core.h"

unsigned short getFreePort()
{
    return 24000;
}

int recordPortUsed(unsigned short port)
{
    // no-op on this platform
    return 0;
}

int startccd(const char* titleId)
{
    std::string ccFolder;
    get_cc_folder(ccFolder);
    return CCDStart("dx_remote_agent", ccFolder.c_str(), NULL, titleId);
}

int startccd(int testInstanceNum, const char* titleId) {
    // Due to the sandboxed model, testInstanceNum shouldn't be needed for iOS.
    return -1;
}

int stopccd()
{
    return 0;
}
