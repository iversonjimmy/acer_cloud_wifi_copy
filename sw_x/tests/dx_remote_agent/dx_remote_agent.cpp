//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include "DxRemoteAgentListener.hpp"

#ifdef VPL_PLAT_IS_WINRT
#include "dx_remote_agent_util_winrt.h"
#elif defined(IOS)
#include "dx_remote_agent_util_ios.h"
#elif defined (WIN32)
#include <dx_remote_agent_util_win.h>
#include "log.h"
#elif defined (LINUX)
#include <dx_remote_agent_util_linux.h>
#include "log.h"
#else
#endif

#include <string>

#if defined(VPL_PLAT_IS_WINRT) || defined(IOS)
#else
int main(int argc, char *argv[])
{
#  if defined (WIN32)
    std::string currDir;
    if (argc > 1) {
        set_dx_ccd_name(std::string(argv[1]));
    }

    getCurDir(currDir);
    LOGInit("dx_remote_agent", currDir.c_str());

    _shutdownprocessW(std::wstring(L"ccd.exe"));
#  elif defined (LINUX)
    if (argc > 1) {
        set_dx_ccd_name(std::string(argv[1]));
    }
    
    LOGInit("dx_remote_agent", NULL);
    //TODO: _shutdownprocessW(std::wstring(L"ccd.exe"));

#  endif

    DxRemoteAgentListener myListener;
    myListener.Run();

    return 0;
}
#endif
