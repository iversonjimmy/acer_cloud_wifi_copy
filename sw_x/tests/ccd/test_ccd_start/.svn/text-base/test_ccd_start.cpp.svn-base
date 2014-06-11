//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

// This is used by buildbot to remotely launch ccd process on a Windows target machine.
// The CCD app local path is specified through the input parameters

#include <ccdi.hpp>

#include <vpl_time.h>
#include <vplex_file.h>
#include <vpl_fs.h>
#include <vplu_types.h>
#include <vplex_plat.h>

#include <log.h>

PROCESS_INFORMATION pi;
HANDLE g_hChildStd_OUT_Rd = NULL;
HANDLE g_hChildStd_OUT_Wr = NULL;

static int setDebugLevel(int level)
{
    for (int i = 0; i < LOG_NUM_LEVELS; i++) {
        if (i >= level) { 
            LOG_ENABLE_LEVEL((LOGLevel)i);
        } else {
            LOG_DISABLE_LEVEL((LOGLevel)i);
        }
    }

    return 0;
}

static int checkRunningCcd()
{
    // Will take 5 seconds...
    setDebugLevel(LOG_LEVEL_CRITICAL);  // Avoid confusing log where it gives error if ccd is not detected yet
    ccd::GetSystemStateInput request;
    request.set_get_players(true);
    ccd::GetSystemStateOutput response;
    int rv;
    rv = CCDIGetSystemState(request, response);
    setDebugLevel(LOG_LEVEL_ERROR);
    if (rv == CCD_OK) {
        LOG_ERROR("CCD is already present!\n");
    }
    return (rv == 0) ? -1 : 0;  // Only a success if CCDIGetSystemState failed.
}

int main(int argc, char* argv[])
{
    STARTUPINFO si;
    wchar_t *ccd_dir = NULL;
    std::string appDataPathStr;
    std::wstring progWStr, appDataPathWStr, cmdLineWStr;
    SECURITY_ATTRIBUTES saAttr; 
    int rv = 0;

    if (argc < 2) {
        LOG_ERROR("Usage: %s <appDataPath>", argv[0]);
        return -1;
    }

    rv = checkRunningCcd();
    if (rv != 0) {
        LOG_ERROR("An active CCD detected. Please kill the CCD first"); 
        return -1;
    }

    appDataPathStr.assign(argv[1]);

    {
        std::wstring tmp(appDataPathStr.length(), L'');
        std::copy(appDataPathStr.begin(), appDataPathStr.end(), tmp.begin());
        appDataPathWStr = tmp;
    }

    wprintf(L"AppData Path: %s\n", appDataPathWStr.c_str());

    progWStr.assign(L"./CCD.exe");
    cmdLineWStr = progWStr + L" \"" + appDataPathWStr + L"\"";

    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
    saAttr.bInheritHandle = TRUE; 
    saAttr.lpSecurityDescriptor = NULL; 

    // Create a pipe for the child process's STDOUT. 
    if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0)) {
        LOG_ERROR("Fail StdoutRd CreatePipe"); 
    }
    if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0)) {
        LOG_ERROR("Fail Stdout SetHandleInformation");
    }
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.hStdOutput = g_hChildStd_OUT_Wr;
    si.hStdError = g_hChildStd_OUT_Wr;
    si.dwFlags |= STARTF_USESTDHANDLES;
    ZeroMemory(&pi, sizeof(pi));

    DWORD processCreationFlags = 0;
    processCreationFlags = DETACHED_PROCESS;

    if (!CreateProcess(NULL,
                       (LPTSTR) cmdLineWStr.c_str(),
                       NULL,
                       NULL,
                       FALSE,
                       processCreationFlags,
                       NULL,
                       NULL,
                       &si,
                       &pi))
    {
        LOG_ERROR("CreateProcess for CCD failed (%d).", GetLastError());
        LOG_ERROR("Please make sure CCD.exe is in current folder or set");
        LOG_ERROR("DX_CCD_EXE_PATH to the directory that contains CCD.exe");
        return -1;
    }

    return 0;
}
