//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#define _CRT_SECURE_NO_WARNINGS 1

#include "stdafx.h"
#include <vpl_plat.h>
#include <vpl_fs.h>
#include "ccd_utils.hpp"

#include <iostream>
#include <sstream>
#include <cstdio>
#include <cerrno>
#include <csignal>
#include <string>
#include <vector>
#include <set>

#include <windows.h>
#include <Tlhelp32.h>
#include <log.h>
#include "dx_common.h"


PROCESS_INFORMATION pi;
HANDLE g_hChildStd_OUT_Rd = NULL;
HANDLE g_hChildStd_OUT_Wr = NULL;

static int _getLocalAppDataWPath(std::wstring &wpath)
{
    int rc = 0;
    wchar_t *localAppDataWPath = NULL;

    rc = _VPLFS__GetLocalAppDataWPath(&localAppDataWPath);
    if (rc != VPL_OK) {
        LOG_ERROR("Failed to determine LocalAppData path");
        goto out;
    }

    wpath.assign(localAppDataWPath);

out:
    if (localAppDataWPath != NULL)
        free(localAppDataWPath);
    return rc;
}

static int _getDxRootWPath(std::wstring &rootWPath)
{
    int rc = 0;
    std::wstring wpathbuf;

    rc = _getLocalAppDataWPath(wpathbuf);
    if (rc != 0)
        goto out;

    wpathbuf.append(L"\\dxshell");

    rootWPath.assign(wpathbuf);

out:
    return rc;
}

int getDxRootPath(std::string &rootPath)
{
    int rc = 0;
    std::wstring rootWPath;
    char *utf8 = NULL;

    rc = _getDxRootWPath(rootWPath);
    if (rc != 0)
        goto out;

    rc = _VPL__wstring_to_utf8_alloc(rootWPath.c_str(), &utf8);
    if (rc != VPL_OK)
        goto out;

    rootPath.assign(utf8);

out:
    if (utf8 != NULL)
        free(utf8);
    return 0;
}

static int _createDirectoryIfMissing(std::wstring wpath)
{
    int rc = 0;

    if (CreateDirectory(wpath.c_str(), NULL) == 0) {  // error
        if (GetLastError() != ERROR_ALREADY_EXISTS) {
            rc = VPLError_XlatWinErrno(GetLastError());
            LOG_ERROR("Fail to create directory %s, rc = %d", wpath.c_str(), rc);
            goto out;
        }
    }

out:
    return rc;
}

static int _getCcdAppDataWPath(std::wstring &wpath)
{
    int rc = 0;
    std::wstring wpathbuf;

    rc = _getLocalAppDataWPath(wpathbuf);
    if (rc != 0)
        goto out;

    wpathbuf.append(L"\\iGware");
    rc = _createDirectoryIfMissing(wpathbuf);
    if (rc != 0)
        goto out;

    wpathbuf.append(L"\\SyncAgent");
    rc = _createDirectoryIfMissing(wpathbuf);
    if (rc != 0)
        goto out;

    wpath.assign(wpathbuf);

out:
    return rc;
}

int getCcdAppDataPath(std::string &path)
{
    int rc = 0;
    std::wstring wpath;
    char *utf8 = NULL;

    rc = _getCcdAppDataWPath(wpath);
    if (rc != 0)
        goto out;

    rc = _VPL__wstring_to_utf8_alloc(wpath.c_str(), &utf8);
    if (rc != 0)
        goto out;

    path.assign(utf8);

    if (testInstanceNum) {
        std::ostringstream instanceSuffix;
        instanceSuffix << "_" << testInstanceNum;
        path += instanceSuffix.str();
    }

out:
    if (utf8 != NULL)
        free(utf8);
    return rc;
}

int startCcd(const char* titleId)
{
    STARTUPINFO si;
    wchar_t *ccd_dir = NULL;
    std::wstring progPathStr, appDataPathStr, commandLineStr;
    SECURITY_ATTRIBUTES saAttr; 
    int rv = 0;

    rv = checkRunningCcd();
    if (rv != 0) {
        LOG_ERROR("An active CCD detected. Please kill the CCD first"); 
        return -1;
    }

    ccd_dir = _wgetenv(L"DX_CCD_EXE_PATH");
    if (ccd_dir != NULL) {
        progPathStr.assign(ccd_dir);
    } else {
        progPathStr.assign(L".");
    }
    progPathStr.append(L"\\ccd.exe");

    // DEFAULT_CCD_APP_DATA_PATH
    rv = _getCcdAppDataWPath(appDataPathStr);
    if (rv != 0) {
        LOG_ERROR("Fail to get ccd app data path: rv %d", rv);
        return rv;
    }

    if (testInstanceNum) {
        std::wostringstream instanceSuffix;
        instanceSuffix << L"_" << testInstanceNum;
        appDataPathStr += instanceSuffix.str();
    }

    std::wstring w_titleId;
    if (titleId) {
        std::string titleIdIdString(titleId);
        w_titleId.assign(titleIdIdString.begin(), titleIdIdString.end());
    }

    wprintf(L"CCD Executable Path: %s\n", progPathStr.c_str());
    wprintf(L"AppData Path: %s\n", appDataPathStr.c_str());

    commandLineStr = progPathStr + L" \"" + appDataPathStr + L"\"" + L" \"\"" + L" \"\"" + L" \"\"" + L" \"\"" + L" \"" + w_titleId + L"\"";

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

    // Issue 1: It is important to detach CCD from the current console window, otherwise it will be
    // terminated when the console window is closed.
    // Issue 2: An unfortunate consequence of DETACHED_PROCESS is that certain types of crashes cause
    // the process to immediately close instead of popping up the "just-in-time debugger" dialog.
    // Using CREATE_NEW_PROCESS_GROUP doesn't help with issue 1.
    // Using CREATE_NO_WINDOW doesn't help with issue 2.
    // Using CREATE_NEW_CONSOLE might be good enough for developers, so allow setting it via the
    // environment variable, DX_CREATE_NEW_CONSOLE.
    DWORD processCreationFlags;
    if (_wgetenv(L"DX_CREATE_NEW_CONSOLE") != NULL) {
        processCreationFlags = CREATE_NEW_CONSOLE;
    } else {
        processCreationFlags = DETACHED_PROCESS;
    }

    wprintf(L"CreateProcess(%s)\n", commandLineStr.c_str());
    if (!CreateProcess(NULL,
                       (LPTSTR) commandLineStr.c_str(),
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

int shutdownCcd()
{
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);
    std::wstring ccdName = L"ccd.exe";

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    if (Process32First(snapshot, &entry) == TRUE) {
        while (Process32Next(snapshot, &entry) == TRUE) {
            if (_wcsicmp(entry.szExeFile, ccdName.c_str()) == 0) {  
                HANDLE hHandle;
                DWORD dwExitCode = 0;
                hHandle = ::OpenProcess(PROCESS_ALL_ACCESS, 0, entry.th32ProcessID);
                      
                ::GetExitCodeProcess(hHandle,&dwExitCode);
                ::TerminateProcess(hHandle,dwExitCode);
            }
        }
    }
    CloseHandle(snapshot);
    return 0;
}
